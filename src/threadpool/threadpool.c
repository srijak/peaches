#include "threadpool.h"

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "queue.h"

typedef struct tp {
  pthread_t* pool;
  int pool_size;
  int stop;
  QUEUE* queue;
} threadpool;

typedef struct tp_job {
  work_fn fn;
  void * arg;
} tp_job;

void* threadpool_worker(void* arg) {
  THREADPOOL* tp = (THREADPOOL*) arg;
  while(tp->stop != 1){
    tp_job *job = queue_pop(tp->queue);
    job->fn(job->arg);
    free(job);
    job = NULL;
  }
  return 0;
}

THREADPOOL *threadpool_create(int num_threads, int max_queue_size) {
  THREADPOOL* tp = malloc(sizeof(*tp));
  if (tp == NULL){
    fprintf(stderr, "Couldn't create threadpool: memory allocation failed.\n");
    goto error;
  }

  tp->pool_size = num_threads > 0 ? num_threads: 2;

  // pool
  tp->pool = calloc(sizeof(*(tp->pool)), tp->pool_size);
  if (tp->pool == NULL){
    fprintf(stderr, "Couldn't create threadpool: memory allocation failed.\n");
    goto error;
  }

  tp->stop = 0;

  //queue
  tp->queue = queue_create(max_queue_size);

  return tp;

error:
  return NULL;
}

void threadpool_destroy(THREADPOOL *tp){
  queue_destroy(tp->queue);
  free(tp->pool);
  tp->pool = NULL;
  free(tp);
  tp = NULL;
}


int threadpool_push(THREADPOOL *tp, work_fn fn, void *argument) {
  tp_job *job = malloc(sizeof(*job));
  job->fn = fn;
  job->arg  = argument;

  queue_push(tp->queue, (void*) job);
  return 0;
}

void threadpool_start(THREADPOOL* tp){
  // make this account for restarts.
  for (int i = 0; i < tp->pool_size; i++){ 
    pthread_create(&(tp->pool[i]), NULL, threadpool_worker, (void*) tp);
  }
}

void threadpool_stop(THREADPOOL* t){
  // make this cleanup.
  t->stop = 1;
}


