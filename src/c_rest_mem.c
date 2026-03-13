/* clang-format off */
#include "c_rest_mem.h"
#include "c_rest_platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* clang-format on */

typedef struct c_rest_mem_node {
  void *ptr;
  size_t size;
  const char *file;
  int line;
  struct c_rest_mem_node *next;
} c_rest_mem_node;

static c_rest_mem_node *mem_list = NULL;
static c_rest_mutex_t mem_mutex = (c_rest_mutex_t)-1;
static int mem_initialized = 0;

int c_rest_mem_tracker_init(void) {
  if (mem_initialized)
    return 0;
  if (c_rest_mutex_create(&mem_mutex) != 0)
    return 1;
  mem_initialized = 1;
  return 0;
}

static int add_node(void *ptr, size_t size, const char *file, int line) {
  c_rest_mem_node *node;
  if (!mem_initialized || !ptr)
    return 1;

  node = (c_rest_mem_node *)malloc(sizeof(c_rest_mem_node));
  if (!node)
    return 1;

  node->ptr = ptr;
  node->size = size;
  node->file = file;
  node->line = line;

  c_rest_mutex_lock(mem_mutex);
  node->next = mem_list;
  mem_list = node;
  c_rest_mutex_unlock(mem_mutex);
  return 0;
}

static int remove_node(void *ptr) {
  c_rest_mem_node *curr;
  c_rest_mem_node *prev = NULL;

  if (!mem_initialized || !ptr)
    return 1;

  c_rest_mutex_lock(mem_mutex);
  curr = mem_list;
  while (curr) {
    if (curr->ptr == ptr) {
      if (prev) {
        prev->next = curr->next;
      } else {
        mem_list = curr->next;
      }
      free(curr);
      break;
    }
    prev = curr;
    curr = curr->next;
  }
  c_rest_mutex_unlock(mem_mutex);
  return 0;
}

int c_rest_mem_malloc(size_t size, const char *file, int line, void **out_ptr) {
  void *ptr;
  if (!out_ptr)
    return 1;
  ptr = malloc(size);
  add_node(ptr, size, file, line);
  *out_ptr = ptr;
  return ptr ? 0 : 1;
}

int c_rest_mem_calloc(size_t count, size_t size, const char *file, int line,
                      void **out_ptr) {
  void *ptr;
  if (!out_ptr)
    return 1;
  ptr = calloc(count, size);
  add_node(ptr, count * size, file, line);
  *out_ptr = ptr;
  return ptr ? 0 : 1;
}

int c_rest_mem_realloc(void *ptr, size_t size, const char *file, int line,
                       void **out_ptr) {
  c_rest_mem_node *curr;
  void *new_ptr;

  if (!out_ptr)
    return 1;

  if (!mem_initialized) {
    new_ptr = realloc(ptr, size);
    *out_ptr = new_ptr;
    return new_ptr ? 0 : 1;
  }

  if (!ptr) {
    return c_rest_mem_malloc(size, file, line, out_ptr);
  }
  if (size == 0) {
    c_rest_mem_free(ptr);
    *out_ptr = NULL;
    return 0;
  }

  c_rest_mutex_lock(mem_mutex);
  curr = mem_list;
  while (curr) {
    if (curr->ptr == ptr) {
      break;
    }
    curr = curr->next;
  }
  c_rest_mutex_unlock(mem_mutex);

  new_ptr = realloc(ptr, size);
  if (new_ptr && curr) {
    c_rest_mutex_lock(mem_mutex);
    curr->ptr = new_ptr;
    curr->size = size;
    curr->file = file;
    curr->line = line;
    c_rest_mutex_unlock(mem_mutex);
  }
  *out_ptr = new_ptr;
  return new_ptr ? 0 : 1;
}
int c_rest_mem_free(void *ptr) {
  if (ptr) {
    remove_node(ptr);
    free(ptr);
  }
  return 0;
}

int c_rest_mem_tracker_print_leaks(void) {
  c_rest_mem_node *curr;
  int count = 0;
  size_t total_leaked = 0;

  if (!mem_initialized)
    return 0;

  c_rest_mutex_lock(mem_mutex);
  curr = mem_list;
  while (curr) {
    fprintf(stderr, "Leak: %lu bytes at %p, allocated in %s:%d\n",
            (unsigned long)curr->size, curr->ptr, curr->file, curr->line);
    total_leaked += curr->size;
    count++;
    curr = curr->next;
  }
  c_rest_mutex_unlock(mem_mutex);

  if (count > 0) {
    fprintf(stderr, "Total Leaks: %d (%lu bytes)\n", count,
            (unsigned long)total_leaked);
  } else {
    fprintf(stderr, "No memory leaks detected.\n");
  }

  return count;
}

int c_rest_mem_tracker_cleanup(void) {
  c_rest_mem_node *curr;
  c_rest_mem_node *next;

  if (!mem_initialized)
    return 0;

  c_rest_mutex_lock(mem_mutex);
  curr = mem_list;
  while (curr) {
    next = curr->next;
    free(curr);
    curr = next;
  }
  mem_list = NULL;
  c_rest_mutex_unlock(mem_mutex);

  c_rest_mutex_destroy(mem_mutex);
  mem_initialized = 0;
  return 0;
}
