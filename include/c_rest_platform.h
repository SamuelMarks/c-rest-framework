/**
 * @file c_rest_platform.h
 * @brief Header file for c_rest_platform.h
 */
#ifndef C_REST_PLATFORM_H
#define C_REST_PLATFORM_H
/* clang-format off */
#include "c_rest_error.h"

#include <stddef.h>
#include "c_rest_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
#include "c_multiplatform.h"

/** @brief Socket handle type. */
typedef cm_socket_t c_rest_socket_t;
/** @brief Thread handle type. */
typedef cm_thread_t c_rest_thread_t;
/** @brief Mutex handle type. */
typedef cm_mutex_t c_rest_mutex_t;
/** @brief Condition variable handle type. */
typedef cm_cond_t c_rest_cond_t;
/** @brief Process handle type. */
typedef void *c_rest_process_t; /* c-multiplatform doesn't have processes */

/** @brief Invalid socket constant. */
#define C_REST_INVALID_SOCKET ((c_rest_socket_t)0)

#else

/*
 * Platform-agnostic handles.
 * We use ptrdiff_t internally so that it can safely hold an int (POSIX fd)
 * or a void pointer handle on Windows.
 */
/** @brief Socket handle type. */
typedef ptrdiff_t c_rest_socket_t;
/** @brief Thread handle type. */
typedef ptrdiff_t c_rest_thread_t;
/** @brief Mutex handle type. */
typedef ptrdiff_t c_rest_mutex_t;
/** @brief Condition variable handle type. */
typedef ptrdiff_t c_rest_cond_t;
/** @brief Process handle type. */
typedef ptrdiff_t c_rest_process_t;

/** @brief Invalid socket constant. */
#define C_REST_INVALID_SOCKET ((c_rest_socket_t) - 1)

#endif

#include <cfs/cfs.h>
/* clang-format on */

/* Format specifiers for cross-platform C89 compliance */
#if defined(_MSC_VER)
/** @brief Format specifier for size_t. */
#define C_REST_FMT_SIZE_T "%lu"
/** @brief Cast for size_t printing. */
#define CAST_SIZE_T(x) ((unsigned long)(x))
/** @brief Format specifier for time_t. */
#define C_REST_FMT_TIME_T "%I64d"
/** @brief Cast for time_t printing. */
#define CAST_TIME_T(x) ((__int64)(x))
#else
/** @brief Format specifier for size_t. */
#define C_REST_FMT_SIZE_T "%lu"
/** @brief Cast for size_t printing. */
#define CAST_SIZE_T(x) ((unsigned long)(x))
/** @brief Format specifier for time_t. */
#define C_REST_FMT_TIME_T "%ld"
/** @brief Cast for time_t printing. */
#define CAST_TIME_T(x) ((long)(x))
#endif

/*
 * Initialization and cleanup.
 */
/** @brief Initialize platform layer.
 * @return 0 on success. */
c_rest_error_t c_rest_platform_init(void);
/** @brief Cleanup platform layer.
 * @return 0 on success. */
c_rest_error_t c_rest_platform_cleanup(void);

/*
 * Sockets
 */
/** @brief Create a socket.
 * @param out_sock Pointer to store socket.
 * @return 0 on success. */
c_rest_error_t c_rest_socket_create(c_rest_socket_t *out_sock);
/** @brief Bind a socket.
 * @param sock The socket.
 * @param host The host to bind to.
 * @param port The port.
 * @return 0 on success. */
c_rest_error_t c_rest_socket_bind(c_rest_socket_t sock, const char *host,
                                  unsigned short port);
/** @brief Listen on a socket.
 * @param sock The socket.
 * @param backlog Backlog size.
 * @return 0 on success. */
c_rest_error_t c_rest_socket_listen(c_rest_socket_t sock, int backlog);
/** @brief Accept a connection.
 * @param server_sock Server socket.
 * @param out_client_sock Pointer to store client socket.
 * @return 0 on success. */
c_rest_error_t c_rest_socket_accept(c_rest_socket_t server_sock,
                                    c_rest_socket_t *out_client_sock);
/** @brief Set non-blocking mode.
 * @param sock The socket.
 * @param nonblocking 1 for non-blocking, 0 for blocking.
 * @return 0 on success. */
c_rest_error_t c_rest_socket_set_nonblocking(c_rest_socket_t sock,
                                             int nonblocking);
