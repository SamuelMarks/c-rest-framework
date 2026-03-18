#ifndef C_REST_MODALITY_H
#define C_REST_MODALITY_H

/* clang-format off */
#include <stddef.h>

#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
#include "c_multiplatform.h"
#endif

#include "c_rest_orm.h"
#include "c_rest_tls.h"
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief The execution modalities supported by the framework.
 *
 * Developers initialize the framework by selecting one of these modalities.
 */
enum c_rest_modality_type {
  C_REST_MODALITY_SYNC,           /**< Synchronous blocking operations */
  C_REST_MODALITY_ASYNC,          /**< Asynchronous non-blocking event loop */
  C_REST_MODALITY_SINGLE_THREAD,  /**< Execute strictly on a single thread */
  C_REST_MODALITY_MULTI_THREAD,   /**< Utilize a thread pool for requests */
  C_REST_MODALITY_SINGLE_PROCESS, /**< Execute strictly in a single process */
  C_REST_MODALITY_MULTI_PROCESS,  /**< Pre-fork worker processes */
  C_REST_MODALITY_GREENTHREAD,    /**< Coroutine-based apparent concurrency */
  C_REST_MODALITY_MESSAGE_PASSING /**< Actor-like message queue processing */
};

/**
 * @brief Memory allocation callback signature.
 * @param size The number of bytes to allocate.
 * @return A pointer to the newly allocated memory or NULL if out of memory.
 */
typedef void *(*c_rest_malloc_fn)(size_t size);

/**
 * @brief Memory deallocation callback signature.
 * @param ptr The pointer to free.
 */
typedef void (*c_rest_free_fn)(void *ptr);

/**
 * @brief Logging callback signature.
 * @param message The log message string.
 */
typedef void (*c_rest_log_fn)(const char *message);

/**
 * @brief Represents the custom memory allocator callbacks.
 */
/** @brief C Rest Allocator */
struct c_rest_allocator {
  /** @brief Malloc callback */
  c_rest_malloc_fn malloc_cb; /**< Pointer to malloc-like function */
  /** @brief Free callback */
  c_rest_free_fn free_cb; /**< Pointer to free-like function */
};

/**
 * @brief Represents the logging infrastructure.
 */
/** @brief C Rest Logger */
struct c_rest_logger {
  /** @brief Log callback */
  c_rest_log_fn log_cb; /**< Pointer to the logging function */
};

struct c_rest_context;

/**
 * @brief Virtual method table for internal modality engine dispatch.
 */
/** @brief C Rest Modality Vtable */
struct c_rest_modality_vtable {
  /**
   * @brief Initializes the underlying modality state.
   * @param ctx Pointer to the context struct.
   * @return 0 on success, non-zero error code on failure.
   */
  /** @brief Initialization function */
  int (*init)(struct c_rest_context *ctx);

  /**
   * @brief Cleans up and destroys the underlying modality state.
   * @param ctx Pointer to the context struct.
   * @return 0 on success, non-zero error code on failure.
   */
  /** @brief Destruction function */
  int (*destroy)(struct c_rest_context *ctx);

  /**
   * @brief Starts the underlying modality engine (e.g. event loop, threads).
   * @param ctx Pointer to the context struct.
   * @return 0 on success, non-zero error code on failure.
   */
  int (*run)(struct c_rest_context *ctx);

  /**
   * @brief Signals the underlying modality engine to stop execution.
   * @param ctx Pointer to the context struct.
   * @return 0 on success, non-zero error code on failure.
   */
  int (*stop)(struct c_rest_context *ctx);
};

/**
 * @brief Global state and context for the framework instance.
 */
/** @brief C Rest Context */
struct c_rest_context {
  /** @brief Modality */
  enum c_rest_modality_type modality; /**< The selected modality */
  /** @brief Allocator */
  struct c_rest_allocator allocator; /**< The memory allocator */
  /** @brief Logger */
  struct c_rest_logger logger; /**< The logging backend */
  /** @brief Const struct c rest modality vtable */
  const struct c_rest_modality_vtable
      /** @brief Virtual table for parser */
      *vtable; /**< Pointer to implementation hooks */
  /** @brief Internal state pointer */
  void *internal_state;         /**< Opaque internal state pointer */
  struct c_rest_router *router; /**< Associated router instance */

  /** @brief Listen address (e.g. "0.0.0.0") */
  const char *listen_address;
  /** @brief Listen port (e.g. 8080) */
  unsigned short listen_port;

  /** @brief Db config */
  struct c_rest_db_config db_config; /**< Database configuration */
  /** @brief Db pool */
  struct c_orm_pool *db_pool; /**< Connection pool instance */

  /** @brief TLS context */
  struct c_rest_tls_context *tls_ctx; /**< The server TLS context */

#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
  /** @brief Cm env */
  cm_env_t cm_env; /**< Optional multiplatform environment context */
#endif
};

/**
 * @brief Initializes a new c-rest-framework context with the desired modality.
 * @param type The selected c_rest_modality_type to use for this instance.
 * @param out_ctx Pointer to store the initialized struct pointer.
 * @return 0 on success, non-zero error code on failure.
 */
int c_rest_init(enum c_rest_modality_type type,
                struct c_rest_context **out_ctx);

/**
 * @brief Starts the framework engine and begins accepting connections.
 * @param ctx Pointer to the initialized context struct.
 * @return 0 on success, non-zero error code on failure.
 */
int c_rest_run(struct c_rest_context *ctx);

/**
 * @brief Signals the framework engine to stop accepting connections and shutdown gracefully.
 * @param ctx Pointer to the initialized context struct.
 * @return 0 on success, non-zero error code on failure.
 */
int c_rest_stop(struct c_rest_context *ctx);

/**
 * @brief Destroys and cleans up a framework context instance.
 * @param ctx Pointer to the context struct to destroy.
 * @return 0 on success, non-zero error code on failure.
 */
int c_rest_destroy(struct c_rest_context *ctx);

/**
 * @brief Set router for framework instance.
 * @param ctx Context
 * @param router Router
 * @return 0 on success
 */
int c_rest_set_router(struct c_rest_context *ctx, struct c_rest_router *router);

#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
/**
 * @brief Attaches a c-multiplatform environment to the framework context.
 * @param ctx Pointer to the framework context.
 * @param env The c-multiplatform environment instance.
 * @return 0 on success, non-zero error code on failure.
 */
int c_rest_set_multiplatform_env(struct c_rest_context *ctx, cm_env_t env);
#endif

/**
 * @brief Connection context state
 */
struct c_rest_connection_context {
  /** @brief Platform socket */
  c_rest_socket_t sock;
  /** @brief Active TLS Connection */
  struct c_rest_tls_connection *tls_conn;
#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
  /** @brief Environment context */
  cm_env_t cm_env;
#endif
};

/**
 * @brief Handle a single complete HTTP connection.
 * @param ctx Context
 * @param sock Socket
 * @return 0 on success
 */
int c_rest_handle_connection(struct c_rest_context *ctx, c_rest_socket_t sock);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* C_REST_MODALITY_H */
