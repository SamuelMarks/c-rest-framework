#ifndef C_REST_ORM_CRUD_H
#define C_REST_ORM_CRUD_H

/* clang-format off */
#include "c_rest_router.h"
#include "c_rest_orm.h"
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Represents a basic model mapping for auto-generating handlers.
 */
/** @brief ORM model */
struct c_rest_orm_model {
  /** @brief Database table name */
  const char *table_name;
  /** @brief Primary key field */
  const char *primary_key;
};

/**
 * @brief Generates a GET handler for retrieving a list of resources.
 */
int c_rest_orm_crud_get_list(struct c_rest_request *req,
                             struct c_rest_response *res, void *user_data);

/**
 * @brief Generates a GET handler for retrieving a specific resource by ID.
 * Expects a path variable (e.g. :id) matching the model's primary key.
 */
int c_rest_orm_crud_get_one(struct c_rest_request *req,
                            struct c_rest_response *res, void *user_data);

/**
 * @brief Generates a POST handler for creating a new resource.
 */
int c_rest_orm_crud_create(struct c_rest_request *req,
                           struct c_rest_response *res, void *user_data);

/**
 * @brief Generates a PUT/PATCH handler for updating a resource.
 */
int c_rest_orm_crud_update(struct c_rest_request *req,
                           struct c_rest_response *res, void *user_data);

/**
 * @brief Generates a DELETE handler for removing a resource.
 */
int c_rest_orm_crud_delete(struct c_rest_request *req,
                           struct c_rest_response *res, void *user_data);

/**
 * @brief Utility handler to perform a database health check.
 * Returns 200 OK if the database connection pool is healthy, 503 otherwise.
 */
int c_rest_orm_health_check(struct c_rest_request *req,
                            struct c_rest_response *res, void *user_data);

struct c_rest_context;

/**
 * @brief Executes database migrations from the specified directory.
 * @param ctx The framework context containing the DB pool.
 * @param migration_dir The path to the directory containing migration files.
 * @return 0 on success, non-zero error code on failure.
 */
int c_rest_orm_run_migrations(struct c_rest_context *ctx,
                              const char *migration_dir);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* C_REST_ORM_CRUD_H */
