#include "c_rest_testing_mocks.h"
/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"
#ifdef C_REST_TESTING_MALLOC_HOOK
C_REST_EXPORT void *(*g_crf_malloc_hook)(size_t) = NULL;
C_REST_EXPORT void *(*g_crf_calloc_hook)(size_t, size_t) = NULL;
C_REST_EXPORT void *(*g_crf_realloc_hook)(void *, size_t) = NULL;
C_REST_EXPORT char *(*g_crf_strdup_hook)(const char *) = NULL;







C_REST_EXPORT void *test_c_rest_internal_malloc(size_t size) {
  if (g_crf_malloc_hook) return g_crf_malloc_hook(size);
  return malloc(size);
}

C_REST_EXPORT void *test_c_rest_internal_calloc(size_t count, size_t size) {
  if (g_crf_calloc_hook) return g_crf_calloc_hook(count, size);
  return calloc(count, size);
}

C_REST_EXPORT void *test_c_rest_internal_realloc(void *ptr, size_t size) {
  if (g_crf_realloc_hook) return g_crf_realloc_hook(ptr, size);
  return realloc(ptr, size);
}
#endif

#include "c_rest_platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "c_rest_log.h"
/* clang-format on */

#ifndef _MSC_VER
char *c_rest_internal_strdup(const char *s) {
  size_t len;
  char *dup;
  if (!s)
    return NULL;
  len = strlen(s) + 1;
  dup = (char *)malloc(len);
  if (dup) {
    memcpy(dup, s, len);
  }
  return dup;
}
#endif

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

#ifdef C_REST_TESTING_MALLOC_HOOK
C_REST_EXPORT c_rest_mutex_t *g_crf_mem_mutex_ptr = &mem_mutex;
C_REST_EXPORT int *g_crf_mem_initialized_ptr = &mem_initialized;
#endif

c_rest_error_t c_rest_mem_tracker_init(void) {
  c_rest_error_t rc;
  if (mem_initialized)
    return C_REST_OK;
  rc = c_rest_mutex_create(&mem_mutex);
  if (rc != C_REST_OK) {
    return rc;
  }
  mem_initialized = 1;
  return C_REST_OK;
}

static c_rest_error_t add_node(void *ptr, size_t size, const char *file,
                               int line) {
  c_rest_mem_node *node;
  c_rest_error_t rc;
  if (!mem_initialized)
    return C_REST_ERROR_GENERIC;

  if (C_REST_MALLOC(sizeof(c_rest_mem_node), &node) != 0) {
    LOG_DEBUG("C_REST_MALLOC failed");
    node = NULL;
  }
  if (!node)
    return C_REST_ERROR_OOM;

  node->ptr = ptr;
  node->size = size;
  node->file = file;
  node->line = line;

  rc = c_rest_mutex_lock(mem_mutex);
  if (rc != C_REST_OK) {
    CRF_FREE(node);
    return rc;
  }
  node->next = mem_list;
  mem_list = node;
  rc = c_rest_mutex_unlock(mem_mutex);
  if (rc != C_REST_OK)
    return rc;
  return C_REST_OK;
}

static c_rest_error_t remove_node(void *ptr) {
  c_rest_mem_node *curr;
  c_rest_mem_node *prev = NULL;

  c_rest_error_t rc;

  if (!mem_initialized)
    return C_REST_ERROR_GENERIC;

  rc = c_rest_mutex_lock(mem_mutex);
  if (rc != C_REST_OK)
    return rc;

  curr = mem_list;
  while (curr) {
    if (curr->ptr == ptr) {
      if (prev) {
        prev->next = curr->next;
      } else {
        mem_list = curr->next;
      }
      CRF_FREE(curr);
      break;
    }
    prev = curr;
    curr = curr->next;
  }
  rc = c_rest_mutex_unlock(mem_mutex);
  if (rc != C_REST_OK)
    return rc;
  return C_REST_OK;
}

c_rest_error_t c_rest_mem_malloc(size_t size, const char *file, int line,
                                 void *out_ptr) {
  void **real_out = (void **)out_ptr;
  void *ptr;
  c_rest_error_t rc;
  if (!real_out)
    return C_REST_ERROR_GENERIC;
  if (C_REST_MALLOC(size, &ptr) != 0) {
    LOG_DEBUG("C_REST_MALLOC failed");
    ptr = NULL;
  }
  if (!ptr)
    return C_REST_ERROR_OOM;
  rc = add_node(ptr, size, file, line);
  if (rc != C_REST_OK)
    return rc;
  *real_out = ptr;
  return C_REST_OK;
}

c_rest_error_t c_rest_mem_calloc(size_t count, size_t size, const char *file,
                                 int line, void *out_ptr) {
  void **real_out = (void **)out_ptr;
  void *ptr;
  c_rest_error_t rc;
  if (!real_out)
    return C_REST_ERROR_GENERIC;
  if (C_REST_CALLOC(count, size, &ptr) != 0) {
    LOG_DEBUG("C_REST_CALLOC failed");
    ptr = NULL;
  }
  if (!ptr)
    return C_REST_ERROR_OOM;
  rc = add_node(ptr, count * size, file, line);
  if (rc != C_REST_OK) {
    CRF_FREE(ptr);
    return rc;
  }
  *real_out = ptr;
  return C_REST_OK;
}

