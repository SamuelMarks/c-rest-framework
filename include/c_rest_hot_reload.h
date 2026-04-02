#ifndef C_REST_HOT_RELOAD_H
#define C_REST_HOT_RELOAD_H

/* clang-format off */
#include <stddef.h>
#include "c_rest_modality.h"
#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
#include "c_multiplatform.h"
/* clang-format on */
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* __cplusplus */

/**
 * \file c_rest_hot_reload.h
 * \brief Hot-Reloading and Auto-Restart API.
 *
 * This API provides functionality to watch file system changes and trigger
 * application reloads or restarts. It adheres to strict C89 compliance.
 */

/**
 * \struct c_rest_hot_reload_ctx_t
 * \brief Opaque context for hot reloading and auto-restarting.
 */
typedef struct c_rest_hot_reload_ctx c_rest_hot_reload_ctx_t;

/**
 * \typedef c_rest_hot_reload_callback_t
 * \brief Callback function type invoked when a watched file changes.
 * \param user_data User-provided data pointer.
 * \return 0 on success, non-zero error code otherwise.
 */
typedef int (*c_rest_hot_reload_callback_t)(void *user_data);

/**
 * \enum c_rest_hot_reload_errc_t
 * \brief Error codes for hot reloading operations.
 */
typedef enum c_rest_hot_reload_errc {
  C_REST_HOT_RELOAD_SUCCESS = 0,
  C_REST_HOT_RELOAD_ERR_ALLOC,
  C_REST_HOT_RELOAD_ERR_PARAM,
  C_REST_HOT_RELOAD_ERR_SYSTEM
} c_rest_hot_reload_errc_t;

/**
 * \brief Initialize a new hot reload context.
 * \param out_ctx Pointer to the pointer that will receive the context.
 * \param logger Optional logger to use for debug tracing.
 * \return 0 on success, or a c_rest_hot_reload_errc_t value on failure.
 */
int c_rest_hot_reload_init(c_rest_hot_reload_ctx_t **out_ctx,
                           struct c_rest_logger *logger);

/**
 * \brief Set the multiplatform environment for hot reloading.
 * \param ctx
 * The hot reload context.
 * \param env The multiplatform environment.
 *
 * \return 0 on success, non-zero on failure.
 */
#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
int c_rest_hot_reload_set_multiplatform_env(c_rest_hot_reload_ctx_t *ctx,
                                            cm_env_t env);
#endif

/**
 * \brief Add a file or directory to watch for changes.
 * \param ctx The hot reload context.
 * \param path The file or directory path to watch.
 * \return 0 on success, or a c_rest_hot_reload_errc_t value on failure.
 */
int c_rest_hot_reload_add_watch(c_rest_hot_reload_ctx_t *ctx, const char *path);

/**
 * \brief Start the hot reload monitoring loop (blocking or polling).
 * \param ctx The hot reload context.
 * \param on_reload The callback to invoke on change detection.
 * \param user_data User data to pass to the callback.
 * \return 0 on success, or a c_rest_hot_reload_errc_t value on failure.
 */
int c_rest_hot_reload_start(c_rest_hot_reload_ctx_t *ctx,
                            c_rest_hot_reload_callback_t on_reload,
                            void *user_data);

/**
 * \brief Poll for file system changes and invoke the callback if any are found.
 * \param ctx The hot reload context.
 * \param on_reload The callback to invoke on change detection.
 * \param user_data User data to pass to the callback.
 * \return 0 on success, or a c_rest_hot_reload_errc_t value on failure.
 */
int c_rest_hot_reload_poll(c_rest_hot_reload_ctx_t *ctx,
                           c_rest_hot_reload_callback_t on_reload,
                           void *user_data);

/**
 * \brief Destroy a hot reload context, freeing resources.
 * \param ctx The context to destroy.
 * \return 0 on success, or a c_rest_hot_reload_errc_t value on failure.
 */
int c_rest_hot_reload_destroy(c_rest_hot_reload_ctx_t *ctx);

struct c_rest_router;

/**
 * \brief Register an SSE endpoint for Hot-Reloading / Auto-Restart.
 * \param router The router instance.
 * \param path The path to mount the endpoint (e.g., "/_hot_reload").
 * \return 0 on success, non-zero on failure.
 */
int c_rest_hot_reload_register_routes(struct c_rest_router *router,
                                      const char *path);

/* __cplusplus */

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* C_REST_HOT_RELOAD_H */
