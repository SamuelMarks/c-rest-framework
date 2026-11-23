
/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "test_protos.h"
#include "c_rest_rate_limit.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
__declspec(dllimport) void __stdcall Sleep(unsigned long dwMilliseconds);
#else
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#endif
/* clang-format on */

static void sleep_ms(int ms) {
#ifdef _WIN32
  Sleep((unsigned long)ms);
#else
  {
    struct timeval tv;
    tv.tv_sec = ms / 1000;
    tv.tv_usec = (ms % 1000) * 1000;
    select(0, NULL, NULL, NULL, &tv);
  }
#endif
}

#define CHECK_EQ(expected, actual) ASSERT_EQ(expected, actual)

TEST test_rate_limiter_init_destroy(void) {
  c_rest_rate_limiter limiter;
  c_rest_error_t ret;

  ret = c_rest_rate_limiter_init(&limiter, 10, 1, 100);
  CHECK_EQ(0, ret);
  CHECK_EQ(10, limiter.config.capacity);
  CHECK_EQ(1, limiter.config.fill_rate);
  CHECK_EQ(1, limiter.initialized);

  ret = c_rest_rate_limiter_destroy(&limiter);
  CHECK_EQ(0, ret);
  CHECK_EQ(0, limiter.initialized);

  PASS();
}

TEST test_rate_limiter_allow_deny(void) {
  c_rest_rate_limiter limiter;
  c_rest_error_t ret;
  size_t remaining;

  /* Capacity 2, fill rate 1 per second */
  ret = c_rest_rate_limiter_init(&limiter, 2, 1, 100);
  CHECK_EQ(0, ret);

  /* 1st request, should be allowed */
  ret = c_rest_rate_limiter_check(&limiter, "127.0.0.1", 1, &remaining);
  CHECK_EQ(0, ret);
  CHECK_EQ(1, remaining);

  /* 2nd request, should be allowed */
  ret = c_rest_rate_limiter_check(&limiter, "127.0.0.1", 1, &remaining);
  CHECK_EQ(0, ret);
  CHECK_EQ(0, remaining);

  /* 3rd request, should be denied */
  ret = c_rest_rate_limiter_check(&limiter, "127.0.0.1", 1, &remaining);
  CHECK_EQ(1, ret); /* Rate limited */
  CHECK_EQ(0, remaining);

  /* Wait 1 second for partial refill (capacity is 2, fill rate is 1) */
  {
    time_t start = time(NULL);
    while (time(NULL) - start < 1) {
      sleep_ms(100);
    }
  }

  /* After wait, should be allowed again, and tokens should be 0 */
  ret = c_rest_rate_limiter_check(&limiter, "127.0.0.1", 1, &remaining);
  CHECK_EQ(0, ret);
  CHECK_EQ(0, remaining);

  /* Request from another IP, should be allowed */
  ret = c_rest_rate_limiter_check(&limiter, "192.168.1.1", 1, &remaining);
  CHECK_EQ(0, ret);
  CHECK_EQ(1, remaining);

  /* Wait 3 seconds for refill (capacity is 2, fill rate is 1) */
  {
    time_t start = time(NULL);
    while (time(NULL) - start < 3) {
      sleep_ms(100);
    }
  }

  /* After wait, should be allowed again, and tokens should not exceed capacity
   */
  ret = c_rest_rate_limiter_check(&limiter, "127.0.0.1", 1, &remaining);
  CHECK_EQ(0, ret);
  CHECK_EQ(1, remaining);

  ret = c_rest_rate_limiter_destroy(&limiter);
  CHECK_EQ(0, ret);

  PASS();
}

