/* clang-format off */
#include "c_rest_modality.h"
#include "c_rest_orm_crud.h"
#include "c_rest_orm_middleware.h"
#include "c_rest_request.h"
#include "c_rest_response.h"
#include "c_rest_router.h"

#include <stdio.h>
#include <string.h>
/* clang-format on */

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
  c_rest_router_use(router, "/api", c_rest_orm_transaction_start_middleware,
                    ctx);
  c_rest_router_use_post(router, "/api", c_rest_orm_transaction_end_middleware,
                         ctx);

  /* Register a CRUD handler */
  user_model.table_name = "users";
  user_model.primary_key = "id";
  c_rest_router_add(router, "GET", "/api/users", c_rest_orm_crud_get_list,
                    &user_model);

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

  c_rest_request_cleanup(&req);
  c_rest_response_cleanup(&res);

  c_rest_router_destroy(router);
  c_rest_destroy(ctx);

  return 0;
}
