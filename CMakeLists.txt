cmake_minimum_required(VERSION 3.20)
project(net77 C)

set(CMAKE_C_STANDARD 17)

include_directories(AFTER include)

set(NET77_SOURCES
        lib/request_misc.c
        lib/freeing.c
        lib/parsing.c
        lib/serialization.c
        lib/string_builder.c
        lib/server.c
        lib/sock.c
        lib/init.c
        lib/utils.c
        lib/threading.c
        lib/mcfss.c
        lib/linked_list.c
        lib/thread_pool.c
        lib/string_utils.c
        lib/request_networking.c
        include/net77/request.h
        include/net77/string_utils.h
        include/net77/utils.h
        include/net77/net_includes.h
        include/net77/sock.h
        include/net77/init.h
        include/net77/thread_includes.h
        include/net77/mcfss.h
        include/net77/linked_list.h
        include/net77/thread_pool.h
        include/net77/int_includes.h
        include/net77/serde.h
        include/net77/server.h
        include/net77/logging.h
)

# net77 library
add_library(net77 STATIC ${NET77_SOURCES})
if (WIN32)
    target_link_libraries(net77 PUBLIC Ws2_32)
endif ()

# net77 library with logging
add_library(net77_with_logging STATIC ${NET77_SOURCES})
target_compile_definitions(net77_with_logging PRIVATE NET77_ENABLE_LOGGING)
if (WIN32)
    target_link_libraries(net77 PUBLIC Ws2_32)
endif ()

# helper to run all test suites
add_executable(run_all_tests tests/run_all_tests.c)
if (WIN32)
    target_link_libraries(run_all_tests PUBLIC Ws2_32)
endif ()

# helper to run all test suites with logging
add_executable(run_all_tests_with_logging tests/run_all_tests.c)
target_compile_definitions(run_all_tests_with_logging PRIVATE NET77_ENABLE_LOGGING)
if (WIN32)
    target_link_libraries(run_all_tests_with_logging PUBLIC Ws2_32)
endif ()

# register a test suite
function(add_test_suite name file)
    # without logging
    add_executable(tests_${name} tests/testing_helpers.h ${file})
    target_link_libraries(tests_${name} PRIVATE net77)
    if (WIN32)
        target_link_libraries(tests_${name} PUBLIC Ws2_32)
    endif ()
    add_dependencies(run_all_tests tests_${name})
    # with logging
    add_executable(tests_with_logging_${name} tests/testing_helpers.h ${file})
    target_link_libraries(tests_with_logging_${name} PRIVATE net77_with_logging)
    target_compile_definitions(tests_with_logging_${name} PRIVATE NET77_ENABLE_LOGGING)
    if (WIN32)
        target_link_libraries(tests_with_logging_${name} PUBLIC Ws2_32)
    endif ()
    add_dependencies(run_all_tests_with_logging tests_with_logging_${name})
endfunction()

# define existing test suites
add_test_suite(local tests/tests_local.c)
add_test_suite(www tests/tests_www.c)
add_test_suite(www_extra tests/tests_www_extra.c)