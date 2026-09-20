/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_http23.h"
#include <signal.h>
#include <stdio.h>
#include <string.h>
/* clang-format on */

int main(void) {
  c_rest_http23_ctx_t *ctx = NULL;
  c_rest_error_t res;
  size_t consumed = 0;
  struct c_rest_request *req = NULL;
  struct c_rest_response res_obj;
  char *out_buf = NULL;
  size_t out_len = 0;

  printf("Starting HTTP/2 & HTTP/3 Example...\n");
  res = c_rest_http23_ctx_init(C_REST_PROTOCOL_HTTP3, &ctx);
  if (res != C_REST_OK) {
    printf("Failed to init HTTP3 ctx\n");
    return 1;
  }

  printf("Processing simulated HTTP/3 frame...\n");
  res = c_rest_http23_process(ctx, "mock_frame_data", 15, &consumed);
  if (res != C_REST_OK) {
    c_rest_http23_ctx_destroy(ctx);
    return 1;
  }
  res = c_rest_http23_get_request(ctx, &req);
  if (res != C_REST_OK) {
    c_rest_http23_ctx_destroy(ctx);
    return 1;
  }
  printf("Request Received: %s %s\n", req->method, req->path);

  memset(&res_obj, 0, sizeof(res_obj));
  res_obj.body = "Hello from HTTP/3!";
  res_obj.body_len = strlen(res_obj.body);

  res = c_rest_http23_format_response(ctx, &res_obj, &out_buf, &out_len);
  if (res != C_REST_OK) {
    c_rest_http23_ctx_destroy(ctx);
    return 1;
  }
  printf("Generated Response Buffer (len=%u):\n%s\n", (unsigned int)out_len,
         out_buf);

  C_REST_FREE(out_buf);

  res = c_rest_http23_ctx_destroy(ctx);
  if (res != C_REST_OK)
    return 1;
  printf("HTTP/2 & HTTP/3 Example Finished.\n");
  return 0;
}
