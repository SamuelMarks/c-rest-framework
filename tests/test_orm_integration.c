/* clang-format off */
#include "c_rest_error.h"
#include "test_protos.h"
#include "c_rest_modality.h"
#include "c_rest_orm.h"
#include "c_rest_orm_crud.h"
#include "c_rest_orm_middleware.h"
#include "c_rest_request.h"
#include "c_rest_response.h"
#include "c_rest_router.h"

#include <stdio.h>
#include <string.h>
/* clang-format on */

static void *fail_malloc(size_t size) {
  (void)size;
  return NULL;
}

static c_rest_error_t dummy_log_cb(const char *msg) {
  (void)msg;
  return C_REST_OK;
}

static c_rest_error_t fail_log_cb(const char *msg) {
  (void)msg;
  return C_REST_ERROR_GENERIC;
}

int test_orm_integration(void) {
  struct c_rest_context *ctx = NULL;
  c_rest_router *router = NULL;
  struct c_rest_request req;
  struct c_rest_response res;
  struct c_rest_orm_model user_model;
  int ret;

  printf("Running orm integration tests...\n");

  ret = c_rest_init(C_REST_MODALITY_SYNC, &ctx);
  if (ret != 0) {
    printf("Failed to init context\n");
    return 1;
  }

  /* Explicitly test mock init edge cases to cover mock branches */
  ret = c_rest_orm_init(NULL, NULL);
  if (ret != C_REST_OK) {
    printf("c_rest_orm_init(NULL, NULL) failed\n");
    return 1;
  }
  ret = c_rest_orm_init(&ctx->db_config, NULL);
  if (ret != C_REST_OK) {
    printf("c_rest_orm_init(config, NULL) failed\n");
    return 1;
  }
  ret = c_rest_orm_cleanup(NULL);
  if (ret != C_REST_OK) {
    printf("c_rest_orm_cleanup(NULL) failed\n");
    return 1;
  }
#ifdef C_REST_TESTING_MALLOC_HOOK
  {
    g_mock_orm_cleanup_fail = 1;
    ret = c_rest_orm_cleanup(NULL);
    g_mock_orm_cleanup_fail = 0;
    if (ret != C_REST_ERROR_GENERIC) {
      printf("c_rest_orm_cleanup(NULL) mock fail failed\n");
      return 1;
    }
  }
#endif

  /* Mock database config */
  ctx->db_config.connection_string = "sqlite://:memory:";
  ret = c_rest_run(ctx); /* Will trigger c-orm pool mock init */
  if (ret != 0 &&
      ret !=
          1) { /* 1 is expected because no modality run loop is implemented */
    printf("c_rest_run returned unexpected error\n");
    return 1;
  }

  ret = c_rest_router_init(&router);
  if (ret != 0) {
    printf("Failed to init router\n");
    return 1;
  }

  /* Register pre and post transaction middlewares */
  (void)!c_rest_router_use(router, "/api",
                           c_rest_orm_transaction_start_middleware, ctx);
  (void)!c_rest_router_use_post(router, "/api",
                                c_rest_orm_transaction_end_middleware, ctx);

  /* Register a CRUD handler */
  user_model.table_name = "users";
  user_model.primary_key = "id";
  (void)!c_rest_router_add(router, "GET", "/api/users",
                           c_rest_orm_crud_get_list, &user_model);

  /* Simulate request */
  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  req.method = "GET";
  req.path = "/api/users";
  res.status_code = 200;

  ret = c_rest_router_dispatch(router, &req, &res);
  if (ret != 0) {
    printf("Dispatch failed\n");
    return 1;
  }

  if (res.status_code != 200) {
    printf("Expected status 200, got %d\n", res.status_code);
    return 1;
  }

  if (!res.body || strcmp(res.body, "[]") != 0) {
    printf("Expected body [], got %s\n", res.body ? res.body : "NULL");
    return 1;
  }

  if (req.db_conn != NULL) {
    printf("Transaction end middleware failed to clear db_conn\n");
    return 1;
  }

  /* Test c_orm_crud error paths (NULL model or req->db_conn == NULL) */
  {
    struct c_rest_request req_err;
    struct c_rest_response res_err;
    memset(&req_err, 0, sizeof(req_err));
    memset(&res_err, 0, sizeof(res_err));

    ret = c_rest_orm_crud_get_list(&req_err, &res_err, NULL);
    if (ret != C_REST_ERROR_GENERIC || res_err.status_code != 500)
      return 1;
    (void)!c_rest_response_cleanup(&res_err);
    memset(&res_err, 0, sizeof(res_err));

    ret = c_rest_orm_crud_get_one(&req_err, &res_err, NULL);
    if (ret != C_REST_ERROR_GENERIC || res_err.status_code != 500)
      return 1;
    (void)!c_rest_response_cleanup(&res_err);
    memset(&res_err, 0, sizeof(res_err));

    ret = c_rest_orm_crud_create(&req_err, &res_err, NULL);
    if (ret != C_REST_ERROR_GENERIC || res_err.status_code != 500)
      return 1;
    (void)!c_rest_response_cleanup(&res_err);
    memset(&res_err, 0, sizeof(res_err));

    ret = c_rest_orm_crud_update(&req_err, &res_err, NULL);
    if (ret != C_REST_ERROR_GENERIC || res_err.status_code != 500)
      return 1;
    (void)!c_rest_response_cleanup(&res_err);
    memset(&res_err, 0, sizeof(res_err));

    ret = c_rest_orm_crud_delete(&req_err, &res_err, NULL);
    if (ret != C_REST_ERROR_GENERIC || res_err.status_code != 500)
      return 1;
    (void)!c_rest_response_cleanup(&res_err);
  }

  /* Test c_orm_crud happy paths */
  {
    struct c_rest_request req_ok;
    struct c_rest_response res_ok;
    memset(&req_ok, 0, sizeof(req_ok));
    memset(&res_ok, 0, sizeof(res_ok));
    req_ok.db_conn = (void *)1;

    ret = c_rest_orm_crud_get_list(&req_ok, &res_ok, &user_model);
    if (ret != C_REST_OK || res_ok.status_code != 200)
      return 1;
    (void)!c_rest_response_cleanup(&res_ok);
    memset(&res_ok, 0, sizeof(res_ok));

    ret = c_rest_orm_crud_get_one(&req_ok, &res_ok, &user_model);
    if (ret != C_REST_OK || res_ok.status_code != 200)
      return 1;
    (void)!c_rest_response_cleanup(&res_ok);
    memset(&res_ok, 0, sizeof(res_ok));

    ret = c_rest_orm_crud_create(&req_ok, &res_ok, &user_model);
    if (ret != C_REST_OK || res_ok.status_code != 201)
      return 1;
    (void)!c_rest_response_cleanup(&res_ok);
    memset(&res_ok, 0, sizeof(res_ok));

    ret = c_rest_orm_crud_update(&req_ok, &res_ok, &user_model);
    if (ret != C_REST_OK || res_ok.status_code != 200)
      return 1;
    (void)!c_rest_response_cleanup(&res_ok);
    memset(&res_ok, 0, sizeof(res_ok));

    ret = c_rest_orm_crud_delete(&req_ok, &res_ok, &user_model);
    if (ret != C_REST_OK || res_ok.status_code != 204)
      return 1;
    (void)!c_rest_response_cleanup(&res_ok);
  }

  /* Test c_orm_crud malloc failure branches */
  {
    struct c_rest_request req_fail;
    struct c_rest_response res_fail;
    memset(&req_fail, 0, sizeof(req_fail));
    memset(&res_fail, 0, sizeof(res_fail));
    req_fail.db_conn = (void *)1;

    /* Malloc failure on success response json */
    g_crf_malloc_hook = fail_malloc;
    ret = c_rest_orm_crud_get_list(&req_fail, &res_fail, &user_model);
    g_crf_malloc_hook = NULL;
    if (ret != C_REST_ERROR_GENERIC)
      return 1;
    (void)!c_rest_response_cleanup(&res_fail);
    memset(&res_fail, 0, sizeof(res_fail));

    g_crf_malloc_hook = fail_malloc;
    ret = c_rest_orm_crud_get_one(&req_fail, &res_fail, &user_model);
    g_crf_malloc_hook = NULL;
    if (ret != C_REST_ERROR_GENERIC)
      return 1;
    (void)!c_rest_response_cleanup(&res_fail);
    memset(&res_fail, 0, sizeof(res_fail));

    g_crf_malloc_hook = fail_malloc;
    ret = c_rest_orm_crud_create(&req_fail, &res_fail, &user_model);
    g_crf_malloc_hook = NULL;
    if (ret != C_REST_ERROR_GENERIC)
      return 1;
    (void)!c_rest_response_cleanup(&res_fail);
    memset(&res_fail, 0, sizeof(res_fail));

    g_crf_malloc_hook = fail_malloc;
    ret = c_rest_orm_crud_update(&req_fail, &res_fail, &user_model);
    g_crf_malloc_hook = NULL;
    if (ret != C_REST_ERROR_GENERIC)
      return 1;
    (void)!c_rest_response_cleanup(&res_fail);
    memset(&res_fail, 0, sizeof(res_fail));

    /* delete doesn't use json, so malloc failure might not affect it. Let's
     * just check normally */
    /* Actually, c_rest_response_set_status doesn't malloc, so delete still
     * succeeds if malloc fails. */

    /* Now test malloc failure on the 500 error branch (req.db_conn = NULL) */
    req_fail.db_conn = NULL;

    g_crf_malloc_hook = fail_malloc;
    ret = c_rest_orm_crud_get_list(&req_fail, &res_fail, &user_model);
    g_crf_malloc_hook = NULL;
    if (ret != C_REST_ERROR_GENERIC)
      return 1;
    (void)!c_rest_response_cleanup(&res_fail);
    memset(&res_fail, 0, sizeof(res_fail));

    ret = c_rest_orm_crud_get_one(&req_fail, &res_fail, &user_model);
    if (ret != C_REST_ERROR_GENERIC)
      return 1;
    (void)!c_rest_response_cleanup(&res_fail);
    memset(&res_fail, 0, sizeof(res_fail));

    ret = c_rest_orm_crud_create(&req_fail, &res_fail, &user_model);
    if (ret != C_REST_ERROR_GENERIC)
      return 1;
    (void)!c_rest_response_cleanup(&res_fail);
    memset(&res_fail, 0, sizeof(res_fail));

    ret = c_rest_orm_crud_update(&req_fail, &res_fail, &user_model);
    if (ret != C_REST_ERROR_GENERIC)
      return 1;
    (void)!c_rest_response_cleanup(&res_fail);
    memset(&res_fail, 0, sizeof(res_fail));

    ret = c_rest_orm_crud_delete(&req_fail, &res_fail, &user_model);
    if (ret != C_REST_ERROR_GENERIC)
      return 1;
    (void)!c_rest_response_cleanup(&res_fail);
    memset(&res_fail, 0, sizeof(res_fail));

    /* also test NULL res to fail set_status */
    ret = c_rest_orm_crud_get_list(&req_fail, NULL, &user_model);
    if (ret != C_REST_ERROR_GENERIC)
      return 1;

    req_fail.db_conn = (void *)1;
    ret = c_rest_orm_crud_get_list(&req_fail, NULL, &user_model);
    if (ret != C_REST_ERROR_GENERIC)
      return 1;

    ret = c_rest_orm_crud_get_one(&req_fail, NULL, &user_model);
    if (ret != C_REST_ERROR_GENERIC)
      return 1;

    ret = c_rest_orm_crud_create(&req_fail, NULL, &user_model);
    if (ret != C_REST_ERROR_GENERIC)
      return 1;

    ret = c_rest_orm_crud_update(&req_fail, NULL, &user_model);
    if (ret != C_REST_ERROR_GENERIC)
      return 1;

    ret = c_rest_orm_crud_delete(&req_fail, NULL, &user_model);
    if (ret != C_REST_ERROR_GENERIC)
      return 1;

    ret = c_rest_orm_health_check(&req_fail, NULL, NULL);
    if (ret != C_REST_ERROR_GENERIC)
      return 1;
    req_fail.db_conn = NULL;
    ret = c_rest_orm_health_check(&req_fail, NULL, NULL);
    if (ret != C_REST_ERROR_GENERIC)
      return 1;
  }

  /* Test health check */
  {
    struct c_rest_request req_hc;
    struct c_rest_response res_hc;
    memset(&req_hc, 0, sizeof(req_hc));
    memset(&res_hc, 0, sizeof(res_hc));

    /* Unhealthy (no db_conn) */
    ret = c_rest_orm_health_check(&req_hc, &res_hc, NULL);
    if (ret != C_REST_OK || res_hc.status_code != 503)
      return 1;
    (void)!c_rest_response_cleanup(&res_hc);
    memset(&res_hc, 0, sizeof(res_hc));

    /* Unhealthy (req is NULL) */
    ret = c_rest_orm_health_check(NULL, &res_hc, NULL);
    if (ret != C_REST_OK || res_hc.status_code != 503)
      return 1;
    (void)!c_rest_response_cleanup(&res_hc);
    memset(&res_hc, 0, sizeof(res_hc));

    /* Unhealthy with malloc fail */
    g_crf_malloc_hook = fail_malloc;
    ret = c_rest_orm_health_check(&req_hc, &res_hc, NULL);
    g_crf_malloc_hook = NULL;
    if (ret != C_REST_ERROR_GENERIC)
      return 1;
    (void)!c_rest_response_cleanup(&res_hc);
    memset(&res_hc, 0, sizeof(res_hc));

    /* Healthy */
    req_hc.db_conn = (void *)1;
    ret = c_rest_orm_health_check(&req_hc, &res_hc, NULL);
    if (ret != C_REST_OK || res_hc.status_code != 200)
      return 1;
    (void)!c_rest_response_cleanup(&res_hc);
    memset(&res_hc, 0, sizeof(res_hc));

    /* Healthy with malloc fail */
    g_crf_malloc_hook = fail_malloc;
    ret = c_rest_orm_health_check(&req_hc, &res_hc, NULL);
    g_crf_malloc_hook = NULL;
    if (ret != C_REST_ERROR_GENERIC)
      return 1;
    (void)!c_rest_response_cleanup(&res_hc);
    memset(&res_hc, 0, sizeof(res_hc));
  }

  /* Test run migrations */
  {
    struct c_rest_context dummy_ctx;
    memset(&dummy_ctx, 0, sizeof(dummy_ctx));
    ret = c_rest_orm_run_migrations(&dummy_ctx, "/tmp/migrations");
    if (ret != C_REST_ERROR_GENERIC)
      return 1;

    ret = c_rest_orm_run_migrations(NULL, NULL);
    if (ret != C_REST_ERROR_GENERIC)
      return 1;

    ret = c_rest_orm_run_migrations(ctx, NULL);
    if (ret != C_REST_ERROR_GENERIC)
      return 1;

    /* ctx has db_pool because of c_rest_run above */
    ret = c_rest_orm_run_migrations(ctx, "/tmp/migrations");
    if (ret != C_REST_OK)
      return 1;

    /* test with logger */
    ctx->logger.log_cb = dummy_log_cb;
    ret = c_rest_orm_run_migrations(ctx, "/tmp/migrations");
    if (ret != C_REST_OK)
      return 1;

    /* test with logger failure */
    ctx->logger.log_cb = fail_log_cb;
    ret = c_rest_orm_run_migrations(ctx, "/tmp/migrations");
    if (ret != C_REST_ERROR_GENERIC)
      return 1;

    ctx->logger.log_cb = NULL;
  }

  /* Test middleware branches directly */
  ret = c_rest_orm_transaction_start_middleware(NULL, NULL, NULL);
  if (ret != C_REST_ERROR_GENERIC) {
    printf("c_rest_orm_transaction_start_middleware(NULL) failed to error\n");
    return 1;
  }
  ret = c_rest_orm_transaction_start_middleware(&req, NULL, NULL);
  if (ret != C_REST_ERROR_GENERIC) {
    printf("c_rest_orm_transaction_start_middleware(&req, NULL, NULL) failed "
           "to error\n");
    return 1;
  }

  /* ctx->db_pool == NULL branch */
  {
    void *orig_pool = ctx->db_pool;
    ctx->db_pool = NULL;
    req.db_conn = NULL;
    ret = c_rest_orm_transaction_start_middleware(&req, NULL, ctx);
    if (ret != C_REST_OK || req.db_conn != NULL) {
      printf("db_pool == NULL branch failed\n");
      return 1;
    }
    ctx->db_pool = orig_pool;
  }

  /* End middleware null req */
  ret = c_rest_orm_transaction_end_middleware(NULL, NULL, NULL);
  if (ret != C_REST_OK) {
    printf("c_rest_orm_transaction_end_middleware(NULL) failed\n");
    return 1;
  }

  /* End middleware req->db_conn null */
  req.db_conn = NULL;
  ret = c_rest_orm_transaction_end_middleware(&req, NULL, NULL);
  if (ret != C_REST_OK) {
    printf("c_rest_orm_transaction_end_middleware(no db_conn) failed\n");
    return 1;
  }

  /* End middleware rollback branch (status >= 400) */
  req.db_conn = (struct c_orm_connection *)1;
  res.status_code = 400;
  ret = c_rest_orm_transaction_end_middleware(&req, &res, NULL);
  if (ret != C_REST_OK || req.db_conn != NULL) {
    printf("c_rest_orm_transaction_end_middleware(status 400) failed\n");
    return 1;
  }

  /* End middleware commit branch (res NULL) */
  req.db_conn = (struct c_orm_connection *)1;
  ret = c_rest_orm_transaction_end_middleware(&req, NULL, NULL);
  if (ret != C_REST_OK || req.db_conn != NULL) {
    printf("c_rest_orm_transaction_end_middleware(res NULL) failed\n");
    return 1;
  }

  (void)!c_rest_request_cleanup(&req);
  (void)!c_rest_response_cleanup(&res);

  (void)!c_rest_router_destroy(router);
  (void)!c_rest_destroy(ctx);

  return 0;
}
