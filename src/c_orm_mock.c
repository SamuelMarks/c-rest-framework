/* clang-format off */
#include "c_rest_testing_mocks.h"
#include "c_rest_orm.h"
#include "c_rest_error.h"
#include "c_rest_export.h"
#include "c_rest_modality.h"
#include "c_rest_platform.h"
#include "c_rest_tls.h"
#include "c_rest_ts_queue.h"
#include "c_rest_string.h"
#include "c_rest_sse.h"
#include "c_rest_response.h"
#include "c_rest_hashmap.h"
#include "c_rest_client.h"
#include "c_rest_crypto.h"
#include "c_rest_base64.h"
#include <stddef.h>
#include <string.h>
/* clang-format on */

C_REST_EXPORT int g_mock_orm_init_fail = 0;
C_REST_EXPORT int g_mock_orm_cleanup_fail = 0;
C_REST_EXPORT int g_mock_socket_fail = 0;
C_REST_EXPORT int g_mock_fork_fail = 0;
C_REST_EXPORT int g_mock_tls_fail = 0;
C_REST_EXPORT int g_mock_client_fail = 0;
C_REST_EXPORT int g_mock_platform_cleanup_fail = 0;
C_REST_EXPORT int g_mock_parser_destroy_fail = 0;
C_REST_EXPORT int g_mock_parser_should_keep_alive_fail = 0;
C_REST_EXPORT int g_mock_parser_vtable_fail = 0;
C_REST_EXPORT int g_mock_req_cleanup_fail = 0;
C_REST_EXPORT int g_mock_res_cleanup_fail = 0;
C_REST_EXPORT int g_mock_res_status_fail = 0;
C_REST_EXPORT int g_mock_graphql_ws_countdown = -1;
C_REST_EXPORT int g_mock_graphql_free_countdown = -1;
C_REST_EXPORT int g_mock_sse_append_fail = -1;
C_REST_EXPORT int g_mock_openapi_fail = 0;
C_REST_EXPORT int g_mock_hot_reload_fail = 0;
C_REST_EXPORT int g_mock_lib_fail = 0;
C_REST_EXPORT int g_mock_jwt_fail = 0;
C_REST_EXPORT int g_mock_mem_fail = 0;
C_REST_EXPORT int g_mock_rate_limit_fail = 0;
C_REST_EXPORT int g_mock_tsq_fail = 0;
C_REST_EXPORT int g_mock_crypto_fail = 0;

#undef c_rest_socket_create
#undef c_rest_socket_bind
#undef c_rest_socket_listen
#undef c_rest_socket_accept
#undef c_rest_thread_create
#undef c_rest_socket_close
#undef c_rest_tls_accept
#undef c_rest_tls_close
#undef c_rest_handle_connection
#undef c_rest_ts_queue_pop

c_rest_error_t mock_c_rest_socket_create(c_rest_socket_t *sock);
c_rest_error_t mock_c_rest_socket_bind(c_rest_socket_t sock, const char *host,
                                       unsigned short port);
c_rest_error_t mock_c_rest_socket_listen(c_rest_socket_t sock, int backlog);
c_rest_error_t mock_c_rest_tls_accept(struct c_rest_tls_context *ctx,
                                      c_rest_socket_t sock,
                                      struct c_rest_tls_connection **conn);
c_rest_error_t mock_c_rest_tls_close(struct c_rest_tls_connection *conn);

c_rest_error_t mock_c_rest_socket_create(c_rest_socket_t *sock) {
  if (g_mock_socket_fail == 1)
    return C_REST_ERROR_GENERIC;
  return c_rest_socket_create(sock);
}

c_rest_error_t mock_c_rest_socket_bind(c_rest_socket_t sock, const char *host,
                                       unsigned short port) {
  if ((g_mock_socket_fail == 2) | (g_mock_socket_fail == 1002))
    return C_REST_ERROR_GENERIC;
  return c_rest_socket_bind(sock, host, port);
}

