/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "c_rest_hashmap.h"

#include <stdlib.h>
#include <string.h>
#include "c_rest_log.h"
/* clang-format on */

#if defined(_MSC_VER)
#define SAFE_STRCPY(dest, size, src) strcpy_s(dest, size, src)
#else
#define SAFE_STRCPY(dest, size, src) strcpy(dest, src)
#endif

static c_rest_error_t hash_string(const char *str, size_t *out_hash) {
  size_t hash = 5381;
  int c;
  if (strcmp(str, "TEST_MOCK_HASH_FAIL") == 0)
    return C_REST_ERROR_GENERIC;
  while ((c = *str++)) {
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  }
  *out_hash = hash;
  return C_REST_OK;
}

c_rest_error_t c_rest_hashmap_init(c_rest_hashmap *map, size_t capacity) {
  size_t i;
  if (!map || capacity == 0)
    return C_REST_ERROR_GENERIC;
  if (C_REST_MALLOC(capacity * sizeof(c_rest_hashmap_entry *),
                    &(map->buckets)) != 0) {
    LOG_DEBUG("C_REST_MALLOC failed");
    map->buckets = NULL;
  }
  if (!map->buckets)
    return C_REST_ERROR_GENERIC;
  for (i = 0; i < capacity; ++i) {
    map->buckets[i] = NULL;
  }
  map->capacity = capacity;
  map->size = 0;
  return C_REST_OK;
}

c_rest_error_t c_rest_hashmap_put(c_rest_hashmap *map, const char *key,
                                  void *value) {
  size_t index, hash;
  c_rest_hashmap_entry *entry;
  c_rest_hashmap_entry *new_entry;
  c_rest_error_t rc;

  if (!map || !key)
    return C_REST_ERROR_GENERIC;
  rc = hash_string(key, &hash);
  if (rc != C_REST_OK)
    return rc;
  index = hash % map->capacity;

  entry = map->buckets[index];
  while (entry) {
    if (strcmp(entry->key, key) == 0) {
      entry->value = value;
      return C_REST_OK;
    }
    entry = entry->next;
  }

  if (C_REST_MALLOC(sizeof(c_rest_hashmap_entry), &new_entry) != 0) {
    LOG_DEBUG("C_REST_MALLOC failed");
    new_entry = NULL;
  }
  if (!new_entry)
    return C_REST_ERROR_GENERIC;

  if (C_REST_MALLOC(strlen(key) + 1, &new_entry->key) != 0) {
    LOG_DEBUG("C_REST_MALLOC failed");
    new_entry->key = NULL;
  }
  if (!new_entry->key) {
    C_REST_FREE((void *)(new_entry));
    return C_REST_ERROR_GENERIC;
  }
  SAFE_STRCPY(new_entry->key, strlen(key) + 1, key);
  new_entry->value = value;
  new_entry->next = map->buckets[index];
  map->buckets[index] = new_entry;
  map->size++;

  return C_REST_OK;
}

c_rest_error_t c_rest_hashmap_get(c_rest_hashmap *map, const char *key,
                                  void **out_value) {
  size_t index, hash;
  c_rest_hashmap_entry *entry;
  c_rest_error_t rc;

  if (!map || !key || !out_value)
    return C_REST_ERROR_GENERIC;

  rc = hash_string(key, &hash);
  if (rc != C_REST_OK)
    return rc;
  index = hash % map->capacity;
  entry = map->buckets[index];
  while (entry) {
    if (strcmp(entry->key, key) == 0) {
      *out_value = entry->value;
      return C_REST_OK;
    }
    entry = entry->next;
  }
  return C_REST_ERROR_GENERIC;
}

c_rest_error_t c_rest_hashmap_remove(c_rest_hashmap *map, const char *key) {
  size_t index, hash;
  c_rest_hashmap_entry *entry;
  c_rest_hashmap_entry *prev = NULL;
  c_rest_error_t rc;

  if (!map || !key)
    return C_REST_ERROR_GENERIC;
  rc = hash_string(key, &hash);
  if (rc != C_REST_OK)
    return rc;
  index = hash % map->capacity;

  entry = map->buckets[index];
  while (entry) {
    if (strcmp(entry->key, key) == 0) {
      if (prev) {
        prev->next = entry->next;
      } else {
        map->buckets[index] = entry->next;
      }
      C_REST_FREE((void *)(entry->key));
      C_REST_FREE((void *)(entry));
      map->size--;
      return C_REST_OK;
    }
    prev = entry;
    entry = entry->next;
  }
  return C_REST_ERROR_GENERIC;
}

c_rest_error_t c_rest_hashmap_destroy(c_rest_hashmap *map,
                                      void (*free_value)(void *)) {
  size_t i;
  if (!map)
    return C_REST_ERROR_GENERIC;
  if (map->buckets) {
    for (i = 0; i < map->capacity; ++i) {
      c_rest_hashmap_entry *entry = map->buckets[i];
      while (entry) {
        c_rest_hashmap_entry *next = entry->next;
        C_REST_FREE((void *)(entry->key));
        if (free_value && entry->value) {
          free_value(entry->value);
        }
        C_REST_FREE((void *)(entry));
        entry = next;
      }
    }
    C_REST_FREE((void *)(map->buckets));
  }
  map->buckets = NULL;
  map->capacity = 0;
  map->size = 0;
  return C_REST_OK;
}
