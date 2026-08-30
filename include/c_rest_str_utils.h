/**
 * @file c_rest_str_utils.h
 * @brief Header file for c_rest_str_utils.h
 */
#ifndef C_REST_STR_UTILS_H
#define C_REST_STR_UTILS_H
/* clang-format off */
#include "c_rest_error.h"

#include <stddef.h>
#include "c_rest_error.h"
#include "c_rest_export.h"
/* clang-format on */

/**
 * @brief Safe sprintf.
 */
C_REST_EXPORT extern c_rest_error_t
c_rest_sprintf_s(char *buffer, size_t sizeOfBuffer, const char *format, ...);

/**
 * @brief Safe strcpy.
 */
C_REST_EXPORT extern c_rest_error_t
c_rest_strcpy_s(char *dest, size_t dest_size, const char *src);

/**
 * @brief Safe strncpy.
 */
C_REST_EXPORT extern c_rest_error_t
c_rest_strncpy_s(char *dest, size_t dest_size, const char *src, size_t count);

#ifdef __cplusplus

extern "C" {
#endif

/**
 * @brief Compare two strings ignoring case.
 *
 * @param s1 First string.
 * @param s2 Second string.
 * @param out_cmp Pointer to hold the comparison result.
 * @return 0 on success, non-zero on error.
 */
C_REST_EXPORT extern c_rest_error_t
c_rest_strcasecmp(const char *s1, const char *s2, int *out_cmp);

/**
 * @brief Compare two strings ignoring case, up to a maximum length.
 *
 * @param s1 First string.
 * @param s2 Second string.
 * @param n Maximum number of characters to compare.
 * @param out_cmp Pointer to hold the comparison result.
 * @return 0 on success, non-zero on error.
 */
C_REST_EXPORT extern c_rest_error_t
c_rest_strncasecmp(const char *s1, const char *s2, size_t n, int *out_cmp);

/**
 * @brief Copy a string to a destination buffer safely.
 *
 * @param dst Destination buffer.
 * @param src Source string.
 * @param dsize Size of the destination buffer.
 * @param out_len Pointer to hold the length of the string it tried to create.
 * @return 0 on success, non-zero on error.
 */
C_REST_EXPORT extern c_rest_error_t
c_rest_strlcpy(char *dst, const char *src, size_t dsize, size_t *out_len);

/**
 * @brief Append a string to a destination buffer safely.
 *
 * @param dst Destination buffer.
 * @param src Source string.
 * @param dsize Size of the destination buffer.
 * @param out_len Pointer to hold the length of the string it tried to create.
 * @return 0 on success, non-zero on error.
 */
C_REST_EXPORT extern c_rest_error_t
c_rest_strlcat(char *dst, const char *src, size_t dsize, size_t *out_len);

/**
 * @brief Decode URL encoded string.
 *
 * @param dst Destination buffer.
 * @param src Source string.
 * @param len Length of source.
 * @return 0 on success, non-zero on error.
 */
C_REST_EXPORT extern c_rest_error_t
c_rest_url_decode(char *dst, const char *src, size_t len);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* C_REST_STR_UTILS_H */
