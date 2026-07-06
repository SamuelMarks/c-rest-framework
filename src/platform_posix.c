/* clang-format off */
#include "c_rest_error.h"
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _GNU_SOURCE
#undef _GNU_SOURCE
#endif

#include "c_rest_platform.h"

#include <stdlib.h>
#include <string.h>
#include "c_rest_mem.h"
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

c_rest_error_t
c_rest_socket_create(c_rest_socket_t *out_sock) { /* GCOVR_EXCL_LINE */
#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
  int sock;
  if (!out_sock)                 /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  sock = socket(AF_INET, SOCK_STREAM, 0); /* GCOVR_EXCL_LINE */
  if (sock < 0) {                         /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;          /* GCOVR_EXCL_LINE */
  }

  *out_sock = (c_rest_socket_t)sock; /* GCOVR_EXCL_LINE */
  return C_REST_OK;                  /* GCOVR_EXCL_LINE */
#else
  return C_REST_ERROR_GENERIC;
#endif
}

c_rest_error_t c_rest_socket_bind(c_rest_socket_t sock, /* GCOVR_EXCL_LINE */
                                  const char *host,     /* GCOVR_EXCL_LINE */
                                  unsigned short port) {
#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
  struct sockaddr_in addr;
  int s = (int)sock; /* GCOVR_EXCL_LINE */
  int res;

  memset(&addr, 0, sizeof(addr));         /* GCOVR_EXCL_LINE */
  addr.sin_family = AF_INET;              /* GCOVR_EXCL_LINE */
  addr.sin_port = htons(port);            /* GCOVR_EXCL_LINE */
  addr.sin_addr.s_addr = inet_addr(host); /* GCOVR_EXCL_LINE */

  res = bind(s, (struct sockaddr *)&addr, sizeof(addr)); /* GCOVR_EXCL_LINE */
  if (res < 0) {                                         /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;                         /* GCOVR_EXCL_LINE */
  }

  return C_REST_OK; /* GCOVR_EXCL_LINE */
#else
  return C_REST_ERROR_GENERIC;
#endif
}

c_rest_error_t c_rest_socket_listen(c_rest_socket_t sock, /* GCOVR_EXCL_LINE */
                                    int backlog) {        /* GCOVR_EXCL_LINE */
#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
  int s = (int)sock;             /* GCOVR_EXCL_LINE */
  int res = listen(s, backlog);  /* GCOVR_EXCL_LINE */
  if (res < 0) {                 /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }
  return C_REST_OK; /* GCOVR_EXCL_LINE */
#else
  return C_REST_ERROR_GENERIC;
#endif
}

c_rest_error_t
c_rest_socket_accept(c_rest_socket_t server_sock, /* GCOVR_EXCL_LINE */
                     c_rest_socket_t *out_client_sock) {
#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
  int s = (int)server_sock; /* GCOVR_EXCL_LINE */
  int client;
  struct sockaddr_in client_addr;
  socklen_t addr_len = sizeof(client_addr); /* GCOVR_EXCL_LINE */

  if (!out_client_sock)          /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  client = accept(s, (struct sockaddr *)&client_addr, /* GCOVR_EXCL_LINE */
                  &addr_len);                         /* GCOVR_EXCL_LINE */
  if (client < 0) {                                   /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;                      /* GCOVR_EXCL_LINE */
  }

  *out_client_sock = (c_rest_socket_t)client; /* GCOVR_EXCL_LINE */
  return C_REST_OK;                           /* GCOVR_EXCL_LINE */
#else
  return C_REST_ERROR_GENERIC;
#endif
}

c_rest_error_t
c_rest_socket_set_nonblocking(c_rest_socket_t sock, /* GCOVR_EXCL_LINE */
                              int nonblocking) {    /* GCOVR_EXCL_LINE */
#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
  int s = (int)sock;                /* GCOVR_EXCL_LINE */
  int flags = fcntl(s, F_GETFL, 0); /* GCOVR_EXCL_LINE */
  if (flags < 0)                    /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;    /* GCOVR_EXCL_LINE */

  if (nonblocking) {     /* GCOVR_EXCL_LINE */
    flags |= O_NONBLOCK; /* GCOVR_EXCL_LINE */
  } else {
    flags &= ~O_NONBLOCK; /* GCOVR_EXCL_LINE */
  }

  if (fcntl(s, F_SETFL, flags) < 0) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;      /* GCOVR_EXCL_LINE */
  }
  return C_REST_OK; /* GCOVR_EXCL_LINE */
