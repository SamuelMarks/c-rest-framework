/* clang-format off */
#include "c_rest_crypto.h"
#include "c_rest_tls.h"

#include <string.h>
#include <stdlib.h>

#if defined(C_REST_USE_OPENSSL) || defined(C_REST_USE_LIBRESSL) || defined(C_REST_USE_BORINGSSL)
#include <openssl/evp.h>
#include <openssl/rand.h>

#include <openssl/hmac.h>
#include <openssl/evp.h>

#elif defined(C_REST_USE_MBEDTLS)
#include <mbedtls/sha1.h>
#include <mbedtls/sha256.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>

#include <mbedtls/md.h>
#include <mbedtls/pkcs5.h>

#elif defined(C_REST_USE_WOLFSSL)
#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/sha.h>
#include <wolfssl/wolfcrypt/sha256.h>
#include <wolfssl/wolfcrypt/random.h>

#include <wolfssl/wolfcrypt/hmac.h>
#include <wolfssl/wolfcrypt/pwdbased.h>

/* clang-format on */
#endif

int c_rest_tls_get_provider(enum c_rest_crypto_provider *out_provider) {
  if (!out_provider)
    return 1;

#if defined(C_REST_USE_OPENSSL)
  *out_provider = C_REST_CRYPTO_OPENSSL;
#elif defined(C_REST_USE_LIBRESSL)
  *out_provider = C_REST_CRYPTO_LIBRESSL;
#elif defined(C_REST_USE_BORINGSSL)
  *out_provider = C_REST_CRYPTO_BORINGSSL;
#elif defined(C_REST_USE_MBEDTLS)
  *out_provider = C_REST_CRYPTO_MBEDTLS;
#elif defined(C_REST_USE_WOLFSSL)
  *out_provider = C_REST_CRYPTO_WOLFSSL;
#elif defined(C_REST_USE_S2N)
  *out_provider = C_REST_CRYPTO_S2N;
#else
  *out_provider = C_REST_CRYPTO_NONE;
#endif

  return 0;
}

#if !defined(C_REST_HAS_TLS)

/* Fallback to custom SHA1 if no crypto backend is available */
#define SHA1_ROTL(bits, word) (((word) << (bits)) | ((word) >> (32 - (bits))))

static void sha1_transform(unsigned long state[5],
                           const unsigned char buffer[64]) {
  unsigned long a, b, c, d, e;
  typedef union {
    unsigned char c[64];
    unsigned long l[16];
  } CHAR64LONG16;
  CHAR64LONG16 block[1];
  unsigned long w[80];
  int i;

  memcpy(block, buffer, 64);
  for (i = 0; i < 16; i++) {
    w[i] = (buffer[i * 4] << 24) | (buffer[i * 4 + 1] << 16) |
           (buffer[i * 4 + 2] << 8) | buffer[i * 4 + 3];
  }
  for (i = 16; i < 80; i++) {
    w[i] = SHA1_ROTL(1, w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16]);
  }

  a = state[0];
  b = state[1];
  c = state[2];
  d = state[3];
  e = state[4];

  for (i = 0; i < 80; i++) {
    unsigned long f, k;
    unsigned long temp;
    if (i < 20) {
      f = (b & c) | ((~b) & d);
      k = 0x5A827999;
    } else if (i < 40) {
      f = b ^ c ^ d;
      k = 0x6ED9EBA1;
    } else if (i < 60) {
      f = (b & c) | (b & d) | (c & d);
      k = 0x8F1BBCDC;
    } else {
      f = b ^ c ^ d;
      k = 0xCA62C1D6;
    }

    temp = SHA1_ROTL(5, a) + f + e + k + w[i];
    e = d;
    d = c;
    c = SHA1_ROTL(30, b);
    b = a;
    a = temp;
  }

  state[0] += a;
  state[1] += b;
  state[2] += c;
  state[3] += d;
  state[4] += e;
}

