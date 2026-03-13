/* clang-format off */
#include "c_rest_parser.h"

#include <stdlib.h>
/* clang-format on */

int c_rest_parser_init(c_rest_parser_context *ctx,
                       const struct c_rest_parser_vtable *vtable,
                       const struct c_rest_parser_callbacks *callbacks,
                       void *user_data) {
  if (!ctx || !vtable || !vtable->init) {
    return 1;
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

size_t c_rest_parser_execute(c_rest_parser_context *ctx, const char *data,
                             size_t len) {
  if (!ctx || !ctx->vtable || !ctx->vtable->execute) {
    return 0;
  }
  return ctx->vtable->execute(ctx, data, len);
}

int c_rest_parser_should_keep_alive(c_rest_parser_context *ctx) {
  if (!ctx || !ctx->vtable || !ctx->vtable->should_keep_alive) {
    return 0;
  }
  return ctx->vtable->should_keep_alive(ctx);
}

void c_rest_parser_destroy(c_rest_parser_context *ctx) {
  if (!ctx || !ctx->vtable || !ctx->vtable->destroy) {
    return;
  }
  ctx->vtable->destroy(ctx);
}
