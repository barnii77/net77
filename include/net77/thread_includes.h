#ifndef N77THREADINCLUDES_H
#define N77THREADINCLUDES_H

#if defined(_WIN32) || defined(_WIN64)

#include <windows.h>

// TODO Mutex

#else

#include <pthread.h>

typedef pthread_mutex_t Mutex;

#endif

typedef void (*ThreadFunc)(void *);

size_t threadCreate(ThreadFunc func, void *arg);

void threadJoin(size_t thread);

Mutex newMutex();

void mutexLock(Mutex *mutex);

void mutexUnlock(Mutex *mutex);

void mutexDestroy(Mutex *mutex);

#endif