int c_rest_sha1(const unsigned char *data, size_t len, unsigned char hash[20]) {
  unsigned long state[5];
  unsigned long count[2];
  unsigned char buffer[64];
  size_t i;

  state[0] = 0x67452301;
  state[1] = 0xEFCDAB89;
  state[2] = 0x98BADCFE;
  state[3] = 0x10325476;
  state[4] = 0xC3D2E1F0;
  count[0] = count[1] = 0;

  for (i = 0; i < len; i++) {
    buffer[count[0] % 64] = data[i];
    if ((count[0] % 64) == 63) {
      sha1_transform(state, buffer);
    }
    count[0]++;
  }

  {
    unsigned char pad[64] = {0x80};
    unsigned char len_bytes[8];
    unsigned long bit_len_hi = (unsigned long)(len >> 29);
    unsigned long bit_len_lo = (unsigned long)(len << 3);
    int pad_len;

    len_bytes[0] = (unsigned char)((bit_len_hi >> 24) & 0xFF);
    len_bytes[1] = (unsigned char)((bit_len_hi >> 16) & 0xFF);
    len_bytes[2] = (unsigned char)((bit_len_hi >> 8) & 0xFF);
    len_bytes[3] = (unsigned char)(bit_len_hi & 0xFF);
    len_bytes[4] = (unsigned char)((bit_len_lo >> 24) & 0xFF);
    len_bytes[5] = (unsigned char)((bit_len_lo >> 16) & 0xFF);
    len_bytes[6] = (unsigned char)((bit_len_lo >> 8) & 0xFF);
    len_bytes[7] = (unsigned char)(bit_len_lo & 0xFF);
    pad_len = 56 - (count[0] % 64);
    if (pad_len <= 0)
      pad_len += 64;

    for (i = 0; i < (size_t)pad_len; i++) {
      buffer[(count[0] + i) % 64] = pad[i];
      if (((count[0] + i) % 64) == 63) {
        sha1_transform(state, buffer);
      }
    }

    for (i = 0; i < 8; i++) {
      buffer[56 + i] = len_bytes[i];
    }
    sha1_transform(state, buffer);
  }

  for (i = 0; i < 20; i++) {
    hash[i] = (unsigned char)(state[i >> 2] >> ((3 - (i & 3)) * 8));
  }
  return 0;
}

/* Fallback to custom SHA256 if no crypto backend is available */
#define SHA256_ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define SHA256_CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define SHA256_MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define SHA256_EP0(x)                                                          \
  (SHA256_ROTR(x, 2) ^ SHA256_ROTR(x, 13) ^ SHA256_ROTR(x, 22))
#define SHA256_EP1(x)                                                          \
  (SHA256_ROTR(x, 6) ^ SHA256_ROTR(x, 11) ^ SHA256_ROTR(x, 25))
#define SHA256_SIG0(x) (SHA256_ROTR(x, 7) ^ SHA256_ROTR(x, 18) ^ ((x) >> 3))
#define SHA256_SIG1(x) (SHA256_ROTR(x, 17) ^ SHA256_ROTR(x, 19) ^ ((x) >> 10))