c_rest_error_t mock_c_rest_socket_listen(c_rest_socket_t sock, int backlog) {
  if ((g_mock_socket_fail == 3) | (g_mock_socket_fail == 1003))
    return C_REST_ERROR_GENERIC;
  return c_rest_socket_listen(sock, backlog);
}

c_rest_error_t mock_c_rest_socket_accept(c_rest_socket_t server,
                                         c_rest_socket_t *out_client) {
  if (g_mock_socket_fail == 4)
    return C_REST_ERROR_GENERIC;
  if (((unsigned int)(g_mock_socket_fail - 5) <= 2U) |
      ((unsigned int)(g_mock_socket_fail - 11) <= 6U)) {
    g_mock_socket_fail += 100;
    *out_client = (c_rest_socket_t)12345;
    return C_REST_OK;
  }
  if ((g_mock_socket_fail == 8) | (g_mock_socket_fail == 10)) {
    g_mock_socket_fail += 100;
    *out_client = C_REST_INVALID_SOCKET;
    return C_REST_OK;
  }
  if (g_mock_socket_fail > 0)
    return C_REST_ERROR_GENERIC;
  return c_rest_socket_accept(server, out_client);
}

c_rest_error_t mock_c_rest_ts_queue_pop(struct c_rest_ts_queue *queue,
                                        void **out_data) {
  if (g_mock_socket_fail == 117)
    return C_REST_ERROR_GENERIC;
  return c_rest_ts_queue_pop(queue, out_data);
}

static void test_invoke_worker_sync(c_rest_error_t (*func)(void *), void *arg) {
  c_rest_error_t worker_rc = func(arg);
  if (worker_rc != C_REST_OK) {
    return;
  }
}

c_rest_error_t mock_c_rest_thread_create(c_rest_thread_t *thread,
                                         c_rest_error_t (*func)(void *),
                                         void *arg) {
  if (g_mock_socket_fail > 0) {
    if ((g_mock_socket_fail == 13) | (g_mock_socket_fail == 113) |
        (g_mock_socket_fail == 114))
      return C_REST_ERROR_GENERIC;
    if (thread)
      *thread = (c_rest_thread_t)0;
    test_invoke_worker_sync(func, arg);
    return C_REST_OK;
  }
  return c_rest_thread_create(thread, func, arg);
}

c_rest_error_t mock_c_rest_socket_close(c_rest_socket_t sock) {
  if ((g_mock_socket_fail == 6) | (g_mock_socket_fail == 106) |
      (g_mock_socket_fail == 108) | (g_mock_socket_fail == 1002) |
      (g_mock_socket_fail == 1003))
    return C_REST_ERROR_GENERIC;
  if ((g_mock_socket_fail == 107) |
      ((unsigned int)(g_mock_socket_fail - 114) <= 2U))
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
  if ((g_mock_socket_fail == 6) | (g_mock_socket_fail == 106) |
      (g_mock_socket_fail == 115))
    return C_REST_OK;
  if ((g_mock_socket_fail == 7) | (g_mock_socket_fail == 107))
    return C_REST_ERROR_GENERIC;
  if (g_mock_socket_fail == 116) {
    return c_rest_stop(ctx);
  }
  return c_rest_handle_connection(ctx, sock);
}

c_rest_error_t crf_seam_modality_platform_init(void) {
  if (g_mock_socket_fail == 99)
    return C_REST_ERROR_GENERIC;
  return c_rest_platform_init();
}

c_rest_error_t crf_seam_platform_cleanup_hook(void) {
  return g_mock_platform_cleanup_fail ? C_REST_ERROR_GENERIC : C_REST_OK;
}

c_rest_error_t crf_seam_fork_hook(void) {
  return g_mock_fork_fail ? C_REST_ERROR_GENERIC : C_REST_OK;
}

c_rest_error_t crf_seam_socket_check(int code, c_rest_error_t err) {
  return (g_mock_socket_fail == code) ? err : C_REST_OK;
}

