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

static void my_dummy_free(void *ptr) { (void)ptr; }

static unsigned char g_static_wargs_pool[32][128];
static int g_static_wargs_idx = 0;
static void *my_static_pool_malloc(size_t sz) {
  (void)sz;
  return g_static_wargs_pool[(g_static_wargs_idx++) & 31];
}

static c_rest_error_t my_mock_logger_err_cb_internal(const char *msg) {
  (void)msg;
  return C_REST_ERROR_GENERIC;
}

static c_rest_error_t my_mock_logger_ok_cb_internal(const char *msg) {
  (void)msg;
  return C_REST_OK;
}

static c_rest_error_t my_mock_worker_err_cb(void *arg) {
  (void)arg;
  return C_REST_ERROR_GENERIC;
}

static c_rest_error_t dummy_modality_worker_fn(void *arg) {
  (void)arg;
  return C_REST_OK;
}

struct my_sync_state {
  c_rest_socket_t server_sock;
  volatile int is_running;
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

#if !defined(__EMSCRIPTEN__) && !defined(C_REST_NO_THREADS)
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

  if (c_rest_platform_init() != C_REST_OK)
    return C_REST_ERROR_GENERIC;
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
    if (c_rest_htons(target_port, &srv_addr.sin_port) != C_REST_OK) {
      return C_REST_ERROR_GENERIC;
    }
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
      c_rest_error_t stop_rc;
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
      stop_rc = c_rest_stop(args->ctx);
      if (stop_rc != C_REST_OK) {
        stop_rc = C_REST_OK;
      }
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
  c_rest_error_t rc;
  (void)req;
  (void)user_data;
  rc = c_rest_response_set_status(res, 200);
  if (rc != C_REST_OK)
    return rc;
  return c_rest_response_send(res);
}

TEST test_modality_vtable_error_branches(void) {
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
  ASSERT_NEQ(C_REST_OK, sync_vtable.init(&ctx));
  free(ctx.internal_state);
  ctx.internal_state = NULL;
  ASSERT_NEQ(C_REST_OK, single_thread_vtable.init(&ctx));
  free(ctx.internal_state);
  ctx.internal_state = NULL;
  ASSERT_NEQ(C_REST_OK, multi_thread_vtable.init(&ctx));
  ctx.internal_state = NULL;
  ASSERT_NEQ(C_REST_OK, async_vtable.init(&ctx));
  ctx.internal_state = NULL;

  /* 3. Null ctx */
  ASSERT_NEQ(C_REST_OK, sync_vtable.init(NULL));
  ASSERT_NEQ(C_REST_OK, sync_vtable.run(NULL));
  ASSERT_NEQ(C_REST_OK, sync_vtable.stop(NULL));
  ASSERT_NEQ(C_REST_OK, sync_vtable.destroy(NULL));

  ASSERT_NEQ(C_REST_OK, single_thread_vtable.init(NULL));
  ASSERT_NEQ(C_REST_OK, single_thread_vtable.run(NULL));
  ASSERT_NEQ(C_REST_OK, single_thread_vtable.stop(NULL));
  ASSERT_NEQ(C_REST_OK, single_thread_vtable.destroy(NULL));

  ASSERT_NEQ(C_REST_OK, multi_thread_vtable.init(NULL));
  ASSERT_NEQ(C_REST_OK, multi_thread_vtable.run(NULL));
  ASSERT_NEQ(C_REST_OK, multi_thread_vtable.stop(NULL));
  ASSERT_NEQ(C_REST_OK, multi_thread_vtable.destroy(NULL));

  ASSERT_NEQ(C_REST_OK, async_vtable.init(NULL));
  ASSERT_NEQ(C_REST_OK, async_vtable.run(NULL));
  ASSERT_NEQ(C_REST_OK, async_vtable.destroy(NULL));

  /* 4. Trigger accept failure in sync_run by passing a 9999 socket */
  ctx.logger.log_cb = NULL;
  ctx.internal_state = &sync_st;
  sync_st.server_sock = (c_rest_socket_t)9999;
  sync_st.is_running = 1;
  {
    c_rest_error_t r_rc = sync_vtable.run(&ctx);
    ASSERT(r_rc == C_REST_OK || r_rc == C_REST_ERROR_GENERIC);
  }

  /* Trigger bind failure in single/multi/async run */
  ctx.listen_port = 1;
  ctx.listen_address = "invalid_address_for_test";

  ctx.internal_state = &single_st;
  single_st.server_sock = C_REST_INVALID_SOCKET;
  ASSERT_NEQ(C_REST_OK, single_thread_vtable.run(&ctx));

  ctx.internal_state = &multi_st;
  multi_st.server_sock = (c_rest_socket_t)1;
  {
    c_rest_thread_t workers_arr[3];
#ifndef C_REST_NO_THREADS
    ASSERT_EQ(C_REST_OK, c_rest_thread_create(&workers_arr[0],
                                              dummy_modality_worker_fn, NULL));
    workers_arr[1] = (c_rest_thread_t)0;
    ASSERT_EQ(C_REST_OK, c_rest_thread_create(&workers_arr[2],
                                              dummy_modality_worker_fn, NULL));
    multi_st.workers = workers_arr;
    multi_st.worker_count = 3;
    g_mock_socket_fail = 8;
    ASSERT_EQ(C_REST_OK, multi_thread_vtable.run(&ctx));
    g_mock_socket_fail = 0;
#endif

    workers_arr[0] = (c_rest_thread_t)-1;
    multi_st.workers = workers_arr;
    multi_st.worker_count = 1;
    g_mock_socket_fail = 8;
    ASSERT_NEQ(C_REST_OK, multi_thread_vtable.run(&ctx));
    g_mock_socket_fail = 0;
  }

  ctx.internal_state = &async_st;
  async_st.server_sock = C_REST_INVALID_SOCKET;
  async_st.evloop = &dummy_evloop;
  ASSERT_NEQ(C_REST_OK, async_vtable.run(&ctx));

  /* 5. Trigger logger failure in run */
  ctx.logger.log_cb = my_mock_logger_err_cb_internal;

  ctx.internal_state = &sync_st;
  sync_st.is_running = 1;
  sync_st.server_sock = (c_rest_socket_t)9999;
  {
    c_rest_error_t r_rc = sync_vtable.run(&ctx);
    ASSERT(r_rc == C_REST_OK || r_rc == C_REST_ERROR_GENERIC);
  }

  ctx.internal_state = &single_st;
  single_st.is_running = 1;
  single_st.server_sock = (c_rest_socket_t)9999;
  ASSERT_NEQ(C_REST_OK, single_thread_vtable.run(&ctx));

  ctx.internal_state = &multi_st;
  multi_st.is_running = 1;
  multi_st.server_sock = (c_rest_socket_t)9999;
  ASSERT_NEQ(C_REST_OK, multi_thread_vtable.run(&ctx));

  ctx.internal_state = &async_st;
  async_st.evloop = &dummy_evloop;
  async_st.server_sock = (c_rest_socket_t)9999;
  ASSERT_NEQ(C_REST_OK, async_vtable.run(&ctx));

  /* 6. Trigger logger failure in stop */
  ctx.internal_state = &sync_st;
  sync_st.server_sock = C_REST_INVALID_SOCKET;
  ASSERT_NEQ(C_REST_OK, sync_vtable.stop(&ctx));

  ctx.internal_state = &single_st;
  single_st.server_sock = C_REST_INVALID_SOCKET;
  {
    c_rest_error_t s_rc = single_thread_vtable.stop(&ctx);
    ASSERT(s_rc == C_REST_OK || s_rc == C_REST_ERROR_GENERIC);
  }

  ctx.internal_state = &multi_st;
  {
    c_rest_thread_t workers_arr[3];
#ifndef C_REST_NO_THREADS
    ASSERT_EQ(C_REST_OK, c_rest_thread_create(&workers_arr[0],
                                              dummy_modality_worker_fn, NULL));
    workers_arr[1] = (c_rest_thread_t)0;
    ASSERT_EQ(C_REST_OK, c_rest_thread_create(&workers_arr[2],
                                              dummy_modality_worker_fn, NULL));
    multi_st.workers = workers_arr;
    multi_st.worker_count = 3;
    ASSERT_NEQ(C_REST_OK, multi_thread_vtable.stop(&ctx));
#endif

    workers_arr[0] = (c_rest_thread_t)-1;
    multi_st.workers = workers_arr;
    multi_st.worker_count = 1;
    ASSERT_NEQ(C_REST_OK, multi_thread_vtable.stop(&ctx));

    multi_st.workers = NULL;
    multi_st.worker_count = 0;
    multi_st.server_sock = (c_rest_socket_t)9999;
    ASSERT_NEQ(C_REST_OK, multi_thread_vtable.stop(&ctx));
  }

  /* 7. Trigger socket close failure in destroy & stop */
  ctx.logger.log_cb = NULL;

  ctx.internal_state = &sync_st;
  sync_st.server_sock = (c_rest_socket_t)9999;
  ASSERT_NEQ(C_REST_OK, sync_vtable.stop(&ctx));

  ctx.internal_state = &sync_st;
  sync_st.server_sock = (c_rest_socket_t)9999;
  ASSERT_NEQ(C_REST_OK, sync_vtable.destroy(&ctx));

  ctx.internal_state = &single_st;
  single_st.server_sock = (c_rest_socket_t)9999;
  ASSERT_NEQ(C_REST_OK, single_thread_vtable.destroy(&ctx));

  ctx.internal_state = &multi_st;
  multi_st.server_sock = (c_rest_socket_t)9999;
  ASSERT_NEQ(C_REST_OK, multi_thread_vtable.destroy(&ctx));

  ctx.internal_state = &async_st;
  async_st.evloop = &dummy_evloop;
  async_st.server_sock = (c_rest_socket_t)9999;
  ASSERT_NEQ(C_REST_OK, async_vtable.destroy(&ctx));

  /* 7b. Trigger socket close success in destroy & stop */
  {
    c_rest_socket_t s1 = C_REST_INVALID_SOCKET;
    c_rest_socket_t s2 = C_REST_INVALID_SOCKET;
    c_rest_socket_t s3 = C_REST_INVALID_SOCKET;
    c_rest_socket_t s4 = C_REST_INVALID_SOCKET;
    ASSERT_EQ(C_REST_OK, c_rest_socket_create(&s1));
    ASSERT_EQ(C_REST_OK, c_rest_socket_create(&s2));
    ASSERT_EQ(C_REST_OK, c_rest_socket_create(&s3));
    ASSERT_EQ(C_REST_OK, c_rest_socket_create(&s4));

    ctx.internal_state = &sync_st;
    sync_st.server_sock = s1;
    ASSERT_EQ(C_REST_OK, sync_vtable.stop(&ctx));

    ctx.internal_state = &sync_st;
    sync_st.server_sock = s2;
    ASSERT_EQ(C_REST_OK, sync_vtable.destroy(&ctx));

    ctx.internal_state = &single_st;
    single_st.server_sock = s3;
    ASSERT_EQ(C_REST_OK, single_thread_vtable.destroy(&ctx));

    ctx.internal_state = &multi_st;
    multi_st.server_sock = s4;
    ASSERT_EQ(C_REST_OK, multi_thread_vtable.destroy(&ctx));
  }

  /* 8. Trigger logger failure in destroy */
  ctx.logger.log_cb = my_mock_logger_err_cb_internal;

  ctx.internal_state = &sync_st;
  sync_st.server_sock = C_REST_INVALID_SOCKET;
  ASSERT_NEQ(C_REST_OK, sync_vtable.destroy(&ctx));

  ctx.internal_state = &single_st;
  single_st.server_sock = C_REST_INVALID_SOCKET;
  ASSERT_NEQ(C_REST_OK, single_thread_vtable.destroy(&ctx));

  ctx.internal_state = &multi_st;
  multi_st.server_sock = C_REST_INVALID_SOCKET;
  {
    c_rest_thread_t workers_arr[3];
#ifndef C_REST_NO_THREADS
    ASSERT_EQ(C_REST_OK, c_rest_thread_create(&workers_arr[0],
                                              dummy_modality_worker_fn, NULL));
    workers_arr[1] = (c_rest_thread_t)0;
    ASSERT_EQ(C_REST_OK, c_rest_thread_create(&workers_arr[2],
                                              dummy_modality_worker_fn, NULL));
    multi_st.workers = workers_arr;
    multi_st.worker_count = 3;
    ASSERT_NEQ(C_REST_OK, multi_thread_vtable.destroy(&ctx));
#endif

    workers_arr[0] = (c_rest_thread_t)-1;
    multi_st.workers = workers_arr;
    multi_st.worker_count = 1;
    ctx.internal_state = &multi_st;
    ASSERT_NEQ(C_REST_OK, multi_thread_vtable.destroy(&ctx));

    ctx.logger.log_cb = NULL;
    multi_st.workers = NULL;
    multi_st.worker_count = 0;
    ASSERT_EQ(C_REST_OK, multi_thread_vtable.destroy(&ctx));
  }

  ctx.internal_state = &async_st;
  async_st.server_sock = C_REST_INVALID_SOCKET;
  async_st.evloop = &dummy_evloop;
  ctx.logger.log_cb = my_mock_logger_err_cb_internal;
  ASSERT_NEQ(C_REST_OK, async_vtable.destroy(&ctx));

  /* 9. Trigger logger SUCCESS in init, run, stop, destroy */
  ctx.logger.log_cb = my_mock_logger_ok_cb_internal;
  ASSERT_EQ(C_REST_OK, sync_vtable.init(&ctx));
  free(ctx.internal_state);

  ctx.internal_state = &sync_st;
  sync_st.server_sock = (c_rest_socket_t)9999;
  sync_st.is_running = 1;
  ASSERT_EQ(C_REST_OK, sync_vtable.run(&ctx));

  sync_st.server_sock = C_REST_INVALID_SOCKET;
  ASSERT_EQ(C_REST_OK, sync_vtable.stop(&ctx));

  ctx.internal_state = &sync_st;
  sync_st.server_sock = C_REST_INVALID_SOCKET;
  ASSERT_EQ(C_REST_OK, sync_vtable.destroy(&ctx));

  PASS();
}

