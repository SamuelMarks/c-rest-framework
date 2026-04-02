/* clang-format off */
#include "test_protos.h"
#include "../include/c_rest_compression.h"
#include "../include/c_rest_mem.h"
#include <stdio.h>
#include <string.h>

#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP)
#include <zlib.h>
#endif
#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI)
#include <brotli/decode.h>
#endif
/* clang-format on */

#define ASSERT(cond)                                                           \
  do {                                                                         \
    if (!(cond)) {                                                             \
      printf("ASSERT FAILED: %s at %d\n", #cond, __LINE__);                    \
      return 1;                                                                \
    }                                                                          \
  } while (0)
#define ASSERT_EQ(exp, act) ASSERT((exp) == (act))

static int test_compression_none(void) {
  c_rest_compression_ctx_t *ctx = NULL;
  int res;

  res = c_rest_compression_ctx_init(&ctx, C_REST_COMPRESSION_NONE);
  ASSERT_EQ(0, res);
  ASSERT_EQ(NULL, ctx);

  res = c_rest_compression_ctx_destroy(ctx);
  ASSERT_EQ(1, res); /* Cannot destroy NULL */

  return 0;
}

#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP)
static int test_compression_gzip_basic(void) {
  const char *test_data =
      "Hello, world! This is a test string to be compressed with GZIP.";
  size_t in_len = strlen(test_data);
  unsigned char *comp_data = NULL;
  size_t comp_len = 0;
  int res;

  /* Compress */
  res = c_rest_compress_buffer(C_REST_COMPRESSION_GZIP,
                               (const unsigned char *)test_data, in_len,
                               &comp_data, &comp_len);
  ASSERT_EQ(0, res);
  ASSERT(comp_data != NULL);
  ASSERT(comp_len > 0);

  /* Verify with zlib directly */
  {
    unsigned char decomp_data[256];
    unsigned long decomp_len = sizeof(decomp_data);
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = (uInt)comp_len;
    strm.next_in = (Bytef *)comp_data;
    /* 15 + 16 for gzip */
    ASSERT_EQ(Z_OK, inflateInit2(&strm, 15 + 16));
    strm.avail_out = decomp_len;
    strm.next_out = decomp_data;
    ASSERT_EQ(Z_STREAM_END, inflate(&strm, Z_NO_FLUSH));
    ASSERT_EQ(Z_OK, inflateEnd(&strm));

    decomp_len = sizeof(decomp_data) - strm.avail_out;
    ASSERT_EQ(in_len, decomp_len);
    ASSERT_EQ(0, memcmp(test_data, decomp_data, in_len));
  }

  C_REST_FREE(comp_data);
  return 0;
}
#endif

#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI)
static int test_compression_brotli_basic(void) {
  const char *test_data =
      "Hello, world! This is a test string to be compressed with BROTLI.";
  size_t in_len = strlen(test_data);
  unsigned char *comp_data = NULL;
  size_t comp_len = 0;
  int res;

  /* Compress */
  res = c_rest_compress_buffer(C_REST_COMPRESSION_BROTLI,
                               (const unsigned char *)test_data, in_len,
                               &comp_data, &comp_len);
  ASSERT_EQ(0, res);
  ASSERT(comp_data != NULL);
  ASSERT(comp_len > 0);

  /* Verify with brotli directly */
  {
    unsigned char decomp_data[256];
    size_t decomp_len = sizeof(decomp_data);
    BrotliDecoderResult b_res =
        BrotliDecoderDecompress(comp_len, comp_data, &decomp_len, decomp_data);
    ASSERT_EQ(BROTLI_DECODER_RESULT_SUCCESS, b_res);
    ASSERT_EQ(in_len, decomp_len);
    ASSERT_EQ(0, memcmp(test_data, decomp_data, in_len));
  }

  C_REST_FREE(comp_data);
  return 0;
}
#endif

static int test_compression_errors(void) {
  int res;
  unsigned char *comp_data = NULL;
  size_t comp_len = 0;
  c_rest_compression_ctx_t *ctx = NULL;

  res = c_rest_compression_ctx_init(NULL, C_REST_COMPRESSION_GZIP);
  ASSERT_EQ(1, res);

  res = c_rest_compression_ctx_destroy(NULL);
  ASSERT_EQ(1, res);

  res = c_rest_compress_data(NULL, (const unsigned char *)"abc", 3, &comp_data,
                             &comp_len);
  ASSERT_EQ(1, res);

  res = c_rest_compression_ctx_init(&ctx, C_REST_COMPRESSION_GZIP);
#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP)
  ASSERT_EQ(0, res);

  res = c_rest_compress_data(ctx, NULL, 3, &comp_data, &comp_len);
  ASSERT_EQ(1, res);

  res = c_rest_compress_data(ctx, (const unsigned char *)"abc", 3, NULL,
                             &comp_len);
  ASSERT_EQ(1, res);

  res = c_rest_compress_data(ctx, (const unsigned char *)"abc", 3, &comp_data,
                             NULL);
  ASSERT_EQ(1, res);

  res = c_rest_compress_finish(NULL, &comp_data, &comp_len);
  ASSERT_EQ(1, res);

  res = c_rest_compress_finish(ctx, NULL, &comp_len);
  ASSERT_EQ(1, res);

  res = c_rest_compress_finish(ctx, &comp_data, NULL);
  ASSERT_EQ(1, res);

  c_rest_compression_ctx_destroy(ctx);
#else
  ASSERT_EQ(1, res);
#endif

  res = c_rest_compress_buffer(C_REST_COMPRESSION_GZIP, NULL, 3, &comp_data,
                               &comp_len);
  ASSERT_EQ(1, res);

  res =
      c_rest_compress_buffer(C_REST_COMPRESSION_GZIP,
                             (const unsigned char *)"abc", 3, NULL, &comp_len);
  ASSERT_EQ(1, res);

  res =
      c_rest_compress_buffer(C_REST_COMPRESSION_GZIP,
                             (const unsigned char *)"abc", 3, &comp_data, NULL);
  ASSERT_EQ(1, res);

  return 0;
}

int test_response_compression_gzip_brotli(void) {
  if (test_compression_errors() != 0)
    return 1;
  if (test_compression_none() != 0)
    return 1;
#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP)
  if (test_compression_gzip_basic() != 0)
    return 1;
#endif
#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI)
  if (test_compression_brotli_basic() != 0)
    return 1;
#endif
  return 0;
}
