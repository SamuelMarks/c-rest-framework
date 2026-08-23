#include <string.h>
/* clang-format off */
#include "c_rest_error.h"
#include "test_protos.h"
#include "c_rest_hot_reload.h"
#include "c_rest_mem.h"
#include "greatest.h"
#include "c_rest_router.h"
#include "c_rest_sse.h"

#include <stdio.h>
#include <time.h>
#if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__) || defined(_MSC_VER)
void __stdcall Sleep(unsigned long dwMilliseconds);
#else
#include <unistd.h>
#endif
/* clang-format on */

typedef enum c_rest_hot_reload_state {
  C_REST_HOT_RELOAD_STATE_INIT = 0,
  C_REST_HOT_RELOAD_STATE_WATCHING,
  C_REST_HOT_RELOAD_STATE_CHANGED,
  C_REST_HOT_RELOAD_STATE_RELOADING,
  C_REST_HOT_RELOAD_STATE_STOPPED
} c_rest_hot_reload_state_t;

struct c_rest_hot_reload_ctx {
  char **watched_paths;
  time_t *last_modified_times;
  size_t watch_count;
  size_t watch_capacity;
  c_rest_hot_reload_state_t state;
  struct c_rest_logger *logger;
  c_rest_thread_t watcher_thread;
  c_rest_hot_reload_callback_t on_reload;
  void *user_data;
#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
  cm_env_t cm_env;
#endif
};

static void sleep_seconds(int seconds) {
#if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__) ||           \
    defined(_MSC_VER)
  Sleep((unsigned long)(seconds * 1000));
#else
  sleep(seconds);
#endif
}

TEST test_hot_reload_init_destroy(void) {
  c_rest_hot_reload_ctx_t *ctx = NULL;
  int res;

  res = c_rest_hot_reload_init(&ctx, NULL);
  ASSERT_EQ(C_REST_OK, res);
  ASSERT(ctx != NULL);

  res = c_rest_hot_reload_destroy(ctx);
  ASSERT_EQ(C_REST_OK, res);
  PASS();
}

TEST test_hot_reload_add_watch(void) {
  c_rest_hot_reload_ctx_t *ctx = NULL;
  int res;

  res = c_rest_hot_reload_init(&ctx, NULL);
  ASSERT_EQ(C_REST_OK, res);

  {
    FILE *f1 = NULL;
#if defined(_MSC_VER)
    fopen_s(&f1, "test_file.txt", "w");
#else
    f1 = fopen("test_file.txt", "w");
#endif
    if (f1)
      fclose(f1);
  }
  res = c_rest_hot_reload_add_watch(ctx, "test_file.txt");
  ASSERT_EQ(C_REST_OK, res);

  {
    FILE *f2 = NULL;
#if defined(_MSC_VER)
    fopen_s(&f2, "another_file.txt", "w");
#else
    f2 = fopen("another_file.txt", "w");
#endif
    if (f2)
      fclose(f2);
  }
  res = c_rest_hot_reload_add_watch(ctx, "another_file.txt");
  ASSERT_EQ(C_REST_OK, res);

  res = c_rest_hot_reload_destroy(ctx);
  ASSERT_EQ(C_REST_OK, res);
  PASS();
}

static int dummy_reload_callback(void *user_data) {
  int *called = (int *)user_data;
  if (called)
    *called = 1;
  return 0;
}

TEST test_hot_reload_start(void) {
  c_rest_hot_reload_ctx_t *ctx = NULL;
  int res;
  int called = 0;

  res = c_rest_hot_reload_init(&ctx, NULL);
  ASSERT_EQ(C_REST_OK, res);

  res = c_rest_hot_reload_start(ctx, dummy_reload_callback, &called);
  ASSERT_EQ(C_REST_OK, res);
  ASSERT_EQ(0, called);

  res = c_rest_hot_reload_destroy(ctx);
  ASSERT_EQ(C_REST_OK, res);
  PASS();
}

