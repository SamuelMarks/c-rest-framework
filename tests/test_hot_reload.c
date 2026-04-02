/* clang-format off */
#include "test_protos.h"
#include "test_protos.h"
#include "c_rest_hot_reload.h"
#include "greatest.h"

#include <stdio.h>
#include <time.h>
/* clang-format on */

static void sleep_seconds(int seconds) {
  time_t start = time(NULL);
  while (time(NULL) - start < seconds) {
    /* busy wait */
  }
}

TEST test_hot_reload_init_destroy(void) {
  c_rest_hot_reload_ctx_t *ctx = NULL;
  int res;

  res = c_rest_hot_reload_init(&ctx, NULL);
  ASSERT_EQ(C_REST_HOT_RELOAD_SUCCESS, res);
  ASSERT(ctx != NULL);

  res = c_rest_hot_reload_destroy(ctx);
  ASSERT_EQ(C_REST_HOT_RELOAD_SUCCESS, res);
  PASS();
}

TEST test_hot_reload_add_watch(void) {
  c_rest_hot_reload_ctx_t *ctx = NULL;
  int res;

  res = c_rest_hot_reload_init(&ctx, NULL);
  ASSERT_EQ(C_REST_HOT_RELOAD_SUCCESS, res);

  res = c_rest_hot_reload_add_watch(ctx, "test_file.txt");
  ASSERT_EQ(C_REST_HOT_RELOAD_SUCCESS, res);

  res = c_rest_hot_reload_add_watch(ctx, "another_file.txt");
  ASSERT_EQ(C_REST_HOT_RELOAD_SUCCESS, res);

  res = c_rest_hot_reload_destroy(ctx);
  ASSERT_EQ(C_REST_HOT_RELOAD_SUCCESS, res);
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
  ASSERT_EQ(C_REST_HOT_RELOAD_SUCCESS, res);

  res = c_rest_hot_reload_start(ctx, dummy_reload_callback, &called);
  ASSERT_EQ(C_REST_HOT_RELOAD_SUCCESS, res);
  ASSERT_EQ(0, called);

  res = c_rest_hot_reload_destroy(ctx);
  ASSERT_EQ(C_REST_HOT_RELOAD_SUCCESS, res);
  PASS();
}

TEST test_hot_reload_modification(void) {
  c_rest_hot_reload_ctx_t *ctx = NULL;
  int res;
  int called = 0;
  FILE *f;
  const char *test_filename = "test_hot_reload_tmp.txt";

  /* Create file */
  f = fopen(test_filename, "w");
  ASSERT(f != NULL);
  fprintf(f, "hello\n");
  fclose(f);

  res = c_rest_hot_reload_init(&ctx, NULL);
  ASSERT_EQ(C_REST_HOT_RELOAD_SUCCESS, res);

  res = c_rest_hot_reload_add_watch(ctx, test_filename);
  ASSERT_EQ(C_REST_HOT_RELOAD_SUCCESS, res);

  /* Poll immediately, shouldn't be called because nothing changed */
  res = c_rest_hot_reload_poll(ctx, dummy_reload_callback, &called);
  ASSERT_EQ(C_REST_HOT_RELOAD_SUCCESS, res);
  ASSERT_EQ(0, called);

  /* Wait at least 1-2 seconds (file mtime usually has 1s resolution) */
  sleep_seconds(2);

  /* Modify file */
  f = fopen(test_filename, "a");
  ASSERT(f != NULL);
  fprintf(f, "world\n");
  fclose(f);

  /* Poll again, should detect change */
  res = c_rest_hot_reload_poll(ctx, dummy_reload_callback, &called);
  ASSERT_EQ(C_REST_HOT_RELOAD_SUCCESS, res);
  ASSERT_EQ(1, called);

  res = c_rest_hot_reload_destroy(ctx);
  ASSERT_EQ(C_REST_HOT_RELOAD_SUCCESS, res);

  remove(test_filename);
  PASS();
}

TEST test_hot_reload_edge_cases(void) {
  c_rest_hot_reload_ctx_t *ctx = NULL;
  int res;

  /* Invalid init */
  res = c_rest_hot_reload_init(NULL, NULL);
  ASSERT_EQ(C_REST_HOT_RELOAD_ERR_PARAM, res);

  /* Invalid watch */
  res = c_rest_hot_reload_add_watch(NULL, "test");
  ASSERT_EQ(C_REST_HOT_RELOAD_ERR_PARAM, res);

  res = c_rest_hot_reload_init(&ctx, NULL);
  ASSERT_EQ(C_REST_HOT_RELOAD_SUCCESS, res);

  res = c_rest_hot_reload_add_watch(ctx, NULL);
  ASSERT_EQ(C_REST_HOT_RELOAD_ERR_PARAM, res);

  /* Invalid start */
  res = c_rest_hot_reload_start(NULL, dummy_reload_callback, NULL);
  ASSERT_EQ(C_REST_HOT_RELOAD_ERR_PARAM, res);

  res = c_rest_hot_reload_start(ctx, NULL, NULL);
  ASSERT_EQ(C_REST_HOT_RELOAD_ERR_PARAM, res);

  /* Invalid poll */
  res = c_rest_hot_reload_poll(NULL, dummy_reload_callback, NULL);
  ASSERT_EQ(C_REST_HOT_RELOAD_ERR_PARAM, res);

  res = c_rest_hot_reload_poll(ctx, NULL, NULL);
  ASSERT_EQ(C_REST_HOT_RELOAD_ERR_PARAM, res);

  /* Invalid destroy */
  res = c_rest_hot_reload_destroy(NULL);
  ASSERT_EQ(C_REST_HOT_RELOAD_ERR_PARAM, res);

  res = c_rest_hot_reload_destroy(ctx);
  ASSERT_EQ(C_REST_HOT_RELOAD_SUCCESS, res);

  PASS();
}

SUITE(suite_hot_reload) {
  RUN_TEST(test_hot_reload_init_destroy);
  RUN_TEST(test_hot_reload_add_watch);
  RUN_TEST(test_hot_reload_start);
  RUN_TEST(test_hot_reload_modification);
  RUN_TEST(test_hot_reload_edge_cases);
}

int test_hot_reload(void) {
  GREATEST_INIT();
  RUN_SUITE(suite_hot_reload);
  return 0;
}