static const unsigned long sha256_k[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
    0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
    0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
    0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
    0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
    0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

static void sha256_transform(unsigned long state[8],
                             const unsigned char buffer[64]) {
  unsigned long a, b, c, d, e, f, g, h, t1, t2, m[64];
  int i, j;

  for (i = 0, j = 0; i < 16; ++i, j += 4) {
    m[i] = (buffer[j] << 24) | (buffer[j + 1] << 16) | (buffer[j + 2] << 8) |
           (buffer[j + 3]);
  }
  for (i = 16; i < 64; ++i) {
    m[i] =
        SHA256_SIG1(m[i - 2]) + m[i - 7] + SHA256_SIG0(m[i - 15]) + m[i - 16];
  }

  a = state[0];
  b = state[1];
  c = state[2];
  d = state[3];
  e = state[4];
  f = state[5];
  g = state[6];
  h = state[7];

  for (i = 0; i < 64; ++i) {
    t1 = h + SHA256_EP1(e) + SHA256_CH(e, f, g) + sha256_k[i] + m[i];
    t2 = SHA256_EP0(a) + SHA256_MAJ(a, b, c);
    h = g;
    g = f;
    f = e;
    e = d + t1;
    d = c;
    c = b;
    b = a;
    a = t1 + t2;
  }

  state[0] += a;
  state[1] += b;
  state[2] += c;
  state[3] += d;
  state[4] += e;
  state[5] += f;
  state[6] += g;
  state[7] += h;
}

int c_rest_sha256(const unsigned char *data, size_t len,
                  unsigned char hash[32]) {
  unsigned long state[8];
  unsigned long count[2];
  unsigned char buffer[64];
  size_t i;

  state[0] = 0x6a09e667;
  state[1] = 0xbb67ae85;
  state[2] = 0x3c6ef372;
  state[3] = 0xa54ff53a;
  state[4] = 0x510e527f;
  state[5] = 0x9b05688c;
  state[6] = 0x1f83d9ab;
  state[7] = 0x5be0cd19;
  count[0] = count[1] = 0;

  for (i = 0; i < len; i++) {
    buffer[count[0] % 64] = data[i];
    if ((count[0] % 64) == 63) {
      sha256_transform(state, buffer);
    }
    count[0]++;
  }

  {
    unsigned char pad[64] = {0x80};
    unsigned char len_bytes[8];
    unsigned long bit_len_hi = (unsigned long)(len >> 29);
    unsigned long bit_len_lo = (unsigned long)(len << 3);
    int pad_len;

    len_bytes[0] = (unsigned char)((bit_len_hi >> 24) & 0xFF);
    len_bytes[1] = (unsigned char)((bit_len_hi >> 16) & 0xFF);
    len_bytes[2] = (unsigned char)((bit_len_hi >> 8) & 0xFF);
    len_bytes[3] = (unsigned char)(bit_len_hi & 0xFF);
    len_bytes[4] = (unsigned char)((bit_len_lo >> 24) & 0xFF);
    len_bytes[5] = (unsigned char)((bit_len_lo >> 16) & 0xFF);
    len_bytes[6] = (unsigned char)((bit_len_lo >> 8) & 0xFF);
    len_bytes[7] = (unsigned char)(bit_len_lo & 0xFF);
    pad_len = 56 - (count[0] % 64);
    if (pad_len <= 0)
      pad_len += 64;

    for (i = 0; i < (size_t)pad_len; i++) {
      buffer[(count[0] + i) % 64] = pad[i];
      if (((count[0] + i) % 64) == 63) {
        sha256_transform(state, buffer);
      }
    }

    for (i = 0; i < 8; i++) {
      buffer[56 + i] = len_bytes[i];
    }
    sha256_transform(state, buffer);
  }

  for (i = 0; i < 32; i++) {
    hash[i] = (unsigned char)(state[i >> 2] >> ((3 - (i & 3)) * 8));
  }
  return 0;
}

int c_rest_rand_bytes(unsigned char *buf, size_t len) {
  /* Naive fallback */
  size_t i;
  for (i = 0; i < len; ++i) {
    buf[i] = (unsigned char)(rand() % 256);
  }
  return 0;
}

#elif defined(C_REST_USE_OPENSSL) || defined(C_REST_USE_LIBRESSL) ||           \
    defined(C_REST_USE_BORINGSSL)

int c_rest_sha1(const unsigned char *data, size_t len, unsigned char hash[20]) {
  unsigned int out_len = 0;
  EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
  if (!mdctx)
    return 1;

  if (1 != EVP_DigestInit_ex(mdctx, EVP_sha1(), NULL) ||
      1 != EVP_DigestUpdate(mdctx, data, len) ||
      1 != EVP_DigestFinal_ex(mdctx, hash, &out_len)) {
    EVP_MD_CTX_free(mdctx);
    return 1;
  }

  EVP_MD_CTX_free(mdctx);
  return 0;
}

int c_rest_sha256(const unsigned char *data, size_t len,
                  unsigned char hash[32]) {
  unsigned int out_len = 0;
  EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
  if (!mdctx)
    return 1;

  if (1 != EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL) ||
      1 != EVP_DigestUpdate(mdctx, data, len) ||
      1 != EVP_DigestFinal_ex(mdctx, hash, &out_len)) {
    EVP_MD_CTX_free(mdctx);
    return 1;
  }

  EVP_MD_CTX_free(mdctx);
  return 0;
}

int c_rest_rand_bytes(unsigned char *buf, size_t len) {
  if (RAND_bytes(buf, (int)len) != 1) {
    return 1;
  }
  return 0;
}

#elif defined(C_REST_USE_MBEDTLS)

