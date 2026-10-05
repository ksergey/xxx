set(TargetName life)
add_executable(${TargetName} life.cpp)
target_compile_features(${TargetName} PRIVATE cxx_std_23)
target_compile_options(${TargetName} PRIVATE -Wall -Wextra -g)
target_link_libraries(${TargetName} PRIVATE xxx::xxx)

# mihomo dashboard: libcurl (https) and nlohmann_json from the system, cxxopts with CPM.
# cxxopts is fetched only when the example is built at all
find_package(CURL QUIET)
find_package(nlohmann_json 3 QUIET)
find_package(Threads)
if (CURL_FOUND AND nlohmann_json_FOUND)
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
    message(STATUS "example 'mihomo' skipped: needs libcurl and nlohmann_json (e.g. libcurl4-openssl-dev nlohmann-json3-dev)")
endif()
