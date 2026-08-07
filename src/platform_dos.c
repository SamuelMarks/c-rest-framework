/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "c_rest_platform.h"

#include <stdlib.h>
#include <string.h>
#include "c_rest_log.h"
#include <stdio.h>
#include <errno.h>
#include <time.h>

#ifdef C_REST_WATT32
#include <tcp.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#endif
#ifdef __DJGPP__
#include <sys/time.h>
#endif
/* clang-format on */

c_rest_error_t c_rest_platform_init(void) {
#ifdef C_REST_WATT32
  sock_init();
#endif
  srand((unsigned int)(time(NULL) ^ clock()));
  return C_REST_OK;
}

c_rest_error_t c_rest_platform_cleanup(void) { return C_REST_OK; }

c_rest_error_t c_rest_socket_create(c_rest_socket_t *out_sock) {
  if (!out_sock)
    return C_REST_ERROR_INVALID_PARAM;
#ifdef C_REST_WATT32
  *out_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (*out_sock < 0) {
    return C_REST_ERROR_SOCKET_CREATE;
  }
  return C_REST_OK;
#else
  *out_sock = C_REST_INVALID_SOCKET;
  return C_REST_ERROR_NOT_SUPPORTED;
#endif
}

c_rest_error_t c_rest_socket_bind(c_rest_socket_t sock, const char *host,
                                  unsigned short port) {
#ifdef C_REST_WATT32
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  if (!host || host[0] == '\0') {
    addr.sin_addr.s_addr = INADDR_ANY;
  } else {
    addr.sin_addr.s_addr = inet_addr(host);
  }
  if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    return C_REST_ERROR_SOCKET_BIND;
  }
  return C_REST_OK;
#else
  (void)sock;
  (void)host;
  (void)port;
  return C_REST_ERROR_NOT_SUPPORTED;
#endif
}

c_rest_error_t c_rest_socket_listen(c_rest_socket_t sock, int backlog) {
#ifdef C_REST_WATT32
  if (listen(sock, backlog) < 0) {
    return C_REST_ERROR_SOCKET_LISTEN;
  }
  return C_REST_OK;
#else
  (void)sock;
  (void)backlog;
  return C_REST_ERROR_NOT_SUPPORTED;
#endif
}

c_rest_error_t c_rest_socket_accept(c_rest_socket_t server_sock,
                                    c_rest_socket_t *out_client_sock) {
#ifdef C_REST_WATT32
  struct sockaddr_in client_addr;
  int addr_len = sizeof(client_addr);
  c_rest_socket_t client_sock;

  if (!out_client_sock)
    return C_REST_ERROR_INVALID_PARAM;

  client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_len);
  if (client_sock < 0) {
    return C_REST_ERROR_SOCKET_ACCEPT;
  }
  *out_client_sock = client_sock;
  return C_REST_OK;
#else
  (void)server_sock;
  if (out_client_sock)
    *out_client_sock = C_REST_INVALID_SOCKET;
  return C_REST_ERROR_NOT_SUPPORTED;
#endif
}

c_rest_error_t c_rest_socket_set_nonblocking(c_rest_socket_t sock,
                                             int nonblocking) {
#ifdef C_REST_WATT32
  int flags = fcntl(sock, F_GETFL, 0);
  if (flags < 0) {
    return C_REST_ERROR_GENERIC;
  }
  if (nonblocking) {
    flags |= O_NONBLOCK;
  } else {
    flags &= ~O_NONBLOCK;
  }
  if (fcntl(sock, F_SETFL, flags) < 0) {
    return C_REST_ERROR_GENERIC;
  }
  return C_REST_OK;
#else
  (void)sock;
  (void)nonblocking;
  return C_REST_ERROR_NOT_SUPPORTED;
#endif
}

c_rest_error_t c_rest_socket_close(c_rest_socket_t sock) {
#ifdef C_REST_WATT32
  if (sock_close(sock) < 0) {
    return C_REST_ERROR_GENERIC;
  }
  return C_REST_OK;
#else
  (void)sock;
  return C_REST_OK;
#endif
}

c_rest_error_t c_rest_thread_create(c_rest_thread_t *out_thread,
                                    c_rest_thread_fn func, void *arg) {
  (void)out_thread;
  (void)func;
  (void)arg;
  return C_REST_ERROR_NOT_SUPPORTED; /* DOS has no native threads */
}