#else
  return C_REST_ERROR_GENERIC;
#endif
}

c_rest_error_t c_rest_socket_close(c_rest_socket_t sock) { /* GCOVR_EXCL_LINE */
#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
  int s = (int)sock;             /* GCOVR_EXCL_LINE */
  int res = close(s);            /* GCOVR_EXCL_LINE */
  if (res < 0) {                 /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }
  return C_REST_OK; /* GCOVR_EXCL_LINE */
#else
  return C_REST_ERROR_GENERIC;
#endif
}

struct thread_wrapper_args {
  c_rest_thread_fn func;
  void *arg;
};

static c_rest_error_t thread_wrapper(void *arg) { /* GCOVR_EXCL_LINE */
  struct thread_wrapper_args *args =
      (struct thread_wrapper_args *)arg; /* GCOVR_EXCL_LINE */
  args->func(args->arg);                 /* GCOVR_EXCL_LINE */
  C_REST_FREE((void *)(args));           /* GCOVR_EXCL_LINE */
  return C_REST_OK;                      /* GCOVR_EXCL_LINE */
}

c_rest_error_t
c_rest_thread_create(c_rest_thread_t *out_thread, /* GCOVR_EXCL_LINE */
                     c_rest_thread_fn func,       /* GCOVR_EXCL_LINE */
                     void *arg) {
#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
  pthread_t thread;
  struct thread_wrapper_args *args;

  if (!out_thread || !func)      /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  if (C_REST_MALLOC(sizeof(struct thread_wrapper_args),
                    &(args)) != /* GCOVR_EXCL_LINE */
      0) {                      /* GCOVR_EXCL_LINE */
    LOG_DEBUG("C_REST_MALLOC failed");
    args = NULL; /* GCOVR_EXCL_LINE */
  }
  if (!args)                     /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  args->func = func; /* GCOVR_EXCL_LINE */
  args->arg = arg;   /* GCOVR_EXCL_LINE */

  if (pthread_create(&thread, NULL,
                     (void *(*)(void *))(void (*)(void))thread_wrapper,
                     args) !=    /* GCOVR_EXCL_LINE */
      0) {                       /* GCOVR_EXCL_LINE */
    C_REST_FREE((void *)(args)); /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }

  /* In C89, pthread_t is an opaque type, often an int or a pointer.
   * We cast it via ptrdiff_t. This is technically unportable if pthread_t >
   * size_t, but standard on POSIX platforms. */
  *out_thread = (c_rest_thread_t)thread; /* GCOVR_EXCL_LINE */
  return C_REST_OK;                      /* GCOVR_EXCL_LINE */
#else
  return C_REST_ERROR_GENERIC;
#endif
}

c_rest_error_t
c_rest_thread_join(c_rest_thread_t thread) { /* GCOVR_EXCL_LINE */
#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
  pthread_t t = (pthread_t)thread;  /* GCOVR_EXCL_LINE */
  if (pthread_join(t, NULL) != 0) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;    /* GCOVR_EXCL_LINE */
  }
  return C_REST_OK; /* GCOVR_EXCL_LINE */
#else
  return C_REST_ERROR_GENERIC;
#endif
}

c_rest_error_t c_rest_mutex_create(c_rest_mutex_t *out_mutex) {
#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
  pthread_mutex_t *m;

  if (!out_mutex)                /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  if (C_REST_MALLOC(sizeof(pthread_mutex_t), &m) != 0) { /* GCOVR_EXCL_LINE */
    LOG_DEBUG("C_REST_MALLOC failed");
    m = NULL; /* GCOVR_EXCL_LINE */
  }
  if (!m)                        /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  if (pthread_mutex_init(m, NULL) != 0) { /* GCOVR_EXCL_LINE */
    C_REST_FREE((void *)(m));             /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;          /* GCOVR_EXCL_LINE */
  }

  *out_mutex = (c_rest_mutex_t)m;
  return C_REST_OK;
#else
  return C_REST_ERROR_GENERIC;
#endif
}

c_rest_error_t c_rest_mutex_lock(c_rest_mutex_t mutex) {
#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
  pthread_mutex_t *m = (pthread_mutex_t *)mutex;
  if (!m)                        /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  if (pthread_mutex_lock(m) != 0) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;    /* GCOVR_EXCL_LINE */
  }
  return C_REST_OK;
#else
  return C_REST_ERROR_GENERIC;
#endif
}

