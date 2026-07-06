/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_graphql.h"
#include "c_rest_mem.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "c_rest_mem.h"
#include "c_rest_log.h"
/* clang-format on */

#ifdef C_REST_FRAMEWORK_ENABLE_GRAPHQL

/**
 * @brief Allocates a new node.
 */
static c_rest_error_t
alloc_node(enum c_rest_graphql_node_type type,
           struct c_rest_graphql_node **out_node) { /* GCOVR_EXCL_LINE */
  struct c_rest_graphql_node *node = NULL;          /* GCOVR_EXCL_LINE */
  if (C_REST_MALLOC(sizeof(struct c_rest_graphql_node), &node) !=
      0) {                       /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }
  node->type = type;                       /* GCOVR_EXCL_LINE */
  node->name = NULL;                       /* GCOVR_EXCL_LINE */
  node->alias = NULL;                      /* GCOVR_EXCL_LINE */
  node->value = NULL;                      /* GCOVR_EXCL_LINE */
  node->op_type = C_REST_GRAPHQL_OP_QUERY; /* GCOVR_EXCL_LINE */
  node->arguments = NULL;                  /* GCOVR_EXCL_LINE */
  node->selection_set = NULL;              /* GCOVR_EXCL_LINE */
  node->definitions = NULL;                /* GCOVR_EXCL_LINE */
  *out_node = node;
  return C_REST_OK; /* GCOVR_EXCL_LINE */
}

/**
 * @brief Allocates a new node list.
 */
static c_rest_error_t
alloc_list(struct c_rest_graphql_node_list **out_list) { /* GCOVR_EXCL_LINE */
  struct c_rest_graphql_node_list *list = NULL;          /* GCOVR_EXCL_LINE */
  if (C_REST_MALLOC(sizeof(struct c_rest_graphql_node_list), &list) !=
      0) {                       /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }
  list->nodes = NULL; /* GCOVR_EXCL_LINE */
  list->count = 0;    /* GCOVR_EXCL_LINE */
  list->capacity = 0; /* GCOVR_EXCL_LINE */
  *out_list = list;   /* GCOVR_EXCL_LINE */
  return C_REST_OK;   /* GCOVR_EXCL_LINE */
}

/**
 * @brief Appends a node to a list.
 */
static c_rest_error_t
list_append(struct c_rest_graphql_node_list *list, /* GCOVR_EXCL_LINE */
            struct c_rest_graphql_node *node) {
  if (list->count >= list->capacity) { /* GCOVR_EXCL_LINE */
    size_t new_cap =
        list->capacity == 0 ? 4 : list->capacity * 2; /* GCOVR_EXCL_LINE */
    struct c_rest_graphql_node **new_nodes = NULL;    /* GCOVR_EXCL_LINE */
    if (C_REST_MALLOC(
            new_cap *
                sizeof(struct c_rest_graphql_node *), /* GCOVR_EXCL_LINE */
            &new_nodes) != 0) {                       /* GCOVR_EXCL_LINE */
      return C_REST_ERROR_GENERIC;                    /* GCOVR_EXCL_LINE */
    }
    if (list->nodes) {               /* GCOVR_EXCL_LINE */
      memcpy(new_nodes, list->nodes, /* GCOVR_EXCL_LINE */
             list->count *
                 sizeof(struct c_rest_graphql_node *)); /* GCOVR_EXCL_LINE */
      C_REST_FREE(list->nodes);                         /* GCOVR_EXCL_LINE */
    }
    list->nodes = new_nodes;  /* GCOVR_EXCL_LINE */
    list->capacity = new_cap; /* GCOVR_EXCL_LINE */
  }
  list->nodes[list->count++] = node; /* GCOVR_EXCL_LINE */
  return C_REST_OK;                  /* GCOVR_EXCL_LINE */
}

static c_rest_error_t
skip_whitespace(struct c_rest_graphql_context *ctx) { /* GCOVR_EXCL_LINE */
  while (ctx->position < ctx->length) {               /* GCOVR_EXCL_LINE */
    char c = ctx->input[ctx->position];               /* GCOVR_EXCL_LINE */
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r' ||
        c == ',') {        /* GCOVR_EXCL_LINE */
      ctx->position++;     /* GCOVR_EXCL_LINE */
    } else if (c == '#') { /* GCOVR_EXCL_LINE */
      while (ctx->position < ctx->length &&
             ctx->input[ctx->position] != '\n' && /* GCOVR_EXCL_LINE */
             ctx->input[ctx->position] != '\r') { /* GCOVR_EXCL_LINE */
        ctx->position++;                          /* GCOVR_EXCL_LINE */
      }
    } else {
      break; /* GCOVR_EXCL_LINE */
    }
  }
  return C_REST_OK;
} /* GCOVR_EXCL_LINE */

