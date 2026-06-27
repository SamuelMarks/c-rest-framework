#ifndef C_MULTIPLATFORM_H
#define C_MULTIPLATFORM_H
/* clang-format off */
#include "c_rest_error.h"

#include <stddef.h>
#include "c_rest_error.h"
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

typedef void *cm_env_t;
typedef void *cm_socket_t;
typedef void *cm_file_t;
typedef void *cm_thread_t;
typedef void *cm_mutex_t;
typedef void *cm_cond_t;

typedef void *(*cm_malloc_fn)(size_t size);
typedef void (*cm_free_fn)(void *ptr);
typedef void (*cm_log_fn)(const char *msg);

/** @brief Custom allocator */
struct cm_allocator {
  /** @brief Malloc callback */
  cm_malloc_fn malloc_cb;
  /** @brief Free callback */
  cm_free_fn free_cb;
};

/** @brief Custom logger */
struct cm_logger {
  /** @brief Log callback */
  cm_log_fn log_cb;
};

c_rest_error_t cm_env_create(cm_env_t *out_env);
c_rest_error_t cm_env_destroy(cm_env_t env);

c_rest_error_t cm_env_set_allocator(cm_env_t env,
                                    const struct cm_allocator *alloc);
c_rest_error_t cm_env_set_logger(cm_env_t env, const struct cm_logger *logger);

c_rest_error_t cm_socket_create(cm_env_t env, cm_socket_t *out_sock);
c_rest_error_t cm_socket_bind(cm_env_t env, cm_socket_t sock, const char *host,
                              unsigned short port);
c_rest_error_t cm_socket_listen(cm_env_t env, cm_socket_t sock, int backlog);
c_rest_error_t cm_socket_accept(cm_env_t env, cm_socket_t server,
                                cm_socket_t *out_client);
c_rest_error_t cm_socket_close(cm_env_t env, cm_socket_t sock);
c_rest_error_t cm_socket_set_nonblocking(cm_env_t env, cm_socket_t sock,
                                         int nonblocking);

c_rest_error_t cm_thread_create(cm_env_t env, cm_thread_t *out_thread,
                                void (*func)(void *), void *arg);
c_rest_error_t cm_thread_join(cm_env_t env, cm_thread_t thread);

c_rest_error_t cm_timer_get_ms(cm_env_t env, unsigned long *out_ms);

c_rest_error_t cm_thread_sleep_ms(cm_env_t env, unsigned long ms);
c_rest_error_t cm_file_get_mtime(cm_env_t env, const char *path,
                                 unsigned long *out_mtime);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
