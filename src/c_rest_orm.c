/* clang-format off */
#include "c_rest_orm.h"
#include "c_rest_error.h"
#include "c_rest_export.h"
#include <stddef.h>
/* clang-format on */

c_rest_error_t c_rest_orm_init(struct c_rest_db_config *config,
                               struct c_orm_pool **pool) {
  /* Provide a dummy pool pointer or integrate with real c-orm pool creation */
  if (config && pool) {
    *pool = (struct c_orm_pool *)1; /* TODO: use real c-orm */
  }
  return C_REST_OK;
}

c_rest_error_t c_rest_orm_cleanup(struct c_orm_pool *pool) {
  (void)pool;
  return C_REST_OK;
}
