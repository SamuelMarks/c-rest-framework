/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_hot_reload.h"
#include "c_rest_router.h"
#include "c_rest_request.h"
#include "c_rest_response.h"
#include "c_rest_modality.h"
#include "c_rest_platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <string.h>
/* clang-format on */

static c_rest_error_t handle_home(struct c_rest_request *req,
                                  struct c_rest_response *res,
                                  void *user_data) {
  const char *html =
      "<html><body><h1>Hello, Hot-Reload!</h1><script>const evtSource = new "
      "EventSource('/_hot_reload'); evtSource.onmessage = function(e) { if "
      "(e.data === 'true') { window.location.reload(); } "
      "};</script></body></html>";
  (void)user_data;
  (void)req;
  res->body = (char *)html;
  res->body_len = strlen(html);
  return 0;
}

static int on_file_changed(void *user_data) {
  (void)user_data;
  printf("File changed! Sending SSE event to clients...\n");
  return 0;
}

static void sig_handler(int sig) {
  (void)sig;
  exit(0);
}

int main(void) {

  struct c_rest_context *ctx;
  c_rest_router *router;
  c_rest_hot_reload_ctx_t *hr_ctx = NULL;

  (void)!c_rest_platform_init();
  signal(SIGTERM, sig_handler);
  signal(SIGINT, sig_handler);
  (void)!c_rest_init(C_REST_MODALITY_MULTI_THREAD, &ctx);

  (void)!c_rest_router_init(&router);
  (void)!c_rest_router_add(router, "GET", "/", handle_home, NULL);

  (void)!c_rest_hot_reload_init(&hr_ctx, NULL);
  (void)!c_rest_hot_reload_add_watch(hr_ctx, "examples/app_hot_reload/main.c");
  (void)!c_rest_hot_reload_register_routes(router, "/_hot_reload");
  (void)!c_rest_hot_reload_start(hr_ctx, on_file_changed, NULL);

  /* Associate with framework context if needed. */
#ifdef C_REST_ENABLE_HOT_RELOADING_AUTO_RESTART
  ctx->hot_reload_ctx = hr_ctx;
#endif

  (void)!c_rest_set_router(ctx, router);

  ctx->listen_port = 8080;
  ctx->listen_address = "127.0.0.1";

  printf("Server running at http://127.0.0.1:8080\n");
  (void)!c_rest_run(ctx);

  (void)!c_rest_hot_reload_destroy(hr_ctx);
  (void)!c_rest_router_destroy(router);
  (void)!c_rest_destroy(ctx);
  (void)!c_rest_platform_cleanup();

  return 0;
}
