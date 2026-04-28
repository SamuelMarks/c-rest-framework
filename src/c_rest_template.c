/* clang-format off */
#include "c_rest_template.h"

#ifdef C_REST_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING
#include "c_rest_mem.h"
#include <string.h>
/* clang-format on */

int c_rest_template_init(struct c_rest_template_context *ctx,
                         const char *template_str) {
  size_t len;
  char *copy;
  if (!ctx || !template_str) {
    return 1;
  }
  len = strlen(template_str);
  if (C_REST_MALLOC(len + 1, (void **)&copy) != 0) {
    return 1;
  }
#if defined(_MSC_VER)
  strcpy_s(copy, len + 1, template_str);
#else
  strcpy(copy, template_str);
#endif
  ctx->template_str = copy;
  ctx->template_len = len;
  return 0;
}

int c_rest_template_destroy(struct c_rest_template_context *ctx) {
  if (!ctx) {
    return 1;
  }
  if (ctx->template_str) {
    C_REST_FREE(ctx->template_str);
    ctx->template_str = NULL;
  }
  ctx->template_len = 0;
  return 0;
}

int c_rest_template_render(const struct c_rest_template_context *ctx,
                           const char **keys, const char **values, size_t count,
                           char **out_result) {
  size_t out_len = 0;
  size_t out_cap = 0;
  char *out_buf = NULL;
  const char *p;
  size_t i;

  if (!ctx || !ctx->template_str || !out_result) {
    return 1;
  }

  out_cap = ctx->template_len + 128;
  if (C_REST_MALLOC(out_cap, (void **)&out_buf) != 0) {
    return 1;
  }

  p = ctx->template_str;
  while (*p) {
    if (*p == '{' && *(p + 1) == '{') {
      size_t key_len = 0;
      const char *end = p + 2;
      while (*end && (*end != '}' || *(end + 1) != '}')) {
        end++;
      }
      if (*end == '}' && *(end + 1) == '}') {
        int found = 0;
        key_len = (size_t)(end - (p + 2));
        for (i = 0; i < count; i++) {
          if (keys[i] && strlen(keys[i]) == key_len &&
              strncmp(keys[i], p + 2, key_len) == 0) {
            size_t val_len = values[i] ? strlen(values[i]) : 0;
            if (out_len + val_len + 1 > out_cap) {
              size_t new_cap = out_cap * 2 + val_len;
              char *new_buf = NULL;
              if (C_REST_REALLOC(out_buf, new_cap, (void **)&new_buf) != 0) {
                C_REST_FREE(out_buf);
                return 1;
              }
              out_buf = new_buf;
              out_cap = new_cap;
            }
            if (val_len > 0) {
#if defined(_MSC_VER)
              strncpy_s(out_buf + out_len, out_cap - out_len, values[i],
                        val_len);
#else
              strncpy(out_buf + out_len, values[i], val_len);
#endif
              out_len += val_len;
            }
            found = 1;
            break;
          }
        }
        if (!found) {
          size_t copy_len = key_len + 4;
          if (out_len + copy_len + 1 > out_cap) {
            size_t new_cap = out_cap * 2 + copy_len;
            char *new_buf = NULL;
            if (C_REST_REALLOC(out_buf, new_cap, (void **)&new_buf) != 0) {
              C_REST_FREE(out_buf);
              return 1;
            }
            out_buf = new_buf;
            out_cap = new_cap;
          }
#if defined(_MSC_VER)
          strncpy_s(out_buf + out_len, out_cap - out_len, p, copy_len);
#else
          strncpy(out_buf + out_len, p, copy_len);
#endif
          out_len += copy_len;
        }
        p = end + 2;
        continue;
      }
    }

    if (out_len + 2 > out_cap) {
      size_t new_cap = out_cap * 2;
      char *new_buf = NULL;
      if (C_REST_REALLOC(out_buf, new_cap, (void **)&new_buf) != 0) {
        C_REST_FREE(out_buf);
        return 1;
      }
      out_buf = new_buf;
      out_cap = new_cap;
    }
    out_buf[out_len++] = *p++;
  }
  out_buf[out_len] = '\0';
  *out_result = out_buf;
  return 0;
}

#endif /* C_REST_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING */
