include(FetchContent)

FetchContent_Declare(termbox
    URL "file://${CMAKE_CURRENT_SOURCE_DIR}/termbox-1.1.2.tar.gz"
)

FetchContent_Populate(termbox)

add_library(termbox)
target_compile_options(termbox PRIVATE -w)
target_include_directories(termbox PUBLIC SYSTEM ${termbox_SOURCE_DIR}/src)

file(GLOB termbox_HEADERS "${termbox_SOURCE_DIR}/src/*.h")
file(GLOB termbox_SOURCES "${termbox_SOURCE_DIR}/src/*.c")
target_sources(termbox PRIVATE ${termbox_HEADERS} ${termbox_SOURCES})
