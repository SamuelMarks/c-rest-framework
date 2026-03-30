/* clang-format off */
#include "c_rest_graphql.h"
#include "c_rest_mem.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* clang-format on */

#ifdef C_REST_FRAMEWORK_ENABLE_GRAPHQL

/**
 * @brief Allocates a new node.
 */
static struct c_rest_graphql_node *
alloc_node(enum c_rest_graphql_node_type type) {
  struct c_rest_graphql_node *node = NULL;
  if (C_REST_MALLOC(sizeof(struct c_rest_graphql_node), (void **)&node) != 0) {
    return NULL;
  }
  node->type = type;
  node->name = NULL;
  node->alias = NULL;
  node->value = NULL;
  node->op_type = C_REST_GRAPHQL_OP_QUERY;
  node->arguments = NULL;
  node->selection_set = NULL;
  node->definitions = NULL;
  return node;
}

/**
 * @brief Allocates a new node list.
 */
static struct c_rest_graphql_node_list *alloc_list(void) {
  struct c_rest_graphql_node_list *list = NULL;
  if (C_REST_MALLOC(sizeof(struct c_rest_graphql_node_list), (void **)&list) !=
      0) {
    return NULL;
  }
  list->nodes = NULL;
  list->count = 0;
  list->capacity = 0;
  return list;
}

/**
 * @brief Appends a node to a list.
 */
static int list_append(struct c_rest_graphql_node_list *list,
                       struct c_rest_graphql_node *node) {
  if (list->count >= list->capacity) {
    size_t new_cap = list->capacity == 0 ? 4 : list->capacity * 2;
    struct c_rest_graphql_node **new_nodes = NULL;
    if (C_REST_MALLOC(new_cap * sizeof(struct c_rest_graphql_node *),
                      (void **)&new_nodes) != 0) {
      return -1;
    }
    if (list->nodes) {
      memcpy(new_nodes, list->nodes,
             list->count * sizeof(struct c_rest_graphql_node *));
      C_REST_FREE(list->nodes);
    }
    list->nodes = new_nodes;
    list->capacity = new_cap;
  }
  list->nodes[list->count++] = node;
  return 0;
}

static void skip_whitespace(struct c_rest_graphql_context *ctx) {
  while (ctx->position < ctx->length) {
    char c = ctx->input[ctx->position];
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == ',') {
      ctx->position++;
    } else if (c == '#') {
      while (ctx->position < ctx->length && ctx->input[ctx->position] != '\n' &&
             ctx->input[ctx->position] != '\r') {
        ctx->position++;
      }
    } else {
      break;
    }
  }
}

static char *parse_name(struct c_rest_graphql_context *ctx) {
  size_t start;
  size_t len;
  char *str;

  skip_whitespace(ctx);
  if (ctx->position >= ctx->length) {
    return NULL;
  }

  start = ctx->position;
  if (isalpha((unsigned char)ctx->input[ctx->position]) ||
      ctx->input[ctx->position] == '_') {
    ctx->position++;
    while (ctx->position < ctx->length &&
           (isalnum((unsigned char)ctx->input[ctx->position]) ||
            ctx->input[ctx->position] == '_')) {
      ctx->position++;
    }
  } else {
    return NULL;
  }

  len = ctx->position - start;
  str = NULL;
  if (C_REST_MALLOC(len + 1, (void **)&str) != 0) {
    return NULL;
  }
  memcpy(str, &ctx->input[start], len);
  str[len] = '\0';
  return str;
}

static int parse_field(struct c_rest_graphql_context *ctx,
                       struct c_rest_graphql_node **out_node);

static int parse_selection_set(struct c_rest_graphql_context *ctx,
                               struct c_rest_graphql_node_list **out_list) {
  struct c_rest_graphql_node_list *list;
  size_t i;

  skip_whitespace(ctx);
  if (ctx->position >= ctx->length || ctx->input[ctx->position] != '{') {
    return -1;
  }
  ctx->position++; /* skip '{' */

  list = alloc_list();
  if (!list)
    return -1;

  while (ctx->position < ctx->length) {
    struct c_rest_graphql_node *field = NULL;
    skip_whitespace(ctx);
    if (ctx->position < ctx->length && ctx->input[ctx->position] == '}') {
      ctx->position++; /* skip '}' */
      if (list->count == 0) {
        break; /* Error: Empty selection set not allowed */
      }
      *out_list = list;
      return 0;
    }

    if (parse_field(ctx, &field) == 0) {
      list_append(list, field);
    } else {
      break; /* fallback or error */
    }
  }

  /* Error handling: EOF without '}' or parse error inside */
  for (i = 0; i < list->count; i++) {
    c_rest_graphql_node_free(list->nodes[i]);
  }
  if (list->nodes)
    C_REST_FREE(list->nodes);
  C_REST_FREE(list);
  return -1;
}

