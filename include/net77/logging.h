#ifndef NET77_LOGGING_H
#define NET77_LOGGING_H

#include <stdio.h>

size_t getTimeInUSecs();

#ifdef NET77_ENABLE_LOGGING
    #ifdef __linux__
        #include <unistd.h>
        #include <sys/syscall.h>

        #ifdef SYS_gettid
            #define LOG_MSG(msg, ...) \
            do { \
                size_t t = getTimeInUSecs(); \
                printf("At %zu.%03zu (thread %zu): ", t / 1000, t % 1000, syscall(SYS_gettid)); \
                printf(msg, ##__VA_ARGS__); \
                printf("\n"); \
                fflush(stdout); \
            } while (0)
        #else
            #define LOG_MSG(msg, ...) \
            do { \
                size_t t = getTimeInUSecs(); \
                printf("At %zu.%03zu: ", t / 1000, t % 1000); \
                printf(msg, ##__VA_ARGS__); \
                printf("\n"); \
                fflush(stdout); \
            } while (0)
        #endif
    #else
        #define LOG_MSG(msg, ...) \
        do { \
            size_t t = getTimeInUSecs(); \
            printf("At %zu.%03zu: ", t / 1000, t % 1000); \
            printf(msg, ##__VA_ARGS__); \
            printf("\n"); \
            fflush(stdout); \
        } while (0)
    #endif
#else
    #define LOG_MSG(msg, ...) do {} while (0)
#endif

#endif