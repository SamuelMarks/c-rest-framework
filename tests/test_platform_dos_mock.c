#define c_rest_platform_init dos_c_rest_platform_init
#define c_rest_platform_cleanup dos_c_rest_platform_cleanup
#define c_rest_socket_create dos_c_rest_socket_create
#define c_rest_socket_bind dos_c_rest_socket_bind
#define c_rest_socket_listen dos_c_rest_socket_listen
#define c_rest_socket_accept dos_c_rest_socket_accept
#define c_rest_socket_set_nonblocking dos_c_rest_socket_set_nonblocking
#define c_rest_socket_close dos_c_rest_socket_close
#define c_rest_thread_create dos_c_rest_thread_create
#define c_rest_thread_join dos_c_rest_thread_join
#define c_rest_mutex_create dos_c_rest_mutex_create
#define c_rest_mutex_lock dos_c_rest_mutex_lock
#define c_rest_mutex_unlock dos_c_rest_mutex_unlock
#define c_rest_mutex_destroy dos_c_rest_mutex_destroy
#define c_rest_cond_create dos_c_rest_cond_create
#define c_rest_cond_wait dos_c_rest_cond_wait
#define c_rest_cond_signal dos_c_rest_cond_signal
#define c_rest_cond_destroy dos_c_rest_cond_destroy
#define c_rest_process_create dos_c_rest_process_create
#define c_rest_process_wait dos_c_rest_process_wait
#define c_rest_timer_get_ms dos_c_rest_timer_get_ms
#define c_rest_random_get dos_c_rest_random_get
#define c_rest_get_last_error dos_c_rest_get_last_error
#define c_rest_socket_recv dos_c_rest_socket_recv
#define c_rest_socket_send dos_c_rest_socket_send

/* clang-format off */
#include "c_rest_error.h"
#include "greatest.h"
#include "greatest_clean.h"

#undef C_REST_EXPORT
#define C_REST_EXPORT

#include "c_rest_platform.h"
#include <stdlib.h>
#include <string.h>
/* clang-format on */

#include "../src/platform_dos.c"

TEST test_dos_platform_lifecycle(void) {
  ASSERT_EQ(C_REST_OK, dos_c_rest_platform_init());
  ASSERT_EQ(C_REST_OK, dos_c_rest_platform_cleanup());
  PASS();
}

TEST test_dos_socket_ops(void) {
  c_rest_socket_t sock;
  c_rest_socket_t client_sock;
  char buf[16];
  size_t read_len;
  size_t write_len;

  ASSERT_EQ(C_REST_ERROR_INVALID_ARG, dos_c_rest_socket_create(NULL));
  sock = 123;
  ASSERT_EQ(C_REST_ERROR_NOT_SUPPORTED, dos_c_rest_socket_create(&sock));
  ASSERT_EQ(C_REST_INVALID_SOCKET, sock);

  ASSERT_EQ(C_REST_ERROR_NOT_SUPPORTED,
            dos_c_rest_socket_bind(sock, "127.0.0.1", 8080));
  ASSERT_EQ(C_REST_ERROR_NOT_SUPPORTED, dos_c_rest_socket_listen(sock, 10));

  ASSERT_EQ(C_REST_ERROR_NOT_SUPPORTED, dos_c_rest_socket_accept(sock, NULL));
  client_sock = 123;
  ASSERT_EQ(C_REST_ERROR_NOT_SUPPORTED,
            dos_c_rest_socket_accept(sock, &client_sock));
  ASSERT_EQ(C_REST_INVALID_SOCKET, client_sock);

  ASSERT_EQ(C_REST_ERROR_NOT_SUPPORTED,
            dos_c_rest_socket_set_nonblocking(sock, 1));
  ASSERT_EQ(C_REST_OK, dos_c_rest_socket_close(sock));

  ASSERT_EQ(C_REST_ERROR_INVALID_ARG,
            dos_c_rest_socket_recv(sock, buf, sizeof(buf), NULL));
  read_len = 99;
  ASSERT_EQ(C_REST_ERROR_NOT_SUPPORTED,
            dos_c_rest_socket_recv(sock, buf, sizeof(buf), &read_len));
  ASSERT_EQ((size_t)0, read_len);

  ASSERT_EQ(C_REST_ERROR_INVALID_ARG,
            dos_c_rest_socket_send(sock, buf, sizeof(buf), NULL));
  write_len = 99;
  ASSERT_EQ(C_REST_ERROR_NOT_SUPPORTED,
            dos_c_rest_socket_send(sock, buf, sizeof(buf), &write_len));
  ASSERT_EQ((size_t)0, write_len);

  PASS();
}

