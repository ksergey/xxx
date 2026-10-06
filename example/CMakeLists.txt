set(TargetName life)
add_executable(${TargetName} life.cpp)
target_compile_features(${TargetName} PRIVATE cxx_std_23)
target_compile_options(${TargetName} PRIVATE -Wall -Wextra -g)
target_link_libraries(${TargetName} PRIVATE xxx::xxx)

# mihomo dashboard: libcurl (https) from the system, nlohmann_json and cxxopts with CPM.
# They are fetched only when the example is built at all
find_package(CURL QUIET)
find_package(Threads)
if (CURL_FOUND)
    # release archive (headers + CMake, ~110 KB) instead of the huge git repository
    CPMAddPackage(
        NAME nlohmann_json
        VERSION 3.12.0
        URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
        URL_HASH SHA256=42f6e95cad6ec532fd372391373363b62a14af6d771056dbfc86160e6dfff7aa
        OPTIONS "JSON_Install OFF"
    )
    CPMAddPackage(
        NAME cxxopts
        GITHUB_REPOSITORY jarro2783/cxxopts
        VERSION 3.3.1
        OPTIONS "CXXOPTS_BUILD_EXAMPLES OFF" "CXXOPTS_BUILD_TESTS OFF" "CXXOPTS_ENABLE_INSTALL OFF"
    )
    set(TargetName mihomo)
    add_executable(${TargetName} mihomo.cpp theme_file.cpp)
    target_compile_features(${TargetName} PRIVATE cxx_std_23)
    target_compile_options(${TargetName} PRIVATE -Wall -Wextra -g)
    target_link_libraries(${TargetName} PRIVATE xxx::xxx CURL::libcurl nlohmann_json::nlohmann_json cxxopts::cxxopts Threads::Threads)

    # JSON themes (--theme FILE) live in the example: the library itself has no JSON dependency
    if (xxx_BUILD_TESTS)
        add_executable(xxx-theme_file-test theme_file_test.cpp theme_file.cpp)
        target_compile_features(xxx-theme_file-test PRIVATE cxx_std_23)
        target_compile_options(xxx-theme_file-test PRIVATE -Wall -Wextra -g)
        target_link_libraries(xxx-theme_file-test PRIVATE doctest::doctest_with_main xxx::xxx nlohmann_json::nlohmann_json)
        add_test(xxx-theme_file-test xxx-theme_file-test)
    endif()
else()
    message(STATUS "example 'mihomo' skipped: needs libcurl (e.g. libcurl4-openssl-dev)")
endif()

set(TargetName poll_loop)
add_executable(${TargetName} poll_loop.cpp)
target_compile_features(${TargetName} PRIVATE cxx_std_23)
target_compile_options(${TargetName} PRIVATE -Wall -Wextra -g)
target_link_libraries(${TargetName} PRIVATE xxx::xxx)

# io_uring example: needs liburing from the system (e.g. liburing-dev), Linux only
find_library(URING_LIBRARY uring)
find_path(URING_INCLUDE_DIR liburing.h)
if (URING_LIBRARY AND URING_INCLUDE_DIR)
    set(TargetName uring_loop)
    add_executable(${TargetName} uring_loop.cpp)
    target_compile_features(${TargetName} PRIVATE cxx_std_23)
    target_compile_options(${TargetName} PRIVATE -Wall -Wextra -g)
    target_include_directories(${TargetName} PRIVATE ${URING_INCLUDE_DIR})
    target_link_libraries(${TargetName} PRIVATE xxx::xxx ${URING_LIBRARY})
else()
    message(STATUS "example 'uring_loop' skipped: needs liburing")
endif()

# external ip from several services: libcurl from the system, cxxopts with CPM (added above)
if (CURL_FOUND)
    set(TargetName myip)
    add_executable(${TargetName} myip.cpp)
    target_compile_features(${TargetName} PRIVATE cxx_std_23)
    target_compile_options(${TargetName} PRIVATE -Wall -Wextra -g)
    target_link_libraries(${TargetName} PRIVATE xxx::xxx CURL::libcurl cxxopts::cxxopts Threads::Threads)
endif()
