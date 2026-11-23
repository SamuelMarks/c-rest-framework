/* clang-format off */
#include "c_rest_testing_mocks.h"
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "c_rest_parser.h"
#include "c_rest_str_utils.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "c_rest_log.h"
/* clang-format on */

c_rest_error_t c_rest_parser_init(
    c_rest_parser_context *ctx, const struct c_rest_parser_vtable *vtable,
    const struct c_rest_parser_callbacks *callbacks, void *user_data) {
  if (!ctx || !vtable || !vtable->init) {
    return C_REST_ERROR_GENERIC;
  }

  ctx->vtable = vtable;
  if (callbacks) {
    ctx->callbacks = *callbacks;
  } else {
    ctx->callbacks.on_method = NULL;
    ctx->callbacks.on_url = NULL;
    ctx->callbacks.on_header = NULL;
    ctx->callbacks.on_body = NULL;
    ctx->callbacks.on_complete = NULL;
    ctx->callbacks.on_error = NULL;
  }
  ctx->user_data = user_data;
  ctx->internal_state = NULL;

  return ctx->vtable->init(ctx, &ctx->callbacks, user_data);
}

c_rest_error_t c_rest_parser_execute(c_rest_parser_context *ctx,
                                     const char *data, size_t len,
                                     size_t *out_parsed) {
  if (!out_parsed)
    return C_REST_ERROR_GENERIC;
  *out_parsed = 0;
  if (!ctx || !data || !ctx->vtable || !ctx->vtable->execute) {
    return C_REST_ERROR_GENERIC;
  }
  return ctx->vtable->execute(ctx, data, len, out_parsed);
}

c_rest_error_t c_rest_parser_should_keep_alive(c_rest_parser_context *ctx,
                                               int *out_keep_alive) {
#ifdef C_REST_TESTING_MALLOC_HOOK
  c_rest_error_t seam_rc = crf_seam_parser_keep_alive_hook();
  if (seam_rc != C_REST_OK)
    return seam_rc;
#endif
  if (!ctx || !out_keep_alive)
    return C_REST_ERROR_GENERIC;
  if (!ctx->vtable || !ctx->vtable->should_keep_alive) {
    *out_keep_alive = 0;
    return C_REST_OK;
  }
  return ctx->vtable->should_keep_alive(ctx, out_keep_alive);
}

c_rest_error_t c_rest_parser_destroy(c_rest_parser_context *ctx) {
#ifdef C_REST_TESTING_MALLOC_HOOK
  c_rest_error_t seam_rc = crf_seam_parser_destroy_hook();
  if (seam_rc != C_REST_OK)
    return seam_rc;
#endif
  if (!ctx || !ctx->vtable || !ctx->vtable->destroy) {
    return C_REST_ERROR_GENERIC;
  }
  return ctx->vtable->destroy(ctx);
}

/* Basic parser implementation */

struct basic_parser_state {
  int state; /* 0:start, 2:url, 3:version, 4:header_key, 5:header_val,
                6:headers_done, 7:body_identity, 8:chunk_size, 9:chunk_ext,
                10:chunk_data, 11:chunk_crlf */
  int has_error;
  int keep_alive;
  size_t content_length;
  int is_chunked;

  char *buf;
  size_t buf_len;
  size_t buf_cap;

  char *key_buf;
  size_t key_len;
  size_t key_cap;

  size_t body_read;
  size_t chunk_left;
};

static NO_DISCARD c_rest_error_t
basic_init(c_rest_parser_context *ctx,
           const struct c_rest_parser_callbacks *callbacks, void *user_data) {
  struct basic_parser_state *st;
  (void)callbacks;
  (void)user_data;

  if (C_REST_MALLOC(sizeof(struct basic_parser_state), &st) != 0) {
    return C_REST_LOG_ERR(C_REST_ERROR_GENERIC, "C_REST_MALLOC failed");
  }

  st->state = 0;
  st->has_error = 0;
  st->keep_alive = 1;
  st->content_length = 0;
  st->is_chunked = 0;
  st->buf = NULL;
  st->buf_len = 0;
  st->buf_cap = 0;
  st->key_buf = NULL;
  st->key_len = 0;
  st->key_cap = 0;
  st->body_read = 0;
  st->chunk_left = 0;

  ctx->internal_state = st;
  return C_REST_OK;
}

#define C_REST_MAX_PARSER_BUF_SIZE ((size_t)65536)

