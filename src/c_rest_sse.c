#include "c_rest_testing_mocks.h"
/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "c_rest_sse.h"

#include <stdlib.h>
#include <string.h>
#include "c_rest_log.h"
#include <stdio.h>

#include "c_rest_string.h"

#ifdef C_REST_TESTING_MALLOC_HOOK
int g_mock_sse_append_fail = -1;
static c_rest_error_t mock_append_cstr(c_rest_string *s, const char *c) {
  if (g_mock_sse_append_fail == 0) return C_REST_ERROR_GENERIC;
  if (g_mock_sse_append_fail > 0) g_mock_sse_append_fail--;
  return c_rest_string_append_cstr(s, c);
}
static c_rest_error_t mock_append(c_rest_string *s, const char *c, size_t l) {
  if (g_mock_sse_append_fail == 0) return C_REST_ERROR_GENERIC;
  if (g_mock_sse_append_fail > 0) g_mock_sse_append_fail--;
  return c_rest_string_append(s, c, l);
}
static c_rest_error_t mock_string_destroy(c_rest_string *s) {
  if (g_mock_sse_append_fail == -2) return C_REST_ERROR_GENERIC;
  return c_rest_string_destroy(s);
}
#define c_rest_string_append_cstr mock_append_cstr
#define c_rest_string_append mock_append
#define c_rest_string_destroy mock_string_destroy

#define INTERNAL_EVENT_DESTROY(ev) (g_mock_sse_append_fail == -3 ? C_REST_ERROR_GENERIC : c_rest_sse_event_destroy(ev))
#define INTERNAL_EVENT_INIT(ev) (g_mock_sse_append_fail == -4 ? C_REST_ERROR_GENERIC : c_rest_sse_event_init(ev))
#else
#define INTERNAL_EVENT_DESTROY(ev) c_rest_sse_event_destroy(ev)
#define INTERNAL_EVENT_INIT(ev) c_rest_sse_event_init(ev)
#endif

struct c_rest_sse_context {
  char *buffer;
  size_t buffer_len;
  size_t buffer_cap;
  struct c_rest_sse_event current_event;
};

c_rest_error_t c_rest_sse_event_init(struct c_rest_sse_event *ev) {
  if (!ev) {
    return C_REST_ERROR_GENERIC;
  }
  ev->id = NULL;
  ev->event = NULL;
  ev->data = NULL;
  ev->retry = -1;
  return C_REST_OK;
}

