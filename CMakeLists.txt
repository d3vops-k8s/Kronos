cmake_minimum_required(VERSION 3.20)

project(Kronos LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(kronos
    src/main.cpp
    src/parser.cpp
)

target_include_directories(kronos PRIVATE
    include
)