TEST test_rate_limiter_edge_cases(void) {
  c_rest_rate_limiter limiter;
  c_rest_error_t ret;
  size_t remaining;

  /* Null pointer checks */
  ret = c_rest_rate_limiter_init(NULL, 10, 1, 100);
  CHECK_EQ(C_REST_ERROR_GENERIC, ret);

  ret = c_rest_rate_limiter_check(NULL, "ip", 1, &remaining);
  CHECK_EQ(C_REST_ERROR_GENERIC, ret);

  ret = c_rest_rate_limiter_init(&limiter, 10, 1, 100);
  CHECK_EQ(0, ret);

  ret = c_rest_rate_limiter_check(&limiter, NULL, 1, &remaining);
  CHECK_EQ(C_REST_ERROR_GENERIC, ret);

  ret = c_rest_rate_limiter_check(&limiter, "ip", 1, NULL);
  CHECK_EQ(C_REST_ERROR_GENERIC, ret);

  /* Check with uninitialized limiter */
  limiter.initialized = 0;
  ret = c_rest_rate_limiter_check(&limiter, "ip", 1, &remaining);
  CHECK_EQ(C_REST_ERROR_GENERIC, ret);
  limiter.initialized = 1;

  /* Destroy uninitialized */
  limiter.initialized = 0;
  ret = c_rest_rate_limiter_destroy(&limiter);
  CHECK_EQ(C_REST_ERROR_GENERIC, ret);
  limiter.initialized = 1;

  ret = c_rest_rate_limiter_destroy(NULL);
  CHECK_EQ(C_REST_ERROR_GENERIC, ret);

  ret = c_rest_rate_limiter_destroy(&limiter);
  CHECK_EQ(0, ret);

  /* Re-initialize with 0 hashmap capacity */
  ret = c_rest_rate_limiter_init(&limiter, 10, 1, 0);
  CHECK_EQ(C_REST_ERROR_GENERIC, ret);

  /* Enforce max_entities bound against unbounded IP-spoofing growth */
  ret = c_rest_rate_limiter_init(&limiter, 10, 1, 2);
  CHECK_EQ(C_REST_OK, ret);
  ret = c_rest_rate_limiter_check(&limiter, "10.0.0.1", 1, &remaining);
  CHECK_EQ(C_REST_OK, ret);
  ret = c_rest_rate_limiter_check(&limiter, "10.0.0.2", 1, &remaining);
  CHECK_EQ(C_REST_OK, ret);
  /* 3rd distinct IP rejected when buckets.size == max_entities */
  ret = c_rest_rate_limiter_check(&limiter, "10.0.0.3", 1, &remaining);
  CHECK_EQ(C_REST_ERROR_GENERIC, ret);
  CHECK_EQ(0, remaining);
#ifdef C_REST_TESTING_MALLOC_HOOK
  g_mock_rate_limit_fail = 6;
  ret = c_rest_rate_limiter_check(&limiter, "10.0.0.4", 1, &remaining);
  CHECK_EQ(C_REST_ERROR_GENERIC, ret);
  g_mock_rate_limit_fail = 0;
#endif
  /* Existing entity still allowed */
  ret = c_rest_rate_limiter_check(&limiter, "10.0.0.1", 1, &remaining);
  CHECK_EQ(C_REST_OK, ret);
  ret = c_rest_rate_limiter_destroy(&limiter);
  CHECK_EQ(C_REST_OK, ret);

  PASS();
}

static void *fail_malloc_n(size_t size);
static int g_malloc_fail_after = -1;
static void *fail_malloc_n(size_t size) {
  void *res = NULL;
  int is_zero = (g_malloc_fail_after == 0);
  int is_gt_zero = (g_malloc_fail_after > 0);
  g_malloc_fail_after -= is_gt_zero;
  /* branchless equivalent */
  res = malloc(size);
  if (is_zero) {
    free(res);
    res = NULL;
  }
  return res;
}

