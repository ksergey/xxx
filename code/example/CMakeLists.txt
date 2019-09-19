add_executable(elements elements.cpp)
target_compile_features(elements PRIVATE cxx_std_17)
target_compile_options(elements PRIVATE -Wall -Wextra)
target_link_libraries(elements PRIVATE xxx::ui)

add_executable(stats stats.cpp)
target_compile_features(stats PRIVATE cxx_std_17)
target_compile_options(stats PRIVATE -Wall -Wextra)
target_link_libraries(stats PRIVATE xxx::ui)
