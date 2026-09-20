/* clang-format off */
#include "c_rest_error.h"
#include "greatest.h"
#include "greatest_clean.h"
#include <string.h>

#undef C_REST_EXPORT
#define C_REST_EXPORT

#include "c_rest_platform.h"
#include "c_rest_hot_reload.h"

#undef C_REST_EXPORT
#if defined(_MSC_VER) && !defined(C_REST_FRAMEWORK_STATIC_DEFINE)
#define C_REST_EXPORT __declspec(dllimport)
#else
#define C_REST_EXPORT
#endif

/* Forward declarations */
extern c_rest_error_t c_rest_thread_join(c_rest_thread_t thread);

static int g_mock_join_fail = 0;

static c_rest_error_t mock_c_rest_thread_join(c_rest_thread_t thread) {
    if (g_mock_join_fail) return C_REST_ERROR_GENERIC;
    return c_rest_thread_join(thread);
}

/* Preprocessor Injection */
#define c_rest_thread_join mock_c_rest_thread_join

/* Include the actual source file */
#include "../src/c_rest_hot_reload.c"

#undef c_rest_thread_join
/* clang-format on */

static void reset_mocks(void *data) {
  (void)data;
  g_mock_join_fail = 0;
}

static c_rest_error_t fail_logger_cb(const char *msg) {
  (void)msg;
  return C_REST_ERROR_GENERIC; /* Fail the logger to make poll fail */
}

static int dummy_on_reload(void *user_data) {
  (void)user_data;
  return 0;
}

TEST test_watcher_thread_func_poll_fail(void) {
  c_rest_hot_reload_ctx_t *ctx = NULL;
  struct c_rest_logger logger = {0};
  c_rest_error_t rc;

  /* Don't use the failing logger for init, or init will fail. */
  rc = c_rest_hot_reload_init(&ctx, NULL);
  ASSERT_EQ(C_REST_OK, rc);

  /* Set the failing logger *after* init */
  logger.log_cb = fail_logger_cb;
  ctx->logger = &logger;
  ctx->on_reload = dummy_on_reload;

  /* We need c_rest_hot_reload_poll to actually execute the log.
     It only executes log if a file has changed.
     Let's fake a file change directly in the ctx struct. */
  ctx->watch_capacity = 1;
  ctx->watch_count = 1;
  ASSERT_EQ(C_REST_OK,
            C_REST_MALLOC(sizeof(char *), (void **)&ctx->watched_paths));
  ASSERT_EQ(C_REST_OK, C_REST_MALLOC(16, (void **)&ctx->watched_paths[0]));
#if defined(_MSC_VER)
  strcpy_s(ctx->watched_paths[0], 16, "test_file_fake");
#else
  strcpy(ctx->watched_paths[0], "test_file_fake");
#endif
  ASSERT_EQ(C_REST_OK,
            C_REST_MALLOC(sizeof(time_t), (void **)&ctx->last_modified_times));
  ctx->last_modified_times[0] = 12345; /* Fake mtime */

  /* Call the static watcher_thread_func directly */
  rc = watcher_thread_func(ctx);
  printf("RC is %d\n", rc);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);

  c_rest_hot_reload_destroy(ctx);
  PASS();
}

TEST test_hot_reload_destroy_join_fail(void) {
  c_rest_hot_reload_ctx_t *ctx = NULL;
  struct c_rest_logger fail_logger;
  c_rest_error_t rc;

  ASSERT_EQ(0, dummy_on_reload(NULL));

  ASSERT_EQ(C_REST_ERROR_INVALID_ARG, c_rest_hot_reload_destroy(NULL));

  /* Case 1: logger fails on join fail */
  rc = c_rest_hot_reload_init(&ctx, NULL);
  ASSERT_EQ(C_REST_OK, rc);
  ASSERT(ctx != NULL);

  memset(&fail_logger, 0, sizeof(fail_logger));
  fail_logger.log_cb = fail_logger_cb;
  ctx->logger = &fail_logger;
  ctx->watcher_thread = (c_rest_thread_t)1; /* Fake thread handle */

  g_mock_join_fail = 1;
  rc = c_rest_hot_reload_destroy(ctx);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);
  g_mock_join_fail = 0;

  /* Case 2: logger is NULL on join fail */
  rc = c_rest_hot_reload_init(&ctx, NULL);
  ASSERT_EQ(C_REST_OK, rc);
  ASSERT(ctx != NULL);
  ctx->watcher_thread = (c_rest_thread_t)1;
  g_mock_join_fail = 1;
  rc = c_rest_hot_reload_destroy(ctx);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);
  g_mock_join_fail = 0;

  /* Hit mock_c_rest_thread_join return when fail is 0 using a real thread */
  rc = c_rest_hot_reload_init(&ctx, NULL);
  ASSERT_EQ(C_REST_OK, rc);
  rc = c_rest_hot_reload_start(ctx, dummy_on_reload, NULL);
  ASSERT_EQ(C_REST_OK, rc);
  ASSERT_EQ(C_REST_OK, c_rest_hot_reload_destroy(ctx));
  PASS();
}

TEST test_hot_reload_init_logger_fail(void) {
  c_rest_hot_reload_ctx_t *ctx = NULL;
  struct c_rest_logger logger;
  c_rest_error_t rc;

  memset(&logger, 0, sizeof(logger));
  logger.log_cb = fail_logger_cb;
  rc = c_rest_hot_reload_init(&ctx, &logger);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);
  ASSERT_EQ(NULL, ctx);
  PASS();
}

SUITE_EXTERN(hot_reload_mock_suite);
SUITE(hot_reload_mock_suite) {
  SET_SETUP(reset_mocks, NULL);
  RUN_TEST(test_watcher_thread_func_poll_fail);
  RUN_TEST(test_hot_reload_destroy_join_fail);
  RUN_TEST(test_hot_reload_init_logger_fail);
}