TEST test_hot_reload_modification(void) {
  c_rest_hot_reload_ctx_t *ctx = NULL;
  int res;
  int called = 0;
  FILE *f;
  const char *test_filename = "test_hot_reload_tmp.txt";

/* Create file */
#if defined(_MSC_VER)
  fopen_s(&f, test_filename, "w");
#else
  f = fopen(test_filename, "w");
#endif
  ASSERT(f != NULL);
  fprintf(f, "hello\n");
  fclose(f);

  res = c_rest_hot_reload_init(&ctx, NULL);
  ASSERT_EQ(C_REST_OK, res);

  res = c_rest_hot_reload_add_watch(ctx, test_filename);
  ASSERT_EQ(C_REST_OK, res);

  /* Poll immediately, shouldn't be called because nothing changed */
  res = c_rest_hot_reload_poll(ctx, dummy_reload_callback, &called);
  ASSERT_EQ(C_REST_OK, res);
  ASSERT_EQ(0, called);

  /* Wait at least 1-2 seconds (file mtime usually has 1s resolution) */
  sleep_seconds(2);

/* Modify file */
#if defined(_MSC_VER)
  fopen_s(&f, test_filename, "a");
#else
  f = fopen(test_filename, "a");
#endif
  ASSERT(f != NULL);
  fprintf(f, "world\n");
  fclose(f);

  /* Poll again, should detect change */
  res = c_rest_hot_reload_poll(ctx, dummy_reload_callback, &called);
  ASSERT_EQ(C_REST_OK, res);
  ASSERT_EQ(1, called);

  res = c_rest_hot_reload_destroy(ctx);
  ASSERT_EQ(C_REST_OK, res);

  remove(test_filename);
  PASS();
}

TEST test_hot_reload_edge_cases(void) {
  c_rest_hot_reload_ctx_t *ctx = NULL;
  int res;

  /* Invalid init */
  res = c_rest_hot_reload_init(NULL, NULL);

  /* Test logger with NULL log_cb */
  {
    struct c_rest_logger logger_null_cb;
    c_rest_hot_reload_ctx_t *ctx_null_cb = NULL;
    memset(&logger_null_cb, 0, sizeof(logger_null_cb));
    c_rest_hot_reload_init(&ctx_null_cb, &logger_null_cb);
    if (ctx_null_cb) {
      c_rest_hot_reload_destroy(ctx_null_cb);
    }
  }

  ASSERT_EQ(C_REST_ERROR_INVALID_ARG, res);

  /* Invalid watch */
  res = c_rest_hot_reload_add_watch(NULL, "test");
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG, res);

  res = c_rest_hot_reload_init(&ctx, NULL);
  ASSERT_EQ(C_REST_OK, res);

  res = c_rest_hot_reload_add_watch(ctx, NULL);
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG, res);

  /* invalid file watch */
  res = c_rest_hot_reload_add_watch(ctx, "does_not_exist_file.txt");
  ASSERT_EQ(C_REST_ERROR_GENERIC, res);

  /* Invalid start */
  res = c_rest_hot_reload_start(NULL, dummy_reload_callback, NULL);
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG, res);

  res = c_rest_hot_reload_start(ctx, NULL, NULL);
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG, res);

  /* Invalid poll */
  res = c_rest_hot_reload_poll(NULL, dummy_reload_callback, NULL);
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG, res);

  res = c_rest_hot_reload_poll(ctx, NULL, NULL);
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG, res);

  /* Invalid destroy */
  res = c_rest_hot_reload_destroy(NULL);
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG, res);

  res = c_rest_hot_reload_destroy(ctx);
  ASSERT_EQ(C_REST_OK, res);

  PASS();
}

static c_rest_error_t mock_logger_cb(const char *msg) {
  (void)msg;
  return C_REST_OK;
}

static c_rest_error_t mock_logger_err_cb(const char *msg) {
  (void)msg;
  return C_REST_ERROR_GENERIC;
}