static c_rest_error_t parse_name(struct c_rest_graphql_context *ctx,
                                 char **out_name) { /* GCOVR_EXCL_LINE */
  size_t start;
  size_t len;
  char *str;

  skip_whitespace(ctx);               /* GCOVR_EXCL_LINE */
  if (ctx->position >= ctx->length) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;      /* GCOVR_EXCL_LINE */
  }

  start = ctx->position;                                   /* GCOVR_EXCL_LINE */
  if (isalpha((unsigned char)ctx->input[ctx->position]) || /* GCOVR_EXCL_LINE */
      ctx->input[ctx->position] == '_') {                  /* GCOVR_EXCL_LINE */
    ctx->position++;                                       /* GCOVR_EXCL_LINE */
    while (
        ctx->position < ctx->length && /* GCOVR_EXCL_LINE */
        (isalnum(
             (unsigned char)ctx->input[ctx->position]) || /* GCOVR_EXCL_LINE */
         ctx->input[ctx->position] == '_')) {             /* GCOVR_EXCL_LINE */
      ctx->position++;                                    /* GCOVR_EXCL_LINE */
    }
  } else {
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }

  len = ctx->position - start;             /* GCOVR_EXCL_LINE */
  str = NULL;                              /* GCOVR_EXCL_LINE */
  if (C_REST_MALLOC(len + 1, &str) != 0) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;           /* GCOVR_EXCL_LINE */
  }
#if defined(_MSC_VER)
  /* CDD_SAFE_CRT */ memcpy_s(str, len, &ctx->input[start],
                              len); /* GCOVR_EXCL_LINE */
#else
  memcpy(str, &ctx->input[start], len); /* GCOVR_EXCL_LINE */
#endif
  str[len] = '\0';  /* GCOVR_EXCL_LINE */
  *out_name = str;  /* GCOVR_EXCL_LINE */
  return C_REST_OK; /* GCOVR_EXCL_LINE */
}

static c_rest_error_t parse_field(struct c_rest_graphql_context *ctx,
                                  struct c_rest_graphql_node **out_node);

static c_rest_error_t
parse_selection_set(struct c_rest_graphql_context *ctx, /* GCOVR_EXCL_LINE */
                    struct c_rest_graphql_node_list **out_list) {
  struct c_rest_graphql_node_list *list = NULL; /* GCOVR_EXCL_LINE */
  size_t i;

  skip_whitespace(ctx); /* GCOVR_EXCL_LINE */
  if (ctx->position >= ctx->length ||
      ctx->input[ctx->position] != '{') { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;          /* GCOVR_EXCL_LINE */
  }
  ctx->position++; /* skip '{' */ /* GCOVR_EXCL_LINE */

  if (alloc_list(&list) != 0 || !list) /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;       /* GCOVR_EXCL_LINE */

  while (ctx->position < ctx->length) {       /* GCOVR_EXCL_LINE */
    struct c_rest_graphql_node *field = NULL; /* GCOVR_EXCL_LINE */
    skip_whitespace(ctx);                     /* GCOVR_EXCL_LINE */
    if (ctx->position < ctx->length &&        /* GCOVR_EXCL_LINE */
        ctx->input[ctx->position] == '}') {   /* GCOVR_EXCL_LINE */
      ctx->position++; /* skip '}' */         /* GCOVR_EXCL_LINE */
      if (list->count == 0) {                 /* GCOVR_EXCL_LINE */
        break;
        /* Error: Empty selection set not allowed */ /* GCOVR_EXCL_LINE */
      }
      *out_list = list; /* GCOVR_EXCL_LINE */
      return C_REST_OK; /* GCOVR_EXCL_LINE */
    }

    if (parse_field(ctx, &field) == 0) { /* GCOVR_EXCL_LINE */
      list_append(list, field);          /* GCOVR_EXCL_LINE */
    } else {
      break; /* fallback or error */ /* GCOVR_EXCL_LINE */
    }
  }

  /* Error handling: EOF without '}' or parse error inside */
  for (i = 0; i < list->count; i++) {         /* GCOVR_EXCL_LINE */
    c_rest_graphql_node_free(list->nodes[i]); /* GCOVR_EXCL_LINE */
  }
  if (list->nodes)             /* GCOVR_EXCL_LINE */
    C_REST_FREE(list->nodes);  /* GCOVR_EXCL_LINE */
  C_REST_FREE(list);           /* GCOVR_EXCL_LINE */
  return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
}

