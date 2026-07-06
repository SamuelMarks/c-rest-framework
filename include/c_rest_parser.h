/**
 * @file c_rest_parser.h
 * @brief Header file for c_rest_parser.h
 */
#ifndef C_REST_PARSER_H
#define C_REST_PARSER_H
/* clang-format off */
#include "c_rest_error.h"

#include <stddef.h>
#include "c_rest_error.h"
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Abstract parser interface for HTTP requests.
 * Allows swapping the underlying HTTP parser (e.g., c-abstract-http vs custom).
 */

struct c_rest_parser_vtable;
/** @brief Opaque context for the HTTP parser. */
typedef struct c_rest_parser_context c_rest_parser_context;

/* Callbacks for parsed data */
/** @brief Callback for parsed method.
 * @param ctx Parser context.
 * @param method Parsed method.
 * @param len Length of method.
 * @return 0 on success. */
typedef c_rest_error_t (*c_rest_on_method_cb)(c_rest_parser_context *ctx,
                                              const char *method, size_t len);
/** @brief Callback for parsed URL.
 * @param ctx Parser context.
 * @param url Parsed URL.
 * @param len Length of URL.
 * @return 0 on success. */
typedef c_rest_error_t (*c_rest_on_url_cb)(c_rest_parser_context *ctx,
                                           const char *url, size_t len);
/** @brief Callback for parsed header.
 * @param ctx Parser context.
 * @param key Header key.
 * @param key_len Key length.
 * @param value Header value.
 * @param value_len Value length.
 * @return 0 on success. */
typedef c_rest_error_t (*c_rest_on_header_cb)(c_rest_parser_context *ctx,
                                              const char *key, size_t key_len,
                                              const char *value,
                                              size_t value_len);
/** @brief Callback for parsed body chunk.
 * @param ctx Parser context.
 * @param data Body chunk.
 * @param len Chunk length.
 * @return 0 on success. */
typedef c_rest_error_t (*c_rest_on_body_cb)(c_rest_parser_context *ctx,
                                            const char *data, size_t len);
/** @brief Callback for parsing complete.
 * @param ctx Parser context.
 * @return 0 on success. */
typedef c_rest_error_t (*c_rest_on_complete_cb)(c_rest_parser_context *ctx);
/** @brief Callback for parsing error.
 * @param ctx Parser context.
 * @param error_msg Error message.
 * @return 0 on success. */
typedef c_rest_error_t (*c_rest_on_error_cb)(c_rest_parser_context *ctx,
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
  c_rest_error_t (*init)(c_rest_parser_context *ctx,
                         const struct c_rest_parser_callbacks *callbacks,
                         void *user_data);
  /** @brief Execution function */
  c_rest_error_t (*execute)(c_rest_parser_context *ctx, const char *data,
                            size_t len, size_t *out_parsed);
  /** @brief Check if connection should be kept alive */
  c_rest_error_t (*should_keep_alive)(c_rest_parser_context *ctx,
                                      int *out_keep_alive);
  /** @brief Destruction function */
  c_rest_error_t (*destroy)(c_rest_parser_context *ctx);
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
/**
 * @brief Initialize a parser context.
 * @param ctx The parser context.
 * @param vtable The parser virtual table.
 * @param callbacks The parser callbacks.
 * @param user_data User data for callbacks.
 * @return 0 on success, error code otherwise.
 */
c_rest_error_t c_rest_parser_init(
    c_rest_parser_context *ctx, const struct c_rest_parser_vtable *vtable,
    const struct c_rest_parser_callbacks *callbacks, void *user_data);
/**
 * @brief Execute parsing on a chunk of data.
 * @param ctx The parser context.
 * @param data The data chunk.
 * @param len The length of the chunk.
 * @param out_parsed Pointer to store number of parsed bytes.
 * @return 0 on success, error code otherwise.
 */
c_rest_error_t c_rest_parser_execute(c_rest_parser_context *ctx,
                                     const char *data, size_t len,
                                     size_t *out_parsed);
/**
 * @brief Check if the connection should be kept alive.
 * @param ctx The parser context.
 * @param out_keep_alive Pointer to store result (1 if true, 0 if false).
 * @return 0 on success, error code otherwise.
 */
c_rest_error_t c_rest_parser_should_keep_alive(c_rest_parser_context *ctx,
                                               int *out_keep_alive);
/**
 * @brief Destroy the parser context.
 * @param ctx The parser context.
 * @return 0 on success, error code otherwise.
 */
c_rest_error_t c_rest_parser_destroy(c_rest_parser_context *ctx);

/**
 * @brief Check if parser reached complete state.
 * @param ctx Context
 * @return 1 if complete, 0 otherwise
 */
c_rest_error_t c_rest_parser_is_complete(c_rest_parser_context *ctx);

/* Specific parser backends */
/**
 * @brief Get the basic parser vtable.
 * @param out_vtable Pointer to store the vtable.
 * @return 0 on success.
 */
c_rest_error_t
c_rest_parser_get_basic_vtable(const struct c_rest_parser_vtable **out_vtable);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* C_REST_PARSER_H */