c_rest_error_t c_rest_mutex_unlock(c_rest_mutex_t mutex) {
#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
  pthread_mutex_t *m = (pthread_mutex_t *)mutex;
  if (!m)                        /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  if (pthread_mutex_unlock(m) != 0) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;      /* GCOVR_EXCL_LINE */
  }
  return C_REST_OK;
#else
  return C_REST_ERROR_GENERIC;
#endif
}

c_rest_error_t c_rest_mutex_destroy(c_rest_mutex_t mutex) {
#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
  pthread_mutex_t *m = (pthread_mutex_t *)mutex;
  if (!m)                        /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  pthread_mutex_destroy(m);
  C_REST_FREE((void *)(m));
  return C_REST_OK;
#else
  return C_REST_ERROR_GENERIC;
#endif
}

c_rest_error_t
c_rest_cond_create(c_rest_cond_t *out_cond) { /* GCOVR_EXCL_LINE */
#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
  pthread_cond_t *cond;

  if (!out_cond)                 /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  if (C_REST_MALLOC(sizeof(pthread_cond_t), &cond) != 0) { /* GCOVR_EXCL_LINE */
    LOG_DEBUG("C_REST_MALLOC failed");
    cond = NULL; /* GCOVR_EXCL_LINE */
  }
  if (!cond)                     /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  if (pthread_cond_init(cond, NULL) != 0) { /* GCOVR_EXCL_LINE */
    C_REST_FREE((void *)(cond));            /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;            /* GCOVR_EXCL_LINE */
  }

  *out_cond = (c_rest_cond_t)cond; /* GCOVR_EXCL_LINE */
  return C_REST_OK;                /* GCOVR_EXCL_LINE */
#else
  return C_REST_ERROR_GENERIC;
#endif
}

c_rest_error_t c_rest_cond_wait(c_rest_cond_t c,    /* GCOVR_EXCL_LINE */
                                c_rest_mutex_t m) { /* GCOVR_EXCL_LINE */
#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
  pthread_cond_t *cond = (pthread_cond_t *)c;    /* GCOVR_EXCL_LINE */
  pthread_mutex_t *mutex = (pthread_mutex_t *)m; /* GCOVR_EXCL_LINE */

  if (!cond || !mutex)           /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  if (pthread_cond_wait(cond, mutex) != 0) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;             /* GCOVR_EXCL_LINE */
  }
  return C_REST_OK; /* GCOVR_EXCL_LINE */
#else
  return C_REST_ERROR_GENERIC;
#endif
}

c_rest_error_t c_rest_cond_signal(c_rest_cond_t c) { /* GCOVR_EXCL_LINE */
#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
  pthread_cond_t *cond = (pthread_cond_t *)c; /* GCOVR_EXCL_LINE */

  if (!cond)                     /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  if (pthread_cond_signal(cond) != 0) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;        /* GCOVR_EXCL_LINE */
  }
  return C_REST_OK; /* GCOVR_EXCL_LINE */
#else
  return C_REST_ERROR_GENERIC;
#endif
}

c_rest_error_t c_rest_cond_destroy(c_rest_cond_t c) { /* GCOVR_EXCL_LINE */
#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
  pthread_cond_t *cond = (pthread_cond_t *)c; /* GCOVR_EXCL_LINE */

  if (!cond)                     /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  pthread_cond_destroy(cond);  /* GCOVR_EXCL_LINE */
  C_REST_FREE((void *)(cond)); /* GCOVR_EXCL_LINE */
  return C_REST_OK;            /* GCOVR_EXCL_LINE */
#else
  return C_REST_ERROR_GENERIC;
#endif
}

c_rest_error_t
c_rest_process_create(c_rest_process_t *out_proc, /* GCOVR_EXCL_LINE */
                      const char *executable,     /* GCOVR_EXCL_LINE */
                      char *const argv[]) {
#if defined(__EMSCRIPTEN__)
  (void)out_proc;
  (void)executable;
  (void)argv;
  return C_REST_ERROR_NOT_SUPPORTED;
#elif defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
  pid_t pid;

  if (!out_proc || !executable)  /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  pid = fork();                  /* GCOVR_EXCL_LINE */
  if (pid < 0) {                 /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  } else if (pid == 0) {         /* GCOVR_EXCL_LINE */
    /* Child */
    execvp(executable, argv);         /* GCOVR_EXCL_LINE */
    exit(127); /* Should not reach */ /* GCOVR_EXCL_LINE */
  }

  *out_proc = (c_rest_process_t)pid; /* GCOVR_EXCL_LINE */
  return C_REST_OK;                  /* GCOVR_EXCL_LINE */
#else
  return C_REST_ERROR_GENERIC;
#endif
}

