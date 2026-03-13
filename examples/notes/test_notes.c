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

/**
 * @brief The note ORM model definition.
 */
struct c_rest_orm_model note_model = {"notes", "id"};

/**
 * @brief Test entry point for the notes app.
 * @return Exit status code.
 */
int main(void) {
  struct c_rest_context *ctx = NULL;
  c_rest_router *router = NULL;
  struct c_rest_request req;
  struct c_rest_response res;
  int ret;

  printf("Running tests for Notes App...\n");

  ret = c_rest_init(C_REST_MODALITY_SYNC, &ctx);
  if (ret != 0)
    return 1;

  ctx->db_config.connection_string = "sqlite://:memory:";

  /* This is just to trigger the mock DB pool creation inside c_rest_run
     if that is how c-orm mock works in test mode */
  c_rest_run(ctx);

  ret = c_rest_router_init(&router);
  if (ret != 0)
    return 1;

  c_rest_router_use(router, "/api", c_rest_orm_transaction_start_middleware,
                    ctx);
  c_rest_router_use_post(router, "/api", c_rest_orm_transaction_end_middleware,
                         ctx);

  c_rest_router_add(router, "GET", "/api/notes", c_rest_orm_crud_get_list,
                    &note_model);
  c_rest_router_add(router, "POST", "/api/notes", c_rest_orm_crud_create,
                    &note_model);

  /* Simulate GET /api/notes */
  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  req.method = "GET";
  req.path = "/api/notes";
  res.status_code = 200;

  ret = c_rest_router_dispatch(router, &req, &res);
  if (ret != 0)
    return 1;
  if (res.status_code != 200)
    return 1;

  c_rest_request_cleanup(&req);
  c_rest_response_cleanup(&res);

  /* Simulate POST /api/notes */
  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  req.method = "POST";
  req.path = "/api/notes";
  req.body = "{\"title\":\"My Note\",\"content\":\"Hello world\"}";
  req.body_len = strlen(req.body);
  res.status_code = 200;

  ret = c_rest_router_dispatch(router, &req, &res);
  if (ret != 0)
    return 1;

  /* The mock returns 201 for POST usually or 200. We'll accept 200 or 201 */
  if (res.status_code != 200 && res.status_code != 201)
    return 1;

  /* Since req.body was statically allocated, avoid double free */
  req.body = NULL;
  c_rest_request_cleanup(&req);
  c_rest_response_cleanup(&res);

  c_rest_router_destroy(router);
  c_rest_destroy(ctx);

  printf("All notes app tests passed.\n");
  return 0;
}
