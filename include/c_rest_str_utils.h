/**
 * @file c_rest_str_utils.h
 * @brief Header file for c_rest_str_utils.h
 */
#ifndef C_REST_STR_UTILS_H
#define C_REST_STR_UTILS_H
/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_export.h"

#include <stddef.h>
/* clang-format on */

/**
 * @def C_REST_ATTR_PRINTF
 * @brief Format attribute macro for printf-style functions.
 * @param fmt_idx Index of format string parameter.
 * @param arg_idx Index of first vararg parameter.
 */
#if defined(__GNUC__) || defined(__clang__)
#define C_REST_ATTR_PRINTF(fmt_idx, arg_idx)                                   \
  __attribute__((__format__(__printf__, fmt_idx, arg_idx)))
#else
#define C_REST_ATTR_PRINTF(fmt_idx, arg_idx)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Safe sprintf.
 *
 * @param buffer Destination buffer.
 * @param sizeOfBuffer Size of destination buffer.
 * @param format Format string.
 * @return 0 on success, non-zero on error.
 */
C_REST_EXPORT extern c_rest_error_t
c_rest_sprintf_s(char *buffer, size_t sizeOfBuffer, const char *format, ...)
    C_REST_ATTR_PRINTF(3, 4);

/**
 * @brief Safe strcpy.
 *
 * @param dest Destination buffer.
 * @param dest_size Size of destination buffer.
 * @param src Source string.
 * @return 0 on success, non-zero on error.
 */
C_REST_EXPORT extern c_rest_error_t
c_rest_strcpy_s(char *dest, size_t dest_size, const char *src);

/**
 * @brief Safe strncpy.
 *
 * @param dest Destination buffer.
 * @param dest_size Size of destination buffer.
 * @param src Source string.
 * @param count Maximum characters to copy.
 * @return 0 on success, non-zero on error.
 */
C_REST_EXPORT extern c_rest_error_t
c_rest_strncpy_s(char *dest, size_t dest_size, const char *src, size_t count);

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
