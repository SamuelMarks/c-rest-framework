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

struct multi_thread_state {
  c_rest_socket_t server_sock;
  volatile int is_running;
  /* In a full implementation, we would have a thread pool and work queue here
   */
  c_rest_thread_t *workers;
  int worker_count;
};

static c_rest_error_t multi_thread_init(struct c_rest_context *ctx) {
  struct multi_thread_state *state;
  c_rest_error_t rc;
#if defined(CDD_DOS)
  if (ctx && ctx->logger.log_cb) {
    rc = ctx->logger.log_cb(
        "FATAL ERROR: MULTI_THREAD modality initialized on DOS "
        "which lacks native threading.");
    if (rc != C_REST_OK) {
      return rc;
    }
  }
  return C_REST_ERROR_NOT_SUPPORTED;
#endif

  if (!ctx) {
    return C_REST_ERROR_INVALID_ARG;
  }

  state = (struct multi_thread_state *)ctx->allocator.malloc_cb(
      sizeof(struct multi_thread_state));
  if (!state) {
    return C_REST_ERROR_OOM;
  }

  state->server_sock = C_REST_INVALID_SOCKET;
  state->is_running = 0;
  state->workers = NULL;
  state->worker_count = 4; /* Default to 4 workers for now */

  ctx->internal_state = state;

  /* OpenSSL < 1.1.0 requires locking callbacks for multithreading.
   * We target newer TLS backends (OpenSSL 3+ / mbedTLS 3+), so this is natively
   * safe. C_REST_USE_OPENSSL_LEGACY would register thread-id and lock callbacks
   * here. */
  if (ctx->logger.log_cb) {
    rc = ctx->logger.log_cb("MULTI_THREAD modality initialized");
    if (rc != C_REST_OK) {
      ctx->allocator.free_cb(state);
      ctx->internal_state = NULL;
      return rc;
    }
  }

  return C_REST_OK;
}