c_rest_error_t crf_seam_socket_send(size_t len, size_t *out_written,
                                    int *out_intercepted) {
  if (g_mock_socket_fail == 200) {
    *out_written = len;
    *out_intercepted = 1;
    return C_REST_OK;
  }
  if (g_mock_socket_fail == 201) {
    *out_intercepted = 1;
    return C_REST_ERROR_GENERIC;
  }
  if ((g_mock_socket_fail == 202) | (g_mock_socket_fail == 203)) {
    g_mock_socket_fail--;
    *out_written = len;
    *out_intercepted = 1;
    return C_REST_OK;
  }
  *out_intercepted = 0;
  return C_REST_OK;
}

c_rest_error_t crf_seam_tls_read(void *buf, size_t len, size_t *out_read,
                                 int *out_intercepted) {
  if (g_mock_tls_fail == 7) {
    static const char mock_req[19] = {'G', 'E', 'T', ' ', '/', ' ', 'H',
                                      'T', 'T', 'P', '/', '1', '.', '0',
                                      13,  10,  13,  10,  0};
    size_t n = sizeof(mock_req) - 1U;
    if (n > len)
      n = len;
#if defined(_MSC_VER)
    memcpy_s(buf, len, mock_req, n);
#else
    memcpy(buf, mock_req, n);
#endif
    *out_read = n;
    g_mock_tls_fail = 3;
    *out_intercepted = 1;
    return C_REST_OK;
  }
  *out_intercepted = 0;
  return C_REST_OK;
}

c_rest_error_t crf_seam_tls_write(size_t len, size_t *out_written,
                                  int *out_intercepted) {
  static int write_count = 0;
  if (g_mock_tls_fail == 1) {
    *out_intercepted = 1;
    return C_REST_ERROR_GENERIC;
  }
  if (g_mock_tls_fail == 3) {
    if (out_written)
      *out_written = len;
    *out_intercepted = 1;
    return C_REST_OK;
  }
  if ((g_mock_tls_fail == 4) | (g_mock_tls_fail == 5)) {
    write_count++;
    if (write_count == g_mock_tls_fail - 2) {
      write_count = 0;
      *out_intercepted = 1;
      return C_REST_ERROR_GENERIC;
    }
    if (out_written)
      *out_written = len;
    *out_intercepted = 1;
    return C_REST_OK;
  }
  *out_intercepted = 0;
  return C_REST_OK;
}

c_rest_error_t crf_seam_tls_close_hook(void) {
  return (g_mock_tls_fail == 6) ? C_REST_ERROR_GENERIC : C_REST_OK;
}

c_rest_error_t crf_seam_parser_keep_alive_hook(void) {
  return g_mock_parser_should_keep_alive_fail ? C_REST_ERROR_GENERIC
                                              : C_REST_OK;
}

c_rest_error_t crf_seam_parser_destroy_hook(void) {
  return g_mock_parser_destroy_fail ? C_REST_ERROR_GENERIC : C_REST_OK;
}

c_rest_error_t crf_seam_parser_vtable_hook(void) {
  return g_mock_parser_vtable_fail ? C_REST_ERROR_GENERIC : C_REST_OK;
}

c_rest_error_t crf_seam_res_status_hook(void) {
  return g_mock_res_status_fail ? C_REST_ERROR_GENERIC : C_REST_OK;
}

c_rest_error_t crf_seam_res_cleanup_hook(void) {
  return g_mock_res_cleanup_fail ? C_REST_ERROR_GENERIC : C_REST_OK;
}

c_rest_error_t crf_seam_req_cleanup_hook(void) {
  return g_mock_req_cleanup_fail ? C_REST_ERROR_GENERIC : C_REST_OK;
}

c_rest_error_t crf_seam_router_openapi_hook(void) {
  return (g_mock_openapi_fail == 7) ? C_REST_ERROR_GENERIC : C_REST_OK;
}

static NO_DISCARD c_rest_error_t crf_step_countdown(int *ctr) {
  if (*ctr >= 0) {
    if (*ctr == 0) {
      (*ctr)--;
      return C_REST_ERROR_GENERIC;
    }
    (*ctr)--;
  }
  return C_REST_OK;
}

