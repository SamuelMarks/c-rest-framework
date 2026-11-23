/* clang-format off */
#include "c_rest_error.h"
#include "greatest.h"
#include "greatest_clean.h"
#include "c_rest_mem.h"
#include "c_rest_modality.h"
#include "c_rest_orm_crud.h"
#include "c_rest_orm_middleware.h"
#include "c_rest_request.h"
#include "c_rest_response.h"
#include "c_rest_router.h"
#include "c_rest_testing_mocks.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void dummy_exit(int s) { (void)s; }
int app_notes_main(void);

static int g_mock_router_use_fail = 0;
static c_rest_error_t mock_notes_c_rest_router_use(c_rest_router *r,
                                                   const char *p,
                                                   c_rest_middleware_fn m,
                                                   void *u) {
  if (g_mock_router_use_fail)
    return C_REST_ERROR_GENERIC;
  return c_rest_router_use(r, p, m, u);
}

static int g_mock_router_use_post_fail = 0;
static c_rest_error_t mock_notes_c_rest_router_use_post(c_rest_router *r,
                                                        const char *p,
                                                        c_rest_middleware_fn m,
                                                        void *u) {
  if (g_mock_router_use_post_fail)
    return C_REST_ERROR_GENERIC;
  return c_rest_router_use_post(r, p, m, u);
}

static int g_mock_set_router_fail = 0;
static c_rest_error_t mock_notes_c_rest_set_router(struct c_rest_context *c,
                                                   c_rest_router *r) {
  if (g_mock_set_router_fail)
    return C_REST_ERROR_GENERIC;
  return c_rest_set_router(c, r);
}

static int g_mock_router_add_fail_at = 0;
static int g_mock_router_add_count = 0;
static c_rest_error_t mock_notes_c_rest_router_add(c_rest_router *r,
                                                   const char *m, const char *p,
                                                   c_rest_handler_fn h,
                                                   void *u) {
  g_mock_router_add_count++;
  if (g_mock_router_add_fail_at > 0 &&
      g_mock_router_add_count == g_mock_router_add_fail_at) {
    return C_REST_ERROR_GENERIC;
  }
  return c_rest_router_add(r, m, p, h, u);
}

static int g_mock_router_destroy_fail = 0;
static c_rest_error_t mock_notes_c_rest_router_destroy(c_rest_router *r) {
  if (g_mock_router_destroy_fail)
    return C_REST_ERROR_GENERIC;
  return c_rest_router_destroy(r);
}

static int g_mock_ctx_destroy_fail = 0;
static c_rest_error_t mock_notes_c_rest_destroy(struct c_rest_context *c) {
  if (g_mock_ctx_destroy_fail)
    return C_REST_ERROR_GENERIC;
  return c_rest_destroy(c);
}

#define exit(s) dummy_exit(s)
#define c_rest_router_use mock_notes_c_rest_router_use
#define c_rest_router_use_post mock_notes_c_rest_router_use_post
#define c_rest_set_router mock_notes_c_rest_set_router
#define c_rest_router_add mock_notes_c_rest_router_add
#define c_rest_router_destroy mock_notes_c_rest_router_destroy
#define c_rest_destroy mock_notes_c_rest_destroy
#define main app_notes_main
#include "main.c"
#undef main
#undef c_rest_destroy
#undef c_rest_router_destroy
#undef c_rest_router_add
#undef c_rest_set_router
#undef c_rest_router_use_post
#undef c_rest_router_use
#undef exit
/* clang-format on */

/**
 * @brief Memory allocation hook that always fails.
 * @param size Requested allocation size.
 * @return NULL.
 */
static void *fail_malloc(size_t size) {
  (void)size;
  return NULL;
}

static int g_notes_alloc_count = 0;
static int g_notes_fail_at = 0;

/**
 * @brief Memory allocation hook that fails at a specific allocation count.
 * @param size Requested allocation size.
 * @return Allocated pointer or NULL on target count.
 */
static void *fail_malloc_at_n(size_t size) {
  g_notes_alloc_count++;
  if (g_notes_alloc_count == g_notes_fail_at) {
    g_notes_fail_at = 0;
    return NULL;
  }
  return malloc(size);
}