TEST test_dos_threading_and_sync(void) {
  c_rest_thread_t thread;
  c_rest_mutex_t mutex;
  c_rest_cond_t cond;

  memset(&thread, 0, sizeof(thread));
  memset(&mutex, 0, sizeof(mutex));
  memset(&cond, 0, sizeof(cond));

  ASSERT_EQ(C_REST_ERROR_NOT_SUPPORTED,
            dos_c_rest_thread_create(&thread, NULL, NULL));
  ASSERT_EQ(C_REST_ERROR_NOT_SUPPORTED, dos_c_rest_thread_join(thread));

  ASSERT_EQ(C_REST_OK, dos_c_rest_mutex_create(NULL));
  ASSERT_EQ(C_REST_OK, dos_c_rest_mutex_create(&mutex));
  ASSERT_EQ(C_REST_OK, dos_c_rest_mutex_lock(mutex));
  ASSERT_EQ(C_REST_OK, dos_c_rest_mutex_unlock(mutex));
  ASSERT_EQ(C_REST_OK, dos_c_rest_mutex_destroy(mutex));

  ASSERT_EQ(C_REST_ERROR_NOT_SUPPORTED, dos_c_rest_cond_create(&cond));
  ASSERT_EQ(C_REST_ERROR_NOT_SUPPORTED, dos_c_rest_cond_wait(cond, mutex));
  ASSERT_EQ(C_REST_ERROR_NOT_SUPPORTED, dos_c_rest_cond_signal(cond));
  ASSERT_EQ(C_REST_ERROR_NOT_SUPPORTED, dos_c_rest_cond_destroy(cond));

  PASS();
}

TEST test_dos_process_and_misc(void) {
  c_rest_process_t proc;
  int exit_code;
  unsigned long ms;
  unsigned char rand_buf[16];
  int last_err;

  memset(&proc, 0, sizeof(proc));
  exit_code = 0;
  ms = 0;
  last_err = 0;

  ASSERT_EQ(C_REST_ERROR_NOT_SUPPORTED,
            dos_c_rest_process_create(&proc, "test", NULL));
  ASSERT_EQ(C_REST_ERROR_NOT_SUPPORTED,
            dos_c_rest_process_wait(proc, &exit_code));

  ASSERT_EQ(C_REST_ERROR_INVALID_ARG, dos_c_rest_timer_get_ms(NULL));
  ASSERT_EQ(C_REST_OK, dos_c_rest_timer_get_ms(&ms));

  ASSERT_EQ(C_REST_ERROR_INVALID_ARG, dos_c_rest_random_get(NULL, 16));
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG, dos_c_rest_random_get(rand_buf, 0));
  ASSERT_EQ(C_REST_OK, dos_c_rest_random_get(rand_buf, sizeof(rand_buf)));

  ASSERT_EQ(C_REST_ERROR_INVALID_ARG, dos_c_rest_get_last_error(NULL));
  ASSERT_EQ(C_REST_OK, dos_c_rest_get_last_error(&last_err));

  PASS();
}

SUITE_EXTERN(platform_dos_suite);
SUITE(platform_dos_suite) {
  RUN_TEST(test_dos_platform_lifecycle);
  RUN_TEST(test_dos_socket_ops);
  RUN_TEST(test_dos_threading_and_sync);
  RUN_TEST(test_dos_process_and_misc);
}
