/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "test_protos.h"
#include "c_rest_crypto.h"
#include "c_rest_tls.h"
#include "c_rest_base64.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* clang-format on */

TEST test_crypto_errors(void);
TEST test_crypto_rand_fail(void);
TEST test_crypto_malloc_failures(void);

TEST test_crypto_main(void) {
  const unsigned char data[] = "hello world";
  unsigned char sha1_hash[20];
  unsigned char sha256_hash[32];
  unsigned char rand_buf[16];
  unsigned char hmac_hash[32];
  int res;
  char *jwt_token = NULL;
  char *jwt_payload = NULL;

  const unsigned char expected_sha1[20] = {
      0x2a, 0xae, 0x6c, 0x35, 0xc9, 0x4f, 0xcf, 0xb4, 0x15, 0xdb,
      0xe9, 0x5f, 0x40, 0x8b, 0x9c, 0xe9, 0x1e, 0xe8, 0x46, 0xed};

  const unsigned char expected_sha256[32] = {
      0xb9, 0x4d, 0x27, 0xb9, 0x93, 0x4d, 0x3e, 0x08, 0xa5, 0x2e, 0x52,
      0xd7, 0xda, 0x7d, 0xab, 0xfa, 0xc4, 0x84, 0xef, 0xe3, 0x7a, 0x53,
      0x80, 0xee, 0x90, 0x88, 0xf7, 0xac, 0xe2, 0xef, 0xcd, 0xe9};

  const unsigned char hmac_key[] = "secret";
  const unsigned char expected_hmac[32] = {
      0x73, 0x4c, 0xc6, 0x2f, 0x32, 0x84, 0x15, 0x68, 0xf4, 0x57, 0x15,
      0xae, 0xb9, 0xf4, 0xd7, 0x89, 0x13, 0x24, 0xe6, 0xd9, 0x48, 0xe4,
      0xc6, 0xc6, 0x0c, 0x06, 0x21, 0xcd, 0xac, 0x48, 0x62, 0x3a};

  res = (int)c_rest_sha1(data, 11, sha1_hash);
  ASSERT_FALSE(res != 0);
  ASSERT_FALSE(memcmp(sha1_hash, expected_sha1, 20) != 0);

  res = (int)c_rest_sha256(data, 11, sha256_hash);
  ASSERT_FALSE(res != 0);
  ASSERT_FALSE(memcmp(sha256_hash, expected_sha256, 32) != 0);

  res = (int)c_rest_rand_bytes(rand_buf, sizeof(rand_buf));
  ASSERT_FALSE(res != 0);

  res = (int)c_rest_hmac_sha256(hmac_key, 6, data, 11, hmac_hash);
  ASSERT_FALSE(res != 0);
  ASSERT_FALSE(memcmp(hmac_hash, expected_hmac, 32) != 0);

  {
    unsigned char large_data[100];
    memset(large_data, 'A', 100);
    res = (int)c_rest_sha1(large_data, 100, sha1_hash);
    ASSERT_FALSE(res != 0);
    res = (int)c_rest_sha256(large_data, 100, sha256_hash);
    ASSERT_FALSE(res != 0);
  }

  res =
      (int)c_rest_jwt_sign_hs256("{\"sub\":\"123\"}", hmac_key, 6, &jwt_token);
  ASSERT_FALSE(res != 0);
  ASSERT_FALSE(jwt_token == NULL);

  res = (int)c_rest_jwt_verify_hs256(jwt_token, hmac_key, 6, &jwt_payload);
  ASSERT_FALSE(res != 0);
  ASSERT_FALSE(jwt_payload == NULL);

  ASSERT_FALSE(strcmp(jwt_payload, "{\"sub\":\"123\"}") != 0);
  C_REST_FREE(jwt_payload);
  C_REST_FREE(jwt_token);

  {
    const unsigned char pwd[] = "password";
    const unsigned char salt[] = "salt";
    unsigned char dk[32];
    res = (int)c_rest_pbkdf2_hmac_sha256(pwd, 8, salt, 4, 1, 32, dk);
    ASSERT_FALSE(res != 0);

    {
      char *rand_str = NULL;
      res = (int)c_rest_random_string_generate(32, &rand_str);
      ASSERT_FALSE(res != 0);
      ASSERT_FALSE(rand_str == NULL);
      C_REST_FREE(rand_str);
    }
  }

  /* jwt token invalid payload base64 */
  ASSERT_FALSE(c_rest_jwt_verify_hs256("header.!!.sig",
                                       (const unsigned char *)"key", 3,
                                       &jwt_payload) != C_REST_ERROR_GENERIC);

  {
    unsigned char key[10];
    ASSERT_FALSE(c_rest_pbkdf2_hmac_sha256((const unsigned char *)"pwd", 3,
                                           (const unsigned char *)"salt", 4, 1,
                                           10, key) != C_REST_OK);
  }

  ASSERT_FALSE(c_rest_verify_password(
                   "pwd", "$pbkdf2-sha256$i=1000$c2FsdA==$"
                          "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=") !=
               C_REST_ERROR_GENERIC);
  ASSERT_FALSE(c_rest_verify_password(
                   "pwd", "$pbkdf2-sha256$i=1000$bad_b64_$"
                          "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=") ==
               C_REST_OK);

  ASSERT_FALSE(c_rest_verify_password(
                   "pwd", "$pbkdf2-sha256$i=1000$c2FsdA==$!!!") == C_REST_OK);

  ASSERT_FALSE(c_rest_verify_password(
                   "pwd", "$pbkdf2-sha256$i=1000$c2FsdA==$bad_b64_") ==
               C_REST_OK);

  PASS();
}

