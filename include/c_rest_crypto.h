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

/**
 * @brief Computes the HMAC-SHA256 of the given data.
 * @param key The secret key.
 * @param key_len The length of the secret key.
 * @param data The input data buffer.
 * @param data_len The length of the input data.
 * @param hash The 32-byte array to store the resulting HMAC.
 * @return 0 on success, non-zero on failure.
 */
int c_rest_hmac_sha256(const unsigned char *key, size_t key_len,
                       const unsigned char *data, size_t data_len,
                       unsigned char hash[32]);

/**
 * @brief Simple JWT HS256 sign utility.
 * @param json_payload The payload as a JSON string.
 * @param secret The shared secret.
 * @param secret_len Length of the shared secret.
 * @param out_token Pointer to store the newly allocated JWT token string.
 * @return 0 on success, non-zero on failure.
 */
int c_rest_jwt_sign_hs256(const char *json_payload, const unsigned char *secret,
                          size_t secret_len, char **out_token);

/**
 * @brief Simple JWT HS256 verify utility.
 * @param token The JWT string.
 * @param secret The shared secret.
 * @param secret_len Length of the shared secret.
 * @param out_payload Pointer to store the newly allocated JSON payload if
 * valid.
 * @return 0 on success (valid signature), non-zero on failure or invalid
 * signature.
 */
int c_rest_jwt_verify_hs256(const char *token, const unsigned char *secret,
                            size_t secret_len, char **out_payload);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* C_REST_CRYPTO_H */
