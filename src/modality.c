/* clang-format off */
#include "c_rest_modality.h"

#include <stdlib.h>
#include <stdio.h>

extern const struct c_rest_modality_vtable sync_vtable;
extern const struct c_rest_modality_vtable single_thread_vtable;
extern const struct c_rest_modality_vtable async_vtable;
extern const struct c_rest_modality_vtable multi_thread_vtable;
extern const struct c_rest_modality_vtable multi_process_vtable;
extern const struct c_rest_modality_vtable greenthread_vtable;
extern const struct c_rest_modality_vtable message_passing_vtable;

/* We will reuse a dummy vtable for the unimplemented modalities for now */
static int dummy_init(struct c_rest_context *ctx) {
  if (ctx && ctx->logger.log_cb) {
    ctx->logger.log_cb("Initializing Dummy modality");
  }
  return 0;
}

static int dummy_destroy(struct c_rest_context *ctx) {
  if (ctx && ctx->logger.log_cb) {
    ctx->logger.log_cb("Destroying Dummy modality");
  }
  return 0;
}

static int dummy_run(struct c_rest_context *ctx) {
  (void)ctx;
  return 0;
}

static int dummy_stop(struct c_rest_context *ctx) {
  if (ctx && ctx->logger.log_cb) {
    ctx->logger.log_cb("Stopping Dummy modality");
  }
  return 0;
}

static const struct c_rest_modality_vtable dummy_vtable = {
    dummy_init, dummy_destroy, dummy_run, dummy_stop};

static int get_vtable(enum c_rest_modality_type type,
                      const struct c_rest_modality_vtable **out_vtable) {
  if (!out_vtable)
    return 1;

  switch (type) {
  case C_REST_MODALITY_SYNC:
    *out_vtable = &sync_vtable;
    return 0;
  case C_REST_MODALITY_SINGLE_THREAD:
    *out_vtable = &single_thread_vtable;
    return 0;
  case C_REST_MODALITY_ASYNC:
    *out_vtable = &async_vtable;
    return 0;
  case C_REST_MODALITY_MULTI_THREAD:
    *out_vtable = &multi_thread_vtable;
    return 0;
  case C_REST_MODALITY_MULTI_PROCESS:
    *out_vtable = &multi_process_vtable;
    return 0;
  case C_REST_MODALITY_GREENTHREAD:
    *out_vtable = &greenthread_vtable;
    return 0;
  case C_REST_MODALITY_MESSAGE_PASSING:
    *out_vtable = &message_passing_vtable;
    return 0;
  case C_REST_MODALITY_SINGLE_PROCESS:
    *out_vtable = &dummy_vtable;
    return 0;
  default:
    *out_vtable = NULL;
    return 1;
  }
}

int c_rest_init(enum c_rest_modality_type type,
                struct c_rest_context **out_ctx) {
  struct c_rest_context *ctx;
  const struct c_rest_modality_vtable *vtable;
  int res;

  if (!out_ctx) {
    return 1;
  }

  *out_ctx = NULL;

  if (c_rest_platform_init() != 0) {
    return 1;
  }

  if (get_vtable(type, &vtable) != 0 || !vtable) {
    return 1; /* Unsupported modality or invalid enum */
  }

  ctx = (struct c_rest_context *)malloc(sizeof(struct c_rest_context));
  if (!ctx) {
    return 1; /* Out of memory */
  }

  ctx->modality = type;
  ctx->allocator.malloc_cb = malloc;
  ctx->allocator.free_cb = free;
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
    free(ctx);
    return res;
  }

  *out_ctx = ctx;
  return 0;
}

int c_rest_run(struct c_rest_context *ctx) {
  int res;
  if (!ctx) {
    return 1;
  }

  /* Initialize c-orm connection pool if configured */
  if (ctx->db_config.connection_string != NULL) {
    if (ctx->logger.log_cb) {
      ctx->logger.log_cb("Initializing c-orm database connection pool...");
    }
    res = c_rest_orm_init(&ctx->db_config, &ctx->db_pool);
    if (res != 0) {
      if (ctx->logger.log_cb) {
        ctx->logger.log_cb("Failed to initialize database connection pool.");
      }
      return res;
    }
  }

  if (ctx->vtable && ctx->vtable->run) {
    return ctx->vtable->run(ctx);
  }

  if (ctx->logger.log_cb) {
    ctx->logger.log_cb("No run loop implemented for the selected modality.");
  }
  return 1;
}

int c_rest_stop(struct c_rest_context *ctx) {
  if (!ctx) {
    return 1;
  }

  if (ctx->vtable && ctx->vtable->stop) {
    return ctx->vtable->stop(ctx);
  }

  if (ctx->logger.log_cb) {
    ctx->logger.log_cb("No stop implemented for the selected modality.");
  }
  return 1;
}

int c_rest_destroy(struct c_rest_context *ctx) {
  int res = 0;
  if (!ctx) {
    return 1;
  }

  if (ctx->db_pool) {
    if (ctx->logger.log_cb) {
      ctx->logger.log_cb("Cleaning up database connection pool...");
    }
    c_rest_orm_cleanup(ctx->db_pool);
    ctx->db_pool = NULL;
  }

  if (ctx->vtable && ctx->vtable->destroy) {
    res = ctx->vtable->destroy(ctx);
  }

  if (ctx->allocator.free_cb) {
    ctx->allocator.free_cb(ctx);
  } else {
    free(ctx);
  }

  c_rest_platform_cleanup();

  return res;
}

