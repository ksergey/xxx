include(FetchContent)

if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
  cmake_policy(SET CMP0135 NEW)
endif()

FetchContent_Declare(termbox
  URL "https://github.com/nsf/termbox/archive/refs/tags/v1.1.2.tar.gz"
)

FetchContent_Populate(termbox)

add_library(termbox)
target_compile_options(termbox PRIVATE -w)
target_include_directories(termbox PUBLIC SYSTEM ${termbox_SOURCE_DIR}/src)

file(GLOB termbox_HEADERS "${termbox_SOURCE_DIR}/src/*.h")
file(GLOB termbox_SOURCES "${termbox_SOURCE_DIR}/src/*.c")
target_sources(termbox PRIVATE ${termbox_HEADERS} ${termbox_SOURCES})
