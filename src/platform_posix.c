#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

/* clang-format off */
#include "c_rest_platform.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#if defined(__unix__) || defined(__APPLE__)
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <time.h>
/* clang-format on */
#endif

int c_rest_platform_init(void) { return 0; }

int c_rest_platform_cleanup(void) { return 0; }

int c_rest_socket_create(c_rest_socket_t *out_sock) {
#if defined(__unix__) || defined(__APPLE__)
  int sock;
  if (!out_sock)
    return 1;

  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    return 1;
  }

  *out_sock = (c_rest_socket_t)sock;
  return 0;
#else
  return 1;
#endif
}

int c_rest_socket_bind(c_rest_socket_t sock, const char *host,
                       unsigned short port) {
#if defined(__unix__) || defined(__APPLE__)
  struct sockaddr_in addr;
  int s = (int)sock;
  int res;

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = inet_addr(host);

  res = bind(s, (struct sockaddr *)&addr, sizeof(addr));
  if (res < 0) {
    return 1;
  }

  return 0;
#else
  return 1;
#endif
}

int c_rest_socket_listen(c_rest_socket_t sock, int backlog) {
#if defined(__unix__) || defined(__APPLE__)
  int s = (int)sock;
  int res = listen(s, backlog);
  if (res < 0) {
    return 1;
  }
  return 0;
#else
  return 1;
#endif
}

int c_rest_socket_accept(c_rest_socket_t server_sock,
                         c_rest_socket_t *out_client_sock) {
#if defined(__unix__) || defined(__APPLE__)
  int s = (int)server_sock;
  int client;
  struct sockaddr_in client_addr;
  socklen_t addr_len = sizeof(client_addr);

  if (!out_client_sock)
    return 1;

  client = accept(s, (struct sockaddr *)&client_addr, &addr_len);
  if (client < 0) {
    return 1;
  }

  *out_client_sock = (c_rest_socket_t)client;
  return 0;
#else
  return 1;
#endif
}

int c_rest_socket_set_nonblocking(c_rest_socket_t sock, int nonblocking) {
#if defined(__unix__) || defined(__APPLE__)
  int s = (int)sock;
  int flags = fcntl(s, F_GETFL, 0);
  if (flags < 0)
    return 1;

  if (nonblocking) {
    flags |= O_NONBLOCK;
  } else {
    flags &= ~O_NONBLOCK;
  }

  if (fcntl(s, F_SETFL, flags) < 0) {
    return 1;
  }
  return 0;
#else
  return 1;
#endif
}

int c_rest_socket_close(c_rest_socket_t sock) {
#if defined(__unix__) || defined(__APPLE__)
  int s = (int)sock;
  int res = close(s);
  if (res < 0) {
    return 1;
  }
  return 0;
#else
  return 1;
#endif
}

struct thread_wrapper_args {
  c_rest_thread_fn func;
  void *arg;
};

static void *thread_wrapper(void *arg) {
  struct thread_wrapper_args *args = (struct thread_wrapper_args *)arg;
  args->func(args->arg);
  free(args);
  return NULL;
}

int c_rest_thread_create(c_rest_thread_t *out_thread, c_rest_thread_fn func,
                         void *arg) {
#if defined(__unix__) || defined(__APPLE__)
  pthread_t thread;
  struct thread_wrapper_args *args;

  if (!out_thread || !func)
    return 1;

  args =
      (struct thread_wrapper_args *)malloc(sizeof(struct thread_wrapper_args));
  if (!args)
    return 1;

  args->func = func;
  args->arg = arg;

  if (pthread_create(&thread, NULL, thread_wrapper, args) != 0) {
    free(args);
    return 1;
  }

  /* In C89, pthread_t is an opaque type, often an int or a pointer.
   * We cast it via ptrdiff_t. This is technically unportable if pthread_t >
   * size_t, but standard on POSIX platforms. */
  *out_thread = (c_rest_thread_t)thread;
  return 0;
#else
  return 1;
#endif
}

int c_rest_thread_join(c_rest_thread_t thread) {
#if defined(__unix__) || defined(__APPLE__)
  pthread_t t = (pthread_t)thread;
  if (pthread_join(t, NULL) != 0) {
    return 1;
  }
  return 0;
#else
  return 1;
#endif
}

int c_rest_mutex_create(c_rest_mutex_t *out_mutex) {
#if defined(__unix__) || defined(__APPLE__)
  pthread_mutex_t *m;

  if (!out_mutex)
    return 1;

  m = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
  if (!m)
    return 1;

  if (pthread_mutex_init(m, NULL) != 0) {
    free(m);
    return 1;
  }

  *out_mutex = (c_rest_mutex_t)m;
  return 0;
#else
  return 1;
#endif
}

int c_rest_mutex_lock(c_rest_mutex_t mutex) {
#if defined(__unix__) || defined(__APPLE__)
  pthread_mutex_t *m = (pthread_mutex_t *)mutex;
  if (!m)
    return 1;

  if (pthread_mutex_lock(m) != 0) {
    return 1;
  }
  return 0;
#else
  return 1;
#endif
}

