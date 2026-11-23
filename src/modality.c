/* clang-format off */
#include "c_rest_testing_mocks.h"
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "c_rest_modality.h"
#include "c_rest_parser.h"
#include "c_rest_request.h"
#include "c_rest_response.h"
#include "c_rest_router.h"
#include "c_rest_str_utils.h"
#include "c_rest_export.h"
#include "c_rest_log.h"

#ifdef C_REST_TESTING_MALLOC_HOOK
c_rest_error_t mock_c_rest_socket_create(c_rest_socket_t *sock);
c_rest_error_t mock_c_rest_socket_bind(c_rest_socket_t sock, const char *host,
                                       unsigned short port);
c_rest_error_t mock_c_rest_socket_listen(c_rest_socket_t sock, int backlog);
#define CRF_SP_SOCK_CREATE mock_c_rest_socket_create
#define CRF_SP_SOCK_BIND mock_c_rest_socket_bind
#define CRF_SP_SOCK_LISTEN mock_c_rest_socket_listen
#define CRF_SP_SOCK_ACCEPT mock_c_rest_socket_accept
#define CRF_SP_SOCK_CLOSE mock_c_rest_socket_close
#define CRF_SP_HANDLE_CONN mock_c_rest_handle_connection
#else
#define CRF_SP_SOCK_CREATE c_rest_socket_create
#define CRF_SP_SOCK_BIND c_rest_socket_bind
#define CRF_SP_SOCK_LISTEN c_rest_socket_listen
#define CRF_SP_SOCK_ACCEPT c_rest_socket_accept
#define CRF_SP_SOCK_CLOSE c_rest_socket_close
#define CRF_SP_HANDLE_CONN c_rest_handle_connection
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
/* clang-format on */

static NO_DISCARD c_rest_error_t
single_process_init(struct c_rest_context *ctx) {
  if (!ctx)
    return C_REST_ERROR_INVALID_ARG;
  ctx->sp_server_sock = C_REST_INVALID_SOCKET;
  ctx->sp_is_running = 0;
  if (ctx->logger.log_cb) {
    c_rest_error_t rc =
        ctx->logger.log_cb("SINGLE_PROCESS modality initialized");
    if (rc != C_REST_OK)
      return rc;
  }
  return C_REST_OK;
}

static NO_DISCARD c_rest_error_t
single_process_destroy(struct c_rest_context *ctx) {
  c_rest_error_t rc;
  if (!ctx)
    return C_REST_ERROR_INVALID_ARG;
  if (ctx->sp_server_sock != C_REST_INVALID_SOCKET) {
#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
    if (ctx->cm_env) {
      rc = cm_socket_close(ctx->cm_env, ctx->sp_server_sock);
      if (rc != C_REST_OK)
        return rc;
    } else {
      rc = CRF_SP_SOCK_CLOSE(ctx->sp_server_sock);
      if (rc != C_REST_OK)
        return rc;
    }
#else
    rc = CRF_SP_SOCK_CLOSE(ctx->sp_server_sock);
    if (rc != C_REST_OK)
      return rc;
#endif
    ctx->sp_server_sock = C_REST_INVALID_SOCKET;
  }
  if (ctx->logger.log_cb) {
    rc = ctx->logger.log_cb("SINGLE_PROCESS modality destroyed");
    if (rc != C_REST_OK)
      return rc;
  }
  return C_REST_OK;
}

