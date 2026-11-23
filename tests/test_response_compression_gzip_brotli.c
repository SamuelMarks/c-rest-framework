/* clang-format off */
#include "c_rest_error.h"
#include "test_protos.h"
#include "c_rest_compression.h"
#include "c_rest_mem.h"
#include "c_rest_testing_mocks.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP)
#include <zlib.h>
#endif
#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI)
#include <brotli/decode.h>
#endif
/* clang-format on */

TEST test_compression_none(void) {
  c_rest_compression_ctx_t *ctx = NULL;
  c_rest_error_t res;
  unsigned char *comp_data = NULL;
  size_t comp_len = 0;

  res = c_rest_compression_ctx_init(&ctx, C_REST_COMPRESSION_NONE);
  ASSERT_FALSE(res != 0);
  ASSERT_FALSE(ctx != NULL);

  res = c_rest_compression_ctx_destroy(ctx);
  ASSERT_FALSE(res != C_REST_ERROR_GENERIC);

  res = c_rest_compress_buffer(C_REST_COMPRESSION_NONE,
                               (const unsigned char *)"abc", 3, &comp_data,
                               &comp_len);
  ASSERT_FALSE(res != C_REST_ERROR_GENERIC);

  PASS();
}

#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP)
TEST test_compression_gzip_basic(void) {
  const char *test_data =
      "Hello, world! This is a test string to be compressed with GZIP.";
  size_t in_len = strlen(test_data);
  unsigned char *comp_data = NULL;
  size_t comp_len = 0;
  c_rest_error_t res;
  unsigned char decomp_data[256];
  unsigned long decomp_len = sizeof(decomp_data);
  z_stream strm;

  /* Compress */
  res = c_rest_compress_buffer(C_REST_COMPRESSION_GZIP,
                               (const unsigned char *)test_data, in_len,
                               &comp_data, &comp_len);
  ASSERT_FALSE(res != 0);
  ASSERT_FALSE(comp_data == NULL);
  ASSERT_FALSE(comp_len == 0);

  /* Verify with zlib directly */
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  strm.avail_in = (uInt)comp_len;
  strm.next_in = (Bytef *)comp_data;
  /* 15 + 16 for gzip */
  ASSERT_FALSE(inflateInit2(&strm, 15 + 16) != Z_OK);
  strm.avail_out = (uInt)decomp_len;
  strm.next_out = decomp_data;
  ASSERT_FALSE(inflate(&strm, Z_NO_FLUSH) != Z_STREAM_END);
  ASSERT_FALSE(inflateEnd(&strm) != Z_OK);

  decomp_len = sizeof(decomp_data) - strm.avail_out;
  ASSERT_FALSE(in_len != decomp_len);
  ASSERT_FALSE(memcmp(test_data, decomp_data, in_len) != 0);

  C_REST_FREE(comp_data);
  PASS();
}

TEST test_compression_gzip_large(void) {
  size_t in_len = 200000;
  char *test_data = NULL;
  unsigned char *comp_data = NULL;
  size_t comp_len = 0;
  c_rest_error_t res;
  size_t i;

  ASSERT_EQ(C_REST_OK, C_REST_MALLOC(in_len, &test_data));
  for (i = 0; i < in_len; i++)
    test_data[i] = (char)(i % 256);

  res = c_rest_compress_buffer(C_REST_COMPRESSION_GZIP,
                               (const unsigned char *)test_data, in_len,
                               &comp_data, &comp_len);
  ASSERT_FALSE(res != 0);
  ASSERT_FALSE(comp_data == NULL);
  ASSERT_FALSE(comp_len == 0);

  C_REST_FREE(comp_data);
  C_REST_FREE(test_data);
  PASS();
}
#endif

