/* clang-format off */
#include "c_rest_crypto.h"
#include <stdio.h>
#include <string.h>
/* clang-format on */

int test_crypto(void);

int test_crypto(void) {
  const unsigned char data[] = "hello world";
  unsigned char sha1_hash[20];
  unsigned char sha256_hash[32];
  unsigned char rand_buf[16];
  int res;

  const unsigned char expected_sha1[20] = {
      0x2a, 0xae, 0x6c, 0x35, 0xc9, 0x4f, 0xcf, 0xb4, 0x15, 0xdb,
      0xe9, 0x5f, 0x40, 0x8b, 0x9c, 0xe9, 0x1e, 0xe8, 0x46, 0xed};

  const unsigned char expected_sha256[32] = {
      0xb9, 0x4d, 0x27, 0xb9, 0x93, 0x4d, 0x3e, 0x08, 0xa5, 0x2e, 0x52,
      0xd7, 0xda, 0x7d, 0xab, 0xfa, 0xc4, 0x84, 0xef, 0xe3, 0x7a, 0x53,
      0x80, 0xee, 0x90, 0x88, 0xf7, 0xac, 0xe2, 0xef, 0xcd, 0xe9};

  res = c_rest_sha1(data, 11, sha1_hash);
  if (res != 0) {
    printf("c_rest_sha1 failed\n");
    return 1;
  }

  if (memcmp(sha1_hash, expected_sha1, 20) != 0) {
    printf("SHA1 mismatch\n");
    return 1;
  }

  res = c_rest_sha256(data, 11, sha256_hash);
  if (res != 0) {
    printf("c_rest_sha256 failed\n");
    return 1;
  }

  if (memcmp(sha256_hash, expected_sha256, 32) != 0) {
    printf("SHA256 mismatch\n");
    return 1;
  }

  res = c_rest_rand_bytes(rand_buf, sizeof(rand_buf));
  if (res != 0) {
    printf("c_rest_rand_bytes failed\n");
    return 1;
  }

  return 0;
}
