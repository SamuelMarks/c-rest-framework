/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "c_rest_modality.h"

#include <stdlib.h>
#include "c_rest_log.h"
#include <stdio.h>

extern const struct c_rest_modality_vtable sync_vtable;
extern const struct c_rest_modality_vtable single_thread_vtable;
extern const struct c_rest_modality_vtable async_vtable;
extern const struct c_rest_modality_vtable multi_thread_vtable;
#if !defined(__EMSCRIPTEN__) && !defined(CDD_DOS)
extern const struct c_rest_modality_vtable multi_process_vtable;
#endif
extern const struct c_rest_modality_vtable greenthread_vtable;
extern const struct c_rest_modality_vtable message_passing_vtable;

/* We will reuse a dummy vtable for the unimplemented modalities for now */
static c_rest_error_t dummy_init(struct c_rest_context *ctx) {
  c_rest_error_t rc;
  if (ctx->logger.log_cb) {
    (void)ctx->logger.log_cb("Initializing Dummy modality");
  }
  return C_REST_OK;
}

static c_rest_error_t dummy_destroy(struct c_rest_context *ctx) {
  c_rest_error_t rc;
  if (ctx->logger.log_cb) {
    (void)ctx->logger.log_cb("Destroying Dummy modality");
  }
  return C_REST_OK;
}

static c_rest_error_t dummy_run(struct c_rest_context *ctx) {
  (void)ctx;
  return C_REST_OK;
}

static c_rest_error_t dummy_stop(struct c_rest_context *ctx) {
  c_rest_error_t rc;
  if (ctx->logger.log_cb) {
    (void)ctx->logger.log_cb("Stopping Dummy modality");
  }
  return C_REST_OK;
}

static const struct c_rest_modality_vtable dummy_vtable = {
    dummy_init, dummy_destroy, dummy_run, dummy_stop};

static c_rest_error_t get_vtable(enum c_rest_modality_type type,
                      const struct c_rest_modality_vtable **out_vtable) {
  switch (type) {
  case C_REST_MODALITY_SYNC:
    *out_vtable = &sync_vtable;
    return C_REST_OK;
  case C_REST_MODALITY_SINGLE_THREAD:
    *out_vtable = &single_thread_vtable;
    return C_REST_OK;
  case C_REST_MODALITY_ASYNC:
    *out_vtable = &async_vtable;
    return C_REST_OK;
  case C_REST_MODALITY_MULTI_THREAD:
    *out_vtable = &multi_thread_vtable;
    return C_REST_OK;
#if !defined(__EMSCRIPTEN__) && !defined(CDD_DOS)
  case C_REST_MODALITY_MULTI_PROCESS:
    *out_vtable = &multi_process_vtable;
    return C_REST_OK;
#endif
  case C_REST_MODALITY_GREENTHREAD:
    *out_vtable = &greenthread_vtable;
    return C_REST_OK;
  case C_REST_MODALITY_MESSAGE_PASSING:
    *out_vtable = &message_passing_vtable;
    return C_REST_OK;
  case C_REST_MODALITY_SINGLE_PROCESS:
    *out_vtable = &dummy_vtable;
    return C_REST_OK;
  default:
    *out_vtable = NULL;
    return C_REST_ERROR_GENERIC;
  }
}

