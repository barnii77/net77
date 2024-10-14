#include <stdlib.h>
#include "net77/thread_includes.h"

typedef struct ThreadArgs {
    ThreadFunc func;
    void *arg;
} ThreadArgs;

#if defined(_WIN32) || defined(_WIN64)

// TODO all the mutex stuff

DWORD WINAPI runThread(void *thread_args) {
    ThreadArgs *args = thread_args;
    ThreadFunc func = args->func;
    void *arg = args->arg;
    free(thread_args);
    func(arg);
    return 0;
}

size_t threadCreate(ThreadFunc func, void *arg) {
    ThreadArgs *thread_args = malloc(sizeof(ThreadArgs));
    thread_args->func = func;
    thread_args->arg = arg;
    HANDLE thread = CreateThread(NULL, 0, runThread, thread_args, 0, NULL);
    return (size_t) thread;
}

void threadJoin(size_t thread) {
    WaitForSingleObject((HANDLE) thread, INFINITE);
}

#else

void *runThread(void *thread_args) {
    ThreadArgs *args = thread_args;
    ThreadFunc func = args->func;
    void *arg = args->arg;
    free(thread_args);
    func(arg);
    return NULL;
}

size_t threadCreate(ThreadFunc func, void *arg) {
    ThreadArgs *thread_args = malloc(sizeof(ThreadArgs));
    thread_args->func = func;
    thread_args->arg = arg;
    pthread_t thread;
    pthread_create(&thread, NULL, runThread, thread_args);
    return thread;
}

void threadJoin(size_t thread) {
    pthread_join((pthread_t)thread, NULL);
}

Mutex newMutex() {
    Mutex mutex;
    pthread_mutex_init(&mutex, NULL);
    return mutex;
}

void mutexLock(Mutex *mutex) {
    pthread_mutex_lock(mutex);
}

void mutexUnlock(Mutex *mutex) {
    pthread_mutex_unlock(mutex);
}

void mutexDestroy(Mutex *mutex) {
    pthread_mutex_destroy(mutex);
}

#endif