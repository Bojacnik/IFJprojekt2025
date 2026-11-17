cmake_minimum_required(VERSION 3.16)
project(IFJcode25 C)

# The graded build is the Makefile (gcc on merlin). This file is only for IDEs.
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -pedantic)

file(GLOB SOURCES src/*.c)
add_executable(IFJcode25 main.c ${SOURCES})
