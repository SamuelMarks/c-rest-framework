/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"

#include "c_rest_middleware.h"
#include "c_rest_request.h"
#include "c_rest_response.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "c_rest_log.h"
/* clang-format on */

c_rest_error_t c_rest_cors_middleware(struct c_rest_request *req,
                                      struct c_rest_response *res,
                                      void *user_data) {
  c_rest_error_t rc;
  (void)user_data;
  if (!req || !res)
    return C_REST_ERROR_GENERIC;

  rc = c_rest_response_set_header(res, "Access-Control-Allow-Origin", "*");
  if (rc != C_REST_OK)
    return rc;
  rc = c_rest_response_set_header(res, "Access-Control-Allow-Methods",
                                  "GET, POST, PUT, DELETE, OPTIONS");
  if (rc != C_REST_OK)
    return rc;
  rc = c_rest_response_set_header(res, "Access-Control-Allow-Headers",
                                  "Content-Type, Authorization");
  if (rc != C_REST_OK)
    return rc;

  if (strcmp(req->method, "OPTIONS") == 0) {
    res->status_code = 204;
    return C_REST_ERROR_GENERIC; /* Short-circuit */
  }

  return C_REST_OK; /* Continue */
}

c_rest_error_t c_rest_logger_middleware(struct c_rest_request *req,
                                        struct c_rest_response *res,
                                        void *user_data) {
  (void)res;
  (void)user_data;
  /* Simple mock logging */
  if (req && req->method && req->path) {
    /* In reality, use framework logger */
  }
  return C_REST_OK;
}

c_rest_error_t c_rest_static_middleware(struct c_rest_request *req,
                                        struct c_rest_response *res,
                                        void *user_data) {
  /* const char *root = (const char *)user_data; */
  (void)req;
  (void)res;
  (void)user_data;
  /* Check file existence, set content type, write body. */
  return C_REST_OK;
}

c_rest_error_t c_rest_hsts_middleware(struct c_rest_request *req,
                                      struct c_rest_response *res,
                                      void *user_data) {
  c_rest_error_t rc;
  (void)req;
  (void)user_data;
  if (!res)
    return C_REST_ERROR_GENERIC;
  rc = c_rest_response_set_header(res, "Strict-Transport-Security",
                                  "max-age=31536000; includeSubDomains");
  if (rc != C_REST_OK)
    return rc;
  return C_REST_OK;
}

c_rest_error_t c_rest_https_redirect_middleware(struct c_rest_request *req,
                                                struct c_rest_response *res,
                                                void *user_data) {
  c_rest_error_t rc;
  (void)user_data;
  if (!res || !req)
    return C_REST_ERROR_GENERIC;
  if (req->scheme) {
    if (strcmp(req->scheme, "https") != 0) {
      char url[1024];
      const char *host = NULL;
      rc = c_rest_request_get_header(req, "Host", &host);
      if (rc != C_REST_OK || !host)
        host = "localhost";
#if defined(_MSC_VER)
      sprintf_s(url, sizeof(url), "https://%s%s", host,
                req->path ? req->path : "/");
#else
      sprintf(url, "https://%s%s", host, req->path ? req->path : "/");
#endif
      rc = c_rest_response_redirect(res, url, 301);
      return rc;
    }
  }
  return C_REST_OK;
}

