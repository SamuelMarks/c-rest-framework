#ifdef C_REST_FRAMEWORK_USE_REAL_CAH
/* clang-format off */
#include <c_abstract_http/c_abstract_http.h>
#else
#include "c_abstract_http.h"
#endif
#include "c_rest_client.h"

#include <stdlib.h>
/* clang-format on */

struct c_rest_client_context {
  cah_client *internal_client;
};

int c_rest_client_init(c_rest_client_context **out_client) {
  struct c_rest_client_context *ctx;
  if (!out_client)
    return 1;

  ctx = (struct c_rest_client_context *)malloc(
      sizeof(struct c_rest_client_context));
  if (!ctx)
    return 1;

  if (cah_client_create(&ctx->internal_client) != 0) {
    free(ctx);
    return 1;
  }
  *out_client = ctx;
  return 0;
}

int c_rest_client_destroy(c_rest_client_context *client) {
  if (!client)
    return 1;
  if (client->internal_client) {
    cah_client_destroy(client->internal_client);
  }
  free(client);
  return 0;
}

int c_rest_client_request_sync(c_rest_client_context *client, const char *url,
                               const char *method) {
  if (!client || !client->internal_client || !url || !method)
    return 1;
  return cah_client_request(client->internal_client, url, method);
}

int c_rest_client_request_async(c_rest_client_context *client, const char *url,
                                const char *method, void (*callback)(void *),
                                void *user_data) {
  int res;
  if (!client || !client->internal_client || !url || !method)
    return 1;

  /* Mock async execution by just doing it synchronously and calling the
   * callback */
  res = cah_client_request(client->internal_client, url, method);
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