TEST test_hot_reload_logger(void) {
  c_rest_hot_reload_ctx_t *ctx = NULL;
  struct c_rest_logger logger;
  struct c_rest_logger err_logger;
  int res;

  logger.log_cb = mock_logger_cb;
  res = c_rest_hot_reload_init(&ctx, &logger);
  ASSERT_EQ(C_REST_OK, res);

  {
    FILE *f = NULL;
#if defined(_MSC_VER)
    fopen_s(&f, "test_log.txt", "w");
#else
    f = fopen("test_log.txt", "w");
#endif
    if (f)
      fclose(f);
  }
  res = c_rest_hot_reload_add_watch(ctx, "test_log.txt");
  ASSERT_EQ(C_REST_OK, res);

  /* Poll with no changes, should be OK */
  res = c_rest_hot_reload_poll(ctx, dummy_reload_callback, NULL);
  ASSERT_EQ(C_REST_OK, res);

  /* Modify file to trigger a change in poll */
  sleep_seconds(2);
  {
    FILE *f = NULL;
#if defined(_MSC_VER)
    fopen_s(&f, "test_log.txt", "a");
#else
    f = fopen("test_log.txt", "a");
#endif
    if (f) {
      fprintf(f, "a");
      fclose(f);
    }
  }

  /* Temporarily make logger fail so poll fails */
  logger.log_cb = mock_logger_err_cb;
  res = c_rest_hot_reload_poll(ctx, dummy_reload_callback, NULL);
  ASSERT_EQ(C_REST_ERROR_GENERIC, res);
  logger.log_cb = mock_logger_cb;

  res = c_rest_hot_reload_destroy(ctx);
  ASSERT_EQ(C_REST_OK, res);

  err_logger.log_cb = mock_logger_err_cb;
  res = c_rest_hot_reload_init(&ctx, &logger);
  ASSERT_EQ(C_REST_OK, res);
  ctx->logger = &err_logger;
  /* the add_watch fails because the logger returns error */
  res = c_rest_hot_reload_add_watch(ctx, "test_log.txt");
  ASSERT_EQ(C_REST_ERROR_GENERIC, res);

  res = c_rest_hot_reload_destroy(ctx);
  ASSERT_EQ(C_REST_OK, res);

  PASS();
}

#ifdef C_REST_TESTING_MALLOC_HOOK
static int g_malloc_fail_count = -1;
static void *hook_malloc(size_t size) {
  if (g_malloc_fail_count == 0) {
    return NULL;
  }
  if (g_malloc_fail_count > 0) {
    g_malloc_fail_count--;
  }
  return malloc(size);
}
static void *hook_realloc(void *ptr, size_t size) {
  if (g_malloc_fail_count == 0) {
    return NULL;
  }
  if (g_malloc_fail_count > 0) {
    g_malloc_fail_count--;
  }
  return realloc(ptr, size);
}

