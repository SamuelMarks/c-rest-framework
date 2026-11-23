#define C_REST_TEST_MAIN 1

/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "test_protos.h"
#include <stdio.h>
#include <stdlib.h>

#include "c_rest_modality.h"
#ifndef CDD_DOS
#include "parson.h"
#endif
/* clang-format on */

TEST test_init_destroy_case(void) {
  struct c_rest_context *ctx = NULL;

  ASSERT_EQ(C_REST_OK, c_rest_init(C_REST_MODALITY_SYNC, &ctx));
  ASSERT(ctx != NULL);
  ASSERT_EQ(C_REST_OK, c_rest_destroy(ctx));
  PASS();
}

TEST test_all_enums_case(void) {
  int i;
  struct c_rest_context *ctx = NULL;

  for (i = 0; i <= C_REST_MODALITY_MESSAGE_PASSING; ++i) {
#if defined(__EMSCRIPTEN__) || defined(CDD_DOS)
    if (i == C_REST_MODALITY_MULTI_PROCESS ||
        i == C_REST_MODALITY_MULTI_THREAD) {
      continue;
    }
#elif defined(C_REST_NO_THREADS)
    if (i == C_REST_MODALITY_MULTI_THREAD) {
      ctx = NULL;
      ASSERT_EQ(C_REST_ERROR_NOT_SUPPORTED,
                c_rest_init((enum c_rest_modality_type)i, &ctx));
      continue;
    }
#endif
    ctx = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_init((enum c_rest_modality_type)i, &ctx));
    ASSERT(ctx != NULL);
    ASSERT_EQ(C_REST_OK, c_rest_destroy(ctx));
  }
  PASS();
}

SUITE(core_lifecycle_suite) {
  RUN_TEST(test_init_destroy_case);
  RUN_TEST(test_all_enums_case);
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
  GREATEST_MAIN_BEGIN();
#if !defined(CDD_DOS) && defined(C_REST_TESTING_MALLOC_HOOK)
  json_set_allocation_functions(test_c_rest_default_malloc,
                                test_c_rest_internal_free);
#endif

  RUN_SUITE(core_lifecycle_suite);
  RUN_SUITE(log_suite);
  RUN_SUITE(endian_suite);
  RUN_SUITE(pool_suite);
  RUN_SUITE(full_multipart_streaming_suite);
#ifdef C_REST_ENABLE_SERVER_SENT_EVENTS_SSE
  RUN_SUITE(sse_suite);
#endif
#ifndef CDD_DOS
  RUN_SUITE(time_suite);
  RUN_SUITE(list_suite);
  RUN_SUITE(string_suite);
  RUN_SUITE(tls_context_suite);
#if !defined(__EMSCRIPTEN__)
  RUN_SUITE(tls_integration_suite);
  RUN_SUITE(http23_suite);
  RUN_SUITE(websocket_suite);
  RUN_SUITE(client_suite);
#endif
  RUN_SUITE(multiplatform_suite);
  RUN_SUITE(base64_suite);
  RUN_SUITE(str_utils_suite);
  RUN_SUITE(hashmap_suite);
  RUN_SUITE(middleware_suite);
  RUN_SUITE(platform_suite);
  RUN_SUITE(rate_limit_suite);
  RUN_SUITE(mem_suite);
  RUN_SUITE(ts_queue_suite);
  RUN_SUITE(parser_suite);
  RUN_SUITE(multipart_suite);
  RUN_SUITE(orm_integration_suite);
  RUN_SUITE(crypto_suite);
  RUN_SUITE(oauth2_suite);
  RUN_SUITE(openapi_suite);
#ifdef C_REST_FRAMEWORK_ENABLE_GRAPHQL
  RUN_SUITE(graphql_suite);
  RUN_SUITE(graphql_ext_suite);
#endif
  RUN_SUITE(router_suite);
  RUN_SUITE(request_response_suite);
  RUN_SUITE(modality_suite);
#endif
#ifdef C_REST_ENABLE_JWT_JSON_WEB_TOKENS_AUTHENTICATION_MIDDLEWARE
  RUN_SUITE(jwt_middleware_suite);
#endif
#ifdef C_REST_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING
  RUN_SUITE(template_suite);
#endif
#if defined(C_REST_ENABLE_HOT_RELOADING_AUTO_RESTART) &&                       \
    !defined(__EMSCRIPTEN__) && !defined(C_REST_NO_THREADS)
  RUN_SUITE(hot_reload_suite);
#endif
#ifdef C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP_BROTLI
  RUN_SUITE(compression_suite);
#endif
  RUN_SUITE(examples_suite);

  GREATEST_MAIN_END();
}
