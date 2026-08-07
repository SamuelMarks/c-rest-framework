/* clang-format off */
#include "c_rest_endian.h"
#include "test_protos.h"
#include <stdio.h>
/* clang-format on */

extern c_rest_error_t (*g_crf_is_little_endian_hook)(int *);

static c_rest_error_t mock_is_little_endian(int *out_is_little) {
  (void)out_is_little;
  return C_REST_ERROR_GENERIC;
}

int test_endian(void) {
  unsigned short s = 0x1234;
  unsigned long l = 0x12345678;
  unsigned short s_out = 0;
  unsigned long l_out = 0;
  int failed = 0;
  c_rest_error_t rc;
  const char *msgs[2];

  rc = c_rest_htons(s, &s_out);
  failed += (rc != C_REST_OK);

  rc = c_rest_ntohs(s_out, &s_out);
  failed += (rc != C_REST_OK);
  failed += (s_out != s);

  rc = c_rest_htonl(l, &l_out);
  failed += (rc != C_REST_OK);

  rc = c_rest_ntohl(l_out, &l_out);
  failed += (rc != C_REST_OK);
  failed += (l_out != l);

  rc = c_rest_htons(s, NULL);
  failed += (rc != C_REST_ERROR_GENERIC);

  rc = c_rest_htonl(l, NULL);
  failed += (rc != C_REST_ERROR_GENERIC);

  rc = c_rest_ntohs(s_out, NULL);
  failed += (rc != C_REST_ERROR_GENERIC);

  rc = c_rest_ntohl(l_out, NULL);
  failed += (rc != C_REST_ERROR_GENERIC);

  g_crf_is_little_endian_hook = mock_is_little_endian;

  rc = c_rest_htons(s, &s_out);
  failed += (rc != C_REST_ERROR_GENERIC);

  rc = c_rest_htonl(l, &l_out);
  failed += (rc != C_REST_ERROR_GENERIC);

  rc = c_rest_ntohs(s_out, &s_out);
  failed += (rc != C_REST_ERROR_GENERIC);

  rc = c_rest_ntohl(l_out, &l_out);
  failed += (rc != C_REST_ERROR_GENERIC);

  g_crf_is_little_endian_hook = NULL;

  msgs[0] = "test_endian passed\n";
  msgs[1] = "test_endian failed\n";
  printf("%s", msgs[failed != 0]);

  return failed;
}
