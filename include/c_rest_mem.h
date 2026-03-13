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
void *c_rest_mem_malloc(size_t size, const char *file, int line);
void *c_rest_mem_calloc(size_t count, size_t size, const char *file, int line);
void *c_rest_mem_realloc(void *ptr, size_t size, const char *file, int line);
void c_rest_mem_free(void *ptr);
int c_rest_mem_tracker_print_leaks(void);
int c_rest_mem_tracker_cleanup(void);

#ifdef C_REST_MEM_TRACK
#define C_REST_MALLOC(size) c_rest_mem_malloc(size, __FILE__, __LINE__)
#define C_REST_CALLOC(count, size)                                             \
  c_rest_mem_calloc(count, size, __FILE__, __LINE__)
#define C_REST_REALLOC(ptr, size)                                              \
  c_rest_mem_realloc(ptr, size, __FILE__, __LINE__)
#define C_REST_FREE(ptr) c_rest_mem_free(ptr)
#else
#define C_REST_MALLOC(size) malloc(size)
#define C_REST_CALLOC(count, size) calloc(count, size)
#define C_REST_REALLOC(ptr, size) realloc(ptr, size)
#define C_REST_FREE(ptr) free(ptr)
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* C_REST_MEM_H */