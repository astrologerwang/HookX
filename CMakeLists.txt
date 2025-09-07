cmake_minimum_required(VERSION 3.10)

project(HookX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(fmt CONFIG REQUIRED)

# Find detours
find_path(DETOURS_INCLUDE_DIRS "detours/detours.h")
find_library(DETOURS_LIBRARY detours REQUIRED)

# Main hook executable (injector)
add_executable(HookX hookx.cpp)

target_include_directories(HookX PRIVATE ${DETOURS_INCLUDE_DIRS})
target_link_libraries(HookX PRIVATE 
    fmt::fmt 
    ${DETOURS_LIBRARY}
    xinput
)

# Hook DLL
add_library(hook_dll SHARED hook_dll.cpp)

target_include_directories(hook_dll PRIVATE ${DETOURS_INCLUDE_DIRS})
target_link_libraries(hook_dll PRIVATE 
    ${DETOURS_LIBRARY}
    xinput
)

# Broadcast message sender (test application)
add_executable(BroadcastSender broadcast_sender.cpp)

# Test client executable
add_executable(TestClient test_client.cpp)
target_link_libraries(TestClient PRIVATE xinput)