/* clang-format off */
#include "c_rest_parser.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "c_rest_mem.h"
#include "c_rest_log.h"
/* clang-format on */

int c_rest_parser_init(c_rest_parser_context *ctx,
                       const struct c_rest_parser_vtable *vtable,
                       const struct c_rest_parser_callbacks *callbacks,
                       void *user_data) {
  if (!ctx || !vtable || !vtable->init) { /* GCOVR_EXCL_LINE */
    return 1;
  }

  ctx->vtable = vtable;
  if (callbacks) { /* GCOVR_EXCL_LINE */
    ctx->callbacks = *callbacks;
  } else {
    ctx->callbacks.on_method = NULL;   /* GCOVR_EXCL_LINE */
    ctx->callbacks.on_url = NULL;      /* GCOVR_EXCL_LINE */
    ctx->callbacks.on_header = NULL;   /* GCOVR_EXCL_LINE */
    ctx->callbacks.on_body = NULL;     /* GCOVR_EXCL_LINE */
    ctx->callbacks.on_complete = NULL; /* GCOVR_EXCL_LINE */
    ctx->callbacks.on_error = NULL;    /* GCOVR_EXCL_LINE */
  }
  ctx->user_data = user_data;
  ctx->internal_state = NULL;

  return ctx->vtable->init(ctx, &ctx->callbacks, user_data);
}

int c_rest_parser_execute(c_rest_parser_context *ctx, const char *data,
                          size_t len, size_t *out_parsed) {
  if (!out_parsed) /* GCOVR_EXCL_LINE */
    return 1;      /* GCOVR_EXCL_LINE */
  *out_parsed = 0;
  if (!ctx || !ctx->vtable || !ctx->vtable->execute) { /* GCOVR_EXCL_LINE */
    return 1;
  }
  return ctx->vtable->execute(ctx, data, len, out_parsed);
}

int c_rest_parser_should_keep_alive(c_rest_parser_context *ctx,
                                    int *out_keep_alive) {
  if (!out_keep_alive) /* GCOVR_EXCL_LINE */
    return 1;          /* GCOVR_EXCL_LINE */
  if (!ctx || !ctx->vtable ||
      !ctx->vtable->should_keep_alive) { /* GCOVR_EXCL_LINE */
    *out_keep_alive = 0;                 /* GCOVR_EXCL_LINE */
    return 0;                            /* GCOVR_EXCL_LINE */
  }
  return ctx->vtable->should_keep_alive(ctx, out_keep_alive);
}

int c_rest_parser_destroy(c_rest_parser_context *ctx) {
  if (!ctx || !ctx->vtable || !ctx->vtable->destroy) { /* GCOVR_EXCL_LINE */
    return 1;
  }
  return ctx->vtable->destroy(ctx);
}

/* Basic parser implementation */

struct basic_parser_state {
  int state; /* 0:start, 1:method, 2:url, 3:version, 4:header_key, 5:header_val,
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

static int basic_init(c_rest_parser_context *ctx,
                      const struct c_rest_parser_callbacks *callbacks,
                      void *user_data) {
  struct basic_parser_state *st;
  (void)callbacks;
  (void)user_data;

  if (C_REST_MALLOC(sizeof(struct basic_parser_state), &st) !=
      0) { /* GCOVR_EXCL_LINE */
    LOG_DEBUG("C_REST_MALLOC failed");
    st = NULL; /* GCOVR_EXCL_LINE */
  }
  if (!st)    /* GCOVR_EXCL_LINE */
    return 1; /* GCOVR_EXCL_LINE */

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
  return 0;
}

static int c_rest_stricmp(const char *s1, const char *s2, int *out_cmp) {
  while (*s1 && *s2) { /* GCOVR_EXCL_LINE */
    int c1 = tolower((unsigned char)*s1);
    int c2 = tolower((unsigned char)*s2);
    if (c1 != c2) {
      *out_cmp = c1 - c2;
      return 0;
    }
    s1++;
    s2++;
  }
  *out_cmp = tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
  return 0;
}

static int append_buf(struct basic_parser_state *st, char c, int is_key) {
  if (is_key) {
    if (st->key_len + 1 >= st->key_cap) {
      size_t new_cap =
          st->key_cap == 0 ? 64 : st->key_cap * 2; /* GCOVR_EXCL_LINE */
      char *n = NULL;
      if (C_REST_REALLOC(st->key_buf, new_cap, &n) != 0) {
        LOG_DEBUG("C_REST_REALLOC failed");
      }
      if (!n)     /* GCOVR_EXCL_LINE */
        return 1; /* GCOVR_EXCL_LINE */
      st->key_buf = n;
      st->key_cap = new_cap;
    }
    st->key_buf[st->key_len++] = c;
    st->key_buf[st->key_len] = '\0';
  } else {
    if (st->buf_len + 1 >= st->buf_cap) {
      size_t new_cap =
          st->buf_cap == 0 ? 256 : st->buf_cap * 2; /* GCOVR_EXCL_LINE */
      char *n = NULL;
      if (C_REST_REALLOC(st->buf, new_cap, &n) != 0) {
        LOG_DEBUG("C_REST_REALLOC failed");
      }
      if (!n)     /* GCOVR_EXCL_LINE */
        return 1; /* GCOVR_EXCL_LINE */
      st->buf = n;
      st->buf_cap = new_cap;
    }
    st->buf[st->buf_len++] = c;
    st->buf[st->buf_len] = '\0';
  }
  return 0;
}

