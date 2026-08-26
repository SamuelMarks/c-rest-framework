/* clang-format off */
#include "c_rest_error.h"
#include "test_protos.h"
#include "c_rest_time.h"
#include "c_rest_platform.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
/* clang-format on */

int test_time(void);

int test_time(void) {
  time_t t;
  char buf[64];
  int res;
  int failed = 0;

  printf("Testing HTTP Date format and parse...\n");

  t = 784111777; /* Sun, 06 Nov 1994 08:49:37 GMT */
  res = (int)(int)c_rest_http_date_format(t, buf, sizeof(buf));
  failed += (res != C_REST_OK);
  failed += (strcmp(buf, "Sun, 06 Nov 1994 08:49:37 GMT") != 0);

  res = (int)(int)c_rest_http_date_parse(buf, &t);
  failed += (res != C_REST_OK);
  failed += (t != 784111777);

  res = (int)(int)c_rest_http_date_format(t, NULL, sizeof(buf));
  failed += (res != C_REST_ERROR_GENERIC);

  res = (int)(int)c_rest_http_date_format(t, buf, 10);
  failed += (res != C_REST_ERROR_GENERIC);

  res = (int)(int)c_rest_http_date_parse(NULL, &t);
  failed += (res != C_REST_ERROR_GENERIC);

  res = (int)(int)c_rest_http_date_parse(buf, NULL);
  failed += (res != C_REST_ERROR_GENERIC);

  res = (int)(int)c_rest_http_date_parse("Invalid format string GMT", &t);
  failed += (res != C_REST_ERROR_GENERIC);

  res = (int)(int)c_rest_http_date_parse("Sun, 06 Xxx 1994 08:49:37 GMT", &t);
  failed += (res != C_REST_ERROR_GENERIC);

  res = (int)(int)c_rest_http_date_parse("Sun, 06 Nov 1969 08:49:37 GMT", &t);
  failed += (res != C_REST_ERROR_GENERIC); /* year < 1970 */

  /* Year after leap year */
  res = (int)(int)c_rest_http_date_parse("Sun, 06 Nov 1973 08:49:37 GMT", &t);
  failed += (res != C_REST_OK);

  /* Leap year logic:
     divisible by 4 => wait, 1996 is leap year? Yes.
     divisible by 100 but not 400 => not leap year (e.g. 1900, 2100) -> 2100 is
     > 1970 divisible by 400 => leap year (e.g. 2000)
  */

  res = (int)(int)c_rest_http_date_parse("Sun, 06 Nov 2100 08:49:37 GMT",
                                         &t); /* Not leap, covers % 100 != 0 */
  failed += (res != C_REST_OK);

  res = (int)(int)c_rest_http_date_parse("Sun, 06 Nov 2000 08:49:37 GMT",
                                         &t); /* Leap year covers % 400 == 0 */
  failed += (res != C_REST_OK);

  res = (int)c_rest_http_date_parse(
      "Sun, 06 Nov 2004 08:49:37 GMT",
      &t); /* Leap year covers % 4 == 0 and % 100 != 0 */
  failed += (res != C_REST_OK);

  /* Test gmtime failure */
  t = (time_t)-1;
  res = (int)(int)c_rest_http_date_format(t, buf, sizeof(buf));
  if (res == C_REST_OK) {
    /* Some systems might succeed with -1, try a huge positive value */
    t = (time_t)(((time_t)1 << (sizeof(time_t) * 8 - 2)) - 1);
    res = (int)(int)c_rest_http_date_format(t, buf, sizeof(buf));
    if (res == C_REST_OK) {
      printf("Failed to trigger gmtime failure\n");
    }
  }

  if (failed) {
    printf("test_time failed\n");
  } else {
    printf("test_time finished.\n");
  }
  return failed;
}
