cmake_minimum_required(VERSION 3.16)
project(ProcLaunch LANGUAGES CXX)
 
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
 
add_library(proclaunch STATIC src/proclaunch.cpp)
target_include_directories(proclaunch PUBLIC include)
 
add_executable(proctest test/proctest.cpp)
target_link_libraries(proctest PRIVATE proclaunch)
 
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(proclaunch PRIVATE -Wall -Wextra)
    target_compile_options(proctest PRIVATE -Wall -Wextra)
endif()