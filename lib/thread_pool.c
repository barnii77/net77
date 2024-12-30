#include <stdlib.h>
#include "net77/thread_pool.h"

void threadPoolWorker(void *arg) {
    ThreadPoolWorker *worker = arg;
    while (!worker->killed) {
        condLockAndWait(&worker->wake_cond);  // wake up when there is work to do (avoid busy waiting)
        void *data = linkedListPopFront(worker->jobs_list);
        if (data)
            worker->handler(data);
        condUnlock(&worker->wake_cond);
    }
    worker->kill_ack = 1;
}

ThreadPool newThreadPool(size_t size, ThreadPoolHandlerFunc handler) {
    ThreadPoolWorker *workers = NULL;
    if (size > 0) {
        workers = malloc(size * sizeof(ThreadPoolWorker));
        for (size_t i = 0; i < size; i++) {
            LinkedList *jobs = malloc(sizeof(LinkedList));
            *jobs = newLinkedList();
            Cond wake_cond = newCond();
            ThreadPoolWorker worker = {
                    .thread = 0,
                    .jobs_list = jobs,
                    .wake_cond = wake_cond,
                    .handler = handler,
                    .killed = 0,
                    .kill_ack = 0,
            };
            workers[i] = worker;
            mutexLock(&jobs->mutex);
            size_t thread = threadCreate(threadPoolWorker, &workers[i]);
            workers[i].thread = thread;
            mutexUnlock(&jobs->mutex);
        }
    }
    ThreadPool out = {
            .size = size,
            .workers = workers,
            .handler = handler,
    };
    return out;
}

void threadPoolDestroy(ThreadPool *pool) {
    if (pool->size) {
        for (ThreadPoolWorker *worker = pool->workers; worker < pool->workers + pool->size; worker++) {
            worker->killed = 1;
            condSignal(&worker->wake_cond);  // wake up worker so it can accept the kill signal
            while (!worker->kill_ack);
            linkedListDestroy(worker->jobs_list);
            condDestroy(&worker->wake_cond);
            free(worker->jobs_list);
        }
        free(pool->workers);
    }
}

void threadPoolDispatchWork(ThreadPool *pool, void *data) {
    if (pool->size) {
        // find the worker with the least amount of jobs (prefer the first one in case of a tie)
        size_t worker_idx = 0;
        size_t min_jobs = -1;
        for (size_t i = 0; i < pool->size; i++) {
            size_t jobs = linkedListLen(pool->workers[i].jobs_list);
            if (jobs < min_jobs) {
                worker_idx = i;
                min_jobs = jobs;
            }
        }

        ThreadPoolWorker *worker = &pool->workers[worker_idx];
        linkedListPushBack(worker->jobs_list, data);
        condSignal(&worker->wake_cond);
    } else {
        pool->handler(data);
    }
}