/* clang-format off */
#include "c_rest_error.h"
#include "greatest.h"
#include "greatest_clean.h"
#include <string.h>

#undef C_REST_EXPORT
#define C_REST_EXPORT

#include "c_rest_crypto.h"
#include "c_rest_base64.h"
#include "c_rest_tls.h"

#undef C_REST_EXPORT
#if defined(_MSC_VER) && !defined(C_REST_FRAMEWORK_STATIC_DEFINE)
#define C_REST_EXPORT __declspec(dllimport)
#else
#define C_REST_EXPORT
#endif

static int g_mock_base64_countdown = -1;
static int g_mock_base64_dec_countdown = -1;
static int g_mock_malloc_fail = 0;
static int g_mock_malloc_fail_size = -1;

/* Forward declarations */
extern c_rest_error_t c_rest_base64_encode(const unsigned char *src, size_t src_len, char *dst, size_t *dst_len);
extern c_rest_error_t c_rest_base64url_encode(const unsigned char *src, size_t src_len, char *dst, size_t *dst_len);
extern c_rest_error_t c_rest_base64_decode(const char *src, size_t src_len, unsigned char *dst, size_t *dst_len);
extern c_rest_error_t c_rest_base64url_decode(const char *src, size_t src_len, unsigned char *dst, size_t *dst_len);
extern void *test_c_rest_internal_malloc(size_t size);

static c_rest_error_t mock_c_rest_base64_encode(const unsigned char *src, size_t src_len, char *dst, size_t *dst_len) {
    if (g_mock_base64_countdown >= 0) {
        if (g_mock_base64_countdown == 0) return C_REST_ERROR_GENERIC;
        g_mock_base64_countdown--;
    }
    return c_rest_base64_encode(src, src_len, dst, dst_len);
}

static c_rest_error_t mock_c_rest_base64url_encode(const unsigned char *src, size_t src_len, char *dst, size_t *dst_len) {
    if (g_mock_base64_countdown >= 0) {
        if (g_mock_base64_countdown == 0) return C_REST_ERROR_GENERIC;
        g_mock_base64_countdown--;
    }
    return c_rest_base64url_encode(src, src_len, dst, dst_len);
}

static c_rest_error_t mock_c_rest_base64_decode(const char *src, size_t src_len, unsigned char *dst, size_t *dst_len) {
    if (g_mock_base64_dec_countdown >= 0) {
        if (g_mock_base64_dec_countdown == 0) return C_REST_ERROR_GENERIC;
        g_mock_base64_dec_countdown--;
    }
    return c_rest_base64_decode(src, src_len, dst, dst_len);
}

static c_rest_error_t mock_c_rest_base64url_decode(const char *src, size_t src_len, unsigned char *dst, size_t *dst_len) {
    if (g_mock_base64_dec_countdown >= 0) {
        if (g_mock_base64_dec_countdown == 0) return C_REST_ERROR_GENERIC;
        g_mock_base64_dec_countdown--;
    }
    return c_rest_base64url_decode(src, src_len, dst, dst_len);
}

static void *mock_internal_malloc(size_t size) {
    if (g_mock_malloc_fail) return NULL;
    if (g_mock_malloc_fail_size > 0 && (int)size == g_mock_malloc_fail_size) return NULL;
    return test_c_rest_internal_malloc(size);
}

#define c_rest_base64_encode mock_c_rest_base64_encode
#define c_rest_base64url_encode mock_c_rest_base64url_encode
#define c_rest_base64_decode mock_c_rest_base64_decode
#define c_rest_base64url_decode mock_c_rest_base64url_decode
#define CRF_MALLOC mock_internal_malloc

#include "../src/c_rest_crypto.c"

#undef c_rest_sha256
#undef c_rest_base64_encode
#undef c_rest_base64url_encode
#undef c_rest_hmac_sha256
#undef c_rest_base64_decode
#undef c_rest_base64url_decode
#undef CRF_MALLOC

/* clang-format on */