static NO_DISCARD c_rest_error_t append_buf(struct basic_parser_state *st,
                                            char c, int is_key) {
  if (is_key) {
    if (st->key_len + 1 >= C_REST_MAX_PARSER_BUF_SIZE) {
      st->has_error = 1;
      return C_REST_ERROR_PARSE;
    }
    if (st->key_len + 1 >= st->key_cap) {
      size_t new_cap = st->key_cap == 0 ? 64 : st->key_cap * 2;
      char *n = NULL;
      if (C_REST_REALLOC(st->key_buf, new_cap, &n) != 0) {
        return C_REST_LOG_ERR(C_REST_ERROR_GENERIC, "C_REST_REALLOC failed");
      }
      st->key_buf = n;
      st->key_cap = new_cap;
    }
    st->key_buf[st->key_len++] = c;
    st->key_buf[st->key_len] = '\0';
  } else {
    if (st->buf_len + 1 >= C_REST_MAX_PARSER_BUF_SIZE) {
      st->has_error = 1;
      return C_REST_ERROR_PARSE;
    }
    if (st->buf_len + 1 >= st->buf_cap) {
      size_t new_cap = st->buf_cap == 0 ? 256 : st->buf_cap * 2;
      char *n = NULL;
      if (C_REST_REALLOC(st->buf, new_cap, &n) != 0) {
        return C_REST_LOG_ERR(C_REST_ERROR_GENERIC, "C_REST_REALLOC failed");
      }
      st->buf = n;
      st->buf_cap = new_cap;
    }
    st->buf[st->buf_len++] = c;
    st->buf[st->buf_len] = '\0';
  }
  return C_REST_OK;
}

static NO_DISCARD c_rest_error_t parser_fail(c_rest_parser_context *ctx,
                                             struct basic_parser_state *st,
                                             const char *msg) {
  st->has_error = 1;
  if (ctx->callbacks.on_error) {
    c_rest_error_t cb_rc = ctx->callbacks.on_error(ctx, msg);
    if (cb_rc != C_REST_OK)
      return cb_rc;
  }
  return C_REST_ERROR_PARSE;
}

