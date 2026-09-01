#include "c_rest_error.h"
#include "c_rest_mem.h"
/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_request.h" /* For struct c_rest_header */
#include "c_rest_response.h"
#define IGNORE_RC(expr) { c_rest_error_t _ign_rc = (expr); (void)_ign_rc; }
#include "c_rest_modality.h"
#include <parson.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "c_rest_log.h"

#include <ctype.h>
#include "c_rest_str_utils.h"

#if defined(_MSC_VER)
#define SAFE_STRCPY(dest, size, src) strcpy_s(dest, size, src)
#else
#define SAFE_STRCPY(dest, size, src) strcpy(dest, src)
#endif


c_rest_error_t c_rest_response_set_header(struct c_rest_response *res, const char *key,
                               const char *value) {
  c_rest_error_t rc;
  struct c_rest_header *h;
  struct c_rest_header *new_h;
  size_t val_len;
  int cmp;

  if (!res || !key || !value) {
    return C_REST_ERROR_GENERIC;
  }

  /* Check if it already exists, replace value if it does */
  rc = c_rest_strcasecmp(key, "Set-Cookie", &cmp);
  if (rc != C_REST_OK) return rc;

  if (cmp != 0) {
    for (h = res->headers; h != NULL; h = h->next) {
      int h_cmp;
      rc = c_rest_strcasecmp(h->key, key, &h_cmp);
      if (rc != C_REST_OK) return rc;
      if (h_cmp == 0) {
        char *new_val;

val_len = strlen(value) + 1;

        if (C_REST_MALLOC(val_len, &new_val) != 0) { LOG_DEBUG("C_REST_MALLOC failed"); new_val = NULL; }
        if (!new_val) {
          return C_REST_ERROR_GENERIC;
        }
        SAFE_STRCPY(new_val, val_len, value);
        C_REST_FREE((void *)(h->value));
        h->value = new_val;
        return C_REST_OK;
      }
    }
  }

  /* Add new header */
  if (C_REST_MALLOC(sizeof(struct c_rest_header), &new_h) != 0) { LOG_DEBUG("C_REST_MALLOC failed"); new_h = NULL; }
  if (!new_h) {
    return C_REST_ERROR_GENERIC;
  }
  if (C_REST_MALLOC(strlen(key) + 1, &new_h->key) != 0) { LOG_DEBUG("C_REST_MALLOC failed"); new_h->key = NULL; }
  if (C_REST_MALLOC(strlen(value) + 1, &new_h->value) != 0) { LOG_DEBUG("C_REST_MALLOC failed"); new_h->value = NULL; }
  if (!new_h->key || !new_h->value) {
    C_REST_FREE((void *)(new_h->key));
    C_REST_FREE((void *)(new_h->value));
    C_REST_FREE((void *)(new_h));
    return C_REST_ERROR_GENERIC;
  }

SAFE_STRCPY(new_h->key, strlen(key) + 1, key);


SAFE_STRCPY(new_h->value, strlen(value) + 1, value);


  new_h->next = res->headers;
  res->headers = new_h;

  return C_REST_OK;
}

c_rest_error_t c_rest_response_set_status(struct c_rest_response *res, int status_code) {
  if (!res) {
    return C_REST_ERROR_GENERIC;
  }
  res->status_code = status_code;
  return C_REST_OK;
}

c_rest_error_t c_rest_response_check_etag(struct c_rest_request *req,
                               struct c_rest_response *res, const char *etag) {
  const char *if_none_match;
  c_rest_error_t rc;
  if (!req || !res || !etag) {
    return C_REST_OK;
  }

  rc = c_rest_response_set_header(res, "ETag", etag);

  if (rc != C_REST_OK) return rc;

  rc = c_rest_request_get_header(req, "If-None-Match", &if_none_match);
  if (rc == C_REST_OK) {
    if (strcmp(if_none_match, etag) == 0) {
      IGNORE_RC(c_rest_response_set_status(res, 304));
      return C_REST_ERROR_GENERIC; /* Match found */
    }
  }

  return C_REST_OK;
}

c_rest_error_t c_rest_response_set_cache_control(struct c_rest_response *res,
                                      const char *policy) {
  if (!res || !policy) {
    return C_REST_ERROR_GENERIC;
  }
  return c_rest_response_set_header(res, "Cache-Control", policy);
}

