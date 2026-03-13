#ifdef C_REST_FRAMEWORK_USE_REAL_CAH
/* clang-format off */
#include <c_abstract_http/c_abstract_http.h>
#else
#include "c_abstract_http.h"
#endif
#include "c_rest_parser.h"

#include <stdlib.h>
#include <string.h>
/* clang-format on */

struct cah_parser_state {
  cah_parser parser;
  struct cah_parser_settings settings;
  c_rest_parser_context *ctx;

  /* State to collect headers (simplified for this plan step) */
  char *current_header_field;
  size_t current_header_field_len;
};

static void on_message_begin(cah_parser *parser) { (void)parser; }

static void on_url(cah_parser *parser, const char *at, size_t length) {
  struct cah_parser_state *state = (struct cah_parser_state *)parser->data;
  if (state && state->ctx && state->ctx->callbacks.on_url) {
    state->ctx->callbacks.on_url(state->ctx, at, length);
  }
}

static void on_header_field(cah_parser *parser, const char *at, size_t length) {
  struct cah_parser_state *state = (struct cah_parser_state *)parser->data;
  if (state) {
    if (state->current_header_field) {
      free(state->current_header_field);
    }
    state->current_header_field = (char *)malloc(length + 1);
    if (state->current_header_field) {
      memcpy(state->current_header_field, at, length);
      state->current_header_field[length] = '\0';
      state->current_header_field_len = length;
    }
  }
}

static void on_header_value(cah_parser *parser, const char *at, size_t length) {
  struct cah_parser_state *state = (struct cah_parser_state *)parser->data;
  if (state && state->ctx && state->ctx->callbacks.on_header &&
      state->current_header_field) {
    state->ctx->callbacks.on_header(state->ctx, state->current_header_field,
                                    state->current_header_field_len, at,
                                    length);
    free(state->current_header_field);
    state->current_header_field = NULL;
    state->current_header_field_len = 0;
  }
}

static void on_headers_complete(cah_parser *parser) {
  struct cah_parser_state *state = (struct cah_parser_state *)parser->data;
  if (state && state->ctx && state->ctx->callbacks.on_method) {
    /* Map method enum to string - very simplified */
    const char *method_str = "UNKNOWN";
    if (parser->method == CAH_GET)
      method_str = "GET";
    else if (parser->method == CAH_POST)
      method_str = "POST";
    else if (parser->method == CAH_PUT)
      method_str = "PUT";
    else if (parser->method == CAH_DELETE)
      method_str = "DELETE";

    state->ctx->callbacks.on_method(state->ctx, method_str, strlen(method_str));
  }
}

static void on_body(cah_parser *parser, const char *at, size_t length) {
  struct cah_parser_state *state = (struct cah_parser_state *)parser->data;
  if (state && state->ctx && state->ctx->callbacks.on_body) {
    state->ctx->callbacks.on_body(state->ctx, at, length);
  }
}

static void on_message_complete(cah_parser *parser) {
  struct cah_parser_state *state = (struct cah_parser_state *)parser->data;
  if (state && state->ctx && state->ctx->callbacks.on_complete) {
    state->ctx->callbacks.on_complete(state->ctx);
  }
}

static int cah_vtable_init(c_rest_parser_context *ctx,
                           const struct c_rest_parser_callbacks *callbacks,
                           void *user_data) {
  struct cah_parser_state *state;
  (void)callbacks;
  (void)user_data;

  state = (struct cah_parser_state *)malloc(sizeof(struct cah_parser_state));
  if (!state) {
    return 1;
  }

  state->ctx = ctx;
  state->current_header_field = NULL;
  state->current_header_field_len = 0;

  cah_parser_init(&state->parser);
  state->parser.data = state;

  state->settings.on_message_begin = on_message_begin;
  state->settings.on_url = on_url;
  state->settings.on_header_field = on_header_field;
  state->settings.on_header_value = on_header_value;
  state->settings.on_headers_complete = on_headers_complete;
  state->settings.on_body = on_body;
  state->settings.on_message_complete = on_message_complete;

  ctx->internal_state = state;
  return 0;
}

static size_t cah_vtable_execute(c_rest_parser_context *ctx, const char *data,
                                 size_t len) {
  struct cah_parser_state *state;
  if (!ctx || !ctx->internal_state)
    return 0;

  state = (struct cah_parser_state *)ctx->internal_state;
  return cah_parser_execute(&state->parser, &state->settings, data, len);
}

static int cah_vtable_should_keep_alive(c_rest_parser_context *ctx) {
  struct cah_parser_state *state;
  if (!ctx || !ctx->internal_state)
    return 0;

  state = (struct cah_parser_state *)ctx->internal_state;
  return cah_should_keep_alive(&state->parser);
}

static void cah_vtable_destroy(c_rest_parser_context *ctx) {
  struct cah_parser_state *state;
  if (!ctx || !ctx->internal_state)
    return;

  state = (struct cah_parser_state *)ctx->internal_state;
  if (state->current_header_field) {
    free(state->current_header_field);
  }
  free(state);
  ctx->internal_state = NULL;
}

static const struct c_rest_parser_vtable cah_vtable = {
    cah_vtable_init, cah_vtable_execute, cah_vtable_should_keep_alive,
    cah_vtable_destroy};

const struct c_rest_parser_vtable *c_rest_parser_get_cah_vtable(void) {
  return &cah_vtable;
}
