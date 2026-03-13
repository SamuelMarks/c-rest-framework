#ifndef C_REST_BASE64_H
#define C_REST_BASE64_H

/* clang-format off */
#include <stddef.h>
/* clang-format on */
#ifdef __cplusplus
extern "C" {
#endif

int c_rest_base64_encode(const unsigned char *src, size_t src_len, char *dst,
                         size_t *dst_len);
int c_rest_base64_decode(const char *src, size_t src_len, unsigned char *dst,
                         size_t *dst_len);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* C_REST_BASE64_H */
