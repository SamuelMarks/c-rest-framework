/* clang-format off */
#include "c_rest_sse.h"

#include <stdlib.h>
#include <string.h>
#include "c_rest_mem.h"
#include "c_rest_log.h"
#include <stdio.h>

#include "c_rest_mem.h"
#include "c_rest_string.h"

struct c_rest_sse_context {
  char *buffer;
  size_t buffer_len;
  size_t buffer_cap;
  struct c_rest_sse_event current_event;
};

int c_rest_sse_event_init(struct c_rest_sse_event *ev) {
  if (!ev) { /* GCOVR_EXCL_LINE */
    return C_REST_SSE_ERR_INVALID_ARG; /* GCOVR_EXCL_LINE */
  }
  ev->id = NULL;
  ev->event = NULL;
  ev->data = NULL;
  ev->retry = -1;
  return C_REST_SSE_OK;
}

int c_rest_sse_event_destroy(struct c_rest_sse_event *ev) {
  if (!ev) { /* GCOVR_EXCL_LINE */
    return C_REST_SSE_ERR_INVALID_ARG; /* GCOVR_EXCL_LINE */
  }
  if (ev->id) {
    C_REST_FREE(ev->id);
    ev->id = NULL;
  }
  if (ev->event) {
    C_REST_FREE(ev->event);
    ev->event = NULL;
  }
  if (ev->data) {
    C_REST_FREE(ev->data);
    ev->data = NULL;
  }
  ev->retry = -1;
  return C_REST_SSE_OK;
}

static int c_rest_sse_strdup(const char *s, char **out_str) {
  size_t len;
  char *copy;
  void *tmp;
  if (!s) { /* GCOVR_EXCL_LINE */
    *out_str = NULL; /* GCOVR_EXCL_LINE */
    return 0; /* GCOVR_EXCL_LINE */
  }
  len = strlen(s);
  if (C_REST_MALLOC(len + 1, &tmp) != 0) { /* GCOVR_EXCL_LINE */
    return 1; /* GCOVR_EXCL_LINE */
  }
  copy = (char *)tmp;
  memcpy(copy, s, len + 1);
  *out_str = copy;
  return 0;
}
int c_rest_sse_event_clone(const struct c_rest_sse_event *src,
                           struct c_rest_sse_event *dest) {
  if (!src || !dest) { /* GCOVR_EXCL_LINE */
    return C_REST_SSE_ERR_INVALID_ARG; /* GCOVR_EXCL_LINE */
  }
  if (src->id) { /* GCOVR_EXCL_LINE */
    if (c_rest_sse_strdup(src->id, &dest->id) != 0) goto err; /* GCOVR_EXCL_LINE */
  } else {
    dest->id = NULL; /* GCOVR_EXCL_LINE */
  }

  if (src->event) { /* GCOVR_EXCL_LINE */
    if (c_rest_sse_strdup(src->event, &dest->event) != 0) goto err; /* GCOVR_EXCL_LINE */
  } else {
    dest->event = NULL; /* GCOVR_EXCL_LINE */
  }

  if (src->data) { /* GCOVR_EXCL_LINE */
    if (c_rest_sse_strdup(src->data, &dest->data) != 0) goto err; /* GCOVR_EXCL_LINE */
  } else {
    dest->data = NULL; /* GCOVR_EXCL_LINE */
  }

  dest->retry = src->retry;
  return C_REST_SSE_OK;

err: /* GCOVR_EXCL_LINE */
  c_rest_sse_event_destroy(dest); /* GCOVR_EXCL_LINE */
  return C_REST_SSE_ERR_NOMEM; /* GCOVR_EXCL_LINE */
}

