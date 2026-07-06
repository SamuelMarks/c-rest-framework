/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_request.h"
#include "c_rest_response.h"
#include "c_rest_router.h"
#include "c_rest_openapi.h"
#ifdef C_REST_ENABLE_SERVER_SENT_EVENTS_SSE
#include "c_rest_sse.h"
#endif

#include <stdlib.h>
#include "c_rest_log.h"
#include "c_rest_mem.h"
#include <string.h>

#ifdef C_REST_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING
static c_rest_error_t c_rest_template_handler(struct c_rest_request *req,
                                   struct c_rest_response *res,
                                   void *user_data);
#endif

static c_rest_error_t c_rest_ws_upgrade_handler(struct c_rest_request *req,
                                     struct c_rest_response *res,
                                     void *user_data);

#ifdef C_REST_ENABLE_SERVER_SENT_EVENTS_SSE
static c_rest_error_t c_rest_sse_handler_wrapper(struct c_rest_request *req,
                                      struct c_rest_response *res,
                                      void *user_data);
#endif

struct c_rest_route_handler {
  char *method;
  c_rest_handler_fn handler;
  void *user_data;
  struct c_rest_route_handler *next;
};

struct c_rest_route_node {
  char *segment;
  int is_var;
  int is_wildcard;
  char *var_name;

  struct c_rest_route_node *parent;
  struct c_rest_route_node *children;
  struct c_rest_route_node *next;

  struct c_rest_route_handler *handlers;
};

struct c_rest_middleware_chain {
  char *path_prefix;
  c_rest_middleware_fn middleware;
  void *user_data;
  struct c_rest_middleware_chain *next;
};

struct c_rest_router {
  struct c_rest_route_node *root;
  struct c_rest_middleware_chain *middlewares;
  struct c_rest_middleware_chain *post_middlewares;
  struct c_rest_openapi_spec *openapi_spec;
};

static c_rest_error_t create_node(const char *segment, size_t len,
                       struct c_rest_route_node **out_node) {
  struct c_rest_route_node *node;

  if (!out_node) /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  node = (struct c_rest_route_node *)malloc(sizeof(struct c_rest_route_node));
  if (!node) /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  node->segment = (char *)malloc(len + 1);
  if (!node->segment) { /* GCOVR_EXCL_LINE */
    C_REST_FREE((void *)(node)); /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }
  #if defined(_MSC_VER)
  /* CDD_SAFE_CRT */ memcpy_s(node->segment, len, segment, len);
  #else
  memcpy(node->segment, segment, len);
  #endif
  node->segment[len] = '\0';

  node->is_var = 0;
  node->is_wildcard = 0;
  node->var_name = NULL;

  if (len > 0 && node->segment[0] == ':') {
    node->is_var = 1;
    node->var_name = (char *)malloc(len);
    if (node->var_name) { /* GCOVR_EXCL_LINE */
#if defined(_MSC_VER)
      strcpy_s(node->var_name, len, node->segment + 1);
#else
      strcpy(node->var_name, node->segment + 1);
#endif
    }
  } else if (len > 0 && node->segment[0] == '*') { /* GCOVR_EXCL_LINE */
    node->is_wildcard = 1; /* GCOVR_EXCL_LINE */
  }

  node->children = NULL;
  node->next = NULL;
  node->handlers = NULL;

  *out_node = node;
  return C_REST_OK;
}

