/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_modality.h"
#include "c_rest_router.h"
#include "c_rest_request.h"
#include "c_rest_response.h"
#include "c_rest_sse.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <string.h>

#if defined(_WIN32)
#if !defined(_SYNCHAPI_H_) && !defined(_INC_SYNCHAPI)
__declspec(dllimport) void __stdcall Sleep(unsigned long dwMilliseconds);
#endif
#else
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#endif
/* clang-format on */

static int g_sse_delay_ms = 1000;

static c_rest_error_t sleep_ms(int milliseconds) {
#if defined(_WIN32)
  Sleep((unsigned long)milliseconds);
#else
  {
    struct timeval tv;
    tv.tv_sec = milliseconds / 1000;
    tv.tv_usec = (milliseconds % 1000) * 1000;
    select(0, NULL, NULL, NULL, &tv);
  }
#endif
  return C_REST_OK;
}

static c_rest_error_t my_sse_handler(struct c_rest_request *req,
                                     struct c_rest_response *res,
                                     void *user_data) {
  struct c_rest_sse_event ev;
  c_rest_error_t ev_rc;
  int i;
  (void)req;
  (void)user_data;

  printf("New SSE connection established!\n");

  /* We don't need to call c_rest_sse_init_response(res) here manually
     because c_rest_router_add_sse injects it automatically! */

  for (i = 0; i < 5; i++) {
    char data_buf[128];

    ev_rc = c_rest_sse_event_init(&ev);
    if (ev_rc != C_REST_OK)
      return ev_rc;
    ev.event = "ping";

#if defined(_MSC_VER)
    sprintf_s(data_buf, sizeof(data_buf),
              "Hello from SSE! This is message %d of 5.", i + 1);
#else
    sprintf(data_buf, "Hello from SSE! This is message %d of 5.", i + 1);
#endif
    ev.data = data_buf;

    printf("Sending event %d...\n", i + 1);

    if (c_rest_sse_send_event(res, &ev) != 0) {
      printf("Failed to send event, client probably disconnected.\n");
      break;
    }

    if (c_rest_sse_send_keepalive(res) != 0) {
      break;
    }

    sleep_ms(g_sse_delay_ms);
  }

  printf("SSE stream closed by server.\n");
  return 0;
}

static void sig_handler(int sig) {
  (void)sig;
  exit(0);
}

int main(void) {
  struct c_rest_context *ctx = NULL;
  c_rest_router *router = NULL;
  c_rest_error_t res;
  c_rest_error_t rc;

  printf("Starting Server-Sent Events (SSE) Example...\n");

  /* 1. Initialize context */
  signal(SIGTERM, sig_handler);
  signal(SIGINT, sig_handler);
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

  /* 3. Add SSE Route */
  res = c_rest_router_add_sse(router, "/events", my_sse_handler, NULL);
  if (res != 0) {
    printf("Failed to add SSE route\n");
    c_rest_router_destroy(router);
    c_rest_destroy(ctx);
    return 1;
  }

  /* Assign router to context */
  ctx->router = router;

  /* 4. Run the server loop (simulated by modality for now) */
  printf("Server listening on http://localhost:8080/events\n");
  rc = c_rest_run(ctx);
  if (rc != C_REST_OK) {
    printf("c_rest_run returned: %d\n", rc);
  }

  /* 5. Cleanup */
  rc = c_rest_router_destroy(router);
  if (rc != C_REST_OK) {
    c_rest_destroy(ctx);
    return 1;
  }
  rc = c_rest_destroy(ctx);
  if (rc != C_REST_OK)
    return 1;

  printf("Server stopped.\n");
  return 0;
}
