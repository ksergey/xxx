cmake_minimum_required(VERSION 4.0)

project(xxx)

# Default build type
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build" FORCE)
endif()

add_subdirectory(3rdparty EXCLUDE_FROM_ALL)
add_subdirectory(code)
add_subdirectory(example)
