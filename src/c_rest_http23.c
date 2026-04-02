/* clang-format off */
#include "c_rest_http23.h"
#include "c_rest_mem.h"
#include "c_rest_request.h"
#include "c_rest_response.h"
#include <string.h>
#include <stdio.h>
#include <stdio.h>
/* clang-format on */

/**
 * @brief Internal state for the HTTP/2 & HTTP/3 context.
 */
struct c_rest_http23_ctx {
  c_rest_protocol_t protocol;
  int is_ready;
  struct c_rest_request *request;
};

int c_rest_http23_ctx_init(c_rest_protocol_t protocol,
                           c_rest_http23_ctx_t **out_ctx) {
  c_rest_http23_ctx_t *ctx;

  if (!out_ctx) {
    return C_REST_HTTP23_ERR_INVALID;
  }

  if (protocol != C_REST_PROTOCOL_HTTP2 && protocol != C_REST_PROTOCOL_HTTP3) {
    return C_REST_HTTP23_ERR_PROTOCOL;
  }

  if (C_REST_MALLOC(sizeof(c_rest_http23_ctx_t), (void **)&ctx) != 0) {
    return C_REST_HTTP23_ERR_MEMORY;
  }

  ctx->protocol = protocol;
  ctx->is_ready = 0;

  if (C_REST_MALLOC(sizeof(struct c_rest_request), (void **)&ctx->request) !=
      0) {
    C_REST_FREE(ctx);
    return C_REST_HTTP23_ERR_MEMORY;
  }
  memset(ctx->request, 0, sizeof(struct c_rest_request));

  *out_ctx = ctx;
  return C_REST_HTTP23_OK;
}

int c_rest_http23_ctx_destroy(c_rest_http23_ctx_t *ctx) {
  if (!ctx) {
    return C_REST_HTTP23_ERR_INVALID;
  }

  if (ctx->request) {
    c_rest_request_cleanup(ctx->request);
    C_REST_FREE(ctx->request);
    ctx->request = NULL;
  }

  C_REST_FREE(ctx);
  return C_REST_HTTP23_OK;
}

int c_rest_http23_process(c_rest_http23_ctx_t *ctx, const char *data,
                          size_t len, size_t *out_consumed) {
  if (!ctx || !data || !out_consumed) {
    return C_REST_HTTP23_ERR_INVALID;
  }

  /* Mock logic: just consume everything and mark as ready if > 0 */
  *out_consumed = len;

  if (len > 0) {
    /* In a real implementation, we would parse HTTP/2 frames here */
    ctx->request->method = "GET";
    ctx->request->path = "/http23_test";
    ctx->is_ready = 1;
  }

  return C_REST_HTTP23_OK;
}

int c_rest_http23_is_request_ready(c_rest_http23_ctx_t *ctx, int *out_ready) {
  if (!ctx || !out_ready) {
    return C_REST_HTTP23_ERR_INVALID;
  }

  *out_ready = ctx->is_ready;
  return C_REST_HTTP23_OK;
}

int c_rest_http23_get_request(c_rest_http23_ctx_t *ctx,
                              struct c_rest_request **out_request) {
  if (!ctx || !out_request) {
    return C_REST_HTTP23_ERR_INVALID;
  }

  if (!ctx->is_ready) {
    return C_REST_HTTP23_ERR_INVALID; /* Not ready */
  }

  *out_request = ctx->request;
  return C_REST_HTTP23_OK;
}

int c_rest_http23_format_response(c_rest_http23_ctx_t *ctx,
                                  struct c_rest_response *response,
                                  char **out_buffer, size_t *out_len) {
  const char *body;
  size_t body_len;
  char *buf;
  size_t buf_size;

  if (!ctx || !response || !out_buffer || !out_len) {
    return C_REST_HTTP23_ERR_INVALID;
  }

  body = response->body;
  body_len = response->body_len;

  buf_size = 100 + body_len; /* Dummy size for headers/frames + body */
  if (C_REST_MALLOC(buf_size, (void **)&buf) != 0) {
    return C_REST_HTTP23_ERR_MEMORY;
  }

  /* Fake frame formatting */
#if defined(_MSC_VER)
  sprintf_s(buf, buf_size, "HTTP/%d FRAME: body_len=%u\n", (int)ctx->protocol,
            (unsigned int)body_len);
#else
  sprintf(buf, "HTTP/%d FRAME: body_len=%u\n", (int)ctx->protocol,
          (unsigned int)body_len);
#endif

  if (body_len > 0) {
#if defined(_MSC_VER)
    strcat_s(buf, buf_size, body);
#else
    strncat(buf, body, buf_size - strlen(buf) - 1);
#endif
  }

  *out_buffer = buf;
  *out_len = strlen(buf);

  return C_REST_HTTP23_OK;
}
