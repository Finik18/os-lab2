#include "proclaunch.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <thread>
#include <chrono>

#ifdef _WIN32
#  include <windows.h>
#endif

static void sleep_ms(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

static int tests_failed = 0;

static void check(bool ok, const char *name) {
    std::printf("[%s] %s\n", ok ? " OK " : "FAIL", name);
    if (!ok) ++tests_failed;
}

int main(int argc, char **argv) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    if (argc >= 4 && std::strcmp(argv[1], "--child") == 0) {
        sleep_ms(std::atoi(argv[3]));
        return std::atoi(argv[2]);
    }

    const char *self = argv[0];

    {
        const char *args[] = { self, "--child", "42", "1000", nullptr };
        pl_process p;
        int rc = pl_start(args, &p);
        int iterations = 0;
        int code = -1;
        if (rc == 0) {
            int r;
            do {
                sleep_ms(100);
                ++iterations;
                r = pl_poll(&p, &code);
            } while (r == 0 && iterations < 100);
            check(iterations > 3, "test1: parent iterated >3 times while child ran");
            check(r == 1 && code == 42, "test1: exit code == 42");
        } else {
            check(false, "test1: pl_start failed");
        }
    }

    {
        const char *args[] = { self, "--child", "7", "300", nullptr };
        pl_process p;
        int rc = pl_start(args, &p);
        int code = -1;
        if (rc == 0) rc = pl_wait(&p, &code);
        check(rc == 0 && code == 7, "test2: wait returns 7");
    }

    {
        const char *args[] = { self, "--child", "5", "0", "hello \"big\" world\\", nullptr };
        pl_process p;
        int rc = pl_start(args, &p);
        int code = -1;
        if (rc == 0) rc = pl_wait(&p, &code);
        check(rc == 0 && code == 5, "test3: arg with spaces/quotes accepted");
    }

    {
        const char *args[] = { "no_such_program_12345", nullptr };
        pl_process p;
        int rc = pl_start(args, &p);
        check(rc == -1, "test4: non-existent program -> -1");
    }

    {
        const char *args[] = { self, "--child", "300", "0", nullptr };
        pl_process p;
        int rc = pl_start(args, &p);
        int code = -1;
        if (rc == 0) rc = pl_wait(&p, &code);
#ifdef _WIN32
        check(rc == 0 && code == 300, "test5: code 300 (Windows, 32-bit)");
#else
        check(rc == 0 && code == 44, "test5: code 300 -> 44 (Linux, 8-bit)");
#endif
    }

    std::printf("%s\n", tests_failed ? "SOME TESTS FAILED" : "ALL TESTS PASSED");
    return tests_failed ? 1 : 0;
}