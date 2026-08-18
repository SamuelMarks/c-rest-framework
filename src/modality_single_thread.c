#include "c_rest_testing_mocks.h"
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
c_rest_error_t mock_c_rest_handle_connection(struct c_rest_context *ctx, c_rest_socket_t sock);

#define c_rest_socket_create mock_c_rest_socket_create
#define c_rest_socket_bind mock_c_rest_socket_bind
#define c_rest_socket_listen mock_c_rest_socket_listen
#define c_rest_socket_accept mock_c_rest_socket_accept
#define c_rest_thread_create mock_c_rest_thread_create
#define c_rest_socket_close mock_c_rest_socket_close
#define c_rest_tls_accept mock_c_rest_tls_accept
#define c_rest_tls_close mock_c_rest_tls_close
#define c_rest_handle_connection mock_c_rest_handle_connection
#endif
#include "c_rest_platform.h"

#include <stdlib.h>
#include "c_rest_log.h"
#include <stdio.h>
/* clang-format on */

struct single_thread_state {
  c_rest_socket_t server_sock;
  volatile int is_running;
};

static c_rest_error_t single_thread_init(struct c_rest_context *ctx) {
  struct single_thread_state *state;
  c_rest_error_t rc;

  if (!ctx) {
    return C_REST_ERROR_INVALID_ARG;
  }

  state = (struct single_thread_state *)ctx->allocator.malloc_cb(
      sizeof(struct single_thread_state));
  if (!state) {
    return C_REST_ERROR_OOM;
  }

  state->server_sock = C_REST_INVALID_SOCKET;
  state->is_running = 0;

  ctx->internal_state = state;

  if (ctx->logger.log_cb) {
    rc = ctx->logger.log_cb("SINGLE_THREAD modality initialized");
    if (rc != C_REST_OK) {
      ctx->allocator.free_cb(state);
      ctx->internal_state = NULL;
      return rc;
    }
  }

  return C_REST_OK;
}

static c_rest_error_t single_thread_destroy(struct c_rest_context *ctx) {
  struct single_thread_state *state;
  c_rest_error_t rc;

  if (!ctx || !ctx->internal_state) {
    return C_REST_ERROR_INVALID_ARG;
  }

  state = (struct single_thread_state *)ctx->internal_state;

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
    rc = ctx->logger.log_cb("SINGLE_THREAD modality destroyed");
    if (rc != C_REST_OK) {
      return rc;
    }
  }

  return C_REST_OK;
}

static c_rest_error_t single_thread_run(struct c_rest_context *ctx) {
  struct single_thread_state *state;
  c_rest_error_t rc;

  if (!ctx || !ctx->internal_state) {
    return C_REST_ERROR_INVALID_ARG;
  }

  state = (struct single_thread_state *)ctx->internal_state;
  state->is_running = 1;

  if (ctx->logger.log_cb) {
#if defined(_WIN32)
    rc = ctx->logger.log_cb("SINGLE_THREAD modality run started");
    if (rc != C_REST_OK) {
      return rc;
    }

#endif
  }

  if (state->server_sock == C_REST_INVALID_SOCKET) {
    rc = c_rest_socket_create(&state->server_sock);
    if (rc != C_REST_OK) {
      fprintf(stderr, "SINGLE_THREAD: Failed to create socket\n");
      return rc;
    }

    rc = c_rest_socket_bind(state->server_sock, ctx->listen_address,
                            ctx->listen_port);
    if (rc != C_REST_OK) {
      fprintf(stderr, "SINGLE_THREAD: Failed to bind socket to %s:%d\n",
              ctx->listen_address, ctx->listen_port);
      {
        c_rest_error_t close_rc = c_rest_socket_close(state->server_sock);
        if (close_rc != C_REST_OK) {
          return close_rc;
        }
      }
      state->server_sock = C_REST_INVALID_SOCKET;
      return rc;
    }

    rc = c_rest_socket_listen(state->server_sock, 128);
    if (rc != C_REST_OK) {
      fprintf(stderr, "SINGLE_THREAD: Failed to listen on socket\n");
      {
        c_rest_error_t close_rc = c_rest_socket_close(state->server_sock);
        if (close_rc != C_REST_OK) {
          return close_rc;
        }
      }
      state->server_sock = C_REST_INVALID_SOCKET;
      return rc;
    }
  }

  /* Similar to SYNC, but conceptually could use an event loop.
     We will implement a blocking model for now to fulfill the immediate
     synchronous/single-thread requirements. */
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
      printf("HANDLING CONNECTION!\n");
      rc = c_rest_handle_connection(ctx, client_sock);
      if (rc != C_REST_OK) {
        return rc;
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
      break;
    }
  }

  state->is_running = 0;

  if (ctx->logger.log_cb) {
#if defined(_WIN32)
    rc = ctx->logger.log_cb("SINGLE_THREAD modality run finished");
    if (rc != C_REST_OK) {
      return rc;
    }

#endif
  }

  return C_REST_OK;
}
static c_rest_error_t single_thread_stop(struct c_rest_context *ctx) {
  struct single_thread_state *state;
  c_rest_error_t rc;

  if (!ctx || !ctx->internal_state)
    return C_REST_ERROR_GENERIC;

  state = (struct single_thread_state *)ctx->internal_state;
  state->is_running = 0;

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

  if (ctx->logger.log_cb) {
#if defined(_WIN32)
    rc = ctx->logger.log_cb("SINGLE_THREAD modality stopped");
    if (rc != C_REST_OK) {
      return rc;
    }

#endif
  }

  return C_REST_OK;
}

const struct c_rest_modality_vtable single_thread_vtable = {
    single_thread_init, single_thread_destroy, single_thread_run,
    single_thread_stop};
