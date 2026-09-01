/* clang-format off */
#include "c_rest_error.h"
#include "greatest.h"
#include <string.h>

/* Redefine C_REST_EXPORT to nothing to prevent dllimport issues on Windows */
#undef C_REST_EXPORT
#define C_REST_EXPORT

#include "c_rest_platform.h"
#include "c_rest_tls.h"
#include "c_rest_response.h"

/* We will mock the I/O functions used by response.c */
static int g_mock_send_countdown = -1;
static int g_mock_tls_countdown = -1;

static c_rest_error_t mock_c_rest_socket_send(c_rest_socket_t sock, const void *buf, size_t len, size_t *written) {
    if (g_mock_send_countdown >= 0) {
        if (g_mock_send_countdown == 0) return C_REST_ERROR_GENERIC;
        g_mock_send_countdown--;
    }
    return c_rest_socket_send(sock, buf, len, written);
}

static c_rest_error_t mock_c_rest_tls_write(struct c_rest_tls_connection *conn, const void *buf, size_t len, size_t *written) {
    if (g_mock_tls_countdown >= 0) {
        if (g_mock_tls_countdown == 0) return C_REST_ERROR_GENERIC;
        g_mock_tls_countdown--;
    }
    return c_rest_tls_write(conn, buf, len, written);
}

/* Preprocessor Injection */
#define c_rest_socket_send mock_c_rest_socket_send
#define c_rest_tls_write mock_c_rest_tls_write

/* To avoid duplicate symbol linking issues with the real library,
   we can just compile this and it will shadow the DLL / static lib.
   If MSVC complains about duplicate symbols with static linking, we can rename the tested function. */
#define c_rest_response_write_chunk test_c_rest_response_write_chunk

#include "../src/response.c"

#undef c_rest_socket_send
#undef c_rest_tls_write
/* clang-format on */

static void reset_mocks(void *data) {
  (void)data;
  g_mock_send_countdown = -1;
  g_mock_tls_countdown = -1;
}

TEST test_write_chunk_socket_fail_payload(void) {
  struct c_rest_response res = {0};
  struct c_rest_connection_context ctx = {0};
  res.context = &ctx;
  res.is_chunked = 1;
  ctx.sock = 123;

  g_mock_send_countdown = 1; /* Succeed on chunk header, fail on payload */
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_response_write_chunk(&res, "abc", 3));
  PASS();
}

TEST test_write_chunk_socket_fail_trailer(void) {
  struct c_rest_response res = {0};
  struct c_rest_connection_context ctx = {0};
  res.context = &ctx;
  res.is_chunked = 1;
  ctx.sock = 123;

  g_mock_send_countdown =
      2; /* Succeed on header, payload, fail on \r\n trailer */
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_response_write_chunk(&res, "abc", 3));
  PASS();
}

TEST test_write_chunk_tls_fail_payload(void) {
  struct c_rest_response res = {0};
  struct c_rest_connection_context ctx = {0};
  res.context = &ctx;
  res.is_chunked = 1;
  ctx.tls_conn = (void *)1;

  g_mock_tls_countdown = 1; /* Succeed on chunk header, fail on payload */
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_response_write_chunk(&res, "abc", 3));
  PASS();
}

TEST test_write_chunk_tls_fail_trailer(void) {
  struct c_rest_response res = {0};
  struct c_rest_connection_context ctx = {0};
  res.context = &ctx;
  res.is_chunked = 1;
  ctx.tls_conn = (void *)1;

  g_mock_tls_countdown =
      2; /* Succeed on header, payload, fail on \r\n trailer */
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_response_write_chunk(&res, "abc", 3));
  PASS();
}

SUITE(response_mock_suite) {
  SET_SETUP(reset_mocks, NULL);
  RUN_TEST(test_write_chunk_socket_fail_payload);
  RUN_TEST(test_write_chunk_socket_fail_trailer);
  RUN_TEST(test_write_chunk_tls_fail_payload);
  RUN_TEST(test_write_chunk_tls_fail_trailer);
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
  GREATEST_MAIN_BEGIN();
  RUN_SUITE(response_mock_suite);
  GREATEST_MAIN_END();
}
