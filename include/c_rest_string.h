#ifndef C_REST_STRING_H
#define C_REST_STRING_H

/* clang-format off */
#include <stddef.h>
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Dynamic string */
typedef struct c_rest_string {
  /** @brief Data payload */
  char *data;
  /** @brief Current length */
  size_t length;
  /** @brief Total capacity */
  size_t capacity;
} c_rest_string;

/**
 * @brief Initializes a dynamic string.
 *
 * @param str The string to initialize.
 * @param initial_capacity The initial capacity to allocate.
 * @return 0 on success, non-zero on error.
 */
int c_rest_string_init(c_rest_string *str, size_t initial_capacity);

/**
 * @brief Appends raw data to a dynamic string.
 *
 * @param str The dynamic string.
 * @param data The data to append.
 * @param len The length of the data to append.
 * @return 0 on success, non-zero on error.
 */
int c_rest_string_append(c_rest_string *str, const char *data, size_t len);

/**
 * @brief Appends a null-terminated C string to a dynamic string.
 *
 * @param str The dynamic string.
 * @param cstr The null-terminated C string to append.
 * @return 0 on success, non-zero on error.
 */
int c_rest_string_append_cstr(c_rest_string *str, const char *cstr);

/**
 * @brief Destroys a dynamic string, freeing its memory.
 *
 * @param str The dynamic string to destroy.
 * @return 0 on success, non-zero on error.
 */
int c_rest_string_destroy(c_rest_string *str);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* C_REST_STRING_H */
