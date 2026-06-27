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

struct c_rest_compression_ctx {
  c_rest_compression_type_t type;
#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP)
  z_stream z_strm;
#endif
#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI)
  BrotliEncoderState *b_strm;
#endif
};

c_rest_error_t c_rest_compression_ctx_init(
    c_rest_compression_ctx_t **ctx, /* GCOVR_EXCL_LINE */
    c_rest_compression_type_t type) {
  c_rest_compression_ctx_t *new_ctx = NULL; /* GCOVR_EXCL_LINE */

  if (!ctx)                      /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  *ctx = NULL;                   /* GCOVR_EXCL_LINE */

  if (type == C_REST_COMPRESSION_NONE) /* GCOVR_EXCL_LINE */
    return C_REST_OK;                  /* GCOVR_EXCL_LINE */

#if (defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP) ||             \
     defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI))
  if (C_REST_MALLOC(sizeof(c_rest_compression_ctx_t), &new_ctx) !=
          0 ||                   /* GCOVR_EXCL_LINE */
      !new_ctx)                  /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  new_ctx->type = type; /* GCOVR_EXCL_LINE */

#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP)
  if (type == C_REST_COMPRESSION_GZIP) { /* GCOVR_EXCL_LINE */
    new_ctx->z_strm.zalloc = Z_NULL;     /* GCOVR_EXCL_LINE */
    new_ctx->z_strm.zfree = Z_NULL;      /* GCOVR_EXCL_LINE */
    new_ctx->z_strm.opaque = Z_NULL;     /* GCOVR_EXCL_LINE */
    /* 15 + 16 for gzip encoding */
    if (deflateInit2(&new_ctx->z_strm, Z_DEFAULT_COMPRESSION,
                     Z_DEFLATED, /* GCOVR_EXCL_LINE */
                     15 + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
      C_REST_FREE(new_ctx);        /* GCOVR_EXCL_LINE */
      return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
    }
  } else
#endif
#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI)
      if (type == C_REST_COMPRESSION_BROTLI) { /* GCOVR_EXCL_LINE */
    new_ctx->b_strm =
        BrotliEncoderCreateInstance(NULL, NULL, NULL); /* GCOVR_EXCL_LINE */
    if (!new_ctx->b_strm) {                            /* GCOVR_EXCL_LINE */
      C_REST_FREE(new_ctx);                            /* GCOVR_EXCL_LINE */
      return C_REST_ERROR_GENERIC;                     /* GCOVR_EXCL_LINE */
    }
  } else
#endif
  {
    C_REST_FREE(new_ctx);        /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }

  *ctx = new_ctx;   /* GCOVR_EXCL_LINE */
  return C_REST_OK; /* GCOVR_EXCL_LINE */
#else
  return C_REST_ERROR_GENERIC; /* Not supported */
#endif
}

c_rest_error_t c_rest_compression_ctx_destroy(
    c_rest_compression_ctx_t *ctx) { /* GCOVR_EXCL_LINE */
  if (!ctx)                          /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;     /* GCOVR_EXCL_LINE */

#if (defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP) ||             \
     defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI))
#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP)
  if (ctx->type == C_REST_COMPRESSION_GZIP) { /* GCOVR_EXCL_LINE */
    deflateEnd(&ctx->z_strm);                 /* GCOVR_EXCL_LINE */
  } else
#endif
#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI)
      if (ctx->type == C_REST_COMPRESSION_BROTLI) { /* GCOVR_EXCL_LINE */
    if (ctx->b_strm) {                              /* GCOVR_EXCL_LINE */
      BrotliEncoderDestroyInstance(ctx->b_strm);    /* GCOVR_EXCL_LINE */
    }
  } else
#endif
  {
  }
  C_REST_FREE(ctx); /* GCOVR_EXCL_LINE */
  return C_REST_OK; /* GCOVR_EXCL_LINE */
#else
  return C_REST_ERROR_GENERIC;
#endif
}

c_rest_error_t
c_rest_compress_data(c_rest_compression_ctx_t *ctx, /* GCOVR_EXCL_LINE */
                     const unsigned char *in_data, size_t in_len,
                     unsigned char **out_data, size_t *out_len) {
#if (defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP) ||             \
     defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI))
  size_t out_capacity;
  unsigned char *out_buf = NULL; /* GCOVR_EXCL_LINE */
  size_t total_out = 0;          /* GCOVR_EXCL_LINE */

  if (!ctx || !in_data || !out_data || !out_len) /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;                 /* GCOVR_EXCL_LINE */
  *out_data = NULL;                              /* GCOVR_EXCL_LINE */
  *out_len = 0;                                  /* GCOVR_EXCL_LINE */
  if (in_len == 0)                               /* GCOVR_EXCL_LINE */
    return C_REST_OK;                            /* GCOVR_EXCL_LINE */

