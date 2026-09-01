/* clang-format off */
#include "c_rest_error.h"
#include "greatest.h"
#include <string.h>

#undef C_REST_EXPORT
#define C_REST_EXPORT

#include "c_rest_crypto.h"
#include "c_rest_base64.h"

static int g_mock_base64_countdown = -1;
static int g_mock_base64_dec_countdown = -1;

/* Forward declarations */
extern c_rest_error_t c_rest_base64_encode(const unsigned char *src, size_t src_len, char *dst, size_t *dst_len);
extern c_rest_error_t c_rest_base64url_encode(const unsigned char *src, size_t src_len, char *dst, size_t *dst_len);
extern c_rest_error_t c_rest_base64_decode(const char *src, size_t src_len, unsigned char *dst, size_t *dst_len);

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

#define c_rest_base64_encode mock_c_rest_base64_encode
#define c_rest_base64url_encode mock_c_rest_base64url_encode
#define c_rest_base64_decode mock_c_rest_base64_decode

#define c_rest_hash_password test_c_rest_hash_password
#define c_rest_verify_password test_c_rest_verify_password
#define c_rest_jwt_sign_hs256 test_c_rest_jwt_sign_hs256
#define c_rest_jwt_verify_hs256 test_c_rest_jwt_verify_hs256

#include "../src/c_rest_crypto.c"

#undef c_rest_pbkdf2_hmac_sha256
#undef c_rest_sha256
#undef c_rest_base64_encode
#undef c_rest_base64url_encode
#undef c_rest_hmac_sha256
#undef c_rest_base64_decode

/* clang-format on */

static void reset_mocks(void *data) {
  (void)data;
  g_mock_base64_countdown = -1;
  g_mock_base64_dec_countdown = -1;
}

extern int g_mock_crypto_fail;

TEST test_crypto_error_branches(void) {
  char *hash = NULL;
  char *jwt = NULL;
  char *payload = NULL;

  /* test c_rest_pbkdf2_hmac_sha256 failure via g_mock_crypto_fail == 2 */
  g_mock_crypto_fail = 2;
  ASSERT_EQ(
      C_REST_ERROR_GENERIC,
      test_c_rest_hash_password("pwd", C_REST_HASH_ALG_PBKDF2_SHA256, &hash));
  g_mock_crypto_fail = 0;

  /* test c_rest_base64_encode failures */
  g_mock_base64_countdown = 0; /* first call */
  ASSERT_EQ(
      C_REST_ERROR_GENERIC,
      test_c_rest_hash_password("pwd", C_REST_HASH_ALG_PBKDF2_SHA256, &hash));

  g_mock_base64_countdown = 1; /* second call */
  ASSERT_EQ(
      C_REST_ERROR_GENERIC,
      test_c_rest_hash_password("pwd", C_REST_HASH_ALG_PBKDF2_SHA256, &hash));

  g_mock_base64_countdown = 2; /* third call */
  ASSERT_EQ(
      C_REST_ERROR_GENERIC,
      test_c_rest_hash_password("pwd", C_REST_HASH_ALG_PBKDF2_SHA256, &hash));

  g_mock_base64_countdown = 3; /* fourth call */
  ASSERT_EQ(
      C_REST_ERROR_GENERIC,
      test_c_rest_hash_password("pwd", C_REST_HASH_ALG_PBKDF2_SHA256, &hash));

  /* JWT Sign base64url encode failures */
  g_mock_base64_countdown = 0;
  ASSERT_EQ(
      C_REST_ERROR_GENERIC,
      test_c_rest_jwt_sign_hs256("{}", (const unsigned char *)"key", 3, &jwt));

  g_mock_base64_countdown = 1;
  ASSERT_EQ(
      C_REST_ERROR_GENERIC,
      test_c_rest_jwt_sign_hs256("{}", (const unsigned char *)"key", 3, &jwt));

  g_mock_base64_countdown = 2;
  ASSERT_EQ(
      C_REST_ERROR_GENERIC,
      test_c_rest_jwt_sign_hs256("{}", (const unsigned char *)"key", 3, &jwt));

  g_mock_base64_countdown = 3;
  ASSERT_EQ(
      C_REST_ERROR_GENERIC,
      test_c_rest_jwt_sign_hs256("{}", (const unsigned char *)"key", 3, &jwt));

  /* Verify decode failure */
  test_c_rest_jwt_sign_hs256("{}", (const unsigned char *)"key", 3, &jwt);
  g_mock_base64_dec_countdown = 0;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_jwt_verify_hs256(jwt, (const unsigned char *)"key", 3,
                                         &payload));

  /* Verify encode failure (it encodes the expected sig) */
  g_mock_base64_countdown = 0;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_jwt_verify_hs256(jwt, (const unsigned char *)"key", 3,
                                         &payload));

  /* Cleanup */
  CRF_FREE(jwt);

  PASS();
}

SUITE(crypto_mock_suite) {
  SET_SETUP(reset_mocks, NULL);
  RUN_TEST(test_crypto_error_branches);
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
  GREATEST_MAIN_BEGIN();
  RUN_SUITE(crypto_mock_suite);
  GREATEST_MAIN_END();
}
