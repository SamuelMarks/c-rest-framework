#ifndef C_REST_SSE_H
#define C_REST_SSE_H

/* clang-format off */
#include <stddef.h>
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Error codes for SSE operations.
 */
enum c_rest_sse_error {
  C_REST_SSE_OK = 0,
  C_REST_SSE_ERR_NOMEM = -1,
  C_REST_SSE_ERR_INVALID_ARG = -2,
  C_REST_SSE_ERR_PARSE_FAILED = -3
};

/**
 * @brief Represents an SSE event to be sent or received.
 */
struct c_rest_sse_event {
  char *id;    /**< Optional event ID */
  char *event; /**< Optional event name */
  char *data;  /**< Event data payload (can contain newlines) */
  int retry;   /**< Reconnection time in milliseconds (-1 if not set) */
};

/**
 * @brief Forward declaration for parsing context.
 */
struct c_rest_sse_context;

/**
 * @brief Initializes an SSE event struct.
 * @param ev Pointer to the event struct to initialize.
 * @return 0 on success, non-zero on error.
 */
int c_rest_sse_event_init(struct c_rest_sse_event *ev);

/**
 * @brief Cleans up memory allocated within an SSE event struct.
 * @param ev Pointer to the event struct.
 * @return 0 on success, non-zero on error.
 */
int c_rest_sse_event_destroy(struct c_rest_sse_event *ev);

/**
 * @brief Deep copies an SSE event.
 * @param src The source event.
 * @param dest The destination event to initialize and copy into.
 * @return 0 on success, non-zero on error.
 */
int c_rest_sse_event_clone(const struct c_rest_sse_event *src,
                           struct c_rest_sse_event *dest);

/**
 * @brief Serializes an SSE event into a string buffer.
 * @param ev The event to serialize.
 * @param out_buf Pointer to receive the allocated string. Caller must free.
 * @param out_len Pointer to receive the length of the string.
 * @return 0 on success, non-zero on error.
 */
int c_rest_sse_serialize(const struct c_rest_sse_event *ev, char **out_buf,
                         size_t *out_len);

/**
 * @brief Initializes an SSE parsing context.
 * @param out_ctx Pointer to receive the context pointer.
 * @return 0 on success, non-zero on error.
 */
int c_rest_sse_context_init(struct c_rest_sse_context **out_ctx);

/**
 * @brief Destroys an SSE parsing context.
 * @param ctx The context to destroy.
 * @return 0 on success, non-zero on error.
 */
int c_rest_sse_context_destroy(struct c_rest_sse_context *ctx);

/**
 * @brief Parses incoming SSE stream data.
 * @param ctx The parsing context.
 * @param data The incoming data buffer.
 * @param len The length of the incoming data.
 * @param out_event Pointer to receive the parsed event, if complete.
 * @return 0 on success (event parsed), 1 if more data is needed, or a negative
 * error code.
 */
int c_rest_sse_parse(struct c_rest_sse_context *ctx, const char *data,
                     size_t len, struct c_rest_sse_event *out_event);

struct c_rest_response;

/**
 * @brief Prepares and sends the HTTP headers for an SSE response.
 * @param res The response object.
 * @return 0 on success, non-zero on failure.
 */
int c_rest_sse_init_response(struct c_rest_response *res);

/**
 * @brief Sends an SSE event to the client over the response connection.
 * @param res The response object.
 * @param ev The event to send.
 * @return 0 on success, non-zero on failure.
 */
int c_rest_sse_send_event(struct c_rest_response *res,
                          const struct c_rest_sse_event *ev);

/**
 * @brief Sends an SSE keep-alive comment to prevent timeouts.
 * @param res The response object.
 * @return 0 on success, non-zero on failure.
 */
int c_rest_sse_send_keepalive(struct c_rest_response *res);

/* __cplusplus */

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* C_REST_SSE_H */
