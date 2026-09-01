#include "c_rest_testing_mocks.h"
/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "c_rest_modality.h"
#include "c_rest_platform.h"

#include <stdlib.h>
#include "c_rest_log.h"
/* clang-format on */

struct c_rest_event_loop {
  int is_running;
  /* Placeholder for actual polling handles (epoll_fd, IOCP handle, etc.) */
  ptrdiff_t poll_backend;
};

struct async_state {
  c_rest_socket_t server_sock;
  struct c_rest_event_loop *evloop;
};

static c_rest_error_t async_init(struct c_rest_context *ctx) {
  c_rest_error_t rc;
  struct async_state *state;
  if (!ctx)
    return C_REST_ERROR_INVALID_ARG;

  state = (struct async_state *)ctx->allocator.malloc_cb(
      sizeof(struct async_state));
  if (!state)
    return C_REST_ERROR_OOM;

  state->server_sock = C_REST_INVALID_SOCKET;

  state->evloop = (struct c_rest_event_loop *)ctx->allocator.malloc_cb(
      sizeof(struct c_rest_event_loop));
  if (!state->evloop) {
    ctx->allocator.free_cb(state);
    return C_REST_ERROR_OOM;
  }
  state->evloop->is_running = 0;
  state->evloop->poll_backend = 0;

  ctx->internal_state = state;

  if (ctx->logger.log_cb) {
    rc = ctx->logger.log_cb("ASYNC modality initialized");
    if (rc != C_REST_OK) {
      if (state->evloop) {
        ctx->allocator.free_cb(state->evloop);
      }
      ctx->allocator.free_cb(state);
      ctx->internal_state = NULL;
      return rc;
    }
  }
  return C_REST_OK;
}

static c_rest_error_t async_destroy(struct c_rest_context *ctx) {
  c_rest_error_t rc;
  struct async_state *state;

  if (!ctx || !ctx->internal_state)
    return C_REST_ERROR_INVALID_ARG;

  state = (struct async_state *)ctx->internal_state;

  if (state->server_sock != C_REST_INVALID_SOCKET) {
#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
    if (ctx->cm_env) {
      rc = cm_socket_close(ctx->cm_env, state->server_sock);
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

  if (state->evloop) {
    ctx->allocator.free_cb(state->evloop);
  }

  ctx->allocator.free_cb(state);
  ctx->internal_state = NULL;

  if (ctx->logger.log_cb) {
    rc = ctx->logger.log_cb("ASYNC modality destroyed");
    if (rc != C_REST_OK)
      return rc;
  }
  return C_REST_OK;
}

static c_rest_error_t async_run(struct c_rest_context *ctx) {
  c_rest_error_t rc;
  struct async_state *state;
  if (!ctx || !ctx->internal_state)
    return C_REST_ERROR_INVALID_ARG;

  state = (struct async_state *)ctx->internal_state;
  state->evloop->is_running = 1;

  if (ctx->logger.log_cb) {
    rc = ctx->logger.log_cb("ASYNC modality run started");
    if (rc != C_REST_OK)
      return rc;
  }

  /* Conceptually: while (state->evloop->is_running) { poll_backend_wait();
   * process_callbacks(); }
   * If TLS is enabled via ctx->tls_ctx, connections returning
   * C_REST_TLS_WANT_READ must be re-polled for EPOLLIN. Connections returning
   * C_REST_TLS_WANT_WRITE must be re-polled for EPOLLOUT. */

  state->evloop->is_running = 0;

  if (ctx->logger.log_cb) {
    rc = ctx->logger.log_cb("ASYNC modality run finished");
    if (rc != C_REST_OK)
      return rc;
  }
  return C_REST_OK;
}

const struct c_rest_modality_vtable async_vtable = {async_init, async_destroy,
                                                    async_run, NULL};
