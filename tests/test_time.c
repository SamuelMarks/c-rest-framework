/* clang-format off */
#include "c_rest_error.h"
#include "test_protos.h"
#include "c_rest_time.h"
#include "c_rest_platform.h"
#include <string.h>
#include <time.h>
/* clang-format on */

TEST test_http_date_format_and_parse(void) {
  time_t t;
  char buf[64];

  t = 784111777; /* Sun, 06 Nov 1994 08:49:37 GMT */
  ASSERT_EQ(C_REST_OK, c_rest_http_date_format(t, buf, sizeof(buf)));
  ASSERT_STR_EQ("Sun, 06 Nov 1994 08:49:37 GMT", buf);

  ASSERT_EQ(C_REST_OK, c_rest_http_date_parse(buf, &t));
  ASSERT_EQ((time_t)784111777, t);

  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_http_date_format(t, NULL, sizeof(buf)));
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_http_date_format(t, buf, 10));
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_http_date_parse(NULL, &t));
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_http_date_parse(buf, NULL));
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_http_date_parse("Invalid format string GMT", &t));
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_http_date_parse("Sun, 06 Xxx 1994 08:49:37 GMT", &t));
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_http_date_parse("Sun, 06 Nov 1969 08:49:37 GMT", &t));
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_http_date_parse("Sun, 00 Nov 1994 08:49:37 GMT", &t));
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_http_date_parse("Sun, 32 Nov 1994 08:49:37 GMT", &t));
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_http_date_parse("Sun, 06 Nov 1994 -1:49:37 GMT", &t));
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_http_date_parse("Sun, 06 Nov 1994 24:49:37 GMT", &t));
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_http_date_parse("Sun, 06 Nov 1994 08:-1:37 GMT", &t));
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_http_date_parse("Sun, 06 Nov 1994 08:60:37 GMT", &t));
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_http_date_parse("Sun, 06 Nov 1994 08:49:-1 GMT", &t));
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_http_date_parse("Sun, 06 Nov 1994 08:49:61 GMT", &t));

  ASSERT_EQ(C_REST_OK,
            c_rest_http_date_parse("Sun, 06 Nov 1973 08:49:37 GMT", &t));
  ASSERT_EQ(C_REST_OK,
            c_rest_http_date_parse("Sun, 06 Nov 2100 08:49:37 GMT", &t));
  ASSERT_EQ(C_REST_OK,
            c_rest_http_date_parse("Sun, 06 Nov 2101 08:49:37 GMT", &t));
  ASSERT_EQ(C_REST_OK,
            c_rest_http_date_parse("Sun, 06 Nov 2000 08:49:37 GMT", &t));
  ASSERT_EQ(C_REST_OK,
            c_rest_http_date_parse("Sun, 06 Nov 2004 08:49:37 GMT", &t));

  t = (time_t)-1;
  ASSERT_EQ(C_REST_OK, c_rest_http_date_format(t, buf, sizeof(buf)));
  t = (time_t)(((time_t)1 << (sizeof(time_t) * 8 - 2)) - 1);
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_http_date_format(t, buf, sizeof(buf)));

  PASS();
}

SUITE(time_suite) { RUN_TEST(test_http_date_format_and_parse); }
