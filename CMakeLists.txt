cmake_minimum_required(VERSION 3.25)
project(untitled2)

set(CMAKE_CXX_STANDARD 17)

add_executable(vpv-lab-1 main.cpp)

target_compile_options(vpv-lab-1 INTERFACE -O0)
