#ifndef C_REST_POOL_H
#define C_REST_POOL_H

/* clang-format off */
#include <stddef.h>
/* clang-format on */
#ifdef __cplusplus
extern "C" {
#endif

/** @brief Memory pool block */
typedef struct c_rest_pool_block {
  /** @brief Pointer to next element */
  struct c_rest_pool_block *next;
} c_rest_pool_block;

/** @brief Memory pool */
typedef struct c_rest_pool {
  /** @brief List of free blocks */
  c_rest_pool_block *free_list;
  /** @brief Size of an object */
  size_t object_size;
} c_rest_pool;

int c_rest_pool_init(c_rest_pool *pool, size_t object_size);
int c_rest_pool_allocate(c_rest_pool *pool, void **out_ptr);
int c_rest_pool_free(c_rest_pool *pool, void *ptr);
int c_rest_pool_destroy(c_rest_pool *pool);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* C_REST_POOL_H */