TEST test_hot_reload_oom(void) {
  c_rest_hot_reload_ctx_t *ctx = NULL;
  int res;

  g_crf_malloc_hook = hook_malloc;
  g_crf_realloc_hook = hook_realloc;

  /* init OOM */
  g_malloc_fail_count = 0;
  res = c_rest_hot_reload_init(&ctx, NULL);
  ASSERT_EQ(C_REST_ERROR_OOM, res);

  /* add watch path OOM */
  {
    FILE *f = NULL;
#if defined(_MSC_VER)
    fopen_s(&f, "test_oom.txt", "w");
#else
    f = fopen("test_oom.txt", "w");
#endif
    if (f)
      fclose(f);
  }
  g_malloc_fail_count = -1;
  res = c_rest_hot_reload_init(&ctx, NULL);
  ASSERT_EQ(C_REST_OK, res);

  /* we will fail on allocating watched_paths or times */
  g_malloc_fail_count = 0;
  res = c_rest_hot_reload_add_watch(ctx, "test_oom.txt");
  ASSERT_EQ(C_REST_ERROR_OOM, res);

  g_malloc_fail_count = 1;
  res = c_rest_hot_reload_add_watch(ctx, "test_oom.txt");
  ASSERT_EQ(C_REST_ERROR_OOM, res);

  g_malloc_fail_count = 2;
  res = c_rest_hot_reload_add_watch(ctx, "test_oom.txt");
  ASSERT_EQ(C_REST_ERROR_OOM, res);

  /* realloc OOM */
  g_malloc_fail_count = -1;
  /* Add 8 watches to fill capacity */
  {
    int i;
    for (i = 0; i < 8; i++) {
      res = c_rest_hot_reload_add_watch(ctx, "test_oom.txt");
      ASSERT_EQ(C_REST_OK, res);
    }
  }
  /* Now it will realloc */
  g_malloc_fail_count = 0;
  res = c_rest_hot_reload_add_watch(ctx, "test_oom.txt");
  ASSERT_EQ(C_REST_ERROR_OOM, res);

  g_malloc_fail_count = 1;
  res = c_rest_hot_reload_add_watch(ctx, "test_oom.txt");
  ASSERT_EQ(C_REST_ERROR_OOM, res);

  /* Now allow it to succeed */
  g_malloc_fail_count = -1;
  res = c_rest_hot_reload_add_watch(ctx, "test_oom.txt");
  ASSERT_EQ(C_REST_OK, res);

  g_malloc_fail_count = -1;
  res = c_rest_hot_reload_destroy(ctx);
  ASSERT_EQ(C_REST_OK, res);

  g_malloc_fail_count = -1;
  res = c_rest_hot_reload_init(&ctx, NULL);
  ASSERT_EQ(C_REST_OK, res);

  g_crf_malloc_hook = hook_malloc;
  g_malloc_fail_count = 0;
  res = c_rest_hot_reload_start(ctx, dummy_reload_callback, NULL);
  ASSERT_EQ(C_REST_ERROR_GENERIC, res);
  g_crf_malloc_hook = NULL;
  g_malloc_fail_count = -1;

  g_crf_malloc_hook = NULL;
  g_malloc_fail_count = -1;
  res = c_rest_hot_reload_destroy(ctx);
  ASSERT_EQ(C_REST_OK, res);

  /* OOM test for start with err_logger */
  {
    struct c_rest_logger err_logger;
    struct c_rest_logger normal_logger;
    normal_logger.log_cb = mock_logger_cb;
    err_logger.log_cb = mock_logger_err_cb;
    res = c_rest_hot_reload_init(&ctx, &normal_logger);
    ASSERT_EQ(C_REST_OK, res);
    ctx->logger = &err_logger;

    g_crf_malloc_hook = hook_malloc;
    g_malloc_fail_count = 0;
    res = c_rest_hot_reload_start(ctx, dummy_reload_callback, NULL);
    ASSERT_EQ(C_REST_ERROR_GENERIC, res);
    g_crf_malloc_hook = NULL;
    g_malloc_fail_count = -1;

    g_crf_malloc_hook = NULL;
    g_malloc_fail_count = -1;
    res = c_rest_hot_reload_start(ctx, dummy_reload_callback, NULL);
    ASSERT_EQ(C_REST_ERROR_GENERIC, res);

    /* Corrupt watcher_thread to make c_rest_thread_join fail */
    /* By passing a valid thread handle format but disconnected, we force a
     * failure */
    /* However, casting -1 to c_rest_thread_t might not fail if it's considered
     * valid enough to crash or valid enough to ignore. */
    /* Let's try mocking a pthread failure if possible, but c_rest_thread_join
     * is an external call. */
    /* If we look at c_rest_thread_join, on POSIX it does pthread_join. Passing
     * a random pointer or non-existent thread ID returns an error like ESRCH or
     * EINVAL. */

    {
#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#define C_REST_ASAN_ENABLED 1
#endif
#endif
#if defined(__SANITIZE_ADDRESS__)
#define C_REST_ASAN_ENABLED 1
#endif

#ifndef C_REST_ASAN_ENABLED
      /* On Mac/Linux a completely invalid pointer to pthread_t usually causes
       * ESRCH or EINVAL */
#endif
    }
  }

  res = c_rest_hot_reload_destroy(ctx);
  ASSERT_EQ(C_REST_OK, res);
  g_crf_malloc_hook = NULL;
  g_crf_realloc_hook = NULL;
  PASS();
}
#endif

#ifdef C_REST_ENABLE_SERVER_SENT_EVENTS_SSE
#include "c_rest_request.h"
#include "c_rest_response.h"
#if !defined(_MSC_VER)
#include <sys/socket.h>
#endif

