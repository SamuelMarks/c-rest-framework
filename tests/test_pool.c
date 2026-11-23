/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "c_rest_pool.h"
#include "test_protos.h"
#include <stdlib.h>
/* clang-format on */

static void *mock_malloc(size_t size) { return malloc(size); }

TEST test_pool_lifecycle_and_oom(void) {
  c_rest_pool pool;
  void *ptr1 = NULL;
  void *ptr2 = NULL;

  ASSERT_NEQ(C_REST_OK, c_rest_pool_init(NULL, 32));
  ASSERT_NEQ(C_REST_OK, c_rest_pool_init(&pool, 0));
  ASSERT_NEQ(C_REST_OK, c_rest_pool_allocate(NULL, &ptr1));
  ASSERT_NEQ(C_REST_OK, c_rest_pool_allocate(&pool, NULL));
  ASSERT_NEQ(C_REST_OK, c_rest_pool_free(NULL, ptr1));
  ASSERT_NEQ(C_REST_OK, c_rest_pool_free(&pool, NULL));
  ASSERT_NEQ(C_REST_OK, c_rest_pool_destroy(NULL));

  ASSERT_EQ(C_REST_OK, c_rest_pool_init(&pool, 1));
  ASSERT(pool.object_size >= sizeof(c_rest_pool_block));
  ASSERT_EQ(C_REST_OK, c_rest_pool_destroy(&pool));

  ASSERT_EQ(C_REST_OK, c_rest_pool_init(&pool, 32));

  pool.object_size = (size_t)-1;
  ASSERT_NEQ(C_REST_OK, c_rest_pool_allocate(&pool, &ptr1));
  pool.object_size = 32;

  g_crf_malloc_hook = mock_malloc;
  ASSERT_EQ(C_REST_OK, c_rest_pool_allocate(&pool, &ptr1));
  ASSERT(ptr1 != NULL);
  g_crf_malloc_hook = NULL;

  ASSERT_EQ(C_REST_OK, c_rest_pool_allocate(&pool, &ptr2));
  ASSERT(ptr2 != NULL);

  ASSERT_EQ(C_REST_OK, c_rest_pool_free(&pool, ptr1));
  ASSERT_EQ(C_REST_OK, c_rest_pool_free(&pool, ptr2));

  ptr1 = NULL;
  ASSERT_EQ(C_REST_OK, c_rest_pool_allocate(&pool, &ptr1));
  ASSERT(ptr1 != NULL);

  ASSERT_EQ(C_REST_OK, c_rest_pool_destroy(&pool));
  PASS();
}

SUITE(pool_suite) { RUN_TEST(test_pool_lifecycle_and_oom); }
