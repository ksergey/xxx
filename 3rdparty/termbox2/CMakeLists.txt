
include(FetchContent)

FetchContent_Declare(termbox2
  GIT https://github.com/termbox/termbox2.git
  GIT_TAG 8ee9dc17e1ca61c630f91db0aa7f81fa29a32040
)
FetchContent_MakeAvailable(termbox2)

add_library(termbox2)
target_include_directories(termbox2 PUBLIC ${termbox2_SOURCE_DIR})
target_compile_definitions(termbox2 PRIVATE -DTB_IMPL)
target_compile_definitions(termbox2 PUBLIC -DTB_OPT_ATTR_W=64 -DTB_OPT_EGC)
target_sources(termbox2 PRIVATE termbox2impl.c)


add_library(3rdparty::termbox2 ALIAS termbox2)
