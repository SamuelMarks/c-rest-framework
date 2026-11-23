/* clang-format off */
#include "c_rest_hashmap.h"
#include "c_rest_mem.h"
#include "test_protos.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
/* clang-format on */

static int g_fail_malloc_at = 0;

static c_rest_error_t hashmap_free_val(void *ptr) {
  C_REST_FREE(ptr);
  return C_REST_OK;
}

static c_rest_error_t hashmap_free_fail(void *ptr) {
  (void)ptr;
  return C_REST_ERROR_IO;
}

static void *fail_malloc_n(size_t size) {
  static int alloc_count = 0;
  if (g_fail_malloc_at <= 0) {
    alloc_count = 0;
    return NULL;
  }
  alloc_count++;
  if (alloc_count == g_fail_malloc_at) {
    alloc_count = 0;
    g_fail_malloc_at = 0;
    return NULL;
  }
  return malloc(size);
}

TEST test_hashmap_operations_and_oom(void) {
  c_rest_hashmap map;
  c_rest_hashmap map2;
  c_rest_hashmap m3;
  void *val = NULL;
  char val1[] = "value1";
  char val2[] = "value2";
  char val3[] = "value3";
  char v_str[] = "v";
  char vx_str[] = "vx";
  int i;

  ASSERT_EQ(C_REST_OK, c_rest_hashmap_init(&map, 16));
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_hashmap_init(NULL, 16));
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_hashmap_init(&map, 0));
  ASSERT_EQ(C_REST_OK, c_rest_hashmap_destroy(&map, NULL));

  g_crf_malloc_hook = fail_malloc_n;
  g_fail_malloc_at = 1;
  ASSERT_NEQ(C_REST_OK, c_rest_hashmap_init(&map, 16));
  g_crf_malloc_hook = NULL;
  g_fail_malloc_at = 0;

  ASSERT_EQ(C_REST_OK, c_rest_hashmap_init(&map, 16));
  ASSERT_EQ(C_REST_OK, c_rest_hashmap_put(&map, "key1", val1));

  g_crf_malloc_hook = fail_malloc_n;
  g_fail_malloc_at = 1;
  ASSERT_NEQ(C_REST_OK, c_rest_hashmap_put(&map, "key2_malloc_fail", val2));
  g_crf_malloc_hook = NULL;
  g_fail_malloc_at = 0;

  g_crf_malloc_hook = fail_malloc_n;
  g_fail_malloc_at = 2;
  ASSERT_NEQ(C_REST_OK, c_rest_hashmap_put(&map, "key3_malloc_fail", val3));
  g_crf_malloc_hook = NULL;
  g_fail_malloc_at = 0;

  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_hashmap_put(NULL, "key1", val1));
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_hashmap_put(&map, NULL, val1));

  ASSERT_EQ(C_REST_OK, c_rest_hashmap_put(&map, "key2", val2));
  ASSERT_EQ(C_REST_OK, c_rest_hashmap_put(&map, "key2", val3));

  for (i = 0; i < 32; i++) {
    char buf[32];
#if defined(_MSC_VER)
    sprintf_s(buf, sizeof(buf), "collide%d", i);
#else
    sprintf(buf, "collide%d", i);
#endif
    ASSERT_EQ(C_REST_OK, c_rest_hashmap_put(&map, buf, v_str));
  }

  for (i = 0; i < 32; i++) {
    char buf[32];
#if defined(_MSC_VER)
    sprintf_s(buf, sizeof(buf), "collide%dx", i);
#else
    sprintf(buf, "collide%dx", i);
#endif
    ASSERT_NEQ(C_REST_OK, c_rest_hashmap_get(&map, buf, &val));
  }

  ASSERT_EQ(C_REST_OK, c_rest_hashmap_put(&map, "collide0x", vx_str));
  ASSERT_EQ(C_REST_OK, c_rest_hashmap_put(&map, "collide1x", vx_str));
  ASSERT_EQ(C_REST_OK, c_rest_hashmap_put(&map, "collide2x", vx_str));

  for (i = 0; i < 32; i++) {
    char buf[32];
#if defined(_MSC_VER)
    sprintf_s(buf, sizeof(buf), "collide%d", i);
#else
    sprintf(buf, "collide%d", i);
#endif
    ASSERT_EQ(C_REST_OK, c_rest_hashmap_remove(&map, buf));
  }

  ASSERT_EQ(C_REST_OK, c_rest_hashmap_get(&map, "key1", &val));
  ASSERT_STR_EQ("value1", (const char *)val);

  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_hashmap_get(NULL, "key1", &val));
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_hashmap_get(&map, NULL, &val));
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_hashmap_get(&map, "key1", NULL));

  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_hashmap_put(&map, "TEST_MOCK_HASH_FAIL", val1));
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_hashmap_get(&map, "TEST_MOCK_HASH_FAIL", &val));
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_hashmap_remove(&map, "TEST_MOCK_HASH_FAIL"));

  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_hashmap_remove(NULL, "key1"));
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_hashmap_remove(&map, NULL));
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_hashmap_remove(&map, "nonexistent2"));

  ASSERT_EQ(C_REST_OK, c_rest_hashmap_remove(&map, "key1"));
  ASSERT_NEQ(C_REST_OK, c_rest_hashmap_get(&map, "key1", &val));

  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_hashmap_destroy(NULL, NULL));
  ASSERT_EQ(C_REST_OK, c_rest_hashmap_destroy(&map, NULL));

  map2.buckets = NULL;
  map2.capacity = 0;
  map2.size = 0;
  ASSERT_EQ(C_REST_OK, c_rest_hashmap_destroy(&map2, NULL));

  ASSERT_EQ(C_REST_OK, c_rest_hashmap_init(&map, 16));
  {
    char *v = NULL;
    ASSERT_EQ(C_REST_OK, C_REST_MALLOC(10, &v));
    ASSERT(v != NULL);
#if defined(_MSC_VER)
    strcpy_s(v, 5, "test");
#else
    strcpy(v, "test");
#endif
    ASSERT_EQ(C_REST_OK, c_rest_hashmap_put(&map, "k", v));
    ASSERT_EQ(C_REST_OK, c_rest_hashmap_put(&map, "k2", NULL));
    ASSERT_EQ(C_REST_OK, c_rest_hashmap_destroy(&map, hashmap_free_val));
  }

  ASSERT_EQ(C_REST_OK, c_rest_hashmap_init(&map, 4));
  ASSERT_EQ(C_REST_OK, c_rest_hashmap_put(&map, "err1", val1));
  ASSERT_EQ(C_REST_OK, c_rest_hashmap_put(&map, "err2", val2));
  ASSERT_EQ(C_REST_ERROR_IO, c_rest_hashmap_destroy(&map, hashmap_free_fail));

  g_fail_malloc_at = 0;
  ASSERT_EQ(NULL, fail_malloc_n(16));
  ASSERT_EQ(C_REST_OK, c_rest_hashmap_init(&m3, 16));
  ASSERT_EQ(C_REST_OK, c_rest_hashmap_put(&m3, "k3", val3));
  ASSERT_EQ(C_REST_OK, c_rest_hashmap_destroy(&m3, NULL));

  PASS();
}

SUITE(hashmap_suite) { RUN_TEST(test_hashmap_operations_and_oom); }
