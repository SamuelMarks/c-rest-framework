/**
 * @file c_multiplatform.h
 * @brief Multiplatform abstractions for the C REST framework.
 */
#ifndef C_MULTIPLATFORM_H
#define C_MULTIPLATFORM_H
/* clang-format off */
#include "c_rest_error.h"

#include <stddef.h>
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Opaque handle to a multiplatform environment. */
typedef void *cm_env_t;
/** @brief Opaque handle to a multiplatform socket. */
typedef void *cm_socket_t;
/** @brief Opaque handle to a multiplatform file. */
typedef void *cm_file_t;
/** @brief Opaque handle to a multiplatform thread. */
typedef void *cm_thread_t;
/** @brief Opaque handle to a multiplatform mutex. */
typedef void *cm_mutex_t;
/** @brief Opaque handle to a multiplatform condition variable. */
typedef void *cm_cond_t;

/** @brief Malloc callback function signature.
 *  @param size The number of bytes to allocate.
 *  @return A pointer to the allocated memory, or NULL on failure.
 */
typedef void *(*cm_malloc_fn)(size_t size);

/** @brief Free callback function signature.
 *  @param ptr A pointer to the memory to free.
 */
typedef void (*cm_free_fn)(void *ptr);

/** @brief Log callback function signature.
 *  @param msg The message to log.
 */
typedef void (*cm_log_fn)(const char *msg);

/** @brief Custom allocator structure. */
struct cm_allocator {
  /** @brief Malloc callback. */
  cm_malloc_fn malloc_cb;
  /** @brief Free callback. */
  cm_free_fn free_cb;
};

/** @brief Custom logger structure. */
struct cm_logger {
  /** @brief Log callback. */
  cm_log_fn log_cb;
};

/**
 * @brief Create a multiplatform environment.
 * @param out_env Pointer to where the environment handle will be stored.
 * @return C_REST_OK on success, or an error code on failure.
 */
c_rest_error_t cm_env_create(cm_env_t *out_env);

/**
 * @brief Destroy a multiplatform environment.
 * @param env The environment handle to destroy.
 * @return C_REST_OK on success, or an error code on failure.
 */
c_rest_error_t cm_env_destroy(cm_env_t env);

/**
 * @brief Set a custom allocator for the environment.
 * @param env The environment handle.
 * @param alloc Pointer to the allocator structure.
 * @return C_REST_OK on success, or an error code on failure.
 */
c_rest_error_t cm_env_set_allocator(cm_env_t env,
                                    const struct cm_allocator *alloc);

/**
 * @brief Set a custom logger for the environment.
 * @param env The environment handle.
 * @param logger Pointer to the logger structure.
 * @return C_REST_OK on success, or an error code on failure.
 */
c_rest_error_t cm_env_set_logger(cm_env_t env, const struct cm_logger *logger);

/**
 * @brief Create a socket.
 * @param env The environment handle.
 * @param out_sock Pointer to where the socket handle will be stored.
 * @return C_REST_OK on success, or an error code on failure.
 */
c_rest_error_t cm_socket_create(cm_env_t env, cm_socket_t *out_sock);

/**
 * @brief Bind a socket to a host and port.
 * @param env The environment handle.
 * @param sock The socket handle.
 * @param host The hostname or IP address to bind to.
 * @param port The port number.
 * @return C_REST_OK on success, or an error code on failure.
 */
c_rest_error_t cm_socket_bind(cm_env_t env, cm_socket_t sock, const char *host,
                              unsigned short port);

/**
 * @brief Listen for incoming connections on a socket.
 * @param env The environment handle.
 * @param sock The socket handle.
 * @param backlog The maximum number of pending connections.
 * @return C_REST_OK on success, or an error code on failure.
 */
c_rest_error_t cm_socket_listen(cm_env_t env, cm_socket_t sock, int backlog);

/**
 * @brief Accept an incoming connection on a socket.
 * @param env The environment handle.
 * @param server The server socket handle.
 * @param out_client Pointer to where the client socket handle will be stored.
 * @return C_REST_OK on success, or an error code on failure.
 */
c_rest_error_t cm_socket_accept(cm_env_t env, cm_socket_t server,
                                cm_socket_t *out_client);

/**
 * @brief Close a socket.
 * @param env The environment handle.
 * @param sock The socket handle to close.
 * @return C_REST_OK on success, or an error code on failure.
 */
c_rest_error_t cm_socket_close(cm_env_t env, cm_socket_t sock);

/**
 * @brief Set a socket to non-blocking mode.
 * @param env The environment handle.
 * @param sock The socket handle.
 * @param nonblocking 1 for non-blocking, 0 for blocking.
 * @return C_REST_OK on success, or an error code on failure.
 */
c_rest_error_t cm_socket_set_nonblocking(cm_env_t env, cm_socket_t sock,
                                         int nonblocking);

/**
 * @brief Create a thread.
 * @param env The environment handle.
 * @param out_thread Pointer to where the thread handle will be stored.
 * @param func The function for the thread to execute.
 * @param arg The argument to pass to the thread function.
 * @return C_REST_OK on success, or an error code on failure.
 */
c_rest_error_t cm_thread_create(cm_env_t env, cm_thread_t *out_thread,
                                void (*func)(void *), void *arg);

/**
 * @brief Wait for a thread to terminate.
 * @param env The environment handle.
 * @param thread The thread handle to wait for.
 * @return C_REST_OK on success, or an error code on failure.
 */
c_rest_error_t cm_thread_join(cm_env_t env, cm_thread_t thread);

/**
 * @brief Get the current time in milliseconds.
 * @param env The environment handle.
 * @param out_ms Pointer to where the time in milliseconds will be stored.
 * @return C_REST_OK on success, or an error code on failure.
 */
c_rest_error_t cm_timer_get_ms(cm_env_t env, unsigned long *out_ms);

/**
 * @brief Sleep for a specified number of milliseconds.
 * @param env The environment handle.
 * @param ms The number of milliseconds to sleep.
 * @return C_REST_OK on success, or an error code on failure.
 */
c_rest_error_t cm_thread_sleep_ms(cm_env_t env, unsigned long ms);

/**
 * @brief Get the modification time of a file.
 * @param env The environment handle.
 * @param path The path to the file.
 * @param out_mtime Pointer to where the modification time will be stored.
 * @return C_REST_OK on success, or an error code on failure.
 */
c_rest_error_t cm_file_get_mtime(cm_env_t env, const char *path,
                                 unsigned long *out_mtime);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* C_MULTIPLATFORM_H */