static c_rest_error_t free_node(struct c_rest_route_node *node) {
  struct c_rest_route_handler *h;
  if (!node)
    return C_REST_ERROR_GENERIC;

  if (node->segment) /* GCOVR_EXCL_LINE */
    C_REST_FREE((void *)(node->segment));
  if (node->var_name)
    C_REST_FREE((void *)(node->var_name));

  h = node->handlers;
  while (h) {
    struct c_rest_route_handler *next_h = h->next;
    if (h->method) /* GCOVR_EXCL_LINE */
      C_REST_FREE((void *)(h->method));
#ifdef C_REST_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING
    if (h->handler == c_rest_template_handler && h->user_data) {
      C_REST_FREE(h->user_data);
    }
#endif
    if (h->handler == c_rest_ws_upgrade_handler && h->user_data) {
      C_REST_FREE(h->user_data);
    }
#ifdef C_REST_ENABLE_SERVER_SENT_EVENTS_SSE
    if (h->handler == c_rest_sse_handler_wrapper && h->user_data) {
      C_REST_FREE(h->user_data);
    }
#endif
    C_REST_FREE((void *)(h));
    h = next_h;
  }

  free_node(node->children);
  free_node(node->next);
  C_REST_FREE((void *)(node));
  return C_REST_OK;
}
c_rest_error_t c_rest_router_init(c_rest_router **out_router) {
  struct c_rest_router *router;

  if (!out_router) /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  router = (struct c_rest_router *)malloc(sizeof(struct c_rest_router));
  if (!router) /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  if (create_node("", 0, &router->root) != 0) { /* GCOVR_EXCL_LINE */
    C_REST_FREE((void *)(router)); /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }
  router->middlewares = NULL;
  router->post_middlewares = NULL;
  router->openapi_spec = NULL;
  c_rest_openapi_spec_init(&router->openapi_spec);

  *out_router = router;
  return C_REST_OK;
}

c_rest_error_t c_rest_router_destroy(c_rest_router *router) {
  struct c_rest_middleware_chain *m;

  if (!router) /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  free_node(router->root);

  m = router->middlewares;
  while (m) {
    struct c_rest_middleware_chain *next_m = m->next;
    if (m->path_prefix) /* GCOVR_EXCL_LINE */
      C_REST_FREE((void *)(m->path_prefix));
    C_REST_FREE((void *)(m));
    m = next_m;
  }

  m = router->post_middlewares;
  while (m) {
    struct c_rest_middleware_chain *next_m = m->next;
    if (m->path_prefix) /* GCOVR_EXCL_LINE */
      C_REST_FREE((void *)(m->path_prefix));
    C_REST_FREE((void *)(m));
    m = next_m;
  }

  if (router->openapi_spec) { /* GCOVR_EXCL_LINE */
    c_rest_openapi_spec_destroy(router->openapi_spec);
  }

  C_REST_FREE((void *)(router));
  return C_REST_OK;
}

static c_rest_error_t find_or_add_child(struct c_rest_route_node *parent,
                             const char *segment, size_t len,
                             struct c_rest_route_node **out_child) {
  struct c_rest_route_node *child;
  struct c_rest_route_node *new_node;

  if (!parent || !out_child) /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  child = parent->children;

  while (child) {
    if (strlen(child->segment) == len && /* GCOVR_EXCL_LINE */
        strncmp(child->segment, segment, len) == 0) { /* GCOVR_EXCL_LINE */
      *out_child = child; /* GCOVR_EXCL_LINE */
      return C_REST_OK; /* GCOVR_EXCL_LINE */
    }
    child = child->next;
  }

  if (create_node(segment, len, &new_node) != C_REST_OK) /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  new_node->next = parent->children;
  parent->children = new_node;
  new_node->parent = parent;

  *out_child = new_node;
  return C_REST_OK;
}
c_rest_error_t c_rest_router_add(c_rest_router *router, const char *method,
                      const char *path, c_rest_handler_fn handler,
                      void *user_data) {
  const char *p;
  struct c_rest_route_node *curr;
  struct c_rest_route_handler *h;

  if (!router || !router->root || !method || !path || !handler) /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  curr = router->root;
  p = path;

  if (*p == '/') /* GCOVR_EXCL_LINE */
    p++;

  while (*p) { /* GCOVR_EXCL_LINE */
    const char *next_slash = strchr(p, '/');
    size_t len;
    struct c_rest_route_node *child;

    if (next_slash) {
      len = next_slash - p;
    } else {
      len = strlen(p);
    }

    if (len > 0) { /* GCOVR_EXCL_LINE */
      if (find_or_add_child(curr, p, len, &child) != C_REST_OK) /* GCOVR_EXCL_LINE */
        return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
      curr = child;
    }

    if (next_slash) {
      p = next_slash + 1;
    } else {
      break;
    }
  }

  h = (struct c_rest_route_handler *)malloc(
      sizeof(struct c_rest_route_handler));
  if (!h) /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  h->method = (char *)malloc(strlen(method) + 1);
  if (!h->method) { /* GCOVR_EXCL_LINE */
    C_REST_FREE((void *)(h)); /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }
#if defined(_MSC_VER)
  strcpy_s(h->method, strlen(method) + 1, method);
#else
  strcpy(h->method, method);
#endif
  h->handler = handler;
  h->user_data = user_data;

  h->next = curr->handlers;
  curr->handlers = h;

  return C_REST_OK;
}

