

/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "test_protos.h"
#include "c_rest_modality.h"
#include "c_rest_router.h"
#include "c_rest_platform.h"
#include "c_rest_response.h"
#include <stdio.h>
#include <string.h>
#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

#endif
/* clang-format on */

#ifdef C_REST_TESTING_MALLOC_HOOK
extern int g_mock_orm_init_fail;
#endif

extern const struct c_rest_modality_vtable sync_vtable;
extern const struct c_rest_modality_vtable single_thread_vtable;
extern const struct c_rest_modality_vtable multi_thread_vtable;
extern const struct c_rest_modality_vtable async_vtable;

static void my_dummy_free(void *ptr) { (void)ptr; }

static c_rest_error_t my_mock_logger_err_cb_internal(const char *msg) {
  (void)msg;
  return C_REST_ERROR_GENERIC;
}

static c_rest_error_t my_mock_logger_ok_cb_internal(const char *msg) {
  (void)msg;
  return C_REST_OK;
}

struct my_sync_state {
  c_rest_socket_t server_sock;
  int is_running;
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
  if (sync_vtable.init) {
    sync_vtable.init(&ctx);
  }
  if (single_thread_vtable.init) {
    single_thread_vtable.init(&ctx);
  }
  if (multi_thread_vtable.init) {
    multi_thread_vtable.init(&ctx);
  }
  if (async_vtable.init) {
    async_vtable.init(&ctx);
  }

  /* 3. Null ctx */
  if (sync_vtable.init) {
    sync_vtable.init(NULL);
  }
  if (sync_vtable.run) {
    sync_vtable.run(NULL);
  }
  if (sync_vtable.stop) {
    sync_vtable.stop(NULL);
  }
  if (sync_vtable.destroy) {
    sync_vtable.destroy(NULL);
  }

  if (single_thread_vtable.init) {
    single_thread_vtable.init(NULL);
  }
  if (single_thread_vtable.run) {
    single_thread_vtable.run(NULL);
  }
  if (single_thread_vtable.stop) {
    single_thread_vtable.stop(NULL);
  }
  if (single_thread_vtable.destroy) {
    single_thread_vtable.destroy(NULL);
  }

  if (multi_thread_vtable.init) {
    multi_thread_vtable.init(NULL);
  }
  if (multi_thread_vtable.run) {
    multi_thread_vtable.run(NULL);
  }
  if (multi_thread_vtable.stop) {
    multi_thread_vtable.stop(NULL);
  }
  if (multi_thread_vtable.destroy) {
    multi_thread_vtable.destroy(NULL);
  }

  if (async_vtable.init) {
    async_vtable.init(NULL);
  }
  if (async_vtable.run) {
    async_vtable.run(NULL);
  }
  if (async_vtable.stop) {
    async_vtable.stop(NULL);
  }
  if (async_vtable.destroy) {
    async_vtable.destroy(NULL);
  }

  /* 4. Trigger accept failure in sync_run by passing a 9999 socket */
  ctx.logger.log_cb = NULL;
  ctx.internal_state = &sync_st;
  sync_st.server_sock = (c_rest_socket_t)9999;
  sync_st.is_running = 1;
  if (sync_vtable.run) {
    sync_vtable.run(&ctx);
  }

  /* Trigger bind failure in single/multi/async run */
  ctx.listen_address = "invalid_address_for_test";

  ctx.internal_state = &single_st;
  single_st.server_sock = C_REST_INVALID_SOCKET;
  if (single_thread_vtable.run) {
    single_thread_vtable.run(&ctx);
  }

  ctx.internal_state = &multi_st;
  multi_st.server_sock = C_REST_INVALID_SOCKET;
  if (multi_thread_vtable.run) {
    multi_thread_vtable.run(&ctx);
  }

  ctx.internal_state = &async_st;
  async_st.server_sock = C_REST_INVALID_SOCKET;
  async_st.evloop = &dummy_evloop;
  if (async_vtable.run) {
    async_vtable.run(&ctx);
  }

  /* 5. Trigger logger failure in run */
  ctx.logger.log_cb = my_mock_logger_err_cb_internal;

  ctx.internal_state = &sync_st;
  sync_st.is_running = 1;
  sync_st.server_sock = (c_rest_socket_t)9999;
  if (sync_vtable.run) {
    sync_vtable.run(&ctx);
  }

  ctx.internal_state = &single_st;
  single_st.is_running = 1;
  single_st.server_sock = (c_rest_socket_t)9999;
  if (single_thread_vtable.run) {
    single_thread_vtable.run(&ctx);
  }

  ctx.internal_state = &multi_st;
  multi_st.is_running = 1;
  multi_st.server_sock = (c_rest_socket_t)9999;
  if (multi_thread_vtable.run) {
    multi_thread_vtable.run(&ctx);
  }

  ctx.internal_state = &async_st;
  async_st.evloop = &dummy_evloop;
  async_st.server_sock = (c_rest_socket_t)9999;
  if (async_vtable.run) {
    async_vtable.run(&ctx);
  }

