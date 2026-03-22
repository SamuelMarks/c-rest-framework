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
 * @brief Derives a key from a password using PBKDF2 with HMAC-SHA256.
 * @param password The password buffer.
 * @param password_len The length of the password.
 * @param salt The salt buffer.
 * @param salt_len The length of the salt.
 * @param iterations The number of iterations.
 * @param dk_len The desired length of the derived key.
 * @param out_key The buffer to store the derived key (must be at least dk_len
 * bytes).
 * @return 0 on success, non-zero on failure.
 */
int c_rest_pbkdf2_hmac_sha256(const unsigned char *password,
                              size_t password_len, const unsigned char *salt,
                              size_t salt_len, unsigned long iterations,
                              size_t dk_len, unsigned char *out_key);

/**
 * @brief Generates an opaque, URL-safe random string.
 * @param entropy_bytes The number of random bytes to generate before encoding.
 * @param out_str Pointer to store the newly allocated base64url-encoded string.
 * @return 0 on success, non-zero on failure.
 */
int c_rest_random_string_generate(size_t entropy_bytes, char **out_str);

/**
 * @brief Generates a guaranteed unique, URL-safe Base64 token natively aligned
 * with RFC6749.
 * @param out_token Pointer to store the newly allocated access token string.
 * @return 0 on success, non-zero on failure.
 */
int c_rest_oauth2_generate_access_token(char **out_token);

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

/**
 * @brief Password hashing algorithms.
 */
enum c_rest_password_hash_alg {
  C_REST_HASH_ALG_PBKDF2_SHA256,
  C_REST_HASH_ALG_BCRYPT,
  C_REST_HASH_ALG_ARGON2,
  C_REST_HASH_ALG_SCRYPT
};

/**
 * @brief Hashes a password using the specified algorithm.
 * @param password The plaintext password.
 * @param alg The algorithm to use.
 * @param out_hash Pointer to store the newly allocated hash string (MCF
 * format).
 * @return 0 on success, non-zero on failure.
 */
int c_rest_hash_password(const char *password,
                         enum c_rest_password_hash_alg alg, char **out_hash);

/**
 * @brief Verifies a password against a hash string.
 * @param password The plaintext password.
 * @param hash The MCF format hash string to verify against.
 * @return 0 on success (match), non-zero on failure (no match or error).
 */
int c_rest_verify_password(const char *password, const char *hash);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* C_REST_CRYPTO_H */
