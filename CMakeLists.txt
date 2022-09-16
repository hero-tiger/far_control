cmake_minimum_required(VERSION 3.17)
project(far_cont C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "-static -s")
add_executable(far_cont main.c  server.c client.c sm4.c sm4.h)