#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
int c_rest_set_multiplatform_env(struct c_rest_context *ctx, cm_env_t env) {
  struct cm_allocator alloc;
  struct cm_logger logger;

  if (!ctx || !env) {
    return 1;
  }

  ctx->cm_env = env;

  alloc.malloc_cb = ctx->allocator.malloc_cb;
  alloc.free_cb = ctx->allocator.free_cb;
  cm_env_set_allocator(env, &alloc);

  logger.log_cb = ctx->logger.log_cb;
  cm_env_set_logger(env, &logger);

  return 0;
}
#endif

int c_rest_set_router(struct c_rest_context *ctx,
                      struct c_rest_router *router) {
  if (!ctx)
    return 1;
  ctx->router = router;
  return 0;
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

static void on_method(c_rest_parser_context *pctx, const char *method,
                      size_t len) {
  struct connection_state *st = (struct connection_state *)pctx->user_data;
  st->method = (char *)malloc(len + 1);
  if (st->method) {
    memcpy(st->method, method, len);
    st->method[len] = '\0';
  }
}

static void on_url(c_rest_parser_context *pctx, const char *url, size_t len) {
  struct connection_state *st = (struct connection_state *)pctx->user_data;
  st->url = (char *)malloc(len + 1);
  if (st->url) {
    memcpy(st->url, url, len);
    st->url[len] = '\0';
  }
}

static void on_header(c_rest_parser_context *pctx, const char *key,
                      size_t key_len, const char *val, size_t val_len) {
  struct connection_state *st = (struct connection_state *)pctx->user_data;
  struct c_rest_header *h =
      (struct c_rest_header *)malloc(sizeof(struct c_rest_header));
  if (h) {
    h->key = (char *)malloc(key_len + 1);
    h->value = (char *)malloc(val_len + 1);
    if (h->key && h->value) {
      memcpy(h->key, key, key_len);
      h->key[key_len] = '\0';
      memcpy(h->value, val, val_len);
      h->value[val_len] = '\0';
      h->next = st->req.headers;
      st->req.headers = h;
    } else {
      if (h->key)
        free(h->key);
      if (h->value)
        free(h->value);
      free(h);
    }
  }
}

static void on_body(c_rest_parser_context *pctx, const char *data, size_t len) {
  struct connection_state *st = (struct connection_state *)pctx->user_data;
  char *new_body = (char *)realloc(st->req.body, st->req.body_len + len + 1);
  if (new_body) {
    memcpy(new_body + st->req.body_len, data, len);
    st->req.body = new_body;
    st->req.body_len += len;
    st->req.body[st->req.body_len] = '\0';
  }
}

static void on_complete(c_rest_parser_context *pctx) {
  (void)pctx;
  /* parsing done */
  ((struct connection_state *)pctx->user_data)->is_done = 1;
}

int c_rest_handle_connection(struct c_rest_context *ctx, c_rest_socket_t sock) {
  struct c_rest_tls_connection *tls_conn = NULL;
  char buf[4096];
  size_t read_bytes, parsed_bytes;
  int res;
  int keep_alive = 0;

  if (!ctx)
    return 1;

  if (ctx->tls_ctx) {
    res = c_rest_tls_accept(ctx->tls_ctx, sock, &tls_conn);
    if (res != 0) {
      /* Handshake failed or WANT_READ/WRITE not handled recursively */
      return 1;
    }
  }

  do {
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

    c_rest_parser_get_basic_vtable(&vt);
    c_rest_parser_init(&pctx, vt, &cbs, &st);

    while (1) {
      if (tls_conn) {
        res = c_rest_tls_read(tls_conn, buf, sizeof(buf), &read_bytes);
      } else {
#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
        if (ctx->cm_env) {
          res =
              cm_socket_recv(ctx->cm_env, sock, buf, sizeof(buf), &read_bytes);
        } else {
          res = c_rest_socket_recv(sock, buf, sizeof(buf), &read_bytes);
        }
#else
        res = c_rest_socket_recv(sock, buf, sizeof(buf), &read_bytes);
#endif
      }
      if (res != 0 || read_bytes == 0)
        break;

      c_rest_parser_execute(&pctx, buf, read_bytes, &parsed_bytes);

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

      if (ctx->tls_ctx) {
        st.req.scheme = "https";
      } else {
        st.req.scheme = "http";
      }

      res_obj.status_code = 404;

      if (ctx->router) {
        c_rest_router_dispatch(ctx->router, &st.req, &res_obj);
      }

      if (res_obj.status_code != 0 && !res_obj.headers_sent) {
        c_rest_response_send(&res_obj);
      }

      c_rest_request_cleanup(&st.req);
      c_rest_response_cleanup(&res_obj);
    }

    c_rest_parser_should_keep_alive(&pctx, &keep_alive);
    c_rest_parser_destroy(&pctx);

    if (st.method)
      free(st.method);
    if (st.url)
      free(st.url);

  } while (keep_alive);

  if (tls_conn) {
    c_rest_tls_close(tls_conn);
  }

  return 0;
}
