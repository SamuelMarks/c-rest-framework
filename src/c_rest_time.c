/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_time.h"

#include <string.h>
#include <stdio.h>
/* clang-format on */

static const char *const wdays[] = {"Sun", "Mon", "Tue", "Wed",
                                    "Thu", "Fri", "Sat"};
static const char *const months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                     "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

#define IS_LEAP_YEAR(y) (((y) % 4 == 0 && ((y) % 100 != 0 || (y) % 400 == 0)))

static NO_DISCARD c_rest_error_t portable_timegm(struct tm *tm,
                                                 time_t *out_time) {
  static const int days_before_month[2][12] = {
      {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334},
      {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335}};
  int year = tm->tm_year + 1900;
  int month = tm->tm_mon;
  int leap = 0;
  time_t days = 0;
  int y;

  if (year < 1970) {
    *out_time = (time_t)-1;
    return C_REST_ERROR_GENERIC;
  }

  for (y = 1970; y < year; y++) {
    days += (time_t)(365 + IS_LEAP_YEAR(y));
  }

  leap = IS_LEAP_YEAR(year);
  days += (time_t)days_before_month[leap][month];
  days += (time_t)(tm->tm_mday - 1);

  *out_time = days * (time_t)86400 + (time_t)tm->tm_hour * (time_t)3600 +
              (time_t)tm->tm_min * (time_t)60 + (time_t)tm->tm_sec;
  return C_REST_OK;
}

c_rest_error_t c_rest_http_date_format(time_t t, char *out_str,
                                       size_t out_len) {
  struct tm *tm_info;
#if defined(_MSC_VER)
  struct tm tm_buf;
  if (!out_str || out_len < 30) {
    return C_REST_ERROR_GENERIC;
  }
  if (gmtime_s(&tm_buf, &t) != 0) {
    return C_REST_ERROR_GENERIC;
  }
  tm_info = &tm_buf;
#else
  if (!out_str || out_len < 30) {
    return C_REST_ERROR_GENERIC;
  }
  tm_info = gmtime(&t);
  if (!tm_info) {
    return C_REST_ERROR_GENERIC;
  }
#endif

  /* Format: Sun, 06 Nov 1994 08:49:37 GMT */
#if defined(_MSC_VER)
  sprintf_s(out_str, out_len, "%s, %02d %s %04d %02d:%02d:%02d GMT",
            wdays[tm_info->tm_wday], tm_info->tm_mday, months[tm_info->tm_mon],
            tm_info->tm_year + 1900, tm_info->tm_hour, tm_info->tm_min,
            tm_info->tm_sec);
#else
  sprintf(out_str, "%s, %02d %s %04d %02d:%02d:%02d GMT",
          wdays[tm_info->tm_wday], tm_info->tm_mday, months[tm_info->tm_mon],
          tm_info->tm_year + 1900, tm_info->tm_hour, tm_info->tm_min,
          tm_info->tm_sec);
#endif

  return C_REST_OK;
}

c_rest_error_t c_rest_http_date_parse(const char *date_str, time_t *out_t) {
  struct tm tm_info;
  char wday[4];
  char month[4];
  int mday, year, hour, min, sec;
  int i;
  int mon = -1;

  if (!date_str || !out_t) {
    return C_REST_ERROR_GENERIC;
  }

  memset(&tm_info, 0, sizeof(tm_info));

  /* Format: Sun, 06 Nov 1994 08:49:37 GMT */
#if defined(_MSC_VER)
  if (sscanf_s(date_str, "%3s, %d %3s %d %d:%d:%d GMT", wday,
               (unsigned int)sizeof(wday), &mday, month,
               (unsigned int)sizeof(month), &year, &hour, &min, &sec) != 7) {
    return C_REST_ERROR_GENERIC;
  }
#else
  if (sscanf(date_str, "%3s, %d %3s %d %d:%d:%d GMT", wday, &mday, month, &year,
             &hour, &min, &sec) != 7) {
    return C_REST_ERROR_GENERIC;
  }
#endif

  for (i = 0; i < 12; i++) {
    if (strcmp(month, months[i]) == 0) {
      mon = i;
      break;
    }
  }

  if (mon == -1 || mday < 1 || mday > 31 || hour < 0 || hour > 23 || min < 0 ||
      min > 59 || sec < 0 || sec > 60) {
    return C_REST_ERROR_GENERIC;
  }

  tm_info.tm_year = year - 1900;
  tm_info.tm_mon = mon;
  tm_info.tm_mday = mday;
  tm_info.tm_hour = hour;
  tm_info.tm_min = min;
  tm_info.tm_sec = sec;
  tm_info.tm_isdst = 0; /* GMT */

  {
    c_rest_error_t rc = portable_timegm(&tm_info, out_t);
    if (rc != C_REST_OK) {
      return rc;
    }
  }

  return C_REST_OK;
}