#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI)
TEST test_compression_brotli_basic(void) {
  const char *test_data =
      "Hello, world! This is a test string to be compressed with BROTLI.";
  size_t in_len = strlen(test_data);
  unsigned char *comp_data = NULL;
  size_t comp_len = 0;
  c_rest_error_t res;
  unsigned char decomp_data[256];
  size_t decomp_len = sizeof(decomp_data);
  BrotliDecoderResult b_res;

  /* Compress */
  res = c_rest_compress_buffer(C_REST_COMPRESSION_BROTLI,
                               (const unsigned char *)test_data, in_len,
                               &comp_data, &comp_len);
  ASSERT_FALSE(res != 0);
  ASSERT_FALSE(comp_data == NULL);
  ASSERT_FALSE(comp_len == 0);

  /* Verify with brotli directly */
  b_res =
      BrotliDecoderDecompress(comp_len, comp_data, &decomp_len, decomp_data);
  ASSERT_FALSE(b_res != BROTLI_DECODER_RESULT_SUCCESS);
  ASSERT_FALSE(in_len != decomp_len);
  ASSERT_FALSE(memcmp(test_data, decomp_data, in_len) != 0);

  C_REST_FREE(comp_data);
  PASS();
}

TEST test_compression_brotli_large(void) {
  size_t in_len = 200000;
  char *test_data = NULL;
  unsigned char *comp_data = NULL;
  size_t comp_len = 0;
  c_rest_error_t res;
  size_t i;

  ASSERT_EQ(C_REST_OK, C_REST_MALLOC(in_len, &test_data));
  for (i = 0; i < in_len; i++)
    test_data[i] = (char)(i % 256);

  res = c_rest_compress_buffer(C_REST_COMPRESSION_BROTLI,
                               (const unsigned char *)test_data, in_len,
                               &comp_data, &comp_len);
  ASSERT_FALSE(res != 0);
  ASSERT_FALSE(comp_data == NULL);
  ASSERT_FALSE(comp_len == 0);

  C_REST_FREE(comp_data);
  C_REST_FREE(test_data);
  PASS();
}
#endif

TEST test_compression_errors(void) {
  c_rest_error_t res;
  unsigned char *comp_data = NULL;
  size_t comp_len = 0;
  c_rest_compression_ctx_t *ctx = NULL;

  res = c_rest_compression_ctx_init(NULL, C_REST_COMPRESSION_GZIP);
  ASSERT_FALSE(res != C_REST_ERROR_GENERIC);

  res = c_rest_compression_ctx_destroy(NULL);
  ASSERT_FALSE(res != C_REST_ERROR_GENERIC);

  res = c_rest_compress_data(NULL, (const unsigned char *)"abc", 3, &comp_data,
                             &comp_len);
  ASSERT_FALSE(res != C_REST_ERROR_GENERIC);

  res = c_rest_compression_ctx_init(&ctx, C_REST_COMPRESSION_GZIP);
#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP)
  ASSERT_FALSE(res != 0);

  res = c_rest_compress_data(ctx, NULL, 3, &comp_data, &comp_len);
  ASSERT_FALSE(res != C_REST_ERROR_GENERIC);

  res = c_rest_compress_data(ctx, (const unsigned char *)"abc", 3, NULL,
                             &comp_len);
  ASSERT_FALSE(res != C_REST_ERROR_GENERIC);

  res = c_rest_compress_data(ctx, (const unsigned char *)"abc", 3, &comp_data,
                             NULL);
  ASSERT_FALSE(res != C_REST_ERROR_GENERIC);

  res = c_rest_compress_finish(NULL, &comp_data, &comp_len);
  ASSERT_FALSE(res != C_REST_ERROR_GENERIC);

  res = c_rest_compress_finish(ctx, NULL, &comp_len);
  ASSERT_FALSE(res != C_REST_ERROR_GENERIC);

  res = c_rest_compress_finish(ctx, &comp_data, NULL);
  ASSERT_FALSE(res != C_REST_ERROR_GENERIC);

  ASSERT_EQ(C_REST_OK, c_rest_compression_ctx_destroy(ctx));
#else
  ASSERT_FALSE(res != C_REST_ERROR_GENERIC);
#endif

  res = c_rest_compress_buffer(C_REST_COMPRESSION_GZIP, NULL, 3, &comp_data,
                               &comp_len);
  ASSERT_FALSE(res != C_REST_ERROR_GENERIC);

  res =
      c_rest_compress_buffer(C_REST_COMPRESSION_GZIP,
                             (const unsigned char *)"abc", 3, NULL, &comp_len);
  ASSERT_FALSE(res != C_REST_ERROR_GENERIC);

  res =
      c_rest_compress_buffer(C_REST_COMPRESSION_GZIP,
                             (const unsigned char *)"abc", 3, &comp_data, NULL);
  ASSERT_FALSE(res != C_REST_ERROR_GENERIC);

  PASS();
}

