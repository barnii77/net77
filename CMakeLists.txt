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
        include/n77_serde.h
        include/n77_server.h
        lib/server.c
        include/n77_sock.h
        lib/sock.c
        include/n77_request.h
        include/n77_string_utils.h
        include/n77_utils.h
        include/n77_net_includes.h
        lib/init.c
        include/n77_init.h
        lib/utils.c
        include/n77_thread_includes.h
        lib/threading.c
        include/n77_fixed_size_set.h
        lib/fixed_size_set.c
        include/n77_linked_list.h
        lib/linked_list.c
        include/n77_thread_pool.h
        lib/thread_pool.c
)

add_executable(tests test/test.c)

if (WIN32)
    target_link_libraries(net77 PUBLIC Ws2_32)
endif ()

target_link_libraries(tests PRIVATE net77)