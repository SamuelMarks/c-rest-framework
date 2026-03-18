/* clang-format off */
#include "c_rest_modality.h"
#include "c_rest_platform.h"

#include <stdlib.h>
#include <stdio.h>
/* clang-format on */

struct single_thread_state {
  c_rest_socket_t server_sock;
  int is_running;
};

static int single_thread_init(struct c_rest_context *ctx) {
  struct single_thread_state *state;
  if (!ctx)
    return 1;

  state = (struct single_thread_state *)ctx->allocator.malloc_cb(
      sizeof(struct single_thread_state));
  if (!state)
    return 1;

  state->server_sock = C_REST_INVALID_SOCKET;
  state->is_running = 0;

  ctx->internal_state = state;

  if (ctx->logger.log_cb) {
    ctx->logger.log_cb("SINGLE_THREAD modality initialized");
  }

  return 0;
}

static int single_thread_destroy(struct c_rest_context *ctx) {
  struct single_thread_state *state;

  if (!ctx || !ctx->internal_state)
    return 1;

  state = (struct single_thread_state *)ctx->internal_state;

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
    ctx->logger.log_cb("SINGLE_THREAD modality destroyed");
  }

  return 0;
}

static int single_thread_run(struct c_rest_context *ctx) {
  struct single_thread_state *state;
  if (!ctx || !ctx->internal_state)
    return 1;

  state = (struct single_thread_state *)ctx->internal_state;
  state->is_running = 1;

  if (ctx->logger.log_cb) {
    ctx->logger.log_cb("SINGLE_THREAD modality run started");
  }

  if (state->server_sock == C_REST_INVALID_SOCKET) {
    if (c_rest_socket_create(&state->server_sock) != 0) {
      fprintf(stderr, "SINGLE_THREAD: Failed to create socket\n");
      return 1;
    }

    if (c_rest_socket_bind(state->server_sock, ctx->listen_address, ctx->listen_port) != 0) {
      fprintf(stderr, "SINGLE_THREAD: Failed to bind socket to %s:%d\n", ctx->listen_address, ctx->listen_port);
      c_rest_socket_close(state->server_sock);
      state->server_sock = C_REST_INVALID_SOCKET;
      return 1;
    }

    if (c_rest_socket_listen(state->server_sock, 128) != 0) {
      fprintf(stderr, "SINGLE_THREAD: Failed to listen on socket\n");
      c_rest_socket_close(state->server_sock);
      state->server_sock = C_REST_INVALID_SOCKET;
      return 1;
    }
  }

  /* Similar to SYNC, but conceptually could use an event loop.
     We will implement a blocking model for now to fulfill the immediate
     synchronous/single-thread requirements. */
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
      c_rest_handle_connection(ctx, client_sock);
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
      break;
    }
  }

  state->is_running = 0;

  if (ctx->logger.log_cb) {
    ctx->logger.log_cb("SINGLE_THREAD modality run finished");
  }

  return 0;
}

static int single_thread_stop(struct c_rest_context *ctx) {
  struct single_thread_state *state;

  if (!ctx || !ctx->internal_state)
    return 1;

  state = (struct single_thread_state *)ctx->internal_state;
  state->is_running = 0;

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

  if (ctx->logger.log_cb) {
    ctx->logger.log_cb("SINGLE_THREAD modality stopped");
  }

  return 0;
}

const struct c_rest_modality_vtable single_thread_vtable = {
    single_thread_init, single_thread_destroy, single_thread_run, single_thread_stop};