  /* 6. Trigger logger failure in stop */
  ctx.internal_state = &sync_st;
  if (sync_vtable.stop) {
    sync_vtable.stop(&ctx);
  }

  ctx.internal_state = &single_st;
  if (single_thread_vtable.stop) {
    single_thread_vtable.stop(&ctx);
  }

  ctx.internal_state = &multi_st;
  if (multi_thread_vtable.stop) {
    multi_thread_vtable.stop(&ctx);
  }

  ctx.internal_state = &async_st;
  if (async_vtable.stop) {
    async_vtable.stop(&ctx);
  }

  /* 7. Trigger socket close failure in destroy */
  ctx.logger.log_cb = NULL;

  ctx.internal_state = &sync_st;
  sync_st.server_sock = (c_rest_socket_t)9999;
  if (sync_vtable.destroy) {
    sync_vtable.destroy(&ctx);
  }

  ctx.internal_state = &single_st;
  single_st.server_sock = (c_rest_socket_t)9999;
  if (single_thread_vtable.destroy) {
    single_thread_vtable.destroy(&ctx);
  }

  ctx.internal_state = &multi_st;
  multi_st.server_sock = (c_rest_socket_t)9999;
  if (multi_thread_vtable.destroy) {
    multi_thread_vtable.destroy(&ctx);
  }

  ctx.internal_state = &async_st;
  async_st.evloop = &dummy_evloop;
  async_st.server_sock = (c_rest_socket_t)9999;
  if (async_vtable.destroy) {
    async_vtable.destroy(&ctx);
  }

  /* 8. Trigger logger failure in destroy */
  ctx.logger.log_cb = my_mock_logger_err_cb_internal;

  ctx.internal_state = &sync_st;
  sync_st.server_sock = C_REST_INVALID_SOCKET;
  if (sync_vtable.destroy) {
    sync_vtable.destroy(&ctx);
  }

  ctx.internal_state = &single_st;
  single_st.server_sock = C_REST_INVALID_SOCKET;
  if (single_thread_vtable.destroy) {
    single_thread_vtable.destroy(&ctx);
  }

  ctx.internal_state = &multi_st;
  multi_st.server_sock = C_REST_INVALID_SOCKET;
  if (multi_thread_vtable.destroy) {
    multi_thread_vtable.destroy(&ctx);
  }

  ctx.internal_state = &async_st;
  async_st.server_sock = C_REST_INVALID_SOCKET;
  async_st.evloop = &dummy_evloop;
  if (async_vtable.destroy) {
    async_vtable.destroy(&ctx);
  }

  /* 9. Trigger logger SUCCESS in init, run, stop, destroy */
  ctx.logger.log_cb = my_mock_logger_ok_cb_internal;

  if (sync_vtable.init) {
    sync_vtable.init(&ctx);
  }

  ctx.internal_state = &sync_st;
  sync_st.server_sock = (c_rest_socket_t)9999;
  sync_st.is_running = 1;
  if (sync_vtable.run) {
    sync_vtable.run(&ctx);
  }

  if (sync_vtable.stop) {
    sync_vtable.stop(&ctx);
  }

  sync_st.server_sock = C_REST_INVALID_SOCKET;
  if (sync_vtable.destroy) {
    sync_vtable.destroy(&ctx);
  }
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
  if (g_malloc_fail_count == 0)
    return NULL;
  if (g_malloc_fail_count > 0)
    g_malloc_fail_count--;
#if defined(_WIN32)
  return _strdup(str);
#else
  return strdup(str);
#endif
}
#endif

struct test_client_args {
  struct c_rest_context *ctx;
  int port;
};
static void my_sigalrm(int sig) { (void)sig; }
static c_rest_error_t test_client_thread(void *arg) {
  struct test_client_args *args = (struct test_client_args *)arg;
#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
  int sock;
  struct sockaddr_in srv_addr;
  int retries = 50;
  usleep(100000);
  while (retries-- > 0) {
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock >= 0) {
      memset(&srv_addr, 0, sizeof(srv_addr));
      srv_addr.sin_family = AF_INET;
      srv_addr.sin_port = htons(args->port);
      srv_addr.sin_addr.s_addr = htonl(0x7F000001);
      if (connect(sock, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) == 0) {
        const char *req =
            "GET / HTTP/1.1\r\nHost: loc\r\nConnection: close\r\n\r\n";
        send(sock, req, strlen(req), 0);
        usleep(50000);
        c_rest_stop(args->ctx);
        close(sock);
        {
          int j;
          for (j = 0; j < 8; j++) {
            sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock >= 0) {
              connect(sock, (struct sockaddr *)&srv_addr, sizeof(srv_addr));
              close(sock);
            }
          }
        }
        break;
      }
      close(sock);
    }
    usleep(10000);
  }
#endif
  return C_REST_OK;
}

