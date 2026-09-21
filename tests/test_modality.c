#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "c_rest_endian.h"
#include "test_protos.h"
#include "c_rest_modality.h"
#include "c_rest_router.h"
#include "c_rest_platform.h"
#include "c_rest_response.h"
#include "c_rest_testing_mocks.h"
#include <stdio.h>
#include <string.h>
#if defined(_WIN32)
#include <winsock2.h>
#ifndef AF_UNIX
#define AF_UNIX 1
#endif
#if defined(__GNUC__) || defined(__clang__)
__attribute__((unused))
#endif
static int socketpair(int domain, int type, int protocol, int sv[2]) {
  (void)domain;
  (void)type;
  (void)protocol;
  sv[0] = -1;
  sv[1] = -1;
  return -1;
}
#else
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#endif
/* clang-format on */

#ifdef C_REST_TESTING_MALLOC_HOOK
#endif

extern int g_accept_calls;

static void my_dummy_free(void *ptr) { (void)ptr; }

static c_rest_error_t my_mock_logger_err_cb_internal(const char *msg) {
  (void)msg;
  return C_REST_ERROR_GENERIC;
}

static c_rest_error_t my_mock_logger_ok_cb_internal(const char *msg) {
  (void)msg;
  return C_REST_OK;
}

static c_rest_error_t dummy_modality_worker_fn(void *arg) {
  (void)arg;
  return C_REST_OK;
}

struct my_sync_state {
  c_rest_socket_t server_sock;
  int is_running;
  c_rest_thread_t *workers;
  int worker_count;
};

struct my_c_rest_event_loop {
  int is_running;
  ptrdiff_t poll_backend;
};

struct my_async_state {
  c_rest_socket_t server_sock;
  struct my_c_rest_event_loop *evloop;
};

static void *my_mock_null_malloc(size_t s) {
  (void)s;
  return NULL;
}

static void test_modality_simple(void) {
  struct c_rest_context ctx;
  struct my_sync_state sync_st;
  struct my_sync_state single_st;
  struct my_sync_state multi_st;
  struct my_async_state async_st;
  struct my_c_rest_event_loop dummy_evloop;

  memset(&ctx, 0, sizeof(ctx));
  memset(&sync_st, 0, sizeof(sync_st));
  memset(&single_st, 0, sizeof(single_st));
  memset(&multi_st, 0, sizeof(multi_st));
  memset(&async_st, 0, sizeof(async_st));
  memset(&dummy_evloop, 0, sizeof(dummy_evloop));

  ctx.allocator.malloc_cb = malloc;
  ctx.allocator.free_cb = my_dummy_free;
  ctx.listen_address = "127.0.0.1";

  /* 2. Logger failures for init */
  ctx.logger.log_cb = my_mock_logger_err_cb_internal;
  sync_vtable.init(&ctx);
  single_thread_vtable.init(&ctx);
  multi_thread_vtable.init(&ctx);
  async_vtable.init(&ctx);

  /* 3. Null ctx */
  sync_vtable.init(NULL);
  sync_vtable.run(NULL);
  sync_vtable.stop(NULL);
  sync_vtable.destroy(NULL);

  single_thread_vtable.init(NULL);
  single_thread_vtable.run(NULL);
  single_thread_vtable.stop(NULL);
  single_thread_vtable.destroy(NULL);

  multi_thread_vtable.init(NULL);
  multi_thread_vtable.run(NULL);
  multi_thread_vtable.stop(NULL);
  multi_thread_vtable.destroy(NULL);

  async_vtable.init(NULL);
  async_vtable.run(NULL);
  async_vtable.destroy(NULL);

  /* 4. Trigger accept failure in sync_run by passing a 9999 socket */
  ctx.logger.log_cb = NULL;
  ctx.internal_state = &sync_st;
  sync_st.server_sock = (c_rest_socket_t)9999;
  sync_st.is_running = 1;
  sync_vtable.run(&ctx);

  /* Trigger bind failure in single/multi/async run */
  ctx.listen_port = 1;
  ctx.listen_address = "invalid_address_for_test";

  ctx.internal_state = &single_st;
  single_st.server_sock = C_REST_INVALID_SOCKET;
  single_thread_vtable.run(&ctx);

  ctx.internal_state = &multi_st;
  multi_st.server_sock = (c_rest_socket_t)1;
  {
    c_rest_thread_t workers_arr[3];
    c_rest_thread_create(&workers_arr[0], dummy_modality_worker_fn, NULL);
    workers_arr[1] = (c_rest_thread_t)0;
    c_rest_thread_create(&workers_arr[2], dummy_modality_worker_fn, NULL);
    multi_st.workers = workers_arr;
    multi_st.worker_count = 3;
    g_mock_socket_fail = 8;
    multi_thread_vtable.run(&ctx);
    g_mock_socket_fail = 0;

    workers_arr[0] = (c_rest_thread_t)-1;
    multi_st.worker_count = 1;
    g_mock_socket_fail = 8;
    multi_thread_vtable.run(&ctx);
    g_mock_socket_fail = 0;
  }

  ctx.internal_state = &async_st;
  async_st.server_sock = C_REST_INVALID_SOCKET;
  async_st.evloop = &dummy_evloop;
  async_vtable.run(&ctx);

  /* 5. Trigger logger failure in run */
  ctx.logger.log_cb = my_mock_logger_err_cb_internal;

  ctx.internal_state = &sync_st;
  sync_st.is_running = 1;
  sync_st.server_sock = (c_rest_socket_t)9999;
  sync_vtable.run(&ctx);

  ctx.internal_state = &single_st;
  single_st.is_running = 1;
  single_st.server_sock = (c_rest_socket_t)9999;
  single_thread_vtable.run(&ctx);

  ctx.internal_state = &multi_st;
  multi_st.is_running = 1;
  multi_st.server_sock = (c_rest_socket_t)9999;
  multi_thread_vtable.run(&ctx);

  ctx.internal_state = &async_st;
  async_st.evloop = &dummy_evloop;
  async_st.server_sock = (c_rest_socket_t)9999;
  async_vtable.run(&ctx);

  /* 6. Trigger logger failure in stop */
  ctx.internal_state = &sync_st;
  sync_vtable.stop(&ctx);

  ctx.internal_state = &single_st;
  single_thread_vtable.stop(&ctx);

  ctx.internal_state = &multi_st;
  {
    c_rest_thread_t workers_arr[3];
    c_rest_thread_create(&workers_arr[0], dummy_modality_worker_fn, NULL);
    workers_arr[1] = (c_rest_thread_t)0;
    c_rest_thread_create(&workers_arr[2], dummy_modality_worker_fn, NULL);
    multi_st.workers = workers_arr;
    multi_st.worker_count = 3;
    multi_thread_vtable.stop(&ctx);

    workers_arr[0] = (c_rest_thread_t)-1;
    multi_st.worker_count = 1;
    multi_thread_vtable.stop(&ctx);

    multi_st.workers = NULL;
    multi_st.worker_count = 0;
    multi_st.server_sock = (c_rest_socket_t)9999;
    multi_thread_vtable.stop(&ctx);
  }

  /* 7. Trigger socket close failure in destroy */
  ctx.logger.log_cb = NULL;

  ctx.internal_state = &sync_st;
  sync_st.server_sock = (c_rest_socket_t)9999;
  sync_vtable.destroy(&ctx);

  ctx.internal_state = &single_st;
  single_st.server_sock = (c_rest_socket_t)9999;
  single_thread_vtable.destroy(&ctx);

  ctx.internal_state = &multi_st;
  multi_st.server_sock = (c_rest_socket_t)9999;
  multi_thread_vtable.destroy(&ctx);

  ctx.internal_state = &async_st;
  async_st.evloop = &dummy_evloop;
  async_st.server_sock = (c_rest_socket_t)9999;
  async_vtable.destroy(&ctx);

  /* 8. Trigger logger failure in destroy */
  ctx.logger.log_cb = my_mock_logger_err_cb_internal;

  ctx.internal_state = &sync_st;
  sync_st.server_sock = C_REST_INVALID_SOCKET;
  sync_vtable.destroy(&ctx);

  ctx.internal_state = &single_st;
  single_st.server_sock = C_REST_INVALID_SOCKET;
  single_thread_vtable.destroy(&ctx);

  ctx.internal_state = &multi_st;
  multi_st.server_sock = C_REST_INVALID_SOCKET;
  {
    c_rest_thread_t workers_arr[3];
    c_rest_thread_create(&workers_arr[0], dummy_modality_worker_fn, NULL);
    workers_arr[1] = (c_rest_thread_t)0;
    c_rest_thread_create(&workers_arr[2], dummy_modality_worker_fn, NULL);
    multi_st.workers = workers_arr;
    multi_st.worker_count = 3;
    multi_thread_vtable.destroy(&ctx);

    workers_arr[0] = (c_rest_thread_t)-1;
    multi_st.worker_count = 1;
    ctx.internal_state = &multi_st;
    multi_thread_vtable.destroy(&ctx);

    ctx.logger.log_cb = NULL;
    multi_st.workers = NULL;
    multi_st.worker_count = 0;
    multi_thread_vtable.destroy(&ctx);
  }

  ctx.internal_state = &async_st;
  async_st.server_sock = C_REST_INVALID_SOCKET;
  async_st.evloop = &dummy_evloop;
  async_vtable.destroy(&ctx);

  /* 9. Trigger logger SUCCESS in init, run, stop, destroy */
  ctx.logger.log_cb = my_mock_logger_ok_cb_internal;

  sync_vtable.init(&ctx);

  ctx.internal_state = &sync_st;
  sync_st.server_sock = (c_rest_socket_t)9999;
  sync_st.is_running = 1;
  sync_vtable.run(&ctx);

  sync_vtable.stop(&ctx);

  sync_st.server_sock = C_REST_INVALID_SOCKET;
  sync_vtable.destroy(&ctx);
}

