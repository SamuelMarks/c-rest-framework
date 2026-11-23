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
#include <cfs/cfs.h>

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

TEST test_orm_integration_cases(void) {
  struct c_rest_context *ctx = NULL;
  c_rest_router *router = NULL;
  struct c_rest_request req;
  struct c_rest_response res;
  struct c_rest_orm_model user_model;
  struct c_rest_orm_column cols[] = {{"id", C_REST_ORM_TYPE_INT}};
  c_rest_error_t ret;
  int run_ok;
  char get_str[] = "GET";
  char users_path[] = "/api/users";

  ret = c_rest_init(C_REST_MODALITY_SYNC, &ctx);
  ASSERT_FALSE(ret != C_REST_OK);

  /* Explicitly test pool init/cleanup validation branches */
  ret = c_rest_orm_init(NULL, NULL);
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG, ret);

  ret = c_rest_orm_init(&ctx->db_config, NULL);
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG, ret);

  ret = c_rest_orm_cleanup(NULL);
  ASSERT_EQ(C_REST_OK, ret);

  ret = c_rest_orm_cleanup((struct c_orm_pool *)2);
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG, ret);

  /* Mock database config */
  ctx->listen_address = NULL;
  ctx->listen_port = 0;
  ctx->db_config.connection_string = "sqlite://:memory:";
  ret = c_rest_run(ctx); /* Will trigger c-orm pool mock init */
  run_ok = (ret == 0) | (ret == 1) | (ret == C_REST_ERROR_NOT_SUPPORTED);
  ASSERT_FALSE(!run_ok);

  ret = c_rest_router_init(&router);
  ASSERT_FALSE(ret != C_REST_OK);

  /* Register pre and post transaction middlewares */
  ASSERT_FALSE(c_rest_router_use(router, "/api",
                                 c_rest_orm_transaction_start_middleware,
                                 ctx) != C_REST_OK);
  ASSERT_FALSE(c_rest_router_use_post(router, "/api",
                                      c_rest_orm_transaction_end_middleware,
                                      ctx) != C_REST_OK);

  /* Register a CRUD handler */
  user_model.table_name = "users";
  user_model.primary_key = "id";
  user_model.columns = cols;
  user_model.num_columns = 1;
  ASSERT_FALSE(c_rest_router_add(router, "GET", "/api/users",
                                 c_rest_orm_crud_get_list,
                                 &user_model) != C_REST_OK);

  /* Simulate request */
  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  req.method = get_str;
  req.path = users_path;
  res.status_code = 200;

  ret = c_rest_router_dispatch(router, &req, &res);
  ASSERT_FALSE(ret != C_REST_OK);
  ASSERT_FALSE(res.status_code != 200);
  ASSERT_FALSE(res.body == NULL);
  {
    const char *body_ptr = res.body;
    int cmp_idx;
    for (cmp_idx = 0; cmp_idx < 2; cmp_idx++) {
      const char *s = (cmp_idx == 0) ? body_ptr : NULL;
      ASSERT_FALSE(strcmp(s ? s : "[]", "[]") != 0);
    }
  }
  ASSERT_FALSE(req.db_conn != NULL);

  /* Test c_orm_crud error paths (NULL model or req->db_conn == NULL) */
  {
    struct c_rest_request req_err;
    struct c_rest_response res_err;
    memset(&req_err, 0, sizeof(req_err));
    memset(&res_err, 0, sizeof(res_err));

    ret = c_rest_orm_crud_get_list(&req_err, &res_err, NULL);
    ASSERT_FALSE(ret != C_REST_ERROR_GENERIC);
    ASSERT_FALSE(res_err.status_code != 500);
    ASSERT_FALSE(c_rest_response_cleanup(&res_err) != C_REST_OK);
    memset(&res_err, 0, sizeof(res_err));

    ret = c_rest_orm_crud_get_one(&req_err, &res_err, NULL);
    ASSERT_FALSE(ret != C_REST_ERROR_GENERIC);
    ASSERT_FALSE(res_err.status_code != 500);
    ASSERT_FALSE(c_rest_response_cleanup(&res_err) != C_REST_OK);
    memset(&res_err, 0, sizeof(res_err));

    ret = c_rest_orm_crud_create(&req_err, &res_err, NULL);
    ASSERT_FALSE(ret != C_REST_ERROR_GENERIC);
    ASSERT_FALSE(res_err.status_code != 500);
    ASSERT_FALSE(c_rest_response_cleanup(&res_err) != C_REST_OK);
    memset(&res_err, 0, sizeof(res_err));

    ret = c_rest_orm_crud_update(&req_err, &res_err, NULL);
    ASSERT_FALSE(ret != C_REST_ERROR_GENERIC);
    ASSERT_FALSE(res_err.status_code != 500);
    ASSERT_FALSE(c_rest_response_cleanup(&res_err) != C_REST_OK);
    memset(&res_err, 0, sizeof(res_err));

    ret = c_rest_orm_crud_delete(&req_err, &res_err, NULL);
    ASSERT_FALSE(ret != C_REST_ERROR_GENERIC);
    ASSERT_FALSE(res_err.status_code != 500);
    ASSERT_FALSE(c_rest_response_cleanup(&res_err) != C_REST_OK);

    /* Test set_status failure when res is NULL in error branch */
    ret = c_rest_orm_crud_get_list(&req_err, NULL, NULL);
    ASSERT_FALSE(ret != C_REST_ERROR_GENERIC);
    ret = c_rest_orm_crud_get_one(&req_err, NULL, NULL);
    ASSERT_FALSE(ret != C_REST_ERROR_GENERIC);
    ret = c_rest_orm_crud_create(&req_err, NULL, NULL);
    ASSERT_FALSE(ret != C_REST_ERROR_GENERIC);
    ret = c_rest_orm_crud_update(&req_err, NULL, NULL);
    ASSERT_FALSE(ret != C_REST_ERROR_GENERIC);
    ret = c_rest_orm_crud_delete(&req_err, NULL, NULL);
    ASSERT_FALSE(ret != C_REST_ERROR_GENERIC);
    ret = c_rest_orm_health_check(&req_err, NULL, NULL);
    ASSERT_FALSE(ret != C_REST_ERROR_GENERIC);
  }

  /* Test c_orm_crud happy paths */
  {
    struct c_rest_request req_ok;
    struct c_rest_response res_ok;
    memset(&req_ok, 0, sizeof(req_ok));
    memset(&res_ok, 0, sizeof(res_ok));
    req_ok.db_conn = (void *)1;

    /* Test set_status failure when res is NULL in happy branch */
    ASSERT_FALSE(c_rest_orm_crud_get_list(&req_ok, NULL, &user_model) !=
                 C_REST_ERROR_GENERIC);
    ASSERT_FALSE(c_rest_orm_crud_get_one(&req_ok, NULL, &user_model) !=
                 C_REST_ERROR_GENERIC);
    ASSERT_FALSE(c_rest_orm_crud_create(&req_ok, NULL, &user_model) !=
                 C_REST_ERROR_GENERIC);
    ASSERT_FALSE(c_rest_orm_crud_update(&req_ok, NULL, &user_model) !=
                 C_REST_ERROR_GENERIC);
    ASSERT_FALSE(c_rest_orm_crud_delete(&req_ok, NULL, &user_model) !=
                 C_REST_ERROR_GENERIC);
    ASSERT_FALSE(c_rest_orm_health_check(&req_ok, NULL, NULL) !=
                 C_REST_ERROR_GENERIC);

    ret = c_rest_orm_crud_get_list(&req_ok, &res_ok, &user_model);
    ASSERT_FALSE(ret != C_REST_OK);
    ASSERT_FALSE(res_ok.status_code != 200);
    ASSERT_FALSE(c_rest_response_cleanup(&res_ok) != C_REST_OK);
    memset(&res_ok, 0, sizeof(res_ok));

    ret = c_rest_orm_crud_get_one(&req_ok, &res_ok, &user_model);
    ASSERT_FALSE(ret != C_REST_OK);
    ASSERT_FALSE(res_ok.status_code != 200);
    ASSERT_FALSE(c_rest_response_cleanup(&res_ok) != C_REST_OK);
    memset(&res_ok, 0, sizeof(res_ok));

    ret = c_rest_orm_crud_create(&req_ok, &res_ok, &user_model);
    ASSERT_FALSE(ret != C_REST_OK);
    ASSERT_FALSE(res_ok.status_code != 201);
    ASSERT_FALSE(c_rest_response_cleanup(&res_ok) != C_REST_OK);
    memset(&res_ok, 0, sizeof(res_ok));

    ret = c_rest_orm_crud_update(&req_ok, &res_ok, &user_model);
    ASSERT_FALSE(ret != C_REST_OK);
    ASSERT_FALSE(res_ok.status_code != 200);
    ASSERT_FALSE(c_rest_response_cleanup(&res_ok) != C_REST_OK);
    memset(&res_ok, 0, sizeof(res_ok));

    ret = c_rest_orm_crud_delete(&req_ok, &res_ok, &user_model);
    ASSERT_FALSE(ret != C_REST_OK);
    ASSERT_FALSE(res_ok.status_code != 204);
    ASSERT_FALSE(c_rest_response_cleanup(&res_ok) != C_REST_OK);
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
    ASSERT_FALSE(ret != C_REST_ERROR_GENERIC);
    ASSERT_FALSE(c_rest_response_cleanup(&res_fail) != C_REST_OK);
    memset(&res_fail, 0, sizeof(res_fail));

    g_crf_malloc_hook = fail_malloc;
    ret = c_rest_orm_crud_get_one(&req_fail, &res_fail, &user_model);
    g_crf_malloc_hook = NULL;
    ASSERT_FALSE(ret != C_REST_ERROR_GENERIC);
    ASSERT_FALSE(c_rest_response_cleanup(&res_fail) != C_REST_OK);
    memset(&res_fail, 0, sizeof(res_fail));

    g_crf_malloc_hook = fail_malloc;
    ret = c_rest_orm_crud_create(&req_fail, &res_fail, &user_model);
    g_crf_malloc_hook = NULL;
    ASSERT_FALSE(ret != C_REST_ERROR_GENERIC);
    ASSERT_FALSE(c_rest_response_cleanup(&res_fail) != C_REST_OK);
    memset(&res_fail, 0, sizeof(res_fail));

    g_crf_malloc_hook = fail_malloc;
    ret = c_rest_orm_crud_update(&req_fail, &res_fail, &user_model);
    g_crf_malloc_hook = NULL;
    ASSERT_FALSE(ret != C_REST_ERROR_GENERIC);
    ASSERT_FALSE(c_rest_response_cleanup(&res_fail) != C_REST_OK);
    memset(&res_fail, 0, sizeof(res_fail));

    /* Now test malloc failure on the 500 error branch (req.db_conn = NULL) */
    req_fail.db_conn = NULL;

    g_crf_malloc_hook = fail_malloc;
    ret = c_rest_orm_crud_get_list(&req_fail, &res_fail, &user_model);
    g_crf_malloc_hook = NULL;
    ASSERT_FALSE(ret != C_REST_ERROR_GENERIC);
    ASSERT_FALSE(c_rest_response_cleanup(&res_fail) != C_REST_OK);
    memset(&res_fail, 0, sizeof(res_fail));

    ret = c_rest_orm_crud_get_one(&req_fail, &res_fail, &user_model);
    ASSERT_FALSE(ret != C_REST_ERROR_GENERIC);
    ASSERT_FALSE(c_rest_response_cleanup(&res_fail) != C_REST_OK);
    memset(&res_fail, 0, sizeof(res_fail));

    ret = c_rest_orm_crud_create(&req_fail, &res_fail, &user_model);
    ASSERT_FALSE(ret != C_REST_ERROR_GENERIC);
    ASSERT_FALSE(c_rest_response_cleanup(&res_fail) != C_REST_OK);
    memset(&res_fail, 0, sizeof(res_fail));

    ret = c_rest_orm_crud_update(&req_fail, &res_fail, &user_model);
    ASSERT_FALSE(ret != C_REST_ERROR_GENERIC);
    ASSERT_FALSE(c_rest_response_cleanup(&res_fail) != C_REST_OK);
    memset(&res_fail, 0, sizeof(res_fail));

    ret = c_rest_orm_crud_delete(&req_fail, &res_fail, &user_model);
    ASSERT_FALSE(ret != C_REST_ERROR_GENERIC);
    ASSERT_FALSE(c_rest_response_cleanup(&res_fail) != C_REST_OK);
    memset(&res_fail, 0, sizeof(res_fail));

    /* also test NULL res to fail set_status */
    ret = c_rest_orm_crud_get_list(&req_fail, NULL, &user_model);
    ASSERT_FALSE(ret != C_REST_ERROR_GENERIC);

    req_fail.db_conn = (void *)1;
    ret = c_rest_orm_crud_get_list(&req_fail, NULL, &user_model);
    ASSERT_FALSE(ret != C_REST_ERROR_GENERIC);

    ret = c_rest_orm_crud_get_one(&req_fail, NULL, &user_model);
    ASSERT_FALSE(ret != C_REST_ERROR_GENERIC);

    ret = c_rest_orm_crud_create(&req_fail, NULL, &user_model);
    ASSERT_FALSE(ret != C_REST_ERROR_GENERIC);

    ret = c_rest_orm_crud_update(&req_fail, NULL, &user_model);
    ASSERT_FALSE(ret != C_REST_ERROR_GENERIC);

    ret = c_rest_orm_crud_delete(&req_fail, NULL, &user_model);
    ASSERT_FALSE(ret != C_REST_ERROR_GENERIC);

    ret = c_rest_orm_health_check(&req_fail, NULL, NULL);
    ASSERT_FALSE(ret != C_REST_ERROR_GENERIC);
    req_fail.db_conn = NULL;
    ret = c_rest_orm_health_check(&req_fail, NULL, NULL);
    ASSERT_FALSE(ret != C_REST_ERROR_GENERIC);
  }

  /* Test health check */
  {
    struct c_rest_request req_hc;
    struct c_rest_response res_hc;
    memset(&req_hc, 0, sizeof(req_hc));
    memset(&res_hc, 0, sizeof(res_hc));

    /* Unhealthy (no db_conn) */
    ret = c_rest_orm_health_check(&req_hc, &res_hc, NULL);
    ASSERT_FALSE(ret != C_REST_OK);
    ASSERT_FALSE(res_hc.status_code != 503);
    ASSERT_FALSE(c_rest_response_cleanup(&res_hc) != C_REST_OK);
    memset(&res_hc, 0, sizeof(res_hc));

    /* Unhealthy (req is NULL) */
    ret = c_rest_orm_health_check(NULL, &res_hc, NULL);
    ASSERT_FALSE(ret != C_REST_OK);
    ASSERT_FALSE(res_hc.status_code != 503);
    ASSERT_FALSE(c_rest_response_cleanup(&res_hc) != C_REST_OK);
    memset(&res_hc, 0, sizeof(res_hc));

    /* Unhealthy with malloc fail */
    g_crf_malloc_hook = fail_malloc;
    ret = c_rest_orm_health_check(&req_hc, &res_hc, NULL);
    g_crf_malloc_hook = NULL;
    ASSERT_FALSE(ret != C_REST_ERROR_GENERIC);
    ASSERT_FALSE(c_rest_response_cleanup(&res_hc) != C_REST_OK);
    memset(&res_hc, 0, sizeof(res_hc));

    /* Healthy */
    req_hc.db_conn = (void *)1;
    ret = c_rest_orm_health_check(&req_hc, &res_hc, NULL);
    ASSERT_FALSE(ret != C_REST_OK);
    ASSERT_FALSE(res_hc.status_code != 200);
    ASSERT_FALSE(c_rest_response_cleanup(&res_hc) != C_REST_OK);
    memset(&res_hc, 0, sizeof(res_hc));

    /* Healthy with malloc fail */
    g_crf_malloc_hook = fail_malloc;
    ret = c_rest_orm_health_check(&req_hc, &res_hc, NULL);
    g_crf_malloc_hook = NULL;
    ASSERT_FALSE(ret != C_REST_ERROR_GENERIC);
    ASSERT_FALSE(c_rest_response_cleanup(&res_hc) != C_REST_OK);
    memset(&res_hc, 0, sizeof(res_hc));
  }

  /* Test run migrations */
  {
    struct c_rest_context dummy_ctx;
    cfs_path mig_dir = {0};
    cfs_error_code ec = {0};
    cfs_size_t removed = 0;
    FILE *fp = NULL;

    memset(&dummy_ctx, 0, sizeof(dummy_ctx));
    ret = c_rest_orm_run_migrations(&dummy_ctx, "test_orm_mig_int");
    ASSERT_EQ(C_REST_ERROR_INVALID_ARG, ret);

    ret = c_rest_orm_run_migrations(NULL, NULL);
    ASSERT_EQ(C_REST_ERROR_INVALID_ARG, ret);

    ret = c_rest_orm_run_migrations(ctx, NULL);
    ASSERT_EQ(C_REST_ERROR_INVALID_ARG, ret);

#if defined(CFS_OS_WINDOWS) && defined(CFS_UNICODE)
    ASSERT_EQ(cfs_errc_success,
              cfs_path_init_str(&mig_dir, L"test_orm_mig_int"));
#else
    ASSERT_EQ(
        cfs_errc_success,
        cfs_path_init_str(&mig_dir, (const cfs_char_t *)"test_orm_mig_int"));
#endif
    {
      cfs_bool ex = 0;
      ASSERT_EQ(cfs_errc_success, cfs_exists_path(&mig_dir, &ex, &ec));
      if (ex) {
        ASSERT_EQ(cfs_errc_success, cfs_remove_all(&mig_dir, &removed, &ec));
      }
    }
    ASSERT_EQ(cfs_errc_success, cfs_create_directories(&mig_dir, &ec));

#if defined(_MSC_VER)
    fopen_s(&fp, "test_orm_mig_int/001_users.sql", "wb");
#else
    fp = fopen("test_orm_mig_int/001_users.sql", "wb");
#endif
    ASSERT_NEQ(NULL, fp);
    fprintf(fp, "CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY, "
                "email TEXT);\n");
    fclose(fp);

#if defined(_MSC_VER)
    fopen_s(&fp, "test_orm_mig_int/002_seed.sql", "wb");
#else
    fp = fopen("test_orm_mig_int/002_seed.sql", "wb");
#endif
    ASSERT_NEQ(NULL, fp);
    fprintf(fp, "INSERT INTO users (email) VALUES ('a@b.com');\n");
    fclose(fp);

    /* ctx has real SQLite db_pool initialized by c_rest_run above */
    ret = c_rest_orm_run_migrations(ctx, "test_orm_mig_int");
    ASSERT_EQ(C_REST_OK, ret);

    /* test with logger */
    ctx->logger.log_cb = dummy_log_cb;
    ret = c_rest_orm_run_migrations(ctx, "test_orm_mig_int");
    ASSERT_EQ(C_REST_OK, ret);

    /* test with logger failure */
    ctx->logger.log_cb = fail_log_cb;
    ret = c_rest_orm_run_migrations(ctx, "test_orm_mig_int");
    ASSERT_EQ(C_REST_ERROR_GENERIC, ret);

    ctx->logger.log_cb = NULL;
    ASSERT_EQ(cfs_errc_success, cfs_remove_all(&mig_dir, &removed, &ec));
    cfs_path_destroy(&mig_dir);
  }

  /* Test middleware branches directly */
  ret = c_rest_orm_transaction_start_middleware(NULL, NULL, NULL);
  ASSERT_FALSE(ret != C_REST_ERROR_GENERIC);

  ret = c_rest_orm_transaction_start_middleware(&req, NULL, NULL);
  ASSERT_FALSE(ret != C_REST_ERROR_GENERIC);

  /* ctx->db_pool == NULL branch */
  {
    void *orig_pool = ctx->db_pool;
    ctx->db_pool = NULL;
    req.db_conn = NULL;
    ret = c_rest_orm_transaction_start_middleware(&req, NULL, ctx);
    ASSERT_FALSE(ret != C_REST_OK);
    ASSERT_FALSE(req.db_conn != NULL);
    ctx->db_pool = orig_pool;
  }

  /* End middleware null req */
  ret = c_rest_orm_transaction_end_middleware(NULL, NULL, NULL);
  ASSERT_FALSE(ret != C_REST_OK);

  /* End middleware req->db_conn null */
  req.db_conn = NULL;
  ret = c_rest_orm_transaction_end_middleware(&req, NULL, NULL);
  ASSERT_FALSE(ret != C_REST_OK);

  /* End middleware rollback branch (status >= 400) */
  req.db_conn = (struct c_orm_connection *)1;
  res.status_code = 400;
  ret = c_rest_orm_transaction_end_middleware(&req, &res, NULL);
  ASSERT_FALSE(ret != C_REST_OK);
  ASSERT_FALSE(req.db_conn != NULL);

  /* End middleware commit branch (res NULL) */
  req.db_conn = (struct c_orm_connection *)1;
  ret = c_rest_orm_transaction_end_middleware(&req, NULL, NULL);
  ASSERT_FALSE(ret != C_REST_OK);
  ASSERT_FALSE(req.db_conn != NULL);

  req.method = NULL;
  req.path = NULL;
  ASSERT_FALSE(c_rest_request_cleanup(&req) != C_REST_OK);
  ASSERT_FALSE(c_rest_response_cleanup(&res) != C_REST_OK);

  ASSERT_FALSE(c_rest_router_destroy(router) != C_REST_OK);
  ASSERT_FALSE(c_rest_destroy(ctx) != C_REST_OK);

  PASS();
}

SUITE(orm_integration_suite) { RUN_TEST(test_orm_integration_cases); }
