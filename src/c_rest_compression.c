/* clang-format off */
#define C_REST_MEM_INTERNAL 1
#include "c_rest_testing_mocks.h"
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

#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP)
static NO_DISCARD c_rest_error_t gzip_deflate_step(z_stream *strm, int flush) {
  int d_rc;
#ifdef C_REST_TESTING_MALLOC_HOOK
  if (crf_seam_lib_check(3) != C_REST_OK)
    d_rc = Z_STREAM_ERROR;
  else
    d_rc = deflate(strm, flush);
#else
  d_rc = deflate(strm, flush);
#endif
  if (d_rc == Z_STREAM_ERROR)
    return C_REST_ERROR_GENERIC;
  return C_REST_OK;
}
#endif

#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI)
static NO_DISCARD c_rest_error_t brotli_compress_step(
    BrotliEncoderState *state, BrotliEncoderOperation op, size_t *available_in,
    const uint8_t **next_in, size_t *available_out, uint8_t **next_out) {
  int b_ok;
#ifdef C_REST_TESTING_MALLOC_HOOK
  if (crf_seam_lib_check(4) != C_REST_OK)
    b_ok = 0;
  else
    b_ok = (int)BrotliEncoderCompressStream(state, op, available_in, next_in,
                                            available_out, next_out, NULL);
#else
  b_ok = (int)BrotliEncoderCompressStream(state, op, available_in, next_in,
                                          available_out, next_out, NULL);
#endif
  return b_ok ? C_REST_OK : C_REST_ERROR_GENERIC;
}
#endif

#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP)
static voidpf c_rest_zalloc_impl(voidpf opaque, uInt items, uInt size) {
  (void)opaque;
  return (voidpf)CRF_MALLOC((size_t)items * (size_t)size);
}

static void c_rest_zlib_free(voidpf opaque, voidpf address) {
  (void)opaque;
  CRF_FREE(address);
}
#endif

#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI) &&            \
    defined(C_REST_BROTLI_CLEANUP_ON_OOM)
static void *c_rest_brotli_alloc_impl(void *opaque, size_t size) {
  (void)opaque;
  return CRF_MALLOC(size);
}

static void c_rest_brotli_free(void *opaque, void *address) {
  (void)opaque;
  CRF_FREE(address);
}
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
    int z_init_rc;
    new_ctx->z_strm.zalloc = c_rest_zalloc_impl;
    new_ctx->z_strm.zfree = c_rest_zlib_free;
    new_ctx->z_strm.opaque = Z_NULL;
    /* 15 + 16 for gzip encoding */
#ifdef C_REST_TESTING_MALLOC_HOOK
    if (crf_seam_lib_check(1) != C_REST_OK)
      z_init_rc = Z_STREAM_ERROR;
    else
      z_init_rc = deflateInit2(&new_ctx->z_strm, Z_DEFAULT_COMPRESSION,
                               Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY);
#else
    z_init_rc = deflateInit2(&new_ctx->z_strm, Z_DEFAULT_COMPRESSION,
                             Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY);
#endif
    if (z_init_rc != Z_OK) {
      C_REST_FREE(new_ctx);
      return C_REST_ERROR_GENERIC;
    }
  } else
#endif
#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI)
      if (type == C_REST_COMPRESSION_BROTLI) {
#ifdef C_REST_TESTING_MALLOC_HOOK
    if (crf_seam_lib_check(2) != C_REST_OK) {
      new_ctx->b_strm = NULL;
    } else
#endif
    {
#if defined(C_REST_BROTLI_CLEANUP_ON_OOM)
      new_ctx->b_strm = BrotliEncoderCreateInstance(c_rest_brotli_alloc_impl,
                                                    c_rest_brotli_free, NULL);
#else
      new_ctx->b_strm = BrotliEncoderCreateInstance(NULL, NULL, NULL);
#endif
    }
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
#ifdef C_REST_TESTING_MALLOC_HOOK
  return crf_seam_lib_check(99);
#else
  return C_REST_OK;
#endif
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

      if (gzip_deflate_step(&ctx->z_strm, Z_NO_FLUSH) != C_REST_OK) {
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
        size_t offset = (size_t)(next_out - out_buf);
        out_capacity *= 2;
        if (C_REST_REALLOC(out_buf, out_capacity, &new_buf) != 0) {
          C_REST_FREE(out_buf);
          return C_REST_ERROR_GENERIC;
        }
        out_buf = new_buf;
        next_out = out_buf + offset;
        available_out = out_capacity - offset;
      }

      if (brotli_compress_step(ctx->b_strm, BROTLI_OPERATION_FLUSH,
                               &available_in, &next_in, &available_out,
                               &next_out) != C_REST_OK) {
        C_REST_FREE(out_buf);
        return C_REST_ERROR_GENERIC;
      }
    }

    *out_data = out_buf;
    *out_len = (size_t)(next_out - out_buf);
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

      if (gzip_deflate_step(&ctx->z_strm, Z_FINISH) != C_REST_OK) {
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
      if (brotli_compress_step(ctx->b_strm, BROTLI_OPERATION_FINISH,
                               &available_in, &next_in, &available_out,
                               &next_out) != C_REST_OK) {
        C_REST_FREE(out_buf);
        return C_REST_ERROR_GENERIC;
      }

      if (!BrotliEncoderIsFinished(ctx->b_strm)) {
        unsigned char *new_buf = NULL;
        size_t offset = (size_t)(next_out - out_buf);
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
    *out_len = (size_t)(next_out - out_buf);
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
  size_t i;

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
  if (rc != C_REST_OK)
    goto compress_cleanup;

  rc = c_rest_compress_finish(ctx, &data2, &len2);
  if (rc != C_REST_OK)
    goto compress_cleanup;

  if (C_REST_MALLOC(len1 + len2, (void **)&(*out_data)) != 0) {
    rc = C_REST_ERROR_GENERIC;
    goto compress_cleanup;
  }

  if (len1 > 0) {
    for (i = 0; i < len1; ++i) {
      (*out_data)[i] = data1[i];
    }
  }
  for (i = 0; i < len2; ++i) {
    (*out_data)[len1 + i] = data2[i];
  }
  *out_len = len1 + len2;

compress_cleanup:
  if (data1)
    C_REST_FREE(data1);
  if (data2)
    C_REST_FREE(data2);
  {
    c_rest_error_t destroy_rc = c_rest_compression_ctx_destroy(ctx);
    if (destroy_rc != C_REST_OK)
      return destroy_rc;
  }
  return rc;
}
