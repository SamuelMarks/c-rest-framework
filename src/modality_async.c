/* clang-format off */
#include "c_rest_testing_mocks.h"
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "c_rest_modality.h"

#ifdef C_REST_TESTING_MALLOC_HOOK
c_rest_error_t mock_c_rest_socket_create(c_rest_socket_t *sock);
c_rest_error_t mock_c_rest_socket_bind(c_rest_socket_t sock, const char *host,
                                       unsigned short port);
c_rest_error_t mock_c_rest_socket_listen(c_rest_socket_t sock, int backlog);
#define c_rest_socket_create mock_c_rest_socket_create
#define c_rest_socket_bind mock_c_rest_socket_bind
#define c_rest_socket_listen mock_c_rest_socket_listen
#define c_rest_socket_accept mock_c_rest_socket_accept
#define c_rest_socket_close mock_c_rest_socket_close
#define c_rest_handle_connection mock_c_rest_handle_connection
#endif

#include "c_rest_platform.h"

#include <stdlib.h>
#include "c_rest_log.h"
/* clang-format on */

struct c_rest_event_loop {
  volatile int is_running;
  ptrdiff_t poll_backend;
};

struct async_state {
  c_rest_socket_t server_sock;
  struct c_rest_event_loop *evloop;
};

static NO_DISCARD c_rest_error_t async_init(struct c_rest_context *ctx) {
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
      ctx->allocator.free_cb(state->evloop);
      ctx->allocator.free_cb(state);
      ctx->internal_state = NULL;
      return rc;
    }
  }
  return C_REST_OK;
}

static NO_DISCARD c_rest_error_t async_destroy(struct c_rest_context *ctx) {
  c_rest_error_t rc;
  struct async_state *state;

  if (!ctx || !ctx->internal_state)
    return C_REST_ERROR_INVALID_ARG;

  state = (struct async_state *)ctx->internal_state;

  if (state->server_sock != C_REST_INVALID_SOCKET) {
#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
    if (ctx->cm_env) {
      rc = cm_socket_close(ctx->cm_env, state->server_sock);
      if (rc != C_REST_OK)
        return rc;
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

static NO_DISCARD c_rest_error_t async_run(struct c_rest_context *ctx) {
  c_rest_error_t rc;
  struct async_state *state;
  if (!ctx || !ctx->internal_state)
    return C_REST_ERROR_INVALID_ARG;

  state = (struct async_state *)ctx->internal_state;
  if (!state->evloop)
    return C_REST_ERROR_INVALID_ARG;

  state->evloop->is_running = 1;

  if (ctx->logger.log_cb) {
    rc = ctx->logger.log_cb("ASYNC modality run started");
    if (rc != C_REST_OK)
      return rc;
  }

  if (state->server_sock == C_REST_INVALID_SOCKET &&
      (ctx->listen_address != NULL || ctx->listen_port != 0)) {
    const char *addr = ctx->listen_address ? ctx->listen_address : "0.0.0.0";
#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
    if (ctx->cm_env) {
      rc = cm_socket_create(ctx->cm_env, &state->server_sock);
      if (rc != C_REST_OK)
        return rc;
      rc = cm_socket_bind(ctx->cm_env, state->server_sock, addr,
                          ctx->listen_port);
      if (rc != C_REST_OK) {
        c_rest_socket_t sock_to_close = state->server_sock;
        c_rest_error_t cl_rc;
        state->server_sock = C_REST_INVALID_SOCKET;
        cl_rc = cm_socket_close(ctx->cm_env, sock_to_close);
        if (cl_rc != C_REST_OK)
          return cl_rc;
        return rc;
      }
      rc = cm_socket_listen(ctx->cm_env, state->server_sock, 128);
      if (rc != C_REST_OK) {
        c_rest_socket_t sock_to_close = state->server_sock;
        c_rest_error_t cl_rc;
        state->server_sock = C_REST_INVALID_SOCKET;
        cl_rc = cm_socket_close(ctx->cm_env, sock_to_close);
        if (cl_rc != C_REST_OK)
          return cl_rc;
        return rc;
      }
    } else
#endif
    {
      rc = c_rest_socket_create(&state->server_sock);
      if (rc != C_REST_OK)
        return rc;
      rc = c_rest_socket_bind(state->server_sock, addr, ctx->listen_port);
      if (rc != C_REST_OK) {
        c_rest_socket_t sock_to_close = state->server_sock;
        c_rest_error_t cl_rc;
        state->server_sock = C_REST_INVALID_SOCKET;
        cl_rc = c_rest_socket_close(sock_to_close);
        if (cl_rc != C_REST_OK)
          return cl_rc;
        return rc;
      }
      rc = c_rest_socket_listen(state->server_sock, 128);
      if (rc != C_REST_OK) {
        c_rest_socket_t sock_to_close = state->server_sock;
        c_rest_error_t cl_rc;
        state->server_sock = C_REST_INVALID_SOCKET;
        cl_rc = c_rest_socket_close(sock_to_close);
        if (cl_rc != C_REST_OK)
          return cl_rc;
        return rc;
      }
    }
  }

  if (state->server_sock != C_REST_INVALID_SOCKET) {
    while (state->evloop->is_running) {
      c_rest_socket_t client_sock = C_REST_INVALID_SOCKET;
      state->evloop->poll_backend++;
#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
      if (ctx->cm_env) {
        rc = cm_socket_accept(ctx->cm_env, state->server_sock, &client_sock);
      } else {
        rc = c_rest_socket_accept(state->server_sock, &client_sock);
      }
#else
      rc = c_rest_socket_accept(state->server_sock, &client_sock);
#endif
      if (rc != C_REST_OK) {
        goto async_loop_done;
      }
      if (client_sock != C_REST_INVALID_SOCKET) {
        c_rest_error_t h_rc;
        c_rest_error_t cl_rc;
        h_rc = c_rest_handle_connection(ctx, client_sock);
        if (h_rc != C_REST_OK) {
          goto async_close_client;
        }
      async_close_client:;
#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
        if (ctx->cm_env) {
          cl_rc = cm_socket_close(ctx->cm_env, client_sock);
        } else {
          cl_rc = c_rest_socket_close(client_sock);
        }
#else
        cl_rc = c_rest_socket_close(client_sock);
#endif
        if (cl_rc != C_REST_OK)
          return cl_rc;
        if (h_rc != C_REST_OK)
          return h_rc;
      }
    }
  }

async_loop_done:
  state->evloop->is_running = 0;

  if (ctx->logger.log_cb) {
    rc = ctx->logger.log_cb("ASYNC modality run finished");
    if (rc != C_REST_OK)
      return rc;
  }
  return C_REST_OK;
}

static NO_DISCARD c_rest_error_t async_stop(struct c_rest_context *ctx) {
  c_rest_error_t rc;
  struct async_state *state;
  if (!ctx || !ctx->internal_state)
    return C_REST_ERROR_INVALID_ARG;
  state = (struct async_state *)ctx->internal_state;
  if (state->evloop) {
    state->evloop->is_running = 0;
  }
  if (state->server_sock != C_REST_INVALID_SOCKET) {
#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
    if (ctx->cm_env) {
      rc = cm_socket_close(ctx->cm_env, state->server_sock);
      if (rc != C_REST_OK)
        return rc;
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
  if (ctx->logger.log_cb) {
    rc = ctx->logger.log_cb("ASYNC modality stopped");
    if (rc != C_REST_OK)
      return rc;
  }
  return C_REST_OK;
}

const struct c_rest_modality_vtable async_vtable = {async_init, async_destroy,
                                                    async_run, async_stop};
