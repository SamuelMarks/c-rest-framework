/* clang-format off */
#ifndef C_REST_TEST_PROTOS_H
#define C_REST_TEST_PROTOS_H

int test_multiplatform_integration(void);
int test_parser(void);
int test_client(void);
int test_multipart(void);
int test_router(void);
int test_request_response(void);
int test_orm_integration(void);
int test_crypto(void);
int test_time(void);
int test_tls_context(void);
int test_tls_integration(void);
int test_oauth2(void);
int test_openapi(void);
int test_websocket(void);
int test_rate_limiting_throttling_middleware(void);
#ifdef C_REST_FRAMEWORK_ENABLE_GRAPHQL
int test_graphql(void);
#endif

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

#endif /* C_REST_TEST_PROTOS_H */
/* clang-format on */
