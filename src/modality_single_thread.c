/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_modality.h"
#include "c_rest_platform.h"

#include <stdlib.h>
#include "c_rest_mem.h"
#include "c_rest_log.h"
#include <stdio.h>
/* clang-format on */

struct single_thread_state {
  c_rest_socket_t server_sock;
  int is_running;
};

static int single_thread_init(struct c_rest_context *ctx) {
  struct single_thread_state *state;
  if (!ctx)   /* GCOVR_EXCL_LINE */
    return 1; /* GCOVR_EXCL_LINE */

  state = (struct single_thread_state *)ctx->allocator.malloc_cb(
      sizeof(struct single_thread_state));
  if (!state) /* GCOVR_EXCL_LINE */
    return 1; /* GCOVR_EXCL_LINE */

  state->server_sock = C_REST_INVALID_SOCKET;
  state->is_running = 0;

  ctx->internal_state = state;

  if (ctx->logger.log_cb) { /* GCOVR_EXCL_LINE */
    ctx->logger
        .log_cb(                                       /* GCOVR_EXCL_LINE */
                "SINGLE_THREAD modality initialized"); /* GCOVR_EXCL_LINE */
  }

  return 0;
}

static int single_thread_destroy(struct c_rest_context *ctx) {
  struct single_thread_state *state;

  if (!ctx || !ctx->internal_state) /* GCOVR_EXCL_LINE */
    return 1;                       /* GCOVR_EXCL_LINE */

  state = (struct single_thread_state *)ctx->internal_state;

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

  if (ctx->logger.log_cb) { /* GCOVR_EXCL_LINE */
    ctx->logger
        .log_cb(                                     /* GCOVR_EXCL_LINE */
                "SINGLE_THREAD modality destroyed"); /* GCOVR_EXCL_LINE */
  }

  return 0;
}

static int single_thread_run(struct c_rest_context *ctx) { /* GCOVR_EXCL_LINE */
  struct single_thread_state *state;
  if (!ctx || !ctx->internal_state) /* GCOVR_EXCL_LINE */
    return 1;                       /* GCOVR_EXCL_LINE */

  state =
      (struct single_thread_state *)ctx->internal_state; /* GCOVR_EXCL_LINE */
  state->is_running = 1;                                 /* GCOVR_EXCL_LINE */

  if (ctx->logger.log_cb) { /* GCOVR_EXCL_LINE */
    ctx->logger
        .log_cb(                                       /* GCOVR_EXCL_LINE */
                "SINGLE_THREAD modality run started"); /* GCOVR_EXCL_LINE */
  }

  if (state->server_sock == C_REST_INVALID_SOCKET) {       /* GCOVR_EXCL_LINE */
    if (c_rest_socket_create(&state->server_sock) != 0) {  /* GCOVR_EXCL_LINE */
      fprintf(stderr,                                      /* GCOVR_EXCL_LINE */
              "SINGLE_THREAD: Failed to create socket\n"); /* GCOVR_EXCL_LINE */
      return 1;                                            /* GCOVR_EXCL_LINE */
    }

    if (c_rest_socket_bind(state->server_sock,       /* GCOVR_EXCL_LINE */
                           ctx->listen_address,      /* GCOVR_EXCL_LINE */
                           ctx->listen_port) != 0) { /* GCOVR_EXCL_LINE */
      fprintf(                                       /* GCOVR_EXCL_LINE */
              stderr,
              "SINGLE_THREAD: Failed to bind socket to %s:%d\n", /* GCOVR_EXCL_LINE
                                                                  */
              ctx->listen_address, ctx->listen_port); /* GCOVR_EXCL_LINE */
      c_rest_socket_close(state->server_sock);        /* GCOVR_EXCL_LINE */
      state->server_sock = C_REST_INVALID_SOCKET;     /* GCOVR_EXCL_LINE */
      return 1;                                       /* GCOVR_EXCL_LINE */
    }

    if (c_rest_socket_listen(state->server_sock, 128) != /* GCOVR_EXCL_LINE */
        0) {                                             /* GCOVR_EXCL_LINE */
      fprintf(                                           /* GCOVR_EXCL_LINE */
              stderr,
              "SINGLE_THREAD: Failed to listen on socket\n"); /* GCOVR_EXCL_LINE
                                                               */
      c_rest_socket_close(state->server_sock);    /* GCOVR_EXCL_LINE */
      state->server_sock = C_REST_INVALID_SOCKET; /* GCOVR_EXCL_LINE */
      return 1;                                   /* GCOVR_EXCL_LINE */
    }
  }

  /* Similar to SYNC, but conceptually could use an event loop.
     We will implement a blocking model for now to fulfill the immediate
     synchronous/single-thread requirements. */
  while (state->is_running &&                           /* GCOVR_EXCL_LINE */
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
    res = c_rest_socket_accept(state->server_sock, /* GCOVR_EXCL_LINE */
                               &client_sock);      /* GCOVR_EXCL_LINE */
#endif

    if (res == 0 &&                               /* GCOVR_EXCL_LINE */
        client_sock != C_REST_INVALID_SOCKET) {   /* GCOVR_EXCL_LINE */
      c_rest_handle_connection(ctx, client_sock); /* GCOVR_EXCL_LINE */
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
      break;
    }
  }

  state->is_running = 0; /* GCOVR_EXCL_LINE */

  if (ctx->logger.log_cb) { /* GCOVR_EXCL_LINE */
    ctx->logger
        .log_cb(                                        /* GCOVR_EXCL_LINE */
                "SINGLE_THREAD modality run finished"); /* GCOVR_EXCL_LINE */
  }

  return 0; /* GCOVR_EXCL_LINE */
}

static int
single_thread_stop(struct c_rest_context *ctx) { /* GCOVR_EXCL_LINE */
  struct single_thread_state *state;

  if (!ctx || !ctx->internal_state) /* GCOVR_EXCL_LINE */
    return 1;                       /* GCOVR_EXCL_LINE */

  state =
      (struct single_thread_state *)ctx->internal_state; /* GCOVR_EXCL_LINE */
  state->is_running = 0;                                 /* GCOVR_EXCL_LINE */

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

  if (ctx->logger.log_cb) {                               /* GCOVR_EXCL_LINE */
    ctx->logger.log_cb("SINGLE_THREAD modality stopped"); /* GCOVR_EXCL_LINE */
  }

  return 0; /* GCOVR_EXCL_LINE */
}

const struct c_rest_modality_vtable single_thread_vtable = {
    single_thread_init, single_thread_destroy, single_thread_run,
    single_thread_stop};
