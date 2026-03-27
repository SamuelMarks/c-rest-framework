#ifndef C_REST_STR_UTILS_H
#define C_REST_STR_UTILS_H

/* clang-format off */
#include <stddef.h>
/* clang-format on */
#ifdef __cplusplus
extern "C" {
#endif

int c_rest_strcasecmp(const char *s1, const char *s2);
int c_rest_strncasecmp(const char *s1, const char *s2, size_t n);

int c_rest_strlcpy(char *dst, const char *src, size_t dsize, size_t *out_len);
int c_rest_strlcat(char *dst, const char *src, size_t dsize, size_t *out_len);

/**
 * @brief Decode URL encoded string.
 * @param dst Destination buffer
 * @param src Source string
 * @param len Length of source
 * @return 0 on success
 */
int c_rest_url_decode(char *dst, const char *src, size_t len);

#if defined(__cplusplus)
}
#endif /* __cplusplus */
#endif /* C_REST_STR_UTILS_H */