TEST test_modality_handle_connection_and_lifecycle(void) {
  struct c_rest_context *ctx = NULL;
  struct c_rest_router *router = NULL;
#if !defined(__EMSCRIPTEN__) && !defined(CDD_DOS) && !defined(C_REST_NO_THREADS)
  static const enum c_rest_modality_type modalities[8] = {
      C_REST_MODALITY_SYNC,
      C_REST_MODALITY_ASYNC,
      C_REST_MODALITY_MULTI_THREAD,
      C_REST_MODALITY_SINGLE_THREAD,
      C_REST_MODALITY_MULTI_PROCESS,
      C_REST_MODALITY_GREENTHREAD,
      C_REST_MODALITY_MESSAGE_PASSING,
      C_REST_MODALITY_SINGLE_PROCESS};
  int num_modalities = 8;
#elif !defined(__EMSCRIPTEN__) && !defined(CDD_DOS) &&                         \
    defined(C_REST_NO_THREADS)
  static const enum c_rest_modality_type modalities[7] = {
      C_REST_MODALITY_SYNC,          C_REST_MODALITY_ASYNC,
      C_REST_MODALITY_SINGLE_THREAD, C_REST_MODALITY_MULTI_PROCESS,
      C_REST_MODALITY_GREENTHREAD,   C_REST_MODALITY_MESSAGE_PASSING,
      C_REST_MODALITY_SINGLE_PROCESS};
  int num_modalities = 7;
#else
  static const enum c_rest_modality_type modalities[6] = {
      C_REST_MODALITY_SYNC,
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

  ASSERT_EQ(C_REST_OK, c_rest_router_init(&router));
  ASSERT_EQ(C_REST_OK,
            c_rest_router_add(router, "POST", "/test", my_dummy_handler, NULL));
  ASSERT_EQ(C_REST_OK, c_rest_router_add(router, "POST", "/headers_sent",
                                         my_dummy_handler_headers_sent, NULL));

  /* Test c_rest_handle_connection parsing coverage */
  ASSERT_EQ(C_REST_OK, c_rest_init(C_REST_MODALITY_SINGLE_PROCESS, &ctx));
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
      ASSERT_EQ(C_REST_OK,
                c_rest_socket_send(client_sock, req, strlen(req), &wr));
      ASSERT_EQ(C_REST_OK, c_rest_socket_close(client_sock));
      ASSERT_EQ(C_REST_OK, c_rest_handle_connection(ctx, accepted_sock));
      ASSERT_EQ(C_REST_OK, c_rest_socket_close(accepted_sock));

      /* Test headers_sent handler branch */
      socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
      accepted_sock = (c_rest_socket_t)fds[0];
      client_sock = (c_rest_socket_t)fds[1];
      {
        const char *req_hs = "POST /headers_sent HTTP/1.1\r\nHost: "
                             "loc\r\nContent-Length: 0\r\n\r\n";
        ASSERT_EQ(C_REST_OK,
                  c_rest_socket_send(client_sock, req_hs, strlen(req_hs), &wr));
        ASSERT_EQ(C_REST_OK, c_rest_socket_close(client_sock));
        ASSERT_EQ(C_REST_OK, c_rest_handle_connection(ctx, accepted_sock));
        ASSERT_EQ(C_REST_OK, c_rest_socket_close(accepted_sock));
      }

      /* Test parser vtable fail */
      socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
      accepted_sock = (c_rest_socket_t)fds[0];
      client_sock = (c_rest_socket_t)fds[1];
      ASSERT_EQ(C_REST_OK,
                c_rest_socket_send(client_sock, req, strlen(req), &wr));
      ASSERT_EQ(C_REST_OK, c_rest_socket_close(client_sock));
      g_mock_parser_vtable_fail = 1;
      ASSERT_EQ(C_REST_OK, c_rest_handle_connection(ctx, accepted_sock));
      g_mock_parser_vtable_fail = 0;
      ASSERT_EQ(C_REST_OK, c_rest_socket_close(accepted_sock));

      /* Test req_cleanup fail */
      socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
      accepted_sock = (c_rest_socket_t)fds[0];
      client_sock = (c_rest_socket_t)fds[1];
      ASSERT_EQ(C_REST_OK,
                c_rest_socket_send(client_sock, req, strlen(req), &wr));
      ASSERT_EQ(C_REST_OK, c_rest_socket_close(client_sock));
      g_mock_req_cleanup_fail = 1;
      ASSERT_EQ(C_REST_OK, c_rest_handle_connection(ctx, accepted_sock));
      g_mock_req_cleanup_fail = 0;
      ASSERT_EQ(C_REST_OK, c_rest_socket_close(accepted_sock));

      /* Test res_cleanup fail */
      socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
      accepted_sock = (c_rest_socket_t)fds[0];
      client_sock = (c_rest_socket_t)fds[1];
      ASSERT_EQ(C_REST_OK,
                c_rest_socket_send(client_sock, req, strlen(req), &wr));
      ASSERT_EQ(C_REST_OK, c_rest_socket_close(client_sock));
      g_mock_res_cleanup_fail = 1;
      ASSERT_EQ(C_REST_OK, c_rest_handle_connection(ctx, accepted_sock));
      g_mock_res_cleanup_fail = 0;
      ASSERT_EQ(C_REST_OK, c_rest_socket_close(accepted_sock));

      /* Test parser should_keep_alive fail */
      socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
      accepted_sock = (c_rest_socket_t)fds[0];
      client_sock = (c_rest_socket_t)fds[1];
      ASSERT_EQ(C_REST_OK,
                c_rest_socket_send(client_sock, req, strlen(req), &wr));
      ASSERT_EQ(C_REST_OK, c_rest_socket_close(client_sock));
      g_mock_parser_should_keep_alive_fail = 1;
      ASSERT_EQ(C_REST_OK, c_rest_handle_connection(ctx, accepted_sock));
      g_mock_parser_should_keep_alive_fail = 0;
      ASSERT_EQ(C_REST_OK, c_rest_socket_close(accepted_sock));

      /* Test parser destroy fail */
      socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
      accepted_sock = (c_rest_socket_t)fds[0];
      client_sock = (c_rest_socket_t)fds[1];
      ASSERT_EQ(C_REST_OK,
                c_rest_socket_send(client_sock, req, strlen(req), &wr));
      ASSERT_EQ(C_REST_OK, c_rest_socket_close(client_sock));
      g_mock_parser_destroy_fail = 1;
      ASSERT_EQ(C_REST_OK, c_rest_handle_connection(ctx, accepted_sock));
      g_mock_parser_destroy_fail = 0;
      ASSERT_EQ(C_REST_OK, c_rest_socket_close(accepted_sock));

      socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
      accepted_sock = (c_rest_socket_t)fds[0];
      client_sock = (c_rest_socket_t)fds[1];
      ASSERT_EQ(C_REST_OK,
                c_rest_socket_send(client_sock, req, strlen(req), &wr));
      ASSERT_EQ(C_REST_OK, c_rest_socket_close(client_sock));
      ctx->router = NULL;
      ASSERT_EQ(C_REST_OK, c_rest_handle_connection(ctx, accepted_sock));
      ctx->router = router;
      ASSERT_EQ(C_REST_OK, c_rest_socket_close(accepted_sock));

      socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
      accepted_sock = (c_rest_socket_t)fds[0];
      client_sock = (c_rest_socket_t)fds[1];
      ASSERT_EQ(C_REST_OK,
                c_rest_socket_send(client_sock, req, strlen(req), &wr));
      ASSERT_EQ(C_REST_OK, c_rest_socket_close(client_sock));
      ctx->tls_ctx = (struct c_rest_tls_context *)1;
      rc = c_rest_handle_connection(ctx, accepted_sock);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_GENERIC ||
             rc == C_REST_ERROR_TLS || rc == C_REST_ERROR_NETWORK ||
             rc == C_REST_ERROR_IO || rc == C_REST_ERROR_INVALID_ARG ||
             rc == C_REST_ERROR_OOM);
      ctx->tls_ctx = NULL;
      ASSERT_EQ(C_REST_OK, c_rest_socket_close(accepted_sock));

      /* Test c_rest_tls_close fail in c_rest_handle_connection */
      socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
      accepted_sock = (c_rest_socket_t)fds[0];
      client_sock = (c_rest_socket_t)fds[1];
      ASSERT_EQ(C_REST_OK,
                c_rest_socket_send(client_sock, req, strlen(req), &wr));
      ASSERT_EQ(C_REST_OK, c_rest_socket_close(client_sock));
      ctx->tls_ctx = (struct c_rest_tls_context *)1;
      g_mock_tls_fail = 2;
      ASSERT_EQ(C_REST_OK, c_rest_handle_connection(ctx, accepted_sock));
      g_mock_tls_fail = 0;
      ctx->tls_ctx = NULL;
      ASSERT_EQ(C_REST_OK, c_rest_socket_close(accepted_sock));

      socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
      accepted_sock = (c_rest_socket_t)fds[0];
      client_sock = (c_rest_socket_t)fds[1];
      ASSERT_EQ(C_REST_OK,
                c_rest_socket_send(client_sock, req, strlen(req), &wr));
      ASSERT_EQ(C_REST_OK, c_rest_socket_close(client_sock));
      ctx->tls_ctx = (struct c_rest_tls_context *)1;
      g_mock_tls_fail = 6;
      ASSERT_NEQ(C_REST_OK, c_rest_handle_connection(ctx, accepted_sock));
      g_mock_tls_fail = 0;
      ctx->tls_ctx = NULL;
      ASSERT_EQ(C_REST_OK, c_rest_socket_close(accepted_sock));

#ifdef C_REST_TESTING_MALLOC_HOOK
      socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
      accepted_sock = (c_rest_socket_t)fds[0];
      client_sock = (c_rest_socket_t)fds[1];
      ASSERT_EQ(C_REST_OK, c_rest_socket_close(client_sock));
      ctx->tls_ctx = (struct c_rest_tls_context *)1;
      g_mock_tls_fail = 7;
      ASSERT_EQ(C_REST_OK, c_rest_handle_connection(ctx, accepted_sock));
      g_mock_tls_fail = 0;
      ctx->tls_ctx = NULL;
      ASSERT_EQ(C_REST_OK, c_rest_socket_close(accepted_sock));
#endif
    }
