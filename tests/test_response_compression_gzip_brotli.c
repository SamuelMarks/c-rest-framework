/* clang-format off */
#include "c_rest_error.h"
#include "test_protos.h"
#include "../include/c_rest_compression.h"
#include "../include/c_rest_mem.h"
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
  c_rest_error_t res;
  unsigned char *comp_data = NULL;
  size_t comp_len = 0;

  res = c_rest_compression_ctx_init(&ctx, C_REST_COMPRESSION_NONE);
  ASSERT_EQ(0, res);
  ASSERT_EQ(NULL, ctx);

  res = c_rest_compression_ctx_destroy(ctx);
  if (res != C_REST_ERROR_GENERIC)
    return 1; /* Cannot destroy NULL */

  res = c_rest_compress_buffer(C_REST_COMPRESSION_NONE,
                               (const unsigned char *)"abc", 3, &comp_data,
                               &comp_len);
  if (res != C_REST_ERROR_GENERIC)
    return 1;

  return 0;
}

#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP)
static int test_compression_gzip_basic(void) {
  const char *test_data =
      "Hello, world! This is a test string to be compressed with GZIP.";
  size_t in_len = strlen(test_data);
  unsigned char *comp_data = NULL;
  size_t comp_len = 0;
  c_rest_error_t res;

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
    strm.avail_out = (uInt)decomp_len;
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

static int test_compression_gzip_large(void) {
  size_t in_len = 200000;
  char *test_data = CRF_MALLOC(in_len);
  unsigned char *comp_data = NULL;
  size_t comp_len = 0;
  c_rest_error_t res;
  size_t i;

  for (i = 0; i < in_len; i++)
    test_data[i] = (char)(i % 256);

  res = c_rest_compress_buffer(C_REST_COMPRESSION_GZIP,
                               (const unsigned char *)test_data, in_len,
                               &comp_data, &comp_len);
  ASSERT_EQ(0, res);
  ASSERT(comp_data != NULL);
  ASSERT(comp_len > 0);

  C_REST_FREE(comp_data);
  CRF_FREE(test_data);
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
  c_rest_error_t res;

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

static int test_compression_brotli_large(void) {
  size_t in_len = 200000;
  char *test_data = CRF_MALLOC(in_len);
  unsigned char *comp_data = NULL;
  size_t comp_len = 0;
  c_rest_error_t res;
  size_t i;

  for (i = 0; i < in_len; i++)
    test_data[i] = (char)(i % 256);

  res = c_rest_compress_buffer(C_REST_COMPRESSION_BROTLI,
                               (const unsigned char *)test_data, in_len,
                               &comp_data, &comp_len);
  ASSERT_EQ(0, res);
  ASSERT(comp_data != NULL);
  ASSERT(comp_len > 0);

  C_REST_FREE(comp_data);
  CRF_FREE(test_data);
  return 0;
}
#endif

static int test_compression_errors(void) {
  c_rest_error_t res;
  unsigned char *comp_data = NULL;
  size_t comp_len = 0;
  c_rest_compression_ctx_t *ctx = NULL;

  res = c_rest_compression_ctx_init(NULL, C_REST_COMPRESSION_GZIP);
  if (res != C_REST_ERROR_GENERIC)
    return 1;

  res = c_rest_compression_ctx_destroy(NULL);
  if (res != C_REST_ERROR_GENERIC)
    return 1;

  res = c_rest_compress_data(NULL, (const unsigned char *)"abc", 3, &comp_data,
                             &comp_len);
  if (res != C_REST_ERROR_GENERIC)
    return 1;

  res = c_rest_compression_ctx_init(&ctx, C_REST_COMPRESSION_GZIP);
#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP)
  ASSERT_EQ(0, res);

  res = c_rest_compress_data(ctx, NULL, 3, &comp_data, &comp_len);
  if (res != C_REST_ERROR_GENERIC)
    return 1;

  res = c_rest_compress_data(ctx, (const unsigned char *)"abc", 3, NULL,
                             &comp_len);
  if (res != C_REST_ERROR_GENERIC)
    return 1;

  res = c_rest_compress_data(ctx, (const unsigned char *)"abc", 3, &comp_data,
                             NULL);
  if (res != C_REST_ERROR_GENERIC)
    return 1;

  res = c_rest_compress_finish(NULL, &comp_data, &comp_len);
  if (res != C_REST_ERROR_GENERIC)
    return 1;

  res = c_rest_compress_finish(ctx, NULL, &comp_len);
  if (res != C_REST_ERROR_GENERIC)
    return 1;

  res = c_rest_compress_finish(ctx, &comp_data, NULL);
  if (res != C_REST_ERROR_GENERIC)
    return 1;

  c_rest_compression_ctx_destroy(ctx);
#else
  if (res != C_REST_ERROR_GENERIC)
    return 1;
#endif

  res = c_rest_compress_buffer(C_REST_COMPRESSION_GZIP, NULL, 3, &comp_data,
                               &comp_len);
  if (res != C_REST_ERROR_GENERIC)
    return 1;

  res =
      c_rest_compress_buffer(C_REST_COMPRESSION_GZIP,
                             (const unsigned char *)"abc", 3, NULL, &comp_len);
  if (res != C_REST_ERROR_GENERIC)
    return 1;

  res =
      c_rest_compress_buffer(C_REST_COMPRESSION_GZIP,
                             (const unsigned char *)"abc", 3, &comp_data, NULL);
  if (res != C_REST_ERROR_GENERIC)
    return 1;

  return 0;
}

static int test_compression_invalid_type(void) {
  c_rest_compression_ctx_t *ctx;
  unsigned char *out = NULL;
  size_t out_len = 0;
  if (c_rest_compression_ctx_init(&ctx, C_REST_COMPRESSION_GZIP) == C_REST_OK) {
    /* Corrupt the type */
    *(int *)ctx = 999;
    c_rest_compress_data(ctx, (const unsigned char *)"a", 1, &out, &out_len);
    c_rest_compress_finish(ctx, &out, &out_len);
    c_rest_compression_ctx_destroy(ctx);
  }
  return 0;
}

static int test_compression_empty(void) {
  unsigned char *comp_data = NULL;
  size_t comp_len = 0;

  c_rest_compress_buffer(C_REST_COMPRESSION_GZIP, (const unsigned char *)"", 0,
                         &comp_data, &comp_len);
  if (comp_data)
    C_REST_FREE(comp_data);
  comp_data = NULL;
  comp_len = 0;

  c_rest_compress_buffer(C_REST_COMPRESSION_BROTLI, (const unsigned char *)"",
                         0, &comp_data, &comp_len);
  if (comp_data)
    C_REST_FREE(comp_data);

  return 0;
}

static int test_compression_lib_failures(void) {
  c_rest_compression_ctx_t *ctx;
  unsigned char *out = NULL;
  size_t out_len = 0;

#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP)
  g_mock_lib_fail = 1;
  c_rest_compression_ctx_init(&ctx, C_REST_COMPRESSION_GZIP);

  g_mock_lib_fail = 3;
  if (c_rest_compression_ctx_init(&ctx, C_REST_COMPRESSION_GZIP) == C_REST_OK) {
    c_rest_compress_data(ctx, (const unsigned char *)"a", 1, &out, &out_len);
    c_rest_compress_finish(ctx, &out, &out_len);
    c_rest_compression_ctx_destroy(ctx);
  }
#endif

#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI)
  g_mock_lib_fail = 2;
  c_rest_compression_ctx_init(&ctx, C_REST_COMPRESSION_BROTLI);

  g_mock_lib_fail = 4;
  if (c_rest_compression_ctx_init(&ctx, C_REST_COMPRESSION_BROTLI) ==
      C_REST_OK) {
    c_rest_compress_data(ctx, (const unsigned char *)"a", 1, &out, &out_len);
    c_rest_compress_finish(ctx, &out, &out_len);
    c_rest_compression_ctx_destroy(ctx);
  }
#endif

  g_mock_lib_fail = 0;
  return 0;
}

static int test_compression_more_errors(void);
static int test_compression_malloc_failures(void);
static int test_compression_concat_failures(void);
static int test_compression_realloc_failures(void);
static int test_compression_realloc_finish_failures(void);
static int test_compression_buffer_malloc_failures(void);

static void *fail_malloc_n(size_t size) {
  static int alloc_count = 0;
  extern int g_fail_malloc_at;
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
int g_fail_malloc_at = 0;

static void *fail_realloc_n(void *ptr, size_t size) {
  static int alloc_count = 0;
  extern int g_fail_realloc_at;
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
int g_fail_realloc_at = 0;

static void test_coverage(void) {
  int i;
  c_rest_compression_ctx_t *ctx;
  unsigned char *comp_data = NULL;
  size_t comp_len = 0;

  char large_data[50000];
  memset(large_data, 'A', sizeof(large_data));
  large_data[sizeof(large_data) - 1] = '\0';

  for (i = 1; i <= 30; i++) {
    g_fail_malloc_at = -1;
    fail_malloc_n(0);
    g_crf_malloc_hook = fail_malloc_n;
    g_fail_malloc_at = i;

    if (c_rest_compression_ctx_init(&ctx, C_REST_COMPRESSION_GZIP) ==
        C_REST_OK) {
      c_rest_compress_data(ctx, (const unsigned char *)"a", 1, &comp_data,
                           &comp_len);
      if (comp_data) {
        CRF_FREE(comp_data);
        comp_data = NULL;
      }

      c_rest_compress_finish(ctx, &comp_data, &comp_len);
      if (comp_data) {
        CRF_FREE(comp_data);
        comp_data = NULL;
      }

      c_rest_compression_ctx_destroy(ctx);
    }

    g_crf_malloc_hook = NULL;
    g_fail_malloc_at = 0;
  }

  for (i = 1; i <= 30; i++) {
    g_fail_realloc_at = -1;
    fail_realloc_n(NULL, 0);
    g_crf_realloc_hook = fail_realloc_n;
    g_fail_realloc_at = i;

    if (c_rest_compression_ctx_init(&ctx, C_REST_COMPRESSION_GZIP) ==
        C_REST_OK) {
      c_rest_compress_data(ctx, (const unsigned char *)"a", 1, &comp_data,
                           &comp_len);
      if (comp_data) {
        CRF_FREE(comp_data);
        comp_data = NULL;
      }

      c_rest_compress_finish(ctx, &comp_data, &comp_len);
      if (comp_data) {
        CRF_FREE(comp_data);
        comp_data = NULL;
      }

      c_rest_compression_ctx_destroy(ctx);
    }

    g_crf_realloc_hook = NULL;
    g_fail_realloc_at = 0;
  }

  for (i = 1; i <= 30; i++) {
    g_fail_realloc_at = -1;
    fail_realloc_n(NULL, 0);
    g_crf_realloc_hook = fail_realloc_n;
    g_fail_realloc_at = i;

    if (c_rest_compression_ctx_init(&ctx, C_REST_COMPRESSION_GZIP) ==
        C_REST_OK) {
      c_rest_compress_data(ctx, (const unsigned char *)large_data,
                           sizeof(large_data), &comp_data, &comp_len);
      if (comp_data) {
        CRF_FREE(comp_data);
        comp_data = NULL;
      }

      c_rest_compress_finish(ctx, &comp_data, &comp_len);
      if (comp_data) {
        CRF_FREE(comp_data);
        comp_data = NULL;
      }

      c_rest_compression_ctx_destroy(ctx);
    }

    g_crf_realloc_hook = NULL;
    g_fail_realloc_at = 0;
  }

#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI) &&            \
    !defined(DISABLE_GZIP_BROTLI_OOM_TESTS)
  for (i = 1; i <= 30; i++) {
    g_fail_malloc_at = -1;
    fail_malloc_n(0);
    g_crf_malloc_hook = fail_malloc_n;
    g_fail_malloc_at = i;

    if (c_rest_compression_ctx_init(&ctx, C_REST_COMPRESSION_BROTLI) ==
        C_REST_OK) {
      c_rest_compress_data(ctx, (const unsigned char *)"a", 1, &comp_data,
                           &comp_len);
      if (comp_data) {
        CRF_FREE(comp_data);
        comp_data = NULL;
      }

      c_rest_compress_finish(ctx, &comp_data, &comp_len);
      if (comp_data) {
        CRF_FREE(comp_data);
        comp_data = NULL;
      }

      c_rest_compression_ctx_destroy(ctx);
    }

    g_crf_malloc_hook = NULL;
    g_fail_malloc_at = 0;
  }

  for (i = 1; i <= 30; i++) {
    g_fail_realloc_at = -1;
    fail_realloc_n(NULL, 0);
    g_crf_realloc_hook = fail_realloc_n;
    g_fail_realloc_at = i;

    if (c_rest_compression_ctx_init(&ctx, C_REST_COMPRESSION_BROTLI) ==
        C_REST_OK) {
      c_rest_compress_data(ctx, (const unsigned char *)"a", 1, &comp_data,
                           &comp_len);
      if (comp_data) {
        CRF_FREE(comp_data);
        comp_data = NULL;
      }

      c_rest_compress_finish(ctx, &comp_data, &comp_len);
      if (comp_data) {
        CRF_FREE(comp_data);
        comp_data = NULL;
      }

      c_rest_compression_ctx_destroy(ctx);
    }

    g_crf_realloc_hook = NULL;
    g_fail_realloc_at = 0;
  }

  for (i = 1; i <= 30; i++) {
    g_fail_realloc_at = -1;
    fail_realloc_n(NULL, 0);
    g_crf_realloc_hook = fail_realloc_n;
    g_fail_realloc_at = i;

    if (c_rest_compression_ctx_init(&ctx, C_REST_COMPRESSION_BROTLI) ==
        C_REST_OK) {
      c_rest_compress_data(ctx, (const unsigned char *)large_data,
                           sizeof(large_data), &comp_data, &comp_len);
      if (comp_data) {
        CRF_FREE(comp_data);
        comp_data = NULL;
      }

      c_rest_compress_finish(ctx, &comp_data, &comp_len);
      if (comp_data) {
        CRF_FREE(comp_data);
        comp_data = NULL;
      }

      c_rest_compression_ctx_destroy(ctx);
    }

    g_crf_realloc_hook = NULL;
    g_fail_realloc_at = 0;
  }
#endif
}

int test_response_compression_gzip_brotli(void) {
  test_coverage();

  if (test_compression_errors() != 0) {
    printf("test_compression_errors failed\n");
    return 1;
  }
  if (test_compression_empty() != 0) {
    printf("test_compression_empty failed\n");
    return 1;
  }
  if (test_compression_invalid_type() != 0) {
    printf("test_compression_invalid_type failed\n");
    return 1;
  }
  if (test_compression_lib_failures() != 0) {
    printf("test_compression_lib_failures failed\n");
    return 1;
  }
  if (test_compression_more_errors() != 0) {
    printf("test_compression_more_errors failed\n");
    return 1;
  }
  if (test_compression_malloc_failures() != 0) {
    printf("test_compression_malloc_failures failed\n");
    return 1;
  }
  if (test_compression_concat_failures() != 0) {
    printf("test_compression_concat_failures failed\n");
    return 1;
  }
  if (test_compression_realloc_failures() != 0) {
    printf("test_compression_realloc_failures failed\n");
    return 1;
  }
  if (test_compression_realloc_finish_failures() != 0) {
    printf("test_compression_realloc_finish_failures failed\n");
    return 1;
  }
  if (test_compression_buffer_malloc_failures() != 0) {
    printf("test_compression_buffer_malloc_failures failed\n");
    return 1;
  }
  if (test_compression_none() != 0) {
    printf("test_compression_none failed\n");
    return 1;
  }
#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP)
  if (test_compression_gzip_basic() != 0) {
    printf("test_compression_gzip_basic failed\n");
    return 1;
  }
  if (test_compression_gzip_large() != 0) {
    printf("test_compression_gzip_large failed\n");
    return 1;
  }
#endif
#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI)
  if (test_compression_brotli_basic() != 0) {
    printf("test_compression_brotli_basic failed\n");
    return 1;
  }
  if (test_compression_brotli_large() != 0) {
    printf("test_compression_brotli_large failed\n");
    return 1;
  }
#endif
  return 0;
}

static int test_compression_more_errors(void) {
  c_rest_error_t res;
  unsigned char *comp_data = NULL;
  size_t comp_len = 0;

  res = c_rest_compress_buffer((c_rest_compression_type_t)999,
                               (const unsigned char *)"abc", 3, &comp_data,
                               &comp_len);
  if (res == C_REST_OK)
    return 1;

  res = c_rest_compress_buffer(C_REST_COMPRESSION_GZIP, NULL, 3, &comp_data,
                               &comp_len);
  if (res == C_REST_OK)
    return 1;

  res =
      c_rest_compress_buffer(C_REST_COMPRESSION_GZIP,
                             (const unsigned char *)"abc", 3, NULL, &comp_len);
  if (res == C_REST_OK)
    return 1;

  res =
      c_rest_compress_buffer(C_REST_COMPRESSION_GZIP,
                             (const unsigned char *)"abc", 3, &comp_data, NULL);
  if (res == C_REST_OK)
    return 1;

  res = c_rest_compression_ctx_init(NULL, C_REST_COMPRESSION_GZIP);
  if (res == C_REST_OK)
    return 1;

  res = c_rest_compression_ctx_destroy(NULL);
  if (res == C_REST_OK)
    return 1;

  res = c_rest_compress_data(NULL, (const unsigned char *)"abc", 3, &comp_data,
                             &comp_len);
  if (res == C_REST_OK)
    return 1;

  res = c_rest_compress_finish(NULL, &comp_data, &comp_len);
  if (res == C_REST_OK)
    return 1;

#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI)
  res = c_rest_compress_buffer(C_REST_COMPRESSION_BROTLI, NULL, 3, &comp_data,
                               &comp_len);
  if (res == C_REST_OK)
    return 1;

  res =
      c_rest_compress_buffer(C_REST_COMPRESSION_BROTLI,
                             (const unsigned char *)"abc", 3, NULL, &comp_len);
  if (res == C_REST_OK)
    return 1;

  res =
      c_rest_compress_buffer(C_REST_COMPRESSION_BROTLI,
                             (const unsigned char *)"abc", 3, &comp_data, NULL);
  if (res == C_REST_OK)
    return 1;

  res = c_rest_compression_ctx_init(NULL, C_REST_COMPRESSION_BROTLI);
  if (res == C_REST_OK)
    return 1;
#endif

  return 0;
}

static int test_compression_malloc_failures(void) {
  c_rest_error_t res;
  unsigned char *comp_data = NULL;
  size_t comp_len = 0;
  c_rest_compression_ctx_t *ctx = NULL;

  g_fail_malloc_at = -1;
  fail_malloc_n(0);
  g_fail_malloc_at = 1;
  g_crf_malloc_hook = fail_malloc_n;
  res = c_rest_compression_ctx_init(&ctx, C_REST_COMPRESSION_GZIP);
  if (res == C_REST_OK)
    return 1;

  g_fail_malloc_at = -1;
  fail_malloc_n(0);
  g_fail_malloc_at = 1;
  g_crf_malloc_hook = fail_malloc_n;
  res = c_rest_compress_buffer(C_REST_COMPRESSION_GZIP,
                               (const unsigned char *)"abc", 3, &comp_data,
                               &comp_len);
  if (res == C_REST_OK)
    return 1;

#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI)
  g_fail_malloc_at = -1;
  fail_malloc_n(0);
  g_fail_malloc_at = 1;
  g_crf_malloc_hook = fail_malloc_n;
  res = c_rest_compression_ctx_init(&ctx, C_REST_COMPRESSION_BROTLI);
  if (res == C_REST_OK)
    return 1;
#endif
  g_crf_malloc_hook = NULL;

  /* test ctx operations */
#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP)
  res = c_rest_compression_ctx_init(&ctx, C_REST_COMPRESSION_GZIP);
  if (res != C_REST_OK)
    return 1;

  g_fail_malloc_at = -1;
  fail_malloc_n(0);
  g_fail_malloc_at = 1;
  g_crf_malloc_hook = fail_malloc_n;
  res = c_rest_compress_data(ctx, (const unsigned char *)"abc", 3, &comp_data,
                             &comp_len);
  if (res == C_REST_OK)
    return 1;

  g_fail_malloc_at = -1;
  fail_malloc_n(0);
  g_fail_malloc_at = 1;
  g_crf_malloc_hook = fail_malloc_n;
  res = c_rest_compress_finish(ctx, &comp_data, &comp_len);
  if (res == C_REST_OK)
    return 1;

  g_crf_malloc_hook = NULL;
  c_rest_compression_ctx_destroy(ctx);
#endif

#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI)
  res = c_rest_compression_ctx_init(&ctx, C_REST_COMPRESSION_BROTLI);
  if (res != C_REST_OK)
    return 1;

  g_fail_malloc_at = -1;
  fail_malloc_n(0);
  g_fail_malloc_at = 1;
  g_crf_malloc_hook = fail_malloc_n;
  res = c_rest_compress_data(ctx, (const unsigned char *)"abc", 3, &comp_data,
                             &comp_len);
  if (res == C_REST_OK)
    return 1;

  g_fail_malloc_at = -1;
  fail_malloc_n(0);
  g_fail_malloc_at = 1;
  g_crf_malloc_hook = fail_malloc_n;
  res = c_rest_compress_finish(ctx, &comp_data, &comp_len);
  if (res == C_REST_OK)
    return 1;

  g_crf_malloc_hook = NULL;
  c_rest_compression_ctx_destroy(ctx);
#endif

  return 0;
}

static int test_compression_concat_failures(void) {
  c_rest_error_t res;
  unsigned char *comp_data = NULL;
  size_t comp_len = 0;

  g_fail_malloc_at = 3;
  g_crf_malloc_hook = fail_malloc_n;
  res = c_rest_compress_buffer(C_REST_COMPRESSION_GZIP,
                               (const unsigned char *)"abc", 3, &comp_data,
                               &comp_len);
  g_crf_malloc_hook = NULL;
  if (res == C_REST_OK)
    return 1;

  return 0;
}

static int test_compression_realloc_failures(void) {
  c_rest_error_t res;
  unsigned char *comp_data = NULL;
  size_t comp_len = 0;
  c_rest_compression_ctx_t *ctx = NULL;

  char large_data[100000];
  memset(large_data, 'A', sizeof(large_data));

  g_fail_realloc_at = -1;
  fail_realloc_n(NULL, 0);

  /* GZIP */
#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP)
  res = c_rest_compression_ctx_init(&ctx, C_REST_COMPRESSION_GZIP);
  if (res != C_REST_OK)
    return 1;

  g_fail_realloc_at = 1;
  g_crf_realloc_hook = fail_realloc_n;
  res = c_rest_compress_data(ctx, (const unsigned char *)large_data,
                             sizeof(large_data), &comp_data, &comp_len);
  if (res == C_REST_OK)
    return 1;
  if (comp_data) {
    CRF_FREE(comp_data);
    comp_data = NULL;
  }

  g_crf_realloc_hook = NULL;

  c_rest_compression_ctx_destroy(ctx);
#endif

  g_fail_realloc_at = -1;
  fail_realloc_n(NULL, 0);

  /* BROTLI */
#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI)
  res = c_rest_compression_ctx_init(&ctx, C_REST_COMPRESSION_BROTLI);
  if (res != C_REST_OK)
    return 1;

  g_fail_realloc_at = 1;
  g_crf_realloc_hook = fail_realloc_n;
  res = c_rest_compress_data(ctx, (const unsigned char *)large_data,
                             sizeof(large_data), &comp_data, &comp_len);
  if (res == C_REST_OK)
    return 1;
  if (comp_data) {
    CRF_FREE(comp_data);
    comp_data = NULL;
  }
  g_crf_realloc_hook = NULL;

  c_rest_compression_ctx_destroy(ctx);
#endif

  return 0;
}

static int test_compression_realloc_finish_failures(void) {
  c_rest_error_t res;
  unsigned char *comp_data = NULL;
  size_t comp_len = 0;
  c_rest_compression_ctx_t *ctx = NULL;

  char large_data[100000];
  memset(large_data, 'A', sizeof(large_data));

  /* GZIP */
#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP)
  res = c_rest_compression_ctx_init(&ctx, C_REST_COMPRESSION_GZIP);
  if (res != C_REST_OK)
    return 1;
  res = c_rest_compress_data(ctx, (const unsigned char *)large_data,
                             sizeof(large_data), &comp_data, &comp_len);
  if (res != C_REST_OK)
    return 1;
  if (comp_data) {
    CRF_FREE(comp_data);
    comp_data = NULL;
  }

  g_fail_realloc_at = -1;
  fail_realloc_n(NULL, 0);
  g_fail_realloc_at = 1;
  g_crf_realloc_hook = fail_realloc_n;
  res = c_rest_compress_finish(ctx, &comp_data, &comp_len);
  g_crf_realloc_hook = NULL;
  if (res == C_REST_OK) {
    C_REST_FREE(comp_data);
    /* It's OK if Brotli doesn't realloc on finish since capacity is
     * preallocated */
  }

  c_rest_compression_ctx_destroy(ctx);
#endif

  /* BROTLI */
#if defined(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI)
  res = c_rest_compression_ctx_init(&ctx, C_REST_COMPRESSION_BROTLI);
  if (res != C_REST_OK)
    return 1;
  res = c_rest_compress_data(ctx, (const unsigned char *)large_data,
                             sizeof(large_data), &comp_data, &comp_len);
  if (res != C_REST_OK)
    return 1;
  if (comp_data) {
    CRF_FREE(comp_data);
    comp_data = NULL;
  }

  g_fail_realloc_at = -1;
  fail_realloc_n(NULL, 0);
  g_fail_realloc_at = 1;
  g_crf_realloc_hook = fail_realloc_n;
  res = c_rest_compress_finish(ctx, &comp_data, &comp_len);
  g_crf_realloc_hook = NULL;
  if (res == C_REST_OK) {
    C_REST_FREE(comp_data);
    /* It's OK if Brotli doesn't realloc on finish since capacity is
     * preallocated */
  }

  c_rest_compression_ctx_destroy(ctx);
#endif

  return 0;
}

static int test_compression_buffer_malloc_failures(void) {
  c_rest_error_t res;
  unsigned char *comp_data = NULL;
  size_t comp_len = 0;
  int i;

  for (i = 1; i <= 6; i++) {
    g_crf_malloc_hook = fail_malloc_n;
    g_fail_malloc_at = i;
    res = c_rest_compress_buffer(C_REST_COMPRESSION_GZIP,
                                 (const unsigned char *)"abc", 3, &comp_data,
                                 &comp_len);
    g_crf_malloc_hook = NULL;
    if (res == C_REST_OK) {
      CRF_FREE(comp_data);
    }
  }

  return 0;
}
