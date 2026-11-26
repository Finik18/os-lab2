#include "proclaunch.h"
#include <cstring>
#include <cerrno>
#include <string>

#ifdef _WIN32
#  include <windows.h>
#else
#  include <spawn.h>
#  include <sys/wait.h>
#  include <csignal>

extern char **environ;
#endif

extern "C" int pl_start(const char *const argv[], pl_process *proc) {
    if (!argv || !argv[0] || !proc) { errno = EINVAL; return -1; }

#ifdef _WIN32
    std::string cmd;
    for (int i = 0; argv[i]; ++i) {
        if (i) cmd += ' ';
        const char *a = argv[i];
        bool need_quotes = (*a == '\0') || std::strpbrk(a, " \t\"") != nullptr;
        if (!need_quotes) { cmd += a; continue; }
        cmd += '"';
        size_t backslashes = 0;
        for (const char *p = a; *p; ++p) {
            if (*p == '\\') { ++backslashes; cmd += '\\'; }
            else if (*p == '"') {
                cmd.append(backslashes, '\\');
                cmd += "\\\"";
                backslashes = 0;
            } else { backslashes = 0; cmd += *p; }
        }
        cmd.append(backslashes, '\\');
        cmd += '"';
    }

    STARTUPINFOA si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof si;

    if (!CreateProcessA(nullptr, cmd.data(), nullptr, nullptr,
                        FALSE, 0, nullptr, nullptr, &si, &pi))
        return -1;

    CloseHandle(pi.hThread);
    proc->handle = reinterpret_cast<intptr_t>(pi.hProcess);
    proc->pid    = static_cast<long>(pi.dwProcessId);
    return 0;

#else
    pid_t pid;
    int rc = posix_spawnp(&pid, argv[0], nullptr, nullptr,
                          const_cast<char* const*>(argv), environ);
    if (rc != 0) { errno = rc; return -1; }
    proc->pid    = static_cast<long>(pid);
    proc->handle = 0;
    return 0;
#endif
}

extern "C" int pl_wait(pl_process *proc, int *exit_code) {
    if (!proc) return -1;

#ifdef _WIN32
    HANDLE h = reinterpret_cast<HANDLE>(proc->handle);
    if (!h) return -1;
    if (WaitForSingleObject(h, INFINITE) != WAIT_OBJECT_0) return -1;
    DWORD code = 0;
    if (!GetExitCodeProcess(h, &code)) return -1;
    CloseHandle(h);
    proc->handle = 0;
    if (exit_code) *exit_code = static_cast<int>(code);
    return 0;
#else
    int status = 0;
    pid_t r;
    do { r = waitpid(static_cast<pid_t>(proc->pid), &status, 0); }
    while (r < 0 && errno == EINTR);
    if (r < 0) return -1;

    int code = 0;
    if (WIFEXITED(status))        code = WEXITSTATUS(status);
    else if (WIFSIGNALED(status)) code = 128 + WTERMSIG(status);
    if (exit_code) *exit_code = code;
    proc->pid = 0;
    return 0;
#endif
}

extern "C" int pl_poll(pl_process *proc, int *exit_code) {
    if (!proc) return -1;

#ifdef _WIN32
    HANDLE h = reinterpret_cast<HANDLE>(proc->handle);
    if (!h) return -1;
    DWORD w = WaitForSingleObject(h, 0);
    if (w == WAIT_TIMEOUT) return 0;
    if (w != WAIT_OBJECT_0) return -1;
    DWORD code = 0;
    if (!GetExitCodeProcess(h, &code)) return -1;
    CloseHandle(h);
    proc->handle = 0;
    if (exit_code) *exit_code = static_cast<int>(code);
    return 1;
#else
    int status = 0;
    pid_t r = waitpid(static_cast<pid_t>(proc->pid), &status, WNOHANG);
    if (r < 0 && errno != EINTR) return -1;
    if (r == 0) return 0;

    int code = 0;
    if (WIFEXITED(status))        code = WEXITSTATUS(status);
    else if (WIFSIGNALED(status)) code = 128 + WTERMSIG(status);
    if (exit_code) *exit_code = code;
    proc->pid = 0;
    return 1;
#endif
}