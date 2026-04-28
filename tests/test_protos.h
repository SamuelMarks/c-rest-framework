/* clang-format off */
#ifndef C_REST_TEST_PROTOS_H
#define C_REST_TEST_PROTOS_H

#ifdef __cplusplus
extern "C" {
#endif

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
int test_base64(void);
int test_rate_limiting_throttling_middleware(void);
int test_http23(void);
int test_full_multipart_form_streaming(void);
int test_server_sent_events_sse(void);
int test_graphql(void);
int test_template(void);
int test_hot_reload(void);
int test_response_compression_gzip_brotli(void);
int test_jwt_json_web_tokens_authentication_middleware(void);

#ifdef __cplusplus
}
#endif

#endif /* C_REST_TEST_PROTOS_H */
/* clang-format on */