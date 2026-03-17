/* clang-format off */
#include "c_rest_request.h"
#include "c_rest_response.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* clang-format on */

int test_request_response(void) {
  struct c_rest_request req;
  struct c_rest_response res;
  const char *val;

  printf("Running request/response tests...\n");

  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  /* Test request query parsing */
  req.query = "id=123&name=test&empty=&no_val";

  val = NULL;
  c_rest_request_get_query(&req, "id", &val);
  if (!val || strcmp(val, "123") != 0) {
    printf("Expected query id=123, got %s\n", val ? val : "NULL");
    return 1;
  }

  val = NULL;
  c_rest_request_get_query(&req, "name", &val);
  if (!val || strcmp(val, "test") != 0) {
    printf("Expected query name=test\n");
    return 1;
  }

  val = NULL;
  c_rest_request_get_query(&req, "empty", &val);
  if (!val || strcmp(val, "") != 0) {
    printf("Expected empty query param\n");
    return 1;
  }

  val = NULL;
  c_rest_request_get_query(&req, "no_val", &val);
  if (!val || strcmp(val, "") != 0) {
    printf("Expected no_val to have empty string\n");
    return 1;
  }

  val = NULL;
  c_rest_request_get_query(&req, "missing", &val);
  if (val != NULL) {
    printf("Expected missing to be NULL\n");
    return 1;
  }

  /* Test response headers */
  c_rest_response_set_header(&res, "Content-Type", "text/plain");
  c_rest_response_set_header(&res, "Content-Type",
                             "application/json"); /* Should replace */

  c_rest_response_set_cookie(&res, "session", "abc", "HttpOnly; Secure");
  c_rest_response_set_cookie(&res, "theme", "dark",
                             NULL); /* Should not replace session */

  /* Verify headers */
  {
    int found_ct = 0;
    int found_cookie_session = 0;
    int found_cookie_theme = 0;
    struct c_rest_header *h;

    for (h = res.headers; h != NULL; h = h->next) {
      if (strcmp(h->key, "Content-Type") == 0) {
        found_ct = 1;
        if (strcmp(h->value, "application/json") != 0) {
          printf("Content-Type was not replaced correctly\n");
          return 1;
        }
      } else if (strcmp(h->key, "Set-Cookie") == 0) {
        if (strstr(h->value, "session=abc") != NULL) {
          found_cookie_session = 1;
        } else if (strstr(h->value, "theme=dark") != NULL) {
          found_cookie_theme = 1;
        }
      }
    }

    if (!found_ct || !found_cookie_session || !found_cookie_theme) {
      printf("Headers not set correctly\n");
      return 1;
    }
  }

  /* Test ETag & Cache Control */
  {
    struct c_rest_header req_h_etag;
    req_h_etag.key = "If-None-Match";
    req_h_etag.value = "\"12345\"";
    req_h_etag.next = NULL;
    req.headers = &req_h_etag;

    c_rest_response_set_cache_control(&res, "max-age=3600");
    if (c_rest_response_check_etag(&req, &res, "\"12345\"")) {
      if (res.status_code != 304) {
        printf("ETag match did not set 304\n");
        return 1;
      }
    } else {
      printf("ETag match failed\n");
      return 1;
    }

    req.headers = NULL; /* remove pointer to stack memory before cleanup */
  }

  /* Test Accepts Encoding */
  {
    struct c_rest_header req_h_enc;
    req_h_enc.key = "Accept-Encoding";
    req_h_enc.value = "gzip, deflate, br";
    req_h_enc.next = NULL;
    req.headers = &req_h_enc;

    if (!c_rest_request_accepts_encoding(&req, "gzip") ||
        !c_rest_request_accepts_encoding(&req, "br") ||
        c_rest_request_accepts_encoding(&req, "identity")) {
      printf("Accept-Encoding parse failed\n");
      return 1;
    }

    req.headers = NULL;
  }

  /* Test large body */
  {
    char *large_body = (char *)malloc(1024 * 1024); /* 1MB */
    char *read_ptr = NULL;
    size_t read_len = 0;

    if (large_body) {
      memset(large_body, 'A', 1024 * 1024 - 1);
      large_body[1024 * 1024 - 1] = '\0';
      req.body = large_body;
      req.body_len = 1024 * 1024 - 1;

      c_rest_request_read_body(&req, &read_ptr, &read_len);
      if (!read_ptr || read_len != 1024 * 1024 - 1 || read_ptr[0] != 'A') {
        printf("Large body read failed\n");
        return 1;
      }
      /* req.body will be freed by cleanup */
    }
  }

  /* Test helpers */
  c_rest_response_json(&res, "{\"hello\":\"world\"}");
  if (res.status_code != 0) { /* Defaults to 0 since we didn't set it */
  }
  if (strcmp(res.body, "{\"hello\":\"world\"}") != 0) {
    printf("JSON body not set correctly\n");
    return 1;
  }

  /* Test JSON Request Parsing */
  {
    void *json_obj = NULL;
    req.body = "{\"key\": \"value\"}";
    req.body_len = strlen(req.body);
    if (c_rest_request_parse_json(&req, &json_obj) != 0 || !json_obj) {
      printf("JSON request parsing failed\n");
      return 1;
    }
    /* Test JSON Response Generation */
    res.headers_sent = 0; /* Reset state */
    if (c_rest_response_json_obj(&res, json_obj) != 0) {
      printf("JSON response generation failed\n");
      return 1;
    }
    /* In parson, you'd typically free it here but we don't include parson.h in
     * test. Assuming we just test it doesn't fail. */
  }

  /* Test JSON Dict Generation */
  {
    struct c_rest_json_pair pairs[] = {
        {"access_token", C_REST_JSON_TYPE_STRING, "test_token_123", 0, 0},
        {"expires_in", C_REST_JSON_TYPE_NUMBER, NULL, 3600.0, 0},
        {"is_active", C_REST_JSON_TYPE_BOOLEAN, NULL, 0, 1},
        {"refresh_token", C_REST_JSON_TYPE_NULL, NULL, 0, 0}};

    res.headers_sent = 0; /* Reset state */
    if (c_rest_response_json_dict(&res, pairs, 4) != 0) {
      printf("JSON dict response generation failed\n");
      return 1;
    }
    if (strstr(res.body, "\"access_token\":\"test_token_123\"") == NULL) {
      printf("JSON dict did not contain access_token\n");
      return 1;
    }
  }

  /* Test URL Encoded Parsing */
  {
    const char *form_val = NULL;
    req.body = "username=admin&password=123%20456&grant_type=password";
    req.body_len = strlen(req.body);
    /* Form params list starts empty since req was cleaned up/not initialized
     * for this */
    if (c_rest_request_parse_urlencoded(&req) != 0) {
      printf("URL encoded parsing failed\n");
      return 1;
    }
    if (c_rest_request_get_form_param(&req, "username", &form_val) != 0 ||
        strcmp(form_val, "admin") != 0) {
      printf("Failed to get username form param\n");
      return 1;
    }
    if (c_rest_request_get_form_param(&req, "password", &form_val) != 0 ||
        strcmp(form_val, "123 456") != 0) {
      printf("Failed to get password form param (or decode failed)\n");
      return 1;
    }
    req.body = NULL; /* Prevent free of string literal */
  }

  /* Test Auth Extraction */
  {
    struct c_rest_header auth_bearer;
    struct c_rest_header auth_basic;
    char *token = NULL;
    char *user = NULL;
    char *pass = NULL;

    auth_bearer.key = "Authorization";
    auth_bearer.value = "Bearer my-token-123";
    auth_bearer.next = NULL;

    req.headers = &auth_bearer;
    if (c_rest_request_get_auth_bearer(&req, &token) != 0 ||
        strcmp(token, "my-token-123") != 0) {
      printf("Bearer token extraction failed\n");
      return 1;
    }
    free(token);

    auth_basic.key = "Authorization";
    /* "admin:secret123" base64 encoded is "YWRtaW46c2VjcmV0MTIz" */
    auth_basic.value = "Basic YWRtaW46c2VjcmV0MTIz";
    auth_basic.next = NULL;

    req.headers = &auth_basic;
    if (c_rest_request_get_auth_basic(&req, &user, &pass) != 0 ||
        strcmp(user, "admin") != 0 || strcmp(pass, "secret123") != 0) {
      printf("Basic auth extraction failed\n");
      return 1;
    }
    free(user);
    free(pass);

    req.headers = NULL; /* Clean stack pointer */
  }

  c_rest_request_cleanup(&req);
  c_rest_response_cleanup(&res);

  return 0;
}
