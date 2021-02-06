cmake_minimum_required(VERSION 3.14)

set(PROJECT_NAME b64c)
project(${PROJECT_NAME} LANGUAGES C)

add_library(${PROJECT_NAME} SHARED 
    buffer.c
    decode.c
    encode.c
)

target_include_directories(${PROJECT_NAME} INTERFACE .)
