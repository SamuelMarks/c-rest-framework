#ifndef C_REST_TESTING_MOCKS_H
#define C_REST_TESTING_MOCKS_H

/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_export.h"
#include "c_rest_platform.h"
#include <stddef.h>
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

/* Exported hooks for testing */
C_REST_EXPORT extern void *(*g_crf_malloc_hook)(size_t);
C_REST_EXPORT extern void *(*g_crf_calloc_hook)(size_t, size_t);
C_REST_EXPORT extern void *(*g_crf_realloc_hook)(void *, size_t);
C_REST_EXPORT extern char *(*g_crf_strdup_hook)(const char *);
C_REST_EXPORT extern c_rest_error_t (*g_crf_is_little_endian_hook)(int *);

C_REST_EXPORT extern int g_mock_lib_fail;
C_REST_EXPORT extern int g_mock_sse_append_fail;
C_REST_EXPORT extern int g_mock_orm_init_fail;
C_REST_EXPORT extern int g_mock_socket_fail;
C_REST_EXPORT extern int g_mock_tls_fail;
C_REST_EXPORT extern int g_mock_orm_cleanup_fail;
C_REST_EXPORT extern int g_mock_crypto_fail;
C_REST_EXPORT extern int g_mock_cm_thread_fail;
C_REST_EXPORT extern int g_mock_cm_join_fail;
C_REST_EXPORT extern int g_mock_cm_file_fail;

C_REST_EXPORT extern c_rest_mutex_t *g_crf_mem_mutex_ptr;
C_REST_EXPORT extern int *g_crf_mem_initialized_ptr;

/* Exported modality vtables for testing */
C_REST_EXPORT extern const struct c_rest_modality_vtable sync_vtable;
C_REST_EXPORT extern const struct c_rest_modality_vtable single_thread_vtable;
C_REST_EXPORT extern const struct c_rest_modality_vtable multi_thread_vtable;
C_REST_EXPORT extern const struct c_rest_modality_vtable async_vtable;
C_REST_EXPORT extern const struct c_rest_modality_vtable greenthread_vtable;
C_REST_EXPORT extern const struct c_rest_modality_vtable message_passing_vtable;
C_REST_EXPORT extern const struct c_rest_modality_vtable multi_process_vtable;

#ifdef __cplusplus
}
#endif

#endif