static NO_DISCARD c_rest_error_t basic_execute(c_rest_parser_context *ctx,
                                               const char *data, size_t len,
                                               size_t *out_parsed) {
  struct basic_parser_state *st;
  size_t i = 0;
  c_rest_error_t rc;

  if (!ctx->internal_state)
    return C_REST_ERROR_GENERIC;
  st = (struct basic_parser_state *)ctx->internal_state;
  *out_parsed = 0;

  if (st->has_error)
    return C_REST_ERROR_GENERIC;

  while (i < len) {
    char c = data[i];
    unsigned char uc = (unsigned char)c;
    switch (st->state) {
    case 0: /* start / method (RFC 9112 Section 3.1) */
      if (c == ' ') {
        if (st->buf_len == 0) {
          return parser_fail(ctx, st, "Empty HTTP method");
        }
        if (ctx->callbacks.on_method) {
          rc = ctx->callbacks.on_method(ctx, st->buf, st->buf_len);
          if (rc != C_REST_OK)
            return rc;
        }
        st->buf_len = 0;
        st->state = 2; /* url */
        i++;
        break;
      } else if (c == '\r' || c == '\n') {
        if (st->buf_len > 0) {
          return parser_fail(ctx, st, "Incomplete request line");
        }
        i++;
        break;
      } else if (uc <= 0x20U || uc >= 0x7FU || c == ':' || c == '/') {
        return parser_fail(ctx, st, "Invalid method token");
      }

      rc = append_buf(st, c, 0);
      if (rc != C_REST_OK)
        return rc;

      i++;
      break;
    case 2: /* request-target (RFC 9112 Section 3.2) */
      if (c == ' ') {
        if (st->buf_len == 0) {
          return parser_fail(ctx, st, "Empty request target");
        }
        if (st->buf[0] != '/' && (st->buf_len != 1 || st->buf[0] != '*') &&
            strncmp(st->buf, "http://", 7) != 0 &&
            strncmp(st->buf, "https://", 8) != 0) {
          return parser_fail(ctx, st, "Invalid request target");
        }
        if (ctx->callbacks.on_url) {
          rc = ctx->callbacks.on_url(ctx, st->buf, st->buf_len);
          if (rc != C_REST_OK)
            return rc;
        }
        st->buf_len = 0;
        st->state = 3; /* version */
      } else if (uc <= 0x20U || uc >= 0x7FU) {
        return parser_fail(ctx, st, "Invalid character in request target");
      } else {
        rc = append_buf(st, c, 0);
        if (rc != C_REST_OK)
          return rc;
      }
      i++;
      break;
    case 3: /* HTTP-version (RFC 9112 Section 2.3) */
      if (c == '\r') {
        /* wait for \n */
      } else if (c == '\n') {
        if (st->buf_len != 8 || strncmp(st->buf, "HTTP/1.", 7) != 0 ||
            (st->buf[7] != '1' && st->buf[7] != '0')) {
          return parser_fail(ctx, st, "Invalid HTTP version");
        }
        if (st->buf[7] == '0')
          st->keep_alive = 0;
        st->state = 4; /* start header key */
        st->buf_len = 0;
      } else {
        rc = append_buf(st, c, 0);
        if (rc != C_REST_OK)
          return rc;
      }
      i++;
      break;
    case 4: /* header field-name (RFC 9112 Section 5) */
      if (c == '\r') {
        /* Empty line or end of line */
      } else if (c == '\n') {
        if (st->key_len > 0) {
          return parser_fail(ctx, st, "Malformed header line");
        }
        st->state = 6; /* headers done */
        if (!st->is_chunked && st->content_length == 0) {
          if (ctx->callbacks.on_complete) {
            rc = ctx->callbacks.on_complete(ctx);
            if (rc != C_REST_OK)
              return rc;
          }
        } else if (st->is_chunked) {
          st->state = 8; /* chunk size */
          st->buf_len = 0;
        } else {
          st->state = 7; /* body identity */
        }
      } else if (c == ':') {
        if (st->key_len == 0) {
          return parser_fail(ctx, st, "Empty header field name");
        }
        st->state = 5; /* header value */
        st->buf_len = 0;
      } else if (uc <= 0x20U || uc >= 0x7FU) {
        return parser_fail(ctx, st, "Invalid header field character");
      } else {
        rc = append_buf(st, c, 1);
        if (rc != C_REST_OK)
          return rc;
      }
      i++;
      break;
    case 5: /* header value */
      if (c == '\r') {
      } else if (c == '\n') {
        char empty_val[] = "";
        char *k = st->key_buf;
        char *v = st->buf_len > 0 ? st->buf : empty_val;
        size_t idx;
        int match_cl = 1, match_te = 1, match_conn = 1;
        static const char cl_lit[] = "content-length";
        static const char te_lit[] = "transfer-encoding";
        static const char conn_lit[] = "connection";
        static const char close_lit[] = "close";
        static const char ka_lit[] = "keep-alive";
        while (*v == ' ')
          v++;

        for (idx = 0; cl_lit[idx] != '\0' || k[idx] != '\0'; idx++) {
          if (tolower((unsigned char)k[idx]) != (unsigned char)cl_lit[idx]) {
            match_cl = 0;
            break;
          }
        }
        if (match_cl) {
          st->content_length = (size_t)strtoul(v, NULL, 10);
        } else {
          for (idx = 0; te_lit[idx] != '\0' || k[idx] != '\0'; idx++) {
            if (tolower((unsigned char)k[idx]) != (unsigned char)te_lit[idx]) {
              match_te = 0;
              break;
            }
          }
          if (match_te) {
            if (strstr(v, "chunked"))
              st->is_chunked = 1;
          } else {
            for (idx = 0; conn_lit[idx] != '\0' || k[idx] != '\0'; idx++) {
              if (tolower((unsigned char)k[idx]) !=
                  (unsigned char)conn_lit[idx]) {
                match_conn = 0;
                break;
              }
            }
            if (match_conn) {
              size_t v_trim_len = strlen(v);
              int match_close = 1;
              int match_ka = 1;
              while (v_trim_len > 0 && v[v_trim_len - 1] == ' ')
                v_trim_len--;
              if (v_trim_len != sizeof(close_lit) - 1)
                match_close = 0;
              for (idx = 0; match_close && idx < v_trim_len; idx++) {
                if (tolower((unsigned char)v[idx]) !=
                    (unsigned char)close_lit[idx]) {
                  match_close = 0;
                }
              }
              if (v_trim_len != sizeof(ka_lit) - 1)
                match_ka = 0;
              for (idx = 0; match_ka && idx < v_trim_len; idx++) {
                if (tolower((unsigned char)v[idx]) !=
                    (unsigned char)ka_lit[idx]) {
                  match_ka = 0;
                }
              }
              if (match_close)
                st->keep_alive = 0;
              else if (match_ka)
                st->keep_alive = 1;
            }
          }
        }

        if (ctx->callbacks.on_header) {
          rc = ctx->callbacks.on_header(ctx, k, st->key_len, v, strlen(v));
          if (rc != C_REST_OK)
            return rc;
        }
        st->key_len = 0;
        st->buf_len = 0;
        st->state = 4; /* next header key */
      } else {
        rc = append_buf(st, c, 0);
        if (rc != C_REST_OK)
          return rc;
      }
      i++;
      break;
    case 6:             /* headers done / wait */
      return C_REST_OK; /* Wait for external state change if needed */
    case 7:             /* body identity */
    {
      size_t to_read = len - i;
      size_t remaining = st->content_length - st->body_read;
      if (to_read > remaining)
        to_read = remaining;

      if (ctx->callbacks.on_body) {
        rc = ctx->callbacks.on_body(ctx, data + i, to_read);
        if (rc != C_REST_OK)
          return rc;
      }
      st->body_read += to_read;
      i += to_read;

      if (st->body_read >= st->content_length) {
        st->state = 6;
        if (ctx->callbacks.on_complete) {
          rc = ctx->callbacks.on_complete(ctx);
          if (rc != C_REST_OK)
            return rc;
        }
      }
    } break;
    case 8: /* chunk size (RFC 9112 Section 7.1) */
      if (c == ';') {
        if (st->buf_len == 0) {
          return parser_fail(ctx, st, "Empty chunk size");
        }
        st->state = 9; /* chunk extension */
      } else if (c == '\r') {
      } else if (c == '\n') {
        if (st->buf_len == 0) {
          return parser_fail(ctx, st, "Empty chunk size");
        }
        st->chunk_left = (size_t)strtoul(st->buf, NULL, 16);
        st->buf_len = 0;
        if (st->chunk_left == 0) {
          st->state = 6;
          if (ctx->callbacks.on_complete) {
            rc = ctx->callbacks.on_complete(ctx);
            if (rc != C_REST_OK)
              return rc;
          }
        } else {
          st->state = 10; /* chunk data */
        }
      } else if (!isxdigit(uc)) {
        return parser_fail(ctx, st, "Invalid chunk hex digit");
      } else {
        rc = append_buf(st, c, 0);
        if (rc != C_REST_OK)
          return rc;
      }
      i++;
      break;
    case 9: /* chunk extension */
      if (c == '\n') {
        st->chunk_left = (size_t)strtoul(st->buf, NULL, 16);
        st->buf_len = 0;
        if (st->chunk_left == 0) {
          st->state = 6;
          if (ctx->callbacks.on_complete) {
            rc = ctx->callbacks.on_complete(ctx);
            if (rc != C_REST_OK)
              return rc;
          }
        } else {
          st->state = 10; /* chunk data */
        }
      }
      i++;
      break;
    case 10: /* chunk data */
    {
      size_t to_read = len - i;
      if (to_read > st->chunk_left)
        to_read = st->chunk_left;

      if (ctx->callbacks.on_body) {
        rc = ctx->callbacks.on_body(ctx, data + i, to_read);
        if (rc != C_REST_OK)
          return rc;
      }
      st->chunk_left -= to_read;
      i += to_read;

      if (st->chunk_left == 0)
        st->state = 11; /* chunk end CR */
    } break;
    case 11: /* chunk crlf */
      if (c == '\n') {
        st->state = 8; /* next chunk size */
        st->buf_len = 0;
      }
      i++;
      break;
    default:
      st->has_error = 1;
      return C_REST_ERROR_GENERIC;
    }
  }

  *out_parsed = len;
  return C_REST_OK;
}

