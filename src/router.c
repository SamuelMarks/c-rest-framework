/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "c_rest_request.h"
#include "c_rest_response.h"
#include "c_rest_router.h"
#include "c_rest_openapi.h"
#ifdef C_REST_ENABLE_SERVER_SENT_EVENTS_SSE
#include "c_rest_sse.h"
#endif

#include <stdlib.h>
#include "c_rest_log.h"
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



  node = (struct c_rest_route_node *)CRF_MALLOC(sizeof(struct c_rest_route_node));
  if (!node) return C_REST_ERROR_GENERIC;

  node->segment = (char *)CRF_MALLOC(len + 1);
  if (!node->segment) {
    C_REST_FREE((void *)(node));
    return C_REST_ERROR_GENERIC;
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
    node->var_name = (char *)CRF_MALLOC(len);
    if (node->var_name) {
#if defined(_MSC_VER)
      strcpy_s(node->var_name, len, node->segment + 1);
#else
      strcpy(node->var_name, node->segment + 1);
#endif
    }
  } else if (len > 0 && node->segment[0] == '*') {
    node->is_wildcard = 1;
  }

  node->children = NULL;
  node->next = NULL;
  node->handlers = NULL;

  *out_node = node;
  return C_REST_OK;
}

static c_rest_error_t free_node(struct c_rest_route_node *node) {
  struct c_rest_route_handler *h;
  c_rest_error_t rc;


  if (node->segment)
    C_REST_FREE((void *)(node->segment));
  if (node->var_name)
    C_REST_FREE((void *)(node->var_name));

  h = node->handlers;
  while (h) {
    struct c_rest_route_handler *next_h = h->next;
    if (h->method)
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

  if (node->children) {
    rc = free_node(node->children);
    if (rc != C_REST_OK) return rc;
  }
  if (node->next) {
    rc = free_node(node->next);
    if (rc != C_REST_OK) return rc;
  }
  C_REST_FREE((void *)(node));
  return C_REST_OK;
}
c_rest_error_t c_rest_router_init(c_rest_router **out_router) {
  struct c_rest_router *router;
  c_rest_error_t rc;

  if (!out_router)
    return C_REST_ERROR_GENERIC;

  router = (struct c_rest_router *)CRF_MALLOC(sizeof(struct c_rest_router));
  if (!router)
    return C_REST_ERROR_GENERIC;

  if (create_node("", 0, &router->root) != 0) {
    C_REST_FREE((void *)(router));
    return C_REST_ERROR_GENERIC;
  }
  router->middlewares = NULL;
  router->post_middlewares = NULL;
  router->openapi_spec = NULL;
  rc = c_rest_openapi_spec_init(&router->openapi_spec);
  if (rc != C_REST_OK) return rc;

  *out_router = router;
  return C_REST_OK;
}

c_rest_error_t c_rest_router_destroy(c_rest_router *router) {
  struct c_rest_middleware_chain *m;
  c_rest_error_t rc;

  if (!router)
    return C_REST_ERROR_GENERIC;

  if (router->root) {
    rc = free_node(router->root);
    if (rc != C_REST_OK) return rc;
  }

  m = router->middlewares;
  while (m) {
    struct c_rest_middleware_chain *next_m = m->next;
    if (m->path_prefix)
      C_REST_FREE((void *)(m->path_prefix));
    C_REST_FREE((void *)(m));
    m = next_m;
  }

  m = router->post_middlewares;
  while (m) {
    struct c_rest_middleware_chain *next_m = m->next;
    if (m->path_prefix)
      C_REST_FREE((void *)(m->path_prefix));
    C_REST_FREE((void *)(m));
    m = next_m;
  }

  if (router->openapi_spec) {
    rc = c_rest_openapi_spec_destroy(router->openapi_spec);
    if (rc != C_REST_OK) return rc;
  }

  C_REST_FREE((void *)(router));
  return C_REST_OK;
}

static c_rest_error_t find_or_add_child(struct c_rest_route_node *parent,
                             const char *segment, size_t len,
                             struct c_rest_route_node **out_child) {
  struct c_rest_route_node *child;
  struct c_rest_route_node *new_node;



  child = parent->children;

  while (child) {
    if (strlen(child->segment) == len &&
        strncmp(child->segment, segment, len) == 0) {
      *out_child = child;
      return C_REST_OK;
    }
    child = child->next;
  }

  if (create_node(segment, len, &new_node) != C_REST_OK)
    return C_REST_ERROR_GENERIC;

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

  if (!router || !router->root || !method || !path || !handler)
    return C_REST_ERROR_GENERIC;

  curr = router->root;
  p = path;

  if (*p == '/')
    p++;

  while (*p) {
    const char *next_slash = strchr(p, '/');
    size_t len;
    struct c_rest_route_node *child;

    if (next_slash) {
      len = next_slash - p;
    } else {
      len = strlen(p);
    }

    if (len > 0) {
      if (find_or_add_child(curr, p, len, &child) != C_REST_OK)
        return C_REST_ERROR_GENERIC;
      curr = child;
    }

    if (next_slash) {
      p = next_slash + 1;
    } else {
      break;
    }
  }

  h = (struct c_rest_route_handler *)CRF_MALLOC(
      sizeof(struct c_rest_route_handler));
  if (!h)
    return C_REST_ERROR_GENERIC;

  h->method = (char *)CRF_MALLOC(strlen(method) + 1);
  if (!h->method) {
    C_REST_FREE((void *)(h));
    return C_REST_ERROR_GENERIC;
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
  c_rest_error_t rc;
  c_rest_error_t res = c_rest_router_add(router, method, path, handler, user_data);
  if (res == C_REST_OK && op_meta && router->openapi_spec) {
    rc = c_rest_openapi_spec_add_path(router->openapi_spec, path, method, op_meta);
    if (rc != C_REST_OK) return rc;
  }
  return res;
}

#include "c_rest_websocket.h"

struct c_rest_ws_route_data {
  c_rest_websocket_on_message_fn on_message;
  c_rest_websocket_on_close_fn on_close;
  void *user_data;
};

static c_rest_error_t c_rest_ws_upgrade_handler(struct c_rest_request *req,
                                     struct c_rest_response *res,
                                     void *user_data) {
  c_rest_error_t ret;
  (void)user_data; /* Used later by connection logic to call on_message */
  ret = c_rest_websocket_upgrade(req, res);
  if (ret != C_REST_OK) return ret;
  return C_REST_OK;
}

c_rest_error_t c_rest_router_add_websocket(c_rest_router *router, const char *path,
                                c_rest_websocket_on_message_fn on_message,
                                c_rest_websocket_on_close_fn on_close,
                                void *user_data) {
  struct c_rest_ws_route_data *ws_data;

  if (!router || !path)
    return C_REST_ERROR_GENERIC;

  ws_data = (struct c_rest_ws_route_data *)CRF_MALLOC(
      sizeof(struct c_rest_ws_route_data));
  if (!ws_data)
    return C_REST_ERROR_GENERIC;

  ws_data->on_message = on_message;
  ws_data->on_close = on_close;
  ws_data->user_data = user_data;

  return c_rest_router_add(router, "GET", path, c_rest_ws_upgrade_handler,
                           ws_data);
}

c_rest_error_t c_rest_router_add_websocket_openapi(
    c_rest_router *router, const char *path,
    c_rest_websocket_on_message_fn on_message,
    c_rest_websocket_on_close_fn on_close, void *user_data,
    const struct c_rest_openapi_operation *op_meta) {
  c_rest_error_t rc;
  c_rest_error_t res = c_rest_router_add_websocket(router, path, on_message, on_close,
                                        user_data);
  if (res == C_REST_OK && op_meta && router->openapi_spec) {
    rc = c_rest_openapi_spec_add_path(router->openapi_spec, path, "GET", op_meta);
    if (rc != C_REST_OK) return rc;
  }
  return res;
}

#ifdef C_REST_ENABLE_SERVER_SENT_EVENTS_SSE
struct c_rest_sse_route_data {
  c_rest_handler_fn handler;
  void *user_data;
};

static c_rest_error_t c_rest_sse_handler_wrapper(struct c_rest_request *req,
                                      struct c_rest_response *res,
                                      void *user_data) {
  struct c_rest_sse_route_data *sse_data =
      (struct c_rest_sse_route_data *)user_data;
  c_rest_error_t rc = C_REST_OK;

  (void)!c_rest_sse_init_response(res);

  if (sse_data && sse_data->handler) {
    rc = sse_data->handler(req, res, sse_data->user_data);
  }

  return rc;
}

c_rest_error_t c_rest_router_add_sse(c_rest_router *router, const char *path,
                          c_rest_handler_fn handler, void *user_data) {
  struct c_rest_sse_route_data *sse_data;

  if (!router || !path)
    return C_REST_ERROR_GENERIC;

  sse_data = (struct c_rest_sse_route_data *)CRF_MALLOC(
      sizeof(struct c_rest_sse_route_data));
  if (!sse_data)
    return C_REST_ERROR_GENERIC;

  sse_data->handler = handler;
  sse_data->user_data = user_data;

  return c_rest_router_add(router, "GET", path, c_rest_sse_handler_wrapper,
                           sse_data);
}

c_rest_error_t c_rest_router_add_sse_openapi(
    c_rest_router *router, const char *path, c_rest_handler_fn handler,
    void *user_data, const struct c_rest_openapi_operation *op_meta) {
  c_rest_error_t rc;
  int res = c_rest_router_add_sse(router, path, handler, user_data);
  if (res == 0 && op_meta && router->openapi_spec) {
    rc = c_rest_openapi_spec_add_path(router->openapi_spec, path, "GET", op_meta);
    if (rc != C_REST_OK) return rc;
  }
  return res;
}
#endif

#ifdef C_REST_FRAMEWORK_ENABLE_GRAPHQL
#include "c_rest_graphql.h"

static c_rest_error_t c_rest_graphql_handler(struct c_rest_request *req,
                                  struct c_rest_response *res,
                                  void *user_data) {
  struct c_rest_graphql_schema *schema =
      (struct c_rest_graphql_schema *)user_data;
  struct c_rest_graphql_node *doc = NULL;
  char *json = NULL;
  size_t len = 0;
  int ret;
  c_rest_error_t rc;

  if (!req->body) {
    (void)!c_rest_response_set_status(res, 400);
    return C_REST_OK;
  }

  ret = c_rest_graphql_parse((const char *)req->body, req->body_len, &doc);
  if (ret != 0) {
    (void)!c_rest_response_set_status(res, 400);
    return C_REST_OK;
  }

  ret = c_rest_graphql_resolve(doc, schema, &json, &len);
  (void)!c_rest_graphql_node_free(doc);



  (void)!c_rest_response_set_status(res, 200);
  c_rest_response_json(res, json);
  C_REST_FREE((void *)(json));
  return C_REST_OK;
}

c_rest_error_t c_rest_router_add_graphql(c_rest_router *router, const char *path,
                              struct c_rest_graphql_schema *schema) {
  if (!router || !path || !schema)
    return C_REST_ERROR_GENERIC;

  return c_rest_router_add(router, "POST", path, c_rest_graphql_handler,
                           schema);
}

c_rest_error_t c_rest_router_add_graphql_openapi(
    c_rest_router *router, const char *path,
    struct c_rest_graphql_schema *schema,
    const struct c_rest_openapi_operation *op_meta) {
  c_rest_error_t rc;
  int res = c_rest_router_add_graphql(router, path, schema);
  if (res == 0 && op_meta && router->openapi_spec) {
    rc = c_rest_openapi_spec_add_path(router->openapi_spec, path, "POST", op_meta);
    if (rc != C_REST_OK) return rc;
  }
  return res;
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
  c_rest_error_t rc;

  rc = route_data->data_provider(req, &keys, &values, &count,
                                 route_data->user_data);
  if (rc != C_REST_OK) {
    (void)!c_rest_response_set_status(res, 500);
    return rc;
  }

  rc = c_rest_response_template(res, route_data->ctx, keys, values, count);
  if (rc != C_REST_OK) {
    (void)!c_rest_response_set_status(res, 500);
    return rc;
  }

  (void)!c_rest_response_set_status(res, 200);
  return C_REST_OK;
}

c_rest_error_t c_rest_router_add_template(
    c_rest_router *router, const char *method, const char *path,
    const struct c_rest_template_context *ctx,
    c_rest_template_data_fn data_provider, void *user_data) {
  struct c_rest_template_route_data *route_data;

  if (!router || !path || !ctx || !data_provider) {
    return C_REST_ERROR_GENERIC;
  }

  route_data = (struct c_rest_template_route_data *)CRF_MALLOC(
      sizeof(struct c_rest_template_route_data));
  if (!route_data) {
    return C_REST_ERROR_GENERIC;
  }

  route_data->ctx = ctx;
  route_data->data_provider = data_provider;
  route_data->user_data = user_data;

  return c_rest_router_add(router, method, path, c_rest_template_handler,
                           route_data);
}

c_rest_error_t c_rest_router_add_template_openapi(
    c_rest_router *router, const char *method, const char *path,
    const struct c_rest_template_context *ctx,
    c_rest_template_data_fn data_provider, void *user_data,
    const struct c_rest_openapi_operation *op_meta) {
  c_rest_error_t rc;
  int res = c_rest_router_add_template(router, method, path, ctx, data_provider,
                                       user_data);
  if (res == 0 && op_meta && router->openapi_spec) {
    (void)!c_rest_openapi_spec_add_path(router->openapi_spec, path, method,
                                        op_meta);
  }
  return res;
}
#endif

c_rest_error_t
c_rest_router_get_openapi_spec(c_rest_router *router,
                               struct c_rest_openapi_spec **out_spec) {
  if (!router || !out_spec)
    return C_REST_ERROR_GENERIC;
  *out_spec = router->openapi_spec;
  return C_REST_OK;
}

c_rest_error_t c_rest_router_use(c_rest_router *router, const char *path_prefix,
                                 c_rest_middleware_fn middleware,
                                 void *user_data) {
  struct c_rest_middleware_chain *m;
  struct c_rest_middleware_chain *tail;

  if (!router || !middleware)
    return C_REST_ERROR_GENERIC;

  m = (struct c_rest_middleware_chain *)CRF_MALLOC(
      sizeof(struct c_rest_middleware_chain));
  if (!m)
    return C_REST_ERROR_GENERIC;

  if (path_prefix) {
    m->path_prefix = (char *)CRF_MALLOC(strlen(path_prefix) + 1);
    if (!m->path_prefix) {
      C_REST_FREE((void *)(m));
      return C_REST_ERROR_GENERIC;
    }
#if defined(_MSC_VER)
    strcpy_s(m->path_prefix, strlen(path_prefix) + 1, path_prefix);
#else
    strcpy(m->path_prefix, path_prefix);
#endif
  } else {
    m->path_prefix = NULL;
  }

  m->middleware = middleware;
  m->user_data = user_data;
  m->next = NULL;

  if (!router->middlewares) {
    router->middlewares = m;
  } else {
    tail = router->middlewares;
    while (tail->next) {
      tail = tail->next;
    }
    tail->next = m;
  }

  return C_REST_OK;
}

c_rest_error_t c_rest_router_use_post(c_rest_router *router,
                                      const char *path_prefix,
                                      c_rest_middleware_fn middleware,
                                      void *user_data) {
  struct c_rest_middleware_chain *m;
  struct c_rest_middleware_chain *tail;

  if (!router || !middleware)
    return C_REST_ERROR_GENERIC;

  m = (struct c_rest_middleware_chain *)CRF_MALLOC(
      sizeof(struct c_rest_middleware_chain));
  if (!m)
    return C_REST_ERROR_GENERIC;

  if (path_prefix) {
    m->path_prefix = (char *)CRF_MALLOC(strlen(path_prefix) + 1);
    if (!m->path_prefix) {
      C_REST_FREE((void *)(m));
      return C_REST_ERROR_GENERIC;
    }
#if defined(_MSC_VER)
    strcpy_s(m->path_prefix, strlen(path_prefix) + 1, path_prefix);
#else
    strcpy(m->path_prefix, path_prefix);
#endif
  } else {
    m->path_prefix = NULL;
  }

  m->middleware = middleware;
  m->user_data = user_data;
  m->next = NULL;

  if (!router->post_middlewares) {
    router->post_middlewares = m;
  } else {
    tail = router->post_middlewares;
    while (tail->next) {
      tail = tail->next;
    }
    tail->next = m;
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
    if (child->is_wildcard) {
      *out_node = child;
      return C_REST_OK;
    }

    if (child->is_var || (strlen(child->segment) == len &&
                          strncmp(child->segment, path, len) == 0)) {
      res = match_route(child, next_slash ? next_slash + 1 : "", req, &matched);
      if (res == C_REST_OK && matched) {
        if (child->is_var && req) {
          struct c_rest_path_var *var = (struct c_rest_path_var *)CRF_MALLOC(
              sizeof(struct c_rest_path_var));
          if (var) {
            var->name = (char *)CRF_MALLOC(strlen(child->var_name) + 1);
            if (var->name) {
#if defined(_MSC_VER)
              strcpy_s(var->name, strlen(child->var_name) + 1, child->var_name);
#else
              strcpy(var->name, child->var_name);
#endif
            }

            var->value = (char *)CRF_MALLOC(len + 1);
            if (var->value) {
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
  c_rest_error_t middleware_res;
  c_rest_error_t match_res;

  if (!router || !req || !res)
    return C_REST_ERROR_GENERIC;

  req->path_vars = NULL;

  /* 1. Execute middleware chain */
  m = router->middlewares;
  while (m) {
    if (!m->path_prefix ||
        strncmp(req->path, m->path_prefix, strlen(m->path_prefix)) == 0) {
      middleware_res = m->middleware(req, res, m->user_data);
      if (middleware_res != C_REST_OK) {
        /* Middleware short-circuited or failed */
        return middleware_res;
      }
    }
    m = m->next;
  }

  /* 2. Resolve Route */
  match_res = match_route(router->root, req->path, req, &matched_node);
  if (match_res != C_REST_OK || !matched_node) {
    /* 404 Not Found */
    (void)!c_rest_response_set_status(res, 404);
    return C_REST_OK;
  }

  /* 3. Check Method */
  h = matched_node->handlers;
  while (h) {
    if (strcmp(h->method, req->method) == 0) {
      match_res = h->handler(req, res, h->user_data);
      if (match_res != C_REST_OK)
        return match_res;
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
    if (!m->path_prefix ||
        strncmp(req->path, m->path_prefix, strlen(m->path_prefix)) == 0) {
      middleware_res = m->middleware(req, res, m->user_data);
      if (middleware_res != C_REST_OK)
        return (c_rest_error_t)middleware_res;
    }
    m = m->next;
  }

  return C_REST_OK;
}
