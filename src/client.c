/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"
#ifdef C_REST_FRAMEWORK_USE_REAL_CAH
#include <c_abstract_http/c_abstract_http.h>
#if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)
#if defined(_MSC_VER) && _MSC_VER < 1600
#include <c_abstract_http/http_wininet.h>
#else
#include <c_abstract_http/http_winhttp.h>
#endif
#elif defined(__APPLE__)
#include <c_abstract_http/http_apple.h>
#else
#if !defined(CDD_DOS) && !defined(__EMSCRIPTEN__)
#include <c_abstract_http/http_curl.h>
#endif
#endif
#else
#include "c_abstract_http.h"
#endif
#include "c_rest_client.h"
#include "c_rest_base64.h"
#include <parson.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include "c_rest_log.h"
#include <string.h>
/* clang-format on */

struct c_rest_client_context {
  struct HttpClient client;
};

static c_rest_error_t method_from_str(const char *method_str,
                                      enum HttpMethod *out_method) {
  if (strcmp(method_str, "POST") == 0) {
    *out_method = HTTP_POST;
    return C_REST_OK;
  }
  if (strcmp(method_str, "PUT") == 0) {
    *out_method = HTTP_PUT;
    return C_REST_OK;
  }
  if (strcmp(method_str, "DELETE") == 0) {
    *out_method = HTTP_DELETE;
    return C_REST_OK;
  }
  if (strcmp(method_str, "PATCH") == 0) {
    *out_method = HTTP_PATCH;
    return C_REST_OK;
  }
  if (strcmp(method_str, "HEAD") == 0) {
    *out_method = HTTP_HEAD;
    return C_REST_OK;
  }
  if (strcmp(method_str, "OPTIONS") == 0) {
    *out_method = HTTP_OPTIONS;
    return C_REST_OK;
  }
  if (strcmp(method_str, "TRACE") == 0) {
    *out_method = HTTP_TRACE;
    return C_REST_OK;
  }
  if (strcmp(method_str, "CONNECT") == 0) {
    *out_method = HTTP_CONNECT;
    return C_REST_OK;
  }
  *out_method = HTTP_GET;
  return C_REST_OK;
}

#ifndef C_REST_FRAMEWORK_USE_REAL_CAH
static int mock_send(struct HttpTransportContext *ctx,
                     const struct HttpRequest *req, struct HttpResponse **res) {
  (void)ctx;
  (void)req;
  if (res)
    *res = NULL;
  return C_REST_OK;
}
#endif

c_rest_error_t c_rest_client_init(c_rest_client_context **out_client) {
  struct c_rest_client_context *ctx;
  if (!out_client)
    return C_REST_ERROR_GENERIC;

  ctx = (struct c_rest_client_context *)CRF_MALLOC(
      sizeof(struct c_rest_client_context));
  if (!ctx)
    return C_REST_ERROR_GENERIC;

  (void)!http_client_init(&ctx->client);

#ifdef C_REST_FRAMEWORK_USE_REAL_CAH
#if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)
#if defined(_MSC_VER) && _MSC_VER < 1600
  (void)!http_wininet_context_init(
      (struct HttpTransportContext **)&ctx->client.transport);
  ctx->client.send = http_wininet_send;
#else
  (void)!http_winhttp_context_init(
      (struct HttpTransportContext **)&ctx->client.transport);
  ctx->client.send = http_winhttp_send;
#endif
#elif defined(__APPLE__)
  (void)!http_apple_context_init(
      (struct HttpTransportContext **)&ctx->client.transport);
  ctx->client.send = http_apple_send;
#else
#if !defined(CDD_DOS) && !defined(__EMSCRIPTEN__)
  (void)!http_curl_context_init(
      (struct HttpTransportContext **)&ctx->client.transport);
  ctx->client.send = http_curl_send;
#else
  (void)ctx;
  return C_REST_ERROR_GENERIC;
#endif
#endif
#else
  ctx->client.transport = (struct HttpTransportContext *)1;
  ctx->client.send = (http_send_fn)mock_send;
#endif

  *out_client = ctx;
  return C_REST_OK;
}

