/* clang-format off */
#include "c_rest_rate_limit.h"
#include "c_rest_mem.h"
#include <stdio.h>
#include <string.h>
/* clang-format on */

#define ASSERT_EQ(expected, actual)                                            \
  do {                                                                         \
    if ((expected) != (actual)) {                                              \
      printf("ASSERT_EQ failed at %s:%d: expected %d, got %d\n", __FILE__,     \
             __LINE__, (int)(expected), (int)(actual));                        \
      return 1;                                                                \
    }                                                                          \
  } while (0)

static int test_rate_limiter_init_destroy(void) {
  c_rest_rate_limiter limiter;
  int ret;

  ret = c_rest_rate_limiter_init(&limiter, 10, 1, 100);
  ASSERT_EQ(0, ret);
  ASSERT_EQ(10, limiter.config.capacity);
  ASSERT_EQ(1, limiter.config.fill_rate);
  ASSERT_EQ(1, limiter.initialized);

  ret = c_rest_rate_limiter_destroy(&limiter);
  ASSERT_EQ(0, ret);
  ASSERT_EQ(0, limiter.initialized);

  return 0;
}

static int test_rate_limiter_allow_deny(void) {
  c_rest_rate_limiter limiter;
  int ret;
  size_t remaining;

  /* Capacity 2, fill rate 0 per second (no auto-refill for this test) */
  ret = c_rest_rate_limiter_init(&limiter, 2, 0, 100);
  ASSERT_EQ(0, ret);

  /* 1st request, should be allowed */
  ret = c_rest_rate_limiter_check(&limiter, "127.0.0.1", 1, &remaining);
  ASSERT_EQ(0, ret);
  ASSERT_EQ(1, remaining);

  /* 2nd request, should be allowed */
  ret = c_rest_rate_limiter_check(&limiter, "127.0.0.1", 1, &remaining);
  ASSERT_EQ(0, ret);
  ASSERT_EQ(0, remaining);

  /* 3rd request, should be denied */
  ret = c_rest_rate_limiter_check(&limiter, "127.0.0.1", 1, &remaining);
  ASSERT_EQ(1, ret); /* Rate limited */
  ASSERT_EQ(0, remaining);

  /* Request from another IP, should be allowed */
  ret = c_rest_rate_limiter_check(&limiter, "192.168.1.1", 1, &remaining);
  ASSERT_EQ(0, ret);
  ASSERT_EQ(1, remaining);

  ret = c_rest_rate_limiter_destroy(&limiter);
  ASSERT_EQ(0, ret);

  return 0;
}

static int test_rate_limiter_edge_cases(void) {
  c_rest_rate_limiter limiter;
  int ret;
  size_t remaining;

  /* Null pointer checks */
  ret = c_rest_rate_limiter_init(NULL, 10, 1, 100);
  ASSERT_EQ(1, ret);

  ret = c_rest_rate_limiter_check(NULL, "ip", 1, &remaining);
  ASSERT_EQ(1, ret);

  ret = c_rest_rate_limiter_init(&limiter, 10, 1, 100);
  ASSERT_EQ(0, ret);

  ret = c_rest_rate_limiter_check(&limiter, NULL, 1, &remaining);
  ASSERT_EQ(1, ret);

  ret = c_rest_rate_limiter_check(&limiter, "ip", 1, NULL);
  ASSERT_EQ(1, ret);

  ret = c_rest_rate_limiter_destroy(&limiter);
  ASSERT_EQ(0, ret);

  return 0;
}

int test_rate_limiting_throttling_middleware(void) {
  int failed = 0;

  if (test_rate_limiter_init_destroy() != 0) {
    printf("test_rate_limiter_init_destroy FAILED\n");
    failed++;
  }
  if (test_rate_limiter_allow_deny() != 0) {
    printf("test_rate_limiter_allow_deny FAILED\n");
    failed++;
  }
  if (test_rate_limiter_edge_cases() != 0) {
    printf("test_rate_limiter_edge_cases FAILED\n");
    failed++;
  }

  return failed > 0 ? 1 : 0;
}