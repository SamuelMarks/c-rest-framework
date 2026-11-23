/* clang-format off */
#include "c_rest_error.h"
#include "test_protos.h"
#include "c_rest_modality.h"
/* clang-format on */

#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
c_rest_error_t cm_env_create(cm_env_t *out_env) {
  *out_env = (cm_env_t)1;
  return C_REST_OK;
}

c_rest_error_t cm_env_destroy(cm_env_t env) {
  (void)env;
  return C_REST_OK;
}

c_rest_error_t cm_env_set_allocator(cm_env_t env,
                                    const struct cm_allocator *alloc) {
  (void)env;
  (void)alloc;
  return C_REST_OK;
}

c_rest_error_t cm_env_set_logger(cm_env_t env, const struct cm_logger *logger) {
  (void)env;
  (void)logger;
  return C_REST_OK;
}

c_rest_error_t cm_socket_create(cm_env_t env, cm_socket_t *out_sock) {
  (void)env;
  *out_sock = (cm_socket_t)2;
  return C_REST_OK;
}

c_rest_error_t cm_socket_bind(cm_env_t env, cm_socket_t sock, const char *host,
                              unsigned short port) {
  (void)env;
  (void)sock;
  (void)host;
  (void)port;
  return C_REST_OK;
}

c_rest_error_t cm_socket_listen(cm_env_t env, cm_socket_t sock, int backlog) {
  (void)env;
  (void)sock;
  (void)backlog;
  return C_REST_OK;
}

int g_accept_calls = 0;
c_rest_error_t cm_socket_accept(cm_env_t env, cm_socket_t server,
                                cm_socket_t *out_client) {
  (void)env;
  (void)server;
  if (g_accept_calls++ < 1) {
    *out_client = (cm_socket_t)3;
    return C_REST_OK;
  }
  return C_REST_ERROR_GENERIC;
}

c_rest_error_t cm_socket_close(cm_env_t env, cm_socket_t sock) {
  (void)env;
  (void)sock;
  return C_REST_OK;
}

c_rest_error_t cm_socket_set_nonblocking(cm_env_t env, cm_socket_t sock,
                                         int nonblocking) {
  (void)env;
  (void)sock;
  (void)nonblocking;
  return C_REST_OK;
}

int g_mock_cm_thread_fail = 0;
c_rest_error_t cm_thread_create(cm_env_t env, cm_thread_t *out_thread,
                                void (*func)(void *), void *arg) {
  if (g_mock_cm_thread_fail)
    return C_REST_ERROR_GENERIC;
  (void)env;
  (void)func;
  (void)arg;
  *out_thread = (cm_thread_t)4;
  return C_REST_OK;
}

int g_mock_cm_join_fail = 0;
c_rest_error_t cm_thread_join(cm_env_t env, cm_thread_t thread) {
  if (g_mock_cm_join_fail)
    return C_REST_ERROR_GENERIC;
  (void)env;
  (void)thread;
  return C_REST_OK;
}

c_rest_error_t cm_thread_sleep_ms(cm_env_t env, unsigned long ms) {
  (void)env;
  (void)ms;
  return C_REST_OK;
}

int g_mock_cm_file_fail = 0;
c_rest_error_t cm_file_get_mtime(cm_env_t env, const char *path,
                                 unsigned long *out_mtime) {
  if (g_mock_cm_file_fail)
    return C_REST_ERROR_GENERIC;
  (void)env;
  (void)path;
  *out_mtime = 123456789UL;
  return C_REST_OK;
}

c_rest_error_t cm_timer_get_ms(cm_env_t env, unsigned long *out_ms) {
  (void)env;
  *out_ms = 1000UL;
  return C_REST_OK;
}

TEST test_multiplatform_env_hooks(void) {
  struct c_rest_context *ctx = NULL;
  cm_env_t env = NULL;
  cm_socket_t sock = 0;
  unsigned long mtime = 0;
  unsigned long ms = 0;

  ASSERT_EQ(C_REST_OK, cm_env_create(&env));
  ASSERT(env != NULL);
  ASSERT_EQ(C_REST_OK, cm_socket_create(env, &sock));
  ASSERT_EQ((cm_socket_t)2, sock);
  ASSERT_EQ(C_REST_OK, cm_file_get_mtime(env, "test", &mtime));
  ASSERT_EQ(123456789UL, mtime);
  ASSERT_EQ(C_REST_OK, cm_timer_get_ms(env, &ms));
  ASSERT_EQ(1000UL, ms);

  ASSERT_EQ(C_REST_OK, c_rest_init(C_REST_MODALITY_SYNC, &ctx));
  ASSERT_EQ(C_REST_OK, c_rest_set_multiplatform_env(ctx, env));
  ASSERT_EQ(C_REST_OK, c_rest_destroy(ctx));
  ASSERT_EQ(C_REST_OK, cm_env_destroy(env));
  PASS();
}
#else
TEST test_multiplatform_env_hooks(void) { PASS(); }
#endif

SUITE(multiplatform_suite) { RUN_TEST(test_multiplatform_env_hooks); }
