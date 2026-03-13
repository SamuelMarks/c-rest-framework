#ifndef C_REST_CRYPTO_H
#define C_REST_CRYPTO_H

/* clang-format off */
#include <stddef.h>
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Computes the SHA1 hash of the given data.
 * @param data The input data buffer.
 * @param len The length of the input data.
 * @param hash The 20-byte array to store the resulting SHA1 hash.
 * @return 0 on success, non-zero on failure.
 */
int c_rest_sha1(const unsigned char *data, size_t len, unsigned char hash[20]);

/**
 * @brief Computes the SHA256 hash of the given data.
 * @param data The input data buffer.
 * @param len The length of the input data.
 * @param hash The 32-byte array to store the resulting SHA256 hash.
 * @return 0 on success, non-zero on failure.
 */
int c_rest_sha256(const unsigned char *data, size_t len,
                  unsigned char hash[32]);

/**
 * @brief Generates cryptographically secure random bytes.
 * @param buf The buffer to store the random bytes.
 * @param len The number of random bytes to generate.
 * @return 0 on success, non-zero on failure.
 */
int c_rest_rand_bytes(unsigned char *buf, size_t len);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* C_REST_CRYPTO_H */
