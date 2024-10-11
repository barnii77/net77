#ifndef N77THREADINCLUDES_H
#define N77THREADINCLUDES_H

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
size_t threadCreate(void *(*func)(void *), void *arg);
#elif defined(_WIN32)
#include <pthread.h>
size_t threadCreate(void *(*func)(void *), void *arg);
#endif

#endif