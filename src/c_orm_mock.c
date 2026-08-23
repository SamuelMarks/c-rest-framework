#include "c_rest_testing_mocks.h"
/* clang-format off */
#include "c_rest_orm.h"

#include <stddef.h>
#include "c_rest_error.h"
/* clang-format on */

/* #ifndef C_REST_FRAMEWORK_USE_REAL_C_ORM */

/* Mock implementations for c-orm functions when c-orm is not present */

#ifdef C_REST_TESTING_MALLOC_HOOK
C_REST_EXPORT int g_mock_orm_init_fail = 0;
C_REST_EXPORT int g_mock_orm_cleanup_fail = 0;
C_REST_EXPORT int g_mock_socket_fail = 0;
#endif

c_rest_error_t c_rest_orm_init(struct c_rest_db_config *config,
                               struct c_orm_pool **pool) {
#ifdef C_REST_TESTING_MALLOC_HOOK
  if (g_mock_orm_init_fail) {
    return C_REST_ERROR_GENERIC;
  }
#endif
  /* Provide a dummy pool pointer */
  if (config && pool) {
    *pool = (struct c_orm_pool *)1;
  }
  return C_REST_OK;
}

c_rest_error_t c_rest_orm_cleanup(struct c_orm_pool *pool) {
#ifdef C_REST_TESTING_MALLOC_HOOK
  if (g_mock_orm_cleanup_fail) {
    return C_REST_ERROR_GENERIC;
  }
#endif
  (void)pool;
  return C_REST_OK;
}

/* #endif C_REST_FRAMEWORK_USE_REAL_C_ORM */

typedef int c_orm_mock_dummy_declaration;

#include "c_rest_export.h"
#include "c_rest_modality.h"
#include "c_rest_platform.h"
#include "c_rest_tls.h"

C_REST_EXPORT extern int g_mock_socket_fail;
C_REST_EXPORT int g_mock_tls_fail = 0;

#undef c_rest_socket_create
#undef c_rest_socket_bind
#undef c_rest_socket_listen
#undef c_rest_socket_accept
#undef c_rest_thread_create
#undef c_rest_socket_close
#undef c_rest_tls_accept
#undef c_rest_tls_close
#undef c_rest_handle_connection

c_rest_error_t mock_c_rest_socket_create(c_rest_socket_t *sock);
c_rest_error_t mock_c_rest_socket_bind(c_rest_socket_t sock, const char *host,
                                       unsigned short port);
c_rest_error_t mock_c_rest_socket_listen(c_rest_socket_t sock, int backlog);
c_rest_error_t mock_c_rest_socket_accept(c_rest_socket_t server,
                                         c_rest_socket_t *out_client);
c_rest_error_t mock_c_rest_thread_create(c_rest_thread_t *thread,
                                         c_rest_error_t (*func)(void *),
                                         void *arg);
c_rest_error_t mock_c_rest_socket_close(c_rest_socket_t sock);
c_rest_error_t mock_c_rest_tls_accept(struct c_rest_tls_context *ctx,
                                      c_rest_socket_t sock,
                                      struct c_rest_tls_connection **conn);
c_rest_error_t mock_c_rest_tls_close(struct c_rest_tls_connection *conn);
c_rest_error_t mock_c_rest_handle_connection(struct c_rest_context *ctx,
                                             c_rest_socket_t sock);

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
  if (g_mock_socket_fail == 11 || g_mock_socket_fail == 12) {
    g_mock_socket_fail += 100;
    *out_client = (c_rest_socket_t)12345;
    return C_REST_OK;
  }
  if (g_mock_socket_fail > 0)
    return C_REST_ERROR_GENERIC;
  return c_rest_socket_accept(server, out_client);
}
c_rest_error_t mock_c_rest_thread_create(c_rest_thread_t *thread,
                                         c_rest_error_t (*func)(void *),
                                         void *arg) {

  if (g_mock_socket_fail > 0) {
    func(arg);
    return C_REST_OK;
  }
  return c_rest_thread_create(thread, func, arg);
}
c_rest_error_t mock_c_rest_socket_close(c_rest_socket_t sock) {
  if (g_mock_socket_fail == 6 || g_mock_socket_fail == 106 ||
      g_mock_socket_fail == 108 || g_mock_socket_fail == 1002 ||
      g_mock_socket_fail == 1003)
    return C_REST_ERROR_GENERIC;

  if (sock == (c_rest_socket_t)9999)
    return C_REST_OK;

  return c_rest_socket_close(sock);
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
c_rest_error_t mock_c_rest_handle_connection(struct c_rest_context *ctx,
                                             c_rest_socket_t sock) {
  return c_rest_handle_connection(ctx, sock);
}
