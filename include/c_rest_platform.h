#ifndef C_REST_PLATFORM_H
#define C_REST_PLATFORM_H

/* clang-format off */
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
#include "c_multiplatform.h"
/* clang-format on */

typedef cm_socket_t c_rest_socket_t;
typedef cm_thread_t c_rest_thread_t;
typedef cm_mutex_t c_rest_mutex_t;
typedef cm_cond_t c_rest_cond_t;
typedef void *c_rest_process_t; /* c-multiplatform doesn't have processes */
typedef cm_file_t c_rest_file_t;

#define C_REST_INVALID_SOCKET ((c_rest_socket_t)0)
#define C_REST_INVALID_FILE ((c_rest_file_t)0)

#else

/*
 * Platform-agnostic handles.
 * We use ptrdiff_t internally so that it can safely hold an int (POSIX fd)
 * or a void pointer handle on Windows.
 */
typedef ptrdiff_t c_rest_socket_t;
typedef ptrdiff_t c_rest_thread_t;
typedef ptrdiff_t c_rest_mutex_t;
typedef ptrdiff_t c_rest_cond_t;
typedef ptrdiff_t c_rest_process_t;
typedef ptrdiff_t c_rest_file_t;

#define C_REST_INVALID_SOCKET ((c_rest_socket_t) - 1)
#define C_REST_INVALID_FILE ((c_rest_file_t) - 1)

#endif

/*
 * Initialization and cleanup.
 */
int c_rest_platform_init(void);
int c_rest_platform_cleanup(void);

/*
 * Sockets
 */
int c_rest_socket_create(c_rest_socket_t *out_sock);
int c_rest_socket_bind(c_rest_socket_t sock, const char *host,
                       unsigned short port);
int c_rest_socket_listen(c_rest_socket_t sock, int backlog);
int c_rest_socket_accept(c_rest_socket_t server_sock,
                         c_rest_socket_t *out_client_sock);
int c_rest_socket_set_nonblocking(c_rest_socket_t sock, int nonblocking);
int c_rest_socket_close(c_rest_socket_t sock);

/*
 * File I/O
 */
int c_rest_file_open(const char *path, int write_mode, c_rest_file_t *out_file);
int c_rest_file_read(c_rest_file_t file, void *buffer, size_t size,
                     size_t *out_read);
int c_rest_file_write(c_rest_file_t file, const void *buffer, size_t size,
                      size_t *out_written);
int c_rest_file_close(c_rest_file_t file);

/*
 * Threading
 */
typedef void (*c_rest_thread_fn)(void *arg);
int c_rest_thread_create(c_rest_thread_t *out_thread, c_rest_thread_fn func,
                         void *arg);
int c_rest_thread_join(c_rest_thread_t thread);

int c_rest_mutex_create(c_rest_mutex_t *out_mutex);
int c_rest_mutex_lock(c_rest_mutex_t mutex);
int c_rest_mutex_unlock(c_rest_mutex_t mutex);
int c_rest_mutex_destroy(c_rest_mutex_t mutex);

int c_rest_cond_create(c_rest_cond_t *out_cond);
int c_rest_cond_wait(c_rest_cond_t cond, c_rest_mutex_t mutex);
int c_rest_cond_signal(c_rest_cond_t cond);
int c_rest_cond_destroy(c_rest_cond_t cond);

/*
 * Processes
 */
int c_rest_process_create(c_rest_process_t *out_proc, const char *executable,
                          char *const argv[]);
int c_rest_process_wait(c_rest_process_t proc, int *out_exit_code);

/*
 * Time & Random
 */
int c_rest_timer_get_ms(unsigned long *out_ms);
int c_rest_random_get(void *buffer, size_t size);

/*
 * Error mapping
 */
int c_rest_get_last_error(int *out_error);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* C_REST_PLATFORM_H */
