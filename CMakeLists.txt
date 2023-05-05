cmake_minimum_required(VERSION 3.25)
project(untitled2)

set(CMAKE_CXX_STANDARD 17)

add_executable(untitled2 main.cpp)

target_compile_options(untitled2 INTERFACE -O0)
