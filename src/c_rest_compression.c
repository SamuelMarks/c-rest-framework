/* clang-format off */
#include "../include/c_rest_compression.h"
#include "../include/c_rest_mem.h"

#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP)
#include <zlib.h>
#endif
#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI)
#include <brotli/encode.h>
/* clang-format on */
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

int c_rest_compression_ctx_init(c_rest_compression_ctx_t **ctx,
                                c_rest_compression_type_t type) {
  c_rest_compression_ctx_t *new_ctx = NULL;

  if (!ctx)
    return 1;
  *ctx = NULL;

  if (type == C_REST_COMPRESSION_NONE)
    return 0;

#if (defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP) ||             \
     defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI))
  if (C_REST_MALLOC(sizeof(c_rest_compression_ctx_t), (void **)&new_ctx) != 0 ||
      !new_ctx)
    return 1;

  new_ctx->type = type;

#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP)
  if (type == C_REST_COMPRESSION_GZIP) {
    new_ctx->z_strm.zalloc = Z_NULL;
    new_ctx->z_strm.zfree = Z_NULL;
    new_ctx->z_strm.opaque = Z_NULL;
    /* 15 + 16 for gzip encoding */
    if (deflateInit2(&new_ctx->z_strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
                     15 + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
      C_REST_FREE(new_ctx);
      return 1;
    }
  } else
#endif
#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI)
      if (type == C_REST_COMPRESSION_BROTLI) {
    new_ctx->b_strm = BrotliEncoderCreateInstance(NULL, NULL, NULL);
    if (!new_ctx->b_strm) {
      C_REST_FREE(new_ctx);
      return 1;
    }
  } else
#endif
  {
    C_REST_FREE(new_ctx);
    return 1;
  }

  *ctx = new_ctx;
  return 0;
#else
  return 1; /* Not supported */
#endif
}

int c_rest_compression_ctx_destroy(c_rest_compression_ctx_t *ctx) {
  if (!ctx)
    return 1;

#if (defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP) ||             \
     defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI))
#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP)
  if (ctx->type == C_REST_COMPRESSION_GZIP) {
    deflateEnd(&ctx->z_strm);
  } else
#endif
#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI)
      if (ctx->type == C_REST_COMPRESSION_BROTLI) {
    if (ctx->b_strm) {
      BrotliEncoderDestroyInstance(ctx->b_strm);
    }
  } else
#endif
  {
  }
  C_REST_FREE(ctx);
  return 0;
#else
  return 1;
#endif
}

int c_rest_compress_data(c_rest_compression_ctx_t *ctx,
                         const unsigned char *in_data, size_t in_len,
                         unsigned char **out_data, size_t *out_len) {
#if (defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP) ||             \
     defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI))
  size_t out_capacity;
  unsigned char *out_buf = NULL;
  size_t total_out = 0;

  if (!ctx || !in_data || !out_data || !out_len)
    return 1;
  *out_data = NULL;
  *out_len = 0;
  if (in_len == 0)
    return 0;

#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP)
  if (ctx->type == C_REST_COMPRESSION_GZIP) {
    out_capacity = in_len + 4096; /* Some extra space */
    if (C_REST_MALLOC(out_capacity, (void **)&out_buf) != 0 || !out_buf)
      return 1;

    ctx->z_strm.avail_in = (uInt)in_len;
    ctx->z_strm.next_in = (Bytef *)in_data;

    do {
      if (ctx->z_strm.avail_out == 0) {
        unsigned char *new_buf = NULL;
        out_capacity *= 2;
        if (C_REST_REALLOC(out_buf, out_capacity, (void **)&new_buf) != 0 ||
            !new_buf) {
          C_REST_FREE(out_buf);
          return 1;
        }
        out_buf = new_buf;
      }

      ctx->z_strm.avail_out = (uInt)(out_capacity - total_out);
      ctx->z_strm.next_out = out_buf + total_out;

      if (deflate(&ctx->z_strm, Z_NO_FLUSH) == Z_STREAM_ERROR) {
        C_REST_FREE(out_buf);
        return 1;
      }

      total_out = out_capacity - ctx->z_strm.avail_out;
    } while (ctx->z_strm.avail_out == 0);

    *out_data = out_buf;
    *out_len = total_out;
    return 0;
  } else
#endif
#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI)
      if (ctx->type == C_REST_COMPRESSION_BROTLI) {
    size_t available_in = in_len;
    const uint8_t *next_in = in_data;
    size_t available_out;
    uint8_t *next_out;

    out_capacity = in_len + 4096;
    if (C_REST_MALLOC(out_capacity, (void **)&out_buf) != 0 || !out_buf)
      return 1;

    available_out = out_capacity;
    next_out = out_buf;

    while (available_in > 0) {
      if (available_out == 0) {
        unsigned char *new_buf = NULL;
        size_t offset = next_out - out_buf;
        out_capacity *= 2;
        if (C_REST_REALLOC(out_buf, out_capacity, (void **)&new_buf) != 0 ||
            !new_buf) {
          C_REST_FREE(out_buf);
          return 1;
        }
        out_buf = new_buf;
        next_out = out_buf + offset;
        available_out = out_capacity - offset;
      }

      if (!BrotliEncoderCompressStream(ctx->b_strm, BROTLI_OPERATION_PROCESS,
                                       &available_in, &next_in, &available_out,
                                       &next_out, NULL)) {
        C_REST_FREE(out_buf);
        return 1;
      }
    }

    *out_data = out_buf;
    *out_len = next_out - out_buf;
    return 0;
  }
