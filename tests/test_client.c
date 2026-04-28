/* clang-format off */
#include "test_protos.h"
#include "c_rest_client.h"
#include "c_rest_tls.h"
#include <parson.h>

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
printf("test_client Line 20\n"); fflush(stdout);
  struct c_rest_client_response *sync_res = NULL;
printf("test_client Line 21\n"); fflush(stdout);
  struct c_rest_client_header headers[1];
printf("test_client Line 22\n"); fflush(stdout);
  int res;
  char *encoded = NULL;
  char *decoded = NULL;
  struct c_rest_client_form_field fields[2];
printf("test_client Line 26\n"); fflush(stdout);
  char *body = NULL;
  size_t body_len = 0;

  struct c_rest_client_form_field *parsed_fields = NULL;
printf("test_client Line 30\n"); fflush(stdout);
  size_t parsed_count = 0;
  struct c_rest_client_header *custom_headers = NULL;
printf("test_client Line 32\n"); fflush(stdout);
  size_t custom_headers_count = 0;
  char *auth_basic = NULL;
  char *auth_bearer = NULL;
  void *json = NULL;

  printf("Running client tests...\n");

  res = c_rest_tls_init();
printf("test_client Line 40\n"); fflush(stdout);
  if (res != 0) {
    printf("Failed to init TLS\n");
    return 1;
  }

  res = c_rest_client_init(&client);
printf("test_client Line 46\n"); fflush(stdout);
  if (res != 0 || !client) {
    printf("Failed to init client %d\n", res);
    return 1;
  }

  headers[0].key = "X-Test-Header";
  headers[0].value = "TestValue";

  res = c_rest_client_request_sync(client, "http://localhost", "GET", headers,
                                   1, NULL, 0, &sync_res);
  (void)res;
  if (sync_res) {
    c_rest_client_response_free(sync_res);
printf("test_client Line 59\n"); fflush(stdout);
  }

  res = c_rest_client_request_async(client, "http://localhost", "POST", NULL, 0,
                                    "test", 4, async_callback, NULL);
  (void)res;

  if (!async_called) {
    printf("Async callback was not invoked\n");
    return 1;
  }

  res = c_rest_proxy_request("http://localhost/proxy", NULL, NULL);
printf("test_client Line 71\n"); fflush(stdout);
  (void)res;

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

  /* Test Form URL encoded parser */
  if (c_rest_client_parse_form_urlencoded(body, &parsed_fields,
                                          &parsed_count) != 0) {
    printf("Failed to parse form urlencoded\n");
    return 1;
  }
  if (parsed_count != 2) {
    printf("Parsed count mismatch: %lu\n", (unsigned long)parsed_count);
    return 1;
  }
  if (strcmp(parsed_fields[0].key, "grant_type") != 0 ||
      strcmp(parsed_fields[0].value, "password") != 0) {
    printf("Parsed field 0 mismatch\n");
    return 1;
  }
  if (strcmp(parsed_fields[1].key, "username") != 0 ||
      strcmp(parsed_fields[1].value, "test user") != 0) {
    printf("Parsed field 1 mismatch\n");
    return 1;
  }
  c_rest_client_form_fields_free(parsed_fields, parsed_count);
printf("test_client Line 136\n"); fflush(stdout);
  free(body);

  /* Test Header Builders */
  if (c_rest_client_header_set(&custom_headers, &custom_headers_count, "Accept",
                               "application/json") != 0) {
    printf("Failed to set header\n");
    return 1;
  }
  if (c_rest_client_header_set(&custom_headers, &custom_headers_count,
                               "Custom-Key", "Custom-Val") != 0) {
    printf("Failed to set header\n");
    return 1;
  }
  if (custom_headers_count != 2)
    return 1;
  if (strcmp(custom_headers[0].key, "Accept") != 0)
    return 1;
  c_rest_client_headers_free(custom_headers, custom_headers_count);
printf("test_client Line 154\n"); fflush(stdout);

  /* Test Auth basic/bearer */
  if (c_rest_client_build_auth_basic("Aladdin", "open sesame", &auth_basic) !=
      0) {
    return 1;
  }
  if (strcmp(auth_basic, "Basic QWxhZGRpbjpvcGVuIHNlc2FtZQ==") != 0) {
    printf("Auth basic mismatch: %s\n", auth_basic);
    return 1;
  }
  free(auth_basic);

  if (c_rest_client_build_auth_bearer("some_token", &auth_bearer) != 0) {
    return 1;
  }
  if (strcmp(auth_bearer, "Bearer some_token") != 0) {
    return 1;
  }
  free(auth_bearer);

  /* Test c_rest_client_post_form_sync */
  res = c_rest_client_post_form_sync(client, "http://localhost", NULL, 0,
                                     fields, 2, &sync_res);
  if (sync_res) {
    c_rest_client_response_free(sync_res);
printf("test_client Line 179\n"); fflush(stdout);
    sync_res = NULL;
  }

  /* Test JSON parsing on dummy response */
  {
    struct c_rest_client_response dummy_res;
printf("test_client Line 185\n"); fflush(stdout);
    dummy_res.body = (void *)"{\"key\":\"value\"}";
    dummy_res.body_len = strlen((char *)dummy_res.body);
    if (c_rest_client_response_parse_json(&dummy_res, &json) != 0) {
      return 1;
    }
    if (!json)
      return 1;
    json_value_free((JSON_Value *)json);
  }

  c_rest_client_destroy(client);
printf("test_client Line 196\n"); fflush(stdout);
  return 0;
}
