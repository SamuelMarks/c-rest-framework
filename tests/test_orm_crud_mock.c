/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_modality.h"
#include "greatest.h"
#include "greatest_clean.h"
#include <string.h>

#undef C_REST_EXPORT
#define C_REST_EXPORT

#include "c_rest_response.h"

static int g_mock_res_json_fail = 0;
static int g_mock_res_status_fail = 0;

extern c_rest_error_t c_rest_response_json(struct c_rest_response *res, const char *json_str);
extern c_rest_error_t c_rest_response_set_status(struct c_rest_response *res, int status_code);

static c_rest_error_t mock_c_rest_response_json(struct c_rest_response *res, const char *json_str) {
    if (g_mock_res_json_fail) return C_REST_ERROR_GENERIC;
    return c_rest_response_json(res, json_str);
}

static c_rest_error_t mock_c_rest_response_set_status(struct c_rest_response *res, int status_code) {
    if (g_mock_res_status_fail) return C_REST_ERROR_GENERIC;
    return c_rest_response_set_status(res, status_code);
}

#define c_rest_response_json mock_c_rest_response_json
#define c_rest_response_set_status mock_c_rest_response_set_status

#define c_rest_orm_crud_create test_c_rest_orm_crud_create
#define c_rest_orm_crud_get_one test_c_rest_orm_crud_get_one
#define c_rest_orm_crud_update test_c_rest_orm_crud_update
#define c_rest_orm_crud_delete test_c_rest_orm_crud_delete
#define c_rest_orm_crud_get_list test_c_rest_orm_crud_get_list
#define c_rest_orm_health_check test_c_rest_orm_health_check
#define c_rest_orm_run_migrations test_c_rest_orm_run_migrations

c_rest_error_t test_c_rest_orm_crud_create(struct c_rest_request *req,
                                           struct c_rest_response *res,
                                           void *user_data);
c_rest_error_t test_c_rest_orm_crud_get_one(struct c_rest_request *req,
                                            struct c_rest_response *res,
                                            void *user_data);
c_rest_error_t test_c_rest_orm_crud_update(struct c_rest_request *req,
                                           struct c_rest_response *res,
                                           void *user_data);
c_rest_error_t test_c_rest_orm_crud_delete(struct c_rest_request *req,
                                           struct c_rest_response *res,
                                           void *user_data);
c_rest_error_t test_c_rest_orm_crud_get_list(struct c_rest_request *req,
                                             struct c_rest_response *res,
                                             void *user_data);
c_rest_error_t test_c_rest_orm_health_check(struct c_rest_request *req,
                                            struct c_rest_response *res,
                                            void *user_data);
c_rest_error_t test_c_rest_orm_run_migrations(struct c_rest_context *ctx,
                                              const char *migration_dir);

#include "../src/c_orm_crud.c"

#undef c_rest_response_json
#undef c_rest_response_set_status

/* clang-format on */

static void reset_mocks(void *data) {
  (void)data;
  g_mock_res_json_fail = 0;
  g_mock_res_status_fail = 0;
}

static c_rest_error_t mock_logger_fail(const char *msg) {
  (void)msg;
  return C_REST_ERROR_GENERIC;
}

static c_rest_error_t mock_logger_ok(const char *msg) {
  (void)msg;
  return C_REST_OK;
}

