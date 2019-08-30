set(xxx_ui_src ui.cpp)

add_library(xxx_ui ${xxx_ui_src})
target_compile_features(xxx_ui PRIVATE cxx_std_17)
target_compile_options(xxx_ui PRIVATE -Wall -Wextra)
target_link_libraries(xxx_ui PRIVATE termbox)
target_include_directories(xxx_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_library(xxx::ui ALIAS xxx_ui)
