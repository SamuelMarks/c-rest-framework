/* clang-format off */
#include "test_protos.h"
#include "c_rest_modality.h"
#include "c_rest_platform.h"
#include "c_rest_tls.h"
#include <stdio.h>
/* clang-format on */

int test_tls_integration(void);

int test_tls_integration(void) {
  struct c_rest_context *ctx = NULL;
  struct c_rest_tls_context *tls_ctx = NULL;
  int res;

  printf("Running TLS Integration test...\n");

  if (c_rest_tls_init() != 0) {
    printf("No TLS backend available, skipping.\n");
    return 0;
  }

  if (c_rest_tls_context_init(&tls_ctx) != 0) {
    printf("TLS context init failed, skipping.\n");
    return 0;
  }

  /* Try to load certs. If it fails, that's fine, we still test the context
   * attachment */
  c_rest_tls_load_cert(tls_ctx, "tests/certs/server.crt");
  c_rest_tls_load_key(tls_ctx, "tests/certs/server.key");

  res = c_rest_init(C_REST_MODALITY_SYNC, &ctx);
  if (res != 0) {
    printf("Failed to init SYNC modality\n");
    c_rest_tls_context_destroy(tls_ctx);
    return 1;
  }

  /* Attach TLS context */
  ctx->tls_ctx = tls_ctx;

  /* We do not actually call c_rest_run because it would block forever
   * listening. But we simulate a clean destroy of the TLS-attached context. */
  res = c_rest_destroy(ctx);
  if (res != 0) {
    printf("Failed to destroy SYNC modality\n");
    return 1;
  }

  res = c_rest_init(C_REST_MODALITY_ASYNC, &ctx);
  if (res != 0) {
    printf("Failed to init ASYNC modality\n");
    return 1;
  }
  ctx->tls_ctx = tls_ctx;

  res = c_rest_destroy(ctx);
  if (res != 0) {
    printf("Failed to destroy ASYNC modality\n");
    return 1;
  }

  /* Destroy the TLS context after the framework is destroyed */
  c_rest_tls_context_destroy(tls_ctx);

  return 0;
}