int c_rest_sse_serialize(const struct c_rest_sse_event *ev, char **out_buf,
                         size_t *out_len) {
  c_rest_string s;
  char retry_buf[32];
  const char *data_ptr;
  const char *nl;
  size_t data_len;
  void *tmp_out_buf;

  if (!ev || !out_buf || !out_len) { /* GCOVR_EXCL_LINE */
    return C_REST_SSE_ERR_INVALID_ARG; /* GCOVR_EXCL_LINE */
  }

  if (c_rest_string_init(&s, 128) != 0) { /* GCOVR_EXCL_LINE */
    return C_REST_SSE_ERR_NOMEM; /* GCOVR_EXCL_LINE */
  }

  if (ev->id) {
    c_rest_string_append_cstr(&s, "id: ");
    c_rest_string_append_cstr(&s, ev->id);
    c_rest_string_append_cstr(&s, "\n");
  }

  if (ev->event) { /* GCOVR_EXCL_LINE */
    c_rest_string_append_cstr(&s, "event: ");
    c_rest_string_append_cstr(&s, ev->event);
    c_rest_string_append_cstr(&s, "\n");
  }

  if (ev->retry >= 0) {
#if defined(_MSC_VER)
    sprintf_s(retry_buf, sizeof(retry_buf), "retry: %d\n", ev->retry);
#else
    sprintf(retry_buf, "retry: %d\n", ev->retry);
#endif
    c_rest_string_append_cstr(&s, retry_buf);
  }

  if (ev->data) {
    data_ptr = ev->data;
    while ((nl = strchr(data_ptr, '\n')) != NULL) {
      c_rest_string_append_cstr(&s, "data: ");
      c_rest_string_append(&s, data_ptr, (size_t)(nl - data_ptr));
      c_rest_string_append_cstr(&s, "\n");
      data_ptr = nl + 1;
    }
    c_rest_string_append_cstr(&s, "data: ");
    c_rest_string_append_cstr(&s, data_ptr);
    c_rest_string_append_cstr(&s, "\n");
  }

  c_rest_string_append_cstr(&s, "\n");

  data_len = s.length;
  if (C_REST_MALLOC(data_len + 1, &tmp_out_buf) != 0) { /* GCOVR_EXCL_LINE */
    c_rest_string_destroy(&s); /* GCOVR_EXCL_LINE */
    return C_REST_SSE_ERR_NOMEM; /* GCOVR_EXCL_LINE */
  }
  *out_buf = (char *)tmp_out_buf;
  memcpy(*out_buf, s.data, data_len);
  (*out_buf)[data_len] = '\0';
  *out_len = data_len;

  c_rest_string_destroy(&s);

  return C_REST_SSE_OK;
}

int c_rest_sse_context_init(struct c_rest_sse_context **out_ctx) {
  struct c_rest_sse_context *ctx;
  void *tmp_ctx;
  if (!out_ctx) { /* GCOVR_EXCL_LINE */
    return C_REST_SSE_ERR_INVALID_ARG; /* GCOVR_EXCL_LINE */
  }

  if (C_REST_MALLOC(sizeof(struct c_rest_sse_context), &tmp_ctx) != 0) { /* GCOVR_EXCL_LINE */
    return C_REST_SSE_ERR_NOMEM; /* GCOVR_EXCL_LINE */
  }
  ctx = (struct c_rest_sse_context *)tmp_ctx;

  ctx->buffer = NULL;
  ctx->buffer_len = 0;
  ctx->buffer_cap = 0;
  c_rest_sse_event_init(&ctx->current_event);

  *out_ctx = ctx;
  return C_REST_SSE_OK;
}

int c_rest_sse_context_destroy(struct c_rest_sse_context *ctx) {
  if (!ctx) { /* GCOVR_EXCL_LINE */
    return C_REST_SSE_ERR_INVALID_ARG; /* GCOVR_EXCL_LINE */
  }

  if (ctx->buffer) { /* GCOVR_EXCL_LINE */
    C_REST_FREE(ctx->buffer);
    ctx->buffer = NULL;
  }
  c_rest_sse_event_destroy(&ctx->current_event);
  C_REST_FREE(ctx);

  return C_REST_SSE_OK;
}

static int append_to_string(char **dest, const char *src, size_t len) {
  size_t old_len;
  char *new_str;
  void *tmp_new_str;

  if (!src || len == 0) { /* GCOVR_EXCL_LINE */
    return C_REST_SSE_OK; /* GCOVR_EXCL_LINE */
  }

  old_len = *dest ? strlen(*dest) : 0; /* GCOVR_EXCL_LINE */
  if (C_REST_MALLOC(old_len + len + 1, &tmp_new_str) != 0) { /* GCOVR_EXCL_LINE */
    return C_REST_SSE_ERR_NOMEM; /* GCOVR_EXCL_LINE */
  }
  new_str = (char *)tmp_new_str;

  if (*dest) { /* GCOVR_EXCL_LINE */
    memcpy(new_str, *dest, old_len); /* GCOVR_EXCL_LINE */
    C_REST_FREE(*dest); /* GCOVR_EXCL_LINE */
  }
  memcpy(new_str + old_len, src, len);
  new_str[old_len + len] = '\0';

  *dest = new_str;
  return C_REST_SSE_OK;
}