c_rest_error_t c_rest_client_destroy(c_rest_client_context *client) {
  if (!client)
    return C_REST_ERROR_GENERIC;

#ifdef C_REST_FRAMEWORK_USE_REAL_CAH
#if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)
#if defined(_MSC_VER) && _MSC_VER < 1600
  http_wininet_context_free(client->client.transport);
#else
  http_winhttp_context_free(client->client.transport);
#endif
#elif defined(__APPLE__)
  http_apple_context_free(client->client.transport);
#else
#if !defined(CDD_DOS) && !defined(__EMSCRIPTEN__)
  http_curl_context_free(client->client.transport);
#endif
#endif
#endif

  http_client_free(&client->client);
  C_REST_FREE((void *)(client));
  return C_REST_OK;
}

c_rest_error_t c_rest_client_response_free(struct c_rest_client_response *res) {
  size_t i;
  if (!res)
    return C_REST_ERROR_GENERIC;
  if (res->headers) {
    for (i = 0; i < res->headers_count; ++i) {
      C_REST_FREE((void *)(res->headers[i].key));
      C_REST_FREE((void *)(res->headers[i].value));
    }
    C_REST_FREE((void *)(res->headers));
  }
  if (res->body) {
    C_REST_FREE((void *)(res->body));
  }
  C_REST_FREE((void *)(res));
  return C_REST_OK;
}

c_rest_error_t c_rest_client_request_sync(
    c_rest_client_context *client, const char *url, const char *method,
    const struct c_rest_client_header *headers, size_t headers_count,
    const void *body, size_t body_len,
    struct c_rest_client_response **out_res) {
  struct HttpRequest req;
  struct HttpResponse *res = NULL;
  int rc;
  struct c_rest_client_response *out = NULL;
  size_t i;

  if (!client || !url || !method)
    return C_REST_ERROR_GENERIC;

  (void)!http_request_init(&req);

  req.url = (char *)url;
  (void)!method_from_str(method, &req.method);

  for (i = 0; i < headers_count; ++i) {
    http_headers_add(&req.headers, headers[i].key, headers[i].value);
  }

  if (body && body_len > 0) {
    req.body = (void *)body;
    req.body_len = body_len;
  }

  if (!client->client.send)
    return C_REST_ERROR_GENERIC;

  rc = client->client.send(client->client.transport, &req, &res);

  if (out_res && res) {
    out = (struct c_rest_client_response *)CRF_MALLOC(
        sizeof(struct c_rest_client_response));
    if (out) {
      out->status_code = res->status_code;
      out->headers_count = res->headers.count;
      out->headers = NULL;
      if (out->headers_count > 0) {
        out->headers = (struct c_rest_client_header *)CRF_CALLOC(
            out->headers_count, sizeof(struct c_rest_client_header));
        if (out->headers) {
          for (i = 0; i < out->headers_count; ++i) {
            if (res->headers.headers[i].key) {
              size_t klen = strlen(res->headers.headers[i].key) + 1;
              out->headers[i].key = (char *)CRF_MALLOC(klen);
              if (out->headers[i].key) {
#if defined(_MSC_VER)
                strcpy_s((char *)out->headers[i].key, klen,
                         res->headers.headers[i].key);
#else
                strcpy((char *)out->headers[i].key,
                       res->headers.headers[i].key);
#endif
              }
            }
            if (res->headers.headers[i].value) {
              size_t vlen = strlen(res->headers.headers[i].value) + 1;
              out->headers[i].value = (char *)CRF_MALLOC(vlen);
              if (out->headers[i].value) {
#if defined(_MSC_VER)
                strcpy_s((char *)out->headers[i].value, vlen,
                         res->headers.headers[i].value);
#else
                strcpy((char *)out->headers[i].value,
                       res->headers.headers[i].value);
#endif
              }
            }
          }
        }
      }
      out->body_len = res->body_len;
      out->body = NULL;
      if (out->body_len > 0 && res->body) {
        out->body = CRF_MALLOC(out->body_len);
        if (out->body) {
#if defined(_MSC_VER)
          /* CDD_SAFE_CRT */ memcpy_s(out->body, out->body_len, res->body,
                                      out->body_len);
#else
          memcpy(out->body, res->body, out->body_len);
#endif
        }
      }
      *out_res = out;
    }
  }

  if (res) {
    http_response_free(res);
    C_REST_FREE((void *)(res));
  }

  req.url = NULL;
  req.body = NULL;
  http_request_free(&req);

  return rc;
}