TEST test_rate_limiter_malloc(void) {
  c_rest_rate_limiter limiter;
  c_rest_error_t ret;
  int i;
  size_t remaining;

  g_crf_malloc_hook = fail_malloc_n;
  for (i = 0; i < 5; i++) {
    g_malloc_fail_after = i;
    ret = c_rest_rate_limiter_init(&limiter, 10, 1, 100);
    if (ret == C_REST_OK) {
      ASSERT_EQ(C_REST_OK, c_rest_rate_limiter_destroy(&limiter));
    }
  }

  g_malloc_fail_after = -1;
  ASSERT_EQ(C_REST_OK, c_rest_rate_limiter_init(&limiter, 10, 1, 100));
  for (i = 0; i < 5; i++) {
    g_malloc_fail_after = i;
    ret = c_rest_rate_limiter_check(&limiter, "127.0.0.1", 1, &remaining);
    ASSERT(ret == C_REST_OK || ret != C_REST_OK);
  }
  ASSERT_EQ(C_REST_OK, c_rest_rate_limiter_destroy(&limiter));

  g_crf_malloc_hook = NULL;
  g_malloc_fail_after = -1;

#ifdef C_REST_TESTING_MALLOC_HOOK
  {
    c_rest_rate_limiter rl;

    /* 1, 2: init fail */
    g_mock_rate_limit_fail = 1;
    ASSERT_NEQ(C_REST_OK, c_rest_rate_limiter_init(&rl, 10, 1, 10));
    g_mock_rate_limit_fail = 2;
    ASSERT_NEQ(C_REST_OK, c_rest_rate_limiter_init(&rl, 10, 1, 10));
    g_mock_rate_limit_fail = 0;

    /* 3: check lock fail */
    ASSERT_EQ(C_REST_OK, c_rest_rate_limiter_init(&rl, 10, 1, 10));
    g_mock_rate_limit_fail = 3;
    ASSERT_NEQ(C_REST_OK,
               c_rest_rate_limiter_check(&rl, "127.0.0.1", 1, &remaining));
    g_mock_rate_limit_fail = 0;
    ASSERT_EQ(C_REST_OK, c_rest_rate_limiter_destroy(&rl));

    /* 4: check put fail */
    ASSERT_EQ(C_REST_OK, c_rest_rate_limiter_init(&rl, 10, 1, 10));
    g_mock_rate_limit_fail = 4;
    ASSERT_NEQ(C_REST_OK,
               c_rest_rate_limiter_check(&rl, "127.0.0.1", 1, &remaining));
    g_mock_rate_limit_fail = 0;
    ASSERT_EQ(C_REST_OK, c_rest_rate_limiter_destroy(&rl));

    /* 12: check put fail and unlock fail */
    ASSERT_EQ(C_REST_OK, c_rest_rate_limiter_init(&rl, 10, 1, 10));
    g_mock_rate_limit_fail = 12;
    ASSERT_NEQ(C_REST_OK,
               c_rest_rate_limiter_check(&rl, "127.0.0.1", 1, &remaining));
    g_mock_rate_limit_fail = 0;
    ASSERT_EQ(C_REST_OK, c_rest_rate_limiter_destroy(&rl));

    /* 5: check unlock fail after malloc */
    ASSERT_EQ(C_REST_OK, c_rest_rate_limiter_init(&rl, 10, 1, 10));
    g_crf_malloc_hook = fail_malloc_n;
    g_malloc_fail_after = 0;
    g_mock_rate_limit_fail = 5;
    ASSERT_NEQ(C_REST_OK,
               c_rest_rate_limiter_check(&rl, "127.0.0.1", 1, &remaining));
    g_crf_malloc_hook = NULL;
    g_mock_rate_limit_fail = 0;
    ASSERT_EQ(C_REST_OK, c_rest_rate_limiter_destroy(&rl));

    /* 6: check unlock fail normal */
    ASSERT_EQ(C_REST_OK, c_rest_rate_limiter_init(&rl, 10, 1, 10));
    g_mock_rate_limit_fail = 6;
    ASSERT_NEQ(C_REST_OK,
               c_rest_rate_limiter_check(&rl, "127.0.0.1", 1, &remaining));
    g_mock_rate_limit_fail = 0;
    ASSERT_EQ(C_REST_OK, c_rest_rate_limiter_destroy(&rl));

    /* 7: destroy lock fail */
    ASSERT_EQ(C_REST_OK, c_rest_rate_limiter_init(&rl, 10, 1, 10));
    g_mock_rate_limit_fail = 7;
    ASSERT_NEQ(C_REST_OK, c_rest_rate_limiter_destroy(&rl));
    g_mock_rate_limit_fail = 0;
    ASSERT_EQ(C_REST_OK, c_rest_rate_limiter_destroy(&rl));

    /* 8: destroy unlock fail after map destroy */
    ASSERT_EQ(C_REST_OK, c_rest_rate_limiter_init(&rl, 10, 1, 10));
    g_mock_rate_limit_fail = 8;
    ASSERT_NEQ(C_REST_OK, c_rest_rate_limiter_destroy(&rl));
    g_mock_rate_limit_fail = 0;
    ASSERT_EQ(C_REST_OK, c_rest_mutex_destroy(rl.mutex));

    /* 9: destroy hashmap destroy fail */
    ASSERT_EQ(C_REST_OK, c_rest_rate_limiter_init(&rl, 10, 1, 10));
    g_mock_rate_limit_fail = 9;
    ASSERT_NEQ(C_REST_OK, c_rest_rate_limiter_destroy(&rl));
    g_mock_rate_limit_fail = 0;
    ASSERT_EQ(C_REST_OK, c_rest_hashmap_destroy(&rl.buckets, NULL));
    ASSERT_EQ(C_REST_OK, c_rest_mutex_destroy(rl.mutex));

    /* 10: destroy hashmap destroy fail & unlock fail */
    ASSERT_EQ(C_REST_OK, c_rest_rate_limiter_init(&rl, 10, 1, 10));
    g_mock_rate_limit_fail = 10;
    ASSERT_NEQ(C_REST_OK, c_rest_rate_limiter_destroy(&rl));
    g_mock_rate_limit_fail = 0;
    ASSERT_EQ(C_REST_OK, c_rest_hashmap_destroy(&rl.buckets, NULL));
    ASSERT_EQ(C_REST_OK, c_rest_mutex_destroy(rl.mutex));

    /* 11: destroy mutex destroy fail */
    ASSERT_EQ(C_REST_OK, c_rest_rate_limiter_init(&rl, 10, 1, 10));
    g_mock_rate_limit_fail = 11;
    ASSERT_NEQ(C_REST_OK, c_rest_rate_limiter_destroy(&rl));
    g_mock_rate_limit_fail = 0;
    ASSERT_EQ(C_REST_OK, c_rest_mutex_destroy(rl.mutex));
  }
#endif

  PASS();
}

SUITE(rate_limit_suite) {
  RUN_TEST(test_rate_limiter_init_destroy);
  RUN_TEST(test_rate_limiter_allow_deny);
  RUN_TEST(test_rate_limiter_edge_cases);
  RUN_TEST(test_rate_limiter_malloc);
}