c_rest_error_t c_rest_init(enum c_rest_modality_type type,
                struct c_rest_context **out_ctx) {
  struct c_rest_context *ctx;
  const struct c_rest_modality_vtable *vtable;
  int res;

  if (!out_ctx) {
    return C_REST_ERROR_GENERIC;
  }

  *out_ctx = NULL;

  res = c_rest_platform_init();
#if defined(_WIN32)
  if (res != C_REST_OK) {
    return res;
  }
#endif


  res = get_vtable(type, &vtable);
  if (res != C_REST_OK) {
    return res;
  }

  if (C_REST_MALLOC(sizeof(struct c_rest_context), &ctx) != 0) { LOG_DEBUG("C_REST_MALLOC failed"); ctx = NULL; }
  if (!ctx) {
    return C_REST_ERROR_GENERIC; /* Out of memory */
  }

#ifdef C_REST_TESTING_MALLOC_HOOK
  extern void *(*g_crf_malloc_hook)(size_t);
#endif
  ctx->modality = type;
#ifdef C_REST_TESTING_MALLOC_HOOK
  ctx->allocator.malloc_cb = g_crf_malloc_hook ? g_crf_malloc_hook : malloc;
  ctx->allocator.free_cb = free;
#else
  ctx->allocator.malloc_cb = malloc;
  ctx->allocator.free_cb = free;
#endif
  ctx->logger.log_cb =
      NULL; /* No default logger to avoid implicit stdout dependency if not
               desired, but we can allow users to set it later */
  ctx->vtable = vtable;
  ctx->internal_state = NULL;
  ctx->listen_address = "0.0.0.0";
  ctx->listen_port = 8080;
  ctx->tls_ctx = NULL;

  /* Initialize db fields to zero */
  ctx->db_config.connection_string = NULL;
  ctx->db_config.max_connections = 0;
  ctx->db_config.min_connections = 0;
  ctx->db_config.connect_timeout_ms = 0;
  ctx->db_pool = NULL;

#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
  ctx->cm_env = NULL;
#endif

#ifdef C_REST_ENABLE_HOT_RELOADING_AUTO_RESTART
  ctx->hot_reload_ctx = NULL;
  /* c_rest_hot_reload_init is left for the user to explicitly call if they want it. */
#endif

  res = ctx->vtable->init(ctx);
  if (res != 0) {
    C_REST_FREE((void *)(ctx));
    return res;
  }

  *out_ctx = ctx;
  return C_REST_OK;
}

c_rest_error_t c_rest_run(struct c_rest_context *ctx) {
  c_rest_error_t rc;
  int res;
  if (!ctx) {
    return C_REST_ERROR_GENERIC;
  }

  /* Initialize c-orm connection pool if configured */
  if (ctx->db_config.connection_string != NULL) {
    if (ctx->logger.log_cb) {
rc = ctx->logger.log_cb("Initializing c-orm database connection pool..."); if (rc != C_REST_OK) return rc;
    }
    res = c_rest_orm_init(&ctx->db_config, &ctx->db_pool);
    if (res != 0) {
      if (ctx->logger.log_cb) {
        (void)ctx->logger.log_cb("Failed to initialize database connection pool.");
      }
      return res;
    }
  }

  if (ctx->vtable && ctx->vtable->run) {
    return ctx->vtable->run(ctx);
  }

  if (ctx->logger.log_cb) {
    (void)ctx->logger.log_cb("No run loop implemented for the selected modality.");
  }
  return C_REST_ERROR_GENERIC;
}

c_rest_error_t c_rest_stop(struct c_rest_context *ctx) {
  c_rest_error_t rc;
  if (!ctx) {
    return C_REST_ERROR_GENERIC;
  }

  if (ctx->vtable && ctx->vtable->stop) {
    return ctx->vtable->stop(ctx);
  }

  if (ctx->logger.log_cb) {
    (void)ctx->logger.log_cb("No stop implemented for the selected modality.");
  }
  return C_REST_ERROR_GENERIC;
}

c_rest_error_t c_rest_destroy(struct c_rest_context *ctx) {
  c_rest_error_t rc;
  int res = 0;
  if (!ctx) {
    return C_REST_ERROR_GENERIC;
  }

  if (ctx->db_pool) {
    if (ctx->logger.log_cb) {
    (void)ctx->logger.log_cb("Cleaning up database connection pool...");
    }
    (void)c_rest_orm_cleanup(ctx->db_pool);
    ctx->db_pool = NULL;
  }

  if (ctx->vtable && ctx->vtable->destroy) {
    res = ctx->vtable->destroy(ctx); if (res != C_REST_OK) return res;
  }

  if (ctx->allocator.free_cb) {
    ctx->allocator.free_cb(ctx);
  } else {
    C_REST_FREE((void *)(ctx));
  }

  (void)c_rest_platform_cleanup();


  return res;
}

