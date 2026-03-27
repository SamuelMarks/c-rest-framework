#ifndef C_REST_RESPONSE_H
#define C_REST_RESPONSE_H

/* clang-format off */
#include <stddef.h>
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

struct c_rest_header;
struct c_rest_request;

/** @brief HTTP response */
struct c_rest_response {
  /** @brief HTTP status code */
  int status_code;
  /** @brief HTTP headers */
  struct c_rest_header *headers;
  /** @brief Message body */
  char *body;
  /** @brief Length of message body */
  size_t body_len;
  /** @brief Context */
  void *context; /* Framework or connection context */
  /** @brief Headers sent flag */
  int headers_sent;
  /** @brief Chunked transfer flag */
  int is_chunked;
  /** @brief Content length */
  size_t content_length;
};

/**
 * @brief Set a header.
 * @param res Response
 * @param key Header key
 * @param value Header value
 * @return 0 on success, 1 on failure
 */
int c_rest_response_set_header(struct c_rest_response *res, const char *key,
                               const char *value);

/**
 * @brief Set the status code.
 * @param res Response
 * @param status_code Status code
 * @return 0 on success, 1 on failure
 */
int c_rest_response_set_status(struct c_rest_response *res, int status_code);

/**
 * @brief Check ETag and set status.
 * @param req Request
 * @param res Response
 * @param etag ETag value
 * @return 1 if matched (304), 0 otherwise
 */
int c_rest_response_check_etag(struct c_rest_request *req,
                               struct c_rest_response *res, const char *etag);

/**
 * @brief Set Cache-Control header.
 * @param res Response
 * @param policy Policy string
 * @return 0 on success, 1 on failure
 */
int c_rest_response_set_cache_control(struct c_rest_response *res,
                                      const char *policy);

/**
 * @brief Send the response headers.
 * @param res Response
 * @return 0 on success, 1 on failure
 */
int c_rest_response_send(struct c_rest_response *res);

/**
 * @brief Send JSON string.
 * @param res Response
 * @param json_str JSON string
 * @return 0 on success, 1 on failure
 */
int c_rest_response_json(struct c_rest_response *res, const char *json_str);

/**
 * @brief Send JSON object.
 * @param res Response
 * @param json_obj JSON object (parson JSON_Value)
 * @return 0 on success, 1 on failure
 */
int c_rest_response_json_obj(struct c_rest_response *res, void *json_obj);

/**
 * @brief JSON value types.
 */
enum c_rest_json_val_type {
  C_REST_JSON_TYPE_STRING,
  C_REST_JSON_TYPE_NUMBER,
  C_REST_JSON_TYPE_BOOLEAN,
  C_REST_JSON_TYPE_NULL
};

/**
 * @brief Key-value pair for basic JSON object construction.
 */
struct c_rest_json_pair {
  /** @brief Key */
  const char *key;
  /** @brief Value type */
  enum c_rest_json_val_type type;
  /** @brief String value */
  const char *str_val;
  /** @brief Number value */
  double num_val;
  /** @brief Boolean value (non-zero is true) */
  int bool_val;
};

/**
 * @brief Send a simple JSON dictionary constructed from an array of pairs.
 * @param res Response
 * @param pairs Array of key-value pairs
 * @param count Number of pairs
 * @return 0 on success, 1 on failure
 */
int c_rest_response_json_dict(struct c_rest_response *res,
                              const struct c_rest_json_pair *pairs,
                              size_t count);

/**
 * @brief Send HTML string.
 * @param res Response
 * @param html_str HTML string
 * @return 0 on success, 1 on failure
 */
int c_rest_response_html(struct c_rest_response *res, const char *html_str);

/**
 * @brief Write a chunk of data.
 * @param res Response
 * @param chunk Data chunk
 * @param chunk_len Chunk length
 * @return 0 on success, 1 on failure
 */
int c_rest_response_write_chunk(struct c_rest_response *res, const char *chunk,
                                size_t chunk_len);

/**
 * @brief Redirect to a URL.
 * @param res Response
 * @param url URL
 * @param status_code Status code
 * @return 0 on success, 1 on failure
 */
int c_rest_response_redirect(struct c_rest_response *res, const char *url,
                             int status_code);

/**
 * @brief Set a cookie.
 * @param res Response
 * @param key Cookie key
 * @param value Cookie value
 * @param attributes Additional attributes (e.g. "HttpOnly; Secure")
 * @return 0 on success, 1 on failure
 */
int c_rest_response_set_cookie(struct c_rest_response *res, const char *key,
                               const char *value, const char *attributes);

/**
 * @brief Send a file.
 * @param res Response
 * @param filepath File path
 * @return 0 on success, 1 on failure
 */
int c_rest_response_send_file(struct c_rest_response *res,
                              const char *filepath);

/**
 * @brief Cleanup the response.
 * @param res Response
 * @return 0 on success, 1 on failure
 */
int c_rest_response_cleanup(struct c_rest_response *res);

/**
 * @brief Serialize a response to an allocated buffer.
 * @param res Response
 * @param out_buf Output buffer (must be free()d)
 * @param out_len Output length
 * @return 0 on success, 1 on failure
 */
int c_rest_response_serialize(struct c_rest_response *res, char **out_buf,
                              size_t *out_len);

/**
 * @brief Emit an OAuth2 error schema.
 * @param res Response
 * @param error Error code
 * @param error_description Optional description
 * @return 0 on success, 1 on failure
 */
int c_rest_response_oauth2_error(struct c_rest_response *res, const char *error,
                                 const char *error_description);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* C_REST_RESPONSE_H */