TEST test_crypto_rand_fail(void) {
  char *hash = NULL;
  char *token = NULL;
  char *jwt = NULL;

  g_mock_crypto_fail = 11;
  {
    unsigned char h[32];
    ASSERT_EQ(C_REST_OK, c_rest_sha256((const unsigned char *)"a", 1, h));
    ASSERT_NEQ(C_REST_OK, c_rest_sha256((const unsigned char *)"a", 1, h));
    g_mock_crypto_fail = 999;
    ASSERT_EQ(C_REST_OK, c_rest_sha256((const unsigned char *)"a", 1, h));
    g_mock_crypto_fail = 0;
  }

  {
    unsigned char hmac[32];
    unsigned char big_key[100];
    int i;
    memset(big_key, 'x', 100);
    for (i = 101; i <= 103; i++) {
      g_mock_crypto_fail = i;
      ASSERT_NEQ(C_REST_OK,
                 c_rest_hmac_sha256(big_key, 100, (const unsigned char *)"data",
                                    4, hmac));
    }
  }

  {
    int i;
    c_rest_error_t rc;
    for (i = 201; i <= 208; i++) {
      g_mock_crypto_fail = i;
      rc = c_rest_random_string_generate(32, &token);
      if (rc == C_REST_OK && token) {
        C_REST_FREE(token);
        token = NULL;
      }
      rc =
          c_rest_jwt_sign_hs256("{}", (const unsigned char *)"secret", 6, &jwt);
      if (rc == C_REST_OK && jwt) {
        C_REST_FREE(jwt);
        jwt = NULL;
      }
      rc = c_rest_jwt_verify_hs256("eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.e30.t-"
                                   "9K23mB5x3oHZZ9tF4K9tqJ3r8",
                                   (const unsigned char *)"secret", 6, &token);
      ASSERT_NEQ(C_REST_OK, rc);
    }
    {
      char *vtok = NULL;
      g_mock_crypto_fail = 0;
      ASSERT_EQ(C_REST_OK, c_rest_jwt_sign_hs256(
                               "{\"sub\":\"123\"}",
                               (const unsigned char *)"secret", 6, &vtok));
      for (i = 201; i <= 208; i++) {
        g_mock_crypto_fail = i;
        rc = c_rest_jwt_verify_hs256(vtok, (const unsigned char *)"secret", 6,
                                     &token);
        if (rc == C_REST_OK && token) {
          C_REST_FREE(token);
          token = NULL;
        }
      }
      for (i = 501; i <= 504; i++) {
        g_mock_crypto_fail = i;
        rc = c_rest_jwt_verify_hs256(vtok, (const unsigned char *)"secret", 6,
                                     &token);
        if (rc == C_REST_OK && token) {
          C_REST_FREE(token);
          token = NULL;
        }
      }
      C_REST_FREE(vtok);
    }
    for (i = 301; i <= 306; i++) {
      g_mock_crypto_fail = i;
      rc = c_rest_hash_password("pwd", C_REST_HASH_ALG_PBKDF2_SHA256, &hash);
      if (rc == C_REST_OK && hash) {
        C_REST_FREE(hash);
        hash = NULL;
      }
    }
    {
      int k;
      for (k = 401; k <= 405; k++) {
        g_mock_crypto_fail = k;
        rc = c_rest_verify_password(
            "pwd", "$pbkdf2-sha256$i=1000$c2FsdA==$"
                   "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=");
        ASSERT_NEQ(C_REST_OK, rc);
      }
    }
  }

  g_mock_crypto_fail = 3;
  ASSERT_FALSE(c_rest_hash_password("pwd", C_REST_HASH_ALG_PBKDF2_SHA256,
                                    &hash) != C_REST_ERROR_GENERIC);

  g_mock_crypto_fail = 4;
  ASSERT_FALSE(c_rest_random_string_generate(32, &token) !=
               C_REST_ERROR_GENERIC);

  g_mock_crypto_fail = 2;
  ASSERT_FALSE(c_rest_hash_password("pwd", C_REST_HASH_ALG_PBKDF2_SHA256,
                                    &hash) != C_REST_ERROR_GENERIC);
  g_mock_crypto_fail = 5;
  ASSERT_FALSE(c_rest_verify_password(
                   "pwd", "$pbkdf2-sha256$i=1000$c2FsdA==$"
                          "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=") !=
               C_REST_ERROR_GENERIC);
  g_mock_crypto_fail = 6;
  ASSERT_FALSE(c_rest_verify_password(
                   "pwd", "$pbkdf2-sha256$i=1000$c2FsdA==$"
                          "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=") !=
               C_REST_ERROR_GENERIC);
  g_mock_crypto_fail = 7;
  ASSERT_FALSE(c_rest_verify_password(
                   "pwd", "$pbkdf2-sha256$i=1000$c2FsdA==$"
                          "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=") !=
               C_REST_ERROR_GENERIC);
  g_mock_crypto_fail = 0;

  g_mock_crypto_fail = 5;
  ASSERT_FALSE(c_rest_verify_password(
                   "pwd", "$pbkdf2-sha256$i=1000$c2FsdA==$"
                          "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=") !=
               C_REST_ERROR_GENERIC);

  g_mock_crypto_fail = 0;

  PASS();
}

