set(TargetName xxx)

add_library(${TargetName}
    xxx.cpp unicode.cpp im_rect.cpp im_renderer.cpp im_backend_termbox.cpp im_backend_headless.cpp)
target_compile_features(${TargetName} PUBLIC cxx_std_23)
target_compile_options(${TargetName} PRIVATE -Wall -Wextra -g)
# termbox is an implementation detail of im_backend_termbox.cpp only
target_link_libraries(${TargetName} PRIVATE 3rdparty::termbox2)
target_include_directories(${TargetName} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(xxx::xxx ALIAS ${TargetName})

if (xxx_BUILD_TESTS)
    file(GLOB Tests "${CMAKE_CURRENT_SOURCE_DIR}/*_test.cpp")
    XxxAddTestsFromSourceList(Tests
        PREFIX xxx
        COMPILE_OPTIONS -Wall -Wextra -g
        LIBS doctest::doctest_with_main xxx::xxx)
endif()

set(TargetName xtest)
add_executable(${TargetName} test.cpp)
target_compile_options(${TargetName} PRIVATE -Wall -Wextra -g)
target_link_libraries(${TargetName} PRIVATE xxx::xxx)

set(TargetName xtest_0)
add_executable(${TargetName} test_0.cpp)
target_compile_options(${TargetName} PRIVATE -Wall -Wextra -g)
target_link_libraries(${TargetName} PRIVATE xxx::xxx)
