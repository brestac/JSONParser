cmake_minimum_required(VERSION 3.15)
project(JSONParser CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Werror)

add_executable(desktop-test
    tests/desktop.cpp
)

target_include_directories(desktop-test PRIVATE
    ${CMAKE_SOURCE_DIR}
)
