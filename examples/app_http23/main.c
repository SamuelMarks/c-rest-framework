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
  static const unsigned char h3_req_frame[4] = {0x01U, 0x02U, 0x82U, 0x84U};
  size_t consumed = 0;
  int ready = 0;
  struct c_rest_request *req = NULL;
  struct c_rest_response resp;
  char resp_body[] = "Hello from HTTP/3!";
  char *out_buf = NULL;
  size_t out_len = 0;

  printf("Starting HTTP/2 & HTTP/3 Example...\n");

  res = c_rest_http23_ctx_init(C_REST_PROTOCOL_HTTP3, &ctx);
  if (res != C_REST_OK) {
    printf("Failed to init HTTP3 ctx\n");
    return 1;
  }

  printf("Processing HTTP/3 HEADERS frame...\n");
  res = c_rest_http23_process(ctx, (const char *)h3_req_frame,
                              sizeof(h3_req_frame), &consumed);
  if (res != C_REST_OK) {
    if (c_rest_http23_ctx_destroy(ctx) != C_REST_OK)
      return 1;
    return 1;
  }

  res = c_rest_http23_is_request_ready(ctx, &ready);
  if (res != C_REST_OK || !ready) {
    if (c_rest_http23_ctx_destroy(ctx) != C_REST_OK)
      return 1;
    return 1;
  }

  res = c_rest_http23_get_request(ctx, &req);
  if (res != C_REST_OK || !req) {
    if (c_rest_http23_ctx_destroy(ctx) != C_REST_OK)
      return 1;
    return 1;
  }

  printf("Request Received: %s %s\n", req->method, req->path);

  memset(&resp, 0, sizeof(resp));
  resp.status_code = 200;
  resp.body = resp_body;
  resp.body_len = strlen(resp_body);

  res = c_rest_http23_format_response(ctx, &resp, &out_buf, &out_len);
  if (res != C_REST_OK) {
    if (c_rest_http23_ctx_destroy(ctx) != C_REST_OK)
      return 1;
    return 1;
  }

  printf("Generated HTTP/3 Response Frames (len=%u)\n", (unsigned int)out_len);
  C_REST_FREE(out_buf);

  res = c_rest_http23_ctx_destroy(ctx);
  if (res != C_REST_OK)
    return 1;
  printf("HTTP/2 & HTTP/3 Example Finished.\n");
  return 0;
}
