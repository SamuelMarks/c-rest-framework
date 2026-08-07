/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "c_rest_pool.h"

#include <stdlib.h>
#include "c_rest_log.h"
/* clang-format on */

c_rest_error_t c_rest_pool_init(c_rest_pool *pool, size_t object_size) {
  if (!pool || object_size == 0)
    return C_REST_ERROR_GENERIC;
  if (object_size < sizeof(c_rest_pool_block)) {
    object_size = sizeof(c_rest_pool_block);
  }
  pool->free_list = NULL;
  pool->object_size = object_size;
  return C_REST_OK;
}

c_rest_error_t c_rest_pool_allocate(c_rest_pool *pool, void **out_ptr) {
  if (!pool || !out_ptr)
    return C_REST_ERROR_GENERIC;
  if (pool->free_list) {
    c_rest_pool_block *block = pool->free_list;
    pool->free_list = block->next;
    *out_ptr = (void *)block;
    return C_REST_OK;
  }
  if (C_REST_MALLOC(pool->object_size, out_ptr) != 0) {
    LOG_DEBUG("C_REST_MALLOC failed");
    *out_ptr = NULL;
  }
  return *out_ptr ? 0 : 1;
}

c_rest_error_t c_rest_pool_free(c_rest_pool *pool, void *ptr) {
  c_rest_pool_block *block;
  if (!pool || !ptr)
    return C_REST_ERROR_GENERIC;
  block = (c_rest_pool_block *)ptr;
  block->next = pool->free_list;
  pool->free_list = block;
  return C_REST_OK;
}

c_rest_error_t c_rest_pool_destroy(c_rest_pool *pool) {
  c_rest_pool_block *block;
  c_rest_pool_block *next;

  if (!pool)
    return C_REST_ERROR_GENERIC;

  block = pool->free_list;
  while (block) {
    next = block->next;
    C_REST_FREE((void *)(block));
    block = next;
  }
  pool->free_list = NULL;
  return C_REST_OK;
}
