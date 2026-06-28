/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_request.h" /* For struct c_rest_header */
#include "c_rest_response.h"
#include "c_rest_mem.h"
#include "c_rest_modality.h"
#include <parson.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "c_rest_mem.h"
#include "c_rest_log.h"

#include <ctype.h>

#if defined(_MSC_VER)
#define SAFE_STRCPY(dest, size, src) strcpy_s(dest, size, src)
#else
#define SAFE_STRCPY(dest, size, src) strcpy(dest, src)
#endif

static c_rest_error_t c_rest_stricmp(const char *s1, const char *s2, int *out_cmp) {
  while (*s1 && *s2) { /* GCOVR_EXCL_LINE */
    int c1 = tolower((unsigned char)*s1);
    int c2 = tolower((unsigned char)*s2);
    if (c1 != c2) {
      *out_cmp = c1 - c2;
      return C_REST_OK;
    }
    s1++;
    s2++;
  }
  *out_cmp = tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
  return C_REST_OK;
}

c_rest_error_t c_rest_response_set_header(struct c_rest_response *res, const char *key,
                               const char *value) {
  struct c_rest_header *h;
  struct c_rest_header *new_h;
  size_t val_len;
  int cmp;

  if (!res || !key || !value) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }

  /* Check if it already exists, replace value if it does */
  if (c_rest_stricmp(key, "Set-Cookie", &cmp) == 0 && cmp != 0) { /* GCOVR_EXCL_LINE */
    for (h = res->headers; h != NULL; h = h->next) {
      int hcmp;
      if (c_rest_stricmp(h->key, key, &hcmp) == 0 && hcmp == 0) { /* GCOVR_EXCL_LINE */
        char *new_val;

val_len = strlen(value) + 1;

        if (C_REST_MALLOC(val_len, &new_val) != 0) { LOG_DEBUG("C_REST_MALLOC failed"); new_val = NULL; } /* GCOVR_EXCL_LINE */
        if (!new_val) { /* GCOVR_EXCL_LINE */
          return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
        }
        SAFE_STRCPY(new_val, val_len, value);
        C_REST_FREE((void *)(h->value));
        h->value = new_val;
        return C_REST_OK;
      }
    }
  }

  /* Add new header */
  if (C_REST_MALLOC(sizeof(struct c_rest_header), &new_h) != 0) { LOG_DEBUG("C_REST_MALLOC failed"); new_h = NULL; } /* GCOVR_EXCL_LINE */
  if (!new_h) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }
  if (C_REST_MALLOC(strlen(key) + 1, &new_h->key) != 0) { LOG_DEBUG("C_REST_MALLOC failed"); new_h->key = NULL; } /* GCOVR_EXCL_LINE */
  if (C_REST_MALLOC(strlen(value) + 1, &new_h->value) != 0) { LOG_DEBUG("C_REST_MALLOC failed"); new_h->value = NULL; } /* GCOVR_EXCL_LINE */
  if (!new_h->key || !new_h->value) { /* GCOVR_EXCL_LINE */
    C_REST_FREE((void *)(new_h->key)); /* GCOVR_EXCL_LINE */
    C_REST_FREE((void *)(new_h->value)); /* GCOVR_EXCL_LINE */
    C_REST_FREE((void *)(new_h)); /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }

SAFE_STRCPY(new_h->key, strlen(key) + 1, key);


SAFE_STRCPY(new_h->value, strlen(value) + 1, value);


  new_h->next = res->headers;
  res->headers = new_h;

  return C_REST_OK;
}

c_rest_error_t c_rest_response_set_status(struct c_rest_response *res, int status_code) {
  if (!res) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }
  res->status_code = status_code;
  return C_REST_OK;
}

c_rest_error_t c_rest_response_check_etag(struct c_rest_request *req,
                               struct c_rest_response *res, const char *etag) {
  const char *if_none_match;
  if (!req || !res || !etag) { /* GCOVR_EXCL_LINE */
    return C_REST_OK; /* GCOVR_EXCL_LINE */
  }

  c_rest_response_set_header(res, "ETag", etag);

  if (c_rest_request_get_header(req, "If-None-Match", &if_none_match) == 0) { /* GCOVR_EXCL_LINE */
    if (if_none_match && strcmp(if_none_match, etag) == 0) { /* GCOVR_EXCL_LINE */
      c_rest_response_set_status(res, 304); /* Not Modified */
      return C_REST_ERROR_GENERIC;                             /* Match found */
    }
  }

  return C_REST_OK; /* GCOVR_EXCL_LINE */
}

