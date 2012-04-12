#ifndef THREADPOOL_H_
#define THREADPOOL_H_

#include <stdbool.h>

typedef void* (*work_fn) (void *);

typedef struct tp THREADPOOL;

THREADPOOL *threadpool_create(int num_threads, int max_queue_size);
void threadpool_destroy(THREADPOOL *tp);

void threadpool_start(THREADPOOL* t);
void threadpool_stop(THREADPOOL* t);
bool threadpool_stopped(THREADPOOL* t);

void threadpool_set_func(THREADPOOL *t, work_fn fn);

/**
 * add job to the threadpool. 
 * will call fn(arg)
 */
int threadpool_push(THREADPOOL *tp, void* job);
void* threadpool_pop(THREADPOOL *tp);
void* threadpool_pop_timedwait(THREADPOOL *tp, int wait_s, int wait_ns);
void* threadpool_pop_no_wait(THREADPOOL *tp);

#endif
