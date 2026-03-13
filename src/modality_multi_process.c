/* clang-format off */
#include "c_rest_modality.h"
#include "c_rest_platform.h"

#include <stdlib.h>
/* clang-format on */

struct multi_process_state {
  c_rest_socket_t server_sock;
  int is_running;
  c_rest_process_t *workers;
  int worker_count;
};

static int multi_process_init(struct c_rest_context *ctx) {
  struct multi_process_state *state;
  if (!ctx)
    return 1;

  state = (struct multi_process_state *)ctx->allocator.malloc_cb(
      sizeof(struct multi_process_state));
  if (!state)
    return 1;

  state->server_sock = C_REST_INVALID_SOCKET;
  state->is_running = 0;
  state->workers = NULL;
  state->worker_count = 4; /* Default to 4 processes for now */

  ctx->internal_state = state;

  if (ctx->logger.log_cb) {
    ctx->logger.log_cb("MULTI_PROCESS modality initialized");
  }

  return 0;
}

static int multi_process_destroy(struct c_rest_context *ctx) {
  struct multi_process_state *state;

  if (!ctx || !ctx->internal_state)
    return 1;

  state = (struct multi_process_state *)ctx->internal_state;

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

  if (state->workers) {
    ctx->allocator.free_cb(state->workers);
  }

  ctx->allocator.free_cb(state);
  ctx->internal_state = NULL;

  if (ctx->logger.log_cb) {
    ctx->logger.log_cb("MULTI_PROCESS modality destroyed");
  }

  return 0;
}

static int multi_process_run(struct c_rest_context *ctx) {
  struct multi_process_state *state;
  if (!ctx || !ctx->internal_state)
    return 1;

  state = (struct multi_process_state *)ctx->internal_state;
  state->is_running = 1;

  if (ctx->logger.log_cb) {
    ctx->logger.log_cb("MULTI_PROCESS modality run started");
  }

  /* Conceptually:
   * 1. Bind shared socket
   * 2. fork() workers
   * 3. Master process monitors workers and respawns them if they crash
   * 4. Worker processes loop on accept() for the shared socket
   */

  state->is_running = 0;

  if (ctx->logger.log_cb) {
    ctx->logger.log_cb("MULTI_PROCESS modality run finished");
  }

  return 0;
}

const struct c_rest_modality_vtable multi_process_vtable = {
    multi_process_init, multi_process_destroy, multi_process_run};