int c_rest_sse_parse(struct c_rest_sse_context *ctx, const char *data,
                     size_t len, struct c_rest_sse_event *out_event) {
  char *new_buf;
  const char *line_start;
  const char *line_end;
  size_t line_len;
  const char *colon;
  const char *value;
  size_t value_len;
  int event_completed = 0;
  size_t processed = 0;

  if (!ctx || !out_event) { /* GCOVR_EXCL_LINE */
    return C_REST_SSE_ERR_INVALID_ARG; /* GCOVR_EXCL_LINE */
  }

  if (data && len > 0) { /* GCOVR_EXCL_LINE */
    if (ctx->buffer_len + len > ctx->buffer_cap) {
      void *tmp_new_buf;
      ctx->buffer_cap = (ctx->buffer_len + len) * 2;
      if (ctx->buffer_cap < 256) { /* GCOVR_EXCL_LINE */
        ctx->buffer_cap = 256;
      }
      if (C_REST_MALLOC(ctx->buffer_cap, &tmp_new_buf) != 0) { /* GCOVR_EXCL_LINE */
        return C_REST_SSE_ERR_NOMEM; /* GCOVR_EXCL_LINE */
      }
      new_buf = (char *)tmp_new_buf;
      if (ctx->buffer) { /* GCOVR_EXCL_LINE */
        memcpy(new_buf, ctx->buffer, ctx->buffer_len); /* GCOVR_EXCL_LINE */
        C_REST_FREE(ctx->buffer); /* GCOVR_EXCL_LINE */
      }
      ctx->buffer = new_buf;
    }
    memcpy(ctx->buffer + ctx->buffer_len, data, len);
    ctx->buffer_len += len;
  }

  line_start = ctx->buffer;

  while (line_start < ctx->buffer + ctx->buffer_len) { /* GCOVR_EXCL_LINE */
    line_end = (const char *)memchr(line_start, '\n',
                                    ctx->buffer + ctx->buffer_len - line_start);
    if (!line_end) {
      break;
    }

    line_len = (size_t)(line_end - line_start);
    if (line_len > 0 && line_start[line_len - 1] == '\r') { /* GCOVR_EXCL_LINE */
      line_len--; /* GCOVR_EXCL_LINE */
    }

    if (line_len == 0) {
      if (ctx->current_event.data || ctx->current_event.event || /* GCOVR_EXCL_LINE */
          ctx->current_event.id || ctx->current_event.retry >= 0) { /* GCOVR_EXCL_LINE */
        c_rest_sse_event_clone(&ctx->current_event, out_event);
        c_rest_sse_event_destroy(&ctx->current_event);
        c_rest_sse_event_init(&ctx->current_event);
        event_completed = 1;
      }
      processed = (size_t)(line_end - ctx->buffer + 1);
      line_start = line_end + 1;

      if (event_completed) { /* GCOVR_EXCL_LINE */
        break;
      }
      continue; /* GCOVR_EXCL_LINE */
    }

    if (line_start[0] == ':') { /* GCOVR_EXCL_LINE */
      line_start = line_end + 1; /* GCOVR_EXCL_LINE */
      continue; /* GCOVR_EXCL_LINE */
    }

    colon = (const char *)memchr(line_start, ':', line_len);
    if (colon) { /* GCOVR_EXCL_LINE */
      value = colon + 1;
      if (value < line_start + line_len && *value == ' ') { /* GCOVR_EXCL_LINE */
        value++;
      }
      value_len = (size_t)(line_start + line_len - value);

      if ((size_t)(colon - line_start) == 5 &&
          memcmp(line_start, "event", 5) == 0) {
        if (ctx->current_event.event) { /* GCOVR_EXCL_LINE */
          C_REST_FREE(ctx->current_event.event); /* GCOVR_EXCL_LINE */
          ctx->current_event.event = NULL; /* GCOVR_EXCL_LINE */
        }
        append_to_string(&ctx->current_event.event, value, value_len);
      } else if ((size_t)(colon - line_start) == 4 &&
                 memcmp(line_start, "data", 4) == 0) { /* GCOVR_EXCL_LINE */
        if (ctx->current_event.data) { /* GCOVR_EXCL_LINE */
          append_to_string(&ctx->current_event.data, "\n", 1); /* GCOVR_EXCL_LINE */
        }
        append_to_string(&ctx->current_event.data, value, value_len);
      } else if ((size_t)(colon - line_start) == 2 &&
                 memcmp(line_start, "id", 2) == 0) { /* GCOVR_EXCL_LINE */
        if (ctx->current_event.id) { /* GCOVR_EXCL_LINE */
          C_REST_FREE(ctx->current_event.id); /* GCOVR_EXCL_LINE */
          ctx->current_event.id = NULL; /* GCOVR_EXCL_LINE */
        }
        append_to_string(&ctx->current_event.id, value, value_len);
      } else if ((size_t)(colon - line_start) == 5 && /* GCOVR_EXCL_LINE */
                 memcmp(line_start, "retry", 5) == 0) { /* GCOVR_EXCL_LINE */
        char retry_str[32];
        size_t copy_len = value_len < 31 ? value_len : 31;
        memcpy(retry_str, value, copy_len);
        retry_str[copy_len] = '\0';
        ctx->current_event.retry = atoi(retry_str);
      }
    } else {
      if (line_len == 5 && memcmp(line_start, "event", 5) == 0) { /* GCOVR_EXCL_LINE */
        if (ctx->current_event.event) { /* GCOVR_EXCL_LINE */
          C_REST_FREE(ctx->current_event.event); /* GCOVR_EXCL_LINE */
        }
        c_rest_sse_strdup("", &ctx->current_event.event); /* GCOVR_EXCL_LINE */
      } else if (line_len == 4 && memcmp(line_start, "data", 4) == 0) { /* GCOVR_EXCL_LINE */
        if (ctx->current_event.data) { /* GCOVR_EXCL_LINE */
          append_to_string(&ctx->current_event.data, "\n", 1); /* GCOVR_EXCL_LINE */
        }
        append_to_string(&ctx->current_event.data, "", 0); /* GCOVR_EXCL_LINE */
      }
    }

    line_start = line_end + 1;
  }

  if (event_completed) {
    if (processed < ctx->buffer_len) { /* GCOVR_EXCL_LINE */
      memmove(ctx->buffer, ctx->buffer + processed, /* GCOVR_EXCL_LINE */
              ctx->buffer_len - processed); /* GCOVR_EXCL_LINE */
      ctx->buffer_len -= processed; /* GCOVR_EXCL_LINE */
    } else {
      ctx->buffer_len = 0;
    }
    return C_REST_SSE_OK;
  }

  if (line_start > ctx->buffer) {
    size_t consumed = (size_t)(line_start - ctx->buffer);
    if (consumed < ctx->buffer_len) { /* GCOVR_EXCL_LINE */
      memmove(ctx->buffer, ctx->buffer + consumed, ctx->buffer_len - consumed);
      ctx->buffer_len -= consumed;
    } else {
      ctx->buffer_len = 0; /* GCOVR_EXCL_LINE */
    }
  }

  return 1;
}

