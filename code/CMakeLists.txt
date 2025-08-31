set(TargetName xxx)

add_library(${TargetName} xxx.h xxx.cpp xxx2.cpp xxx_io.cpp)
target_compile_features(${TargetName} PRIVATE cxx_std_23)
target_compile_options(${TargetName} PRIVATE -Wall -Wextra -pedantic -g)
target_link_libraries(${TargetName} PUBLIC 3rdparty::termbox2)
target_include_directories(${TargetName} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(xxx::xxx ALIAS ${TargetName})
