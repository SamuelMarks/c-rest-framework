/* clang-format off */
#include "c_rest_error.h"
#include "greatest.h"
#include <string.h>

#include "c_rest_sse.h"
#include "c_rest_response.h"
#include "c_rest_mem.h"
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

#ifdef C_REST_TESTING_MALLOC_HOOK
int g_fail_malloc_at = 0;
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
#endif

TEST test_sse_error_branches(void) {
  struct c_rest_sse_event evt;
  char *out_buf;
  size_t out_len;
  struct c_rest_sse_context *ctx;
#ifdef C_REST_TESTING_MALLOC_HOOK
  struct c_rest_sse_event src_evt;
  struct c_rest_sse_event dst_evt;
  struct c_rest_response res;
  c_rest_error_t rrc;
#endif

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

  /* serialize error + string destroy error */
  evt.id = (char *)"test_id";
  g_mock_sse_append_fail = -5;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_sse_serialize(&evt, &out_buf, &out_len));
  g_mock_sse_append_fail = -1;
  evt.id = NULL;

  /* test mock_append with g_mock_sse_append_fail > 0 */
  evt.data = (char *)"data";
  g_mock_sse_append_fail = 10;
  ASSERT_EQ(C_REST_OK, c_rest_sse_serialize(&evt, &out_buf, &out_len));
  g_mock_sse_append_fail = -1;
  evt.data = NULL;
  C_REST_FREE(out_buf);
  out_buf = NULL;

#ifdef C_REST_TESTING_MALLOC_HOOK
  memset(&src_evt, 0, sizeof(src_evt));
  memset(&dst_evt, 0, sizeof(dst_evt));
  src_evt.id = (char *)"id1";
  src_evt.event = (char *)"event1";

  g_fail_malloc_at = -1;
  fail_malloc_n(0);
  g_crf_malloc_hook = fail_malloc_n;
  g_fail_malloc_at = 2;
  g_mock_sse_append_fail = -3;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_sse_event_clone(&src_evt, &dst_evt));
  g_crf_malloc_hook = NULL;
  g_fail_malloc_at = 0;
  g_mock_sse_append_fail = -1;

  memset(&res, 0, sizeof(res));
  g_mock_res_status_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_sse_init_response(&res));
  g_mock_res_status_fail = 0;
  rrc = c_rest_response_cleanup(&res);
  ASSERT_EQ(C_REST_OK, rrc);
#endif

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