int c_rest_mutex_unlock(c_rest_mutex_t mutex) {
#if defined(__unix__) || defined(__APPLE__)
  pthread_mutex_t *m = (pthread_mutex_t *)mutex;
  if (!m)
    return 1;

  if (pthread_mutex_unlock(m) != 0) {
    return 1;
  }
  return 0;
#else
  return 1;
#endif
}

int c_rest_mutex_destroy(c_rest_mutex_t mutex) {
#if defined(__unix__) || defined(__APPLE__)
  pthread_mutex_t *m = (pthread_mutex_t *)mutex;
  if (!m)
    return 1;

  pthread_mutex_destroy(m);
  free(m);
  return 0;
#else
  return 1;
#endif
}

int c_rest_cond_create(c_rest_cond_t *out_cond) {
#if defined(__unix__) || defined(__APPLE__)
  pthread_cond_t *cond;

  if (!out_cond)
    return 1;

  cond = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
  if (!cond)
    return 1;

  if (pthread_cond_init(cond, NULL) != 0) {
    free(cond);
    return 1;
  }

  *out_cond = (c_rest_cond_t)cond;
  return 0;
#else
  return 1;
#endif
}

int c_rest_cond_wait(c_rest_cond_t c, c_rest_mutex_t m) {
#if defined(__unix__) || defined(__APPLE__)
  pthread_cond_t *cond = (pthread_cond_t *)c;
  pthread_mutex_t *mutex = (pthread_mutex_t *)m;

  if (!cond || !mutex)
    return 1;

  if (pthread_cond_wait(cond, mutex) != 0) {
    return 1;
  }
  return 0;
#else
  return 1;
#endif
}

int c_rest_cond_signal(c_rest_cond_t c) {
#if defined(__unix__) || defined(__APPLE__)
  pthread_cond_t *cond = (pthread_cond_t *)c;

  if (!cond)
    return 1;

  if (pthread_cond_signal(cond) != 0) {
    return 1;
  }
  return 0;
#else
  return 1;
#endif
}

int c_rest_cond_destroy(c_rest_cond_t c) {
#if defined(__unix__) || defined(__APPLE__)
  pthread_cond_t *cond = (pthread_cond_t *)c;

  if (!cond)
    return 1;

  pthread_cond_destroy(cond);
  free(cond);
  return 0;
#else
  return 1;
#endif
}

int c_rest_process_create(c_rest_process_t *out_proc, const char *executable,
                          char *const argv[]) {
#if defined(__unix__) || defined(__APPLE__)
  pid_t pid;

  if (!out_proc || !executable)
    return 1;

  pid = fork();
  if (pid < 0) {
    return 1;
  } else if (pid == 0) {
    /* Child */
    execvp(executable, argv);
    exit(127); /* Should not reach */
  }

  *out_proc = (c_rest_process_t)pid;
  return 0;
#else
  return 1;
#endif
}

int c_rest_process_wait(c_rest_process_t proc, int *out_exit_code) {
#if defined(__unix__) || defined(__APPLE__)
  pid_t pid = (pid_t)proc;
  int status;

  if (waitpid(pid, &status, 0) < 0) {
    return 1;
  }

  if (out_exit_code) {
    if (WIFEXITED(status)) {
      *out_exit_code = WEXITSTATUS(status);
    } else {
      *out_exit_code = 1; /* Terminated by signal or otherwise */
    }
  }

  return 0;
#else
  return 1;
#endif
}

int c_rest_timer_get_ms(unsigned long *out_ms) {
#if defined(__unix__) || defined(__APPLE__)
  struct timespec ts;
  if (!out_ms)
    return 1;

  /* Strict C89 lacks clock_gettime, but POSIX has it */
  if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
    *out_ms = (unsigned long)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
    return 0;
  }
  return 1;
#else
  return 1;
#endif
}

int c_rest_random_get(void *buffer, size_t size) {
  if (!buffer || size == 0)
    return 1;
  /* Strict C89 fallback: */
  {
    size_t i;
    char *buf = (char *)buffer;
    for (i = 0; i < size; ++i) {
      buf[i] = (char)(rand() % 256);
    }
  }
  return 0;
}

int c_rest_get_last_error(int *out_error) {
  if (!out_error)
    return 1;
  *out_error = errno;
  return 0;
}

int c_rest_socket_recv(c_rest_socket_t sock, void *buf, size_t len,
                       size_t *out_read) {
  ssize_t ret;
  if (!buf || !out_read)
    return 1;
  *out_read = 0;
  ret = recv((int)sock, buf, len, 0);
  if (ret > 0) {
    *out_read = (size_t)ret;
    return 0;
  }
  return 1;
}

int c_rest_socket_send(c_rest_socket_t sock, const void *buf, size_t len,
                       size_t *out_written) {
  ssize_t ret;
  if (!buf || !out_written)
    return 1;
  *out_written = 0;
  ret = send((int)sock, buf, len, 0);
  if (ret > 0) {
    *out_written = (size_t)ret;
    return 0;
  }
  return 1;
}
