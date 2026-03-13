#ifndef C_ABSTRACT_HTTP_H
#define C_ABSTRACT_HTTP_H

/* clang-format off */
#include <stddef.h>
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

/* Stub definitions for c-abstract-http */

typedef struct cah_parser cah_parser;
typedef struct cah_client cah_client;
typedef struct cah_request cah_request;
typedef struct cah_response cah_response;

typedef void (*cah_on_message_begin_cb)(cah_parser *parser);
typedef void (*cah_on_url_cb)(cah_parser *parser, const char *at,
                              size_t length);
typedef void (*cah_on_header_field_cb)(cah_parser *parser, const char *at,
                                       size_t length);
typedef void (*cah_on_header_value_cb)(cah_parser *parser, const char *at,
                                       size_t length);
typedef void (*cah_on_headers_complete_cb)(cah_parser *parser);
typedef void (*cah_on_body_cb)(cah_parser *parser, const char *at,
                               size_t length);
typedef void (*cah_on_message_complete_cb)(cah_parser *parser);

/** @brief Abstract HTTP parser settings */
struct cah_parser_settings {
  /** @brief Callback on message begin */
  cah_on_message_begin_cb on_message_begin;
  /** @brief Callback for URL */
  cah_on_url_cb on_url;
  /** @brief Callback on header field */
  cah_on_header_field_cb on_header_field;
  /** @brief Callback on header value */
  cah_on_header_value_cb on_header_value;
  /** @brief Callback on headers complete */
  cah_on_headers_complete_cb on_headers_complete;
  /** @brief Callback for body */
  cah_on_body_cb on_body;
  /** @brief Callback on message complete */
  cah_on_message_complete_cb on_message_complete;
};

/* Methods */
enum cah_method {
  CAH_DELETE,
  CAH_GET,
  CAH_HEAD,
  CAH_POST,
  CAH_PUT
  /* ... */
};
/** @brief Abstract HTTP parser */
struct cah_parser {
  /** @brief Data payload */
  void *data; /* user data */
  /** @brief HTTP major version */
  int http_major;
  /** @brief HTTP minor version */
  int http_minor;
  /** @brief HTTP status code */
  int status_code;
  /** @brief HTTP method */
  unsigned int method;
};

void cah_parser_init(cah_parser *parser);
size_t cah_parser_execute(cah_parser *parser,
                          const struct cah_parser_settings *settings,
                          const char *data, size_t len);
int cah_should_keep_alive(const cah_parser *parser);

/* Client functionality */
cah_client *cah_client_create(void);
void cah_client_destroy(cah_client *client);
int cah_client_request(cah_client *client, const char *url, const char *method);

/* TLS capabilities */
int cah_tls_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* C_ABSTRACT_HTTP_H */