static int g_async_logger_calls = 0;
static c_rest_error_t mock_logger_fail_on_second(const char *msg) {
  (void)msg;
  g_async_logger_calls++;
  if (g_async_logger_calls == 2)
    return C_REST_ERROR_GENERIC;
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
  enum c_rest_modality_type modalities[] = {C_REST_MODALITY_SYNC,
                                            C_REST_MODALITY_ASYNC,
                                            C_REST_MODALITY_MULTI_THREAD,
                                            C_REST_MODALITY_SINGLE_THREAD,
                                            C_REST_MODALITY_MULTI_PROCESS,
                                            C_REST_MODALITY_GREENTHREAD,
                                            C_REST_MODALITY_MESSAGE_PASSING,
                                            C_REST_MODALITY_SINGLE_PROCESS};
  int num_modalities = 8;
  int i;
  int rc;
  c_rest_socket_t client_sock = C_REST_INVALID_SOCKET;
  c_rest_socket_t server_sock = C_REST_INVALID_SOCKET;
  c_rest_socket_t accepted_sock = C_REST_INVALID_SOCKET;

#ifdef C_REST_TESTING_MALLOC_HOOK
  g_crf_malloc_hook = hook_malloc_modality;
  g_crf_realloc_hook = hook_realloc_modality;
  g_crf_calloc_hook = hook_calloc_modality;
  g_crf_strdup_hook = hook_strdup_modality;
#endif

  c_rest_router_init(&router);
  c_rest_router_add(router, "POST", "/test", my_dummy_handler, NULL);

  /* Test c_rest_handle_connection parsing coverage */
  rc = c_rest_init(C_REST_MODALITY_SINGLE_PROCESS, &ctx);
  if (rc == C_REST_OK) {
    int fds[2];
    ctx->router = router;
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0) {
      accepted_sock = (c_rest_socket_t)(intptr_t)fds[0];
      client_sock = (c_rest_socket_t)(intptr_t)fds[1];

      /* Send a complete valid request to cover parsing callbacks */
      {
        const char *req = "POST /test HTTP/1.1\r\nHost: loc\r\nContent-Length: "
                          "5\r\n\r\nhello";
        size_t wr = 0;
        c_rest_socket_send(client_sock, req, strlen(req), &wr);
        c_rest_socket_close(client_sock);
        c_rest_handle_connection(ctx, accepted_sock);
        c_rest_socket_close(accepted_sock);

        if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0) {
          accepted_sock = (c_rest_socket_t)(intptr_t)fds[0];
          client_sock = (c_rest_socket_t)(intptr_t)fds[1];
          c_rest_socket_send(client_sock, req, strlen(req), &wr);
          c_rest_socket_close(client_sock);
          ctx->router = NULL;
          c_rest_handle_connection(ctx, accepted_sock);
          ctx->router = router;
          c_rest_socket_close(accepted_sock);

          if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0) {
            accepted_sock = (c_rest_socket_t)(intptr_t)fds[0];
            client_sock = (c_rest_socket_t)(intptr_t)fds[1];
            c_rest_socket_send(client_sock, req, strlen(req), &wr);
            c_rest_socket_close(client_sock);
            ctx->tls_ctx = (void *)1;
            c_rest_handle_connection(ctx, accepted_sock);
            ctx->tls_ctx = NULL;
            c_rest_socket_close(accepted_sock);
          }
        }
      }
    }
    c_rest_destroy(ctx);
  }

  /* Test c_rest_handle_connection with OOM in parsing callbacks */
  rc = c_rest_init(C_REST_MODALITY_SINGLE_PROCESS, &ctx);
  if (rc == C_REST_OK) {
    ctx->router = router;
#ifdef C_REST_TESTING_MALLOC_HOOK
    {
      int oom_idx;
      for (oom_idx = 0; oom_idx < 150; oom_idx++) {
        int fds[2];
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0) {
          accepted_sock = (c_rest_socket_t)(intptr_t)fds[0];
          client_sock = (c_rest_socket_t)(intptr_t)fds[1];
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
      }
      g_malloc_fail_count = -1;
      g_crf_realloc_hook = NULL;
      g_crf_calloc_hook = NULL;
      g_crf_strdup_hook = NULL;

      {
        int fds[2];
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0) {
          accepted_sock = (c_rest_socket_t)(intptr_t)fds[0];
          client_sock = (c_rest_socket_t)(intptr_t)fds[1];
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
    }
#endif
    c_rest_destroy(ctx);
  }

  /* Test c_rest_handle_connection with TLS coverage */
  rc = c_rest_init(C_REST_MODALITY_SINGLE_PROCESS, &ctx);
  if (rc == C_REST_OK) {
    int fds[2];
    ctx->router = router;
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0) {
      accepted_sock = (c_rest_socket_t)(intptr_t)fds[0];
      client_sock = (c_rest_socket_t)(intptr_t)fds[1];

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

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0) {
      accepted_sock = (c_rest_socket_t)(intptr_t)fds[0];
      client_sock = (c_rest_socket_t)(intptr_t)fds[1];

      /* Send an invalid request */
      {
        const char *req = "INVALID REQUEST\r\n\r\n";
        size_t wr = 0;
        c_rest_socket_send(client_sock, req, strlen(req), &wr);
        c_rest_socket_close(client_sock);
        c_rest_handle_connection(ctx, accepted_sock);
        c_rest_socket_close(accepted_sock);
      }
    }

    c_rest_destroy(ctx);
  }

  /* Test invalid c_rest_handle_connection */
  c_rest_handle_connection(NULL, C_REST_INVALID_SOCKET);

  /* Test invalid modality */
  failed +=
      ((c_rest_init((enum c_rest_modality_type)999, &ctx) == C_REST_OK) != 0);

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
    if ((c_rest_init(mod, NULL) == C_REST_OK) != 0) {
      printf("Failed at %d\n", __LINE__);
      failed++;
    }
    /* LCOV_EXCL_LINE */