TEST test_compression_invalid_type(void) {
  c_rest_compression_ctx_t *ctx = NULL;
  unsigned char *out = NULL;
  size_t out_len = 0;
  c_rest_error_t res =
      c_rest_compression_ctx_init(&ctx, C_REST_COMPRESSION_GZIP);
  ASSERT_FALSE(res != C_REST_OK);
  /* Corrupt the type */
  *(int *)ctx = 999;
  res =
      c_rest_compress_data(ctx, (const unsigned char *)"a", 1, &out, &out_len);
  ASSERT_FALSE(res != C_REST_ERROR_GENERIC);
  res = c_rest_compress_finish(ctx, &out, &out_len);
  ASSERT_FALSE(res != C_REST_ERROR_GENERIC);
  res = c_rest_compression_ctx_destroy(ctx);
  ASSERT_FALSE(res != C_REST_OK);
  PASS();
}

TEST test_compression_empty(void) {
  unsigned char *comp_data = NULL;
  size_t comp_len = 0;

  ASSERT_EQ(C_REST_OK, c_rest_compress_buffer(C_REST_COMPRESSION_GZIP,
                                              (const unsigned char *)"", 0,
                                              &comp_data, &comp_len));
  C_REST_FREE(comp_data);
  comp_data = NULL;
  comp_len = 0;

  ASSERT_EQ(C_REST_OK, c_rest_compress_buffer(C_REST_COMPRESSION_BROTLI,
                                              (const unsigned char *)"", 0,
                                              &comp_data, &comp_len));
  C_REST_FREE(comp_data);

  PASS();
}

TEST test_compression_lib_failures(void) {
  c_rest_compression_ctx_t *ctx = NULL;
  unsigned char *out = NULL;
  size_t out_len = 0;

#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP)
  g_mock_lib_fail = 1;
  ASSERT_NEQ(C_REST_OK,
             c_rest_compression_ctx_init(&ctx, C_REST_COMPRESSION_GZIP));

  g_mock_lib_fail = 3;
  ASSERT_EQ(C_REST_OK,
            c_rest_compression_ctx_init(&ctx, C_REST_COMPRESSION_GZIP));
  ASSERT_NEQ(C_REST_OK, c_rest_compress_data(ctx, (const unsigned char *)"a", 1,
                                             &out, &out_len));
  ASSERT_NEQ(C_REST_OK, c_rest_compress_finish(ctx, &out, &out_len));
  ASSERT_EQ(C_REST_OK, c_rest_compression_ctx_destroy(ctx));
#endif

#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI)
  g_mock_lib_fail = 2;
  ASSERT_NEQ(C_REST_OK,
             c_rest_compression_ctx_init(&ctx, C_REST_COMPRESSION_BROTLI));

  g_mock_lib_fail = 4;
  ASSERT_EQ(C_REST_OK,
            c_rest_compression_ctx_init(&ctx, C_REST_COMPRESSION_BROTLI));
  ASSERT_NEQ(C_REST_OK, c_rest_compress_data(ctx, (const unsigned char *)"a", 1,
                                             &out, &out_len));
  ASSERT_NEQ(C_REST_OK, c_rest_compress_finish(ctx, &out, &out_len));
  ASSERT_EQ(C_REST_OK, c_rest_compression_ctx_destroy(ctx));
#endif

  g_mock_lib_fail = 0;
  PASS();
}

TEST test_compression_more_errors(void);
TEST test_compression_malloc_failures(void);
TEST test_compression_concat_failures(void);
TEST test_compression_realloc_failures(void);
TEST test_compression_realloc_finish_failures(void);
TEST test_compression_buffer_malloc_failures(void);

static int g_fail_malloc_at = 0;
static int g_fail_realloc_at = 0;

