/* clang-format off */
#include "c_rest_error.h"
#include "test_protos.h"
#include "c_rest_modality.h"
#include "c_rest_platform.h"
#include "c_rest_tls.h"
/* clang-format on */

static void *fail_malloc(size_t s) {
  (void)s;
  return NULL;
}

TEST test_tls_modality_integration(void) {
  struct c_rest_context *ctx = NULL;
  struct c_rest_tls_context *tls_ctx = NULL;

  ASSERT_EQ(C_REST_OK, c_rest_tls_init());

  g_crf_malloc_hook = fail_malloc;
  ASSERT_NEQ(C_REST_OK, c_rest_tls_context_init(&tls_ctx));
  g_crf_malloc_hook = NULL;

  ASSERT_EQ(C_REST_OK, c_rest_tls_context_init(&tls_ctx));
  ASSERT_EQ(C_REST_OK, c_rest_tls_load_cert(tls_ctx, "tests/certs/server.crt"));
  ASSERT_EQ(C_REST_OK, c_rest_tls_load_key(tls_ctx, "tests/certs/server.key"));

  ASSERT_EQ(C_REST_OK, c_rest_init(C_REST_MODALITY_SYNC, &ctx));
  ctx->tls_ctx = tls_ctx;
  ASSERT_EQ(C_REST_OK, c_rest_destroy(ctx));

  ASSERT_EQ(C_REST_OK, c_rest_init(C_REST_MODALITY_ASYNC, &ctx));
  ctx->tls_ctx = tls_ctx;
  ASSERT_EQ(C_REST_OK, c_rest_destroy(ctx));

  ASSERT_EQ(C_REST_OK, c_rest_tls_context_destroy(tls_ctx));
  PASS();
}

SUITE(tls_integration_suite) { RUN_TEST(test_tls_modality_integration); }
