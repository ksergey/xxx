add_executable(example main.cpp)
target_compile_features(example PRIVATE cxx_std_17)
target_compile_options(example PRIVATE -Wall -Wextra)
target_link_libraries(example PRIVATE xxx::ui termbox)