c_rest_error_t c_rest_router_add_openapi(c_rest_router *router, const char *method,
                              const char *path, c_rest_handler_fn handler,
                              void *user_data,
                              const struct c_rest_openapi_operation *op_meta) {
  int res = c_rest_router_add(router, method, path, handler, user_data);
  if (res == 0 && op_meta && router->openapi_spec) { /* GCOVR_EXCL_LINE */
    c_rest_openapi_spec_add_path(router->openapi_spec, path, method, op_meta);
  }
  return res;
}

#include "c_rest_websocket.h"

struct c_rest_ws_route_data {
  c_rest_websocket_on_message_fn on_message;
  c_rest_websocket_on_close_fn on_close;
  void *user_data;
};

static c_rest_error_t c_rest_ws_upgrade_handler(struct c_rest_request *req, /* GCOVR_EXCL_LINE */
                                     struct c_rest_response *res,
                                     void *user_data) {
  int ret;
  (void)user_data; /* Used later by connection logic to call on_message */
  ret = c_rest_websocket_upgrade(req, res); /* GCOVR_EXCL_LINE */
  return ret; /* GCOVR_EXCL_LINE */
}

c_rest_error_t c_rest_router_add_websocket(c_rest_router *router, const char *path, /* GCOVR_EXCL_LINE */
                                c_rest_websocket_on_message_fn on_message,
                                c_rest_websocket_on_close_fn on_close,
                                void *user_data) {
  struct c_rest_ws_route_data *ws_data;

  if (!router || !path) /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  ws_data = (struct c_rest_ws_route_data *)malloc( /* GCOVR_EXCL_LINE */
      sizeof(struct c_rest_ws_route_data));
  if (!ws_data) /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  ws_data->on_message = on_message; /* GCOVR_EXCL_LINE */
  ws_data->on_close = on_close; /* GCOVR_EXCL_LINE */
  ws_data->user_data = user_data; /* GCOVR_EXCL_LINE */

  return c_rest_router_add(router, "GET", path, c_rest_ws_upgrade_handler, /* GCOVR_EXCL_LINE */
                           ws_data);
}

c_rest_error_t c_rest_router_add_websocket_openapi( /* GCOVR_EXCL_LINE */
    c_rest_router *router, const char *path,
    c_rest_websocket_on_message_fn on_message,
    c_rest_websocket_on_close_fn on_close, void *user_data,
    const struct c_rest_openapi_operation *op_meta) {
  int res = c_rest_router_add_websocket(router, path, on_message, on_close, /* GCOVR_EXCL_LINE */
                                        user_data);
  if (res == 0 && op_meta && router->openapi_spec) { /* GCOVR_EXCL_LINE */
    c_rest_openapi_spec_add_path(router->openapi_spec, path, "GET", op_meta); /* GCOVR_EXCL_LINE */
  }
  return res; /* GCOVR_EXCL_LINE */
}

#ifdef C_REST_ENABLE_SERVER_SENT_EVENTS_SSE
struct c_rest_sse_route_data {
  c_rest_handler_fn handler;
  void *user_data;
};

