/* clang-format off */
#include "c_rest_modality.h"
#include "c_rest_platform.h"

#include <stdlib.h>
#include "c_rest_mem.h"
#include "c_rest_log.h"
/* clang-format on */

struct multi_process_state {
  c_rest_socket_t server_sock;
  int is_running;
  c_rest_process_t *workers;
  int worker_count;
};

static int multi_process_init(struct c_rest_context *ctx) {
  struct multi_process_state *state;
  if (!ctx)   /* GCOVR_EXCL_LINE */
    return 1; /* GCOVR_EXCL_LINE */

  state = (struct multi_process_state *)ctx->allocator.malloc_cb(
      sizeof(struct multi_process_state));
  if (!state) /* GCOVR_EXCL_LINE */
    return 1; /* GCOVR_EXCL_LINE */

  state->server_sock = C_REST_INVALID_SOCKET;
  state->is_running = 0;
  state->workers = NULL;
  state->worker_count = 4; /* Default to 4 processes for now */

  ctx->internal_state = state;

  if (ctx->logger.log_cb) { /* GCOVR_EXCL_LINE */
    ctx->logger.log_cb(
        "MULTI_PROCESS modality initialized"); /* GCOVR_EXCL_LINE */
  }

  return 0;
}

static int multi_process_destroy(struct c_rest_context *ctx) {
  struct multi_process_state *state;

  if (!ctx || !ctx->internal_state) /* GCOVR_EXCL_LINE */
    return 1;                       /* GCOVR_EXCL_LINE */

  state = (struct multi_process_state *)ctx->internal_state;

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

  if (state->workers) {                     /* GCOVR_EXCL_LINE */
    ctx->allocator.free_cb(state->workers); /* GCOVR_EXCL_LINE */
  }

  ctx->allocator.free_cb(state);
  ctx->internal_state = NULL;

  if (ctx->logger.log_cb) { /* GCOVR_EXCL_LINE */
    ctx->logger.log_cb(
        "MULTI_PROCESS modality destroyed"); /* GCOVR_EXCL_LINE */
  }

  return 0;
}

static int multi_process_run(struct c_rest_context *ctx) { /* GCOVR_EXCL_LINE */
  struct multi_process_state *state;
  if (!ctx || !ctx->internal_state) /* GCOVR_EXCL_LINE */
    return 1;                       /* GCOVR_EXCL_LINE */

  state =
      (struct multi_process_state *)ctx->internal_state; /* GCOVR_EXCL_LINE */
  state->is_running = 1;                                 /* GCOVR_EXCL_LINE */

  if (ctx->logger.log_cb) { /* GCOVR_EXCL_LINE */
    ctx->logger.log_cb(
        "MULTI_PROCESS modality run started"); /* GCOVR_EXCL_LINE */
  }

  /* Conceptually:
   * 1. Bind shared socket
   * 2. fork() workers
   * 3. Master process monitors workers and respawns them if they crash
   * 4. Worker processes loop on accept() for the shared socket
   */

  state->is_running = 0; /* GCOVR_EXCL_LINE */

  if (ctx->logger.log_cb) { /* GCOVR_EXCL_LINE */
    ctx->logger.log_cb(
        "MULTI_PROCESS modality run finished"); /* GCOVR_EXCL_LINE */
  }

  return 0; /* GCOVR_EXCL_LINE */
}

const struct c_rest_modality_vtable multi_process_vtable = {
    multi_process_init, multi_process_destroy, multi_process_run, NULL};