TEST test_crypto_errors(void) {
  char *hash = NULL;
  unsigned char dummy_hash[32];
  int res;

  /* hash_password errors */
  res = (int)c_rest_hash_password(NULL, C_REST_HASH_ALG_PBKDF2_SHA256, &hash);
  ASSERT_FALSE(res == C_REST_OK);
  res = (int)c_rest_hash_password("pwd", C_REST_HASH_ALG_PBKDF2_SHA256, NULL);
  ASSERT_FALSE(res == C_REST_OK);
  res = (int)c_rest_hash_password("pwd", (enum c_rest_password_hash_alg)999,
                                  &hash);
  ASSERT_FALSE(res == C_REST_OK);

  /* verify_password errors */
  res = (int)c_rest_verify_password(NULL, "$pbkdf2-sha256$i=1000$salt$hash");
  ASSERT_FALSE(res == C_REST_OK);
  res = (int)c_rest_verify_password("pwd", NULL);
  ASSERT_FALSE(res == C_REST_OK);
  res = (int)c_rest_verify_password("pwd", "invalid");
  ASSERT_FALSE(res == C_REST_OK);
  res = (int)c_rest_verify_password("pwd", "$pbkdf2-sha256$i=1000$saltonly");
  ASSERT_FALSE(res == C_REST_OK);
  res = (int)c_rest_verify_password("pwd", "$pbkdf2-sha256$i=1000$bad$hash");
  ASSERT_FALSE(res == C_REST_OK);
  res = (int)c_rest_verify_password("pwd", "$pbkdf2-sha256$i=1000$sal~$hash");
  ASSERT_FALSE(res == C_REST_OK);
  res =
      (int)c_rest_verify_password("pwd", "$pbkdf2-sha256$i=1000$c2FsdA==$has~");
  ASSERT_FALSE(res == C_REST_OK);

  /* random_string_generate errors */
  res = (int)c_rest_random_string_generate(10, NULL);
  ASSERT_FALSE(res == C_REST_OK);
  res = (int)c_rest_random_string_generate(0, &hash);
  ASSERT_FALSE(res == C_REST_OK);

  /* tls_get_provider errors */
  res = (int)c_rest_tls_get_provider(NULL);
  ASSERT_FALSE(res == C_REST_OK);

  /* jwt_sign_hs256 errors */
  res =
      (int)c_rest_jwt_sign_hs256(NULL, (const unsigned char *)"key", 3, &hash);
  ASSERT_FALSE(res == C_REST_OK);
  res = (int)c_rest_jwt_sign_hs256("{}", NULL, 3, &hash);
  ASSERT_FALSE(res == C_REST_OK);
  res = (int)c_rest_jwt_sign_hs256("{}", (const unsigned char *)"key", 3, NULL);
  ASSERT_FALSE(res == C_REST_OK);

  /* jwt_verify_hs256 errors */
  res = (int)c_rest_jwt_verify_hs256(NULL, (const unsigned char *)"key", 3,
                                     &hash);
  ASSERT_FALSE(res == C_REST_OK);
  res = (int)c_rest_jwt_verify_hs256("token", NULL, 3, &hash);
  ASSERT_FALSE(res == C_REST_OK);
  res = (int)c_rest_jwt_verify_hs256("token", (const unsigned char *)"key", 3,
                                     NULL);
  ASSERT_FALSE(res == C_REST_OK);
  res = (int)c_rest_jwt_verify_hs256("invalid.token",
                                     (const unsigned char *)"key", 3, &hash);
  ASSERT_FALSE(res == C_REST_OK);
  res = (int)c_rest_jwt_verify_hs256("invalid", (const unsigned char *)"key", 3,
                                     &hash);
  ASSERT_FALSE(res == C_REST_OK);
  res = (int)c_rest_jwt_verify_hs256("invalid.token.here",
                                     (const unsigned char *)"key", 3, &hash);
  ASSERT_FALSE(res == C_REST_OK);

  {
    const unsigned char key[] = "key";
    const char *to_sign = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.sal~";
    unsigned char expected_sig[32];
    char encoded_sig[100];
    size_t encoded_sig_len;
    char token_buf[200];
    char *out_payload = NULL;

    ASSERT_EQ(C_REST_OK,
              c_rest_hmac_sha256(key, 3, (const unsigned char *)to_sign,
                                 strlen(to_sign), expected_sig));
    ASSERT_FALSE(c_rest_base64url_encode(expected_sig, 32, NULL,
                                         &encoded_sig_len) != C_REST_OK);
    ASSERT_FALSE(c_rest_base64url_encode(expected_sig, 32, encoded_sig,
                                         &encoded_sig_len) != C_REST_OK);
    encoded_sig[encoded_sig_len] = '\0';
#if defined(_MSC_VER)
    sprintf_s(token_buf, sizeof(token_buf), "%s.%s", to_sign, encoded_sig);
#else
    sprintf(token_buf, "%s.%s", to_sign, encoded_sig);
#endif

    res = (int)c_rest_jwt_verify_hs256(token_buf, key, 3, &out_payload);
    ASSERT_FALSE(res == C_REST_OK);
  }

  /* pbkdf2_hmac_sha256 errors */
  res = (int)c_rest_pbkdf2_hmac_sha256(NULL, 0, (const unsigned char *)"salt",
                                       4, 1, 32, dummy_hash); /* Dummy hash */
  ASSERT_FALSE(res == C_REST_OK);
  res = (int)c_rest_pbkdf2_hmac_sha256((const unsigned char *)"pwd", 3, NULL, 0,
                                       1, 32, dummy_hash);
  ASSERT_FALSE(res == C_REST_OK);
  res = (int)c_rest_pbkdf2_hmac_sha256((const unsigned char *)"pwd", 3,
                                       (const unsigned char *)"salt", 4, 1, 32,
                                       NULL);
  ASSERT_FALSE(res == C_REST_OK);
  res = (int)c_rest_pbkdf2_hmac_sha256((const unsigned char *)"pwd", 3,
                                       (const unsigned char *)"salt", 4, 0, 32,
                                       dummy_hash);
  ASSERT_FALSE(res == C_REST_OK);

  /* hmac_sha256 errors and large key */
  {
    unsigned char large_key[100];
    memset(large_key, 'A', 100);
    res = (int)c_rest_hmac_sha256(large_key, 100, (const unsigned char *)"data",
                                  4, dummy_hash);
    ASSERT_FALSE(res != C_REST_OK);
  }
  res = (int)c_rest_hmac_sha256(NULL, 3, (const unsigned char *)"data", 4,
                                dummy_hash);
  ASSERT_FALSE(res == C_REST_OK);
  res = (int)c_rest_hmac_sha256((const unsigned char *)"key", 3, NULL, 4,
                                dummy_hash);
  ASSERT_FALSE(res == C_REST_OK);
  res = (int)c_rest_hmac_sha256((const unsigned char *)"key", 3,
                                (const unsigned char *)"data", 4, NULL);
  ASSERT_FALSE(res == C_REST_OK);

  PASS();
}