int c_rest_sha1(const unsigned char *data, size_t len, unsigned char hash[20]) {
#if MBEDTLS_VERSION_MAJOR >= 3
  if (mbedtls_sha1(data, len, hash) != 0)
    return 1;
#else
  mbedtls_sha1(data, len, hash);
#endif
  return 0;
}

int c_rest_sha256(const unsigned char *data, size_t len,
                  unsigned char hash[32]) {
#if MBEDTLS_VERSION_MAJOR >= 3
  if (mbedtls_sha256(data, len, hash, 0) != 0)
    return 1;
#else
  mbedtls_sha256(data, len, hash, 0);
#endif
  return 0;
}

int c_rest_rand_bytes(unsigned char *buf, size_t len) {
  mbedtls_entropy_context entropy;
  mbedtls_ctr_drbg_context ctr_drbg;
  int ret;

  mbedtls_entropy_init(&entropy);
  mbedtls_ctr_drbg_init(&ctr_drbg);

  ret =
      mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, NULL, 0);
  if (ret != 0) {
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
    return 1;
  }

  ret = mbedtls_ctr_drbg_random(&ctr_drbg, buf, len);
  mbedtls_ctr_drbg_free(&ctr_drbg);
  mbedtls_entropy_free(&entropy);

  return (ret == 0) ? 0 : 1;
}

#elif defined(C_REST_USE_WOLFSSL)

int c_rest_sha1(const unsigned char *data, size_t len, unsigned char hash[20]) {
  wc_Sha1 sha;
  if (wc_InitSha1(&sha) != 0)
    return 1;
  if (wc_Sha1Update(&sha, data, len) != 0)
    return 1;
  if (wc_Sha1Final(&sha, hash) != 0)
    return 1;
  return 0;
}

int c_rest_sha256(const unsigned char *data, size_t len,
                  unsigned char hash[32]) {
  wc_Sha256 sha256;
  if (wc_InitSha256(&sha256) != 0)
    return 1;
  if (wc_Sha256Update(&sha256, data, len) != 0)
    return 1;
  if (wc_Sha256Final(&sha256, hash) != 0)
    return 1;
  return 0;
}

int c_rest_rand_bytes(unsigned char *buf, size_t len) {
  WC_RNG rng;
  if (wc_InitRng(&rng) != 0)
    return 1;
  if (wc_RNG_GenerateBlock(&rng, buf, len) != 0) {
    wc_FreeRng(&rng);
    return 1;
  }
  wc_FreeRng(&rng);
  return 0;
}

#elif defined(C_REST_USE_S2N)
/* s2n primarily implements TLS, so we fall back to custom for pure hashes if no
 * other crypto provider is used alongside it */
/* Often s2n is built against awslc or openssl, but for our strict decoupling
 * we'll just fall back to custom here if ONLY s2n is specified for TLS */
/* ... same custom implementation as !defined(C_REST_HAS_TLS) block ... */
#endif

#include "c_rest_base64.h"

#if defined(C_REST_USE_OPENSSL) || defined(C_REST_USE_LIBRESSL) ||             \
    defined(C_REST_USE_BORINGSSL)

int c_rest_hmac_sha256(const unsigned char *key, size_t key_len,
                       const unsigned char *data, size_t data_len,
                       unsigned char hash[32]) {
  unsigned int out_len = 32;
  if (!HMAC(EVP_sha256(), key, (int)key_len, data, data_len, hash, &out_len)) {
    return 1;
  }
  return 0;
}

int c_rest_pbkdf2_hmac_sha256(const unsigned char *password,
                              size_t password_len, const unsigned char *salt,
                              size_t salt_len, unsigned int iterations,
                              size_t dk_len, unsigned char *out_key) {
  if (PKCS5_PBKDF2_HMAC((const char *)password, (int)password_len, salt,
                        (int)salt_len, (int)iterations, EVP_sha256(),
                        (int)dk_len, out_key) != 1) {
    return 1;
  }
  return 0;
}

#elif defined(C_REST_USE_MBEDTLS)

