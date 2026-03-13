/* clang-format off */
#include "c_rest_modality.h"
#include "c_rest_orm_crud.h"
#include "c_rest_request.h"
#include "c_rest_response.h"

#include <stdio.h>
/* clang-format on */

void c_rest_orm_crud_get_list(struct c_rest_request *req,
                              struct c_rest_response *res, void *user_data) {
  struct c_rest_orm_model *model = (struct c_rest_orm_model *)user_data;
  (void)req;

  if (!model || !req->db_conn) {
    c_rest_response_set_status(res, 500);
    c_rest_response_json(res, "{\"error\": \"Internal Server Error\"}");
    return;
  }

  /* Stub: SELECT * FROM model->table_name */
  c_rest_response_set_status(res, 200);
  c_rest_response_json(res, "[]");
}

void c_rest_orm_crud_get_one(struct c_rest_request *req,
                             struct c_rest_response *res, void *user_data) {
  struct c_rest_orm_model *model = (struct c_rest_orm_model *)user_data;

  if (!model || !req->db_conn) {
    c_rest_response_set_status(res, 500);
    return;
  }

  /* Stub: SELECT * FROM model->table_name WHERE model->primary_key =
   * req->path_vars */
  c_rest_response_set_status(res, 200);
  c_rest_response_json(res, "{}");
}

void c_rest_orm_crud_create(struct c_rest_request *req,
                            struct c_rest_response *res, void *user_data) {
  struct c_rest_orm_model *model = (struct c_rest_orm_model *)user_data;

  if (!model || !req->db_conn) {
    c_rest_response_set_status(res, 500);
    return;
  }

  /* Stub: INSERT INTO model->table_name VALUES (req->body) */
  c_rest_response_set_status(res, 201);
  c_rest_response_json(res, "{\"status\": \"created\"}");
}

void c_rest_orm_crud_update(struct c_rest_request *req,
                            struct c_rest_response *res, void *user_data) {
  struct c_rest_orm_model *model = (struct c_rest_orm_model *)user_data;

  if (!model || !req->db_conn) {
    c_rest_response_set_status(res, 500);
    return;
  }

  /* Stub: UPDATE model->table_name SET req->body WHERE id = req->path_vars */
  c_rest_response_set_status(res, 200);
  c_rest_response_json(res, "{\"status\": \"updated\"}");
}

void c_rest_orm_crud_delete(struct c_rest_request *req,
                            struct c_rest_response *res, void *user_data) {
  struct c_rest_orm_model *model = (struct c_rest_orm_model *)user_data;

  if (!model || !req->db_conn) {
    c_rest_response_set_status(res, 500);
    return;
  }

  /* Stub: DELETE FROM model->table_name WHERE id = req->path_vars */
  c_rest_response_set_status(res, 204);
}

void c_rest_orm_health_check(struct c_rest_request *req,
                             struct c_rest_response *res, void *user_data) {
  (void)req;
  (void)user_data;

  if (req && req->db_conn) {
    /* Stub: SELECT 1 to verify health */
    c_rest_response_set_status(res, 200);
    c_rest_response_json(res, "{\"status\": \"healthy\"}");
  } else {
    c_rest_response_set_status(res, 503);
    c_rest_response_json(
        res,
        "{\"status\": \"unhealthy\", \"error\": \"No database connection\"}");
  }
}

int c_rest_orm_run_migrations(struct c_rest_context *ctx,
                              const char *migration_dir) {
  if (!ctx || !ctx->db_pool || !migration_dir) {
    return 1;
  }

  /* Stub: read files from migration_dir and apply them sequentially using c-orm
   */
  if (ctx->logger.log_cb) {
    ctx->logger.log_cb("Executing database migrations...");
  }

  return 0;
}
