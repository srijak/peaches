#ifndef THREADPOOL_QUEUE_H
#define THREADPOOL_QUEUE_H

typedef struct queue QUEUE;

QUEUE* queue_create(int size);
void queue_destroy(QUEUE* q);
void queue_push(QUEUE *q, void *val);
void* queue_pop(QUEUE *q);
void* queue_pop_timedwait(QUEUE *q, int wait_s, int wait_ns);
void* queue_pop_no_wait(QUEUE *q);
int queue_size(QUEUE *q);
void queue_print(QUEUE *q);

#endif
