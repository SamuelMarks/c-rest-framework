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

int c_rest_response_set_header(struct c_rest_response *res, const char *key,
                               const char *value);

int c_rest_response_set_status(struct c_rest_response *res, int status_code);

int c_rest_response_check_etag(struct c_rest_request *req,
                               struct c_rest_response *res, const char *etag);

int c_rest_response_set_cache_control(struct c_rest_response *res,
                                      const char *policy);

int c_rest_response_send(struct c_rest_response *res);

int c_rest_response_json(struct c_rest_response *res, const char *json_str);

int c_rest_response_html(struct c_rest_response *res, const char *html_str);

int c_rest_response_write_chunk(struct c_rest_response *res, const char *chunk,
                                size_t chunk_len);

int c_rest_response_redirect(struct c_rest_response *res, const char *url,
                             int status_code);

int c_rest_response_set_cookie(struct c_rest_response *res, const char *key,
                               const char *value, const char *attributes);

int c_rest_response_send_file(struct c_rest_response *res,
                              const char *filepath);

void c_rest_response_cleanup(struct c_rest_response *res);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* C_REST_RESPONSE_H */
