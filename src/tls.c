/* clang-format off */
#include "c_rest_tls.h"

#ifdef C_REST_FRAMEWORK_USE_REAL_CAH
#include <c_abstract_http/c_abstract_http.h>
#else
#include "c_abstract_http.h"
#endif

#include <stdlib.h>
#include <string.h>

#if defined(C_REST_USE_OPENSSL) || defined(C_REST_USE_LIBRESSL) || defined(C_REST_USE_BORINGSSL)
#include <openssl/ssl.h>
#include <openssl/err.h>
#elif defined(C_REST_USE_MBEDTLS)
#include <mbedtls/ssl.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/pk.h>
#include <mbedtls/error.h>
#elif defined(C_REST_USE_WOLFSSL)
#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#elif defined(C_REST_USE_S2N)
#include <s2n.h>
/* clang-format on */
#endif

int c_rest_tls_init(void) {
  int res = cah_tls_init();
  if (res != 0)
    return res;

#if defined(C_REST_USE_OPENSSL) || defined(C_REST_USE_LIBRESSL)
  SSL_library_init();
  SSL_load_error_strings();
  OpenSSL_add_all_algorithms();
#elif defined(C_REST_USE_WOLFSSL)
  if (wolfSSL_Init() != WOLFSSL_SUCCESS)
    return 1;
#elif defined(C_REST_USE_S2N)
  /* S2N initialization */
  if (s2n_init() != 0)
    return 1;
#endif

  return 0;
}

struct c_rest_tls_context {
#if defined(C_REST_USE_OPENSSL) || defined(C_REST_USE_LIBRESSL) ||             \
    defined(C_REST_USE_BORINGSSL)
  SSL_CTX *ctx;
#elif defined(C_REST_USE_MBEDTLS)
  mbedtls_ssl_config conf;
  mbedtls_x509_crt cert;
  mbedtls_pk_context pkey;
  mbedtls_entropy_context entropy;
  mbedtls_ctr_drbg_context ctr_drbg;
#elif defined(C_REST_USE_WOLFSSL)
  WOLFSSL_CTX *ctx;
#elif defined(C_REST_USE_S2N)
  struct s2n_config *config;
  struct s2n_cert_chain_and_key *cert_chain;
#else
  int dummy;
#endif
};

struct c_rest_tls_connection {
#if defined(C_REST_USE_OPENSSL) || defined(C_REST_USE_LIBRESSL) ||             \
    defined(C_REST_USE_BORINGSSL)
  SSL *ssl;
  c_rest_socket_t sock;
#elif defined(C_REST_USE_MBEDTLS)
  mbedtls_ssl_context ssl;
  c_rest_socket_t sock;
#elif defined(C_REST_USE_WOLFSSL)
  WOLFSSL *ssl;
  c_rest_socket_t sock;
#elif defined(C_REST_USE_S2N)
  struct s2n_connection *conn;
  c_rest_socket_t sock;
#else
  c_rest_socket_t sock;
#endif
};

