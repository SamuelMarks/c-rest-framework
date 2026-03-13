/* clang-format off */
#include "c_rest_orm.h"

#include <stddef.h>
/* clang-format on */

#ifndef C_REST_FRAMEWORK_USE_REAL_C_ORM

/* Mock implementations for c-orm functions when c-orm is not present */

int c_rest_orm_init(struct c_rest_db_config *config, struct c_orm_pool **pool) {
  /* Provide a dummy pool pointer */
  if (config && pool) {
    *pool = (struct c_orm_pool *)1;
  }
  return 0;
}

void c_rest_orm_cleanup(struct c_orm_pool *pool) { (void)pool; }

#endif /* C_REST_FRAMEWORK_USE_REAL_C_ORM */
