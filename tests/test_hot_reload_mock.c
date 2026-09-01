/* clang-format off */
#include "c_rest_error.h"
#include "greatest.h"
#include <string.h>

#undef C_REST_EXPORT
#define C_REST_EXPORT

#include "c_rest_platform.h"
#include "c_rest_hot_reload.h"

/* Forward declarations */
extern c_rest_error_t c_rest_thread_join(c_rest_thread_t thread);

static int g_mock_join_countdown = -1;

static c_rest_error_t mock_c_rest_thread_join(c_rest_thread_t thread) {
    if (g_mock_join_countdown >= 0) {
        if (g_mock_join_countdown == 0) return C_REST_ERROR_GENERIC;
        g_mock_join_countdown--;
    }
    return c_rest_thread_join(thread);
}

/* Preprocessor Injection */
#define c_rest_thread_join mock_c_rest_thread_join

/* Rename the tested function */
#define c_rest_hot_reload_destroy test_c_rest_hot_reload_destroy

/* Include the actual source file */
#include "../src/c_rest_hot_reload.c"

#undef c_rest_thread_join
/* clang-format on */

static void reset_mocks(void *data) {
  (void)data;
  g_mock_join_countdown = -1;
}

static c_rest_error_t fail_logger_cb(const char *msg) {
  (void)msg;
  return C_REST_ERROR_GENERIC; /* Fail the logger to make poll fail */
}

static c_rest_error_t dummy_on_reload(void *user_data) {
  (void)user_data;
  return C_REST_OK;
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
  ctx->watched_paths = malloc(sizeof(char *));
  ctx->watched_paths[0] = strdup("test_file_fake");
  ctx->last_modified_times = malloc(sizeof(time_t));
  ctx->last_modified_times[0] = 12345; /* Fake mtime */

  /* Call the static watcher_thread_func directly */
  rc = watcher_thread_func(ctx);
  printf("RC is %d\n", rc);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);

  test_c_rest_hot_reload_destroy(ctx);
  PASS();
}

TEST test_hot_reload_destroy_join_fail(void) {
  c_rest_hot_reload_ctx_t *ctx = NULL;
  c_rest_error_t rc;

  rc = c_rest_hot_reload_init(&ctx, NULL);
  ASSERT_EQ(C_REST_OK, rc);
  ASSERT(ctx != NULL);

  ctx->watcher_thread = (c_rest_thread_t)1; /* Fake thread handle */

  g_mock_join_countdown = 0; /* Fail on the first call to join */

  rc = test_c_rest_hot_reload_destroy(ctx);
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            rc); /* It should return the ret_rc error from join */
  PASS();
}

SUITE(hot_reload_mock_suite) {
  SET_SETUP(reset_mocks, NULL);
  RUN_TEST(test_watcher_thread_func_poll_fail);
  RUN_TEST(test_hot_reload_destroy_join_fail);
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
  GREATEST_MAIN_BEGIN();
  RUN_SUITE(hot_reload_mock_suite);
  GREATEST_MAIN_END();
}