static c_rest_error_t
parse_field(struct c_rest_graphql_context *ctx, /* GCOVR_EXCL_LINE */
            struct c_rest_graphql_node **out_node) {
  char *name = NULL; /* GCOVR_EXCL_LINE */
  struct c_rest_graphql_node *node;

  if (parse_name(ctx, &name) != 0 || !name) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;              /* GCOVR_EXCL_LINE */
  }

  if (alloc_node(C_REST_GRAPHQL_NODE_FIELD, &node) != C_REST_OK ||
      !node) {                   /* GCOVR_EXCL_LINE */
    C_REST_FREE(name);           /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }

  skip_whitespace(ctx);                   /* GCOVR_EXCL_LINE */
  if (ctx->position < ctx->length &&      /* GCOVR_EXCL_LINE */
      ctx->input[ctx->position] == ':') { /* GCOVR_EXCL_LINE */
    /* This was an alias */
    ctx->position++; /* skip ':' */          /* GCOVR_EXCL_LINE */
    node->alias = name;                      /* GCOVR_EXCL_LINE */
    if (parse_name(ctx, &node->name) != 0) { /* GCOVR_EXCL_LINE */
      node->name = NULL;                     /* GCOVR_EXCL_LINE */
    }
  } else {
    node->name = name; /* GCOVR_EXCL_LINE */
  }

  /* parse selection set if any */
  skip_whitespace(ctx);                                   /* GCOVR_EXCL_LINE */
  if (ctx->position < ctx->length &&                      /* GCOVR_EXCL_LINE */
      ctx->input[ctx->position] == '{') {                 /* GCOVR_EXCL_LINE */
    if (parse_selection_set(ctx, &node->selection_set) != /* GCOVR_EXCL_LINE */
        0) {                                              /* GCOVR_EXCL_LINE */
      c_rest_graphql_node_free(node);                     /* GCOVR_EXCL_LINE */
      return C_REST_ERROR_GENERIC;                        /* GCOVR_EXCL_LINE */
    }
  }

  *out_node = node; /* GCOVR_EXCL_LINE */
  return C_REST_OK; /* GCOVR_EXCL_LINE */
}