#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
c_rest_error_t c_rest_set_multiplatform_env(struct c_rest_context *ctx, cm_env_t env) {
  struct cm_allocator alloc;
  struct cm_logger logger;

  if (!ctx || !env) {
    return C_REST_ERROR_GENERIC;
  }

  ctx->cm_env = env;

  alloc.malloc_cb = ctx->allocator.malloc_cb;
  alloc.free_cb = ctx->allocator.free_cb;
  cm_env_set_allocator(env, &alloc);

  logger.log_cb = (void (*)(const char *))ctx->logger.log_cb;
  cm_env_set_logger(env, &logger);

  return C_REST_OK;
}
#endif

c_rest_error_t c_rest_set_router(struct c_rest_context *ctx,
                      struct c_rest_router *router) {
  if (!ctx)
    return C_REST_ERROR_GENERIC;
  ctx->router = router;
  return C_REST_OK;
}

#include "c_rest_parser.h"
#include "c_rest_request.h"
#include "c_rest_response.h"
#include "c_rest_router.h"
#include "c_rest_str_utils.h"
#include <string.h>
/* clang-format on */

struct connection_state {
  struct c_rest_request req;
  char *method;
  char *url;
  int is_done;
};

static c_rest_error_t on_method(c_rest_parser_context *pctx, const char *method,
                                size_t len) {
  struct connection_state *st = (struct connection_state *)pctx->user_data;
  if (C_REST_MALLOC(len + 1, &st->method) != 0) {
    LOG_DEBUG("C_REST_MALLOC failed");
    st->method = NULL;
  }
  if (st->method) {
#if defined(_MSC_VER)
    /* CDD_SAFE_CRT */ memcpy_s(st->method, len, method, len);
#else
    memcpy(st->method, method, len);
#endif
    st->method[len] = '\0';
  }
  return C_REST_OK;
}

static c_rest_error_t on_url(c_rest_parser_context *pctx, const char *url,
                             size_t len) {
  struct connection_state *st = (struct connection_state *)pctx->user_data;
  if (C_REST_MALLOC(len + 1, &st->url) != 0) {
    LOG_DEBUG("C_REST_MALLOC failed");
    st->url = NULL;
  }
  if (st->url) {
#if defined(_MSC_VER)
    /* CDD_SAFE_CRT */ memcpy_s(st->url, len, url, len);
#else
    memcpy(st->url, url, len);
#endif
    st->url[len] = '\0';
  }
  return C_REST_OK;
}

static c_rest_error_t on_header(c_rest_parser_context *pctx, const char *key,
                                size_t key_len, const char *val,
                                size_t val_len) {
  struct connection_state *st = (struct connection_state *)pctx->user_data;
  struct c_rest_header *h = NULL;
  if (C_REST_MALLOC(sizeof(struct c_rest_header), &h) != 0) {
    LOG_DEBUG("C_REST_MALLOC failed");
  }
  if (h) {
    if (C_REST_MALLOC(key_len + 1, &h->key) != 0) {
      LOG_DEBUG("C_REST_MALLOC failed");
      h->key = NULL;
    }
    if (C_REST_MALLOC(val_len + 1, &h->value) != 0) {
      LOG_DEBUG("C_REST_MALLOC failed");
      h->value = NULL;
    }
    if (h->key && h->value) {
#if defined(_MSC_VER)
      /* CDD_SAFE_CRT */ memcpy_s(h->key, key_len, key, key_len);
#else
      memcpy(h->key, key, key_len);
#endif
      h->key[key_len] = '\0';
#if defined(_MSC_VER)
      /* CDD_SAFE_CRT */ memcpy_s(h->value, val_len, val, val_len);
#else
      memcpy(h->value, val, val_len);
#endif
      h->value[val_len] = '\0';
      h->next = st->req.headers;
      st->req.headers = h;
    } else {
      if (h->key)
        C_REST_FREE((void *)(h->key));
      if (h->value)
        C_REST_FREE((void *)(h->value));
      C_REST_FREE((void *)(h));
    }
  }
  return C_REST_OK;
}

static c_rest_error_t on_body(c_rest_parser_context *pctx, const char *data,
                              size_t len) {
  struct connection_state *st = (struct connection_state *)pctx->user_data;
  char *new_body = NULL;
  (void)C_REST_REALLOC(st->req.body, st->req.body_len + len + 1,
                       (void **)&new_body);
  if (new_body) {
#if defined(_MSC_VER)
    /* CDD_SAFE_CRT */ memcpy_s(new_body + st->req.body_len, len, data, len);
#else
    memcpy(new_body + st->req.body_len, data, len);
#endif
    st->req.body = new_body;
    st->req.body_len += len;
    st->req.body[st->req.body_len] = '\0';
  }
  return C_REST_OK;
}

