/* clang-format off */
#include "test_protos.h"
#include <stdio.h>
#include <stdlib.h>

#include "c_rest_modality.h"
/* clang-format on */

static void test_logger(const char *msg) { printf("LOG: %s\n", msg); }

static int test_init_destroy(void) {
  struct c_rest_context *ctx = NULL;
  int res;

  res = c_rest_init(C_REST_MODALITY_SYNC, &ctx);
  if (res != 0 || ctx == NULL) {
    printf("Failed to init C_REST_MODALITY_SYNC\n");
    return 1;
  }

  ctx->logger.log_cb = test_logger;

  res = c_rest_destroy(ctx);
  if (res != 0) {
    printf("Failed to destroy ctx\n");
    return 1;
  }

  return 0;
}

static int test_all_enums(void) {
  int i;
  int res;
  struct c_rest_context *ctx = NULL;

  for (i = 0; i <= C_REST_MODALITY_MESSAGE_PASSING; ++i) {
    res = c_rest_init((enum c_rest_modality_type)i, &ctx);
    if (res != 0 || ctx == NULL) {
      printf("Failed to init enum %d\n", i);
      return 1;
    }
    res = c_rest_destroy(ctx);
    if (res != 0) {
      printf("Failed to destroy enum %d\n", i);
      return 1;
    }
  }
  return 0;
}

int test_multiplatform_integration(void);
int test_parser(void);
#ifndef CDD_DOS
int test_client(void);
#endif
#ifndef CDD_DOS
int test_multipart(void);
int test_full_multipart_form_streaming(void);
#endif
int test_router(void);
#ifdef C_REST_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING
int test_template(void);
#endif
#ifdef C_REST_ENABLE_HOT_RELOADING_AUTO_RESTART
int test_hot_reload(void);
#endif
#ifdef C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP_BROTLI
int test_response_compression_gzip_brotli(void);
#endif
#ifdef C_REST_ENABLE_JWT_JSON_WEB_TOKENS_AUTHENTICATION_MIDDLEWARE
int test_jwt_json_web_tokens_authentication_middleware(void);
#endif
int test_request_response(void);
#ifndef CDD_DOS
int test_orm_integration(void);
#endif
#ifndef CDD_DOS
int test_crypto(void);
#endif
#ifndef CDD_DOS
int test_time(void);
#endif
int test_tls_context(void);
#ifndef CDD_DOS
int test_tls_integration(void);
#endif
#ifndef CDD_DOS
int test_oauth2(void);
#endif
#ifndef CDD_DOS
int test_openapi(void);
int test_rate_limiting_throttling_middleware(void);
#endif
#ifdef C_REST_ENABLE_SERVER_SENT_EVENTS_SSE
int test_server_sent_events_sse(void);
#endif

int main(void) {

  int res = 0;

  printf("Running test_init_destroy...\n");
  res = test_init_destroy();
  if (res != 0)
    return res;

#ifdef C_REST_ENABLE_SERVER_SENT_EVENTS_SSE
  printf("Running test_server_sent_events_sse...\n");
  res = test_server_sent_events_sse();
  if (res != 0)
    return res;
#endif

#ifndef CDD_DOS
  printf("Running test_openapi...\n");
  res = test_openapi();
  if (res != 0)
    return res;

  printf("Running test_rate_limiting_throttling_middleware...\n");
  res = test_rate_limiting_throttling_middleware();
  if (res != 0)
    return res;
#endif

  printf("Running test_all_enums...\n");
  res = test_all_enums();
  if (res != 0)
    return res;

  printf("Running test_multiplatform_integration...\n");
  res = test_multiplatform_integration();
  if (res != 0)
    return res;

  printf("Running test_parser...\n");
  res = test_parser();
  if (res != 0)
    return res;

#ifndef CDD_DOS
  printf("Running test_client...\n");
  res = test_client();
  if (res != 0)
    return res;
#endif

#ifndef CDD_DOS
  printf("Running test_multipart...\n");
  res = test_multipart();
  if (res == 0) {
    printf("Running test_full_multipart_form_streaming...\n");
    res = test_full_multipart_form_streaming();
  }
  if (res != 0)
    return res;
#endif

  printf("Running test_router...\n");
  res = test_router();
  if (res != 0)
    return res;

  printf("Running test_request_response...\n");
  res = test_request_response();
  if (res != 0)
    return res;

#ifndef CDD_DOS
  printf("Running test_orm_integration...\n");
  res = test_orm_integration();
  if (res != 0)
    return res;
#endif

#ifndef CDD_DOS
  printf("Running test_crypto...\n");
  res = test_crypto();
  if (res != 0)
    return res;
#endif

#ifndef CDD_DOS
  printf("Running test_time...\n");
  res = test_time();
  if (res != 0)
    return res;
#endif

  printf("Running test_tls_context...\n");
  res = test_tls_context();
  if (res != 0)
    return res;

#ifndef CDD_DOS
  printf("Running test_tls_integration...\n");
  res = test_tls_integration();
  if (res != 0)
    return res;
#endif

#ifndef CDD_DOS
  printf("Running test_oauth2...\n");
  res = test_oauth2();
  if (res != 0)
    return res;
#endif

#ifdef C_REST_ENABLE_JWT_JSON_WEB_TOKENS_AUTHENTICATION_MIDDLEWARE
  printf("Running test_jwt_json_web_tokens_authentication_middleware...\n");
  res = test_jwt_json_web_tokens_authentication_middleware();
  if (res != 0)
    return res;
#endif

#ifdef C_REST_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING
  printf("Running test_template...\n");
  res = test_template();
  if (res != 0)
    return res;
#endif

#ifdef C_REST_ENABLE_HOT_RELOADING_AUTO_RESTART
  printf("Running test_hot_reload...\n");
  res = test_hot_reload();
  if (res != 0)
    return res;
#endif

#ifdef C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP_BROTLI
  printf("Running test_response_compression_gzip_brotli...\n");
  res = test_response_compression_gzip_brotli();
  if (res != 0)
    return res;
#endif

  printf("All tests passed.\\n");
  return 0;
}
