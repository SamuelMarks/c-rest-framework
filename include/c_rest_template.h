#ifndef C_REST_TEMPLATE_H
#define C_REST_TEMPLATE_H

#ifdef C_REST_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING

/* clang-format off */
#include <stddef.h>
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file c_rest_template.h
 * @brief Server-Side Template Engine (HTML Rendering) API
 */

/**
 * @brief Template context structure.
 */
struct c_rest_template_context {
  /** @brief Pointer to the template string. */
  char *template_str;
  /** @brief Length of the template string. */
  size_t template_len;
};

/**
 * @brief Initializes a template context.
 *
 * @param ctx Pointer to the context to initialize.
 * @param template_str The template string.
 * @return 0 on success, non-zero error code on failure.
 */
int c_rest_template_init(struct c_rest_template_context *ctx,
                         const char *template_str);

/**
 * @brief Destroys a template context.
 *
 * @param ctx Pointer to the context to destroy.
 * @return 0 on success, non-zero error code on failure.
 */
int c_rest_template_destroy(struct c_rest_template_context *ctx);

/**
 * @brief Renders the template with provided values.
 *
 * @param ctx Pointer to the context.
 * @param keys Array of keys to replace.
 * @param values Array of corresponding values.
 * @param count Number of key-value pairs.
 * @param out_result Pointer to receive the allocated rendered string. Must be
 * freed.
 * @return 0 on success, non-zero error code on failure.
 */
int c_rest_template_render(const struct c_rest_template_context *ctx,
                           const char **keys, const char **values, size_t count,
                           char **out_result);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* C_REST_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING */
#endif /* C_REST_TEMPLATE_H */