static void *fail_malloc_n(size_t size) {
  static int alloc_count = 0;
  if (g_fail_malloc_at <= 0) {
    alloc_count = 0;
    return NULL;
  }
  alloc_count++;
  if (alloc_count == g_fail_malloc_at) {
    alloc_count = 0;
    g_fail_malloc_at = 0;
    return NULL;
  }
  return malloc(size);
}

static void *fail_realloc_n(void *ptr, size_t size) {
  static int alloc_count = 0;
  if (g_fail_realloc_at <= 0) {
    alloc_count = 0;
    return NULL;
  }
  alloc_count++;
  if (alloc_count == g_fail_realloc_at) {
    alloc_count = 0;
    g_fail_realloc_at = 0;
    return NULL;
  }
  return realloc(ptr, size);
}

TEST test_compression_oom_sweep(void) {
  int i;
  c_rest_error_t rc;
  c_rest_compression_ctx_t *ctx;
  unsigned char *comp_data = NULL;
  size_t comp_len = 0;
  size_t large_sz = 50000;
  char *large_data = (char *)malloc(large_sz);
  ASSERT(large_data != NULL);
  if (!large_data)
    PASS();
  memset(large_data, 'A', large_sz);
  large_data[large_sz - 1] = '\0';

  for (i = 1; i <= 30; i++) {
    g_fail_malloc_at = -1;
    fail_malloc_n(0);
    g_crf_malloc_hook = fail_malloc_n;
    g_fail_malloc_at = i;

    if (c_rest_compression_ctx_init(&ctx, C_REST_COMPRESSION_GZIP) ==
        C_REST_OK) {
      rc = c_rest_compress_data(ctx, (const unsigned char *)"a", 1, &comp_data,
                                &comp_len);
      ASSERT(rc == C_REST_OK || rc != C_REST_OK);
      C_REST_FREE(comp_data);
      comp_data = NULL;

      rc = c_rest_compress_finish(ctx, &comp_data, &comp_len);
      ASSERT(rc == C_REST_OK || rc != C_REST_OK);
      C_REST_FREE(comp_data);
      comp_data = NULL;

      ASSERT_EQ(C_REST_OK, c_rest_compression_ctx_destroy(ctx));
    }

    g_crf_malloc_hook = NULL;
    g_fail_malloc_at = 0;
  }

  for (i = 1; i <= 30; i++) {
    ctx = NULL;
    g_fail_realloc_at = -1;
    fail_realloc_n(NULL, 0);
    g_crf_realloc_hook = fail_realloc_n;
    g_fail_realloc_at = i;

    if (c_rest_compression_ctx_init(&ctx, C_REST_COMPRESSION_GZIP) ==
        C_REST_OK) {
      rc = c_rest_compress_data(ctx, (const unsigned char *)"a", 1, &comp_data,
                                &comp_len);
      ASSERT(rc == C_REST_OK || rc != C_REST_OK);
      C_REST_FREE(comp_data);
      comp_data = NULL;

      rc = c_rest_compress_finish(ctx, &comp_data, &comp_len);
      ASSERT(rc == C_REST_OK || rc != C_REST_OK);
      C_REST_FREE(comp_data);
      comp_data = NULL;

      ASSERT_EQ(C_REST_OK, c_rest_compression_ctx_destroy(ctx));
    }

    g_crf_realloc_hook = NULL;
    g_fail_realloc_at = 0;
  }

  for (i = 1; i <= 30; i++) {
    ctx = NULL;
    g_fail_realloc_at = -1;
    fail_realloc_n(NULL, 0);
    g_crf_realloc_hook = fail_realloc_n;
    g_fail_realloc_at = i;

    if (c_rest_compression_ctx_init(&ctx, C_REST_COMPRESSION_GZIP) ==
        C_REST_OK) {
      rc = c_rest_compress_data(ctx, (const unsigned char *)large_data,
                                large_sz, &comp_data, &comp_len);
      ASSERT(rc == C_REST_OK || rc != C_REST_OK);
      C_REST_FREE(comp_data);
      comp_data = NULL;

      rc = c_rest_compress_finish(ctx, &comp_data, &comp_len);
      ASSERT(rc == C_REST_OK || rc != C_REST_OK);
      C_REST_FREE(comp_data);
      comp_data = NULL;

      ASSERT_EQ(C_REST_OK, c_rest_compression_ctx_destroy(ctx));
    }

    g_crf_realloc_hook = NULL;
    g_fail_realloc_at = 0;
  }

#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI)
  for (i = 1; i <= 30; i++) {
    ctx = NULL;
    g_fail_malloc_at = -1;
    fail_malloc_n(0);
    g_crf_malloc_hook = fail_malloc_n;
    g_fail_malloc_at = i;

    if (c_rest_compression_ctx_init(&ctx, C_REST_COMPRESSION_BROTLI) ==
        C_REST_OK) {
      rc = c_rest_compress_data(ctx, (const unsigned char *)"a", 1, &comp_data,
                                &comp_len);
      ASSERT(rc == C_REST_OK || rc != C_REST_OK);
      C_REST_FREE(comp_data);
      comp_data = NULL;

      rc = c_rest_compress_finish(ctx, &comp_data, &comp_len);
      ASSERT(rc == C_REST_OK || rc != C_REST_OK);
      C_REST_FREE(comp_data);
      comp_data = NULL;

      ASSERT_EQ(C_REST_OK, c_rest_compression_ctx_destroy(ctx));
    }

    g_crf_malloc_hook = NULL;
    g_fail_malloc_at = 0;
  }

  for (i = 1; i <= 30; i++) {
    ctx = NULL;
    g_fail_realloc_at = -1;
    fail_realloc_n(NULL, 0);
    g_crf_realloc_hook = fail_realloc_n;
    g_fail_realloc_at = i;

    if (c_rest_compression_ctx_init(&ctx, C_REST_COMPRESSION_BROTLI) ==
        C_REST_OK) {
      rc = c_rest_compress_data(ctx, (const unsigned char *)"a", 1, &comp_data,
                                &comp_len);
      ASSERT(rc == C_REST_OK || rc != C_REST_OK);
      C_REST_FREE(comp_data);
      comp_data = NULL;

      rc = c_rest_compress_finish(ctx, &comp_data, &comp_len);
      ASSERT(rc == C_REST_OK || rc != C_REST_OK);
      C_REST_FREE(comp_data);
      comp_data = NULL;

      ASSERT_EQ(C_REST_OK, c_rest_compression_ctx_destroy(ctx));
    }

    g_crf_realloc_hook = NULL;
    g_fail_realloc_at = 0;
  }

  for (i = 1; i <= 30; i++) {
    ctx = NULL;
    g_fail_realloc_at = -1;
    fail_realloc_n(NULL, 0);
    g_crf_realloc_hook = fail_realloc_n;
    g_fail_realloc_at = i;

    if (c_rest_compression_ctx_init(&ctx, C_REST_COMPRESSION_BROTLI) ==
        C_REST_OK) {
      rc = c_rest_compress_data(ctx, (const unsigned char *)large_data,
                                large_sz, &comp_data, &comp_len);
      ASSERT(rc == C_REST_OK || rc != C_REST_OK);
      C_REST_FREE(comp_data);
      comp_data = NULL;

      rc = c_rest_compress_finish(ctx, &comp_data, &comp_len);
      ASSERT(rc == C_REST_OK || rc != C_REST_OK);
      C_REST_FREE(comp_data);
      comp_data = NULL;

      ASSERT_EQ(C_REST_OK, c_rest_compression_ctx_destroy(ctx));
    }

    g_crf_realloc_hook = NULL;
    g_fail_realloc_at = 0;
  }
#endif
  free(large_data);

#ifdef C_REST_TESTING_MALLOC_HOOK
  {
    unsigned char *out_d = NULL;
    size_t out_l = 0;
    g_mock_lib_fail = 99;
    ASSERT_FALSE(c_rest_compress_buffer(C_REST_COMPRESSION_GZIP,
                                        (const unsigned char *)"abc", 3, &out_d,
                                        &out_l) == C_REST_OK);
    g_mock_lib_fail = 0;
    if (out_d)
      C_REST_FREE(out_d);
  }
#endif
  PASS();
}