static c_rest_error_t c_rest_sse_handler_wrapper(struct c_rest_request *req, /* GCOVR_EXCL_LINE */
                                      struct c_rest_response *res,
                                      void *user_data) {
  struct c_rest_sse_route_data *sse_data = /* GCOVR_EXCL_LINE */
      (struct c_rest_sse_route_data *)user_data;
  int ret;

  ret = c_rest_sse_init_response(res); /* GCOVR_EXCL_LINE */
  if (ret != 0) { /* GCOVR_EXCL_LINE */
    return ret; /* GCOVR_EXCL_LINE */
  }

  if (sse_data && sse_data->handler) { /* GCOVR_EXCL_LINE */
    return sse_data->handler(req, res, sse_data->user_data); /* GCOVR_EXCL_LINE */
  }
  return C_REST_OK; /* GCOVR_EXCL_LINE */
}

c_rest_error_t c_rest_router_add_sse(c_rest_router *router, const char *path, /* GCOVR_EXCL_LINE */
                          c_rest_handler_fn handler, void *user_data) {
  struct c_rest_sse_route_data *sse_data;

  if (!router || !path) /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  sse_data = (struct c_rest_sse_route_data *)malloc( /* GCOVR_EXCL_LINE */
      sizeof(struct c_rest_sse_route_data));
  if (!sse_data) /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  sse_data->handler = handler; /* GCOVR_EXCL_LINE */
  sse_data->user_data = user_data; /* GCOVR_EXCL_LINE */

  return c_rest_router_add(router, "GET", path, c_rest_sse_handler_wrapper, /* GCOVR_EXCL_LINE */
                           sse_data);
}

c_rest_error_t c_rest_router_add_sse_openapi( /* GCOVR_EXCL_LINE */
    c_rest_router *router, const char *path, c_rest_handler_fn handler,
    void *user_data, const struct c_rest_openapi_operation *op_meta) {
  int res = c_rest_router_add_sse(router, path, handler, user_data); /* GCOVR_EXCL_LINE */
  if (res == 0 && op_meta && router->openapi_spec) { /* GCOVR_EXCL_LINE */
    c_rest_openapi_spec_add_path(router->openapi_spec, path, "GET", op_meta); /* GCOVR_EXCL_LINE */
  }
  return res; /* GCOVR_EXCL_LINE */
}
#endif

#ifdef C_REST_FRAMEWORK_ENABLE_GRAPHQL
#include "c_rest_graphql.h"

static c_rest_error_t c_rest_graphql_handler(struct c_rest_request *req, /* GCOVR_EXCL_LINE */
                                  struct c_rest_response *res,
                                  void *user_data) {
  struct c_rest_graphql_schema *schema = /* GCOVR_EXCL_LINE */
      (struct c_rest_graphql_schema *)user_data;
  struct c_rest_graphql_node *doc = NULL; /* GCOVR_EXCL_LINE */
  char *json = NULL; /* GCOVR_EXCL_LINE */
  size_t len = 0; /* GCOVR_EXCL_LINE */
  int ret;

  if (!req->body) { /* GCOVR_EXCL_LINE */
    c_rest_response_set_status(res, 400); /* GCOVR_EXCL_LINE */
    return C_REST_OK; /* GCOVR_EXCL_LINE */
  }

  ret = c_rest_graphql_parse((const char *)req->body, req->body_len, &doc); /* GCOVR_EXCL_LINE */
  if (ret != 0) { /* GCOVR_EXCL_LINE */
    c_rest_response_set_status(res, 400); /* GCOVR_EXCL_LINE */
    return C_REST_OK; /* GCOVR_EXCL_LINE */
  }

  ret = c_rest_graphql_resolve(doc, schema, &json, &len); /* GCOVR_EXCL_LINE */
  c_rest_graphql_node_free(doc); /* GCOVR_EXCL_LINE */

  if (ret != 0) { /* GCOVR_EXCL_LINE */
    c_rest_response_set_status(res, 500); /* GCOVR_EXCL_LINE */
    return C_REST_OK; /* GCOVR_EXCL_LINE */
  }

  c_rest_response_set_status(res, 200); /* GCOVR_EXCL_LINE */
  c_rest_response_json(res, json); /* GCOVR_EXCL_LINE */
  C_REST_FREE((void *)(json)); /* GCOVR_EXCL_LINE */
  return C_REST_OK; /* GCOVR_EXCL_LINE */
}

