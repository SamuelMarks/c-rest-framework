#ifndef C_REST_TEST_PROTOS_H
#define C_REST_TEST_PROTOS_H

/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_testing_mocks.h"
#include "greatest.h"
#include "greatest_clean.h"
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Shared multiplatform accept counter for modality tests. */
extern int g_accept_calls;

#ifdef C_REST_TEST_MAIN
SUITE_EXTERN(log_suite);
SUITE_EXTERN(endian_suite);
SUITE_EXTERN(pool_suite);
SUITE_EXTERN(full_multipart_streaming_suite);
SUITE_EXTERN(time_suite);
SUITE_EXTERN(list_suite);
SUITE_EXTERN(string_suite);
SUITE_EXTERN(tls_context_suite);
SUITE_EXTERN(tls_integration_suite);
SUITE_EXTERN(multiplatform_suite);
SUITE_EXTERN(base64_suite);
SUITE_EXTERN(str_utils_suite);
SUITE_EXTERN(hashmap_suite);
SUITE_EXTERN(graphql_ext_suite);
SUITE_EXTERN(examples_suite);
SUITE_EXTERN(hot_reload_suite);
SUITE_EXTERN(http23_suite);
SUITE_EXTERN(oauth2_suite);
SUITE_EXTERN(jwt_middleware_suite);
SUITE_EXTERN(middleware_suite);
SUITE_EXTERN(crypto_suite);
SUITE_EXTERN(rate_limit_suite);
SUITE_EXTERN(template_suite);
SUITE_EXTERN(ts_queue_suite);
SUITE_EXTERN(mem_suite);
SUITE_EXTERN(multipart_suite);
SUITE_EXTERN(graphql_suite);
SUITE_EXTERN(compression_suite);
SUITE_EXTERN(sse_suite);
SUITE_EXTERN(parser_suite);
SUITE_EXTERN(orm_integration_suite);
SUITE_EXTERN(websocket_suite);
SUITE_EXTERN(platform_suite);
SUITE_EXTERN(openapi_suite);
SUITE_EXTERN(client_suite);
SUITE_EXTERN(router_suite);
SUITE_EXTERN(request_response_suite);
SUITE_EXTERN(modality_suite);
#endif /* C_REST_TEST_MAIN */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* C_REST_TEST_PROTOS_H */
