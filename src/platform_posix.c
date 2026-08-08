/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "c_rest_endian.h"
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _GNU_SOURCE
#undef _GNU_SOURCE
#endif

#include "c_rest_platform.h"

#include <stdlib.h>
#include <string.h>
#include "c_rest_log.h"
#include <stdio.h>
#include <errno.h>

#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <time.h>
#endif
/* clang-format on */

c_rest_error_t c_rest_platform_init(void) { return C_REST_OK; }

c_rest_error_t c_rest_platform_cleanup(void) { return C_REST_OK; }

c_rest_error_t c_rest_socket_create(c_rest_socket_t *out_sock) {
#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
  int sock;
  if (!out_sock)
    return C_REST_ERROR_GENERIC;

  sock = socket(AF_INET, SOCK_STREAM, 0);

  *out_sock = (c_rest_socket_t)sock;
  return C_REST_OK;
#else
  return C_REST_ERROR_GENERIC;
#endif
}

c_rest_error_t c_rest_socket_bind(c_rest_socket_t sock, const char *host,
                                  unsigned short port) {
#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
  struct sockaddr_in addr;
  int s = (int)sock;
  int res;

  int opt = 1;

  setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  c_rest_htons(port, &addr.sin_port);
  addr.sin_addr.s_addr = inet_addr(host);

  res = bind(s, (struct sockaddr *)&addr, sizeof(addr));
  if (res < 0)
    return C_REST_ERROR_GENERIC;
  return C_REST_OK;
#else
  return C_REST_ERROR_GENERIC;
#endif
}

c_rest_error_t c_rest_socket_listen(c_rest_socket_t sock, int backlog) {
#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
  int s = (int)sock;
  int res = listen(s, backlog);
  if (res < 0)
    return C_REST_ERROR_GENERIC;
  return C_REST_OK;
#else
  return C_REST_ERROR_GENERIC;
#endif
}

c_rest_error_t c_rest_socket_accept(c_rest_socket_t server_sock,
                                    c_rest_socket_t *out_client_sock) {
#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
  int s = (int)server_sock;
  int client;
  struct sockaddr_in client_addr;
  socklen_t addr_len = sizeof(client_addr);

  if (!out_client_sock)
    return C_REST_ERROR_GENERIC;

  client = accept(s, (struct sockaddr *)&client_addr, &addr_len);
  if (client < 0)
    return C_REST_ERROR_GENERIC;

  *out_client_sock = (c_rest_socket_t)client;
  return C_REST_OK;
#else
  return C_REST_ERROR_GENERIC;
#endif
}

c_rest_error_t c_rest_socket_set_nonblocking(c_rest_socket_t sock,
                                             int nonblocking) {
#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
  int s = (int)sock;
  int flags = fcntl(s, F_GETFL, 0);
  if (flags < 0)
    return C_REST_ERROR_GENERIC;

  if (nonblocking) {
    flags |= O_NONBLOCK;
  } else {
    flags &= ~O_NONBLOCK;
  }

  fcntl(s, F_SETFL, flags);
  return C_REST_OK;
#else
  return C_REST_ERROR_GENERIC;
#endif
}

c_rest_error_t c_rest_socket_close(c_rest_socket_t sock) {
#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
  int s = (int)sock;
  int res = close(s);
  if (res != 0) {
    return C_REST_ERROR_NETWORK;
  }
  return C_REST_OK;
#else
  return C_REST_ERROR_GENERIC;
#endif
}

struct thread_wrapper_args {
  c_rest_thread_fn func;
  void *arg;
};

static c_rest_error_t thread_wrapper(void *arg) {
  c_rest_error_t rc;
  struct thread_wrapper_args *args = (struct thread_wrapper_args *)arg;
  rc = args->func(args->arg);
  C_REST_FREE((void *)(args));
  return rc;
}

