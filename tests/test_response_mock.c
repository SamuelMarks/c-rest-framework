/* clang-format off */
#include "c_rest_error.h"
#include "greatest.h"
#include "greatest_clean.h"
#include <string.h>

#include "c_rest_platform.h"
#include "c_rest_tls.h"
#include "c_rest_modality.h"
#include "c_rest_request.h"
#include "c_rest_response.h"
#include "c_rest_testing_mocks.h"
/* clang-format on */

static void *fail_malloc(size_t size) {
  (void)size;
  return NULL;
}

static void reset_mocks(void *data) {
  (void)data;
#ifdef C_REST_TESTING_MALLOC_HOOK
  g_mock_socket_fail = 0;
  g_mock_tls_fail = 0;
  g_mock_res_status_fail = 0;
  g_crf_malloc_hook = NULL;
#endif
}

TEST test_write_chunk_socket_fail_payload(void) {
  struct c_rest_response res;
  struct c_rest_connection_context ctx;
  memset(&res, 0, sizeof(res));
  memset(&ctx, 0, sizeof(ctx));
  res.context = &ctx;
  res.is_chunked = 1;
  res.headers_sent = 1;
  ctx.sock = 123;

#ifdef C_REST_TESTING_MALLOC_HOOK
  g_mock_socket_fail = 202; /* Succeed on chunk header, fail on payload */
#endif
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_response_write_chunk(&res, "abc", 3));
  PASS();
}

TEST test_write_chunk_socket_fail_trailer(void) {
  struct c_rest_response res;
  struct c_rest_connection_context ctx;
  memset(&res, 0, sizeof(res));
  memset(&ctx, 0, sizeof(ctx));
  res.context = &ctx;
  res.is_chunked = 1;
  res.headers_sent = 1;
  ctx.sock = 123;

#ifdef C_REST_TESTING_MALLOC_HOOK
  g_mock_socket_fail = 203; /* Succeed on header, payload, fail on trailer */
#endif
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_response_write_chunk(&res, "abc", 3));
  PASS();
}

TEST test_write_chunk_tls_fail_payload(void) {
  struct c_rest_response res;
  struct c_rest_connection_context ctx;
  memset(&res, 0, sizeof(res));
  memset(&ctx, 0, sizeof(ctx));
  res.context = &ctx;
  res.is_chunked = 1;
  res.headers_sent = 1;
  ctx.tls_conn = (struct c_rest_tls_connection *)1;

#ifdef C_REST_TESTING_MALLOC_HOOK
  g_mock_tls_fail = 4; /* Succeed on chunk header, fail on payload */
#endif
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_response_write_chunk(&res, "abc", 3));
  PASS();
}

TEST test_write_chunk_tls_fail_trailer(void) {
  struct c_rest_response res;
  struct c_rest_connection_context ctx;
  memset(&res, 0, sizeof(res));
  memset(&ctx, 0, sizeof(ctx));
  res.context = &ctx;
  res.is_chunked = 1;
  res.headers_sent = 1;
  ctx.tls_conn = (struct c_rest_tls_connection *)1;

#ifdef C_REST_TESTING_MALLOC_HOOK
  g_mock_tls_fail = 5; /* Succeed on header, payload, fail on trailer */
#endif
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_response_write_chunk(&res, "abc", 3));
  PASS();
}

TEST test_response_send_tls_fail(void) {
  struct c_rest_response res;
  struct c_rest_connection_context ctx;
  memset(&res, 0, sizeof(res));
  memset(&ctx, 0, sizeof(ctx));
  res.context = &ctx;
  ctx.tls_conn = (struct c_rest_tls_connection *)1;
  res.body = "body";
  res.body_len = 4;

#ifdef C_REST_TESTING_MALLOC_HOOK
  /* Header write fails */
  g_mock_tls_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_response_send(&res));
  g_mock_tls_fail = 0;

  /* Body write fails */
  g_mock_tls_fail = 4;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_response_send(&res));
  g_mock_tls_fail = 0;
#endif
  PASS();
}

TEST test_response_send_socket_fail_body(void) {
  struct c_rest_response res;
  struct c_rest_connection_context ctx;
  memset(&res, 0, sizeof(res));
  memset(&ctx, 0, sizeof(ctx));
  res.context = &ctx;
  ctx.sock = 123;
  res.body = "body";
  res.body_len = 4;

#ifdef C_REST_TESTING_MALLOC_HOOK
  /* 1st send (header) succeeds, 2nd send (body) fails */
  g_mock_socket_fail = 202;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_response_send(&res));
  g_mock_socket_fail = 0;
#endif
  PASS();
}

TEST test_response_send_socket_success_body(void) {
  struct c_rest_response res;
  struct c_rest_connection_context ctx;
  memset(&res, 0, sizeof(res));
  memset(&ctx, 0, sizeof(ctx));
  res.context = &ctx;
  ctx.sock = 123;
  res.body = "body";
  res.body_len = 4;

#ifdef C_REST_TESTING_MALLOC_HOOK
  g_mock_socket_fail = 200;
  ASSERT_EQ(C_REST_OK, c_rest_response_send(&res));
  g_mock_socket_fail = 0;
#endif
  PASS();
}

