cmake_minimum_required(VERSION 4.0)

project(xxx)

# no C++ modules here: skip dependency scanning (with clang it would need clang-scan-deps)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

# threads (tests and examples only, the library starts none): -pthread rather than a bare -lpthread.
# glibc before 2.34 keeps pthread_create in a separate libpthread
set(THREADS_PREFER_PTHREAD_FLAG ON)

# Default build type
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build" FORCE)
endif()

# library itself has no dependencies. Tests (doctest) and examples (cxxopts, nlohmann_json) fetch theirs with CPM;
# both are built by default only when xxx is the top level project, not when it is embedded
option(xxx_BUILD_TESTS "Build tests (*_test.cpp files, linked against doctest)" ${PROJECT_IS_TOP_LEVEL})
option(xxx_BUILD_EXAMPLES "Build examples" ${PROJECT_IS_TOP_LEVEL})

include(cmake/XxxHelpers.cmake)

if (xxx_BUILD_TESTS OR xxx_BUILD_EXAMPLES)
    include(cmake/GetCPM.cmake)
endif()


if (xxx_BUILD_TESTS)
    CPMAddPackage("gh:doctest/doctest@2.5.2")
    enable_testing()
endif()

add_subdirectory(code)
if (xxx_BUILD_EXAMPLES)
    add_subdirectory(example)
endif()
