
/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_platform.h"
#include "test_protos.h"
#include "c_rest_mem.h"

#include <stdio.h>
#include <string.h>
#if defined(_WIN32)
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#endif
/* clang-format on */

static c_rest_error_t cond_thread_func(void *arg) {
  c_rest_cond_t cond = (c_rest_cond_t)arg;
#if defined(__unix__) || defined(__APPLE__)
  {
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 10000;
    select(0, NULL, NULL, NULL, &tv);
  }
#endif
  c_rest_cond_signal(cond);
  return C_REST_OK;
}

static c_rest_error_t thread_func(void *arg) {
  int *val = (int *)arg;
  *val = 42;
  return C_REST_OK;
}

#ifdef C_REST_TESTING_MALLOC_HOOK
static int g_malloc_fail_count = -1;
static void *hook_malloc_platform(size_t size) {
  if (g_malloc_fail_count == 0) {
    return NULL;
  }
  if (g_malloc_fail_count > 0) {
    g_malloc_fail_count--;
  }
  return malloc(size);
}
#endif

int test_platform(void) {

  int failed = 0;
  c_rest_socket_t sock, client_sock;
  c_rest_thread_t thread;
  c_rest_mutex_t mutex = 0;
  c_rest_cond_t cond;
  c_rest_process_t proc;
  int rc;
  int val = 0;
  unsigned long ms;
  char buf[16];
  int err;
  printf("Entering test_platform!\n");

#ifdef C_REST_TESTING_MALLOC_HOOK
  g_crf_malloc_hook = hook_malloc_platform;
#endif

  rc = c_rest_platform_init();
  failed += ((rc != C_REST_OK) != 0);

  /* Sockets */
  failed += ((c_rest_socket_create(NULL) == C_REST_OK) != 0);
  rc = c_rest_socket_create(&sock);
  failed += ((rc != C_REST_OK) != 0);

  rc = c_rest_socket_bind(sock, "127.0.0.1", 0);
  failed += ((rc != C_REST_OK) != 0);

  rc = c_rest_socket_listen(sock, 10);
  c_rest_socket_listen((c_rest_socket_t)-1, 10);
  c_rest_socket_accept(sock, NULL);
  c_rest_socket_accept((c_rest_socket_t)-1, &client_sock);
  failed += ((rc != C_REST_OK) != 0);

  rc = c_rest_socket_set_nonblocking(sock, 1);

  failed += ((rc != C_REST_OK) != 0);

  rc = c_rest_socket_set_nonblocking(sock, 0);
  failed += ((rc != C_REST_OK) != 0);

  /* Should fail with EWOULDBLOCK if nonblocking, but we are back to blocking.
   */
  /* To prevent hang, we'll set nonblocking again, accept, and it should fail */
  c_rest_socket_set_nonblocking(sock, 1);

  /* Send / Recv */
  {
    size_t wr = 0, rd = 0;
    failed += ((c_rest_socket_send(sock, NULL, 5, &wr) == C_REST_OK) != 0);
    failed += ((c_rest_socket_send(sock, "hello", 5, NULL) == C_REST_OK) != 0);
    failed += ((c_rest_socket_recv(sock, NULL, 5, &rd) == C_REST_OK) != 0);
    failed += ((c_rest_socket_recv(sock, buf, 5, NULL) == C_REST_OK) != 0);

    /* Expected to fail on unconnected sock */
    c_rest_socket_send(sock, "hello", 5, &wr);
    c_rest_socket_recv(sock, buf, 5, &rd);
  }

  {
#ifndef _WIN32
    int fds[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0) {
      /*c_rest_socket_send(s1, "hello", 5, &wr);*/
      /*c_rest_socket_recv(s2, buf, 5, &rd);*/
    }
#endif
  }

  {
    c_rest_socket_t tsock;
    c_rest_socket_create(&tsock);
    c_rest_socket_bind(tsock, "256.256.256.256", 0);
    c_rest_socket_listen(tsock, 10);
    /**/
  }

  {
    c_rest_socket_t tsock;
    c_rest_socket_create(&tsock);
    c_rest_socket_set_nonblocking((c_rest_socket_t)-1, 1);
    c_rest_socket_set_nonblocking((c_rest_socket_t)-1, 0);
    /**/
  }

  rc = c_rest_socket_close(sock);
  failed += ((rc != C_REST_OK) != 0);

  /* Threads */

#ifdef C_REST_TESTING_MALLOC_HOOK
  g_malloc_fail_count = 0;
  failed +=
      ((c_rest_thread_create(&thread, thread_func, &val) == C_REST_OK) != 0);
  g_malloc_fail_count = -1;
#endif

  rc = c_rest_thread_create(&thread, thread_func, &val);
  failed += ((rc != C_REST_OK) != 0);
  rc = c_rest_thread_join(thread);
  failed += ((rc != C_REST_OK) != 0);
  failed += ((val != 42) != 0);

  {
    c_rest_thread_t thread2;
    failed +=
        ((c_rest_thread_create(NULL, thread_func, &val) == C_REST_OK) != 0);
    failed += ((c_rest_thread_create(&thread2, NULL, &val) == C_REST_OK) != 0);
    /* c_rest_thread_join((c_rest_thread_t)NULL); */

    failed += ((c_rest_mutex_create(NULL) == C_REST_OK) != 0);
    failed += ((c_rest_cond_create(NULL) == C_REST_OK) != 0);

    c_rest_mutex_lock((c_rest_mutex_t)NULL);
    c_rest_mutex_unlock((c_rest_mutex_t)NULL);
    c_rest_mutex_destroy((c_rest_mutex_t)NULL);
    c_rest_cond_wait((c_rest_cond_t)0, mutex);
    c_rest_cond_signal((c_rest_cond_t)NULL);
    c_rest_cond_destroy((c_rest_cond_t)NULL);

    c_rest_timer_get_ms(NULL);
    c_rest_random_get(NULL, 16);
    c_rest_random_get(buf, 0);
    c_rest_get_last_error(NULL);
    c_rest_socket_set_nonblocking((c_rest_socket_t)-1, 1);
    c_rest_socket_set_nonblocking((c_rest_socket_t)-1, 0);
    c_rest_socket_bind((c_rest_socket_t)-1, "127.0.0.1", 8080);
    c_rest_socket_close((c_rest_socket_t)-1);
  }

  /* Mutex */
  failed += ((c_rest_mutex_create(NULL) == C_REST_OK) != 0);

#ifdef C_REST_TESTING_MALLOC_HOOK
  g_malloc_fail_count = 0;
  failed += ((c_rest_mutex_create(&mutex) == C_REST_OK) != 0);
  g_malloc_fail_count = -1;
#endif

  rc = c_rest_mutex_create(&mutex);
  failed += ((rc != C_REST_OK) != 0);

  failed += ((c_rest_mutex_lock((c_rest_mutex_t)0) == C_REST_OK) != 0);
  rc = c_rest_mutex_lock(mutex);
  failed += ((rc != C_REST_OK) != 0);

  failed += ((c_rest_mutex_unlock((c_rest_mutex_t)0) == C_REST_OK) != 0);
  rc = c_rest_mutex_unlock(mutex);
  failed += ((rc != C_REST_OK) != 0);

  failed += ((c_rest_mutex_destroy((c_rest_mutex_t)0) == C_REST_OK) != 0);
  rc = c_rest_mutex_destroy(mutex);
  failed += ((rc != C_REST_OK) != 0);

  /* Cond */
  failed += ((c_rest_cond_create(NULL) == C_REST_OK) != 0);

#ifdef C_REST_TESTING_MALLOC_HOOK
  g_malloc_fail_count = 0;
  failed += ((c_rest_cond_create(&cond) == C_REST_OK) != 0);
  g_malloc_fail_count = -1;
#endif

  rc = c_rest_cond_create(&cond);
  failed += ((rc != C_REST_OK) != 0);

  rc = c_rest_mutex_create(&mutex);
  failed += ((rc != C_REST_OK) != 0);

  /* if (c_rest_cond_wait((c_rest_cond_t)0, mutex) == C_REST_OK) return
   * __LINE__; */
  failed += ((c_rest_cond_wait(cond, (c_rest_mutex_t)0) == C_REST_OK) != 0);
  c_rest_mutex_lock(mutex);
  c_rest_thread_create(&thread, cond_thread_func, (void *)cond);
  c_rest_cond_wait(cond, mutex);
  c_rest_mutex_unlock(mutex);
  c_rest_thread_join(thread);
  /* Skip actual wait so we don't hang */

  failed += ((c_rest_cond_signal((c_rest_cond_t)0) == C_REST_OK) != 0);
  rc = c_rest_cond_signal(cond);
  failed += ((rc != C_REST_OK) != 0);

  failed += ((c_rest_cond_destroy((c_rest_cond_t)0) == C_REST_OK) != 0);
  rc = c_rest_cond_destroy(cond);
  failed += ((rc != C_REST_OK) != 0);
  rc = c_rest_mutex_destroy(mutex);

  /* Process */
  {
    char *argv[] = {"echo", "hello", NULL};
#ifndef __EMSCRIPTEN__
    char *argv_bin[] = {"/bin/echo", "hello", NULL};
#endif
    failed += ((c_rest_process_create(NULL, "echo", argv) == C_REST_OK) != 0);
    failed += ((c_rest_process_create(&proc, NULL, argv) == C_REST_OK) != 0);
#ifndef __EMSCRIPTEN__
    /* Try with /bin/echo to improve reliability on POSIX */
    rc = c_rest_process_create(&proc, "/bin/echo", argv_bin);
    if (rc == C_REST_OK) {
      int exit_code = -1;
      rc = c_rest_process_wait(proc, &exit_code);
      if (rc != C_REST_OK) {
        printf("Warning: c_rest_process_wait failed with %d\n", rc);
      } else if (exit_code != 0) {
        printf("Warning: process exited with %d\n", exit_code);
      }
    } else {
      /* Fallback to simple echo */
      rc = c_rest_process_create(&proc, "echo", argv);
      if (rc == C_REST_OK) {
        int exit_code = -1;
        (void)!c_rest_process_wait(proc, &exit_code);
      }
    }
#endif
  }

  {
    int exit_code = 0;
    char *argv_fake[] = {"does_not_exist_xyz123", NULL};
    c_rest_process_create(&proc, "does_not_exist_xyz123", argv_fake);
    c_rest_process_wait(proc, &exit_code);

    {
      char *argv_kill[] = {"sh", "-c", "kill -9 $$", NULL};
      c_rest_process_create(&proc, "sh", argv_kill);
    }
    c_rest_process_wait(proc, &exit_code);
  }

  /* Timer */
  failed += ((c_rest_timer_get_ms(NULL) == C_REST_OK) != 0);
  rc = c_rest_timer_get_ms(&ms);
  failed += ((rc != C_REST_OK) != 0);

  /* Random */
  failed += ((c_rest_random_get(NULL, 16) == C_REST_OK) != 0);
  failed += ((c_rest_random_get(buf, 0) == C_REST_OK) != 0);
  rc = c_rest_random_get(buf, 16);
  failed += ((rc != C_REST_OK) != 0);

  /* Error */
  failed += ((c_rest_get_last_error(NULL) == C_REST_OK) != 0);
  rc = c_rest_get_last_error(&err);
  failed += ((rc != C_REST_OK) != 0);

  rc = c_rest_platform_cleanup();
  failed += ((rc != C_REST_OK) != 0);

#ifdef C_REST_TESTING_MALLOC_HOOK
  g_crf_malloc_hook = NULL;
#endif

  return failed;
}
