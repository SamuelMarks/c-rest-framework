/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "test_protos.h"
#include "c_rest_http23.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
/* clang-format on */

static int g_malloc_fail_after = -1;
static void *fail_malloc_n(size_t size) {
  if (g_malloc_fail_after == 0) {
    return NULL;
  }
  if (g_malloc_fail_after > 0) {
    g_malloc_fail_after--;
  }
  return malloc(size);
}
static void *fail_malloc(size_t size) {
  (void)size;
  return NULL;
}

int test_http23(void) {
  c_rest_http23_ctx_t *ctx = NULL;
  size_t consumed = 0;
  int is_ready = 0;
  struct c_rest_request *req = NULL;
  struct c_rest_response res_obj;
  char *out_buf = NULL;
  size_t out_len = 0;

  int fails = 0;

  printf("Testing HTTP2/3 Init...\n");
  fails += c_rest_http23_ctx_init(C_REST_PROTOCOL_HTTP2, &ctx) != C_REST_OK;
  fails += ctx == NULL;

  printf("Testing HTTP2/3 Process...\n");
  if (ctx) {
    fails += c_rest_http23_process(ctx, "mock_data", 9, &consumed) != C_REST_OK;
    fails += consumed != 9;

    fails += c_rest_http23_is_request_ready(ctx, &is_ready) != C_REST_OK;
    fails += !is_ready;

    printf("Testing HTTP2/3 Get Request...\n");
    fails += c_rest_http23_get_request(ctx, &req) != C_REST_OK;
    fails += req == NULL;

    if (req) {
      fails += strcmp(req->method, "GET") != 0;
      fails += strcmp(req->path, "/http23_test") != 0;
    }

    printf("Testing HTTP2/3 Format Response...\n");
    memset(&res_obj, 0, sizeof(res_obj));
    res_obj.body = "hello";
    res_obj.body_len = 5;

    fails += c_rest_http23_format_response(ctx, &res_obj, &out_buf, &out_len) !=
             C_REST_OK;
    fails += out_buf == NULL;

    if (out_buf) {
      fails += strstr(out_buf, "HTTP/2 FRAME: body_len=5") == NULL;
      fails += strstr(out_buf, "hello") == NULL;
      C_REST_FREE((void *)out_buf);
    }

    out_buf = NULL;
    res_obj.body = NULL;
    res_obj.body_len = 0;
    fails += c_rest_http23_format_response(ctx, &res_obj, &out_buf, &out_len) !=
             C_REST_OK;
    fails += out_buf == NULL;

    if (out_buf) {
      fails += strstr(out_buf, "HTTP/2 FRAME: body_len=0") == NULL;
      C_REST_FREE((void *)out_buf);
    }

    fails += c_rest_http23_ctx_destroy(ctx) != C_REST_OK;
  }

  if (fails > 0)
    return 1;

  /* Error handling and coverage tests */
  {
    c_rest_http23_ctx_t *err_ctx = NULL;
    int ready = 0;
    size_t cons = 0;
    struct c_rest_request *err_req = NULL;

    /* init errors */
    fails += c_rest_http23_ctx_init(C_REST_PROTOCOL_HTTP2, NULL) == C_REST_OK;
    fails +=
        c_rest_http23_ctx_init((c_rest_protocol_t)-1, &err_ctx) == C_REST_OK;
    fails +=
        c_rest_http23_ctx_init(C_REST_PROTOCOL_HTTP3, &err_ctx) != C_REST_OK;
    c_rest_http23_ctx_destroy(err_ctx);

    /* process errors */
    fails += c_rest_http23_process(NULL, "data", 4, &cons) == C_REST_OK;
    fails += c_rest_http23_process(ctx, NULL, 4, &cons) == C_REST_OK;
    fails += c_rest_http23_process(ctx, "data", 4, NULL) == C_REST_OK;
    fails += (c_rest_http23_process(ctx, "data", 0, &cons) != C_REST_OK) |
             (cons != 0);

    /* is_ready errors */
    fails += c_rest_http23_is_request_ready(NULL, &ready) == C_REST_OK;
    fails += c_rest_http23_is_request_ready(ctx, NULL) == C_REST_OK;

    /* get_request errors */
    fails += c_rest_http23_get_request(NULL, &err_req) == C_REST_OK;
    fails += c_rest_http23_get_request(ctx, NULL) == C_REST_OK;

    c_rest_http23_ctx_init(C_REST_PROTOCOL_HTTP2, &err_ctx);
    fails += c_rest_http23_get_request(err_ctx, &err_req) == C_REST_OK;
    c_rest_http23_ctx_destroy(err_ctx);

    /* format_response errors */
    fails += c_rest_http23_format_response(NULL, &res_obj, &out_buf,
                                           &out_len) == C_REST_OK;
    fails += c_rest_http23_format_response((c_rest_http23_ctx_t *)1, NULL,
                                           &out_buf, &out_len) == C_REST_OK;
    fails += c_rest_http23_format_response((c_rest_http23_ctx_t *)1, &res_obj,
                                           NULL, &out_len) == C_REST_OK;
    fails += c_rest_http23_format_response((c_rest_http23_ctx_t *)1, &res_obj,
                                           &out_buf, NULL) == C_REST_OK;

    /* destroy errors */
    fails += c_rest_http23_ctx_destroy(NULL) == C_REST_OK;

    if (fails > 0)
      return 1;
  }

  /* Malloc failure tests */
  {
    c_rest_http23_ctx_t *err_ctx = NULL;

    g_crf_malloc_hook = fail_malloc;
    if (c_rest_http23_ctx_init(C_REST_PROTOCOL_HTTP2, &err_ctx) == C_REST_OK)
      return 1;
    g_crf_malloc_hook = NULL;

    c_rest_http23_ctx_init(C_REST_PROTOCOL_HTTP2, &err_ctx);
    g_crf_malloc_hook = fail_malloc;
    if (c_rest_http23_format_response(err_ctx, &res_obj, &out_buf, &out_len) ==
        C_REST_OK)
      return 1;
    g_crf_malloc_hook = NULL;
    c_rest_http23_ctx_destroy(err_ctx);
  }

  /* Malloc fail inside init ctx->request */
  {
    extern int g_malloc_fail_after;
    c_rest_http23_ctx_t *err_ctx = NULL;

    g_crf_malloc_hook = fail_malloc_n;
    g_malloc_fail_after = 1;
    if (c_rest_http23_ctx_init(C_REST_PROTOCOL_HTTP2, &err_ctx) == C_REST_OK)
      return 1;
    g_crf_malloc_hook = NULL;
    g_malloc_fail_after = -1;
  }

  printf("HTTP2/3 Tests Passed!\n");
  return 0;
}
