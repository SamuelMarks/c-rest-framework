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

  c_rest_request_cleanup(&req);
  c_rest_response_cleanup(&res);

  return 0;
}