static c_rest_error_t mock_logger_cb(const char *msg) {
  (void)msg;
  return C_REST_OK;
}

static c_rest_error_t mock_logger_err_cb(const char *msg) {
  (void)msg;
  return C_REST_ERROR_GENERIC;
}

#ifdef C_REST_TESTING_MALLOC_HOOK
static int g_malloc_fail_count = -1;
static void *hook_malloc_modality(size_t size) {
  if (g_malloc_fail_count == 0) {
    g_malloc_fail_count = -1;
    return NULL;
  }
  if (g_malloc_fail_count > 0) {
    g_malloc_fail_count--;
  }
  return malloc(size);
}

static void *hook_realloc_modality(void *ptr, size_t size) {
  if (g_malloc_fail_count == 0) {
    g_malloc_fail_count = -1;
    return NULL;
  }
  if (g_malloc_fail_count > 0)
    g_malloc_fail_count--;
  return realloc(ptr, size);
}

static void *hook_calloc_modality(size_t count, size_t size) {
  if (g_malloc_fail_count == 0)
    return NULL;
  if (g_malloc_fail_count > 0)
    g_malloc_fail_count--;
  return calloc(count, size);
}

static char *hook_strdup_modality(const char *str) {
  size_t len;
  char *dup;
  if (g_malloc_fail_count == 0)
    return NULL;
  if (g_malloc_fail_count > 0)
    g_malloc_fail_count--;
  len = strlen(str) + 1;
  dup = (char *)malloc(len);
  memcpy(dup, str, len);
  return dup;
}
#endif

#if !defined(__EMSCRIPTEN__)
struct test_client_args {
  struct c_rest_context *ctx;
  int port;
};
#if (defined(__unix__) || defined(__APPLE__))
static void my_sigalrm(int sig) { (void)sig; }
#endif
static c_rest_error_t test_client_thread(void *arg) {
  struct test_client_args *args = (struct test_client_args *)arg;
  c_rest_socket_t sock = C_REST_INVALID_SOCKET;
  struct sockaddr_in srv_addr;
  int retries = (args->port == 1) ? 2 : 50;
  int initial_fail = (args->port == 1) ? 0 : 1;

  c_rest_platform_init();
#if defined(_WIN32)
  Sleep(100);
#else
  {
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000;
    select(0, NULL, NULL, NULL, &tv);
  }
#endif

  while (retries-- > 0) {
    unsigned short target_port = (unsigned short)args->port;
    sock = (c_rest_socket_t)socket(AF_INET, SOCK_STREAM, 0);
    memset(&srv_addr, 0, sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    if (initial_fail) {
      target_port = 1;
      initial_fail = 0;
    }
    c_rest_htons(target_port, &srv_addr.sin_port);
    srv_addr.sin_addr.s_addr = htonl(0x7F000001);
#if defined(_WIN32)
    if (connect((SOCKET)sock, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) ==
        0) {
#else
    if (connect((int)sock, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) ==
        0) {
#endif
      const char *req =
          "GET / HTTP/1.1\r\nHost: loc\r\nConnection: close\r\n\r\n";
#if defined(_WIN32)
      send((SOCKET)sock, req, (int)strlen(req), 0);
#else
      send((int)sock, req, strlen(req), 0);
#endif
#if defined(_WIN32)
      Sleep(50);
#else
      {
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 50000;
        select(0, NULL, NULL, NULL, &tv);
      }
#endif
      c_rest_stop(args->ctx);
#if defined(_WIN32)
      closesocket((SOCKET)sock);
#else
      close((int)sock);
#endif
      {
        int j;
        for (j = 0; j < 8; j++) {
          sock = (c_rest_socket_t)socket(AF_INET, SOCK_STREAM, 0);
#if defined(_WIN32)
          connect((SOCKET)sock, (struct sockaddr *)&srv_addr, sizeof(srv_addr));
          closesocket((SOCKET)sock);
#else
          connect((int)sock, (struct sockaddr *)&srv_addr, sizeof(srv_addr));
          close((int)sock);
#endif
        }
      }
      break;
    }
#if defined(_WIN32)
    closesocket((SOCKET)sock);
#else
    close((int)sock);
#endif
#if defined(_WIN32)
    Sleep(10);
#else
    {
      struct timeval tv;
      tv.tv_sec = 0;
      tv.tv_usec = 10000;
      select(0, NULL, NULL, NULL, &tv);
    }
#endif
  }
  c_rest_platform_cleanup();
  return C_REST_OK;
}
#endif

static int g_async_logger_calls = 0;
static c_rest_error_t mock_logger_fail_on_second(const char *msg) {
  (void)msg;
  g_async_logger_calls++;
  if (g_async_logger_calls == 2)
    return C_REST_ERROR_GENERIC;
  return C_REST_OK;
}

static c_rest_error_t my_dummy_handler_headers_sent(struct c_rest_request *req,
                                                    struct c_rest_response *res,
                                                    void *user_data) {
  (void)req;
  (void)user_data;
  res->headers_sent = 1;
  return C_REST_OK;
}

static c_rest_error_t my_dummy_handler(struct c_rest_request *req,
                                       struct c_rest_response *res,
                                       void *user_data) {
  (void)req;
  (void)user_data;
  c_rest_response_set_status(res, 200);
  return c_rest_response_send(res);
}

int test_modality(void) {
  int failed = 0;
  struct c_rest_context *ctx = NULL;
  struct c_rest_router *router = NULL;
#if !defined(__EMSCRIPTEN__) && !defined(CDD_DOS)
  enum c_rest_modality_type modalities[] = {C_REST_MODALITY_SYNC,
                                            C_REST_MODALITY_ASYNC,
                                            C_REST_MODALITY_MULTI_THREAD,
                                            C_REST_MODALITY_SINGLE_THREAD,
                                            C_REST_MODALITY_MULTI_PROCESS,
                                            C_REST_MODALITY_GREENTHREAD,
                                            C_REST_MODALITY_MESSAGE_PASSING,
                                            C_REST_MODALITY_SINGLE_PROCESS};
  int num_modalities = 8;
#else
  enum c_rest_modality_type modalities[] = {C_REST_MODALITY_SYNC,
                                            C_REST_MODALITY_ASYNC,
                                            C_REST_MODALITY_SINGLE_THREAD,
                                            C_REST_MODALITY_GREENTHREAD,
                                            C_REST_MODALITY_MESSAGE_PASSING,
                                            C_REST_MODALITY_SINGLE_PROCESS};
  int num_modalities = 6;
#endif
  int i;
  c_rest_error_t rc;
  c_rest_socket_t client_sock = C_REST_INVALID_SOCKET;
  c_rest_socket_t accepted_sock = C_REST_INVALID_SOCKET;

  (void)client_sock;
  (void)accepted_sock;

#ifdef C_REST_TESTING_MALLOC_HOOK
  g_crf_malloc_hook = hook_malloc_modality;
  g_crf_realloc_hook = hook_realloc_modality;
  g_crf_calloc_hook = hook_calloc_modality;
  g_crf_strdup_hook = hook_strdup_modality;
#endif

  c_rest_router_init(&router);
  c_rest_router_add(router, "POST", "/test", my_dummy_handler, NULL);
  c_rest_router_add(router, "POST", "/headers_sent",
                    my_dummy_handler_headers_sent, NULL);

  /* Test c_rest_handle_connection parsing coverage */
  c_rest_init(C_REST_MODALITY_SINGLE_PROCESS, &ctx);
  {
    ctx->router = router;
#if (defined(__unix__) || defined(__APPLE__)) && !defined(__EMSCRIPTEN__)
    {
      int fds[2];
      const char *req = "POST /test HTTP/1.1\r\nHost: loc\r\nContent-Length: "
                        "5\r\n\r\nhello";
      size_t wr = 0;

      socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
      accepted_sock = (c_rest_socket_t)fds[0];
      client_sock = (c_rest_socket_t)fds[1];
      c_rest_socket_send(client_sock, req, strlen(req), &wr);
      c_rest_socket_close(client_sock);
      c_rest_handle_connection(ctx, accepted_sock);
      c_rest_socket_close(accepted_sock);

      /* Test headers_sent handler branch */
      socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
      accepted_sock = (c_rest_socket_t)fds[0];
      client_sock = (c_rest_socket_t)fds[1];
      {
        const char *req_hs = "POST /headers_sent HTTP/1.1\r\nHost: "
                             "loc\r\nContent-Length: 0\r\n\r\n";
        c_rest_socket_send(client_sock, req_hs, strlen(req_hs), &wr);
        c_rest_socket_close(client_sock);
        c_rest_handle_connection(ctx, accepted_sock);
        c_rest_socket_close(accepted_sock);
      }

      /* Test parser vtable fail */
      socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
      accepted_sock = (c_rest_socket_t)fds[0];
      client_sock = (c_rest_socket_t)fds[1];
      c_rest_socket_send(client_sock, req, strlen(req), &wr);
      c_rest_socket_close(client_sock);
      g_mock_parser_vtable_fail = 1;
      c_rest_handle_connection(ctx, accepted_sock);
      g_mock_parser_vtable_fail = 0;
      c_rest_socket_close(accepted_sock);

      /* Test req_cleanup fail */
      socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
      accepted_sock = (c_rest_socket_t)fds[0];
      client_sock = (c_rest_socket_t)fds[1];
      c_rest_socket_send(client_sock, req, strlen(req), &wr);
      c_rest_socket_close(client_sock);
      g_mock_req_cleanup_fail = 1;
      c_rest_handle_connection(ctx, accepted_sock);
      g_mock_req_cleanup_fail = 0;
      c_rest_socket_close(accepted_sock);

      /* Test res_cleanup fail */
      socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
      accepted_sock = (c_rest_socket_t)fds[0];
      client_sock = (c_rest_socket_t)fds[1];
      c_rest_socket_send(client_sock, req, strlen(req), &wr);
      c_rest_socket_close(client_sock);
      g_mock_res_cleanup_fail = 1;
      c_rest_handle_connection(ctx, accepted_sock);
      g_mock_res_cleanup_fail = 0;
      c_rest_socket_close(accepted_sock);

      /* Test parser should_keep_alive fail */
      socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
      accepted_sock = (c_rest_socket_t)fds[0];
      client_sock = (c_rest_socket_t)fds[1];
      c_rest_socket_send(client_sock, req, strlen(req), &wr);
      c_rest_socket_close(client_sock);
      g_mock_parser_should_keep_alive_fail = 1;
      c_rest_handle_connection(ctx, accepted_sock);
      g_mock_parser_should_keep_alive_fail = 0;
      c_rest_socket_close(accepted_sock);

      /* Test parser destroy fail */
      socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
      accepted_sock = (c_rest_socket_t)fds[0];
      client_sock = (c_rest_socket_t)fds[1];
      c_rest_socket_send(client_sock, req, strlen(req), &wr);
      c_rest_socket_close(client_sock);
      g_mock_parser_destroy_fail = 1;
      c_rest_handle_connection(ctx, accepted_sock);
      g_mock_parser_destroy_fail = 0;
      c_rest_socket_close(accepted_sock);

      socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
      accepted_sock = (c_rest_socket_t)fds[0];
      client_sock = (c_rest_socket_t)fds[1];
      c_rest_socket_send(client_sock, req, strlen(req), &wr);
      c_rest_socket_close(client_sock);
      ctx->router = NULL;
      c_rest_handle_connection(ctx, accepted_sock);
      ctx->router = router;
      c_rest_socket_close(accepted_sock);

      socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
      accepted_sock = (c_rest_socket_t)fds[0];
      client_sock = (c_rest_socket_t)fds[1];
      c_rest_socket_send(client_sock, req, strlen(req), &wr);
      c_rest_socket_close(client_sock);
      ctx->tls_ctx = (void *)1;
      c_rest_handle_connection(ctx, accepted_sock);
      ctx->tls_ctx = NULL;
      c_rest_socket_close(accepted_sock);

      /* Test c_rest_tls_close fail in c_rest_handle_connection */
      socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
      accepted_sock = (c_rest_socket_t)fds[0];
      client_sock = (c_rest_socket_t)fds[1];
      c_rest_socket_send(client_sock, req, strlen(req), &wr);
      c_rest_socket_close(client_sock);
      ctx->tls_ctx = (void *)1;
      g_mock_tls_fail = 2;
      c_rest_handle_connection(ctx, accepted_sock);
      g_mock_tls_fail = 0;
      ctx->tls_ctx = NULL;
      c_rest_socket_close(accepted_sock);
    }
#endif
    c_rest_destroy(ctx);
  }

  /* Test c_rest_handle_connection with OOM in parsing callbacks */
  c_rest_init(C_REST_MODALITY_SINGLE_PROCESS, &ctx);
  {
    ctx->router = router;
#if (defined(__unix__) || defined(__APPLE__)) && !defined(__EMSCRIPTEN__)
#ifdef C_REST_TESTING_MALLOC_HOOK
    {
      int oom_idx;
      for (oom_idx = 0; oom_idx < 150; oom_idx++) {
        int fds[2];
        socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
        accepted_sock = (c_rest_socket_t)fds[0];
        client_sock = (c_rest_socket_t)fds[1];
        {
          const char *req = "POST /test?foo=bar HTTP/1.1\r\nHost: "
                            "loc\r\nContent-Length: 5\r\n\r\nhello";
          size_t wr = 0;
          c_rest_socket_send(client_sock, req, strlen(req), &wr);
          c_rest_socket_close(client_sock);
          g_malloc_fail_count = oom_idx;
          c_rest_handle_connection(ctx, accepted_sock);
          c_rest_socket_close(accepted_sock);
        }
      }
      g_malloc_fail_count = -1;
      g_crf_realloc_hook = NULL;
      g_crf_calloc_hook = NULL;
      g_crf_strdup_hook = NULL;

      {
        int fds[2];
        socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
        accepted_sock = (c_rest_socket_t)fds[0];
        client_sock = (c_rest_socket_t)fds[1];
        {
          const char *req = "POST /test?foo=bar HTTP/1.1\r\nHost: "
                            "loc\r\nContent-Length: 5\r\n\r\nhello";
          size_t wr = 0;
          c_rest_socket_send(client_sock, req, strlen(req), &wr);
          c_rest_socket_close(client_sock);
          c_rest_handle_connection(ctx, accepted_sock);
          c_rest_socket_close(accepted_sock);
        }
      }
    }
#endif
#endif
    c_rest_destroy(ctx);
  }

  /* Test c_rest_handle_connection with TLS coverage */
  c_rest_init(C_REST_MODALITY_SINGLE_PROCESS, &ctx);
  {
    ctx->router = router;
#if (defined(__unix__) || defined(__APPLE__)) && !defined(__EMSCRIPTEN__)
    {
      int fds[2];
      socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
      accepted_sock = (c_rest_socket_t)fds[0];
      client_sock = (c_rest_socket_t)fds[1];

      /* Make c_rest_tls_accept fail */
      ctx->tls_ctx = (struct c_rest_tls_context *)1;

#ifdef C_REST_TESTING_MALLOC_HOOK
      g_malloc_fail_count = 0;
      g_crf_malloc_hook = hook_malloc_modality;
      c_rest_handle_connection(ctx, accepted_sock);
      g_malloc_fail_count = -1;
      g_crf_malloc_hook = NULL;
#endif

      c_rest_handle_connection(ctx, accepted_sock);

      ctx->tls_ctx = NULL;
      c_rest_socket_close(client_sock);
      c_rest_socket_close(accepted_sock);
    }

    {
      int fds[2];
      const char *req = "INVALID REQUEST\r\n\r\n";
      size_t wr = 0;
      socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
      accepted_sock = (c_rest_socket_t)fds[0];
      client_sock = (c_rest_socket_t)fds[1];

      c_rest_socket_send(client_sock, req, strlen(req), &wr);
      c_rest_socket_close(client_sock);
      c_rest_handle_connection(ctx, accepted_sock);
      c_rest_socket_close(accepted_sock);
    }
#endif

    c_rest_destroy(ctx);
  }

#define RECORD_FAIL()                                                          \
  do {                                                                         \
    printf("MODALITY FAIL AT LINE %d\n", __LINE__);                            \
    failed++;                                                                  \
  } while (0)

  /* Test invalid c_rest_handle_connection */
  c_rest_handle_connection(NULL, C_REST_INVALID_SOCKET);

  /* Modality context error branches */
  {
    struct c_rest_context mctx;
    struct c_rest_logger lgr;
    memset(&mctx, 0, sizeof(mctx));
    memset(&lgr, 0, sizeof(lgr));
    lgr.log_cb = my_mock_logger_err_cb_internal;
    mctx.logger = lgr;
    mctx.allocator.free_cb = my_dummy_free;

    /* 1. dummy_init log fail */
    c_rest_init(C_REST_MODALITY_SINGLE_PROCESS, &ctx);
    mctx.logger = lgr;
    ctx->vtable->init(&mctx);
    /* 1b. dummy_init log success */
    mctx.logger.log_cb = my_mock_logger_ok_cb_internal;
    ctx->vtable->init(&mctx);
    c_rest_destroy(ctx);
    ctx = NULL;

    /* 2. db pool init fail with err logger */
    mctx.logger = lgr;
    g_mock_orm_init_fail = 1;
    mctx.db_config.connection_string = "invalid://";
    c_rest_run(&mctx);

    /* 2b. db pool init fail with ok logger */
    mctx.logger.log_cb = my_mock_logger_ok_cb_internal;
    c_rest_run(&mctx);

    /* 3. db pool init fail without logger */
    mctx.logger.log_cb = NULL;
    c_rest_run(&mctx);
    g_mock_orm_init_fail = 0;
    mctx.db_config.connection_string = NULL;
    mctx.logger = lgr;

    /* 4. run with no run callback and logger */
    mctx.vtable = NULL;
    c_rest_run(&mctx);

    /* 5. stop with no stop callback and logger */
    c_rest_stop(&mctx);

    /* 6. destroy with db_pool and failing logger */
    mctx.db_pool = (void *)1;
    c_rest_destroy(&mctx);

    /* 6b. destroy with db_pool and ok logger */
    mctx.db_pool = (void *)1;
    mctx.logger.log_cb = my_mock_logger_ok_cb_internal;
    c_rest_destroy(&mctx);
    mctx.db_pool = NULL;

    /* 7. destroy with db_pool and failing orm_cleanup */
    mctx.db_pool = (void *)1;
    mctx.logger.log_cb = NULL;
    g_mock_orm_cleanup_fail = 1;
    c_rest_destroy(&mctx);
    g_mock_orm_cleanup_fail = 0;

    /* 8. destroy with platform_cleanup fail */
    mctx.db_pool = NULL;
    g_mock_platform_cleanup_fail = 1;
    c_rest_destroy(&mctx);
    g_mock_platform_cleanup_fail = 0;
  }

  /* Test invalid modality */
  failed += (c_rest_init((enum c_rest_modality_type)999, &ctx) == C_REST_OK);

#ifdef C_REST_TESTING_MALLOC_HOOK
  /* Exercise hook_calloc_modality and hook_strdup_modality branches */
  {
    char *s;
    void *p;

    g_malloc_fail_count = 0;
    failed += (hook_calloc_modality(1, 1) != NULL);
    g_malloc_fail_count = 0;
    failed += (hook_strdup_modality("t") != NULL);

    g_malloc_fail_count = 1;
    p = hook_calloc_modality(1, 1);
    failed += (p == NULL);
    free(p);

    g_malloc_fail_count = 1;
    s = hook_strdup_modality("t");
    failed += (s == NULL);
    free(s);

    g_malloc_fail_count = -1;
    p = hook_calloc_modality(1, 1);
    failed += (p == NULL);
    free(p);

    g_malloc_fail_count = -1;
    s = hook_strdup_modality("t");
    failed += (s == NULL);
    free(s);
  }
#endif

#if (defined(__unix__) || defined(__APPLE__)) && !defined(__EMSCRIPTEN__)
  my_sigalrm(0);
#endif

  for (i = 0; i < num_modalities; i++) {
    enum c_rest_modality_type mod = modalities[i];
#ifdef C_REST_TESTING_MALLOC_HOOK
    g_crf_malloc_hook = hook_malloc_modality;
    g_crf_realloc_hook = hook_realloc_modality;
    g_crf_calloc_hook = hook_calloc_modality;
    g_crf_strdup_hook = hook_strdup_modality;
#endif

    printf("Testing modality %d\n", mod);

    /* Null checks */
    failed += (c_rest_init(mod, NULL) == C_REST_OK);

#ifdef C_REST_TESTING_MALLOC_HOOK
    /* OOM on c_rest_init itself */
    g_malloc_fail_count = 0;
    failed += (c_rest_init(mod, &ctx) == C_REST_OK);

    if (mod != C_REST_MODALITY_SINGLE_PROCESS) {
      g_malloc_fail_count = 1;
      failed += (c_rest_init(mod, &ctx) == C_REST_OK);
      g_malloc_fail_count = 2;
      rc = c_rest_init(mod, &ctx);
    }
#endif
    g_malloc_fail_count = -1;

    rc = c_rest_init(mod, &ctx);
    failed += (rc != C_REST_OK);

    /* Invalid run */
    failed += (c_rest_run(NULL) == C_REST_OK);

    /* Invalid stop */
    failed += (c_rest_stop(NULL) == C_REST_OK);

    /* Stop (should be safe to call before run, or just set flag) */
    c_rest_stop(ctx);

    /* Run with invalid host to fail bind immediately and not hang */
    ctx->listen_address = "invalid_address_for_test";
    ctx->listen_port = 1;
    ctx->logger.log_cb = mock_logger_cb;
    c_rest_run(ctx);

#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
    /* Run with multiplatform mock */
    {
      g_accept_calls = 0;
      ctx->listen_address = "127.0.0.1";
      ctx->listen_port = 8080;
      ctx->cm_env = (cm_env_t)1;
      c_rest_run(ctx);
    }
#endif

    /* Destroy invalid */
    failed += (c_rest_destroy(NULL) == C_REST_OK);

    /* Valid destroy */
    rc = c_rest_destroy(ctx);
    failed += (rc != C_REST_OK);

    /* Test with logger error on init */
    c_rest_init(mod, &ctx);
    ctx->logger.log_cb = mock_logger_err_cb;
    c_rest_destroy(ctx);
  }

  /* Specifically test dummy modality coverage with logger */
  c_rest_init(C_REST_MODALITY_SINGLE_PROCESS, &ctx);
  ctx->logger.log_cb = mock_logger_cb;
  /* run should cover dummy_run */
  c_rest_run(ctx);
  /* stop should cover dummy_stop */
  c_rest_stop(ctx);
  /* destroy covers dummy_destroy */
  c_rest_destroy(ctx);

  /* Test db config in c_rest_run */
  c_rest_init(C_REST_MODALITY_SINGLE_PROCESS, &ctx);
  ctx->logger.log_cb = mock_logger_cb;
  ctx->db_config.connection_string = "sqlite://:memory:";
  c_rest_run(ctx);

  ctx->vtable = NULL;
  ctx->logger.log_cb = mock_logger_cb;
  c_rest_run(ctx);
  ctx->logger.log_cb = NULL;
  c_rest_run(ctx);

  ctx->logger.log_cb = mock_logger_err_cb;
  c_rest_run(ctx);

  /* Induce c_rest_orm_init failure with logger NULL */
  g_async_logger_calls = 0;
  ctx->db_config.connection_string = "invalid_url://";
  g_mock_orm_init_fail = 1;
  ctx->logger.log_cb = NULL;
  c_rest_run(ctx);

  /* Induce c_rest_orm_init failure with mock_logger_fail_on_second */
  g_async_logger_calls = 0;
  ctx->logger.log_cb = mock_logger_fail_on_second;
  c_rest_run(ctx);
  g_mock_orm_init_fail = 0;

  ctx->db_config.connection_string = NULL;
  ctx->logger.log_cb = mock_logger_err_cb;
  c_rest_destroy(ctx);

  /* Test logger failure on dummy destroy/stop */
  c_rest_init(C_REST_MODALITY_SINGLE_PROCESS, &ctx);
  ctx->logger.log_cb = mock_logger_err_cb;
  c_rest_stop(ctx);
  ctx->vtable = NULL;
  ctx->logger.log_cb = mock_logger_cb;
  c_rest_run(ctx);
  c_rest_stop(ctx);
  ctx->logger.log_cb = mock_logger_cb;
  c_rest_run(ctx);
  c_rest_stop(ctx);
  ctx->allocator.free_cb = NULL;
  c_rest_destroy(ctx);

  {
    struct c_rest_modality_vtable null_vt = {0};
    c_rest_init(C_REST_MODALITY_SINGLE_PROCESS, &ctx);
    ctx->vtable = &null_vt;
    c_rest_run(ctx);
    c_rest_stop(ctx);
    c_rest_destroy(ctx);
  }

  /* Test dummy init logger failure */
  ctx = NULL;
  {
    struct c_rest_logger err_log;
    err_log.log_cb = mock_logger_err_cb;
    c_rest_init(C_REST_MODALITY_SINGLE_PROCESS, &ctx);
    ctx->logger = err_log;
    ctx->vtable->init(ctx);
    c_rest_destroy(ctx);
    ctx = NULL;
  }
  c_rest_set_router(NULL, NULL);

  /* Test get_vtable null out_vtable coverage indirectly (would need direct
   * call, but we can't. We can test c_rest_init null context) */
  rc = c_rest_init(C_REST_MODALITY_SINGLE_PROCESS, NULL);
  failed += (rc != C_REST_ERROR_GENERIC);

#ifdef C_REST_TESTING_MALLOC_HOOK
  g_crf_malloc_hook = NULL;
#endif

#ifdef C_REST_TESTING_MALLOC_HOOK
  g_crf_malloc_hook = NULL;
  g_crf_realloc_hook = NULL;
  g_crf_calloc_hook = NULL;
  g_crf_strdup_hook = NULL;
#endif
  /* async modality direct tests */
  {

    struct c_rest_context dummy_ctx_gt;
#if (defined(__unix__) || defined(__APPLE__)) && !defined(__EMSCRIPTEN__)
    c_rest_socket_t valid_sock =
        (c_rest_socket_t)socket(AF_INET, SOCK_STREAM, 0);
#else
    c_rest_socket_t valid_sock =
        (c_rest_socket_t)socket(AF_INET, SOCK_STREAM, 0);
#endif

    memset(&dummy_ctx_gt, 0, sizeof(dummy_ctx_gt));
    dummy_ctx_gt.allocator.malloc_cb = malloc;
    dummy_ctx_gt.allocator.free_cb = free;

    /* async_init NULL */
    async_vtable.init(NULL);

    /* async_init OOM state */
#ifdef C_REST_TESTING_MALLOC_HOOK
    g_crf_malloc_hook = hook_malloc_modality;
    g_malloc_fail_count = 0;
    dummy_ctx_gt.allocator.malloc_cb = hook_malloc_modality;
    async_vtable.init(&dummy_ctx_gt);

    /* async_init OOM evloop */
    g_malloc_fail_count = 1;
    async_vtable.init(&dummy_ctx_gt);
    g_crf_malloc_hook = NULL;
    dummy_ctx_gt.allocator.malloc_cb = malloc;
#endif

    /* async_init logger failure */
    dummy_ctx_gt.logger.log_cb = mock_logger_err_cb;
    async_vtable.init(&dummy_ctx_gt);

    /* async_run NULL */
    async_vtable.run(NULL);

    /* async_run no internal_state */
    dummy_ctx_gt.internal_state = NULL;
    async_vtable.run(&dummy_ctx_gt);

    /* async_run logger failure start */
    dummy_ctx_gt.logger.log_cb = mock_logger_cb;
    async_vtable.init(&dummy_ctx_gt);
    dummy_ctx_gt.logger.log_cb = mock_logger_err_cb;
    async_vtable.run(&dummy_ctx_gt);
    dummy_ctx_gt.logger.log_cb = mock_logger_cb;
    async_vtable.destroy(&dummy_ctx_gt);

    /* run logger failure end */
    async_vtable.init(&dummy_ctx_gt);
    g_async_logger_calls = 0;
    dummy_ctx_gt.logger.log_cb = mock_logger_fail_on_second;
    async_vtable.run(&dummy_ctx_gt);
    dummy_ctx_gt.logger.log_cb = mock_logger_cb;
    async_vtable.destroy(&dummy_ctx_gt);

    /* destroy logger failure */
    async_vtable.init(&dummy_ctx_gt);
    dummy_ctx_gt.logger.log_cb = mock_logger_err_cb;
    async_vtable.destroy(&dummy_ctx_gt);

    /* async_destroy NULL */
    async_vtable.destroy(NULL);

    /* async_destroy no internal_state */
    dummy_ctx_gt.internal_state = NULL;
    async_vtable.destroy(&dummy_ctx_gt);

    /* async_destroy with valid server_sock socket */
    dummy_ctx_gt.logger.log_cb = mock_logger_cb;
    async_vtable.init(&dummy_ctx_gt);
    *(c_rest_socket_t *)dummy_ctx_gt.internal_state = valid_sock;
    async_vtable.destroy(&dummy_ctx_gt);

    /* async_destroy with invalid server_sock to trigger socket close error */
    async_vtable.init(&dummy_ctx_gt);
    {
      void **ptrs;
#if (defined(__unix__) || defined(__APPLE__)) && !defined(__EMSCRIPTEN__)
      *(c_rest_socket_t *)dummy_ctx_gt.internal_state = (c_rest_socket_t)9999;
#else
      *(c_rest_socket_t *)dummy_ctx_gt.internal_state = (c_rest_socket_t)9999;
#endif
      async_vtable.destroy(&dummy_ctx_gt);
      /* free leaked internal_state since destroy bailed early */
      ptrs = (void **)dummy_ctx_gt.internal_state;
      free(ptrs[1]);
      free(dummy_ctx_gt.internal_state);
      dummy_ctx_gt.internal_state = NULL;
    }

    /* async_destroy with evloop == NULL */
    async_vtable.init(&dummy_ctx_gt);
    {
      void **ptrs = (void **)dummy_ctx_gt.internal_state;
      void *tmp_evloop = ptrs[1];
      ptrs[1] = NULL;
      async_vtable.destroy(&dummy_ctx_gt);
      free(tmp_evloop);
    }

    /* async_run with logger == NULL */
    async_vtable.init(&dummy_ctx_gt);
    dummy_ctx_gt.logger.log_cb = NULL;
    async_vtable.run(&dummy_ctx_gt);
    async_vtable.destroy(&dummy_ctx_gt);
  }

  /* greenthread modality direct tests */
  {

    struct c_rest_context dummy_ctx_gt;
#if (defined(__unix__) || defined(__APPLE__)) && !defined(__EMSCRIPTEN__)
    c_rest_socket_t valid_sock =
        (c_rest_socket_t)socket(AF_INET, SOCK_STREAM, 0);
#else
    c_rest_socket_t valid_sock =
        (c_rest_socket_t)socket(AF_INET, SOCK_STREAM, 0);
#endif

    memset(&dummy_ctx_gt, 0, sizeof(dummy_ctx_gt));
    dummy_ctx_gt.allocator.malloc_cb = malloc;
    dummy_ctx_gt.allocator.free_cb = free;

    /* init NULL */
    greenthread_vtable.init(NULL);

    /* init OOM state */
#ifdef C_REST_TESTING_MALLOC_HOOK
    g_crf_malloc_hook = hook_malloc_modality;
    g_malloc_fail_count = 0;
    dummy_ctx_gt.allocator.malloc_cb = hook_malloc_modality;
    greenthread_vtable.init(&dummy_ctx_gt);

    g_crf_malloc_hook = NULL;
    dummy_ctx_gt.allocator.malloc_cb = malloc;
#endif

    /* init logger failure */
    dummy_ctx_gt.logger.log_cb = mock_logger_err_cb;
    greenthread_vtable.init(&dummy_ctx_gt);

    /* run NULL */
    greenthread_vtable.run(NULL);

    /* run no internal_state */
    dummy_ctx_gt.internal_state = NULL;
    greenthread_vtable.run(&dummy_ctx_gt);

    /* run logger failure start */
    dummy_ctx_gt.logger.log_cb = mock_logger_cb;
    greenthread_vtable.init(&dummy_ctx_gt);
    dummy_ctx_gt.logger.log_cb = mock_logger_err_cb;
    greenthread_vtable.run(&dummy_ctx_gt);
    dummy_ctx_gt.logger.log_cb = mock_logger_cb;
    greenthread_vtable.destroy(&dummy_ctx_gt);

    /* run logger failure end */
    greenthread_vtable.init(&dummy_ctx_gt);
    g_async_logger_calls = 0;
    dummy_ctx_gt.logger.log_cb = mock_logger_fail_on_second;
    greenthread_vtable.run(&dummy_ctx_gt);
    dummy_ctx_gt.logger.log_cb = mock_logger_cb;
    greenthread_vtable.destroy(&dummy_ctx_gt);

    /* run with logger == NULL */
    greenthread_vtable.init(&dummy_ctx_gt);
    dummy_ctx_gt.logger.log_cb = NULL;
    greenthread_vtable.run(&dummy_ctx_gt);
    greenthread_vtable.destroy(&dummy_ctx_gt);

    /* destroy logger failure */
    greenthread_vtable.init(&dummy_ctx_gt);
    dummy_ctx_gt.logger.log_cb = mock_logger_err_cb;
    greenthread_vtable.destroy(&dummy_ctx_gt);
    /* free leaked internal_state since destroy bailed early */
    free(dummy_ctx_gt.internal_state);
    dummy_ctx_gt.internal_state = NULL;

    /* destroy NULL */
    greenthread_vtable.destroy(NULL);

    /* destroy no internal_state */
    dummy_ctx_gt.internal_state = NULL;
    greenthread_vtable.destroy(&dummy_ctx_gt);

    /* destroy with valid server_sock socket */
    dummy_ctx_gt.logger.log_cb = mock_logger_cb;
    greenthread_vtable.init(&dummy_ctx_gt);
    *(c_rest_socket_t *)dummy_ctx_gt.internal_state = valid_sock;
    greenthread_vtable.destroy(&dummy_ctx_gt);

    /* destroy with invalid server_sock to trigger socket close error */
    greenthread_vtable.init(&dummy_ctx_gt);
#if (defined(__unix__) || defined(__APPLE__)) && !defined(__EMSCRIPTEN__)
    *(c_rest_socket_t *)dummy_ctx_gt.internal_state = (c_rest_socket_t)9999;
#else
    *(c_rest_socket_t *)dummy_ctx_gt.internal_state = (c_rest_socket_t)9999;
#endif
    greenthread_vtable.destroy(&dummy_ctx_gt);
    /* free leaked internal_state since destroy bailed early */
    free(dummy_ctx_gt.internal_state);
    dummy_ctx_gt.internal_state = NULL;
  }

  /* message_passing modality direct tests */
  {

    struct c_rest_context dummy_ctx_mp;

    memset(&dummy_ctx_mp, 0, sizeof(dummy_ctx_mp));
    dummy_ctx_mp.allocator.malloc_cb = malloc;
    dummy_ctx_mp.allocator.free_cb = free;

    /* init NULL */
    message_passing_vtable.init(NULL);

    /* init OOM state */
#ifdef C_REST_TESTING_MALLOC_HOOK
    g_crf_malloc_hook = hook_malloc_modality;
    g_malloc_fail_count = 0;
    dummy_ctx_mp.allocator.malloc_cb = hook_malloc_modality;
    message_passing_vtable.init(&dummy_ctx_mp);
    g_crf_malloc_hook = NULL;
    dummy_ctx_mp.allocator.malloc_cb = malloc;
#endif

    /* init logger failure */
    dummy_ctx_mp.logger.log_cb = mock_logger_err_cb;
    message_passing_vtable.init(&dummy_ctx_mp);

    /* run NULL */
    message_passing_vtable.run(NULL);

    /* run no internal_state */
    dummy_ctx_mp.internal_state = NULL;
    message_passing_vtable.run(&dummy_ctx_mp);

    /* run logger failure start */
    dummy_ctx_mp.logger.log_cb = mock_logger_cb;
    message_passing_vtable.init(&dummy_ctx_mp);
    dummy_ctx_mp.logger.log_cb = mock_logger_err_cb;
    message_passing_vtable.run(&dummy_ctx_mp);
    dummy_ctx_mp.logger.log_cb = mock_logger_cb;
    message_passing_vtable.destroy(&dummy_ctx_mp);

    /* run logger failure end */
    message_passing_vtable.init(&dummy_ctx_mp);
    g_async_logger_calls = 0;
    dummy_ctx_mp.logger.log_cb = mock_logger_fail_on_second;
    message_passing_vtable.run(&dummy_ctx_mp);
    dummy_ctx_mp.logger.log_cb = mock_logger_cb;
    message_passing_vtable.destroy(&dummy_ctx_mp);

    /* run with logger == NULL */
    message_passing_vtable.init(&dummy_ctx_mp);
    dummy_ctx_mp.logger.log_cb = NULL;
    message_passing_vtable.run(&dummy_ctx_mp);
    message_passing_vtable.destroy(&dummy_ctx_mp);

    /* destroy logger failure */
    message_passing_vtable.init(&dummy_ctx_mp);
    dummy_ctx_mp.logger.log_cb = mock_logger_err_cb;
    message_passing_vtable.destroy(&dummy_ctx_mp);
    free(dummy_ctx_mp.internal_state);
    dummy_ctx_mp.internal_state = NULL;

    /* destroy NULL */
    message_passing_vtable.destroy(NULL);

    /* destroy no internal_state */
    dummy_ctx_mp.internal_state = NULL;
    message_passing_vtable.destroy(&dummy_ctx_mp);

    /* destroy with valid server_sock socket */
    dummy_ctx_mp.logger.log_cb = mock_logger_cb;
    message_passing_vtable.init(&dummy_ctx_mp);
    *(c_rest_socket_t *)dummy_ctx_mp.internal_state =
        (c_rest_socket_t)socket(AF_INET, SOCK_STREAM, 0);
    message_passing_vtable.destroy(&dummy_ctx_mp);

    /* destroy with invalid server_sock to trigger socket close error */
    message_passing_vtable.init(&dummy_ctx_mp);
#if (defined(__unix__) || defined(__APPLE__)) && !defined(__EMSCRIPTEN__)
    *(c_rest_socket_t *)dummy_ctx_mp.internal_state = (c_rest_socket_t)9999;
#else
    *(c_rest_socket_t *)dummy_ctx_mp.internal_state = (c_rest_socket_t)9999;
#endif
    message_passing_vtable.destroy(&dummy_ctx_mp);
    free(dummy_ctx_mp.internal_state);
    dummy_ctx_mp.internal_state = NULL;
  }

#if !defined(__EMSCRIPTEN__) && !defined(CDD_DOS)
  /* multi_process modality direct tests */
  {

    struct c_rest_context dummy_ctx_mproc;

    memset(&dummy_ctx_mproc, 0, sizeof(dummy_ctx_mproc));
    dummy_ctx_mproc.allocator.malloc_cb = malloc;
    dummy_ctx_mproc.allocator.free_cb = free;

    /* init NULL */
    multi_process_vtable.init(NULL);

    /* init OOM state */
#ifdef C_REST_TESTING_MALLOC_HOOK
    g_crf_malloc_hook = hook_malloc_modality;
    g_malloc_fail_count = 0;
    dummy_ctx_mproc.allocator.malloc_cb = hook_malloc_modality;
    multi_process_vtable.init(&dummy_ctx_mproc);
    g_crf_malloc_hook = NULL;
    dummy_ctx_mproc.allocator.malloc_cb = malloc;
#endif

    /* init logger failure */
    dummy_ctx_mproc.logger.log_cb = mock_logger_err_cb;
    multi_process_vtable.init(&dummy_ctx_mproc);

    /* run NULL */
    multi_process_vtable.run(NULL);

    /* run no internal_state */
    dummy_ctx_mproc.internal_state = NULL;
    multi_process_vtable.run(&dummy_ctx_mproc);

    /* run logger failure start */
    dummy_ctx_mproc.logger.log_cb = mock_logger_cb;
    multi_process_vtable.init(&dummy_ctx_mproc);
    dummy_ctx_mproc.logger.log_cb = mock_logger_err_cb;
    multi_process_vtable.run(&dummy_ctx_mproc);
    dummy_ctx_mproc.logger.log_cb = mock_logger_cb;
    multi_process_vtable.destroy(&dummy_ctx_mproc);

    /* run logger failure end */
    multi_process_vtable.init(&dummy_ctx_mproc);
    g_async_logger_calls = 0;
    dummy_ctx_mproc.logger.log_cb = mock_logger_fail_on_second;
    multi_process_vtable.run(&dummy_ctx_mproc);
    dummy_ctx_mproc.logger.log_cb = mock_logger_cb;
    multi_process_vtable.destroy(&dummy_ctx_mproc);

    /* run with logger == NULL */
    multi_process_vtable.init(&dummy_ctx_mproc);
    dummy_ctx_mproc.logger.log_cb = NULL;
    multi_process_vtable.run(&dummy_ctx_mproc);
    multi_process_vtable.destroy(&dummy_ctx_mproc);

    /* destroy logger failure */
    multi_process_vtable.init(&dummy_ctx_mproc);
    dummy_ctx_mproc.logger.log_cb = mock_logger_err_cb;
    multi_process_vtable.destroy(&dummy_ctx_mproc);
    free(dummy_ctx_mproc.internal_state);
    dummy_ctx_mproc.internal_state = NULL;

    /* destroy NULL */
    multi_process_vtable.destroy(NULL);

    /* destroy no internal_state */
    dummy_ctx_mproc.internal_state = NULL;
    multi_process_vtable.destroy(&dummy_ctx_mproc);

    /* destroy with valid server_sock socket */
    dummy_ctx_mproc.logger.log_cb = mock_logger_cb;
    multi_process_vtable.init(&dummy_ctx_mproc);
    *(c_rest_socket_t *)dummy_ctx_mproc.internal_state =
        (c_rest_socket_t)socket(AF_INET, SOCK_STREAM, 0);
    multi_process_vtable.destroy(&dummy_ctx_mproc);

    /* destroy with invalid server_sock to trigger socket close error */
    multi_process_vtable.init(&dummy_ctx_mproc);
#if (defined(__unix__) || defined(__APPLE__)) && !defined(__EMSCRIPTEN__)
    *(c_rest_socket_t *)dummy_ctx_mproc.internal_state = (c_rest_socket_t)9999;
#else
    *(c_rest_socket_t *)dummy_ctx_mproc.internal_state = (c_rest_socket_t)9999;
#endif
    multi_process_vtable.destroy(&dummy_ctx_mproc);
    free(dummy_ctx_mproc.internal_state);
    dummy_ctx_mproc.internal_state = NULL;

    /* destroy with workers != NULL */
    multi_process_vtable.init(&dummy_ctx_mproc);
    {
      struct multi_process_state_mock {
        c_rest_socket_t server_sock;
        int is_running;
        c_rest_process_t *workers;
        int worker_count;
      };
      struct multi_process_state_mock *mock =
          (struct multi_process_state_mock *)dummy_ctx_mproc.internal_state;
      mock->workers = (c_rest_process_t *)malloc(sizeof(c_rest_process_t));
      multi_process_vtable.destroy(&dummy_ctx_mproc);
    }
  }
#endif

#if !defined(__EMSCRIPTEN__)
  /* Extra run tests with thread connection to hit accept success */
  {

    struct c_rest_context dummy_ctx;
    c_rest_thread_t client_thread;
    struct test_client_args args;

#if (defined(__unix__) || defined(__APPLE__))
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = my_sigalrm;
    sigaction(SIGALRM, &sa, NULL);
#endif

    memset(&dummy_ctx, 0, sizeof(dummy_ctx));
    dummy_ctx.allocator.malloc_cb = malloc;
    dummy_ctx.allocator.free_cb = free;
    dummy_ctx.listen_address = "127.0.0.1";
    dummy_ctx.logger.log_cb = mock_logger_cb;
    dummy_ctx.router = router;

    args.ctx = &dummy_ctx;

    /* Test client timeout when port is not listening */
    {
      struct test_client_args fail_args;
      fail_args.ctx = &dummy_ctx;
      fail_args.port = 1;
      test_client_thread(&fail_args);
    }

    /* sync */
    dummy_ctx.listen_port = 46781;
    args.port = 46781;
    dummy_ctx.vtable = &sync_vtable;
    sync_vtable.init(&dummy_ctx);
    {
      struct my_sync_state *state =
          (struct my_sync_state *)dummy_ctx.internal_state;
      c_rest_socket_create(&state->server_sock);
      c_rest_socket_bind(state->server_sock, "127.0.0.1", 46781);
      c_rest_socket_listen(state->server_sock, 128);

      c_rest_thread_create(&client_thread, test_client_thread, &args);
      sync_vtable.run(&dummy_ctx);
      c_rest_thread_join(client_thread);
      sync_vtable.destroy(&dummy_ctx);
    }

    /* single_thread */
    dummy_ctx.listen_port = 46779;
    args.port = 46779;
    dummy_ctx.vtable = &single_thread_vtable;
    single_thread_vtable.init(&dummy_ctx);
    {
      c_rest_thread_create(&client_thread, test_client_thread, &args);
      single_thread_vtable.run(&dummy_ctx);
      c_rest_thread_join(client_thread);
      single_thread_vtable.destroy(&dummy_ctx);
    }

    /* multi_thread */
    dummy_ctx.listen_port = 46780;
    args.port = 46780;
    dummy_ctx.vtable = &multi_thread_vtable;
    multi_thread_vtable.init(&dummy_ctx);
    {
      c_rest_thread_create(&client_thread, test_client_thread, &args);
      multi_thread_vtable.run(&dummy_ctx);
      c_rest_thread_join(client_thread);
      multi_thread_vtable.destroy(&dummy_ctx);
    }
  }
#endif

  {

    struct c_rest_context ctx_err;
    struct my_sync_state multi_st;
    memset(&ctx_err, 0, sizeof(ctx_err));
    memset(&multi_st, 0, sizeof(multi_st));

    ctx_err.allocator.malloc_cb = my_mock_null_malloc;
    ctx_err.allocator.free_cb = NULL;
    ctx_err.internal_state = &multi_st;

    multi_st.server_sock = (c_rest_socket_t)1;
    multi_st.is_running = 1;
    g_mock_socket_fail = 11;

    multi_thread_vtable.run(&ctx_err);

    multi_st.server_sock = (c_rest_socket_t)1;
    multi_st.is_running = 1;
    g_mock_socket_fail = 11;

    ctx_err.allocator.malloc_cb = malloc;
    multi_thread_vtable.run(&ctx_err);

    {
      c_rest_thread_t dummy_workers[65];
      memset(dummy_workers, 0, sizeof(dummy_workers));
      multi_st.server_sock = (c_rest_socket_t)1;
      multi_st.is_running = 1;
      multi_st.workers = dummy_workers;
      multi_st.worker_count = 64;
      g_mock_socket_fail = 11;
      multi_thread_vtable.run(&ctx_err);

      multi_st.server_sock = (c_rest_socket_t)1;
      multi_st.is_running = 1;
      multi_st.workers = dummy_workers;
      multi_st.worker_count = 0;
      g_mock_socket_fail = 6;
      multi_thread_vtable.run(&ctx_err);

      multi_st.server_sock = (c_rest_socket_t)1;
      multi_st.is_running = 1;
      multi_st.workers = dummy_workers;
      multi_st.worker_count = 0;
      g_mock_socket_fail = 13;
      multi_thread_vtable.run(&ctx_err);
      g_mock_socket_fail = 0;
    }

    g_mock_socket_fail = 0;
  }

  {

    struct c_rest_context ctx_tls;
    struct my_sync_state sync_st;
    struct my_sync_state single_st;
    struct my_sync_state multi_st;
    int j_mod;

    memset(&ctx_tls, 0, sizeof(ctx_tls));
    ctx_tls.listen_address = "127.0.0.1";
    memset(&sync_st, 0, sizeof(sync_st));
    memset(&single_st, 0, sizeof(single_st));
    memset(&multi_st, 0, sizeof(multi_st));

    ctx_tls.allocator.malloc_cb = malloc;
    ctx_tls.allocator.free_cb = free;

    for (j_mod = 1; j_mod <= 11; j_mod++) {
      if (j_mod == 9)
        continue;
      g_mock_socket_fail = j_mod;
      ctx_tls.internal_state = &sync_st;
      sync_st.server_sock = C_REST_INVALID_SOCKET;
      sync_st.is_running = 1;
      sync_vtable.run(&ctx_tls);

      g_mock_socket_fail = j_mod;
      ctx_tls.internal_state = &single_st;
      single_st.server_sock = C_REST_INVALID_SOCKET;
      single_st.is_running = 1;
      single_thread_vtable.run(&ctx_tls);

      g_mock_socket_fail = j_mod;
      ctx_tls.internal_state = &multi_st;
      multi_st.server_sock = C_REST_INVALID_SOCKET;
      multi_st.is_running = 1;
      multi_thread_vtable.run(&ctx_tls);
    }

    for (j_mod = 1002; j_mod <= 1003; j_mod++) {
      g_mock_socket_fail = j_mod;
      ctx_tls.internal_state = &sync_st;
      sync_st.server_sock = C_REST_INVALID_SOCKET;
      sync_st.is_running = 1;
      sync_vtable.run(&ctx_tls);

      g_mock_socket_fail = j_mod;
      ctx_tls.internal_state = &single_st;
      single_st.server_sock = C_REST_INVALID_SOCKET;
      single_st.is_running = 1;
      single_thread_vtable.run(&ctx_tls);

      g_mock_socket_fail = j_mod;
      ctx_tls.internal_state = &multi_st;
      multi_st.server_sock = C_REST_INVALID_SOCKET;
      multi_st.is_running = 1;
      multi_thread_vtable.run(&ctx_tls);
    }

    g_mock_socket_fail = 109;
    ctx_tls.internal_state = &sync_st;
    sync_st.server_sock = (c_rest_socket_t)1;
    sync_st.is_running = 1;
    sync_vtable.run(&ctx_tls);

    g_mock_socket_fail = 109;
    ctx_tls.internal_state = &single_st;
    single_st.server_sock = (c_rest_socket_t)1;
    single_st.is_running = 1;
    single_thread_vtable.run(&ctx_tls);

    g_mock_socket_fail = 109;
    ctx_tls.internal_state = &multi_st;
    multi_st.server_sock = (c_rest_socket_t)1;
    multi_st.is_running = 1;
    multi_thread_vtable.run(&ctx_tls);

    g_mock_socket_fail = 0;

    ctx_tls.tls_ctx = (void *)1;
    for (j_mod = 1; j_mod <= 3; j_mod++) {
      g_mock_tls_fail = j_mod;

      g_mock_socket_fail = 7;
      ctx_tls.internal_state = &sync_st;
      sync_st.server_sock = (c_rest_socket_t)1;
      sync_st.is_running = 1;
      sync_vtable.run(&ctx_tls);

      g_mock_socket_fail = 107;
      ctx_tls.internal_state = &sync_st;
      sync_st.server_sock = (c_rest_socket_t)1;
      sync_st.is_running = 1;
      sync_vtable.run(&ctx_tls);

      g_mock_socket_fail = 7;
      ctx_tls.internal_state = &single_st;
      single_st.server_sock = (c_rest_socket_t)1;
      single_st.is_running = 1;
      single_thread_vtable.run(&ctx_tls);

      g_mock_socket_fail = 107;
      ctx_tls.internal_state = &single_st;
      single_st.server_sock = (c_rest_socket_t)1;
      single_st.is_running = 1;
      single_thread_vtable.run(&ctx_tls);
    }

    g_mock_socket_fail = 0;
    ctx_tls.tls_ctx = NULL;
  }

  {
    struct c_rest_context null_ctx;
    memset(&null_ctx, 0, sizeof(null_ctx));
    null_ctx.internal_state = NULL;
    sync_vtable.run(&null_ctx);
    sync_vtable.stop(&null_ctx);
    sync_vtable.destroy(&null_ctx);
    single_thread_vtable.run(&null_ctx);
    single_thread_vtable.stop(&null_ctx);
    single_thread_vtable.destroy(&null_ctx);
    multi_thread_vtable.run(&null_ctx);
    multi_thread_vtable.stop(&null_ctx);
    multi_thread_vtable.destroy(&null_ctx);
  }

  {

    struct c_rest_context ctx_err;
    struct my_sync_state st;
    int fails[] = {106, 108};
    int k;
    memset(&ctx_err, 0, sizeof(ctx_err));
    ctx_err.listen_address = "127.0.0.1";
    memset(&st, 0, sizeof(st));
    ctx_err.allocator.malloc_cb = malloc;
    ctx_err.allocator.free_cb = my_dummy_free;

    ctx_err.logger.log_cb = mock_logger_err_cb;

    for (k = 0; k < 2; k++) {
      ctx_err.internal_state = &st;
      st.server_sock = (c_rest_socket_t)1;
      g_mock_socket_fail = fails[k];
      sync_vtable.destroy(&ctx_err);

      ctx_err.internal_state = &st;
      st.server_sock = (c_rest_socket_t)1;
      g_mock_socket_fail = fails[k];
      sync_vtable.stop(&ctx_err);

      ctx_err.internal_state = &st;
      st.server_sock = (c_rest_socket_t)1;
      g_mock_socket_fail = fails[k];
      single_thread_vtable.destroy(&ctx_err);

      ctx_err.internal_state = &st;
      st.server_sock = (c_rest_socket_t)1;
      g_mock_socket_fail = fails[k];
      single_thread_vtable.stop(&ctx_err);

      ctx_err.internal_state = &st;
      st.server_sock = (c_rest_socket_t)1;
      g_mock_socket_fail = fails[k];
      multi_thread_vtable.destroy(&ctx_err);

      ctx_err.internal_state = &st;
      st.server_sock = (c_rest_socket_t)1;
      g_mock_socket_fail = fails[k];
      multi_thread_vtable.stop(&ctx_err);
    }

    ctx_err.logger.log_cb = mock_logger_cb;

    ctx_err.internal_state = &st;
    st.server_sock = (c_rest_socket_t)1;
    g_mock_socket_fail = 108;
    sync_vtable.stop(&ctx_err);

    ctx_err.internal_state = &st;
    st.server_sock = (c_rest_socket_t)1;
    g_mock_socket_fail = 108;
    single_thread_vtable.stop(&ctx_err);

    ctx_err.internal_state = &st;
    st.server_sock = (c_rest_socket_t)1;
    g_mock_socket_fail = 108;
    multi_thread_vtable.stop(&ctx_err);

    ctx_err.internal_state = &st;
    st.server_sock = (c_rest_socket_t)1;
    g_mock_socket_fail = 108;
    sync_vtable.destroy(&ctx_err);

    ctx_err.internal_state = &st;
    st.server_sock = (c_rest_socket_t)1;
    g_mock_socket_fail = 108;
    single_thread_vtable.destroy(&ctx_err);

    ctx_err.internal_state = &st;
    st.server_sock = (c_rest_socket_t)1;
    g_mock_socket_fail = 108;
    multi_thread_vtable.destroy(&ctx_err);

    g_mock_socket_fail = 0;
  }

  {
    c_rest_socket_t client;
    c_rest_thread_t thread;

    g_mock_socket_fail = 11;
    mock_c_rest_socket_accept(C_REST_INVALID_SOCKET, &client);
    g_mock_socket_fail = 12;
    mock_c_rest_socket_accept(C_REST_INVALID_SOCKET, &client);
    g_mock_socket_fail = 109;
    mock_c_rest_socket_accept(C_REST_INVALID_SOCKET, &client);

    g_mock_socket_fail = 6;
    mock_c_rest_socket_close(C_REST_INVALID_SOCKET);
    g_mock_socket_fail = 106;
    mock_c_rest_socket_close(C_REST_INVALID_SOCKET);

    g_mock_socket_fail = 6;
    mock_c_rest_handle_connection(NULL, C_REST_INVALID_SOCKET);
    g_mock_socket_fail = 106;
    mock_c_rest_handle_connection(NULL, C_REST_INVALID_SOCKET);

    g_mock_socket_fail = 7;
    mock_c_rest_handle_connection(NULL, C_REST_INVALID_SOCKET);
    g_mock_socket_fail = 107;
    mock_c_rest_handle_connection(NULL, C_REST_INVALID_SOCKET);

    g_mock_socket_fail = 13;
    mock_c_rest_thread_create(&thread, NULL, NULL);
    g_mock_socket_fail = 113;
    mock_c_rest_thread_create(&thread, NULL, NULL);

    g_mock_socket_fail = 0;
  }

  test_modality_simple();
  return failed;
}
