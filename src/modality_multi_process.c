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

struct multi_process_state {
  c_rest_socket_t server_sock;
  volatile int is_running;
  c_rest_process_t *workers;
  int worker_count;
};

static NO_DISCARD c_rest_error_t
multi_process_init(struct c_rest_context *ctx) {
  c_rest_error_t rc;
  struct multi_process_state *state;
#if defined(CDD_DOS)
  if (ctx && ctx->logger.log_cb) {
    rc = ctx->logger.log_cb(
        "FATAL ERROR: MULTI_PROCESS modality initialized on DOS "
        "which lacks native multi-process concurrency.");
    if (rc != C_REST_OK) {
      /* log error handled */
    }
  }
  return C_REST_ERROR_NOT_SUPPORTED;
#endif
  if (!ctx)
    return C_REST_ERROR_GENERIC;

  state = (struct multi_process_state *)ctx->allocator.malloc_cb(
      sizeof(struct multi_process_state));
  if (!state)
    return C_REST_ERROR_GENERIC;

  state->server_sock = C_REST_INVALID_SOCKET;
  state->is_running = 0;
  state->workers = NULL;
  state->worker_count = 1;

  ctx->internal_state = state;

  if (ctx->logger.log_cb) {
    rc = ctx->logger.log_cb("MULTI_PROCESS modality initialized");
    if (rc != C_REST_OK) {
      ctx->allocator.free_cb(state);
      ctx->internal_state = NULL;
      return rc;
    }
  }

  return C_REST_OK;
}

static NO_DISCARD c_rest_error_t
multi_process_destroy(struct c_rest_context *ctx) {
  c_rest_error_t rc;
  struct multi_process_state *state;

  if (!ctx || !ctx->internal_state)
    return C_REST_ERROR_GENERIC;

  state = (struct multi_process_state *)ctx->internal_state;

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

  if (state->workers) {
    ctx->allocator.free_cb(state->workers);
  }

  ctx->allocator.free_cb(state);
  ctx->internal_state = NULL;

  if (ctx->logger.log_cb) {
    rc = ctx->logger.log_cb("MULTI_PROCESS modality destroyed");
    if (rc != C_REST_OK)
      return rc;
  }

  return C_REST_OK;
}

static NO_DISCARD c_rest_error_t multi_process_run(struct c_rest_context *ctx) {
  c_rest_error_t rc;
  struct multi_process_state *state;
  if (!ctx || !ctx->internal_state)
    return C_REST_ERROR_INVALID_ARG;

  state = (struct multi_process_state *)ctx->internal_state;
  if (state->worker_count > 1) {
    return C_REST_ERROR_NOT_SUPPORTED;
  }

  state->is_running = 1;

  if (ctx->logger.log_cb) {
    rc = ctx->logger.log_cb("MULTI_PROCESS modality run started");
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
    while (state->is_running) {
      c_rest_socket_t client_sock = C_REST_INVALID_SOCKET;
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
        goto mproc_loop_done;
      }
      if (client_sock != C_REST_INVALID_SOCKET) {
        c_rest_error_t h_rc;
        c_rest_error_t cl_rc;
        h_rc = c_rest_handle_connection(ctx, client_sock);
        if (h_rc != C_REST_OK) {
          goto mproc_close_client;
        }
      mproc_close_client:;
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

mproc_loop_done:
  state->is_running = 0;

  if (ctx->logger.log_cb) {
    rc = ctx->logger.log_cb("MULTI_PROCESS modality run finished");
    if (rc != C_REST_OK)
      return rc;
  }

  return C_REST_OK;
}

static NO_DISCARD c_rest_error_t
multi_process_stop(struct c_rest_context *ctx) {
  c_rest_error_t rc;
  struct multi_process_state *state;
  if (!ctx || !ctx->internal_state)
    return C_REST_ERROR_INVALID_ARG;
  state = (struct multi_process_state *)ctx->internal_state;
  state->is_running = 0;
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
    rc = ctx->logger.log_cb("MULTI_PROCESS modality stopped");
    if (rc != C_REST_OK)
      return rc;
  }
  return C_REST_OK;
}

const struct c_rest_modality_vtable multi_process_vtable = {
    multi_process_init, multi_process_destroy, multi_process_run,
    multi_process_stop};
