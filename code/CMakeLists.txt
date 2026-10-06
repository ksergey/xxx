set(TargetName xxx)

add_library(${TargetName}
    xxx.cpp unicode.cpp im_rect.cpp im_renderer.cpp im_backend_headless.cpp
    terminal.cpp ansi_input.cpp ansi_screen.cpp)
target_compile_features(${TargetName} PUBLIC cxx_std_23)
target_compile_options(${TargetName} PRIVATE -Wall -Wextra -g)
target_include_directories(${TargetName} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(xxx::xxx ALIAS ${TargetName})


if (xxx_BUILD_TESTS)
    file(GLOB Tests "${CMAKE_CURRENT_SOURCE_DIR}/*_test.cpp")
    set(TestLibs doctest::doctest_with_main xxx::xxx)
    XxxAddTestsFromSourceList(Tests
        PREFIX xxx
        COMPILE_OPTIONS -Wall -Wextra -g
        LIBS ${TestLibs})
endif()

set(TargetName xtest)
add_executable(${TargetName} test.cpp)
target_compile_options(${TargetName} PRIVATE -Wall -Wextra -g)
target_link_libraries(${TargetName} PRIVATE xxx::xxx)

set(TargetName xtest_0)
add_executable(${TargetName} test_0.cpp)
target_compile_options(${TargetName} PRIVATE -Wall -Wextra -g)
target_link_libraries(${TargetName} PRIVATE xxx::xxx)
