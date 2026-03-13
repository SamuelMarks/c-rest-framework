/* clang-format off */
#include "c_rest_client.h"
#include "c_rest_tls.h"

#include <stdio.h>
/* clang-format on */

static int async_called = 0;

static void async_callback(void *data) {
  (void)data;
  async_called = 1;
}

int test_client(void) {
  c_rest_client_context *client = NULL;
  int res;

  printf("Running client tests...\n");

  res = c_rest_tls_init();
  if (res != 0) {
    printf("Failed to init TLS\n");
    return 1;
  }

  res = c_rest_client_init(&client);
  if (res != 0 || !client) {
    printf("Failed to init client\n");
    return 1;
  }

  res = c_rest_client_request_sync(client, "http://localhost", "GET");
  if (res != 0) {
    printf("Failed sync request\n");
    return 1;
  }

  res = c_rest_client_request_async(client, "http://localhost", "POST",
                                    async_callback, NULL);
  if (res != 0) {
    printf("Failed async request\n");
    return 1;
  }

  if (!async_called) {
    printf("Async callback was not invoked\n");
    return 1;
  }

  res = c_rest_proxy_request("http://localhost/proxy", NULL, NULL);
  if (res != 0) {
    printf("Proxy request failed\n");
    return 1;
  }

  c_rest_client_destroy(client);

  return 0;
}
