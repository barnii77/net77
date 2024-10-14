#include <stdlib.h>
#include "net77/thread_pool.h"

void threadPoolWorker(void *arg) {
    ThreadPoolWorker *worker = arg;
    while (!worker->killed) {
        void *data = linkedListPopFront(worker->jobs_list);
        if (data)
            worker->handler(data);
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
            ThreadPoolWorker worker = {
                    .thread = 0,
                    .jobs_list = jobs,
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
            .last_dispatched_worker = 0,
            .handler = handler,
    };
    return out;
}

void threadPoolDestroy(ThreadPool *pool) {
    if (pool->size) {
        for (ThreadPoolWorker *worker = pool->workers; worker < pool->workers + pool->size; worker++) {
            worker->killed = 1;
            while (!worker->kill_ack);
            linkedListDestroy(worker->jobs_list);
            free(worker->jobs_list);
        }
        free(pool->workers);
    }
}

void threadPoolDispatchWork(ThreadPool *pool, void *data) {
    if (pool->size) {
        size_t worker_idx = pool->last_dispatched_worker++;
        pool->last_dispatched_worker %= pool->size;
        ThreadPoolWorker *worker = &pool->workers[worker_idx];
        linkedListPushBack(worker->jobs_list, data);
    } else {
        pool->handler(data);
    }
}