c_rest_error_t crf_seam_graphql_ws_step(void) {
  return crf_step_countdown(&g_mock_graphql_ws_countdown);
}

c_rest_error_t crf_seam_graphql_free_step(void) {
  return crf_step_countdown(&g_mock_graphql_free_countdown);
}

c_rest_error_t crf_seam_sse_append_cstr(struct c_rest_string *s,
                                        const char *c) {
  if ((g_mock_sse_append_fail == 0) | (g_mock_sse_append_fail == -5))
    return C_REST_ERROR_GENERIC;
  if (g_mock_sse_append_fail > 0)
    g_mock_sse_append_fail--;
  return c_rest_string_append_cstr(s, c);
}

c_rest_error_t crf_seam_sse_append(struct c_rest_string *s, const char *c,
                                   size_t l) {
  if (g_mock_sse_append_fail == 0)
    return C_REST_ERROR_GENERIC;
  if (g_mock_sse_append_fail > 0)
    g_mock_sse_append_fail--;
  return c_rest_string_append(s, c, l);
}

c_rest_error_t crf_seam_sse_string_destroy(struct c_rest_string *s) {
  if ((g_mock_sse_append_fail == -2) | (g_mock_sse_append_fail == -5))
    return C_REST_ERROR_GENERIC;
  return c_rest_string_destroy(s);
}

#ifdef C_REST_ENABLE_SERVER_SENT_EVENTS_SSE
c_rest_error_t crf_seam_sse_event_destroy(struct c_rest_sse_event *ev) {
  if (g_mock_sse_append_fail == -3)
    return C_REST_ERROR_GENERIC;
  return c_rest_sse_event_destroy(ev);
}

c_rest_error_t crf_seam_sse_event_init(struct c_rest_sse_event *ev) {
  if (g_mock_sse_append_fail == -4)
    return C_REST_ERROR_GENERIC;
  return c_rest_sse_event_init(ev);
}

c_rest_error_t crf_seam_hr_sse_event_init(struct c_rest_sse_event *ev) {
  if (g_mock_hot_reload_fail == 4)
    return C_REST_ERROR_GENERIC;
  return c_rest_sse_event_init(ev);
}

c_rest_error_t crf_seam_hr_sse_event_destroy(struct c_rest_sse_event *ev) {
  if (g_mock_hot_reload_fail == 3)
    return C_REST_ERROR_GENERIC;
  return c_rest_sse_event_destroy(ev);
}
#endif

c_rest_error_t crf_seam_openapi_op_step(void) {
  if ((unsigned int)(g_mock_openapi_fail - 10) <= 8U) {
    if (g_mock_openapi_fail == 10)
      return C_REST_ERROR_GENERIC;
    g_mock_openapi_fail--;
  }
  return C_REST_OK;
}

c_rest_error_t crf_seam_openapi_check(int code) {
  return (g_mock_openapi_fail == code) ? C_REST_ERROR_GENERIC : C_REST_OK;
}

c_rest_error_t crf_seam_hot_reload_check(int code, c_rest_error_t err) {
  return (g_mock_hot_reload_fail == code) ? err : C_REST_OK;
}

c_rest_error_t crf_seam_lib_check(int code) {
  return (g_mock_lib_fail == code) ? C_REST_ERROR_GENERIC : C_REST_OK;
}

c_rest_error_t crf_seam_jwt_set_status(struct c_rest_response *r, int s) {
  unsigned int code = (unsigned int)g_mock_jwt_fail & 31U;
  if (((1UL << code) & ((1UL << 1) | (1UL << 4) | (1UL << 7) | (1UL << 10))) !=
      0UL)
    return C_REST_ERROR_GENERIC;
  return c_rest_response_set_status(r, s);
}

c_rest_error_t crf_seam_jwt_html(struct c_rest_response *r, const char *h) {
  unsigned int code = (unsigned int)g_mock_jwt_fail & 31U;
  if (((1UL << code) & ((1UL << 2) | (1UL << 3) | (1UL << 6) | (1UL << 9) |
                        (1UL << 12))) != 0UL)
    return C_REST_ERROR_GENERIC;
  return c_rest_response_html(r, h);
}