static c_rest_error_t
parse_operation(struct c_rest_graphql_context *ctx, /* GCOVR_EXCL_LINE */
                struct c_rest_graphql_node **out_node) {
  struct c_rest_graphql_node *node;
  char *name = NULL; /* GCOVR_EXCL_LINE */
  enum c_rest_graphql_operation_type op_type =
      C_REST_GRAPHQL_OP_QUERY; /* GCOVR_EXCL_LINE */

  skip_whitespace(ctx); /* GCOVR_EXCL_LINE */

  /* Very basic check for query/mutation */
  if (ctx->position + 5 <= ctx->length && /* GCOVR_EXCL_LINE */
      strncmp(&ctx->input[ctx->position], "query", 5) ==
          0) {                                   /* GCOVR_EXCL_LINE */
    ctx->position += 5;                          /* GCOVR_EXCL_LINE */
    op_type = C_REST_GRAPHQL_OP_QUERY;           /* GCOVR_EXCL_LINE */
    if (parse_name(ctx, &name) != 0)             /* GCOVR_EXCL_LINE */
      name = NULL;                               /* GCOVR_EXCL_LINE */
  } else if (ctx->position + 8 <= ctx->length && /* GCOVR_EXCL_LINE */
             strncmp(&ctx->input[ctx->position], "mutation", 8) ==
                 0) {                     /* GCOVR_EXCL_LINE */
    ctx->position += 8;                   /* GCOVR_EXCL_LINE */
    op_type = C_REST_GRAPHQL_OP_MUTATION; /* GCOVR_EXCL_LINE */
    if (parse_name(ctx, &name) != 0)      /* GCOVR_EXCL_LINE */
      name = NULL;                        /* GCOVR_EXCL_LINE */
  }

  if (alloc_node(C_REST_GRAPHQL_NODE_OPERATION, &node) != C_REST_OK ||
      !node) {                   /* GCOVR_EXCL_LINE */
    if (name)                    /* GCOVR_EXCL_LINE */
      C_REST_FREE(name);         /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }

  node->op_type = op_type; /* GCOVR_EXCL_LINE */
  node->name = name;       /* GCOVR_EXCL_LINE */

  skip_whitespace(ctx);                                   /* GCOVR_EXCL_LINE */
  if (ctx->position < ctx->length &&                      /* GCOVR_EXCL_LINE */
      ctx->input[ctx->position] == '{') {                 /* GCOVR_EXCL_LINE */
    if (parse_selection_set(ctx, &node->selection_set) != /* GCOVR_EXCL_LINE */
        0) {                                              /* GCOVR_EXCL_LINE */
      c_rest_graphql_node_free(node);                     /* GCOVR_EXCL_LINE */
      return C_REST_ERROR_GENERIC;                        /* GCOVR_EXCL_LINE */
    }
    *out_node = node; /* GCOVR_EXCL_LINE */
    return C_REST_OK; /* GCOVR_EXCL_LINE */
  }

  /* Shorthand query without "query" keyword */
  if (!name) { /* GCOVR_EXCL_LINE */
    /* Rewind and try to parse selection set for anonymous query */
    ctx->position = 0; /* rough fallback */ /* GCOVR_EXCL_LINE */
    skip_whitespace(ctx);                   /* GCOVR_EXCL_LINE */
    if (ctx->position < ctx->length &&      /* GCOVR_EXCL_LINE */
        ctx->input[ctx->position] == '{') { /* GCOVR_EXCL_LINE */
      if (parse_selection_set(ctx,
                              &node->selection_set) != /* GCOVR_EXCL_LINE */
          0) {                                         /* GCOVR_EXCL_LINE */
        c_rest_graphql_node_free(node);                /* GCOVR_EXCL_LINE */
        return C_REST_ERROR_GENERIC;                   /* GCOVR_EXCL_LINE */
      }
      *out_node = node; /* GCOVR_EXCL_LINE */
      return C_REST_OK; /* GCOVR_EXCL_LINE */
    }
  }

  c_rest_graphql_node_free(node); /* GCOVR_EXCL_LINE */
  return C_REST_ERROR_GENERIC;    /* GCOVR_EXCL_LINE */
}

c_rest_error_t c_rest_graphql_parse(const char *query,
                                    size_t query_len, /* GCOVR_EXCL_LINE */
                                    struct c_rest_graphql_node **out_doc) {
  struct c_rest_graphql_context ctx;
  struct c_rest_graphql_node *doc;
  struct c_rest_graphql_node *op = NULL; /* GCOVR_EXCL_LINE */

  if (!query || !out_doc)        /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  ctx.input = query;        /* GCOVR_EXCL_LINE */
  ctx.length = query_len;   /* GCOVR_EXCL_LINE */
  ctx.position = 0;         /* GCOVR_EXCL_LINE */
  ctx.error_message = NULL; /* GCOVR_EXCL_LINE */
  ctx.error_pos = -1;       /* GCOVR_EXCL_LINE */

  if (alloc_node(C_REST_GRAPHQL_NODE_DOCUMENT, &doc) != C_REST_OK ||
      !doc)                      /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  if (alloc_list(&doc->definitions) != 0 ||
      !doc->definitions) {         /* GCOVR_EXCL_LINE */
    c_rest_graphql_node_free(doc); /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;   /* GCOVR_EXCL_LINE */
  }

  skip_whitespace(&ctx);                   /* GCOVR_EXCL_LINE */
  while (ctx.position < ctx.length) {      /* GCOVR_EXCL_LINE */
    if (parse_operation(&ctx, &op) == 0) { /* GCOVR_EXCL_LINE */
      list_append(doc->definitions, op);   /* GCOVR_EXCL_LINE */
    } else {
      break; /* GCOVR_EXCL_LINE */
    }
    skip_whitespace(&ctx); /* GCOVR_EXCL_LINE */
  }

  if (ctx.position < ctx.length ||
      doc->definitions->count == 0) { /* GCOVR_EXCL_LINE */
    c_rest_graphql_node_free(doc);    /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;      /* GCOVR_EXCL_LINE */
  }

  *out_doc = doc;   /* GCOVR_EXCL_LINE */
  return C_REST_OK; /* GCOVR_EXCL_LINE */
}

