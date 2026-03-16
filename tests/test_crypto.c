/* clang-format off */
#include "c_rest_crypto.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* clang-format on */

int test_crypto(void);

int test_crypto(void) {
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

  printf("Testing SHA1...\n");
  res = c_rest_sha1(data, 11, sha1_hash);
  if (res != 0) {
    printf("c_rest_sha1 failed\n");
    return 1;
  }

  if (memcmp(sha1_hash, expected_sha1, 20) != 0) {
    printf("SHA1 mismatch\n");
    return 1;
  }

  printf("Testing SHA256...\n");
  res = c_rest_sha256(data, 11, sha256_hash);
  if (res != 0) {
    printf("c_rest_sha256 failed\n");
    return 1;
  }

  if (memcmp(sha256_hash, expected_sha256, 32) != 0) {
    printf("SHA256 mismatch\n");
    return 1;
  }

  printf("Testing rand_bytes...\n");
  res = c_rest_rand_bytes(rand_buf, sizeof(rand_buf));
  if (res != 0) {
    printf("c_rest_rand_bytes failed\n");
    return 1;
  }

  printf("Testing HMAC...\n");
  res = c_rest_hmac_sha256(hmac_key, 6, data, 11, hmac_hash);
  if (res != 0) {
    printf("c_rest_hmac_sha256 failed\n");
    return 1;
  }

  if (memcmp(hmac_hash, expected_hmac, 32) != 0) {
    printf("HMAC mismatch\n");
    return 1;
  }

  printf("Testing JWT sign...\n");
  res = c_rest_jwt_sign_hs256("{\"sub\":\"123\"}", hmac_key, 6, &jwt_token);
  if (res != 0 || !jwt_token) {
    printf("JWT sign failed\n");
    return 1;
  }

  printf("Testing JWT verify...\n");
  res = c_rest_jwt_verify_hs256(jwt_token, hmac_key, 6, &jwt_payload);
  if (res != 0 || !jwt_payload) {
    printf("JWT verify failed\n");
    return 1;
  }

  if (strcmp(jwt_payload, "{\"sub\":\"123\"}") != 0) {
    printf("JWT payload mismatch\n");
    return 1;
  }

  free(jwt_token);
  free(jwt_payload);

  printf("Testing PBKDF2-HMAC-SHA256...\n");
  {
    const unsigned char pwd[] = "password";
    const unsigned char salt[] = "salt";
    unsigned char dk[32];
    res = c_rest_pbkdf2_hmac_sha256(pwd, 8, salt, 4, 1, 32, dk);
    if (res != 0) {
      printf("PBKDF2 failed\n");
      return 1;
    }

    {
      char *rand_str = NULL;
      res = c_rest_random_string_generate(32, &rand_str);
      if (res != 0 || !rand_str) {
        printf("c_rest_random_string_generate failed\n");
        return 1;
      }
      free(rand_str);
    }
  }

  printf("test_crypto finished.\n");

  return 0;
}