static int g_malloc_fail_after = -1;
static int g_malloc_fail_size = -1;
static void *fail_malloc_n(size_t size) {
  void *res = NULL;
  int should_fail = 0;

  if (g_malloc_fail_size == -1 || (int)size == g_malloc_fail_size) {
    if (g_malloc_fail_after == 0) {
      should_fail = 1;
    }
    if (g_malloc_fail_after >= 0) {
      g_malloc_fail_after--;
    }
  }

  res = malloc(size);
  if (should_fail) {
    free(res);
    res = NULL;
  }
  return res;
}

TEST test_crypto_malloc_failures(void) {
  char *hash = NULL;
  unsigned char dummy_hash[32];
  int res;
  int i;
  enum c_rest_crypto_provider provider;

  /* Check tls_get_provider valid pointer */
  res = (int)c_rest_tls_get_provider(&provider);
  ASSERT_FALSE(res != C_REST_OK);

  g_crf_malloc_hook = fail_malloc_n;

  {
    int sizes[] = {25, 26, 45, 46, 110, 112};
    int j;
    for (j = 0; j < 6; j++) {
      g_malloc_fail_size = sizes[j];
      g_malloc_fail_after = 0;
      res = (int)c_rest_hash_password("pwd", C_REST_HASH_ALG_PBKDF2_SHA256,
                                      &hash);
      if (res == C_REST_OK)
        C_REST_FREE(hash);
    }
  }
  g_malloc_fail_size = -1;

  for (i = 0; i < 20; i++) {
    g_malloc_fail_after = i;
    res = (int)c_rest_verify_password(
        "pwd", "$pbkdf2-sha256$i=1000$c2FsdA==$"
               "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=");
  }

  for (i = 0; i < 5; i++) {
    g_malloc_fail_after = i;
    res = (int)c_rest_random_string_generate(10, &hash);
    if (res == C_REST_OK)
      C_REST_FREE(hash);
  }
  g_malloc_fail_after = -1;
  /* Malloc failure when encoding hash/salt */
  for (i = 0; i < 20; i++) {
    g_malloc_fail_after = (i == 0 ? -1 : i);
    res =
        (int)c_rest_hash_password("pwd", C_REST_HASH_ALG_PBKDF2_SHA256, &hash);
    if (res != C_REST_OK) {
      /* expected failure */
    } else {
      C_REST_FREE(hash);
    }
  }

  for (i = 0; i < 15; i++) {
    g_malloc_fail_after = i;
    res = (int)c_rest_jwt_sign_hs256("{}", (const unsigned char *)"key", 3,
                                     &hash);
    if (res == C_REST_OK)
      C_REST_FREE(hash);
  }

  {
    char *valid_token = NULL;
    g_malloc_fail_after = -1;
    res = (int)c_rest_jwt_sign_hs256(
        "{\"sub\":\"123\"}", (const unsigned char *)"key", 3, &valid_token);
    ASSERT_EQ(C_REST_OK, res);
    for (i = 0; i < 10; i++) {
      g_malloc_fail_size = -1;
      g_malloc_fail_after = i;
      res = (int)c_rest_jwt_verify_hs256(
          valid_token, (const unsigned char *)"key", 3, &hash);
      if (res == C_REST_OK)
        C_REST_FREE(hash);
    }
    C_REST_FREE(valid_token);
  }

  for (i = 0; i < 5; i++) {
    g_malloc_fail_after = i;
    res = (int)c_rest_pbkdf2_hmac_sha256((const unsigned char *)"pwd", 3,
                                         (const unsigned char *)"salt", 4, 1,
                                         32, dummy_hash);
  }

  for (i = 0; i < 5; i++) {
    g_malloc_fail_after = i;
    res = (int)c_rest_hmac_sha256((const unsigned char *)"key", 3,
                                  (const unsigned char *)"data", 4, dummy_hash);
  }
  for (i = 0; i < 20; i++) {
    g_malloc_fail_after = i;
    res = (int)c_rest_verify_password(
        "pwd", "$pbkdf2-sha256$i=1000$c2FsdA==$"
               "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=");
  }

  /* Cover bad base64 in verify_password */
  res = (int)c_rest_verify_password(
      "pwd", "$pbkdf2-sha256$i=1000$+++=$"
             "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=");
  res =
      (int)c_rest_verify_password("pwd", "$pbkdf2-sha256$i=1000$c2FsdA==$+++=");
  res = (int)c_rest_verify_password("pwd",
                                    "$pbkdf2-sha256$i=1000$c2FsdA==$aG==aA==");

  /* Cover hmac_sha256 fail in verify_password */
  for (i = 0; i < 5; i++) {
    g_malloc_fail_after = i;
    res = (int)c_rest_verify_password(
        "pwd", "$pbkdf2-sha256$i=2$c2FsdA==$"
               "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=");
  }

  /* Cover bad base64 in jwt_verify */
  res = (int)c_rest_jwt_verify_hs256(
      "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.+++.+++",
      (const unsigned char *)"key", 3, &hash);

  g_crf_malloc_hook = NULL;

  PASS();
}

SUITE(crypto_suite) {
  RUN_TEST(test_crypto_main);
  RUN_TEST(test_crypto_errors);
  RUN_TEST(test_crypto_rand_fail);
  RUN_TEST(test_crypto_malloc_failures);
}
