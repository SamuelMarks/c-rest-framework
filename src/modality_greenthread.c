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

#define C_REST_GREENTHREAD_RING_CAP 64

struct greenthread_state {
  c_rest_socket_t server_sock;
  volatile int is_running;
  int active_tasks;
  int ring_head;
  int ring_tail;
  c_rest_socket_t ready_ring[C_REST_GREENTHREAD_RING_CAP];
};

static NO_DISCARD c_rest_error_t greenthread_init(struct c_rest_context *ctx) {
  c_rest_error_t rc;
  struct greenthread_state *state;
  if (!ctx)
    return C_REST_ERROR_INVALID_ARG;

  state = (struct greenthread_state *)ctx->allocator.malloc_cb(
      sizeof(struct greenthread_state));
  if (!state)
    return C_REST_ERROR_OOM;

  state->server_sock = C_REST_INVALID_SOCKET;
  state->is_running = 0;
  state->active_tasks = 0;
  state->ring_head = 0;
  state->ring_tail = 0;

  ctx->internal_state = state;

  if (ctx->logger.log_cb) {
    rc = ctx->logger.log_cb("GREENTHREAD modality initialized");
    if (rc != C_REST_OK) {
      ctx->allocator.free_cb(state);
      ctx->internal_state = NULL;
      return rc;
    }
  }
  return C_REST_OK;
}

static NO_DISCARD c_rest_error_t
greenthread_destroy(struct c_rest_context *ctx) {
  c_rest_error_t rc;
  struct greenthread_state *state;

  if (!ctx || !ctx->internal_state)
    return C_REST_ERROR_INVALID_ARG;

  state = (struct greenthread_state *)ctx->internal_state;

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

  ctx->allocator.free_cb(state);
  ctx->internal_state = NULL;

  if (ctx->logger.log_cb) {
    rc = ctx->logger.log_cb("GREENTHREAD modality destroyed");
    if (rc != C_REST_OK)
      return rc;
  }
  return C_REST_OK;
}

static NO_DISCARD c_rest_error_t greenthread_run(struct c_rest_context *ctx) {
  c_rest_error_t rc;
  struct greenthread_state *state;
  if (!ctx || !ctx->internal_state)
    return C_REST_ERROR_INVALID_ARG;

  state = (struct greenthread_state *)ctx->internal_state;
  state->is_running = 1;
  state->active_tasks = 0;
  state->ring_head = 0;
  state->ring_tail = 0;

  if (ctx->logger.log_cb) {
    rc = ctx->logger.log_cb("GREENTHREAD modality run started");
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
        goto greenthread_loop_done;
      }
      if (client_sock != C_REST_INVALID_SOCKET) {
        state->ready_ring[state->ring_tail] = client_sock;
        state->ring_tail = (state->ring_tail + 1) % C_REST_GREENTHREAD_RING_CAP;
        state->active_tasks++;
      }

      while (state->active_tasks > 0) {
        c_rest_socket_t task_sock = state->ready_ring[state->ring_head];
        c_rest_error_t h_rc;
        c_rest_error_t cl_rc;
        state->ring_head = (state->ring_head + 1) % C_REST_GREENTHREAD_RING_CAP;
        state->active_tasks--;
        h_rc = c_rest_handle_connection(ctx, task_sock);
        if (h_rc != C_REST_OK) {
          goto gt_close_task;
        }
      gt_close_task:;
#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
        if (ctx->cm_env) {
          cl_rc = cm_socket_close(ctx->cm_env, task_sock);
        } else {
          cl_rc = c_rest_socket_close(task_sock);
        }
#else
        cl_rc = c_rest_socket_close(task_sock);
#endif
        if (cl_rc != C_REST_OK)
          return cl_rc;
        if (h_rc != C_REST_OK)
          return h_rc;
      }
    }
  }

greenthread_loop_done:
  state->is_running = 0;

  if (ctx->logger.log_cb) {
    rc = ctx->logger.log_cb("GREENTHREAD modality run finished");
    if (rc != C_REST_OK)
      return rc;
  }
  return C_REST_OK;
}

static NO_DISCARD c_rest_error_t greenthread_stop(struct c_rest_context *ctx) {
  c_rest_error_t rc;
  struct greenthread_state *state;
  if (!ctx || !ctx->internal_state)
    return C_REST_ERROR_INVALID_ARG;
  state = (struct greenthread_state *)ctx->internal_state;
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
    rc = ctx->logger.log_cb("GREENTHREAD modality stopped");
    if (rc != C_REST_OK)
      return rc;
  }
  return C_REST_OK;
}

const struct c_rest_modality_vtable greenthread_vtable = {
    greenthread_init, greenthread_destroy, greenthread_run, greenthread_stop};