static NO_DISCARD c_rest_error_t
single_process_run(struct c_rest_context *ctx) {
  c_rest_error_t rc;
  if (!ctx)
    return C_REST_ERROR_INVALID_ARG;

  ctx->sp_is_running = 1;
  if (ctx->logger.log_cb) {
    rc = ctx->logger.log_cb("SINGLE_PROCESS modality run started");
    if (rc != C_REST_OK)
      return rc;
  }

  if (ctx->sp_server_sock == C_REST_INVALID_SOCKET &&
      (ctx->listen_address != NULL || ctx->listen_port != 0)) {
    const char *addr = ctx->listen_address ? ctx->listen_address : "0.0.0.0";
#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
    if (ctx->cm_env) {
      rc = cm_socket_create(ctx->cm_env, &ctx->sp_server_sock);
      if (rc != C_REST_OK)
        return rc;
      rc = cm_socket_bind(ctx->cm_env, ctx->sp_server_sock, addr,
                          ctx->listen_port);
      if (rc != C_REST_OK) {
        c_rest_socket_t sock_to_close = ctx->sp_server_sock;
        c_rest_error_t cl_rc;
        ctx->sp_server_sock = C_REST_INVALID_SOCKET;
        cl_rc = cm_socket_close(ctx->cm_env, sock_to_close);
        if (cl_rc != C_REST_OK)
          return cl_rc;
        return rc;
      }
      rc = cm_socket_listen(ctx->cm_env, ctx->sp_server_sock, 128);
      if (rc != C_REST_OK) {
        c_rest_socket_t sock_to_close = ctx->sp_server_sock;
        c_rest_error_t cl_rc;
        ctx->sp_server_sock = C_REST_INVALID_SOCKET;
        cl_rc = cm_socket_close(ctx->cm_env, sock_to_close);
        if (cl_rc != C_REST_OK)
          return cl_rc;
        return rc;
      }
    } else
#endif
    {
      rc = CRF_SP_SOCK_CREATE(&ctx->sp_server_sock);
      if (rc != C_REST_OK)
        return rc;
      rc = CRF_SP_SOCK_BIND(ctx->sp_server_sock, addr, ctx->listen_port);
      if (rc != C_REST_OK) {
        c_rest_socket_t sock_to_close = ctx->sp_server_sock;
        c_rest_error_t cl_rc;
        ctx->sp_server_sock = C_REST_INVALID_SOCKET;
        cl_rc = CRF_SP_SOCK_CLOSE(sock_to_close);
        if (cl_rc != C_REST_OK)
          return cl_rc;
        return rc;
      }
      rc = CRF_SP_SOCK_LISTEN(ctx->sp_server_sock, 128);
      if (rc != C_REST_OK) {
        c_rest_socket_t sock_to_close = ctx->sp_server_sock;
        c_rest_error_t cl_rc;
        ctx->sp_server_sock = C_REST_INVALID_SOCKET;
        cl_rc = CRF_SP_SOCK_CLOSE(sock_to_close);
        if (cl_rc != C_REST_OK)
          return cl_rc;
        return rc;
      }
    }
  }

  if (ctx->sp_server_sock != C_REST_INVALID_SOCKET) {
    while (ctx->sp_is_running) {
      c_rest_socket_t client_sock = C_REST_INVALID_SOCKET;
#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
      if (ctx->cm_env) {
        rc = cm_socket_accept(ctx->cm_env, ctx->sp_server_sock, &client_sock);
      } else {
        rc = CRF_SP_SOCK_ACCEPT(ctx->sp_server_sock, &client_sock);
      }
#else
      rc = CRF_SP_SOCK_ACCEPT(ctx->sp_server_sock, &client_sock);
#endif
      if (rc != C_REST_OK) {
        goto sp_loop_done;
      }
      if (client_sock != C_REST_INVALID_SOCKET) {
        c_rest_error_t h_rc;
        c_rest_error_t cl_rc;
        h_rc = CRF_SP_HANDLE_CONN(ctx, client_sock);
        if (h_rc != C_REST_OK) {
          goto sp_close_client;
        }
      sp_close_client:;
#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
        if (ctx->cm_env) {
          cl_rc = cm_socket_close(ctx->cm_env, client_sock);
        } else {
          cl_rc = CRF_SP_SOCK_CLOSE(client_sock);
        }
#else
        cl_rc = CRF_SP_SOCK_CLOSE(client_sock);
#endif
        if (cl_rc != C_REST_OK)
          return cl_rc;
        if (h_rc != C_REST_OK)
          return h_rc;
      }
    }
  }

sp_loop_done:
  ctx->sp_is_running = 0;
  if (ctx->logger.log_cb) {
    rc = ctx->logger.log_cb("SINGLE_PROCESS modality run finished");
    if (rc != C_REST_OK)
      return rc;
  }
  return C_REST_OK;
}

