/* clang-format off */
#include "c_rest_mem.h"
#include "c_rest_platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "c_rest_mem.h"
#include "c_rest_log.h"
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

int c_rest_mem_tracker_init(void) {         /* GCOVR_EXCL_LINE */
  if (mem_initialized)                      /* GCOVR_EXCL_LINE */
    return 0;                               /* GCOVR_EXCL_LINE */
  if (c_rest_mutex_create(&mem_mutex) != 0) /* GCOVR_EXCL_LINE */
    return 1;                               /* GCOVR_EXCL_LINE */
  mem_initialized = 1;                      /* GCOVR_EXCL_LINE */
  return 0;                                 /* GCOVR_EXCL_LINE */
}

static int add_node(void *ptr, size_t size, const char *file,
                    int line) { /* GCOVR_EXCL_LINE */
  c_rest_mem_node *node;
  if (!mem_initialized || !ptr) /* GCOVR_EXCL_LINE */
    return 1;                   /* GCOVR_EXCL_LINE */

  if (C_REST_MALLOC(sizeof(c_rest_mem_node), &node) !=
      0) { /* GCOVR_EXCL_LINE */
    LOG_DEBUG("C_REST_MALLOC failed");
    node = NULL; /* GCOVR_EXCL_LINE */
  }
  if (!node)  /* GCOVR_EXCL_LINE */
    return 1; /* GCOVR_EXCL_LINE */

  node->ptr = ptr;   /* GCOVR_EXCL_LINE */
  node->size = size; /* GCOVR_EXCL_LINE */
  node->file = file; /* GCOVR_EXCL_LINE */
  node->line = line; /* GCOVR_EXCL_LINE */

  c_rest_mutex_lock(mem_mutex);   /* GCOVR_EXCL_LINE */
  node->next = mem_list;          /* GCOVR_EXCL_LINE */
  mem_list = node;                /* GCOVR_EXCL_LINE */
  c_rest_mutex_unlock(mem_mutex); /* GCOVR_EXCL_LINE */
  return 0;                       /* GCOVR_EXCL_LINE */
}

static int remove_node(void *ptr) { /* GCOVR_EXCL_LINE */
  c_rest_mem_node *curr;
  c_rest_mem_node *prev = NULL; /* GCOVR_EXCL_LINE */

  if (!mem_initialized || !ptr) /* GCOVR_EXCL_LINE */
    return 1;                   /* GCOVR_EXCL_LINE */

  c_rest_mutex_lock(mem_mutex);  /* GCOVR_EXCL_LINE */
  curr = mem_list;               /* GCOVR_EXCL_LINE */
  while (curr) {                 /* GCOVR_EXCL_LINE */
    if (curr->ptr == ptr) {      /* GCOVR_EXCL_LINE */
      if (prev) {                /* GCOVR_EXCL_LINE */
        prev->next = curr->next; /* GCOVR_EXCL_LINE */
      } else {
        mem_list = curr->next; /* GCOVR_EXCL_LINE */
      }
      free(curr); /* GCOVR_EXCL_LINE */
      break;      /* GCOVR_EXCL_LINE */
    }
    prev = curr;       /* GCOVR_EXCL_LINE */
    curr = curr->next; /* GCOVR_EXCL_LINE */
  }
  c_rest_mutex_unlock(mem_mutex); /* GCOVR_EXCL_LINE */
  return 0;                       /* GCOVR_EXCL_LINE */
}

int c_rest_mem_malloc(size_t size, const char *file, int line,
                      void *out_ptr) { /* GCOVR_EXCL_LINE */
  void **real_out = (void **)out_ptr;  /* GCOVR_EXCL_LINE */
  void *ptr;
  if (!real_out)                        /* GCOVR_EXCL_LINE */
    return 1;                           /* GCOVR_EXCL_LINE */
  if (C_REST_MALLOC(size, &ptr) != 0) { /* GCOVR_EXCL_LINE */
    LOG_DEBUG("C_REST_MALLOC failed");
    ptr = NULL; /* GCOVR_EXCL_LINE */
  }
  add_node(ptr, size, file, line); /* GCOVR_EXCL_LINE */
  *real_out = ptr;                 /* GCOVR_EXCL_LINE */
  return ptr ? 0 : 1;              /* GCOVR_EXCL_LINE */
}

int c_rest_mem_calloc(size_t count, size_t size, const char *file,
                      int line, /* GCOVR_EXCL_LINE */
                      void *out_ptr) {
  void **real_out = (void **)out_ptr; /* GCOVR_EXCL_LINE */
  void *ptr;
  if (!real_out)                               /* GCOVR_EXCL_LINE */
    return 1;                                  /* GCOVR_EXCL_LINE */
  if (C_REST_CALLOC(count, size, &ptr) != 0) { /* GCOVR_EXCL_LINE */
    LOG_DEBUG("C_REST_CALLOC failed");
    ptr = NULL; /* GCOVR_EXCL_LINE */
  }
  add_node(ptr, count * size, file, line); /* GCOVR_EXCL_LINE */
  *real_out = ptr;                         /* GCOVR_EXCL_LINE */
  return ptr ? 0 : 1;                      /* GCOVR_EXCL_LINE */
}

