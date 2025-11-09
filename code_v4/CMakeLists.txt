set(TargetName xxx_v4)

add_library(${TargetName} xxx.cpp unicode.cpp im_rect.cpp im_renderer.cpp)
target_compile_features(${TargetName} PRIVATE cxx_std_23)
target_compile_options(${TargetName} PRIVATE -Wall -Wextra -g)
target_link_libraries(${TargetName} PUBLIC 3rdparty::termbox2)
target_include_directories(${TargetName} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(xxx::xxx_v4 ALIAS ${TargetName})

set(TargetName xtest)
add_executable(${TargetName} test.cpp)
target_compile_features(${TargetName} PRIVATE cxx_std_23)
target_compile_options(${TargetName} PRIVATE -Wall -Wextra -g)
target_link_libraries(${TargetName} PRIVATE xxx::xxx_v4)

set(TargetName xtest_0)
add_executable(${TargetName} test_0.cpp)
target_compile_features(${TargetName} PRIVATE cxx_std_23)
target_compile_options(${TargetName} PRIVATE -Wall -Wextra -g)
target_link_libraries(${TargetName} PRIVATE xxx::xxx_v4)
