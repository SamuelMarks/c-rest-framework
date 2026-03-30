/* clang-format off */
#include "c_rest_graphql.h"
#include "c_rest_router.h"
#include "c_rest_modality.h"
#include "c_rest_middleware.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <winsock2.h>
#else
#include <signal.h>
#endif
/* clang-format on */

static void logger_cb(const char *msg) { printf("LOG: %s\n", msg); }

static int resolve_user(const char *field_name, char **out_json,
                        size_t *out_len, void *user_data) {
  const char *res = "{\"id\": 1, \"name\": \"Alice\", \"role\": \"Admin\"}";
  size_t len = strlen(res);

  (void)field_name;
  (void)user_data;

  *out_json = (char *)malloc(len + 1);
  if (!*out_json)
    return -1;

  memcpy(*out_json, res, len + 1);
  *out_len = len;
  return 0;
}

static int resolve_posts(const char *field_name, char **out_json,
                         size_t *out_len, void *user_data) {
  const char *res = "[{\"id\": 101, \"title\": \"Hello GraphQL\"}, "
                    "{\"id\": 102, \"title\": \"C89 is strictly awesome\"}]";
  size_t len = strlen(res);

  (void)field_name;
  (void)user_data;

  *out_json = (char *)malloc(len + 1);
  if (!*out_json)
    return -1;

  memcpy(*out_json, res, len + 1);
  *out_len = len;
  return 0;
}

int main(void) {
  struct c_rest_context *ctx = NULL;
  struct c_rest_router *router = NULL;
  struct c_rest_graphql_schema *schema = NULL;
  int res;

#if defined(_WIN32)
  WSADATA wsa;
  WSAStartup(MAKEWORD(2, 2), &wsa);
#else
  signal(SIGPIPE, SIG_IGN);
#endif

  printf("Initializing modality context...\n");
  res = c_rest_init(C_REST_MODALITY_SYNC, &ctx);
  if (res != 0) {
    printf("Failed to init modality context\n");
    return 1;
  }

  ctx->logger.log_cb = logger_cb;
  ctx->listen_port = 8080;

  printf("Initializing router...\n");
  if (c_rest_router_init(&router) != 0) {
    printf("Failed to init router\n");
    return 1;
  }

  c_rest_router_use(router, NULL, c_rest_cors_middleware, NULL);

  /* Set up GraphQL schema */
  printf("Setting up GraphQL schema...\n");
  if (c_rest_graphql_schema_init(&schema) != 0) {
    printf("Failed to initialize GraphQL schema\n");
    return 1;
  }

  c_rest_graphql_schema_add_resolver(schema, "user", resolve_user, NULL);
  c_rest_graphql_schema_add_resolver(schema, "posts", resolve_posts, NULL);

  /* Register GraphQL route */
  c_rest_router_add_graphql(router, "/graphql", schema);

  printf("Starting server on http://localhost:8080/graphql\n");
  printf("Try sending a POST request to /graphql with a body like:\n");
  printf("  query { user { id } }\n");

  ctx->internal_state = router;
  res = c_rest_run(ctx);
  if (res != 0) {
    printf("Server failed to start or crashed.\n");
  }

  c_rest_graphql_schema_free(schema);
  c_rest_router_destroy(router);
  c_rest_destroy(ctx);

#if defined(_WIN32)
  WSACleanup();
#endif

  return 0;
}