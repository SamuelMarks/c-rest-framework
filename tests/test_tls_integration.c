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

static void *fail_malloc(size_t s) {
  (void)s;
  return NULL;
}

int test_tls_integration(void) {
  struct c_rest_context *ctx = NULL;
  struct c_rest_tls_context *tls_ctx = NULL;
  int res;
  int failed = 0;
  const char *msgs[2];

  res = (int)c_rest_tls_init();
  failed += (res != C_REST_OK);

  /* Test malloc failure */
  g_crf_malloc_hook = fail_malloc;
  res = (int)c_rest_tls_context_init(&tls_ctx);
  failed += (res == C_REST_OK);
  g_crf_malloc_hook = NULL;

  res = (int)c_rest_tls_context_init(&tls_ctx);
  failed += (res != C_REST_OK);

  res = (int)c_rest_tls_load_cert(tls_ctx, "tests/certs/server.crt");
  res = (int)c_rest_tls_load_key(tls_ctx, "tests/certs/server.key");

  res = (int)c_rest_init(C_REST_MODALITY_SYNC, &ctx);
  failed += (res != C_REST_OK);
  ctx->tls_ctx = tls_ctx;
  res = (int)c_rest_destroy(ctx);
  failed += (res != C_REST_OK);

  res = (int)c_rest_init(C_REST_MODALITY_ASYNC, &ctx);
  failed += (res != C_REST_OK);
  ctx->tls_ctx = tls_ctx;
  res = (int)c_rest_destroy(ctx);
  failed += (res != C_REST_OK);

  res = (int)c_rest_tls_context_destroy(tls_ctx);
  failed += (res != C_REST_OK);

  msgs[0] = "test_tls_integration passed\n";
  msgs[1] = "test_tls_integration failed\n";
  printf("%s", msgs[failed != 0]);

  return failed;
}
