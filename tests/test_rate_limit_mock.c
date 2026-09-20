/* clang-format off */
#include "c_rest_error.h"
#include "greatest.h"
#include "greatest_clean.h"
#include <string.h>
#include "c_rest_mem.h"

#undef C_REST_EXPORT
#define C_REST_EXPORT

#include "c_rest_rate_limit.h"
#include "c_rest_request.h"

static int g_mock_mutex_lock_fail = 0;
static int g_mock_mutex_unlock_fail = 0;
static int g_mock_mutex_destroy_fail = 0;
static int g_mock_hashmap_destroy_fail = 0;
static int g_mock_hashmap_put_fail = 0;

extern c_rest_error_t c_rest_mutex_lock(c_rest_mutex_t mutex);
extern c_rest_error_t c_rest_mutex_unlock(c_rest_mutex_t mutex);
extern c_rest_error_t c_rest_mutex_destroy(c_rest_mutex_t mutex);
extern c_rest_error_t c_rest_hashmap_destroy(struct c_rest_hashmap *map, void (*free_value)(void *));
extern c_rest_error_t c_rest_hashmap_put(struct c_rest_hashmap *map, const char *key, void *value);

static c_rest_error_t mock_c_rest_mutex_lock(c_rest_mutex_t mutex) {
    if (g_mock_mutex_lock_fail) {
        g_mock_mutex_lock_fail = 0;
        return C_REST_ERROR_GENERIC;
    }
    return c_rest_mutex_lock(mutex);
}

static c_rest_error_t mock_c_rest_mutex_unlock(c_rest_mutex_t mutex) {
    if (g_mock_mutex_unlock_fail) {
        g_mock_mutex_unlock_fail = 0;
        c_rest_mutex_unlock(mutex);
        return C_REST_ERROR_GENERIC;
    }
    return c_rest_mutex_unlock(mutex);
}

static c_rest_error_t mock_c_rest_mutex_destroy(c_rest_mutex_t mutex) {
    if (g_mock_mutex_destroy_fail) {
        g_mock_mutex_destroy_fail = 0;
        return C_REST_ERROR_GENERIC;
    }
    return c_rest_mutex_destroy(mutex);
}

static c_rest_error_t mock_c_rest_hashmap_destroy(struct c_rest_hashmap *map, void (*free_value)(void *)) {
    if (g_mock_hashmap_destroy_fail) {
        g_mock_hashmap_destroy_fail = 0;
        return C_REST_ERROR_GENERIC;
    }
    return c_rest_hashmap_destroy(map, free_value);
}

static c_rest_error_t mock_c_rest_hashmap_put(struct c_rest_hashmap *map, const char *key, void *value) {
    if (g_mock_hashmap_put_fail) return C_REST_ERROR_GENERIC;
    return c_rest_hashmap_put(map, key, value);
}

static int g_mock_mutex_create_fail = 0;
extern c_rest_error_t c_rest_mutex_create(c_rest_mutex_t *mutex);

static c_rest_error_t mock_c_rest_mutex_create(c_rest_mutex_t *mutex) {
    if (g_mock_mutex_create_fail) {
        g_mock_mutex_create_fail = 0;
        return C_REST_ERROR_GENERIC;
    }
    return c_rest_mutex_create(mutex);
}

static void *fail_malloc(size_t size) {
  (void)size;
  return NULL;
}

#define c_rest_mutex_create mock_c_rest_mutex_create
#define c_rest_mutex_lock mock_c_rest_mutex_lock
#define c_rest_mutex_unlock mock_c_rest_mutex_unlock
#define c_rest_mutex_destroy mock_c_rest_mutex_destroy
#define c_rest_hashmap_destroy mock_c_rest_hashmap_destroy
#define c_rest_hashmap_put mock_c_rest_hashmap_put

#include "../src/c_rest_rate_limit.c"

#undef c_rest_mutex_create
#undef c_rest_mutex_lock
#undef c_rest_mutex_unlock
#undef c_rest_mutex_destroy
#undef c_rest_hashmap_destroy
#undef c_rest_hashmap_put

/* clang-format on */

static void reset_mocks(void *data) {
  (void)data;
  g_mock_mutex_create_fail = 0;
  g_mock_mutex_lock_fail = 0;
  g_mock_mutex_unlock_fail = 0;
  g_mock_mutex_destroy_fail = 0;
  g_mock_hashmap_destroy_fail = 0;
  g_mock_hashmap_put_fail = 0;
  g_crf_malloc_hook = NULL;
}