static int parse_field(struct c_rest_graphql_context *ctx,
                       struct c_rest_graphql_node **out_node) {
  char *name;
  struct c_rest_graphql_node *node;

  name = parse_name(ctx);
  if (!name) {
    return -1;
  }

  node = alloc_node(C_REST_GRAPHQL_NODE_FIELD);
  if (!node) {
    C_REST_FREE(name);
    return -1;
  }

  skip_whitespace(ctx);
  if (ctx->position < ctx->length && ctx->input[ctx->position] == ':') {
    /* This was an alias */
    ctx->position++; /* skip ':' */
    node->alias = name;
    node->name = parse_name(ctx);
  } else {
    node->name = name;
  }

  /* parse selection set if any */
  skip_whitespace(ctx);
  if (ctx->position < ctx->length && ctx->input[ctx->position] == '{') {
    if (parse_selection_set(ctx, &node->selection_set) != 0) {
      c_rest_graphql_node_free(node);
      return -1;
    }
  }

  *out_node = node;
  return 0;
}

static int parse_operation(struct c_rest_graphql_context *ctx,
                           struct c_rest_graphql_node **out_node) {
  struct c_rest_graphql_node *node;
  char *name = NULL;
  enum c_rest_graphql_operation_type op_type = C_REST_GRAPHQL_OP_QUERY;

  skip_whitespace(ctx);

  /* Very basic check for query/mutation */
  if (ctx->position + 5 <= ctx->length &&
      strncmp(&ctx->input[ctx->position], "query", 5) == 0) {
    ctx->position += 5;
    op_type = C_REST_GRAPHQL_OP_QUERY;
    name = parse_name(ctx);
  } else if (ctx->position + 8 <= ctx->length &&
             strncmp(&ctx->input[ctx->position], "mutation", 8) == 0) {
    ctx->position += 8;
    op_type = C_REST_GRAPHQL_OP_MUTATION;
    name = parse_name(ctx);
  }

  node = alloc_node(C_REST_GRAPHQL_NODE_OPERATION);
  if (!node) {
    if (name)
      C_REST_FREE(name);
    return -1;
  }

  node->op_type = op_type;
  node->name = name;

  skip_whitespace(ctx);
  if (ctx->position < ctx->length && ctx->input[ctx->position] == '{') {
    if (parse_selection_set(ctx, &node->selection_set) != 0) {
      c_rest_graphql_node_free(node);
      return -1;
    }
    *out_node = node;
    return 0;
  }

  /* Shorthand query without "query" keyword */
  if (!name) {
    /* Rewind and try to parse selection set for anonymous query */
    ctx->position = 0; /* rough fallback */
    skip_whitespace(ctx);
    if (ctx->position < ctx->length && ctx->input[ctx->position] == '{') {
      if (parse_selection_set(ctx, &node->selection_set) != 0) {
        c_rest_graphql_node_free(node);
        return -1;
      }
      *out_node = node;
      return 0;
    }
  }

  c_rest_graphql_node_free(node);
  return -1;
}

int c_rest_graphql_parse(const char *query, size_t query_len,
                         struct c_rest_graphql_node **out_doc) {
  struct c_rest_graphql_context ctx;
  struct c_rest_graphql_node *doc;
  struct c_rest_graphql_node *op = NULL;

  if (!query || !out_doc)
    return -1;

  ctx.input = query;
  ctx.length = query_len;
  ctx.position = 0;
  ctx.error_message = NULL;
  ctx.error_pos = -1;

  doc = alloc_node(C_REST_GRAPHQL_NODE_DOCUMENT);
  if (!doc)
    return -1;

  doc->definitions = alloc_list();
  if (!doc->definitions) {
    c_rest_graphql_node_free(doc);
    return -1;
  }

  skip_whitespace(&ctx);
  while (ctx.position < ctx.length) {
    if (parse_operation(&ctx, &op) == 0) {
      list_append(doc->definitions, op);
    } else {
      break;
    }
    skip_whitespace(&ctx);
  }

  if (ctx.position < ctx.length || doc->definitions->count == 0) {
    c_rest_graphql_node_free(doc);
    return -1;
  }

  *out_doc = doc;
  return 0;
}