c_rest_error_t c_rest_response_send(struct c_rest_response *res) {
  struct c_rest_connection_context *ctx;
  c_rest_error_t rc;
  char header_buf[4096];
  size_t offset = 0;
  size_t written = 0;
  struct c_rest_header *h;
  const char *status_text = "OK";

  if (!res) {
    return C_REST_ERROR_GENERIC;
  }
  if (res->headers_sent) {
    return C_REST_ERROR_GENERIC;
  }

  if (res->status_code == 400)
    status_text = "Bad Request";
  else if (res->status_code == 401)
    status_text = "Unauthorized";
  else if (res->status_code == 404)
    status_text = "Not Found";
  else if (res->status_code == 500)
    status_text = "Internal Server Error";

  if (!res->is_chunked) {
    char cl_buf[32];
#if defined(_MSC_VER)
    sprintf_s(cl_buf, sizeof(cl_buf), C_REST_FMT_SIZE_T, CAST_SIZE_T(res->body_len));
#else
    sprintf(cl_buf, C_REST_FMT_SIZE_T, CAST_SIZE_T(res->body_len));
#endif
    rc = c_rest_response_set_header(res, "Content-Length", cl_buf);
    if (rc != C_REST_OK) return rc;
  }

#if defined(_MSC_VER)
  offset += (size_t)sprintf_s(header_buf + offset, sizeof(header_buf) - offset,
                      "HTTP/1.1 %d %s\r\n", res->status_code, status_text);
#else
  offset += (size_t)sprintf(header_buf + offset, "HTTP/1.1 %d %s\r\n", res->status_code,
                    status_text);
#endif

  for (h = res->headers; h != NULL; h = h->next) {
#if defined(_MSC_VER)
    offset += (size_t)sprintf_s(header_buf + offset, sizeof(header_buf) - offset,
                        "%s: %s\r\n", h->key, h->value);
#else

#ifdef _MSC_VER
/* CDD_SAFE_CRT */ offset += (size_t)sprintf_s(header_buf + offset, sizeof(header_buf) - offset, "%s: %s\r\n", h->key, h->value);
#else
/* CDD_SAFE_CRT */ offset += (size_t)sprintf(header_buf + offset, "%s: %s\r\n", h->key, h->value);
#endif

#endif
  }

#if defined(_MSC_VER)
  offset += (size_t)sprintf_s(header_buf + offset, sizeof(header_buf) - offset, "\r\n");
#else
  offset += (size_t)sprintf(header_buf + offset, "\r\n");
#endif

  ctx = (struct c_rest_connection_context *)res->context;
  if (ctx) {
    if (ctx->tls_conn) {
      IGNORE_RC(c_rest_tls_write(ctx->tls_conn, header_buf, offset, &written));
      if (res->body && res->body_len > 0) {
        IGNORE_RC(c_rest_tls_write(ctx->tls_conn, res->body, res->body_len, &written));
      }
    } else {
#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
      if (ctx->cm_env) {
        rc = c_rest_socket_send((c_rest_socket_t)ctx->sock, header_buf, offset, &written);
        if (rc != C_REST_OK) return rc;
        if (res->body && res->body_len > 0) {
          rc = c_rest_socket_send((c_rest_socket_t)ctx->sock, res->body, res->body_len,
                         &written);
          if (rc != C_REST_OK) return rc;
        }
      } else {
        rc = c_rest_socket_send(ctx->sock, header_buf, offset, &written);
        if (rc != C_REST_OK) return rc;
        if (res->body && res->body_len > 0) {
          rc = c_rest_socket_send(ctx->sock, res->body, res->body_len, &written);
          if (rc != C_REST_OK) return rc;
        }
      }
#else
        IGNORE_RC(c_rest_socket_send(ctx->sock, header_buf, offset, &written));
        if (res->body && res->body_len > 0) {
          IGNORE_RC(c_rest_socket_send(ctx->sock, res->body, res->body_len, &written));
        }
#endif
    }
  }

  res->headers_sent = 1;
  return C_REST_OK;
}

