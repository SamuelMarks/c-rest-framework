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

int c_rest_client_request_sync(c_rest_client_context *client, const char *url,
                               const char *method) {
  struct HttpRequest req;
  struct HttpResponse *res = NULL;
  int rc;

  if (!client || !client->client.transport || !url || !method)
    return 1;

  if (http_request_init(&req) != 0)
    return 1;

  /* We cast url to char* because HttpRequest url is non-const in http_types.h
   */
  req.url = (char *)url;
  req.method = method_from_str(method);

  rc = client->client.send(client->client.transport, &req, &res);

  if (res) {
    http_response_free(res);
    free(res);
  }

  /* Do not free req.url because we didn't allocate it */
  req.url = NULL;
  http_request_free(&req);

  return rc;
}

int c_rest_client_request_async(c_rest_client_context *client, const char *url,
                                const char *method, void (*callback)(void *),
                                void *user_data) {
  int res;
  if (!client || !client->client.transport || !url || !method)
    return 1;

  /* Mock async execution by just doing it synchronously and calling the
   * callback */
  res = c_rest_client_request_sync(client, url, method);
  if (callback) {
    callback(user_data);
  }
  return res;
}

int c_rest_proxy_request(const char *target_url, void *req, void *res) {
  /* Simple proxy stub */
  c_rest_client_context *client;
  int ret;
  (void)req; /* unused in mock */
  (void)res; /* unused in mock */

  if (!target_url)
    return 1;

  if (c_rest_client_init(&client) != 0) {
    return 1;
  }

  ret = c_rest_client_request_sync(client, target_url, "GET");
  c_rest_client_destroy(client);

  return ret;
}
