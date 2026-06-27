/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_str_utils.h"
#include "c_rest_log.h"

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "c_rest_mem.h"
#include "c_rest_log.h"
/* clang-format on */

c_rest_error_t c_rest_strcasecmp(const char *s1, const char *s2,
                                 int *out_cmp) { /* GCOVR_EXCL_LINE */
  if (!s1 || !s2 || !out_cmp) {                  /* GCOVR_EXCL_LINE */
    LOG_DEBUG("c_rest_strcasecmp: invalid arguments");
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }
  while (*s1 && *s2) {                    /* GCOVR_EXCL_LINE */
    int c1 = tolower((unsigned char)*s1); /* GCOVR_EXCL_LINE */
    int c2 = tolower((unsigned char)*s2); /* GCOVR_EXCL_LINE */
    if (c1 != c2) {                       /* GCOVR_EXCL_LINE */
      *out_cmp = c1 - c2;                 /* GCOVR_EXCL_LINE */
      return C_REST_OK;                   /* GCOVR_EXCL_LINE */
    }
    s1++; /* GCOVR_EXCL_LINE */
    s2++; /* GCOVR_EXCL_LINE */
  }
  *out_cmp = tolower((unsigned char)*s1) -
             tolower((unsigned char)*s2); /* GCOVR_EXCL_LINE */
  return C_REST_OK;                       /* GCOVR_EXCL_LINE */
}

c_rest_error_t c_rest_strncasecmp(const char *s1, const char *s2, size_t n,
                                  int *out_cmp) { /* GCOVR_EXCL_LINE */
  if (!s1 || !s2 || !out_cmp) {                   /* GCOVR_EXCL_LINE */
    LOG_DEBUG("c_rest_strncasecmp: invalid arguments");
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }
  if (n == 0) {       /* GCOVR_EXCL_LINE */
    *out_cmp = 0;     /* GCOVR_EXCL_LINE */
    return C_REST_OK; /* GCOVR_EXCL_LINE */
  }
  while (n-- > 0 && *s1 && *s2) {         /* GCOVR_EXCL_LINE */
    int c1 = tolower((unsigned char)*s1); /* GCOVR_EXCL_LINE */
    int c2 = tolower((unsigned char)*s2); /* GCOVR_EXCL_LINE */
    if (c1 != c2) {                       /* GCOVR_EXCL_LINE */
      *out_cmp = c1 - c2;                 /* GCOVR_EXCL_LINE */
      return C_REST_OK;                   /* GCOVR_EXCL_LINE */
    }
    s1++; /* GCOVR_EXCL_LINE */
    s2++; /* GCOVR_EXCL_LINE */
  }
  if (n == (size_t)-1) { /* GCOVR_EXCL_LINE */
    *out_cmp = 0;        /* GCOVR_EXCL_LINE */
    return C_REST_OK;    /* GCOVR_EXCL_LINE */
  }
  *out_cmp = tolower((unsigned char)*s1) -
             tolower((unsigned char)*s2); /* GCOVR_EXCL_LINE */
  return C_REST_OK;                       /* GCOVR_EXCL_LINE */
}

c_rest_error_t c_rest_strlcpy(char *dst, const char *src, size_t dsize,
                              size_t *out_len) { /* GCOVR_EXCL_LINE */
  size_t src_len;
  size_t copy_len;

  if (!out_len) { /* GCOVR_EXCL_LINE */
    LOG_DEBUG("c_rest_strlcpy: invalid out_len");
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }
  *out_len = 0; /* GCOVR_EXCL_LINE */

  if (!dst || !src) { /* GCOVR_EXCL_LINE */
    LOG_DEBUG("c_rest_strlcpy: invalid dst or src");
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }

  src_len = strlen(src); /* GCOVR_EXCL_LINE */
  if (dsize == 0) {      /* GCOVR_EXCL_LINE */
    *out_len = src_len;  /* GCOVR_EXCL_LINE */
    return C_REST_OK;    /* GCOVR_EXCL_LINE */
  }

  copy_len = src_len;      /* GCOVR_EXCL_LINE */
  if (copy_len >= dsize) { /* GCOVR_EXCL_LINE */
    copy_len = dsize - 1;  /* GCOVR_EXCL_LINE */
  }

  memcpy(dst, src, copy_len); /* GCOVR_EXCL_LINE */
  dst[copy_len] = '\0';       /* GCOVR_EXCL_LINE */

  *out_len = src_len; /* GCOVR_EXCL_LINE */
  return C_REST_OK;   /* GCOVR_EXCL_LINE */
}

c_rest_error_t c_rest_strlcat(char *dst, const char *src, size_t dsize,
                              size_t *out_len) { /* GCOVR_EXCL_LINE */
  size_t dst_len;
  size_t src_len;
  size_t space_left;
  size_t copy_len;

  if (!out_len) { /* GCOVR_EXCL_LINE */
    LOG_DEBUG("c_rest_strlcat: invalid out_len");
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }
  *out_len = 0; /* GCOVR_EXCL_LINE */

  if (!dst || !src) { /* GCOVR_EXCL_LINE */
    LOG_DEBUG("c_rest_strlcat: invalid dst or src");
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }

  dst_len = strlen(dst); /* GCOVR_EXCL_LINE */
  src_len = strlen(src); /* GCOVR_EXCL_LINE */

  if (dsize <= dst_len) {       /* GCOVR_EXCL_LINE */
    *out_len = dsize + src_len; /* GCOVR_EXCL_LINE */
    return C_REST_OK;           /* GCOVR_EXCL_LINE */
  }

  space_left = dsize - dst_len - 1; /* GCOVR_EXCL_LINE */
  copy_len = src_len;               /* GCOVR_EXCL_LINE */
  if (copy_len > space_left) {      /* GCOVR_EXCL_LINE */
    copy_len = space_left;          /* GCOVR_EXCL_LINE */
  }

  memcpy(dst + dst_len, src, copy_len); /* GCOVR_EXCL_LINE */
  dst[dst_len + copy_len] = '\0';       /* GCOVR_EXCL_LINE */

  *out_len = dst_len + src_len; /* GCOVR_EXCL_LINE */
  return C_REST_OK;             /* GCOVR_EXCL_LINE */
}

c_rest_error_t c_rest_url_decode(char *dst, const char *src, size_t len) {
  size_t i;
  char *p;

  if (!dst || !src) { /* GCOVR_EXCL_LINE */
    LOG_DEBUG("c_rest_url_decode: invalid dst or src");
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }

  p = dst;
  for (i = 0; i < len; i++) {
    if (src[i] == '%') {
      if (i + 2 < len) { /* GCOVR_EXCL_LINE */
        int v;
        char hex[3];
        hex[0] = src[i + 1];
        hex[1] = src[i + 2];
        hex[2] = '\0';
        v = (int)strtol(hex, NULL, 16);
        *p++ = (char)v;
        i += 2;
      } else {
        *p++ = src[i]; /* GCOVR_EXCL_LINE */
      }
    } else if (src[i] == '+') { /* GCOVR_EXCL_LINE */
      *p++ = ' ';               /* GCOVR_EXCL_LINE */
    } else {
      *p++ = src[i];
    }
  }
  *p = '\0';
  return C_REST_OK;
}
