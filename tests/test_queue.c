#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>
#include <check.h>
#include "threadpool/queue.h"

void* push_item(void* q) {
  int val = 123;
  queue_push((QUEUE*)q, &val);
  return NULL;
}
void* pop_item(void* q) {
  queue_pop((QUEUE*)q);
  return NULL;
}
START_TEST (single_push_pop)
{
  int val = 23;
  int *ret_val = 0;
  QUEUE* q = queue_create(10);
  queue_push(q, &val);
  ret_val = queue_pop(q);
  fail_if( &val != ret_val , "Error: Pop should have returned push. 1");

  queue_destroy(q);
}
END_TEST

START_TEST (does_fifo_and_correct_size)
{
  // push items. verify count. pop items and verify order is correct.
  int size = 5;
  int items[5] = {1, 2, 3, 4, 5};

  QUEUE* q = queue_create(10);
  for (int i = 0; i < size; i++){
    queue_push(q, &(items[i]));
  }

  fail_if(queue_size(q) != size, "Error: Expected 5 items in queue.");

  for (int i =0; i < size; i++){
    fail_if( &(items[i]) != queue_pop(q)  , "Error: Pop should have returned pushed item in order.");
  }

  queue_destroy(q);
}
END_TEST

START_TEST (full_queue_makes_push_wait_before_adding)
{
  int val = 23;

  QUEUE* q = queue_create(1);
  // fill up queue
  queue_push(q, &val);

  // try to add an item from another thread.
  pthread_t producer;
  pthread_create(&producer, NULL, push_item, (void*) q);

  // give it time to add item, if it was going to.
  sleep(1);

  fail_if(queue_size(q) != 1, "Error: Queue size should have remained at 1.");

  // now pop the first item. Then pop again. We should
  //  get a different item the second time.
  int* retval = queue_pop(q);

  sleep(1);
  fail_if(queue_size(q) != 1, "Error: Queue size should have gone back to 1 \
                              since there was a thread waitign to do so.");
  fail_if(queue_pop(q) == retval, "Error: Second push item shouldn't be same as first push."); 

  queue_destroy(q);
}
END_TEST


START_TEST (empty_queue_makes_pop_wait)
{
  QUEUE* q = queue_create(10);

  // try to pop an item from another thread.
  pthread_t producer;
  pthread_create(&producer, NULL, pop_item, (void*) q);

  fail_if(queue_size(q) != 0, "Error: Queue size should have still be 0.");

  // now push and item and verify that it gets popped.
  // since we haven't dont anything, that pop was from the other thread.
  int val = 34;
  queue_push(q, &val);
  sleep(1);

  fail_if(queue_size(q) != 0, "Error: Queue size should be 0.");

  queue_destroy(q);
}
END_TEST


int main(void) {
  Suite *s = suite_create("QueueSuite");
  TCase *tc_core = tcase_create("tests");
  tcase_set_timeout(tc_core, 60);
  tcase_add_test (tc_core, single_push_pop);
  tcase_add_test (tc_core, does_fifo_and_correct_size);
  tcase_add_test (tc_core, full_queue_makes_push_wait_before_adding);
  tcase_add_test (tc_core, empty_queue_makes_pop_wait);

  suite_add_tcase(s, tc_core);

  SRunner *sr = srunner_create(s);
  srunner_run_all(sr, CK_VERBOSE);
  int failed = srunner_ntests_failed (sr);
  srunner_free(sr);
  return (failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