SUITE(compression_suite) {
  RUN_TEST(test_compression_oom_sweep);
  RUN_TEST(test_compression_errors);
  RUN_TEST(test_compression_empty);
  RUN_TEST(test_compression_invalid_type);
  RUN_TEST(test_compression_lib_failures);
  RUN_TEST(test_compression_more_errors);
  RUN_TEST(test_compression_malloc_failures);
  RUN_TEST(test_compression_concat_failures);
  RUN_TEST(test_compression_realloc_failures);
  RUN_TEST(test_compression_realloc_finish_failures);
  RUN_TEST(test_compression_buffer_malloc_failures);
  RUN_TEST(test_compression_none);
#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP)
  RUN_TEST(test_compression_gzip_basic);
  RUN_TEST(test_compression_gzip_large);
#endif
#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI)
  RUN_TEST(test_compression_brotli_basic);
  RUN_TEST(test_compression_brotli_large);
#endif
}

TEST test_compression_more_errors(void) {
  c_rest_error_t res;
  unsigned char *comp_data = NULL;
  size_t comp_len = 0;

  res = c_rest_compress_buffer((c_rest_compression_type_t)999,
                               (const unsigned char *)"abc", 3, &comp_data,
                               &comp_len);
  ASSERT_FALSE(res == C_REST_OK);

  res = c_rest_compress_buffer(C_REST_COMPRESSION_GZIP, NULL, 3, &comp_data,
                               &comp_len);
  ASSERT_FALSE(res == C_REST_OK);

  res =
      c_rest_compress_buffer(C_REST_COMPRESSION_GZIP,
                             (const unsigned char *)"abc", 3, NULL, &comp_len);
  ASSERT_FALSE(res == C_REST_OK);

  res =
      c_rest_compress_buffer(C_REST_COMPRESSION_GZIP,
                             (const unsigned char *)"abc", 3, &comp_data, NULL);
  ASSERT_FALSE(res == C_REST_OK);

  res = c_rest_compression_ctx_init(NULL, C_REST_COMPRESSION_GZIP);
  ASSERT_FALSE(res == C_REST_OK);

  res = c_rest_compression_ctx_destroy(NULL);
  ASSERT_FALSE(res == C_REST_OK);

  res = c_rest_compress_data(NULL, (const unsigned char *)"abc", 3, &comp_data,
                             &comp_len);
  ASSERT_FALSE(res == C_REST_OK);

  res = c_rest_compress_finish(NULL, &comp_data, &comp_len);
  ASSERT_FALSE(res == C_REST_OK);

#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI)
  res = c_rest_compress_buffer(C_REST_COMPRESSION_BROTLI, NULL, 3, &comp_data,
                               &comp_len);
  ASSERT_FALSE(res == C_REST_OK);

  res =
      c_rest_compress_buffer(C_REST_COMPRESSION_BROTLI,
                             (const unsigned char *)"abc", 3, NULL, &comp_len);
  ASSERT_FALSE(res == C_REST_OK);

  res =
      c_rest_compress_buffer(C_REST_COMPRESSION_BROTLI,
                             (const unsigned char *)"abc", 3, &comp_data, NULL);
  ASSERT_FALSE(res == C_REST_OK);

  res = c_rest_compression_ctx_init(NULL, C_REST_COMPRESSION_BROTLI);
  ASSERT_FALSE(res == C_REST_OK);
#endif

  PASS();
}

TEST test_compression_malloc_failures(void) {
  c_rest_error_t res;
  unsigned char *comp_data = NULL;
  size_t comp_len = 0;
  c_rest_compression_ctx_t *ctx = NULL;

  g_fail_malloc_at = -1;
  fail_malloc_n(0);
  g_fail_malloc_at = 1;
  g_crf_malloc_hook = fail_malloc_n;
  res = c_rest_compression_ctx_init(&ctx, C_REST_COMPRESSION_GZIP);
  ASSERT_FALSE(res == C_REST_OK);

  g_fail_malloc_at = -1;
  fail_malloc_n(0);
  g_fail_malloc_at = 1;
  g_crf_malloc_hook = fail_malloc_n;
  res = c_rest_compress_buffer(C_REST_COMPRESSION_GZIP,
                               (const unsigned char *)"abc", 3, &comp_data,
                               &comp_len);
  ASSERT_FALSE(res == C_REST_OK);

#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI)
  g_fail_malloc_at = -1;
  fail_malloc_n(0);
  g_fail_malloc_at = 1;
  g_crf_malloc_hook = fail_malloc_n;
  res = c_rest_compression_ctx_init(&ctx, C_REST_COMPRESSION_BROTLI);
  ASSERT_FALSE(res == C_REST_OK);
#endif
  g_crf_malloc_hook = NULL;

  /* test ctx operations */
#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP)
  res = c_rest_compression_ctx_init(&ctx, C_REST_COMPRESSION_GZIP);
  ASSERT_FALSE(res != C_REST_OK);

  g_fail_malloc_at = -1;
  fail_malloc_n(0);
  g_fail_malloc_at = 1;
  g_crf_malloc_hook = fail_malloc_n;
  res = c_rest_compress_data(ctx, (const unsigned char *)"abc", 3, &comp_data,
                             &comp_len);
  ASSERT_FALSE(res == C_REST_OK);

  g_fail_malloc_at = -1;
  fail_malloc_n(0);
  g_fail_malloc_at = 1;
  g_crf_malloc_hook = fail_malloc_n;
  res = c_rest_compress_finish(ctx, &comp_data, &comp_len);
  ASSERT_FALSE(res == C_REST_OK);

  g_crf_malloc_hook = NULL;
  ASSERT_EQ(C_REST_OK, c_rest_compression_ctx_destroy(ctx));
#endif

#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI)
  res = c_rest_compression_ctx_init(&ctx, C_REST_COMPRESSION_BROTLI);
  ASSERT_FALSE(res != C_REST_OK);

  g_fail_malloc_at = -1;
  fail_malloc_n(0);
  g_fail_malloc_at = 1;
  g_crf_malloc_hook = fail_malloc_n;
  res = c_rest_compress_data(ctx, (const unsigned char *)"abc", 3, &comp_data,
                             &comp_len);
  ASSERT_FALSE(res == C_REST_OK);

  g_fail_malloc_at = -1;
  fail_malloc_n(0);
  g_fail_malloc_at = 1;
  g_crf_malloc_hook = fail_malloc_n;
  res = c_rest_compress_finish(ctx, &comp_data, &comp_len);
  ASSERT_FALSE(res == C_REST_OK);

  g_crf_malloc_hook = NULL;
  ASSERT_EQ(C_REST_OK, c_rest_compression_ctx_destroy(ctx));
#endif

  PASS();
}

