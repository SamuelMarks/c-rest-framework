/* clang-format off */
#include "c_rest_str_utils.h"
#include "c_rest_log.h"

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "c_rest_mem.h"
#include "c_rest_log.h"
/* clang-format on */

int c_rest_strcasecmp(const char *s1, const char *s2, int *out_cmp) {
  if (!s1 || !s2 || !out_cmp) {
    LOG_DEBUG("c_rest_strcasecmp: invalid arguments");
    return 1;
  }
  while (*s1 && *s2) {
    int c1 = tolower((unsigned char)*s1);
    int c2 = tolower((unsigned char)*s2);
    if (c1 != c2) {
      *out_cmp = c1 - c2;
      return 0;
    }
    s1++;
    s2++;
  }
  *out_cmp = tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
  return 0;
}

int c_rest_strncasecmp(const char *s1, const char *s2, size_t n, int *out_cmp) {
  if (!s1 || !s2 || !out_cmp) {
    LOG_DEBUG("c_rest_strncasecmp: invalid arguments");
    return 1;
  }
  if (n == 0) {
    *out_cmp = 0;
    return 0;
  }
  while (n-- > 0 && *s1 && *s2) {
    int c1 = tolower((unsigned char)*s1);
    int c2 = tolower((unsigned char)*s2);
    if (c1 != c2) {
      *out_cmp = c1 - c2;
      return 0;
    }
    s1++;
    s2++;
  }
  if (n == (size_t)-1) {
    *out_cmp = 0;
    return 0;
  }
  *out_cmp = tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
  return 0;
}

int c_rest_strlcpy(char *dst, const char *src, size_t dsize, size_t *out_len) {
  size_t src_len;
  size_t copy_len;

  if (!out_len) {
    LOG_DEBUG("c_rest_strlcpy: invalid out_len");
    return 1;
  }
  *out_len = 0;

  if (!dst || !src) {
    LOG_DEBUG("c_rest_strlcpy: invalid dst or src");
    return 1;
  }

  src_len = strlen(src);
  if (dsize == 0) {
    *out_len = src_len;
    return 0;
  }

  copy_len = src_len;
  if (copy_len >= dsize) {
    copy_len = dsize - 1;
  }

  memcpy(dst, src, copy_len);
  dst[copy_len] = '\0';

  *out_len = src_len;
  return 0;
}

int c_rest_strlcat(char *dst, const char *src, size_t dsize, size_t *out_len) {
  size_t dst_len;
  size_t src_len;
  size_t space_left;
  size_t copy_len;

  if (!out_len) {
    LOG_DEBUG("c_rest_strlcat: invalid out_len");
    return 1;
  }
  *out_len = 0;

  if (!dst || !src) {
    LOG_DEBUG("c_rest_strlcat: invalid dst or src");
    return 1;
  }

  dst_len = strlen(dst);
  src_len = strlen(src);

  if (dsize <= dst_len) {
    *out_len = dsize + src_len;
    return 0;
  }

  space_left = dsize - dst_len - 1;
  copy_len = src_len;
  if (copy_len > space_left) {
    copy_len = space_left;
  }

  memcpy(dst + dst_len, src, copy_len);
  dst[dst_len + copy_len] = '\0';

  *out_len = dst_len + src_len;
  return 0;
}

int c_rest_url_decode(char *dst, const char *src, size_t len) {
  size_t i;
  char *p;
  
  if (!dst || !src) {
    LOG_DEBUG("c_rest_url_decode: invalid dst or src");
    return 1;
  }
  
  p = dst;
  for (i = 0; i < len; i++) {
    if (src[i] == '%') {
      if (i + 2 < len) {
        int v;
        char hex[3];
        hex[0] = src[i + 1];
        hex[1] = src[i + 2];
        hex[2] = '\0';
        v = (int)strtol(hex, NULL, 16);
        *p++ = (char)v;
        i += 2;
      } else {
        *p++ = src[i];
      }
    } else if (src[i] == '+') {
      *p++ = ' ';
    } else {
      *p++ = src[i];
    }
  }
  *p = '\0';
  return 0;
}