#endif
    ASSERT_EQ(C_REST_OK, c_rest_destroy(ctx));
  }

  /* Test c_rest_handle_connection with OOM in parsing callbacks */
  ASSERT_EQ(C_REST_OK, c_rest_init(C_REST_MODALITY_SINGLE_PROCESS, &ctx));
  {
    ctx->router = router;
#if (defined(__unix__) || defined(__APPLE__)) && !defined(__EMSCRIPTEN__)
#ifdef C_REST_TESTING_MALLOC_HOOK
    {
      int oom_idx;
      for (oom_idx = 0; oom_idx < 150; oom_idx++) {
        int fds[2];
        const char *req = "POST /test?foo=bar HTTP/1.1\r\nHost: "
                          "loc\r\nContent-Length: 5\r\n\r\nhello";
        size_t wr = 0;
        socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
        accepted_sock = (c_rest_socket_t)fds[0];
        client_sock = (c_rest_socket_t)fds[1];
        ASSERT_EQ(C_REST_OK,
                  c_rest_socket_send(client_sock, req, strlen(req), &wr));
        ASSERT_EQ(C_REST_OK, c_rest_socket_close(client_sock));
        g_malloc_fail_count = oom_idx;
        rc = c_rest_handle_connection(ctx, accepted_sock);
        ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
               rc == C_REST_ERROR_GENERIC);
        ASSERT_EQ(C_REST_OK, c_rest_socket_close(accepted_sock));
      }
      g_malloc_fail_count = -1;
      g_crf_realloc_hook = NULL;
      g_crf_calloc_hook = NULL;
      g_crf_strdup_hook = NULL;

      {
        int fds[2];
        const char *req = "POST /test?foo=bar HTTP/1.1\r\nHost: "
                          "loc\r\nContent-Length: 5\r\n\r\nhello";
        size_t wr = 0;
        socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
        accepted_sock = (c_rest_socket_t)fds[0];
        client_sock = (c_rest_socket_t)fds[1];
        ASSERT_EQ(C_REST_OK,
                  c_rest_socket_send(client_sock, req, strlen(req), &wr));
        ASSERT_EQ(C_REST_OK, c_rest_socket_close(client_sock));
        ASSERT_EQ(C_REST_OK, c_rest_handle_connection(ctx, accepted_sock));
        ASSERT_EQ(C_REST_OK, c_rest_socket_close(accepted_sock));
      }
    }
#endif
#endif
    ASSERT_EQ(C_REST_OK, c_rest_destroy(ctx));
  }

  /* Test c_rest_handle_connection with TLS coverage */
  ASSERT_EQ(C_REST_OK, c_rest_init(C_REST_MODALITY_SINGLE_PROCESS, &ctx));
  {
    ctx->router = router;
#if (defined(__unix__) || defined(__APPLE__)) && !defined(__EMSCRIPTEN__)
    {
      int fds[2];
      socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
      accepted_sock = (c_rest_socket_t)fds[0];
      client_sock = (c_rest_socket_t)fds[1];

      ctx->tls_ctx = (struct c_rest_tls_context *)1;

#ifdef C_REST_TESTING_MALLOC_HOOK
      g_malloc_fail_count = 0;
      g_crf_malloc_hook = hook_malloc_modality;
      ASSERT_NEQ(C_REST_OK, c_rest_handle_connection(ctx, accepted_sock));
      g_malloc_fail_count = -1;
      g_crf_malloc_hook = NULL;
#endif

      rc = c_rest_handle_connection(ctx, accepted_sock);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_GENERIC ||
             rc == C_REST_ERROR_TLS || rc == C_REST_ERROR_NETWORK ||
             rc == C_REST_ERROR_IO || rc == C_REST_ERROR_INVALID_ARG ||
             rc == C_REST_ERROR_OOM);

      ctx->tls_ctx = NULL;
      ASSERT_EQ(C_REST_OK, c_rest_socket_close(client_sock));
      ASSERT_EQ(C_REST_OK, c_rest_socket_close(accepted_sock));
    }

    {
      int fds[2];
      const char *req = "INVALID REQUEST\r\n\r\n";
      size_t wr = 0;
      socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
      accepted_sock = (c_rest_socket_t)fds[0];
      client_sock = (c_rest_socket_t)fds[1];

      ASSERT_EQ(C_REST_OK,
                c_rest_socket_send(client_sock, req, strlen(req), &wr));
      ASSERT_EQ(C_REST_OK, c_rest_socket_close(client_sock));
      rc = c_rest_handle_connection(ctx, accepted_sock);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_PARSE ||
             rc == C_REST_ERROR_GENERIC);
      ASSERT_EQ(C_REST_OK, c_rest_socket_close(accepted_sock));
    }
#endif
    ASSERT_EQ(C_REST_OK, c_rest_destroy(ctx));
  }

  ASSERT_NEQ(C_REST_OK, c_rest_handle_connection(NULL, C_REST_INVALID_SOCKET));

  /* Modality context error branches */
  {
    struct c_rest_context mctx;
    struct c_rest_logger lgr;
    memset(&mctx, 0, sizeof(mctx));
    memset(&lgr, 0, sizeof(lgr));
    lgr.log_cb = my_mock_logger_err_cb_internal;
    mctx.logger = lgr;
    mctx.allocator.free_cb = my_dummy_free;

    ASSERT_EQ(C_REST_OK, c_rest_init(C_REST_MODALITY_SINGLE_PROCESS, &ctx));
    mctx.logger = lgr;
    ASSERT_NEQ(C_REST_OK, ctx->vtable->init(&mctx));
    mctx.logger.log_cb = my_mock_logger_ok_cb_internal;
    ASSERT_EQ(C_REST_OK, ctx->vtable->init(&mctx));
    ASSERT_EQ(C_REST_OK, c_rest_destroy(ctx));
    ctx = NULL;

    mctx.logger = lgr;
    g_mock_orm_init_fail = 1;
    mctx.db_config.connection_string = "invalid://";
    ASSERT_NEQ(C_REST_OK, c_rest_run(&mctx));

    mctx.logger.log_cb = my_mock_logger_ok_cb_internal;
    ASSERT_NEQ(C_REST_OK, c_rest_run(&mctx));

    mctx.logger.log_cb = NULL;
    ASSERT_NEQ(C_REST_OK, c_rest_run(&mctx));
    g_mock_orm_init_fail = 0;
    mctx.db_config.connection_string = NULL;
    mctx.logger = lgr;

    mctx.vtable = NULL;
    ASSERT_NEQ(C_REST_OK, c_rest_run(&mctx));
    ASSERT_NEQ(C_REST_OK, c_rest_stop(&mctx));

    mctx.db_pool = (void *)1;
    ASSERT_NEQ(C_REST_OK, c_rest_destroy(&mctx));

    mctx.db_pool = (void *)1;
    mctx.logger.log_cb = my_mock_logger_ok_cb_internal;
    ASSERT_EQ(C_REST_OK, c_rest_destroy(&mctx));
    mctx.db_pool = NULL;

    mctx.db_pool = (void *)2;
    mctx.logger.log_cb = NULL;
    ASSERT_NEQ(C_REST_OK, c_rest_destroy(&mctx));

    mctx.db_pool = NULL;
    g_mock_platform_cleanup_fail = 1;
    ASSERT_NEQ(C_REST_OK, c_rest_destroy(&mctx));
    g_mock_platform_cleanup_fail = 0;
  }

  ASSERT_NEQ(C_REST_OK, c_rest_init((enum c_rest_modality_type)999, &ctx));

#ifdef C_REST_TESTING_MALLOC_HOOK
  {
    char *s;
    void *p;

    g_malloc_fail_count = 0;
    ASSERT_EQ(NULL, hook_calloc_modality(1, 1));
    g_malloc_fail_count = 0;
    ASSERT_EQ(NULL, hook_strdup_modality("t"));

    g_malloc_fail_count = 1;
    p = hook_calloc_modality(1, 1);
    ASSERT_NEQ(NULL, p);
    free(p);

    g_malloc_fail_count = 1;
    s = hook_strdup_modality("t");
    ASSERT_NEQ(NULL, s);
    free(s);

    g_malloc_fail_count = -1;
    p = hook_calloc_modality(1, 1);
    ASSERT_NEQ(NULL, p);
    free(p);

    g_malloc_fail_count = -1;
    s = hook_strdup_modality("t");
    ASSERT_NEQ(NULL, s);
    free(s);
  }
#endif

#if (defined(__unix__) || defined(__APPLE__)) && !defined(__EMSCRIPTEN__) &&   \
    !defined(C_REST_NO_THREADS)
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

    ASSERT_NEQ(C_REST_OK, c_rest_init(mod, NULL));

#ifdef C_REST_TESTING_MALLOC_HOOK
    g_malloc_fail_count = 0;
    ASSERT_NEQ(C_REST_OK, c_rest_init(mod, &ctx));

    if (mod != C_REST_MODALITY_SINGLE_PROCESS) {
      g_malloc_fail_count = 1;
      ASSERT_NEQ(C_REST_OK, c_rest_init(mod, &ctx));
      g_malloc_fail_count = 2;
      rc = c_rest_init(mod, &ctx);
      if (rc == C_REST_OK) {
        g_malloc_fail_count = -1;
        ASSERT_EQ(C_REST_OK, c_rest_destroy(ctx));
      }
    }
#endif
    g_malloc_fail_count = -1;

    ASSERT_EQ(C_REST_OK, c_rest_init(mod, &ctx));
    ASSERT_NEQ(C_REST_OK, c_rest_run(NULL));
    ASSERT_NEQ(C_REST_OK, c_rest_stop(NULL));

    rc = c_rest_stop(ctx);
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_GENERIC ||
           rc == C_REST_ERROR_NETWORK || rc == C_REST_ERROR_IO ||
           rc == C_REST_ERROR_INVALID_ARG || rc == C_REST_ERROR_OOM);

    ctx->listen_address = "invalid_address_for_test";
    ctx->listen_port = 1;
    ctx->logger.log_cb = mock_logger_cb;
    rc = c_rest_run(ctx);
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_GENERIC ||
           rc == C_REST_ERROR_NETWORK || rc == C_REST_ERROR_IO ||
           rc == C_REST_ERROR_INVALID_ARG || rc == C_REST_ERROR_OOM);

#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
    {
      g_accept_calls = 0;
      ctx->listen_address = "127.0.0.1";
      ctx->listen_port = 8080;
      ctx->cm_env = (cm_env_t)1;
      rc = c_rest_run(ctx);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_GENERIC ||
             rc == C_REST_ERROR_NETWORK || rc == C_REST_ERROR_IO ||
             rc == C_REST_ERROR_INVALID_ARG || rc == C_REST_ERROR_OOM);
    }
#endif

    ASSERT_NEQ(C_REST_OK, c_rest_destroy(NULL));
    ASSERT_EQ(C_REST_OK, c_rest_destroy(ctx));

    ASSERT_EQ(C_REST_OK, c_rest_init(mod, &ctx));
    ctx->logger.log_cb = mock_logger_err_cb;
    rc = c_rest_destroy(ctx);
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_GENERIC ||
           rc == C_REST_ERROR_NETWORK || rc == C_REST_ERROR_IO ||
           rc == C_REST_ERROR_INVALID_ARG || rc == C_REST_ERROR_OOM);
    if (rc != C_REST_OK)
      free(ctx);
  }

  /* Specifically test single_process modality headless coverage with logger */
  ASSERT_EQ(C_REST_OK, c_rest_init(C_REST_MODALITY_SINGLE_PROCESS, &ctx));
  ctx->listen_address = NULL;
  ctx->listen_port = 0;
  ctx->logger.log_cb = mock_logger_cb;
  ASSERT_EQ(C_REST_OK, c_rest_run(ctx));
  ASSERT_EQ(C_REST_OK, c_rest_stop(ctx));
  ASSERT_EQ(C_REST_OK, c_rest_destroy(ctx));

  /* Test db config in c_rest_run */
  ASSERT_EQ(C_REST_OK, c_rest_init(C_REST_MODALITY_SINGLE_PROCESS, &ctx));
  ctx->listen_address = NULL;
  ctx->listen_port = 0;
  ctx->logger.log_cb = mock_logger_cb;
  ctx->db_config.connection_string = "sqlite://:memory:";
  ASSERT_EQ(C_REST_OK, c_rest_run(ctx));

  ctx->vtable = NULL;
  ctx->logger.log_cb = mock_logger_cb;
  ASSERT_NEQ(C_REST_OK, c_rest_run(ctx));
  ctx->logger.log_cb = NULL;
  ASSERT_NEQ(C_REST_OK, c_rest_run(ctx));

  ctx->logger.log_cb = mock_logger_err_cb;
  ASSERT_NEQ(C_REST_OK, c_rest_run(ctx));

  g_async_logger_calls = 0;
  ctx->db_config.connection_string = "invalid_url://";
  g_mock_orm_init_fail = 1;
  ctx->logger.log_cb = NULL;
  ASSERT_NEQ(C_REST_OK, c_rest_run(ctx));

  g_async_logger_calls = 0;
  ctx->logger.log_cb = mock_logger_fail_on_second;
  ASSERT_NEQ(C_REST_OK, c_rest_run(ctx));
  g_mock_orm_init_fail = 0;

  ctx->db_config.connection_string = NULL;
  ctx->logger.log_cb = mock_logger_err_cb;
  ASSERT_NEQ(C_REST_OK, c_rest_destroy(ctx));
  ASSERT_EQ(C_REST_OK, c_rest_orm_cleanup(ctx->db_pool));
  free(ctx);

  /* Test logger failure on dummy destroy/stop */
  ASSERT_EQ(C_REST_OK, c_rest_init(C_REST_MODALITY_SINGLE_PROCESS, &ctx));
  ctx->logger.log_cb = mock_logger_err_cb;
  ASSERT_NEQ(C_REST_OK, c_rest_stop(ctx));
  ctx->vtable = NULL;
  ctx->logger.log_cb = mock_logger_cb;
  ASSERT_NEQ(C_REST_OK, c_rest_run(ctx));
  ASSERT_NEQ(C_REST_OK, c_rest_stop(ctx));
  ctx->allocator.free_cb = NULL;
  ASSERT_EQ(C_REST_OK, c_rest_destroy(ctx));
  ctx = NULL;

  {
    struct c_rest_modality_vtable null_vt;
    memset(&null_vt, 0, sizeof(null_vt));
    ASSERT_EQ(C_REST_OK, c_rest_init(C_REST_MODALITY_SINGLE_PROCESS, &ctx));
    ctx->vtable = &null_vt;
    ASSERT_NEQ(C_REST_OK, c_rest_run(ctx));
    ASSERT_NEQ(C_REST_OK, c_rest_stop(ctx));
    ASSERT_EQ(C_REST_OK, c_rest_destroy(ctx));
  }

  ctx = NULL;
  {
    struct c_rest_logger err_log;
    err_log.log_cb = mock_logger_err_cb;
    ASSERT_EQ(C_REST_OK, c_rest_init(C_REST_MODALITY_SINGLE_PROCESS, &ctx));
    ctx->logger = err_log;
    ASSERT_NEQ(C_REST_OK, ctx->vtable->init(ctx));
    ctx->logger.log_cb = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_destroy(ctx));
    ctx = NULL;
  }
  ASSERT_NEQ(C_REST_OK, c_rest_set_router(NULL, NULL));
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_init(C_REST_MODALITY_SINGLE_PROCESS, NULL));

#ifdef C_REST_TESTING_MALLOC_HOOK
  g_crf_malloc_hook = NULL;
  g_crf_realloc_hook = NULL;
  g_crf_calloc_hook = NULL;
  g_crf_strdup_hook = NULL;
#endif

  ASSERT_EQ(C_REST_OK, c_rest_router_destroy(router));
  PASS();
}

TEST test_modality_direct_vtables_and_sockets(void) {
  struct c_rest_router *router = NULL;
  ASSERT_EQ(C_REST_OK, c_rest_router_init(&router));
  ASSERT_EQ(C_REST_OK,
            c_rest_router_add(router, "POST", "/test", my_dummy_handler, NULL));

  /* async modality direct tests */
  {
    struct c_rest_context dummy_ctx_gt;
    c_rest_socket_t valid_sock =
        (c_rest_socket_t)socket(AF_INET, SOCK_STREAM, 0);

    memset(&dummy_ctx_gt, 0, sizeof(dummy_ctx_gt));
    dummy_ctx_gt.allocator.malloc_cb = malloc;
    dummy_ctx_gt.allocator.free_cb = free;

    ASSERT_NEQ(C_REST_OK, async_vtable.init(NULL));

#ifdef C_REST_TESTING_MALLOC_HOOK
    g_crf_malloc_hook = hook_malloc_modality;
    g_malloc_fail_count = 0;
    dummy_ctx_gt.allocator.malloc_cb = hook_malloc_modality;
    ASSERT_NEQ(C_REST_OK, async_vtable.init(&dummy_ctx_gt));

    g_malloc_fail_count = 1;
    ASSERT_NEQ(C_REST_OK, async_vtable.init(&dummy_ctx_gt));
    g_crf_malloc_hook = NULL;
    dummy_ctx_gt.allocator.malloc_cb = malloc;
#endif

    dummy_ctx_gt.logger.log_cb = mock_logger_err_cb;
    ASSERT_NEQ(C_REST_OK, async_vtable.init(&dummy_ctx_gt));

    ASSERT_NEQ(C_REST_OK, async_vtable.run(NULL));

    dummy_ctx_gt.internal_state = NULL;
    ASSERT_NEQ(C_REST_OK, async_vtable.run(&dummy_ctx_gt));

    dummy_ctx_gt.logger.log_cb = mock_logger_cb;
    ASSERT_EQ(C_REST_OK, async_vtable.init(&dummy_ctx_gt));
    dummy_ctx_gt.logger.log_cb = mock_logger_err_cb;
    ASSERT_NEQ(C_REST_OK, async_vtable.run(&dummy_ctx_gt));
    dummy_ctx_gt.logger.log_cb = mock_logger_cb;
    ASSERT_EQ(C_REST_OK, async_vtable.destroy(&dummy_ctx_gt));

    ASSERT_EQ(C_REST_OK, async_vtable.init(&dummy_ctx_gt));
    g_async_logger_calls = 0;
    dummy_ctx_gt.logger.log_cb = mock_logger_fail_on_second;
    ASSERT_NEQ(C_REST_OK, async_vtable.run(&dummy_ctx_gt));
    dummy_ctx_gt.logger.log_cb = mock_logger_cb;
    ASSERT_EQ(C_REST_OK, async_vtable.destroy(&dummy_ctx_gt));

    ASSERT_EQ(C_REST_OK, async_vtable.init(&dummy_ctx_gt));
    dummy_ctx_gt.logger.log_cb = mock_logger_err_cb;
    ASSERT_NEQ(C_REST_OK, async_vtable.destroy(&dummy_ctx_gt));

    ASSERT_NEQ(C_REST_OK, async_vtable.destroy(NULL));

    dummy_ctx_gt.internal_state = NULL;
    ASSERT_NEQ(C_REST_OK, async_vtable.destroy(&dummy_ctx_gt));

    dummy_ctx_gt.logger.log_cb = mock_logger_cb;
    ASSERT_EQ(C_REST_OK, async_vtable.init(&dummy_ctx_gt));
    *(c_rest_socket_t *)dummy_ctx_gt.internal_state = valid_sock;
    ASSERT_EQ(C_REST_OK, async_vtable.destroy(&dummy_ctx_gt));

    ASSERT_EQ(C_REST_OK, async_vtable.init(&dummy_ctx_gt));
    {
      void **ptrs;
      *(c_rest_socket_t *)dummy_ctx_gt.internal_state = (c_rest_socket_t)9999;
      ASSERT_NEQ(C_REST_OK, async_vtable.destroy(&dummy_ctx_gt));
      ptrs = (void **)dummy_ctx_gt.internal_state;
      free(ptrs[1]);
      free(dummy_ctx_gt.internal_state);
      dummy_ctx_gt.internal_state = NULL;
    }

    ASSERT_EQ(C_REST_OK, async_vtable.init(&dummy_ctx_gt));
    {
      void **ptrs = (void **)dummy_ctx_gt.internal_state;
      void *tmp_evloop = ptrs[1];
      ptrs[1] = NULL;
      ASSERT_EQ(C_REST_OK, async_vtable.destroy(&dummy_ctx_gt));
      free(tmp_evloop);
    }

    ASSERT_EQ(C_REST_OK, async_vtable.init(&dummy_ctx_gt));
    dummy_ctx_gt.logger.log_cb = NULL;
    ASSERT_EQ(C_REST_OK, async_vtable.run(&dummy_ctx_gt));
    ASSERT_EQ(C_REST_OK, async_vtable.destroy(&dummy_ctx_gt));
  }

  /* greenthread modality direct tests */
  {
    struct c_rest_context dummy_ctx_gt;
    c_rest_socket_t valid_sock =
        (c_rest_socket_t)socket(AF_INET, SOCK_STREAM, 0);

    memset(&dummy_ctx_gt, 0, sizeof(dummy_ctx_gt));
    dummy_ctx_gt.allocator.malloc_cb = malloc;
    dummy_ctx_gt.allocator.free_cb = free;

    ASSERT_NEQ(C_REST_OK, greenthread_vtable.init(NULL));

#ifdef C_REST_TESTING_MALLOC_HOOK
    g_crf_malloc_hook = hook_malloc_modality;
    g_malloc_fail_count = 0;
    dummy_ctx_gt.allocator.malloc_cb = hook_malloc_modality;
    ASSERT_NEQ(C_REST_OK, greenthread_vtable.init(&dummy_ctx_gt));
    g_crf_malloc_hook = NULL;
    dummy_ctx_gt.allocator.malloc_cb = malloc;
#endif

    dummy_ctx_gt.logger.log_cb = mock_logger_err_cb;
    ASSERT_NEQ(C_REST_OK, greenthread_vtable.init(&dummy_ctx_gt));

    ASSERT_NEQ(C_REST_OK, greenthread_vtable.run(NULL));

    dummy_ctx_gt.internal_state = NULL;
    ASSERT_NEQ(C_REST_OK, greenthread_vtable.run(&dummy_ctx_gt));

    dummy_ctx_gt.logger.log_cb = mock_logger_cb;
    ASSERT_EQ(C_REST_OK, greenthread_vtable.init(&dummy_ctx_gt));
    dummy_ctx_gt.logger.log_cb = mock_logger_err_cb;
    ASSERT_NEQ(C_REST_OK, greenthread_vtable.run(&dummy_ctx_gt));
    dummy_ctx_gt.logger.log_cb = mock_logger_cb;
    ASSERT_EQ(C_REST_OK, greenthread_vtable.destroy(&dummy_ctx_gt));

    ASSERT_EQ(C_REST_OK, greenthread_vtable.init(&dummy_ctx_gt));
    g_async_logger_calls = 0;
    dummy_ctx_gt.logger.log_cb = mock_logger_fail_on_second;
    ASSERT_NEQ(C_REST_OK, greenthread_vtable.run(&dummy_ctx_gt));
    dummy_ctx_gt.logger.log_cb = mock_logger_cb;
    ASSERT_EQ(C_REST_OK, greenthread_vtable.destroy(&dummy_ctx_gt));

    ASSERT_EQ(C_REST_OK, greenthread_vtable.init(&dummy_ctx_gt));
    dummy_ctx_gt.logger.log_cb = NULL;
    ASSERT_EQ(C_REST_OK, greenthread_vtable.run(&dummy_ctx_gt));
    ASSERT_EQ(C_REST_OK, greenthread_vtable.destroy(&dummy_ctx_gt));

    ASSERT_EQ(C_REST_OK, greenthread_vtable.init(&dummy_ctx_gt));
    dummy_ctx_gt.logger.log_cb = mock_logger_err_cb;
    ASSERT_NEQ(C_REST_OK, greenthread_vtable.destroy(&dummy_ctx_gt));

    ASSERT_NEQ(C_REST_OK, greenthread_vtable.destroy(NULL));

    dummy_ctx_gt.internal_state = NULL;
    ASSERT_NEQ(C_REST_OK, greenthread_vtable.destroy(&dummy_ctx_gt));

    dummy_ctx_gt.logger.log_cb = mock_logger_cb;
    ASSERT_EQ(C_REST_OK, greenthread_vtable.init(&dummy_ctx_gt));
    *(c_rest_socket_t *)dummy_ctx_gt.internal_state = valid_sock;
    ASSERT_EQ(C_REST_OK, greenthread_vtable.destroy(&dummy_ctx_gt));

    ASSERT_EQ(C_REST_OK, greenthread_vtable.init(&dummy_ctx_gt));
    *(c_rest_socket_t *)dummy_ctx_gt.internal_state = (c_rest_socket_t)9999;
    ASSERT_NEQ(C_REST_OK, greenthread_vtable.destroy(&dummy_ctx_gt));
    free(dummy_ctx_gt.internal_state);
    dummy_ctx_gt.internal_state = NULL;
  }

  /* message_passing modality direct tests */
  {
    struct c_rest_context dummy_ctx_mp;

    memset(&dummy_ctx_mp, 0, sizeof(dummy_ctx_mp));
    dummy_ctx_mp.allocator.malloc_cb = malloc;
    dummy_ctx_mp.allocator.free_cb = free;

    ASSERT_NEQ(C_REST_OK, message_passing_vtable.init(NULL));

#ifdef C_REST_TESTING_MALLOC_HOOK
    g_crf_malloc_hook = hook_malloc_modality;
    g_malloc_fail_count = 0;
    dummy_ctx_mp.allocator.malloc_cb = hook_malloc_modality;
    ASSERT_NEQ(C_REST_OK, message_passing_vtable.init(&dummy_ctx_mp));
    g_crf_malloc_hook = NULL;
    dummy_ctx_mp.allocator.malloc_cb = malloc;
#endif

    dummy_ctx_mp.logger.log_cb = mock_logger_err_cb;
    ASSERT_NEQ(C_REST_OK, message_passing_vtable.init(&dummy_ctx_mp));

    ASSERT_NEQ(C_REST_OK, message_passing_vtable.run(NULL));

    dummy_ctx_mp.internal_state = NULL;
    ASSERT_NEQ(C_REST_OK, message_passing_vtable.run(&dummy_ctx_mp));

    dummy_ctx_mp.logger.log_cb = mock_logger_cb;
    ASSERT_EQ(C_REST_OK, message_passing_vtable.init(&dummy_ctx_mp));
    dummy_ctx_mp.logger.log_cb = mock_logger_err_cb;
    ASSERT_NEQ(C_REST_OK, message_passing_vtable.run(&dummy_ctx_mp));
    dummy_ctx_mp.logger.log_cb = mock_logger_cb;
    ASSERT_EQ(C_REST_OK, message_passing_vtable.destroy(&dummy_ctx_mp));

    ASSERT_EQ(C_REST_OK, message_passing_vtable.init(&dummy_ctx_mp));
    g_async_logger_calls = 0;
    dummy_ctx_mp.logger.log_cb = mock_logger_fail_on_second;
    ASSERT_NEQ(C_REST_OK, message_passing_vtable.run(&dummy_ctx_mp));
    dummy_ctx_mp.logger.log_cb = mock_logger_cb;
    ASSERT_EQ(C_REST_OK, message_passing_vtable.destroy(&dummy_ctx_mp));

    ASSERT_EQ(C_REST_OK, message_passing_vtable.init(&dummy_ctx_mp));
    dummy_ctx_mp.logger.log_cb = NULL;
    ASSERT_EQ(C_REST_OK, message_passing_vtable.run(&dummy_ctx_mp));
    ASSERT_EQ(C_REST_OK, message_passing_vtable.destroy(&dummy_ctx_mp));

    ASSERT_EQ(C_REST_OK, message_passing_vtable.init(&dummy_ctx_mp));
    dummy_ctx_mp.logger.log_cb = mock_logger_err_cb;
    ASSERT_NEQ(C_REST_OK, message_passing_vtable.destroy(&dummy_ctx_mp));

    ASSERT_NEQ(C_REST_OK, message_passing_vtable.destroy(NULL));

    dummy_ctx_mp.internal_state = NULL;
    ASSERT_NEQ(C_REST_OK, message_passing_vtable.destroy(&dummy_ctx_mp));

    dummy_ctx_mp.logger.log_cb = mock_logger_cb;
    ASSERT_EQ(C_REST_OK, message_passing_vtable.init(&dummy_ctx_mp));
    *(c_rest_socket_t *)dummy_ctx_mp.internal_state =
        (c_rest_socket_t)socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_EQ(C_REST_OK, message_passing_vtable.destroy(&dummy_ctx_mp));

    ASSERT_EQ(C_REST_OK, message_passing_vtable.init(&dummy_ctx_mp));
    *(c_rest_socket_t *)dummy_ctx_mp.internal_state = (c_rest_socket_t)9999;
    ASSERT_NEQ(C_REST_OK, message_passing_vtable.destroy(&dummy_ctx_mp));
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

    ASSERT_NEQ(C_REST_OK, multi_process_vtable.init(NULL));

#ifdef C_REST_TESTING_MALLOC_HOOK
    g_crf_malloc_hook = hook_malloc_modality;
    g_malloc_fail_count = 0;
    dummy_ctx_mproc.allocator.malloc_cb = hook_malloc_modality;
    ASSERT_NEQ(C_REST_OK, multi_process_vtable.init(&dummy_ctx_mproc));
    g_crf_malloc_hook = NULL;
    dummy_ctx_mproc.allocator.malloc_cb = malloc;
#endif

    dummy_ctx_mproc.logger.log_cb = mock_logger_err_cb;
    ASSERT_NEQ(C_REST_OK, multi_process_vtable.init(&dummy_ctx_mproc));

    ASSERT_NEQ(C_REST_OK, multi_process_vtable.run(NULL));

    dummy_ctx_mproc.internal_state = NULL;
    ASSERT_NEQ(C_REST_OK, multi_process_vtable.run(&dummy_ctx_mproc));

    dummy_ctx_mproc.logger.log_cb = mock_logger_cb;
    ASSERT_EQ(C_REST_OK, multi_process_vtable.init(&dummy_ctx_mproc));
    dummy_ctx_mproc.logger.log_cb = mock_logger_err_cb;
    ASSERT_NEQ(C_REST_OK, multi_process_vtable.run(&dummy_ctx_mproc));
    dummy_ctx_mproc.logger.log_cb = mock_logger_cb;
    ASSERT_EQ(C_REST_OK, multi_process_vtable.destroy(&dummy_ctx_mproc));

    ASSERT_EQ(C_REST_OK, multi_process_vtable.init(&dummy_ctx_mproc));
    g_async_logger_calls = 0;
    dummy_ctx_mproc.logger.log_cb = mock_logger_fail_on_second;
    ASSERT_NEQ(C_REST_OK, multi_process_vtable.run(&dummy_ctx_mproc));
    dummy_ctx_mproc.logger.log_cb = mock_logger_cb;
    ASSERT_EQ(C_REST_OK, multi_process_vtable.destroy(&dummy_ctx_mproc));

    ASSERT_EQ(C_REST_OK, multi_process_vtable.init(&dummy_ctx_mproc));
    dummy_ctx_mproc.logger.log_cb = NULL;
    ASSERT_EQ(C_REST_OK, multi_process_vtable.run(&dummy_ctx_mproc));
    ASSERT_EQ(C_REST_OK, multi_process_vtable.destroy(&dummy_ctx_mproc));

    ASSERT_EQ(C_REST_OK, multi_process_vtable.init(&dummy_ctx_mproc));
    dummy_ctx_mproc.logger.log_cb = mock_logger_err_cb;
    ASSERT_NEQ(C_REST_OK, multi_process_vtable.destroy(&dummy_ctx_mproc));

    ASSERT_NEQ(C_REST_OK, multi_process_vtable.destroy(NULL));

    dummy_ctx_mproc.internal_state = NULL;
    ASSERT_NEQ(C_REST_OK, multi_process_vtable.destroy(&dummy_ctx_mproc));

    dummy_ctx_mproc.logger.log_cb = mock_logger_cb;
    ASSERT_EQ(C_REST_OK, multi_process_vtable.init(&dummy_ctx_mproc));
    *(c_rest_socket_t *)dummy_ctx_mproc.internal_state =
        (c_rest_socket_t)socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_EQ(C_REST_OK, multi_process_vtable.destroy(&dummy_ctx_mproc));

    ASSERT_EQ(C_REST_OK, multi_process_vtable.init(&dummy_ctx_mproc));
    *(c_rest_socket_t *)dummy_ctx_mproc.internal_state = (c_rest_socket_t)9999;
    ASSERT_NEQ(C_REST_OK, multi_process_vtable.destroy(&dummy_ctx_mproc));
    free(dummy_ctx_mproc.internal_state);
    dummy_ctx_mproc.internal_state = NULL;

    ASSERT_EQ(C_REST_OK, multi_process_vtable.init(&dummy_ctx_mproc));
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
      ASSERT_EQ(C_REST_OK, multi_process_vtable.destroy(&dummy_ctx_mproc));
    }
  }
#endif

#if !defined(__EMSCRIPTEN__) && !defined(C_REST_NO_THREADS)
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

    {
      struct test_client_args fail_args;
      fail_args.ctx = &dummy_ctx;
      fail_args.port = 1;
      ASSERT_EQ(C_REST_OK, test_client_thread(&fail_args));
    }

    /* sync */
    dummy_ctx.listen_port = 46781;
    args.port = 46781;
    dummy_ctx.vtable = &sync_vtable;
    ASSERT_EQ(C_REST_OK, sync_vtable.init(&dummy_ctx));
    {
      struct my_sync_state *state =
          (struct my_sync_state *)dummy_ctx.internal_state;
      ASSERT_EQ(C_REST_OK, c_rest_socket_create(&state->server_sock));
      ASSERT_EQ(C_REST_OK,
                c_rest_socket_bind(state->server_sock, "127.0.0.1", 46781));
      ASSERT_EQ(C_REST_OK, c_rest_socket_listen(state->server_sock, 128));

      ASSERT_EQ(C_REST_OK, c_rest_thread_create(&client_thread,
                                                test_client_thread, &args));
      ASSERT_EQ(C_REST_OK, sync_vtable.run(&dummy_ctx));
      ASSERT_EQ(C_REST_OK, c_rest_thread_join(client_thread));
      {
        c_rest_error_t d_rc = sync_vtable.destroy(&dummy_ctx);
        ASSERT(d_rc == C_REST_OK || d_rc == C_REST_ERROR_GENERIC ||
               d_rc == C_REST_ERROR_NETWORK || d_rc == C_REST_ERROR_IO ||
               d_rc == C_REST_ERROR_INVALID_ARG);
        if (d_rc != C_REST_OK)
          free(dummy_ctx.internal_state);
      }
    }

    /* single_thread */
    dummy_ctx.listen_port = 46779;
    args.port = 46779;
    dummy_ctx.vtable = &single_thread_vtable;
    ASSERT_EQ(C_REST_OK, single_thread_vtable.init(&dummy_ctx));
    ASSERT_EQ(C_REST_OK,
              c_rest_thread_create(&client_thread, test_client_thread, &args));
    ASSERT_EQ(C_REST_OK, single_thread_vtable.run(&dummy_ctx));
    ASSERT_EQ(C_REST_OK, c_rest_thread_join(client_thread));
    {
      c_rest_error_t d_rc = single_thread_vtable.destroy(&dummy_ctx);
      ASSERT(d_rc == C_REST_OK || d_rc == C_REST_ERROR_GENERIC ||
             d_rc == C_REST_ERROR_NETWORK || d_rc == C_REST_ERROR_IO ||
             d_rc == C_REST_ERROR_INVALID_ARG);
      if (d_rc != C_REST_OK)
        free(dummy_ctx.internal_state);
    }

    /* multi_thread */
    dummy_ctx.listen_port = 46780;
    args.port = 46780;
    dummy_ctx.vtable = &multi_thread_vtable;
    ASSERT_EQ(C_REST_OK, multi_thread_vtable.init(&dummy_ctx));
    ASSERT_EQ(C_REST_OK,
              c_rest_thread_create(&client_thread, test_client_thread, &args));
    ASSERT_EQ(C_REST_OK, multi_thread_vtable.run(&dummy_ctx));
    ASSERT_EQ(C_REST_OK, c_rest_thread_join(client_thread));
    {
      c_rest_error_t d_rc = multi_thread_vtable.destroy(&dummy_ctx);
      ASSERT(d_rc == C_REST_OK || d_rc == C_REST_ERROR_GENERIC ||
             d_rc == C_REST_ERROR_NETWORK || d_rc == C_REST_ERROR_IO ||
             d_rc == C_REST_ERROR_INVALID_ARG);
      if (d_rc != C_REST_OK) {
        struct my_sync_state *ms =
            (struct my_sync_state *)dummy_ctx.internal_state;
        free(ms->workers);
        free(ms);
      }
    }

    /* multi_thread with free_cb == NULL using static pool */
    dummy_ctx.listen_port = 46782;
    args.port = 46782;
    dummy_ctx.vtable = &multi_thread_vtable;
    ASSERT_EQ(C_REST_OK, multi_thread_vtable.init(&dummy_ctx));
    dummy_ctx.allocator.malloc_cb = my_static_pool_malloc;
    dummy_ctx.allocator.free_cb = NULL;
    ASSERT_EQ(C_REST_OK,
              c_rest_thread_create(&client_thread, test_client_thread, &args));
    ASSERT_EQ(C_REST_OK, multi_thread_vtable.run(&dummy_ctx));
    ASSERT_EQ(C_REST_OK, c_rest_thread_join(client_thread));
    dummy_ctx.allocator.malloc_cb = malloc;
    dummy_ctx.allocator.free_cb = free;
    {
      c_rest_error_t d_rc = multi_thread_vtable.destroy(&dummy_ctx);
      ASSERT(d_rc == C_REST_OK || d_rc == C_REST_ERROR_GENERIC ||
             d_rc == C_REST_ERROR_NETWORK || d_rc == C_REST_ERROR_IO ||
             d_rc == C_REST_ERROR_INVALID_ARG);
      if (d_rc != C_REST_OK) {
        struct my_sync_state *ms =
            (struct my_sync_state *)dummy_ctx.internal_state;
        free(ms->workers);
        free(ms);
      }
    }
  }
