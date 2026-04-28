/* clang-format off */
#include "c_rest_hot_reload.h"
#include "c_rest_mem.h"
#include "c_rest_platform.h"

#include <stdlib.h>
#include <string.h>
#include "c_rest_mem.h"
#include "c_rest_log.h"
#include <sys/stat.h>
#include <time.h>

typedef enum c_rest_hot_reload_state {
  C_REST_HOT_RELOAD_STATE_INIT = 0,
  C_REST_HOT_RELOAD_STATE_WATCHING,
  C_REST_HOT_RELOAD_STATE_CHANGED,
  C_REST_HOT_RELOAD_STATE_RELOADING,
  C_REST_HOT_RELOAD_STATE_STOPPED
} c_rest_hot_reload_state_t;

struct c_rest_hot_reload_ctx {
  char **watched_paths;
  time_t *last_modified_times;
  size_t watch_count;
  size_t watch_capacity;
  c_rest_hot_reload_state_t state;
  struct c_rest_logger *logger;
  c_rest_thread_t watcher_thread;
  c_rest_hot_reload_callback_t on_reload;
  void *user_data;
#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
  cm_env_t cm_env;
#endif
};

static void hot_reload_log(c_rest_hot_reload_ctx_t *ctx, const char *msg) {
  if (ctx && ctx->logger && ctx->logger->log_cb) {
    ctx->logger->log_cb(msg);
  }
}

static int get_file_mtime(c_rest_hot_reload_ctx_t *ctx, const char *path,
                          time_t *out_mtime) {
  (void)ctx;
#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
  if (ctx && ctx->cm_env) {
    unsigned long mtime_ul;
    if (cm_file_get_mtime(ctx->cm_env, path, &mtime_ul) == 0) {
      *out_mtime = (time_t)mtime_ul;
      return 0;
    }
  }
#endif
#if defined(_MSC_VER)
  struct _stat64 st;
  if (_stat64(path, &st) == 0) {
    *out_mtime = st.st_mtime;
    return 0;
  }
#else
  struct stat st;
  if (stat(path, &st) == 0) {
    *out_mtime = st.st_mtime;
    return 0;
  }
#endif
  *out_mtime = 0;
  return -1;
}

static void sleep_seconds(c_rest_hot_reload_ctx_t *ctx, int seconds) {
  (void)ctx;
#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
  if (ctx && ctx->cm_env) {
    cm_thread_sleep_ms(ctx->cm_env, (unsigned long)(seconds * 1000));
    return;
  }
#endif
  time_t start = time(NULL);
  while (time(NULL) - start < seconds) {
    /* busy wait, ideally replaced by platform sleep */
  }
}

static void watcher_thread_func(void *arg) {
  c_rest_hot_reload_ctx_t *ctx = (c_rest_hot_reload_ctx_t *)arg;
  while (ctx->state == C_REST_HOT_RELOAD_STATE_WATCHING) {
    c_rest_hot_reload_poll(ctx, ctx->on_reload, ctx->user_data);
    sleep_seconds(ctx, 1);
  }
}

#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
int c_rest_hot_reload_set_multiplatform_env(c_rest_hot_reload_ctx_t *ctx,
                                            cm_env_t env) {
  if (!ctx)
    return C_REST_HOT_RELOAD_ERR_PARAM;
  ctx->cm_env = env;
  return C_REST_HOT_RELOAD_SUCCESS;
}
#endif

int c_rest_hot_reload_init(c_rest_hot_reload_ctx_t **out_ctx,
                           struct c_rest_logger *logger) {
  int err;

  if (!out_ctx) {
    return C_REST_HOT_RELOAD_ERR_PARAM;
  }

  err = C_REST_MALLOC(sizeof(c_rest_hot_reload_ctx_t), (void **)out_ctx);
  if (err != 0) {
    return C_REST_HOT_RELOAD_ERR_ALLOC;
  }

  (*out_ctx)->watched_paths = NULL;
  (*out_ctx)->last_modified_times = NULL;
  (*out_ctx)->watch_count = 0;
  (*out_ctx)->watch_capacity = 0;
  (*out_ctx)->state = C_REST_HOT_RELOAD_STATE_INIT;
  (*out_ctx)->logger = logger;
  (*out_ctx)->watcher_thread = (c_rest_thread_t)0;
  (*out_ctx)->on_reload = NULL;
  (*out_ctx)->user_data = NULL;
#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
  (*out_ctx)->cm_env = NULL;
#endif

  hot_reload_log(*out_ctx, "[HOT RELOAD] Initialized context");

  return C_REST_HOT_RELOAD_SUCCESS;
}