c_rest_error_t c_rest_client_request_async(
    c_rest_client_context *client, const char *url, const char *method,
    const struct c_rest_client_header *headers, size_t headers_count,
    const void *body, size_t body_len,
    void (*callback)(struct c_rest_client_response *res, void *data),
    void *user_data) {
  int res;
  struct c_rest_client_response *out = NULL;
  if (!client || !client->client.transport || !url || !method)
    return C_REST_ERROR_GENERIC;

  res = c_rest_client_request_sync(client, url, method, headers, headers_count,
                                   body, body_len, &out);
  if (callback) {
    callback(out, user_data);
  }
  if (out) {
    (void)!c_rest_client_response_free(out);
  }
  return res;
}

static c_rest_error_t hex_digit(int v, char *out_char) {
  if (v < 10)
    *out_char = (char)('0' + v);
  else
    *out_char = (char)('A' + (v - 10));
  return C_REST_OK;
}

c_rest_error_t c_rest_client_url_encode(const char *in_str, char **out_str) {
  size_t len;
  size_t i;
  size_t j = 0;
  char *out;
  if (!in_str || !out_str)
    return C_REST_ERROR_GENERIC;

  len = strlen(in_str);
  out = (char *)CRF_MALLOC(len * 3 + 1);
  if (!out)
    return C_REST_ERROR_GENERIC;

  for (i = 0; i < len; ++i) {
    unsigned char c = (unsigned char)in_str[i];
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      out[j++] = c;
    } else if (c == ' ') {
      out[j++] = '+';
    } else {
      char hd1 = 0, hd2 = 0;
      (void)!hex_digit(c >> 4, &hd1);
      (void)!hex_digit(c & 15, &hd2);
      out[j++] = '%';
      out[j++] = hd1;
      out[j++] = hd2;
    }
  }
  out[j] = '\0';
  *out_str = out;
  return C_REST_OK;
}

static c_rest_error_t from_hex(char c, int *out_val) {
  if (c >= '0' && c <= '9') {
    *out_val = c - '0';
    return C_REST_OK;
  }
  if (c >= 'A' && c <= 'F') {
    *out_val = c - 'A' + 10;
    return C_REST_OK;
  }
  if (c >= 'a' && c <= 'f') {
    *out_val = c - 'a' + 10;
    return C_REST_OK;
  }
  return C_REST_ERROR_GENERIC;
}

c_rest_error_t c_rest_client_url_decode(const char *in_str, char **out_str) {
  size_t len;
  size_t i;
  size_t j = 0;
  char *out;
  if (!in_str || !out_str)
    return C_REST_ERROR_GENERIC;

  len = strlen(in_str);
  out = (char *)CRF_MALLOC(len + 1);
  if (!out)
    return C_REST_ERROR_GENERIC;

  for (i = 0; i < len; ++i) {
    if (in_str[i] == '+') {
      out[j++] = ' ';
    } else if (in_str[i] == '%' && i + 2 < len) {
      int h1, h2;
      if (from_hex(in_str[i + 1], &h1) == C_REST_OK &&
          from_hex(in_str[i + 2], &h2) == C_REST_OK) {
        out[j++] = (char)((h1 << 4) | h2);
        i += 2;
      } else {
        out[j++] = in_str[i];
      }
    } else {
      out[j++] = in_str[i];
    }
  }
  out[j] = '\0';
  *out_str = out;
  return C_REST_OK;
}

