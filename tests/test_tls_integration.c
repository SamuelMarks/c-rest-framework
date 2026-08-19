/* clang-format off */
#include "c_rest_error.h"
#include "test_protos.h"
#include "c_rest_modality.h"
#include "c_rest_platform.h"
#include "c_rest_tls.h"
#include <stdio.h>
#include <stdlib.h>
/* clang-format on */

int test_tls_integration(void);

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

int test_tls_integration(void) {
  struct c_rest_context *ctx = NULL;
  struct c_rest_tls_context *tls_ctx = NULL;
  int res;
  int failed = 0;
  const char *msgs[2];
  int i;

  res = c_rest_tls_init();
  failed += (res != C_REST_OK);

  /* Test malloc failures */
  g_crf_malloc_hook = fail_malloc_n;
  for (i = 0; i < 5; i++) {
    g_malloc_fail_after = i;
    res = c_rest_tls_context_init(&tls_ctx);
    if (res == C_REST_OK) {
      c_rest_tls_context_destroy(tls_ctx);
    }
  }
  g_crf_malloc_hook = NULL;

  res = c_rest_tls_context_init(&tls_ctx);
  failed += (res != C_REST_OK);

  if (res == C_REST_OK) {
    res = c_rest_tls_load_cert(tls_ctx, "tests/certs/server.crt");
    res = c_rest_tls_load_key(tls_ctx, "tests/certs/server.key");

    res = c_rest_init(C_REST_MODALITY_SYNC, &ctx);
    failed += (res != C_REST_OK);
    if (res == C_REST_OK) {
      ctx->tls_ctx = tls_ctx;
      res = c_rest_destroy(ctx);
      failed += (res != C_REST_OK);
    }

    res = c_rest_init(C_REST_MODALITY_ASYNC, &ctx);
    failed += (res != C_REST_OK);
    if (res == C_REST_OK) {
      ctx->tls_ctx = tls_ctx;
      res = c_rest_destroy(ctx);
      failed += (res != C_REST_OK);
    }

    res = c_rest_tls_context_destroy(tls_ctx);
    failed += (res != C_REST_OK);
  }

  msgs[0] = "test_tls_integration passed\n";
  msgs[1] = "test_tls_integration failed\n";
  printf("%s", msgs[failed != 0]);

  return failed;
}