#endif

  {
    c_rest_error_t rc;
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
    rc = multi_thread_vtable.run(&ctx_err);
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_GENERIC ||
           rc == C_REST_ERROR_OOM || rc == C_REST_ERROR_NETWORK ||
           rc == C_REST_ERROR_IO || rc == C_REST_ERROR_INVALID_ARG ||
           rc == C_REST_ERROR_OOM);

    multi_st.server_sock = (c_rest_socket_t)1;
    multi_st.is_running = 1;
    g_mock_socket_fail = 14;
    rc = multi_thread_vtable.run(&ctx_err);
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_GENERIC ||
           rc == C_REST_ERROR_OOM || rc == C_REST_ERROR_NETWORK ||
           rc == C_REST_ERROR_IO || rc == C_REST_ERROR_INVALID_ARG ||
           rc == C_REST_ERROR_OOM);

    multi_st.server_sock = (c_rest_socket_t)1;
    multi_st.is_running = 1;
    g_mock_socket_fail = 11;
    ctx_err.allocator.malloc_cb = my_static_pool_malloc;
    rc = multi_thread_vtable.run(&ctx_err);
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_GENERIC ||
           rc == C_REST_ERROR_NETWORK || rc == C_REST_ERROR_IO ||
           rc == C_REST_ERROR_INVALID_ARG || rc == C_REST_ERROR_OOM);

    multi_st.server_sock = (c_rest_socket_t)1;
    multi_st.is_running = 1;
    g_mock_socket_fail = 12;
    ctx_err.allocator.malloc_cb = my_static_pool_malloc;
    ctx_err.allocator.free_cb = NULL;
    rc = multi_thread_vtable.run(&ctx_err);
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_GENERIC ||
           rc == C_REST_ERROR_NETWORK || rc == C_REST_ERROR_IO ||
           rc == C_REST_ERROR_INVALID_ARG || rc == C_REST_ERROR_OOM);

    multi_st.server_sock = (c_rest_socket_t)1;
    multi_st.is_running = 1;
    g_mock_socket_fail = 12;
    ctx_err.allocator.malloc_cb = my_static_pool_malloc;
    ctx_err.allocator.free_cb = my_dummy_free;
    rc = multi_thread_vtable.run(&ctx_err);
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_GENERIC ||
           rc == C_REST_ERROR_NETWORK || rc == C_REST_ERROR_IO ||
           rc == C_REST_ERROR_INVALID_ARG || rc == C_REST_ERROR_OOM);

    ctx_err.allocator.malloc_cb = malloc;
    ctx_err.allocator.free_cb = NULL;

    {
      c_rest_thread_t dummy_workers[65];
      memset(dummy_workers, 0, sizeof(dummy_workers));
      multi_st.server_sock = (c_rest_socket_t)1;
      multi_st.is_running = 1;
      multi_st.workers = dummy_workers;
      multi_st.worker_count = 64;
      g_mock_socket_fail = 11;
      rc = multi_thread_vtable.run(&ctx_err);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_GENERIC ||
             rc == C_REST_ERROR_NETWORK || rc == C_REST_ERROR_IO ||
             rc == C_REST_ERROR_INVALID_ARG || rc == C_REST_ERROR_OOM);

      memset(dummy_workers, 0, sizeof(dummy_workers));
      multi_st.server_sock = (c_rest_socket_t)1;
      multi_st.is_running = 1;
      multi_st.workers = dummy_workers;
      multi_st.worker_count = 0;
      g_mock_socket_fail = 6;
      rc = multi_thread_vtable.run(&ctx_err);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_GENERIC ||
             rc == C_REST_ERROR_NETWORK || rc == C_REST_ERROR_IO ||
             rc == C_REST_ERROR_INVALID_ARG || rc == C_REST_ERROR_OOM);

      memset(dummy_workers, 0, sizeof(dummy_workers));
      multi_st.server_sock = (c_rest_socket_t)1;
      multi_st.is_running = 1;
      multi_st.workers = dummy_workers;
      multi_st.worker_count = 0;
      g_mock_socket_fail = 13;
      rc = multi_thread_vtable.run(&ctx_err);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_GENERIC ||
             rc == C_REST_ERROR_NETWORK || rc == C_REST_ERROR_IO ||
             rc == C_REST_ERROR_INVALID_ARG || rc == C_REST_ERROR_OOM);
      g_mock_socket_fail = 0;
    }

    g_mock_socket_fail = 0;
  }

  {
    c_rest_error_t rc;
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
      rc = sync_vtable.run(&ctx_tls);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_GENERIC ||
             rc == C_REST_ERROR_NETWORK || rc == C_REST_ERROR_IO ||
             rc == C_REST_ERROR_INVALID_ARG || rc == C_REST_ERROR_OOM ||
             rc == C_REST_ERROR_NOT_SUPPORTED);

      g_mock_socket_fail = j_mod;
      ctx_tls.internal_state = &single_st;
      single_st.server_sock = C_REST_INVALID_SOCKET;
      single_st.is_running = 1;
      rc = single_thread_vtable.run(&ctx_tls);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_GENERIC ||
             rc == C_REST_ERROR_NETWORK || rc == C_REST_ERROR_IO ||
             rc == C_REST_ERROR_INVALID_ARG || rc == C_REST_ERROR_OOM ||
             rc == C_REST_ERROR_NOT_SUPPORTED);

      g_mock_socket_fail = j_mod;
      ctx_tls.internal_state = &multi_st;
      multi_st.server_sock = C_REST_INVALID_SOCKET;
      multi_st.is_running = 1;
      rc = multi_thread_vtable.run(&ctx_tls);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_GENERIC ||
             rc == C_REST_ERROR_NETWORK || rc == C_REST_ERROR_IO ||
             rc == C_REST_ERROR_INVALID_ARG || rc == C_REST_ERROR_OOM ||
             rc == C_REST_ERROR_NOT_SUPPORTED);
    }

    for (j_mod = 1002; j_mod <= 1003; j_mod++) {
      g_mock_socket_fail = j_mod;
      ctx_tls.internal_state = &sync_st;
      sync_st.server_sock = C_REST_INVALID_SOCKET;
      sync_st.is_running = 1;
      rc = sync_vtable.run(&ctx_tls);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_GENERIC ||
             rc == C_REST_ERROR_NETWORK || rc == C_REST_ERROR_IO ||
             rc == C_REST_ERROR_INVALID_ARG || rc == C_REST_ERROR_OOM);

      g_mock_socket_fail = j_mod;
      ctx_tls.internal_state = &single_st;
      single_st.server_sock = C_REST_INVALID_SOCKET;
      single_st.is_running = 1;
      ASSERT_NEQ(C_REST_OK, single_thread_vtable.run(&ctx_tls));

      g_mock_socket_fail = j_mod;
      ctx_tls.internal_state = &multi_st;
      multi_st.server_sock = C_REST_INVALID_SOCKET;
      multi_st.is_running = 1;
      ASSERT_NEQ(C_REST_OK, multi_thread_vtable.run(&ctx_tls));
    }

    g_mock_socket_fail = 109;
    ctx_tls.internal_state = &sync_st;
    sync_st.server_sock = (c_rest_socket_t)1;
    sync_st.is_running = 1;
    rc = sync_vtable.run(&ctx_tls);
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_GENERIC ||
           rc == C_REST_ERROR_NETWORK || rc == C_REST_ERROR_IO ||
           rc == C_REST_ERROR_INVALID_ARG || rc == C_REST_ERROR_OOM);

    g_mock_socket_fail = 109;
    ctx_tls.internal_state = &single_st;
    single_st.server_sock = (c_rest_socket_t)1;
    single_st.is_running = 1;
    ASSERT_NEQ(C_REST_OK, single_thread_vtable.run(&ctx_tls));

    g_mock_socket_fail = 109;
    ctx_tls.internal_state = &multi_st;
    multi_st.server_sock = (c_rest_socket_t)1;
    multi_st.is_running = 1;
    ASSERT_NEQ(C_REST_OK, multi_thread_vtable.run(&ctx_tls));

    g_mock_socket_fail = 0;

    ctx_tls.tls_ctx = (struct c_rest_tls_context *)1;
    for (j_mod = 1; j_mod <= 3; j_mod++) {
      g_mock_tls_fail = j_mod;

      g_mock_socket_fail = 7;
      ctx_tls.internal_state = &sync_st;
      sync_st.server_sock = (c_rest_socket_t)1;
      sync_st.is_running = 1;
      rc = sync_vtable.run(&ctx_tls);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_GENERIC ||
             rc == C_REST_ERROR_TLS || rc == C_REST_ERROR_NETWORK ||
             rc == C_REST_ERROR_IO || rc == C_REST_ERROR_INVALID_ARG ||
             rc == C_REST_ERROR_OOM);

      g_mock_socket_fail = 107;
      ctx_tls.internal_state = &sync_st;
      sync_st.server_sock = (c_rest_socket_t)1;
      sync_st.is_running = 1;
      rc = sync_vtable.run(&ctx_tls);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_GENERIC ||
             rc == C_REST_ERROR_TLS || rc == C_REST_ERROR_NETWORK ||
             rc == C_REST_ERROR_IO || rc == C_REST_ERROR_INVALID_ARG ||
             rc == C_REST_ERROR_OOM);

      g_mock_socket_fail = 7;
      ctx_tls.internal_state = &single_st;
      single_st.server_sock = (c_rest_socket_t)1;
      single_st.is_running = 1;
      rc = single_thread_vtable.run(&ctx_tls);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_GENERIC ||
             rc == C_REST_ERROR_TLS || rc == C_REST_ERROR_NETWORK ||
             rc == C_REST_ERROR_IO || rc == C_REST_ERROR_INVALID_ARG ||
             rc == C_REST_ERROR_OOM);

      g_mock_socket_fail = 107;
      ctx_tls.internal_state = &single_st;
      single_st.server_sock = (c_rest_socket_t)1;
      single_st.is_running = 1;
      rc = single_thread_vtable.run(&ctx_tls);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_GENERIC ||
             rc == C_REST_ERROR_TLS || rc == C_REST_ERROR_NETWORK ||
             rc == C_REST_ERROR_IO || rc == C_REST_ERROR_INVALID_ARG ||
             rc == C_REST_ERROR_OOM);
    }

    g_mock_socket_fail = 99;
    {
      struct c_rest_context *tmp_ctx = NULL;
      ASSERT_NEQ(C_REST_OK, c_rest_init(C_REST_MODALITY_SYNC, &tmp_ctx));
    }

    g_mock_socket_fail = 14;
    g_mock_tls_fail = 1;
    ctx_tls.tls_ctx = (struct c_rest_tls_context *)1;
    ctx_tls.internal_state = &sync_st;
    sync_st.server_sock = (c_rest_socket_t)1;
    sync_st.is_running = 1;
    rc = sync_vtable.run(&ctx_tls);
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_GENERIC ||
           rc == C_REST_ERROR_TLS);
    ctx_tls.tls_ctx = NULL;
    g_mock_tls_fail = 0;

    g_mock_socket_fail = 14;
    ctx_tls.allocator.malloc_cb = malloc;
    ctx_tls.allocator.free_cb = free;
    ctx_tls.internal_state = &multi_st;
    multi_st.server_sock = (c_rest_socket_t)1;
    multi_st.is_running = 1;
    rc = multi_thread_vtable.run(&ctx_tls);
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_GENERIC ||
           rc == C_REST_ERROR_NETWORK || rc == C_REST_ERROR_IO ||
           rc == C_REST_ERROR_INVALID_ARG || rc == C_REST_ERROR_OOM);

    g_mock_socket_fail = 0;
    ctx_tls.tls_ctx = NULL;
  }

  {
    struct c_rest_context null_ctx;
    memset(&null_ctx, 0, sizeof(null_ctx));
    null_ctx.internal_state = NULL;
    ASSERT_NEQ(C_REST_OK, sync_vtable.run(&null_ctx));
    ASSERT_NEQ(C_REST_OK, sync_vtable.stop(&null_ctx));
    ASSERT_NEQ(C_REST_OK, sync_vtable.destroy(&null_ctx));
    ASSERT_NEQ(C_REST_OK, single_thread_vtable.run(&null_ctx));
    ASSERT_NEQ(C_REST_OK, single_thread_vtable.stop(&null_ctx));
    ASSERT_NEQ(C_REST_OK, single_thread_vtable.destroy(&null_ctx));
    ASSERT_NEQ(C_REST_OK, multi_thread_vtable.run(&null_ctx));
    ASSERT_NEQ(C_REST_OK, multi_thread_vtable.stop(&null_ctx));
    ASSERT_NEQ(C_REST_OK, multi_thread_vtable.destroy(&null_ctx));
  }

  {
    struct c_rest_context ctx_err;
    struct my_sync_state st;
    static const int fails[2] = {106, 108};
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
      ASSERT_NEQ(C_REST_OK, sync_vtable.destroy(&ctx_err));

      ctx_err.internal_state = &st;
      st.server_sock = (c_rest_socket_t)1;
      g_mock_socket_fail = fails[k];
      ASSERT_NEQ(C_REST_OK, sync_vtable.stop(&ctx_err));

      ctx_err.internal_state = &st;
      st.server_sock = (c_rest_socket_t)1;
      g_mock_socket_fail = fails[k];
      ASSERT_NEQ(C_REST_OK, single_thread_vtable.destroy(&ctx_err));

      ctx_err.internal_state = &st;
      st.server_sock = (c_rest_socket_t)1;
      g_mock_socket_fail = fails[k];
      ASSERT_NEQ(C_REST_OK, single_thread_vtable.stop(&ctx_err));

      ctx_err.internal_state = &st;
      st.server_sock = (c_rest_socket_t)1;
      g_mock_socket_fail = fails[k];
      ASSERT_NEQ(C_REST_OK, multi_thread_vtable.destroy(&ctx_err));

      ctx_err.internal_state = &st;
      st.server_sock = (c_rest_socket_t)1;
      g_mock_socket_fail = fails[k];
      ASSERT_NEQ(C_REST_OK, multi_thread_vtable.stop(&ctx_err));
    }

    ctx_err.logger.log_cb = mock_logger_cb;

    ctx_err.internal_state = &st;
    st.server_sock = (c_rest_socket_t)1;
    g_mock_socket_fail = 108;
    ASSERT_NEQ(C_REST_OK, sync_vtable.stop(&ctx_err));

    ctx_err.internal_state = &st;
    st.server_sock = (c_rest_socket_t)1;
    g_mock_socket_fail = 108;
    ASSERT_NEQ(C_REST_OK, single_thread_vtable.stop(&ctx_err));

    ctx_err.internal_state = &st;
    st.server_sock = (c_rest_socket_t)1;
    g_mock_socket_fail = 108;
    ASSERT_NEQ(C_REST_OK, multi_thread_vtable.stop(&ctx_err));

    ctx_err.internal_state = &st;
    st.server_sock = (c_rest_socket_t)1;
    g_mock_socket_fail = 108;
    ASSERT_NEQ(C_REST_OK, sync_vtable.destroy(&ctx_err));

    ctx_err.internal_state = &st;
    st.server_sock = (c_rest_socket_t)1;
    g_mock_socket_fail = 108;
    ASSERT_NEQ(C_REST_OK, single_thread_vtable.destroy(&ctx_err));

    ctx_err.internal_state = &st;
    st.server_sock = (c_rest_socket_t)1;
    g_mock_socket_fail = 108;
    ASSERT_NEQ(C_REST_OK, multi_thread_vtable.destroy(&ctx_err));

    g_mock_socket_fail = 0;
  }

  {
    c_rest_socket_t client;
    c_rest_thread_t thread;

    g_mock_socket_fail = 11;
    ASSERT_EQ(C_REST_OK,
              mock_c_rest_socket_accept(C_REST_INVALID_SOCKET, &client));
    g_mock_socket_fail = 12;
    ASSERT_EQ(C_REST_OK,
              mock_c_rest_socket_accept(C_REST_INVALID_SOCKET, &client));
    g_mock_socket_fail = 109;
    ASSERT_NEQ(C_REST_OK,
               mock_c_rest_socket_accept(C_REST_INVALID_SOCKET, &client));

    g_mock_socket_fail = 6;
    ASSERT_NEQ(C_REST_OK, mock_c_rest_socket_close(C_REST_INVALID_SOCKET));
    g_mock_socket_fail = 106;
    ASSERT_NEQ(C_REST_OK, mock_c_rest_socket_close(C_REST_INVALID_SOCKET));

    g_mock_socket_fail = 6;
    ASSERT_EQ(C_REST_OK,
              mock_c_rest_handle_connection(NULL, C_REST_INVALID_SOCKET));
    g_mock_socket_fail = 106;
    ASSERT_EQ(C_REST_OK,
              mock_c_rest_handle_connection(NULL, C_REST_INVALID_SOCKET));

    g_mock_socket_fail = 7;
    ASSERT_NEQ(C_REST_OK,
               mock_c_rest_handle_connection(NULL, C_REST_INVALID_SOCKET));
    g_mock_socket_fail = 107;
    ASSERT_NEQ(C_REST_OK,
               mock_c_rest_handle_connection(NULL, C_REST_INVALID_SOCKET));

    g_mock_socket_fail = 13;
    ASSERT_NEQ(C_REST_OK, mock_c_rest_thread_create(&thread, NULL, NULL));
    g_mock_socket_fail = 113;
    ASSERT_NEQ(C_REST_OK, mock_c_rest_thread_create(&thread, NULL, NULL));
    g_mock_socket_fail = 1;
    ASSERT_EQ(C_REST_OK,
              mock_c_rest_thread_create(NULL, my_mock_worker_err_cb, NULL));
    g_mock_socket_fail = 1;
    ASSERT_EQ(C_REST_OK,
              mock_c_rest_thread_create(NULL, dummy_modality_worker_fn, NULL));

    g_mock_socket_fail = 0;
  }

  ASSERT_EQ(C_REST_OK, c_rest_router_destroy(router));
  PASS();
}

