#ifndef C_REST_ORM_H
#define C_REST_ORM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations of c-orm structures */
struct c_orm_connection;
struct c_orm_pool;
struct c_orm_query;

/* Database configuration for the framework context */
/** @brief Database configuration */
struct c_rest_db_config {
  /** @brief Connection string */
  char *connection_string;
  /** @brief Maximum connections */
  int max_connections;
  /** @brief Minimum connections */
  int min_connections;
  /** @brief Connection timeout in milliseconds */
  int connect_timeout_ms;
};

/*
 * Wrapper for c-orm initialization within the framework.
 * Returns 0 on success, non-zero on error.
 */
int c_rest_orm_init(struct c_rest_db_config *config, struct c_orm_pool **pool);

/*
 * Wrapper for c-orm cleanup.
 */
int c_rest_orm_cleanup(struct c_orm_pool *pool);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* C_REST_ORM_H */