static NO_DISCARD c_rest_error_t
single_process_stop(struct c_rest_context *ctx) {
  c_rest_error_t rc;
  if (!ctx)
    return C_REST_ERROR_INVALID_ARG;
  ctx->sp_is_running = 0;
  if (ctx->sp_server_sock != C_REST_INVALID_SOCKET) {
#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
    if (ctx->cm_env) {
      rc = cm_socket_close(ctx->cm_env, ctx->sp_server_sock);
      if (rc != C_REST_OK)
        return rc;
    } else {
      rc = CRF_SP_SOCK_CLOSE(ctx->sp_server_sock);
      if (rc != C_REST_OK)
        return rc;
    }
#else
    rc = CRF_SP_SOCK_CLOSE(ctx->sp_server_sock);
    if (rc != C_REST_OK)
      return rc;
#endif
    ctx->sp_server_sock = C_REST_INVALID_SOCKET;
  }
  if (ctx->logger.log_cb) {
    rc = ctx->logger.log_cb("SINGLE_PROCESS modality stopped");
    if (rc != C_REST_OK)
      return rc;
  }
  return C_REST_OK;
}

const struct c_rest_modality_vtable single_process_vtable = {
    single_process_init, single_process_destroy, single_process_run,
    single_process_stop};

static NO_DISCARD c_rest_error_t
get_vtable(enum c_rest_modality_type type,
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
    *out_vtable = &single_process_vtable;
    return C_REST_OK;
  default:
    *out_vtable = NULL;
    return C_REST_ERROR_GENERIC;
  }
}

c_rest_error_t c_rest_init(enum c_rest_modality_type type,
                           struct c_rest_context **out_ctx) {
  struct c_rest_context *ctx = NULL;
  const struct c_rest_modality_vtable *vtable = NULL;
  c_rest_error_t res;

  if (!out_ctx) {
    return C_REST_ERROR_GENERIC;
  }

  *out_ctx = NULL;

#ifdef C_REST_TESTING_MALLOC_HOOK
  res = crf_seam_modality_platform_init();
#else
  res = c_rest_platform_init();
#endif
  if (res != C_REST_OK) {
    return res;
  }

  res = get_vtable(type, &vtable);
  if (res != C_REST_OK) {
    return res;
  }

  if (C_REST_MALLOC(sizeof(struct c_rest_context), &ctx) != 0) {
    return C_REST_LOG_ERR(C_REST_ERROR_GENERIC, "C_REST_MALLOC failed");
  }

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
  ctx->sp_server_sock = C_REST_INVALID_SOCKET;
  ctx->sp_is_running = 0;
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
  /* c_rest_hot_reload_init is left for the user to explicitly call if they want
   * it. */
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
  c_rest_error_t res;
  if (!ctx) {
    return C_REST_ERROR_GENERIC;
  }

  /* Initialize c-orm connection pool if configured */
  if (ctx->db_config.connection_string != NULL) {
    if (ctx->logger.log_cb) {
      rc = ctx->logger.log_cb("Initializing c-orm database connection pool...");
      if (rc != C_REST_OK)
        return rc;
    }
    res = c_rest_orm_init(&ctx->db_config, &ctx->db_pool);
    if (res != 0) {
      if (ctx->logger.log_cb) {
        c_rest_error_t lrc = ctx->logger.log_cb(
            "Failed to initialize database connection pool.");
        if (lrc != C_REST_OK)
          return lrc;
      }
      return res;
    }
  }

  if (ctx->vtable && ctx->vtable->run) {
    return ctx->vtable->run(ctx);
  }

  if (ctx->logger.log_cb) {
    c_rest_error_t lrc = ctx->logger.log_cb(
        "No run loop implemented for the selected modality.");
    if (lrc != C_REST_OK)
      return lrc;
  }
  return C_REST_ERROR_GENERIC;
}

