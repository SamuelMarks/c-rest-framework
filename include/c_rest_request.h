#ifndef C_REST_REQUEST_H
#define C_REST_REQUEST_H

/* clang-format off */
#include <stddef.h>
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief HTTP header */
struct c_rest_header {
  /** @brief Key */
  char *key;
  /** @brief Value */
  char *value;
  /** @brief Pointer to next element */
  struct c_rest_header *next;
};

/** @brief Path variable */
struct c_rest_path_var {
  /** @brief Name */
  char *name;
  /** @brief Value */
  char *value;
  /** @brief Pointer to next element */
  struct c_rest_path_var *next;
};

/* Forward declaration to avoid exposing full c-orm headers directly */
struct c_orm_connection;

/** @brief HTTP request */
struct c_rest_request {
  /** @brief HTTP scheme (http or https) */
  const char *scheme;
  /** @brief HTTP method */
  const char *method;
  /** @brief Request path */
  const char *path;
  /** @brief Path variables */
  struct c_rest_path_var *path_vars;
  /** @brief HTTP headers */
  struct c_rest_header *headers;
  /** @brief Query parameters */
  struct c_rest_header *query_params;
  /** @brief Form parameters */
  struct c_rest_header *form_params;
  /** @brief Cookies */
  struct c_rest_header *cookies;
  /** @brief Message body */
  char *body;
  /** @brief Length of message body */
  size_t body_len;
  /** @brief Query string */
  char *query;
  /** @brief Database connection */
  struct c_orm_connection *db_conn; /* Scoped database connection */
  /** @brief Authenticated context */
  void *auth_context;
};

/**
 * @brief Get a header from the request.
 * @param req Request
 * @param key Header key
 * @param out_value Output value
 * @return 0 on success, 1 on failure
 */
int c_rest_request_get_header(struct c_rest_request *req, const char *key,
                              const char **out_value);

/**
 * @brief Get a cookie from the request.
 * @param req Request
 * @param key Cookie key
 * @param out_value Output value
 * @return 0 on success, 1 on failure
 */
int c_rest_request_get_cookie(struct c_rest_request *req, const char *key,
                              const char **out_value);

/**
 * @brief Get a query parameter from the request.
 * @param req Request
 * @param key Query parameter key
 * @param out_value Output value
 * @return 0 on success, 1 on failure
 */
int c_rest_request_get_query(struct c_rest_request *req, const char *key,
                             const char **out_value);

/**
 * @brief Parse the urlencoded body.
 * @param req Request
 * @return 0 on success, 1 on failure
 */
int c_rest_request_parse_urlencoded(struct c_rest_request *req);

/**
 * @brief Get a form parameter from the urlencoded body.
 * @param req Request
 * @param key Form parameter key
 * @param out_value Output value
 * @return 0 on success, 1 on failure
 */
int c_rest_request_get_form_param(struct c_rest_request *req, const char *key,
                                  const char **out_value);

/**
 * @brief Read the body of the request.
 * @param req Request
 * @param body Output body
 * @param body_len Output body length
 * @return 0 on success, 1 on failure
 */
int c_rest_request_read_body(struct c_rest_request *req, char **body,
                             size_t *body_len);

/**
 * @brief Check if the request accepts the encoding.
 * @param req Request
 * @param encoding Encoding
 * @return 1 if accepted, 0 otherwise
 */
int c_rest_request_accepts_encoding(struct c_rest_request *req,
                                    const char *encoding);

/**
 * @brief Parse the json body.
 * @param req Request
 * @param json_obj Output json object
 * @return 0 on success, 1 on failure
 */
int c_rest_request_parse_json(struct c_rest_request *req, void **json_obj);

/**
 * @brief Cleanup the request.
 * @param req Request
 * @return 0 on success, 1 on failure
 */
int c_rest_request_cleanup(struct c_rest_request *req);

/**
 * @brief Extract Bearer token from Authorization header.
 * @param req Request
 * @param out_token Output token
 * @return 0 on success, 1 on failure
 */
int c_rest_request_get_auth_bearer(struct c_rest_request *req,
                                   char **out_token);

/**
 * @brief Extract username and password from Basic Auth header.
 * @param req Request
 * @param out_username Output username
 * @param out_password Output password
 * @return 0 on success, 1 on failure
 */
int c_rest_request_get_auth_basic(struct c_rest_request *req,
                                  char **out_username, char **out_password);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* C_REST_REQUEST_H */