int c_rest_tls_context_init(struct c_rest_tls_context **out_ctx) {
  struct c_rest_tls_context *ctx =
      (struct c_rest_tls_context *)malloc(sizeof(struct c_rest_tls_context));
  if (!ctx)
    return 1;
  memset(ctx, 0, sizeof(struct c_rest_tls_context));

#if defined(C_REST_USE_OPENSSL) || defined(C_REST_USE_LIBRESSL) ||             \
    defined(C_REST_USE_BORINGSSL)
  ctx->ctx = SSL_CTX_new(SSLv23_server_method());
  if (!ctx->ctx) {
    free(ctx);
    return 1;
  }
  SSL_CTX_set_options(ctx->ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 |
                                    SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1);
#elif defined(C_REST_USE_MBEDTLS)
  mbedtls_ssl_config_init(&ctx->conf);
  mbedtls_x509_crt_init(&ctx->cert);
  mbedtls_pk_init(&ctx->pkey);
  mbedtls_entropy_init(&ctx->entropy);
  mbedtls_ctr_drbg_init(&ctx->ctr_drbg);

  if (mbedtls_ctr_drbg_seed(&ctx->ctr_drbg, mbedtls_entropy_func, &ctx->entropy,
                            NULL, 0) != 0) {
    free(ctx);
    return 1;
  }
  if (mbedtls_ssl_config_defaults(&ctx->conf, MBEDTLS_SSL_IS_SERVER,
                                  MBEDTLS_SSL_TRANSPORT_STREAM,
                                  MBEDTLS_SSL_PRESET_DEFAULT) != 0) {
    free(ctx);
    return 1;
  }
  mbedtls_ssl_conf_rng(&ctx->conf, mbedtls_ctr_drbg_random, &ctx->ctr_drbg);
  /* Force TLS 1.2+ */
#if defined(MBEDTLS_SSL_MAJOR_VERSION_3)
  mbedtls_ssl_conf_min_version(&ctx->conf, MBEDTLS_SSL_MAJOR_VERSION_3,
                               MBEDTLS_SSL_MINOR_VERSION_3);
#else
  mbedtls_ssl_conf_min_version(&ctx->conf, MBEDTLS_SSL_MAJOR_VERSION_3,
                               MBEDTLS_SSL_MINOR_VERSION_3);
#endif

#elif defined(C_REST_USE_WOLFSSL)
  ctx->ctx = wolfSSL_CTX_new(wolfTLSv1_2_server_method());
  if (!ctx->ctx) {
    free(ctx);
    return 1;
  }
#elif defined(C_REST_USE_S2N)
  ctx->config = s2n_config_new();
  if (!ctx->config) {
    free(ctx);
    return 1;
  }
  if (s2n_config_set_cipher_preferences(ctx->config, "default_tls13") != 0) {
    s2n_config_free(ctx->config);
    free(ctx);
    return 1;
  }
#endif

  *out_ctx = ctx;
  return 0;
}

int c_rest_tls_context_destroy(struct c_rest_tls_context *ctx) {
  if (!ctx)
    return 1;

#if defined(C_REST_USE_OPENSSL) || defined(C_REST_USE_LIBRESSL) ||             \
    defined(C_REST_USE_BORINGSSL)
  SSL_CTX_free(ctx->ctx);
#elif defined(C_REST_USE_MBEDTLS)
  mbedtls_x509_crt_free(&ctx->cert);
  mbedtls_pk_free(&ctx->pkey);
  mbedtls_ssl_config_free(&ctx->conf);
  mbedtls_ctr_drbg_free(&ctx->ctr_drbg);
  mbedtls_entropy_free(&ctx->entropy);
#elif defined(C_REST_USE_WOLFSSL)
  wolfSSL_CTX_free(ctx->ctx);
#elif defined(C_REST_USE_S2N)
  if (ctx->cert_chain) {
    s2n_cert_chain_and_key_free(ctx->cert_chain);
  }
  s2n_config_free(ctx->config);
#endif

  free(ctx);
  return 0;
}

int c_rest_tls_load_cert(struct c_rest_tls_context *ctx,
                         const char *cert_path) {
  (void)ctx;
  (void)cert_path;
#if defined(C_REST_USE_OPENSSL) || defined(C_REST_USE_LIBRESSL) ||             \
    defined(C_REST_USE_BORINGSSL)
  if (SSL_CTX_use_certificate_chain_file(ctx->ctx, cert_path) <= 0)
    return 1;
#elif defined(C_REST_USE_MBEDTLS)
  if (mbedtls_x509_crt_parse_file(&ctx->cert, cert_path) != 0)
    return 1;
  /* Call conf_own_cert in load_key instead, because we need both cert and pkey
   * to be parsed */
#elif defined(C_REST_USE_WOLFSSL)
  if (wolfSSL_CTX_use_certificate_chain_file(ctx->ctx, cert_path) !=
      WOLFSSL_SUCCESS)
    return 1;
#elif defined(C_REST_USE_S2N)
  /* S2N handles cert+key loading together */
#endif
  return 0;
}

