
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#include <errno.h>
#include <pthread.h>
#include <sys/time.h>
#include "queue.h"

typedef struct queue {
  void **data;
  int max_size;
  int current_size;
  int in;
  int out;
  pthread_mutex_t mutex;
  pthread_cond_t can_push;
  pthread_cond_t can_pop;
}queue;

QUEUE* queue_create(int size){

  QUEUE* q = malloc(sizeof(*q));
  if (q == NULL){
    fprintf(stderr, "Error: Couldn't malloc new queue\n");
    goto error;
  }

  q->max_size = size > 0 ? size : 1;
  q->data = calloc(q->max_size, sizeof(void*));
  if (q->data == NULL){
    fprintf(stderr, "Error: Couldn't malloc new data for queue\n");
    goto error;
  }

  q->current_size = 0;
  q->in = 0;
  q->out = 0;

  if (pthread_mutex_init(&(q->mutex), NULL) != 0){
    fprintf(stderr, "Error: Couldn't initialize q->mutex\n");
    goto error;
  }
  if (pthread_cond_init(&(q->can_pop), NULL) != 0){
    fprintf(stderr, "Error: Couldn't initialize q->can_pop\n");
    goto error;
  }
  if (pthread_cond_init(&(q->can_push), NULL) != 0){
    fprintf(stderr, "Error: Couldn't initialize q->can_push\n");
    goto error;
  }
 return q;

error:
  return NULL;
}

void queue_destroy(QUEUE* q){
  free(q->data);
  q->data = 0;
  free(q);
  q = 0;
}

void queue_push(QUEUE *q, void *val){
  pthread_mutex_lock(&(q->mutex));
  while (q->current_size == q->max_size){
    pthread_cond_wait(&(q->can_push), &(q->mutex));
  }
  q->data[q->in] = val;
  q->current_size++;
  q->in++;
  q->in %= q->max_size;
  pthread_mutex_unlock(&(q->mutex));
  pthread_cond_broadcast(&(q->can_pop));
}

void* queue_pop(QUEUE *q){
  return (void*) queue_pop_timedwait(q, -1, -1);
}

void* queue_pop_no_wait(QUEUE *q){
  pthread_mutex_lock(&(q->mutex));
  if (q->current_size == 0){
    pthread_mutex_unlock(&(q->mutex));
    return NULL;
  }

  void *val = q->data[q->out];
  q->current_size--;
  q->out++;
  q->out %= q->max_size;

  pthread_mutex_unlock(&(q->mutex));
  pthread_cond_broadcast(&(q->can_push));

  return val;
}

void* queue_pop_timedwait(QUEUE *q, int wait_s, int wait_ns){
  pthread_mutex_lock(&(q->mutex));

  if (wait_ns > 0 || wait_s > 0){
    struct timeval tv;
    struct timespec ts;
    gettimeofday(&tv, NULL);

    if (wait_s > 0){
      ts.tv_sec = tv.tv_sec + wait_s;
    }
    if (wait_ns > 0){
      ts.tv_nsec = tv.tv_usec + wait_ns;
    }
    int retcode = 0;
    while (q->current_size == 0 && retcode != ETIMEDOUT){
      retcode = pthread_cond_timedwait(&(q->can_pop), &(q->mutex), &ts);
    }
    if (retcode == ETIMEDOUT){
      pthread_mutex_unlock(&(q->mutex));
      return NULL;
    }
  }else{
    while (q->current_size == 0){
      pthread_cond_wait(&(q->can_pop), &(q->mutex));
   }
  }

  void *val = q->data[q->out];
  q->current_size--;
  q->out++;
  q->out %= q->max_size;

  pthread_mutex_unlock(&(q->mutex));
  pthread_cond_broadcast(&(q->can_push));

  return val;
}

int queue_size(QUEUE *q){
  pthread_mutex_lock(&(q->mutex));
  int size = q->current_size;
  pthread_mutex_unlock(&(q->mutex));
  return size;
}