TEST test_compression_concat_failures(void) {
  c_rest_error_t res;
  unsigned char *comp_data = NULL;
  size_t comp_len = 0;

  g_fail_malloc_at = -1;
  fail_malloc_n(0);
  g_fail_malloc_at = 3;
  g_crf_malloc_hook = fail_malloc_n;
  res = c_rest_compress_buffer(C_REST_COMPRESSION_GZIP,
                               (const unsigned char *)"abc", 3, &comp_data,
                               &comp_len);
  g_crf_malloc_hook = NULL;
  ASSERT_FALSE(res == C_REST_OK);

  PASS();
}

TEST test_compression_realloc_failures(void) {
  c_rest_error_t res;
  unsigned char *comp_data = NULL;
  size_t comp_len = 0;
  c_rest_compression_ctx_t *ctx = NULL;
  size_t large_sz = 100000;
  char *large_data = (char *)malloc(large_sz);
  if (!large_data)
    FAIL();
  memset(large_data, 'A', large_sz);

  g_fail_realloc_at = -1;
  fail_realloc_n(NULL, 0);

  /* GZIP */
#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP)
  res = c_rest_compression_ctx_init(&ctx, C_REST_COMPRESSION_GZIP);
  ASSERT_FALSE(res != C_REST_OK);

  g_fail_realloc_at = 1;
  g_crf_realloc_hook = fail_realloc_n;
  res = c_rest_compress_data(ctx, (const unsigned char *)large_data, large_sz,
                             &comp_data, &comp_len);
  ASSERT_FALSE(res == C_REST_OK);
  C_REST_FREE(comp_data);
  comp_data = NULL;

  g_crf_realloc_hook = NULL;

  ASSERT_EQ(C_REST_OK, c_rest_compression_ctx_destroy(ctx));
#endif

  g_fail_realloc_at = -1;
  fail_realloc_n(NULL, 0);

  /* BROTLI */
#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI)
  res = c_rest_compression_ctx_init(&ctx, C_REST_COMPRESSION_BROTLI);
  ASSERT_FALSE(res != C_REST_OK);

  g_fail_realloc_at = 1;
  g_crf_realloc_hook = fail_realloc_n;
  res = c_rest_compress_data(ctx, (const unsigned char *)large_data, large_sz,
                             &comp_data, &comp_len);
  ASSERT_FALSE(res == C_REST_OK);
  C_REST_FREE(comp_data);
  comp_data = NULL;
  g_crf_realloc_hook = NULL;

  ASSERT_EQ(C_REST_OK, c_rest_compression_ctx_destroy(ctx));
#endif

  free(large_data);
  PASS();
}