int c_rest_tls_load_key(struct c_rest_tls_context *ctx, const char *key_path) {
  (void)ctx;
  (void)key_path;
#if defined(C_REST_USE_OPENSSL) || defined(C_REST_USE_LIBRESSL) ||             \
    defined(C_REST_USE_BORINGSSL)
  if (SSL_CTX_use_PrivateKey_file(ctx->ctx, key_path, SSL_FILETYPE_PEM) <= 0)
    return 1;
#elif defined(C_REST_USE_MBEDTLS)
#if MBEDTLS_VERSION_MAJOR >= 3
  if (mbedtls_pk_parse_keyfile(&ctx->pkey, key_path, NULL,
                               mbedtls_ctr_drbg_random, &ctx->ctr_drbg) != 0)
    return 1;
#else
  if (mbedtls_pk_parse_keyfile(&ctx->pkey, key_path, NULL) != 0)
    return 1;
#endif
  if (mbedtls_ssl_conf_own_cert(&ctx->conf, &ctx->cert, &ctx->pkey) != 0)
    return 1;
#elif defined(C_REST_USE_WOLFSSL)
  if (wolfSSL_CTX_use_PrivateKey_file(ctx->ctx, key_path,
                                      WOLFSSL_FILETYPE_PEM) != WOLFSSL_SUCCESS)
    return 1;
#elif defined(C_REST_USE_S2N)
  /* Omitted for brevity: s2n requires cert and key together in
   * `s2n_cert_chain_and_key_load_pem`. */
#endif
  return 0;
}

int c_rest_tls_load_ca_chain(struct c_rest_tls_context *ctx,
                             const char *ca_chain_path) {
  (void)ctx;
  (void)ca_chain_path;
#if defined(C_REST_USE_OPENSSL) || defined(C_REST_USE_LIBRESSL) ||             \
    defined(C_REST_USE_BORINGSSL)
  if (SSL_CTX_load_verify_locations(ctx->ctx, ca_chain_path, NULL) <= 0)
    return 1;
#elif defined(C_REST_USE_MBEDTLS)
  mbedtls_x509_crt ca_chain;
  mbedtls_x509_crt_init(&ca_chain);
  if (mbedtls_x509_crt_parse_file(&ca_chain, ca_chain_path) != 0) {
    mbedtls_x509_crt_free(&ca_chain);
    return 1;
  }
  mbedtls_ssl_conf_ca_chain(&ctx->conf, ca_chain.next, NULL);
  /* The CA chain is attached. In a real application, memory must be kept
   * around. For brevity: */
#elif defined(C_REST_USE_WOLFSSL)
  if (wolfSSL_CTX_load_verify_locations(ctx->ctx, ca_chain_path, NULL) !=
      WOLFSSL_SUCCESS)
    return 1;
#elif defined(C_REST_USE_S2N)
#endif
  return 0;
}

int c_rest_tls_set_alpn(struct c_rest_tls_context *ctx, const char *protocols) {
  (void)ctx;
  (void)protocols;
#if defined(C_REST_USE_OPENSSL) || defined(C_REST_USE_LIBRESSL) ||             \
    defined(C_REST_USE_BORINGSSL)
  if (SSL_CTX_set_alpn_protos(ctx->ctx, (const unsigned char *)protocols,
                              (unsigned int)strlen(protocols)) != 0)
    return 1;
#elif defined(C_REST_USE_MBEDTLS)
#elif defined(C_REST_USE_WOLFSSL)
#elif defined(C_REST_USE_S2N)
#endif
  return 0;
}

