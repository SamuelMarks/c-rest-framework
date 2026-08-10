#ifdef __unix__
int usleep(unsigned int);
#endif
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
#include <unistd.h>
#endif
/* clang-format on */

static void sleep_ms(int ms) {
#ifdef _WIN32
  Sleep(ms);
#else
  usleep(ms * 1000);
#endif
}

#define CHECK_EQ(expected, actual) failed += ((expected) != (actual))

static int test_rate_limiter_init_destroy(void) {
  c_rest_rate_limiter limiter;
  int ret;
  int failed = 0;

  ret = c_rest_rate_limiter_init(&limiter, 10, 1, 100);
  CHECK_EQ(0, ret);
  CHECK_EQ(10, limiter.config.capacity);
  CHECK_EQ(1, limiter.config.fill_rate);
  CHECK_EQ(1, limiter.initialized);

  ret = c_rest_rate_limiter_destroy(&limiter);
  CHECK_EQ(0, ret);
  CHECK_EQ(0, limiter.initialized);

  return failed;
}

static int test_rate_limiter_allow_deny(void) {
  c_rest_rate_limiter limiter;
  int ret;
  size_t remaining;
  int failed = 0;

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

  return failed;
}

static int test_rate_limiter_edge_cases(void) {
  c_rest_rate_limiter limiter;
  int ret;
  size_t remaining;
  int failed = 0;

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

  return failed;
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

static int test_rate_limiter_malloc(void) {
  c_rest_rate_limiter limiter;
  int ret;
  extern void *(*g_crf_malloc_hook)(size_t);
  int i;
  size_t remaining;

  g_crf_malloc_hook = fail_malloc_n;
  for (i = 0; i < 5; i++) {
    g_malloc_fail_after = i;
    ret = c_rest_rate_limiter_init(&limiter, 10, 1, 100);
    if (ret == C_REST_OK) {
      c_rest_rate_limiter_destroy(&limiter);
    }
  }

  c_rest_rate_limiter_init(&limiter, 10, 1, 100);
  for (i = 0; i < 5; i++) {
    g_malloc_fail_after = i;
    ret = c_rest_rate_limiter_check(&limiter, "127.0.0.1", 1, &remaining);
  }
  c_rest_rate_limiter_destroy(&limiter);

  g_crf_malloc_hook = NULL;
  g_malloc_fail_after = -1;
  return 0;
}

int test_rate_limiting_throttling_middleware(void) {
  int failed = 0;

  failed += test_rate_limiter_init_destroy();
  failed += test_rate_limiter_allow_deny();
  failed += test_rate_limiter_edge_cases();
  failed += test_rate_limiter_malloc();

  return failed > 0 ? 1 : 0;
}
