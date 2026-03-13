/* clang-format off */
#include "c_rest_modality.h"
#include <stdio.h>
/* clang-format on */

#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
/* Mock c-multiplatform implementations */
int cm_env_create(cm_env_t *out_env) {
  *out_env = (cm_env_t)1;
  return 0;
}

int cm_env_destroy(cm_env_t env) {
  (void)env;
  return 0;
}

int cm_env_set_allocator(cm_env_t env, const struct cm_allocator *alloc) {
  (void)env;
  (void)alloc;
  return 0;
}

int cm_env_set_logger(cm_env_t env, const struct cm_logger *logger) {
  (void)env;
  (void)logger;
  return 0;
}

int cm_socket_create(cm_env_t env, cm_socket_t *out_sock) {
  (void)env;
  *out_sock = (cm_socket_t)2;
  return 0;
}

int cm_socket_bind(cm_env_t env, cm_socket_t sock, const char *host,
                   unsigned short port) {
  (void)env;
  (void)sock;
  (void)host;
  (void)port;
  return 0;
}

int cm_socket_listen(cm_env_t env, cm_socket_t sock, int backlog) {
  (void)env;
  (void)sock;
  (void)backlog;
  return 0;
}

int cm_socket_accept(cm_env_t env, cm_socket_t server,
                     cm_socket_t *out_client) {
  (void)env;
  (void)server;
  *out_client = (cm_socket_t)3;
  return 0;
}

int cm_socket_close(cm_env_t env, cm_socket_t sock) {
  (void)env;
  (void)sock;
  return 0;
}

int cm_socket_set_nonblocking(cm_env_t env, cm_socket_t sock, int nonblocking) {
  (void)env;
  (void)sock;
  (void)nonblocking;
  return 0;
}

int cm_thread_create(cm_env_t env, cm_thread_t *out_thread,
                     void (*func)(void *), void *arg) {
  (void)env;
  (void)func;
  (void)arg;
  *out_thread = (cm_thread_t)4;
  return 0;
}

int cm_thread_join(cm_env_t env, cm_thread_t thread) {
  (void)env;
  (void)thread;
  return 0;
}

int cm_timer_get_ms(cm_env_t env, unsigned long *out_ms) {
  (void)env;
  *out_ms = 1000;
  return 0;
}

int test_multiplatform_integration(void) {
  struct c_rest_context *ctx;
  cm_env_t env;
  int res;

  printf("Running multiplatform integration test...\n");

  res = cm_env_create(&env);
  if (res != 0)
    return 1;

  res = c_rest_init(C_REST_MODALITY_SYNC, &ctx);
  if (res != 0)
    return 1;

  res = c_rest_set_multiplatform_env(ctx, env);
  if (res != 0)
    return 1;

  /* Should use mock cm_socket_close via cm_env */
  res = c_rest_destroy(ctx);
  if (res != 0)
    return 1;

  res = cm_env_destroy(env);
  if (res != 0)
    return 1;

  return 0;
}
#else
int test_multiplatform_integration(void) {
  printf("Multiplatform integration not enabled, skipping test.\n");
  return 0;
}
#endif
