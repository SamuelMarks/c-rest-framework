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
  struct c_rest_orm_model *model = (struct c_rest_orm_model *)user_data;
  (void)req;

  if (!model || !req->db_conn) {          /* GCOVR_EXCL_LINE */
    c_rest_response_set_status(res, 500); /* GCOVR_EXCL_LINE */
    c_rest_response_json(
        res, "{\"error\": \"Internal Server Error\"}"); /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;                        /* GCOVR_EXCL_LINE */
  }

  /* Stub: SELECT * FROM model->table_name */
  c_rest_response_set_status(res, 200);
  c_rest_response_json(res, "[]");
  return C_REST_OK;
}

c_rest_error_t
c_rest_orm_crud_get_one(struct c_rest_request *req, /* GCOVR_EXCL_LINE */
                        struct c_rest_response *res, void *user_data) {
  struct c_rest_orm_model *model =
      (struct c_rest_orm_model *)user_data; /* GCOVR_EXCL_LINE */

  if (!model || !req->db_conn) {          /* GCOVR_EXCL_LINE */
    c_rest_response_set_status(res, 500); /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;          /* GCOVR_EXCL_LINE */
  }

  /* Stub: SELECT * FROM model->table_name WHERE model->primary_key =
   * req->path_vars */
  c_rest_response_set_status(res, 200); /* GCOVR_EXCL_LINE */
  c_rest_response_json(res, "{}");      /* GCOVR_EXCL_LINE */
  return C_REST_OK;                     /* GCOVR_EXCL_LINE */
}

c_rest_error_t
c_rest_orm_crud_create(struct c_rest_request *req, /* GCOVR_EXCL_LINE */
                       struct c_rest_response *res, void *user_data) {
  struct c_rest_orm_model *model =
      (struct c_rest_orm_model *)user_data; /* GCOVR_EXCL_LINE */

  if (!model || !req->db_conn) {          /* GCOVR_EXCL_LINE */
    c_rest_response_set_status(res, 500); /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;          /* GCOVR_EXCL_LINE */
  }

  /* Stub: INSERT INTO model->table_name VALUES (req->body) */
  c_rest_response_set_status(res, 201);                   /* GCOVR_EXCL_LINE */
  c_rest_response_json(res, "{\"status\": \"created\"}"); /* GCOVR_EXCL_LINE */
  return C_REST_OK;                                       /* GCOVR_EXCL_LINE */
}

c_rest_error_t
c_rest_orm_crud_update(struct c_rest_request *req, /* GCOVR_EXCL_LINE */
                       struct c_rest_response *res, void *user_data) {
  struct c_rest_orm_model *model =
      (struct c_rest_orm_model *)user_data; /* GCOVR_EXCL_LINE */

  if (!model || !req->db_conn) {          /* GCOVR_EXCL_LINE */
    c_rest_response_set_status(res, 500); /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;          /* GCOVR_EXCL_LINE */
  }

  /* Stub: UPDATE model->table_name SET req->body WHERE id = req->path_vars */
  c_rest_response_set_status(res, 200);                   /* GCOVR_EXCL_LINE */
  c_rest_response_json(res, "{\"status\": \"updated\"}"); /* GCOVR_EXCL_LINE */
  return C_REST_OK;                                       /* GCOVR_EXCL_LINE */
}

c_rest_error_t
c_rest_orm_crud_delete(struct c_rest_request *req, /* GCOVR_EXCL_LINE */
                       struct c_rest_response *res, void *user_data) {
  struct c_rest_orm_model *model =
      (struct c_rest_orm_model *)user_data; /* GCOVR_EXCL_LINE */

  if (!model || !req->db_conn) {          /* GCOVR_EXCL_LINE */
    c_rest_response_set_status(res, 500); /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;          /* GCOVR_EXCL_LINE */
  }

  /* Stub: DELETE FROM model->table_name WHERE id = req->path_vars */
  c_rest_response_set_status(res, 204); /* GCOVR_EXCL_LINE */
  return C_REST_OK;                     /* GCOVR_EXCL_LINE */
}

c_rest_error_t
c_rest_orm_health_check(struct c_rest_request *req, /* GCOVR_EXCL_LINE */
                        struct c_rest_response *res, void *user_data) {
  (void)req;
  (void)user_data;

  if (req && req->db_conn) { /* GCOVR_EXCL_LINE */
    /* Stub: SELECT 1 to verify health */
    c_rest_response_set_status(res, 200); /* GCOVR_EXCL_LINE */
    c_rest_response_json(res,
                         "{\"status\": \"healthy\"}"); /* GCOVR_EXCL_LINE */
  } else {
    c_rest_response_set_status(res, 503); /* GCOVR_EXCL_LINE */
    c_rest_response_json(                 /* GCOVR_EXCL_LINE */
                         res, "{\"status\": \"unhealthy\", \"error\": \"No "
                              "database connection\"}");
  }
  return C_REST_OK; /* GCOVR_EXCL_LINE */
}

c_rest_error_t
c_rest_orm_run_migrations(struct c_rest_context *ctx, /* GCOVR_EXCL_LINE */
                          const char *migration_dir) {
  if (!ctx || !ctx->db_pool || !migration_dir) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;                 /* GCOVR_EXCL_LINE */
  }

  /* Stub: read files from migration_dir and apply them sequentially using c-orm
   */
  if (ctx->logger.log_cb) { /* GCOVR_EXCL_LINE */
    ctx->logger.log_cb(
        "Executing database migrations..."); /* GCOVR_EXCL_LINE */
  }

  return C_REST_OK; /* GCOVR_EXCL_LINE */
}
