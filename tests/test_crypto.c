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

int test_crypto(void);
static int test_crypto_errors(void);
static int test_crypto_rand_fail(void);
static int test_crypto_malloc_failures(void);

int test_crypto(void) {
  const unsigned char data[] = "hello world";
  unsigned char sha1_hash[20];
  unsigned char sha256_hash[32];
  unsigned char rand_buf[16];
  unsigned char hmac_hash[32];
  int res;
  char *jwt_token = NULL;
  char *jwt_payload = NULL;
  int failed = 0;
  const char *msgs[2];

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
  failed += (res != 0);
  failed += (memcmp(sha1_hash, expected_sha1, 20) != 0);

  res = (int)c_rest_sha256(data, 11, sha256_hash);
  failed += (res != 0);
  failed += (memcmp(sha256_hash, expected_sha256, 32) != 0);

  res = (int)c_rest_rand_bytes(rand_buf, sizeof(rand_buf));
  failed += (res != 0);

  res = (int)c_rest_hmac_sha256(hmac_key, 6, data, 11, hmac_hash);
  failed += (res != 0);
  failed += (memcmp(hmac_hash, expected_hmac, 32) != 0);

  {
    unsigned char large_data[100];
    memset(large_data, 'A', 100);
    res = (int)c_rest_sha1(large_data, 100, sha1_hash);
    failed += (res != 0);
    res = (int)c_rest_sha256(large_data, 100, sha256_hash);
    failed += (res != 0);
  }

  res =
      (int)c_rest_jwt_sign_hs256("{\"sub\":\"123\"}", hmac_key, 6, &jwt_token);
  failed += (res != 0);
  failed += (jwt_token == NULL);

  if (jwt_token) {
    res = (int)c_rest_jwt_verify_hs256(jwt_token, hmac_key, 6, &jwt_payload);
    failed += (res != 0);
    failed += (jwt_payload == NULL);

    if (jwt_payload) {
      failed += (strcmp(jwt_payload, "{\"sub\":\"123\"}") != 0);
      CRF_FREE(jwt_payload);
    }
    CRF_FREE(jwt_token);
  }

  {
    const unsigned char pwd[] = "password";
    const unsigned char salt[] = "salt";
    unsigned char dk[32];
    res = (int)c_rest_pbkdf2_hmac_sha256(pwd, 8, salt, 4, 1, 32, dk);
    failed += (res != 0);

    {
      char *rand_str = NULL;
      res = (int)c_rest_random_string_generate(32, &rand_str);
      failed += (res != 0);
      failed += (rand_str == NULL);
      if (rand_str) {
        CRF_FREE(rand_str);
      }
    }
  }

  failed += (test_crypto_errors() != 0);

  /* jwt token invalid payload base64 */
  failed +=
      (c_rest_jwt_verify_hs256("header.!!.sig", (const unsigned char *)"key", 3,
                               &jwt_payload) != C_REST_ERROR_GENERIC);

  {
    unsigned char key[10];
    failed += (c_rest_pbkdf2_hmac_sha256((const unsigned char *)"pwd", 3,
                                         (const unsigned char *)"salt", 4, 1,
                                         10, key) != C_REST_OK);
  }

  failed += (c_rest_verify_password(
                 "pwd", "$pbkdf2-sha256$i=1000$c2FsdA==$"
                        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=") !=
             C_REST_ERROR_GENERIC);
  failed += (c_rest_verify_password(
                 "pwd", "$pbkdf2-sha256$i=1000$bad_b64_$"
                        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=") !=
             C_REST_ERROR_GENERIC);

  failed +=
      (c_rest_verify_password("pwd", "$pbkdf2-sha256$i=1000$c2FsdA==$!!!") !=
       C_REST_ERROR_GENERIC);

  failed += (c_rest_verify_password(
                 "pwd", "$pbkdf2-sha256$i=1000$c2FsdA==$bad_b64_") !=
             C_REST_ERROR_GENERIC);

  failed += (test_crypto_rand_fail() != 0);

  failed += (test_crypto_malloc_failures() != 0);

  msgs[0] = "test_crypto passed\n";
  msgs[1] = "test_crypto failed\n";
  printf("%s", msgs[failed != 0]);

  return failed;
}

static int test_crypto_rand_fail(void) {
  int failed = 0;
  char *hash = NULL;
  char *token = NULL;

  g_mock_crypto_fail = 3;
  failed += (c_rest_hash_password("pwd", C_REST_HASH_ALG_PBKDF2_SHA256,
                                  &hash) != C_REST_ERROR_GENERIC);
  printf("DID HASH FAIL FOR MOCK 3? failed=%d\n", failed);

  g_mock_crypto_fail = 4;
  failed += (c_rest_random_string_generate(32, &token) != C_REST_ERROR_GENERIC);

  g_mock_crypto_fail = 2;
  failed += (c_rest_hash_password("pwd", C_REST_HASH_ALG_PBKDF2_SHA256,
                                  &hash) != C_REST_ERROR_GENERIC);
  g_mock_crypto_fail = 5;
  failed += (c_rest_verify_password(
                 "pwd", "$pbkdf2-sha256$i=1000$c2FsdA==$"
                        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=") !=
             C_REST_ERROR_GENERIC);
  g_mock_crypto_fail = 6;
  failed += (c_rest_verify_password(
                 "pwd", "$pbkdf2-sha256$i=1000$c2FsdA==$"
                        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=") !=
             C_REST_ERROR_GENERIC);
  g_mock_crypto_fail = 7;
  failed += (c_rest_verify_password(
                 "pwd", "$pbkdf2-sha256$i=1000$c2FsdA==$"
                        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=") !=
             C_REST_ERROR_GENERIC);
  g_mock_crypto_fail = 0;

  g_mock_crypto_fail = 5;
  failed += (c_rest_verify_password(
                 "pwd", "$pbkdf2-sha256$i=1000$c2FsdA==$"
                        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=") !=
             C_REST_ERROR_GENERIC);

  g_mock_crypto_fail = 0;

  return failed;
}

static int test_crypto_errors(void) {
  char *hash = NULL;
  unsigned char dummy_hash[32];
  int failed = 0;
  int res;

  /* hash_password errors */
  res = (int)c_rest_hash_password(NULL, C_REST_HASH_ALG_PBKDF2_SHA256, &hash);
  failed += (res == C_REST_OK);
  res = (int)c_rest_hash_password("pwd", C_REST_HASH_ALG_PBKDF2_SHA256, NULL);
  failed += (res == C_REST_OK);
  res = (int)c_rest_hash_password("pwd", (enum c_rest_password_hash_alg)999,
                                  &hash);
  failed += (res == C_REST_OK);

  /* verify_password errors */
  res = (int)c_rest_verify_password(NULL, "$pbkdf2-sha256$i=1000$salt$hash");
  failed += (res == C_REST_OK);
  res = (int)c_rest_verify_password("pwd", NULL);
  failed += (res == C_REST_OK);
  res = (int)c_rest_verify_password("pwd", "invalid");
  failed += (res == C_REST_OK);
  res = (int)c_rest_verify_password("pwd", "$pbkdf2-sha256$i=1000$saltonly");
  failed += (res == C_REST_OK);
  res = (int)c_rest_verify_password("pwd", "$pbkdf2-sha256$i=1000$sal~$hash");
  failed += (res == C_REST_OK);
  res =
      (int)c_rest_verify_password("pwd", "$pbkdf2-sha256$i=1000$c2FsdA==$has~");
  failed += (res == C_REST_OK);

  /* random_string_generate errors */
  res = (int)c_rest_random_string_generate(10, NULL);
  failed += (res == C_REST_OK);
  res = (int)c_rest_random_string_generate(0, &hash);
  failed += (res == C_REST_OK);

  /* tls_get_provider errors */
  res = (int)c_rest_tls_get_provider(NULL);
  failed += (res == C_REST_OK);

  /* jwt_sign_hs256 errors */
  res =
      (int)c_rest_jwt_sign_hs256(NULL, (const unsigned char *)"key", 3, &hash);
  failed += (res == C_REST_OK);
  res = (int)c_rest_jwt_sign_hs256("{}", NULL, 3, &hash);
  failed += (res == C_REST_OK);
  res = (int)c_rest_jwt_sign_hs256("{}", (const unsigned char *)"key", 3, NULL);
  failed += (res == C_REST_OK);

  /* jwt_verify_hs256 errors */
  res = (int)c_rest_jwt_verify_hs256(NULL, (const unsigned char *)"key", 3,
                                     &hash);
  failed += (res == C_REST_OK);
  res = (int)c_rest_jwt_verify_hs256("token", NULL, 3, &hash);
  failed += (res == C_REST_OK);
  res = (int)c_rest_jwt_verify_hs256("token", (const unsigned char *)"key", 3,
                                     NULL);
  failed += (res == C_REST_OK);
  res = (int)c_rest_jwt_verify_hs256("invalid.token",
                                     (const unsigned char *)"key", 3, &hash);
  failed += (res == C_REST_OK);
  res = (int)c_rest_jwt_verify_hs256("invalid", (const unsigned char *)"key", 3,
                                     &hash);
  failed += (res == C_REST_OK);
  res = (int)c_rest_jwt_verify_hs256("invalid.token.here",
                                     (const unsigned char *)"key", 3, &hash);
  failed += (res == C_REST_OK);

  {
    const unsigned char key[] = "key";
    const char *to_sign = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.sal~";
    unsigned char expected_sig[32];
    char encoded_sig[100];
    size_t encoded_sig_len;
    char token_buf[200];
    char *out_payload = NULL;

    c_rest_hmac_sha256(key, 3, (const unsigned char *)to_sign, strlen(to_sign),
                       expected_sig);
    c_rest_base64url_encode(expected_sig, 32, NULL, &encoded_sig_len);
    c_rest_base64url_encode(expected_sig, 32, encoded_sig, &encoded_sig_len);
    encoded_sig[encoded_sig_len] = '\0';
#if defined(_MSC_VER)
    sprintf_s(token_buf, sizeof(token_buf), "%s.%s", to_sign, encoded_sig);
#else
    sprintf(token_buf, "%s.%s", to_sign, encoded_sig);
#endif

    res = (int)c_rest_jwt_verify_hs256(token_buf, key, 3, &out_payload);
    failed += (res == C_REST_OK);
  }

  /* pbkdf2_hmac_sha256 errors */
  res = (int)c_rest_pbkdf2_hmac_sha256(NULL, 0, (const unsigned char *)"salt",
                                       4, 1, 32, dummy_hash); /* Dummy hash */
  failed += (res == C_REST_OK);
  res = (int)c_rest_pbkdf2_hmac_sha256((const unsigned char *)"pwd", 3, NULL, 0,
                                       1, 32, dummy_hash);
  failed += (res == C_REST_OK);
  res = (int)c_rest_pbkdf2_hmac_sha256((const unsigned char *)"pwd", 3,
                                       (const unsigned char *)"salt", 4, 1, 32,
                                       NULL);
  failed += (res == C_REST_OK);
  res = (int)c_rest_pbkdf2_hmac_sha256((const unsigned char *)"pwd", 3,
                                       (const unsigned char *)"salt", 4, 0, 32,
                                       dummy_hash);
  failed += (res == C_REST_OK);

  /* hmac_sha256 errors and large key */
  {
    unsigned char large_key[100];
    memset(large_key, 'A', 100);
    res = (int)c_rest_hmac_sha256(large_key, 100, (const unsigned char *)"data",
                                  4, dummy_hash);
    failed += (res != C_REST_OK);
  }
  res = (int)c_rest_hmac_sha256(NULL, 3, (const unsigned char *)"data", 4,
                                dummy_hash);
  failed += (res == C_REST_OK);
  res = (int)c_rest_hmac_sha256((const unsigned char *)"key", 3, NULL, 4,
                                dummy_hash);
  failed += (res == C_REST_OK);
  res = (int)c_rest_hmac_sha256((const unsigned char *)"key", 3,
                                (const unsigned char *)"data", 4, NULL);
  failed += (res == C_REST_OK);

  return failed;
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

static int test_crypto_malloc_failures(void) {
  char *hash = NULL;
  unsigned char dummy_hash[32];
  int res;
  int failed = 0;
  int i;
  enum c_rest_crypto_provider provider;

  /* Check tls_get_provider valid pointer */
  res = (int)c_rest_tls_get_provider(&provider);
  failed += (res != C_REST_OK);

  g_crf_malloc_hook = fail_malloc_n;

  {
    int sizes[] = {25, 45, 110};
    int j;
    for (j = 0; j < 3; j++) {
      g_malloc_fail_size = sizes[j];
      g_malloc_fail_after = 0;
      res = (int)c_rest_hash_password("pwd", C_REST_HASH_ALG_PBKDF2_SHA256,
                                      &hash);
      if (res == C_REST_OK)
        CRF_FREE(hash);
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
      CRF_FREE(hash);
  }
  g_malloc_fail_after = -1;
  /* Malloc failure when encoding hash/salt */
  for (i = 0; i < 20; i++) {
    g_malloc_fail_after = i;
    res =
        (int)c_rest_hash_password("pwd", C_REST_HASH_ALG_PBKDF2_SHA256, &hash);
    if (res == C_REST_OK)
      CRF_FREE(hash);
  }

  for (i = 0; i < 15; i++) {
    g_malloc_fail_after = i;
    res = (int)c_rest_jwt_sign_hs256("{}", (const unsigned char *)"key", 3,
                                     &hash);
    if (res == C_REST_OK)
      CRF_FREE(hash);
  }

  {
    char *valid_token = NULL;
    if (c_rest_jwt_sign_hs256("{\"sub\":\"123\"}", (const unsigned char *)"key",
                              3, &valid_token) == C_REST_OK) {
      for (i = 0; i < 10; i++) {
        g_malloc_fail_size = -1;
        g_malloc_fail_after = i;
        res = (int)c_rest_jwt_verify_hs256(
            valid_token, (const unsigned char *)"key", 3, &hash);
        if (res == C_REST_OK)
          CRF_FREE(hash);
      }
      CRF_FREE(valid_token);
    }
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

  return failed;
}
