#ifndef C_REST_TLS_H
#define C_REST_TLS_H

/* clang-format off */
#include <stddef.h>
#include "c_rest_platform.h"
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Enum defining the available cryptographic providers.
 */
enum c_rest_crypto_provider {
  C_REST_CRYPTO_NONE = 0,
  C_REST_CRYPTO_OPENSSL,
  C_REST_CRYPTO_LIBRESSL,
  C_REST_CRYPTO_BORINGSSL,
  C_REST_CRYPTO_MBEDTLS,
  C_REST_CRYPTO_WOLFSSL,
  C_REST_CRYPTO_S2N,
  C_REST_CRYPTO_BEARSSL,
  C_REST_CRYPTO_SCHANNEL,
  C_REST_CRYPTO_GNUTLS,
  C_REST_CRYPTO_BOTAN,
  C_REST_CRYPTO_COMMONCRYPTO,
  C_REST_CRYPTO_WINCRYPT
};

/**
 * @brief Initialize the global TLS library state.
 * @return 0 on success, non-zero on failure.
 */
int c_rest_tls_init(void);

/**
 * @brief Get the currently active cryptographic provider.
 * @return The active c_rest_crypto_provider.
 */
int c_rest_tls_get_provider(enum c_rest_crypto_provider *out_provider);

/**
 * @brief Opaque structure representing the server's global TLS/SSL context.
 */
struct c_rest_tls_context;

/**
 * @brief Opaque structure representing an active TLS connection.
 */
struct c_rest_tls_connection;

/**
 * @brief Allocate and initialize a new TLS context.
 * @param out_ctx Pointer to the resulting context pointer.
 * @return 0 on success, non-zero on failure.
 */
int c_rest_tls_context_init(struct c_rest_tls_context **out_ctx);

/**
 * @brief Destroy and clean up a TLS context.
 * @param ctx The context to destroy.
 */
int c_rest_tls_context_destroy(struct c_rest_tls_context *ctx);

/**
 * @brief Load a server certificate into the context.
 * @param ctx The TLS context.
 * @param cert_path Path to the certificate file (PEM/DER).
 * @return 0 on success, non-zero on failure.
 */
int c_rest_tls_load_cert(struct c_rest_tls_context *ctx, const char *cert_path);

/**
 * @brief Load a server private key into the context.
 * @param ctx The TLS context.
 * @param key_path Path to the private key file (PEM/DER).
 * @return 0 on success, non-zero on failure.
 */
int c_rest_tls_load_key(struct c_rest_tls_context *ctx, const char *key_path);

/**
 * @brief Load a Certificate Authority (CA) chain into the context.
 * @param ctx The TLS context.
 * @param ca_chain_path Path to the CA chain file.
 * @return 0 on success, non-zero on failure.
 */
int c_rest_tls_load_ca_chain(struct c_rest_tls_context *ctx,
                             const char *ca_chain_path);

/**
 * @brief Set the ALPN protocols for the context.
 * @param ctx The TLS context.
 * @param protocols A comma-separated list of protocols (e.g., "h2,http/1.1").
 * @return 0 on success, non-zero on failure.
 */
int c_rest_tls_set_alpn(struct c_rest_tls_context *ctx, const char *protocols);

#define C_REST_TLS_WANT_READ -2
#define C_REST_TLS_WANT_WRITE -3

/**
 * @brief Wrap an accepted socket with a TLS connection instance.
 * @param ctx The global TLS context.
 * @param sock The raw network socket.
 * @param out_conn Pointer to the resulting TLS connection.
 * @return 0 on success, C_REST_TLS_WANT_READ, C_REST_TLS_WANT_WRITE, or error.
 */
int c_rest_tls_accept(struct c_rest_tls_context *ctx, c_rest_socket_t sock,
                      struct c_rest_tls_connection **out_conn);

/**
 * @brief Read data from a TLS connection.
 * @param conn The TLS connection.
 * @param buf The buffer to read into.
 * @param len The maximum number of bytes to read.
 * @param out_read Pointer to store the number of bytes actually read.
 * @return 0 on success, C_REST_TLS_WANT_READ, C_REST_TLS_WANT_WRITE, or error.
 */
int c_rest_tls_read(struct c_rest_tls_connection *conn, void *buf, size_t len,
                    size_t *out_read);

/**
 * @brief Write data to a TLS connection.
 * @param conn The TLS connection.
 * @param buf The buffer containing data to write.
 * @param len The number of bytes to write.
 * @param out_written Pointer to store the number of bytes actually written.
 * @return 0 on success, C_REST_TLS_WANT_READ, C_REST_TLS_WANT_WRITE, or error.
 */
int c_rest_tls_write(struct c_rest_tls_connection *conn, const void *buf,
                     size_t len, size_t *out_written);

/**
 * @brief Close and clean up a TLS connection.
 * @param conn The TLS connection to close.
 * @return 0 on success, non-zero on failure.
 */
int c_rest_tls_close(struct c_rest_tls_connection *conn);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* C_REST_TLS_H */
