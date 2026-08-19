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

  res = c_rest_tls_init();
  failed += (res != C_REST_OK);

  res = c_rest_tls_context_init(&ctx);
  failed += (res != C_REST_OK);

  if (res == C_REST_OK) {
    res = c_rest_tls_load_cert(ctx, "tests/certs/server.crt");
    res = c_rest_tls_load_key(ctx, "tests/certs/server.key");

    res = c_rest_tls_load_ca_chain(ctx, "tests/certs/ca.crt");
    if (res != C_REST_OK) {
      printf("Failed load_ca_chain\n");
      failed++;
    }

    res = c_rest_tls_set_alpn(ctx, "h2,http/1.1");
    if (res != C_REST_OK) {
      printf("Failed set_alpn\n");
      failed++;
    }

    res = c_rest_tls_accept(ctx, 0, &conn);

    if (conn) {
      res = c_rest_tls_read(conn, buf, 10, &rd);
      if (res == C_REST_OK) {
        printf("Read should fail in dummy or fail handhshake\n");
      }

      res = c_rest_tls_write(conn, buf, 10, &written);
      if (res == C_REST_OK) {
        printf("Write should fail in dummy or fail handshake\n");
      }

      res = c_rest_tls_close(conn);
      if (res != C_REST_OK) {
        printf("Close conn failed\n");
        failed++;
      }
    }

    res = c_rest_tls_context_destroy(ctx);
    if (res != C_REST_OK) {
      printf("Destroy ctx failed\n");
      failed++;
    }
  }

  if (test_tls_context_errors() != 0) {
    printf("test_tls_context_errors failed\n");
    failed++;
  }
  if (test_tls_context_malloc_failures() != 0) {
    printf("test_tls_context_malloc_failures failed\n");
    failed++;
  }

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

  res = c_rest_tls_context_destroy(NULL);
  if (res == C_REST_OK) {
    printf("Err: destroy(NULL)\n");
    failed++;
  }

  res = c_rest_tls_accept(NULL, 0, &conn);
  if (res == C_REST_OK) {
    printf("Err: accept(NULL)\n");
    failed++;
  }

  res = c_rest_tls_read(NULL, buf, 10, &rd);
  if (res == C_REST_OK) {
    printf("Err: read(NULL)\n");
    failed++;
  }

  res = c_rest_tls_write(NULL, buf, 10, &written);
  if (res == C_REST_OK) {
    printf("Err: write(NULL)\n");
    failed++;
  }

  res = c_rest_tls_close(NULL);
  if (res != C_REST_OK) {
    printf("Err: close(NULL)\n");
    failed++;
  }

  return failed;
}

static int g_malloc_fail_after = -1;
static void *fail_malloc_n(size_t size) {
  void *res = NULL;
  int should_fail = 0;

  if (g_malloc_fail_after == 0) {
    should_fail = 1;
  }
  if (g_malloc_fail_after >= 0) {
    g_malloc_fail_after--;
  }

  res = malloc(size);
  if (should_fail) {
    free(res);
    res = NULL;
  }
  return res;
}

static int test_tls_context_malloc_failures(void) {
  struct c_rest_tls_context *ctx = NULL;
  struct c_rest_tls_connection *conn = NULL;
  int res;
  int failed = 0;
  int i;

  g_crf_malloc_hook = fail_malloc_n;

  for (i = 0; i < 5; i++) {
    g_malloc_fail_after = i;
    res = c_rest_tls_context_init(&ctx);
    if (res == C_REST_OK) {
      c_rest_tls_context_destroy(ctx);
    }
  }

  /* we need a valid ctx to test accept */
  g_crf_malloc_hook = NULL;
  if (c_rest_tls_context_init(&ctx) == C_REST_OK) {
    g_crf_malloc_hook = fail_malloc_n;
    for (i = 0; i < 5; i++) {
      g_malloc_fail_after = i;
      res = c_rest_tls_accept(ctx, 0, &conn);
      if (res == C_REST_OK) {
        c_rest_tls_close(conn);
      }
    }
    g_crf_malloc_hook = NULL;
    c_rest_tls_context_destroy(ctx);
  }

  g_crf_malloc_hook = NULL;
  return failed;
}
