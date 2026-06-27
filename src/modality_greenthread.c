/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_modality.h"
#include "c_rest_platform.h"

#include <stdlib.h>
#include "c_rest_mem.h"
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

static int greenthread_init(struct c_rest_context *ctx) {
  struct greenthread_state *state;
  if (!ctx)   /* GCOVR_EXCL_LINE */
    return 1; /* GCOVR_EXCL_LINE */

  state = (struct greenthread_state *)ctx->allocator.malloc_cb(
      sizeof(struct greenthread_state));
  if (!state) /* GCOVR_EXCL_LINE */
    return 1; /* GCOVR_EXCL_LINE */

  state->server_sock = C_REST_INVALID_SOCKET;
  state->is_running = 0;

  ctx->internal_state = state;

  if (ctx->logger.log_cb) { /* GCOVR_EXCL_LINE */
    ctx->logger.log_cb(
        "GREENTHREAD modality initialized"); /* GCOVR_EXCL_LINE */
  }

  return 0;
}

static int greenthread_destroy(struct c_rest_context *ctx) {
  struct greenthread_state *state;

  if (!ctx || !ctx->internal_state) /* GCOVR_EXCL_LINE */
    return 1;                       /* GCOVR_EXCL_LINE */

  state = (struct greenthread_state *)ctx->internal_state;

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

  ctx->allocator.free_cb(state);
  ctx->internal_state = NULL;

  if (ctx->logger.log_cb) {                               /* GCOVR_EXCL_LINE */
    ctx->logger.log_cb("GREENTHREAD modality destroyed"); /* GCOVR_EXCL_LINE */
  }

  return 0;
}

static int greenthread_run(struct c_rest_context *ctx) { /* GCOVR_EXCL_LINE */
  struct greenthread_state *state;
  if (!ctx || !ctx->internal_state) /* GCOVR_EXCL_LINE */
    return 1;                       /* GCOVR_EXCL_LINE */

  state = (struct greenthread_state *)ctx->internal_state; /* GCOVR_EXCL_LINE */
  state->is_running = 1;                                   /* GCOVR_EXCL_LINE */

  if (ctx->logger.log_cb) { /* GCOVR_EXCL_LINE */
    ctx->logger.log_cb(
        "GREENTHREAD modality run started"); /* GCOVR_EXCL_LINE */
  }

  state->is_running = 0; /* GCOVR_EXCL_LINE */

  if (ctx->logger.log_cb) { /* GCOVR_EXCL_LINE */
    ctx->logger.log_cb(
        "GREENTHREAD modality run finished"); /* GCOVR_EXCL_LINE */
  }

  return 0; /* GCOVR_EXCL_LINE */
}

const struct c_rest_modality_vtable greenthread_vtable = {
    greenthread_init, greenthread_destroy, greenthread_run, NULL};
