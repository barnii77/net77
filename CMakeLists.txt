cmake_minimum_required(VERSION 3.20)
project(net77 C)

set(CMAKE_C_STANDARD 17)

include_directories(AFTER include)

set(NET77_SOURCES
        lib/request.c
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

set(NET77_TESTS tests/test.c)

add_library(net77 STATIC ${NET77_SOURCES})
if (WIN32)
    target_link_libraries(net77 PUBLIC Ws2_32)
endif ()

add_library(net77_with_logging STATIC ${NET77_SOURCES})
target_compile_definitions(net77_with_logging PRIVATE NET77_ENABLE_LOGGING)
if (WIN32)
    target_link_libraries(net77 PUBLIC Ws2_32)
endif ()

add_executable(tests ${NET77_TESTS})
target_link_libraries(tests PRIVATE net77)

add_executable(tests_with_logging ${NET77_TESTS})
target_link_libraries(tests_with_logging PRIVATE net77_with_logging)