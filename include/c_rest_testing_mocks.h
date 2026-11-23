/**
 * @file c_rest_testing_mocks.h
 * @brief Test fault-injection state and seam hook declarations.
 */
#ifndef C_REST_TESTING_MOCKS_H
#define C_REST_TESTING_MOCKS_H

/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_export.h"
#include "c_rest_no_discard.h"
#include "c_rest_platform.h"
#include "c_rest_mem.h"
#include <stddef.h>
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) && !defined(C_REST_FRAMEWORK_STATIC) &&                    \
    !defined(C_REST_FRAMEWORK_STATIC_DEFINE)
#ifdef c_rest_framework_EXPORTS
/** @brief DLL export/import qualifier for test mock data symbols. */
#define C_REST_MOCK_DATA __declspec(dllexport)
#else
/** @brief DLL export/import qualifier for test mock data symbols. */
#define C_REST_MOCK_DATA __declspec(dllimport)
#endif
#else
/** @brief DLL export/import qualifier for test mock data symbols. */
#define C_REST_MOCK_DATA C_REST_EXPORT
#endif

struct c_rest_context;
struct c_rest_ts_queue;
struct c_rest_response;
struct c_rest_string;
struct c_rest_sse_event;
struct c_rest_hashmap;
struct c_rest_client_response;

/** @brief Optional endianness override hook for unit tests. */
C_REST_EXPORT extern c_rest_error_t (*g_crf_is_little_endian_hook)(int *);