c_rest_error_t c_rest_thread_join(c_rest_thread_t thread) {
  (void)thread;
  return C_REST_ERROR_NOT_SUPPORTED;
}

c_rest_error_t c_rest_mutex_create(c_rest_mutex_t *out_mutex) {
  if (out_mutex) {
    *out_mutex = (c_rest_mutex_t)1; /* Dummy handle */
  }
  return C_REST_OK;
}

c_rest_error_t c_rest_mutex_lock(c_rest_mutex_t mutex) {
  (void)mutex;
  return C_REST_OK;
}

c_rest_error_t c_rest_mutex_unlock(c_rest_mutex_t mutex) {
  (void)mutex;
  return C_REST_OK;
}

c_rest_error_t c_rest_mutex_destroy(c_rest_mutex_t mutex) {
  (void)mutex;
  return C_REST_OK;
}

c_rest_error_t c_rest_cond_create(c_rest_cond_t *out_cond) {
  (void)out_cond;
  return C_REST_ERROR_NOT_SUPPORTED;
}

c_rest_error_t c_rest_cond_wait(c_rest_cond_t c, c_rest_mutex_t m) {
  (void)c;
  (void)m;
  return C_REST_ERROR_NOT_SUPPORTED;
}

c_rest_error_t c_rest_cond_signal(c_rest_cond_t c) {
  (void)c;
  return C_REST_ERROR_NOT_SUPPORTED;
}

c_rest_error_t c_rest_cond_destroy(c_rest_cond_t c) {
  (void)c;
  return C_REST_ERROR_NOT_SUPPORTED;
}

c_rest_error_t c_rest_process_create(c_rest_process_t *out_proc,
                                     const char *executable,
                                     char *const argv[]) {
  (void)out_proc;
  (void)executable;
  (void)argv;
  return C_REST_ERROR_NOT_SUPPORTED;
}

c_rest_error_t c_rest_process_wait(c_rest_process_t proc, int *out_exit_code) {
  (void)proc;
  (void)out_exit_code;
  return C_REST_ERROR_NOT_SUPPORTED;
}

c_rest_error_t c_rest_timer_get_ms(unsigned long *out_ms) {
  if (!out_ms)
    return C_REST_ERROR_INVALID_PARAM;
#ifdef __DJGPP__
  {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    *out_ms = (unsigned long)((tv.tv_sec * 1000) + (tv.tv_usec / 1000));
  }
#else
  *out_ms = (unsigned long)((clock() * 1000) / CLOCKS_PER_SEC);
#endif
  return C_REST_OK;
}

c_rest_error_t c_rest_random_get(void *buffer, size_t size) {
  if (!buffer || size == 0)
    return C_REST_ERROR_INVALID_PARAM;
  {
    size_t i;
    unsigned char *buf = (unsigned char *)buffer;
    for (i = 0; i < size; ++i) {
      buf[i] = (unsigned char)(rand() % 256);
    }
  }
  return C_REST_OK;
}

c_rest_error_t c_rest_get_last_error(int *out_error) {
  if (!out_error)
    return C_REST_ERROR_INVALID_PARAM;
  *out_error = errno;
  return C_REST_OK;
}

c_rest_error_t c_rest_socket_recv(c_rest_socket_t sock, void *buf, size_t len,
                                  size_t *out_read) {
  if (!out_read)
    return C_REST_ERROR_INVALID_PARAM;
#ifdef C_REST_WATT32
  {
    int res = recv(sock, buf, len, 0);
    if (res < 0) {
      return C_REST_ERROR_SOCKET_READ;
    }
    *out_read = (size_t)res;
    return C_REST_OK;
  }
#else
  (void)sock;
  (void)buf;
  (void)len;
  *out_read = 0;
  return C_REST_ERROR_NOT_SUPPORTED;
#endif
}

c_rest_error_t c_rest_socket_send(c_rest_socket_t sock, const void *buf,
                                  size_t len, size_t *out_written) {
  if (!out_written)
    return C_REST_ERROR_INVALID_PARAM;
#ifdef C_REST_WATT32
  {
    int res = send(sock, buf, len, 0);
    if (res < 0) {
      return C_REST_ERROR_SOCKET_WRITE;
    }
    *out_written = (size_t)res;
    return C_REST_OK;
  }
#else
  (void)sock;
  (void)buf;
  (void)len;
  *out_written = 0;
  return C_REST_ERROR_NOT_SUPPORTED;
#endif
}