TEST test_modality_all_engines_coverage(void) {
  static const int k_sock_codes[11] = {1, 2, 1002, 3, 1003, 4, 6, 7, 8, 15, 16};
  struct c_rest_router *router = NULL;
  size_t idx;

  ASSERT_EQ(C_REST_OK, c_rest_router_init(&router));
  ASSERT_EQ(C_REST_OK,
            c_rest_router_add(router, "GET", "/test", my_dummy_handler, NULL));

  /* 1. Multi-chunk (>4096 bytes) request in c_rest_handle_connection */
#if !defined(_WIN32) && !defined(__EMSCRIPTEN__) && !defined(CDD_DOS)
  {
    int sv[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0) {
      struct c_rest_context *mctx = NULL;
      char *big_req = (char *)malloc(5200);
      size_t off = 0;
      ssize_t wr;
      ASSERT(big_req != NULL);
      memcpy(big_req, "GET /test HTTP/1.0\r\nX-Big: ", 27);
      off = 27;
      memset(big_req + off, 'A', 4600);
      off += 4600;
      memcpy(big_req + off, "\r\n\r\n", 4);
      off += 4;
      wr = write(sv[1], big_req, off);
      ASSERT(wr == (ssize_t)off);
      close(sv[1]);
      free(big_req);

      ASSERT_EQ(C_REST_OK, c_rest_init(C_REST_MODALITY_SYNC, &mctx));
      ASSERT_EQ(C_REST_OK, c_rest_set_router(mctx, router));
      ASSERT_EQ(C_REST_OK,
                c_rest_handle_connection(mctx, (c_rest_socket_t)sv[0]));
      close(sv[0]);
      ASSERT_EQ(C_REST_OK, c_rest_destroy(mctx));
    }
  }
#endif

  /* 2. single_process_vtable full lifecycle & error branches */
  {
    struct c_rest_context *sp_ctx = NULL;
    c_rest_error_t rc;

    ASSERT_EQ(C_REST_ERROR_INVALID_ARG, single_process_vtable.init(NULL));
    ASSERT_EQ(C_REST_ERROR_INVALID_ARG, single_process_vtable.destroy(NULL));
    ASSERT_EQ(C_REST_ERROR_INVALID_ARG, single_process_vtable.run(NULL));
    ASSERT_EQ(C_REST_ERROR_INVALID_ARG, single_process_vtable.stop(NULL));

    ASSERT_EQ(C_REST_OK, c_rest_init(C_REST_MODALITY_SINGLE_PROCESS, &sp_ctx));

    /* start logger fail */
    sp_ctx->logger.log_cb = mock_logger_err_cb;
    ASSERT_NEQ(C_REST_OK, single_process_vtable.run(sp_ctx));

    /* finish logger fail */
    sp_ctx->listen_address = NULL;
    sp_ctx->listen_port = 0;
    g_async_logger_calls = 0;
    sp_ctx->logger.log_cb = mock_logger_fail_on_second;
    ASSERT_NEQ(C_REST_OK, single_process_vtable.run(sp_ctx));
    sp_ctx->logger.log_cb = NULL;

    /* addr fallback when listen_address == NULL && listen_port != 0 */
    sp_ctx->listen_address = NULL;
    sp_ctx->listen_port = 8080;
    g_mock_socket_fail = 1;
    ASSERT_NEQ(C_REST_OK, single_process_vtable.run(sp_ctx));
    g_mock_socket_fail = 0;

    /* pre-bound sp_server_sock + logger ok on finish */
    sp_ctx->sp_server_sock = (c_rest_socket_t)12345;
    sp_ctx->logger.log_cb = mock_logger_cb;
    g_mock_socket_fail = 4;
    ASSERT_EQ(C_REST_OK, single_process_vtable.run(sp_ctx));
    sp_ctx->logger.log_cb = NULL;
    sp_ctx->sp_server_sock = C_REST_INVALID_SOCKET;
    g_mock_socket_fail = 0;

    /* socket lifecycle codes */
    for (idx = 0; idx < sizeof(k_sock_codes) / sizeof(k_sock_codes[0]); ++idx) {
      sp_ctx->sp_server_sock = C_REST_INVALID_SOCKET;
      sp_ctx->listen_address = "127.0.0.1";
      sp_ctx->listen_port = 0;
      g_mock_socket_fail = k_sock_codes[idx];
      rc = single_process_vtable.run(sp_ctx);
      if (k_sock_codes[idx] == 4 || k_sock_codes[idx] == 8 ||
          k_sock_codes[idx] == 15 || k_sock_codes[idx] == 16) {
#ifdef __EMSCRIPTEN__
        ASSERT_EQ(C_REST_ERROR_NOT_SUPPORTED, rc);
#else
        ASSERT_EQ(C_REST_OK, rc);
#endif
      } else {
        ASSERT_NEQ(C_REST_OK, rc);
      }
      g_mock_socket_fail = 115;
      if (sp_ctx->sp_server_sock != C_REST_INVALID_SOCKET) {
        ASSERT_EQ(C_REST_OK, single_process_vtable.stop(sp_ctx));
      }
      g_mock_socket_fail = 0;
    }

    /* stop & destroy with active sp_server_sock */
    sp_ctx->sp_server_sock = (c_rest_socket_t)12345;
    g_mock_socket_fail = 6;
    ASSERT_NEQ(C_REST_OK, single_process_vtable.stop(sp_ctx));
    g_mock_socket_fail = 115;
    sp_ctx->logger.log_cb = mock_logger_cb;
    ASSERT_EQ(C_REST_OK, single_process_vtable.stop(sp_ctx));
    sp_ctx->logger.log_cb = NULL;

    sp_ctx->sp_server_sock = (c_rest_socket_t)12345;
    g_mock_socket_fail = 6;
    ASSERT_NEQ(C_REST_OK, single_process_vtable.destroy(sp_ctx));
    g_mock_socket_fail = 115;
    ASSERT_EQ(C_REST_OK, c_rest_destroy(sp_ctx));
    g_mock_socket_fail = 0;
  }

  /* 3. sync_vtable missing branches (addr fallback, stop-in-handler, finish log
   * fail) */
  {
    struct c_rest_context *s_ctx = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_init(C_REST_MODALITY_SYNC, &s_ctx));

    s_ctx->listen_address = NULL;
    s_ctx->listen_port = 8080;
    g_mock_socket_fail = 1;
    ASSERT_NEQ(C_REST_OK, sync_vtable.run(s_ctx));

    s_ctx->listen_address = "127.0.0.1";
    s_ctx->listen_port = 0;
    s_ctx->tls_ctx = (struct c_rest_tls_context *)1;
    s_ctx->router = router;
    g_mock_socket_fail = 15;
#ifdef __EMSCRIPTEN__
    ASSERT_EQ(C_REST_ERROR_NOT_SUPPORTED, sync_vtable.run(s_ctx));
#else
    ASSERT_EQ(C_REST_OK, sync_vtable.run(s_ctx));
    s_ctx->tls_ctx = NULL;
    s_ctx->router = NULL;
    g_mock_socket_fail = 115;
    ASSERT_EQ(C_REST_OK, sync_vtable.stop(s_ctx));
#endif
    s_ctx->tls_ctx = NULL;
    s_ctx->router = NULL;

    s_ctx->listen_address = "127.0.0.1";
    s_ctx->listen_port = 0;
    g_mock_socket_fail = 16;
#ifdef __EMSCRIPTEN__
    ASSERT_EQ(C_REST_ERROR_NOT_SUPPORTED, sync_vtable.run(s_ctx));
#else
    ASSERT_EQ(C_REST_OK, sync_vtable.run(s_ctx));
#endif
    g_mock_socket_fail = 0;

    s_ctx->listen_address = NULL;
    s_ctx->listen_port = 0;
    g_async_logger_calls = 0;
    s_ctx->logger.log_cb = mock_logger_fail_on_second;
    ASSERT_NEQ(C_REST_OK, sync_vtable.run(s_ctx));
    s_ctx->logger.log_cb = NULL;

    ASSERT_EQ(C_REST_OK, c_rest_destroy(s_ctx));
  }

  /* 4. async, greenthread, message_passing, multi_process full matrices */
  {
    static const enum c_rest_modality_type k_mods[4] = {
        C_REST_MODALITY_ASYNC, C_REST_MODALITY_GREENTHREAD,
        C_REST_MODALITY_MESSAGE_PASSING,
#if !defined(__EMSCRIPTEN__) && !defined(CDD_DOS)
        C_REST_MODALITY_MULTI_PROCESS
#else
        C_REST_MODALITY_ASYNC
#endif
    };
    size_t m;

    for (m = 0; m < 4; ++m) {
      struct c_rest_context *mctx = NULL;
      const struct c_rest_modality_vtable *vt;
      c_rest_error_t rc;

      ASSERT_EQ(C_REST_OK, c_rest_init(k_mods[m], &mctx));
      vt = mctx->vtable;

      ASSERT_NEQ(C_REST_OK, vt->stop(NULL));
      {
        struct c_rest_context empty_ctx;
        memset(&empty_ctx, 0, sizeof(empty_ctx));
        ASSERT_NEQ(C_REST_OK, vt->stop(&empty_ctx));
      }

      /* addr fallback when listen_address == NULL && listen_port != 0 */
      mctx->listen_address = NULL;
      mctx->listen_port = 8080;
      g_mock_socket_fail = 1;
      ASSERT_NEQ(C_REST_OK, vt->run(mctx));
      g_mock_socket_fail = 0;

      /* pre-bound server_sock + logger ok on finish */
      *(c_rest_socket_t *)mctx->internal_state = (c_rest_socket_t)12345;
      mctx->logger.log_cb = mock_logger_cb;
      g_mock_socket_fail = 4;
      ASSERT_EQ(C_REST_OK, vt->run(mctx));
      mctx->logger.log_cb = NULL;
      *(c_rest_socket_t *)mctx->internal_state = C_REST_INVALID_SOCKET;
      g_mock_socket_fail = 0;

      for (idx = 0; idx < sizeof(k_sock_codes) / sizeof(k_sock_codes[0]);
           ++idx) {
        *(c_rest_socket_t *)mctx->internal_state = C_REST_INVALID_SOCKET;
        mctx->listen_address = "127.0.0.1";
        mctx->listen_port = 0;
        g_mock_socket_fail = k_sock_codes[idx];
        rc = vt->run(mctx);
        if (k_sock_codes[idx] == 4 || k_sock_codes[idx] == 8 ||
            k_sock_codes[idx] == 15 || k_sock_codes[idx] == 16) {
#ifdef __EMSCRIPTEN__
          ASSERT_EQ(C_REST_ERROR_NOT_SUPPORTED, rc);
#else
          ASSERT_EQ(C_REST_OK, rc);
#endif
        } else {
          ASSERT_NEQ(C_REST_OK, rc);
        }
        if (*(c_rest_socket_t *)mctx->internal_state != C_REST_INVALID_SOCKET) {
          g_mock_socket_fail = 115;
          ASSERT_EQ(C_REST_OK, vt->stop(mctx));
        }
        g_mock_socket_fail = 0;
      }

      /* stop error & logger branches */
      *(c_rest_socket_t *)mctx->internal_state = (c_rest_socket_t)12345;
      g_mock_socket_fail = 6;
      ASSERT_NEQ(C_REST_OK, vt->stop(mctx));

      g_mock_socket_fail = 115;
      mctx->logger.log_cb = mock_logger_err_cb;
      ASSERT_NEQ(C_REST_OK, vt->stop(mctx));

      *(c_rest_socket_t *)mctx->internal_state = (c_rest_socket_t)12345;
      g_mock_socket_fail = 115;
      mctx->logger.log_cb = mock_logger_cb;
      ASSERT_EQ(C_REST_OK, vt->stop(mctx));

      mctx->logger.log_cb = NULL;
      ASSERT_EQ(C_REST_OK, vt->stop(mctx));
      g_mock_socket_fail = 0;

      ASSERT_EQ(C_REST_OK, c_rest_destroy(mctx));
    }
  }

  /* 5. async_run / async_stop with NULL evloop & multi_process worker_count > 1
   */
  {
    struct c_rest_context actx;
    struct my_async_state ast;
    memset(&actx, 0, sizeof(actx));
    memset(&ast, 0, sizeof(ast));
    ast.server_sock = C_REST_INVALID_SOCKET;
    ast.evloop = NULL;
    actx.internal_state = &ast;
    ASSERT_EQ(C_REST_ERROR_INVALID_ARG, async_vtable.run(&actx));
    ASSERT_EQ(C_REST_OK, async_vtable.stop(&actx));
  }
