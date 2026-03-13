/* clang-format off */
#include "c_rest_str_utils.h"

#include <ctype.h>
#include <string.h>
/* clang-format on */

int c_rest_strcasecmp(const char *s1, const char *s2) {
  if (!s1 || !s2)
    return -1;
  while (*s1 && *s2) {
    int c1 = tolower((unsigned char)*s1);
    int c2 = tolower((unsigned char)*s2);
    if (c1 != c2) {
      return c1 - c2;
    }
    s1++;
    s2++;
  }
  return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}

int c_rest_strncasecmp(const char *s1, const char *s2, size_t n) {
  if (!s1 || !s2 || n == 0)
    return 0;
  while (n-- > 0 && *s1 && *s2) {
    int c1 = tolower((unsigned char)*s1);
    int c2 = tolower((unsigned char)*s2);
    if (c1 != c2) {
      return c1 - c2;
    }
    s1++;
    s2++;
  }
  if (n == (size_t)-1) {
    return 0;
  }
  return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}

size_t c_rest_strlcpy(char *dst, const char *src, size_t dsize) {
  size_t src_len;
  size_t copy_len;

  if (!dst || !src)
    return 0;

  src_len = strlen(src);
  if (dsize == 0)
    return src_len;

  copy_len = src_len;
  if (copy_len >= dsize) {
    copy_len = dsize - 1;
  }

  memcpy(dst, src, copy_len);
  dst[copy_len] = '\0';

  return src_len;
}

size_t c_rest_strlcat(char *dst, const char *src, size_t dsize) {
  size_t dst_len;
  size_t src_len;
  size_t space_left;
  size_t copy_len;

  if (!dst || !src)
    return 0;

  dst_len = strlen(dst);
  src_len = strlen(src);

  if (dsize <= dst_len) {
    return dsize + src_len;
  }

  space_left = dsize - dst_len - 1;
  copy_len = src_len;
  if (copy_len > space_left) {
    copy_len = space_left;
  }

  memcpy(dst + dst_len, src, copy_len);
  dst[dst_len + copy_len] = '\0';

  return dst_len + src_len;
}
