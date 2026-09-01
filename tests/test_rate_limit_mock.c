/* clang-format off */
#include "c_rest_error.h"
#include "greatest.h"
#include <string.h>

#undef C_REST_EXPORT
#define C_REST_EXPORT

#include "c_rest_rate_limit.h"
#include "c_rest_request.h"

static int g_mock_mutex_lock_countdown = -1;
static int g_mock_mutex_unlock_countdown = -1;
static int g_mock_mutex_destroy_countdown = -1;
static int g_mock_hashmap_destroy_countdown = -1;

extern c_rest_error_t c_rest_mutex_lock(c_rest_mutex_t mutex);
extern c_rest_error_t c_rest_mutex_unlock(c_rest_mutex_t mutex);
extern c_rest_error_t c_rest_mutex_destroy(c_rest_mutex_t mutex);
extern c_rest_error_t c_rest_hashmap_destroy(struct c_rest_hashmap *map, void (*free_value)(void *));

static c_rest_error_t mock_c_rest_mutex_lock(c_rest_mutex_t mutex) {
    if (g_mock_mutex_lock_countdown >= 0) {
        if (g_mock_mutex_lock_countdown == 0) return C_REST_ERROR_GENERIC;
        g_mock_mutex_lock_countdown--;
    }
    return c_rest_mutex_lock(mutex);
}

static c_rest_error_t mock_c_rest_mutex_unlock(c_rest_mutex_t mutex) {
    if (g_mock_mutex_unlock_countdown >= 0) {
        if (g_mock_mutex_unlock_countdown == 0) return C_REST_ERROR_GENERIC;
        g_mock_mutex_unlock_countdown--;
    }
    return c_rest_mutex_unlock(mutex);
}

static c_rest_error_t mock_c_rest_mutex_destroy(c_rest_mutex_t mutex) {
    if (g_mock_mutex_destroy_countdown >= 0) {
        if (g_mock_mutex_destroy_countdown == 0) return C_REST_ERROR_GENERIC;
        g_mock_mutex_destroy_countdown--;
    }
    return c_rest_mutex_destroy(mutex);
}

static c_rest_error_t mock_c_rest_hashmap_destroy(struct c_rest_hashmap *map, void (*free_value)(void *)) {
    if (g_mock_hashmap_destroy_countdown >= 0) {
        if (g_mock_hashmap_destroy_countdown == 0) return C_REST_ERROR_GENERIC;
        g_mock_hashmap_destroy_countdown--;
    }
    return c_rest_hashmap_destroy(map, free_value);
}

static int g_mock_mutex_create_countdown = -1;
extern c_rest_error_t c_rest_mutex_create(c_rest_mutex_t **mutex);

static c_rest_error_t mock_c_rest_mutex_create(c_rest_mutex_t **mutex) {
    if (g_mock_mutex_create_countdown >= 0) {
        if (g_mock_mutex_create_countdown == 0) return C_REST_ERROR_GENERIC;
        g_mock_mutex_create_countdown--;
    }
    return c_rest_mutex_create(mutex);
}

#define c_rest_mutex_create mock_c_rest_mutex_create

#define c_rest_mutex_lock mock_c_rest_mutex_lock
#define c_rest_mutex_unlock mock_c_rest_mutex_unlock
#define c_rest_mutex_destroy mock_c_rest_mutex_destroy
#define c_rest_hashmap_destroy mock_c_rest_hashmap_destroy

#define c_rest_rate_limiter_init test_c_rest_rate_limiter_init
#define c_rest_rate_limiter_check test_c_rest_rate_limiter_check
#define c_rest_rate_limiter_destroy test_c_rest_rate_limiter_destroy

#include "../src/c_rest_rate_limit.c"

#undef c_rest_mutex_create
#undef c_rest_mutex_lock
#undef c_rest_mutex_unlock
#undef c_rest_mutex_destroy
#undef c_rest_hashmap_destroy

/* clang-format on */

static void reset_mocks(void *data) {
  (void)data;
  g_mock_mutex_create_countdown = -1;
  g_mock_mutex_lock_countdown = -1;
  g_mock_mutex_unlock_countdown = -1;
  g_mock_mutex_destroy_countdown = -1;
  g_mock_hashmap_destroy_countdown = -1;
}

TEST test_rate_limit_error_branches(void) {
  c_rest_rate_limiter rl = {0};
  struct c_rest_request req = {0};
  size_t tokens = 0;

  /* init: fails on hashmap_destroy (via mutex_create fail) */
  g_mock_mutex_create_countdown = 0;
  g_mock_hashmap_destroy_countdown = 0;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_rate_limiter_init(&rl, 10, 60, 1));

  test_c_rest_rate_limiter_init(&rl, 10, 60, 1);

  /* check: fails on mutex_lock */
  g_mock_mutex_lock_countdown = 0;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_rate_limiter_check(&rl, "127.0.0.1", 1, &tokens));

  /* check: fails on mutex_unlock */
  g_mock_mutex_unlock_countdown = 0;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_rate_limiter_check(&rl, "127.0.0.1", 1, &tokens));

  /* check: fails on mutex_unlock (the other one) */
  g_mock_mutex_unlock_countdown = 0;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_rate_limiter_check(&rl, "127.0.0.1", 1, &tokens));

  /* destroy: fails on mutex_lock */
  g_mock_mutex_lock_countdown = 0;
  ASSERT_EQ(C_REST_ERROR_GENERIC, test_c_rest_rate_limiter_destroy(&rl));

  /* destroy: fails on hashmap_destroy */
  g_mock_hashmap_destroy_countdown = 0;
  ASSERT_EQ(C_REST_ERROR_GENERIC, test_c_rest_rate_limiter_destroy(&rl));

  /* destroy: fails on mutex_unlock */
  g_mock_mutex_unlock_countdown = 0;
  ASSERT_EQ(C_REST_ERROR_GENERIC, test_c_rest_rate_limiter_destroy(&rl));

  /* destroy: fails on mutex_destroy */
  g_mock_mutex_destroy_countdown = 0;
  ASSERT_EQ(C_REST_ERROR_GENERIC, test_c_rest_rate_limiter_destroy(&rl));

  test_c_rest_rate_limiter_destroy(&rl);
  PASS();
}

SUITE(rate_limit_mock_suite) {
  SET_SETUP(reset_mocks, NULL);
  RUN_TEST(test_rate_limit_error_branches);
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
  GREATEST_MAIN_BEGIN();
  RUN_SUITE(rate_limit_mock_suite);
  GREATEST_MAIN_END();
}
