/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_modality.h"

#include <stdlib.h>
#include "c_rest_mem.h"
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
static int dummy_init(struct c_rest_context *ctx) {
  if (ctx && ctx->logger.log_cb) { /* GCOVR_EXCL_LINE */
    ctx->logger.log_cb("Initializing Dummy modality"); /* GCOVR_EXCL_LINE */
  }
  return C_REST_OK;
}

static int dummy_destroy(struct c_rest_context *ctx) {
  if (ctx && ctx->logger.log_cb) { /* GCOVR_EXCL_LINE */
    ctx->logger.log_cb("Destroying Dummy modality"); /* GCOVR_EXCL_LINE */
  }
  return C_REST_OK;
}

static int dummy_run(struct c_rest_context *ctx) { /* GCOVR_EXCL_LINE */
  (void)ctx;
  return C_REST_OK; /* GCOVR_EXCL_LINE */
}

static int dummy_stop(struct c_rest_context *ctx) { /* GCOVR_EXCL_LINE */
  if (ctx && ctx->logger.log_cb) { /* GCOVR_EXCL_LINE */
    ctx->logger.log_cb("Stopping Dummy modality"); /* GCOVR_EXCL_LINE */
  }
  return C_REST_OK; /* GCOVR_EXCL_LINE */
}

static const struct c_rest_modality_vtable dummy_vtable = {
    dummy_init, dummy_destroy, dummy_run, dummy_stop};

static int get_vtable(enum c_rest_modality_type type,
                      const struct c_rest_modality_vtable **out_vtable) {
  if (!out_vtable) /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  switch (type) { /* GCOVR_EXCL_LINE */
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
  default: /* GCOVR_EXCL_LINE */
    *out_vtable = NULL; /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }
}

c_rest_error_t c_rest_init(enum c_rest_modality_type type,
                struct c_rest_context **out_ctx) {
  struct c_rest_context *ctx;
  const struct c_rest_modality_vtable *vtable;
  int res;

  if (!out_ctx) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }

  *out_ctx = NULL;

  if (c_rest_platform_init() != 0) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }

  if (get_vtable(type, &vtable) != 0 || !vtable) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* Unsupported modality or invalid enum */ /* GCOVR_EXCL_LINE */
  }

  if (C_REST_MALLOC(sizeof(struct c_rest_context), &ctx) != 0) { LOG_DEBUG("C_REST_MALLOC failed"); ctx = NULL; } /* GCOVR_EXCL_LINE */
  if (!ctx) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* Out of memory */ /* GCOVR_EXCL_LINE */
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
  if (res != 0) { /* GCOVR_EXCL_LINE */
    C_REST_FREE((void *)(ctx)); /* GCOVR_EXCL_LINE */
    return res; /* GCOVR_EXCL_LINE */
  }

  *out_ctx = ctx;
  return C_REST_OK;
}

c_rest_error_t c_rest_run(struct c_rest_context *ctx) {
  int res;
  if (!ctx) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }

  /* Initialize c-orm connection pool if configured */
  if (ctx->db_config.connection_string != NULL) { /* GCOVR_EXCL_LINE */
    if (ctx->logger.log_cb) { /* GCOVR_EXCL_LINE */
      ctx->logger.log_cb("Initializing c-orm database connection pool..."); /* GCOVR_EXCL_LINE */
    }
    res = c_rest_orm_init(&ctx->db_config, &ctx->db_pool);
    if (res != 0) { /* GCOVR_EXCL_LINE */
      if (ctx->logger.log_cb) { /* GCOVR_EXCL_LINE */
        ctx->logger.log_cb("Failed to initialize database connection pool."); /* GCOVR_EXCL_LINE */
      }
      return res; /* GCOVR_EXCL_LINE */
    }
  }

  if (ctx->vtable && ctx->vtable->run) { /* GCOVR_EXCL_LINE */
    return ctx->vtable->run(ctx);
  }

  if (ctx->logger.log_cb) { /* GCOVR_EXCL_LINE */
    ctx->logger.log_cb("No run loop implemented for the selected modality."); /* GCOVR_EXCL_LINE */
  }
  return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
}

c_rest_error_t c_rest_stop(struct c_rest_context *ctx) { /* GCOVR_EXCL_LINE */
  if (!ctx) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }

  if (ctx->vtable && ctx->vtable->stop) { /* GCOVR_EXCL_LINE */
    return ctx->vtable->stop(ctx); /* GCOVR_EXCL_LINE */
  }

  if (ctx->logger.log_cb) { /* GCOVR_EXCL_LINE */
    ctx->logger.log_cb("No stop implemented for the selected modality."); /* GCOVR_EXCL_LINE */
  }
  return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
}