c_rest_error_t c_rest_router_add_graphql(c_rest_router *router, const char *path, /* GCOVR_EXCL_LINE */
                              struct c_rest_graphql_schema *schema) {
  if (!router || !path || !schema) /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  return c_rest_router_add(router, "POST", path, c_rest_graphql_handler, /* GCOVR_EXCL_LINE */
                           schema);
}

c_rest_error_t c_rest_router_add_graphql_openapi( /* GCOVR_EXCL_LINE */
    c_rest_router *router, const char *path,
    struct c_rest_graphql_schema *schema,
    const struct c_rest_openapi_operation *op_meta) {
  int res = c_rest_router_add_graphql(router, path, schema); /* GCOVR_EXCL_LINE */
  if (res == 0 && op_meta && router->openapi_spec) { /* GCOVR_EXCL_LINE */
    c_rest_openapi_spec_add_path(router->openapi_spec, path, "POST", op_meta); /* GCOVR_EXCL_LINE */
  }
  return res; /* GCOVR_EXCL_LINE */
}
#endif

#ifdef C_REST_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING
#include "c_rest_template.h"
/* clang-format on */

struct c_rest_template_route_data {
  const struct c_rest_template_context *ctx;
  c_rest_template_data_fn data_provider;
  void *user_data;
};

static c_rest_error_t c_rest_template_handler(struct c_rest_request *req,
                                              struct c_rest_response *res,
                                              void *user_data) {
  struct c_rest_template_route_data *route_data =
      (struct c_rest_template_route_data *)user_data;
  const char **keys = NULL;
  const char **values = NULL;
  size_t count = 0;
  int ret = 0;

  if (!route_data || !route_data->ctx ||
      !route_data->data_provider) {       /* GCOVR_EXCL_LINE */
    c_rest_response_set_status(res, 500); /* GCOVR_EXCL_LINE */
    return C_REST_OK;                     /* GCOVR_EXCL_LINE */
  }

  ret = route_data->data_provider(req, &keys, &values, &count,
                                  route_data->user_data);
  if (ret != 0) {                         /* GCOVR_EXCL_LINE */
    c_rest_response_set_status(res, 500); /* GCOVR_EXCL_LINE */
    return C_REST_OK;                     /* GCOVR_EXCL_LINE */
  }

  ret = c_rest_response_template(res, route_data->ctx, keys, values, count);
  if (ret != 0) {                         /* GCOVR_EXCL_LINE */
    c_rest_response_set_status(res, 500); /* GCOVR_EXCL_LINE */
    return C_REST_OK;                     /* GCOVR_EXCL_LINE */
  }

  c_rest_response_set_status(res, 200);
  return C_REST_OK;
}

c_rest_error_t c_rest_router_add_template(
    c_rest_router *router, const char *method, const char *path,
    const struct c_rest_template_context *ctx,
    c_rest_template_data_fn data_provider, void *user_data) {
  struct c_rest_template_route_data *route_data;

  if (!router || !path || !ctx || !data_provider) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;                    /* GCOVR_EXCL_LINE */
  }

  route_data = (struct c_rest_template_route_data *)malloc(
      sizeof(struct c_rest_template_route_data));
  if (!route_data) {             /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }

  route_data->ctx = ctx;
  route_data->data_provider = data_provider;
  route_data->user_data = user_data;

  return c_rest_router_add(router, method, path, c_rest_template_handler,
                           route_data);
}

c_rest_error_t
c_rest_router_add_template_openapi(/* GCOVR_EXCL_LINE */
                                   c_rest_router *router, const char *method,
                                   const char *path,
                                   const struct c_rest_template_context *ctx,
                                   c_rest_template_data_fn data_provider,
                                   void *user_data,
                                   const struct c_rest_openapi_operation
                                       *op_meta) {
  int res = c_rest_router_add_template(router, method, path,
                                       ctx,           /* GCOVR_EXCL_LINE */
                                       data_provider, /* GCOVR_EXCL_LINE */
                                       user_data);
  if (res == 0 && op_meta && router->openapi_spec) { /* GCOVR_EXCL_LINE */
    c_rest_openapi_spec_add_path(router->openapi_spec, path,
                                 method,   /* GCOVR_EXCL_LINE */
                                 op_meta); /* GCOVR_EXCL_LINE */
  }
  return res; /* GCOVR_EXCL_LINE */
}
#endif