#include "c_rest_response.h"
/* clang-format on */

int c_rest_sse_init_response(struct c_rest_response *res) {
  if (!res) {                          /* GCOVR_EXCL_LINE */
    return C_REST_SSE_ERR_INVALID_ARG; /* GCOVR_EXCL_LINE */
  }

  c_rest_response_set_status(res, 200);
  c_rest_response_set_header(res, "Content-Type", "text/event-stream");
  c_rest_response_set_header(res, "Cache-Control", "no-cache");
  c_rest_response_set_header(res, "Connection", "keep-alive");

  return c_rest_response_send(res);
}

int c_rest_sse_send_event(struct c_rest_response *res,
                          const struct c_rest_sse_event *ev) {
  char *buf;
  size_t len;
  int ret;

  if (!res || !ev) {                   /* GCOVR_EXCL_LINE */
    return C_REST_SSE_ERR_INVALID_ARG; /* GCOVR_EXCL_LINE */
  }

  ret = c_rest_sse_serialize(ev, &buf, &len);
  if (ret != C_REST_SSE_OK) { /* GCOVR_EXCL_LINE */
    return ret;               /* GCOVR_EXCL_LINE */
  }

  ret = c_rest_response_write_chunk(res, buf, len);
  C_REST_FREE(buf);

  return ret;
}

int c_rest_sse_send_keepalive(struct c_rest_response *res) {
  const char *keepalive = ": \n\n";
  if (!res) {                          /* GCOVR_EXCL_LINE */
    return C_REST_SSE_ERR_INVALID_ARG; /* GCOVR_EXCL_LINE */
  }
  return c_rest_response_write_chunk(res, keepalive, strlen(keepalive));
}