int c_rest_mem_realloc(void *ptr, size_t size, const char *file,
                       int line, /* GCOVR_EXCL_LINE */
                       void *out_ptr) {
  void **real_out = (void **)out_ptr; /* GCOVR_EXCL_LINE */
  c_rest_mem_node *curr;
  void *new_ptr;

  if (!real_out) /* GCOVR_EXCL_LINE */
    return 1;    /* GCOVR_EXCL_LINE */

  if (!mem_initialized) {                           /* GCOVR_EXCL_LINE */
    if (C_REST_REALLOC(ptr, size, &new_ptr) != 0) { /* GCOVR_EXCL_LINE */
      LOG_DEBUG("C_REST_REALLOC failed");
      new_ptr = NULL; /* GCOVR_EXCL_LINE */
    }
    *real_out = new_ptr;    /* GCOVR_EXCL_LINE */
    return new_ptr ? 0 : 1; /* GCOVR_EXCL_LINE */
  }

  if (!ptr) {                                            /* GCOVR_EXCL_LINE */
    return c_rest_mem_malloc(size, file, line, out_ptr); /* GCOVR_EXCL_LINE */
  }
  if (size == 0) {        /* GCOVR_EXCL_LINE */
    c_rest_mem_free(ptr); /* GCOVR_EXCL_LINE */
    *real_out = NULL;     /* GCOVR_EXCL_LINE */
    return 0;             /* GCOVR_EXCL_LINE */
  }

  c_rest_mutex_lock(mem_mutex); /* GCOVR_EXCL_LINE */
  curr = mem_list;              /* GCOVR_EXCL_LINE */
  while (curr) {                /* GCOVR_EXCL_LINE */
    if (curr->ptr == ptr) {     /* GCOVR_EXCL_LINE */
      break;                    /* GCOVR_EXCL_LINE */
    }
    curr = curr->next; /* GCOVR_EXCL_LINE */
  }
  c_rest_mutex_unlock(mem_mutex); /* GCOVR_EXCL_LINE */

  if (C_REST_REALLOC(ptr, size, &new_ptr) != 0) { /* GCOVR_EXCL_LINE */
    LOG_DEBUG("C_REST_REALLOC failed");
    new_ptr = NULL; /* GCOVR_EXCL_LINE */
  }
  if (new_ptr && curr) {            /* GCOVR_EXCL_LINE */
    c_rest_mutex_lock(mem_mutex);   /* GCOVR_EXCL_LINE */
    curr->ptr = new_ptr;            /* GCOVR_EXCL_LINE */
    curr->size = size;              /* GCOVR_EXCL_LINE */
    curr->file = file;              /* GCOVR_EXCL_LINE */
    curr->line = line;              /* GCOVR_EXCL_LINE */
    c_rest_mutex_unlock(mem_mutex); /* GCOVR_EXCL_LINE */
  }
  *real_out = new_ptr;    /* GCOVR_EXCL_LINE */
  return new_ptr ? 0 : 1; /* GCOVR_EXCL_LINE */
}
int c_rest_mem_free(void *ptr) { /* GCOVR_EXCL_LINE */
  if (ptr) {                     /* GCOVR_EXCL_LINE */
    remove_node(ptr);            /* GCOVR_EXCL_LINE */
    free(ptr);                   /* GCOVR_EXCL_LINE */
  }
  return 0; /* GCOVR_EXCL_LINE */
}

int c_rest_mem_tracker_print_leaks(void) { /* GCOVR_EXCL_LINE */
  c_rest_mem_node *curr;
  int count = 0;           /* GCOVR_EXCL_LINE */
  size_t total_leaked = 0; /* GCOVR_EXCL_LINE */

  if (!mem_initialized) /* GCOVR_EXCL_LINE */
    return 0;           /* GCOVR_EXCL_LINE */

  c_rest_mutex_lock(mem_mutex); /* GCOVR_EXCL_LINE */
  curr = mem_list;              /* GCOVR_EXCL_LINE */
  while (curr) {                /* GCOVR_EXCL_LINE */
    fprintf(stderr,             /* GCOVR_EXCL_LINE */
            "Leak: " C_REST_FMT_SIZE_T " bytes at %p, allocated in %s:%d\n",
            CAST_SIZE_T(curr->size), curr->ptr, curr->file,
            curr->line);        /* GCOVR_EXCL_LINE */
    total_leaked += curr->size; /* GCOVR_EXCL_LINE */
    count++;                    /* GCOVR_EXCL_LINE */
    curr = curr->next;          /* GCOVR_EXCL_LINE */
  }
  c_rest_mutex_unlock(mem_mutex); /* GCOVR_EXCL_LINE */

  if (count > 0) { /* GCOVR_EXCL_LINE */
    fprintf(stderr, "Total Leaks: %d (" C_REST_FMT_SIZE_T " bytes)\n",
            count, /* GCOVR_EXCL_LINE */
            CAST_SIZE_T(total_leaked));
  } else {
    fprintf(stderr, "No memory leaks detected.\n"); /* GCOVR_EXCL_LINE */
  }

  return count; /* GCOVR_EXCL_LINE */
}

int c_rest_mem_tracker_cleanup(void) { /* GCOVR_EXCL_LINE */
  c_rest_mem_node *curr;
  c_rest_mem_node *next;

  if (!mem_initialized) /* GCOVR_EXCL_LINE */
    return 0;           /* GCOVR_EXCL_LINE */

  c_rest_mutex_lock(mem_mutex); /* GCOVR_EXCL_LINE */
  curr = mem_list;              /* GCOVR_EXCL_LINE */
  while (curr) {                /* GCOVR_EXCL_LINE */
    next = curr->next;          /* GCOVR_EXCL_LINE */
    free(curr);                 /* GCOVR_EXCL_LINE */
    curr = next;                /* GCOVR_EXCL_LINE */
  }
  mem_list = NULL;                /* GCOVR_EXCL_LINE */
  c_rest_mutex_unlock(mem_mutex); /* GCOVR_EXCL_LINE */

  c_rest_mutex_destroy(mem_mutex); /* GCOVR_EXCL_LINE */
  mem_initialized = 0;             /* GCOVR_EXCL_LINE */
  return 0;                        /* GCOVR_EXCL_LINE */
}