c_rest_error_t c_rest_stop(struct c_rest_context *ctx) {
  if (!ctx) {
    return C_REST_ERROR_GENERIC;
  }

  if (ctx->vtable && ctx->vtable->stop) {
    return ctx->vtable->stop(ctx);
  }

  if (ctx->logger.log_cb) {
    c_rest_error_t lrc =
        ctx->logger.log_cb("No stop implemented for the selected modality.");
    if (lrc != C_REST_OK)
      return lrc;
  }
  return C_REST_ERROR_GENERIC;
}

c_rest_error_t c_rest_destroy(struct c_rest_context *ctx) {
  c_rest_error_t res = C_REST_OK;
  c_rest_error_t rc;
  if (!ctx) {
    return C_REST_ERROR_GENERIC;
  }

  if (ctx->db_pool) {
    if (ctx->logger.log_cb) {
      rc = ctx->logger.log_cb("Cleaning up database connection pool...");
      if (rc != C_REST_OK)
        return rc;
    }
    rc = c_rest_orm_cleanup(ctx->db_pool);
    if (rc != C_REST_OK)
      return rc;
    ctx->db_pool = NULL;
  }

  if (ctx->vtable && ctx->vtable->destroy) {
    res = ctx->vtable->destroy(ctx);
    if (res != C_REST_OK)
      return res;
  }

  if (ctx->allocator.free_cb) {
    ctx->allocator.free_cb(ctx);
  } else {
    C_REST_FREE((void *)(ctx));
  }

  rc = c_rest_platform_cleanup();
  if (rc != C_REST_OK)
    return rc;

  return res;
}

