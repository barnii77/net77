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
        include/n77serde.h
        include/n77server.h
        lib/server.c
        include/n77sock.h
        lib/sock.c
        include/n77request.h
        include/n77stringutils.h
        include/n77requtils.h
        include/n77netincludes.h
)

add_executable(tests test/test.c)

if (WIN32)
    target_link_libraries(net77 PUBLIC Ws2_32)
endif ()

target_link_libraries(tests PRIVATE net77)