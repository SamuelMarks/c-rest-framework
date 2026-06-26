#ifndef C_REST_BASE64_H
#define C_REST_BASE64_H

/* clang-format off */
#include <stddef.h>
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

enum c_rest_base64_error {
  C_REST_BASE64_SUCCESS = 0,
  C_REST_BASE64_ERROR_NULL_POINTER = 1,
  C_REST_BASE64_ERROR_BUFFER_TOO_SMALL = 2,
  C_REST_BASE64_ERROR_INVALID_INPUT = 3
};

/**
 * @brief Base64 encode data.
 * @param src Input buffer.
 * @param src_len Input length.
 * @param dst Output buffer.
 * @param dst_len Output buffer size / returned size.
 * @return C_REST_BASE64_SUCCESS on success.
 */
enum c_rest_base64_error c_rest_base64_encode(const unsigned char *src,
                                              size_t src_len, char *dst,
                                              size_t *dst_len);

/**
 * @brief Base64 decode string.
 * @param src Input string.
 * @param src_len Input string length.
 * @param dst Output buffer.
 * @param dst_len Output buffer size / returned size.
 * @return C_REST_BASE64_SUCCESS on success.
 */
enum c_rest_base64_error c_rest_base64_decode(const char *src, size_t src_len,
                                              unsigned char *dst,
                                              size_t *dst_len);

/**
 * @brief Base64url encode data (RFC 4648).
 * @param src Input buffer.
 * @param src_len Input length.
 * @param dst Output buffer.
 * @param dst_len Output buffer size / returned size.
 * @return C_REST_BASE64_SUCCESS on success.
 */
enum c_rest_base64_error c_rest_base64url_encode(const unsigned char *src,
                                                 size_t src_len, char *dst,
                                                 size_t *dst_len);

/**
 * @brief Base64url decode string (RFC 4648).
 * @param src Input string.
 * @param src_len Input string length.
 * @param dst Output buffer.
 * @param dst_len Output buffer size / returned size.
 * @return C_REST_BASE64_SUCCESS on success.
 */
enum c_rest_base64_error c_rest_base64url_decode(const char *src,
                                                 size_t src_len,
                                                 unsigned char *dst,
                                                 size_t *dst_len);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* C_REST_BASE64_H */