c_rest_error_t c_rest_response_json(struct c_rest_response *res, const char *json_str) {
  size_t len;
  c_rest_error_t rc;
  if (!res || !json_str) {
    return C_REST_ERROR_GENERIC;
  }

len = strlen(json_str);

  rc = c_rest_response_set_header(res, "Content-Type", "application/json");

  if (rc != C_REST_OK) return rc;

  if (res->body) {
    C_REST_FREE((void *)(res->body));
  }
  if (C_REST_MALLOC(len + 1, &res->body) != 0) { LOG_DEBUG("C_REST_MALLOC failed"); res->body = NULL; }
  if (!res->body) {
    return C_REST_ERROR_GENERIC;
  }
  SAFE_STRCPY(res->body, len + 1, json_str);
  res->body_len = len;

  return c_rest_response_send(res);
}

c_rest_error_t c_rest_response_json_obj(struct c_rest_response *res, void *json_obj) {
  char *json_str;
  c_rest_error_t rc;

  if (!res || !json_obj) {
    return C_REST_ERROR_GENERIC;
  }

  json_str = json_serialize_to_string((JSON_Value *)json_obj);

  rc = c_rest_response_json(res, json_str);
  if (rc != C_REST_OK) {
      json_free_serialized_string(json_str);
      return rc;
  }
  json_free_serialized_string(json_str);
  return C_REST_OK;
  }

  c_rest_error_t c_rest_response_json_dict(struct c_rest_response *res,
                              const struct c_rest_json_pair *pairs,
                              size_t count) {
  JSON_Value *root_val;
  JSON_Object *root_obj;
  size_t i;
  c_rest_error_t rc;

  if (!res) {
    return C_REST_ERROR_GENERIC;
  }

  root_val = json_value_init_object();
  root_obj = json_value_get_object(root_val);

  for (i = 0; i < count; i++) {
    switch (pairs[i].type) {
    case C_REST_JSON_TYPE_STRING:
      json_object_set_string(root_obj, pairs[i].key, pairs[i].str_val);
      break;
    case C_REST_JSON_TYPE_NUMBER:
      json_object_set_number(root_obj, pairs[i].key, pairs[i].num_val);
      break;
    case C_REST_JSON_TYPE_BOOLEAN:
      json_object_set_boolean(root_obj, pairs[i].key, pairs[i].bool_val);
      break;
    case C_REST_JSON_TYPE_NULL:
      json_object_set_null(root_obj, pairs[i].key);
      break;
    }
  }

  rc = c_rest_response_json_obj(res, root_val);
  json_value_free(root_val);
  return rc;
}

#ifdef C_REST_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING
#include "c_rest_template.h"
/* clang-format on */

c_rest_error_t
c_rest_response_template(struct c_rest_response *res,
                         const struct c_rest_template_context *ctx,
                         const char **keys, const char **values, size_t count) {
  char *rendered = NULL;
  c_rest_error_t rc;
  if (!res || !ctx) {
    return C_REST_ERROR_GENERIC;
  }
  IGNORE_RC(c_rest_template_render(ctx, keys, values, count, &rendered));
  rc = c_rest_response_html(res, rendered);
  if (rendered) {
    C_REST_FREE(rendered);
  }
  return rc;
}
#endif /* C_REST_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING */

c_rest_error_t c_rest_response_html(struct c_rest_response *res,
                                    const char *html_str) {
  size_t len;
  c_rest_error_t rc;
  if (!res || !html_str) {
    return C_REST_ERROR_GENERIC;
  }

  len = strlen(html_str);

  rc = c_rest_response_set_header(res, "Content-Type", "text/html");

  if (rc != C_REST_OK)
    return rc;

  if (res->body) {
    C_REST_FREE((void *)(res->body));
  }
  if (C_REST_MALLOC(len + 1, &res->body) != 0) {
    LOG_DEBUG("C_REST_MALLOC failed");
    res->body = NULL;
  }
  if (!res->body) {
    return C_REST_ERROR_OOM;
  }
  SAFE_STRCPY(res->body, len + 1, html_str);
  res->body_len = len;

  return c_rest_response_send(res);
}

