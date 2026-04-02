#ifndef C_REST_HTTP23_H
#define C_REST_HTTP23_H

/* clang-format off */
#include <stddef.h>
#include "c_rest_request.h"
#include "c_rest_response.h"
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

/* __cplusplus */

/**
 * @file c_rest_http23.h
 * @brief HTTP/2 and HTTP/3 Support module.
 *
 * This module provides the data structures and functions required to
 * handle HTTP/2 and HTTP/3 multiplexing, parsing, and framing within
 * the c-rest-framework.
 */

/**
 * @brief HTTP/2 and HTTP/3 error codes.
 */
typedef enum {
  C_REST_HTTP23_OK = 0,           /**< Success */
  C_REST_HTTP23_ERR_MEMORY = -1,  /**< Memory allocation failed */
  C_REST_HTTP23_ERR_INVALID = -2, /**< Invalid argument or state */
  C_REST_HTTP23_ERR_PROTOCOL = -3 /**< Protocol error */
} c_rest_http23_error_t;

/**
 * @brief Protocol version for the context.
 */
typedef enum {
  C_REST_PROTOCOL_HTTP2 = 2, /**< HTTP/2 protocol */
  C_REST_PROTOCOL_HTTP3 = 3  /**< HTTP/3 protocol */
} c_rest_protocol_t;

/**
 * @brief Opaque context for HTTP/2 & HTTP/3 handling.
 */
typedef struct c_rest_http23_ctx c_rest_http23_ctx_t;

/**
 * @brief Initialize a new HTTP/2 or HTTP/3 context.
 *
 * @param protocol The protocol version to use (2 or 3).
 * @param out_ctx Pointer to the pointer that will receive the context.
 * @return 0 on success, or a negative error code.
 */
int c_rest_http23_ctx_init(c_rest_protocol_t protocol,
                           c_rest_http23_ctx_t **out_ctx);

/**
 * @brief Destroy an HTTP/2 or HTTP/3 context.
 *
 * @param ctx The context to destroy.
 * @return 0 on success, or a negative error code.
 */
int c_rest_http23_ctx_destroy(c_rest_http23_ctx_t *ctx);

/**
 * @brief Process incoming data through the HTTP/2 or HTTP/3 state machine.
 *
 * @param ctx The context.
 * @param data Pointer to the incoming data buffer.
 * @param len Length of the incoming data.
 * @param out_consumed Pointer to store the number of bytes consumed.
 * @return 0 on success, or a negative error code.
 */
int c_rest_http23_process(c_rest_http23_ctx_t *ctx, const char *data,
                          size_t len, size_t *out_consumed);

/**
 * @brief Check if the current context has a completed request ready.
 *
 * @param ctx The context.
 * @param out_ready Pointer to store 1 if ready, 0 otherwise.
 * @return 0 on success, or a negative error code.
 */
int c_rest_http23_is_request_ready(c_rest_http23_ctx_t *ctx, int *out_ready);

/**
 * @brief Retrieve the completed request from the context.
 *
 * @param ctx The context.
 * @param out_request Pointer to store the request pointer.
 * @return 0 on success, or a negative error code.
 */
int c_rest_http23_get_request(c_rest_http23_ctx_t *ctx,
                              struct c_rest_request **out_request);

/**
 * @brief Format an HTTP/2 or HTTP/3 response frame from a response object.
 *
 * @param ctx The context.
 * @param response The response object.
 * @param out_buffer Pointer to store the allocated output buffer.
 * @param out_len Pointer to store the length of the output buffer.
 * @return 0 on success, or a negative error code.
 */
int c_rest_http23_format_response(c_rest_http23_ctx_t *ctx,
                                  struct c_rest_response *response,
                                  char **out_buffer, size_t *out_len);

/* __cplusplus */

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* C_REST_HTTP23_H */
