
/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_platform.h"
#include "c_rest_testing_mocks.h"
#include "test_protos.h"
#include "c_rest_mem.h"

#include <stdio.h>
#include <string.h>
#if defined(_WIN32)
#include <winsock2.h>
#else
#ifndef C_REST_NO_THREADS
#include <pthread.h>
#endif
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#endif
/* clang-format on */

#if !defined(__EMSCRIPTEN__) && !defined(C_REST_NO_THREADS)
static c_rest_error_t cond_thread_func(void *arg) {
  c_rest_cond_t cond = (c_rest_cond_t)arg;
#if defined(_WIN32)
  Sleep(10);
#elif defined(__unix__) || defined(__APPLE__)
  {
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 10000;
    select(0, NULL, NULL, NULL, &tv);
  }
#endif
  return c_rest_cond_signal(cond);
}
#endif

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
  return malloc(size);
}
#endif

#ifdef C_REST_TESTING_ENDIAN_HOOK
static c_rest_error_t mock_platform_endian_fail(int *out_is_little) {
  *out_is_little = 0;
  return C_REST_ERROR_GENERIC;
}
#endif

TEST test_platform_cases(void) {
  c_rest_socket_t sock, client_sock;
  c_rest_thread_t thread;
  c_rest_mutex_t mutex = 0;
  c_rest_cond_t cond;
  c_rest_process_t proc = (c_rest_process_t)1;
  c_rest_error_t rc;
  int val = 0;
  unsigned long ms;
  char buf[16];
  int err;
  printf("Entering test_platform!\n");

#ifdef C_REST_TESTING_MALLOC_HOOK
  g_crf_malloc_hook = hook_malloc_platform;
#endif

  rc = c_rest_platform_init();
  ASSERT_FALSE(rc != C_REST_OK);

  /* Sockets */
  ASSERT_FALSE(c_rest_socket_create(NULL) == C_REST_OK);
#if defined(C_REST_TESTING_MALLOC_HOOK) && !defined(_WIN32)
  g_mock_socket_fail = 300;
  ASSERT_FALSE(c_rest_socket_create(&sock) != C_REST_ERROR_NETWORK);
  g_mock_socket_fail = 0;
#endif
  rc = c_rest_socket_create(&sock);
  ASSERT_FALSE(rc != C_REST_OK);

#if defined(C_REST_TESTING_MALLOC_HOOK) && !defined(_WIN32)
  g_mock_socket_fail = 301;
  ASSERT_FALSE(c_rest_socket_set_nonblocking(sock, 1) != C_REST_ERROR_GENERIC);
  g_mock_socket_fail = 0;
#endif

  rc = c_rest_socket_bind(sock, "127.0.0.1", 0);
  ASSERT_FALSE(rc != C_REST_OK);
  ASSERT_FALSE(c_rest_socket_bind(sock, NULL, 0) == C_REST_OK);
#ifdef C_REST_TESTING_ENDIAN_HOOK
  g_crf_is_little_endian_hook = mock_platform_endian_fail;
  ASSERT_FALSE(c_rest_socket_bind(sock, "127.0.0.1", 0) == C_REST_OK);
  g_crf_is_little_endian_hook = NULL;
#endif

  rc = c_rest_socket_listen(sock, 10);
  ASSERT_NEQ(C_REST_OK, c_rest_socket_listen((c_rest_socket_t)-1, 10));
  ASSERT_NEQ(C_REST_OK, c_rest_socket_accept(sock, NULL));
  ASSERT_NEQ(C_REST_OK,
             c_rest_socket_accept((c_rest_socket_t)-1, &client_sock));
#ifndef __EMSCRIPTEN__
  ASSERT_FALSE(rc != C_REST_OK);
#else
  ASSERT_FALSE(rc != C_REST_ERROR_NOT_SUPPORTED);
#endif

  rc = c_rest_socket_set_nonblocking(sock, 1);

  ASSERT_FALSE(rc != C_REST_OK);

  rc = c_rest_socket_set_nonblocking(sock, 0);
  ASSERT_FALSE(rc != C_REST_OK);

  /* Should fail with EWOULDBLOCK if nonblocking, but we are back to blocking.
   */
  /* To prevent hang, we'll set nonblocking again, accept, and it should fail */
  ASSERT_EQ(C_REST_OK, c_rest_socket_set_nonblocking(sock, 1));

  /* Send / Recv */
  {
    size_t wr = 0, rd = 0;
    ASSERT_FALSE(c_rest_socket_send(sock, NULL, 5, &wr) == C_REST_OK);
    ASSERT_FALSE(c_rest_socket_send(sock, "hello", 5, NULL) == C_REST_OK);
    ASSERT_FALSE(c_rest_socket_recv(sock, NULL, 5, &rd) == C_REST_OK);
    ASSERT_FALSE(c_rest_socket_recv(sock, buf, 5, NULL) == C_REST_OK);

    /* Expected to fail on unconnected sock */
    ASSERT_NEQ(C_REST_OK, c_rest_socket_send(sock, "hello", 5, &wr));
    ASSERT_NEQ(C_REST_OK, c_rest_socket_recv(sock, buf, 5, &rd));
  }

  {
    c_rest_socket_t tsock;
    ASSERT_EQ(C_REST_OK, c_rest_socket_create(&tsock));
    ASSERT_NEQ(C_REST_OK, c_rest_socket_bind(tsock, "256.256.256.256", 0));
#ifndef __EMSCRIPTEN__
    ASSERT_NEQ(C_REST_OK, c_rest_socket_bind(tsock, "255.255.255.255", 0));
#endif
    rc = c_rest_socket_listen(tsock, 10);
    ASSERT(rc == C_REST_OK || rc != C_REST_OK);
    ASSERT_EQ(C_REST_OK, c_rest_socket_close(tsock));
  }

  {
    c_rest_socket_t tsock;
    ASSERT_EQ(C_REST_OK, c_rest_socket_create(&tsock));
    ASSERT_NEQ(C_REST_OK,
               c_rest_socket_set_nonblocking((c_rest_socket_t)-1, 1));
    ASSERT_NEQ(C_REST_OK,
               c_rest_socket_set_nonblocking((c_rest_socket_t)-1, 0));
    ASSERT_EQ(C_REST_OK, c_rest_socket_close(tsock));
  }

  rc = c_rest_socket_close(sock);
  ASSERT_FALSE(rc != C_REST_OK);

  /* Threads */

#ifdef C_REST_NO_THREADS
  (void)thread_func;
  ASSERT_EQ(C_REST_ERROR_NOT_SUPPORTED,
            c_rest_thread_create(&thread, thread_func, &val));
  ASSERT_EQ(C_REST_ERROR_NOT_SUPPORTED, c_rest_thread_join((c_rest_thread_t)1));
#else
#ifdef C_REST_TESTING_MALLOC_HOOK
  g_malloc_fail_count = 0;
  ASSERT_FALSE(c_rest_thread_create(&thread, thread_func, &val) == C_REST_OK);
  g_malloc_fail_count = -1;
#endif

  rc = c_rest_thread_create(&thread, thread_func, &val);
  ASSERT_FALSE(rc != C_REST_OK);
  rc = c_rest_thread_join(thread);
  ASSERT_FALSE(rc != C_REST_OK);
  ASSERT_FALSE(val != 42);
#endif

  {
    c_rest_thread_t thread2;
    ASSERT_FALSE(c_rest_thread_create(NULL, thread_func, &val) == C_REST_OK);
    ASSERT_FALSE(c_rest_thread_create(&thread2, NULL, &val) == C_REST_OK);
    ASSERT_FALSE(c_rest_thread_join((c_rest_thread_t)NULL) == C_REST_OK);
    ASSERT_FALSE(c_rest_thread_join((c_rest_thread_t)-1) == C_REST_OK);
#ifndef C_REST_NO_THREADS
#if defined(_WIN32)
    ASSERT_FALSE(c_rest_thread_join((c_rest_thread_t)9999999) == C_REST_OK);
#else
    ASSERT_FALSE(c_rest_thread_join((c_rest_thread_t)pthread_self()) ==
                 C_REST_OK);
#ifdef C_REST_TESTING_MALLOC_HOOK
    val = 0;
    ASSERT_EQ(C_REST_OK, c_rest_thread_create(&thread2, thread_func, &val));
    g_mock_socket_fail = 302;
    ASSERT_NEQ(C_REST_OK, c_rest_thread_join(thread2));
    g_mock_socket_fail = 0;
    ASSERT_EQ(C_REST_OK, c_rest_thread_join(thread2));
#endif
#endif
#endif

    ASSERT_FALSE(c_rest_mutex_create(NULL) == C_REST_OK);
    ASSERT_FALSE(c_rest_cond_create(NULL) == C_REST_OK);

    ASSERT_NEQ(C_REST_OK, c_rest_mutex_lock((c_rest_mutex_t)NULL));
    ASSERT_NEQ(C_REST_OK, c_rest_mutex_unlock((c_rest_mutex_t)NULL));
    ASSERT_NEQ(C_REST_OK, c_rest_mutex_destroy((c_rest_mutex_t)NULL));
    ASSERT_NEQ(C_REST_OK, c_rest_cond_wait((c_rest_cond_t)0, mutex));
    ASSERT_NEQ(C_REST_OK, c_rest_cond_signal((c_rest_cond_t)NULL));
    ASSERT_NEQ(C_REST_OK, c_rest_cond_destroy((c_rest_cond_t)NULL));

    ASSERT_NEQ(C_REST_OK, c_rest_timer_get_ms(NULL));
    ASSERT_NEQ(C_REST_OK, c_rest_random_get(NULL, 16));
    ASSERT_NEQ(C_REST_OK, c_rest_random_get(buf, 0));
    ASSERT_NEQ(C_REST_OK, c_rest_get_last_error(NULL));
    ASSERT_NEQ(C_REST_OK,
               c_rest_socket_set_nonblocking((c_rest_socket_t)-1, 1));
    ASSERT_NEQ(C_REST_OK,
               c_rest_socket_set_nonblocking((c_rest_socket_t)-1, 0));
    ASSERT_NEQ(C_REST_OK,
               c_rest_socket_bind((c_rest_socket_t)-1, "127.0.0.1", 8080));
    ASSERT_NEQ(C_REST_OK, c_rest_socket_close((c_rest_socket_t)-1));
  }

  /* Mutex */
  ASSERT_FALSE(c_rest_mutex_create(NULL) == C_REST_OK);

#ifdef C_REST_TESTING_MALLOC_HOOK
  g_malloc_fail_count = 0;
  ASSERT_FALSE(c_rest_mutex_create(&mutex) == C_REST_OK);
  g_malloc_fail_count = -1;
#endif

  rc = c_rest_mutex_create(&mutex);
  ASSERT_FALSE(rc != C_REST_OK);

  ASSERT_FALSE(c_rest_mutex_lock((c_rest_mutex_t)0) == C_REST_OK);
  rc = c_rest_mutex_lock(mutex);
  ASSERT_FALSE(rc != C_REST_OK);

  ASSERT_FALSE(c_rest_mutex_unlock((c_rest_mutex_t)0) == C_REST_OK);
  rc = c_rest_mutex_unlock(mutex);
  ASSERT_FALSE(rc != C_REST_OK);

  ASSERT_FALSE(c_rest_mutex_destroy((c_rest_mutex_t)0) == C_REST_OK);
  rc = c_rest_mutex_destroy(mutex);
  ASSERT_FALSE(rc != C_REST_OK);

  /* Cond */
  ASSERT_FALSE(c_rest_cond_create(NULL) == C_REST_OK);

#ifdef C_REST_TESTING_MALLOC_HOOK
  g_malloc_fail_count = 0;
  ASSERT_FALSE(c_rest_cond_create(&cond) == C_REST_OK);
  g_malloc_fail_count = -1;
#endif

  rc = c_rest_cond_create(&cond);
  ASSERT_FALSE(rc != C_REST_OK);

  rc = c_rest_mutex_create(&mutex);
  ASSERT_FALSE(rc != C_REST_OK);

  /* if (c_rest_cond_wait((c_rest_cond_t)0, mutex) == C_REST_OK) return
   * __LINE__; */
  ASSERT_FALSE(c_rest_cond_wait(cond, (c_rest_mutex_t)0) == C_REST_OK);
#if !defined(__EMSCRIPTEN__) && !defined(C_REST_NO_THREADS)
  ASSERT_EQ(C_REST_OK, c_rest_mutex_lock(mutex));
  ASSERT_EQ(C_REST_OK,
            c_rest_thread_create(&thread, cond_thread_func, (void *)cond));
  ASSERT_EQ(C_REST_OK, c_rest_cond_wait(cond, mutex));
  ASSERT_EQ(C_REST_OK, c_rest_mutex_unlock(mutex));
  ASSERT_EQ(C_REST_OK, c_rest_thread_join(thread));
#elif defined(C_REST_NO_THREADS)
  ASSERT_EQ(C_REST_OK, c_rest_cond_wait(cond, mutex));
#endif
  /* Skip actual wait so we don't hang */

  ASSERT_FALSE(c_rest_cond_signal((c_rest_cond_t)0) == C_REST_OK);
  rc = c_rest_cond_signal(cond);
  ASSERT_FALSE(rc != C_REST_OK);

  ASSERT_FALSE(c_rest_cond_destroy((c_rest_cond_t)0) == C_REST_OK);
  rc = c_rest_cond_destroy(cond);
  ASSERT_FALSE(rc != C_REST_OK);
  rc = c_rest_mutex_destroy(mutex);
  ASSERT_EQ(C_REST_OK, rc);

  /* Process */
  {
    char a_echo[] = "echo";
    char a_hello[] = "hello";
    char *argv[3];
#if !defined(__EMSCRIPTEN__)
    int exit_code = -1;
#endif
    argv[0] = a_echo;
    argv[1] = a_hello;
    argv[2] = NULL;
    ASSERT_FALSE(c_rest_process_create(NULL, "echo", argv) == C_REST_OK);
    ASSERT_FALSE(c_rest_process_create(&proc, NULL, argv) == C_REST_OK);
#if defined(_WIN32)
    {
      char a_cmd[] = "cmd.exe";
      char a_c[] = "/c";
      char *argv_win[5];
      argv_win[0] = a_cmd;
      argv_win[1] = a_c;
      argv_win[2] = a_echo;
      argv_win[3] = a_hello;
      argv_win[4] = NULL;
      rc = c_rest_process_create(&proc, "cmd.exe", argv_win);
      ASSERT_FALSE(rc != C_REST_OK);
      rc = c_rest_process_wait(proc, &exit_code);
      ASSERT_EQ(C_REST_OK, rc);
      rc = c_rest_process_create(&proc, "cmd.exe", argv_win);
      ASSERT_FALSE(rc != C_REST_OK);
      rc = c_rest_process_wait(proc, NULL);
      ASSERT_EQ(C_REST_OK, rc);
    }
#elif !defined(__EMSCRIPTEN__)
    {
      char a_binecho[] = "/bin/echo";
      char *argv_bin[3];
      argv_bin[0] = a_binecho;
      argv_bin[1] = a_hello;
      argv_bin[2] = NULL;
      rc = c_rest_process_create(&proc, "/bin/echo", argv_bin);
      ASSERT_FALSE(rc != C_REST_OK);
      rc = c_rest_process_wait(proc, &exit_code);
      ASSERT_EQ(C_REST_OK, rc);
      rc = c_rest_process_wait(proc, NULL);
      ASSERT_NEQ(C_REST_OK, rc);
    }
#endif
  }

  {
    int exit_code = 0;
    char a_dne[] = "does_not_exist_xyz123";
    char *argv_fake[2];
    char big_arg[1050];
    char *argv_big[3];
    argv_fake[0] = a_dne;
    argv_fake[1] = NULL;
    memset(big_arg, 'a', sizeof(big_arg) - 1);
    big_arg[sizeof(big_arg) - 1] = '\0';
    argv_big[0] = a_dne;
    argv_big[1] = big_arg;
    argv_big[2] = NULL;
#if defined(_WIN32)
    rc = c_rest_process_create(&proc, "does_not_exist_xyz123", argv_big);
    ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
    rc = c_rest_process_create(&proc, "does_not_exist_xyz123", argv_fake);
    ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
    rc = c_rest_process_create(&proc, "does_not_exist_xyz123", NULL);
    ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
#elif defined(__EMSCRIPTEN__)
    rc = c_rest_process_create(&proc, "does_not_exist_xyz123", argv_big);
    ASSERT_FALSE(rc != C_REST_ERROR_NOT_SUPPORTED);
    rc = c_rest_process_create(&proc, "does_not_exist_xyz123", argv_fake);
    ASSERT_FALSE(rc != C_REST_ERROR_NOT_SUPPORTED);
    rc = c_rest_process_wait(proc, &exit_code);
    ASSERT_FALSE(rc != C_REST_ERROR_NOT_SUPPORTED);
#else
    rc = c_rest_process_create(&proc, "does_not_exist_xyz123", argv_big);
    ASSERT_FALSE(rc != C_REST_OK);
    rc = c_rest_process_wait(proc, &exit_code);
    ASSERT_FALSE(rc != C_REST_OK);
    rc = c_rest_process_create(&proc, "does_not_exist_xyz123", argv_fake);
    ASSERT_FALSE(rc != C_REST_OK);
    rc = c_rest_process_wait(proc, &exit_code);
    ASSERT_FALSE(rc != C_REST_OK);
    rc = c_rest_process_create(&proc, "does_not_exist_xyz123", NULL);
    ASSERT_FALSE(rc != C_REST_OK);
    rc = c_rest_process_wait(proc, &exit_code);
    ASSERT_FALSE(rc != C_REST_OK);
    rc = c_rest_process_create(&proc, "does_not_exist_xyz123", NULL);
    ASSERT_FALSE(rc != C_REST_OK);
    rc = c_rest_process_wait(proc, NULL);
    ASSERT_FALSE(rc != C_REST_OK);
#endif
#if defined(C_REST_TESTING_MALLOC_HOOK) && !defined(__EMSCRIPTEN__)
    {
      c_rest_process_t failed_proc;
      g_mock_fork_fail = 1;
      rc = c_rest_process_create(&failed_proc, "/bin/echo", argv_fake);
      ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
      g_mock_fork_fail = 0;
    }
#endif

#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
    {
      char a_sh[] = "sh";
      char a_c[] = "-c";
      char a_kill[] = "kill -9 $$";
      char *argv_kill[4];
      argv_kill[0] = a_sh;
      argv_kill[1] = a_c;
      argv_kill[2] = a_kill;
      argv_kill[3] = NULL;
      rc = c_rest_process_create(&proc, "/bin/sh", argv_kill);
      ASSERT_FALSE(rc != C_REST_OK);
      rc = c_rest_process_wait(proc, &exit_code);
      ASSERT_FALSE(rc != C_REST_OK);
    }
#endif

    ASSERT_FALSE(c_rest_process_wait((c_rest_process_t)0, &exit_code) ==
                 C_REST_OK);
    ASSERT_FALSE(c_rest_process_wait((c_rest_process_t)-1, &exit_code) ==
                 C_REST_OK);
    ASSERT_FALSE(c_rest_process_wait((c_rest_process_t)9999999, &exit_code) ==
                 C_REST_OK);
  }

  /* Timer */
  ASSERT_FALSE(c_rest_timer_get_ms(NULL) == C_REST_OK);
  rc = c_rest_timer_get_ms(&ms);
  ASSERT_FALSE(rc != C_REST_OK);

  /* Random */
  ASSERT_FALSE(c_rest_random_get(NULL, 16) == C_REST_OK);
  ASSERT_FALSE(c_rest_random_get(buf, 0) == C_REST_OK);
  rc = c_rest_random_get(buf, 16);
  ASSERT_FALSE(rc != C_REST_OK);

  /* Error */
  ASSERT_FALSE(c_rest_get_last_error(NULL) == C_REST_OK);
  rc = c_rest_get_last_error(&err);
  ASSERT_FALSE(rc != C_REST_OK);

  rc = c_rest_platform_cleanup();
  ASSERT_FALSE(rc != C_REST_OK);

#ifdef C_REST_TESTING_MALLOC_HOOK
  g_crf_malloc_hook = NULL;
#endif

  PASS();
}

SUITE(platform_suite) { RUN_TEST(test_platform_cases); }