#endif
  return 1;
#else
  return 1;
#endif
}

int c_rest_compress_finish(c_rest_compression_ctx_t *ctx,
                           unsigned char **out_data, size_t *out_len) {
#if (defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP) ||             \
     defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI))
  size_t out_capacity = 4096;
  unsigned char *out_buf = NULL;
  size_t total_out = 0;

  if (!ctx || !out_data || !out_len)
    return 1;
  *out_data = NULL;
  *out_len = 0;

  if (C_REST_MALLOC(out_capacity, (void **)&out_buf) != 0 || !out_buf)
    return 1;

#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP)
  if (ctx->type == C_REST_COMPRESSION_GZIP) {
    ctx->z_strm.avail_in = 0;
    ctx->z_strm.next_in = Z_NULL;

    do {
      if (ctx->z_strm.avail_out == 0) {
        unsigned char *new_buf = NULL;
        out_capacity *= 2;
        if (C_REST_REALLOC(out_buf, out_capacity, (void **)&new_buf) != 0 ||
            !new_buf) {
          C_REST_FREE(out_buf);
          return 1;
        }
        out_buf = new_buf;
      }

      ctx->z_strm.avail_out = (uInt)(out_capacity - total_out);
      ctx->z_strm.next_out = out_buf + total_out;

      if (deflate(&ctx->z_strm, Z_FINISH) == Z_STREAM_ERROR) {
        C_REST_FREE(out_buf);
        return 1;
      }

      total_out = out_capacity - ctx->z_strm.avail_out;
    } while (ctx->z_strm.avail_out == 0);

    *out_data = out_buf;
    *out_len = total_out;
    return 0;
  } else
#endif
#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI)
      if (ctx->type == C_REST_COMPRESSION_BROTLI) {
    size_t available_in = 0;
    const uint8_t *next_in = NULL;
    size_t available_out = out_capacity;
    uint8_t *next_out = out_buf;

    while (1) {
      if (available_out == 0) {
        unsigned char *new_buf = NULL;
        size_t offset = next_out - out_buf;
        out_capacity *= 2;
        if (C_REST_REALLOC(out_buf, out_capacity, (void **)&new_buf) != 0 ||
            !new_buf) {
          C_REST_FREE(out_buf);
          return 1;
        }
        out_buf = new_buf;
        next_out = out_buf + offset;
        available_out = out_capacity - offset;
      }

      if (!BrotliEncoderCompressStream(ctx->b_strm, BROTLI_OPERATION_FINISH,
                                       &available_in, &next_in, &available_out,
                                       &next_out, NULL)) {
        C_REST_FREE(out_buf);
        return 1;
      }

      if (BrotliEncoderIsFinished(ctx->b_strm)) {
        break;
      }
    }

    *out_data = out_buf;
    *out_len = next_out - out_buf;
    return 0;
  }
#endif
  C_REST_FREE(out_buf);
  return 1;
#else
  return 1;
#endif
}

int c_rest_compress_buffer(c_rest_compression_type_t type,
                           const unsigned char *in_data, size_t in_len,
                           unsigned char **out_data, size_t *out_len) {
  c_rest_compression_ctx_t *ctx = NULL;
  unsigned char *data1 = NULL;
  size_t len1 = 0;
  unsigned char *data2 = NULL;
  size_t len2 = 0;

  if (!in_data || !out_data || !out_len)
    return 1;
  *out_data = NULL;
  *out_len = 0;

  if (type == C_REST_COMPRESSION_NONE)
    return 1;

  if (c_rest_compression_ctx_init(&ctx, type) != 0)
    return 1;

  if (c_rest_compress_data(ctx, in_data, in_len, &data1, &len1) != 0) {
    c_rest_compression_ctx_destroy(ctx);
    return 1;
  }

  if (c_rest_compress_finish(ctx, &data2, &len2) != 0) {
    if (data1)
      C_REST_FREE(data1);
    c_rest_compression_ctx_destroy(ctx);
    return 1;
  }

  if (C_REST_MALLOC(len1 + len2, (void **)out_data) != 0 || !*out_data) {
    if (data1)
      C_REST_FREE(data1);
    if (data2)
      C_REST_FREE(data2);
    c_rest_compression_ctx_destroy(ctx);
    return 1;
  }

  if (len1 > 0) {
    /* c_rest_mem does not have a memcpy directly, using loop or standard memcpy
     */
    size_t i;
    for (i = 0; i < len1; ++i) {
      (*out_data)[i] = data1[i];
    }
  }
  if (len2 > 0) {
    size_t i;
    for (i = 0; i < len2; ++i) {
      (*out_data)[len1 + i] = data2[i];
    }
  }
  *out_len = len1 + len2;

  if (data1)
    C_REST_FREE(data1);
  if (data2)
    C_REST_FREE(data2);
  c_rest_compression_ctx_destroy(ctx);
  return 0;
}