static NO_DISCARD c_rest_error_t
basic_should_keep_alive(c_rest_parser_context *ctx, int *out_keep_alive) {
  struct basic_parser_state *st;
  if (!ctx->internal_state)
    return C_REST_ERROR_GENERIC;
  st = (struct basic_parser_state *)ctx->internal_state;
  *out_keep_alive = st->keep_alive;
  return C_REST_OK;
}

static NO_DISCARD c_rest_error_t basic_destroy(c_rest_parser_context *ctx) {
  struct basic_parser_state *st;
  if (!ctx->internal_state)
    return C_REST_ERROR_GENERIC;
  st = (struct basic_parser_state *)ctx->internal_state;
  if (st->buf)
    C_REST_FREE((void *)(st->buf));
  if (st->key_buf)
    C_REST_FREE((void *)(st->key_buf));
  C_REST_FREE((void *)(st));
  ctx->internal_state = NULL;
  return C_REST_OK;
}

static const struct c_rest_parser_vtable basic_vtable = {
    basic_init, basic_execute, basic_should_keep_alive, basic_destroy};

c_rest_error_t
c_rest_parser_get_basic_vtable(const struct c_rest_parser_vtable **out_vtable) {
#ifdef C_REST_TESTING_MALLOC_HOOK
  c_rest_error_t seam_rc = crf_seam_parser_vtable_hook();
  if (seam_rc != C_REST_OK)
    return seam_rc;
#endif
  if (!out_vtable)
    return C_REST_ERROR_GENERIC;
  *out_vtable = &basic_vtable;
  return C_REST_OK;
}

c_rest_error_t c_rest_parser_is_complete(c_rest_parser_context *ctx,
                                         int *out_complete) {
  struct basic_parser_state *st;
  if (!out_complete)
    return C_REST_ERROR_GENERIC;
  *out_complete = 0;
  if (!ctx || ctx->vtable != &basic_vtable || !ctx->internal_state)
    return C_REST_ERROR_GENERIC;
  st = (struct basic_parser_state *)ctx->internal_state;
  *out_complete = (st->state == 6) ? 1 : 0;
  return C_REST_OK;
}
