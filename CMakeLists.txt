cmake_minimum_required(VERSION 4.0)
project(IFJcode25 C)

set(CMAKE_C_STANDARD 23)

add_executable(IFJcode25 main.c
        src/lexer.c
        src/lexer.h
        src/token.h
        src/error.h
        src/string_builder.c
        src/string_builder.h
        src/parser.c
        src/parser.h
        src/list.c
        src/list.h
        src/codegen.c
        src/codegen.h)