#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP)
  if (ctx->type == C_REST_COMPRESSION_GZIP) {            /* GCOVR_EXCL_LINE */
    out_capacity = in_len + 4096; /* Some extra space */ /* GCOVR_EXCL_LINE */
    if (C_REST_MALLOC(out_capacity, &out_buf) != 0 ||
        !out_buf)                  /* GCOVR_EXCL_LINE */
      return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

    ctx->z_strm.avail_in = (uInt)in_len;    /* GCOVR_EXCL_LINE */
    ctx->z_strm.next_in = (Bytef *)in_data; /* GCOVR_EXCL_LINE */

    do {
      if (ctx->z_strm.avail_out == 0) { /* GCOVR_EXCL_LINE */
        unsigned char *new_buf = NULL;  /* GCOVR_EXCL_LINE */
        out_capacity *= 2;              /* GCOVR_EXCL_LINE */
        if (C_REST_REALLOC(out_buf, out_capacity, &new_buf) != 0 ||
            !new_buf) {                /* GCOVR_EXCL_LINE */
          C_REST_FREE(out_buf);        /* GCOVR_EXCL_LINE */
          return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
        }
        out_buf = new_buf; /* GCOVR_EXCL_LINE */
      }

      ctx->z_strm.avail_out =
          (uInt)(out_capacity - total_out);       /* GCOVR_EXCL_LINE */
      ctx->z_strm.next_out = out_buf + total_out; /* GCOVR_EXCL_LINE */

      if (deflate(&ctx->z_strm, Z_NO_FLUSH) ==
          Z_STREAM_ERROR) {          /* GCOVR_EXCL_LINE */
        C_REST_FREE(out_buf);        /* GCOVR_EXCL_LINE */
        return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
      }

      total_out = out_capacity - ctx->z_strm.avail_out; /* GCOVR_EXCL_LINE */
    } while (ctx->z_strm.avail_out == 0); /* GCOVR_EXCL_LINE */

    *out_data = out_buf;  /* GCOVR_EXCL_LINE */
    *out_len = total_out; /* GCOVR_EXCL_LINE */
    return C_REST_OK;     /* GCOVR_EXCL_LINE */
  } else
#endif
#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI)
      if (ctx->type == C_REST_COMPRESSION_BROTLI) { /* GCOVR_EXCL_LINE */
    size_t available_in = in_len;                   /* GCOVR_EXCL_LINE */
    const uint8_t *next_in = in_data;               /* GCOVR_EXCL_LINE */
    size_t available_out;
    uint8_t *next_out;

    out_capacity = in_len + 4096; /* GCOVR_EXCL_LINE */
    if (C_REST_MALLOC(out_capacity, &out_buf) != 0 ||
        !out_buf)                  /* GCOVR_EXCL_LINE */
      return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

    available_out = out_capacity; /* GCOVR_EXCL_LINE */
    next_out = out_buf;           /* GCOVR_EXCL_LINE */

    while (available_in > 0) {              /* GCOVR_EXCL_LINE */
      if (available_out == 0) {             /* GCOVR_EXCL_LINE */
        unsigned char *new_buf = NULL;      /* GCOVR_EXCL_LINE */
        size_t offset = next_out - out_buf; /* GCOVR_EXCL_LINE */
        out_capacity *= 2;                  /* GCOVR_EXCL_LINE */
        if (C_REST_REALLOC(out_buf, out_capacity, &new_buf) != 0 ||
            !new_buf) {                /* GCOVR_EXCL_LINE */
          C_REST_FREE(out_buf);        /* GCOVR_EXCL_LINE */
          return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
        }
        out_buf = new_buf;                     /* GCOVR_EXCL_LINE */
        next_out = out_buf + offset;           /* GCOVR_EXCL_LINE */
        available_out = out_capacity - offset; /* GCOVR_EXCL_LINE */
      }

      if (!BrotliEncoderCompressStream(
              ctx->b_strm, BROTLI_OPERATION_PROCESS, /* GCOVR_EXCL_LINE */
              &available_in, &next_in, &available_out, &next_out, NULL)) {
        C_REST_FREE(out_buf);        /* GCOVR_EXCL_LINE */
        return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
      }
    }

    *out_data = out_buf;           /* GCOVR_EXCL_LINE */
    *out_len = next_out - out_buf; /* GCOVR_EXCL_LINE */
    return C_REST_OK;              /* GCOVR_EXCL_LINE */
  }