TEST test_response_send_socket_body_len_zero(void) {
  struct c_rest_response res;
  struct c_rest_connection_context ctx;
  memset(&res, 0, sizeof(res));
  memset(&ctx, 0, sizeof(ctx));
  res.context = &ctx;
  ctx.sock = 123;
  res.body = "body";
  res.body_len = 0;

#ifdef C_REST_TESTING_MALLOC_HOOK
  g_mock_socket_fail = 200;
  ASSERT_EQ(C_REST_OK, c_rest_response_send(&res));
  g_mock_socket_fail = 0;
#endif
  PASS();
}

TEST test_write_chunk_headers_not_sent_success(void) {
  struct c_rest_response res;
  struct c_rest_connection_context ctx;
  memset(&res, 0, sizeof(res));
  memset(&ctx, 0, sizeof(ctx));
  res.context = &ctx;
  ctx.sock = 123;
  res.headers_sent = 0;

#ifdef C_REST_TESTING_MALLOC_HOOK
  g_mock_socket_fail = 200;
  ASSERT_EQ(C_REST_OK, c_rest_response_write_chunk(&res, "abc", 3));
  g_mock_socket_fail = 0;
#endif
  PASS();
}

TEST test_write_chunk_headers_not_sent_fail(void) {
  struct c_rest_response res;
  struct c_rest_connection_context ctx;
  memset(&res, 0, sizeof(res));
  memset(&ctx, 0, sizeof(ctx));
  res.context = &ctx;
  ctx.sock = 123;
  res.headers_sent = 0;

#ifdef C_REST_TESTING_MALLOC_HOOK
  /* fails on response_send when sending headers */
  g_mock_socket_fail = 201;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_response_write_chunk(&res, "abc", 3));
  g_mock_socket_fail = 0;

  /* fails on response_set_header when malloc fails */
  g_crf_malloc_hook = fail_malloc;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_response_write_chunk(&res, "abc", 3));
  g_crf_malloc_hook = NULL;
#endif
  PASS();
}

TEST test_response_etag_status_fail(void) {
  struct c_rest_request req;
  struct c_rest_response res;
  struct c_rest_header hdr;
  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));
  memset(&hdr, 0, sizeof(hdr));
  hdr.key = "If-None-Match";
  hdr.value = "etag123";
  req.headers = &hdr;

#ifdef C_REST_TESTING_MALLOC_HOOK
  g_mock_res_status_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_response_check_etag(&req, &res, "etag123"));
  g_mock_res_status_fail = 0;
#endif
  c_rest_response_cleanup(&res);
  PASS();
}

TEST test_response_redirect_status_fail(void) {
  struct c_rest_response res;
  memset(&res, 0, sizeof(res));

#ifdef C_REST_TESTING_MALLOC_HOOK
  g_mock_res_status_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_response_redirect(&res, "/dest", 302));
  g_mock_res_status_fail = 0;
#endif
  c_rest_response_cleanup(&res);
  PASS();
}

TEST test_response_oauth2_error_status_fail(void) {
  struct c_rest_response res;
  memset(&res, 0, sizeof(res));

#ifdef C_REST_TESTING_MALLOC_HOOK
  g_mock_res_status_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_response_oauth2_error(&res, "invalid_request", "desc"));
  g_mock_res_status_fail = 0;
#endif
  c_rest_response_cleanup(&res);
  PASS();
}

TEST test_response_serialize_status_text_fail(void) {
  struct c_rest_response res;
  char *buf = NULL;
  size_t len = 0;
  memset(&res, 0, sizeof(res));
  res.status_code = 9999;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_response_serialize(&res, &buf, &len));
  PASS();
}

SUITE_EXTERN(response_mock_suite);
SUITE(response_mock_suite) {
  SET_SETUP(reset_mocks, NULL);
  RUN_TEST(test_write_chunk_socket_fail_payload);
  RUN_TEST(test_write_chunk_socket_fail_trailer);
  RUN_TEST(test_write_chunk_tls_fail_payload);
  RUN_TEST(test_write_chunk_tls_fail_trailer);
  RUN_TEST(test_response_send_tls_fail);
  RUN_TEST(test_response_send_socket_fail_body);
  RUN_TEST(test_response_send_socket_success_body);
  RUN_TEST(test_response_send_socket_body_len_zero);
  RUN_TEST(test_write_chunk_headers_not_sent_success);
  RUN_TEST(test_write_chunk_headers_not_sent_fail);
  RUN_TEST(test_response_etag_status_fail);
  RUN_TEST(test_response_redirect_status_fail);
  RUN_TEST(test_response_oauth2_error_status_fail);
  RUN_TEST(test_response_serialize_status_text_fail);
}
