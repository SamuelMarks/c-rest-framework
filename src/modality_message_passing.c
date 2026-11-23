/* clang-format off */
#include "c_rest_testing_mocks.h"
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "c_rest_modality.h"
#include "c_rest_ts_queue.h"

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
#define c_rest_ts_queue_pop mock_c_rest_ts_queue_pop
#endif

#include "c_rest_platform.h"

#include <stdlib.h>
#include "c_rest_log.h"
/* clang-format on */

struct mp_conn_msg {
  struct c_rest_context *ctx;
  c_rest_socket_t client_sock;
};

struct message_passing_state {
  c_rest_socket_t server_sock;
  volatile int is_running;
};

static NO_DISCARD c_rest_error_t
message_passing_init(struct c_rest_context *ctx) {
  c_rest_error_t rc;
  struct message_passing_state *state;
  if (!ctx)
    return C_REST_ERROR_INVALID_ARG;

  state = (struct message_passing_state *)ctx->allocator.malloc_cb(
      sizeof(struct message_passing_state));
  if (!state)
    return C_REST_ERROR_OOM;

  state->server_sock = C_REST_INVALID_SOCKET;
  state->is_running = 0;

  ctx->internal_state = state;

  if (ctx->logger.log_cb) {
    rc = ctx->logger.log_cb("MESSAGE_PASSING modality initialized");
    if (rc != C_REST_OK) {
      ctx->allocator.free_cb(state);
      ctx->internal_state = NULL;
      return rc;
    }
  }
  return C_REST_OK;
}

static NO_DISCARD c_rest_error_t
message_passing_destroy(struct c_rest_context *ctx) {
  c_rest_error_t rc;
  struct message_passing_state *state;

  if (!ctx || !ctx->internal_state)
    return C_REST_ERROR_INVALID_ARG;

  state = (struct message_passing_state *)ctx->internal_state;

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
    rc = ctx->logger.log_cb("MESSAGE_PASSING modality destroyed");
    if (rc != C_REST_OK)
      return rc;
  }
  return C_REST_OK;
}

static NO_DISCARD c_rest_error_t
message_passing_run(struct c_rest_context *ctx) {
  c_rest_error_t rc;
  struct message_passing_state *state;
  if (!ctx || !ctx->internal_state)
    return C_REST_ERROR_INVALID_ARG;

  state = (struct message_passing_state *)ctx->internal_state;
  state->is_running = 1;

  if (ctx->logger.log_cb) {
    rc = ctx->logger.log_cb("MESSAGE_PASSING modality run started");
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
    c_rest_ts_queue queue;
    rc = c_rest_ts_queue_init(&queue);
    if (rc != C_REST_OK)
      return rc;

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
        goto mp_loop_done;
      }
      if (client_sock != C_REST_INVALID_SOCKET) {
        struct mp_conn_msg *msg = NULL;
        void *popped = NULL;
        c_rest_error_t h_rc;
        c_rest_error_t cl_rc;
        c_rest_socket_t sock_for_conn;
        msg = (struct mp_conn_msg *)ctx->allocator.malloc_cb(
            sizeof(struct mp_conn_msg));
        if (!msg) {
          c_rest_error_t sc_rc;
          c_rest_error_t qd_rc;
          sc_rc = c_rest_socket_close(client_sock);
          if (sc_rc != C_REST_OK) {
            goto mp_oom_destroy_q;
          }
        mp_oom_destroy_q:;
          qd_rc = c_rest_ts_queue_destroy(&queue, NULL);
          if (sc_rc != C_REST_OK)
            return sc_rc;
          if (qd_rc != C_REST_OK)
            return qd_rc;
          return C_REST_ERROR_OOM;
        }
        msg->ctx = ctx;
        msg->client_sock = client_sock;
        rc = c_rest_ts_queue_push(&queue, msg);
        if (rc != C_REST_OK) {
          c_rest_error_t sc_rc;
          c_rest_error_t qd_rc;
          ctx->allocator.free_cb(msg);
          sc_rc = c_rest_socket_close(client_sock);
          if (sc_rc != C_REST_OK) {
            goto mp_push_destroy_q;
          }
        mp_push_destroy_q:;
          qd_rc = c_rest_ts_queue_destroy(&queue, NULL);
          if (sc_rc != C_REST_OK)
            return sc_rc;
          if (qd_rc != C_REST_OK)
            return qd_rc;
          return rc;
        }
        rc = c_rest_ts_queue_pop(&queue, &popped);
        if (rc != C_REST_OK) {
          c_rest_error_t qd_rc =
              c_rest_ts_queue_destroy(&queue, ctx->allocator.free_cb);
          if (qd_rc != C_REST_OK)
            return qd_rc;
          return rc;
        }
        msg = (struct mp_conn_msg *)popped;
        sock_for_conn = msg->client_sock;
        ctx->allocator.free_cb(msg);
        h_rc = c_rest_handle_connection(ctx, sock_for_conn);
        if (h_rc != C_REST_OK) {
          goto mp_close_client;
        }
      mp_close_client:;
#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
        if (ctx->cm_env) {
          cl_rc = cm_socket_close(ctx->cm_env, sock_for_conn);
        } else {
          cl_rc = c_rest_socket_close(sock_for_conn);
        }
#else
        cl_rc = c_rest_socket_close(sock_for_conn);
#endif
        if (cl_rc != C_REST_OK) {
          c_rest_error_t qd_rc = c_rest_ts_queue_destroy(&queue, NULL);
          if (qd_rc != C_REST_OK)
            return qd_rc;
          return cl_rc;
        }
        if (h_rc != C_REST_OK) {
          c_rest_error_t qd_rc = c_rest_ts_queue_destroy(&queue, NULL);
          if (qd_rc != C_REST_OK)
            return qd_rc;
          return h_rc;
        }
      }
    }
  mp_loop_done:;

    rc = c_rest_ts_queue_close(&queue);
    if (rc != C_REST_OK) {
      c_rest_error_t qd_rc = c_rest_ts_queue_destroy(&queue, NULL);
      if (qd_rc != C_REST_OK)
        return qd_rc;
      return rc;
    }
    rc = c_rest_ts_queue_destroy(&queue, NULL);
    if (rc != C_REST_OK)
      return rc;
  }

  state->is_running = 0;

  if (ctx->logger.log_cb) {
    rc = ctx->logger.log_cb("MESSAGE_PASSING modality run finished");
    if (rc != C_REST_OK)
      return rc;
  }

  return C_REST_OK;
}

static NO_DISCARD c_rest_error_t
message_passing_stop(struct c_rest_context *ctx) {
  c_rest_error_t rc;
  struct message_passing_state *state;
  if (!ctx || !ctx->internal_state)
    return C_REST_ERROR_INVALID_ARG;
  state = (struct message_passing_state *)ctx->internal_state;
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
    rc = ctx->logger.log_cb("MESSAGE_PASSING modality stopped");
    if (rc != C_REST_OK)
      return rc;
  }
  return C_REST_OK;
}

const struct c_rest_modality_vtable message_passing_vtable = {
    message_passing_init, message_passing_destroy, message_passing_run,
    message_passing_stop};