c_rest_error_t c_rest_response_write_chunk(struct c_rest_response *res,
                                           const char *chunk,
                                           size_t chunk_len) {
  struct c_rest_connection_context *ctx;
  c_rest_error_t rc;
  size_t written = 0;
  char hex_buf[32];
  size_t hex_len;

  if (!res || (!chunk && chunk_len > 0)) {
    return C_REST_ERROR_GENERIC;
  }

  if (!res->headers_sent) {
    IGNORE_RC(c_rest_response_set_header(res, "Transfer-Encoding", "chunked"));
    res->is_chunked = 1;
    IGNORE_RC(c_rest_response_send(res));
  }

  ctx = (struct c_rest_connection_context *)res->context;
  if (!ctx) {
    return C_REST_ERROR_GENERIC;
  }

  if (res->is_chunked) {
#if defined(_MSC_VER)
    hex_len =
        sprintf_s(hex_buf, sizeof(hex_buf), "%X\r\n", (unsigned int)chunk_len);
#else
    hex_len = (size_t)sprintf(hex_buf, "%X\r\n", (unsigned int)chunk_len);
#endif

    if (ctx->tls_conn) {
      rc = c_rest_tls_write(ctx->tls_conn, hex_buf, hex_len, &written);
      if (rc != C_REST_OK)
        return rc;
      if (chunk_len > 0) {
        rc = c_rest_tls_write(ctx->tls_conn, chunk, chunk_len, &written);
        if (rc != C_REST_OK)
          return rc;
      }
      rc = c_rest_tls_write(ctx->tls_conn, "\r\n", 2, &written);
      if (rc != C_REST_OK)
        return rc;
    } else {
#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
      if (ctx->cm_env) {
        rc = c_rest_socket_send((c_rest_socket_t)ctx->sock, hex_buf, hex_len,
                                &written);
        if (rc != C_REST_OK)
          return rc;
        if (chunk_len > 0) {
          rc = c_rest_socket_send((c_rest_socket_t)ctx->sock, chunk, chunk_len,
                                  &written);
          if (rc != C_REST_OK)
            return rc;
        }
        rc =
            c_rest_socket_send((c_rest_socket_t)ctx->sock, "\r\n", 2, &written);
        if (rc != C_REST_OK)
          return rc;
      } else {
        rc = c_rest_socket_send(ctx->sock, hex_buf, hex_len, &written);
        if (rc != C_REST_OK)
          return rc;
        if (chunk_len > 0) {
          rc = c_rest_socket_send(ctx->sock, chunk, chunk_len, &written);
          if (rc != C_REST_OK)
            return rc;
        }
        rc = c_rest_socket_send(ctx->sock, "\r\n", 2, &written);
        if (rc != C_REST_OK)
          return rc;
      }
#else
      rc = c_rest_socket_send(ctx->sock, hex_buf, hex_len, &written);
      if (rc != C_REST_OK)
        return rc;
      if (chunk_len > 0) {
        rc = c_rest_socket_send(ctx->sock, chunk, chunk_len, &written);
        if (rc != C_REST_OK)
          return rc;
      }
      rc = c_rest_socket_send(ctx->sock, "\r\n", 2, &written);
      if (rc != C_REST_OK)
        return rc;
#endif
    }
  } else {
    /* Not chunked HTTP/1.1, just stream raw bytes (used heavily by SSE) */
    if (chunk_len > 0) {
      if (ctx->tls_conn) {
        rc = c_rest_tls_write(ctx->tls_conn, chunk, chunk_len, &written);
        if (rc != C_REST_OK)
          return rc;
      } else {
#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
        if (ctx->cm_env) {
          rc = c_rest_socket_send((c_rest_socket_t)ctx->sock, chunk, chunk_len,
                                  &written);
          if (rc != C_REST_OK)
            return rc;
        } else {
          rc = c_rest_socket_send(ctx->sock, chunk, chunk_len, &written);
          if (rc != C_REST_OK)
            return rc;
        }
#else
        rc = c_rest_socket_send(ctx->sock, chunk, chunk_len, &written);
        if (rc != C_REST_OK)
          return rc;
#endif
      }
    }
  }

  return C_REST_OK;
}

