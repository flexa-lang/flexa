cmake_minimum_required(VERSION 3.10)
project(flexa)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB SOURCES "src/*.cpp")
file(GLOB HEADERS "src/*.hpp")

add_executable(flexa 
    ${SOURCES}
    ${HEADERS}
)

target_include_directories(flexa PRIVATE src/)

target_compile_options(flexa PRIVATE -Wall -Wextra)
