set(src ui.cpp)

add_library(xxx_ui ${src})
target_compile_features(xxx_ui PRIVATE cxx_std_20)
target_compile_options(xxx_ui PRIVATE -Wall -Wextra -Werror -pedantic -g)
target_link_libraries(xxx_ui PRIVATE termbox)
target_include_directories(xxx_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_library(xxx::ui ALIAS xxx_ui)
