/* clang-format off */
#include "mock_platform.h"
#include "c_rest_modality.h"
/* clang-format on */

int g_mock_socket_fail = 0;
int g_mock_tls_fail = 0;
int g_mock_platform_cleanup_fail = 0;
int g_mock_fork_fail = 0;

c_rest_error_t mock_c_rest_socket_create(c_rest_socket_t *sock) {
  if (g_mock_socket_fail == 1)
    return C_REST_ERROR_GENERIC;
  return c_rest_socket_create(sock);
}
c_rest_error_t mock_c_rest_socket_bind(c_rest_socket_t sock, const char *host,
                                       unsigned short port) {
  if (g_mock_socket_fail == 2 || g_mock_socket_fail == 1002)
    return C_REST_ERROR_GENERIC;
  return c_rest_socket_bind(sock, host, port);
}
c_rest_error_t mock_c_rest_socket_listen(c_rest_socket_t sock, int backlog) {
  if (g_mock_socket_fail == 3 || g_mock_socket_fail == 1003)
    return C_REST_ERROR_GENERIC;
  return c_rest_socket_listen(sock, backlog);
}
c_rest_error_t mock_c_rest_socket_accept(c_rest_socket_t server,
                                         c_rest_socket_t *out_client) {
  if (g_mock_socket_fail == 4)
    return C_REST_ERROR_GENERIC;
  if (g_mock_socket_fail >= 5 && g_mock_socket_fail <= 7) {
    g_mock_socket_fail += 100;
    *out_client = (c_rest_socket_t)12345;
    return C_REST_OK;
  }
  if (g_mock_socket_fail == 8 || g_mock_socket_fail == 10) {
    g_mock_socket_fail += 100;
    *out_client = C_REST_INVALID_SOCKET;
    return C_REST_OK;
  }
  if (g_mock_socket_fail == 11 || g_mock_socket_fail == 12 ||
      g_mock_socket_fail == 13) {
    g_mock_socket_fail += 100;
    *out_client = (c_rest_socket_t)12345;
    return C_REST_OK;
  }
  if (g_mock_socket_fail > 0)
    return C_REST_ERROR_GENERIC;
  return c_rest_socket_accept(server, out_client);
}
c_rest_error_t mock_c_rest_socket_close(c_rest_socket_t sock) {
  if (g_mock_socket_fail == 6 || g_mock_socket_fail == 106 ||
      g_mock_socket_fail == 108 || g_mock_socket_fail == 1002 ||
      g_mock_socket_fail == 1003)
    return C_REST_ERROR_GENERIC;
  if (sock == (c_rest_socket_t)9999 || sock == (c_rest_socket_t)12345)
    return C_REST_OK;
  return c_rest_socket_close(sock);
}
c_rest_error_t mock_c_rest_thread_create(c_rest_thread_t *thread,
                                         c_rest_error_t (*func)(void *),
                                         void *arg) {
  if (g_mock_socket_fail > 0) {
    if (g_mock_socket_fail == 13)
      return C_REST_ERROR_GENERIC;
    if (g_mock_socket_fail == 113)
      return C_REST_ERROR_GENERIC;
    func(arg);
    return C_REST_OK;
  }
  return c_rest_thread_create(thread, func, arg);
}
c_rest_error_t mock_c_rest_tls_accept(struct c_rest_tls_context *ctx,
                                      c_rest_socket_t sock,
                                      struct c_rest_tls_connection **out_conn) {
  if (g_mock_tls_fail == 1)
    return C_REST_ERROR_GENERIC;
  if (g_mock_tls_fail == 2) {
    *out_conn = (struct c_rest_tls_connection *)1;
    return C_REST_OK;
  }
  return c_rest_tls_accept(ctx, sock, out_conn);
}
c_rest_error_t mock_c_rest_tls_close(struct c_rest_tls_connection *conn) {
  if (g_mock_tls_fail == 2)
    return C_REST_ERROR_GENERIC;
  return c_rest_tls_close(conn);
}

const struct c_rest_platform_vtable mock_platform_vtable = {
    mock_c_rest_socket_create, mock_c_rest_socket_bind,
    mock_c_rest_socket_listen, mock_c_rest_socket_accept,
    mock_c_rest_socket_close,  c_rest_socket_send, /* pass through real func */
    c_rest_socket_recv,                            /* pass through real func */
    mock_c_rest_thread_create, c_rest_thread_join,
    mock_c_rest_tls_accept,    mock_c_rest_tls_close};
c_rest_error_t mock_c_rest_handle_connection(struct c_rest_context *ctx,
                                             c_rest_socket_t sock) {
  (void)ctx;
  (void)sock;
  return C_REST_OK;
}

c_rest_error_t mock_c_rest_process_create(c_rest_process_t *out_proc,
                                          const char *executable,
                                          char *const argv[]) {
  (void)executable;
  (void)argv;
  if (g_mock_fork_fail)
    return C_REST_ERROR_GENERIC;
  *out_proc = (c_rest_process_t)999;
  return C_REST_OK;
}
c_rest_error_t mock_c_rest_process_wait(c_rest_process_t proc,
                                        int *out_exit_code) {
  if (proc == (c_rest_process_t)999) {
    *out_exit_code = 0;
    return C_REST_OK;
  }
  return C_REST_ERROR_GENERIC;
}

int *g_crf_mem_initialized_ptr = NULL;
int g_mock_orm_cleanup_fail = 0;
int g_mock_orm_init_fail = 0;
int g_mock_parser_destroy_fail = 0;
int g_mock_parser_should_keep_alive_fail = 0;
int g_mock_parser_vtable_fail = 0;
int g_mock_req_cleanup_fail = 0;
int g_mock_res_cleanup_fail = 0;
int g_mock_res_status_fail = 0;
int g_mock_crypto_fail = 0;
int g_mock_client_fail = 0;
