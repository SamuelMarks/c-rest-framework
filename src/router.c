/* clang-format off */
#include "c_rest_request.h"
#include "c_rest_response.h"
#include "c_rest_router.h"
#include "c_rest_openapi.h"

#include <stdlib.h>
#include <string.h>
/* clang-format on */

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

static int create_node(const char *segment, size_t len,
                       struct c_rest_route_node **out_node) {
  struct c_rest_route_node *node;

  if (!out_node)
    return 1;

  node = (struct c_rest_route_node *)malloc(sizeof(struct c_rest_route_node));
  if (!node)
    return 1;

  node->segment = (char *)malloc(len + 1);
  if (!node->segment) {
    free(node);
    return 1;
  }
  memcpy(node->segment, segment, len);
  node->segment[len] = '\0';

  node->is_var = 0;
  node->is_wildcard = 0;
  node->var_name = NULL;

  if (len > 0 && node->segment[0] == ':') {
    node->is_var = 1;
    node->var_name = (char *)malloc(len);
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
  return 0;
}

static int free_node(struct c_rest_route_node *node) {
  struct c_rest_route_handler *h;
  if (!node)
    return 1;

  if (node->segment)
    free(node->segment);
  if (node->var_name)
    free(node->var_name);

  h = node->handlers;
  while (h) {
    struct c_rest_route_handler *next_h = h->next;
    if (h->method)
      free(h->method);
    free(h);
    h = next_h;
  }

  free_node(node->children);
  free_node(node->next);
  free(node);
  return 0;
}
int c_rest_router_init(c_rest_router **out_router) {
  struct c_rest_router *router;

  if (!out_router)
    return 1;

  router = (struct c_rest_router *)malloc(sizeof(struct c_rest_router));
  if (!router)
    return 1;

  if (create_node("", 0, &router->root) != 0) {
    free(router);
    return 1;
  }
  router->middlewares = NULL;
  router->post_middlewares = NULL;
  router->openapi_spec = NULL;
  c_rest_openapi_spec_init(&router->openapi_spec);

  *out_router = router;
  return 0;
}

int c_rest_router_destroy(c_rest_router *router) {
  struct c_rest_middleware_chain *m;

  if (!router)
    return 1;

  free_node(router->root);

  m = router->middlewares;
  while (m) {
    struct c_rest_middleware_chain *next_m = m->next;
    if (m->path_prefix)
      free(m->path_prefix);
    free(m);
    m = next_m;
  }

  m = router->post_middlewares;
  while (m) {
    struct c_rest_middleware_chain *next_m = m->next;
    if (m->path_prefix)
      free(m->path_prefix);
    free(m);
    m = next_m;
  }

  if (router->openapi_spec) {
    c_rest_openapi_spec_destroy(router->openapi_spec);
  }

  free(router);
  return 0;
}

static int find_or_add_child(struct c_rest_route_node *parent,
                             const char *segment, size_t len,
                             struct c_rest_route_node **out_child) {
  struct c_rest_route_node *child;
  struct c_rest_route_node *new_node;

  if (!parent || !out_child)
    return 1;

  child = parent->children;

  while (child) {
    if (strlen(child->segment) == len &&
        strncmp(child->segment, segment, len) == 0) {
      *out_child = child;
      return 0;
    }
    child = child->next;
  }

  if (create_node(segment, len, &new_node) != 0)
    return 1;

  new_node->next = parent->children;
  parent->children = new_node;
  new_node->parent = parent;

  *out_child = new_node;
  return 0;
}
int c_rest_router_add(c_rest_router *router, const char *method,
                      const char *path, c_rest_handler_fn handler,
                      void *user_data) {
  const char *p;
  struct c_rest_route_node *curr;
  struct c_rest_route_handler *h;

  if (!router || !router->root || !method || !path || !handler)
    return 1;

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
      if (find_or_add_child(curr, p, len, &child) != 0)
        return 1;
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
  if (!h)
    return 1;

  h->method = (char *)malloc(strlen(method) + 1);
  if (!h->method) {
    free(h);
    return 1;
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

  return 0;
}

int c_rest_router_add_openapi(c_rest_router *router, const char *method,
                              const char *path, c_rest_handler_fn handler,
                              void *user_data,
                              const struct c_rest_openapi_operation *op_meta) {
  int res = c_rest_router_add(router, method, path, handler, user_data);
  if (res == 0 && op_meta && router->openapi_spec) {
    c_rest_openapi_spec_add_path(router->openapi_spec, path, method, op_meta);
  }
  return res;
}

int c_rest_router_get_openapi_spec(c_rest_router *router, struct c_rest_openapi_spec **out_spec) {
  if (!router || !out_spec)
    return -1;
  *out_spec = router->openapi_spec;
  return 0;
}

int c_rest_router_use(c_rest_router *router, const char *path_prefix,
                      c_rest_middleware_fn middleware, void *user_data) {
  struct c_rest_middleware_chain *m;
  struct c_rest_middleware_chain *tail;

  if (!router || !middleware)
    return 1;

  m = (struct c_rest_middleware_chain *)malloc(
      sizeof(struct c_rest_middleware_chain));
  if (!m)
    return 1;

  if (path_prefix) {
    m->path_prefix = (char *)malloc(strlen(path_prefix) + 1);
    if (!m->path_prefix) {
      free(m);
      return 1;
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

  return 0;
}

int c_rest_router_use_post(c_rest_router *router, const char *path_prefix,
                           c_rest_middleware_fn middleware, void *user_data) {
  struct c_rest_middleware_chain *m;
  struct c_rest_middleware_chain *tail;

  if (!router || !middleware)
    return 1;

  m = (struct c_rest_middleware_chain *)malloc(
      sizeof(struct c_rest_middleware_chain));
  if (!m)
    return 1;

  if (path_prefix) {
    m->path_prefix = (char *)malloc(strlen(path_prefix) + 1);
    if (!m->path_prefix) {
      free(m);
      return 1;
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

  return 0;
}

static int match_route(struct c_rest_route_node *node, const char *path,
                       struct c_rest_request *req,
                       struct c_rest_route_node **out_node) {
  const char *next_slash;
  size_t len;
  struct c_rest_route_node *child;
  struct c_rest_route_node *matched = NULL;
  int res;

  if (!node || !out_node)
    return 1;

  while (*path == '/')
    path++;

  if (*path == '\0') {
    *out_node = node;
    return 0;
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
      return 0;
    }

    if (child->is_var || (strlen(child->segment) == len &&
                          strncmp(child->segment, path, len) == 0)) {
      res = match_route(child, next_slash ? next_slash + 1 : "", req, &matched);
      if (res == 0 && matched) {
        if (child->is_var && req) {
          struct c_rest_path_var *var =
              (struct c_rest_path_var *)malloc(sizeof(struct c_rest_path_var));
          if (var) {
            var->name = (char *)malloc(strlen(child->var_name) + 1);
            if (var->name) {
#if defined(_MSC_VER)
              strcpy_s(var->name, strlen(child->var_name) + 1, child->var_name);
#else
              strcpy(var->name, child->var_name);
#endif
            }

            var->value = (char *)malloc(len + 1);
            if (var->value) {
              memcpy(var->value, path, len);
              var->value[len] = '\0';
            }

            var->next = req->path_vars;
            req->path_vars = var;
          }
        }
        *out_node = matched;
        return 0;
      }
    }
    child = child->next;
  }

  *out_node = NULL;
  return 1;
}

int c_rest_router_dispatch(c_rest_router *router, struct c_rest_request *req,
                           struct c_rest_response *res) {
  struct c_rest_middleware_chain *m;
  struct c_rest_route_node *matched_node = NULL;
  struct c_rest_route_handler *h;
  int middleware_res;
  int match_res;

  if (!router || !req || !res)
    return 1;

  req->path_vars = NULL;

  /* 1. Execute middleware chain */
  m = router->middlewares;
  while (m) {
    if (!m->path_prefix ||
        strncmp(req->path, m->path_prefix, strlen(m->path_prefix)) == 0) {
      middleware_res = m->middleware(req, res, m->user_data);
      if (middleware_res != 0) {
        /* Middleware short-circuited */
        return 0;
      }
    }
    m = m->next;
  }

  /* 2. Resolve Route */
  match_res = match_route(router->root, req->path, req, &matched_node);
  if (match_res != 0 || !matched_node) {
    /* 404 Not Found */
    res->status_code = 404;
    return 0;
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
    if (!m->path_prefix ||
        strncmp(req->path, m->path_prefix, strlen(m->path_prefix)) == 0) {
      m->middleware(req, res, m->user_data);
    }
    m = m->next;
  }

  return 0;
}