/**
 * @brief Seam wrapper for popping from a thread-safe queue during tests.
 * @param queue Queue instance.
 * @param out_data Output element pointer.
 * @return C_REST_OK on success, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
mock_c_rest_ts_queue_pop(struct c_rest_ts_queue *queue, void **out_data);

/**
 * @brief Seam wrapper for accepting a client socket during modality tests.
 * @param server Server listening socket.
 * @param out_client Output accepted client socket.
 * @return C_REST_OK on success, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
mock_c_rest_socket_accept(c_rest_socket_t server, c_rest_socket_t *out_client);

/**
 * @brief Seam wrapper for closing a socket during modality tests.
 * @param sock Socket handle to close.
 * @return C_REST_OK on success, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
mock_c_rest_socket_close(c_rest_socket_t sock);

/**
 * @brief Seam wrapper for handling an accepted connection during modality
 * tests.
 * @param ctx Framework context.
 * @param sock Client socket handle.
 * @return C_REST_OK on success, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
mock_c_rest_handle_connection(struct c_rest_context *ctx, c_rest_socket_t sock);

/**
 * @brief Seam wrapper for spawning worker threads during modality tests.
 * @param thread Output thread handle.
 * @param func Thread entry function.
 * @param arg User argument passed to thread entry function.
 * @return C_REST_OK on success, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
mock_c_rest_thread_create(c_rest_thread_t *thread,
                          c_rest_error_t (*func)(void *), void *arg);

/**
 * @brief Seam wrapper for initializing platform subsystem in c_rest_init.
 * @return C_REST_OK on success, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_modality_platform_init(void);

/**
 * @brief Seam hook for c_rest_platform_cleanup fault injection.
 * @return C_REST_OK on success, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_platform_cleanup_hook(void);

/**
 * @brief Seam hook for process fork/spawn fault injection.
 * @return C_REST_OK on success, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t crf_seam_fork_hook(void);

/**
 * @brief Seam hook checking whether g_mock_socket_fail matches a code.
 * @param code Fault-injection code to match.
 * @param err Error enum to return when matched.
 * @return err if matched, C_REST_OK otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_socket_check(int code, c_rest_error_t err);

/**
 * @brief Seam hook for c_rest_socket_send fault injection.
 * @param len Requested byte length.
 * @param out_written Output bytes written when intercepted.
 * @param out_intercepted Set to 1 if intercepted, 0 otherwise.
 * @return Error code to return when intercepted.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_socket_send(size_t len, size_t *out_written, int *out_intercepted);

/**
 * @brief Seam hook for c_rest_tls_read fault injection.
 * @param buf Destination read buffer.
 * @param len Buffer capacity.
 * @param out_read Output bytes read when intercepted.
 * @param out_intercepted Set to 1 if intercepted, 0 otherwise.
 * @return Error code to return when intercepted.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_tls_read(void *buf, size_t len, size_t *out_read,
                  int *out_intercepted);

/**
 * @brief Seam hook for c_rest_tls_write fault injection.
 * @param len Requested write length.
 * @param out_written Output bytes written when intercepted.
 * @param out_intercepted Set to 1 if intercepted, 0 otherwise.
 * @return Error code to return when intercepted.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_tls_write(size_t len, size_t *out_written, int *out_intercepted);

/**
 * @brief Seam hook for c_rest_tls_close fault injection.
 * @return C_REST_OK when not triggered, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t crf_seam_tls_close_hook(void);

/**
 * @brief Seam hook for c_rest_parser_should_keep_alive fault injection.
 * @return C_REST_OK when not triggered, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_parser_keep_alive_hook(void);

/**
 * @brief Seam hook for c_rest_parser_destroy fault injection.
 * @return C_REST_OK when not triggered, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_parser_destroy_hook(void);

/**
 * @brief Seam hook for c_rest_parser_get_basic_vtable fault injection.
 * @return C_REST_OK when not triggered, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_parser_vtable_hook(void);

/**
 * @brief Seam hook for c_rest_response_set_status fault injection.
 * @return C_REST_OK when not triggered, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t crf_seam_res_status_hook(void);

/**
 * @brief Seam hook for c_rest_response_cleanup fault injection.
 * @return C_REST_OK when not triggered, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t crf_seam_res_cleanup_hook(void);

/**
 * @brief Seam hook for c_rest_request_cleanup fault injection.
 * @return C_REST_OK when not triggered, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t crf_seam_req_cleanup_hook(void);

/**
 * @brief Seam hook for c_rest_router_get_openapi_spec fault injection.
 * @return C_REST_OK when not triggered, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_router_openapi_hook(void);

/**
 * @brief Seam countdown step for GraphQL whitespace skipping.
 * @return C_REST_OK when not triggered, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t crf_seam_graphql_ws_step(void);

/**
 * @brief Seam countdown step for GraphQL AST node deallocation.
 * @return C_REST_OK when not triggered, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t crf_seam_graphql_free_step(void);

/**
 * @brief Seam wrapper for c_rest_string_append_cstr in SSE module.
 * @param s Target dynamic string.
 * @param c Null-terminated C string to append.
 * @return C_REST_OK on success, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_sse_append_cstr(struct c_rest_string *s, const char *c);

/**
 * @brief Seam wrapper for c_rest_string_append in SSE module.
 * @param s Target dynamic string.
 * @param c Byte buffer to append.
 * @param l Length of byte buffer.
 * @return C_REST_OK on success, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_sse_append(struct c_rest_string *s, const char *c, size_t l);

/**
 * @brief Seam wrapper for c_rest_string_destroy in SSE module.
 * @param s Dynamic string to destroy.
 * @return C_REST_OK on success, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_sse_string_destroy(struct c_rest_string *s);

/**
 * @brief Seam wrapper for c_rest_sse_event_destroy in SSE module.
 * @param ev SSE event to destroy.
 * @return C_REST_OK on success, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_sse_event_destroy(struct c_rest_sse_event *ev);

/**
 * @brief Seam wrapper for c_rest_sse_event_init in SSE module.
 * @param ev SSE event to initialize.
 * @return C_REST_OK on success, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_sse_event_init(struct c_rest_sse_event *ev);

/**
 * @brief Seam countdown step for OpenAPI operation serialization.
 * @return C_REST_OK when not triggered, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t crf_seam_openapi_op_step(void);

/**
 * @brief Seam check for g_mock_openapi_fail matching code.
 * @param code Fault-injection code to check.
 * @return C_REST_ERROR_GENERIC if matched, C_REST_OK otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t crf_seam_openapi_check(int code);

/**
 * @brief Seam check for g_mock_hot_reload_fail matching code.
 * @param code Fault-injection code to check.
 * @param err Error enum value to return when matched.
 * @return err if matched, C_REST_OK otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_hot_reload_check(int code, c_rest_error_t err);

/**
 * @brief Seam wrapper for c_rest_sse_event_init in hot-reload module.
 * @param ev SSE event to initialize.
 * @return C_REST_OK on success, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_hr_sse_event_init(struct c_rest_sse_event *ev);

/**
 * @brief Seam wrapper for c_rest_sse_event_destroy in hot-reload module.
 * @param ev SSE event to destroy.
 * @return C_REST_OK on success, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_hr_sse_event_destroy(struct c_rest_sse_event *ev);

/**
 * @brief Seam check for g_mock_lib_fail matching code in compression module.
 * @param code Fault-injection code to check.
 * @return C_REST_ERROR_GENERIC if matched, C_REST_OK otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t crf_seam_lib_check(int code);

/**
 * @brief Seam wrapper for c_rest_response_set_status in JWT middleware.
 * @param r Response object.
 * @param s HTTP status code.
 * @return C_REST_OK on success, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_jwt_set_status(struct c_rest_response *r, int s);

/**
 * @brief Seam wrapper for c_rest_response_html in JWT middleware.
 * @param r Response object.
 * @param h HTML body string.
 * @return C_REST_OK on success, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_jwt_html(struct c_rest_response *r, const char *h);

/**
 * @brief Seam wrapper for c_rest_response_set_header in JWT middleware.
 * @param r Response object.
 * @param k Header key.
 * @param v Header value.
 * @return C_REST_OK on success, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_jwt_set_header(struct c_rest_response *r, const char *k,
                        const char *v);

/**
 * @brief Seam wrapper for c_rest_mutex_lock in memory tracker.
 * @param m Mutex handle.
 * @return C_REST_OK on success, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_mem_lock(c_rest_mutex_t m);

/**
 * @brief Seam wrapper for c_rest_mutex_unlock in memory tracker.
 * @param m Mutex handle.
 * @return C_REST_OK on success, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_mem_unlock(c_rest_mutex_t m);

/**
 * @brief Seam wrapper for c_rest_mutex_destroy in memory tracker.
 * @param m Mutex handle.
 * @return C_REST_OK on success, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_mem_destroy(c_rest_mutex_t m);

/**
 * @brief Seam wrapper for c_rest_mutex_create in rate limiter.
 * @param m Output mutex handle.
 * @return C_REST_OK on success, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_rl_mutex_create(c_rest_mutex_t *m);

/**
 * @brief Seam wrapper for c_rest_mutex_lock in rate limiter.
 * @param m Mutex handle.
 * @return C_REST_OK on success, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_rl_mutex_lock(c_rest_mutex_t m);

/**
 * @brief Seam wrapper for c_rest_hashmap_put in rate limiter.
 * @param m Hashmap pointer.
 * @param k Key string.
 * @param v Value pointer.
 * @return C_REST_OK on success, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_rl_hashmap_put(struct c_rest_hashmap *m, const char *k, void *v);

/**
 * @brief Seam wrapper for c_rest_mutex_unlock in rate limiter.
 * @param m Mutex handle.
 * @return C_REST_OK on success, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_rl_mutex_unlock(c_rest_mutex_t m);

/**
 * @brief Seam wrapper for c_rest_hashmap_destroy in rate limiter.
 * @param m Hashmap pointer.
 * @param f Value destructor callback.
 * @return C_REST_OK on success, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_rl_hashmap_destroy(struct c_rest_hashmap *m,
                            c_rest_error_t (*f)(void *));

/**
 * @brief Seam wrapper for c_rest_mutex_destroy in rate limiter.
 * @param m Mutex handle.
 * @return C_REST_OK on success, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_rl_mutex_destroy(c_rest_mutex_t m);

/**
 * @brief Seam wrapper for c_rest_cond_create in thread-safe queue.
 * @param c Output condition variable handle.
 * @return C_REST_OK on success, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_tsq_cond_create(c_rest_cond_t *c);

/**
 * @brief Seam wrapper for c_rest_mutex_destroy in thread-safe queue.
 * @param m Mutex handle.
 * @return C_REST_OK on success, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_tsq_mutex_destroy(c_rest_mutex_t m);

/**
 * @brief Seam wrapper for c_rest_mutex_lock in thread-safe queue.
 * @param m Mutex handle.
 * @return C_REST_OK on success, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_tsq_mutex_lock(c_rest_mutex_t m);

/**
 * @brief Seam wrapper for c_rest_cond_signal in thread-safe queue.
 * @param c Condition variable handle.
 * @return C_REST_OK on success, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_tsq_cond_signal(c_rest_cond_t c);

/**
 * @brief Seam wrapper for c_rest_mutex_unlock in thread-safe queue.
 * @param m Mutex handle.
 * @return C_REST_OK on success, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_tsq_mutex_unlock(c_rest_mutex_t m);

/**
 * @brief Seam wrapper for c_rest_cond_wait in thread-safe queue.
 * @param c Condition variable handle.
 * @param m Mutex handle.
 * @return C_REST_OK on success, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_tsq_cond_wait(c_rest_cond_t c, c_rest_mutex_t m);

/**
 * @brief Seam wrapper for c_rest_cond_destroy in thread-safe queue.
 * @param c Condition variable handle.
 * @return C_REST_OK on success, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_tsq_cond_destroy(c_rest_cond_t c);

/**
 * @brief Seam check for g_mock_client_fail matching code.
 * @param code Fault-injection code to check.
 * @return C_REST_ERROR_GENERIC if matched, C_REST_OK otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t crf_seam_client_check(int code);

/**
 * @brief Seam hook for c_rest_client_destroy fault injection.
 * @return C_REST_OK when not triggered, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_client_destroy_hook(void);

/**
 * @brief Seam hook for c_rest_client_response_free fault injection.
 * @return C_REST_OK when not triggered, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_client_res_free_hook(void);

/**
 * @brief Seam countdown step for client hex encoding.
 * @return C_REST_OK when not triggered, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t crf_seam_client_hex_step(void);

/**
 * @brief Seam hook for c_rest_client_url_decode fault injection.
 * @return C_REST_OK when not triggered, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_client_url_decode_hook(void);

/**
 * @brief Seam hook for c_rest_client_form_fields_free fault injection.
 * @return C_REST_OK when not triggered, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_client_form_free_hook(void);

/**
 * @brief Seam hook for c_rest_client_headers_free fault injection.
 * @return C_REST_OK when not triggered, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_client_headers_free_hook(void);

/**
 * @brief Seam countdown step for SHA-1 block compression.
 * @return C_REST_OK when not triggered, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_crypto_sha1_transform_step(void);

/**
 * @brief Seam hook for c_rest_sha1 fault injection.
 * @return C_REST_OK when not triggered, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t crf_seam_crypto_sha1_hook(void);

/**
 * @brief Seam countdown step for SHA-256 block compression.
 * @return C_REST_OK when not triggered, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_crypto_sha256_transform_step(void);

/**
 * @brief Seam hook for c_rest_sha256 fault injection.
 * @return C_REST_OK when not triggered, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_crypto_sha256_hook(void);

/**
 * @brief Seam wrapper for c_rest_sha256 calls inside crypto module.
 * @param data Input bytes.
 * @param len Input length.
 * @param hash Output 32-byte digest.
 * @return C_REST_OK on success, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_crypto_sha256_call(const unsigned char *data, size_t len,
                            unsigned char hash[32]);

/**
 * @brief Seam wrapper for c_rest_base64url_encode inside crypto module.
 * @param src Source bytes.
 * @param src_len Source length.
 * @param dst Destination string buffer.
 * @param dst_len Input/output encoded length.
 * @return C_REST_OK on success, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_crypto_b64url_enc(const unsigned char *src, size_t src_len, char *dst,
                           size_t *dst_len);

/**
 * @brief Seam wrapper for c_rest_base64_encode inside crypto module.
 * @param src Source bytes.
 * @param src_len Source length.
 * @param dst Destination string buffer.
 * @param dst_len Input/output encoded length.
 * @return C_REST_OK on success, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_crypto_b64_enc(const unsigned char *src, size_t src_len, char *dst,
                        size_t *dst_len);

/**
 * @brief Seam wrapper for c_rest_base64_decode inside crypto module.
 * @param src Source base64 string.
 * @param src_len Source string length.
 * @param dst Destination byte buffer.
 * @param dst_len Input/output decoded length.
 * @return C_REST_OK on success, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_crypto_b64_dec(const char *src, size_t src_len, unsigned char *dst,
                        size_t *dst_len);

/**
 * @brief Seam wrapper for c_rest_base64url_decode inside crypto module.
 * @param src Source base64url string.
 * @param src_len Source string length.
 * @param dst Destination byte buffer.
 * @param dst_len Input/output decoded length.
 * @return C_REST_OK on success, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_crypto_b64url_dec(const char *src, size_t src_len, unsigned char *dst,
                           size_t *dst_len);

/**
 * @brief Seam wrapper for c_rest_rand_bytes inside crypto module.
 * @param buf Output random buffer.
 * @param len Byte length.
 * @return C_REST_OK on success, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_crypto_rand_bytes(unsigned char *buf, size_t len);

/**
 * @brief Seam wrapper for c_rest_pbkdf2_hmac_sha256 inside crypto module.
 * @param password Password bytes.
 * @param password_len Password length.
 * @param salt Salt bytes.
 * @param salt_len Salt length.
 * @param iterations Iteration count.
 * @param dk_len Derived key length.
 * @param out_key Output derived key buffer.
 * @return C_REST_OK on success, error code otherwise.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
crf_seam_crypto_pbkdf2(const unsigned char *password, size_t password_len,
                       const unsigned char *salt, size_t salt_len,
                       unsigned long iterations, size_t dk_len,
                       unsigned char *out_key);

/** @brief Compression library fault-injection selector. */
C_REST_MOCK_DATA extern int g_mock_lib_fail;
/** @brief Hot-reload module fault-injection selector. */
C_REST_MOCK_DATA extern int g_mock_hot_reload_fail;
/** @brief JWT middleware fault-injection selector. */
C_REST_MOCK_DATA extern int g_mock_jwt_fail;
/** @brief Rate-limiter fault-injection selector. */
C_REST_MOCK_DATA extern int g_mock_rate_limit_fail;
/** @brief Memory tracker fault-injection selector. */
C_REST_MOCK_DATA extern int g_mock_mem_fail;
/** @brief Thread-safe queue fault-injection selector. */
C_REST_MOCK_DATA extern int g_mock_tsq_fail;
/** @brief SSE module fault-injection countdown/selector. */
C_REST_MOCK_DATA extern int g_mock_sse_append_fail;
/** @brief ORM init fault-injection flag. */
C_REST_MOCK_DATA extern int g_mock_orm_init_fail;
/** @brief Socket/modality fault-injection selector. */
C_REST_MOCK_DATA extern int g_mock_socket_fail;
/** @brief TLS module fault-injection selector. */
C_REST_MOCK_DATA extern int g_mock_tls_fail;
/** @brief ORM cleanup fault-injection flag. */
C_REST_MOCK_DATA extern int g_mock_orm_cleanup_fail;
/** @brief Crypto module fault-injection selector. */
C_REST_MOCK_DATA extern int g_mock_crypto_fail;
/** @brief Process fork/spawn fault-injection flag. */
C_REST_MOCK_DATA extern int g_mock_fork_fail;
/** @brief Multiplatform thread create fault-injection flag. */
C_REST_MOCK_DATA extern int g_mock_cm_thread_fail;
/** @brief Multiplatform thread join fault-injection flag. */
C_REST_MOCK_DATA extern int g_mock_cm_join_fail;
/** @brief Multiplatform file access fault-injection flag. */
C_REST_MOCK_DATA extern int g_mock_cm_file_fail;
/** @brief Response status fault-injection flag. */
C_REST_MOCK_DATA extern int g_mock_res_status_fail;
/** @brief Platform cleanup fault-injection flag. */
C_REST_MOCK_DATA extern int g_mock_platform_cleanup_fail;
/** @brief Parser basic vtable fault-injection flag. */
C_REST_MOCK_DATA extern int g_mock_parser_vtable_fail;
/** @brief Parser should_keep_alive fault-injection flag. */
C_REST_MOCK_DATA extern int g_mock_parser_should_keep_alive_fail;
/** @brief Parser destroy fault-injection flag. */
C_REST_MOCK_DATA extern int g_mock_parser_destroy_fail;
/** @brief Request cleanup fault-injection flag. */
C_REST_MOCK_DATA extern int g_mock_req_cleanup_fail;
/** @brief Response cleanup fault-injection flag. */
C_REST_MOCK_DATA extern int g_mock_res_cleanup_fail;
/** @brief GraphQL node_free fault-injection countdown. */
C_REST_MOCK_DATA extern int g_mock_graphql_free_countdown;
/** @brief GraphQL skip_whitespace fault-injection countdown. */
C_REST_MOCK_DATA extern int g_mock_graphql_ws_countdown;
/** @brief HTTP client fault-injection selector. */
C_REST_MOCK_DATA extern int g_mock_client_fail;
/** @brief OpenAPI module fault-injection selector. */
C_REST_MOCK_DATA extern int g_mock_openapi_fail;

