cmake_minimum_required(VERSION 3.10)

project(HookX)

find_package(fmt CONFIG REQUIRED)

add_executable(HookX hookx.cpp)

target_link_libraries(HookX PRIVATE fmt::fmt)