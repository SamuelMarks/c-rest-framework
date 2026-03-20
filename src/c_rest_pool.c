/* clang-format off */
#include "c_rest_pool.h"

#include <stdlib.h>
/* clang-format on */

int c_rest_pool_init(c_rest_pool *pool, size_t object_size) {
  if (!pool || object_size == 0)
    return 1;
  if (object_size < sizeof(c_rest_pool_block)) {
    object_size = sizeof(c_rest_pool_block);
  }
  pool->free_list = NULL;
  pool->object_size = object_size;
  return 0;
}

int c_rest_pool_allocate(c_rest_pool *pool, void **out_ptr) {
  if (!pool || !out_ptr)
    return 1;
  if (pool->free_list) {
    c_rest_pool_block *block = pool->free_list;
    pool->free_list = block->next;
    *out_ptr = (void *)block;
    return 0;
  }
  *out_ptr = malloc(pool->object_size);
  return *out_ptr ? 0 : 1;
}

int c_rest_pool_free(c_rest_pool *pool, void *ptr) {
  c_rest_pool_block *block;
  if (!pool || !ptr)
    return 1;
  block = (c_rest_pool_block *)ptr;
  block->next = pool->free_list;
  pool->free_list = block;
  return 0;
}

int c_rest_pool_destroy(c_rest_pool *pool) {
  c_rest_pool_block *block;
  c_rest_pool_block *next;

  if (!pool)
    return 1;

  block = pool->free_list;
  while (block) {
    next = block->next;
    free(block);
    block = next;
  }
  pool->free_list = NULL;
  return 0;
}
