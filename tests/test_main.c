/* clang-format off */
#include "c_rest_error.h"
#include "test_protos.h"
#include <stdio.h>
#include <stdlib.h>

#include "c_rest_modality.h"
/* clang-format on */

static enum c_rest_error test_logger(const char *msg) {
  printf("LOG: %s\n", msg);
  return C_REST_OK;
}

static int test_init_destroy(void) {
  struct c_rest_context *ctx = NULL;
  int res;
  int failed = 0;

  res = (int)c_rest_init(C_REST_MODALITY_SYNC, &ctx);
  failed += (res != 0);
  failed += (ctx == NULL);

  test_logger("SYNC modality destroyed");

  res = (int)c_rest_destroy(ctx);
  failed += (res != 0);

  return failed;
}

static int test_all_enums(void) {
  int i;
  int res;
  struct c_rest_context *ctx = NULL;
  int failed = 0;

  for (i = 0; i <= C_REST_MODALITY_MESSAGE_PASSING; ++i) {
#if defined(__EMSCRIPTEN__) || defined(CDD_DOS)
    if (i == C_REST_MODALITY_MULTI_PROCESS ||
        i == C_REST_MODALITY_MULTI_THREAD) {
      continue;
    }
#endif
    res = (int)c_rest_init((enum c_rest_modality_type)i, &ctx);
    failed += (res != 0);
    failed += (ctx == NULL);
    res = (int)c_rest_destroy(ctx);
    failed += (res != 0);
  }
  return failed;
}

int test_multiplatform_integration(void);
int test_parser(void);
#ifndef CDD_DOS
#if !defined(__EMSCRIPTEN__)
int test_client(void);
#endif
#endif
#ifndef CDD_DOS
int test_multipart(void);
int test_full_multipart_form_streaming(void);
#endif
int test_router(void);
#ifdef C_REST_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING
int test_template(void);
#endif
#if defined(C_REST_ENABLE_HOT_RELOADING_AUTO_RESTART) &&                       \
    !defined(__EMSCRIPTEN__)
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
#if !defined(__EMSCRIPTEN__)
int test_tls_integration(void);
#endif
#endif
#ifndef CDD_DOS
int test_oauth2(void);
#endif
#ifndef CDD_DOS
int test_platform(void);
int test_openapi(void);
int test_middleware_suite(void);
int test_modality(void);
int test_rate_limiting_throttling_middleware(void);
#endif
#ifdef C_REST_ENABLE_SERVER_SENT_EVENTS_SSE
int test_server_sent_events_sse(void);
#endif
int test_examples(void);

int main(int argc, char **argv) {
  int failed = 0;
  (void)argc;
  (void)argv;

#define RUN_TEST(name, call)                                                   \
  do {                                                                         \
    fflush(stdout);                                                            \
    printf("Running " name "...\n");                                           \
    failed += (call);                                                          \
  } while (0)

  RUN_TEST("test_init_destroy", test_init_destroy());

#ifdef C_REST_ENABLE_SERVER_SENT_EVENTS_SSE
  RUN_TEST("test_server_sent_events_sse", test_server_sent_events_sse());
#endif

#ifndef CDD_DOS
  RUN_TEST("test_openapi", test_openapi());
  RUN_TEST("test_middleware", test_middleware_suite());
  RUN_TEST("test_modality", test_modality());
  RUN_TEST("test_platform", test_platform());
  RUN_TEST("test_rate_limiting_throttling_middleware",
           test_rate_limiting_throttling_middleware());
  RUN_TEST("test_endian", test_endian());
  RUN_TEST("test_hashmap", test_hashmap());
  RUN_TEST("test_list", test_list());
  RUN_TEST("test_str_utils", test_str_utils());
  RUN_TEST("test_mem", test_mem());
  RUN_TEST("test_ts_queue", test_ts_queue());
  RUN_TEST("test_log", test_log());
  RUN_TEST("test_string", test_string());
  RUN_TEST("test_pool", test_pool());

#ifdef C_REST_FRAMEWORK_ENABLE_GRAPHQL
  RUN_TEST("test_graphql", test_graphql());
#endif

#if !defined(__EMSCRIPTEN__)
  RUN_TEST("test_http23", test_http23());
#endif

#if !defined(__EMSCRIPTEN__)
  RUN_TEST("test_websocket", test_websocket());
#endif

  RUN_TEST("test_all_enums", test_all_enums());
  RUN_TEST("test_multiplatform_integration", test_multiplatform_integration());
  RUN_TEST("test_parser", test_parser());

#if !defined(__EMSCRIPTEN__)
  RUN_TEST("test_client", test_client());
#endif

  RUN_TEST("test_multipart", test_multipart());
  RUN_TEST("test_full_multipart_form_streaming",
           test_full_multipart_form_streaming());
  RUN_TEST("test_router", test_router());
  RUN_TEST("test_request_response", test_request_response());
  RUN_TEST("test_orm_integration", test_orm_integration());
  RUN_TEST("test_crypto", test_crypto());
  RUN_TEST("test_time", test_time());
  RUN_TEST("test_tls_context", test_tls_context());
  RUN_TEST("test_base64", test_base64());

#if !defined(__EMSCRIPTEN__)
  RUN_TEST("test_tls_integration", test_tls_integration());
#endif

  RUN_TEST("test_oauth2", test_oauth2());
#endif

#ifdef C_REST_ENABLE_JWT_JSON_WEB_TOKENS_AUTHENTICATION_MIDDLEWARE
  RUN_TEST("test_jwt_json_web_tokens_authentication_middleware",
           test_jwt_json_web_tokens_authentication_middleware());
#endif

#ifdef C_REST_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING
  RUN_TEST("test_template", test_template());
#endif

#if defined(C_REST_ENABLE_HOT_RELOADING_AUTO_RESTART) &&                       \
    !defined(__EMSCRIPTEN__)
  RUN_TEST("test_hot_reload", test_hot_reload());
#endif

#ifdef C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP_BROTLI
  RUN_TEST("test_response_compression_gzip_brotli",
           test_response_compression_gzip_brotli());
#endif

  RUN_TEST("test_examples", test_examples());

  printf("All tests passed.\n");
  return failed;
}