/** @brief Internal pointer to memory tracker mutex for test inspection. */
C_REST_EXPORT extern c_rest_mutex_t *g_crf_mem_mutex_ptr;
/** @brief Internal pointer to memory tracker initialized flag for tests. */
C_REST_EXPORT extern int *g_crf_mem_initialized_ptr;

/** @brief Synchronous modality vtable exported for unit tests. */
C_REST_EXPORT extern const struct c_rest_modality_vtable sync_vtable;
/** @brief Single-thread modality vtable exported for unit tests. */
C_REST_EXPORT extern const struct c_rest_modality_vtable single_thread_vtable;
/** @brief Multi-thread modality vtable exported for unit tests. */
C_REST_EXPORT extern const struct c_rest_modality_vtable multi_thread_vtable;
/** @brief Event-loop async modality vtable exported for unit tests. */
C_REST_EXPORT extern const struct c_rest_modality_vtable async_vtable;
/** @brief Greenthread modality vtable exported for unit tests. */
C_REST_EXPORT extern const struct c_rest_modality_vtable greenthread_vtable;
/** @brief Message-passing modality vtable exported for unit tests. */
C_REST_EXPORT extern const struct c_rest_modality_vtable message_passing_vtable;
/** @brief Single-process modality vtable exported for unit tests. */
C_REST_EXPORT extern const struct c_rest_modality_vtable single_process_vtable;
#if !defined(__EMSCRIPTEN__) && !defined(CDD_DOS)
/** @brief Multi-process modality vtable exported for unit tests. */
C_REST_EXPORT extern const struct c_rest_modality_vtable multi_process_vtable;
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* C_REST_TESTING_MOCKS_H */
