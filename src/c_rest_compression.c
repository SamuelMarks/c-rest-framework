#include "c_rest_testing_mocks.h"
/* clang-format off */
#include "c_rest_error.h"
#include "../include/c_rest_compression.h"
#include "../include/c_rest_mem.h"

#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP)
#include <zlib.h>
#endif
#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI)
#include <brotli/encode.h>
#endif
/* clang-format on */

#ifndef C_REST_TESTING_MALLOC_HOOK
#define c_rest_deflateInit2 deflateInit2
#define c_rest_deflate deflate
#define c_rest_BrotliEncoderCreateInstance BrotliEncoderCreateInstance
#define c_rest_BrotliEncoderCompressStream BrotliEncoderCompressStream
#endif

#ifdef C_REST_TESTING_MALLOC_HOOK
C_REST_EXPORT int g_mock_lib_fail = 0;
#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP)
#define c_rest_deflateInit2(a, b, c, d, e, f)                                  \
  (g_mock_lib_fail == 1 ? Z_STREAM_ERROR : deflateInit2(a, b, c, d, e, f))
#define c_rest_deflate(a, b)                                                   \
  (g_mock_lib_fail == 3 ? Z_STREAM_ERROR : deflate(a, b))
#endif
#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI)
#define c_rest_BrotliEncoderCreateInstance(a, b, c)                            \
  (g_mock_lib_fail == 2 ? NULL : BrotliEncoderCreateInstance(a, b, c))
static c_rest_error_t
test_mock_brotli_compress(BrotliEncoderState *state, BrotliEncoderOperation op,
                          size_t *available_in, const uint8_t **next_in,
                          size_t *available_out, uint8_t **next_out,
                          size_t *total_out) {
  if (g_mock_lib_fail == 4)
    return C_REST_OK; /* 0 corresponds to failure in Brotli */
  return (c_rest_error_t)BrotliEncoderCompressStream(
      state, op, available_in, next_in, available_out, next_out, total_out);
}
#define c_rest_BrotliEncoderCompressStream test_mock_brotli_compress
#endif
#endif

struct c_rest_compression_ctx {
  c_rest_compression_type_t type;
#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP)
  z_stream z_strm;
#endif
#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI)
  BrotliEncoderState *b_strm;
#endif
};

c_rest_error_t c_rest_compression_ctx_init(c_rest_compression_ctx_t **ctx,
                                           c_rest_compression_type_t type) {
  c_rest_compression_ctx_t *new_ctx = NULL;

  if (!ctx)
    return C_REST_ERROR_GENERIC;
  *ctx = NULL;

  if (type == C_REST_COMPRESSION_NONE)
    return C_REST_OK;

#if (defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP) ||             \
     defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI))
  if (C_REST_MALLOC(sizeof(c_rest_compression_ctx_t), &new_ctx) != 0)
    return C_REST_ERROR_GENERIC;

  new_ctx->type = type;

#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP)
  if (type == C_REST_COMPRESSION_GZIP) {
    new_ctx->z_strm.zalloc = Z_NULL;
    new_ctx->z_strm.zfree = Z_NULL;
    new_ctx->z_strm.opaque = Z_NULL;
    /* 15 + 16 for gzip encoding */
    if (c_rest_deflateInit2(&new_ctx->z_strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
                            15 + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
      C_REST_FREE(new_ctx);
      return C_REST_ERROR_GENERIC;
    }
  } else
#endif
#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI)
      if (type == C_REST_COMPRESSION_BROTLI) {
    new_ctx->b_strm = c_rest_BrotliEncoderCreateInstance(NULL, NULL, NULL);
    if (!new_ctx->b_strm) {
      C_REST_FREE(new_ctx);
      return C_REST_ERROR_GENERIC;
    }
  } else
#endif
  {
    C_REST_FREE(new_ctx);
    return C_REST_ERROR_GENERIC;
  }

  *ctx = new_ctx;
  return C_REST_OK;
#else
  return C_REST_ERROR_GENERIC; /* Not supported */
#endif
}

c_rest_error_t c_rest_compression_ctx_destroy(c_rest_compression_ctx_t *ctx) {
  if (!ctx)
    return C_REST_ERROR_GENERIC;

#if (defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP) ||             \
     defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI))
#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP)
  if (ctx->type == C_REST_COMPRESSION_GZIP) {
    deflateEnd(&ctx->z_strm);
  } else
#endif
#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI)
      if (ctx->type == C_REST_COMPRESSION_BROTLI) {
    BrotliEncoderDestroyInstance(ctx->b_strm);
  } else
