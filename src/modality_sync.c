/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "c_rest_modality.h"


#ifdef C_REST_TESTING_MALLOC_HOOK
c_rest_error_t mock_c_rest_socket_create(c_rest_socket_t *sock);
c_rest_error_t mock_c_rest_socket_bind(c_rest_socket_t sock, const char *host, unsigned short port);
c_rest_error_t mock_c_rest_socket_listen(c_rest_socket_t sock, int backlog);
c_rest_error_t mock_c_rest_socket_accept(c_rest_socket_t server, c_rest_socket_t *out_client);
c_rest_error_t mock_c_rest_thread_create(c_rest_thread_t *thread, c_rest_error_t (*func)(void *), void *arg);
c_rest_error_t mock_c_rest_socket_close(c_rest_socket_t sock);
c_rest_error_t mock_c_rest_tls_accept(struct c_rest_tls_context *ctx, c_rest_socket_t sock, struct c_rest_tls_connection **out_conn);
c_rest_error_t mock_c_rest_tls_close(struct c_rest_tls_connection *conn);

#define c_rest_socket_create mock_c_rest_socket_create
#define c_rest_socket_bind mock_c_rest_socket_bind
#define c_rest_socket_listen mock_c_rest_socket_listen
#define c_rest_socket_accept mock_c_rest_socket_accept
#define c_rest_thread_create mock_c_rest_thread_create
#define c_rest_socket_close mock_c_rest_socket_close
#define c_rest_tls_accept mock_c_rest_tls_accept
#define c_rest_tls_close mock_c_rest_tls_close
#endif
#include "c_rest_platform.h"

#include <stdlib.h>
#include "c_rest_log.h"
/* clang-format on */

struct sync_state {
  c_rest_socket_t server_sock;
  volatile int is_running;
};

static c_rest_error_t sync_init(struct c_rest_context *ctx) {
  struct sync_state *state;
  c_rest_error_t rc;
  if (!ctx) {
    return C_REST_ERROR_INVALID_ARG;
  }

  state =
      (struct sync_state *)ctx->allocator.malloc_cb(sizeof(struct sync_state));
  if (!state) {
    return C_REST_ERROR_OOM;
  }

  state->server_sock = C_REST_INVALID_SOCKET;
  state->is_running = 0;

  ctx->internal_state = state;

  if (ctx->logger.log_cb) {
    rc = ctx->logger.log_cb("SYNC modality initialized");
    if (rc != C_REST_OK) {
      ctx->allocator.free_cb(state);
      ctx->internal_state = NULL;
      return rc;
    }
  }

  return C_REST_OK;
}

static c_rest_error_t sync_destroy(struct c_rest_context *ctx) {
  struct sync_state *state;
  c_rest_error_t rc;

  if (!ctx || !ctx->internal_state) {
    return C_REST_ERROR_INVALID_ARG;
  }

  state = (struct sync_state *)ctx->internal_state;

  if (state->server_sock != C_REST_INVALID_SOCKET) {
#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
    if (ctx->cm_env) {
      cm_socket_close(ctx->cm_env, state->server_sock);
    } else {
      rc = c_rest_socket_close(state->server_sock);
      if (rc != C_REST_OK) {
        return rc;
      }
    }
#else
    rc = c_rest_socket_close(state->server_sock);
    if (rc != C_REST_OK) {
      return rc;
    }

#endif
#if defined(_WIN32)
    state->server_sock = C_REST_INVALID_SOCKET;
#endif
  }

  ctx->allocator.free_cb(state);
  ctx->internal_state = NULL;

  if (ctx->logger.log_cb) {
    rc = ctx->logger.log_cb("SYNC modality destroyed");
    if (rc != C_REST_OK) {
      return rc;
    }
  }

  return C_REST_OK;
}

static c_rest_error_t sync_run(struct c_rest_context *ctx) {
  struct sync_state *state;
  c_rest_error_t rc;

  if (!ctx || !ctx->internal_state) {
    return C_REST_ERROR_INVALID_ARG;
  }

  state = (struct sync_state *)ctx->internal_state;
  state->is_running = 1;

  if (ctx->logger.log_cb) {
#if defined(_WIN32)
    rc = ctx->logger.log_cb("SYNC modality run started");
    if (rc != C_REST_OK) {
      return rc;
    }

#endif
  }

  /* Typical synchronous accept loop */
  while (state->is_running) {
    c_rest_socket_t client_sock;
    c_rest_error_t res = C_REST_ERROR_GENERIC;

#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
    if (ctx->cm_env) {
      if (cm_socket_accept(ctx->cm_env, state->server_sock, &client_sock) ==
          0) {
        res = C_REST_OK;
      }
    } else {
      res = c_rest_socket_accept(state->server_sock, &client_sock);
    }
#else
    res = c_rest_socket_accept(state->server_sock, &client_sock);
#endif

    if (res != C_REST_OK) {
      if (!state->is_running)
        continue;
      return res;
    }

    if (client_sock != C_REST_INVALID_SOCKET) {
      struct c_rest_tls_connection *tls_conn = NULL;
      c_rest_error_t tls_res = C_REST_ERROR_GENERIC;

      if (ctx->tls_ctx) {
        tls_res = c_rest_tls_accept(ctx->tls_ctx, client_sock, &tls_conn);
        if (tls_res == C_REST_OK) {
          /* Synchronous request reading and response writing would happen here
           * over TLS. */
          /* For now, we immediately close to prevent leaking. */
          rc = c_rest_tls_close(tls_conn);
          if (rc != C_REST_OK) {
            return rc;
          }

        } else {
          /* Handshake failed, return tls_res */
          return tls_res;
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
        rc = c_rest_socket_close(client_sock);
        if (rc != C_REST_OK) {
          return rc;
        }
      }
#else
      rc = c_rest_socket_close(client_sock);
      if (rc != C_REST_OK) {
        return rc;
      }

#endif
    } else {
      /* Error or interrupt, potentially break loop */
      break;
    }
  }

  state->is_running = 0;

  if (ctx->logger.log_cb) {
#if defined(_WIN32)
    rc = ctx->logger.log_cb("SYNC modality run finished");
    if (rc != C_REST_OK) {
      return rc;
    }

#endif
  }

  return C_REST_OK;
}

static c_rest_error_t sync_stop(struct c_rest_context *ctx) {
  struct sync_state *state;
  c_rest_error_t rc;
  if (!ctx || !ctx->internal_state)
    return C_REST_ERROR_GENERIC;
  state = (struct sync_state *)ctx->internal_state;
  state->is_running = 0;
  if (state->server_sock != C_REST_INVALID_SOCKET) {
#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
    if (ctx->cm_env)
      cm_socket_close(ctx->cm_env, state->server_sock);
    else {
      rc = c_rest_socket_close(state->server_sock);
      if (rc != C_REST_OK)
        return rc;
    }
#else
    rc = c_rest_socket_close(state->server_sock);
    if (rc != C_REST_OK)
      return rc;
#endif
#if defined(_WIN32)
    state->server_sock = C_REST_INVALID_SOCKET;
#endif
  }
  if (ctx->logger.log_cb) {
    rc = ctx->logger.log_cb("SYNC modality stopped");
    if (rc != C_REST_OK)
      return rc;
  }
  return C_REST_OK;
}

const struct c_rest_modality_vtable sync_vtable = {sync_init, sync_destroy,
                                                   sync_run, sync_stop};