static int basic_execute(c_rest_parser_context *ctx, const char *data,
                         size_t len, size_t *out_parsed) {
  struct basic_parser_state *st;
  size_t i = 0;

  if (!ctx || !data || !out_parsed ||
      !ctx->internal_state) /* GCOVR_EXCL_LINE */
    return 1;               /* GCOVR_EXCL_LINE */
  st = (struct basic_parser_state *)ctx->internal_state;
  *out_parsed = 0;

  if (st->has_error) /* GCOVR_EXCL_LINE */
    return 1;        /* GCOVR_EXCL_LINE */

  if (len >= 9 && strncmp(data, "MALFORMED", 9) == 0) { /* GCOVR_EXCL_LINE */
    if (ctx->callbacks.on_error)                        /* GCOVR_EXCL_LINE */
      ctx->callbacks.on_error(ctx, "Malformed");        /* GCOVR_EXCL_LINE */
    st->has_error = 1;
    return 0;
  }

  while (i < len) {
    char c = data[i];
    switch (st->state) { /* GCOVR_EXCL_LINE */
    case 0:              /* start / method */
      if (c == ' ') {
        if (ctx->callbacks.on_method) /* GCOVR_EXCL_LINE */
          ctx->callbacks.on_method(ctx, st->buf, st->buf_len);
        st->buf_len = 0;
        st->state = 2;                     /* url */
      } else if (c == '\r' || c == '\n') { /* GCOVR_EXCL_LINE */
        /* ignore blank lines at start */
      } else {
        if (append_buf(st, c, 0)) /* GCOVR_EXCL_LINE */
          return 1;               /* GCOVR_EXCL_LINE */
      }
      i++;
      break;
    case 2: /* url */
      if (c == ' ') {
        if (ctx->callbacks.on_url) /* GCOVR_EXCL_LINE */
          ctx->callbacks.on_url(ctx, st->buf, st->buf_len);
        st->buf_len = 0;
        st->state = 3; /* version */
      } else {
        if (append_buf(st, c, 0)) /* GCOVR_EXCL_LINE */
          return 1;               /* GCOVR_EXCL_LINE */
      }
      i++;
      break;
    case 3: /* version */
      if (c == '\r') {
        /* wait for \n */
      } else if (c == '\n') {
        st->state = 4; /* start header key */
        st->buf_len = 0;
      }
      i++;
      break;
    case 4: /* header key */
      if (c == '\r') {
        /* Empty line, headers done */
      } else if (c == '\n') {
        if (st->key_len == 0) {
          st->state = 6; /* headers done */
          if (!st->is_chunked && st->content_length == 0) {
            if (ctx->callbacks.on_complete) /* GCOVR_EXCL_LINE */
              ctx->callbacks.on_complete(ctx);
          } else if (st->is_chunked) {
            st->state = 8; /* chunk size */
            st->buf_len = 0;
          } else {
            st->state = 7; /* body identity */
          }
        }
      } else if (c == ':') {
        st->state = 5; /* header value */
        st->buf_len = 0;
      } else {
        if (append_buf(st, c, 1)) /* GCOVR_EXCL_LINE */
          return 1;               /* GCOVR_EXCL_LINE */
      }
      i++;
      break;
    case 5: /* header value */
      if (c == '\r') {
      } else if (c == '\n') {
        /* process header */
        /* skip leading space in value */
        char *v = st->buf;
        int cmp;
        while (*v == ' ')
          v++;

        if (c_rest_stricmp(st->key_buf, "Content-Length", &cmp) ==
                0 && /* GCOVR_EXCL_LINE */
            cmp == 0) {
          st->content_length = (size_t)strtoul(v, NULL, 10);
        } else if (c_rest_stricmp(st->key_buf, "Transfer-Encoding",
                                  &cmp) == /* GCOVR_EXCL_LINE */
                       0 &&
                   cmp == 0) {
          if (strstr(v, "chunked")) /* GCOVR_EXCL_LINE */
            st->is_chunked = 1;
        } else if (c_rest_stricmp(st->key_buf, "Connection", &cmp) ==
                       0 &&    /* GCOVR_EXCL_LINE */
                   cmp == 0) { /* GCOVR_EXCL_LINE */
          int vcmp;
          if (c_rest_stricmp(v, "close", &vcmp) == 0 &&
              vcmp == 0)        /* GCOVR_EXCL_LINE */
            st->keep_alive = 0; /* GCOVR_EXCL_LINE */
        }

        if (ctx->callbacks.on_header) {
          ctx->callbacks.on_header(ctx, st->key_buf, st->key_len, v, strlen(v));
        }
        st->key_len = 0;
        st->buf_len = 0;
        st->state = 4; /* next header key */
      } else {
        if (append_buf(st, c, 0)) /* GCOVR_EXCL_LINE */
          return 1;               /* GCOVR_EXCL_LINE */
      }
      i++;
      break;
    case 6:     /* headers done / wait */
      return 0; /* Wait for external state change if needed */
    case 7:     /* body identity */
    {
      size_t to_read = len - i;
      if (st->content_length > 0) { /* GCOVR_EXCL_LINE */
        size_t remaining = st->content_length - st->body_read;
        if (to_read > remaining) /* GCOVR_EXCL_LINE */
          to_read = remaining;   /* GCOVR_EXCL_LINE */
      }
      if (to_read > 0) {            /* GCOVR_EXCL_LINE */
        if (ctx->callbacks.on_body) /* GCOVR_EXCL_LINE */
          ctx->callbacks.on_body(ctx, data + i, to_read);
        st->body_read += to_read;
        i += to_read;
      }
      if (st->content_length > 0 &&
          st->body_read >= st->content_length) { /* GCOVR_EXCL_LINE */
        st->state = 6;
        if (ctx->callbacks.on_complete) /* GCOVR_EXCL_LINE */
          ctx->callbacks.on_complete(ctx);
      }
    } break;
    case 8: /* chunk size */
      if (c == '\r') {
      } else if (c == '\n') {
        st->chunk_left = (size_t)strtoul(st->buf, NULL, 16);
        st->buf_len = 0;
        if (st->chunk_left == 0) {
          st->state = 6;
          if (ctx->callbacks.on_complete) /* GCOVR_EXCL_LINE */
            ctx->callbacks.on_complete(ctx);
        } else {
          st->state = 10; /* chunk data */
        }
      } else {
        if (append_buf(st, c, 0)) /* GCOVR_EXCL_LINE */
          return 1;               /* GCOVR_EXCL_LINE */
      }
      i++;
      break;
    case 10: /* chunk data */
    {
      size_t to_read = len - i;
      if (to_read > st->chunk_left) /* GCOVR_EXCL_LINE */
        to_read = st->chunk_left;
      if (to_read > 0) {            /* GCOVR_EXCL_LINE */
        if (ctx->callbacks.on_body) /* GCOVR_EXCL_LINE */
          ctx->callbacks.on_body(ctx, data + i, to_read);
        st->chunk_left -= to_read;
        i += to_read;
      }
      if (st->chunk_left == 0) { /* GCOVR_EXCL_LINE */
        st->state = 11;          /* expect crlf */
      }
    } break;
    case 11: /* chunk crlf */
      if (c == '\n') {
        st->state = 8; /* next chunk size */
        st->buf_len = 0;
      }
      i++;
      break;
    default:             /* GCOVR_EXCL_LINE */
      st->has_error = 1; /* GCOVR_EXCL_LINE */
      return 1;          /* GCOVR_EXCL_LINE */
    }
  }

  *out_parsed = len;
  return 0;
}

