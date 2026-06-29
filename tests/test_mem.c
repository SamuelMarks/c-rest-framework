/* clang-format off */
#define C_REST_MEM_TRACK 1
#include "c_rest_mem.h"
#include "test_protos.h"
#include <stdio.h>
/* clang-format on */

int test_mem(void) {
  void *ptr1 = NULL;
  void *ptr2 = NULL;
  int failed = 0;
  c_rest_error_t rc;

  void *ptr_leak = NULL;

  rc = c_rest_mem_tracker_init();
  if (rc != C_REST_OK)
    failed++;

  rc = C_REST_MALLOC(50, &ptr_leak);
  if (rc != C_REST_OK || ptr_leak == NULL)
    failed++;

  rc = C_REST_MALLOC(100, &ptr1);
  if (rc != C_REST_OK || ptr1 == NULL)
    failed++;

  rc = C_REST_CALLOC(10, 10, &ptr2);
  if (rc != C_REST_OK || ptr2 == NULL)
    failed++;

  rc = C_REST_REALLOC(ptr1, 200, &ptr1);
  if (rc != C_REST_OK || ptr1 == NULL)
    failed++;

  rc = C_REST_FREE(ptr1);
  if (rc != C_REST_OK)
    failed++;

  rc = C_REST_FREE(ptr2);
  if (rc != C_REST_OK)
    failed++;

  c_rest_mem_tracker_print_leaks();

  c_rest_mem_tracker_cleanup();
  C_REST_FREE(
      ptr_leak); /* Actually free it using normal free if tracker is dead, or
                    maybe track doesn't matter anymore, just free it using
                    C_REST_FREE before cleanup if we want, but wait, the tracker
                    print is what we wanted to hit. So we free it AFTER print
                    but BEFORE cleanup to keep it clean. */

  if (failed) {
    printf("test_mem failed\n");
  } else {
    printf("test_mem passed\n");
  }

  return failed;
}
