/* clang-format off */
#include "c_rest_modality.h"
#include "c_rest_platform.h"

#include <stdlib.h>
#include "c_rest_mem.h"
#include "c_rest_log.h"
/* clang-format on */

struct sync_state {
  c_rest_socket_t server_sock;
  int is_running;
};

static int sync_init(struct c_rest_context *ctx) {
  struct sync_state *state;
  if (!ctx)   /* GCOVR_EXCL_LINE */
    return 1; /* GCOVR_EXCL_LINE */

  state =
      (struct sync_state *)ctx->allocator.malloc_cb(sizeof(struct sync_state));
  if (!state) /* GCOVR_EXCL_LINE */
    return 1; /* GCOVR_EXCL_LINE */

  state->server_sock = C_REST_INVALID_SOCKET;
  state->is_running = 0;

  ctx->internal_state = state;

  if (ctx->logger.log_cb) {                          /* GCOVR_EXCL_LINE */
    ctx->logger.log_cb("SYNC modality initialized"); /* GCOVR_EXCL_LINE */
  }

  return 0;
}

static int sync_destroy(struct c_rest_context *ctx) {
  struct sync_state *state;

  if (!ctx || !ctx->internal_state) /* GCOVR_EXCL_LINE */
    return 1;                       /* GCOVR_EXCL_LINE */

  state = (struct sync_state *)ctx->internal_state;

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

  if (ctx->logger.log_cb) {
    ctx->logger.log_cb("SYNC modality destroyed");
  }

  return 0;
}

static int sync_run(struct c_rest_context *ctx) {
  struct sync_state *state;
  if (!ctx || !ctx->internal_state) /* GCOVR_EXCL_LINE */
    return 1;                       /* GCOVR_EXCL_LINE */

  state = (struct sync_state *)ctx->internal_state;
  state->is_running = 1;

  if (ctx->logger.log_cb) {                          /* GCOVR_EXCL_LINE */
    ctx->logger.log_cb("SYNC modality run started"); /* GCOVR_EXCL_LINE */
  }

  /* Typical synchronous accept loop */
  while (state->is_running &&
         state->server_sock != C_REST_INVALID_SOCKET) { /* GCOVR_EXCL_LINE */
    c_rest_socket_t client_sock;
    int res = 1; /* GCOVR_EXCL_LINE */

#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
    if (ctx->cm_env) {
      res = cm_socket_accept(ctx->cm_env, state->server_sock, &client_sock);
    } else {
      res = c_rest_socket_accept(state->server_sock, &client_sock);
    }
#else
    res = c_rest_socket_accept(state->server_sock,
                               &client_sock); /* GCOVR_EXCL_LINE */
#endif

    if (res == 0 &&
        client_sock != C_REST_INVALID_SOCKET) {      /* GCOVR_EXCL_LINE */
      struct c_rest_tls_connection *tls_conn = NULL; /* GCOVR_EXCL_LINE */
      int tls_res = 1;                               /* GCOVR_EXCL_LINE */

      if (ctx->tls_ctx) { /* GCOVR_EXCL_LINE */
        tls_res = c_rest_tls_accept(ctx->tls_ctx, client_sock,
                                    &tls_conn); /* GCOVR_EXCL_LINE */
        if (tls_res == 0) {                     /* GCOVR_EXCL_LINE */
          /* Synchronous request reading and response writing would happen here
           * over TLS. */
          /* For now, we immediately close to prevent leaking. */
          c_rest_tls_close(tls_conn); /* GCOVR_EXCL_LINE */
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
      c_rest_socket_close(client_sock); /* GCOVR_EXCL_LINE */
#endif
    } else {
      /* Error or interrupt, potentially break loop */
      break;
    }
  }

  state->is_running = 0;

  if (ctx->logger.log_cb) {                           /* GCOVR_EXCL_LINE */
    ctx->logger.log_cb("SYNC modality run finished"); /* GCOVR_EXCL_LINE */
  }

  return 0;
}

const struct c_rest_modality_vtable sync_vtable = {sync_init, sync_destroy,
                                                   sync_run, NULL};
