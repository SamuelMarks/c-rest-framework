/**
 * @file c_rest_mem.h
 * @brief Memory management and tracking for the C REST framework.
 */
#ifndef C_REST_MEM_H
#define C_REST_MEM_H
/* clang-format off */
#include "c_rest_error.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "c_rest_export.h"
/* clang-format on */

#ifndef CRF_MALLOC
#ifdef C_REST_TESTING_MALLOC_HOOK
C_REST_EXPORT extern void *(*g_crf_malloc_hook)(size_t);
C_REST_EXPORT void *test_c_rest_internal_malloc(size_t size);
#define CRF_MALLOC test_c_rest_internal_malloc
#else
#define CRF_MALLOC malloc
#endif
#endif

#ifndef CRF_CALLOC
#ifdef C_REST_TESTING_MALLOC_HOOK
C_REST_EXPORT extern void *(*g_crf_calloc_hook)(size_t, size_t);
C_REST_EXPORT void *test_c_rest_internal_calloc(size_t count, size_t size);
#define CRF_CALLOC test_c_rest_internal_calloc
#else
#define CRF_CALLOC calloc
#endif
#endif

#ifndef CRF_REALLOC
#ifdef C_REST_TESTING_MALLOC_HOOK
C_REST_EXPORT extern void *(*g_crf_realloc_hook)(void *, size_t);
C_REST_EXPORT void *test_c_rest_internal_realloc(void *ptr, size_t size);
#define CRF_REALLOC test_c_rest_internal_realloc
#else
#define CRF_REALLOC realloc
#endif
#endif

#ifndef CRF_FREE
#define CRF_FREE free
#endif

#ifndef CRF_STRDUP
#ifdef C_REST_TESTING_MALLOC_HOOK
C_REST_EXPORT extern char *(*g_crf_strdup_hook)(const char *);

#ifdef _MSC_VER
#define CRF_STRDUP(s) (g_crf_strdup_hook ? g_crf_strdup_hook(s) : _strdup(s))
#else
char *c_rest_internal_strdup(const char *s);
#define CRF_STRDUP(s)                                                          \
  (g_crf_strdup_hook ? g_crf_strdup_hook(s) : c_rest_internal_strdup(s))
#endif
#else
#ifdef _MSC_VER
#define CRF_STRDUP _strdup
#else
char *c_rest_internal_strdup(const char *s);
#define CRF_STRDUP c_rest_internal_strdup
#endif
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the memory tracker.
 * @return C_REST_OK on success, or an error code on failure.
 */
C_REST_EXPORT extern c_rest_error_t c_rest_mem_tracker_init(void);

/**
 * @brief Allocate memory, optionally tracking the allocation.
 * @param size The number of bytes to allocate.
 * @param file The source file name where the allocation occurred.
 * @param line The line number where the allocation occurred.
 * @param out_ptr Pointer to where the allocated memory pointer will be stored.
 * @return C_REST_OK on success, or an error code on failure.
 */
C_REST_EXPORT extern c_rest_error_t
c_rest_mem_malloc(size_t size, const char *file, int line, void *out_ptr);

/**
 * @brief Allocate memory for an array, optionally tracking the allocation.
 * @param count The number of elements to allocate.
 * @param size The size of each element.
 * @param file The source file name where the allocation occurred.
 * @param line The line number where the allocation occurred.
 * @param out_ptr Pointer to where the allocated memory pointer will be stored.
 * @return C_REST_OK on success, or an error code on failure.
 */
C_REST_EXPORT extern c_rest_error_t c_rest_mem_calloc(size_t count, size_t size,
                                                      const char *file,
                                                      int line, void *out_ptr);

/**
 * @brief Reallocate memory, optionally tracking the allocation.
 * @param ptr A pointer to the previously allocated memory.
 * @param size The new size of the memory block.
 * @param file The source file name where the reallocation occurred.
 * @param line The line number where the reallocation occurred.
 * @param out_ptr Pointer to where the newly allocated memory pointer will be
 * stored.
 * @return C_REST_OK on success, or an error code on failure.
 */
C_REST_EXPORT extern c_rest_error_t c_rest_mem_realloc(void *ptr, size_t size,
                                                       const char *file,
                                                       int line, void *out_ptr);

/**
 * @brief Duplicate a string, optionally tracking the allocation.
 * @param str The string to duplicate.
 * @param file The source file name where the allocation occurred.
 * @param line The line number where the allocation occurred.
 * @param out_str Pointer to where the newly allocated string pointer will be
 * stored.
 * @return C_REST_OK on success, or an error code on failure.
 */
C_REST_EXPORT extern c_rest_error_t
c_rest_mem_strdup(const char *str, const char *file, int line, char **out_str);

/**
 * @brief Free allocated memory.
 * @param ptr A pointer to the memory to free.
 * @return C_REST_OK on success, or an error code on failure.
 */
C_REST_EXPORT extern c_rest_error_t c_rest_mem_free(void *ptr);

/**
 * @brief Print a report of memory leaks detected by the tracker.
 * @return C_REST_OK on success, or an error code on failure.
 */
C_REST_EXPORT extern c_rest_error_t c_rest_mem_tracker_print_leaks(void);

/**
 * @brief Clean up the memory tracker and release its resources.
 * @return C_REST_OK on success, or an error code on failure.
 */
C_REST_EXPORT extern c_rest_error_t c_rest_mem_tracker_cleanup(void);

#ifdef C_REST_MEM_TRACK
/** @brief Macro to allocate memory with tracking. */
#define C_REST_MALLOC(size, out_ptr)                                           \
  c_rest_mem_malloc(size, __FILE__, __LINE__, out_ptr)
/** @brief Macro to allocate zero-initialized memory with tracking. */
#define C_REST_CALLOC(count, size, out_ptr)                                    \
  c_rest_mem_calloc(count, size, __FILE__, __LINE__, out_ptr)
/** @brief Macro to reallocate memory with tracking. */
#define C_REST_REALLOC(ptr, size, out_ptr)                                     \
  c_rest_mem_realloc(ptr, size, __FILE__, __LINE__, out_ptr)
/** @brief Macro to free memory with tracking. */
#define C_REST_FREE(ptr) c_rest_mem_free(ptr)
#else
/** @brief Macro to allocate memory without tracking. */
#define C_REST_MALLOC(size, out_ptr)                                           \
  (((*((void **)(out_ptr))) = CRF_MALLOC(size)) == NULL)
/** @brief Macro to allocate zero-initialized memory without tracking. */
#define C_REST_CALLOC(count, size, out_ptr)                                    \
  (((*((void **)(out_ptr))) = CRF_CALLOC(count, size)) == NULL)
/** @brief Macro to reallocate memory without tracking. */
#define C_REST_REALLOC(ptr, size, out_ptr)                                     \
  (((*((void **)(out_ptr))) = CRF_REALLOC(ptr, size)) == NULL)
/** @brief Macro to free memory without tracking. */
#define C_REST_FREE(ptr) (CRF_FREE(ptr), 0)
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* C_REST_MEM_H */
