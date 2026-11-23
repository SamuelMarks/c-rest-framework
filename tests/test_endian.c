/* clang-format off */
#include "c_rest_endian.h"
#include "test_protos.h"
/* clang-format on */

static c_rest_error_t mock_is_little_endian(int *out_is_little) {
  (void)out_is_little;
  return C_REST_ERROR_GENERIC;
}

TEST test_endian_conversions_and_errors(void) {
  unsigned short s = 0x1234;
  unsigned long l = 0x12345678UL;
  unsigned short s_out = 0;
  unsigned long l_out = 0;

  ASSERT_EQ(C_REST_OK, c_rest_htons(s, &s_out));
  ASSERT_EQ(C_REST_OK, c_rest_ntohs(s_out, &s_out));
  ASSERT_EQ(s, s_out);

  ASSERT_EQ(C_REST_OK, c_rest_htonl(l, &l_out));
  ASSERT_EQ(C_REST_OK, c_rest_ntohl(l_out, &l_out));
  ASSERT_EQ(l, l_out);

  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_htons(s, NULL));
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_htonl(l, NULL));
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_ntohs(s_out, NULL));
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_ntohl(l_out, NULL));

  g_crf_is_little_endian_hook = mock_is_little_endian;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_htons(s, &s_out));
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_htonl(l, &l_out));
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_ntohs(s_out, &s_out));
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_ntohl(l_out, &l_out));
  g_crf_is_little_endian_hook = NULL;

  PASS();
}

SUITE(endian_suite) { RUN_TEST(test_endian_conversions_and_errors); }