int c_rest_hmac_sha256(const unsigned char *key, size_t key_len,
                       const unsigned char *data, size_t data_len,
                       unsigned char hash[32]) {
  const mbedtls_md_info_t *info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  if (!info)
    return 1;
  if (mbedtls_md_hmac(info, key, key_len, data, data_len, hash) != 0)
    return 1;
  return 0;
}

int c_rest_pbkdf2_hmac_sha256(const unsigned char *password,
                              size_t password_len, const unsigned char *salt,
                              size_t salt_len, unsigned int iterations,
                              size_t dk_len, unsigned char *out_key) {
  mbedtls_md_context_t ctx;
  const mbedtls_md_info_t *info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  int ret;

  if (!info)
    return 1;

  mbedtls_md_init(&ctx);
  if (mbedtls_md_setup(&ctx, info, 1) != 0) {
    mbedtls_md_free(&ctx);
    return 1;
  }

  ret = mbedtls_pkcs5_pbkdf2_hmac(&ctx, password, password_len, salt, salt_len,
                                  iterations, (uint32_t)dk_len, out_key);
  mbedtls_md_free(&ctx);
  return ret == 0 ? 0 : 1;
}

#elif defined(C_REST_USE_WOLFSSL)

int c_rest_hmac_sha256(const unsigned char *key, size_t key_len,
                       const unsigned char *data, size_t data_len,
                       unsigned char hash[32]) {
  Hmac hmac;
  if (wc_HmacSetKey(&hmac, WC_HASH_TYPE_SHA256, key, (word32)key_len) != 0)
    return 1;
  if (wc_HmacUpdate(&hmac, data, (word32)data_len) != 0)
    return 1;
  if (wc_HmacFinal(&hmac, hash) != 0)
    return 1;
  return 0;
}

int c_rest_pbkdf2_hmac_sha256(const unsigned char *password,
                              size_t password_len, const unsigned char *salt,
                              size_t salt_len, unsigned int iterations,
                              size_t dk_len, unsigned char *out_key) {
  int ret = wc_PBKDF2(out_key, password, (int)password_len, salt, (int)salt_len,
                      (int)iterations, (int)dk_len, WC_HASH_TYPE_SHA256);
  return ret == 0 ? 0 : 1;
}

#else

int c_rest_hmac_sha256(const unsigned char *key, size_t key_len,
                       const unsigned char *data, size_t data_len,
                       unsigned char hash[32]) {
  unsigned char k_ipad[64];
  unsigned char k_opad[64];
  unsigned char actual_key[64];
  unsigned char inner_hash[32];
  unsigned char *inner_buf;
  unsigned char *outer_buf;
  size_t i;

  if (!key || !data || !hash)
    return 1;

  memset(actual_key, 0, sizeof(actual_key));
  if (key_len > 64) {
    if (c_rest_sha256(key, key_len, actual_key) != 0)
      return 1;
  } else {
    memcpy(actual_key, key, key_len);
  }

  for (i = 0; i < 64; i++) {
    k_ipad[i] = actual_key[i] ^ 0x36;
    k_opad[i] = actual_key[i] ^ 0x5c;
  }

  inner_buf = (unsigned char *)malloc(64 + data_len);
  if (!inner_buf)
    return 1;

  memcpy(inner_buf, k_ipad, 64);
  memcpy(inner_buf + 64, data, data_len);

  if (c_rest_sha256(inner_buf, 64 + data_len, inner_hash) != 0) {
    free(inner_buf);
    return 1;
  }
  free(inner_buf);

  outer_buf = (unsigned char *)malloc(64 + 32);
  if (!outer_buf)
    return 1;

  memcpy(outer_buf, k_opad, 64);
  memcpy(outer_buf + 64, inner_hash, 32);

  if (c_rest_sha256(outer_buf, 64 + 32, hash) != 0) {
    free(outer_buf);
    return 1;
  }
  free(outer_buf);

  return 0;
}