c_rest_error_t c_rest_auth_middleware(struct c_rest_request *req,
                                      struct c_rest_response *res,
                                      void *user_data) {
  c_rest_error_t rc;
  struct c_rest_auth_verifier *verifier;
  char *token = NULL;
  char *user = NULL;
  char *pass = NULL;
  void *auth_ctx = NULL;
  int is_bearer = 0;
  int is_basic = 0;

  if (!req || !res) {
    return C_REST_ERROR_GENERIC;
  }

  if (!user_data) {
    (void)!c_rest_response_set_status(res, 500);
    rc = c_rest_response_html(res,
                              "Internal Server Error: Missing auth verifier");
    if (rc != C_REST_OK)
      return rc;
    return C_REST_ERROR_GENERIC;
  }

  verifier = (struct c_rest_auth_verifier *)user_data;

  rc = c_rest_request_get_auth_bearer(req, &token);
  if (rc == C_REST_OK) {
    is_bearer = 1;
  } else {
    rc = c_rest_request_get_auth_basic(req, &user, &pass);
    if (rc == C_REST_OK) {
      is_basic = 1;
    }
  }

  if (!is_bearer && !is_basic) {
    (void)!c_rest_response_set_status(res, 401);
    rc = c_rest_response_set_header(res, "WWW-Authenticate",
                                    "Bearer realm=\"API\"");
    if (rc != C_REST_OK)
      return rc;
    rc = c_rest_response_html(res, "Unauthorized: Missing authentication");
    if (rc != C_REST_OK)
      return rc;
    return C_REST_ERROR_GENERIC;
  }

  if (is_bearer) {
    if (!verifier->verify_bearer) {
      C_REST_FREE((void *)(token));
      (void)!c_rest_response_set_status(res, 500);
      rc = c_rest_response_html(res, "Internal Server Error: Bearer auth not "
                                     "supported by verifier");
      if (rc != C_REST_OK)
        return rc;
      return C_REST_ERROR_GENERIC;
    }
    rc = (c_rest_error_t)verifier->verify_bearer(token, &auth_ctx);
    if (rc != C_REST_OK) {
      C_REST_FREE((void *)(token));
      (void)!c_rest_response_set_status(res, 401);
      rc = c_rest_response_set_header(
          res, "WWW-Authenticate",
          "Bearer realm=\"API\", error=\"invalid_token\"");
      if (rc != C_REST_OK)
        return rc;
      rc = c_rest_response_html(res, "Unauthorized: Invalid token");
      if (rc != C_REST_OK)
        return rc;
      return C_REST_ERROR_GENERIC;
    }
    C_REST_FREE((void *)(token));
  } else { /* is_basic */
    if (!verifier->verify_basic) {
      C_REST_FREE((void *)(user));
      C_REST_FREE((void *)(pass));
      (void)!c_rest_response_set_status(res, 500);
      rc = c_rest_response_html(res, "Internal Server Error: Basic auth not "
                                     "supported by verifier");
      if (rc != C_REST_OK)
        return rc;
      return C_REST_ERROR_GENERIC;
    }
    if (verifier->verify_basic(user, pass, &auth_ctx) != 0) {
      C_REST_FREE((void *)(user));
      C_REST_FREE((void *)(pass));
      (void)!c_rest_response_set_status(res, 401);
      rc = c_rest_response_set_header(res, "WWW-Authenticate",
                                      "Basic realm=\"API\"");
      if (rc != C_REST_OK)
        return rc;
      rc = c_rest_response_html(res, "Unauthorized: Invalid credentials");
      if (rc != C_REST_OK)
        return rc;
      return C_REST_ERROR_GENERIC;
    }
    C_REST_FREE((void *)(user));
    C_REST_FREE((void *)(pass));
  }

  req->auth_context = auth_ctx;
  return C_REST_OK;
}

c_rest_error_t c_rest_oauth2_middleware(struct c_rest_request *req,
                                        struct c_rest_response *res,
                                        void *user_data) {
  c_rest_error_t rc;
  char *token = NULL;
  c_rest_oauth2_verify_fn verify_fn;
  void *auth_ctx = NULL;
  union {
    void *ptr;
    c_rest_oauth2_verify_fn func;
  } u;

  if (!req || !res) {
    return C_REST_ERROR_GENERIC;
  }

  if (!user_data) {
    (void)!c_rest_response_set_status(res, 500);
    rc = c_rest_response_html(res,
                              "Internal Server Error: Missing OAuth2 verifier");
    if (rc != C_REST_OK)
      return rc;
    return C_REST_ERROR_GENERIC;
  }

  u.ptr = user_data;
  verify_fn = u.func;

  rc = c_rest_request_get_auth_bearer(req, &token);
  if (rc != C_REST_OK) {
    (void)!c_rest_response_set_status(res, 401);
    rc = c_rest_response_set_header(res, "WWW-Authenticate",
                                    "Bearer realm=\"API\"");
    if (rc != C_REST_OK)
      return rc;
    rc = c_rest_response_html(res,
                              "Unauthorized: Missing or invalid Bearer token");
    if (rc != C_REST_OK)
      return rc;
    return C_REST_ERROR_GENERIC;
  }

  if (verify_fn(token, &auth_ctx) != 0) {
    C_REST_FREE((void *)(token));
    (void)!c_rest_response_set_status(res, 401);
    rc = c_rest_response_set_header(
        res, "WWW-Authenticate",
        "Bearer realm=\"API\", error=\"invalid_token\"");
    if (rc != C_REST_OK)
      return rc;
    rc = c_rest_response_html(res, "Unauthorized: Invalid token");
    if (rc != C_REST_OK)
      return rc;
    return C_REST_ERROR_GENERIC;
  }

  C_REST_FREE((void *)(token));
  req->auth_context = auth_ctx;

  return C_REST_OK;
}
