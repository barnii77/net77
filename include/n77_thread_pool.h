#ifndef NET77_N77_THREAD_POOL_H
#define NET77_N77_THREAD_POOL_H

#include <stddef.h>
#include "n77_linked_list.h"

/**
 * Handler function that any work dispatched to a thread is passed to. *Takes ownership of the data*
 */
typedef void (*ThreadPoolHandlerFunc)(void *);

typedef struct ThreadPoolWorker {
    size_t thread;
    // this is a single heap-allocated struct
    LinkedList *jobs_list;
    ThreadPoolHandlerFunc handler;
    int killed;
    int kill_ack;
} ThreadPoolWorker;

/**
 * A thread pool structure that allows for having either some number of workers (size > 0, workers != NULL) or no workers (size == 0, workers == NULL).
 * It allows for this so that functions can accept a thread pool and still, when work is dispatched, it is all done on one thread.
 * That way, you are able to keep things single-threaded without the lib having to implement complex interfaces.
 * To create a "fake" thread pool that just does work on the same thread, call newThreadPool(0, handler_func).
 */
typedef struct ThreadPool {
    ThreadPoolWorker *workers;
    size_t size;
    size_t last_dispatched_worker;
    ThreadPoolHandlerFunc handler;
} ThreadPool;

ThreadPool newThreadPool(size_t size, ThreadPoolHandlerFunc handler);

void threadPoolDestroy(ThreadPool *pool);

/**
 * Dispatch some work to a thread selected by the thread pool. *Takes ownership of the data*
 */
void threadPoolDispatchWork(ThreadPool *pool, void *data);

#endif