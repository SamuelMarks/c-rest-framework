/* clang-format off */
#include "c_rest_pool.h"
#include "test_protos.h"
#include <stdio.h>
/* clang-format on */

int test_pool(void) {
  c_rest_pool pool;
  void *ptr1 = NULL;
  void *ptr2 = NULL;
  int failed = 0;
  c_rest_error_t rc;

  rc = c_rest_pool_init(&pool, 32);
  if (rc != C_REST_OK)
    failed++;

  rc = c_rest_pool_allocate(&pool, &ptr1);
  if (rc != C_REST_OK || ptr1 == NULL)
    failed++;

  rc = c_rest_pool_allocate(&pool, &ptr2);
  if (rc != C_REST_OK || ptr2 == NULL)
    failed++;

  rc = c_rest_pool_free(&pool, ptr1);
  if (rc != C_REST_OK)
    failed++;

  rc = c_rest_pool_free(&pool, ptr2);
  if (rc != C_REST_OK)
    failed++;

  rc = c_rest_pool_destroy(&pool);
  if (rc != C_REST_OK)
    failed++;

  if (failed) {
    printf("test_pool failed\n");
  } else {
    printf("test_pool passed\n");
  }
  return failed;
}