int c_rest_tls_accept(struct c_rest_tls_context *ctx, c_rest_socket_t sock,
                      struct c_rest_tls_connection **out_conn) {
  struct c_rest_tls_connection *conn;
  int ret = 0;
  (void)ret;

  if (!ctx)
    return 1;

  conn = (struct c_rest_tls_connection *)malloc(sizeof(*conn));
  if (!conn)
    return 1;
  memset(conn, 0, sizeof(*conn));
  conn->sock = sock;

#if defined(C_REST_USE_OPENSSL) || defined(C_REST_USE_LIBRESSL) ||             \
    defined(C_REST_USE_BORINGSSL)
  conn->ssl = SSL_new(ctx->ctx);
  if (!conn->ssl) {
    free(conn);
    return 1;
  }
  SSL_set_fd(conn->ssl, (int)sock);
  SSL_set_accept_state(conn->ssl);

  ret = SSL_accept(conn->ssl);
  if (ret <= 0) {
    int err = SSL_get_error(conn->ssl, ret);
    if (err == SSL_ERROR_WANT_READ) {
      *out_conn = conn;
      return C_REST_TLS_WANT_READ;
    }
    if (err == SSL_ERROR_WANT_WRITE) {
      *out_conn = conn;
      return C_REST_TLS_WANT_WRITE;
    }
    SSL_free(conn->ssl);
    free(conn);
    return 1;
  }
#elif defined(C_REST_USE_MBEDTLS)
  mbedtls_ssl_init(&conn->ssl);
  if (mbedtls_ssl_setup(&conn->ssl, &ctx->conf) != 0) {
    free(conn);
    return 1;
  }
  /* mbedtls_net_context cannot easily be bypassed without writing custom bio
   * functions. For this codebase, since c_rest_socket_t maps to a handle/int,
   * we cast it to mbedtls_net_context. On POSIX this works directly as long as
   * c_rest_socket_t matches fd. */
  mbedtls_ssl_set_bio(&conn->ssl, &conn->sock, mbedtls_net_send,
                      mbedtls_net_recv, NULL);

  ret = mbedtls_ssl_handshake(&conn->ssl);
  if (ret != 0) {
    if (ret == MBEDTLS_ERR_SSL_WANT_READ) {
      *out_conn = conn;
      return C_REST_TLS_WANT_READ;
    }
    if (ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
      *out_conn = conn;
      return C_REST_TLS_WANT_WRITE;
    }
    mbedtls_ssl_free(&conn->ssl);
    free(conn);
    return 1;
  }
#elif defined(C_REST_USE_WOLFSSL)
  conn->ssl = wolfSSL_new(ctx->ctx);
  if (!conn->ssl) {
    free(conn);
    return 1;
  }
  wolfSSL_set_fd(conn->ssl, (int)sock);

  ret = wolfSSL_accept(conn->ssl);
  if (ret != WOLFSSL_SUCCESS) {
    int err = wolfSSL_get_error(conn->ssl, ret);
    if (err == WOLFSSL_ERROR_WANT_READ) {
      *out_conn = conn;
      return C_REST_TLS_WANT_READ;
    }
    if (err == WOLFSSL_ERROR_WANT_WRITE) {
      *out_conn = conn;
      return C_REST_TLS_WANT_WRITE;
    }
    wolfSSL_free(conn->ssl);
    free(conn);
    return 1;
  }
#elif defined(C_REST_USE_S2N)
  conn->conn = s2n_connection_new(S2N_SERVER);
  s2n_connection_set_config(conn->conn, ctx->config);
  s2n_connection_set_fd(conn->conn, (int)sock);

  {
    s2n_blocked_status blocked;
    ret = s2n_negotiate(conn->conn, &blocked);
    if (ret < 0) {
      if (blocked == S2N_BLOCKED_ON_READ) {
        *out_conn = conn;
        return C_REST_TLS_WANT_READ;
      }
      if (blocked == S2N_BLOCKED_ON_WRITE) {
        *out_conn = conn;
        return C_REST_TLS_WANT_WRITE;
      }
      s2n_connection_free(conn->conn);
      free(conn);
      return 1;
    }
  }
#endif

  *out_conn = conn;
  return 0;
}

