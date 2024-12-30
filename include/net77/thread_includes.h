#ifndef N77THREADINCLUDES_H
#define N77THREADINCLUDES_H

#if defined(_WIN32) || defined(_WIN64)

#include <windows.h>

// TODO Mutex and cond

#else

#include <pthread.h>

typedef pthread_mutex_t Mutex;

typedef struct {
    Mutex mutex;
    pthread_cond_t cond;
} Cond;

#endif

typedef void (*ThreadFunc)(void *);

size_t threadCreate(ThreadFunc func, void *arg);

void threadJoin(size_t thread);

Mutex newMutex();

void mutexLock(Mutex *mutex);

void mutexUnlock(Mutex *mutex);

void mutexDestroy(Mutex *mutex);

Cond newCond();

void condLockAndWait(Cond *cond);

void condUnlock(Cond *cond);

void condSignal(Cond *cond);

void condDestroy(Cond *cond);

#endif