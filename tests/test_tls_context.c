/* clang-format off */
#include "c_rest_error.h"
#include "test_protos.h"
#include "c_rest_tls.h"
#include "c_rest_mem.h"
/* clang-format on */

static void *fail_malloc(size_t s) {
  (void)s;
  return NULL;
}

TEST test_tls_context_lifecycle(void) {
  struct c_rest_tls_context *ctx = NULL;
  struct c_rest_tls_connection *conn = NULL;
  c_rest_error_t res;
  size_t written = 0, rd = 0;
  char buf[10];

#ifdef C_REST_TESTING_MALLOC_HOOK
  g_mock_tls_fail = 3;
  ASSERT_EQ(C_REST_OK, c_rest_tls_write(NULL, NULL, 0, NULL));
  g_mock_tls_fail = 4;
  ASSERT_EQ(C_REST_OK, c_rest_tls_write(NULL, NULL, 0, NULL));
  ASSERT_NEQ(C_REST_OK, c_rest_tls_write(NULL, NULL, 0, NULL));
  g_mock_tls_fail = 5;
  ASSERT_EQ(C_REST_OK, c_rest_tls_write(NULL, NULL, 0, NULL));
  ASSERT_EQ(C_REST_OK, c_rest_tls_write(NULL, NULL, 0, NULL));
  ASSERT_NEQ(C_REST_OK, c_rest_tls_write(NULL, NULL, 0, NULL));
  g_mock_tls_fail = 7;
  ASSERT_EQ(C_REST_OK, c_rest_tls_read(NULL, buf, sizeof(buf), &rd));
  ASSERT_EQ(sizeof(buf), rd);
  g_mock_tls_fail = 0;
#endif

  ASSERT_EQ(C_REST_OK, c_rest_tls_init());
  ASSERT_EQ(C_REST_OK, c_rest_tls_context_init(&ctx));
  ASSERT_EQ(C_REST_OK, c_rest_tls_load_cert(ctx, "tests/certs/server.crt"));
  ASSERT_EQ(C_REST_OK, c_rest_tls_load_key(ctx, "tests/certs/server.key"));
  ASSERT_EQ(C_REST_OK, c_rest_tls_load_ca_chain(ctx, "tests/certs/ca.crt"));
  ASSERT_EQ(C_REST_OK, c_rest_tls_set_alpn(ctx, "h2,http/1.1"));

  res = c_rest_tls_accept(ctx, 0, &conn);
  ASSERT(res == C_REST_OK || res == C_REST_ERROR_GENERIC ||
         res == C_REST_TLS_WANT_READ || res == C_REST_TLS_WANT_WRITE);

  res = c_rest_tls_read(conn, buf, 10, &rd);
  ASSERT(res == C_REST_OK || res == C_REST_ERROR_GENERIC ||
         res == C_REST_TLS_WANT_READ || res == C_REST_TLS_WANT_WRITE);

  res = c_rest_tls_write(conn, buf, 10, &written);
  ASSERT(res == C_REST_OK || res == C_REST_ERROR_GENERIC ||
         res == C_REST_TLS_WANT_READ || res == C_REST_TLS_WANT_WRITE);

  ASSERT_EQ(C_REST_OK, c_rest_tls_close(conn));
  ASSERT_EQ(C_REST_OK, c_rest_tls_context_destroy(ctx));
  PASS();
}

TEST test_tls_context_errors(void) {
  struct c_rest_tls_connection *conn = NULL;
  size_t written = 0, rd = 0;
  char buf[10];

  ASSERT_EQ(C_REST_ERROR_INVALID_ARG, c_rest_tls_context_init(NULL));
  ASSERT_NEQ(C_REST_OK, c_rest_tls_context_destroy(NULL));
  ASSERT_NEQ(C_REST_OK, c_rest_tls_accept(NULL, 0, &conn));
  ASSERT_NEQ(C_REST_OK, c_rest_tls_read(NULL, buf, 10, &rd));
  ASSERT_NEQ(C_REST_OK, c_rest_tls_write(NULL, buf, 10, &written));
  ASSERT_EQ(C_REST_OK, c_rest_tls_close(NULL));
  PASS();
}

TEST test_tls_context_malloc_failures(void) {
  struct c_rest_tls_context *ctx = NULL;
  struct c_rest_tls_connection *conn = NULL;

  g_crf_malloc_hook = fail_malloc;
  ASSERT_NEQ(C_REST_OK, c_rest_tls_context_init(&ctx));
  g_crf_malloc_hook = NULL;

  ASSERT_EQ(C_REST_OK, c_rest_tls_context_init(&ctx));

  g_crf_malloc_hook = fail_malloc;
  ASSERT_NEQ(C_REST_OK, c_rest_tls_accept(ctx, 0, &conn));
  g_crf_malloc_hook = NULL;

  ASSERT_EQ(C_REST_OK, c_rest_tls_context_destroy(ctx));
  PASS();
}

SUITE(tls_context_suite) {
  RUN_TEST(test_tls_context_lifecycle);
  RUN_TEST(test_tls_context_errors);
  RUN_TEST(test_tls_context_malloc_failures);
}
