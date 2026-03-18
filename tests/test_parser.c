/* clang-format off */
#include "c_rest_parser.h"

#include <stdio.h>
#include <string.h>
/* clang-format on */

static int method_called = 0;
static int url_called = 0;
static int complete_called = 0;

static void on_method(c_rest_parser_context *ctx, const char *method,
                      size_t len) {
  (void)ctx;
  (void)method;
  (void)len;
  method_called = 1;
}

static void on_url(c_rest_parser_context *ctx, const char *url, size_t len) {
  (void)ctx;
  (void)url;
  (void)len;
  url_called = 1;
}

static void on_complete(c_rest_parser_context *ctx) {
  (void)ctx;
  complete_called = 1;
}

static int header_called = 0;
static int body_called = 0;

static void on_header(c_rest_parser_context *ctx, const char *key,
                      size_t key_len, const char *val, size_t val_len) {
  (void)ctx;
  (void)key;
  (void)key_len;
  (void)val;
  (void)val_len;
  header_called++;
}

static void on_body(c_rest_parser_context *ctx, const char *data, size_t len) {
  (void)ctx;
  (void)data;
  (void)len;
  body_called++;
}

static void on_error(c_rest_parser_context *ctx, const char *msg) {
  (void)ctx;
  (void)msg;
}

static int test_parser_chunked(void) {
  c_rest_parser_context ctx;
  struct c_rest_parser_callbacks callbacks;
  const struct c_rest_parser_vtable *vtable = NULL;
  size_t parsed;
  int keep_alive;
  const char *req = "POST / HTTP/1.1\r\nTransfer-Encoding: "
                    "chunked\r\n\r\n4\r\nWiki\r\n5\r\npedia\r\n0\r\n\r\n";
  callbacks.on_method = on_method;
  callbacks.on_url = on_url;
  callbacks.on_header = on_header;
  callbacks.on_body = on_body;
  callbacks.on_complete = on_complete;
  callbacks.on_error = on_error;
  c_rest_parser_get_basic_vtable(&vtable);
  c_rest_parser_init(&ctx, vtable, &callbacks, NULL);
  c_rest_parser_execute(&ctx, req, strlen(req), &parsed);
  c_rest_parser_should_keep_alive(&ctx, &keep_alive);
  c_rest_parser_is_complete(&ctx);
  c_rest_parser_destroy(&ctx);
  return 0;
}

static int test_parser_content_length(void) {
  c_rest_parser_context ctx;
  struct c_rest_parser_callbacks callbacks;
  const struct c_rest_parser_vtable *vtable = NULL;
  size_t parsed;
  const char *req = "POST / HTTP/1.1\r\nContent-Length: 10\r\n\r\n1234567890";
  callbacks.on_method = on_method;
  callbacks.on_url = on_url;
  callbacks.on_header = on_header;
  callbacks.on_body = on_body;
  callbacks.on_complete = on_complete;
  callbacks.on_error = on_error;
  c_rest_parser_get_basic_vtable(&vtable);
  c_rest_parser_init(&ctx, vtable, &callbacks, NULL);
  c_rest_parser_execute(&ctx, req, strlen(req), &parsed);
  c_rest_parser_destroy(&ctx);
  return 0;
}

static int test_parser_errors(void) {
  c_rest_parser_context ctx;
  struct c_rest_parser_callbacks callbacks;
  const struct c_rest_parser_vtable *vtable = NULL;
  size_t parsed;
  const char *req = "GET / HTTP/1.1\r\nHeader-No-Colon\r\n\r\n";
  callbacks.on_method = on_method;
  callbacks.on_url = on_url;
  callbacks.on_header = on_header;
  callbacks.on_body = on_body;
  callbacks.on_complete = on_complete;
  callbacks.on_error = on_error;
  c_rest_parser_get_basic_vtable(&vtable);
  c_rest_parser_init(&ctx, vtable, &callbacks, NULL);
  c_rest_parser_execute(&ctx, req, strlen(req), &parsed);
  c_rest_parser_destroy(&ctx);

  /* NULL checks */
  c_rest_parser_init(NULL, vtable, &callbacks, NULL);
  c_rest_parser_init(&ctx, NULL, &callbacks, NULL);
  c_rest_parser_execute(NULL, req, strlen(req), &parsed);
  c_rest_parser_destroy(NULL);
  return 0;
}

int test_parser(void) {
  c_rest_parser_context ctx;
  struct c_rest_parser_callbacks callbacks;
  const char *valid_req = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
  const char *malformed_req = "MALFORMED REQUEST DATA";
  size_t parsed;
  int res;

  const struct c_rest_parser_vtable *vtable = NULL;

  printf("Running parser tests...\n");

  callbacks.on_method = on_method;
  callbacks.on_url = on_url;
  callbacks.on_header = NULL;
  callbacks.on_body = NULL;
  callbacks.on_complete = on_complete;
  callbacks.on_error = NULL;

  res = c_rest_parser_get_basic_vtable(&vtable);
  if (res != 0 || !vtable) {
    printf("Failed to get vtable\n");
    return 1;
  }

  res = c_rest_parser_init(&ctx, vtable, &callbacks, NULL);
  if (res != 0) {
    printf("Failed to init parser\n");
    return 1;
  }

  res = c_rest_parser_execute(&ctx, valid_req, strlen(valid_req), &parsed);
  if (res != 0 || parsed != strlen(valid_req)) {
    printf("Failed to parse valid request: %u != %u\n", (unsigned int)parsed,
           (unsigned int)strlen(valid_req));
    return 1;
  }

  if (!method_called || !url_called || !complete_called) {
    printf("Callbacks were not invoked appropriately\n");
    return 1;
  }

  /* Test malformed request */
  res = c_rest_parser_execute(&ctx, malformed_req, strlen(malformed_req),
                              &parsed);
  if (res != 0 || parsed != 0) {
    printf("Malformed request should have returned 0 parsed bytes\n");
    return 1;
  }

  c_rest_parser_destroy(&ctx);
  test_parser_chunked();
  test_parser_content_length();
  test_parser_errors();

  return 0;
}