int c_rest_graphql_node_free(struct c_rest_graphql_node *node) {
  size_t i;

  if (!node)
    return 0;

  if (node->name)
    C_REST_FREE(node->name);
  if (node->alias)
    C_REST_FREE(node->alias);
  if (node->value)
    C_REST_FREE(node->value);

  if (node->arguments) {
    for (i = 0; i < node->arguments->count; i++) {
      c_rest_graphql_node_free(node->arguments->nodes[i]);
    }
    if (node->arguments->nodes)
      C_REST_FREE(node->arguments->nodes);
    C_REST_FREE(node->arguments);
  }

  if (node->selection_set) {
    for (i = 0; i < node->selection_set->count; i++) {
      c_rest_graphql_node_free(node->selection_set->nodes[i]);
    }
    if (node->selection_set->nodes)
      C_REST_FREE(node->selection_set->nodes);
    C_REST_FREE(node->selection_set);
  }

  if (node->definitions) {
    for (i = 0; i < node->definitions->count; i++) {
      c_rest_graphql_node_free(node->definitions->nodes[i]);
    }
    if (node->definitions->nodes)
      C_REST_FREE(node->definitions->nodes);
    C_REST_FREE(node->definitions);
  }

  C_REST_FREE(node);
  return 0;
}

struct c_rest_graphql_resolver_entry {
  char *field_name;
  c_rest_graphql_resolver_fn resolver;
  void *user_data;
  struct c_rest_graphql_resolver_entry *next;
};

int c_rest_graphql_schema_init(struct c_rest_graphql_schema **schema) {
  if (!schema)
    return -1;
  if (C_REST_MALLOC(sizeof(struct c_rest_graphql_schema), (void **)schema) != 0)
    return -1;
  (*schema)->resolvers = NULL;
  return 0;
}

int c_rest_graphql_schema_free(struct c_rest_graphql_schema *schema) {
  struct c_rest_graphql_resolver_entry *entry, *next;
  if (!schema)
    return 0;
  entry = schema->resolvers;
  while (entry) {
    next = entry->next;
    C_REST_FREE(entry->field_name);
    C_REST_FREE(entry);
    entry = next;
  }
  C_REST_FREE(schema);
  return 0;
}

int c_rest_graphql_schema_add_resolver(struct c_rest_graphql_schema *schema,
                                       const char *field_name,
                                       c_rest_graphql_resolver_fn resolver,
                                       void *user_data) {
  struct c_rest_graphql_resolver_entry *entry;
  size_t len;

  if (!schema || !field_name || !resolver)
    return -1;

  if (C_REST_MALLOC(sizeof(struct c_rest_graphql_resolver_entry),
                    (void **)&entry) != 0)
    return -1;

  len = strlen(field_name);
  if (C_REST_MALLOC(len + 1, (void **)&entry->field_name) != 0) {
    C_REST_FREE(entry);
    return -1;
  }
  memcpy(entry->field_name, field_name, len + 1);

  entry->resolver = resolver;
  entry->user_data = user_data;
  entry->next = schema->resolvers;
  schema->resolvers = entry;

  return 0;
}

int c_rest_graphql_resolve(struct c_rest_graphql_node *doc,
                           struct c_rest_graphql_schema *schema,
                           char **out_json, size_t *out_len) {
  const char *dummy = "{\"data\": {}}";
  size_t len = strlen(dummy);
  size_t i, j;

  if (!doc || !out_json || !out_len)
    return -1;

  /* Basic mock traversal: look for requested fields and trigger resolvers */
  if (schema && doc->definitions) {
    for (i = 0; i < doc->definitions->count; i++) {
      struct c_rest_graphql_node *op = doc->definitions->nodes[i];
      if (op->type == C_REST_GRAPHQL_NODE_OPERATION && op->selection_set) {
        for (j = 0; j < op->selection_set->count; j++) {
          struct c_rest_graphql_node *field = op->selection_set->nodes[j];
          if (field->type == C_REST_GRAPHQL_NODE_FIELD) {
            struct c_rest_graphql_resolver_entry *entry = schema->resolvers;
            while (entry) {
              if (strcmp(entry->field_name, field->name) == 0) {
                char *res_json = NULL;
                size_t res_len = 0;
                /* Fire resolver (ignore output for this basic skeleton) */
                entry->resolver(field->name, &res_json, &res_len,
                                entry->user_data);
                if (res_json) {
                  C_REST_FREE(res_json);
                }
              }
              entry = entry->next;
            }
          }
        }
      }
    }
  }

  *out_json = NULL;
  if (C_REST_MALLOC(len + 1, (void **)out_json) != 0)
    return -1;

  memcpy(*out_json, dummy, len + 1);
  *out_len = len;

  return 0;
}

#endif /* C_REST_FRAMEWORK_ENABLE_GRAPHQL */