TEST test_notes_main_and_error_paths(void) {
  struct c_rest_request req;
  struct c_rest_response res;
  int i;

  ASSERT_EQ(0, app_notes_main());

  sig_handler(0);

  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));
  ASSERT_EQ(C_REST_OK, custom_404_handler(&req, &res, NULL));
  ASSERT_EQ(404, res.status_code);
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));

  ASSERT_NEQ(C_REST_OK, custom_404_handler(&req, NULL, NULL));

  g_crf_malloc_hook = fail_malloc;
  ASSERT_EQ(1, app_notes_main());
  g_crf_malloc_hook = NULL;

  g_notes_alloc_count = 0;
  g_notes_fail_at = 3;
  g_crf_malloc_hook = fail_malloc_at_n;
  ASSERT_EQ(1, app_notes_main());
  g_crf_malloc_hook = NULL;
  g_notes_fail_at = 0;

  g_mock_router_use_fail = 1;
  ASSERT_EQ(1, app_notes_main());
  g_mock_router_use_fail = 0;

  g_mock_router_use_post_fail = 1;
  ASSERT_EQ(1, app_notes_main());
  g_mock_router_use_post_fail = 0;

  g_mock_set_router_fail = 1;
  ASSERT_EQ(1, app_notes_main());
  g_mock_set_router_fail = 0;

  for (i = 1; i <= 6; i++) {
    g_mock_router_add_count = 0;
    g_mock_router_add_fail_at = i;
    ASSERT_EQ(1, app_notes_main());
  }
  g_mock_router_add_fail_at = 0;
  g_mock_router_add_count = 0;

  g_mock_router_destroy_fail = 1;
  ASSERT_EQ(1, app_notes_main());
  g_mock_router_destroy_fail = 0;

  g_mock_ctx_destroy_fail = 1;
  ASSERT_EQ(1, app_notes_main());
  g_mock_ctx_destroy_fail = 0;

  PASS();
}

TEST test_notes_crud_routes(void) {
  struct c_rest_context *ctx = NULL;
  c_rest_router *router = NULL;
  struct c_rest_request req;
  struct c_rest_response res;
  c_rest_error_t run_rc;
  char get_method[] = "GET";
  char post_method[] = "POST";
  char notes_path[] = "/api/v0/notes";
  char post_body[] = "{\"title\":\"My Note\",\"content\":\"Hello world\"}";

  ASSERT_EQ(C_REST_OK, c_rest_init(C_REST_MODALITY_SYNC, &ctx));
  ctx->listen_address = NULL;
  ctx->listen_port = 0;
  ctx->db_config.connection_string = "sqlite://:memory:";
  run_rc = c_rest_run(ctx);
  ASSERT(run_rc == C_REST_OK || run_rc == C_REST_ERROR_GENERIC);

  ASSERT_EQ(C_REST_OK, c_rest_router_init(&router));
  ASSERT_EQ(C_REST_OK,
            c_rest_router_use(router, "/api/v0",
                              c_rest_orm_transaction_start_middleware, ctx));
  ASSERT_EQ(C_REST_OK,
            c_rest_router_use_post(router, "/api/v0",
                                   c_rest_orm_transaction_end_middleware, ctx));
  ASSERT_EQ(C_REST_OK,
            c_rest_router_add(router, "GET", "/api/v0/notes",
                              c_rest_orm_crud_get_list, &note_model));
  ASSERT_EQ(C_REST_OK, c_rest_router_add(router, "POST", "/api/v0/notes",
                                         c_rest_orm_crud_create, &note_model));

  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));
  req.method = get_method;
  req.path = notes_path;
  res.status_code = 200;

  ASSERT_EQ(C_REST_OK, c_rest_router_dispatch(router, &req, &res));
  ASSERT_EQ(200, res.status_code);
  ASSERT_EQ(C_REST_OK, c_rest_request_cleanup(&req));
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));

  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));
  req.method = post_method;
  req.path = notes_path;
  req.body = post_body;
  req.body_len = strlen(post_body);
  res.status_code = 200;

  ASSERT_EQ(C_REST_OK, c_rest_router_dispatch(router, &req, &res));
  ASSERT_EQ(201, res.status_code);

  req.body = NULL;
  ASSERT_EQ(C_REST_OK, c_rest_request_cleanup(&req));
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));

  ASSERT_EQ(C_REST_OK, c_rest_router_destroy(router));
  ASSERT_EQ(C_REST_OK, c_rest_destroy(ctx));
  PASS();
}

SUITE(notes_suite) {
  RUN_TEST(test_notes_main_and_error_paths);
  RUN_TEST(test_notes_crud_routes);
}

GREATEST_MAIN_DEFS();

/**
 * @brief Test entry point for the notes app.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Exit status code.
 */
int main(int argc, char **argv) {
  GREATEST_MAIN_BEGIN();
  RUN_SUITE(notes_suite);
  GREATEST_MAIN_END();
}
