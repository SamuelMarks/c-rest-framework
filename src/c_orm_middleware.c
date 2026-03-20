/* clang-format off */
#include "c_rest_modality.h"
#include "c_rest_orm_middleware.h"
#include "c_rest_request.h"
#include "c_rest_response.h"

#include <stddef.h>
/* clang-format on */

#ifndef C_REST_FRAMEWORK_USE_REAL_C_ORM

int c_rest_orm_transaction_start_middleware(struct c_rest_request *req,
                                            struct c_rest_response *res,
                                            void *user_data) {
  struct c_rest_context *ctx = (struct c_rest_context *)user_data;
  (void)res;

  if (!req || !ctx) {
    return 1;
  }

  if (ctx->db_pool) {
    /* Mock: Assign a dummy pointer to db_conn and pretend to start transaction
     */
    req->db_conn = (struct c_orm_connection *)1;
  }

  return 0;
}

int c_rest_orm_transaction_end_middleware(struct c_rest_request *req,
                                          struct c_rest_response *res,
                                          void *user_data) {
  (void)user_data;

  if (!req || !req->db_conn) {
    return 0;
  }

  /* Mock: if status >= 400 rollback, else commit, then return to pool */
  if (res && res->status_code >= 400) {
    /* Rollback mock */
  } else {
    /* Commit mock */
  }

  req->db_conn = NULL;
  return 0;
}

#else

/* Here we would include c-orm real headers and call c_orm_pool_borrow(),
   c_orm_begin(), c_orm_commit(), c_orm_rollback(), c_orm_pool_release(). */
/* ... */

#endif /* C_REST_FRAMEWORK_USE_REAL_C_ORM */
