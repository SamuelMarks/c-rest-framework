#ifndef C_REST_MEM_H
#define C_REST_MEM_H

/* clang-format off */
#include <stddef.h>
#include <stdlib.h>
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

int c_rest_mem_tracker_init(void);
int c_rest_mem_malloc(size_t size, const char *file, int line, void **out_ptr);
int c_rest_mem_calloc(size_t count, size_t size, const char *file, int line,
                      void **out_ptr);
int c_rest_mem_realloc(void *ptr, size_t size, const char *file, int line,
                       void **out_ptr);
int c_rest_mem_free(void *ptr);
int c_rest_mem_tracker_print_leaks(void);
int c_rest_mem_tracker_cleanup(void);

#ifdef C_REST_MEM_TRACK
#define C_REST_MALLOC(size, out_ptr)                                           \
  c_rest_mem_malloc(size, __FILE__, __LINE__, out_ptr)
#define C_REST_CALLOC(count, size, out_ptr)                                    \
  c_rest_mem_calloc(count, size, __FILE__, __LINE__, out_ptr)
#define C_REST_REALLOC(ptr, size, out_ptr)                                     \
  c_rest_mem_realloc(ptr, size, __FILE__, __LINE__, out_ptr)
#define C_REST_FREE(ptr) c_rest_mem_free(ptr)
#else
#define C_REST_MALLOC(size, out_ptr)                                           \
  (*(out_ptr) = malloc(size), *(out_ptr) ? 0 : 1)
#define C_REST_CALLOC(count, size, out_ptr)                                    \
  (*(out_ptr) = calloc(count, size), *(out_ptr) ? 0 : 1)
#define C_REST_REALLOC(ptr, size, out_ptr)                                     \
  (*(out_ptr) = realloc(ptr, size), *(out_ptr) ? 0 : 1)
#define C_REST_FREE(ptr) (free(ptr), 0)
#endif

#if defined(__cplusplus)
}
#endif /* __cplusplus */
#endif /* C_REST_MEM_H */