#endif
  {
  }
  C_REST_FREE(ctx);
  return C_REST_OK;
#else
  return C_REST_ERROR_GENERIC;
#endif
}

c_rest_error_t c_rest_compress_data(c_rest_compression_ctx_t *ctx,
                                    const unsigned char *in_data, size_t in_len,
                                    unsigned char **out_data, size_t *out_len) {
#if (defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP) ||             \
     defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI))
  size_t out_capacity;
  unsigned char *out_buf = NULL;
  size_t total_out = 0;

  if (!ctx || !in_data || !out_data || !out_len)
    return C_REST_ERROR_GENERIC;
  *out_data = NULL;
  *out_len = 0;
  if (in_len == 0)
    return C_REST_OK;

#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP)
  if (ctx->type == C_REST_COMPRESSION_GZIP) {
    out_capacity = 1;
    if (C_REST_MALLOC(out_capacity, &out_buf) != 0)
      return C_REST_ERROR_GENERIC;

    ctx->z_strm.avail_in = (uInt)in_len;
    ctx->z_strm.next_in = (Bytef *)in_data;
    ctx->z_strm.avail_out = (uInt)out_capacity;

    do {
      if (ctx->z_strm.avail_out == 0) {
        unsigned char *new_buf = NULL;
        out_capacity *= 2;
        if (C_REST_REALLOC(out_buf, out_capacity, &new_buf) != 0) {
          C_REST_FREE(out_buf);
          return C_REST_ERROR_GENERIC;
        }
        out_buf = new_buf;
      }

      ctx->z_strm.avail_out = (uInt)(out_capacity - total_out);
      ctx->z_strm.next_out = out_buf + total_out;

      if (c_rest_deflate(&ctx->z_strm, Z_NO_FLUSH) == Z_STREAM_ERROR) {
        C_REST_FREE(out_buf);
        return C_REST_ERROR_GENERIC;
      }

      total_out = out_capacity - ctx->z_strm.avail_out;
    } while (ctx->z_strm.avail_out == 0);

    *out_data = out_buf;
    *out_len = total_out;
    return C_REST_OK;
  } else
#endif
#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI)
      if (ctx->type == C_REST_COMPRESSION_BROTLI) {
    size_t available_in = in_len;
    const uint8_t *next_in = in_data;
    size_t available_out;
    uint8_t *next_out;

    out_capacity = 1;
    if (C_REST_MALLOC(out_capacity, &out_buf) != 0)
      return C_REST_ERROR_GENERIC;

    available_out = out_capacity;
    next_out = out_buf;

    while (available_in > 0 || BrotliEncoderHasMoreOutput(ctx->b_strm)) {
      if (available_out == 0) {
        unsigned char *new_buf = NULL;
        size_t offset = next_out - out_buf;
        out_capacity *= 2;
        if (C_REST_REALLOC(out_buf, out_capacity, &new_buf) != 0) {
          C_REST_FREE(out_buf);
          return C_REST_ERROR_GENERIC;
        }
        out_buf = new_buf;
        next_out = out_buf + offset;
        available_out = out_capacity - offset;
      }

      if (!c_rest_BrotliEncoderCompressStream(
              ctx->b_strm, BROTLI_OPERATION_FLUSH, &available_in, &next_in,
              &available_out, &next_out, NULL)) {
        C_REST_FREE(out_buf);
        return C_REST_ERROR_GENERIC;
      }
    }

    *out_data = out_buf;
    *out_len = next_out - out_buf;
    return C_REST_OK;
  }
#endif
  return C_REST_ERROR_GENERIC;
#else
  return C_REST_ERROR_GENERIC;
#endif
}

c_rest_error_t c_rest_compress_finish(c_rest_compression_ctx_t *ctx,
                                      unsigned char **out_data,
                                      size_t *out_len) {
#if (defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP) ||             \
     defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI))
  size_t out_capacity = 1;
  unsigned char *out_buf = NULL;
  size_t total_out = 0;

  if (!ctx || !out_data || !out_len)
    return C_REST_ERROR_GENERIC;
  *out_data = NULL;
  *out_len = 0;

  if (C_REST_MALLOC(out_capacity, &out_buf) != 0)
    return C_REST_ERROR_GENERIC;

