/* clang-format off */
#include "c_rest_modality.h"
#include "c_rest_multipart.h"
#include "c_rest_request.h"
#include "c_rest_response.h"
#include "c_rest_router.h"
#include <stdio.h>
/* clang-format on */

static int on_part_begin(c_rest_multipart_parser *parser) {
  (void)parser;
  printf("[Multipart] Part begin\n");
  return 0;
}

static int on_header_field(c_rest_multipart_parser *parser, const char *at,
                           size_t length) {
  (void)parser;
  printf("[Multipart] Header field: %.*s\n", (int)length, at);
  return 0;
}

static int on_header_value(c_rest_multipart_parser *parser, const char *at,
                           size_t length) {
  (void)parser;
  printf("[Multipart] Header value: %.*s\n", (int)length, at);
  return 0;
}

static int on_headers_complete(c_rest_multipart_parser *parser) {
  (void)parser;
  printf("[Multipart] Headers complete\n");
  return 0;
}

static int on_part_data(c_rest_multipart_parser *parser, const char *at,
                        size_t length) {
  (void)parser;
  printf("[Multipart] Part data: %.*s\n", (int)length, at);
  return 0;
}

static int on_part_end(c_rest_multipart_parser *parser) {
  (void)parser;
  printf("[Multipart] Part end\n");
  return 0;
}

static int on_body_end(c_rest_multipart_parser *parser) {
  (void)parser;
  printf("[Multipart] Body end\n");
  return 0;
}

static int upload_handler(struct c_rest_request *req,
                          struct c_rest_response *res, void *ctx) {
  c_rest_multipart_parser *parser = NULL;
  struct c_rest_multipart_callbacks callbacks = {0};
  size_t parsed = 0;
  int parse_res = 0;
  (void)ctx;

  callbacks.on_part_begin = on_part_begin;
  callbacks.on_header_field = on_header_field;
  callbacks.on_header_value = on_header_value;
  callbacks.on_headers_complete = on_headers_complete;
  callbacks.on_part_data = on_part_data;
  callbacks.on_part_end = on_part_end;
  callbacks.on_body_end = on_body_end;

  /* In a real app, you would parse the boundary from the Content-Type header.
   */
  if (c_rest_multipart_parser_init(&parser, "boundary", &callbacks, NULL) ==
      0) {
    if (req->body && req->body_len > 0) {
      parse_res = c_rest_multipart_parser_execute(parser, req->body,
                                                  req->body_len, &parsed);
      if (parse_res != 0) {
        printf("[Multipart] Failed to parse multipart data.\n");
      }
    }
    c_rest_multipart_parser_destroy(parser);
  }

  res->status_code = 200;
  res->body = "{\"status\": \"ok\"}";
  res->body_len = 15;
  return 0;
}

int main(void) {
  struct c_rest_context *ctx = NULL;
  c_rest_router *router = NULL;

  printf("Initializing Full Multipart Form Streaming App...\n");

  if (c_rest_init(C_REST_MODALITY_SYNC, &ctx) != 0) {
    return 1;
  }

  c_rest_router_init(&router);
  c_rest_router_add(router, "POST", "/upload", upload_handler, NULL);
  c_rest_set_router(ctx, router);

  /* Uncomment to run server */
  /* c_rest_run(ctx); */

  c_rest_router_destroy(router);
  c_rest_destroy(ctx);
  printf("Done.\n");
  return 0;
}