/* clang-format off */
#include "c_rest_pool.h"
#include "test_protos.h"
#include <stdio.h>
#include <stdlib.h>
/* clang-format on */

static void *mock_malloc(size_t size) { return malloc(size); }

int test_pool(void) {
  c_rest_pool pool;
  void *ptr1 = NULL;
  void *ptr2 = NULL;
  int failed = 0;
  c_rest_error_t rc;
  const char *msgs[2];

  /* Null checks */
  rc = c_rest_pool_init(NULL, 32);
  failed += (rc == C_REST_OK);

  rc = c_rest_pool_init(&pool, 0);
  failed += (rc == C_REST_OK);

  rc = c_rest_pool_allocate(NULL, &ptr1);
  failed += (rc == C_REST_OK);

  rc = c_rest_pool_allocate(&pool, NULL);
  failed += (rc == C_REST_OK);

  rc = c_rest_pool_free(NULL, ptr1);
  failed += (rc == C_REST_OK);

  rc = c_rest_pool_free(&pool, NULL);
  failed += (rc == C_REST_OK);

  rc = c_rest_pool_destroy(NULL);
  failed += (rc == C_REST_OK);

  /* Test size padding (size < sizeof(block)) */
  rc = c_rest_pool_init(&pool, 1);
  failed += (rc != C_REST_OK);
  failed += (pool.object_size < sizeof(c_rest_pool_block));
  rc = c_rest_pool_destroy(&pool);
  failed += (rc != C_REST_OK);

  /* Normal flow */
  rc = c_rest_pool_init(&pool, 32);
  failed += (rc != C_REST_OK);

  /* Malloc failure path */
  pool.object_size = (size_t)-1;
  rc = c_rest_pool_allocate(&pool, &ptr1);
  failed += (rc == C_REST_OK);
  pool.object_size = 32;

  /* Malloc path */
  g_crf_malloc_hook = mock_malloc;
  rc = c_rest_pool_allocate(&pool, &ptr1);
  failed += (rc != C_REST_OK);
  failed += (ptr1 == NULL);
  g_crf_malloc_hook = NULL;

  rc = c_rest_pool_allocate(&pool, &ptr2);
  failed += (rc != C_REST_OK);
  failed += (ptr2 == NULL);

  rc = c_rest_pool_free(&pool, ptr1);
  failed += (rc != C_REST_OK);

  rc = c_rest_pool_free(&pool, ptr2);
  failed += (rc != C_REST_OK);

  /* Free list path */
  ptr1 = NULL;
  rc = c_rest_pool_allocate(&pool, &ptr1);
  failed += (rc != C_REST_OK);
  failed += (ptr1 == NULL);

  rc = c_rest_pool_destroy(&pool);
  failed += (rc != C_REST_OK);

  msgs[0] = "test_pool passed\n";
  msgs[1] = "test_pool failed\n";
  printf("%s", msgs[failed != 0]);
  return failed;
}