int c_rest_hot_reload_add_watch(c_rest_hot_reload_ctx_t *ctx,
                                const char *path) {
  size_t path_len;
  char *path_copy;
  time_t current_mtime;
  int err;

  if (!ctx || !path) {
    return C_REST_HOT_RELOAD_ERR_PARAM;
  }

  if (ctx->watch_count == ctx->watch_capacity) {
    size_t new_cap = ctx->watch_capacity == 0 ? 8 : ctx->watch_capacity * 2;
    char **new_paths = NULL;
    time_t *new_times = NULL;

    if (ctx->watched_paths) {
      err = C_REST_REALLOC(ctx->watched_paths, sizeof(char *) * new_cap,
                           (void **)&new_paths);
      if (err != 0) {
        return C_REST_HOT_RELOAD_ERR_ALLOC;
      }
      ctx->watched_paths = new_paths;

      err = C_REST_REALLOC(ctx->last_modified_times, sizeof(time_t) * new_cap,
                           (void **)&new_times);
      if (err != 0) {
        /* We successfully grew watched_paths but not last_modified_times.
         * The capacity isn't updated yet, so next time it will retry growing.
         * This is a safe state, but to be completely clean we might want to
         * shrink back, or just leave watched_paths slightly larger. Leaving it
         * is safe. */
        return C_REST_HOT_RELOAD_ERR_ALLOC;
      }
      ctx->last_modified_times = new_times;
    } else {
      err = C_REST_MALLOC(sizeof(char *) * new_cap, (void **)&new_paths);
      if (err != 0) {
        return C_REST_HOT_RELOAD_ERR_ALLOC;
      }

      err = C_REST_MALLOC(sizeof(time_t) * new_cap, (void **)&new_times);
      if (err != 0) {
        C_REST_FREE(new_paths);
        return C_REST_HOT_RELOAD_ERR_ALLOC;
      }

      ctx->watched_paths = new_paths;
      ctx->last_modified_times = new_times;
    }

    ctx->watch_capacity = new_cap;
  }

  path_len = strlen(path);
  err = C_REST_MALLOC(path_len + 1, (void **)&path_copy);
  if (err != 0) {
    return C_REST_HOT_RELOAD_ERR_ALLOC;
  }

#if defined(_MSC_VER)
  strcpy_s(path_copy, path_len + 1, path);
#else
  strcpy(path_copy, path);
#endif

  get_file_mtime(ctx, path_copy, &current_mtime);

  ctx->watched_paths[ctx->watch_count] = path_copy;
  ctx->last_modified_times[ctx->watch_count] = current_mtime;
  ctx->watch_count++;

  hot_reload_log(ctx, "[HOT RELOAD] Added watch for path");

  return C_REST_HOT_RELOAD_SUCCESS;
}

int c_rest_hot_reload_poll(c_rest_hot_reload_ctx_t *ctx,
                           c_rest_hot_reload_callback_t on_reload,
                           void *user_data) {
  size_t i;
  int changed = 0;
  time_t current_mtime;

  if (!ctx || !on_reload) {
    return C_REST_HOT_RELOAD_ERR_PARAM;
  }

  for (i = 0; i < ctx->watch_count; ++i) {
    if (get_file_mtime(ctx, ctx->watched_paths[i], &current_mtime) == 0) {
      if (current_mtime != ctx->last_modified_times[i]) {
        ctx->last_modified_times[i] = current_mtime;
        changed = 1;
        hot_reload_log(ctx, "[HOT RELOAD] Change detected on path");
      }
    }
  }

  if (changed) {
    ctx->state = C_REST_HOT_RELOAD_STATE_CHANGED;
    return on_reload(user_data);
  }

  return C_REST_HOT_RELOAD_SUCCESS;
}

int c_rest_hot_reload_start(c_rest_hot_reload_ctx_t *ctx,
                            c_rest_hot_reload_callback_t on_reload,
                            void *user_data) {
  if (!ctx || !on_reload) {
    return C_REST_HOT_RELOAD_ERR_PARAM;
  }

  ctx->on_reload = on_reload;
  ctx->user_data = user_data;
  ctx->state = C_REST_HOT_RELOAD_STATE_WATCHING;

  int thread_err;
#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
  if (ctx->cm_env) {
    thread_err =
        cm_thread_create(ctx->cm_env, (cm_thread_t *)&ctx->watcher_thread,
                         watcher_thread_func, ctx);
  } else {
    thread_err =
        c_rest_thread_create(&ctx->watcher_thread, watcher_thread_func, ctx);
  }
#else
  thread_err =
      c_rest_thread_create(&ctx->watcher_thread, watcher_thread_func, ctx);
#endif

  if (thread_err != 0) {
    ctx->state = C_REST_HOT_RELOAD_STATE_STOPPED;
    hot_reload_log(ctx, "[HOT RELOAD] Failed to start watcher thread");
    return C_REST_HOT_RELOAD_ERR_SYSTEM;
  }

  hot_reload_log(ctx, "[HOT RELOAD] Started watcher thread");
  return C_REST_HOT_RELOAD_SUCCESS;
}

