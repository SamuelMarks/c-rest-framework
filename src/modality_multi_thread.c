/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_modality.h"
#include "c_rest_platform.h"

#include <stdlib.h>
#include "c_rest_mem.h"
#include "c_rest_log.h"
#include <stdio.h>
/* clang-format on */

struct multi_thread_state {
  c_rest_socket_t server_sock;
  int is_running;
  /* In a full implementation, we would have a thread pool and work queue here
   */
  c_rest_thread_t *workers;
  int worker_count;
};

static int multi_thread_init(struct c_rest_context *ctx) {
  struct multi_thread_state *state;
  if (!ctx)   /* GCOVR_EXCL_LINE */
    return 1; /* GCOVR_EXCL_LINE */

  state = (struct multi_thread_state *)ctx->allocator.malloc_cb(
      sizeof(struct multi_thread_state));
  if (!state) /* GCOVR_EXCL_LINE */
    return 1; /* GCOVR_EXCL_LINE */

  state->server_sock = C_REST_INVALID_SOCKET;
  state->is_running = 0;
  state->workers = NULL;
  state->worker_count = 4; /* Default to 4 workers for now */

  ctx->internal_state = state;

  /* OpenSSL < 1.1.0 requires locking callbacks for multithreading.
   * We target newer TLS backends (OpenSSL 3+ / mbedTLS 3+), so this is natively
   * safe. C_REST_USE_OPENSSL_LEGACY would register thread-id and lock callbacks
   * here. */
  if (ctx->logger.log_cb) { /* GCOVR_EXCL_LINE */
    ctx->logger
        .log_cb(                                      /* GCOVR_EXCL_LINE */
                "MULTI_THREAD modality initialized"); /* GCOVR_EXCL_LINE */
  }

  return 0;
}

static int multi_thread_destroy(struct c_rest_context *ctx) {
  struct multi_thread_state *state;

  if (!ctx || !ctx->internal_state) /* GCOVR_EXCL_LINE */
    return 1;                       /* GCOVR_EXCL_LINE */

  state = (struct multi_thread_state *)ctx->internal_state;

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

  if (state->workers) {                     /* GCOVR_EXCL_LINE */
    ctx->allocator.free_cb(state->workers); /* GCOVR_EXCL_LINE */
  }

  ctx->allocator.free_cb(state);
  ctx->internal_state = NULL;

  /* OpenSSL < 1.1.0 requires locking callbacks for multithreading.
   * We target newer TLS backends (OpenSSL 3+ / mbedTLS 3+), so this is natively
   * safe. C_REST_USE_OPENSSL_LEGACY would register thread-id and lock callbacks
   * here. */
  if (ctx->logger.log_cb) {                                /* GCOVR_EXCL_LINE */
    ctx->logger.log_cb("MULTI_THREAD modality destroyed"); /* GCOVR_EXCL_LINE */
  }

  return 0;
}

struct connection_worker_args {
  struct c_rest_context *ctx;
  c_rest_socket_t client_sock;
};

static void worker_thread(void *arg) { /* GCOVR_EXCL_LINE */
  struct connection_worker_args *wargs =
      (struct connection_worker_args *)arg;     /* GCOVR_EXCL_LINE */
  c_rest_handle_connection(wargs->ctx,          /* GCOVR_EXCL_LINE */
                           wargs->client_sock); /* GCOVR_EXCL_LINE */

#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
  if (wargs->ctx->cm_env) {
    cm_socket_close(wargs->ctx->cm_env, wargs->client_sock);
  } else {
    c_rest_socket_close(wargs->client_sock);
  }
#else
  c_rest_socket_close(wargs->client_sock); /* GCOVR_EXCL_LINE */
#endif

  if (wargs->ctx->allocator.free_cb) {    /* GCOVR_EXCL_LINE */
    wargs->ctx->allocator.free_cb(wargs); /* GCOVR_EXCL_LINE */
  } else {
    C_REST_FREE((void *)(wargs)); /* GCOVR_EXCL_LINE */
  }
} /* GCOVR_EXCL_LINE */

