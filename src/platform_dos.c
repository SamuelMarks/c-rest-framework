/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_platform.h"

#include <stdlib.h>
#include <string.h>
#include "c_rest_mem.h"
#include "c_rest_log.h"
#include <stdio.h>
#include <errno.h>
#include <time.h>
/* clang-format on */

c_rest_error_t c_rest_platform_init(void) { return C_REST_OK; }

c_rest_error_t c_rest_platform_cleanup(void) { return C_REST_OK; }

c_rest_error_t c_rest_socket_create(c_rest_socket_t *out_sock) {
  (void)out_sock;
  return C_REST_ERROR_GENERIC; /* Not implemented for DOS yet */
}

c_rest_error_t c_rest_socket_bind(c_rest_socket_t sock, const char *host,
                                  unsigned short port) {
  (void)sock;
  (void)host;
  (void)port;
  return C_REST_ERROR_GENERIC;
}

c_rest_error_t c_rest_socket_listen(c_rest_socket_t sock, int backlog) {
  (void)sock;
  (void)backlog;
  return C_REST_ERROR_GENERIC;
}

c_rest_error_t c_rest_socket_accept(c_rest_socket_t server_sock,
                                    c_rest_socket_t *out_client_sock) {
  (void)server_sock;
  (void)out_client_sock;
  return C_REST_ERROR_GENERIC;
}

c_rest_error_t c_rest_socket_set_nonblocking(c_rest_socket_t sock,
                                             int nonblocking) {
  (void)sock;
  (void)nonblocking;
  return C_REST_ERROR_GENERIC;
}
c_rest_error_t c_rest_socket_close(c_rest_socket_t sock) {
  /* Simple socket close mock */
  (void)sock;
  return C_REST_OK;
}
c_rest_error_t c_rest_thread_create(c_rest_thread_t *out_thread,
                                    c_rest_thread_fn func, void *arg) {
  (void)out_thread;
  (void)func;
  (void)arg;
  return C_REST_ERROR_GENERIC; /* DOS has no native threads */
}

c_rest_error_t c_rest_thread_join(c_rest_thread_t thread) {
  (void)thread;
  return C_REST_ERROR_GENERIC;
}

c_rest_error_t c_rest_mutex_create(c_rest_mutex_t *out_mutex) {
  (void)out_mutex;
  return C_REST_ERROR_GENERIC;
}

c_rest_error_t c_rest_mutex_lock(c_rest_mutex_t mutex) {
  (void)mutex;
  return C_REST_ERROR_GENERIC;
}

c_rest_error_t c_rest_mutex_unlock(c_rest_mutex_t mutex) {
  (void)mutex;
  return C_REST_ERROR_GENERIC;
}

c_rest_error_t c_rest_mutex_destroy(c_rest_mutex_t mutex) {
  (void)mutex;
  return C_REST_ERROR_GENERIC;
}

c_rest_error_t c_rest_cond_create(c_rest_cond_t *out_cond) {
  (void)out_cond;
  return C_REST_ERROR_GENERIC;
}

c_rest_error_t c_rest_cond_wait(c_rest_cond_t c, c_rest_mutex_t m) {
  (void)c;
  (void)m;
  return C_REST_ERROR_GENERIC;
}

c_rest_error_t c_rest_cond_signal(c_rest_cond_t c) {
  (void)c;
  return C_REST_ERROR_GENERIC;
}

c_rest_error_t c_rest_cond_destroy(c_rest_cond_t c) {
  (void)c;
  return C_REST_ERROR_GENERIC;
}

c_rest_error_t c_rest_process_create(c_rest_process_t *out_proc,
                                     const char *executable,
                                     char *const argv[]) {
  (void)out_proc;
  (void)executable;
  (void)argv;
  return C_REST_ERROR_GENERIC; /* system() could be used but it's blocking */
}

c_rest_error_t c_rest_process_wait(c_rest_process_t proc, int *out_exit_code) {
  (void)proc;
  (void)out_exit_code;
  return C_REST_ERROR_GENERIC;
}

c_rest_error_t c_rest_timer_get_ms(unsigned long *out_ms) {
  if (!out_ms)
    return C_REST_ERROR_GENERIC;
  /* Very inaccurate fallback for standard C89 */
  /* CLOCKS_PER_SEC varies */
  *out_ms = (unsigned long)((clock() * 1000) / CLOCKS_PER_SEC);
  return C_REST_OK;
}

c_rest_error_t c_rest_random_get(void *buffer, size_t size) {
  if (!buffer || size == 0)
    return C_REST_ERROR_GENERIC;
  /* Strict C89 fallback: */
  {
    size_t i;
    char *buf = (char *)buffer;
    for (i = 0; i < size; ++i) {
      buf[i] = (char)(rand() % 256);
    }
  }
  return C_REST_OK;
}

c_rest_error_t c_rest_get_last_error(int *out_error) {
  if (!out_error)
    return C_REST_ERROR_GENERIC;
  *out_error = errno;
  return C_REST_OK;
}

c_rest_error_t c_rest_socket_recv(c_rest_socket_t sock, void *buf, size_t len,
                                  size_t *out_read) {
  (void)sock;
  (void)buf;
  (void)len;
  (void)out_read;
  return C_REST_ERROR_GENERIC;
}

c_rest_error_t c_rest_socket_send(c_rest_socket_t sock, const void *buf,
                                  size_t len, size_t *out_written) {
  (void)sock;
  (void)buf;
  (void)len;
  (void)out_written;
  return C_REST_ERROR_GENERIC;
}