c_rest_error_t crf_seam_jwt_set_header(struct c_rest_response *r, const char *k,
                                       const char *v) {
  unsigned int code = (unsigned int)g_mock_jwt_fail & 31U;
  if (((1UL << code) & ((1UL << 5) | (1UL << 8) | (1UL << 11))) != 0UL)
    return C_REST_ERROR_GENERIC;
  return c_rest_response_set_header(r, k, v);
}

c_rest_error_t crf_seam_mem_lock(c_rest_mutex_t m) {
  if (g_mock_mem_fail == 5)
    return C_REST_ERROR_GENERIC;
  return c_rest_mutex_lock(m);
}

c_rest_error_t crf_seam_mem_unlock(c_rest_mutex_t m) {
  unsigned int code = (unsigned int)g_mock_mem_fail & 31U;
  if (((1UL << code) & ((1UL << 1) | (1UL << 2) | (1UL << 4) | (1UL << 8) |
                        (1UL << 10))) != 0UL) {
    return (c_rest_error_t)((int)C_REST_ERROR_GENERIC |
                            (int)c_rest_mutex_unlock(m));
  }
  return c_rest_mutex_unlock(m);
}

c_rest_error_t crf_seam_mem_destroy(c_rest_mutex_t m) {
  if (g_mock_mem_fail == 11)
    return C_REST_ERROR_GENERIC;
  return c_rest_mutex_destroy(m);
}

c_rest_error_t crf_seam_rl_mutex_create(c_rest_mutex_t *m) {
  if ((g_mock_rate_limit_fail == 1) | (g_mock_rate_limit_fail == 2))
    return C_REST_ERROR_GENERIC;
  return c_rest_mutex_create(m);
}

c_rest_error_t crf_seam_rl_mutex_lock(c_rest_mutex_t m) {
  if ((g_mock_rate_limit_fail == 3) | (g_mock_rate_limit_fail == 7))
    return C_REST_ERROR_GENERIC;
  return c_rest_mutex_lock(m);
}

c_rest_error_t crf_seam_rl_hashmap_put(struct c_rest_hashmap *m, const char *k,
                                       void *v) {
  if ((g_mock_rate_limit_fail == 4) | (g_mock_rate_limit_fail == 12))
    return C_REST_ERROR_GENERIC;
  return c_rest_hashmap_put(m, k, v);
}

c_rest_error_t crf_seam_rl_mutex_unlock(c_rest_mutex_t m) {
  unsigned int code = (unsigned int)g_mock_rate_limit_fail & 31U;
  if (((1UL << code) & ((1UL << 5) | (1UL << 6) | (1UL << 8) | (1UL << 10) |
                        (1UL << 12))) != 0UL) {
    return (c_rest_error_t)((int)C_REST_ERROR_GENERIC |
                            (int)c_rest_mutex_unlock(m));
  }
  return c_rest_mutex_unlock(m);
}

c_rest_error_t crf_seam_rl_hashmap_destroy(struct c_rest_hashmap *m,
                                           c_rest_error_t (*f)(void *)) {
  unsigned int code = (unsigned int)g_mock_rate_limit_fail & 31U;
  if (((1UL << code) & ((1UL << 2) | (1UL << 9) | (1UL << 10) | (1UL << 12))) !=
      0UL)
    return C_REST_ERROR_GENERIC;
  return c_rest_hashmap_destroy(m, f);
}

c_rest_error_t crf_seam_rl_mutex_destroy(c_rest_mutex_t m) {
  if (g_mock_rate_limit_fail == 11)
    return C_REST_ERROR_GENERIC;
  return c_rest_mutex_destroy(m);
}

c_rest_error_t crf_seam_tsq_cond_create(c_rest_cond_t *c) {
  if ((g_mock_tsq_fail == 1) | (g_mock_tsq_fail == 2))
    return C_REST_ERROR_GENERIC;
  return c_rest_cond_create(c);
}