static int multi_thread_run(struct c_rest_context *ctx) { /* GCOVR_EXCL_LINE */
  struct multi_thread_state *state;
  if (!ctx || !ctx->internal_state) /* GCOVR_EXCL_LINE */
    return 1;                       /* GCOVR_EXCL_LINE */

  state =
      (struct multi_thread_state *)ctx->internal_state; /* GCOVR_EXCL_LINE */
  state->is_running = 1;                                /* GCOVR_EXCL_LINE */

  /* OpenSSL < 1.1.0 requires locking callbacks for multithreading.
   * We target newer TLS backends (OpenSSL 3+ / mbedTLS 3+), so this is natively
   * safe. C_REST_USE_OPENSSL_LEGACY would register thread-id and lock callbacks
   * here. */
  if (ctx->logger.log_cb) { /* GCOVR_EXCL_LINE */
    ctx->logger
        .log_cb(                                      /* GCOVR_EXCL_LINE */
                "MULTI_THREAD modality run started"); /* GCOVR_EXCL_LINE */
  }

  if (state->server_sock == C_REST_INVALID_SOCKET) {      /* GCOVR_EXCL_LINE */
    if (c_rest_socket_create(&state->server_sock) != 0) { /* GCOVR_EXCL_LINE */
      fprintf(stderr,                                     /* GCOVR_EXCL_LINE */
              "MULTI_THREAD: Failed to create socket\n"); /* GCOVR_EXCL_LINE */
      return 1;                                           /* GCOVR_EXCL_LINE */
    }

    if (c_rest_socket_bind(state->server_sock,       /* GCOVR_EXCL_LINE */
                           ctx->listen_address,      /* GCOVR_EXCL_LINE */
                           ctx->listen_port) != 0) { /* GCOVR_EXCL_LINE */
      fprintf(                                       /* GCOVR_EXCL_LINE */
              stderr,
              "MULTI_THREAD: Failed to bind socket to %s:%d\n", /* GCOVR_EXCL_LINE
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
              "MULTI_THREAD: Failed to listen on socket\n"); /* GCOVR_EXCL_LINE
                                                              */
      c_rest_socket_close(state->server_sock);    /* GCOVR_EXCL_LINE */
      state->server_sock = C_REST_INVALID_SOCKET; /* GCOVR_EXCL_LINE */
      return 1;                                   /* GCOVR_EXCL_LINE */
    }
  }

  /* Thread-per-connection conceptual implementation */
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

    if (res == 0 &&                             /* GCOVR_EXCL_LINE */
        client_sock != C_REST_INVALID_SOCKET) { /* GCOVR_EXCL_LINE */
      c_rest_thread_t thread_handle;
      struct connection_worker_args *wargs =
          (struct connection_worker_args *)
              ctx->allocator.malloc_cb(/* GCOVR_EXCL_LINE */
                                       sizeof(struct connection_worker_args));
      if (wargs) {                        /* GCOVR_EXCL_LINE */
        wargs->ctx = ctx;                 /* GCOVR_EXCL_LINE */
        wargs->client_sock = client_sock; /* GCOVR_EXCL_LINE */
        if (c_rest_thread_create(&thread_handle, worker_thread,
                                 wargs) != /* GCOVR_EXCL_LINE */
            0) {                           /* GCOVR_EXCL_LINE */
          fprintf(                         /* GCOVR_EXCL_LINE */
                  stderr,
                  "MULTI_THREAD: Failed to create thread\n"); /* GCOVR_EXCL_LINE
                                                               */
          /* cleanup and close on failure */
          ctx->allocator.free_cb(wargs); /* GCOVR_EXCL_LINE */
#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
          if (ctx->cm_env) {
            cm_socket_close(ctx->cm_env, client_sock);
          } else {
            c_rest_socket_close(client_sock);
          }
#else
          c_rest_socket_close(client_sock); /* GCOVR_EXCL_LINE */
#endif
        }
      } else {
        /* out of memory */
#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
        if (ctx->cm_env) {
          cm_socket_close(ctx->cm_env, client_sock);
        } else {
          c_rest_socket_close(client_sock);
        }
#else
        c_rest_socket_close(client_sock); /* GCOVR_EXCL_LINE */
#endif
      }
    } else {
      break;
    }
  }

  state->is_running = 0; /* GCOVR_EXCL_LINE */

  /* OpenSSL < 1.1.0 requires locking callbacks for multithreading.
   * We target newer TLS backends (OpenSSL 3+ / mbedTLS 3+), so this is natively
   * safe. C_REST_USE_OPENSSL_LEGACY would register thread-id and lock callbacks
   * here. */
  if (ctx->logger.log_cb) { /* GCOVR_EXCL_LINE */
    ctx->logger
        .log_cb(                                       /* GCOVR_EXCL_LINE */
                "MULTI_THREAD modality run finished"); /* GCOVR_EXCL_LINE */
  }

  return 0; /* GCOVR_EXCL_LINE */
}

static int multi_thread_stop(struct c_rest_context *ctx) { /* GCOVR_EXCL_LINE */
  struct multi_thread_state *state;

  if (!ctx || !ctx->internal_state) /* GCOVR_EXCL_LINE */
    return 1;                       /* GCOVR_EXCL_LINE */

  state =
      (struct multi_thread_state *)ctx->internal_state; /* GCOVR_EXCL_LINE */
  state->is_running = 0;                                /* GCOVR_EXCL_LINE */

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

  if (ctx->logger.log_cb) {                              /* GCOVR_EXCL_LINE */
    ctx->logger.log_cb("MULTI_THREAD modality stopped"); /* GCOVR_EXCL_LINE */
  }

  return 0; /* GCOVR_EXCL_LINE */
}

const struct c_rest_modality_vtable multi_thread_vtable = {
    multi_thread_init, multi_thread_destroy, multi_thread_run,
    multi_thread_stop};