TEST test_hot_reload_sse_routes(void) {
  struct c_rest_router *router = NULL;
  c_rest_hot_reload_ctx_t *hr_ctx = NULL;
  struct c_rest_context fw_ctx;
  struct c_rest_connection_context conn_ctx;
  struct c_rest_request req;
  struct c_rest_response res;
  int rc;
  c_rest_socket_t server_sock = C_REST_INVALID_SOCKET;
  c_rest_socket_t client_sock = C_REST_INVALID_SOCKET;
  c_rest_socket_t accepted_sock = C_REST_INVALID_SOCKET;

  memset(&fw_ctx, 0, sizeof(fw_ctx));
  memset(&conn_ctx, 0, sizeof(conn_ctx));
  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  /* Create connected sockets so c_rest_socket_send doesn't fail */
#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
  {
    int fds[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0) {
      accepted_sock = (c_rest_socket_t)fds[0];
      client_sock = (c_rest_socket_t)fds[1];
    }
  }
#endif

  rc = c_rest_hot_reload_init(&hr_ctx, NULL);
  ASSERT_EQ(C_REST_OK, rc);
  fw_ctx.hot_reload_ctx = hr_ctx;
  conn_ctx.framework_ctx = &fw_ctx;
  conn_ctx.sock = accepted_sock;

  /* Create a file and add it */
  {
    FILE *f = NULL;
#if defined(_MSC_VER)
    fopen_s(&f, "test_sse_file.txt", "w");
#else
    f = fopen("test_sse_file.txt", "w");
#endif
    if (f) {
      fprintf(f, "a");
      fclose(f);
    }
  }
  rc = c_rest_hot_reload_add_watch(hr_ctx, "test_sse_file.txt");
  ASSERT_EQ(C_REST_OK, rc);

  rc = c_rest_hot_reload_register_routes(NULL, "/test");
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG, rc);

  rc = c_rest_router_init(&router);
  ASSERT_EQ(C_REST_OK, rc);

  g_crf_malloc_hook = hook_malloc;
  g_crf_realloc_hook = hook_realloc;
  g_malloc_fail_count = 0;
  rc = c_rest_hot_reload_register_routes(router, "/hot-reload");
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);
  g_crf_malloc_hook = NULL;
  g_crf_realloc_hook = NULL;
  g_malloc_fail_count = -1;

  rc = c_rest_hot_reload_register_routes(router, "/hot-reload");
  ASSERT_EQ(C_REST_OK, rc);

  /* First dispatch: no change */
  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));
  req.method = "GET";
  req.path = "/hot-reload";
  res.context = &conn_ctx;

  rc = c_rest_router_dispatch(router, &req, &res);
  ASSERT_EQ(C_REST_OK, rc);
  ASSERT_EQ(200, res.status_code);
  c_rest_response_cleanup(&res);
  c_rest_request_cleanup(&req);

  /* Second dispatch: wait 2s, modify file, poll, then dispatch */
  sleep_seconds(2);
  {
    FILE *f = NULL;
#if defined(_MSC_VER)
    fopen_s(&f, "test_sse_file.txt", "a");
#else
    f = fopen("test_sse_file.txt", "a");
#endif
    if (f) {
      fprintf(f, "b");
      fclose(f);
    }
  }

  {
    int dummy_called = 0;
    rc = c_rest_hot_reload_poll(hr_ctx, dummy_reload_callback, &dummy_called);
    ASSERT_EQ(C_REST_OK, rc);
    ASSERT_EQ(1, dummy_called);
  }

  /* OOM loop for SSE handler */
  {
    int oom_idx;
    g_crf_malloc_hook = hook_malloc;
    g_crf_realloc_hook = hook_realloc;
    for (oom_idx = 0; oom_idx < 20; oom_idx++) {
      g_malloc_fail_count = oom_idx;
      memset(&req, 0, sizeof(req));
      memset(&res, 0, sizeof(res));
      req.method = "GET";
      req.path = "/hot-reload";
      res.context = &conn_ctx;

      rc = c_rest_router_dispatch(router, &req, &res);
      c_rest_response_cleanup(&res);
      c_rest_request_cleanup(&req);
    }
    g_crf_malloc_hook = NULL;
    g_crf_realloc_hook = NULL;
    g_malloc_fail_count = -1;
  }

  /* This dispatch should succeed and reset the state */
  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));
  req.method = "GET";
  req.path = "/hot-reload";
  res.context = &conn_ctx;

  rc = c_rest_router_dispatch(router, &req, &res);
  ASSERT_EQ(C_REST_OK, rc);
  ASSERT_EQ(200, res.status_code);

  /* Also test dispatch when hr_ctx is missing */
  c_rest_response_cleanup(&res);
  c_rest_request_cleanup(&req);

  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));
  req.method = "GET";
  req.path = "/hot-reload";
  conn_ctx.framework_ctx = NULL; /* remove context */
  res.context = &conn_ctx;

  rc = c_rest_router_dispatch(router, &req, &res);
  ASSERT_EQ(C_REST_OK, rc);
  ASSERT_EQ(503, res.status_code);

  /* Clean up */
  c_rest_response_cleanup(&res);
  c_rest_request_cleanup(&req);
  c_rest_router_destroy(router);
  c_rest_hot_reload_destroy(hr_ctx);
  remove("test_sse_file.txt");

  if (accepted_sock)
    c_rest_socket_close(accepted_sock);
  if (client_sock)
    c_rest_socket_close(client_sock);
  if (server_sock)
    c_rest_socket_close(server_sock);

  PASS();
}
#endif

