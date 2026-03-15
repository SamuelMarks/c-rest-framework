/* clang-format off */
#ifdef C_REST_FRAMEWORK_USE_REAL_CAH
#include <c_abstract_http/c_abstract_http.h>
#if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)
#include <c_abstract_http/http_winhttp.h>
#else
#include <c_abstract_http/http_curl.h>
#endif
#else
#include "c_abstract_http.h"
#endif
#include "c_rest_client.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* clang-format on */

struct c_rest_client_context {
  struct HttpClient client;
};

static enum HttpMethod method_from_str(const char *method_str) {
  if (!method_str)
    return HTTP_GET;
  if (strcmp(method_str, "POST") == 0)
    return HTTP_POST;
  if (strcmp(method_str, "PUT") == 0)
    return HTTP_PUT;
  if (strcmp(method_str, "DELETE") == 0)
    return HTTP_DELETE;
  if (strcmp(method_str, "PATCH") == 0)
    return HTTP_PATCH;
  if (strcmp(method_str, "HEAD") == 0)
    return HTTP_HEAD;
  if (strcmp(method_str, "OPTIONS") == 0)
    return HTTP_OPTIONS;
  if (strcmp(method_str, "TRACE") == 0)
    return HTTP_TRACE;
  if (strcmp(method_str, "CONNECT") == 0)
    return HTTP_CONNECT;
  return HTTP_GET;
}

#ifndef C_REST_FRAMEWORK_USE_REAL_CAH
static int mock_send(struct HttpTransportContext *ctx,
                     const struct HttpRequest *req, struct HttpResponse **res) {
  (void)ctx;
  (void)req;
  if (res)
    *res = NULL;
  return 0;
}
#endif

int c_rest_client_init(c_rest_client_context **out_client) {
  struct c_rest_client_context *ctx;
  if (!out_client)
    return 1;

  ctx = (struct c_rest_client_context *)malloc(
      sizeof(struct c_rest_client_context));
  if (!ctx)
    return 1;

  if (http_client_init(&ctx->client) != 0) {
    free(ctx);
    return 1;
  }

#ifdef C_REST_FRAMEWORK_USE_REAL_CAH
#if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)
  if (http_winhttp_context_init(
          (struct HttpTransportContext **)&ctx->client.transport) != 0) {
    http_client_free(&ctx->client);
    free(ctx);
    return 1;
  }
  ctx->client.send = http_winhttp_send;
#else
  if (http_curl_context_init(
          (struct HttpTransportContext **)&ctx->client.transport, NULL) != 0) {
    http_client_free(&ctx->client);
    free(ctx);
    return 1;
  }
  ctx->client.send = http_curl_send;
#endif
#else
  ctx->client.transport = (struct HttpTransportContext *)1;
  ctx->client.send = (http_send_fn)mock_send;
#endif

  *out_client = ctx;
  return 0;
}

int c_rest_client_destroy(c_rest_client_context *client) {
  if (!client)
    return 1;

#ifdef C_REST_FRAMEWORK_USE_REAL_CAH
#if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)
  http_winhttp_context_free(client->client.transport);
#else
  http_curl_context_free(client->client.transport);
#endif
#endif

  http_client_free(&client->client);
  free(client);
  return 0;
}

int c_rest_client_response_free(struct c_rest_client_response *res) {
  size_t i;
  if (!res)
    return 1;
  if (res->headers) {
    for (i = 0; i < res->headers_count; ++i) {
      free((void *)res->headers[i].key);
      free((void *)res->headers[i].value);
    }
    free(res->headers);
  }
  if (res->body) {
    free(res->body);
  }
  free(res);
  return 0;
}

