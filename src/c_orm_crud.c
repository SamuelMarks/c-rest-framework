/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_modality.h"
#include "c_rest_orm_crud.h"
#include "c_rest_request.h"
#include "c_rest_response.h"

#include <stdio.h>
/* clang-format on */

c_rest_error_t c_rest_orm_crud_get_list(struct c_rest_request *req,
                                        struct c_rest_response *res,
                                        void *user_data) {
  c_rest_error_t rc;
  struct c_rest_orm_model *model = (struct c_rest_orm_model *)user_data;
  (void)req;

  if (!model || !req->db_conn) {
    rc = c_rest_response_set_status(res, 500);
    if (rc != C_REST_OK)
      return rc;
    c_rest_response_json(res, "{\"error\": \"Internal Server Error\"}");
    return C_REST_ERROR_GENERIC;
  }

  /* Stub: SELECT * FROM model->table_name */
  rc = c_rest_response_set_status(res, 200);
  if (rc != C_REST_OK)
    return rc;
  rc = c_rest_response_json(res, "[]");
  if (rc != C_REST_OK)
    return rc;
  return C_REST_OK;
}

c_rest_error_t c_rest_orm_crud_get_one(struct c_rest_request *req,
                                       struct c_rest_response *res,
                                       void *user_data) {
  c_rest_error_t rc;
  struct c_rest_orm_model *model = (struct c_rest_orm_model *)user_data;

  if (!model || !req->db_conn) {
    c_rest_response_set_status(res, 500);
    return C_REST_ERROR_GENERIC;
  }

  /* Stub: SELECT * FROM model->table_name WHERE model->primary_key =
   * req->path_vars */
  rc = c_rest_response_set_status(res, 200);
  if (rc != C_REST_OK)
    return rc;
  rc = c_rest_response_json(res, "{}");
  if (rc != C_REST_OK)
    return rc;
  return C_REST_OK;
}

c_rest_error_t c_rest_orm_crud_create(struct c_rest_request *req,
                                      struct c_rest_response *res,
                                      void *user_data) {
  c_rest_error_t rc;
  struct c_rest_orm_model *model = (struct c_rest_orm_model *)user_data;

  if (!model || !req->db_conn) {
    c_rest_response_set_status(res, 500);
    return C_REST_ERROR_GENERIC;
  }

  /* Stub: INSERT INTO model->table_name VALUES (req->body) */
  rc = c_rest_response_set_status(res, 201);
  if (rc != C_REST_OK)
    return rc;
  rc = c_rest_response_json(res, "{\"status\": \"created\"}");
  if (rc != C_REST_OK)
    return rc;
  return C_REST_OK;
}

c_rest_error_t c_rest_orm_crud_update(struct c_rest_request *req,
                                      struct c_rest_response *res,
                                      void *user_data) {
  c_rest_error_t rc;
  struct c_rest_orm_model *model = (struct c_rest_orm_model *)user_data;

  if (!model || !req->db_conn) {
    c_rest_response_set_status(res, 500);
    return C_REST_ERROR_GENERIC;
  }

  /* Stub: UPDATE model->table_name SET req->body WHERE id = req->path_vars */
  rc = c_rest_response_set_status(res, 200);
  if (rc != C_REST_OK)
    return rc;
  rc = c_rest_response_json(res, "{\"status\": \"updated\"}");
  if (rc != C_REST_OK)
    return rc;
  return C_REST_OK;
}

c_rest_error_t c_rest_orm_crud_delete(struct c_rest_request *req,
                                      struct c_rest_response *res,
                                      void *user_data) {
  c_rest_error_t rc;
  struct c_rest_orm_model *model = (struct c_rest_orm_model *)user_data;

  if (!model || !req->db_conn) {
    c_rest_response_set_status(res, 500);
    return C_REST_ERROR_GENERIC;
  }

  /* Stub: DELETE FROM model->table_name WHERE id = req->path_vars */
  rc = c_rest_response_set_status(res, 204);
  if (rc != C_REST_OK)
    return rc;
  return C_REST_OK;
}

c_rest_error_t c_rest_orm_health_check(struct c_rest_request *req,
                                       struct c_rest_response *res,
                                       void *user_data) {
  c_rest_error_t rc;
  (void)req;
  (void)user_data;

  if (req && req->db_conn) {
    /* Stub: SELECT 1 to verify health */
    rc = c_rest_response_set_status(res, 200);
    if (rc != C_REST_OK)
      return rc;
    rc = c_rest_response_json(res, "{\"status\": \"healthy\"}");
    if (rc != C_REST_OK)
      return rc;
  } else {
    rc = c_rest_response_set_status(res, 503);
    if (rc != C_REST_OK)
      return rc;
    rc =
        c_rest_response_json(res, "{\"status\": \"unhealthy\", \"error\": \"No "
                                  "database connection\"}");
    if (rc != C_REST_OK)
      return rc;
  }
  return C_REST_OK;
}

c_rest_error_t c_rest_orm_run_migrations(struct c_rest_context *ctx,
                                         const char *migration_dir) {
  c_rest_error_t rc;
  if (!ctx || !ctx->db_pool || !migration_dir) {
    return C_REST_ERROR_GENERIC;
  }

  /* Stub: read files from migration_dir and apply them sequentially using c-orm
   */
  if (ctx->logger.log_cb) {
    rc = ctx->logger.log_cb("Executing database migrations...");
    if (rc != C_REST_OK)
      return rc;
  }

  return C_REST_OK;
}
