/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"

#include "c_rest_middleware.h"
#include "c_rest_modality.h"
#include "c_rest_request.h"
#include "c_rest_response.h"
#include "c_rest_str_utils.h"
#include <cfs/cfs.h>
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

  if (req->method && strcmp(req->method, "OPTIONS") == 0) {
    rc = c_rest_response_set_status(res, 204);
    if (rc != C_REST_OK)
      return rc;
    return C_REST_SHORT_CIRCUIT;
  }

  return C_REST_OK;
}

c_rest_error_t c_rest_logger_middleware(struct c_rest_request *req,
                                        struct c_rest_response *res,
                                        void *user_data) {
  const struct c_rest_logger *logger = (const struct c_rest_logger *)user_data;
  c_rest_log_fn cb = NULL;
  const char *method;
  const char *path;
  char msg[512];
  size_t mlen;
  size_t plen;
  c_rest_error_t rc;

  if (!req || !res)
    return C_REST_ERROR_INVALID_ARG;

  if (logger && logger->log_cb) {
    cb = logger->log_cb;
  } else if (res->context) {
    const struct c_rest_connection_context *conn_ctx =
        (const struct c_rest_connection_context *)res->context;
    if (conn_ctx->framework_ctx && conn_ctx->framework_ctx->logger.log_cb) {
      cb = conn_ctx->framework_ctx->logger.log_cb;
    }
  }

  method = req->method ? req->method : "UNKNOWN";
  path = req->path ? req->path : "/";

  if (!cb) {
    rc = c_rest_framework_log_debug("[HTTP] %s %s", method, path);
    if (rc != C_REST_OK)
      return rc;
    return C_REST_OK;
  }

  mlen = strlen(method);
  plen = strlen(path);
  if (7U + mlen + 1U + plen + 1U > sizeof(msg)) {
    char *dyn_msg = NULL;
    size_t total = 7U + mlen + 1U + plen + 1U;
    rc = C_REST_MALLOC(total, (void **)&dyn_msg);
    if (rc != C_REST_OK)
      return rc;
#if defined(_MSC_VER)
    sprintf_s(dyn_msg, total, "[HTTP] %s %s", method, path);
#else
    sprintf(dyn_msg, "[HTTP] %s %s", method, path);
#endif
    rc = cb(dyn_msg);
    if (rc != C_REST_OK) {
      C_REST_FREE(dyn_msg);
      return rc;
    }
    C_REST_FREE(dyn_msg);
    return C_REST_OK;
  }

#if defined(_MSC_VER)
  sprintf_s(msg, sizeof(msg), "[HTTP] %s %s", method, path);
#else
  sprintf(msg, "[HTTP] %s %s", method, path);
#endif
  rc = cb(msg);
  if (rc != C_REST_OK)
    return rc;
  return C_REST_OK;
}

static NO_DISCARD c_rest_error_t validate_safe_rel_path(const char *raw_path,
                                                        const char **out_rel) {
  const char *p = raw_path;
  const char *seg_start;

  while (*p == '/') {
    p++;
  }
  *out_rel = (*p == '\0') ? "index.html" : p;

  seg_start = p;
  for (;; ++p) {
    unsigned char ch = (unsigned char)*p;
    if (ch == '/' || ch == '\0') {
      size_t seg_len = (size_t)(p - seg_start);
      if ((seg_len == 1U && seg_start[0] == '.') ||
          (seg_len == 2U && seg_start[0] == '.' && seg_start[1] == '.')) {
        return C_REST_SHORT_CIRCUIT;
      }
      if (ch == '\0') {
        break;
      }
      seg_start = p + 1;
    } else if (ch < 0x20U || ch == '\\' || ch == ':') {
      return C_REST_SHORT_CIRCUIT;
    }
  }
  return C_REST_OK;
}

static NO_DISCARD c_rest_error_t detect_mime_type(const char *path,
                                                  const char **out_mime) {
  static const struct {
    const char *ext;
    const char *mime;
  } k_mimes[] = {{".html", "text/html; charset=utf-8"},
                 {".htm", "text/html; charset=utf-8"},
                 {".css", "text/css; charset=utf-8"},
                 {".js", "application/javascript; charset=utf-8"},
                 {".json", "application/json; charset=utf-8"},
                 {".txt", "text/plain; charset=utf-8"},
                 {".svg", "image/svg+xml"},
                 {".png", "image/png"},
                 {".jpg", "image/jpeg"},
                 {".jpeg", "image/jpeg"},
                 {".wasm", "application/wasm"}};
  const char *dot = strrchr(path, '.');
  size_t i;

  if (dot) {
    for (i = 0U; i < sizeof(k_mimes) / sizeof(k_mimes[0]); ++i) {
      int cmp = 1;
      c_rest_error_t rc = c_rest_strcasecmp(dot, k_mimes[i].ext, &cmp);
      if (rc != C_REST_OK)
        return rc;
      if (cmp == 0) {
        *out_mime = k_mimes[i].mime;
        return C_REST_OK;
      }
    }
  }
  *out_mime = "application/octet-stream";
  return C_REST_OK;
}