c_rest_error_t c_rest_sse_event_destroy(struct c_rest_sse_event *ev) {
  if (!ev) {
    return C_REST_ERROR_GENERIC;
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
  return C_REST_OK;
}

static c_rest_error_t c_rest_sse_strdup(const char *s, char **out_str) {
  size_t len;
  char *copy;
  void *tmp;
  len = strlen(s);
  if (C_REST_MALLOC(len + 1, &tmp) != 0) {
    return C_REST_ERROR_GENERIC;
  }
  copy = (char *)tmp;
  #if defined(_MSC_VER)
  /* CDD_SAFE_CRT */ memcpy_s(copy, len + 1, s, len + 1);
  #else
  memcpy(copy, s, len + 1);
  #endif
  *out_str = copy;
  return C_REST_OK;
}
c_rest_error_t c_rest_sse_event_clone(const struct c_rest_sse_event *src,
                           struct c_rest_sse_event *dest) {
  c_rest_error_t rc;
  if (!src || !dest) {
    return C_REST_ERROR_GENERIC;
  }
  if (src->id) {
    rc = c_rest_sse_strdup(src->id, &dest->id);
    if (rc != C_REST_OK) goto err;
  } else {
    dest->id = NULL;
  }

  if (src->event) {
    rc = c_rest_sse_strdup(src->event, &dest->event);
    if (rc != C_REST_OK) goto err;
  } else {
    dest->event = NULL;
  }

  if (src->data) {
    rc = c_rest_sse_strdup(src->data, &dest->data);
    if (rc != C_REST_OK) goto err;
  } else {
    dest->data = NULL;
  }

  dest->retry = src->retry;
  return C_REST_OK;

err:
  (void)!c_rest_sse_event_destroy(dest); /* Best effort destroy on error path */
  return rc;
}

c_rest_error_t c_rest_sse_serialize(const struct c_rest_sse_event *ev, char **out_buf,
                         size_t *out_len) {
  c_rest_string s;
  char retry_buf[32];
  const char *data_ptr;
  const char *nl;
  size_t data_len;
  void *tmp_out_buf;
  c_rest_error_t rc;

  if (!ev || !out_buf || !out_len) {
    return C_REST_ERROR_GENERIC;
  }

#ifdef C_REST_TESTING_MALLOC_HOOK
  rc = c_rest_string_init(&s, 1);
#else
  rc = c_rest_string_init(&s, 128);
#endif
  if (rc != C_REST_OK) {
    return rc;
  }

  if (ev->id) {
    rc = c_rest_string_append_cstr(&s, "id: ");
    if (rc != C_REST_OK) goto err;
    rc = c_rest_string_append_cstr(&s, ev->id);
    if (rc != C_REST_OK) goto err;
    rc = c_rest_string_append_cstr(&s, "\n");
    if (rc != C_REST_OK) goto err;
  }

  if (ev->event) {
    rc = c_rest_string_append_cstr(&s, "event: ");
    if (rc != C_REST_OK) goto err;
    rc = c_rest_string_append_cstr(&s, ev->event);
    if (rc != C_REST_OK) goto err;
    rc = c_rest_string_append_cstr(&s, "\n");
    if (rc != C_REST_OK) goto err;
  }

  if (ev->retry >= 0) {
#if defined(_MSC_VER)
    sprintf_s(retry_buf, sizeof(retry_buf), "retry: %d\n", ev->retry);
#else
    sprintf(retry_buf, "retry: %d\n", ev->retry);
#endif
    rc = c_rest_string_append_cstr(&s, retry_buf);
    if (rc != C_REST_OK) goto err;
  }

  if (ev->data) {
    data_ptr = ev->data;
    while ((nl = strchr(data_ptr, '\n')) != NULL) {
      rc = c_rest_string_append_cstr(&s, "data: ");
      if (rc != C_REST_OK) goto err;
      rc = c_rest_string_append(&s, data_ptr, (size_t)(nl - data_ptr));
      if (rc != C_REST_OK) goto err;
      rc = c_rest_string_append_cstr(&s, "\n");
      if (rc != C_REST_OK) goto err;
      data_ptr = nl + 1;
    }
    rc = c_rest_string_append_cstr(&s, "data: ");
    if (rc != C_REST_OK) goto err;
    rc = c_rest_string_append_cstr(&s, data_ptr);
    if (rc != C_REST_OK) goto err;
    rc = c_rest_string_append_cstr(&s, "\n");
    if (rc != C_REST_OK) goto err;
  }

  rc = c_rest_string_append_cstr(&s, "\n");
  if (rc != C_REST_OK) goto err;

  data_len = s.length;
  if (C_REST_MALLOC(data_len + 1, &tmp_out_buf) != 0) {
    rc = C_REST_ERROR_GENERIC;
    goto err;
  }
  *out_buf = (char *)tmp_out_buf;
  #if defined(_MSC_VER)
  /* CDD_SAFE_CRT */ memcpy_s(*out_buf, data_len, s.data, data_len);
  #else
  memcpy(*out_buf, s.data, data_len);
  #endif
  (*out_buf)[data_len] = '\0';
  *out_len = data_len;

  rc = c_rest_string_destroy(&s);
  if (rc != C_REST_OK) return rc;

  return C_REST_OK;

err:
(void)!c_rest_string_destroy(&s);
  return rc;
}

c_rest_error_t c_rest_sse_context_init(struct c_rest_sse_context **out_ctx) {
  struct c_rest_sse_context *ctx;
  void *tmp_ctx;
  c_rest_error_t rc;
  if (!out_ctx) {
    return C_REST_ERROR_GENERIC;
  }

  if (C_REST_MALLOC(sizeof(struct c_rest_sse_context), &tmp_ctx) != 0) {
    return C_REST_ERROR_GENERIC;
  }
  ctx = (struct c_rest_sse_context *)tmp_ctx;

  ctx->buffer = NULL;
  ctx->buffer_len = 0;
  ctx->buffer_cap = 0;
  rc = c_rest_sse_event_init(&ctx->current_event);
  if (rc != C_REST_OK) {
    C_REST_FREE(ctx);
    return rc;
  }

  *out_ctx = ctx;
  return C_REST_OK;
}

c_rest_error_t c_rest_sse_context_destroy(struct c_rest_sse_context *ctx) {
  c_rest_error_t rc;
  if (!ctx) {
    return C_REST_ERROR_GENERIC;
  }

  if (ctx->buffer) {
    C_REST_FREE(ctx->buffer);
    ctx->buffer = NULL;
  }
  rc = c_rest_sse_event_destroy(&ctx->current_event);
  C_REST_FREE(ctx);
  return rc;
}

static c_rest_error_t append_to_string(char **dest, const char *src,
                                       size_t len) {
  size_t old_len;
  char *new_str;
  void *tmp_new_str;

  if (len == 0) {
    return C_REST_OK;
  }

  old_len = *dest ? strlen(*dest) : 0;
  if (C_REST_MALLOC(old_len + len + 1, &tmp_new_str) != 0) {
    return C_REST_ERROR_GENERIC;
  }
  new_str = (char *)tmp_new_str;


    if (*dest) {
      #if defined(_MSC_VER)
      /* CDD_SAFE_CRT */ memcpy_s(new_str, old_len, *dest, old_len);
      #else
      memcpy(new_str, *dest, old_len);
      #endif
      C_REST_FREE(*dest);
    }

  #if defined(_MSC_VER)
  /* CDD_SAFE_CRT */ memcpy_s(new_str + old_len, len, src, len);
  #else
  memcpy(new_str + old_len, src, len);
  #endif
  new_str[old_len + len] = '\0';

  *dest = new_str;
  return C_REST_OK;
}

c_rest_error_t c_rest_sse_parse(struct c_rest_sse_context *ctx, const char *data,
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

  if (!ctx || !out_event) {
    return C_REST_ERROR_GENERIC;
  }

  if (data && len > 0) {
    if (ctx->buffer_len + len > ctx->buffer_cap) {
      void *tmp_new_buf;
      ctx->buffer_cap = (ctx->buffer_len + len) * 2;
      if (ctx->buffer_cap < 256) {
        ctx->buffer_cap = 256;
      }
      if (C_REST_MALLOC(ctx->buffer_cap, &tmp_new_buf) != 0) {
        return C_REST_ERROR_GENERIC;
      }
      new_buf = (char *)tmp_new_buf;

      if (ctx->buffer) {
        #if defined(_MSC_VER)
        /* CDD_SAFE_CRT */ memcpy_s(new_buf, ctx->buffer_len, ctx->buffer, ctx->buffer_len);
        #else
        memcpy(new_buf, ctx->buffer, ctx->buffer_len);
        #endif
        C_REST_FREE(ctx->buffer);
      }

      ctx->buffer = new_buf;
    }
    #if defined(_MSC_VER)
    /* CDD_SAFE_CRT */ memcpy_s(ctx->buffer + ctx->buffer_len, len, data, len);
    #else
    memcpy(ctx->buffer + ctx->buffer_len, data, len);
    #endif
    ctx->buffer_len += len;
  }

  line_start = ctx->buffer;

  while (line_start < ctx->buffer + ctx->buffer_len) {
    line_end = (const char *)memchr(line_start, '\n',
                                    (size_t)(ctx->buffer + ctx->buffer_len - line_start));
    if (!line_end) {
      break;
    }

    line_len = (size_t)(line_end - line_start);

    if (line_len > 0 && line_start[line_len - 1] == '\r') {
      line_len--;
    }


    if (line_len == 0) {
      if (ctx->current_event.data || ctx->current_event.event ||
          ctx->current_event.id || ctx->current_event.retry >= 0) {
        c_rest_error_t rc;
        rc = c_rest_sse_event_clone(&ctx->current_event, out_event);
        if (rc != C_REST_OK) return rc;

        rc = INTERNAL_EVENT_DESTROY(&ctx->current_event);
        if (rc != C_REST_OK) return rc;

        rc = INTERNAL_EVENT_INIT(&ctx->current_event);
        if (rc != C_REST_OK) return rc;
        event_completed = 1;
      }
      processed = (size_t)(line_end - ctx->buffer + 1);
      line_start = line_end + 1;

      if (event_completed) {
        break;
      }
      continue;
    }

    if (line_start[0] == ':') {
      line_start = line_end + 1;
      continue;
    }

    colon = (const char *)memchr(line_start, ':', line_len);
    if (colon) {
      value = colon + 1;
      if (value < line_start + line_len && *value == ' ') {
        value++;
      }
      value_len = (size_t)(line_start + line_len - value);

      if ((size_t)(colon - line_start) == 5 &&
          memcmp(line_start, "event", 5) == 0) {
        c_rest_error_t rc;

        if (ctx->current_event.event) {
          C_REST_FREE(ctx->current_event.event);
          ctx->current_event.event = NULL;
        }

        rc = append_to_string(&ctx->current_event.event, value, value_len);

        if (rc != C_REST_OK) {
          return rc;
        }

      } else if ((size_t)(colon - line_start) == 4 &&
                 memcmp(line_start, "data", 4) == 0) {
        c_rest_error_t rc;

        if (ctx->current_event.data) {
          rc = append_to_string(&ctx->current_event.data, "\n", 1);

        if (rc != C_REST_OK) {
          return rc;
        }

        }

        rc = append_to_string(&ctx->current_event.data, value, value_len);

        if (rc != C_REST_OK) {
          return rc;
        }

      } else if ((size_t)(colon - line_start) == 2 &&
                 memcmp(line_start, "id", 2) == 0) {
        c_rest_error_t rc;

        if (ctx->current_event.id) {
          C_REST_FREE(ctx->current_event.id);
          ctx->current_event.id = NULL;
        }

        rc = append_to_string(&ctx->current_event.id, value, value_len);

        if (rc != C_REST_OK) {
          return rc;
        }

      } else if ((size_t)(colon - line_start) == 5 &&
                 memcmp(line_start, "retry", 5) == 0) {
        char retry_str[32];
        size_t copy_len;

        if (value_len < 31) {
          copy_len = value_len;
        } else {
          copy_len = 31;
        }

        #if defined(_MSC_VER)
        /* CDD_SAFE_CRT */ memcpy_s(retry_str, copy_len, value, copy_len);
        #else
        memcpy(retry_str, value, copy_len);
        #endif
        retry_str[copy_len] = '\0';
        ctx->current_event.retry = atoi(retry_str);
      }
    } else {

      if (line_len == 5 && memcmp(line_start, "event", 5) == 0) {
        c_rest_error_t rc;
        if (ctx->current_event.event) {
          C_REST_FREE(ctx->current_event.event);
          ctx->current_event.event = NULL;
        }
        rc = c_rest_sse_strdup("", &ctx->current_event.event);
        if (rc != C_REST_OK) return rc;
      } else if (line_len == 4 && memcmp(line_start, "data", 4) == 0) {
        c_rest_error_t rc;
        if (ctx->current_event.data) {
          rc = append_to_string(&ctx->current_event.data, "\n", 1);

        if (rc != C_REST_OK) {
          return rc;
        }

        }
        (void)append_to_string(&ctx->current_event.data, "", 0);

      }

    }

    line_start = line_end + 1;
  }

  if (event_completed) {

    if (processed < ctx->buffer_len) {
      memmove(ctx->buffer, ctx->buffer + processed,
              ctx->buffer_len - processed);
      ctx->buffer_len -= processed;
    } else {
      ctx->buffer_len = 0;
    }

    return C_REST_OK;
  }

  if (line_start > ctx->buffer) {
    size_t consumed = (size_t)(line_start - ctx->buffer);
    if (consumed < ctx->buffer_len) {
      memmove(ctx->buffer, ctx->buffer + consumed, ctx->buffer_len - consumed);
      ctx->buffer_len -= consumed;
    } else {
      ctx->buffer_len = 0;
    }
  }

  return C_REST_ERROR_GENERIC;
}

#include "c_rest_response.h"
/* clang-format on */

c_rest_error_t c_rest_sse_init_response(struct c_rest_response *res) {
  c_rest_error_t rc;
  if (!res) {
    return C_REST_ERROR_GENERIC;
  }

  (void)c_rest_response_set_status(res, 200);
  rc = c_rest_response_set_header(res, "Content-Type", "text/event-stream");
  if (rc != C_REST_OK)
    return rc;
  rc = c_rest_response_set_header(res, "Cache-Control", "no-cache");
  if (rc != C_REST_OK)
    return rc;
  rc = c_rest_response_set_header(res, "Connection", "keep-alive");
  if (rc != C_REST_OK)
    return rc;

  return c_rest_response_send(res);
}

c_rest_error_t c_rest_sse_send_event(struct c_rest_response *res,
                                     const struct c_rest_sse_event *ev) {
  char *buf;
  size_t len;
  c_rest_error_t ret;

  if (!res || !ev) {
    return C_REST_ERROR_GENERIC;
  }

  ret = c_rest_sse_serialize(ev, &buf, &len);
  if (ret != C_REST_OK) {
    return ret;
  }

  ret = c_rest_response_write_chunk(res, buf, len);
  C_REST_FREE(buf);

  return ret;
}

c_rest_error_t c_rest_sse_send_keepalive(struct c_rest_response *res) {
  const char *keepalive = ": \n\n";
  if (!res) {
    return C_REST_ERROR_GENERIC;
  }
  return c_rest_response_write_chunk(res, keepalive, strlen(keepalive));
}
