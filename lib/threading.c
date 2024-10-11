#include "n77threadincludes.h"

typedef void (*ThreadFunc)(void *);

typedef struct ThreadArgs {
    ThreadFunc func;
    void *arg;
} ThreadArgs;

#if defined(_WIN32) || defined(_WIN64)
DWORD WINAPI runThread(void *thread_args) {
    ThreadArgs *args = (ThreadArgs *)thread_args;
    args->func(args->arg);
    return 0;
}

size_t threadCreate(void (*func)(void *), void *arg) {
    ThreadArgs thread_args = {.func = func, .arg = arg};
    HANDLE thread = CreateThread(NULL, 0, runThread, &thread_args, 0, NULL);
    return (size_t)thread;
}
#else
void *runThread(void *thread_args) {
    ThreadArgs *args = (ThreadArgs *)thread_args;
    args->func(args->arg);
    return NULL;
}

size_t threadCreate(void (*func)(void *), void *arg) {
    pthread_t thread;
    ThreadArgs thread_args = {.func = func, .arg = arg};
    pthread_create(&thread, NULL, runThread, &thread_args);
    return thread;
}
#endif