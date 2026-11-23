/* clang-format off */
#include "c_rest_testing_mocks.h"
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
#elif defined(__APPLE__) && defined(__clang__)
#include <c_abstract_http/http_apple.h>
#elif defined(__EMSCRIPTEN__)
#include <c_abstract_http/http_wasm.h>
#else
#if !defined(CDD_DOS) && !defined(__EMSCRIPTEN__)
#include <c_abstract_http/http_curl.h>
#endif
#endif
#else
#include "c_abstract_http.h"
#endif
#include "c_rest_client.h"
#include "c_rest_client_hooks.h"
#include "c_rest_base64.h"
#include <parson.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include "c_rest_log.h"
#include <string.h>
/* clang-format on */

c_rest_error_t
c_rest_client_set_hooks(const struct c_rest_client_hooks *hooks) {
  (void)hooks;
  return C_REST_OK;
}

struct c_rest_client_context {
  struct HttpClient client;
};

static NO_DISCARD c_rest_error_t method_from_str(const char *method_str,
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
  if (strcmp(method_str, "GET") == 0) {
    *out_method = HTTP_GET;
    return C_REST_OK;
  }
  return C_REST_ERROR_GENERIC;
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

static NO_DISCARD c_rest_error_t cah_to_rest_error(int err) {
  return c_rest_error_from_cah(err);
}

c_rest_error_t c_rest_client_init(c_rest_client_context **out_client) {
  struct c_rest_client_context *ctx;
  enum c_abstract_http_error init_err;
#if defined(C_REST_FRAMEWORK_USE_REAL_CAH) && !defined(CDD_DOS)
  enum c_abstract_http_error tr_err;
#endif
  if (!out_client)
    return C_REST_ERROR_GENERIC;

  ctx = NULL;
  if (C_REST_MALLOC(sizeof(struct c_rest_client_context), &ctx) != C_REST_OK)
    return C_REST_ERROR_GENERIC;

#ifdef C_REST_TESTING_MALLOC_HOOK
  init_err = (crf_seam_client_check(1) != C_REST_OK)
                 ? C_ABSTRACT_HTTP_ERR_NOMEM
                 : http_client_init(&ctx->client);
#else
  init_err = http_client_init(&ctx->client);
#endif
  if (init_err != C_ABSTRACT_HTTP_SUCCESS) {
    C_REST_FREE(ctx);
    return cah_to_rest_error(init_err);
  }

#ifdef C_REST_FRAMEWORK_USE_REAL_CAH
#if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)
#if defined(_MSC_VER) && _MSC_VER < 1600
#ifdef C_REST_TESTING_MALLOC_HOOK
  tr_err = (crf_seam_client_check(2) != C_REST_OK)
               ? C_ABSTRACT_HTTP_ERR_NOMEM
               : http_wininet_context_init(
                     (struct HttpTransportContext **)&ctx->client.transport);
#else
  tr_err = http_wininet_context_init(
      (struct HttpTransportContext **)&ctx->client.transport);
#endif
  if (tr_err != C_ABSTRACT_HTTP_SUCCESS) {
    C_REST_FREE(ctx);
    return cah_to_rest_error(tr_err);
  }
  ctx->client.send = http_wininet_send;
#else
#ifdef C_REST_TESTING_MALLOC_HOOK
  tr_err = (crf_seam_client_check(2) != C_REST_OK)
               ? C_ABSTRACT_HTTP_ERR_NOMEM
               : http_winhttp_context_init(
                     (struct HttpTransportContext **)&ctx->client.transport);
#else
  tr_err = http_winhttp_context_init(
      (struct HttpTransportContext **)&ctx->client.transport);
#endif
  if (tr_err != C_ABSTRACT_HTTP_SUCCESS) {
    C_REST_FREE(ctx);
    return cah_to_rest_error(tr_err);
  }
  ctx->client.send = http_winhttp_send;
#endif
#elif defined(__APPLE__) && defined(__clang__)
#ifdef C_REST_TESTING_MALLOC_HOOK
  tr_err = (crf_seam_client_check(2) != C_REST_OK)
               ? C_ABSTRACT_HTTP_ERR_NOMEM
               : http_apple_context_init(
                     (struct HttpTransportContext **)&ctx->client.transport);
#else
  tr_err = http_apple_context_init(
      (struct HttpTransportContext **)&ctx->client.transport);
#endif
  if (tr_err != C_ABSTRACT_HTTP_SUCCESS) {
    C_REST_FREE(ctx);
    return cah_to_rest_error(tr_err);
  }
  ctx->client.send = http_apple_send;
#elif defined(__EMSCRIPTEN__)
  tr_err = http_wasm_context_init(
      (struct HttpTransportContext **)&ctx->client.transport);
  if (tr_err != C_ABSTRACT_HTTP_SUCCESS) {
    C_REST_FREE(ctx);
    return cah_to_rest_error(tr_err);
  }
  ctx->client.send = http_wasm_send;
#else
#if !defined(CDD_DOS) && !defined(__EMSCRIPTEN__)
#ifdef C_REST_TESTING_MALLOC_HOOK
  tr_err = (crf_seam_client_check(2) != C_REST_OK)
               ? C_ABSTRACT_HTTP_ERR_NOMEM
               : http_curl_context_init(
                     (struct HttpTransportContext **)&ctx->client.transport);
#else
  tr_err = http_curl_context_init(
      (struct HttpTransportContext **)&ctx->client.transport);
#endif
  if (tr_err != C_ABSTRACT_HTTP_SUCCESS) {
    C_REST_FREE(ctx);
    return cah_to_rest_error(tr_err);
  }
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
  enum c_abstract_http_error cah_rc = C_ABSTRACT_HTTP_SUCCESS;
  enum c_abstract_http_error cah_rc2 = C_ABSTRACT_HTTP_SUCCESS;
#ifdef C_REST_TESTING_MALLOC_HOOK
  c_rest_error_t seam_rc;
#endif
  if (!client)
    return C_REST_ERROR_GENERIC;

#ifdef C_REST_TESTING_MALLOC_HOOK
  seam_rc = crf_seam_client_destroy_hook();
  if (seam_rc != C_REST_OK)
    return seam_rc;
#endif

#ifdef C_REST_FRAMEWORK_USE_REAL_CAH
#if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)
#if defined(_MSC_VER) && _MSC_VER < 1600
#ifdef C_REST_TESTING_MALLOC_HOOK
  cah_rc = (crf_seam_client_check(14) != C_REST_OK)
               ? C_ABSTRACT_HTTP_ERR_INVAL
               : http_wininet_context_free(client->client.transport);
#else
  cah_rc = http_wininet_context_free(client->client.transport);
#endif
  if (cah_rc != C_ABSTRACT_HTTP_SUCCESS) {
    return cah_to_rest_error(cah_rc);
  }
#else
#ifdef C_REST_TESTING_MALLOC_HOOK
  cah_rc = (crf_seam_client_check(14) != C_REST_OK)
               ? C_ABSTRACT_HTTP_ERR_INVAL
               : http_winhttp_context_free(client->client.transport);
#else
  cah_rc = http_winhttp_context_free(client->client.transport);
#endif
  if (cah_rc != C_ABSTRACT_HTTP_SUCCESS) {
    return cah_to_rest_error(cah_rc);
  }
#endif
#elif defined(__APPLE__) && defined(__clang__)
#ifdef C_REST_TESTING_MALLOC_HOOK
  cah_rc = (crf_seam_client_check(14) != C_REST_OK)
               ? C_ABSTRACT_HTTP_ERR_INVAL
               : http_apple_context_free(client->client.transport);
#else
  cah_rc = http_apple_context_free(client->client.transport);
#endif
  if (cah_rc != C_ABSTRACT_HTTP_SUCCESS) {
    return cah_to_rest_error(cah_rc);
  }
#elif defined(__EMSCRIPTEN__)
  cah_rc = http_wasm_context_free(client->client.transport);
  if (cah_rc != C_ABSTRACT_HTTP_SUCCESS) {
    return cah_to_rest_error(cah_rc);
  }
#else
#if !defined(CDD_DOS) && !defined(__EMSCRIPTEN__)
#ifdef C_REST_TESTING_MALLOC_HOOK
  cah_rc = (crf_seam_client_check(14) != C_REST_OK)
               ? C_ABSTRACT_HTTP_ERR_INVAL
               : http_curl_context_free(client->client.transport);
#else
  cah_rc = http_curl_context_free(client->client.transport);
#endif
  if (cah_rc != C_ABSTRACT_HTTP_SUCCESS) {
    return cah_to_rest_error(cah_rc);
  }
#endif
#endif
#endif

#ifdef C_REST_TESTING_MALLOC_HOOK
  cah_rc2 = (crf_seam_client_check(15) != C_REST_OK)
                ? C_ABSTRACT_HTTP_ERR_INVAL
                : http_client_free(&client->client);
#else
  cah_rc2 = http_client_free(&client->client);
#endif
  if (cah_rc2 != C_ABSTRACT_HTTP_SUCCESS) {
    return cah_to_rest_error(cah_rc2);
  }
  C_REST_FREE((void *)(client));
  return C_REST_OK;
}

c_rest_error_t c_rest_client_response_free(struct c_rest_client_response *res) {
  size_t i;
#ifdef C_REST_TESTING_MALLOC_HOOK
  c_rest_error_t seam_rc;
#endif
  if (!res)
    return C_REST_ERROR_GENERIC;
#ifdef C_REST_TESTING_MALLOC_HOOK
  seam_rc = crf_seam_client_res_free_hook();
  if (seam_rc != C_REST_OK)
    return seam_rc;
#endif
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
  enum c_abstract_http_error cah_rc = C_ABSTRACT_HTTP_SUCCESS;
  enum c_abstract_http_error cah_rc_req = C_ABSTRACT_HTTP_SUCCESS;
  struct HttpRequest req;
  struct HttpResponse *res = NULL;
  c_rest_error_t rc;
  struct c_rest_client_response *out = NULL;
  size_t i;
  enum c_abstract_http_error req_err;

  if (!client || !url || !method)
    return C_REST_ERROR_GENERIC;

#ifdef C_REST_TESTING_MALLOC_HOOK
  req_err = (crf_seam_client_check(12) != C_REST_OK) ? C_ABSTRACT_HTTP_ERR_INVAL
                                                     : http_request_init(&req);
#else
  req_err = http_request_init(&req);
#endif
  if (req_err != C_ABSTRACT_HTTP_SUCCESS) {
    return cah_to_rest_error(req_err);
  }

  req.url = (char *)url;
  rc = method_from_str(method, &req.method);
  if (rc != C_REST_OK)
    return rc;

  for (i = 0; i < headers_count; ++i) {
    enum c_abstract_http_error hdr_err =
        http_headers_add(&req.headers, headers[i].key, headers[i].value);
    if (hdr_err != C_ABSTRACT_HTTP_SUCCESS) {
      return cah_to_rest_error(hdr_err);
    }
  }

  if (body && body_len > 0) {
    req.body = (void *)body;
    req.body_len = body_len;
  }

  if (!client->client.send)
    return C_REST_ERROR_GENERIC;

  rc =
      (c_rest_error_t)client->client.send(client->client.transport, &req, &res);
  if (rc != C_REST_OK) {
    if (!res)
      goto sync_cleanup;
    goto copy_sync_res;
  }

copy_sync_res:
  if (out_res && res) {
    out = NULL;
    if (C_REST_MALLOC(sizeof(struct c_rest_client_response), &out) ==
        C_REST_OK) {
      out->status_code = res->status_code;
      out->headers_count = res->headers.count;
      out->headers = NULL;
      if (out->headers_count > 0) {
        if (C_REST_CALLOC(out->headers_count,
                          sizeof(struct c_rest_client_header),
                          &out->headers) == C_REST_OK) {
          for (i = 0; i < out->headers_count; ++i) {
            if (res->headers.headers[i].key) {
              size_t klen = strlen(res->headers.headers[i].key) + 1;
              if (C_REST_MALLOC(klen, &out->headers[i].key) == C_REST_OK) {
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
              if (C_REST_MALLOC(vlen, &out->headers[i].value) == C_REST_OK) {
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
        if (C_REST_MALLOC(out->body_len, &out->body) == C_REST_OK) {
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

sync_cleanup:
  if (res) {
#ifdef C_REST_TESTING_MALLOC_HOOK
    cah_rc = (crf_seam_client_check(16) != C_REST_OK)
                 ? C_ABSTRACT_HTTP_ERR_INVAL
                 : http_response_free(res);
#else
    cah_rc = http_response_free(res);
#endif
    if (cah_rc != C_ABSTRACT_HTTP_SUCCESS) {
      C_REST_FREE((void *)(res));
      return cah_to_rest_error(cah_rc);
    }
    C_REST_FREE((void *)(res));
  }

  req.url = NULL;
  req.body = NULL;
#ifdef C_REST_TESTING_MALLOC_HOOK
  cah_rc_req = (crf_seam_client_check(17) != C_REST_OK)
                   ? C_ABSTRACT_HTTP_ERR_INVAL
                   : http_request_free(&req);
#else
  cah_rc_req = http_request_free(&req);
#endif
  if (cah_rc_req != C_ABSTRACT_HTTP_SUCCESS) {
    return cah_to_rest_error(cah_rc_req);
  }

  return rc;
}

c_rest_error_t c_rest_client_request_async(
    c_rest_client_context *client, const char *url, const char *method,
    const struct c_rest_client_header *headers, size_t headers_count,
    const void *body, size_t body_len,
    void (*callback)(struct c_rest_client_response *res, void *data),
    void *user_data) {
  c_rest_error_t res;
  struct c_rest_client_response *out = NULL;
  if (!client || !client->client.transport || !url || !method)
    return C_REST_ERROR_GENERIC;

  res = c_rest_client_request_sync(client, url, method, headers, headers_count,
                                   body, body_len, &out);
  if (res != C_REST_OK)
    goto async_finish;

async_finish:
  if (callback) {
    callback(out, user_data);
  }
  if (out) {
    c_rest_error_t free_rc = c_rest_client_response_free(out);
    if (free_rc != C_REST_OK)
      return free_rc;
  }
  return res;
}

static NO_DISCARD c_rest_error_t hex_digit(int v, char *out_char) {
#ifdef C_REST_TESTING_MALLOC_HOOK
  c_rest_error_t seam_rc = crf_seam_client_hex_step();
  if (seam_rc != C_REST_OK)
    return seam_rc;
#endif
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
  c_rest_error_t rc;
  if (!in_str || !out_str)
    return C_REST_ERROR_GENERIC;

  len = strlen(in_str);
  out = NULL;
  if (C_REST_MALLOC(len * 3 + 1, &out) != C_REST_OK)
    return C_REST_ERROR_GENERIC;

  for (i = 0; i < len; ++i) {
    unsigned char c = (unsigned char)in_str[i];
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      out[j++] = (char)c;
    } else if (c == ' ') {
      out[j++] = '+';
    } else {
      char hd1 = 0, hd2 = 0;
      rc = hex_digit(c >> 4, &hd1);
      if (rc != C_REST_OK) {
        C_REST_FREE(out);
        return rc;
      }
      rc = hex_digit(c & 15, &hd2);
      if (rc != C_REST_OK) {
        C_REST_FREE(out);
        return rc;
      }
      out[j++] = '%';
      out[j++] = hd1;
      out[j++] = hd2;
    }
  }
  out[j] = '\0';
  *out_str = out;
  return C_REST_OK;
}

static NO_DISCARD c_rest_error_t from_hex(char c, int *out_val) {
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
#ifdef C_REST_TESTING_MALLOC_HOOK
  c_rest_error_t seam_rc = crf_seam_client_url_decode_hook();
  if (seam_rc != C_REST_OK)
    return seam_rc;
#endif
  if (!in_str || !out_str)
    return C_REST_ERROR_GENERIC;

  len = strlen(in_str);
  out = NULL;
  if (C_REST_MALLOC(len + 1, &out) != C_REST_OK)
    return C_REST_ERROR_GENERIC;

  for (i = 0; i < len; ++i) {
    if (in_str[i] == '+') {
      out[j++] = ' ';
    } else if (in_str[i] == '%' && i + 2 < len) {
      int h1 = 0, h2 = 0;
      c_rest_error_t hex_rc = from_hex(in_str[i + 1], &h1);
      if (hex_rc != C_REST_OK) {
        out[j++] = in_str[i];
        goto decode_next_char;
      }
      hex_rc = from_hex(in_str[i + 2], &h2);
      if (hex_rc != C_REST_OK) {
        out[j++] = in_str[i];
        goto decode_next_char;
      }
      out[j++] = (char)((h1 << 4) | h2);
      i += 2;
    } else {
      out[j++] = in_str[i];
    }
  decode_next_char:;
  }
  out[j] = '\0';
  *out_str = out;
  {
    c_rest_error_t hex_rc = C_REST_OK;
    return hex_rc;
  }
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
    c_rest_error_t enc_rc;

    if (fields[i].key) {
      enc_rc = c_rest_client_url_encode(fields[i].key, &ekey);
      if (enc_rc != C_REST_OK) {
        return enc_rc;
      }
      key_len = strlen(ekey);
    }
    if (fields[i].value) {
      enc_rc = c_rest_client_url_encode(fields[i].value, &eval);
      if (enc_rc != C_REST_OK) {
        if (ekey)
          C_REST_FREE((void *)(ekey));
        return enc_rc;
      }
      val_len = strlen(eval);
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

  buf = NULL;
  if (C_REST_MALLOC(total_len + 1, &buf) != C_REST_OK)
    return C_REST_ERROR_GENERIC;

  for (i = 0; i < num_fields; ++i) {
    char *ekey = NULL;
    char *eval = NULL;
    c_rest_error_t rc;

    if (fields[i].key) {
      rc = c_rest_client_url_encode(fields[i].key, &ekey);
      if (rc != C_REST_OK) {
        C_REST_FREE(buf);
        return rc;
      }
    }
    if (fields[i].value) {
      rc = c_rest_client_url_encode(fields[i].value, &eval);
      if (rc != C_REST_OK) {
        C_REST_FREE(ekey);
        C_REST_FREE(buf);
        return rc;
      }
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
  if (rc != C_REST_OK) {
    c_res = NULL;
    goto proxy_cleanup;
  }

proxy_cleanup:;
  {
    c_rest_error_t free_rc = C_REST_OK;
#ifdef C_REST_TESTING_MALLOC_HOOK
    free_rc = crf_seam_client_res_free_hook();
    if (free_rc != C_REST_OK) {
      static struct c_rest_client_response s_dummy_res;
      c_res = &s_dummy_res;
      goto proxy_do_free;
    }
  proxy_do_free:;
#endif
    if (c_res) {
      free_rc = c_rest_client_response_free(c_res);
      if (free_rc != C_REST_OK) {
        c_rest_error_t destroy_rc = c_rest_client_destroy(client);
        if (destroy_rc != C_REST_OK)
          return destroy_rc;
        return free_rc;
      }
    }
  }
  {
    c_rest_error_t destroy_rc = c_rest_client_destroy(client);
    if (destroy_rc != C_REST_OK)
      return destroy_rc;
  }

  return rc;
}

c_rest_error_t c_rest_client_parse_form_urlencoded(
    const char *body, struct c_rest_client_form_field **out_fields,
    size_t *out_num_fields) {
  const char *p;
  size_t count = 0;
  struct c_rest_client_form_field *fields = NULL;
  size_t i;
  c_rest_error_t rc;

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

  if (C_REST_CALLOC(count, sizeof(struct c_rest_client_form_field), &fields) !=
      C_REST_OK)
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

      if (C_REST_MALLOC(key_len + 1, &ekey) == C_REST_OK) {
#if defined(_MSC_VER)
        /* CDD_SAFE_CRT */ memcpy_s(ekey, key_len, p, key_len);
#else
        memcpy(ekey, p, key_len);
#endif
        ekey[key_len] = '\0';
      }

      if (C_REST_MALLOC(val_len + 1, &eval) == C_REST_OK) {
#if defined(_MSC_VER)
        /* CDD_SAFE_CRT */ memcpy_s(eval, val_len, eq + 1, val_len);
#else
        memcpy(eval, eq + 1, val_len);
#endif
        eval[val_len] = '\0';
      }
    } else {
      key_len = (size_t)(amp - p);
      if (C_REST_MALLOC(key_len + 1, &ekey) == C_REST_OK) {
#if defined(_MSC_VER)
        /* CDD_SAFE_CRT */ memcpy_s(ekey, key_len, p, key_len);
#else
        memcpy(ekey, p, key_len);
#endif
        ekey[key_len] = '\0';
      }
      if (C_REST_MALLOC(1, &eval) == C_REST_OK) {
        eval[0] = '\0';
      }
    }

    if (ekey) {
      rc = c_rest_client_url_decode(ekey, (char **)&fields[i].key);
      if (rc != C_REST_OK) {
        c_rest_error_t free_rc;
        C_REST_FREE((void *)(ekey));
        if (eval)
          C_REST_FREE((void *)(eval));
        free_rc = c_rest_client_form_fields_free(fields, count);
        if (free_rc != C_REST_OK)
          return free_rc;
        return rc;
      }
      C_REST_FREE((void *)(ekey));
    }
    if (eval) {
      rc = c_rest_client_url_decode(eval, (char **)&fields[i].value);
      if (rc != C_REST_OK) {
        c_rest_error_t free_rc;
        C_REST_FREE((void *)(eval));
        free_rc = c_rest_client_form_fields_free(fields, count);
        if (free_rc != C_REST_OK)
          return free_rc;
        return rc;
      }
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
#ifdef C_REST_TESTING_MALLOC_HOOK
  c_rest_error_t seam_rc = crf_seam_client_form_free_hook();
  if (seam_rc != C_REST_OK)
    return seam_rc;
#endif
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
#ifdef C_REST_TESTING_MALLOC_HOOK
  c_rest_error_t seam_rc = crf_seam_client_check(42);
  if (seam_rc != C_REST_OK)
    return seam_rc;
#endif
  if (!headers || !headers_count || !key || !value)
    return C_REST_ERROR_GENERIC;

  new_headers = NULL;
  if (C_REST_REALLOC(*headers,
                     sizeof(struct c_rest_client_header) * (*headers_count + 1),
                     &new_headers) != C_REST_OK)
    return C_REST_ERROR_GENERIC;

  *headers = new_headers;

  klen = strlen(key) + 1;
  vlen = strlen(value) + 1;

  (*headers)[*headers_count].key = NULL;
  if (C_REST_MALLOC(klen, &(*headers)[*headers_count].key) != C_REST_OK)
    return C_REST_ERROR_GENERIC;
#if defined(_MSC_VER)
  strcpy_s((char *)(*headers)[*headers_count].key, klen, key);
#else
  strcpy((char *)(*headers)[*headers_count].key, key);
#endif

  (*headers)[*headers_count].value = NULL;
  if (C_REST_MALLOC(vlen, &(*headers)[*headers_count].value) != C_REST_OK) {
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
#ifdef C_REST_TESTING_MALLOC_HOOK
  c_rest_error_t seam_rc = crf_seam_client_headers_free_hook();
  if (seam_rc != C_REST_OK)
    return seam_rc;
#endif
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
  c_rest_error_t rc;

  if (!username || !password || !out_header)
    return C_REST_ERROR_GENERIC;

  ulen = strlen(username);
  plen = strlen(password);
  clen = ulen + plen + 1;

  concat = NULL;
  if (C_REST_MALLOC(clen + 1, &concat) != C_REST_OK)
    return C_REST_ERROR_GENERIC;

#if defined(_MSC_VER)
  sprintf_s(concat, clen + 1, "%s:%s", username, password);
#else
  sprintf(concat, "%s:%s", username, password);
#endif

  b64_len = 0;
#ifdef C_REST_TESTING_MALLOC_HOOK
  rc = crf_seam_client_check(30);
  if (rc != C_REST_OK)
    goto b64_first_check;
#endif
  rc = c_rest_base64_encode((unsigned char *)concat, clen, NULL, &b64_len);
#ifdef C_REST_TESTING_MALLOC_HOOK
b64_first_check:
#endif
  if (rc != C_REST_OK) {
    C_REST_FREE((void *)(concat));
    return rc;
  }

  b64 = NULL;
  if (C_REST_MALLOC(b64_len + 1, &b64) != C_REST_OK) {
    C_REST_FREE((void *)(concat));
    return C_REST_ERROR_OOM;
  }

#ifdef C_REST_TESTING_MALLOC_HOOK
  rc = crf_seam_client_check(31);
  if (rc != C_REST_OK)
    goto b64_second_check;
#endif
  rc = c_rest_base64_encode((unsigned char *)concat, clen, b64, &b64_len);
#ifdef C_REST_TESTING_MALLOC_HOOK
b64_second_check:
#endif
  if (rc != C_REST_OK) {
    C_REST_FREE((void *)(concat));
    C_REST_FREE((void *)(b64));
    return rc;
  }
  b64[b64_len] = '\0';
  C_REST_FREE((void *)(concat));

  hlen = 6 + b64_len + 1; /* "Basic " + b64 + null */
  *out_header = NULL;
  if (C_REST_MALLOC(hlen, out_header) != C_REST_OK) {
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
  *out_header = NULL;
  if (C_REST_MALLOC(hlen, out_header) != C_REST_OK)
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
  c_rest_error_t rc;
  c_rest_error_t ret;
  char *body = NULL;
  size_t body_len = 0;
  struct c_rest_client_header *all_headers = NULL;
  size_t all_headers_count = 0;
  size_t i;

  if (!client || !url)
    return C_REST_ERROR_GENERIC;

  rc =
      c_rest_client_build_form_urlencoded(fields, num_fields, &body, &body_len);
  if (rc != C_REST_OK)
    return rc;

  if (headers_count > 0 && headers) {
    for (i = 0; i < headers_count; ++i) {
      rc = c_rest_client_header_set(&all_headers, &all_headers_count,
                                    headers[i].key, headers[i].value);
      if (rc != C_REST_OK) {
        c_rest_error_t free_rc =
            c_rest_client_headers_free(all_headers, all_headers_count);
        C_REST_FREE((void *)(body));
        if (free_rc != C_REST_OK)
          return free_rc;
        return rc;
      }
    }
  }

  /* Add content type */
  rc =
      c_rest_client_header_set(&all_headers, &all_headers_count, "Content-Type",
                               "application/x-www-form-urlencoded");
  if (rc != C_REST_OK) {
    c_rest_error_t free_rc =
        c_rest_client_headers_free(all_headers, all_headers_count);
    C_REST_FREE((void *)(body));
    if (free_rc != C_REST_OK)
      return free_rc;
    return rc;
  }

  ret = c_rest_client_request_sync(client, url, "POST", all_headers,
                                   all_headers_count, body, body_len, out_res);
  if (ret != C_REST_OK) {
    rc = c_rest_client_headers_free(all_headers, all_headers_count);
    C_REST_FREE((void *)(body));
    if (rc != C_REST_OK)
      return rc;
    return ret;
  }

  rc = c_rest_client_headers_free(all_headers, all_headers_count);
  C_REST_FREE((void *)(body));
  if (rc != C_REST_OK)
    return rc;

  return C_REST_OK;
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

  str = NULL;
  if (C_REST_MALLOC(res->body_len + 1, &str) != C_REST_OK)
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
