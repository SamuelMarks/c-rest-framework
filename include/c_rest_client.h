#ifndef C_REST_CLIENT_H
#define C_REST_CLIENT_H

/* clang-format off */
#include <stddef.h>
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct c_rest_client_context c_rest_client_context;

/**
 * @brief Represents a single HTTP header.
 */
struct c_rest_client_header {
  const char *key;   /**< The header key. */
  const char *value; /**< The header value. */
};

/**
 * @brief Represents an HTTP response from the client.
 */
struct c_rest_client_response {
  int status_code;                      /**< The HTTP status code. */
  struct c_rest_client_header *headers; /**< Array of response headers. */
  size_t headers_count;                 /**< Number of response headers. */
  void *body;                           /**< Response body data. */
  size_t body_len;                      /**< Length of the response body. */
};

/**
 * @brief Key-value pair for x-www-form-urlencoded payloads.
 */
struct c_rest_client_form_field {
  const char *key;   /**< The field key. */
  const char *value; /**< The field value. */
};

/**
 * @brief Initializes a new HTTP client context.
 * @param out_client Pointer to store the initialized context.
 * @return 0 on success, or an error code.
 */
int c_rest_client_init(c_rest_client_context **out_client);

/**
 * @brief Destroys an HTTP client context.
 * @param client The context to destroy.
 * @return 0 on success, or an error code.
 */
int c_rest_client_destroy(c_rest_client_context *client);

/**
 * @brief Executes a synchronous HTTP request.
 * @param client The client context.
 * @param url The target URL.
 * @param method The HTTP method (e.g., "GET", "POST").
 * @param headers Array of request headers.
 * @param headers_count Number of request headers.
 * @param body Pointer to request body data.
 * @param body_len Length of request body data.
 * @param out_res Pointer to store the resulting response.
 * @return 0 on success, or an error code.
 */
int c_rest_client_request_sync(c_rest_client_context *client, const char *url,
                               const char *method,
                               const struct c_rest_client_header *headers,
                               size_t headers_count, const void *body,
                               size_t body_len,
                               struct c_rest_client_response **out_res);

/**
 * @brief Executes an asynchronous HTTP request.
 * @param client The client context.
 * @param url The target URL.
 * @param method The HTTP method (e.g., "GET", "POST").
 * @param headers Array of request headers.
 * @param headers_count Number of request headers.
 * @param body Pointer to request body data.
 * @param body_len Length of request body data.
 * @param callback Callback to invoke upon completion.
 * @param user_data User data to pass to the callback.
 * @return 0 on success, or an error code.
 */
int c_rest_client_request_async(
    c_rest_client_context *client, const char *url, const char *method,
    const struct c_rest_client_header *headers, size_t headers_count,
    const void *body, size_t body_len,
    void (*callback)(struct c_rest_client_response *res, void *data),
    void *user_data);

/**
 * @brief Frees a client response.
 * @param res The response to free.
 * @return 0 on success, or an error code.
 */
int c_rest_client_response_free(struct c_rest_client_response *res);

/**
 * @brief Builds a URL-encoded form string.
 * @param fields Array of form fields.
 * @param num_fields Number of form fields.
 * @param out_body Pointer to store the newly allocated encoded string.
 * @param out_len Pointer to store the length of the string.
 * @return 0 on success, or an error code.
 */
int c_rest_client_build_form_urlencoded(
    const struct c_rest_client_form_field *fields, size_t num_fields,
    char **out_body, size_t *out_len);

/**
 * @brief URL encodes a string.
 * @param in_str The input string.
 * @param out_str Pointer to store the newly allocated encoded string.
 * @return 0 on success, or an error code.
 */
int c_rest_client_url_encode(const char *in_str, char **out_str);

/**
 * @brief URL decodes a string.
 * @param in_str The input string.
 * @param out_str Pointer to store the newly allocated decoded string.
 * @return 0 on success, or an error code.
 */
int c_rest_client_url_decode(const char *in_str, char **out_str);

/**
 * @brief Basic reverse proxy stub utility.
 * @param target_url The URL to proxy to.
 * @param req The request.
 * @param res The response.
 * @return 0 on success, or an error code.
 */
int c_rest_proxy_request(const char *target_url, void *req, void *res);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* C_REST_CLIENT_H */