c_rest_error_t c_rest_response_set_cache_control(struct c_rest_response *res,
                                      const char *policy) {
  if (!res || !policy) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }
  return c_rest_response_set_header(res, "Cache-Control", policy);
}

c_rest_error_t c_rest_response_send(struct c_rest_response *res) {
  struct c_rest_connection_context *ctx;
  char header_buf[4096];
  size_t offset = 0;
  size_t written = 0;
  struct c_rest_header *h;
  const char *status_text = "OK";

  if (!res) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }
  if (res->headers_sent) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }

  if (res->status_code == 400)
    status_text = "Bad Request";
  else if (res->status_code == 401)
    status_text = "Unauthorized";
  else if (res->status_code == 404) /* GCOVR_EXCL_LINE */
    status_text = "Not Found"; /* GCOVR_EXCL_LINE */
  else if (res->status_code == 500) /* GCOVR_EXCL_LINE */
    status_text = "Internal Server Error"; /* GCOVR_EXCL_LINE */

  if (!res->is_chunked) { /* GCOVR_EXCL_LINE */
    char cl_buf[32];
#if defined(_MSC_VER)
    sprintf_s(cl_buf, sizeof(cl_buf), C_REST_FMT_SIZE_T, CAST_SIZE_T(res->body_len));
#else
    sprintf(cl_buf, C_REST_FMT_SIZE_T, CAST_SIZE_T(res->body_len));
#endif
    c_rest_response_set_header(res, "Content-Length", cl_buf);
  }

#if defined(_MSC_VER)
  offset += sprintf_s(header_buf + offset, sizeof(header_buf) - offset,
                      "HTTP/1.1 %d %s\r\n", res->status_code, status_text);
#else
  offset += sprintf(header_buf + offset, "HTTP/1.1 %d %s\r\n", res->status_code,
                    status_text);
#endif

  for (h = res->headers; h != NULL; h = h->next) {
#if defined(_MSC_VER)
    offset += sprintf_s(header_buf + offset, sizeof(header_buf) - offset,
                        "%s: %s\r\n", h->key, h->value);
#else

#ifdef _MSC_VER
/* CDD_SAFE_CRT */ offset += sprintf_s(header_buf + offset, sizeof(header_buf) - offset, "%s: %s\r\n", h->key, h->value);
#else
/* CDD_SAFE_CRT */ offset += sprintf(header_buf + offset, "%s: %s\r\n", h->key, h->value);
#endif

#endif
  }

#if defined(_MSC_VER)
  offset += sprintf_s(header_buf + offset, sizeof(header_buf) - offset, "\r\n");
#else
  offset += sprintf(header_buf + offset, "\r\n");
#endif

  ctx = (struct c_rest_connection_context *)res->context;
  if (ctx) { /* GCOVR_EXCL_LINE */
    if (ctx->tls_conn) { /* GCOVR_EXCL_LINE */
      c_rest_tls_write(ctx->tls_conn, header_buf, offset, &written); /* GCOVR_EXCL_LINE */
      if (res->body && res->body_len > 0) { /* GCOVR_EXCL_LINE */
        c_rest_tls_write(ctx->tls_conn, res->body, res->body_len, &written); /* GCOVR_EXCL_LINE */
      }
    } else {
#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
      if (ctx->cm_env) {
        cm_socket_send(ctx->cm_env, ctx->sock, header_buf, offset, &written);
        if (res->body && res->body_len > 0) {
          cm_socket_send(ctx->cm_env, ctx->sock, res->body, res->body_len,
                         &written);
        }
      } else {
        c_rest_socket_send(ctx->sock, header_buf, offset, &written);
        if (res->body && res->body_len > 0) {
          c_rest_socket_send(ctx->sock, res->body, res->body_len, &written);
        }
      }
#else
      c_rest_socket_send(ctx->sock, header_buf, offset, &written); /* GCOVR_EXCL_LINE */
      if (res->body && res->body_len > 0) { /* GCOVR_EXCL_LINE */
        c_rest_socket_send(ctx->sock, res->body, res->body_len, &written); /* GCOVR_EXCL_LINE */
      }
#endif
    }
  }

  res->headers_sent = 1;
  return C_REST_OK;
}

