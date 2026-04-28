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

/**
 * @brief Initializes a memory pool.
 *
 * @param pool The pool to initialize.
 * @param object_size The size of each object in the pool.
 * @return 0 on success, non-zero on error.
 */
int c_rest_pool_init(c_rest_pool *pool, size_t object_size);

/**
 * @brief Allocates an object from the memory pool.
 *
 * @param pool The pool to allocate from.
 * @param out_ptr Pointer to hold the allocated object.
 * @return 0 on success, non-zero on error.
 */
int c_rest_pool_allocate(c_rest_pool *pool, void **out_ptr);

/**
 * @brief Frees an object back into the memory pool.
 *
 * @param pool The pool to free into.
 * @param ptr The object to free.
 * @return 0 on success, non-zero on error.
 */
int c_rest_pool_free(c_rest_pool *pool, void *ptr);

/**
 * @brief Destroys the memory pool and frees all blocks.
 *
 * @param pool The pool to destroy.
 * @return 0 on success, non-zero on error.
 */
int c_rest_pool_destroy(c_rest_pool *pool);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* C_REST_POOL_H */