int c_rest_client_request_sync(c_rest_client_context *client, const char *url,
                               const char *method,
                               const struct c_rest_client_header *headers,
                               size_t headers_count, const void *body,
                               size_t body_len,
                               struct c_rest_client_response **out_res) {
  struct HttpRequest req;
  struct HttpResponse *res = NULL;
  int rc;
  size_t i;
  struct c_rest_client_response *out = NULL;

  if (!client || !client->client.transport || !url || !method)
    return 1;

  if (http_request_init(&req) != 0)
    return 1;

  req.url = (char *)url;
  req.method = method_from_str(method);

  for (i = 0; i < headers_count; ++i) {
    http_headers_add(&req.headers, headers[i].key, headers[i].value);
  }

  if (body && body_len > 0) {
    req.body = (void *)body;
    req.body_len = body_len;
  }

  rc = client->client.send(client->client.transport, &req, &res);

  if (out_res && res) {
    out = (struct c_rest_client_response *)malloc(
        sizeof(struct c_rest_client_response));
    if (out) {
      out->status_code = res->status_code;
      out->headers_count = res->headers.count;
      out->headers = NULL;
      if (out->headers_count > 0) {
        out->headers = (struct c_rest_client_header *)calloc(
            out->headers_count, sizeof(struct c_rest_client_header));
        if (out->headers) {
          for (i = 0; i < out->headers_count; ++i) {
            if (res->headers.headers[i].key) {
              size_t klen = strlen(res->headers.headers[i].key) + 1;
              out->headers[i].key = (char *)malloc(klen);
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
              out->headers[i].value = (char *)malloc(vlen);
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
        out->body = malloc(out->body_len);
        if (out->body) {
          memcpy(out->body, res->body, out->body_len);
        }
      }
      *out_res = out;
    }
  }

  if (res) {
    http_response_free(res);
    free(res);
  }

  req.url = NULL;
  req.body = NULL;
  http_request_free(&req);

  return rc;
}

int c_rest_client_request_async(
    c_rest_client_context *client, const char *url, const char *method,
    const struct c_rest_client_header *headers, size_t headers_count,
    const void *body, size_t body_len,
    void (*callback)(struct c_rest_client_response *res, void *data),
    void *user_data) {
  int res;
  struct c_rest_client_response *out = NULL;
  if (!client || !client->client.transport || !url || !method)
    return 1;

  res = c_rest_client_request_sync(client, url, method, headers, headers_count,
                                   body, body_len, &out);
  if (callback) {
    callback(out, user_data);
  }
  if (out) {
    c_rest_client_response_free(out);
  }
  return res;
}

static char hex_digit(int v) {
  if (v >= 0 && v < 10)
    return (char)('0' + v);
  return (char)('A' + (v - 10));
}

int c_rest_client_url_encode(const char *in_str, char **out_str) {
  size_t len;
  size_t i;
  size_t j = 0;
  char *out;
  if (!in_str || !out_str)
    return 1;

  len = strlen(in_str);
  out = (char *)malloc(len * 3 + 1);
  if (!out)
    return 1;

  for (i = 0; i < len; ++i) {
    unsigned char c = (unsigned char)in_str[i];
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      out[j++] = c;
    } else if (c == ' ') {
      out[j++] = '+';
    } else {
      out[j++] = '%';
      out[j++] = hex_digit(c >> 4);
      out[j++] = hex_digit(c & 15);
    }
  }
  out[j] = '\0';
  *out_str = out;
  return 0;
}

static int from_hex(char c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  return -1;
}

int c_rest_client_url_decode(const char *in_str, char **out_str) {
  size_t len;
  size_t i;
  size_t j = 0;
  char *out;
  if (!in_str || !out_str)
    return 1;

  len = strlen(in_str);
  out = (char *)malloc(len + 1);
  if (!out)
    return 1;

  for (i = 0; i < len; ++i) {
    if (in_str[i] == '+') {
      out[j++] = ' ';
    } else if (in_str[i] == '%' && i + 2 < len) {
      int h1 = from_hex(in_str[i + 1]);
      int h2 = from_hex(in_str[i + 2]);
      if (h1 >= 0 && h2 >= 0) {
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
  return 0;
}

int c_rest_client_build_form_urlencoded(
    const struct c_rest_client_form_field *fields, size_t num_fields,
    char **out_body, size_t *out_len) {
  size_t i;
  size_t total_len = 0;
  char *buf = NULL;
  size_t curr_pos = 0;

  if (!out_body || !out_len)
    return 1;

  *out_body = NULL;
  *out_len = 0;

  if (num_fields == 0 || !fields)
    return 0;

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
      free(ekey);
    if (eval)
      free(eval);
  }

  buf = (char *)malloc(total_len + 1);
  if (!buf)
    return 1;

  for (i = 0; i < num_fields; ++i) {
    char *ekey = NULL;
    char *eval = NULL;

    if (fields[i].key) {
      c_rest_client_url_encode(fields[i].key, &ekey);
    }
    if (fields[i].value) {
      c_rest_client_url_encode(fields[i].value, &eval);
    }

    if (i > 0) {
      buf[curr_pos++] = '&';
    }
    if (ekey) {
      size_t len = strlen(ekey);
      memcpy(buf + curr_pos, ekey, len);
      curr_pos += len;
      free(ekey);
    }
    buf[curr_pos++] = '=';
    if (eval) {
      size_t len = strlen(eval);
      memcpy(buf + curr_pos, eval, len);
      curr_pos += len;
      free(eval);
    }
  }

  buf[curr_pos] = '\0';
  *out_body = buf;
  *out_len = curr_pos;

  return 0;
}

int c_rest_proxy_request(const char *target_url, void *req, void *res) {
  /* Simple proxy stub */
  c_rest_client_context *client;
  int ret;
  struct c_rest_client_response *c_res = NULL;
  (void)req; /* unused in mock */
  (void)res; /* unused in mock */

  if (!target_url)
    return 1;

  if (c_rest_client_init(&client) != 0) {
    return 1;
  }

  ret = c_rest_client_request_sync(client, target_url, "GET", NULL, 0, NULL, 0,
                                   &c_res);
  if (c_res) {
    c_rest_client_response_free(c_res);
  }
  c_rest_client_destroy(client);

  return ret;
}