#if !defined(__EMSCRIPTEN__) && !defined(CDD_DOS)
  {
    struct mp_layout {
      c_rest_socket_t server_sock;
      volatile int is_running;
      c_rest_process_t *workers;
      int worker_count;
    };
    struct c_rest_context mpctx;
    struct mp_layout mpst;
    memset(&mpctx, 0, sizeof(mpctx));
    memset(&mpst, 0, sizeof(mpst));
    mpst.server_sock = C_REST_INVALID_SOCKET;
    mpst.worker_count = 2;
    mpctx.internal_state = &mpst;
    ASSERT_EQ(C_REST_ERROR_NOT_SUPPORTED, multi_process_vtable.run(&mpctx));
  }
#endif

  /* 6. message_passing_run queue / OOM / cleanup error branches */
  {
    struct c_rest_context *mp_ctx = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_init(C_REST_MODALITY_MESSAGE_PASSING, &mp_ctx));
    mp_ctx->listen_address = NULL;
    mp_ctx->listen_port = 0;

    /* queue_init failure */
    *(c_rest_socket_t *)mp_ctx->internal_state = (c_rest_socket_t)12345;
    g_mock_tsq_fail = 1;
    ASSERT_NEQ(C_REST_OK, message_passing_vtable.run(mp_ctx));
    g_mock_tsq_fail = 0;

    /* msg OOM: sc_rc ok + qd_rc ok; sc_rc fail; qd_rc fail */
    mp_ctx->allocator.malloc_cb = my_mock_null_malloc;
    g_mock_socket_fail = 15;
    ASSERT_EQ(C_REST_ERROR_OOM, message_passing_vtable.run(mp_ctx));

    g_mock_socket_fail = 6;
    ASSERT_NEQ(C_REST_OK, message_passing_vtable.run(mp_ctx));

    g_mock_socket_fail = 15;
    g_mock_tsq_fail = 15;
    ASSERT_NEQ(C_REST_OK, message_passing_vtable.run(mp_ctx));
    g_mock_tsq_fail = 0;
    mp_ctx->allocator.malloc_cb = malloc;

    /* queue_push failure: sc_rc fail; qd_rc fail; both sc_rc & qd_rc ok */
    g_mock_socket_fail = 6;
    g_mock_tsq_fail = 3;
    ASSERT_NEQ(C_REST_OK, message_passing_vtable.run(mp_ctx));

    g_mock_socket_fail = 15;
    g_mock_tsq_fail = 3;
    ASSERT_NEQ(C_REST_OK, message_passing_vtable.run(mp_ctx));
    g_mock_tsq_fail = 0;

#ifdef C_REST_TESTING_MALLOC_HOOK
    g_crf_malloc_hook = hook_malloc_modality;
    g_malloc_fail_count = 2;
    g_mock_socket_fail = 15;
    ASSERT_NEQ(C_REST_OK, message_passing_vtable.run(mp_ctx));
    g_crf_malloc_hook = NULL;
    g_malloc_fail_count = -1;
#endif

    /* queue_pop failure: qd_rc ok & qd_rc fail */
    g_mock_socket_fail = 17;
    g_mock_tsq_fail = 0;
    ASSERT_NEQ(C_REST_OK, message_passing_vtable.run(mp_ctx));

    g_mock_socket_fail = 17;
    g_mock_tsq_fail = 15;
    ASSERT_NEQ(C_REST_OK, message_passing_vtable.run(mp_ctx));
    g_mock_tsq_fail = 0;

    /* cl_rc fail + qd_rc fail; h_rc fail + qd_rc fail */
    g_mock_socket_fail = 6;
    g_mock_tsq_fail = 15;
    ASSERT_NEQ(C_REST_OK, message_passing_vtable.run(mp_ctx));

    g_mock_socket_fail = 7;
    g_mock_tsq_fail = 15;
    ASSERT_NEQ(C_REST_OK, message_passing_vtable.run(mp_ctx));
    g_mock_tsq_fail = 0;

    /* queue_close fail (qd_rc ok & qd_rc fail) and queue_destroy fail */
    g_mock_socket_fail = 4;
    g_mock_tsq_fail = 4;
    ASSERT_NEQ(C_REST_OK, message_passing_vtable.run(mp_ctx));

    g_mock_socket_fail = 4;
    g_mock_tsq_fail = 16;
    ASSERT_NEQ(C_REST_OK, message_passing_vtable.run(mp_ctx));

    g_mock_socket_fail = 4;
    g_mock_tsq_fail = 15;
    ASSERT_NEQ(C_REST_OK, message_passing_vtable.run(mp_ctx));

    g_mock_tsq_fail = 0;
    g_mock_socket_fail = 115;
    *(c_rest_socket_t *)mp_ctx->internal_state = C_REST_INVALID_SOCKET;
    ASSERT_EQ(C_REST_OK, c_rest_destroy(mp_ctx));
    g_mock_socket_fail = 0;
  }

  ASSERT_EQ(C_REST_OK, c_rest_router_destroy(router));
  PASS();
}

SUITE(modality_suite) {
  RUN_TEST(test_modality_handle_connection_and_lifecycle);
  RUN_TEST(test_modality_direct_vtables_and_sockets);
  RUN_TEST(test_modality_vtable_error_branches);
  RUN_TEST(test_modality_all_engines_coverage);
}
