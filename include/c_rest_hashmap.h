#ifndef C_REST_HASHMAP_H
#define C_REST_HASHMAP_H

/* clang-format off */
#include <stddef.h>
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Hash map entry */
typedef struct c_rest_hashmap_entry {
  /** @brief Key */
  char *key;
  /** @brief Value */
  void *value;
  /** @brief Pointer to next element */
  struct c_rest_hashmap_entry *next;
} c_rest_hashmap_entry;

/** @brief Hash map structure */
typedef struct c_rest_hashmap {
  /** @brief Array of buckets */
  c_rest_hashmap_entry **buckets;
  /** @brief Total capacity */
  size_t capacity;
  /** @brief Current size */
  size_t size;
} c_rest_hashmap;

/**
 * @brief Initializes a hash map.
 *
 * @param map The hash map to initialize.
 * @param capacity The number of buckets.
 * @return 0 on success, non-zero on error.
 */
int c_rest_hashmap_init(c_rest_hashmap *map, size_t capacity);

/**
 * @brief Puts a key-value pair into the hash map.
 *
 * @param map The hash map.
 * @param key The key string.
 * @param value The value pointer.
 * @return 0 on success, non-zero on error.
 */
int c_rest_hashmap_put(c_rest_hashmap *map, const char *key, void *value);

/**
 * @brief Gets a value from the hash map by key.
 *
 * @param map The hash map.
 * @param key The key string.
 * @param out_value Pointer to hold the retrieved value.
 * @return 0 on success, non-zero on error.
 */
int c_rest_hashmap_get(c_rest_hashmap *map, const char *key, void **out_value);

/**
 * @brief Removes an entry from the hash map by key.
 *
 * @param map The hash map.
 * @param key The key string.
 * @return 0 on success, non-zero on error.
 */
int c_rest_hashmap_remove(c_rest_hashmap *map, const char *key);

/**
 * @brief Destroys the hash map.
 *
 * @param map The hash map to destroy.
 * @param free_value Function to free the values stored in the map.
 * @return 0 on success, non-zero on error.
 */
int c_rest_hashmap_destroy(c_rest_hashmap *map, void (*free_value)(void *));

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* C_REST_HASHMAP_H */
