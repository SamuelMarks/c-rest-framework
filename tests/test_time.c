/* clang-format off */
#include "c_rest_time.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
/* clang-format on */

int test_time(void);

int test_time(void) {
  time_t t;
  char buf[64];
  int res;

  printf("Testing HTTP Date format and parse...\n");

  t = 784111777; /* Sun, 06 Nov 1994 08:49:37 GMT */
  res = c_rest_http_date_format(t, buf, sizeof(buf));
  if (res != 0) {
    printf("c_rest_http_date_format failed\n");
    return 1;
  }

  if (strcmp(buf, "Sun, 06 Nov 1994 08:49:37 GMT") != 0) {
    printf("Formatted date mismatch: %s\n", buf);
    return 1;
  }

  res = c_rest_http_date_parse(buf, &t);
  if (res != 0) {
    printf("c_rest_http_date_parse failed\n");
    return 1;
  }

  if (t != 784111777) {
    printf("Parsed time mismatch: %ld\n", (long)t);
    return 1;
  }

  printf("test_time finished.\n");
  return 0;
}
