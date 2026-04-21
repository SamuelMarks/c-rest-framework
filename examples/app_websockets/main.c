/* clang-format off */
#include "c_rest_modality.h"
#include "c_rest_router.h"
#include "c_rest_request.h"
#include "c_rest_response.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* clang-format on */

static int my_ws_on_message(struct c_rest_request *req,
                            const unsigned char *payload, size_t payload_len,
                            int is_binary, void *user_data) {
  (void)req;
  (void)user_data;

  if (is_binary) {
    printf("Received binary message: %u bytes\n", (unsigned int)payload_len);
  } else {
    printf("Received text message: %.*s\n", (int)payload_len, payload);
  }

  /* Echo server logic would go here if modality was fully implemented */
  return 0;
}

static void my_ws_on_close(struct c_rest_request *req, int status_code,
                           void *user_data) {
  (void)req;
  (void)user_data;
  printf("WebSocket closed with status: %d\n", status_code);
}

int main(void) {
  struct c_rest_context *ctx = NULL;
  c_rest_router *router = NULL;
  int res;

  printf("Starting WebSocket Server Example...\n");

  /* 1. Initialize context */
  res = c_rest_init(C_REST_MODALITY_SYNC, &ctx);
  if (res != 0) {
    printf("Failed to init context\n");
    return 1;
  }

  /* 2. Initialize router */
  res = c_rest_router_init(&router);
  if (res != 0) {
    printf("Failed to init router\n");
    c_rest_destroy(ctx);
    return 1;
  }

  /* 3. Add WebSocket Route */
  res = c_rest_router_add_websocket(router, "/ws", my_ws_on_message,
                                    my_ws_on_close, NULL);
  if (res != 0) {
    printf("Failed to add WebSocket route\n");
    c_rest_router_destroy(router);
    c_rest_destroy(ctx);
    return 1;
  }

  /* Assign router to context */
  ctx->router = router;

  /* 4. Run the server loop (simulated by modality for now) */
  printf("Server listening on ws://localhost:8080/ws\n");
  c_rest_run(ctx);

  /* 5. Cleanup */
  c_rest_router_destroy(router);
  c_rest_destroy(ctx);

  printf("Server stopped.\n");
  return 0;
}