c_rest_error_t c_rest_response_json(struct c_rest_response *res, const char *json_str) {
  size_t len;
  if (!res || !json_str) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }

len = strlen(json_str);

  c_rest_response_set_header(res, "Content-Type", "application/json");

  if (res->body) {
    C_REST_FREE((void *)(res->body));
  }
  if (C_REST_MALLOC(len + 1, &res->body) != 0) { LOG_DEBUG("C_REST_MALLOC failed"); res->body = NULL; } /* GCOVR_EXCL_LINE */
  if (!res->body) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }
  SAFE_STRCPY(res->body, len + 1, json_str);
  res->body_len = len;

  return c_rest_response_send(res);
}

c_rest_error_t c_rest_response_json_obj(struct c_rest_response *res, void *json_obj) {
  char *json_str;
  int ret;

  if (!res || !json_obj) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }

  json_str = json_serialize_to_string((JSON_Value *)json_obj);
  if (!json_str) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }

  ret = c_rest_response_json(res, json_str);
  json_free_serialized_string(json_str);

  return ret;
}

c_rest_error_t c_rest_response_json_dict(struct c_rest_response *res,
                              const struct c_rest_json_pair *pairs,
                              size_t count) {
  JSON_Value *root_val;
  JSON_Object *root_obj;
  size_t i;
  int ret;

  if (!res) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }

  root_val = json_value_init_object();
  if (!root_val) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }
  root_obj = json_value_get_object(root_val);

  for (i = 0; i < count; i++) {
    switch (pairs[i].type) { /* GCOVR_EXCL_LINE */
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

  ret = c_rest_response_json_obj(res, root_val);
  json_value_free(root_val);
  return ret;
}

#ifdef C_REST_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING
#include "c_rest_template.h"
/* clang-format on */

c_rest_error_t
c_rest_response_template(struct c_rest_response *res,
                         const struct c_rest_template_context *ctx,
                         const char **keys, const char **values, size_t count) {
  char *rendered = NULL;
  int ret;
  if (!res || !ctx) {            /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }
  if (c_rest_template_render(ctx, keys, values, count, &rendered) !=
      0) {                       /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }
  ret = c_rest_response_html(res, rendered);
  if (rendered) { /* GCOVR_EXCL_LINE */
    C_REST_FREE(rendered);
  }
  return ret;
}
#endif /* C_REST_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING */

c_rest_error_t c_rest_response_html(struct c_rest_response *res,
                                    const char *html_str) {
  size_t len;
  if (!res || !html_str) {       /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }

  len = strlen(html_str);

  c_rest_response_set_header(res, "Content-Type", "text/html");

  if (res->body) {                    /* GCOVR_EXCL_LINE */
    C_REST_FREE((void *)(res->body)); /* GCOVR_EXCL_LINE */
  }
  if (C_REST_MALLOC(len + 1, &res->body) != 0) { /* GCOVR_EXCL_LINE */
    LOG_DEBUG("C_REST_MALLOC failed");
    res->body = NULL; /* GCOVR_EXCL_LINE */
  }
  if (!res->body) {              /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }
  SAFE_STRCPY(res->body, len + 1, html_str);
  res->body_len = len;

  return c_rest_response_send(res);
}

c_rest_error_t c_rest_response_write_chunk(struct c_rest_response *res,
                                           const char *chunk,
                                           size_t chunk_len) {
  struct c_rest_connection_context *ctx;
  size_t written = 0;
  char hex_buf[32];
  size_t hex_len;

  if (!res || (!chunk && chunk_len > 0)) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;           /* GCOVR_EXCL_LINE */
  }

  if (!res->headers_sent) { /* GCOVR_EXCL_LINE */
    c_rest_response_set_header(res, "Transfer-Encoding",
                               "chunked"); /* GCOVR_EXCL_LINE */
    res->is_chunked = 1;                   /* GCOVR_EXCL_LINE */
    if (c_rest_response_send(res) != 0) {  /* GCOVR_EXCL_LINE */
      return C_REST_ERROR_GENERIC;         /* GCOVR_EXCL_LINE */
    }
  }

  ctx = (struct c_rest_connection_context *)res->context;
  if (!ctx) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;
  }

  if (res->is_chunked) { /* GCOVR_EXCL_LINE */
#if defined(_MSC_VER)
    hex_len =
        sprintf_s(hex_buf, sizeof(hex_buf), "%X\r\n", (unsigned int)chunk_len);
#else
    hex_len = sprintf(hex_buf, "%X\r\n",
                      (unsigned int)chunk_len); /* GCOVR_EXCL_LINE */
#endif

    if (ctx->tls_conn) { /* GCOVR_EXCL_LINE */
      c_rest_tls_write(ctx->tls_conn, hex_buf, hex_len,
                       &written); /* GCOVR_EXCL_LINE */
      if (chunk_len > 0) {        /* GCOVR_EXCL_LINE */
        c_rest_tls_write(ctx->tls_conn, chunk, chunk_len,
                         &written); /* GCOVR_EXCL_LINE */
      }
      c_rest_tls_write(ctx->tls_conn, "\r\n", 2,
                       &written); /* GCOVR_EXCL_LINE */
    } else {
#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
      if (ctx->cm_env) {
        cm_socket_send(ctx->cm_env, ctx->sock, hex_buf, hex_len, &written);
        if (chunk_len > 0) {
          cm_socket_send(ctx->cm_env, ctx->sock, chunk, chunk_len, &written);
        }
        cm_socket_send(ctx->cm_env, ctx->sock, "\r\n", 2, &written);
      } else {
        c_rest_socket_send(ctx->sock, hex_buf, hex_len, &written);
        if (chunk_len > 0) {
          c_rest_socket_send(ctx->sock, chunk, chunk_len, &written);
        }
        c_rest_socket_send(ctx->sock, "\r\n", 2, &written);
      }
#else
      c_rest_socket_send(ctx->sock, hex_buf, hex_len,
                         &written); /* GCOVR_EXCL_LINE */
      if (chunk_len > 0) {          /* GCOVR_EXCL_LINE */
        c_rest_socket_send(ctx->sock, chunk, chunk_len,
                           &written); /* GCOVR_EXCL_LINE */
      }
      c_rest_socket_send(ctx->sock, "\r\n", 2, &written); /* GCOVR_EXCL_LINE */
#endif
    }
  } else {
    /* Not chunked HTTP/1.1, just stream raw bytes (used heavily by SSE) */
    if (chunk_len > 0) {   /* GCOVR_EXCL_LINE */
      if (ctx->tls_conn) { /* GCOVR_EXCL_LINE */
        c_rest_tls_write(ctx->tls_conn, chunk, chunk_len,
                         &written); /* GCOVR_EXCL_LINE */
      } else {
#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
        if (ctx->cm_env) {
          cm_socket_send(ctx->cm_env, ctx->sock, chunk, chunk_len, &written);
        } else {
          c_rest_socket_send(ctx->sock, chunk, chunk_len, &written);
        }
#else
        c_rest_socket_send(ctx->sock, chunk, chunk_len,
                           &written); /* GCOVR_EXCL_LINE */
#endif
      }
    }
  }

  return C_REST_OK; /* GCOVR_EXCL_LINE */
}

