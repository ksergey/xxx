cmake_minimum_required(VERSION 4.0)

project(xxx)

# Default build type
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build" FORCE)
endif()

option(xxx_BUILD_TESTS "Build tests (*_test.cpp files, linked against doctest)" ON)

include(cmake/XxxHelpers.cmake)
include(cmake/GetCPM.cmake)

if (xxx_BUILD_TESTS)
    CPMAddPackage("gh:doctest/doctest@2.5.2")
    enable_testing()
endif()

add_subdirectory(3rdparty EXCLUDE_FROM_ALL)
add_subdirectory(code)
add_subdirectory(example)
