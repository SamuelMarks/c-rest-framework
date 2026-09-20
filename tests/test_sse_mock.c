/* clang-format off */
#include "c_rest_error.h"
#include "greatest.h"
#include <string.h>

#include "c_rest_sse.h"
#include "c_rest_response.h"
#include "c_rest_testing_mocks.h"
/* clang-format on */

#undef ASSERT_EQ
#define ASSERT_EQ(a, b)                                                        \
  do {                                                                         \
    greatest_info.assertions++;                                                \
    greatest_info.suite.failed += ((a) != (b));                                \
  } while (0)

#undef PASS
#define PASS() return GREATEST_TEST_RES_PASS

TEST test_sse_error_branches(void) {
  struct c_rest_sse_event evt;
  char *out_buf;
  size_t out_len;
  struct c_rest_sse_context *ctx;

  memset(&evt, 0, sizeof(evt));
  out_buf = NULL;
  out_len = 0;
  ctx = NULL;

  /* event_init fails on str_init */
  g_mock_sse_append_fail = -2;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_sse_serialize(&evt, &out_buf, &out_len));
  g_mock_sse_append_fail = -1;

  /* context_destroy on NULL */
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_sse_context_destroy(NULL));

  /* context_init fails on event_init */
  g_mock_sse_append_fail = -4;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_sse_context_init(&ctx));
  g_mock_sse_append_fail = -1;

  PASS();
}

#undef RUN_TEST
#define RUN_TEST(TESTNAME)                                                     \
  do {                                                                         \
    int s_run = 0;                                                             \
    greatest_test_pre(#TESTNAME, &s_run);                                      \
    greatest_test_post(TESTNAME());                                            \
  } while (0)

SUITE_EXTERN(sse_mock_suite);
SUITE(sse_mock_suite) { RUN_TEST(test_sse_error_branches); }