c_rest_error_t crf_seam_tsq_mutex_destroy(c_rest_mutex_t m) {
  if ((g_mock_tsq_fail == 2) | (g_mock_tsq_fail == 13))
    return C_REST_ERROR_GENERIC;
  return c_rest_mutex_destroy(m);
}

c_rest_error_t crf_seam_tsq_mutex_lock(c_rest_mutex_t m) {
  if (g_mock_tsq_fail == 3)
    return C_REST_ERROR_GENERIC;
  return c_rest_mutex_lock(m);
}

c_rest_error_t crf_seam_tsq_cond_signal(c_rest_cond_t c) {
  unsigned int code = (unsigned int)g_mock_tsq_fail & 31U;
  if (((1UL << code) & ((1UL << 4) | (1UL << 5) | (1UL << 9) | (1UL << 10) |
                        (1UL << 16))) != 0UL)
    return C_REST_ERROR_GENERIC;
  return c_rest_cond_signal(c);
}

c_rest_error_t crf_seam_tsq_mutex_unlock(c_rest_mutex_t m) {
  unsigned int code = (unsigned int)g_mock_tsq_fail & 31U;
  if (((1UL << code) &
       ((1UL << 5) | (1UL << 6) | (1UL << 7) | (1UL << 8) | (1UL << 10) |
        (1UL << 11) | (1UL << 12) | (1UL << 14))) != 0UL)
    return C_REST_ERROR_GENERIC;
  return c_rest_mutex_unlock(m);
}

c_rest_error_t crf_seam_tsq_cond_wait(c_rest_cond_t c, c_rest_mutex_t m) {
  if (g_mock_tsq_fail == 7)
    return C_REST_ERROR_GENERIC;
  return c_rest_cond_wait(c, m);
}

c_rest_error_t crf_seam_tsq_cond_destroy(c_rest_cond_t c) {
  if ((g_mock_tsq_fail == 15) | (g_mock_tsq_fail == 16))
    return C_REST_ERROR_GENERIC;
  return c_rest_cond_destroy(c);
}

c_rest_error_t crf_seam_client_check(int code) {
  return (g_mock_client_fail == code) ? C_REST_ERROR_GENERIC : C_REST_OK;
}

c_rest_error_t crf_seam_client_destroy_hook(void) {
  return ((g_mock_client_fail == 11) | (g_mock_client_fail == 13))
             ? C_REST_ERROR_GENERIC
             : C_REST_OK;
}

c_rest_error_t crf_seam_client_res_free_hook(void) {
  return ((g_mock_client_fail == 10) | (g_mock_client_fail == 13))
             ? C_REST_ERROR_GENERIC
             : C_REST_OK;
}

c_rest_error_t crf_seam_client_hex_step(void) {
  if (g_mock_client_fail == 20)
    return C_REST_ERROR_GENERIC;
  if (g_mock_client_fail == 21)
    g_mock_client_fail = 20;
  return C_REST_OK;
}

c_rest_error_t crf_seam_client_url_decode_hook(void) {
  static int decode_count = 0;
  if ((g_mock_client_fail == 43) | (g_mock_client_fail == 45))
    return C_REST_ERROR_GENERIC;
  if ((g_mock_client_fail == 44) | (g_mock_client_fail == 46)) {
    decode_count++;
    if ((decode_count & 1) == 0)
      return C_REST_ERROR_GENERIC;
  }
  return C_REST_OK;
}

c_rest_error_t crf_seam_client_form_free_hook(void) {
  return ((g_mock_client_fail == 41) | (g_mock_client_fail == 43) |
          (g_mock_client_fail == 44))
             ? C_REST_ERROR_GENERIC
             : C_REST_OK;
}

c_rest_error_t crf_seam_client_headers_free_hook(void) {
  return ((g_mock_client_fail == 40) | (g_mock_client_fail == 42))
             ? C_REST_ERROR_GENERIC
             : C_REST_OK;
}

