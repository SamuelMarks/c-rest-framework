/* clang-format off */
#include "c_rest_error.h"
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

/**
 * @brief Test entry point for the notes app.
 * @return Exit status code.
 */
int main(void) {
  struct c_rest_context *ctx = NULL;
  c_rest_router *router = NULL;
  struct c_rest_request req;
  struct c_rest_response res;
  c_rest_error_t ret;
  int failed = 0;
  const char *msgs[2];

  printf("Running tests for Notes App...\n");

  /* Test main.c app_notes_main normal path */
  ret = (c_rest_error_t)app_notes_main();
  failed += (ret != C_REST_OK);

  /* Test sig_handler */
  sig_handler(0);

  /* Test custom_404_handler */
  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));
  ret = custom_404_handler(&req, &res, NULL);
  failed += (ret != C_REST_OK);
  failed += (res.status_code != 404);
  ret = c_rest_response_cleanup(&res);
  failed += (ret != C_REST_OK);

  /* Test custom_404_handler fail response_set_status (line 30-31) */
  ret = custom_404_handler(&req, NULL, NULL);
  failed += (ret == C_REST_OK);

  /* Test app_notes_main fail c_rest_init */
  g_crf_malloc_hook = fail_malloc;
  ret = (c_rest_error_t)app_notes_main();
  failed += (ret != (c_rest_error_t)1);
  g_crf_malloc_hook = NULL;

  /* Test app_notes_main fail c_rest_router_init */
  g_notes_alloc_count = 0;
  g_notes_fail_at = 3;
  g_crf_malloc_hook = fail_malloc_at_n;
  ret = (c_rest_error_t)app_notes_main();
  failed += (ret != (c_rest_error_t)1);
  g_crf_malloc_hook = NULL;
  g_notes_fail_at = 0;

  /* Test app_notes_main fail c_rest_router_use (lines 82-85) */
  g_mock_router_use_fail = 1;
  ret = (c_rest_error_t)app_notes_main();
  failed += (ret != (c_rest_error_t)1);
  g_mock_router_use_fail = 0;

  /* Test app_notes_main fail c_rest_router_use_post (lines 89-92) */
  g_mock_router_use_post_fail = 1;
  ret = (c_rest_error_t)app_notes_main();
  failed += (ret != (c_rest_error_t)1);
  g_mock_router_use_post_fail = 0;

  /* Test app_notes_main fail c_rest_set_router (lines 97-100) */
  g_mock_set_router_fail = 1;
  ret = (c_rest_error_t)app_notes_main();
  failed += (ret != (c_rest_error_t)1);
  g_mock_set_router_fail = 0;

  /* Test app_notes_main fail c_rest_router_add 1-6 (lines 104-143) */
  {
    int i;
    for (i = 1; i <= 6; i++) {
      g_mock_router_add_count = 0;
      g_mock_router_add_fail_at = i;
      ret = (c_rest_error_t)app_notes_main();
      failed += (ret != (c_rest_error_t)1);
    }
    g_mock_router_add_fail_at = 0;
    g_mock_router_add_count = 0;
  }

  /* Test app_notes_main fail c_rest_router_destroy (lines 156-158) */
  g_mock_router_destroy_fail = 1;
  ret = (c_rest_error_t)app_notes_main();
  failed += (ret != (c_rest_error_t)1);
  g_mock_router_destroy_fail = 0;

  /* Test app_notes_main fail c_rest_destroy (lines 161-162) */
  g_mock_ctx_destroy_fail = 1;
  ret = (c_rest_error_t)app_notes_main();
  failed += (ret != (c_rest_error_t)1);
  g_mock_ctx_destroy_fail = 0;

  ret = c_rest_init(C_REST_MODALITY_SYNC, &ctx);
  failed += (ret != C_REST_OK);

  ctx->db_config.connection_string = "sqlite://:memory:";

  /* This is just to trigger the mock DB pool creation inside c_rest_run
     if that is how c-orm mock works in test mode */
  ret = c_rest_run(ctx);

  ret = c_rest_router_init(&router);
  failed += (ret != C_REST_OK);

  ret = c_rest_router_use(router, "/api/v0",
                          c_rest_orm_transaction_start_middleware, ctx);
  failed += (ret != C_REST_OK);
  ret = c_rest_router_use_post(router, "/api/v0",
                               c_rest_orm_transaction_end_middleware, ctx);
  failed += (ret != C_REST_OK);

  ret = c_rest_router_add(router, "GET", "/api/v0/notes",
                          c_rest_orm_crud_get_list, &note_model);
  failed += (ret != C_REST_OK);
  ret = c_rest_router_add(router, "POST", "/api/v0/notes",
                          c_rest_orm_crud_create, &note_model);
  failed += (ret != C_REST_OK);

  /* Simulate GET /api/v0/notes */
  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  req.method = "GET";
  req.path = "/api/v0/notes";
  res.status_code = 200;

  ret = c_rest_router_dispatch(router, &req, &res);
  failed += (ret != C_REST_OK);
  failed += (res.status_code != 200);

  ret = c_rest_request_cleanup(&req);
  failed += (ret != C_REST_OK);
  ret = c_rest_response_cleanup(&res);
  failed += (ret != C_REST_OK);

  /* Simulate POST /api/v0/notes */
  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  req.method = "POST";
  req.path = "/api/v0/notes";
  req.body = "{\"title\":\"My Note\",\"content\":\"Hello world\"}";
  req.body_len = strlen(req.body);
  res.status_code = 200;

  ret = c_rest_router_dispatch(router, &req, &res);
  failed += (ret != C_REST_OK);
  failed += (res.status_code != 201);

  /* Since req.body was statically allocated, avoid double free */
  req.body = NULL;
  ret = c_rest_request_cleanup(&req);
  failed += (ret != C_REST_OK);
  ret = c_rest_response_cleanup(&res);
  failed += (ret != C_REST_OK);

  ret = c_rest_router_destroy(router);
  failed += (ret != C_REST_OK);
  ret = c_rest_destroy(ctx);
  failed += (ret != C_REST_OK);

  msgs[0] = "All notes app tests passed.\n";
  msgs[1] = "Notes app tests failed.\n";
  printf("%s", msgs[failed != 0]);
  return failed;
}
