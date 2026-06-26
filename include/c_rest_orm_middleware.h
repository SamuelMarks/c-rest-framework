#ifndef C_REST_ORM_MIDDLEWARE_H
#define C_REST_ORM_MIDDLEWARE_H

/* clang-format off */
#include "c_rest_router.h"
#include "c_rest_orm.h"
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

struct c_rest_request;
struct c_rest_response;

/**
 * @brief Middleware that borrows a c-orm connection and starts a transaction.
 *
 * user_data should be a pointer to the c_rest_context containing the db_pool.
 */
int c_rest_orm_transaction_start_middleware(struct c_rest_request *req,
                                            struct c_rest_response *res,
                                            void *user_data);

/**
 * @brief Post-handler hook to commit/rollback transactions and release the
 * connection.
 */
int c_rest_orm_transaction_end_middleware(struct c_rest_request *req,
                                          struct c_rest_response *res,
                                          void *user_data);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* C_REST_ORM_MIDDLEWARE_H */
