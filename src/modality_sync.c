/* clang-format off */
#include "c_rest_modality.h"
#include "c_rest_platform.h"

#include <stdlib.h>
/* clang-format on */

struct sync_state {
  c_rest_socket_t server_sock;
  int is_running;
};

static int sync_init(struct c_rest_context *ctx) {
  struct sync_state *state;
  if (!ctx)
    return 1;

  state =
      (struct sync_state *)ctx->allocator.malloc_cb(sizeof(struct sync_state));
  if (!state)
    return 1;

  state->server_sock = C_REST_INVALID_SOCKET;
  state->is_running = 0;

  ctx->internal_state = state;

  if (ctx->logger.log_cb) {
    ctx->logger.log_cb("SYNC modality initialized");
  }

  return 0;
}

static int sync_destroy(struct c_rest_context *ctx) {
  struct sync_state *state;

  if (!ctx || !ctx->internal_state)
    return 1;

  state = (struct sync_state *)ctx->internal_state;

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
    ctx->logger.log_cb("SYNC modality destroyed");
  }

  return 0;
}

static int sync_run(struct c_rest_context *ctx) {
  struct sync_state *state;
  if (!ctx || !ctx->internal_state)
    return 1;

  state = (struct sync_state *)ctx->internal_state;
  state->is_running = 1;

  if (ctx->logger.log_cb) {
    ctx->logger.log_cb("SYNC modality run started");
  }

  /* Typical synchronous accept loop */
  while (state->is_running && state->server_sock != C_REST_INVALID_SOCKET) {
    c_rest_socket_t client_sock;
    int res = 1;

#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
    if (ctx->cm_env) {
      res = cm_socket_accept(ctx->cm_env, state->server_sock, &client_sock);
    } else {
      res = c_rest_socket_accept(state->server_sock, &client_sock);
    }
#else
    res = c_rest_socket_accept(state->server_sock, &client_sock);
#endif

    if (res == 0 && client_sock != C_REST_INVALID_SOCKET) {
      struct c_rest_tls_connection *tls_conn = NULL;
      int tls_res = 1;

      if (ctx->tls_ctx) {
        tls_res = c_rest_tls_accept(ctx->tls_ctx, client_sock, &tls_conn);
        if (tls_res == 0) {
          /* Synchronous request reading and response writing would happen here
           * over TLS. */
          /* For now, we immediately close to prevent leaking. */
          c_rest_tls_close(tls_conn);
        } else {
          /* Handshake failed, close the raw socket below */
        }
      } else {
        /* Synchronous request reading and response writing would happen here
         * over cleartext. */
        /* For now, we immediately close to prevent leaking. */
      }

#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
      if (ctx->cm_env) {
        cm_socket_close(ctx->cm_env, client_sock);
      } else {
        c_rest_socket_close(client_sock);
      }
#else
      c_rest_socket_close(client_sock);
#endif
    } else {
      /* Error or interrupt, potentially break loop */
      break;
    }
  }

  state->is_running = 0;

  if (ctx->logger.log_cb) {
    ctx->logger.log_cb("SYNC modality run finished");
  }

  return 0;
}

const struct c_rest_modality_vtable sync_vtable = {sync_init, sync_destroy,
                                                   sync_run, NULL};
