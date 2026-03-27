#ifndef C_REST_PARSER_H
#define C_REST_PARSER_H

/* clang-format off */
#include <stddef.h>
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Abstract parser interface for HTTP requests.
 * Allows swapping the underlying HTTP parser (e.g., c-abstract-http vs custom).
 */

struct c_rest_parser_vtable;
typedef struct c_rest_parser_context c_rest_parser_context;

/* Callbacks for parsed data */
typedef void (*c_rest_on_method_cb)(c_rest_parser_context *ctx,
                                    const char *method, size_t len);
typedef void (*c_rest_on_url_cb)(c_rest_parser_context *ctx, const char *url,
                                 size_t len);
typedef void (*c_rest_on_header_cb)(c_rest_parser_context *ctx, const char *key,
                                    size_t key_len, const char *value,
                                    size_t value_len);
typedef void (*c_rest_on_body_cb)(c_rest_parser_context *ctx, const char *data,
                                  size_t len);
typedef void (*c_rest_on_complete_cb)(c_rest_parser_context *ctx);
typedef void (*c_rest_on_error_cb)(c_rest_parser_context *ctx,
                                   const char *error_msg);

/** @brief Parser callbacks */
struct c_rest_parser_callbacks {
  /** @brief Callback for HTTP method */
  c_rest_on_method_cb on_method;
  /** @brief Callback for URL */
  c_rest_on_url_cb on_url;
  /** @brief Callback for header */
  c_rest_on_header_cb on_header;
  /** @brief Callback for body */
  c_rest_on_body_cb on_body;
  /** @brief Callback on completion */
  c_rest_on_complete_cb on_complete;
  /** @brief Callback on error */
  c_rest_on_error_cb on_error;
};

/** @brief Parser vtable */
struct c_rest_parser_vtable {
  /** @brief Initialization function */
  int (*init)(c_rest_parser_context *ctx,
              const struct c_rest_parser_callbacks *callbacks, void *user_data);
  /** @brief Execution function */
  int (*execute)(c_rest_parser_context *ctx, const char *data, size_t len,
                 size_t *out_parsed);
  /** @brief Check if connection should be kept alive */
  int (*should_keep_alive)(c_rest_parser_context *ctx, int *out_keep_alive);
  /** @brief Destruction function */
  int (*destroy)(c_rest_parser_context *ctx);
};

/** @brief Parser context */
struct c_rest_parser_context {
  /** @brief Virtual table for parser */
  const struct c_rest_parser_vtable *vtable;
  /** @brief Parser callbacks */
  struct c_rest_parser_callbacks callbacks;
  /** @brief User data pointer */
  void *user_data;
  /** @brief Internal state pointer */
  void *internal_state;
};

/* Wrapper functions */
int c_rest_parser_init(c_rest_parser_context *ctx,
                       const struct c_rest_parser_vtable *vtable,
                       const struct c_rest_parser_callbacks *callbacks,
                       void *user_data);
int c_rest_parser_execute(c_rest_parser_context *ctx, const char *data,
                          size_t len, size_t *out_parsed);
int c_rest_parser_should_keep_alive(c_rest_parser_context *ctx,
                                    int *out_keep_alive);
int c_rest_parser_destroy(c_rest_parser_context *ctx);

/**
 * @brief Check if parser reached complete state.
 * @param ctx Context
 * @return 1 if complete, 0 otherwise
 */
int c_rest_parser_is_complete(c_rest_parser_context *ctx);

/* Specific parser backends */
int c_rest_parser_get_basic_vtable(
    const struct c_rest_parser_vtable **out_vtable);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* C_REST_PARSER_H */
