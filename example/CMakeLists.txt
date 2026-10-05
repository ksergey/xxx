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
    add_executable(${TargetName} mihomo.cpp)
    target_compile_features(${TargetName} PRIVATE cxx_std_23)
    target_compile_options(${TargetName} PRIVATE -Wall -Wextra -g)
    target_link_libraries(${TargetName} PRIVATE xxx::xxx CURL::libcurl nlohmann_json::nlohmann_json cxxopts::cxxopts Threads::Threads)
else()
    message(STATUS "example 'mihomo' skipped: needs libcurl (e.g. libcurl4-openssl-dev)")
endif()
