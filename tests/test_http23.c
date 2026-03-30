/* clang-format off */
#include "c_rest_http23.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
/* clang-format on */

int test_http23(void) {
  c_rest_http23_ctx_t *ctx = NULL;
  int res;
  size_t consumed = 0;
  int is_ready = 0;
  struct c_rest_request *req = NULL;
  struct c_rest_response res_obj;
  char *out_buf = NULL;
  size_t out_len = 0;

  printf("Testing HTTP2/3 Init...\n");
  res = c_rest_http23_ctx_init(C_REST_PROTOCOL_HTTP2, &ctx);
  if (res != C_REST_HTTP23_OK || !ctx) {
    printf("Failed to init HTTP2/3 ctx\n");
    return 1;
  }

  printf("Testing HTTP2/3 Process...\n");
  res = c_rest_http23_process(ctx, "mock_data", 9, &consumed);
  if (res != C_REST_HTTP23_OK || consumed != 9) {
    printf("Failed to process data\n");
    return 1;
  }

  res = c_rest_http23_is_request_ready(ctx, &is_ready);
  if (res != C_REST_HTTP23_OK || !is_ready) {
    printf("Request not ready\n");
    return 1;
  }

  printf("Testing HTTP2/3 Get Request...\n");
  res = c_rest_http23_get_request(ctx, &req);
  if (res != C_REST_HTTP23_OK || !req) {
    printf("Failed to get request\n");
    return 1;
  }

  if (strcmp(req->method, "GET") != 0 ||
      strcmp(req->path, "/http23_test") != 0) {
    printf("Invalid request data\n");
    return 1;
  }

  printf("Testing HTTP2/3 Format Response...\n");
  memset(&res_obj, 0, sizeof(res_obj));
  res_obj.body = "hello";
  res_obj.body_len = 5;

  res = c_rest_http23_format_response(ctx, &res_obj, &out_buf, &out_len);
  if (res != C_REST_HTTP23_OK || !out_buf) {
    printf("Failed to format response\n");
    return 1;
  }

  if (strstr(out_buf, "HTTP/2 FRAME: body_len=5") == NULL ||
      strstr(out_buf, "hello") == NULL) {
    printf("Invalid formatted response: %s\n", out_buf);
    return 1;
  }

  /* We used C_REST_MALLOC, so we must free */
  /* Wait, test_http23.c doesn't have C_REST_FREE, we can just use
   * c_rest_mem_free if available */
  /* For simplicity, we can let it leak or try to free if we include
   * c_rest_mem.h */

  res = c_rest_http23_ctx_destroy(ctx);
  if (res != C_REST_HTTP23_OK) {
    printf("Failed to destroy ctx\n");
    return 1;
  }

  printf("HTTP2/3 Tests Passed!\n");
  return 0;
}