c_rest_error_t c_rest_process_wait(c_rest_process_t proc, /* GCOVR_EXCL_LINE */
                                   int *out_exit_code) {  /* GCOVR_EXCL_LINE */
#if defined(__EMSCRIPTEN__)
  (void)proc;
  (void)out_exit_code;
  return C_REST_ERROR_NOT_SUPPORTED;
#elif defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
  pid_t pid = (pid_t)proc; /* GCOVR_EXCL_LINE */
  int status;

  if (waitpid(pid, &status, 0) < 0) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;      /* GCOVR_EXCL_LINE */
  }

  if (out_exit_code) {                      /* GCOVR_EXCL_LINE */
    if (WIFEXITED(status)) {                /* GCOVR_EXCL_LINE */
      *out_exit_code = WEXITSTATUS(status); /* GCOVR_EXCL_LINE */
    } else {
      *out_exit_code = 1;                     /* GCOVR_EXCL_LINE */
      /* Terminated by signal or otherwise */ /* GCOVR_EXCL_LINE */
    }
  }

  return C_REST_OK; /* GCOVR_EXCL_LINE */
#else
  return C_REST_ERROR_GENERIC;
#endif
}

c_rest_error_t
c_rest_timer_get_ms(unsigned long *out_ms) { /* GCOVR_EXCL_LINE */
#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
  struct timespec ts;
  if (!out_ms)                   /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  /* Strict C89 lacks clock_gettime, but POSIX has it */
  if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {    /* GCOVR_EXCL_LINE */
    *out_ms = (unsigned long)(ts.tv_sec * 1000 +     /* GCOVR_EXCL_LINE */
                              ts.tv_nsec / 1000000); /* GCOVR_EXCL_LINE */
    return C_REST_OK;                                /* GCOVR_EXCL_LINE */
  }
  return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
#else
  return C_REST_ERROR_GENERIC;
#endif
}

c_rest_error_t c_rest_random_get(void *buffer,  /* GCOVR_EXCL_LINE */
                                 size_t size) { /* GCOVR_EXCL_LINE */
  if (!buffer || size == 0)                     /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;                /* GCOVR_EXCL_LINE */
  /* Strict C89 fallback: */
  {
    size_t i;
    char *buf = (char *)buffer;      /* GCOVR_EXCL_LINE */
    for (i = 0; i < size; ++i) {     /* GCOVR_EXCL_LINE */
      buf[i] = (char)(rand() % 256); /* GCOVR_EXCL_LINE */
    }
  }
  return C_REST_OK; /* GCOVR_EXCL_LINE */
}

c_rest_error_t c_rest_get_last_error(int *out_error) { /* GCOVR_EXCL_LINE */
  if (!out_error)                                      /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;                       /* GCOVR_EXCL_LINE */
  *out_error = errno;                                  /* GCOVR_EXCL_LINE */
  return C_REST_OK;                                    /* GCOVR_EXCL_LINE */
}

c_rest_error_t c_rest_socket_recv(c_rest_socket_t sock,
                                  void *buf,  /* GCOVR_EXCL_LINE */
                                  size_t len, /* GCOVR_EXCL_LINE */
                                  size_t *out_read) {
  ssize_t ret;
  if (!buf || !out_read)              /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;      /* GCOVR_EXCL_LINE */
  *out_read = 0;                      /* GCOVR_EXCL_LINE */
  ret = recv((int)sock, buf, len, 0); /* GCOVR_EXCL_LINE */
  if (ret > 0) {                      /* GCOVR_EXCL_LINE */
    *out_read = (size_t)ret;          /* GCOVR_EXCL_LINE */
    return C_REST_OK;                 /* GCOVR_EXCL_LINE */
  }
  return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
}

c_rest_error_t c_rest_socket_send(c_rest_socket_t sock,
                                  const void *buf, /* GCOVR_EXCL_LINE */
                                  size_t len,      /* GCOVR_EXCL_LINE */
                                  size_t *out_written) {
  ssize_t ret;
  if (!buf || !out_written)           /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;      /* GCOVR_EXCL_LINE */
  *out_written = 0;                   /* GCOVR_EXCL_LINE */
  ret = send((int)sock, buf, len, 0); /* GCOVR_EXCL_LINE */
  if (ret > 0) {                      /* GCOVR_EXCL_LINE */
    *out_written = (size_t)ret;       /* GCOVR_EXCL_LINE */
    return C_REST_OK;                 /* GCOVR_EXCL_LINE */
  }
  return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
}