static void reset_mocks(void *data) {
  (void)data;
  g_mock_base64_countdown = -1;
  g_mock_base64_dec_countdown = -1;
  g_mock_malloc_fail = 0;
  g_mock_malloc_fail_size = -1;
}

TEST test_crypto_error_branches(void) {
  char *hash = NULL;
  char *jwt = NULL;
  char *payload = NULL;

  /* test c_rest_pbkdf2_hmac_sha256 failure via g_mock_crypto_fail == 2 */
  g_mock_crypto_fail = 2;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_hash_password("pwd", C_REST_HASH_ALG_PBKDF2_SHA256, &hash));
  g_mock_crypto_fail = 0;

  /* test c_rest_base64_encode failures */
  g_mock_base64_countdown = 0; /* first call */
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_hash_password("pwd", C_REST_HASH_ALG_PBKDF2_SHA256, &hash));

  g_mock_base64_countdown = 1; /* second call */
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_hash_password("pwd", C_REST_HASH_ALG_PBKDF2_SHA256, &hash));

  g_mock_base64_countdown = 2; /* third call */
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_hash_password("pwd", C_REST_HASH_ALG_PBKDF2_SHA256, &hash));

  g_mock_base64_countdown = 3; /* fourth call */
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_hash_password("pwd", C_REST_HASH_ALG_PBKDF2_SHA256, &hash));
  reset_mocks(NULL);

  /* test malloc failures in hash_password */
  g_mock_malloc_fail_size = 26;
  ASSERT_EQ(C_REST_ERROR_OOM,
            c_rest_hash_password("pwd", C_REST_HASH_ALG_PBKDF2_SHA256, &hash));

  g_mock_malloc_fail_size = 46;
  ASSERT_EQ(C_REST_ERROR_OOM,
            c_rest_hash_password("pwd", C_REST_HASH_ALG_PBKDF2_SHA256, &hash));

  g_mock_malloc_fail_size = 110;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_hash_password("pwd", C_REST_HASH_ALG_PBKDF2_SHA256, &hash));
  reset_mocks(NULL);

  /* JWT Sign base64url encode failures */
  g_mock_base64_countdown = 0;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_jwt_sign_hs256("{}", (const unsigned char *)"key", 3, &jwt));

  g_mock_base64_countdown = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_jwt_sign_hs256("{}", (const unsigned char *)"key", 3, &jwt));

  g_mock_base64_countdown = 2;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_jwt_sign_hs256("{}", (const unsigned char *)"key", 3, &jwt));

  g_mock_base64_countdown = 3;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_jwt_sign_hs256("{}", (const unsigned char *)"key", 3, &jwt));

  g_mock_base64_countdown = 4;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_jwt_sign_hs256("{}", (const unsigned char *)"key", 3, &jwt));

  g_mock_base64_countdown = 5;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_jwt_sign_hs256("{}", (const unsigned char *)"key", 3, &jwt));

  /* Verify decode failure */
  reset_mocks(NULL);
  {
    unsigned char dbuf[10];
    size_t dlen = 10;
    mock_c_rest_base64url_decode("a", 1, dbuf, &dlen);
  }
  c_rest_jwt_sign_hs256("{}", (const unsigned char *)"key", 3, &jwt);
  g_mock_base64_dec_countdown = 1;
  ASSERT_EQ(
      C_REST_ERROR_GENERIC,
      c_rest_jwt_verify_hs256(jwt, (const unsigned char *)"key", 3, &payload));
  reset_mocks(NULL);
  g_mock_base64_dec_countdown = 0;
  ASSERT_EQ(
      C_REST_ERROR_GENERIC,
      c_rest_jwt_verify_hs256(jwt, (const unsigned char *)"key", 3, &payload));

  /* Verify encode failure (it encodes the expected sig) */
  reset_mocks(NULL);
  g_mock_base64_countdown = 0;
  ASSERT_EQ(
      C_REST_ERROR_GENERIC,
      c_rest_jwt_verify_hs256(jwt, (const unsigned char *)"key", 3, &payload));
  reset_mocks(NULL);
  g_mock_base64_countdown = 1;
  ASSERT_EQ(
      C_REST_ERROR_GENERIC,
      c_rest_jwt_verify_hs256(jwt, (const unsigned char *)"key", 3, &payload));

  /* Cleanup */
  CRF_FREE(jwt);
  reset_mocks(NULL);

  /* Password hash and verify tests */
  ASSERT_EQ(C_REST_OK,
            c_rest_hash_password("pwd", C_REST_HASH_ALG_PBKDF2_SHA256, &hash));

  /* NULL checks */
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_verify_password(NULL, hash));
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_verify_password("pwd", NULL));

  /* Invalid prefix */
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_verify_password("pwd", "$unknown$i=1000$a$b"));

  /* Invalid format (sscanf failure) */
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_verify_password("pwd", "$pbkdf2-sha256$i=invalid"));

  /* First base64_decode failure on salt */
  g_mock_base64_dec_countdown = 0;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_verify_password("pwd", hash));
  reset_mocks(NULL);

  /* Malloc failure on salt */
  g_mock_malloc_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_verify_password("pwd", hash));
  reset_mocks(NULL);

  /* Second base64_decode failure on salt */
  g_mock_base64_dec_countdown = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_verify_password("pwd", hash));
  reset_mocks(NULL);

  /* Third base64_decode failure on hash */
  g_mock_base64_dec_countdown = 2;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_verify_password("pwd", hash));
  reset_mocks(NULL);

  /* Fourth base64_decode failure on hash */
  g_mock_base64_dec_countdown = 3;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_verify_password("pwd", hash));
  reset_mocks(NULL);

  /* Wrong expected_hash_len */
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_verify_password(
                "pwd", "$pbkdf2-sha256$i=1000$c2FsdA==$dG9vc2hvcnQ="));

  /* pbkdf2 failure via g_mock_crypto_fail == 5 */
  g_mock_crypto_fail = 5;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_verify_password("pwd", hash));
  g_mock_crypto_fail = 0;

  /* Wrong password (diff != 0) */
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_verify_password("wrongpwd", hash));

  /* Success case */
  ASSERT_EQ(C_REST_OK, c_rest_verify_password("pwd", hash));

  CRF_FREE(hash);

  /* test c_rest_random_string_generate base64url failures */
  {
    char *rand_str = NULL;
    g_mock_base64_countdown = 0;
    ASSERT_EQ(C_REST_ERROR_GENERIC,
              c_rest_random_string_generate(16, &rand_str));
    g_mock_base64_countdown = 1;
    ASSERT_EQ(C_REST_ERROR_GENERIC,
              c_rest_random_string_generate(16, &rand_str));
    reset_mocks(NULL);
  }

  /* test c_rest_hmac_sha256 failures */
  {
    unsigned char mac[32];
    unsigned char long_key[65];
    memset(long_key, 'A', sizeof(long_key));
    g_mock_crypto_fail = 10;
    ASSERT_EQ(C_REST_ERROR_GENERIC,
              c_rest_hmac_sha256(long_key, sizeof(long_key),
                                 (const unsigned char *)"data", 4, mac));
    ASSERT_EQ(C_REST_ERROR_GENERIC,
              c_rest_hmac_sha256((const unsigned char *)"key", 3,
                                 (const unsigned char *)"data", 4, mac));
    g_mock_crypto_fail = 11;
    ASSERT_EQ(C_REST_ERROR_GENERIC,
              c_rest_hmac_sha256((const unsigned char *)"key", 3,
                                 (const unsigned char *)"data", 4, mac));
    g_mock_crypto_fail = 0;
  }

  PASS();
}

SUITE_EXTERN(crypto_mock_suite);
SUITE(crypto_mock_suite) {
  SET_SETUP(reset_mocks, NULL);
  RUN_TEST(test_crypto_error_branches);
}