c_rest_error_t c_rest_response_redirect(struct c_rest_response *res,
                                        const char *url, int status_code) {
  if (!res || !url) {
    return C_REST_ERROR_GENERIC;
  }
  if (status_code < 300 || status_code > 399) {
    status_code = 302; /* Default to temporary redirect */
  }
  IGNORE_RC(c_rest_response_set_status(res, status_code));
  IGNORE_RC(c_rest_response_set_header(res, "Location", url));
  return c_rest_response_send(res);
}

c_rest_error_t c_rest_response_set_cookie(struct c_rest_response *res,
                                          const char *key, const char *value,
                                          const char *attributes) {
  char *cookie_str;
  size_t len;
  c_rest_error_t rc;

  if (!res || !key || !value) {
    return C_REST_ERROR_GENERIC;
  }

  len = strlen(key) + strlen(value) + 2;
  /* key=value */
  if (attributes) {

    len += strlen(attributes) + 2;
    /* ; attributes */
  }

  if (C_REST_MALLOC(len, &cookie_str) != 0) {
    LOG_DEBUG("C_REST_MALLOC failed");
    cookie_str = NULL;
  }
  if (!cookie_str) {
    return C_REST_ERROR_GENERIC;
  }

#if defined(_MSC_VER)
  if (attributes) {
    sprintf_s(cookie_str, len, "%s=%s; %s", key, value, attributes);
  } else {
    sprintf_s(cookie_str, len, "%s=%s", key, value);
  }
#else
  if (attributes) {

#ifdef _MSC_VER
    /* CDD_SAFE_CRT */ sprintf_s(cookie_str, len, "%s=%s; %s", key, value,
                                 attributes);
#else
    /* CDD_SAFE_CRT */ sprintf(cookie_str, "%s=%s; %s", key, value, attributes);
#endif

  } else {

#ifdef _MSC_VER
    /* CDD_SAFE_CRT */ sprintf_s(cookie_str, len, "%s=%s", key, value);
#else
    /* CDD_SAFE_CRT */ sprintf(cookie_str, "%s=%s", key, value);
#endif
  }
#endif

  rc = c_rest_response_set_header(res, "Set-Cookie", cookie_str);
  if (rc != C_REST_OK) {
    C_REST_FREE((void *)(cookie_str));
    return rc;
  }
  C_REST_FREE((void *)(cookie_str));

  return C_REST_OK;
}

c_rest_error_t c_rest_response_send_file(struct c_rest_response *res,
                                         const char *filepath) {
  if (!res || !filepath) {
    return C_REST_ERROR_GENERIC;
  }
  /* Stub: read file and put into response or stream it. */
  return C_REST_OK;
}

c_rest_error_t c_rest_response_cleanup(struct c_rest_response *res) {
  struct c_rest_header *h;
  struct c_rest_header *next_h;

  if (!res) {
    return C_REST_ERROR_GENERIC;
  }

  h = res->headers;
  while (h) {
    next_h = h->next;
    C_REST_FREE((void *)(h->key));
    C_REST_FREE((void *)(h->value));
    C_REST_FREE((void *)(h));
    h = next_h;
  }
  res->headers = NULL;

  if (res->body) {
    C_REST_FREE((void *)(res->body));
    res->body = NULL;
  }
  return C_REST_OK;
}

static c_rest_error_t get_status_text(int status_code, const char **out_text) {
  switch (status_code) {
  case 200:
    *out_text = "OK";
    break;
  case 201:
    *out_text = "Created";
    break;
  case 202:
    *out_text = "Accepted";
    break;
  case 204:
    *out_text = "No Content";
    break;
  case 301:
    *out_text = "Moved Permanently";
    break;
  case 302:
    *out_text = "Found";
    break;
  case 304:
    *out_text = "Not Modified";
    break;
  case 400:
    *out_text = "Bad Request";
    break;
  case 401:
    *out_text = "Unauthorized";
    break;
  case 403:
    *out_text = "Forbidden";
    break;
  case 404:
    *out_text = "Not Found";
    break;
  case 405:
    *out_text = "Method Not Allowed";
    break;
  case 500:
    *out_text = "Internal Server Error";
    break;
  case 501:
    *out_text = "Not Implemented";
    break;
  default:
    *out_text = "Unknown";
    break;
  }
  return C_REST_OK;
}

