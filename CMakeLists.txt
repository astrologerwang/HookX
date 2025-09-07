cmake_minimum_required(VERSION 3.10)

project(HookX)

find_package(fmt CONFIG REQUIRED)

# Find detours
find_path(DETOURS_INCLUDE_DIRS "detours/detours.h")
find_library(DETOURS_LIBRARY detours REQUIRED)

# Main hook executable
add_executable(HookX hookx.cpp)

target_include_directories(HookX PRIVATE ${DETOURS_INCLUDE_DIRS})
target_link_libraries(HookX PRIVATE 
    fmt::fmt 
    ${DETOURS_LIBRARY}
    xinput
)

# Test client executable
add_executable(TestClient test_client.cpp)
target_link_libraries(TestClient PRIVATE xinput)