TEST test_rate_limit_error_branches(void) {
  c_rest_rate_limiter rl = {0};
  size_t tokens = 0;

  /* NULL argument checks */
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_rate_limiter_init(NULL, 10, 60, 1));
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_rate_limiter_check(NULL, "key", 0, &tokens));
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_rate_limiter_check(&rl, NULL, 0, &tokens));
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_rate_limiter_check(&rl, "key", 0, NULL));
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_rate_limiter_destroy(NULL));

  /* init: fails on hashmap_init OOM */
  g_crf_malloc_hook = fail_malloc;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_rate_limiter_init(&rl, 10, 60, 1));
  g_crf_malloc_hook = NULL;

  /* init: fails on mutex_create fail, hashmap_destroy fails */
  g_mock_mutex_create_fail = 1;
  g_mock_hashmap_destroy_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_rate_limiter_init(&rl, 10, 60, 1));

  /* init: fails on mutex_create fail, hashmap_destroy succeeds */
  g_mock_mutex_create_fail = 1;
  g_mock_hashmap_destroy_fail = 0;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_rate_limiter_init(&rl, 10, 60, 1));

  c_rest_rate_limiter_init(&rl, 10, 60, 1);

  /* check: fails on mutex_lock */
  g_mock_mutex_lock_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_rate_limiter_check(&rl, "127.0.0.1", 1, &tokens));

  /* check: fails on bucket malloc OOM */
  g_crf_malloc_hook = fail_malloc;
  ASSERT_EQ(C_REST_ERROR_OOM,
            c_rest_rate_limiter_check(&rl, "oom_key", 1, &tokens));

  /* check: fails on bucket malloc OOM AND unlock fails */
  g_mock_mutex_unlock_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_rate_limiter_check(&rl, "oom_key2", 1, &tokens));
  g_crf_malloc_hook = NULL;

  /* check: fails on hashmap_put */
  g_mock_hashmap_put_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_rate_limiter_check(&rl, "put_key", 1, &tokens));

  /* check: fails on hashmap_put AND unlock fails */
  g_mock_hashmap_put_fail = 1;
  g_mock_mutex_unlock_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_rate_limiter_check(&rl, "put_key2", 1, &tokens));
  g_mock_hashmap_put_fail = 0;

  /* check: normal bucket creation (first call) */
  ASSERT_EQ(C_REST_OK, c_rest_rate_limiter_check(&rl, "127.0.0.1", 1, &tokens));

  /* check: existing bucket with tokens (tokens_to_add > 0) */
  ASSERT_EQ(C_REST_OK, c_rest_rate_limiter_check(&rl, "127.0.0.1", 1, &tokens));

  /* check: rate limited (tokens needed > capacity) */
  ASSERT_EQ(1, c_rest_rate_limiter_check(&rl, "127.0.0.1", 1000, &tokens));

  /* check: fails on mutex_unlock on exit */
  g_mock_mutex_unlock_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_rate_limiter_check(&rl, "127.0.0.1", 1, &tokens));

  /* destroy: fails on mutex_lock */
  g_mock_mutex_lock_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_rate_limiter_destroy(&rl));

  /* destroy: fails on hashmap_destroy and unlock fails */
  g_mock_hashmap_destroy_fail = 1;
  g_mock_mutex_unlock_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_rate_limiter_destroy(&rl));

  /* destroy: fails on hashmap_destroy and unlock succeeds */
  g_mock_hashmap_destroy_fail = 1;
  g_mock_mutex_unlock_fail = 0;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_rate_limiter_destroy(&rl));

  /* destroy: fails on mutex_unlock */
  g_mock_mutex_unlock_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_rate_limiter_destroy(&rl));

  /* destroy: fails on mutex_destroy */
  g_mock_mutex_destroy_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_rate_limiter_destroy(&rl));

  /* destroy: normal success */
  ASSERT_EQ(C_REST_OK, c_rest_rate_limiter_destroy(&rl));

  /* check on uninitialized limiter */
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_rate_limiter_check(&rl, "127.0.0.1", 1, &tokens));

  /* destroy on uninitialized limiter */
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_rate_limiter_destroy(&rl));

  PASS();
}

SUITE_EXTERN(rate_limit_mock_suite);
SUITE(rate_limit_mock_suite) {
  SET_SETUP(reset_mocks, NULL);
  RUN_TEST(test_rate_limit_error_branches);
}