c_rest_error_t c_rest_response_serialize(struct c_rest_response *res,
                                         char **out_buf, size_t *out_len) {
  size_t est_len = 128;
  /* initial estimate */
  char *buf;
  struct c_rest_header *h;
  size_t offset = 0;
  char cl_buf[32];
  const char *status_text;
  c_rest_error_t rc;

  if (!res || !out_buf || !out_len)
    return C_REST_ERROR_GENERIC;

  if (!res->is_chunked) {
    int found_cl = 0;
    for (h = res->headers; h != NULL; h = h->next) {
      int h_cmp;
      rc = c_rest_strcasecmp(h->key, "Content-Length", &h_cmp);
      if (rc != C_REST_OK)
        return rc;
      if (h_cmp == 0) {
        found_cl = 1;
        break;
      }
    }
    if (!found_cl) {
#if defined(_MSC_VER)
      sprintf_s(cl_buf, sizeof(cl_buf), C_REST_FMT_SIZE_T,
                CAST_SIZE_T(res->body_len));
#else
      sprintf(cl_buf, C_REST_FMT_SIZE_T, CAST_SIZE_T(res->body_len));
#endif
      rc = c_rest_response_set_header(res, "Content-Length", cl_buf);
      if (rc != C_REST_OK)
        return rc;
    }
  }

  for (h = res->headers; h != NULL; h = h->next) {
    est_len += strlen(h->key) + strlen(h->value) + 4;
  }
  est_len += res->body_len;

  if (C_REST_MALLOC(est_len, &buf) != 0) {
    LOG_DEBUG("C_REST_MALLOC failed");
    buf = NULL;
  }

  if (!buf)
    return C_REST_ERROR_GENERIC;

  IGNORE_RC(
      get_status_text(res->status_code ? res->status_code : 200, &status_text));

#if defined(_MSC_VER)
  offset +=
      (size_t)sprintf_s(buf + offset, est_len - offset, "HTTP/1.1 %d %s\r\n",
                        res->status_code ? res->status_code : 200, status_text);
#else
  offset +=
      (size_t)sprintf(buf + offset, "HTTP/1.1 %d %s\r\n",
                      res->status_code ? res->status_code : 200, status_text);
#endif

  for (h = res->headers; h != NULL; h = h->next) {
#if defined(_MSC_VER)
    offset += (size_t)sprintf_s(buf + offset, est_len - offset, "%s: %s\r\n",
                                h->key, h->value);
#else

#ifdef _MSC_VER
    /* CDD_SAFE_CRT */ offset += (size_t)sprintf_s(
        buf + offset, est_len - offset, "%s: %s\r\n", h->key, h->value);
#else
    /* CDD_SAFE_CRT */ offset +=
        (size_t)sprintf(buf + offset, "%s: %s\r\n", h->key, h->value);
#endif

#endif
  }

#if defined(_MSC_VER)
  offset += (size_t)sprintf_s(buf + offset, est_len - offset, "\r\n");
#else
  offset += (size_t)sprintf(buf + offset, "\r\n");
#endif

  if (res->body && res->body_len > 0) {

#ifdef _MSC_VER
    /* CDD_SAFE_CRT */ memcpy_s(buf + offset, est_len - offset, res->body,
                                res->body_len);
#else
    /* CDD_SAFE_CRT */ memcpy(buf + offset, res->body, res->body_len);
#endif

    offset += res->body_len;
  }

  *out_buf = buf;
  *out_len = offset;
  return C_REST_OK;
}

c_rest_error_t c_rest_response_oauth2_error(struct c_rest_response *res,
                                            const char *error,
                                            const char *error_description) {
  struct c_rest_json_pair pairs[2];
  if (!res || !error)
    return C_REST_ERROR_GENERIC;

  pairs[0].key = "error";
  pairs[0].type = C_REST_JSON_TYPE_STRING;
  pairs[0].str_val = error;

  if (error_description) {
    pairs[1].key = "error_description";
    pairs[1].type = C_REST_JSON_TYPE_STRING;
    pairs[1].str_val = error_description;
    IGNORE_RC(c_rest_response_set_status(res, 400));
    return c_rest_response_json_dict(res, pairs, 2);
  } else {
    IGNORE_RC(c_rest_response_set_status(res, 400));
    return c_rest_response_json_dict(res, pairs, 1);
  }
}
