/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_pool.h"

#include <stdlib.h>
#include "c_rest_mem.h"
#include "c_rest_log.h"
/* clang-format on */

c_rest_error_t c_rest_pool_init(c_rest_pool *pool,
                                size_t object_size) { /* GCOVR_EXCL_LINE */
  if (!pool || object_size == 0)                      /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;                      /* GCOVR_EXCL_LINE */
  if (object_size < sizeof(c_rest_pool_block)) {      /* GCOVR_EXCL_LINE */
    object_size = sizeof(c_rest_pool_block);          /* GCOVR_EXCL_LINE */
  }
  pool->free_list = NULL;          /* GCOVR_EXCL_LINE */
  pool->object_size = object_size; /* GCOVR_EXCL_LINE */
  return C_REST_OK;                /* GCOVR_EXCL_LINE */
}

c_rest_error_t c_rest_pool_allocate(c_rest_pool *pool,
                                    void **out_ptr) { /* GCOVR_EXCL_LINE */
  if (!pool || !out_ptr)                              /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;                      /* GCOVR_EXCL_LINE */
  if (pool->free_list) {                              /* GCOVR_EXCL_LINE */
    c_rest_pool_block *block = pool->free_list;       /* GCOVR_EXCL_LINE */
    pool->free_list = block->next;                    /* GCOVR_EXCL_LINE */
    *out_ptr = (void *)block;                         /* GCOVR_EXCL_LINE */
    return C_REST_OK;                                 /* GCOVR_EXCL_LINE */
  }
  if (C_REST_MALLOC(pool->object_size, out_ptr) != 0) { /* GCOVR_EXCL_LINE */
    LOG_DEBUG("C_REST_MALLOC failed");
    *out_ptr = NULL; /* GCOVR_EXCL_LINE */
  }
  return *out_ptr ? 0 : 1; /* GCOVR_EXCL_LINE */
}

c_rest_error_t c_rest_pool_free(c_rest_pool *pool,
                                void *ptr) { /* GCOVR_EXCL_LINE */
  c_rest_pool_block *block;
  if (!pool || !ptr)                /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;    /* GCOVR_EXCL_LINE */
  block = (c_rest_pool_block *)ptr; /* GCOVR_EXCL_LINE */
  block->next = pool->free_list;    /* GCOVR_EXCL_LINE */
  pool->free_list = block;          /* GCOVR_EXCL_LINE */
  return C_REST_OK;                 /* GCOVR_EXCL_LINE */
}

c_rest_error_t c_rest_pool_destroy(c_rest_pool *pool) { /* GCOVR_EXCL_LINE */
  c_rest_pool_block *block;
  c_rest_pool_block *next;

  if (!pool)                     /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  block = pool->free_list;        /* GCOVR_EXCL_LINE */
  while (block) {                 /* GCOVR_EXCL_LINE */
    next = block->next;           /* GCOVR_EXCL_LINE */
    C_REST_FREE((void *)(block)); /* GCOVR_EXCL_LINE */
    block = next;                 /* GCOVR_EXCL_LINE */
  }
  pool->free_list = NULL; /* GCOVR_EXCL_LINE */
  return C_REST_OK;       /* GCOVR_EXCL_LINE */
}