#ifdef C_REST_TESTING_MALLOC_HOOK
    /* OOM on c_rest_init itself */
    g_malloc_fail_count = 0;
    if ((c_rest_init(mod, &ctx) == C_REST_OK) != 0) {
      printf("Failed at %d\n", __LINE__);
      failed++;
    }

    if (mod != C_REST_MODALITY_SINGLE_PROCESS) {
      g_malloc_fail_count = 1;
      if ((c_rest_init(mod, &ctx) == C_REST_OK) != 0) {
        printf("Failed at %d\n", __LINE__);
        failed++;
      }
    }
#endif
    /* LCOV_EXCL_LINE */
    g_malloc_fail_count = -1;

    rc = c_rest_init(mod, &ctx);
    if (rc != C_REST_OK) {
      printf("INIT FAILED %d\n", rc);
      failed++;
    }

    /* Invalid run */
    if ((c_rest_run(NULL) == C_REST_OK) != 0) {
      printf("Failed at %d\n", __LINE__);
      failed++;
    }

    /* Invalid stop */
    if ((c_rest_stop(NULL) == C_REST_OK) != 0) {
      printf("Failed at %d\n", __LINE__);
      failed++;
    }
    /* LCOV_EXCL_LINE */
    /* Stop (should be safe to call before run, or just set flag) */
    c_rest_stop(ctx);

    /* Run with invalid host to fail bind immediately and not hang */ /* LCOV_EXCL_LINE
                                                                       */
    ctx->listen_address = "invalid_address_for_test";
    ctx->listen_port = 9999;
    ctx->logger.log_cb = mock_logger_cb;
    c_rest_run(ctx);

#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
    /* Run with multiplatform mock */
    {
      extern int g_accept_calls;
      g_accept_calls = 0;
      ctx->listen_address = "127.0.0.1";
      ctx->listen_port = 8080;
      ctx->cm_env = (cm_env_t)1;
      c_rest_run(ctx);
    }
#endif

    /* Destroy invalid */
    if ((c_rest_destroy(NULL) == C_REST_OK) != 0) {
      printf("Failed at %d\n", __LINE__);
      failed++;
    }

    /* Valid destroy */
    rc = c_rest_destroy(ctx);
    if ((rc != C_REST_OK) != 0) {
      printf("Failed at %d\n", __LINE__);
      failed++;
    }

    /* Test with logger error on init */ /* LCOV_EXCL_LINE */
    rc = c_rest_init(mod, &ctx);
    if (rc == C_REST_OK) {
      ctx->logger.log_cb = mock_logger_err_cb;
      c_rest_destroy(ctx);
    }
  }

  /* Specifically test dummy modality coverage with logger */
  rc = c_rest_init(C_REST_MODALITY_SINGLE_PROCESS, &ctx);
  if (rc == C_REST_OK) {
    ctx->logger.log_cb = mock_logger_cb;
    /* run should cover dummy_run */
    c_rest_run(ctx);
    /* stop should cover dummy_stop */
    c_rest_stop(ctx);
    /* destroy covers dummy_destroy */
    c_rest_destroy(ctx);
  }

  /* Test db config in c_rest_run */
  rc = c_rest_init(C_REST_MODALITY_SINGLE_PROCESS, &ctx);
  if (rc == C_REST_OK) {
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
  }

  /* Test logger failure on dummy destroy/stop */
  rc = c_rest_init(C_REST_MODALITY_SINGLE_PROCESS, &ctx);
  if (rc == C_REST_OK) {
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
  }

  {
    struct c_rest_modality_vtable null_vt = {0};
    rc = c_rest_init(C_REST_MODALITY_SINGLE_PROCESS, &ctx);
    if (rc == C_REST_OK) {
      ctx->vtable = &null_vt;
      c_rest_run(ctx);
      c_rest_stop(ctx);
      c_rest_destroy(ctx);
    }
  }

  /* Test dummy init logger failure */
  ctx = NULL;
  {
    struct c_rest_logger err_log;
    err_log.log_cb = mock_logger_err_cb;
    rc = c_rest_init(C_REST_MODALITY_SINGLE_PROCESS, &ctx);
    if (rc == C_REST_OK) {
      ctx->logger = err_log;
      /* Can't easily test dummy_init logger failure through c_rest_init because
         it creates a new context each time and sets logger AFTER init. But we
         can test it indirectly by observing it never hits dummy_init since
         c_rest_init nulls logger initially. To cover dummy_init logger failure,
         we would need a hook or mock. Wait, c_rest_init calls
         ctx->vtable->init(ctx) with an empty logger. So dummy_init log_cb check
         is never false unless we manually invoke it. Let's manually invoke the
         vtable. */
      if (ctx->vtable && ctx->vtable->init) {
        ctx->vtable->init(ctx);
      }
      c_rest_destroy(ctx);
    }
  }
  c_rest_set_router(NULL, NULL);
  if (ctx)
    c_rest_set_router(ctx, NULL);

  /* Test get_vtable null out_vtable coverage indirectly (would need direct
   * call, but we can't. We can test c_rest_init null context) */
  rc = c_rest_init(C_REST_MODALITY_SINGLE_PROCESS, NULL);
  if ((rc != C_REST_ERROR_GENERIC) != 0) {
    printf("Failed at %d\n", __LINE__);
    failed++;
  }

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
    extern const struct c_rest_modality_vtable async_vtable;
    struct c_rest_context dummy_ctx_gt;
    c_rest_error_t rc_async;
