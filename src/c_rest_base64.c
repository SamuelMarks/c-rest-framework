/* clang-format off */
#include "c_rest_base64.h"

#include <string.h>
/* clang-format on */

static const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                   "abcdefghijklmnopqrstuvwxyz"
                                   "0123456789+/";

static const char base64url_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                      "abcdefghijklmnopqrstuvwxyz"
                                      "0123456789-_";

static int encode_internal(const unsigned char *src, size_t src_len, char *dst,
                           size_t *dst_len, const char *chars,
                           int use_padding) {
  size_t i = 0;
  size_t j = 0;
  size_t out_len = 4 * ((src_len + 2) / 3);
  size_t val;
  size_t pad_len;

  if (!dst_len)
    return 1;
  if (!dst) {
    if (use_padding) {
      *dst_len = out_len + 1;
    } else {
      val = src_len % 3;
      pad_len = (val == 0) ? 0 : (3 - val);
      *dst_len = out_len - pad_len + 1;
    }
    return 0;
  }

  for (i = 0; i < src_len;) {
    unsigned int octet_a = i < src_len ? (unsigned char)src[i++] : 0;
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

  return 0;
}

static unsigned char get_val(unsigned char c, int is_url) {
  if (c >= 'A' && c <= 'Z')
    return c - 'A';
  if (c >= 'a' && c <= 'z')
    return c - 'a' + 26;
  if (c >= '0' && c <= '9')
    return c - '0' + 52;
  if (!is_url) {
    if (c == '+')
      return 62;
    if (c == '/')
      return 63;
  } else {
    if (c == '-')
      return 62;
    if (c == '_')
      return 63;
  }
  return 0;
}

static int decode_internal(const char *src, size_t src_len, unsigned char *dst,
                           size_t *dst_len, int is_url) {
  size_t i = 0;
  size_t j = 0;
  size_t in_len = src_len;
  size_t out_len;
  size_t pad_len = 0;

  if (!src || !dst_len)
    return 1;

  if (!is_url && in_len % 4 != 0)
    return 1;

  if (in_len > 0 && src[in_len - 1] == '=') {
    pad_len++;
    if (in_len > 1 && src[in_len - 2] == '=')
      pad_len++;
  } else if (is_url) {
    pad_len = (4 - (in_len % 4)) % 4;
  }

  out_len = ((in_len + pad_len) / 4) * 3 - pad_len;

  if (!dst) {
    *dst_len = out_len;
    return 0;
  }
  if (*dst_len < out_len)
    return 1;

  for (i = 0, j = 0; i < in_len;) {
    unsigned int sextet_a =
        i < in_len ? (src[i] == '=' ? 0 & i++ : get_val(src[i++], is_url)) : 0;
    unsigned int sextet_b =
        i < in_len ? (src[i] == '=' ? 0 & i++ : get_val(src[i++], is_url)) : 0;
    unsigned int sextet_c =
        i < in_len ? (src[i] == '=' ? 0 & i++ : get_val(src[i++], is_url)) : 0;
    unsigned int sextet_d =
        i < in_len ? (src[i] == '=' ? 0 & i++ : get_val(src[i++], is_url)) : 0;

    unsigned long triple = ((unsigned long)sextet_a << 18) +
                           ((unsigned long)sextet_b << 12) +
                           ((unsigned long)sextet_c << 6) + sextet_d;

    if (j < out_len)
      dst[j++] = (unsigned char)((triple >> 16) & 0xFF);
    if (j < out_len)
      dst[j++] = (unsigned char)((triple >> 8) & 0xFF);
    if (j < out_len)
      dst[j++] = (unsigned char)(triple & 0xFF);
  }
  *dst_len = out_len;
  return 0;
}

int c_rest_base64_encode(const unsigned char *src, size_t src_len, char *dst,
                         size_t *dst_len) {
  return encode_internal(src, src_len, dst, dst_len, base64_chars, 1);
}

int c_rest_base64url_encode(const unsigned char *src, size_t src_len, char *dst,
                            size_t *dst_len) {
  return encode_internal(src, src_len, dst, dst_len, base64url_chars, 0);
}

int c_rest_base64_decode(const char *src, size_t src_len, unsigned char *dst,
                         size_t *dst_len) {
  return decode_internal(src, src_len, dst, dst_len, 0);
}

int c_rest_base64url_decode(const char *src, size_t src_len, unsigned char *dst,
                            size_t *dst_len) {
  return decode_internal(src, src_len, dst, dst_len, 1);
}