int c_rest_pbkdf2_hmac_sha256(const unsigned char *password,
                              size_t password_len, const unsigned char *salt,
                              size_t salt_len, unsigned int iterations,
                              size_t dk_len, unsigned char *out_key) {
  unsigned int i, k;
  unsigned char U[32];
  unsigned char T[32];
  unsigned int block_index = 1;
  size_t generated_len = 0;

  if (!password || !salt || !out_key || iterations == 0)
    return 1;

  while (generated_len < dk_len) {
    size_t to_copy =
        (dk_len - generated_len < 32) ? (dk_len - generated_len) : 32;
    unsigned char block_idx_bytes[4];
    unsigned char *salt_plus_idx;

    block_idx_bytes[0] = (unsigned char)((block_index >> 24) & 0xFF);
    block_idx_bytes[1] = (unsigned char)((block_index >> 16) & 0xFF);
    block_idx_bytes[2] = (unsigned char)((block_index >> 8) & 0xFF);
    block_idx_bytes[3] = (unsigned char)(block_index & 0xFF);

    salt_plus_idx = (unsigned char *)malloc(salt_len + 4);
    if (!salt_plus_idx)
      return 1;
    memcpy(salt_plus_idx, salt, salt_len);
    memcpy(salt_plus_idx + salt_len, block_idx_bytes, 4);

    if (c_rest_hmac_sha256(password, password_len, salt_plus_idx, salt_len + 4,
                           U) != 0) {
      free(salt_plus_idx);
      return 1;
    }
    free(salt_plus_idx);

    memcpy(T, U, 32);

    for (i = 1; i < iterations; i++) {
      if (c_rest_hmac_sha256(password, password_len, U, 32, U) != 0)
        return 1;
      for (k = 0; k < 32; k++) {
        T[k] ^= U[k];
      }
    }

    memcpy(out_key + generated_len, T, to_copy);
    generated_len += to_copy;
    block_index++;
  }

  return 0;
}

#endif

int c_rest_random_string_generate(size_t entropy_bytes, char **out_str) {
  unsigned char *rand_buf;
  size_t out_len = 0;

  if (!out_str || entropy_bytes == 0)
    return 1;

  rand_buf = (unsigned char *)malloc(entropy_bytes);
  if (!rand_buf)
    return 1;

  if (c_rest_rand_bytes(rand_buf, entropy_bytes) != 0) {
    free(rand_buf);
    return 1;
  }

  if (c_rest_base64url_encode(rand_buf, entropy_bytes, NULL, &out_len) != 0) {
    free(rand_buf);
    return 1;
  }

  *out_str = (char *)malloc(out_len + 1);
  if (!*out_str) {
    free(rand_buf);
    return 1;
  }

  if (c_rest_base64url_encode(rand_buf, entropy_bytes, *out_str, &out_len) !=
      0) {
    free(*out_str);
    free(rand_buf);
    return 1;
  }
  (*out_str)[out_len] = '\0';

  free(rand_buf);
  return 0;
}