c_rest_error_t c_rest_client_build_form_urlencoded(
    const struct c_rest_client_form_field *fields, size_t num_fields,
    char **out_body, size_t *out_len) {
  size_t i;
  size_t total_len = 0;
  char *buf = NULL;
  size_t curr_pos = 0;

  if (!out_body || !out_len)
    return C_REST_ERROR_GENERIC;

  *out_body = NULL;
  *out_len = 0;

  if (num_fields == 0 || !fields)
    return C_REST_OK;

  for (i = 0; i < num_fields; ++i) {
    char *ekey = NULL;
    char *eval = NULL;
    size_t key_len = 0;
    size_t val_len = 0;

    if (fields[i].key) {
      if (c_rest_client_url_encode(fields[i].key, &ekey) == 0) {
        key_len = strlen(ekey);
      }
    }
    if (fields[i].value) {
      if (c_rest_client_url_encode(fields[i].value, &eval) == 0) {
        val_len = strlen(eval);
      }
    }

    if (i > 0) {
      total_len += 1; /* '&' */
    }
    total_len += key_len;
    total_len += 1; /* '=' */
    total_len += val_len;

    if (ekey)
      C_REST_FREE((void *)(ekey));
    if (eval)
      C_REST_FREE((void *)(eval));
  }

  buf = (char *)CRF_MALLOC(total_len + 1);
  if (!buf)
    return C_REST_ERROR_GENERIC;

  for (i = 0; i < num_fields; ++i) {
    char *ekey = NULL;
    char *eval = NULL;

    if (fields[i].key) {
      (void)!c_rest_client_url_encode(fields[i].key, &ekey);
    }
    if (fields[i].value) {
      (void)!c_rest_client_url_encode(fields[i].value, &eval);
    }

    if (i > 0) {
      buf[curr_pos++] = '&';
    }
    if (ekey) {
      size_t len = strlen(ekey);
#if defined(_MSC_VER)
      /* CDD_SAFE_CRT */ memcpy_s(buf + curr_pos, len, ekey, len);
#else
      memcpy(buf + curr_pos, ekey, len);
#endif
      curr_pos += len;
      C_REST_FREE((void *)(ekey));
    }
    buf[curr_pos++] = '=';
    if (eval) {
      size_t len = strlen(eval);
#if defined(_MSC_VER)
      /* CDD_SAFE_CRT */ memcpy_s(buf + curr_pos, len, eval, len);
#else
      memcpy(buf + curr_pos, eval, len);
#endif
      curr_pos += len;
      C_REST_FREE((void *)(eval));
    }
  }

  buf[curr_pos] = '\0';
  *out_body = buf;
  *out_len = curr_pos;

  return C_REST_OK;
}

c_rest_error_t c_rest_proxy_request(const char *target_url, void *req,
                                    void *res) {
  c_rest_error_t rc;
  /* Simple proxy stub */
  c_rest_client_context *client;
  struct c_rest_client_response *c_res = NULL;
  (void)req; /* unused in mock */
  (void)res; /* unused in mock */

  if (!target_url)
    return C_REST_ERROR_GENERIC;

  rc = c_rest_client_init(&client);
  if (rc != C_REST_OK) {
    return rc;
  }

  rc = c_rest_client_request_sync(client, target_url, "GET", NULL, 0, NULL, 0,
                                  &c_res);
  if (c_res) {
    (void)!c_rest_client_response_free(c_res);
  }
  (void)!c_rest_client_destroy(client);

  return rc;
}

c_rest_error_t c_rest_client_parse_form_urlencoded(
    const char *body, struct c_rest_client_form_field **out_fields,
    size_t *out_num_fields) {
  const char *p;
  size_t count = 0;
  struct c_rest_client_form_field *fields = NULL;
  size_t i;

  if (!body || !out_fields || !out_num_fields)
    return C_REST_ERROR_GENERIC;

  p = body;
  while (*p) {
    if (*p == '&') {
      count++;
    }
    p++;
  }
  count++;

  fields = (struct c_rest_client_form_field *)CRF_CALLOC(
      count, sizeof(struct c_rest_client_form_field));
  if (!fields)
    return C_REST_ERROR_GENERIC;

  p = body;
  for (i = 0; i < count; ++i) {
    const char *eq = strchr(p, '=');
    const char *amp = strchr(p, '&');
    size_t key_len, val_len;
    char *ekey = NULL, *eval = NULL;

    if (!amp) {
      amp = p + strlen(p);
    }

    if (eq && eq < amp) {
      key_len = (size_t)(eq - p);
      val_len = (size_t)(amp - eq - 1);

      ekey = (char *)CRF_MALLOC(key_len + 1);
      if (ekey) {
#if defined(_MSC_VER)
        /* CDD_SAFE_CRT */ memcpy_s(ekey, key_len, p, key_len);
#else
        memcpy(ekey, p, key_len);
#endif
        ekey[key_len] = '\0';
      }

      eval = (char *)CRF_MALLOC(val_len + 1);
      if (eval) {
#if defined(_MSC_VER)
        /* CDD_SAFE_CRT */ memcpy_s(eval, val_len, eq + 1, val_len);
#else
        memcpy(eval, eq + 1, val_len);
#endif
        eval[val_len] = '\0';
      }
    } else {
      key_len = (size_t)(amp - p);
      ekey = (char *)CRF_MALLOC(key_len + 1);
      if (ekey) {
#if defined(_MSC_VER)
        /* CDD_SAFE_CRT */ memcpy_s(ekey, key_len, p, key_len);
#else
        memcpy(ekey, p, key_len);
#endif
        ekey[key_len] = '\0';
      }
      eval = (char *)CRF_MALLOC(1);
      if (eval) {
        eval[0] = '\0';
      }
    }

    if (ekey) {
      (void)!c_rest_client_url_decode(ekey, (char **)&fields[i].key);
      C_REST_FREE((void *)(ekey));
    }
    if (eval) {
      (void)!c_rest_client_url_decode(eval, (char **)&fields[i].value);
      C_REST_FREE((void *)(eval));
    }

    if (*amp != '\0') {
      p = amp + 1;
    }
  }

  *out_fields = fields;
  *out_num_fields = count;
  return C_REST_OK;
}