c_rest_error_t c_rest_thread_create(c_rest_thread_t *out_thread,
                                    c_rest_thread_fn func, void *arg) {
#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
  pthread_t thread;
  struct thread_wrapper_args *args;

  if (!out_thread || !func)
    return C_REST_ERROR_GENERIC;

  if (C_REST_MALLOC(sizeof(struct thread_wrapper_args), (void **)&(args)) !=
      0) {
    LOG_DEBUG("C_REST_MALLOC failed");
    args = NULL;
  }
  if (!args)
    return C_REST_ERROR_GENERIC;

  args->func = func;
  args->arg = arg;

  pthread_create(&thread, NULL,
                 (void *(*)(void *))(void (*)(void))thread_wrapper, args);

  /* In C89, pthread_t is an opaque type, often an int or a pointer.
   * We cast it via ptrdiff_t. This is technically unportable if pthread_t >
   * size_t, but standard on POSIX platforms. */
  *out_thread = (c_rest_thread_t)thread;
  return C_REST_OK;
#else
  return C_REST_ERROR_GENERIC;
#endif
}

c_rest_error_t c_rest_thread_join(c_rest_thread_t thread) {
#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
  pthread_t t = (pthread_t)thread;
  if (pthread_join(t, NULL) != 0) {
    return C_REST_ERROR_GENERIC;
  }
  return C_REST_OK;

#else
  return C_REST_ERROR_GENERIC;
#endif
}

c_rest_error_t c_rest_mutex_create(c_rest_mutex_t *out_mutex) {
#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
  pthread_mutex_t *m;

  if (!out_mutex)
    return C_REST_ERROR_GENERIC;

  if (C_REST_MALLOC(sizeof(pthread_mutex_t), (void **)&m) != 0) {
    LOG_DEBUG("C_REST_MALLOC failed");
    m = NULL;
  }
  if (!m)
    return C_REST_ERROR_GENERIC;

  pthread_mutex_init(m, NULL);

  *out_mutex = (c_rest_mutex_t)m;
  return C_REST_OK;
#else
  return C_REST_ERROR_GENERIC;
#endif
}

c_rest_error_t c_rest_mutex_lock(c_rest_mutex_t mutex) {
#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
  pthread_mutex_t *m = (pthread_mutex_t *)mutex;
  if (!m)
    return C_REST_ERROR_GENERIC;

  pthread_mutex_lock(m);
  return C_REST_OK;
#else
  return C_REST_ERROR_GENERIC;
#endif
}

c_rest_error_t c_rest_mutex_unlock(c_rest_mutex_t mutex) {
#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
  pthread_mutex_t *m = (pthread_mutex_t *)mutex;
  if (!m)
    return C_REST_ERROR_GENERIC;

  pthread_mutex_unlock(m);
  return C_REST_OK;
#else
  return C_REST_ERROR_GENERIC;
#endif
}

c_rest_error_t c_rest_mutex_destroy(c_rest_mutex_t mutex) {
#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
  pthread_mutex_t *m = (pthread_mutex_t *)mutex;
  if (!m)
    return C_REST_ERROR_GENERIC;

  pthread_mutex_destroy(m);
  C_REST_FREE((void *)(m));
  return C_REST_OK;
#else
  return C_REST_ERROR_GENERIC;
#endif
}

c_rest_error_t c_rest_cond_create(c_rest_cond_t *out_cond) {
#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
  pthread_cond_t *cond;

  if (!out_cond)
    return C_REST_ERROR_GENERIC;

  if (C_REST_MALLOC(sizeof(pthread_cond_t), (void **)&cond) != 0) {
    LOG_DEBUG("C_REST_MALLOC failed");
    cond = NULL;
  }
  if (!cond)
    return C_REST_ERROR_GENERIC;

  pthread_cond_init(cond, NULL);

  *out_cond = (c_rest_cond_t)cond;
  return C_REST_OK;
#else
  return C_REST_ERROR_GENERIC;
#endif
}

c_rest_error_t c_rest_cond_wait(c_rest_cond_t c, c_rest_mutex_t m) {
#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
  pthread_cond_t *cond = (pthread_cond_t *)c;
  pthread_mutex_t *mutex = (pthread_mutex_t *)m;

  if (!cond || !mutex)
    return C_REST_ERROR_GENERIC;

  pthread_cond_wait(cond, mutex);
  return C_REST_OK;
#else
  return C_REST_ERROR_GENERIC;
#endif
}