c_rest_error_t c_rest_static_middleware(struct c_rest_request *req,
                                        struct c_rest_response *res,
                                        void *user_data) {
  const char *root = (const char *)user_data;
  const char *rel = NULL;
  const char *mime = NULL;
  const cfs_char_t *raw_full = NULL;
  const char *utf8_full = NULL;
  cfs_path full_p = {0};
  cfs_file_status st;
  cfs_error_code ec = {0};
  cfs_bool exists = 0;
  cfs_bool is_reg = 0;
  cfs_errc cfs_rc;
  FILE *fp = NULL;
  long fsize;
  int is_get;
  int is_head;
  c_rest_error_t rc;
#if defined(CFS_OS_WINDOWS) && defined(CFS_UNICODE)
  wchar_t wroot[CFS_MAX_PATH];
  wchar_t wrel[CFS_MAX_PATH];
  char utf8_buf[CFS_MAX_PATH * 3];
  cfs_size_t req_len = 0;
#endif

  if (!req || !res) {
    return C_REST_ERROR_INVALID_ARG;
  }
  if (!root || root[0] == '\0' || !req->path || !req->method) {
    return C_REST_OK;
  }

  is_get = (strcmp(req->method, "GET") == 0);
  is_head = (!is_get && strcmp(req->method, "HEAD") == 0);
  if (!is_get && !is_head) {
    return C_REST_OK;
  }

  rc = validate_safe_rel_path(req->path, &rel);
  if (rc != C_REST_OK) {
    c_rest_error_t s_rc = c_rest_response_set_status(res, 403);
    if (s_rc != C_REST_OK)
      return s_rc;
    s_rc = c_rest_response_html(res, "Forbidden");
    if (s_rc != C_REST_OK)
      return s_rc;
    return rc;
  }

#if defined(CFS_OS_WINDOWS) && defined(CFS_UNICODE)
  cfs_rc = cfs_utf8_to_utf16(root, wroot, sizeof(wroot) / sizeof(wroot[0]),
                             &req_len);
  if (cfs_rc != cfs_errc_success) {
    return c_rest_error_from_cfs((int)cfs_rc);
  }
  cfs_rc =
      cfs_utf8_to_utf16(rel, wrel, sizeof(wrel) / sizeof(wrel[0]), &req_len);
  if (cfs_rc != cfs_errc_success) {
    return c_rest_error_from_cfs((int)cfs_rc);
  }
  cfs_rc = cfs_path_init_str(&full_p, wroot);
  if (cfs_rc != cfs_errc_success) {
    return c_rest_error_from_cfs((int)cfs_rc);
  }
  cfs_rc = cfs_path_append(&full_p, wrel);
  if (cfs_rc != cfs_errc_success) {
    cfs_path_destroy(&full_p);
    return c_rest_error_from_cfs((int)cfs_rc);
  }
#else
  cfs_rc = cfs_path_init_str(&full_p, (const cfs_char_t *)root);
  if (cfs_rc != cfs_errc_success) {
    return c_rest_error_from_cfs((int)cfs_rc);
  }
  cfs_rc = cfs_path_append(&full_p, (const cfs_char_t *)rel);
  if (cfs_rc != cfs_errc_success) {
    cfs_path_destroy(&full_p);
    return c_rest_error_from_cfs((int)cfs_rc);
  }
#endif

  cfs_rc = cfs_exists_path(&full_p, &exists, &ec);
  if (cfs_rc != cfs_errc_success) {
    cfs_path_destroy(&full_p);
    return c_rest_error_from_cfs((int)cfs_rc);
  }
  if (!exists) {
    cfs_path_destroy(&full_p);
    return C_REST_OK;
  }

  cfs_rc = cfs_status(&full_p, &st, &ec);
  if (cfs_rc != cfs_errc_success) {
    cfs_path_destroy(&full_p);
    return c_rest_error_from_cfs((int)cfs_rc);
  }

  cfs_rc = cfs_is_regular_file(st, &is_reg);
  if (cfs_rc != cfs_errc_success) {
    cfs_path_destroy(&full_p);
    return c_rest_error_from_cfs((int)cfs_rc);
  }
  if (!is_reg) {
    cfs_path_destroy(&full_p);
    return C_REST_OK;
  }

  cfs_rc = cfs_path_c_str(&full_p, &raw_full);
  if (cfs_rc != cfs_errc_success) {
    cfs_path_destroy(&full_p);
    return c_rest_error_from_cfs((int)cfs_rc);
  }

#if defined(CFS_OS_WINDOWS) && defined(CFS_UNICODE)
  cfs_rc = cfs_utf16_to_utf8(raw_full, utf8_buf, sizeof(utf8_buf), &req_len);
  if (cfs_rc != cfs_errc_success) {
    cfs_path_destroy(&full_p);
    return c_rest_error_from_cfs((int)cfs_rc);
  }
  utf8_full = utf8_buf;
#else
  utf8_full = (const char *)raw_full;
#endif

#if defined(_MSC_VER)
  if (fopen_s(&fp, utf8_full, "rb") != 0 || !fp) {
    cfs_path_destroy(&full_p);
    return C_REST_ERROR_IO;
  }
#else
  fp = fopen(utf8_full, "rb");
  if (!fp) {
    cfs_path_destroy(&full_p);
    return C_REST_ERROR_IO;
  }
#endif
  cfs_path_destroy(&full_p);

  if (fseek(fp, 0L, SEEK_END) != 0) {
    fclose(fp);
    return C_REST_ERROR_IO;
  }
  fsize = ftell(fp);
  if (fsize < 0L || fseek(fp, 0L, SEEK_SET) != 0) {
    fclose(fp);
    return C_REST_ERROR_IO;
  }

  rc = detect_mime_type(rel, &mime);
  if (rc != C_REST_OK) {
    fclose(fp);
    return rc;
  }

  if (is_get) {
    char *file_buf = NULL;
    size_t read_n;
    rc = C_REST_MALLOC((size_t)fsize + 1U, (void **)&file_buf);
    if (rc != C_REST_OK) {
      fclose(fp);
      return rc;
    }
    read_n = (fsize > 0L) ? fread(file_buf, 1U, (size_t)fsize, fp) : 0U;
    file_buf[read_n] = '\0';
    fclose(fp);
    if (res->body) {
      C_REST_FREE(res->body);
    }
    res->body = file_buf;
    res->body_len = read_n;
  } else {
    char cl_buf[32];
    fclose(fp);
#if defined(_MSC_VER)
    sprintf_s(cl_buf, sizeof(cl_buf), C_REST_FMT_SIZE_T,
              CAST_SIZE_T((size_t)fsize));
#else
    sprintf(cl_buf, C_REST_FMT_SIZE_T, CAST_SIZE_T((size_t)fsize));
#endif
    rc = c_rest_response_set_header(res, "Content-Length", cl_buf);
    if (rc != C_REST_OK)
      return rc;
  }

  rc = c_rest_response_set_status(res, 200);
  if (rc != C_REST_OK)
    return rc;
  rc = c_rest_response_set_header(res, "Content-Type", mime);
  if (rc != C_REST_OK)
    return rc;

  return C_REST_SHORT_CIRCUIT;
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
      const char *path = req->path ? req->path : "/";
      rc = c_rest_request_get_header(req, "Host", &host);
      if (rc != C_REST_OK) {
        host = "localhost";
        goto build_redirect_url;
      }
      if (!host) {
        host = "localhost";
      }
    build_redirect_url:
      if (8 + strlen(host) + strlen(path) + 1 > sizeof(url))
        return C_REST_ERROR_GENERIC;
#if defined(_MSC_VER)
      sprintf_s(url, sizeof(url), "https://%s%s", host, path);
#else
      sprintf(url, "https://%s%s", host, path);
#endif
      rc = c_rest_response_redirect(res, url, 301);
      if (rc != C_REST_OK)
        return rc;
      return C_REST_SHORT_CIRCUIT;
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

  if (!req || !res) {
    return C_REST_ERROR_GENERIC;
  }

  if (!user_data) {
    rc = c_rest_response_set_status(res, 500);
    if (rc != C_REST_OK)
      return rc;
    rc = c_rest_response_html(res,
                              "Internal Server Error: Missing auth verifier");
    if (rc != C_REST_OK)
      return rc;
    return C_REST_ERROR_GENERIC;
  }

  verifier = (struct c_rest_auth_verifier *)user_data;

  rc = c_rest_request_get_auth_bearer(req, &token);
  if (rc != C_REST_OK) {
    c_rest_error_t basic_rc = c_rest_request_get_auth_basic(req, &user, &pass);
    if (basic_rc != C_REST_OK) {
      c_rest_error_t s_rc = c_rest_response_set_status(res, 401);
      if (s_rc != C_REST_OK)
        return s_rc;
      s_rc = c_rest_response_set_header(res, "WWW-Authenticate",
                                        "Bearer realm=\"API\"");
      if (s_rc != C_REST_OK)
        return s_rc;
      s_rc = c_rest_response_html(res, "Unauthorized: Missing authentication");
      if (s_rc != C_REST_OK)
        return s_rc;
      return basic_rc;
    }
    goto verify_creds;
  }
  is_bearer = 1;

verify_creds:
  if (is_bearer) {
    c_rest_error_t v_rc;
    if (!verifier->verify_bearer) {
      C_REST_FREE((void *)(token));
      rc = c_rest_response_set_status(res, 500);
      if (rc != C_REST_OK)
        return rc;
      rc = c_rest_response_html(res, "Internal Server Error: Bearer auth not "
                                     "supported by verifier");
      if (rc != C_REST_OK)
        return rc;
      return C_REST_ERROR_GENERIC;
    }
    v_rc = verifier->verify_bearer(token, &auth_ctx);
    if (v_rc != C_REST_OK) {
      C_REST_FREE((void *)(token));
      rc = c_rest_response_set_status(res, 401);
      if (rc != C_REST_OK)
        return rc;
      rc = c_rest_response_set_header(
          res, "WWW-Authenticate",
          "Bearer realm=\"API\", error=\"invalid_token\"");
      if (rc != C_REST_OK)
        return rc;
      rc = c_rest_response_html(res, "Unauthorized: Invalid token");
      if (rc != C_REST_OK)
        return rc;
      return v_rc;
    }
    C_REST_FREE((void *)(token));
  } else {
    c_rest_error_t v_rc;
    if (!verifier->verify_basic) {
      C_REST_FREE((void *)(user));
      C_REST_FREE((void *)(pass));
      rc = c_rest_response_set_status(res, 500);
      if (rc != C_REST_OK)
        return rc;
      rc = c_rest_response_html(res, "Internal Server Error: Basic auth not "
                                     "supported by verifier");
      if (rc != C_REST_OK)
        return rc;
      return C_REST_ERROR_GENERIC;
    }
    v_rc = verifier->verify_basic(user, pass, &auth_ctx);
    if (v_rc != C_REST_OK) {
      C_REST_FREE((void *)(user));
      C_REST_FREE((void *)(pass));
      rc = c_rest_response_set_status(res, 401);
      if (rc != C_REST_OK)
        return rc;
      rc = c_rest_response_set_header(res, "WWW-Authenticate",
                                      "Basic realm=\"API\"");
      if (rc != C_REST_OK)
        return rc;
      rc = c_rest_response_html(res, "Unauthorized: Invalid credentials");
      if (rc != C_REST_OK)
        return rc;
      return v_rc;
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
  c_rest_error_t auth_rc;
  c_rest_error_t v_rc;
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
    rc = c_rest_response_set_status(res, 500);
    if (rc != C_REST_OK)
      return rc;
    rc = c_rest_response_html(res,
                              "Internal Server Error: Missing OAuth2 verifier");
    if (rc != C_REST_OK)
      return rc;
    return C_REST_ERROR_GENERIC;
  }

  u.ptr = user_data;
  verify_fn = u.func;

  auth_rc = c_rest_request_get_auth_bearer(req, &token);
  if (auth_rc != C_REST_OK) {
    rc = c_rest_response_set_status(res, 401);
    if (rc != C_REST_OK)
      return rc;
    rc = c_rest_response_set_header(res, "WWW-Authenticate",
                                    "Bearer realm=\"API\"");
    if (rc != C_REST_OK)
      return rc;
    rc = c_rest_response_html(res,
                              "Unauthorized: Missing or invalid Bearer token");
    if (rc != C_REST_OK)
      return rc;
    return auth_rc;
  }

  v_rc = verify_fn(token, &auth_ctx);
  if (v_rc != C_REST_OK) {
    C_REST_FREE((void *)(token));
    rc = c_rest_response_set_status(res, 401);
    if (rc != C_REST_OK)
      return rc;
    rc = c_rest_response_set_header(
        res, "WWW-Authenticate",
        "Bearer realm=\"API\", error=\"invalid_token\"");
    if (rc != C_REST_OK)
      return rc;
    rc = c_rest_response_html(res, "Unauthorized: Invalid token");
    if (rc != C_REST_OK)
      return rc;
    return v_rc;
  }

  C_REST_FREE((void *)(token));
  req->auth_context = auth_ctx;

  return C_REST_OK;
}