TEST test_orm_crud_error_branches(void) {
  struct c_rest_request req;
  struct c_rest_response res;
  struct c_rest_orm_model model;
  struct c_rest_context ctx;

  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));
  memset(&model, 0, sizeof(model));
  memset(&ctx, 0, sizeof(ctx));

  req.body = "{}";

  /* --- c_rest_orm_crud_get_list --- */
  /* model == NULL: status fail */
  g_mock_res_status_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_orm_crud_get_list(&req, &res, NULL));
  g_mock_res_status_fail = 0;
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* model == NULL: json fail */
  g_mock_res_json_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_orm_crud_get_list(&req, &res, NULL));
  g_mock_res_json_fail = 0;
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* model == NULL: default error return */
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_orm_crud_get_list(&req, &res, NULL));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* model != NULL but req.db_conn == NULL */
  req.db_conn = NULL;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_orm_crud_get_list(&req, &res, &model));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* valid model and req.db_conn */
  req.db_conn = (void *)1;
  g_mock_res_status_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_orm_crud_get_list(&req, &res, &model));
  g_mock_res_status_fail = 0;
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  g_mock_res_json_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_orm_crud_get_list(&req, &res, &model));
  g_mock_res_json_fail = 0;
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  ASSERT_EQ(C_REST_OK, test_c_rest_orm_crud_get_list(&req, &res, &model));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));
  req.db_conn = NULL;

  /* --- c_rest_orm_crud_get_one --- */
  /* model == NULL: status fail */
  g_mock_res_status_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_orm_crud_get_one(&req, &res, NULL));
  g_mock_res_status_fail = 0;
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* model == NULL: json fail */
  g_mock_res_json_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_orm_crud_get_one(&req, &res, NULL));
  g_mock_res_json_fail = 0;
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_orm_crud_get_one(&req, &res, NULL));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* model != NULL but req.db_conn == NULL */
  req.db_conn = NULL;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_orm_crud_get_one(&req, &res, &model));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* valid model and req.db_conn */
  req.db_conn = (void *)1;
  g_mock_res_status_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_orm_crud_get_one(&req, &res, &model));
  g_mock_res_status_fail = 0;
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  g_mock_res_json_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_orm_crud_get_one(&req, &res, &model));
  g_mock_res_json_fail = 0;
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  ASSERT_EQ(C_REST_OK, test_c_rest_orm_crud_get_one(&req, &res, &model));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));
  req.db_conn = NULL;

  /* --- c_rest_orm_crud_create --- */
  /* model == NULL: status fail */
  g_mock_res_status_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_orm_crud_create(&req, &res, NULL));
  g_mock_res_status_fail = 0;
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* model == NULL: json fail */
  g_mock_res_json_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_orm_crud_create(&req, &res, NULL));
  g_mock_res_json_fail = 0;
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_orm_crud_create(&req, &res, NULL));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* model != NULL but req.db_conn == NULL */
  req.db_conn = NULL;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_orm_crud_create(&req, &res, &model));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* valid model and req.db_conn */
  req.db_conn = (void *)1;
  g_mock_res_status_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_orm_crud_create(&req, &res, &model));
  g_mock_res_status_fail = 0;
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  g_mock_res_json_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_orm_crud_create(&req, &res, &model));
  g_mock_res_json_fail = 0;
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  ASSERT_EQ(C_REST_OK, test_c_rest_orm_crud_create(&req, &res, &model));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));
  req.db_conn = NULL;

  /* --- c_rest_orm_crud_update --- */
  /* model == NULL: status fail */
  g_mock_res_status_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_orm_crud_update(&req, &res, NULL));
  g_mock_res_status_fail = 0;
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* model == NULL: json fail */
  g_mock_res_json_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_orm_crud_update(&req, &res, NULL));
  g_mock_res_json_fail = 0;
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_orm_crud_update(&req, &res, NULL));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* model != NULL but req.db_conn == NULL */
  req.db_conn = NULL;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_orm_crud_update(&req, &res, &model));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* valid model and req.db_conn */
  req.db_conn = (void *)1;
  g_mock_res_status_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_orm_crud_update(&req, &res, &model));
  g_mock_res_status_fail = 0;
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  g_mock_res_json_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_orm_crud_update(&req, &res, &model));
  g_mock_res_json_fail = 0;
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  ASSERT_EQ(C_REST_OK, test_c_rest_orm_crud_update(&req, &res, &model));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));
  req.db_conn = NULL;

  /* --- c_rest_orm_crud_delete --- */
  /* model == NULL: status fail */
  g_mock_res_status_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_orm_crud_delete(&req, &res, NULL));
  g_mock_res_status_fail = 0;
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* model == NULL: json fail */
  g_mock_res_json_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_orm_crud_delete(&req, &res, NULL));
  g_mock_res_json_fail = 0;
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_orm_crud_delete(&req, &res, NULL));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* model != NULL but req.db_conn == NULL */
  req.db_conn = NULL;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_orm_crud_delete(&req, &res, &model));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* valid model and req.db_conn */
  req.db_conn = (void *)1;
  g_mock_res_status_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_orm_crud_delete(&req, &res, &model));
  g_mock_res_status_fail = 0;
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  ASSERT_EQ(C_REST_OK, test_c_rest_orm_crud_delete(&req, &res, &model));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));
  req.db_conn = NULL;

  /* --- c_rest_orm_health_check --- */
  /* req == NULL */
  ASSERT_EQ(C_REST_OK, test_c_rest_orm_health_check(NULL, &res, NULL));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* --- c_rest_orm_health_check --- */
  /* with db_conn */
  req.db_conn = (void *)1;
  g_mock_res_status_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_orm_health_check(&req, &res, NULL));
  g_mock_res_status_fail = 0;
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  g_mock_res_json_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_orm_health_check(&req, &res, NULL));
  g_mock_res_json_fail = 0;
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  ASSERT_EQ(C_REST_OK, test_c_rest_orm_health_check(&req, &res, NULL));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));
  req.db_conn = NULL;

  /* without db_conn */
  g_mock_res_status_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_orm_health_check(&req, &res, NULL));
  g_mock_res_status_fail = 0;
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  g_mock_res_json_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_orm_health_check(&req, &res, NULL));
  g_mock_res_json_fail = 0;
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  ASSERT_EQ(C_REST_OK, test_c_rest_orm_health_check(&req, &res, NULL));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* --- c_rest_orm_run_migrations --- */
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_orm_run_migrations(NULL, "migrations"));
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_orm_run_migrations(&ctx, "migrations"));
  ctx.db_pool = (void *)1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, test_c_rest_orm_run_migrations(&ctx, NULL));

  /* logger callback failure */
  ctx.logger.log_cb = mock_logger_fail;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_orm_run_migrations(&ctx, "migrations"));

  /* logger callback success */
  ctx.logger.log_cb = mock_logger_ok;
  ASSERT_EQ(C_REST_OK, test_c_rest_orm_run_migrations(&ctx, "migrations"));

  /* no logger callback */
  ctx.logger.log_cb = NULL;
  ASSERT_EQ(C_REST_OK, test_c_rest_orm_run_migrations(&ctx, "migrations"));

  PASS();
}

SUITE_EXTERN(orm_crud_mock_suite);
SUITE(orm_crud_mock_suite) {
  SET_SETUP(reset_mocks, NULL);
  RUN_TEST(test_orm_crud_error_branches);
}
