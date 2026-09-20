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
  int failed = 0;
  struct c_rest_context *ctx = NULL;
  c_rest_router *router = NULL;
  struct c_rest_request req;
  struct c_rest_response res;
  struct c_rest_orm_model user_model;
  c_rest_error_t ret;
  int run_ok;

  printf("Running orm integration tests...\n");

  ret = c_rest_init(C_REST_MODALITY_SYNC, &ctx);
  failed += (ret != C_REST_OK);

  /* Explicitly test mock init edge cases to cover mock branches */
  ret = c_rest_orm_init(NULL, NULL);
  failed += (ret != C_REST_OK);

  ret = c_rest_orm_init(&ctx->db_config, NULL);
  failed += (ret != C_REST_OK);

  ret = c_rest_orm_cleanup(NULL);
  failed += (ret != C_REST_OK);

#ifdef C_REST_TESTING_MALLOC_HOOK
  {
    g_mock_orm_cleanup_fail = 1;
    ret = c_rest_orm_cleanup(NULL);
    g_mock_orm_cleanup_fail = 0;
    failed += (ret != C_REST_ERROR_GENERIC);
  }
#endif

  /* Mock database config */
  ctx->db_config.connection_string = "sqlite://:memory:";
  ret = c_rest_run(ctx); /* Will trigger c-orm pool mock init */
  run_ok = (ret == 0) | (ret == 1) | (ret == C_REST_ERROR_NOT_SUPPORTED);
  failed += (!run_ok);

  ret = c_rest_router_init(&router);
  failed += (ret != C_REST_OK);

  /* Register pre and post transaction middlewares */
  failed += (c_rest_router_use(router, "/api",
                               c_rest_orm_transaction_start_middleware,
                               ctx) != C_REST_OK);
  failed += (c_rest_router_use_post(router, "/api",
                                    c_rest_orm_transaction_end_middleware,
                                    ctx) != C_REST_OK);

  /* Register a CRUD handler */
  user_model.table_name = "users";
  user_model.primary_key = "id";
  failed +=
      (c_rest_router_add(router, "GET", "/api/users", c_rest_orm_crud_get_list,
                         &user_model) != C_REST_OK);

  /* Simulate request */
  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  req.method = "GET";
  req.path = "/api/users";
  res.status_code = 200;

  ret = c_rest_router_dispatch(router, &req, &res);
  failed += (ret != C_REST_OK);
  failed += (res.status_code != 200);
  failed += (res.body == NULL);
  {
    const char *body_ptr = res.body;
    int cmp_idx;
    for (cmp_idx = 0; cmp_idx < 2; cmp_idx++) {
      const char *s = (cmp_idx == 0) ? body_ptr : NULL;
      failed += (strcmp(s ? s : "[]", "[]") != 0);
    }
  }
  failed += (req.db_conn != NULL);

  /* Test c_orm_crud error paths (NULL model or req->db_conn == NULL) */
  {
    struct c_rest_request req_err;
    struct c_rest_response res_err;
    memset(&req_err, 0, sizeof(req_err));
    memset(&res_err, 0, sizeof(res_err));

    ret = c_rest_orm_crud_get_list(&req_err, &res_err, NULL);
    failed += (ret != C_REST_ERROR_GENERIC);
    failed += (res_err.status_code != 500);
    failed += (c_rest_response_cleanup(&res_err) != C_REST_OK);
    memset(&res_err, 0, sizeof(res_err));

    ret = c_rest_orm_crud_get_one(&req_err, &res_err, NULL);
    failed += (ret != C_REST_ERROR_GENERIC);
    failed += (res_err.status_code != 500);
    failed += (c_rest_response_cleanup(&res_err) != C_REST_OK);
    memset(&res_err, 0, sizeof(res_err));

    ret = c_rest_orm_crud_create(&req_err, &res_err, NULL);
    failed += (ret != C_REST_ERROR_GENERIC);
    failed += (res_err.status_code != 500);
    failed += (c_rest_response_cleanup(&res_err) != C_REST_OK);
    memset(&res_err, 0, sizeof(res_err));

    ret = c_rest_orm_crud_update(&req_err, &res_err, NULL);
    failed += (ret != C_REST_ERROR_GENERIC);
    failed += (res_err.status_code != 500);
    failed += (c_rest_response_cleanup(&res_err) != C_REST_OK);
    memset(&res_err, 0, sizeof(res_err));

    ret = c_rest_orm_crud_delete(&req_err, &res_err, NULL);
    failed += (ret != C_REST_ERROR_GENERIC);
    failed += (res_err.status_code != 500);
    failed += (c_rest_response_cleanup(&res_err) != C_REST_OK);

    /* Test set_status failure when res is NULL in error branch */
    ret = c_rest_orm_crud_get_list(&req_err, NULL, NULL);
    failed += (ret != C_REST_ERROR_GENERIC);
    ret = c_rest_orm_crud_get_one(&req_err, NULL, NULL);
    failed += (ret != C_REST_ERROR_GENERIC);
    ret = c_rest_orm_crud_create(&req_err, NULL, NULL);
    failed += (ret != C_REST_ERROR_GENERIC);
    ret = c_rest_orm_crud_update(&req_err, NULL, NULL);
    failed += (ret != C_REST_ERROR_GENERIC);
    ret = c_rest_orm_crud_delete(&req_err, NULL, NULL);
    failed += (ret != C_REST_ERROR_GENERIC);
    ret = c_rest_orm_health_check(&req_err, NULL, NULL);
    failed += (ret != C_REST_ERROR_GENERIC);
  }

  /* Test c_orm_crud happy paths */
  {
    struct c_rest_request req_ok;
    struct c_rest_response res_ok;
    memset(&req_ok, 0, sizeof(req_ok));
    memset(&res_ok, 0, sizeof(res_ok));
    req_ok.db_conn = (void *)1;

    /* Test set_status failure when res is NULL in happy branch */
    failed += (c_rest_orm_crud_get_list(&req_ok, NULL, &user_model) !=
               C_REST_ERROR_GENERIC);
    failed += (c_rest_orm_crud_get_one(&req_ok, NULL, &user_model) !=
               C_REST_ERROR_GENERIC);
    failed += (c_rest_orm_crud_create(&req_ok, NULL, &user_model) !=
               C_REST_ERROR_GENERIC);
    failed += (c_rest_orm_crud_update(&req_ok, NULL, &user_model) !=
               C_REST_ERROR_GENERIC);
    failed += (c_rest_orm_crud_delete(&req_ok, NULL, &user_model) !=
               C_REST_ERROR_GENERIC);
    failed +=
        (c_rest_orm_health_check(&req_ok, NULL, NULL) != C_REST_ERROR_GENERIC);

    ret = c_rest_orm_crud_get_list(&req_ok, &res_ok, &user_model);
    failed += (ret != C_REST_OK);
    failed += (res_ok.status_code != 200);
    failed += (c_rest_response_cleanup(&res_ok) != C_REST_OK);
    memset(&res_ok, 0, sizeof(res_ok));

    ret = c_rest_orm_crud_get_one(&req_ok, &res_ok, &user_model);
    failed += (ret != C_REST_OK);
    failed += (res_ok.status_code != 200);
    failed += (c_rest_response_cleanup(&res_ok) != C_REST_OK);
    memset(&res_ok, 0, sizeof(res_ok));

    ret = c_rest_orm_crud_create(&req_ok, &res_ok, &user_model);
    failed += (ret != C_REST_OK);
    failed += (res_ok.status_code != 201);
    failed += (c_rest_response_cleanup(&res_ok) != C_REST_OK);
    memset(&res_ok, 0, sizeof(res_ok));

    ret = c_rest_orm_crud_update(&req_ok, &res_ok, &user_model);
    failed += (ret != C_REST_OK);
    failed += (res_ok.status_code != 200);
    failed += (c_rest_response_cleanup(&res_ok) != C_REST_OK);
    memset(&res_ok, 0, sizeof(res_ok));

    ret = c_rest_orm_crud_delete(&req_ok, &res_ok, &user_model);
    failed += (ret != C_REST_OK);
    failed += (res_ok.status_code != 204);
    failed += (c_rest_response_cleanup(&res_ok) != C_REST_OK);
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
    failed += (ret != C_REST_ERROR_GENERIC);
    failed += (c_rest_response_cleanup(&res_fail) != C_REST_OK);
    memset(&res_fail, 0, sizeof(res_fail));

    g_crf_malloc_hook = fail_malloc;
    ret = c_rest_orm_crud_get_one(&req_fail, &res_fail, &user_model);
    g_crf_malloc_hook = NULL;
    failed += (ret != C_REST_ERROR_GENERIC);
    failed += (c_rest_response_cleanup(&res_fail) != C_REST_OK);
    memset(&res_fail, 0, sizeof(res_fail));

    g_crf_malloc_hook = fail_malloc;
    ret = c_rest_orm_crud_create(&req_fail, &res_fail, &user_model);
    g_crf_malloc_hook = NULL;
    failed += (ret != C_REST_ERROR_GENERIC);
    failed += (c_rest_response_cleanup(&res_fail) != C_REST_OK);
    memset(&res_fail, 0, sizeof(res_fail));

    g_crf_malloc_hook = fail_malloc;
    ret = c_rest_orm_crud_update(&req_fail, &res_fail, &user_model);
    g_crf_malloc_hook = NULL;
    failed += (ret != C_REST_ERROR_GENERIC);
    failed += (c_rest_response_cleanup(&res_fail) != C_REST_OK);
    memset(&res_fail, 0, sizeof(res_fail));

    /* Now test malloc failure on the 500 error branch (req.db_conn = NULL) */
    req_fail.db_conn = NULL;

    g_crf_malloc_hook = fail_malloc;
    ret = c_rest_orm_crud_get_list(&req_fail, &res_fail, &user_model);
    g_crf_malloc_hook = NULL;
    failed += (ret != C_REST_ERROR_GENERIC);
    failed += (c_rest_response_cleanup(&res_fail) != C_REST_OK);
    memset(&res_fail, 0, sizeof(res_fail));

    ret = c_rest_orm_crud_get_one(&req_fail, &res_fail, &user_model);
    failed += (ret != C_REST_ERROR_GENERIC);
    failed += (c_rest_response_cleanup(&res_fail) != C_REST_OK);
    memset(&res_fail, 0, sizeof(res_fail));

    ret = c_rest_orm_crud_create(&req_fail, &res_fail, &user_model);
    failed += (ret != C_REST_ERROR_GENERIC);
    failed += (c_rest_response_cleanup(&res_fail) != C_REST_OK);
    memset(&res_fail, 0, sizeof(res_fail));

    ret = c_rest_orm_crud_update(&req_fail, &res_fail, &user_model);
    failed += (ret != C_REST_ERROR_GENERIC);
    failed += (c_rest_response_cleanup(&res_fail) != C_REST_OK);
    memset(&res_fail, 0, sizeof(res_fail));

    ret = c_rest_orm_crud_delete(&req_fail, &res_fail, &user_model);
    failed += (ret != C_REST_ERROR_GENERIC);
    failed += (c_rest_response_cleanup(&res_fail) != C_REST_OK);
    memset(&res_fail, 0, sizeof(res_fail));

    /* also test NULL res to fail set_status */
    ret = c_rest_orm_crud_get_list(&req_fail, NULL, &user_model);
    failed += (ret != C_REST_ERROR_GENERIC);

    req_fail.db_conn = (void *)1;
    ret = c_rest_orm_crud_get_list(&req_fail, NULL, &user_model);
    failed += (ret != C_REST_ERROR_GENERIC);

    ret = c_rest_orm_crud_get_one(&req_fail, NULL, &user_model);
    failed += (ret != C_REST_ERROR_GENERIC);

    ret = c_rest_orm_crud_create(&req_fail, NULL, &user_model);
    failed += (ret != C_REST_ERROR_GENERIC);

    ret = c_rest_orm_crud_update(&req_fail, NULL, &user_model);
    failed += (ret != C_REST_ERROR_GENERIC);

    ret = c_rest_orm_crud_delete(&req_fail, NULL, &user_model);
    failed += (ret != C_REST_ERROR_GENERIC);

    ret = c_rest_orm_health_check(&req_fail, NULL, NULL);
    failed += (ret != C_REST_ERROR_GENERIC);
    req_fail.db_conn = NULL;
    ret = c_rest_orm_health_check(&req_fail, NULL, NULL);
    failed += (ret != C_REST_ERROR_GENERIC);
  }

  /* Test health check */
  {
    struct c_rest_request req_hc;
    struct c_rest_response res_hc;
    memset(&req_hc, 0, sizeof(req_hc));
    memset(&res_hc, 0, sizeof(res_hc));

    /* Unhealthy (no db_conn) */
    ret = c_rest_orm_health_check(&req_hc, &res_hc, NULL);
    failed += (ret != C_REST_OK);
    failed += (res_hc.status_code != 503);
    failed += (c_rest_response_cleanup(&res_hc) != C_REST_OK);
    memset(&res_hc, 0, sizeof(res_hc));

    /* Unhealthy (req is NULL) */
    ret = c_rest_orm_health_check(NULL, &res_hc, NULL);
    failed += (ret != C_REST_OK);
    failed += (res_hc.status_code != 503);
    failed += (c_rest_response_cleanup(&res_hc) != C_REST_OK);
    memset(&res_hc, 0, sizeof(res_hc));

    /* Unhealthy with malloc fail */
    g_crf_malloc_hook = fail_malloc;
    ret = c_rest_orm_health_check(&req_hc, &res_hc, NULL);
    g_crf_malloc_hook = NULL;
    failed += (ret != C_REST_ERROR_GENERIC);
    failed += (c_rest_response_cleanup(&res_hc) != C_REST_OK);
    memset(&res_hc, 0, sizeof(res_hc));

    /* Healthy */
    req_hc.db_conn = (void *)1;
    ret = c_rest_orm_health_check(&req_hc, &res_hc, NULL);
    failed += (ret != C_REST_OK);
    failed += (res_hc.status_code != 200);
    failed += (c_rest_response_cleanup(&res_hc) != C_REST_OK);
    memset(&res_hc, 0, sizeof(res_hc));

    /* Healthy with malloc fail */
    g_crf_malloc_hook = fail_malloc;
    ret = c_rest_orm_health_check(&req_hc, &res_hc, NULL);
    g_crf_malloc_hook = NULL;
    failed += (ret != C_REST_ERROR_GENERIC);
    failed += (c_rest_response_cleanup(&res_hc) != C_REST_OK);
    memset(&res_hc, 0, sizeof(res_hc));
  }

  /* Test run migrations */
  {
    struct c_rest_context dummy_ctx;
    memset(&dummy_ctx, 0, sizeof(dummy_ctx));
    ret = c_rest_orm_run_migrations(&dummy_ctx, "/tmp/migrations");
    failed += (ret != C_REST_ERROR_GENERIC);

    ret = c_rest_orm_run_migrations(NULL, NULL);
    failed += (ret != C_REST_ERROR_GENERIC);

    ret = c_rest_orm_run_migrations(ctx, NULL);
    failed += (ret != C_REST_ERROR_GENERIC);

    /* ctx has db_pool because of c_rest_run above */
    ret = c_rest_orm_run_migrations(ctx, "/tmp/migrations");
    failed += (ret != C_REST_OK);

    /* test with logger */
    ctx->logger.log_cb = dummy_log_cb;
    ret = c_rest_orm_run_migrations(ctx, "/tmp/migrations");
    failed += (ret != C_REST_OK);

    /* test with logger failure */
    ctx->logger.log_cb = fail_log_cb;
    ret = c_rest_orm_run_migrations(ctx, "/tmp/migrations");
    failed += (ret != C_REST_ERROR_GENERIC);

    ctx->logger.log_cb = NULL;
  }

  /* Test middleware branches directly */
  ret = c_rest_orm_transaction_start_middleware(NULL, NULL, NULL);
  failed += (ret != C_REST_ERROR_GENERIC);

  ret = c_rest_orm_transaction_start_middleware(&req, NULL, NULL);
  failed += (ret != C_REST_ERROR_GENERIC);

  /* ctx->db_pool == NULL branch */
  {
    void *orig_pool = ctx->db_pool;
    ctx->db_pool = NULL;
    req.db_conn = NULL;
    ret = c_rest_orm_transaction_start_middleware(&req, NULL, ctx);
    failed += (ret != C_REST_OK);
    failed += (req.db_conn != NULL);
    ctx->db_pool = orig_pool;
  }

  /* End middleware null req */
  ret = c_rest_orm_transaction_end_middleware(NULL, NULL, NULL);
  failed += (ret != C_REST_OK);

  /* End middleware req->db_conn null */
  req.db_conn = NULL;
  ret = c_rest_orm_transaction_end_middleware(&req, NULL, NULL);
  failed += (ret != C_REST_OK);

  /* End middleware rollback branch (status >= 400) */
  req.db_conn = (struct c_orm_connection *)1;
  res.status_code = 400;
  ret = c_rest_orm_transaction_end_middleware(&req, &res, NULL);
  failed += (ret != C_REST_OK);
  failed += (req.db_conn != NULL);

  /* End middleware commit branch (res NULL) */
  req.db_conn = (struct c_orm_connection *)1;
  ret = c_rest_orm_transaction_end_middleware(&req, NULL, NULL);
  failed += (ret != C_REST_OK);
  failed += (req.db_conn != NULL);

  failed += (c_rest_request_cleanup(&req) != C_REST_OK);
  failed += (c_rest_response_cleanup(&res) != C_REST_OK);

  failed += (c_rest_router_destroy(router) != C_REST_OK);
  failed += (c_rest_destroy(ctx) != C_REST_OK);

  return failed;
}