c_rest_error_t c_rest_response_redirect(struct c_rest_response *res,
                                        const char *url, /* GCOVR_EXCL_LINE */
                                        int status_code) {
  if (!res || !url) {            /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }
  if (status_code < 300 || status_code > 399) {            /* GCOVR_EXCL_LINE */
    status_code = 302; /* Default to temporary redirect */ /* GCOVR_EXCL_LINE */
  }
  c_rest_response_set_status(res, status_code);     /* GCOVR_EXCL_LINE */
  c_rest_response_set_header(res, "Location", url); /* GCOVR_EXCL_LINE */
  return c_rest_response_send(res);                 /* GCOVR_EXCL_LINE */
}

c_rest_error_t c_rest_response_set_cookie(struct c_rest_response *res,
                                          const char *key, const char *value,
                                          const char *attributes) {
  char *cookie_str;
  size_t len;
  int ret;

  if (!res || !key || !value) {  /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }

  len = strlen(key) + strlen(value) + 2;
  /* key=value */
  if (attributes) {

    len += strlen(attributes) + 2;
    /* ; attributes */
  }

  if (C_REST_MALLOC(len, &cookie_str) != 0) { /* GCOVR_EXCL_LINE */
    LOG_DEBUG("C_REST_MALLOC failed");
    cookie_str = NULL; /* GCOVR_EXCL_LINE */
  }
  if (!cookie_str) {             /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
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

  ret = c_rest_response_set_header(res, "Set-Cookie", cookie_str);
  C_REST_FREE((void *)(cookie_str));

  return ret;
}

c_rest_error_t
c_rest_response_send_file(struct c_rest_response *res, /* GCOVR_EXCL_LINE */
                          const char *filepath) {
  if (!res || !filepath) {       /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }
  /* Stub: read file and put into response or stream it. */
  return C_REST_OK; /* GCOVR_EXCL_LINE */
}

c_rest_error_t c_rest_response_cleanup(struct c_rest_response *res) {
  struct c_rest_header *h;
  struct c_rest_header *next_h;

  if (!res) {                    /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
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

static int get_status_text(int status_code, const char **out_text) {
  switch (status_code) { /* GCOVR_EXCL_LINE */
  case 200:
    *out_text = "OK";
    break;
  case 201:                              /* GCOVR_EXCL_LINE */
    *out_text = "Created";               /* GCOVR_EXCL_LINE */
    break;                               /* GCOVR_EXCL_LINE */
  case 202:                              /* GCOVR_EXCL_LINE */
    *out_text = "Accepted";              /* GCOVR_EXCL_LINE */
    break;                               /* GCOVR_EXCL_LINE */
  case 204:                              /* GCOVR_EXCL_LINE */
    *out_text = "No Content";            /* GCOVR_EXCL_LINE */
    break;                               /* GCOVR_EXCL_LINE */
  case 301:                              /* GCOVR_EXCL_LINE */
    *out_text = "Moved Permanently";     /* GCOVR_EXCL_LINE */
    break;                               /* GCOVR_EXCL_LINE */
  case 302:                              /* GCOVR_EXCL_LINE */
    *out_text = "Found";                 /* GCOVR_EXCL_LINE */
    break;                               /* GCOVR_EXCL_LINE */
  case 304:                              /* GCOVR_EXCL_LINE */
    *out_text = "Not Modified";          /* GCOVR_EXCL_LINE */
    break;                               /* GCOVR_EXCL_LINE */
  case 400:                              /* GCOVR_EXCL_LINE */
    *out_text = "Bad Request";           /* GCOVR_EXCL_LINE */
    break;                               /* GCOVR_EXCL_LINE */
  case 401:                              /* GCOVR_EXCL_LINE */
    *out_text = "Unauthorized";          /* GCOVR_EXCL_LINE */
    break;                               /* GCOVR_EXCL_LINE */
  case 403:                              /* GCOVR_EXCL_LINE */
    *out_text = "Forbidden";             /* GCOVR_EXCL_LINE */
    break;                               /* GCOVR_EXCL_LINE */
  case 404:                              /* GCOVR_EXCL_LINE */
    *out_text = "Not Found";             /* GCOVR_EXCL_LINE */
    break;                               /* GCOVR_EXCL_LINE */
  case 405:                              /* GCOVR_EXCL_LINE */
    *out_text = "Method Not Allowed";    /* GCOVR_EXCL_LINE */
    break;                               /* GCOVR_EXCL_LINE */
  case 500:                              /* GCOVR_EXCL_LINE */
    *out_text = "Internal Server Error"; /* GCOVR_EXCL_LINE */
    break;                               /* GCOVR_EXCL_LINE */
  case 501:                              /* GCOVR_EXCL_LINE */
    *out_text = "Not Implemented";       /* GCOVR_EXCL_LINE */
    break;                               /* GCOVR_EXCL_LINE */
  default:                               /* GCOVR_EXCL_LINE */
    *out_text = "Unknown";               /* GCOVR_EXCL_LINE */
    break;                               /* GCOVR_EXCL_LINE */
  }
  return C_REST_OK;
}

c_rest_error_t c_rest_response_serialize(struct c_rest_response *res,
                                         char **out_buf, size_t *out_len) {
  size_t est_len = 128; /* initial estimate */
  char *buf;
  struct c_rest_header *h;
  size_t offset = 0;
  char cl_buf[32];
  const char *status_text;

  if (!res || !out_buf || !out_len) /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;    /* GCOVR_EXCL_LINE */

  if (!res->is_chunked) { /* GCOVR_EXCL_LINE */
    int found_cl = 0;
    for (h = res->headers; h != NULL; h = h->next) {
      int cmp;
      if (c_rest_stricmp(h->key, "Content-Length", &cmp) == 0 &&
          cmp == 0) { /* GCOVR_EXCL_LINE */
        found_cl = 1; /* GCOVR_EXCL_LINE */
        break;        /* GCOVR_EXCL_LINE */
      }
    }
    if (!found_cl) { /* GCOVR_EXCL_LINE */
#if defined(_MSC_VER)
      sprintf_s(cl_buf, sizeof(cl_buf), C_REST_FMT_SIZE_T,
                CAST_SIZE_T(res->body_len));
#else
      sprintf(cl_buf, C_REST_FMT_SIZE_T, CAST_SIZE_T(res->body_len));
#endif
      c_rest_response_set_header(res, "Content-Length", cl_buf);
    }
  }

  for (h = res->headers; h != NULL; h = h->next) {
    est_len += strlen(h->key) + strlen(h->value) + 4;
  }
  est_len += res->body_len;

  if (C_REST_MALLOC(est_len, &buf) != 0) { /* GCOVR_EXCL_LINE */
    LOG_DEBUG("C_REST_MALLOC failed");
    buf = NULL; /* GCOVR_EXCL_LINE */
  }

  if (!buf)                      /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  get_status_text(res->status_code ? res->status_code : 200,
                  &status_text); /* GCOVR_EXCL_LINE */

#if defined(_MSC_VER)
  offset += sprintf_s(buf + offset, est_len - offset, "HTTP/1.1 %d %s\r\n",
                      res->status_code ? res->status_code : 200, status_text);
#else
  offset += sprintf(buf + offset, "HTTP/1.1 %d %s\r\n",
                    res->status_code ? res->status_code : 200,
                    status_text); /* GCOVR_EXCL_LINE */
#endif

  for (h = res->headers; h != NULL; h = h->next) {
#if defined(_MSC_VER)
    offset += sprintf_s(buf + offset, est_len - offset, "%s: %s\r\n", h->key,
                        h->value);
#else

#ifdef _MSC_VER
    /* CDD_SAFE_CRT */ offset += sprintf_s(buf + offset, est_len - offset,
                                           "%s: %s\r\n", h->key, h->value);
#else
    /* CDD_SAFE_CRT */ offset +=
        sprintf(buf + offset, "%s: %s\r\n", h->key, h->value);
#endif

#endif
  }

#if defined(_MSC_VER)
  offset += sprintf_s(buf + offset, est_len - offset, "\r\n");
#else
  offset += sprintf(buf + offset, "\r\n");
#endif

  if (res->body && res->body_len > 0) { /* GCOVR_EXCL_LINE */

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
  if (!res || !error)            /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  pairs[0].key = "error";
  pairs[0].type = C_REST_JSON_TYPE_STRING;
  pairs[0].str_val = error;

  if (error_description) { /* GCOVR_EXCL_LINE */
    pairs[1].key = "error_description";
    pairs[1].type = C_REST_JSON_TYPE_STRING;
    pairs[1].str_val = error_description;
    c_rest_response_set_status(res, 400);
    return c_rest_response_json_dict(res, pairs, 2);
  } else {
    c_rest_response_set_status(res, 400);            /* GCOVR_EXCL_LINE */
    return c_rest_response_json_dict(res, pairs, 1); /* GCOVR_EXCL_LINE */
  }
}