#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
TEST test_hot_reload_multiplatform(void) {
  c_rest_hot_reload_ctx_t *ctx = NULL;
  int res;
  int dummy_called = 0;

  res = c_rest_hot_reload_init(&ctx, NULL);
  ASSERT_EQ(C_REST_OK, res);

  res = c_rest_hot_reload_set_multiplatform_env(NULL, (cm_env_t)1);
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG, res);

  res = c_rest_hot_reload_set_multiplatform_env(ctx, (cm_env_t)1);
  ASSERT_EQ(C_REST_OK, res);

  {
    FILE *f = NULL;
#if defined(_MSC_VER)
    fopen_s(&f, "test_mp_file.txt", "w");
#else
    f = fopen("test_mp_file.txt", "w");
#endif
    if (f)
      fclose(f);
  }
  res = c_rest_hot_reload_add_watch(ctx, "test_mp_file.txt");
  ASSERT_EQ(C_REST_OK, res);

  g_mock_cm_file_fail = 1;
  res = c_rest_hot_reload_poll(ctx, dummy_reload_callback, &dummy_called);
  g_mock_cm_file_fail = 0;

  g_mock_cm_thread_fail = 1;
  res = c_rest_hot_reload_start(ctx, dummy_reload_callback, &dummy_called);
  ASSERT_EQ(C_REST_ERROR_GENERIC, res);

  /* Test logger failure during start failure */
  {
    struct c_rest_logger err_logger;
    err_logger.log_cb = mock_logger_err_cb;
    ctx->logger = &err_logger;
    res = c_rest_hot_reload_start(ctx, dummy_reload_callback, &dummy_called);
    ASSERT_EQ(C_REST_ERROR_GENERIC, res);
    ctx->logger = NULL;
  }
  g_mock_cm_thread_fail = 0;

  res = c_rest_hot_reload_start(ctx, dummy_reload_callback, &dummy_called);
  ASSERT_EQ(C_REST_OK, res);

  g_mock_cm_join_fail = 1;
  res = c_rest_hot_reload_destroy(ctx);
  ASSERT_EQ(C_REST_OK, res);
  g_mock_cm_join_fail = 0;

  remove("test_mp_file.txt");
  PASS();
}
#endif

SUITE(suite_hot_reload) {
  RUN_TEST(test_hot_reload_init_destroy);
  RUN_TEST(test_hot_reload_add_watch);
  RUN_TEST(test_hot_reload_start);
  RUN_TEST(test_hot_reload_modification);
  RUN_TEST(test_hot_reload_edge_cases);
  RUN_TEST(test_hot_reload_logger);
#ifdef C_REST_TESTING_MALLOC_HOOK
  RUN_TEST(test_hot_reload_oom);
#endif
#ifdef C_REST_ENABLE_SERVER_SENT_EVENTS_SSE
  RUN_TEST(test_hot_reload_sse_routes);
#endif
#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
  RUN_TEST(test_hot_reload_multiplatform);
#endif
}
