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

int c_rest_hashmap_init(c_rest_hashmap *map, size_t capacity);
int c_rest_hashmap_put(c_rest_hashmap *map, const char *key, void *value);
int c_rest_hashmap_get(c_rest_hashmap *map, const char *key, void **out_value);
int c_rest_hashmap_remove(c_rest_hashmap *map, const char *key);
int c_rest_hashmap_destroy(c_rest_hashmap *map, void (*free_value)(void *));

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* C_REST_HASHMAP_H */