int c_rest_hot_reload_destroy(c_rest_hot_reload_ctx_t *ctx) {
  size_t i;

  if (!ctx) {
    return C_REST_HOT_RELOAD_ERR_PARAM;
  }

  ctx->state = C_REST_HOT_RELOAD_STATE_STOPPED;
  if (ctx->watcher_thread) {
#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
    if (ctx->cm_env) {
      cm_thread_join(ctx->cm_env, (cm_thread_t)ctx->watcher_thread);
    } else {
      c_rest_thread_join(ctx->watcher_thread);
    }
#else
    c_rest_thread_join(ctx->watcher_thread);
#endif
    ctx->watcher_thread = (c_rest_thread_t)0;
  }

  if (ctx->watched_paths) {
    for (i = 0; i < ctx->watch_count; ++i) {
      if (ctx->watched_paths[i]) {
        C_REST_FREE(ctx->watched_paths[i]);
      }
    }
    C_REST_FREE(ctx->watched_paths);
  }

  if (ctx->last_modified_times) {
    C_REST_FREE(ctx->last_modified_times);
  }

  C_REST_FREE(ctx);

  return C_REST_HOT_RELOAD_SUCCESS;
}

#ifdef C_REST_ENABLE_SERVER_SENT_EVENTS_SSE
#include "c_rest_request.h"
#include "c_rest_response.h"
#include "c_rest_router.h"
#include "c_rest_sse.h"

static int hot_reload_sse_handler(struct c_rest_request *req,
                                  struct c_rest_response *res,
                                  void *user_data) {
  (void)user_data;
  struct c_rest_connection_context *conn_ctx = NULL;
  struct c_rest_hot_reload_ctx *hr_ctx = NULL;

  if (req && res && res->context) {
    conn_ctx = (struct c_rest_connection_context *)res->context;
    if (conn_ctx->framework_ctx) {
      hr_ctx = conn_ctx->framework_ctx->hot_reload_ctx;
    }
  }

  if (!hr_ctx) {
    res->status_code = 503;
    res->body = (char *)"Hot reload context not available";
    res->body_len = 32;
    return 0;
  }

  res->status_code = 200;
  if (c_rest_sse_init_response(res) != 0) {
    return 0;
  }

  while (hr_ctx->state == C_REST_HOT_RELOAD_STATE_WATCHING) {
    if (c_rest_sse_send_keepalive(res) != 0) {
      break; /* Connection closed */
    }
    sleep_seconds(hr_ctx, 1);
  }

  if (hr_ctx->state == C_REST_HOT_RELOAD_STATE_CHANGED) {
    struct c_rest_sse_event ev;
    c_rest_sse_event_init(&ev);
    /* We must dup strings if we rely on c_rest_sse_event_destroy */
    C_REST_MALLOC(7, (void **)&ev.event);
    if (ev.event) {
#if defined(_MSC_VER)
      strcpy_s(ev.event, 7, "reload");
#else
      strcpy(ev.event, "reload");
#endif
    }

    C_REST_MALLOC(5, (void **)&ev.data);
    if (ev.data) {
#if defined(_MSC_VER)
      strcpy_s(ev.data, 5, "true");
#else
      strcpy(ev.data, "true");
#endif
    }

    c_rest_sse_send_event(res, &ev);
    c_rest_sse_event_destroy(&ev);
  }
  return 0;
}

int c_rest_hot_reload_register_routes(struct c_rest_router *router,
                                      const char *path) {
  if (!router || !path) {
    return C_REST_HOT_RELOAD_ERR_PARAM;
  }
  if (c_rest_router_add(router, "GET", path, hot_reload_sse_handler, NULL) !=
      0) {
    return C_REST_HOT_RELOAD_ERR_SYSTEM;
  }
  return C_REST_HOT_RELOAD_SUCCESS;
}
#else
#include "c_rest_router.h"
/* clang-format on */
int c_rest_hot_reload_register_routes(struct c_rest_router *router,
                                      const char *path) {
  (void)router;
  (void)path;
  return C_REST_HOT_RELOAD_ERR_SYSTEM;
}
#endif
