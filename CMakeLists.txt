cmake_minimum_required(VERSION 3.20)
project(net77 C)

set(CMAKE_C_STANDARD 17)

include_directories(AFTER include)

add_library(net77 STATIC
        lib/request.c
        lib/freeing.c
        lib/parsing.c
        lib/serialization.c
        lib/string_builder.c
        include/net77/serde.h
        include/net77/server.h
        lib/server.c
        include/net77/sock.h
        lib/sock.c
        include/net77/request.h
        include/net77/string_utils.h
        include/net77/utils.h
        include/net77/net_includes.h
        lib/init.c
        include/net77/init.h
        lib/utils.c
        include/net77/thread_includes.h
        lib/threading.c
        include/net77/fixed_size_set.h
        lib/fixed_size_set.c
        include/net77/linked_list.h
        lib/linked_list.c
        include/net77/thread_pool.h
        lib/thread_pool.c
)

add_executable(tests test/test.c)

if (WIN32)
    target_link_libraries(net77 PUBLIC Ws2_32)
endif ()

target_link_libraries(tests PRIVATE net77)