/* clang-format off */
#include "c_rest_client.h"
#include "c_rest_tls.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* clang-format on */

static int async_called = 0;

static void async_callback(struct c_rest_client_response *res, void *data) {
  (void)res;
  (void)data;
  async_called = 1;
}

int test_client(void) {
  c_rest_client_context *client = NULL;
  struct c_rest_client_response *sync_res = NULL;
  struct c_rest_client_header headers[1];
  int res;
  char *encoded = NULL;
  char *decoded = NULL;
  struct c_rest_client_form_field fields[2];
  char *body = NULL;
  size_t body_len = 0;

  printf("Running client tests...\n");

  res = c_rest_tls_init();
  if (res != 0) {
    printf("Failed to init TLS\n");
    return 1;
  }

  res = c_rest_client_init(&client);
  if (res != 0 || !client) {
    printf("Failed to init client\n");
    return 1;
  }

  headers[0].key = "X-Test-Header";
  headers[0].value = "TestValue";

  res = c_rest_client_request_sync(client, "http://localhost", "GET", headers,
                                   1, NULL, 0, &sync_res);
  (void)res;
  if (sync_res) {
    c_rest_client_response_free(sync_res);
  }

  res = c_rest_client_request_async(client, "http://localhost", "POST", NULL, 0,
                                    "test", 4, async_callback, NULL);
  (void)res;

  if (!async_called) {
    printf("Async callback was not invoked\n");
    return 1;
  }

  res = c_rest_proxy_request("http://localhost/proxy", NULL, NULL);
  (void)res;

  c_rest_client_destroy(client);

  /* Test URL encoding/decoding */
  if (c_rest_client_url_encode("test + & = ?", &encoded) != 0) {
    printf("Failed to encode\n");
    return 1;
  }
  if (strcmp(encoded, "test+%2B+%26+%3D+%3F") != 0) {
    printf("Encoded string mismatch: %s\n", encoded);
    return 1;
  }

  if (c_rest_client_url_decode(encoded, &decoded) != 0) {
    printf("Failed to decode\n");
    return 1;
  }
  if (strcmp(decoded, "test + & = ?") != 0) {
    printf("Decoded string mismatch: %s\n", decoded);
    return 1;
  }

  free(encoded);
  free(decoded);

  /* Test Form URL encoded builder */
  fields[0].key = "grant_type";
  fields[0].value = "password";
  fields[1].key = "username";
  fields[1].value = "test user";

  if (c_rest_client_build_form_urlencoded(fields, 2, &body, &body_len) != 0) {
    printf("Failed to build form urlencoded\n");
    return 1;
  }

  if (strcmp(body, "grant_type=password&username=test+user") != 0) {
    printf("Form urlencoded mismatch: %s\n", body);
    return 1;
  }
  if (body_len != strlen("grant_type=password&username=test+user")) {
    printf("Form urlencoded length mismatch\n");
    return 1;
  }

  free(body);

  return 0;
}
