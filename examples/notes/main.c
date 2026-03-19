/* clang-format off */
#include "c_rest_modality.h"
#include "c_rest_orm_crud.h"
#include "c_rest_orm_middleware.h"
#include "c_rest_response.h"
#include "c_rest_router.h"
#include <stdio.h>
/* clang-format on */

/**
 * @brief The note ORM model definition.
 */
struct c_rest_orm_model note_model = {"notes", "id"};

/**
 * @brief Custom 404 Not Found handler.
 * @param req The HTTP request.
 * @param res The HTTP response.
 * @param user_data Custom user data.
 */
static int custom_404_handler(struct c_rest_request *req,
                              struct c_rest_response *res, void *user_data) {
  (void)req;
  (void)user_data;
  c_rest_response_set_status(res, 404);
  c_rest_response_json(res, "{\"error\": \"Not Found\"}");
  return 0;
}

/**
 * @brief Application entry point.
 * @return Exit status code.
 */
int main(void) {
  struct c_rest_context *ctx = NULL;
  c_rest_router *router = NULL;
  int res;

  printf("Initializing Notes App...\n");

  res = c_rest_init(C_REST_MODALITY_SYNC, &ctx);
  if (res != 0) {
    printf("Failed to init context\n");
    return 1;
  }

  /* Configure ORM Connection to a local SQLite database */
  ctx->db_config.connection_string = "sqlite://notes.db";

  /* Initialize Router */
  res = c_rest_router_init(&router);
  if (res != 0) {
    printf("Failed to init router\n");
    c_rest_destroy(ctx);
    return 1;
  }

  /*
   * Apply ORM Middlewares
   * These wrap requests in a database transaction,
   * committing on success, rolling back on error.
   */
  c_rest_router_use(router, "/api/v0", c_rest_orm_transaction_start_middleware,
                    ctx);
  c_rest_router_use_post(router, "/api/v0",
                         c_rest_orm_transaction_end_middleware, ctx);

  /* Auto-generate CRUD endpoints using the c_rest_orm_crud handlers */
  c_rest_set_router(ctx, router);
  c_rest_router_add(router, "GET", "/api/v0/notes", c_rest_orm_crud_get_list,
                    &note_model);
  c_rest_router_add(router, "POST", "/api/v0/notes", c_rest_orm_crud_create,
                    &note_model);
  c_rest_router_add(router, "GET", "/api/v0/notes/:id", c_rest_orm_crud_get_one,
                    &note_model);
  c_rest_router_add(router, "PUT", "/api/v0/notes/:id", c_rest_orm_crud_update,
                    &note_model);
  c_rest_router_add(router, "DELETE", "/api/v0/notes/:id",
                    c_rest_orm_crud_delete, &note_model);

  /* Fallback handler */
  c_rest_router_add(router, "GET", "/*", custom_404_handler, NULL);

  /* In a real application, you would attach the router to the context
   * and start the framework:
   *
   * ctx->internal_state = router; Depends on modality routing
   * c_rest_run(ctx);
   */

  printf("App configured successfully. (Mock Run)\n");

  c_rest_router_destroy(router);
  c_rest_destroy(ctx);
  return 0;
}
