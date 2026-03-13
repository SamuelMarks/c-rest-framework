/* clang-format off */
#include "c_rest_modality.h"
#include "c_rest_platform.h"

#include <stdlib.h>
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
  if (!ctx)
    return 1;

  state = (struct multi_thread_state *)ctx->allocator.malloc_cb(
      sizeof(struct multi_thread_state));
  if (!state)
    return 1;

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
    ctx->logger.log_cb("MULTI_THREAD modality initialized");
  }

  return 0;
}

static int multi_thread_destroy(struct c_rest_context *ctx) {
  struct multi_thread_state *state;

  if (!ctx || !ctx->internal_state)
    return 1;

  state = (struct multi_thread_state *)ctx->internal_state;

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
    ctx->logger.log_cb("MULTI_THREAD modality destroyed");
  }

  return 0;
}

static int multi_thread_run(struct c_rest_context *ctx) {
  struct multi_thread_state *state;
  if (!ctx || !ctx->internal_state)
    return 1;

  state = (struct multi_thread_state *)ctx->internal_state;
  state->is_running = 1;

  /* OpenSSL < 1.1.0 requires locking callbacks for multithreading.
   * We target newer TLS backends (OpenSSL 3+ / mbedTLS 3+), so this is natively
   * safe. C_REST_USE_OPENSSL_LEGACY would register thread-id and lock callbacks
   * here. */
  if (ctx->logger.log_cb) {
    ctx->logger.log_cb("MULTI_THREAD modality run started");
  }

  /* Conceptually:
   * 1. Spawn workers
   * 2. Main thread blocks on accept()
   * 3. Dispatch accepted sockets to worker queues
   */

  state->is_running = 0;

  /* OpenSSL < 1.1.0 requires locking callbacks for multithreading.
   * We target newer TLS backends (OpenSSL 3+ / mbedTLS 3+), so this is natively
   * safe. C_REST_USE_OPENSSL_LEGACY would register thread-id and lock callbacks
   * here. */
  if (ctx->logger.log_cb) {
    ctx->logger.log_cb("MULTI_THREAD modality run finished");
  }

  return 0;
}

const struct c_rest_modality_vtable multi_thread_vtable = {
    multi_thread_init, multi_thread_destroy, multi_thread_run};