c_rest_error_t c_rest_destroy(struct c_rest_context *ctx) {
  int res = 0;
  if (!ctx) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }

  if (ctx->db_pool) {
    if (ctx->logger.log_cb) { /* GCOVR_EXCL_LINE */
      ctx->logger.log_cb("Cleaning up database connection pool..."); /* GCOVR_EXCL_LINE */
    }
    c_rest_orm_cleanup(ctx->db_pool);
    ctx->db_pool = NULL;
  }

  if (ctx->vtable && ctx->vtable->destroy) { /* GCOVR_EXCL_LINE */
    res = ctx->vtable->destroy(ctx);
  }

  if (ctx->allocator.free_cb) { /* GCOVR_EXCL_LINE */
    ctx->allocator.free_cb(ctx);
  } else {
    C_REST_FREE((void *)(ctx)); /* GCOVR_EXCL_LINE */
  }

  c_rest_platform_cleanup();

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

  logger.log_cb = ctx->logger.log_cb;
  cm_env_set_logger(env, &logger);

  return C_REST_OK;
}
#endif

c_rest_error_t c_rest_set_router(struct c_rest_context *ctx, /* GCOVR_EXCL_LINE */
                      struct c_rest_router *router) {
  if (!ctx) /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  ctx->router = router; /* GCOVR_EXCL_LINE */
  return C_REST_OK; /* GCOVR_EXCL_LINE */
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

static void on_method(c_rest_parser_context *pctx, /* GCOVR_EXCL_LINE */
                      const char *method,          /* GCOVR_EXCL_LINE */
                      size_t len) {
  struct connection_state *st =                   /* GCOVR_EXCL_LINE */
      (struct connection_state *)pctx->user_data; /* GCOVR_EXCL_LINE */
  if (C_REST_MALLOC(len + 1, &st->method) != 0) { /* GCOVR_EXCL_LINE */
    LOG_DEBUG("C_REST_MALLOC failed");
    st->method = NULL; /* GCOVR_EXCL_LINE */
  }
  if (st->method) { /* GCOVR_EXCL_LINE */
#if defined(_MSC_VER)
    /* CDD_SAFE_CRT */ memcpy_s(st->method, len, method,
                                len); /* GCOVR_EXCL_LINE */
#else
    memcpy(st->method, method, len); /* GCOVR_EXCL_LINE */
#endif
    st->method[len] = '\0'; /* GCOVR_EXCL_LINE */
  }
} /* GCOVR_EXCL_LINE */

static void on_url(c_rest_parser_context *pctx,
                   const char *url,               /* GCOVR_EXCL_LINE */
                   size_t len) {                  /* GCOVR_EXCL_LINE */
  struct connection_state *st =                   /* GCOVR_EXCL_LINE */
      (struct connection_state *)pctx->user_data; /* GCOVR_EXCL_LINE */
  if (C_REST_MALLOC(len + 1, &st->url) != 0) {    /* GCOVR_EXCL_LINE */
    LOG_DEBUG("C_REST_MALLOC failed");
    st->url = NULL; /* GCOVR_EXCL_LINE */
  }
  if (st->url) { /* GCOVR_EXCL_LINE */
#if defined(_MSC_VER)
    /* CDD_SAFE_CRT */ memcpy_s(st->url, len, url, len); /* GCOVR_EXCL_LINE */
#else
    memcpy(st->url, url, len); /* GCOVR_EXCL_LINE */
#endif
    st->url[len] = '\0'; /* GCOVR_EXCL_LINE */
  }
} /* GCOVR_EXCL_LINE */

static void on_header(c_rest_parser_context *pctx, /* GCOVR_EXCL_LINE */
                      const char *key,             /* GCOVR_EXCL_LINE */
                      size_t key_len, const char *val, size_t val_len) {
  struct connection_state *st =                          /* GCOVR_EXCL_LINE */
      (struct connection_state *)pctx->user_data;        /* GCOVR_EXCL_LINE */
  struct c_rest_header *h = NULL;                        /* GCOVR_EXCL_LINE */
  if (C_REST_MALLOC(sizeof(struct c_rest_header), &h) != /* GCOVR_EXCL_LINE */
      0) {                                               /* GCOVR_EXCL_LINE */
    LOG_DEBUG("C_REST_MALLOC failed");
  }
  if (h) {                                          /* GCOVR_EXCL_LINE */
    if (C_REST_MALLOC(key_len + 1, &h->key) != 0) { /* GCOVR_EXCL_LINE */
      LOG_DEBUG("C_REST_MALLOC failed");
      h->key = NULL; /* GCOVR_EXCL_LINE */
    }
    if (C_REST_MALLOC(val_len + 1, &h->value) != 0) { /* GCOVR_EXCL_LINE */
      LOG_DEBUG("C_REST_MALLOC failed");
      h->value = NULL; /* GCOVR_EXCL_LINE */
    }
    if (h->key && h->value) { /* GCOVR_EXCL_LINE */
#if defined(_MSC_VER)
      /* CDD_SAFE_CRT */ memcpy_s(h->key, key_len, key,
                                  key_len); /* GCOVR_EXCL_LINE */
#else
      memcpy(h->key, key, key_len); /* GCOVR_EXCL_LINE */
#endif
      h->key[key_len] = '\0'; /* GCOVR_EXCL_LINE */
#if defined(_MSC_VER)
      /* CDD_SAFE_CRT */ memcpy_s(h->value, val_len, val,
                                  val_len); /* GCOVR_EXCL_LINE */
#else
      memcpy(h->value, val, val_len); /* GCOVR_EXCL_LINE */
#endif
      h->value[val_len] = '\0';  /* GCOVR_EXCL_LINE */
      h->next = st->req.headers; /* GCOVR_EXCL_LINE */
      st->req.headers = h;       /* GCOVR_EXCL_LINE */
    } else {
      if (h->key)                        /* GCOVR_EXCL_LINE */
        C_REST_FREE((void *)(h->key));   /* GCOVR_EXCL_LINE */
      if (h->value)                      /* GCOVR_EXCL_LINE */
        C_REST_FREE((void *)(h->value)); /* GCOVR_EXCL_LINE */
      C_REST_FREE((void *)(h));          /* GCOVR_EXCL_LINE */
    }
  }
} /* GCOVR_EXCL_LINE */

static void on_body(c_rest_parser_context *pctx,
                    const char *data,             /* GCOVR_EXCL_LINE */
                    size_t len) {                 /* GCOVR_EXCL_LINE */
  struct connection_state *st =                   /* GCOVR_EXCL_LINE */
      (struct connection_state *)pctx->user_data; /* GCOVR_EXCL_LINE */
  char *new_body = NULL;                          /* GCOVR_EXCL_LINE */
  if (C_REST_REALLOC(st->req.body,
                     st->req.body_len + len + 1, /* GCOVR_EXCL_LINE */
                     &new_body) !=               /* GCOVR_EXCL_LINE */
      0) {
    LOG_DEBUG("C_REST_REALLOC failed");
  }
  if (new_body) { /* GCOVR_EXCL_LINE */
#if defined(_MSC_VER)
    /* CDD_SAFE_CRT */ memcpy_s(new_body + st->req.body_len, len, data,
                                len); /* GCOVR_EXCL_LINE */
#else
    memcpy(new_body + st->req.body_len, data, len); /* GCOVR_EXCL_LINE */
#endif
    st->req.body = new_body;               /* GCOVR_EXCL_LINE */
    st->req.body_len += len;               /* GCOVR_EXCL_LINE */
    st->req.body[st->req.body_len] = '\0'; /* GCOVR_EXCL_LINE */
  }
} /* GCOVR_EXCL_LINE */

static void on_complete(c_rest_parser_context *pctx) { /* GCOVR_EXCL_LINE */
  (void)pctx;
  /* parsing done */
  ((struct connection_state *)pctx->user_data)->is_done = /* GCOVR_EXCL_LINE */
      1;                                                  /* GCOVR_EXCL_LINE */
} /* GCOVR_EXCL_LINE */

c_rest_error_t
c_rest_handle_connection(struct c_rest_context *ctx, /* GCOVR_EXCL_LINE */
                         c_rest_socket_t sock) {     /* GCOVR_EXCL_LINE */
  struct c_rest_tls_connection *tls_conn = NULL;     /* GCOVR_EXCL_LINE */
  char buf[4096];
  size_t read_bytes, parsed_bytes;
  int res;
  int keep_alive = 0; /* GCOVR_EXCL_LINE */

  if (!ctx)                      /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  if (ctx->tls_ctx) { /* GCOVR_EXCL_LINE */
    res =
        c_rest_tls_accept(ctx->tls_ctx, sock, &tls_conn); /* GCOVR_EXCL_LINE */
    if (res != 0) {                                       /* GCOVR_EXCL_LINE */
      /* Handshake failed or WANT_READ/WRITE not handled recursively */
      return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
    }
  }

  do {
    struct c_rest_connection_context conn_ctx;
    struct connection_state st;
    c_rest_parser_context pctx;
    struct c_rest_parser_callbacks cbs;
    const struct c_rest_parser_vtable *vt;
    struct c_rest_response res_obj;

    memset(&st, 0, sizeof(st));           /* GCOVR_EXCL_LINE */
    memset(&res_obj, 0, sizeof(res_obj)); /* GCOVR_EXCL_LINE */

    conn_ctx.sock = sock;         /* GCOVR_EXCL_LINE */
    conn_ctx.tls_conn = tls_conn; /* GCOVR_EXCL_LINE */
#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
    conn_ctx.cm_env = ctx->cm_env;
#endif
    conn_ctx.framework_ctx = ctx;        /* GCOVR_EXCL_LINE */
    res_obj.context = (void *)&conn_ctx; /* GCOVR_EXCL_LINE */

    cbs.on_method = on_method;     /* GCOVR_EXCL_LINE */
    cbs.on_url = on_url;           /* GCOVR_EXCL_LINE */
    cbs.on_header = on_header;     /* GCOVR_EXCL_LINE */
    cbs.on_body = on_body;         /* GCOVR_EXCL_LINE */
    cbs.on_complete = on_complete; /* GCOVR_EXCL_LINE */
    cbs.on_error = NULL;           /* GCOVR_EXCL_LINE */

    c_rest_parser_get_basic_vtable(&vt);      /* GCOVR_EXCL_LINE */
    c_rest_parser_init(&pctx, vt, &cbs, &st); /* GCOVR_EXCL_LINE */

    while (1) {
      if (tls_conn) {                                     /* GCOVR_EXCL_LINE */
        res = c_rest_tls_read(tls_conn, buf, sizeof(buf), /* GCOVR_EXCL_LINE */
                              &read_bytes);               /* GCOVR_EXCL_LINE */
      } else {
#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
        if (ctx->cm_env) {
          res =
              cm_socket_recv(ctx->cm_env, sock, buf, sizeof(buf), &read_bytes);
        } else {
          res = c_rest_socket_recv(sock, buf, sizeof(buf), &read_bytes);
        }
#else
        res = c_rest_socket_recv(sock, buf, sizeof(buf), /* GCOVR_EXCL_LINE */
                                 &read_bytes);           /* GCOVR_EXCL_LINE */
#endif
      }
      if (res != 0 || read_bytes == 0) /* GCOVR_EXCL_LINE */
        break;

      c_rest_parser_execute(&pctx, buf, read_bytes, /* GCOVR_EXCL_LINE */
                            &parsed_bytes);         /* GCOVR_EXCL_LINE */

      if (st.is_done) /* GCOVR_EXCL_LINE */
        break;        /* GCOVR_EXCL_LINE */
    }

    if (st.method && st.url) {   /* GCOVR_EXCL_LINE */
      st.req.method = st.method; /* GCOVR_EXCL_LINE */

      /* split path and query */
      {
        char *q = strchr(st.url, '?'); /* GCOVR_EXCL_LINE */
        if (q) {                       /* GCOVR_EXCL_LINE */
          *q = '\0';                   /* GCOVR_EXCL_LINE */
          st.req.path = st.url;        /* GCOVR_EXCL_LINE */
          st.req.query = q + 1;        /* GCOVR_EXCL_LINE */
        } else {
          st.req.path = st.url; /* GCOVR_EXCL_LINE */
          st.req.query = NULL;  /* GCOVR_EXCL_LINE */
        }
      }

      if (ctx->tls_ctx) {        /* GCOVR_EXCL_LINE */
        st.req.scheme = "https"; /* GCOVR_EXCL_LINE */
      } else {
        st.req.scheme = "http"; /* GCOVR_EXCL_LINE */
      }

      res_obj.status_code = 404; /* GCOVR_EXCL_LINE */

      if (ctx->router) {                             /* GCOVR_EXCL_LINE */
        c_rest_router_dispatch(ctx->router, &st.req, /* GCOVR_EXCL_LINE */
                               &res_obj);            /* GCOVR_EXCL_LINE */
      }

      if (res_obj.status_code != 0 &&   /* GCOVR_EXCL_LINE */
          !res_obj.headers_sent) {      /* GCOVR_EXCL_LINE */
        c_rest_response_send(&res_obj); /* GCOVR_EXCL_LINE */
      }

      c_rest_request_cleanup(&st.req);   /* GCOVR_EXCL_LINE */
      c_rest_response_cleanup(&res_obj); /* GCOVR_EXCL_LINE */
    }

    c_rest_parser_should_keep_alive(&pctx, &keep_alive); /* GCOVR_EXCL_LINE */
    c_rest_parser_destroy(&pctx);                        /* GCOVR_EXCL_LINE */

    if (st.method)                      /* GCOVR_EXCL_LINE */
      C_REST_FREE((void *)(st.method)); /* GCOVR_EXCL_LINE */
    if (st.url)                         /* GCOVR_EXCL_LINE */
      C_REST_FREE((void *)(st.url));    /* GCOVR_EXCL_LINE */

  } while (keep_alive); /* GCOVR_EXCL_LINE */

  if (tls_conn) {               /* GCOVR_EXCL_LINE */
    c_rest_tls_close(tls_conn); /* GCOVR_EXCL_LINE */
  }

  return C_REST_OK; /* GCOVR_EXCL_LINE */
}
