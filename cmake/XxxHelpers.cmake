include(CMakeParseArguments)

# Adapted from github.com/ksergey/turboq (cmake/TurboQHelpers.cmake)

function(XxxAddTestsFromSourceList)
    set(options)
    set(oneValueArgs PREFIX)
    set(multiValueArgs LIBS COMPILE_OPTIONS DEFINITIONS)

    cmake_parse_arguments(XXX_PARSED "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    foreach (XXX_LIST ${XXX_PARSED_UNPARSED_ARGUMENTS})
        foreach (XXX_ENTRY ${${XXX_LIST}})
            if (${XXX_ENTRY} MATCHES ".*_test\\.cpp$")
                string(REGEX REPLACE "^.*\\/(.*)_test\\.cpp$" "\\1" testName "${XXX_ENTRY}")
                set(testName "${XXX_PARSED_PREFIX}-${testName}-test")

                add_executable(${testName} ${XXX_ENTRY})
                target_compile_options(${testName} PRIVATE ${XXX_PARSED_COMPILE_OPTIONS})
                target_compile_definitions(${testName} PRIVATE ${XXX_PARSED_DEFINITIONS})
                target_link_libraries(${testName} PRIVATE ${XXX_PARSED_LIBS})

                add_test(${testName} ${testName})
            endif()
        endforeach()
    endforeach()
endfunction()

function(XxxExcludeTestsFromSourceList XXX_LIST)
    list(FILTER ${XXX_LIST} EXCLUDE REGEX ".*_test\\.cpp$")
    set(${XXX_LIST} ${${XXX_LIST}} PARENT_SCOPE)
endfunction()