/** @brief Close a socket.
 * @param sock The socket.
 * @return 0 on success. */
c_rest_error_t c_rest_socket_close(c_rest_socket_t sock);
/**
 * @brief Receive data from a socket.
 * @param sock The socket.
 * @param buf Buffer.
 * @param len Max length.
 * @param out_read Amount read.
 * @return 0 on success.
 */
c_rest_error_t c_rest_socket_recv(c_rest_socket_t sock, void *buf, size_t len,
                                  size_t *out_read);

/**
 * @brief Send data to a socket.
 * @param sock The socket.
 * @param buf Data buffer.
 * @param len Length to send.
 * @param out_written Amount written.
 * @return 0 on success.
 */
c_rest_error_t c_rest_socket_send(c_rest_socket_t sock, const void *buf,
                                  size_t len, size_t *out_written);

/*
 * Threading
 */
/** @brief Thread function signature.
 * @param arg User argument. */
typedef c_rest_error_t (*c_rest_thread_fn)(void *arg);
/** @brief Create a thread.
 * @param out_thread Pointer to store thread.
 * @param func Thread function.
 * @param arg User argument.
 * @return 0 on success. */
c_rest_error_t c_rest_thread_create(c_rest_thread_t *out_thread,
                                    c_rest_thread_fn func, void *arg);
/** @brief Join a thread.
 * @param thread Thread handle.
 * @return 0 on success. */
c_rest_error_t c_rest_thread_join(c_rest_thread_t thread);

/** @brief Create a mutex.
 * @param out_mutex Pointer to store mutex.
 * @return 0 on success. */
c_rest_error_t c_rest_mutex_create(c_rest_mutex_t *out_mutex);
/** @brief Lock a mutex.
 * @param mutex Mutex handle.
 * @return 0 on success. */
c_rest_error_t c_rest_mutex_lock(c_rest_mutex_t mutex);
/** @brief Unlock a mutex.
 * @param mutex Mutex handle.
 * @return 0 on success. */
c_rest_error_t c_rest_mutex_unlock(c_rest_mutex_t mutex);
/** @brief Destroy a mutex.
 * @param mutex Mutex handle.
 * @return 0 on success. */
c_rest_error_t c_rest_mutex_destroy(c_rest_mutex_t mutex);

/** @brief Create a condition variable.
 * @param out_cond Pointer to store condition variable.
 * @return 0 on success. */
c_rest_error_t c_rest_cond_create(c_rest_cond_t *out_cond);
/** @brief Wait on a condition variable.
 * @param cond Condition variable.
 * @param mutex Mutex handle.
 * @return 0 on success. */
c_rest_error_t c_rest_cond_wait(c_rest_cond_t cond, c_rest_mutex_t mutex);
/** @brief Signal a condition variable.
 * @param cond Condition variable.
 * @return 0 on success. */
c_rest_error_t c_rest_cond_signal(c_rest_cond_t cond);
/** @brief Destroy a condition variable.
 * @param cond Condition variable.
 * @return 0 on success. */
c_rest_error_t c_rest_cond_destroy(c_rest_cond_t cond);

/*
 * Processes
 */
/** @brief Create a process.
 * @param out_proc Pointer to store process handle.
 * @param executable Path to executable.
 * @param argv Arguments array.
 * @return 0 on success. */
c_rest_error_t c_rest_process_create(c_rest_process_t *out_proc,
                                     const char *executable,
                                     char *const argv[]);
/** @brief Wait for a process.
 * @param proc Process handle.
 * @param out_exit_code Pointer to store exit code.
 * @return 0 on success. */
c_rest_error_t c_rest_process_wait(c_rest_process_t proc, int *out_exit_code);

/*
 * Time & Random
 */
/** @brief Get current time in milliseconds.
 * @param out_ms Pointer to store result.
 * @return 0 on success. */
c_rest_error_t c_rest_timer_get_ms(unsigned long *out_ms);
/** @brief Get random bytes.
 * @param buffer Output buffer.
 * @param size Buffer size.
 * @return 0 on success. */
c_rest_error_t c_rest_random_get(void *buffer, size_t size);

/*
 * Error mapping
 */
/** @brief Get last platform error code.
 * @param out_error Pointer to store error code.
 * @return 0 on success. */
c_rest_error_t c_rest_get_last_error(int *out_error);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* C_REST_PLATFORM_H */