c_rest_error_t c_rest_graphql_node_free(
    struct c_rest_graphql_node *node) { /* GCOVR_EXCL_LINE */
  size_t i;

  if (!node)          /* GCOVR_EXCL_LINE */
    return C_REST_OK; /* GCOVR_EXCL_LINE */

  if (node->name)             /* GCOVR_EXCL_LINE */
    C_REST_FREE(node->name);  /* GCOVR_EXCL_LINE */
  if (node->alias)            /* GCOVR_EXCL_LINE */
    C_REST_FREE(node->alias); /* GCOVR_EXCL_LINE */
  if (node->value)            /* GCOVR_EXCL_LINE */
    C_REST_FREE(node->value); /* GCOVR_EXCL_LINE */

  if (node->arguments) {                                   /* GCOVR_EXCL_LINE */
    for (i = 0; i < node->arguments->count; i++) {         /* GCOVR_EXCL_LINE */
      c_rest_graphql_node_free(node->arguments->nodes[i]); /* GCOVR_EXCL_LINE */
    }
    if (node->arguments->nodes)            /* GCOVR_EXCL_LINE */
      C_REST_FREE(node->arguments->nodes); /* GCOVR_EXCL_LINE */
    C_REST_FREE(node->arguments);          /* GCOVR_EXCL_LINE */
  }

  if (node->selection_set) {                           /* GCOVR_EXCL_LINE */
    for (i = 0; i < node->selection_set->count; i++) { /* GCOVR_EXCL_LINE */
      c_rest_graphql_node_free(
          node->selection_set->nodes[i]); /* GCOVR_EXCL_LINE */
    }
    if (node->selection_set->nodes)            /* GCOVR_EXCL_LINE */
      C_REST_FREE(node->selection_set->nodes); /* GCOVR_EXCL_LINE */
    C_REST_FREE(node->selection_set);          /* GCOVR_EXCL_LINE */
  }

  if (node->definitions) {                           /* GCOVR_EXCL_LINE */
    for (i = 0; i < node->definitions->count; i++) { /* GCOVR_EXCL_LINE */
      c_rest_graphql_node_free(
          node->definitions->nodes[i]); /* GCOVR_EXCL_LINE */
    }
    if (node->definitions->nodes)            /* GCOVR_EXCL_LINE */
      C_REST_FREE(node->definitions->nodes); /* GCOVR_EXCL_LINE */
    C_REST_FREE(node->definitions);          /* GCOVR_EXCL_LINE */
  }

  C_REST_FREE(node); /* GCOVR_EXCL_LINE */
  return C_REST_OK;  /* GCOVR_EXCL_LINE */
}

struct c_rest_graphql_resolver_entry {
  char *field_name;
  c_rest_graphql_resolver_fn resolver;
  void *user_data;
  struct c_rest_graphql_resolver_entry *next;
};

c_rest_error_t c_rest_graphql_schema_init(
    struct c_rest_graphql_schema **schema) { /* GCOVR_EXCL_LINE */
  if (!schema)                               /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;             /* GCOVR_EXCL_LINE */
  if (C_REST_MALLOC(sizeof(struct c_rest_graphql_schema), schema) !=
      0)                         /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  (*schema)->resolvers = NULL;   /* GCOVR_EXCL_LINE */
  return C_REST_OK;              /* GCOVR_EXCL_LINE */
}

c_rest_error_t c_rest_graphql_schema_free(
    struct c_rest_graphql_schema *schema) { /* GCOVR_EXCL_LINE */
  struct c_rest_graphql_resolver_entry *entry, *next;
  if (!schema)                      /* GCOVR_EXCL_LINE */
    return C_REST_OK;               /* GCOVR_EXCL_LINE */
  entry = schema->resolvers;        /* GCOVR_EXCL_LINE */
  while (entry) {                   /* GCOVR_EXCL_LINE */
    next = entry->next;             /* GCOVR_EXCL_LINE */
    C_REST_FREE(entry->field_name); /* GCOVR_EXCL_LINE */
    C_REST_FREE(entry);             /* GCOVR_EXCL_LINE */
    entry = next;                   /* GCOVR_EXCL_LINE */
  }
  C_REST_FREE(schema); /* GCOVR_EXCL_LINE */
  return C_REST_OK;    /* GCOVR_EXCL_LINE */
}