c_rest_error_t crf_seam_crypto_sha1_transform_step(void) {
  if ((unsigned int)(g_mock_crypto_fail - 20) <= 2U) {
    if (g_mock_crypto_fail == 20)
      return C_REST_ERROR_GENERIC;
    g_mock_crypto_fail--;
  }
  return C_REST_OK;
}

c_rest_error_t crf_seam_crypto_sha1_hook(void) {
  return (g_mock_crypto_fail == 8) ? C_REST_ERROR_GENERIC : C_REST_OK;
}

c_rest_error_t crf_seam_crypto_sha256_transform_step(void) {
  if ((unsigned int)(g_mock_crypto_fail - 30) <= 2U) {
    if (g_mock_crypto_fail == 30)
      return C_REST_ERROR_GENERIC;
    g_mock_crypto_fail--;
  }
  return C_REST_OK;
}

c_rest_error_t crf_seam_crypto_sha256_hook(void) {
  if (g_mock_crypto_fail == 10)
    return C_REST_ERROR_GENERIC;
  if (g_mock_crypto_fail == 11)
    g_mock_crypto_fail = 10;
  return C_REST_OK;
}

static NO_DISCARD c_rest_error_t crf_seam_crypto_check(int base) {
  if ((unsigned int)(g_mock_crypto_fail - (base + 1)) < 99U) {
    g_mock_crypto_fail--;
    if (g_mock_crypto_fail == base)
      return C_REST_ERROR_GENERIC;
  }
  return C_REST_OK;
}

c_rest_error_t crf_seam_crypto_sha256_call(const unsigned char *data,
                                           size_t len, unsigned char hash[32]) {
  c_rest_error_t rc = crf_seam_crypto_check(100);
  if (rc != C_REST_OK)
    return rc;
  return c_rest_sha256(data, len, hash);
}

c_rest_error_t crf_seam_crypto_b64url_enc(const unsigned char *src,
                                          size_t src_len, char *dst,
                                          size_t *dst_len) {
  c_rest_error_t rc = crf_seam_crypto_check(200);
  if (rc != C_REST_OK)
    return rc;
  return c_rest_base64url_encode(src, src_len, dst, dst_len);
}

c_rest_error_t crf_seam_crypto_b64_enc(const unsigned char *src, size_t src_len,
                                       char *dst, size_t *dst_len) {
  c_rest_error_t rc = crf_seam_crypto_check(300);
  if (rc != C_REST_OK)
    return rc;
  return c_rest_base64_encode(src, src_len, dst, dst_len);
}

c_rest_error_t crf_seam_crypto_b64_dec(const char *src, size_t src_len,
                                       unsigned char *dst, size_t *dst_len) {
  c_rest_error_t rc = crf_seam_crypto_check(400);
  if (rc != C_REST_OK)
    return rc;
  return c_rest_base64_decode(src, src_len, dst, dst_len);
}

c_rest_error_t crf_seam_crypto_b64url_dec(const char *src, size_t src_len,
                                          unsigned char *dst, size_t *dst_len) {
  c_rest_error_t rc = crf_seam_crypto_check(500);
  if (rc != C_REST_OK)
    return rc;
  return c_rest_base64url_decode(src, src_len, dst, dst_len);
}

c_rest_error_t crf_seam_crypto_rand_bytes(unsigned char *buf, size_t len) {
  if ((g_mock_crypto_fail == 3) | (g_mock_crypto_fail == 4))
    return C_REST_ERROR_GENERIC;
  return c_rest_rand_bytes(buf, len);
}

c_rest_error_t crf_seam_crypto_pbkdf2(const unsigned char *password,
                                      size_t password_len,
                                      const unsigned char *salt,
                                      size_t salt_len, unsigned long iterations,
                                      size_t dk_len, unsigned char *out_key) {
  if ((g_mock_crypto_fail == 2) |
      ((unsigned int)(g_mock_crypto_fail - 5) <= 2U))
    return C_REST_ERROR_GENERIC;
  return c_rest_pbkdf2_hmac_sha256(password, password_len, salt, salt_len,
                                   iterations, dk_len, out_key);
}
