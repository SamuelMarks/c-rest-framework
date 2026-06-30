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

  C_REST_FREE(ptr_leak);

  c_rest_mem_tracker_cleanup();

  if (failed) {
    printf("test_mem failed\n");
  } else {
    printf("test_mem passed\n");
  }

  return failed;
}