static c_rest_error_t on_complete(c_rest_parser_context *pctx) {
  (void)pctx;
  /* parsing done */
  ((struct connection_state *)pctx->user_data)->is_done = 1;
  return C_REST_OK;
}

c_rest_error_t c_rest_handle_connection(struct c_rest_context *ctx,
                                        c_rest_socket_t sock) {
  struct c_rest_tls_connection *tls_conn = NULL;
  char buf[4096];
  size_t read_bytes, parsed_bytes;
  int res;
  int keep_alive = 0;
  c_rest_error_t rc;

  if (!ctx)
    return C_REST_ERROR_GENERIC;

  if (ctx->tls_ctx) {
    rc = c_rest_tls_accept(ctx->tls_ctx, sock, &tls_conn);
    if (rc != C_REST_OK) {
      /* Handshake failed or WANT_READ/WRITE not handled recursively */
      return rc;
    }
  }

  {
    int done = 0;
    while (!done) {
      struct c_rest_connection_context conn_ctx;
      struct connection_state st;
      c_rest_parser_context pctx;
      struct c_rest_parser_callbacks cbs;
      const struct c_rest_parser_vtable *vt;
      struct c_rest_response res_obj;

      memset(&st, 0, sizeof(st));
      memset(&res_obj, 0, sizeof(res_obj));

      conn_ctx.sock = sock;
      conn_ctx.tls_conn = tls_conn;
#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
      conn_ctx.cm_env = ctx->cm_env;
#endif
      conn_ctx.framework_ctx = ctx;
      res_obj.context = (void *)&conn_ctx;

      cbs.on_method = on_method;
      cbs.on_url = on_url;
      cbs.on_header = on_header;
      cbs.on_body = on_body;
      cbs.on_complete = on_complete;
      cbs.on_error = NULL;

      (void)!c_rest_parser_get_basic_vtable(&vt);
      rc = c_rest_parser_init(&pctx, vt, &cbs, &st);
      if (rc != C_REST_OK) {
        keep_alive = 0;
      } else {
        while (1) {
          if (tls_conn) {
            res = c_rest_tls_read(tls_conn, buf, sizeof(buf), &read_bytes);
          } else {
            res = c_rest_socket_recv((c_rest_socket_t)sock, buf, sizeof(buf),
                                     &read_bytes);
          }
          if (res != 0)
            break;

          rc = c_rest_parser_execute(&pctx, buf, read_bytes, &parsed_bytes);
          if (rc != C_REST_OK)
            break;

          if (st.is_done)
            break;
        }

        if (st.method && st.url) {
          st.req.method = st.method;

          /* split path and query */
          {
            char *q = strchr(st.url, '?');
            if (q) {
              *q = '\0';
              st.req.path = st.url;
              st.req.query = q + 1;
            } else {
              st.req.path = st.url;
              st.req.query = NULL;
            }
          }

          st.req.scheme = ctx->tls_ctx ? "https" : "http";

          res_obj.status_code = 404;

          if (ctx->router) {
            rc = c_rest_router_dispatch(ctx->router, &st.req, &res_obj);
          }

          if (rc == C_REST_OK && !res_obj.headers_sent) {
            (void)c_rest_response_send(&res_obj);
          }

          (void)!c_rest_request_cleanup(&st.req);
          (void)!c_rest_response_cleanup(&res_obj);
        }

        (void)!c_rest_parser_should_keep_alive(&pctx, &keep_alive);
        if (res != 0)
          keep_alive = 0;
        (void)!c_rest_parser_destroy(&pctx);

        if (st.method)
          C_REST_FREE((void *)(st.method));
        if (st.url)
          C_REST_FREE((void *)(st.url));
      }

      if (!keep_alive) {
        done = 1;
      }
    }
  }

  if (tls_conn) {
    (void)!c_rest_tls_close(tls_conn);
  }

  return C_REST_OK;
}