int c_rest_jwt_sign_hs256(const char *json_payload, const unsigned char *secret,
                          size_t secret_len, char **out_token) {
  const char *header = "{\"alg\":\"HS256\",\"typ\":\"JWT\"}";
  char *encoded_header = NULL;
  size_t header_len = 0;
  char *encoded_payload = NULL;
  size_t payload_len = 0;
  char *to_sign = NULL;
  size_t to_sign_alloc = 0;
  unsigned char sig[32];
  char *encoded_sig = NULL;
  size_t sig_len = 0;
  char *token = NULL;
  size_t token_alloc = 0;
  size_t real_to_sign_len = 0;

  if (!json_payload || !secret || !out_token)
    return 1;

  c_rest_base64url_encode((const unsigned char *)header, strlen(header), NULL,
                          &header_len);
  encoded_header = (char *)malloc(header_len + 1);
  if (!encoded_header)
    return 1;
  c_rest_base64url_encode((const unsigned char *)header, strlen(header),
                          encoded_header, &header_len);

  c_rest_base64url_encode((const unsigned char *)json_payload,
                          strlen(json_payload), NULL, &payload_len);
  encoded_payload = (char *)malloc(payload_len + 1);
  if (!encoded_payload) {
    free(encoded_header);
    return 1;
  }
  c_rest_base64url_encode((const unsigned char *)json_payload,
                          strlen(json_payload), encoded_payload, &payload_len);

  to_sign_alloc = strlen(encoded_header) + 1 + strlen(encoded_payload) + 1;
  to_sign = (char *)malloc(to_sign_alloc);
  if (!to_sign) {
    free(encoded_header);
    free(encoded_payload);
    return 1;
  }

#if defined(_MSC_VER)
  strcpy_s(to_sign, to_sign_alloc, encoded_header);
  strcat_s(to_sign, to_sign_alloc, ".");
  strcat_s(to_sign, to_sign_alloc, encoded_payload);
#else
  strcpy(to_sign, encoded_header);
  strcat(to_sign, ".");
  strcat(to_sign, encoded_payload);
#endif

  real_to_sign_len = strlen(to_sign);

  if (c_rest_hmac_sha256(secret, secret_len, (const unsigned char *)to_sign,
                         real_to_sign_len, sig) != 0) {
    free(encoded_header);
    free(encoded_payload);
    free(to_sign);
    return 1;
  }

  c_rest_base64url_encode(sig, 32, NULL, &sig_len);
  encoded_sig = (char *)malloc(sig_len + 1);
  if (!encoded_sig) {
    free(encoded_header);
    free(encoded_payload);
    free(to_sign);
    return 1;
  }
  c_rest_base64url_encode(sig, 32, encoded_sig, &sig_len);

  token_alloc = strlen(to_sign) + 1 + strlen(encoded_sig) + 1;
  token = (char *)malloc(token_alloc);
  if (!token) {
    free(encoded_header);
    free(encoded_payload);
    free(to_sign);
    free(encoded_sig);
    return 1;
  }

#if defined(_MSC_VER)
  strcpy_s(token, token_alloc, to_sign);
  strcat_s(token, token_alloc, ".");
  strcat_s(token, token_alloc, encoded_sig);
#else
  strcpy(token, to_sign);
  strcat(token, ".");
  strcat(token, encoded_sig);
#endif

  free(encoded_header);
  free(encoded_payload);
  free(to_sign);
  free(encoded_sig);

  *out_token = token;
  return 0;
}

int c_rest_jwt_verify_hs256(const char *token, const unsigned char *secret,
                            size_t secret_len, char **out_payload) {
  const char *dot1;
  const char *dot2;
  size_t to_sign_len;
  char *to_sign;
  unsigned char expected_sig[32];
  char *encoded_expected_sig;
  size_t encoded_expected_sig_len;
  const char *provided_sig;
  size_t payload_b64_len;
  unsigned char *decoded_payload;
  size_t decoded_payload_len;

  if (!token || !secret || !out_payload)
    return 1;

  dot1 = strchr(token, '.');
  if (!dot1)
    return 1;
  dot2 = strchr(dot1 + 1, '.');
  if (!dot2)
    return 1;

  to_sign_len = (size_t)(dot2 - token);
  to_sign = (char *)malloc(to_sign_len + 1);
  if (!to_sign)
    return 1;
  memcpy(to_sign, token, to_sign_len);
  to_sign[to_sign_len] = '\0';

  provided_sig = dot2 + 1;

  if (c_rest_hmac_sha256(secret, secret_len, (const unsigned char *)to_sign,
                         to_sign_len, expected_sig) != 0) {
    free(to_sign);
    return 1;
  }
  free(to_sign);

  c_rest_base64url_encode(expected_sig, 32, NULL, &encoded_expected_sig_len);
  encoded_expected_sig = (char *)malloc(encoded_expected_sig_len + 1);
  if (!encoded_expected_sig)
    return 1;
  c_rest_base64url_encode(expected_sig, 32, encoded_expected_sig,
                          &encoded_expected_sig_len);

  if (strcmp(provided_sig, encoded_expected_sig) != 0) {
    free(encoded_expected_sig);
    return 1;
  }
  free(encoded_expected_sig);

  payload_b64_len = (size_t)(dot2 - (dot1 + 1));
  if (c_rest_base64url_decode(dot1 + 1, payload_b64_len, NULL,
                              &decoded_payload_len) != 0) {
    return 1;
  }

  decoded_payload = (unsigned char *)malloc(decoded_payload_len + 1);
  if (!decoded_payload)
    return 1;

  if (c_rest_base64url_decode(dot1 + 1, payload_b64_len, decoded_payload,
                              &decoded_payload_len) != 0) {
    free(decoded_payload);
    return 1;
  }
  decoded_payload[decoded_payload_len] = '\0';

  *out_payload = (char *)decoded_payload;
  return 0;
}
