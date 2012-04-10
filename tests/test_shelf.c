#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <check.h>
#include "shelf/shelf.h"

const char* path = "/tmp/shelf_test";

void setup(void){
  rmdir(path);
}
void teardown(void){
  rmdir(path);
}

START_TEST (open_fails_nonexistant_path_create_false) 
{
  SHELF *s = shelf_open(path, false);
  fail_if( s != NULL , "Error: SHELF should have been NULL");
}
END_TEST

START_TEST (open_passes_nonexistant_path_create_true) 
{
  SHELF *s = shelf_open(path, true);
  fail_if( s == NULL , "Error: SHELF should not have been NULL");
}
END_TEST

int main(void) {
  Suite *s = suite_create("ShelfSuite");
  TCase *tc_core = tcase_create("tests");
  tcase_add_checked_fixture(tc_core, setup, teardown);
  tcase_add_test (tc_core, open_fails_nonexistant_path_create_false);
  tcase_add_test (tc_core, open_passes_nonexistant_path_create_true);
  suite_add_tcase(s, tc_core);

  SRunner *sr = srunner_create(s);
  srunner_run_all(sr, CK_VERBOSE);
  int failed = srunner_ntests_failed (sr);
  srunner_free(sr);
  return (failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
