set(TargetName xxx_v3)

add_library(${TargetName} xxx.cpp xxx-input.cpp xxx-renderer.cpp xxx-layout.cpp xxx-unicode.cpp)
target_compile_features(${TargetName} PRIVATE cxx_std_23)
target_compile_options(${TargetName} PRIVATE -Wall -Wextra -g)
target_link_libraries(${TargetName} PUBLIC 3rdparty::termbox2)
target_include_directories(${TargetName} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(xxx::xxx_v3 ALIAS ${TargetName})
