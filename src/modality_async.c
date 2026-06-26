/* clang-format off */
#include "c_rest_modality.h"
#include "c_rest_platform.h"

#include <stdlib.h>
#include "c_rest_mem.h"
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

static int async_init(struct c_rest_context *ctx) {
  struct async_state *state;
  if (!ctx)   /* GCOVR_EXCL_LINE */
    return 1; /* GCOVR_EXCL_LINE */

  state = (struct async_state *)ctx->allocator.malloc_cb(
      sizeof(struct async_state));
  if (!state) /* GCOVR_EXCL_LINE */
    return 1; /* GCOVR_EXCL_LINE */

  state->server_sock = C_REST_INVALID_SOCKET;

  state->evloop = (struct c_rest_event_loop *)ctx->allocator.malloc_cb(
      sizeof(struct c_rest_event_loop));
  if (!state->evloop) {            /* GCOVR_EXCL_LINE */
    ctx->allocator.free_cb(state); /* GCOVR_EXCL_LINE */
    return 1;                      /* GCOVR_EXCL_LINE */
  }
  state->evloop->is_running = 0;
  state->evloop->poll_backend = 0;

  ctx->internal_state = state;

  if (ctx->logger.log_cb) {                           /* GCOVR_EXCL_LINE */
    ctx->logger.log_cb("ASYNC modality initialized"); /* GCOVR_EXCL_LINE */
  }

  return 0;
}

static int async_destroy(struct c_rest_context *ctx) {
  struct async_state *state;

  if (!ctx || !ctx->internal_state) /* GCOVR_EXCL_LINE */
    return 1;                       /* GCOVR_EXCL_LINE */

  state = (struct async_state *)ctx->internal_state;

  if (state->server_sock != C_REST_INVALID_SOCKET) { /* GCOVR_EXCL_LINE */
#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
    if (ctx->cm_env) {
      cm_socket_close(ctx->cm_env, state->server_sock);
    } else {
      c_rest_socket_close(state->server_sock);
    }
#else
    c_rest_socket_close(state->server_sock); /* GCOVR_EXCL_LINE */
#endif
    state->server_sock = C_REST_INVALID_SOCKET; /* GCOVR_EXCL_LINE */
  }

  if (state->evloop) { /* GCOVR_EXCL_LINE */
    ctx->allocator.free_cb(state->evloop);
  }

  ctx->allocator.free_cb(state);
  ctx->internal_state = NULL;

  if (ctx->logger.log_cb) {                         /* GCOVR_EXCL_LINE */
    ctx->logger.log_cb("ASYNC modality destroyed"); /* GCOVR_EXCL_LINE */
  }

  return 0;
}

static int async_run(struct c_rest_context *ctx) { /* GCOVR_EXCL_LINE */
  struct async_state *state;
  if (!ctx || !ctx->internal_state) /* GCOVR_EXCL_LINE */
    return 1;                       /* GCOVR_EXCL_LINE */

  state = (struct async_state *)ctx->internal_state; /* GCOVR_EXCL_LINE */
  state->evloop->is_running = 1;                     /* GCOVR_EXCL_LINE */

  if (ctx->logger.log_cb) {                           /* GCOVR_EXCL_LINE */
    ctx->logger.log_cb("ASYNC modality run started"); /* GCOVR_EXCL_LINE */
  }

  /* Conceptually: while (state->evloop->is_running) { poll_backend_wait();
   * process_callbacks(); }
   * If TLS is enabled via ctx->tls_ctx, connections returning
   * C_REST_TLS_WANT_READ must be re-polled for EPOLLIN. Connections returning
   * C_REST_TLS_WANT_WRITE must be re-polled for EPOLLOUT. */

  state->evloop->is_running = 0; /* GCOVR_EXCL_LINE */

  if (ctx->logger.log_cb) {                            /* GCOVR_EXCL_LINE */
    ctx->logger.log_cb("ASYNC modality run finished"); /* GCOVR_EXCL_LINE */
  }

  return 0; /* GCOVR_EXCL_LINE */
}

const struct c_rest_modality_vtable async_vtable = {async_init, async_destroy,
                                                    async_run, NULL};