#endif
  return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
#else
  return C_REST_ERROR_GENERIC;
#endif
}

c_rest_error_t
c_rest_compress_finish(c_rest_compression_ctx_t *ctx, /* GCOVR_EXCL_LINE */
                       unsigned char **out_data, size_t *out_len) {
#if (defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP) ||             \
     defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI))
  size_t out_capacity = 4096;    /* GCOVR_EXCL_LINE */
  unsigned char *out_buf = NULL; /* GCOVR_EXCL_LINE */
  size_t total_out = 0;          /* GCOVR_EXCL_LINE */

  if (!ctx || !out_data || !out_len) /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;     /* GCOVR_EXCL_LINE */
  *out_data = NULL;                  /* GCOVR_EXCL_LINE */
  *out_len = 0;                      /* GCOVR_EXCL_LINE */

  if (C_REST_MALLOC(out_capacity, &out_buf) != 0 ||
      !out_buf)                  /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP)
  if (ctx->type == C_REST_COMPRESSION_GZIP) { /* GCOVR_EXCL_LINE */
    ctx->z_strm.avail_in = 0;                 /* GCOVR_EXCL_LINE */
    ctx->z_strm.next_in = Z_NULL;             /* GCOVR_EXCL_LINE */

    do {
      if (ctx->z_strm.avail_out == 0) { /* GCOVR_EXCL_LINE */
        unsigned char *new_buf = NULL;  /* GCOVR_EXCL_LINE */
        out_capacity *= 2;              /* GCOVR_EXCL_LINE */
        if (C_REST_REALLOC(out_buf, out_capacity, &new_buf) != 0 ||
            !new_buf) {                /* GCOVR_EXCL_LINE */
          C_REST_FREE(out_buf);        /* GCOVR_EXCL_LINE */
          return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
        }
        out_buf = new_buf; /* GCOVR_EXCL_LINE */
      }

      ctx->z_strm.avail_out =
          (uInt)(out_capacity - total_out);       /* GCOVR_EXCL_LINE */
      ctx->z_strm.next_out = out_buf + total_out; /* GCOVR_EXCL_LINE */

      if (deflate(&ctx->z_strm, Z_FINISH) ==
          Z_STREAM_ERROR) {          /* GCOVR_EXCL_LINE */
        C_REST_FREE(out_buf);        /* GCOVR_EXCL_LINE */
        return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
      }

      total_out = out_capacity - ctx->z_strm.avail_out; /* GCOVR_EXCL_LINE */
    } while (ctx->z_strm.avail_out == 0); /* GCOVR_EXCL_LINE */

    *out_data = out_buf;  /* GCOVR_EXCL_LINE */
    *out_len = total_out; /* GCOVR_EXCL_LINE */
    return C_REST_OK;     /* GCOVR_EXCL_LINE */
  } else
#endif
#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI)
      if (ctx->type == C_REST_COMPRESSION_BROTLI) { /* GCOVR_EXCL_LINE */
    size_t available_in = 0;                        /* GCOVR_EXCL_LINE */
    const uint8_t *next_in = NULL;                  /* GCOVR_EXCL_LINE */
    size_t available_out = out_capacity;            /* GCOVR_EXCL_LINE */
    uint8_t *next_out = out_buf;                    /* GCOVR_EXCL_LINE */

    while (1) {
      if (available_out == 0) {             /* GCOVR_EXCL_LINE */
        unsigned char *new_buf = NULL;      /* GCOVR_EXCL_LINE */
        size_t offset = next_out - out_buf; /* GCOVR_EXCL_LINE */
        out_capacity *= 2;                  /* GCOVR_EXCL_LINE */
        if (C_REST_REALLOC(out_buf, out_capacity, &new_buf) != 0 ||
            !new_buf) {                /* GCOVR_EXCL_LINE */
          C_REST_FREE(out_buf);        /* GCOVR_EXCL_LINE */
          return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
        }
        out_buf = new_buf;                     /* GCOVR_EXCL_LINE */
        next_out = out_buf + offset;           /* GCOVR_EXCL_LINE */
        available_out = out_capacity - offset; /* GCOVR_EXCL_LINE */
      }

      if (!BrotliEncoderCompressStream(
              ctx->b_strm, BROTLI_OPERATION_FINISH, /* GCOVR_EXCL_LINE */
              &available_in, &next_in, &available_out, &next_out, NULL)) {
        C_REST_FREE(out_buf);        /* GCOVR_EXCL_LINE */
        return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
      }

      if (BrotliEncoderIsFinished(ctx->b_strm)) { /* GCOVR_EXCL_LINE */
        break;                                    /* GCOVR_EXCL_LINE */
      }
    }

    *out_data = out_buf;           /* GCOVR_EXCL_LINE */
    *out_len = next_out - out_buf; /* GCOVR_EXCL_LINE */
    return C_REST_OK;              /* GCOVR_EXCL_LINE */
  }