c_rest_error_t
c_rest_client_form_fields_free(struct c_rest_client_form_field *fields,
                               size_t num_fields) {
  size_t i;
  if (!fields)
    return C_REST_ERROR_GENERIC;
  for (i = 0; i < num_fields; ++i) {
    if (fields[i].key)
      C_REST_FREE((void *)(fields[i].key));
    if (fields[i].value)
      C_REST_FREE((void *)(fields[i].value));
  }
  C_REST_FREE((void *)(fields));
  return C_REST_OK;
}

c_rest_error_t c_rest_client_header_set(struct c_rest_client_header **headers,
                                        size_t *headers_count, const char *key,
                                        const char *value) {
  struct c_rest_client_header *new_headers;
  size_t klen, vlen;
  if (!headers || !headers_count || !key || !value)
    return C_REST_ERROR_GENERIC;

  new_headers = (struct c_rest_client_header *)CRF_REALLOC(
      *headers, sizeof(struct c_rest_client_header) * (*headers_count + 1));
  if (!new_headers)
    return C_REST_ERROR_GENERIC;

  *headers = new_headers;

  klen = strlen(key) + 1;
  vlen = strlen(value) + 1;

  (*headers)[*headers_count].key = (char *)CRF_MALLOC(klen);
  if (!(*headers)[*headers_count].key)
    return C_REST_ERROR_GENERIC;
#if defined(_MSC_VER)
  strcpy_s((char *)(*headers)[*headers_count].key, klen, key);
#else
  strcpy((char *)(*headers)[*headers_count].key, key);
#endif

  (*headers)[*headers_count].value = (char *)CRF_MALLOC(vlen);
  if (!(*headers)[*headers_count].value) {
    C_REST_FREE((void *)((*headers)[*headers_count].key));
    return C_REST_ERROR_GENERIC;
  }
#if defined(_MSC_VER)
  strcpy_s((char *)(*headers)[*headers_count].value, vlen, value);
#else
  strcpy((char *)(*headers)[*headers_count].value, value);
#endif

  (*headers_count)++;
  return C_REST_OK;
}

c_rest_error_t c_rest_client_headers_free(struct c_rest_client_header *headers,
                                          size_t headers_count) {
  size_t i;
  if (!headers)
    return C_REST_ERROR_GENERIC;
  for (i = 0; i < headers_count; ++i) {
    if (headers[i].key)
      C_REST_FREE((void *)(headers[i].key));
    if (headers[i].value)
      C_REST_FREE((void *)(headers[i].value));
  }
  C_REST_FREE((void *)(headers));
  return C_REST_OK;
}