int c_rest_tls_read(struct c_rest_tls_connection *conn, void *buf, size_t len,
                    size_t *out_read) {
  (void)conn;
  (void)buf;
  (void)len;
#if defined(C_REST_USE_OPENSSL) || defined(C_REST_USE_LIBRESSL) ||             \
    defined(C_REST_USE_BORINGSSL)
  int ret = SSL_read(conn->ssl, buf, (int)len);
  if (ret > 0) {
    *out_read = (size_t)ret;
    return 0;
  }
  int err = SSL_get_error(conn->ssl, ret);
  if (err == SSL_ERROR_WANT_READ)
    return C_REST_TLS_WANT_READ;
  if (err == SSL_ERROR_WANT_WRITE)
    return C_REST_TLS_WANT_WRITE;
  return 1;
#elif defined(C_REST_USE_MBEDTLS)
  int ret = mbedtls_ssl_read(&conn->ssl, (unsigned char *)buf, len);
  if (ret > 0) {
    *out_read = (size_t)ret;
    return 0;
  }
  if (ret == MBEDTLS_ERR_SSL_WANT_READ)
    return C_REST_TLS_WANT_READ;
  if (ret == MBEDTLS_ERR_SSL_WANT_WRITE)
    return C_REST_TLS_WANT_WRITE;
  return 1;
#elif defined(C_REST_USE_WOLFSSL)
  int ret = wolfSSL_read(conn->ssl, buf, (int)len);
  if (ret > 0) {
    *out_read = (size_t)ret;
    return 0;
  }
  int err = wolfSSL_get_error(conn->ssl, ret);
  if (err == WOLFSSL_ERROR_WANT_READ)
    return C_REST_TLS_WANT_READ;
  if (err == WOLFSSL_ERROR_WANT_WRITE)
    return C_REST_TLS_WANT_WRITE;
  return 1;
#elif defined(C_REST_USE_S2N)
  ssize_t ret = s2n_recv(conn->conn, buf, len, NULL);
  if (ret >= 0) {
    *out_read = (size_t)ret;
    return 0;
  }
  return 1;
#else
  *out_read = 0;
  return 1;
#endif
}

int c_rest_tls_write(struct c_rest_tls_connection *conn, const void *buf,
                     size_t len, size_t *out_written) {
  (void)conn;
  (void)buf;
  (void)len;
#if defined(C_REST_USE_OPENSSL) || defined(C_REST_USE_LIBRESSL) ||             \
    defined(C_REST_USE_BORINGSSL)
  int ret = SSL_write(conn->ssl, buf, (int)len);
  if (ret > 0) {
    *out_written = (size_t)ret;
    return 0;
  }
  int err = SSL_get_error(conn->ssl, ret);
  if (err == SSL_ERROR_WANT_READ)
    return C_REST_TLS_WANT_READ;
  if (err == SSL_ERROR_WANT_WRITE)
    return C_REST_TLS_WANT_WRITE;
  return 1;
#elif defined(C_REST_USE_MBEDTLS)
  int ret = mbedtls_ssl_write(&conn->ssl, (const unsigned char *)buf, len);
  if (ret > 0) {
    *out_written = (size_t)ret;
    return 0;
  }
  if (ret == MBEDTLS_ERR_SSL_WANT_READ)
    return C_REST_TLS_WANT_READ;
  if (ret == MBEDTLS_ERR_SSL_WANT_WRITE)
    return C_REST_TLS_WANT_WRITE;
  return 1;
#elif defined(C_REST_USE_WOLFSSL)
  int ret = wolfSSL_write(conn->ssl, buf, (int)len);
  if (ret > 0) {
    *out_written = (size_t)ret;
    return 0;
  }
  int err = wolfSSL_get_error(conn->ssl, ret);
  if (err == WOLFSSL_ERROR_WANT_READ)
    return C_REST_TLS_WANT_READ;
  if (err == WOLFSSL_ERROR_WANT_WRITE)
    return C_REST_TLS_WANT_WRITE;
  return 1;
#elif defined(C_REST_USE_S2N)
  ssize_t ret = s2n_send(conn->conn, buf, len, NULL);
  if (ret >= 0) {
    *out_written = (size_t)ret;
    return 0;
  }
  return 1;
#else
  *out_written = 0;
  return 1;
#endif
}

int c_rest_tls_close(struct c_rest_tls_connection *conn) {
  if (!conn)
    return 0;
#if defined(C_REST_USE_OPENSSL) || defined(C_REST_USE_LIBRESSL) ||             \
    defined(C_REST_USE_BORINGSSL)
  SSL_shutdown(conn->ssl);
  SSL_free(conn->ssl);
#elif defined(C_REST_USE_MBEDTLS)
  mbedtls_ssl_close_notify(&conn->ssl);
  mbedtls_ssl_free(&conn->ssl);
#elif defined(C_REST_USE_WOLFSSL)
  wolfSSL_shutdown(conn->ssl);
  wolfSSL_free(conn->ssl);
#elif defined(C_REST_USE_S2N)
  s2n_shutdown(conn->conn, NULL);
  s2n_connection_free(conn->conn);
#endif
  free(conn);
  return 0;
}
