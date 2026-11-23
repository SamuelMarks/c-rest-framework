/* clang-format off */
#include "c_rest_error.h"
#include "greatest.h"
#include "greatest_clean.h"
#include <string.h>

#undef C_REST_EXPORT
#define C_REST_EXPORT

#include "c_rest_http23.h"
#include "c_rest_request.h"

static int g_mock_req_cleanup_countdown = -1;

static c_rest_error_t mock_c_rest_request_cleanup(struct c_rest_request *req) {
  if (g_mock_req_cleanup_countdown >= 0) {
    if (g_mock_req_cleanup_countdown == 0)
      return C_REST_ERROR_GENERIC;
    g_mock_req_cleanup_countdown--;
  }
  return c_rest_request_cleanup(req);
}

#define c_rest_request_cleanup mock_c_rest_request_cleanup
#define c_rest_http23_ctx_destroy test_c_rest_http23_ctx_destroy

NO_DISCARD c_rest_error_t
test_c_rest_http23_ctx_destroy(c_rest_http23_ctx_t *ctx);

#include "../src/c_rest_http23.c"

#undef c_rest_request_cleanup

/* clang-format on */

static void reset_mocks(void *data) {
  (void)data;
  g_mock_req_cleanup_countdown = -1;
}

TEST test_http23_error_branches(void) {
  c_rest_http23_ctx_t *ctx = NULL;
  /* RFC 9113 9-byte HEADERS frame (stream 1, END_HEADERS|END_STREAM, 0x82=GET,
   * 0x84=/) */
  static const unsigned char h2_get_root[11] = {0, 0, 2, 0x01U, 0x05U, 0,
                                                0, 0, 1, 0x82U, 0x84U};
  size_t consumed = 0;

  /* Test destroy with NULL ctx */
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG, test_c_rest_http23_ctx_destroy(NULL));

  /* Test destroy after parsing real RFC 9113 frame when cleanup succeeds */
  ASSERT_EQ(C_REST_OK, c_rest_http23_ctx_init(C_REST_PROTOCOL_HTTP2, &ctx));
  ASSERT_EQ(C_REST_OK, c_rest_http23_process(ctx, (const char *)h2_get_root,
                                             sizeof(h2_get_root), &consumed));
  ASSERT_EQ(sizeof(h2_get_root), consumed);
  g_mock_req_cleanup_countdown = 1;
  ASSERT_EQ(C_REST_OK, test_c_rest_http23_ctx_destroy(ctx));

  /* Test destroy error when request cleanup fails */
  ASSERT_EQ(C_REST_OK, c_rest_http23_ctx_init(C_REST_PROTOCOL_HTTP2, &ctx));
  g_mock_req_cleanup_countdown = 0;
  ASSERT_EQ(C_REST_ERROR_GENERIC, test_c_rest_http23_ctx_destroy(ctx));
  reset_mocks(NULL);
  ASSERT_EQ(C_REST_OK, test_c_rest_http23_ctx_destroy(ctx));

  PASS();
}

SUITE(http23_mock_suite) {
  SET_SETUP(reset_mocks, NULL);
  RUN_TEST(test_http23_error_branches);
}