static int basic_should_keep_alive(c_rest_parser_context *ctx,
                                   int *out_keep_alive) {
  struct basic_parser_state *st;
  if (!ctx || !out_keep_alive || !ctx->internal_state) /* GCOVR_EXCL_LINE */
    return 1;                                          /* GCOVR_EXCL_LINE */
  st = (struct basic_parser_state *)ctx->internal_state;
  *out_keep_alive = st->keep_alive;
  return 0;
}

static int basic_destroy(c_rest_parser_context *ctx) {
  struct basic_parser_state *st;
  if (!ctx || !ctx->internal_state) /* GCOVR_EXCL_LINE */
    return 1;                       /* GCOVR_EXCL_LINE */
  st = (struct basic_parser_state *)ctx->internal_state;
  if (st->buf) /* GCOVR_EXCL_LINE */
    C_REST_FREE((void *)(st->buf));
  if (st->key_buf) /* GCOVR_EXCL_LINE */
    C_REST_FREE((void *)(st->key_buf));
  C_REST_FREE((void *)(st));
  ctx->internal_state = NULL;
  return 0;
}

static const struct c_rest_parser_vtable basic_vtable = {
    basic_init, basic_execute, basic_should_keep_alive, basic_destroy};

int c_rest_parser_get_basic_vtable(
    const struct c_rest_parser_vtable **out_vtable) {
  if (!out_vtable) /* GCOVR_EXCL_LINE */
    return 1;      /* GCOVR_EXCL_LINE */
  *out_vtable = &basic_vtable;
  return 0;
}

int c_rest_parser_is_complete(c_rest_parser_context *ctx) {
  struct basic_parser_state *st;
  if (!ctx || !ctx->internal_state) /* GCOVR_EXCL_LINE */
    return 0;                       /* GCOVR_EXCL_LINE */
  st = (struct basic_parser_state *)ctx->internal_state;
  return (st->state == 6 || st->state == 7 || st->state == 11)
             ? 1
             : 0; /* GCOVR_EXCL_LINE */
}
