/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "c_rest_modality.h"
#include "c_rest_platform.h"

#include <stdlib.h>
#include "c_rest_log.h"
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

static c_rest_error_t greenthread_init(struct c_rest_context *ctx) {
  c_rest_error_t rc;
  struct greenthread_state *state;
  if (!ctx)
    return C_REST_ERROR_INVALID_ARG;

  state = (struct greenthread_state *)ctx->allocator.malloc_cb(
      sizeof(struct greenthread_state));
  if (!state)
    return C_REST_ERROR_OOM;

  state->server_sock = C_REST_INVALID_SOCKET;
  state->is_running = 0;

  ctx->internal_state = state;

  if (ctx->logger.log_cb) {
    rc = ctx->logger.log_cb("GREENTHREAD modality initialized");
    if (rc != C_REST_OK)
      return rc;
  }
  return C_REST_OK;
}

static c_rest_error_t greenthread_destroy(struct c_rest_context *ctx) {
  c_rest_error_t rc;
  struct greenthread_state *state;

  if (!ctx || !ctx->internal_state)
    return C_REST_ERROR_INVALID_ARG;

  state = (struct greenthread_state *)ctx->internal_state;

  if (state->server_sock != C_REST_INVALID_SOCKET) {
#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
    if (ctx->cm_env) {
      cm_socket_close(ctx->cm_env, state->server_sock);
    } else {
      rc = c_rest_socket_close(state->server_sock);
      if (rc != C_REST_OK)
        return rc;
    }
#else
    rc = c_rest_socket_close(state->server_sock);
    if (rc != C_REST_OK)
      return rc;
#endif
    state->server_sock = C_REST_INVALID_SOCKET;
  }

  ctx->allocator.free_cb(state);
  ctx->internal_state = NULL;

  if (ctx->logger.log_cb) {
    rc = ctx->logger.log_cb("GREENTHREAD modality destroyed");
    if (rc != C_REST_OK)
      return rc;
  }
  return C_REST_OK;
}

static c_rest_error_t greenthread_run(struct c_rest_context *ctx) {
  c_rest_error_t rc;
  struct greenthread_state *state;
  if (!ctx || !ctx->internal_state)
    return C_REST_ERROR_INVALID_ARG;

  state = (struct greenthread_state *)ctx->internal_state;
  state->is_running = 1;

  if (ctx->logger.log_cb) {
    rc = ctx->logger.log_cb("GREENTHREAD modality run started");
    if (rc != C_REST_OK)
      return rc;
  }

  state->is_running = 0;

  if (ctx->logger.log_cb) {
    rc = ctx->logger.log_cb("GREENTHREAD modality run finished");
    if (rc != C_REST_OK)
      return rc;
  }
  return C_REST_OK;
}

const struct c_rest_modality_vtable greenthread_vtable = {
    greenthread_init, greenthread_destroy, greenthread_run, NULL};