c_rest_error_t c_rest_client_build_auth_basic(const char *username,
                                              const char *password,
                                              char **out_header) {
  char *concat;
  size_t ulen, plen;
  size_t clen;
  char *b64;
  size_t b64_len;
  size_t hlen;

  if (!username || !password || !out_header)
    return C_REST_ERROR_GENERIC;

  ulen = strlen(username);
  plen = strlen(password);
  clen = ulen + plen + 1;

  concat = (char *)CRF_MALLOC(clen + 1);
  if (!concat)
    return C_REST_ERROR_GENERIC;

#if defined(_MSC_VER)
  sprintf_s(concat, clen + 1, "%s:%s", username, password);
#else
  sprintf(concat, "%s:%s", username, password);
#endif

  b64_len = 0;
  (void)!c_rest_base64_encode((unsigned char *)concat, clen, NULL, &b64_len);

  b64 = (char *)CRF_MALLOC(b64_len + 1);
  if (!b64) {
    C_REST_FREE((void *)(concat));
    return C_REST_ERROR_OOM;
  }

  (void)!c_rest_base64_encode((unsigned char *)concat, clen, b64, &b64_len);
  b64[b64_len] = '\0';
  C_REST_FREE((void *)(concat));

  hlen = 6 + b64_len + 1; /* "Basic " + b64 + null */
  *out_header = (char *)CRF_MALLOC(hlen);
  if (!*out_header) {
    C_REST_FREE((void *)(b64));
    return C_REST_ERROR_GENERIC;
  }

#if defined(_MSC_VER)
  sprintf_s(*out_header, hlen, "Basic %s", b64);
#else
  sprintf(*out_header, "Basic %s", b64);
#endif

  C_REST_FREE((void *)(b64));
  return C_REST_OK;
}

c_rest_error_t c_rest_client_build_auth_bearer(const char *token,
                                               char **out_header) {
  size_t hlen;

  if (!token || !out_header)
    return C_REST_ERROR_GENERIC;

  hlen = 7 + strlen(token) + 1; /* "Bearer " + token + null */
  *out_header = (char *)CRF_MALLOC(hlen);
  if (!*out_header)
    return C_REST_ERROR_GENERIC;

#if defined(_MSC_VER)
  sprintf_s(*out_header, hlen, "Bearer %s", token);
#else
  sprintf(*out_header, "Bearer %s", token);
#endif

  return C_REST_OK;
}

c_rest_error_t c_rest_client_post_form_sync(
    c_rest_client_context *client, const char *url,
    const struct c_rest_client_header *headers, size_t headers_count,
    const struct c_rest_client_form_field *fields, size_t num_fields,
    struct c_rest_client_response **out_res) {
  int ret;
  char *body = NULL;
  size_t body_len = 0;
  struct c_rest_client_header *all_headers = NULL;
  size_t all_headers_count = 0;
  size_t i;

  if (!client || !url)
    return C_REST_ERROR_GENERIC;

  ret =
      c_rest_client_build_form_urlencoded(fields, num_fields, &body, &body_len);
  if (ret != 0)
    return ret;

  if (headers_count > 0 && headers) {
    for (i = 0; i < headers_count; ++i) {
      (void)!c_rest_client_header_set(&all_headers, &all_headers_count,
                                      headers[i].key, headers[i].value);
    }
  }

  /* Add content type */
  (void)!c_rest_client_header_set(&all_headers, &all_headers_count,
                                  "Content-Type",
                                  "application/x-www-form-urlencoded");

  ret = c_rest_client_request_sync(client, url, "POST", all_headers,
                                   all_headers_count, body, body_len, out_res);

  (void)!c_rest_client_headers_free(all_headers, all_headers_count);
  if (body)
    C_REST_FREE((void *)(body));

  return ret;
}

c_rest_error_t
c_rest_client_response_parse_json(const struct c_rest_client_response *res,
                                  void **out_json) {
  char *str;
  size_t i;
  if (!res || !out_json)
    return C_REST_ERROR_GENERIC;
  *out_json = NULL;
  if (!res->body || res->body_len == 0)
    return C_REST_ERROR_GENERIC;

  str = (char *)CRF_MALLOC(res->body_len + 1);
  if (!str)
    return C_REST_ERROR_GENERIC;

  for (i = 0; i < res->body_len; ++i) {
    str[i] = ((const char *)res->body)[i];
  }
  str[res->body_len] = '\0';

  *out_json = (void *)json_parse_string(str);
  C_REST_FREE((void *)(str));

  if (!*out_json)
    return C_REST_ERROR_GENERIC;

  return C_REST_OK;
}
