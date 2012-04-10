#ifndef THREADPOOL_H_
#define THREADPOOL_H_

typedef struct tp THREADPOOL;

/**
 * signature of function to call.
 */
typedef void (*work_fn) (void *);

THREADPOOL *threadpool_create(int num_threads, int max_queue_size);
void threadpool_destroy(THREADPOOL *tp);

void threadpool_start(THREADPOOL* t);
void threadpool_stop(THREADPOOL* t);

/**
 * add job to the threadpool. 
 * will call fn(arg)
 */
int threadpool_push(THREADPOOL *tp, work_fn fn, void *arg);


#endif
