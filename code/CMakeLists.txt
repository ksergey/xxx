set(TargetName xxx)

add_library(${TargetName} xxx.h xxx.cpp)
target_compile_features(${TargetName} PRIVATE cxx_std_20)
target_compile_options(${TargetName} PRIVATE -Wall -Wextra -Werror -pedantic -g)
target_link_libraries(${TargetName} PRIVATE termbox)
target_include_directories(${TargetName} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(xxx::xxx ALIAS ${TargetName})