c_rest_error_t
c_rest_router_get_openapi_spec(c_rest_router *router,
                               struct c_rest_openapi_spec **out_spec) {
  if (!router || !out_spec)      /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  *out_spec = router->openapi_spec;
  return C_REST_OK;
}

c_rest_error_t c_rest_router_use(c_rest_router *router, const char *path_prefix,
                                 c_rest_middleware_fn middleware,
                                 void *user_data) {
  struct c_rest_middleware_chain *m;
  struct c_rest_middleware_chain *tail;

  if (!router || !middleware)    /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  m = (struct c_rest_middleware_chain *)malloc(
      sizeof(struct c_rest_middleware_chain));
  if (!m)                        /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  if (path_prefix) { /* GCOVR_EXCL_LINE */
    m->path_prefix = (char *)malloc(strlen(path_prefix) + 1);
    if (!m->path_prefix) {         /* GCOVR_EXCL_LINE */
      C_REST_FREE((void *)(m));    /* GCOVR_EXCL_LINE */
      return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
    }
#if defined(_MSC_VER)
    strcpy_s(m->path_prefix, strlen(path_prefix) + 1, path_prefix);
#else
    strcpy(m->path_prefix, path_prefix);
#endif
  } else {
    m->path_prefix = NULL; /* GCOVR_EXCL_LINE */
  }

  m->middleware = middleware;
  m->user_data = user_data;
  m->next = NULL;

  if (!router->middlewares) { /* GCOVR_EXCL_LINE */
    router->middlewares = m;
  } else {
    tail = router->middlewares; /* GCOVR_EXCL_LINE */
    while (tail->next) {        /* GCOVR_EXCL_LINE */
      tail = tail->next;        /* GCOVR_EXCL_LINE */
    }
    tail->next = m; /* GCOVR_EXCL_LINE */
  }

  return C_REST_OK;
}

c_rest_error_t c_rest_router_use_post(c_rest_router *router,
                                      const char *path_prefix,
                                      c_rest_middleware_fn middleware,
                                      void *user_data) {
  struct c_rest_middleware_chain *m;
  struct c_rest_middleware_chain *tail;

  if (!router || !middleware)    /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  m = (struct c_rest_middleware_chain *)malloc(
      sizeof(struct c_rest_middleware_chain));
  if (!m)                        /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  if (path_prefix) { /* GCOVR_EXCL_LINE */
    m->path_prefix = (char *)malloc(strlen(path_prefix) + 1);
    if (!m->path_prefix) {         /* GCOVR_EXCL_LINE */
      C_REST_FREE((void *)(m));    /* GCOVR_EXCL_LINE */
      return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
    }
#if defined(_MSC_VER)
    strcpy_s(m->path_prefix, strlen(path_prefix) + 1, path_prefix);
#else
    strcpy(m->path_prefix, path_prefix);
#endif
  } else {
    m->path_prefix = NULL; /* GCOVR_EXCL_LINE */
  }

  m->middleware = middleware;
  m->user_data = user_data;
  m->next = NULL;

  if (!router->post_middlewares) { /* GCOVR_EXCL_LINE */
    router->post_middlewares = m;
  } else {
    tail = router->post_middlewares; /* GCOVR_EXCL_LINE */
    while (tail->next) {             /* GCOVR_EXCL_LINE */
      tail = tail->next;             /* GCOVR_EXCL_LINE */
    }
    tail->next = m; /* GCOVR_EXCL_LINE */
  }

  return C_REST_OK;
}

