/* clang-format off */
#ifndef C_REST_GRAPHQL_H
#define C_REST_GRAPHQL_H

#ifdef C_REST_FRAMEWORK_ENABLE_GRAPHQL

#include <stddef.h>

/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file c_rest_graphql.h
 * @brief GraphQL API Parsing and Resolving definitions.
 */

/**
 * @brief Enum for GraphQL AST node types.
 */
enum c_rest_graphql_node_type {
  C_REST_GRAPHQL_NODE_DOCUMENT,
  C_REST_GRAPHQL_NODE_OPERATION,
  C_REST_GRAPHQL_NODE_SELECTION_SET,
  C_REST_GRAPHQL_NODE_FIELD,
  C_REST_GRAPHQL_NODE_ARGUMENT,
  C_REST_GRAPHQL_NODE_VALUE
};

/**
 * @brief Enum for GraphQL operation types.
 */
enum c_rest_graphql_operation_type {
  C_REST_GRAPHQL_OP_QUERY,
  C_REST_GRAPHQL_OP_MUTATION,
  C_REST_GRAPHQL_OP_SUBSCRIPTION
};

struct c_rest_graphql_node;

/**
 * @brief Structure representing a GraphQL list of nodes.
 */
struct c_rest_graphql_node_list {
  struct c_rest_graphql_node **nodes;
  size_t count;
  size_t capacity;
};

/**
 * @brief Structure representing a GraphQL AST node.
 */
struct c_rest_graphql_node {
  enum c_rest_graphql_node_type type;

  /* For operations / fields / arguments */
  char *name;

  /* For fields */
  char *alias;

  /* For values */
  char *value;

  /* For operation */
  enum c_rest_graphql_operation_type op_type;

  /* Children / nested items */
  struct c_rest_graphql_node_list *arguments;
  struct c_rest_graphql_node_list *selection_set;
  struct c_rest_graphql_node_list *definitions;
};

/**
 * @brief GraphQL parsing context.
 */
struct c_rest_graphql_context {
  const char *input;
  size_t position;
  size_t length;
  char *error_message;
  int error_pos;
};

/**
 * @brief Parse a GraphQL query string into an AST.
 * @param query The input GraphQL query string.
 * @param query_len The length of the query string.
 * @param out_doc Pointer to store the resulting Document AST node.
 * @return 0 on success, non-zero on error.
 */
int c_rest_graphql_parse(const char *query, size_t query_len,
                         struct c_rest_graphql_node **out_doc);

/**
 * @brief Free a parsed GraphQL AST document.
 * @param doc The Document node to free.
 * @return 0 on success.
 */
int c_rest_graphql_node_free(struct c_rest_graphql_node *doc);

/**
 * @brief Dummy resolver function prototype.
 * @param field_name The field being resolved.
 * @param out_json Result string formatted as JSON.
 * @param out_len Length of the result string.
 * @return 0 on success.
 */
typedef int (*c_rest_graphql_resolver_fn)(const char *field_name,
                                          char **out_json, size_t *out_len,
                                          void *user_data);

struct c_rest_graphql_resolver_entry;

/**
 * @brief Schema containing resolver functions.
 */
struct c_rest_graphql_schema {
  struct c_rest_graphql_resolver_entry *resolvers;
};

/**
 * @brief Initializes a GraphQL schema.
 * @param schema Pointer to store the schema.
 * @return 0 on success.
 */
int c_rest_graphql_schema_init(struct c_rest_graphql_schema **schema);

/**
 * @brief Frees a GraphQL schema.
 * @param schema The schema to free.
 * @return 0 on success.
 */
int c_rest_graphql_schema_free(struct c_rest_graphql_schema *schema);

/**
 * @brief Register a resolver function for a specific field name.
 * @param schema The schema.
 * @param field_name The field name to resolve.
 * @param resolver The resolver function.
 * @param user_data User data passed to the resolver.
 * @return 0 on success.
 */
int c_rest_graphql_schema_add_resolver(struct c_rest_graphql_schema *schema,
                                       const char *field_name,
                                       c_rest_graphql_resolver_fn resolver,
                                       void *user_data);

/**
 * @brief Resolve a parsed GraphQL AST using the given schema.
 * @param doc The parsed Document AST node.
 * @param schema The schema containing resolvers.
 * @param out_json Result string formatted as JSON.
 * @param out_len Length of the result string.
 * @return 0 on success.
 */
int c_rest_graphql_resolve(struct c_rest_graphql_node *doc,
                           struct c_rest_graphql_schema *schema,
                           char **out_json, size_t *out_len);

/* __cplusplus */

#endif /* C_REST_FRAMEWORK_ENABLE_GRAPHQL */

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* C_REST_GRAPHQL_H */