c_rest_error_t c_rest_cond_signal(c_rest_cond_t c) {
#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
  pthread_cond_t *cond = (pthread_cond_t *)c;

  if (!cond)
    return C_REST_ERROR_GENERIC;

  pthread_cond_signal(cond);
  return C_REST_OK;
#else
  return C_REST_ERROR_GENERIC;
#endif
}

c_rest_error_t c_rest_cond_destroy(c_rest_cond_t c) {
#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
  pthread_cond_t *cond = (pthread_cond_t *)c;

  if (!cond)
    return C_REST_ERROR_GENERIC;

  pthread_cond_destroy(cond);
  C_REST_FREE((void *)(cond));
  return C_REST_OK;
#else
  return C_REST_ERROR_GENERIC;
#endif
}

c_rest_error_t c_rest_process_create(c_rest_process_t *out_proc,
                                     const char *executable,
                                     char *const argv[]) {
#if defined(__EMSCRIPTEN__)
  (void)out_proc;
  (void)executable;
  (void)argv;
  return C_REST_ERROR_NOT_SUPPORTED;
#elif defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
  pid_t pid;

  if (!out_proc || !executable)
    return C_REST_ERROR_GENERIC;

  pid = fork();
  if (pid == 0) {
    /* Child */
    execvp(executable, argv);
    exit(127); /* Should not reach */
  }

  *out_proc = (c_rest_process_t)pid;
  return C_REST_OK;
#else
  return C_REST_ERROR_GENERIC;
#endif
}

c_rest_error_t c_rest_process_wait(c_rest_process_t proc, int *out_exit_code) {
#if defined(__EMSCRIPTEN__)
  (void)proc;
  (void)out_exit_code;
  return C_REST_ERROR_NOT_SUPPORTED;
#elif defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
  pid_t pid = (pid_t)proc;
  int status;

  waitpid(pid, &status, 0);

  if (out_exit_code) {
    if (WIFEXITED(status)) {
      *out_exit_code = WEXITSTATUS(status);
    } else {
      *out_exit_code = 1;
      /* Terminated by signal or otherwise */
    }
  }

  return C_REST_OK;
#else
  return C_REST_ERROR_GENERIC;
#endif
}

c_rest_error_t c_rest_timer_get_ms(unsigned long *out_ms) {
#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
  struct timespec ts;
  if (!out_ms)
    return C_REST_ERROR_GENERIC;

  /* Strict C89 lacks clock_gettime, but POSIX has it */
  clock_gettime(CLOCK_MONOTONIC, &ts);
  *out_ms = (unsigned long)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
  return C_REST_OK;
#else
  return C_REST_ERROR_GENERIC;
#endif
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
  ssize_t ret;
  if (!buf || !out_read)
    return C_REST_ERROR_GENERIC;
  *out_read = 0;
  ret = recv((int)sock, buf, len, 0);
  *out_read = (size_t)ret;
  if (ret <= 0)
    return C_REST_ERROR_GENERIC;
  return C_REST_OK;
}

c_rest_error_t c_rest_socket_send(c_rest_socket_t sock, const void *buf,
                                  size_t len, size_t *out_written) {
  ssize_t ret;
  if (!buf || !out_written)
    return C_REST_ERROR_GENERIC;
  *out_written = 0;
#ifdef MSG_NOSIGNAL
  ret = send((int)sock, buf, len, MSG_NOSIGNAL);
#elif defined(SO_NOSIGPIPE)
  {
    int val = 1;
    setsockopt((int)sock, SOL_SOCKET, SO_NOSIGPIPE, &val, sizeof(val));
    ret = send((int)sock, buf, len, 0);
  }
#else
  ret = send((int)sock, buf, len, 0);
#endif
  *out_written = (size_t)ret;
  if (ret <= 0)
    return C_REST_ERROR_GENERIC;
  return C_REST_OK;
}