static c_rest_error_t match_route(struct c_rest_route_node *node,
                                  const char *path, struct c_rest_request *req,
                                  struct c_rest_route_node **out_node) {
  const char *next_slash;
  size_t len;
  struct c_rest_route_node *child;
  struct c_rest_route_node *matched = NULL;
  c_rest_error_t res;

  if (!node || !out_node)        /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  while (*path == '/')
    path++;

  if (*path == '\0') {
    *out_node = node;
    return C_REST_OK;
  }

  next_slash = strchr(path, '/');
  if (next_slash) {
    len = next_slash - path;
  } else {
    len = strlen(path);
  }

  child = node->children;
  while (child) {
    if (child->is_wildcard) { /* GCOVR_EXCL_LINE */
      *out_node = child;      /* GCOVR_EXCL_LINE */
      return C_REST_OK;       /* GCOVR_EXCL_LINE */
    }

    if (child->is_var ||
        (strlen(child->segment) == len &&
         strncmp(child->segment, path, len) == 0)) { /* GCOVR_EXCL_LINE */
      res = match_route(child, next_slash ? next_slash + 1 : "", req, &matched);
      if (res == C_REST_OK && matched) { /* GCOVR_EXCL_LINE */
        if (child->is_var && req) {      /* GCOVR_EXCL_LINE */
          struct c_rest_path_var *var =
              (struct c_rest_path_var *)malloc(sizeof(struct c_rest_path_var));
          if (var) { /* GCOVR_EXCL_LINE */
            var->name = (char *)malloc(strlen(child->var_name) + 1);
            if (var->name) { /* GCOVR_EXCL_LINE */
#if defined(_MSC_VER)
              strcpy_s(var->name, strlen(child->var_name) + 1, child->var_name);
#else
              strcpy(var->name, child->var_name);
#endif
            }

            var->value = (char *)malloc(len + 1);
            if (var->value) { /* GCOVR_EXCL_LINE */
#if defined(_MSC_VER)
              /* CDD_SAFE_CRT */ memcpy_s(var->value, len, path, len);
#else
              memcpy(var->value, path, len);
#endif
              var->value[len] = '\0';
            }

            var->next = req->path_vars;
            req->path_vars = var;
          }
        }
        *out_node = matched;
        return C_REST_OK;
      }
    }
    child = child->next;
  }

  *out_node = NULL;
  return C_REST_ERROR_GENERIC;
}

c_rest_error_t c_rest_router_dispatch(c_rest_router *router,
                                      struct c_rest_request *req,
                                      struct c_rest_response *res) {
  struct c_rest_middleware_chain *m;
  struct c_rest_route_node *matched_node = NULL;
  struct c_rest_route_handler *h;
  int middleware_res;
  c_rest_error_t match_res;

  if (!router || !req || !res)   /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  req->path_vars = NULL;

  /* 1. Execute middleware chain */
  m = router->middlewares;
  while (m) {
    if (!m->path_prefix || /* GCOVR_EXCL_LINE */
        strncmp(req->path, m->path_prefix, strlen(m->path_prefix)) ==
            0) { /* GCOVR_EXCL_LINE */
      middleware_res = m->middleware(req, res, m->user_data);
      if (middleware_res != 0) { /* GCOVR_EXCL_LINE */
        /* Middleware short-circuited */
        return C_REST_OK; /* GCOVR_EXCL_LINE */
      }
    }
    m = m->next;
  }

  /* 2. Resolve Route */
  match_res = match_route(router->root, req->path, req, &matched_node);
  if (match_res != C_REST_OK || !matched_node) { /* GCOVR_EXCL_LINE */
    /* 404 Not Found */
    res->status_code = 404;
    return C_REST_OK;
  }

  /* 3. Check Method */
  h = matched_node->handlers;
  while (h) {
    if (strcmp(h->method, req->method) == 0) {
      h->handler(req, res, h->user_data);
      break;
    }
    h = h->next;
  }

  if (!h) {
    /* 405 Method Not Allowed */
    res->status_code = 405;
  }

  /* 4. Execute post-middleware chain */
  m = router->post_middlewares;
  while (m) {
    if (!m->path_prefix || /* GCOVR_EXCL_LINE */
        strncmp(req->path, m->path_prefix, strlen(m->path_prefix)) ==
            0) { /* GCOVR_EXCL_LINE */
      m->middleware(req, res, m->user_data);
    }
    m = m->next;
  }

  return C_REST_OK;
}
