/* clang-format off */
#include "c_rest_modality.h"
#include "c_rest_platform.h"

#include <stdlib.h>
/* clang-format on */

struct message_passing_state {
  c_rest_socket_t server_sock;
  int is_running;
};

static int message_passing_init(struct c_rest_context *ctx) {
  struct message_passing_state *state;
  if (!ctx)
    return 1;

  state = (struct message_passing_state *)ctx->allocator.malloc_cb(
      sizeof(struct message_passing_state));
  if (!state)
    return 1;

  state->server_sock = C_REST_INVALID_SOCKET;
  state->is_running = 0;

  ctx->internal_state = state;

  if (ctx->logger.log_cb) {
    ctx->logger.log_cb("MESSAGE_PASSING modality initialized");
  }

  return 0;
}

static int message_passing_destroy(struct c_rest_context *ctx) {
  struct message_passing_state *state;

  if (!ctx || !ctx->internal_state)
    return 1;

  state = (struct message_passing_state *)ctx->internal_state;

  if (state->server_sock != C_REST_INVALID_SOCKET) {
#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
    if (ctx->cm_env) {
      cm_socket_close(ctx->cm_env, state->server_sock);
    } else {
      c_rest_socket_close(state->server_sock);
    }
#else
    c_rest_socket_close(state->server_sock);
#endif
    state->server_sock = C_REST_INVALID_SOCKET;
  }

  ctx->allocator.free_cb(state);
  ctx->internal_state = NULL;

  if (ctx->logger.log_cb) {
    ctx->logger.log_cb("MESSAGE_PASSING modality destroyed");
  }

  return 0;
}

static int message_passing_run(struct c_rest_context *ctx) {
  struct message_passing_state *state;
  if (!ctx || !ctx->internal_state)
    return 1;

  state = (struct message_passing_state *)ctx->internal_state;
  state->is_running = 1;

  if (ctx->logger.log_cb) {
    ctx->logger.log_cb("MESSAGE_PASSING modality run started");
  }

  state->is_running = 0;

  if (ctx->logger.log_cb) {
    ctx->logger.log_cb("MESSAGE_PASSING modality run finished");
  }

  return 0;
}

const struct c_rest_modality_vtable message_passing_vtable = {
    message_passing_init, message_passing_destroy, message_passing_run, NULL};