#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP)
  if (ctx->type == C_REST_COMPRESSION_GZIP) {
    ctx->z_strm.avail_in = 0;
    ctx->z_strm.next_in = Z_NULL;
    ctx->z_strm.avail_out = (uInt)out_capacity;

    do {
      if (ctx->z_strm.avail_out == 0) {
        unsigned char *new_buf = NULL;
        out_capacity *= 2;
        if (C_REST_REALLOC(out_buf, out_capacity, &new_buf) != 0) {
          C_REST_FREE(out_buf);
          return C_REST_ERROR_GENERIC;
        }
        out_buf = new_buf;
      }

      ctx->z_strm.avail_out = (uInt)(out_capacity - total_out);
      ctx->z_strm.next_out = out_buf + total_out;

      if (c_rest_deflate(&ctx->z_strm, Z_FINISH) == Z_STREAM_ERROR) {
        C_REST_FREE(out_buf);
        return C_REST_ERROR_GENERIC;
      }

      total_out = out_capacity - ctx->z_strm.avail_out;
    } while (ctx->z_strm.avail_out == 0);

    *out_data = out_buf;
    *out_len = total_out;
    return C_REST_OK;
  } else
#endif
#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI)
      if (ctx->type == C_REST_COMPRESSION_BROTLI) {
    size_t available_in = 0;
    const uint8_t *next_in = NULL;
    size_t brotli_out_capacity = 1;
    size_t available_out = 0;
    uint8_t *next_out = out_buf;

    while (!BrotliEncoderIsFinished(ctx->b_strm)) {
      if (!c_rest_BrotliEncoderCompressStream(
              ctx->b_strm, BROTLI_OPERATION_FINISH, &available_in, &next_in,
              &available_out, &next_out, NULL)) {
        C_REST_FREE(out_buf);
        return C_REST_ERROR_GENERIC;
      }

      if (!BrotliEncoderIsFinished(ctx->b_strm)) {
        unsigned char *new_buf = NULL;
        size_t offset = next_out - out_buf;
        brotli_out_capacity *= 2;
        if (C_REST_REALLOC(out_buf, brotli_out_capacity, &new_buf) != 0) {
          C_REST_FREE(out_buf);
          return C_REST_ERROR_GENERIC;
        }
        out_buf = new_buf;
        next_out = out_buf + offset;
        available_out = brotli_out_capacity - offset;
      }
    }

    *out_data = out_buf;
    *out_len = next_out - out_buf;
    return C_REST_OK;
  }
#endif
  C_REST_FREE(out_buf);
  return C_REST_ERROR_GENERIC;
#else
  return C_REST_ERROR_GENERIC;
#endif
}

c_rest_error_t c_rest_compress_buffer(c_rest_compression_type_t type,
                                      const unsigned char *in_data,
                                      size_t in_len, unsigned char **out_data,
                                      size_t *out_len) {
  c_rest_error_t rc;
  c_rest_compression_ctx_t *ctx = NULL;
  unsigned char *data1 = NULL;
  size_t len1 = 0;
  unsigned char *data2 = NULL;
  size_t len2 = 0;

  if (!in_data || !out_data || !out_len)
    return C_REST_ERROR_GENERIC;
  *out_data = NULL;
  *out_len = 0;

  if (type == C_REST_COMPRESSION_NONE)
    return C_REST_ERROR_GENERIC;

  rc = c_rest_compression_ctx_init(&ctx, type);
  if (rc != C_REST_OK)
    return rc;

  rc = c_rest_compress_data(ctx, in_data, in_len, &data1, &len1);
  if (rc != C_REST_OK) {
    (void)!c_rest_compression_ctx_destroy(ctx);
    return rc;
  }

  rc = c_rest_compress_finish(ctx, &data2, &len2);
  if (rc != C_REST_OK) {
    C_REST_FREE(data1);
    (void)!c_rest_compression_ctx_destroy(ctx);
    return rc;
  }

  rc = C_REST_MALLOC(len1 + len2, (void **)&(*out_data));
  if (rc != C_REST_OK) {
    C_REST_FREE(data1);
    C_REST_FREE(data2);
    (void)!c_rest_compression_ctx_destroy(ctx);
    return C_REST_ERROR_GENERIC;
  }

  if (len1 > 0) {
    /* c_rest_mem does not have a memcpy directly, using loop or standard memcpy
     */
    size_t i;
    for (i = 0; i < len1; ++i) {
      (*out_data)[i] = data1[i];
    }
  }
  {
    size_t i;
    for (i = 0; i < len2; ++i) {
      (*out_data)[len1 + i] = data2[i];
    }
  }
  *out_len = len1 + len2;

  C_REST_FREE(data1);
  C_REST_FREE(data2);
  (void)!c_rest_compression_ctx_destroy(ctx);
  return C_REST_OK;
}
