cmake_minimum_required(VERSION 3.9)

project(xxx)

# Default build type
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build" FORCE)
endif()

add_subdirectory(ext)
add_subdirectory(code)