static c_rest_error_t multi_thread_destroy(struct c_rest_context *ctx) {
  struct multi_thread_state *state;
  c_rest_error_t rc;
  if (!ctx || !ctx->internal_state) {
    return C_REST_ERROR_INVALID_ARG;
  }

  state = (struct multi_thread_state *)ctx->internal_state;

  if (state->server_sock != C_REST_INVALID_SOCKET) {
#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
    if (ctx->cm_env) {
      rc = cm_socket_close(ctx->cm_env, state->server_sock);
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

  if (state->workers) {
    ctx->allocator.free_cb(state->workers);
  }

  ctx->allocator.free_cb(state);
  ctx->internal_state = NULL;

  /* OpenSSL < 1.1.0 requires locking callbacks for multithreading.
   * We target newer TLS backends (OpenSSL 3+ / mbedTLS 3+), so this is natively
   * safe. C_REST_USE_OPENSSL_LEGACY would register thread-id and lock callbacks
   * here. */
  if (ctx->logger.log_cb) {
    rc = ctx->logger.log_cb("MULTI_THREAD modality destroyed");
    if (rc != C_REST_OK) {
      return rc;
    }
  }

  return C_REST_OK;
}

struct connection_worker_args {
  struct c_rest_context *ctx;
  c_rest_socket_t client_sock;
};

static c_rest_error_t worker_thread(void *arg) {
  struct connection_worker_args *wargs = (struct connection_worker_args *)arg;
  c_rest_error_t rc;
  c_rest_error_t final_rc;

  rc = c_rest_handle_connection(wargs->ctx, wargs->client_sock);
  final_rc = rc;

#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
  if (wargs->ctx->cm_env) {
    rc = cm_socket_close(wargs->ctx->cm_env, wargs->client_sock);
  } else {
    rc = c_rest_socket_close(wargs->client_sock);
    if (rc != C_REST_OK && final_rc == C_REST_OK) {
      final_rc = rc;
    }
  }
#else
  rc = c_rest_socket_close(wargs->client_sock);
  if (rc != C_REST_OK && final_rc == C_REST_OK) {
    final_rc = rc;
  }
#endif

  if (wargs->ctx->allocator.free_cb) {
    wargs->ctx->allocator.free_cb(wargs);
  } else {
#if defined(_WIN32)
    C_REST_FREE((void *)(wargs));
#endif
  }
  return final_rc;
}

static c_rest_error_t multi_thread_run(struct c_rest_context *ctx) {
  struct multi_thread_state *state;
  c_rest_error_t rc;
  if (!ctx || !ctx->internal_state) {
    return C_REST_ERROR_INVALID_ARG;
  }

  state = (struct multi_thread_state *)ctx->internal_state;
  state->is_running = 1;

  /* OpenSSL < 1.1.0 requires locking callbacks for multithreading.
   * We target newer TLS backends (OpenSSL 3+ / mbedTLS 3+), so this is natively
   * safe. C_REST_USE_OPENSSL_LEGACY would register thread-id and lock callbacks
   * here. */
  if (ctx->logger.log_cb) {
#if defined(_WIN32)
    rc = ctx->logger.log_cb("MULTI_THREAD modality run started");
    if (rc != C_REST_OK) {
      return rc;
    }

#endif
  }

  if (state->server_sock == C_REST_INVALID_SOCKET) {
    rc = c_rest_socket_create(&state->server_sock);
    if (rc != C_REST_OK) {
      fprintf(stderr, "MULTI_THREAD: Failed to create socket\n");
      return rc;
    }

    rc = c_rest_socket_bind(state->server_sock, ctx->listen_address,
                            ctx->listen_port);
    if (rc != C_REST_OK) {
      fprintf(stderr, "MULTI_THREAD: Failed to bind socket to %s:%d\n",
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
      fprintf(stderr, "MULTI_THREAD: Failed to listen on socket\n");
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

  /* Thread-per-connection conceptual implementation */
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
      c_rest_thread_t thread_handle;
      struct connection_worker_args *wargs =
          (struct connection_worker_args *)ctx->allocator.malloc_cb(
              sizeof(struct connection_worker_args));
      if (wargs) {
        wargs->ctx = ctx;
        wargs->client_sock = client_sock;
        rc = c_rest_thread_create(&thread_handle, worker_thread, wargs);
#if defined(_WIN32)
        if (rc != C_REST_OK) {
          fprintf(stderr, "MULTI_THREAD: Failed to create thread\n");
          /* cleanup and close on failure */
          ctx->allocator.free_cb(wargs);
          rc = c_rest_socket_close(client_sock);
          if (rc != C_REST_OK)
            return rc;
          return rc;
        }
#endif
      } else {
#if defined(_WIN32)
        rc = c_rest_socket_close(client_sock);
        if (rc != C_REST_OK)
          return rc;
#endif
      }
    } else {
      break;
    }
  }

  state->is_running = 0;

  /* OpenSSL < 1.1.0 requires locking callbacks for multithreading.
   * We target newer TLS backends (OpenSSL 3+ / mbedTLS 3+), so this is natively
   * safe. C_REST_USE_OPENSSL_LEGACY would register thread-id and lock callbacks
   * here. */
  if (ctx->logger.log_cb) {
#if defined(_WIN32)
    rc = ctx->logger.log_cb("MULTI_THREAD modality run finished");
    if (rc != C_REST_OK) {
      return rc;
    }

#endif
  }

  return C_REST_OK;
}

static c_rest_error_t multi_thread_stop(struct c_rest_context *ctx) {
  struct multi_thread_state *state;
  c_rest_error_t rc;
  if (!ctx || !ctx->internal_state) {
    return C_REST_ERROR_GENERIC;
  }

  state = (struct multi_thread_state *)ctx->internal_state;
  state->is_running = 0;

  if (state->server_sock != C_REST_INVALID_SOCKET) {
#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
    if (ctx->cm_env) {
      rc = cm_socket_close(ctx->cm_env, state->server_sock);
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
    rc = ctx->logger.log_cb("MULTI_THREAD modality stopped");
    if (rc != C_REST_OK) {
      return rc;
    }

#endif
  }

  return C_REST_OK;
}

const struct c_rest_modality_vtable multi_thread_vtable = {
    multi_thread_init, multi_thread_destroy, multi_thread_run,
    multi_thread_stop};
