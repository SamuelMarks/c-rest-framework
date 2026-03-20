/* clang-format off */
#include "c_rest_hashmap.h"

#include <stdlib.h>
#include <string.h>
/* clang-format on */

#if defined(_MSC_VER)
#define SAFE_STRCPY(dest, size, src) strcpy_s(dest, size, src)
#else
#define SAFE_STRCPY(dest, size, src) strcpy(dest, src)
#endif

static size_t hash_string(const char *str) {
  size_t hash = 5381;
  int c;
  while ((c = *str++)) {
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  }
  return hash;
}

int c_rest_hashmap_init(c_rest_hashmap *map, size_t capacity) {
  size_t i;
  if (!map || capacity == 0)
    return 1;
  map->buckets = (c_rest_hashmap_entry **)malloc(
      capacity * sizeof(c_rest_hashmap_entry *));
  if (!map->buckets)
    return 1;
  for (i = 0; i < capacity; ++i) {
    map->buckets[i] = NULL;
  }
  map->capacity = capacity;
  map->size = 0;
  return 0;
}

int c_rest_hashmap_put(c_rest_hashmap *map, const char *key, void *value) {
  size_t index;
  c_rest_hashmap_entry *entry;
  c_rest_hashmap_entry *new_entry;

  if (!map || !key)
    return 1;
  index = hash_string(key) % map->capacity;

  entry = map->buckets[index];
  while (entry) {
    if (strcmp(entry->key, key) == 0) {
      entry->value = value;
      return 0;
    }
    entry = entry->next;
  }

  new_entry = (c_rest_hashmap_entry *)malloc(sizeof(c_rest_hashmap_entry));
  if (!new_entry)
    return 1;

  new_entry->key = (char *)malloc(strlen(key) + 1);
  if (!new_entry->key) {
    free(new_entry);
    return 1;
  }
  SAFE_STRCPY(new_entry->key, strlen(key) + 1, key);
  new_entry->value = value;
  new_entry->next = map->buckets[index];
  map->buckets[index] = new_entry;
  map->size++;

  return 0;
}

int c_rest_hashmap_get(c_rest_hashmap *map, const char *key, void **out_value) {
  size_t index;
  c_rest_hashmap_entry *entry;

  if (!map || !key || !out_value)
    return 1;

  index = hash_string(key) % map->capacity;
  entry = map->buckets[index];
  while (entry) {
    if (strcmp(entry->key, key) == 0) {
      *out_value = entry->value;
      return 0;
    }
    entry = entry->next;
  }
  return 1;
}

int c_rest_hashmap_remove(c_rest_hashmap *map, const char *key) {
  size_t index;
  c_rest_hashmap_entry *entry;
  c_rest_hashmap_entry *prev = NULL;

  if (!map || !key)
    return 1;
  index = hash_string(key) % map->capacity;

  entry = map->buckets[index];
  while (entry) {
    if (strcmp(entry->key, key) == 0) {
      if (prev) {
        prev->next = entry->next;
      } else {
        map->buckets[index] = entry->next;
      }
      free(entry->key);
      free(entry);
      map->size--;
      return 0;
    }
    prev = entry;
    entry = entry->next;
  }
  return 1;
}

int c_rest_hashmap_destroy(c_rest_hashmap *map, void (*free_value)(void *)) {
  size_t i;
  if (!map)
    return 1;
  if (map->buckets) {
    for (i = 0; i < map->capacity; ++i) {
      c_rest_hashmap_entry *entry = map->buckets[i];
      while (entry) {
        c_rest_hashmap_entry *next = entry->next;
        free(entry->key);
        if (free_value && entry->value) {
          free_value(entry->value);
        }
        free(entry);
        entry = next;
      }
    }
    free(map->buckets);
  }
  map->buckets = NULL;
  map->capacity = 0;
  map->size = 0;
  return 0;
}
