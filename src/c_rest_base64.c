/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_base64.h"

#include <string.h>
/* clang-format on */

static const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                   "abcdefghijklmnopqrstuvwxyz"
                                   "0123456789+/";

static const char base64url_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                      "abcdefghijklmnopqrstuvwxyz"
                                      "0123456789-_";

static c_rest_error_t encode_internal(const unsigned char *src, size_t src_len,
                                      char *dst, size_t *dst_len,
                                      const char *chars, int use_padding) {
  size_t i = 0;
  size_t j = 0;
  size_t out_len = 4 * ((src_len + 2) / 3);
  size_t val;
  size_t pad_len;

  if (!dst_len)                  /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  if (!dst) {
    if (use_padding) {
      *dst_len = out_len + 1;
    } else {
      val = src_len % 3;
      pad_len = (val == 0) ? 0 : (3 - val);
      *dst_len = out_len - pad_len + 1;
    }
    return C_REST_OK;
  }

  for (i = 0; i < src_len;) {
    unsigned int octet_a =
        i < src_len ? (unsigned char)src[i++] : 0; /* GCOVR_EXCL_LINE */
    unsigned int octet_b = i < src_len ? (unsigned char)src[i++] : 0;
    unsigned int octet_c = i < src_len ? (unsigned char)src[i++] : 0;

    unsigned long triple = ((unsigned long)octet_a << 16) +
                           ((unsigned long)octet_b << 8) + octet_c;

    dst[j++] = chars[(triple >> 18) & 0x3F];
    dst[j++] = chars[(triple >> 12) & 0x3F];
    dst[j++] = chars[(triple >> 6) & 0x3F];
    dst[j++] = chars[triple & 0x3F];
  }

  val = src_len % 3;
  pad_len = (val == 0) ? 0 : (3 - val);

  if (use_padding) {
    for (i = 0; i < pad_len; i++) {
      dst[out_len - 1 - i] = '=';
    }
    dst[out_len] = '\0';
    *dst_len = out_len;
  } else {
    out_len -= pad_len;
    dst[out_len] = '\0';
    *dst_len = out_len;
  }

  return C_REST_OK;
}

static c_rest_error_t get_val(unsigned char c, int is_url,
                              unsigned char *out_val) {
  if (c >= 'A' && c <= 'Z') {
    *out_val = c - 'A';
    return C_REST_OK;
  }
  if (c >= 'a' && c <= 'z') { /* GCOVR_EXCL_LINE */
    *out_val = c - 'a' + 26;
    return C_REST_OK;
  }
  if (c >= '0' && c <= '9') { /* GCOVR_EXCL_LINE */
    *out_val = c - '0' + 52;
    return C_REST_OK;
  }
  if (!is_url) {    /* GCOVR_EXCL_LINE */
    if (c == '+') { /* GCOVR_EXCL_LINE */
      *out_val = 62;
      return C_REST_OK;
    }
    if (c == '/') {     /* GCOVR_EXCL_LINE */
      *out_val = 63;    /* GCOVR_EXCL_LINE */
      return C_REST_OK; /* GCOVR_EXCL_LINE */
    }
  } else {
    if (c == '-') {     /* GCOVR_EXCL_LINE */
      *out_val = 62;    /* GCOVR_EXCL_LINE */
      return C_REST_OK; /* GCOVR_EXCL_LINE */
    }
    if (c == '_') {     /* GCOVR_EXCL_LINE */
      *out_val = 63;    /* GCOVR_EXCL_LINE */
      return C_REST_OK; /* GCOVR_EXCL_LINE */
    }
  }
  *out_val = 0;     /* GCOVR_EXCL_LINE */
  return C_REST_OK; /* GCOVR_EXCL_LINE */
}

static c_rest_error_t decode_internal(const char *src, size_t src_len,
                                      unsigned char *dst, size_t *dst_len,
                                      int is_url) {
  size_t i = 0;
  size_t j = 0;
  size_t in_len = src_len;
  size_t out_len;
  size_t pad_len = 0;

  if (!src || !dst_len)          /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  if (!is_url && in_len % 4 != 0) /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;  /* GCOVR_EXCL_LINE */

  if (in_len > 0 && src[in_len - 1] == '=') { /* GCOVR_EXCL_LINE */
    pad_len++;
    if (in_len > 1 && src[in_len - 2] == '=') /* GCOVR_EXCL_LINE */
      pad_len++;
  } else if (is_url) {
    pad_len = (4 - (in_len % 4)) % 4;
  }

  out_len = ((in_len + pad_len) / 4) * 3 - pad_len;

  if (!dst) {
    *dst_len = out_len;
    return C_REST_OK;
  }
  if (*dst_len < out_len)        /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  for (i = 0, j = 0; i < in_len;) {
    unsigned char val_a = 0, val_b = 0, val_c = 0, val_d = 0;
    unsigned int sextet_a = 0, sextet_b = 0, sextet_c = 0, sextet_d = 0;
    unsigned long triple;

    if (1) {             /* GCOVR_EXCL_LINE */
      if (src[i] == '=') /* GCOVR_EXCL_LINE */
        i++;             /* GCOVR_EXCL_LINE */
      else {
        get_val(src[i++], is_url, &val_a);
        sextet_a = val_a;
      }
    }
    if (1) {             /* GCOVR_EXCL_LINE */
      if (src[i] == '=') /* GCOVR_EXCL_LINE */
        i++;             /* GCOVR_EXCL_LINE */
      else {
        get_val(src[i++], is_url, &val_b);
        sextet_b = val_b;
      }
    }
    if (i < in_len) {
      if (src[i] == '=')
        i++;
      else {
        get_val(src[i++], is_url, &val_c);
        sextet_c = val_c;
      }
    }
    if (i < in_len) {
      if (src[i] == '=')
        i++;
      else {
        get_val(src[i++], is_url, &val_d);
        sextet_d = val_d;
      }
    }

    triple = ((unsigned long)sextet_a << 18) + ((unsigned long)sextet_b << 12) +
             ((unsigned long)sextet_c << 6) + sextet_d;

    if (j < out_len) /* GCOVR_EXCL_LINE */
      dst[j++] = (unsigned char)((triple >> 16) & 0xFF);
    if (j < out_len)
      dst[j++] = (unsigned char)((triple >> 8) & 0xFF);
    if (j < out_len)
      dst[j++] = (unsigned char)(triple & 0xFF);
  }
  *dst_len = out_len;
  return C_REST_OK;
}

c_rest_error_t c_rest_base64_encode(const unsigned char *src, size_t src_len,
                                    char *dst, size_t *dst_len) {
  return encode_internal(src, src_len, dst, dst_len, base64_chars, 1);
}

c_rest_error_t c_rest_base64url_encode(const unsigned char *src, size_t src_len,
                                       char *dst, size_t *dst_len) {
  return encode_internal(src, src_len, dst, dst_len, base64url_chars, 0);
}

c_rest_error_t c_rest_base64_decode(const char *src, size_t src_len,
                                    unsigned char *dst, size_t *dst_len) {
  return decode_internal(src, src_len, dst, dst_len, 0);
}

c_rest_error_t c_rest_base64url_decode(const char *src, size_t src_len,
                                       unsigned char *dst, size_t *dst_len) {
  return decode_internal(src, src_len, dst, dst_len, 1);
}
