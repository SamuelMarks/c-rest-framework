
#include "c_rest_str_utils.h"
#include "c_rest_error.h"
#include "c_rest_log.h"
#include "c_rest_mem.h"

#include "c_rest_log.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

c_rest_error_t c_rest_strcasecmp(const char *s1, const char *s2, int *out_cmp) {
  int done = 0;
  if (!s1 || !s2 || !out_cmp) {
    LOG_DEBUG("c_rest_strcasecmp: invalid arguments");
    return C_REST_ERROR_GENERIC;
  }
  while (!done) {
    if (*s1 == '\0' || *s2 == '\0') {
      done = 1;
    } else {
      int c1 = tolower((unsigned char)*s1);
      int c2 = tolower((unsigned char)*s2);
      if (c1 != c2) {
        *out_cmp = c1 - c2;
        return C_REST_OK;
      }
      s1++;
      s2++;
    }
  }
  *out_cmp = tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
  return C_REST_OK;
}

c_rest_error_t c_rest_strncasecmp(const char *s1, const char *s2, size_t n,
                                  int *out_cmp) {
  int done = 0;
  if (!s1 || !s2 || !out_cmp) {
    LOG_DEBUG("c_rest_strncasecmp: invalid arguments");
    return C_REST_ERROR_GENERIC;
  }
  if (n == 0) {
    *out_cmp = 0;
    return C_REST_OK;
  }
  while (!done) {
    if (n == 0 || *s1 == '\0' || *s2 == '\0') {
      done = 1;
    } else {
      int c1 = tolower((unsigned char)*s1);
      int c2 = tolower((unsigned char)*s2);
      if (c1 != c2) {
        *out_cmp = c1 - c2;
        return C_REST_OK;
      }
      s1++;
      s2++;
      n--;
    }
  }

  if (n == 0) {
    *out_cmp = 0;
    return C_REST_OK;
  }

  *out_cmp = tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
  return C_REST_OK;
}

c_rest_error_t c_rest_strlcpy(char *dst, const char *src, size_t dsize,
                              size_t *out_len) {
  size_t src_len;
  size_t copy_len;

  if (!out_len) {
    LOG_DEBUG("c_rest_strlcpy: invalid out_len");
    return C_REST_ERROR_GENERIC;
  }
  *out_len = 0;

  if (!dst || !src) {
    LOG_DEBUG("c_rest_strlcpy: invalid dst or src");
    return C_REST_ERROR_GENERIC;
  }

  src_len = strlen(src);
  if (dsize == 0) {
    *out_len = src_len;
    return C_REST_OK;
  }

  copy_len = src_len;
  if (copy_len >= dsize) {
    copy_len = dsize - 1;
  }

#if defined(_MSC_VER)
  /* CDD_SAFE_CRT */ memcpy_s(dst, copy_len, src, copy_len);
#else
  memcpy(dst, src, copy_len);
#endif
  dst[copy_len] = '\0';

  *out_len = src_len;
  return C_REST_OK;
}

c_rest_error_t c_rest_strlcat(char *dst, const char *src, size_t dsize,
                              size_t *out_len) {
  size_t dst_len;
  size_t src_len;
  size_t space_left;
  size_t copy_len;

  if (!out_len) {
    LOG_DEBUG("c_rest_strlcat: invalid out_len");
    return C_REST_ERROR_GENERIC;
  }
  *out_len = 0;

  if (!dst || !src) {
    LOG_DEBUG("c_rest_strlcat: invalid dst or src");
    return C_REST_ERROR_GENERIC;
  }

  dst_len = strlen(dst);
  src_len = strlen(src);

  if (dsize <= dst_len) {
    *out_len = dsize + src_len;
    return C_REST_OK;
  }

  space_left = dsize - dst_len - 1;
  copy_len = src_len;
  if (copy_len > space_left) {
    copy_len = space_left;
  }

#if defined(_MSC_VER)
  /* CDD_SAFE_CRT */ memcpy_s(dst + dst_len, copy_len, src, copy_len);
#else
  memcpy(dst + dst_len, src, copy_len);
#endif
  dst[dst_len + copy_len] = '\0';

  *out_len = dst_len + src_len;
  return C_REST_OK;
}

c_rest_error_t c_rest_url_decode(char *dst, const char *src, size_t len) {
  size_t i;
  char *p;

  if (!dst || !src) {
    LOG_DEBUG("c_rest_url_decode: invalid dst or src");
    return C_REST_ERROR_GENERIC;
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
  return C_REST_OK;
}

/* clang-format off */
#include <stdarg.h>
#include <stdio.h>
/* clang-format on */

c_rest_error_t c_rest_sprintf_s(char *buffer, size_t sizeOfBuffer,
                                const char *format, ...) {
  int ret;
  va_list args;
  va_start(args, format);
#if defined(_MSC_VER)
  ret = vsprintf_s(buffer, sizeOfBuffer, format, args);
#else
  (void)sizeOfBuffer;
  ret = vsprintf(buffer, format, args);
#endif
  va_end(args);
  return (ret >= 0) ? C_REST_OK : C_REST_ERROR_INVALID_ARG;
}

c_rest_error_t c_rest_strcpy_s(char *dest, size_t dest_size, const char *src) {
#if defined(_MSC_VER)
  return (strcpy_s(dest, dest_size, src) == 0) ? C_REST_OK
                                               : C_REST_ERROR_INVALID_ARG;
  (dest, dest_size, src);
#else
  (void)dest_size;
  strcpy(dest, src);
  return C_REST_OK;
#endif
}

c_rest_error_t c_rest_strncpy_s(char *dest, size_t dest_size, const char *src,
                                size_t count) {
#if defined(_MSC_VER)
  return (strncpy_s(dest, dest_size, src, count) == 0)
             ? C_REST_OK
             : C_REST_ERROR_INVALID_ARG;
  (dest, dest_size, src, count);
#else
  (void)dest_size;
  strncpy(dest, src, count);
  return C_REST_OK;
#endif
}