#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
c_rest_error_t c_rest_set_multiplatform_env(struct c_rest_context *ctx,
                                            cm_env_t env) {
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

struct connection_state {
  struct c_rest_request req;
  char *method;
  char *url;
  int is_done;
};

static NO_DISCARD c_rest_error_t on_method(c_rest_parser_context *pctx,
                                           const char *method, size_t len) {
  struct connection_state *st = (struct connection_state *)pctx->user_data;
  if (C_REST_MALLOC(len + 1, &st->method) != 0) {
    st->method = NULL;
    return C_REST_LOG_ERR(C_REST_ERROR_OOM, "C_REST_MALLOC failed");
  }
#if defined(_MSC_VER)
  /* CDD_SAFE_CRT */ memcpy_s(st->method, len, method, len);
#else
  memcpy(st->method, method, len);
#endif
  st->method[len] = '\0';
  return C_REST_OK;
}

static NO_DISCARD c_rest_error_t on_url(c_rest_parser_context *pctx,
                                        const char *url, size_t len) {
  struct connection_state *st = (struct connection_state *)pctx->user_data;
  if (C_REST_MALLOC(len + 1, &st->url) != 0) {
    st->url = NULL;
    return C_REST_LOG_ERR(C_REST_ERROR_OOM, "C_REST_MALLOC failed");
  }
#if defined(_MSC_VER)
  /* CDD_SAFE_CRT */ memcpy_s(st->url, len, url, len);
#else
  memcpy(st->url, url, len);
#endif
  st->url[len] = '\0';
  return C_REST_OK;
}

static NO_DISCARD c_rest_error_t on_header(c_rest_parser_context *pctx,
                                           const char *key, size_t key_len,
                                           const char *val, size_t val_len) {
  struct connection_state *st = (struct connection_state *)pctx->user_data;
  struct c_rest_header *h = NULL;
  if (C_REST_MALLOC(sizeof(struct c_rest_header), &h) != 0) {
    return C_REST_LOG_ERR(C_REST_ERROR_OOM, "C_REST_MALLOC failed");
  }
  if (C_REST_MALLOC(key_len + 1, &h->key) != 0) {
    C_REST_FREE((void *)(h));
    return C_REST_LOG_ERR(C_REST_ERROR_OOM, "C_REST_MALLOC failed");
  }
  if (C_REST_MALLOC(val_len + 1, &h->value) != 0) {
    C_REST_FREE((void *)(h->key));
    C_REST_FREE((void *)(h));
    return C_REST_LOG_ERR(C_REST_ERROR_OOM, "C_REST_MALLOC failed");
  }
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
  return C_REST_OK;
}

static NO_DISCARD c_rest_error_t on_body(c_rest_parser_context *pctx,
                                         const char *data, size_t len) {
  struct connection_state *st = (struct connection_state *)pctx->user_data;
  char *new_body = NULL;

  if (C_REST_REALLOC(st->req.body, st->req.body_len + len + 1,
                     (void **)&new_body) != 0) {
    return C_REST_LOG_ERR(C_REST_ERROR_OOM, "C_REST_REALLOC failed");
  }

#if defined(_MSC_VER)
  /* CDD_SAFE_CRT */ memcpy_s(new_body + st->req.body_len, len, data, len);
#else
  memcpy(new_body + st->req.body_len, data, len);
#endif
  st->req.body = new_body;
  st->req.body_len += len;
  st->req.body[st->req.body_len] = '\0';
  return C_REST_OK;
}

static NO_DISCARD c_rest_error_t on_complete(c_rest_parser_context *pctx) {
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

      rc = c_rest_parser_get_basic_vtable(&vt);
      if (rc != C_REST_OK) {
        keep_alive = 0;
        goto conn_finish;
      }
      rc = c_rest_parser_init(&pctx, vt, &cbs, &st);
      if (rc != C_REST_OK) {
        keep_alive = 0;
        goto conn_iter_done;
      }
      {
        int read_eof = 0;
        for (;;) {
          if (!tls_conn) {
            rc = c_rest_socket_recv((c_rest_socket_t)sock, buf, sizeof(buf),
                                    &read_bytes);
            if (rc != C_REST_OK) {
              read_eof = 1;
              goto conn_read_done;
            }
            goto conn_parse_exec;
          }
          rc = c_rest_tls_read(tls_conn, buf, sizeof(buf), &read_bytes);
          if (rc != C_REST_OK) {
            read_eof = 1;
            goto conn_read_done;
          }
        conn_parse_exec:;

          rc = c_rest_parser_execute(&pctx, buf, read_bytes, &parsed_bytes);
          if (rc != C_REST_OK) {
            read_eof = 1;
            goto conn_read_done;
          }

          if (st.is_done)
            break;
        }
      conn_read_done:;

        if (st.is_done) {
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
            if (rc != C_REST_OK) {
              goto conn_cleanup_req;
            }
          }

          if (!res_obj.headers_sent) {
            rc = c_rest_response_send(&res_obj);
            if (rc != C_REST_OK) {
              goto conn_cleanup_req;
            }
          }
        }

      conn_cleanup_req:;
        rc = c_rest_request_cleanup(&st.req);
        if (rc != C_REST_OK) {
          goto conn_cleanup_res;
        }
      conn_cleanup_res:;
        rc = c_rest_response_cleanup(&res_obj);
        if (rc != C_REST_OK) {
          goto conn_parser_cleanup;
        }

      conn_parser_cleanup:;
        rc = c_rest_parser_should_keep_alive(&pctx, &keep_alive);
        if (rc != C_REST_OK) {
          keep_alive = 0;
          goto conn_destroy_parser;
        }
        if (read_eof)
          keep_alive = 0;
      conn_destroy_parser:;
        rc = c_rest_parser_destroy(&pctx);
        if (rc != C_REST_OK) {
          goto conn_free_strings;
        }

      conn_free_strings:;
        if (st.method)
          C_REST_FREE((void *)(st.method));
        if (st.url)
          C_REST_FREE((void *)(st.url));
      }

    conn_iter_done:
      if (!keep_alive) {
        done = 1;
      }
    }
  }

conn_finish:
  if (tls_conn) {
    c_rest_error_t tls_close_rc = c_rest_tls_close(tls_conn);
    if (tls_close_rc != C_REST_OK)
      return tls_close_rc;
  }

  rc = C_REST_OK;
  return rc;
}
