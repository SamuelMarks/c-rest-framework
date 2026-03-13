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
};

int c_rest_request_get_header(struct c_rest_request *req, const char *key,
                              const char **out_value);

int c_rest_request_get_cookie(struct c_rest_request *req, const char *key,
                              const char **out_value);

int c_rest_request_get_query(struct c_rest_request *req, const char *key,
                             const char **out_value);

int c_rest_request_read_body(struct c_rest_request *req, char **body,
                             size_t *body_len);

int c_rest_request_accepts_encoding(struct c_rest_request *req,
                                    const char *encoding);

int c_rest_request_parse_json(struct c_rest_request *req, void **json_obj);

int c_rest_request_cleanup(struct c_rest_request *req);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* C_REST_REQUEST_H */