#endif
  C_REST_FREE(out_buf);        /* GCOVR_EXCL_LINE */
  return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
#else
  return C_REST_ERROR_GENERIC;
#endif
}

c_rest_error_t
c_rest_compress_buffer(c_rest_compression_type_t type, /* GCOVR_EXCL_LINE */
                       const unsigned char *in_data, size_t in_len,
                       unsigned char **out_data, size_t *out_len) {
  c_rest_compression_ctx_t *ctx = NULL; /* GCOVR_EXCL_LINE */
  unsigned char *data1 = NULL;          /* GCOVR_EXCL_LINE */
  size_t len1 = 0;                      /* GCOVR_EXCL_LINE */
  unsigned char *data2 = NULL;          /* GCOVR_EXCL_LINE */
  size_t len2 = 0;                      /* GCOVR_EXCL_LINE */

  if (!in_data || !out_data || !out_len) /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;         /* GCOVR_EXCL_LINE */
  *out_data = NULL;                      /* GCOVR_EXCL_LINE */
  *out_len = 0;                          /* GCOVR_EXCL_LINE */

  if (type == C_REST_COMPRESSION_NONE) /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;       /* GCOVR_EXCL_LINE */

  if (c_rest_compression_ctx_init(&ctx, type) != 0) /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;                    /* GCOVR_EXCL_LINE */

  if (c_rest_compress_data(ctx, in_data, in_len, &data1, &len1) !=
      0) {                               /* GCOVR_EXCL_LINE */
    c_rest_compression_ctx_destroy(ctx); /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;         /* GCOVR_EXCL_LINE */
  }

  if (c_rest_compress_finish(ctx, &data2, &len2) != 0) { /* GCOVR_EXCL_LINE */
    if (data1)                                           /* GCOVR_EXCL_LINE */
      C_REST_FREE(data1);                                /* GCOVR_EXCL_LINE */
    c_rest_compression_ctx_destroy(ctx);                 /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;                         /* GCOVR_EXCL_LINE */
  }

  if (C_REST_MALLOC(len1 + len2, out_data) != 0 ||
      !*out_data) {                      /* GCOVR_EXCL_LINE */
    if (data1)                           /* GCOVR_EXCL_LINE */
      C_REST_FREE(data1);                /* GCOVR_EXCL_LINE */
    if (data2)                           /* GCOVR_EXCL_LINE */
      C_REST_FREE(data2);                /* GCOVR_EXCL_LINE */
    c_rest_compression_ctx_destroy(ctx); /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;         /* GCOVR_EXCL_LINE */
  }

  if (len1 > 0) { /* GCOVR_EXCL_LINE */
    /* c_rest_mem does not have a memcpy directly, using loop or standard memcpy
     */
    size_t i;
    for (i = 0; i < len1; ++i) { /* GCOVR_EXCL_LINE */
      (*out_data)[i] = data1[i]; /* GCOVR_EXCL_LINE */
    }
  }
  if (len2 > 0) { /* GCOVR_EXCL_LINE */
    size_t i;
    for (i = 0; i < len2; ++i) {        /* GCOVR_EXCL_LINE */
      (*out_data)[len1 + i] = data2[i]; /* GCOVR_EXCL_LINE */
    }
  }
  *out_len = len1 + len2; /* GCOVR_EXCL_LINE */

  if (data1)                           /* GCOVR_EXCL_LINE */
    C_REST_FREE(data1);                /* GCOVR_EXCL_LINE */
  if (data2)                           /* GCOVR_EXCL_LINE */
    C_REST_FREE(data2);                /* GCOVR_EXCL_LINE */
  c_rest_compression_ctx_destroy(ctx); /* GCOVR_EXCL_LINE */
  return C_REST_OK;                    /* GCOVR_EXCL_LINE */
}
