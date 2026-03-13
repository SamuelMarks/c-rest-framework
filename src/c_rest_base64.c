/* clang-format off */
#include "c_rest_base64.h"
/* clang-format on */

static size_t c_rest_base64_encode_pad(size_t src_len);
static unsigned char get_val(unsigned char c);

static const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                   "abcdefghijklmnopqrstuvwxyz"
                                   "0123456789+/";

int c_rest_base64_encode(const unsigned char *src, size_t src_len, char *dst,
                         size_t *dst_len) {
  size_t i = 0;
  size_t j = 0;
  size_t out_len = 4 * ((src_len + 2) / 3);

  if (!dst_len)
    return 1;
  if (!dst) {
    *dst_len = out_len + 1;
    return 0;
  }
  if (*dst_len < out_len + 1)
    return 1;

  for (i = 0; i < src_len;) {
    unsigned int octet_a = i < src_len ? (unsigned char)src[i++] : 0;
    unsigned int octet_b = i < src_len ? (unsigned char)src[i++] : 0;
    unsigned int octet_c = i < src_len ? (unsigned char)src[i++] : 0;

    unsigned int triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

    dst[j++] = base64_chars[(triple >> 3 * 6) & 0x3F];
    dst[j++] = base64_chars[(triple >> 2 * 6) & 0x3F];
    dst[j++] = base64_chars[(triple >> 1 * 6) & 0x3F];
    dst[j++] = base64_chars[(triple >> 0 * 6) & 0x3F];
  }

  for (i = 0; i < c_rest_base64_encode_pad(src_len); i++) {
    dst[out_len - 1 - i] = '=';
  }
  dst[out_len] = '\0';
  *dst_len = out_len;

  return 0;
}

static size_t c_rest_base64_encode_pad(size_t src_len) {
  size_t val = src_len % 3;
  if (val == 0)
    return 0;
  return 3 - val;
}

static unsigned char get_val(unsigned char c) {
  if (c >= 'A' && c <= 'Z')
    return c - 'A';
  if (c >= 'a' && c <= 'z')
    return c - 'a' + 26;
  if (c >= '0' && c <= '9')
    return c - '0' + 52;
  if (c == '+')
    return 62;
  if (c == '/')
    return 63;
  return 0;
}

int c_rest_base64_decode(const char *src, size_t src_len, unsigned char *dst,
                         size_t *dst_len) {
  size_t i = 0;
  size_t j = 0;
  size_t in_len = src_len;
  size_t out_len;

  if (!src || !dst_len)
    return 1;

  if (in_len % 4 != 0)
    return 1;
  out_len = in_len / 4 * 3;
  if (src[in_len - 1] == '=')
    out_len--;
  if (src[in_len - 2] == '=')
    out_len--;

  if (!dst) {
    *dst_len = out_len;
    return 0;
  }
  if (*dst_len < out_len)
    return 1;

  for (i = 0, j = 0; i < in_len;) {
    unsigned int sextet_a = src[i] == '=' ? 0 & i++ : get_val(src[i++]);
    unsigned int sextet_b = src[i] == '=' ? 0 & i++ : get_val(src[i++]);
    unsigned int sextet_c = src[i] == '=' ? 0 & i++ : get_val(src[i++]);
    unsigned int sextet_d = src[i] == '=' ? 0 & i++ : get_val(src[i++]);

    unsigned int triple = (sextet_a << 3 * 6) + (sextet_b << 2 * 6) +
                          (sextet_c << 1 * 6) + (sextet_d << 0 * 6);

    if (j < out_len)
      dst[j++] = (unsigned char)((triple >> 2 * 8) & 0xFF);
    if (j < out_len)
      dst[j++] = (unsigned char)((triple >> 1 * 8) & 0xFF);
    if (j < out_len)
      dst[j++] = (unsigned char)((triple >> 0 * 8) & 0xFF);
  }
  *dst_len = out_len;
  return 0;
}
