/* clang-format off */
#include "c_rest_http23.h"
#include <stdio.h>
#include <string.h>
/* clang-format on */

int main(void) {
  c_rest_http23_ctx_t *ctx = NULL;
  int res;
  size_t consumed = 0;
  int is_ready = 0;
  struct c_rest_request *req = NULL;
  struct c_rest_response res_obj;
  char *out_buf = NULL;
  size_t out_len = 0;

  printf("Starting HTTP/2 & HTTP/3 Example...\n");
  res = c_rest_http23_ctx_init(C_REST_PROTOCOL_HTTP3, &ctx);
  if (res != C_REST_HTTP23_OK || !ctx) {
    printf("Failed to init HTTP3 ctx\n");
    return 1;
  }

  printf("Processing simulated HTTP/3 frame...\n");
  res = c_rest_http23_process(ctx, "mock_frame_data", 15, &consumed);
  if (res != C_REST_HTTP23_OK) {
    printf("Failed to process data\n");
    return 1;
  }

  res = c_rest_http23_is_request_ready(ctx, &is_ready);
  if (is_ready) {
    c_rest_http23_get_request(ctx, &req);
    printf("Request Received: %s %s\n", req->method, req->path);

    memset(&res_obj, 0, sizeof(res_obj));
    res_obj.body = "Hello from HTTP/3!";
    res_obj.body_len = strlen(res_obj.body);

    c_rest_http23_format_response(ctx, &res_obj, &out_buf, &out_len);
    printf("Generated Response Buffer (len=%u):\n%s\n", (unsigned int)out_len,
           out_buf);

    /* Note: In a real system, use C_REST_FREE instead of free if using custom
     * allocator */
    /* Since this is a simple example we rely on the implementation's memory
     * handling. */
  }

  c_rest_http23_ctx_destroy(ctx);
  printf("HTTP/2 & HTTP/3 Example Finished.\n");
  return 0;
}
