/* clang-format off */
#include "c_rest_endian.h"
#include "test_protos.h"
#include <stdio.h>
/* clang-format on */

int test_endian(void) {
  unsigned short s = 0x1234;
  unsigned long l = 0x12345678;
  unsigned short s_out = 0;
  unsigned long l_out = 0;
  int failed = 0;
  c_rest_error_t rc;

  rc = c_rest_htons(s, &s_out);
  if (rc != C_REST_OK)
    failed++;
  rc = c_rest_ntohs(s_out, &s_out);
  if (rc != C_REST_OK || s_out != s)
    failed++;

  rc = c_rest_htonl(l, &l_out);
  if (rc != C_REST_OK)
    failed++;
  rc = c_rest_ntohl(l_out, &l_out);
  if (rc != C_REST_OK || l_out != l)
    failed++;

  if (failed) {
    printf("test_endian failed\n");
  } else {
    printf("test_endian passed\n");
  }

  return failed;
}