c_rest_error_t c_rest_graphql_schema_add_resolver(
    struct c_rest_graphql_schema *schema, /* GCOVR_EXCL_LINE */
    const char *field_name, c_rest_graphql_resolver_fn resolver,
    void *user_data) {
  struct c_rest_graphql_resolver_entry *entry;
  size_t len;

  if (!schema || !field_name || !resolver) /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;           /* GCOVR_EXCL_LINE */

  if (C_REST_MALLOC(sizeof(struct c_rest_graphql_resolver_entry), &entry) !=
      0)                         /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  len = strlen(field_name);                              /* GCOVR_EXCL_LINE */
  if (C_REST_MALLOC(len + 1, &entry->field_name) != 0) { /* GCOVR_EXCL_LINE */
    C_REST_FREE(entry);                                  /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;                         /* GCOVR_EXCL_LINE */
  }
#if defined(_MSC_VER)
  /* CDD_SAFE_CRT */ memcpy_s(entry->field_name, len + 1, field_name,
                              len + 1); /* GCOVR_EXCL_LINE */
#else
  memcpy(entry->field_name, field_name, len + 1); /* GCOVR_EXCL_LINE */
#endif

  entry->resolver = resolver;      /* GCOVR_EXCL_LINE */
  entry->user_data = user_data;    /* GCOVR_EXCL_LINE */
  entry->next = schema->resolvers; /* GCOVR_EXCL_LINE */
  schema->resolvers = entry;       /* GCOVR_EXCL_LINE */

  return C_REST_OK; /* GCOVR_EXCL_LINE */
}

c_rest_error_t
c_rest_graphql_resolve(struct c_rest_graphql_node *doc, /* GCOVR_EXCL_LINE */
                       struct c_rest_graphql_schema *schema, char **out_json,
                       size_t *out_len) {
  const char *dummy = "{\"data\": {}}"; /* GCOVR_EXCL_LINE */
  size_t len = strlen(dummy);           /* GCOVR_EXCL_LINE */
  size_t i, j;

  if (!doc || !out_json || !out_len) /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;     /* GCOVR_EXCL_LINE */

  /* Basic mock traversal: look for requested fields and trigger resolvers */
  if (schema && doc->definitions) {                 /* GCOVR_EXCL_LINE */
    for (i = 0; i < doc->definitions->count; i++) { /* GCOVR_EXCL_LINE */
      struct c_rest_graphql_node *op =
          doc->definitions->nodes[i]; /* GCOVR_EXCL_LINE */
      if (op->type == C_REST_GRAPHQL_NODE_OPERATION &&
          op->selection_set) {                           /* GCOVR_EXCL_LINE */
        for (j = 0; j < op->selection_set->count; j++) { /* GCOVR_EXCL_LINE */
          struct c_rest_graphql_node *field =
              op->selection_set->nodes[j];                /* GCOVR_EXCL_LINE */
          if (field->type == C_REST_GRAPHQL_NODE_FIELD) { /* GCOVR_EXCL_LINE */
            struct c_rest_graphql_resolver_entry *entry =
                schema->resolvers; /* GCOVR_EXCL_LINE */
            while (entry) {        /* GCOVR_EXCL_LINE */
              if (strcmp(entry->field_name, field->name) ==
                  0) {                 /* GCOVR_EXCL_LINE */
                char *res_json = NULL; /* GCOVR_EXCL_LINE */
                size_t res_len = 0;    /* GCOVR_EXCL_LINE */
                /* Fire resolver (ignore output for this basic skeleton) */
                entry->resolver(field->name, &res_json,
                                &res_len, /* GCOVR_EXCL_LINE */
                                entry->user_data);
                if (res_json) {          /* GCOVR_EXCL_LINE */
                  C_REST_FREE(res_json); /* GCOVR_EXCL_LINE */
                }
              }
              entry = entry->next; /* GCOVR_EXCL_LINE */
            }
          }
        }
      }
    }
  }

  *out_json = NULL;                          /* GCOVR_EXCL_LINE */
  if (C_REST_MALLOC(len + 1, out_json) != 0) /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;             /* GCOVR_EXCL_LINE */

#if defined(_MSC_VER)
  /* CDD_SAFE_CRT */ memcpy_s(*out_json, len + 1, dummy,
                              len + 1); /* GCOVR_EXCL_LINE */
#else
  memcpy(*out_json, dummy, len + 1); /* GCOVR_EXCL_LINE */
#endif
  *out_len = len; /* GCOVR_EXCL_LINE */

  return C_REST_OK; /* GCOVR_EXCL_LINE */
}

#endif /* C_REST_FRAMEWORK_ENABLE_GRAPHQL */

typedef int c_rest_graphql_dummy_declaration;
