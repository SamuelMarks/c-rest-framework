
/* clang-format off */
#include "c_rest_middleware.h"
#include "c_rest_request.h"
#include "c_rest_response.h"
#include <stdio.h>
#include <string.h>
/* clang-format on */

int c_rest_cors_middleware(struct c_rest_request *req,
                           struct c_rest_response *res, void *user_data) {
  (void)req;
  (void)user_data;
  /* Set CORS headers here. For now just mock. */
  /* res->headers["Access-Control-Allow-Origin"] = "*"; */
  res->status_code = 200;
  return 0;
}

int c_rest_logger_middleware(struct c_rest_request *req,
                             struct c_rest_response *res, void *user_data) {
  (void)res;
  (void)user_data;
  /* Simple mock logging */
  if (req && req->method && req->path) {
    /* In reality, use framework logger */
  }
  return 0;
}

int c_rest_static_middleware(struct c_rest_request *req,
                             struct c_rest_response *res, void *user_data) {
  /* const char *root = (const char *)user_data; */
  (void)req;
  (void)res;
  (void)user_data;
  /* Check file existence, set content type, write body. */
  return 0;
}

int c_rest_hsts_middleware(struct c_rest_request *req,
                           struct c_rest_response *res, void *user_data) {
  (void)req;
  (void)user_data;
  if (!res)
    return 1;
  c_rest_response_set_header(res, "Strict-Transport-Security",
                             "max-age=31536000; includeSubDomains");
  return 0;
}

int c_rest_https_redirect_middleware(struct c_rest_request *req,
                                     struct c_rest_response *res,
                                     void *user_data) {
  (void)user_data;
  if (!res || !req)
    return 1;
  if (req->scheme) {
    if (strcmp(req->scheme, "https") != 0) {
      char url[1024];
      const char *host = NULL;
      if (c_rest_request_get_header(req, "Host", &host) != 0 || !host)
        host = "localhost";
#if defined(_MSC_VER)
      sprintf_s(url, sizeof(url), "https://%s%s", host,
                req->path ? req->path : "/");
#else
      sprintf(url, "https://%s%s", host, req->path ? req->path : "/");
#endif
      return c_rest_response_redirect(res, url, 301);
    }
  }
  return 0;
}