#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
    c_rest_socket_t valid_sock =
        (c_rest_socket_t)(intptr_t)socket(AF_INET, SOCK_STREAM, 0);
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
    rc_async = async_vtable.init(&dummy_ctx_gt);
    if (rc_async == C_REST_OK) {
      dummy_ctx_gt.logger.log_cb = mock_logger_err_cb;
      async_vtable.run(&dummy_ctx_gt);
      dummy_ctx_gt.logger.log_cb = mock_logger_cb;
      async_vtable.destroy(&dummy_ctx_gt);
    }

    /* run logger failure end */
    rc_async = async_vtable.init(&dummy_ctx_gt);
    if (rc_async == C_REST_OK) {
      g_async_logger_calls = 0;
      dummy_ctx_gt.logger.log_cb = mock_logger_fail_on_second;
      async_vtable.run(&dummy_ctx_gt);
      dummy_ctx_gt.logger.log_cb = mock_logger_cb;
      async_vtable.destroy(&dummy_ctx_gt);
    }

    /* destroy logger failure */
    rc_async = async_vtable.init(&dummy_ctx_gt);
    if (rc_async == C_REST_OK) {
      dummy_ctx_gt.logger.log_cb = mock_logger_err_cb;
      async_vtable.destroy(&dummy_ctx_gt);
      /* free leaked internal_state since destroy bailed early */
      if (dummy_ctx_gt.internal_state) {
        void **ptrs = (void **)dummy_ctx_gt.internal_state;
        if (ptrs[1])
          free(ptrs[1]);
        free(dummy_ctx_gt.internal_state);
      }
    }

    /* async_destroy NULL */
    async_vtable.destroy(NULL);

    /* async_destroy no internal_state */
    dummy_ctx_gt.internal_state = NULL;
    async_vtable.destroy(&dummy_ctx_gt);

    /* async_destroy with valid server_sock socket */
    dummy_ctx_gt.logger.log_cb = mock_logger_cb;
    rc_async = async_vtable.init(&dummy_ctx_gt);
    if (rc_async == C_REST_OK) {
      *(c_rest_socket_t *)dummy_ctx_gt.internal_state = valid_sock;
      async_vtable.destroy(&dummy_ctx_gt);
    }

    /* async_destroy with invalid server_sock to trigger socket close error */
    rc_async = async_vtable.init(&dummy_ctx_gt);
    if (rc_async == C_REST_OK) {
#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
      *(c_rest_socket_t *)dummy_ctx_gt.internal_state =
          (c_rest_socket_t)(intptr_t)9999;
#else
      *(c_rest_socket_t *)dummy_ctx_gt.internal_state = (c_rest_socket_t)9999;
#endif
      rc_async = async_vtable.destroy(&dummy_ctx_gt);
      /* free leaked internal_state since destroy bailed early */
      if (dummy_ctx_gt.internal_state) {
        void **ptrs = (void **)dummy_ctx_gt.internal_state;
        if (ptrs[1])
          free(ptrs[1]);
        free(dummy_ctx_gt.internal_state);
      }
    }

    /* async_destroy with evloop == NULL */
    rc_async = async_vtable.init(&dummy_ctx_gt);
    if (rc_async == C_REST_OK) {
      void **ptrs = (void **)dummy_ctx_gt.internal_state;
      void *tmp_evloop =
          ptrs[1]; /* evloop is second member (after size_t or c_rest_socket_t
                      which is 4 or 8 bytes) wait! c_rest_socket_t could be 4 or
                      8 bytes, so pointer might be unaligned? No, on 64-bit,
                      c_rest_socket_t (intptr_t) is 8 bytes, so ptrs[1] is
                      correct */
      ptrs[1] = NULL;
      async_vtable.destroy(&dummy_ctx_gt);
      free(tmp_evloop);
    }

    /* async_run with logger == NULL */
    rc_async = async_vtable.init(&dummy_ctx_gt);
    if (rc_async == C_REST_OK) {
      dummy_ctx_gt.logger.log_cb = NULL;
      async_vtable.run(&dummy_ctx_gt);
      async_vtable.destroy(&dummy_ctx_gt);
    }
  }

  /* greenthread modality direct tests */
  {
    extern const struct c_rest_modality_vtable greenthread_vtable;
    struct c_rest_context dummy_ctx_gt;
    c_rest_error_t rc_gt;
#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
    c_rest_socket_t valid_sock =
        (c_rest_socket_t)(intptr_t)socket(AF_INET, SOCK_STREAM, 0);
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
    rc_gt = greenthread_vtable.init(&dummy_ctx_gt);
    if (rc_gt == C_REST_OK) {
      dummy_ctx_gt.logger.log_cb = mock_logger_err_cb;
      greenthread_vtable.run(&dummy_ctx_gt);
      dummy_ctx_gt.logger.log_cb = mock_logger_cb;
      greenthread_vtable.destroy(&dummy_ctx_gt);
    }

    /* run logger failure end */
    rc_gt = greenthread_vtable.init(&dummy_ctx_gt);
    if (rc_gt == C_REST_OK) {
      g_async_logger_calls = 0;
      dummy_ctx_gt.logger.log_cb = mock_logger_fail_on_second;
      greenthread_vtable.run(&dummy_ctx_gt);
      dummy_ctx_gt.logger.log_cb = mock_logger_cb;
      greenthread_vtable.destroy(&dummy_ctx_gt);
    }

    /* run with logger == NULL */
    rc_gt = greenthread_vtable.init(&dummy_ctx_gt);
    if (rc_gt == C_REST_OK) {
      dummy_ctx_gt.logger.log_cb = NULL;
      greenthread_vtable.run(&dummy_ctx_gt);
      greenthread_vtable.destroy(&dummy_ctx_gt);
    }

    /* destroy logger failure */
    rc_gt = greenthread_vtable.init(&dummy_ctx_gt);
    if (rc_gt == C_REST_OK) {
      dummy_ctx_gt.logger.log_cb = mock_logger_err_cb;
      greenthread_vtable.destroy(&dummy_ctx_gt);
      /* free leaked internal_state since destroy bailed early */
      if (dummy_ctx_gt.internal_state) {
        free(dummy_ctx_gt.internal_state);
      }
    }

    /* destroy NULL */
    greenthread_vtable.destroy(NULL);

    /* destroy no internal_state */
    dummy_ctx_gt.internal_state = NULL;
    greenthread_vtable.destroy(&dummy_ctx_gt);

    /* destroy with valid server_sock socket */
    dummy_ctx_gt.logger.log_cb = mock_logger_cb;
    rc_gt = greenthread_vtable.init(&dummy_ctx_gt);
    if (rc_gt == C_REST_OK) {
      *(c_rest_socket_t *)dummy_ctx_gt.internal_state = valid_sock;
      greenthread_vtable.destroy(&dummy_ctx_gt);
    }

    /* destroy with invalid server_sock to trigger socket close error */
    rc_gt = greenthread_vtable.init(&dummy_ctx_gt);
    if (rc_gt == C_REST_OK) {
#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
      *(c_rest_socket_t *)dummy_ctx_gt.internal_state =
          (c_rest_socket_t)(intptr_t)9999;
#else
      *(c_rest_socket_t *)dummy_ctx_gt.internal_state = (c_rest_socket_t)9999;
#endif
      rc_gt = greenthread_vtable.destroy(&dummy_ctx_gt);
      /* free leaked internal_state since destroy bailed early */
      if (dummy_ctx_gt.internal_state) {
        free(dummy_ctx_gt.internal_state);
      }
    }
  }

  /* message_passing modality direct tests */
  {
    extern const struct c_rest_modality_vtable message_passing_vtable;
    struct c_rest_context dummy_ctx_mp;
    c_rest_error_t rc_mp;

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
    rc_mp = message_passing_vtable.init(&dummy_ctx_mp);
    if (rc_mp == C_REST_OK) {
      dummy_ctx_mp.logger.log_cb = mock_logger_err_cb;
      message_passing_vtable.run(&dummy_ctx_mp);
      dummy_ctx_mp.logger.log_cb = mock_logger_cb;
      message_passing_vtable.destroy(&dummy_ctx_mp);
    }

    /* run logger failure end */
    rc_mp = message_passing_vtable.init(&dummy_ctx_mp);
    if (rc_mp == C_REST_OK) {
      g_async_logger_calls = 0;
      dummy_ctx_mp.logger.log_cb = mock_logger_fail_on_second;
      message_passing_vtable.run(&dummy_ctx_mp);
      dummy_ctx_mp.logger.log_cb = mock_logger_cb;
      message_passing_vtable.destroy(&dummy_ctx_mp);
    }

    /* run with logger == NULL */
    rc_mp = message_passing_vtable.init(&dummy_ctx_mp);
    if (rc_mp == C_REST_OK) {
      dummy_ctx_mp.logger.log_cb = NULL;
      message_passing_vtable.run(&dummy_ctx_mp);
      message_passing_vtable.destroy(&dummy_ctx_mp);
    }

    /* destroy logger failure */
    rc_mp = message_passing_vtable.init(&dummy_ctx_mp);
    if (rc_mp == C_REST_OK) {
      dummy_ctx_mp.logger.log_cb = mock_logger_err_cb;
      message_passing_vtable.destroy(&dummy_ctx_mp);
      if (dummy_ctx_mp.internal_state) {
        free(dummy_ctx_mp.internal_state);
      }
    }

    /* destroy NULL */
    message_passing_vtable.destroy(NULL);

    /* destroy no internal_state */
    dummy_ctx_mp.internal_state = NULL;
    message_passing_vtable.destroy(&dummy_ctx_mp);

    /* destroy with valid server_sock socket */
    dummy_ctx_mp.logger.log_cb = mock_logger_cb;
    rc_mp = message_passing_vtable.init(&dummy_ctx_mp);
    if (rc_mp == C_REST_OK) {
      *(c_rest_socket_t *)dummy_ctx_mp.internal_state =
          (c_rest_socket_t)(intptr_t)socket(AF_INET, SOCK_STREAM, 0);
      message_passing_vtable.destroy(&dummy_ctx_mp);
    }

    /* destroy with invalid server_sock to trigger socket close error */
    rc_mp = message_passing_vtable.init(&dummy_ctx_mp);
    if (rc_mp == C_REST_OK) {
#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
      *(c_rest_socket_t *)dummy_ctx_mp.internal_state =
          (c_rest_socket_t)(intptr_t)9999;
#else
      *(c_rest_socket_t *)dummy_ctx_mp.internal_state = (c_rest_socket_t)9999;
#endif
      rc_mp = message_passing_vtable.destroy(&dummy_ctx_mp);
      if (dummy_ctx_mp.internal_state) {
        free(dummy_ctx_mp.internal_state);
      }
    }
  }

  /* multi_process modality direct tests */
  {
    extern const struct c_rest_modality_vtable multi_process_vtable;
    struct c_rest_context dummy_ctx_mproc;
    c_rest_error_t rc_mproc;

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
    rc_mproc = multi_process_vtable.init(&dummy_ctx_mproc);
    if (rc_mproc == C_REST_OK) {
      dummy_ctx_mproc.logger.log_cb = mock_logger_err_cb;
      multi_process_vtable.run(&dummy_ctx_mproc);
      dummy_ctx_mproc.logger.log_cb = mock_logger_cb;
      multi_process_vtable.destroy(&dummy_ctx_mproc);
    }

    /* run logger failure end */
    rc_mproc = multi_process_vtable.init(&dummy_ctx_mproc);
    if (rc_mproc == C_REST_OK) {
      g_async_logger_calls = 0;
      dummy_ctx_mproc.logger.log_cb = mock_logger_fail_on_second;
      multi_process_vtable.run(&dummy_ctx_mproc);
      dummy_ctx_mproc.logger.log_cb = mock_logger_cb;
      multi_process_vtable.destroy(&dummy_ctx_mproc);
    }

    /* run with logger == NULL */
    rc_mproc = multi_process_vtable.init(&dummy_ctx_mproc);
    if (rc_mproc == C_REST_OK) {
      dummy_ctx_mproc.logger.log_cb = NULL;
      multi_process_vtable.run(&dummy_ctx_mproc);
      multi_process_vtable.destroy(&dummy_ctx_mproc);
    }

    /* destroy logger failure */
    rc_mproc = multi_process_vtable.init(&dummy_ctx_mproc);
    if (rc_mproc == C_REST_OK) {
      dummy_ctx_mproc.logger.log_cb = mock_logger_err_cb;
      multi_process_vtable.destroy(&dummy_ctx_mproc);
      if (dummy_ctx_mproc.internal_state) {
        free(dummy_ctx_mproc.internal_state);
      }
    }

    /* destroy NULL */
    multi_process_vtable.destroy(NULL);

    /* destroy no internal_state */
    dummy_ctx_mproc.internal_state = NULL;
    multi_process_vtable.destroy(&dummy_ctx_mproc);

    /* destroy with valid server_sock socket */
    dummy_ctx_mproc.logger.log_cb = mock_logger_cb;
    rc_mproc = multi_process_vtable.init(&dummy_ctx_mproc);
    if (rc_mproc == C_REST_OK) {
      *(c_rest_socket_t *)dummy_ctx_mproc.internal_state =
          (c_rest_socket_t)(intptr_t)socket(AF_INET, SOCK_STREAM, 0);
      multi_process_vtable.destroy(&dummy_ctx_mproc);
    }

    /* destroy with invalid server_sock to trigger socket close error */
    rc_mproc = multi_process_vtable.init(&dummy_ctx_mproc);
    if (rc_mproc == C_REST_OK) {
#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
      *(c_rest_socket_t *)dummy_ctx_mproc.internal_state =
          (c_rest_socket_t)(intptr_t)9999;
#else
      *(c_rest_socket_t *)dummy_ctx_mproc.internal_state =
          (c_rest_socket_t)9999;
#endif
      rc_mproc = multi_process_vtable.destroy(&dummy_ctx_mproc);
      if (dummy_ctx_mproc.internal_state) {
        free(dummy_ctx_mproc.internal_state);
      }
    }

    /* destroy with workers != NULL */
    rc_mproc = multi_process_vtable.init(&dummy_ctx_mproc);
    if (rc_mproc == C_REST_OK) {
      struct multi_process_state_mock {
        c_rest_socket_t server_sock;
        int is_running;
        c_rest_process_t *workers;
        int worker_count;
      };
      struct multi_process_state_mock *mock =
          (struct multi_process_state_mock *)dummy_ctx_mproc.internal_state;
      mock->workers = (c_rest_process_t *)malloc(1);
      multi_process_vtable.destroy(&dummy_ctx_mproc);
    }
  }

  /* Extra run tests with thread connection to hit accept success */
  {
    extern const struct c_rest_modality_vtable single_thread_vtable;
    extern const struct c_rest_modality_vtable multi_thread_vtable;

    struct c_rest_context dummy_ctx;
    c_rest_thread_t client_thread;
    struct test_client_args args;

#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
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

    /* sync */
    dummy_ctx.listen_port = 46781;
    args.port = 46781;
    dummy_ctx.vtable = &sync_vtable;
    if (sync_vtable.init(&dummy_ctx) == C_REST_OK) {
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
    if (single_thread_vtable.init(&dummy_ctx) == C_REST_OK) {
      c_rest_thread_create(&client_thread, test_client_thread, &args);
      single_thread_vtable.run(&dummy_ctx);
      c_rest_thread_join(client_thread);
      single_thread_vtable.destroy(&dummy_ctx);
    }

    /* multi_thread */
    dummy_ctx.listen_port = 46780;
    args.port = 46780;
    dummy_ctx.vtable = &multi_thread_vtable;
    if (multi_thread_vtable.init(&dummy_ctx) == C_REST_OK) {
      c_rest_thread_create(&client_thread, test_client_thread, &args);
      multi_thread_vtable.run(&dummy_ctx);
      c_rest_thread_join(client_thread);
      multi_thread_vtable.destroy(&dummy_ctx);
    }
  }

  {
    extern int g_mock_socket_fail;
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

    g_mock_socket_fail = 0;
  }

  {
    extern int g_mock_socket_fail;
    extern int g_mock_tls_fail;
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
      if (sync_vtable.run)
        sync_vtable.run(&ctx_tls);

      g_mock_socket_fail = j_mod;
      ctx_tls.internal_state = &single_st;
      single_st.server_sock = C_REST_INVALID_SOCKET;
      single_st.is_running = 1;
      if (single_thread_vtable.run)
        single_thread_vtable.run(&ctx_tls);

      g_mock_socket_fail = j_mod;
      ctx_tls.internal_state = &multi_st;
      multi_st.server_sock = C_REST_INVALID_SOCKET;
      multi_st.is_running = 1;
      if (multi_thread_vtable.run)
        multi_thread_vtable.run(&ctx_tls);
    }

    for (j_mod = 1002; j_mod <= 1003; j_mod++) {
      g_mock_socket_fail = j_mod;
      ctx_tls.internal_state = &sync_st;
      sync_st.server_sock = C_REST_INVALID_SOCKET;
      sync_st.is_running = 1;
      if (sync_vtable.run)
        sync_vtable.run(&ctx_tls);

      g_mock_socket_fail = j_mod;
      ctx_tls.internal_state = &single_st;
      single_st.server_sock = C_REST_INVALID_SOCKET;
      single_st.is_running = 1;
      if (single_thread_vtable.run)
        single_thread_vtable.run(&ctx_tls);

      g_mock_socket_fail = j_mod;
      ctx_tls.internal_state = &multi_st;
      multi_st.server_sock = C_REST_INVALID_SOCKET;
      multi_st.is_running = 1;
      if (multi_thread_vtable.run)
        multi_thread_vtable.run(&ctx_tls);
    }

    g_mock_socket_fail = 109;
    ctx_tls.internal_state = &sync_st;
    sync_st.server_sock = (c_rest_socket_t)1;
    sync_st.is_running = 1;
    if (sync_vtable.run)
      sync_vtable.run(&ctx_tls);

    g_mock_socket_fail = 109;
    ctx_tls.internal_state = &single_st;
    single_st.server_sock = (c_rest_socket_t)1;
    single_st.is_running = 1;
    if (single_thread_vtable.run)
      single_thread_vtable.run(&ctx_tls);

    g_mock_socket_fail = 109;
    ctx_tls.internal_state = &multi_st;
    multi_st.server_sock = (c_rest_socket_t)1;
    multi_st.is_running = 1;
    if (multi_thread_vtable.run)
      multi_thread_vtable.run(&ctx_tls);

    g_mock_socket_fail = 0;

    ctx_tls.tls_ctx = (void *)1;
    for (j_mod = 1; j_mod <= 3; j_mod++) {
      g_mock_tls_fail = j_mod;

      g_mock_socket_fail = 7;
      ctx_tls.internal_state = &sync_st;
      sync_st.server_sock = (c_rest_socket_t)1;
      sync_st.is_running = 1;
      if (sync_vtable.run)
        sync_vtable.run(&ctx_tls);
    }
    g_mock_tls_fail = 0;
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
    extern int g_mock_socket_fail;
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
  test_modality_simple();
  return failed;
}
