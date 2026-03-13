/* clang-format off */
#include "c_rest_modality.h"
#include "c_rest_platform.h"

#include <stdlib.h>
/* clang-format on */

/*
 * Note: Pure C89 coroutines are heavily system-dependent. We can implement a
 * stackful approach using setjmp/longjmp, but it requires inline assembly for
 * stack manipulation. For now, this is a stub scaffolding for the Greenthreads
 * modality.
 */

struct greenthread_state {
  c_rest_socket_t server_sock;
  int is_running;
};

static int greenthread_init(struct c_rest_context *ctx) {
  struct greenthread_state *state;
  if (!ctx)
    return 1;

  state = (struct greenthread_state *)ctx->allocator.malloc_cb(
      sizeof(struct greenthread_state));
  if (!state)
    return 1;

  state->server_sock = C_REST_INVALID_SOCKET;
  state->is_running = 0;

  ctx->internal_state = state;

  if (ctx->logger.log_cb) {
    ctx->logger.log_cb("GREENTHREAD modality initialized");
  }

  return 0;
}

static int greenthread_destroy(struct c_rest_context *ctx) {
  struct greenthread_state *state;

  if (!ctx || !ctx->internal_state)
    return 1;

  state = (struct greenthread_state *)ctx->internal_state;

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
    ctx->logger.log_cb("GREENTHREAD modality destroyed");
  }

  return 0;
}

static int greenthread_run(struct c_rest_context *ctx) {
  struct greenthread_state *state;
  if (!ctx || !ctx->internal_state)
    return 1;

  state = (struct greenthread_state *)ctx->internal_state;
  state->is_running = 1;

  if (ctx->logger.log_cb) {
    ctx->logger.log_cb("GREENTHREAD modality run started");
  }

  state->is_running = 0;

  if (ctx->logger.log_cb) {
    ctx->logger.log_cb("GREENTHREAD modality run finished");
  }

  return 0;
}

const struct c_rest_modality_vtable greenthread_vtable = {
    greenthread_init, greenthread_destroy, greenthread_run};