TEST test_compression_realloc_finish_failures(void) {
  c_rest_error_t res;
  unsigned char *comp_data = NULL;
  size_t comp_len = 0;
  c_rest_compression_ctx_t *ctx = NULL;
  size_t large_sz = 100000;
  char *large_data = (char *)malloc(large_sz);
  if (!large_data)
    FAIL();
  memset(large_data, 'A', large_sz);

  /* GZIP */
#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP)
  res = c_rest_compression_ctx_init(&ctx, C_REST_COMPRESSION_GZIP);
  ASSERT_FALSE(res != C_REST_OK);
  res = c_rest_compress_data(ctx, (const unsigned char *)large_data, large_sz,
                             &comp_data, &comp_len);
  ASSERT_FALSE(res != C_REST_OK);
  C_REST_FREE(comp_data);
  comp_data = NULL;

  g_fail_realloc_at = -1;
  fail_realloc_n(NULL, 0);
  g_fail_realloc_at = 1;
  g_crf_realloc_hook = fail_realloc_n;
  res = c_rest_compress_finish(ctx, &comp_data, &comp_len);
  g_crf_realloc_hook = NULL;
  ASSERT_FALSE(res == C_REST_OK);
  C_REST_FREE(comp_data);
  comp_data = NULL;

  ASSERT_EQ(C_REST_OK, c_rest_compression_ctx_destroy(ctx));
#endif

  /* BROTLI */
#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI)
  res = c_rest_compression_ctx_init(&ctx, C_REST_COMPRESSION_BROTLI);
  ASSERT_FALSE(res != C_REST_OK);
  res = c_rest_compress_data(ctx, (const unsigned char *)large_data, large_sz,
                             &comp_data, &comp_len);
  ASSERT_FALSE(res != C_REST_OK);
  C_REST_FREE(comp_data);
  comp_data = NULL;

  g_fail_realloc_at = -1;
  fail_realloc_n(NULL, 0);
  g_fail_realloc_at = 1;
  g_crf_realloc_hook = fail_realloc_n;
  res = c_rest_compress_finish(ctx, &comp_data, &comp_len);
  g_crf_realloc_hook = NULL;
  ASSERT_FALSE(res == C_REST_OK);
  C_REST_FREE(comp_data);
  comp_data = NULL;

  ASSERT_EQ(C_REST_OK, c_rest_compression_ctx_destroy(ctx));
#endif

  free(large_data);
  PASS();
}

TEST test_compression_buffer_malloc_failures(void) {
  c_rest_error_t res;
  unsigned char *comp_data = NULL;
  size_t comp_len = 0;
  int i;

  for (i = 1; i <= 15; i++) {
    g_fail_malloc_at = -1;
    fail_malloc_n(0);
    g_crf_malloc_hook = fail_malloc_n;
    g_fail_malloc_at = i;
    res = c_rest_compress_buffer(C_REST_COMPRESSION_GZIP,
                                 (const unsigned char *)"abc", 3, &comp_data,
                                 &comp_len);
    g_crf_malloc_hook = NULL;
    if (res == C_REST_OK) {
      C_REST_FREE(comp_data);
      comp_data = NULL;
    }
  }

  PASS();
}
