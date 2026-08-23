/* clang-format off */
#include "c_rest_hashmap.h"
#include "c_rest_mem.h"
#include "test_protos.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
/* clang-format on */

extern int g_fail_malloc_at;

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

int test_hashmap(void) {
  c_rest_hashmap map;
  c_rest_error_t rc;
  void *val = NULL;
  int failed = 0;
  extern int g_fail_malloc_at;

  rc = c_rest_hashmap_init(&map, 16);
  failed += (rc != C_REST_OK);

  rc = c_rest_hashmap_init(NULL, 16);
  failed += (rc != C_REST_ERROR_GENERIC);

  rc = c_rest_hashmap_init(&map, 0);
  failed += (rc != C_REST_ERROR_GENERIC);

  c_rest_hashmap_destroy(&map, NULL);

  g_crf_malloc_hook = fail_malloc_n;
  g_fail_malloc_at = 1;
  rc = c_rest_hashmap_init(&map, 16);
  failed += (rc != C_REST_ERROR_OOM && rc != C_REST_ERROR_GENERIC);
  g_crf_malloc_hook = NULL;
  g_fail_malloc_at = 0;

  rc = c_rest_hashmap_init(&map, 16);
  failed += (rc != C_REST_OK);

  rc = c_rest_hashmap_put(&map, "key1", "value1");
  failed += (rc != C_REST_OK);

  g_crf_malloc_hook = fail_malloc_n;
  g_fail_malloc_at = 1;
  rc = c_rest_hashmap_put(&map, "key2_malloc_fail", "value2");
  failed += (rc != C_REST_ERROR_OOM && rc != C_REST_ERROR_GENERIC);
  g_crf_malloc_hook = NULL;
  g_fail_malloc_at = 0;

  g_crf_malloc_hook = fail_malloc_n;
  g_fail_malloc_at = 2;
  rc = c_rest_hashmap_put(&map, "key3_malloc_fail", "value3");
  failed += (rc != C_REST_ERROR_OOM && rc != C_REST_ERROR_GENERIC);
  g_crf_malloc_hook = NULL;
  g_fail_malloc_at = 0;

  rc = c_rest_hashmap_put(NULL, "key1", "value1");
  failed += (rc != C_REST_ERROR_GENERIC);

  rc = c_rest_hashmap_put(&map, NULL, "value1");
  failed += (rc != C_REST_ERROR_GENERIC);

  rc = c_rest_hashmap_put(&map, "key2", "value2");
  failed += (rc != C_REST_OK);

  rc = c_rest_hashmap_put(&map, "key2", "value3");
  failed += (rc != C_REST_OK);

  {
    int i;
    for (i = 0; i < 32; i++) {
      char buf[32];
#if defined(_MSC_VER)
      sprintf_s(buf, sizeof(buf), "collide%d", i);
#else
      sprintf(buf, "collide%d", i);
#endif
      rc = c_rest_hashmap_put(&map, buf, "v");
      failed += (rc != C_REST_OK);
    }

    for (i = 0; i < 32; i++) {
      char buf[32];
#if defined(_MSC_VER)
      sprintf_s(buf, sizeof(buf), "collide%dx", i);
#else
      sprintf(buf, "collide%dx", i);
#endif
      rc = c_rest_hashmap_get(&map, buf, &val);
    }

    rc = c_rest_hashmap_put(&map, "collide0x", "vx");
    rc = c_rest_hashmap_put(&map, "collide1x", "vx");
    rc = c_rest_hashmap_put(&map, "collide2x", "vx");

    for (i = 0; i < 32; i++) {
      char buf[32];
#if defined(_MSC_VER)
      sprintf_s(buf, sizeof(buf), "collide%d", i);
#else
      sprintf(buf, "collide%d", i);
#endif
      rc = c_rest_hashmap_remove(&map, buf);
      failed += (rc != C_REST_OK);
    }
  }

  rc = c_rest_hashmap_get(&map, "key1", &val);
  failed += (rc != C_REST_OK || strcmp((const char *)val, "value1") != 0);

  rc = c_rest_hashmap_get(NULL, "key1", &val);
  failed += (rc != C_REST_ERROR_GENERIC);

  rc = c_rest_hashmap_get(&map, NULL, &val);
  failed += (rc != C_REST_ERROR_GENERIC);

  rc = c_rest_hashmap_get(&map, "key1", NULL);
  failed += (rc != C_REST_ERROR_GENERIC);

  rc = c_rest_hashmap_put(&map, "TEST_MOCK_HASH_FAIL", "value1");
  failed += (rc != C_REST_ERROR_GENERIC);

  rc = c_rest_hashmap_get(&map, "TEST_MOCK_HASH_FAIL", &val);
  failed += (rc != C_REST_ERROR_GENERIC);

  rc = c_rest_hashmap_remove(&map, "TEST_MOCK_HASH_FAIL");
  failed += (rc != C_REST_ERROR_GENERIC);

  rc = c_rest_hashmap_remove(NULL, "key1");
  failed += (rc != C_REST_ERROR_GENERIC);

  /* Test NULL key on get, put, remove to trigger hash_string failures if
   * mocked, or just NULL checks */
  rc = c_rest_hashmap_remove(&map, NULL);
  failed += (rc != C_REST_ERROR_GENERIC);

  rc = c_rest_hashmap_remove(&map, NULL);
  failed += (rc != C_REST_ERROR_GENERIC);

  rc = c_rest_hashmap_remove(&map, "nonexistent2");
  failed += (rc != C_REST_ERROR_GENERIC);

  rc = c_rest_hashmap_remove(&map, "key1");
  failed += (rc != C_REST_OK);

  rc = c_rest_hashmap_get(&map, "key1", &val);
  failed += (rc == C_REST_OK);

  rc = c_rest_hashmap_destroy(NULL, NULL);
  failed += (rc != C_REST_ERROR_GENERIC);

  rc = c_rest_hashmap_destroy(&map, NULL);
  failed += (rc != C_REST_OK);

  {
    c_rest_hashmap map2;
    map2.buckets = NULL;
    map2.capacity = 0;
    map2.size = 0;
    rc = c_rest_hashmap_destroy(&map2, NULL);
    failed += (rc != C_REST_OK);
  }

  rc = c_rest_hashmap_init(&map, 16);
  if (rc == C_REST_OK) {
    char *v = CRF_MALLOC(10);
    char *v2 = NULL;
    if (v) {
#if defined(_MSC_VER)
      strcpy_s(v, 5, "test");
#else
      strcpy(v, "test");
#endif
      rc = c_rest_hashmap_put(&map, "k", v);
      rc = c_rest_hashmap_put(&map, "k2", v2);
      rc = c_rest_hashmap_destroy(&map, free);
    }
  }

  {

    c_rest_hashmap m3;
    (void)!c_rest_hashmap_init(&m3, 16);
    /* Try to simulate malloc failure by failing io */

    rc = c_rest_hashmap_put(
        &m3, "k3", "v3"); /* not testing C_REST_MALLOC failure unless mocked */

    (void)!c_rest_hashmap_destroy(&m3, NULL);
  }

  if (failed) {
    printf("test_hashmap failed\n");
  } else {
    printf("test_hashmap passed\n");
  }

  return failed;
}
