/* clang-format off */
#include "c_rest_router.h"
#include "c_rest_modality.h"
#include "c_orm_db.h"
#include "c_orm_sqlite.h"
#include "c_orm_oauth2.h"
#include "oauth2_server.h"
#include "c_rest_openapi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
/* clang-format on */

static struct c_rest_context *g_ctx = NULL;

static void handle_sigint(int sig) {
  (void)sig;
  if (g_ctx) {
    printf("\nCaught SIGINT! Shutting down gracefully...\n");
    c_rest_stop(g_ctx);
  }
}

int main(int argc, char **argv) {
  struct c_rest_context *ctx = NULL;
  c_rest_router *router = NULL;
  c_orm_db_t *db = NULL;
  enum c_rest_modality_type modality;
  struct c_rest_tls_context *tls_ctx = NULL;

  const char *db_url = ":memory:";
  const char *listen_addr = "0.0.0.0";
  unsigned short listen_port = 8080;
  const char *tls_cert = NULL;
  const char *tls_key = NULL;

  int i;
  int run_res;

  /* 1. Read environment variables first */
  const char *env_db_url = getenv("OAUTH2_DB_URL");
  const char *env_listen_addr = getenv("OAUTH2_LISTEN_ADDR");
  const char *env_listen_port = getenv("OAUTH2_LISTEN_PORT");
  const char *env_tls_cert = getenv("OAUTH2_TLS_CERT");
  const char *env_tls_key = getenv("OAUTH2_TLS_KEY");

  if (env_db_url)
    db_url = env_db_url;
  if (env_listen_addr)
    listen_addr = env_listen_addr;
  if (env_listen_port)
    listen_port = (unsigned short)atoi(env_listen_port);
  if (env_tls_cert)
    tls_cert = env_tls_cert;
  if (env_tls_key)
    tls_key = env_tls_key;

  /* 2. Parse command line arguments (override env vars) */
  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0 ||
        strcmp(argv[i], "-?") == 0 || strcmp(argv[i], "/?") == 0) {
      printf("Usage: %s [OPTIONS]\n", argv[0]);
      printf("Options:\n");
      printf("  --db-url <url>        Database URL (default: :memory:)\n");
      printf("  --listen-addr <addr>  Listen address (default: 0.0.0.0)\n");
      printf("  --listen-port <port>  Listen port (default: 8080)\n");
      printf("  --tls-cert <path>     TLS certificate path\n");
      printf("  --tls-key <path>      TLS key path\n");
      printf("  --help, -h, -?, /?    Show this help message\n");
      return 0;
    } else if (strcmp(argv[i], "--db-url") == 0 && i + 1 < argc) {
      db_url = argv[++i];
    } else if (strcmp(argv[i], "--listen-addr") == 0 && i + 1 < argc) {
      listen_addr = argv[++i];
    } else if (strcmp(argv[i], "--listen-port") == 0 && i + 1 < argc) {
      listen_port = (unsigned short)atoi(argv[++i]);
    } else if (strcmp(argv[i], "--tls-cert") == 0 && i + 1 < argc) {
      tls_cert = argv[++i];
    } else if (strcmp(argv[i], "--tls-key") == 0 && i + 1 < argc) {
      tls_key = argv[++i];
    }
  }

  printf("Initializing OAuth2 Server...\n");

#ifdef OAUTH2_MULTI_THREADED
  modality = C_REST_MODALITY_MULTI_THREAD;
#else
  modality = C_REST_MODALITY_SINGLE_THREAD;
#endif

  if (c_rest_init(modality, &ctx) != 0) {
    printf("Failed to init rest context\n");
    return 1;
  }

  /* Set configuration on context */
  ctx->listen_address = listen_addr;
  ctx->listen_port = listen_port;

  /* Optional TLS configuration */
  if (tls_cert && tls_key) {
    if (c_rest_tls_init() == 0 && c_rest_tls_context_init(&tls_ctx) == 0) {
      if (c_rest_tls_load_cert(tls_ctx, tls_cert) == 0 &&
          c_rest_tls_load_key(tls_ctx, tls_key) == 0) {
        ctx->tls_ctx = tls_ctx;
      } else {
        printf("Failed to load TLS cert/key.\n");
        c_rest_tls_context_destroy(tls_ctx);
        tls_ctx = NULL;
      }
    } else {
      printf("Failed to init TLS context.\n");
    }
  }

  if (c_rest_router_init(&router) != 0) {
    printf("Failed to init router\n");
    if (tls_ctx)
      c_rest_tls_context_destroy(tls_ctx);
    c_rest_destroy(ctx);
    return 1;
  }

  /* Notice we only support SQLite for this example currently, but we pass
   * db_url */
  if (c_orm_sqlite_connect(db_url, &db) != 0) {
    printf("Failed to connect to SQLite\n");
    c_rest_router_destroy(router);
    if (tls_ctx)
      c_rest_tls_context_destroy(tls_ctx);
    c_rest_destroy(ctx);
    return 1;
  }

  /* Create tables (for mock setup) */
  c_orm_oauth2_create_tables(db);

  if (oauth2_server_init(router, db) != 0) {
    printf("Failed to init oauth2 server\n");
    c_rest_router_destroy(router);
    if (tls_ctx)
      c_rest_tls_context_destroy(tls_ctx);
    c_rest_destroy(ctx);
    return 1;
  }

  c_rest_enable_openapi(router, "/api/v0/openapi.json");
  c_rest_enable_swagger_ui(router, "/api/v0/docs", "/api/v0/openapi.json");

  c_rest_set_router(ctx, router);

  g_ctx = ctx;
  signal(SIGINT, handle_sigint);
  signal(SIGTERM, handle_sigint);

  printf("Listening on %s:%d (HTTPS: %s)\n", listen_addr, listen_port,
         ctx->tls_ctx ? "yes" : "no");
  printf("Connecting to database: %s\n", db_url);
  printf("Starting server. Endpoint: POST /api/v0/oauth/token\n");

  run_res = c_rest_run(ctx);
  if (run_res != 0) {
    fprintf(stderr, "Server failed to run (error code: %d)\n", run_res);
  }

  c_rest_router_destroy(router);
  if (ctx->tls_ctx) {
    c_rest_tls_context_destroy(ctx->tls_ctx);
  }
  c_rest_destroy(ctx);

  return run_res != 0 ? 1 : 0;
}
