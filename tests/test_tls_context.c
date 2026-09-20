/* clang-format off */
#include "c_rest_error.h"
#include "test_protos.h"
#include "c_rest_tls.h"
#include "c_rest_mem.h"
#include <stdio.h>
#include <stdlib.h>
/* clang-format on */

int test_tls_context(void);
static int test_tls_context_errors(void);
static int test_tls_context_malloc_failures(void);

int test_tls_context(void) {
  struct c_rest_tls_context *ctx = NULL;
  struct c_rest_tls_connection *conn = NULL;
  int res;
  int failed = 0;
  const char *msgs[2];
  size_t written = 0, rd = 0;
  char buf[10];

  res = (int)c_rest_tls_init();
  failed += (res != C_REST_OK);

  res = (int)c_rest_tls_context_init(&ctx);
  failed += (res != C_REST_OK);

  res = (int)c_rest_tls_load_cert(ctx, "tests/certs/server.crt");
  failed += (res != C_REST_OK);
  res = (int)c_rest_tls_load_key(ctx, "tests/certs/server.key");
  failed += (res != C_REST_OK);

  res = (int)c_rest_tls_load_ca_chain(ctx, "tests/certs/ca.crt");
  failed += (res != C_REST_OK);

  res = (int)c_rest_tls_set_alpn(ctx, "h2,http/1.1");
  failed += (res != C_REST_OK);

  res = (int)c_rest_tls_accept(ctx, 0, &conn);
  (void)res;

  res = (int)c_rest_tls_read(conn, buf, 10, &rd);
  (void)res;

  res = (int)c_rest_tls_write(conn, buf, 10, &written);
  (void)res;

  res = (int)c_rest_tls_close(conn);
  failed += (res != C_REST_OK);

  res = (int)c_rest_tls_context_destroy(ctx);
  failed += (res != C_REST_OK);

  failed += (test_tls_context_errors() != 0);
  failed += (test_tls_context_malloc_failures() != 0);

  msgs[0] = "test_tls_context passed\n";
  msgs[1] = "test_tls_context failed\n";
  printf("%s", msgs[failed != 0]);

  return failed;
}

static int test_tls_context_errors(void) {
  int failed = 0;
  int res;
  struct c_rest_tls_connection *conn = NULL;
  size_t written = 0, rd = 0;
  char buf[10];

  res = (int)c_rest_tls_context_destroy(NULL);
  failed += (res == C_REST_OK);

  res = (int)c_rest_tls_accept(NULL, 0, &conn);
  failed += (res == C_REST_OK);

  res = (int)c_rest_tls_read(NULL, buf, 10, &rd);
  failed += (res == C_REST_OK);

  res = (int)c_rest_tls_write(NULL, buf, 10, &written);
  failed += (res == C_REST_OK);

  res = (int)c_rest_tls_close(NULL);
  failed += (res != C_REST_OK);

  return failed;
}

static void *fail_malloc(size_t s) {
  (void)s;
  return NULL;
}

static int test_tls_context_malloc_failures(void) {
  struct c_rest_tls_context *ctx = NULL;
  struct c_rest_tls_connection *conn = NULL;
  int res;
  int failed = 0;

  g_crf_malloc_hook = fail_malloc;
  res = (int)c_rest_tls_context_init(&ctx);
  failed += (res == C_REST_OK);
  g_crf_malloc_hook = NULL;

  res = (int)c_rest_tls_context_init(&ctx);
  failed += (res != C_REST_OK);

  g_crf_malloc_hook = fail_malloc;
  res = (int)c_rest_tls_accept(ctx, 0, &conn);
  failed += (res == C_REST_OK);
  g_crf_malloc_hook = NULL;

  res = (int)c_rest_tls_context_destroy(ctx);
  failed += (res != C_REST_OK);

  return failed;
}