c_rest_error_t c_rest_mem_realloc(void *ptr, size_t size, const char *file,
                                  int line, void *out_ptr) {
  void **real_out = (void **)out_ptr;
  c_rest_mem_node *curr;
  void *new_ptr;
  c_rest_error_t rc;

  if (!real_out)
    return C_REST_ERROR_GENERIC;

  if (!mem_initialized) {
    if (C_REST_REALLOC(ptr, size, &new_ptr) != 0) {
      LOG_DEBUG("C_REST_REALLOC failed");
      new_ptr = NULL;
    }
    *real_out = new_ptr;
    return new_ptr ? C_REST_OK : C_REST_ERROR_OOM;
  }

  if (!ptr) {
    return c_rest_mem_malloc(size, file, line, out_ptr);
  }
  if (size == 0) {
    {
      rc = c_rest_mem_free(ptr);
      if (rc != C_REST_OK)
        return rc;
    }
    *real_out = NULL;
    return C_REST_OK;
  }

  rc = c_rest_mutex_lock(mem_mutex);
  if (rc != C_REST_OK)
    return rc;
  curr = mem_list;
  if (!curr)
    goto end_search2;
search_loop2:
  if (curr->ptr == ptr) {
    goto end_search2;
  }
  curr = curr->next;
  if (curr)
    goto search_loop2;
end_search2:
  rc = c_rest_mutex_unlock(mem_mutex);
  if (rc != C_REST_OK)
    return rc;

  if (C_REST_REALLOC(ptr, size, &new_ptr) != 0) {
    LOG_DEBUG("C_REST_REALLOC failed");
    new_ptr = NULL;
  }
  if (new_ptr && curr) {
    rc = c_rest_mutex_lock(mem_mutex);
    if (rc != C_REST_OK)
      return rc;
    curr->ptr = new_ptr;
    curr->size = size;
    curr->file = file;
    curr->line = line;
    rc = c_rest_mutex_unlock(mem_mutex);
    if (rc != C_REST_OK)
      return rc;
  }
  *real_out = new_ptr;
  return new_ptr ? C_REST_OK : C_REST_ERROR_OOM;
}
c_rest_error_t c_rest_mem_strdup(const char *str, const char *file, int line,
                                 char **out_str) {
  size_t len;
  char *ptr;
  c_rest_error_t rc;

  if (!str || !out_str)
    return C_REST_ERROR_GENERIC;

  len = strlen(str) + 1;
  ptr = CRF_STRDUP(str);
  if (!ptr)
    return C_REST_ERROR_OOM;

  rc = add_node(ptr, len, file, line);
  if (rc != C_REST_OK) {
    CRF_FREE(ptr);
    return rc;
  }
  *out_str = ptr;
  return C_REST_OK;
}

c_rest_error_t c_rest_mem_free(void *ptr) {
#ifdef C_REST_TESTING_MALLOC_HOOK
  goto do_free;
#else
  if (!ptr)
    return C_REST_OK;
  goto do_free;
#endif
do_free: {
  c_rest_error_t rc;
  rc = remove_node(ptr);
  if (rc != C_REST_OK)
    return rc;
  CRF_FREE(ptr);
  return C_REST_OK;
}
}

c_rest_error_t c_rest_mem_tracker_print_leaks(void) {
  c_rest_mem_node *curr;
  int count = 0;
  size_t total_leaked = 0;

  c_rest_error_t rc;

  if (!mem_initialized)
    return C_REST_OK;

  rc = c_rest_mutex_lock(mem_mutex);
  if (rc != C_REST_OK)
    return rc;
  curr = mem_list;
  while (curr) {
    fprintf(stderr,
            "Leak: " C_REST_FMT_SIZE_T " bytes at %p, allocated in %s:%d\n",
            CAST_SIZE_T(curr->size), curr->ptr, curr->file, curr->line);
    total_leaked += curr->size;
    count++;
    curr = curr->next;
  }
  rc = c_rest_mutex_unlock(mem_mutex);
  if (rc != C_REST_OK)
    return rc;

#ifdef C_REST_TESTING_MALLOC_HOOK
  goto print_leaks;
#else
  if (count > 0)
    goto print_leaks;
  goto skip_leaks;
#endif
print_leaks: {
  fprintf(stderr, "Total Leaks: %d (" C_REST_FMT_SIZE_T " bytes)\n", count,
          CAST_SIZE_T(total_leaked));
}
#ifndef C_REST_TESTING_MALLOC_HOOK
skip_leaks: { /* No memory leaks detected. */
}
#endif

  if (count > 0)
    return C_REST_ERROR_GENERIC;
  return C_REST_OK;
}

c_rest_error_t c_rest_mem_tracker_cleanup(void) {
  c_rest_mem_node *curr;
  c_rest_mem_node *next;

  c_rest_error_t rc;

  if (!mem_initialized)
    return C_REST_OK;

  rc = c_rest_mutex_lock(mem_mutex);
  if (rc != C_REST_OK)
    return rc;

  curr = mem_list;
  while (curr) {
    next = curr->next;
    CRF_FREE(curr);
    curr = next;
  }
  mem_list = NULL;

  rc = c_rest_mutex_unlock(mem_mutex);
  if (rc != C_REST_OK)
    return rc;

  rc = c_rest_mutex_destroy(mem_mutex);
  if (rc != C_REST_OK)
    return rc;
  mem_initialized = 0;
  return C_REST_OK;
}
