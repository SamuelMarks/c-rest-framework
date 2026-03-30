# `c-rest-framework` Usage Guide

This document details the configuration options, build system flags, and core enums available in the `c-rest-framework` project. The framework uses a **modality-driven architecture**, allowing developers to swap the execution paradigm (e.g., synchronous vs. asynchronous, single-threaded vs. multithreaded) without modifying the routing and request-handling API.

## 1. CMake Configuration Options

The build system exposes several toggles to configure CRT linkage, character sets, sanitizers, and cryptographic backends.

### Windows & CRT Toggles
- **`C_REST_FRAMEWORK_STATIC_CRT`** (`OFF` by default): Use the Static C Runtime Library (`/MT`, `/MTd`). Recommended when you want to avoid redistributing the Visual C++ Redistributable.
- **`C_REST_FRAMEWORK_SHARED_CRT`** (`ON` by default): Use the Shared C Runtime Library (`/MD`, `/MDd`).
- **`C_REST_FRAMEWORK_UNICODE`** (`OFF` by default): Compile with `UNICODE` and `_UNICODE` defined (Windows only).
- **`C_REST_FRAMEWORK_ANSI`** (`OFF` by default): Compile with ANSI (Multi-Byte) character set.

### Cryptography Backends
You can choose a specific backend for TLS/SSL and cryptographic hashing. (Only one backend should be enabled at a time).
- **`C_REST_FRAMEWORK_USE_MBEDTLS`**: Enable `mbedtls` integration.
- **`C_REST_FRAMEWORK_USE_OPENSSL`**: Enable `OpenSSL` integration.
- **`C_REST_FRAMEWORK_USE_LIBRESSL`**: Enable `LibreSSL` integration.
- **`C_REST_FRAMEWORK_USE_BORINGSSL`**: Enable `BoringSSL` integration.
- **`C_REST_FRAMEWORK_USE_WOLFSSL`**: Enable `wolfSSL` integration.
- **`C_REST_FRAMEWORK_USE_S2N`**: Enable `s2n-tls` integration.

### Quality Assurance & Testing
- **`C_REST_FRAMEWORK_USE_ASAN`** (`OFF` by default): Enable GCC/Clang AddressSanitizer (`-fsanitize=address`) for memory error detection.
- **`C_REST_FRAMEWORK_USE_TSAN`** (`OFF` by default): Enable GCC/Clang ThreadSanitizer (`-fsanitize=thread`) for data race detection.

### Dependencies & Integration
- **`C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION`** (`OFF` by default): Enable integration with `c-multiplatform` for event-driven asynchronous reactor execution.
- **`C_REST_FRAMEWORK_FETCH_CAH`** (`OFF` by default): Automatically fetch `c-abstract-http` from GitHub during the CMake configure step (useful if not using `vcpkg`).
- **`C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP_BROTLI`** (`ON` by default): Enable Response Compression (Gzip and Brotli) capabilities via zlib and brotli.

## 2. Response Compression (Gzip / Brotli)

When compiled with `C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP_BROTLI=ON`, you can compress response data seamlessly in memory or stream it.

```c
#include "c_rest_compression.h"
#include <stdio.h>

void compress_example(void) {
    const unsigned char *data = (const unsigned char *)"Hello, world!";
    size_t in_len = 13;
    unsigned char *comp_data = NULL;
    size_t comp_len = 0;

    /* Perform one-shot buffer compression */
    if (c_rest_compress_buffer(C_REST_COMPRESSION_GZIP, data, in_len, &comp_data, &comp_len) == 0) {
        printf("Compressed size: %zu\n", comp_len);
        /* Make sure to free the allocated compressed buffer */
        c_rest_mem_free(comp_data);
    }
}
```
- **`C_REST_FRAMEWORK_FETCH_C_ORM`** (`OFF` by default): Automatically fetch `c-orm` from GitHub.
- **`TRY_SYS_LIB`** (`ON` by default): If dependencies are not found via `vcpkg`, fallback to searching system-installed packages.

---

## 2. Framework Execution Modalities

When initializing the framework via `c_rest_init()`, you must pass an execution modality defined in `enum c_rest_modality_type`. This instructs the core engine on how to handle incoming network traffic and process requests.

```c
#include "c_rest_modality.h"

enum c_rest_modality_type {
  C_REST_MODALITY_SYNC,           /**< Synchronous blocking operations */
  C_REST_MODALITY_ASYNC,          /**< Asynchronous non-blocking event loop */
  C_REST_MODALITY_SINGLE_THREAD,  /**< Execute strictly on a single thread */
  C_REST_MODALITY_MULTI_THREAD,   /**< Utilize a thread pool for requests */
  C_REST_MODALITY_SINGLE_PROCESS, /**< Execute strictly in a single process */
  C_REST_MODALITY_MULTI_PROCESS,  /**< Pre-fork worker processes */
  C_REST_MODALITY_GREENTHREAD,    /**< Coroutine-based apparent concurrency */
  C_REST_MODALITY_MESSAGE_PASSING /**< Actor-like message queue processing */
};
```

### Modality Highlights
- **`C_REST_MODALITY_SYNC`**: The simplest model; handles one request at a time, blocking until completion. Ideal for DOS or highly constrained embedded systems.
- **`C_REST_MODALITY_ASYNC`**: A Node.js-style event loop. Single-threaded but concurrent, capable of handling thousands of idle connections.
- **`C_REST_MODALITY_MULTI_THREAD`**: Spawns a pool of worker threads to process CPU-bound requests simultaneously.
- **`C_REST_MODALITY_MULTI_PROCESS`**: A pre-forking architecture (similar to Apache MPM prefork). Maximizes fault isolation between requests.

---

## 3. Cryptographic Providers

If you need to dynamically check which cryptography provider the framework was compiled against, use the `c_rest_tls_get_provider()` function.

```c
#include "c_rest_tls.h"

enum c_rest_crypto_provider {
  C_REST_CRYPTO_NONE = 0,     /**< No TLS/SSL support, using custom fallback hashes */
  C_REST_CRYPTO_OPENSSL,      /**< OpenSSL backend */
  C_REST_CRYPTO_LIBRESSL,     /**< LibreSSL backend */
  C_REST_CRYPTO_BORINGSSL,    /**< BoringSSL backend */
  C_REST_CRYPTO_MBEDTLS,      /**< mbedtls backend */
  C_REST_CRYPTO_WOLFSSL,      /**< wolfSSL backend */
  C_REST_CRYPTO_S2N           /**< AWS s2n-tls backend */
};
```

This returns an instance of `enum c_rest_crypto_provider`, corresponding directly to the `C_REST_FRAMEWORK_USE_*` CMake toggles passed during build time.

---

## 4. Bootstrapping the Server & Router

The typical lifecycle involves initializing the context, configuring the database (optional), setting up TLS (optional), binding routes, and calling `c_rest_run`.

```c
#include "c_rest_modality.h"
#include "c_rest_router.h"
#include "c_rest_tls.h"
#include "c_rest_response.h"

static void hello_handler(struct c_rest_request *req, struct c_rest_response *res, void *user_data) {
  c_rest_response_html(res, "<h1>Hello World</h1>");
}

int main(void) {
  struct c_rest_context *ctx = NULL;
  c_rest_router *router = NULL;

  /* 1. Init Framework */
  c_rest_init(C_REST_MODALITY_SYNC, &ctx);

  /* 2. Configure ORM (Optional) */
  ctx->db_config.connection_string = "sqlite://app.db";

  /* 3. Configure TLS (Optional) */
  if (c_rest_tls_init() == 0) {
    struct c_rest_tls_context *tls_ctx;
    if (c_rest_tls_context_init(&tls_ctx) == 0) {
      c_rest_tls_load_cert(tls_ctx, "server.crt");
      c_rest_tls_load_key(tls_ctx, "server.key");
      ctx->tls_ctx = tls_ctx;
    }
  }

  /* 4. Init Router & Add Routes */
  c_rest_router_init(&router);
  c_rest_router_add(router, "GET", "/", hello_handler, NULL);
  
  /* 5. Start Server */
  ctx->internal_state = router;
  c_rest_run(ctx);

  /* 6. Cleanup */
  c_rest_router_destroy(router);
  if (ctx->tls_ctx) c_rest_tls_context_destroy(ctx->tls_ctx);
  c_rest_destroy(ctx);
  return 0;
}
```

## 5. Working with c-orm

To leverage built-in REST generation for `c-orm` database models, define a `c_rest_orm_model` and pass it to the CRUD handlers.

```c
#include "c_rest_orm_crud.h"
#include "c_rest_orm_middleware.h"

struct c_rest_orm_model user_model = { "users", "id" };

void setup_user_routes(c_rest_router *router, struct c_rest_context *ctx) {
  /* Enforce transaction wrappers on /api/v0 routes */
  c_rest_router_use(router, "/api/v0", c_rest_orm_transaction_start_middleware, ctx);
  c_rest_router_use_post(router, "/api/v0", c_rest_orm_transaction_end_middleware, ctx);

  /* Auto-generate REST APIs */
  c_rest_router_add(router, "GET", "/api/v0/users", c_rest_orm_crud_get_list, &user_model);
  c_rest_router_add(router, "POST", "/api/v0/users", c_rest_orm_crud_create, &user_model);
  c_rest_router_add(router, "GET", "/api/v0/users/:id", c_rest_orm_crud_get_one, &user_model);
  c_rest_router_add(router, "PUT", "/api/v0/users/:id", c_rest_orm_crud_update, &user_model);
  c_rest_router_add(router, "DELETE", "/api/v0/users/:id", c_rest_orm_crud_delete, &user_model);
}
```

## 6. Serving OpenAPI and Swagger UI

The framework has built-in support for generating and serving `openapi.json` specifications, as well as a fully interactive Swagger UI documentation portal.

To use this feature, add `C_REST_FRAMEWORK_FETCH_CDD_C=ON` during your CMake configure step.

```c
#include "c_rest_openapi.h"
#include "c_rest_router.h"

void setup_openapi(c_rest_router *router) {
  struct c_rest_openapi_operation op;
  struct c_rest_openapi_spec *spec;
  const char *tags[] = {"Users"};

  /* 1. Define operation metadata */
  memset(&op, 0, sizeof(op));
  op.summary = "Get Users";
  op.description = "Retrieves a list of users.";
  op.tags = tags;
  op.n_tags = 1;
  op.res_body_schema.ref_name = "UserResponse";

  /* 2. Bind route with OpenAPI metadata */
  c_rest_router_add_openapi(router, "GET", "/api/v0/users", c_rest_orm_crud_get_list, NULL, &op);

  /* 3. Register components/schemas (often generated by cdd-c) */
  c_rest_router_get_openapi_spec(router, &spec);
  if (spec) {
    c_rest_openapi_spec_add_component_schema(spec, "UserResponse", 
      "{\"type\": \"object\", \"properties\": {\"id\": {\"type\": \"integer\"}}}");
  }

  /* 4. Enable endpoints */
  c_rest_enable_openapi(router, "/api/v0/openapi.json");
  c_rest_enable_swagger_ui(router, "/api/v0/docs", "/api/v0/openapi.json");
}
```

## 7. WebSockets (RFC 6455)

The framework provides native support for upgrading HTTP requests to WebSocket connections and handling raw frames (text or binary). This feature handles the required SHA-1/Base64 handshake securely and transparently.

```c
#include "c_rest_router.h"
#include "c_rest_websocket.h"

/* Callback triggered when a new frame is received */
static int my_ws_on_message(struct c_rest_request *req,
                            const unsigned char *payload, size_t payload_len,
                            int is_binary, void *user_data) {
  if (is_binary) {
      printf("Received binary frame of size: %u\n", (unsigned int)payload_len);
  } else {
      printf("Received text message: %.*s\n", (int)payload_len, payload);
  }
  return 0;
}

/* Callback triggered when the WebSocket is closed */
static void my_ws_on_close(struct c_rest_request *req, int status_code,
                           void *user_data) {
  printf("WebSocket closed with status: %d\n", status_code);
}

void setup_websocket(c_rest_router *router) {
  /* Register the WebSocket endpoint */
  c_rest_router_add_websocket(router, "/ws", my_ws_on_message, my_ws_on_close, NULL);
}
```

To enable this feature, build with `-DC_REST_ENABLE_WEBSOCKETS=ON` (enabled by default).

## 8. GraphQL API Parsing & Resolving

The framework provides an optional module for parsing GraphQL queries and mapping resolvers directly to endpoints. The module translates string queries into an AST tree and traverses it against a user-defined schema.        

```c
#include "c_rest_graphql.h"
#include "c_rest_router.h"

/* Define a resolver function */
static int resolve_user(const char *field_name, char **out_json,
                        size_t *out_len, void *user_data) {
  const char *res = "{\"id\": 1, \"name\": \"Alice\"}";
  size_t len = strlen(res);

  *out_json = (char *)malloc(len + 1);
  memcpy(*out_json, res, len + 1);
  *out_len = len;
  return 0;
}

void setup_graphql(c_rest_router *router) {
  struct c_rest_graphql_schema *schema = NULL;

  /* Initialize the schema */
  c_rest_graphql_schema_init(&schema);

  /* Register resolvers */
  c_rest_graphql_schema_add_resolver(schema, "user", resolve_user, NULL);

  /* Bind the schema to a POST endpoint */
  c_rest_router_add_graphql(router, "/graphql", schema);

  /* The router retains the schema reference; ensure you free it during cleanup:
     c_rest_graphql_schema_free(schema); */
}
```

To enable this feature, build with `-DC_REST_FRAMEWORK_ENABLE_GRAPHQL=ON` (enabled by default).

## 9. Server-Sent Events (SSE)

The framework supports streaming real-time data to clients using Server-Sent Events (SSE). It abstracts the underlying chunked HTTP response and provides a simple API to initialize an SSE stream and send structured events.

```c
#include "c_rest_router.h"
#include "c_rest_sse.h"
#include "c_rest_response.h"

/* Define your SSE handler */
static int my_sse_handler(struct c_rest_request *req,
                          struct c_rest_response *res,
                          void *user_data) {
  struct c_rest_sse_event ev;
  int i;

  /* The router automatically calls c_rest_sse_init_response(res) before calling this handler,
     so the connection is already upgraded to an event-stream. */

  for (i = 0; i < 5; i++) {
    c_rest_sse_event_init(&ev);
    ev.event = "ping";
    ev.data = "Hello from SSE!";
    ev.id = "1";

    /* Send the event to the client */
    c_rest_sse_send_event(res, &ev);

    /* Optional: Send keep-alive to prevent proxies from closing the connection */
    c_rest_sse_send_keepalive(res);

    /* Simulate a delay (e.g. sleep or wait for an async event) */
    /* sleep(1); */
  }

  /* Returning from the handler will close the connection cleanly */
  return 0;
}

void setup_sse(c_rest_router *router) {
  /* Register the SSE endpoint */
  c_rest_router_add_sse(router, "/events", my_sse_handler, NULL);
}
```

To enable this feature, build with `-DC_REST_ENABLE_SERVER_SENT_EVENTS_SSE=ON` (enabled by default).


## Opaque Bearer Tokens (OAuth2 Password Grant)

If you prefer a simpler RFC 6749 opaque token strategy instead of JWTs, the framework provides `c_rest_oauth2_middleware` out-of-the-box. This pairs perfectly with `c-orm` to issue tokens via the password grant flow and validate them.

```c
#include "c_rest_middleware.h"

/* 1. Define your bearer token verification callback */
static int verify_opaque_token(const char *token, void **out_auth_context) {
  /* Look up the opaque token in your database/cache (e.g. using c-orm) */
  if (strcmp(token, "access_token_demo_123") == 0) {
    /* Token is valid! Set context. */
    *out_auth_context = (void *)1;
    return 0; /* Success */
  }
  return 1; /* Invalid token */
}

void setup_opaque_auth(c_rest_router *router) {
  /* 2. Register the middleware. It extracts 'Authorization: Bearer <token>' 
        and invokes your callback. */
  c_rest_router_use(router, "/api/protected", c_rest_oauth2_middleware, verify_opaque_token);
}
```

*Note:* For a complete, production-ready OAuth2 Password Grant Flow implementation (including `POST /oauth/token` handling, securely hashing passwords, and saving tokens using `c-orm`), please see the included `examples/oauth2` application directory.

## JWT (JSON Web Tokens) Authentication Middleware

The framework provides an easy-to-use middleware for authenticating requests using JWT Bearer tokens. The middleware verifies the HS256 signature and allows you to validate the payload and set user context securely.

```c
#include "c_rest_jwt_middleware.h"

/* 1. Define your custom payload verification callback */
static int verify_my_jwt_payload(const char *payload, void **out_auth_context) {
  /* In a real app, parse the JSON payload using parson or your preferred JSON library */
  if (strstr(payload, "\"sub\":\"user123\"") != NULL) {
    /* Token belongs to user123. Store the user struct pointer or ID in out_auth_context */
    *out_auth_context = (void *)1; 
    return 0; /* Success */
  }
  return 1; /* Invalid payload */
}

/* 2. Configure the middleware */
struct c_rest_jwt_middleware_config jwt_config;
const unsigned char secret[] = "my_super_secret_key";
c_rest_jwt_middleware_config_init(&jwt_config, secret, sizeof(secret) - 1, verify_my_jwt_payload);

/* 3. Register the middleware on protected routes */
c_rest_router_use(router, "/api/protected", c_rest_jwt_middleware, &jwt_config);

/* 4. Access auth_context in your route handler */
static int my_protected_route(struct c_rest_request *req,
                              struct c_rest_response *res,
                              void *user_data) {
  /* req->auth_context will contain the value set by your verify callback */
  if (req->auth_context == (void *)1) {
    c_rest_response_html(res, "Welcome back, user123!");
  } else {
    /* If the middleware succeeds, auth_context should be set, but you can still double check */
    c_rest_response_html(res, "Welcome back!");
  }
  return 0;
}
```

## Server-Side Template Engine

The `c_rest_template` module allows simple, robust `{{key}}` HTML template rendering with zero external dependencies. It works beautifully across all framework modalities.

```c
#include "c_rest_router.h"
#include "c_rest_template.h"

/* Define a data provider to map your dynamic values */
static int provide_data(struct c_rest_request *req,
                        const char ***out_keys,
                        const char ***out_values,
                        size_t *out_count,
                        void *user_data) {
    static const char *keys[] = {"name", "role"};
    static const char *values[] = {"Alice", "Admin"};

    *out_keys = keys;
    *out_values = values;
    *out_count = 2;
    return 0;
}

int main(void) {
    c_rest_router *router = NULL;
    struct c_rest_template_context tpl;

    c_rest_router_init(&router);
    c_rest_template_init(&tpl, "Hello {{name}}, your role is {{role}}!");

    /* Register the route - the framework automatically maps and renders */
    c_rest_router_add_template(router, "GET", "/profile", &tpl, provide_data, NULL);

    /* Clean up */
    c_rest_template_destroy(&tpl);
    c_rest_router_destroy(router);
    return 0;
}
```



### 4. Full Multipart Form Streaming

c-rest-framework natively supports parsing large multipart/form-data uploads using an iterative, state-machine-driven streaming parser. This avoids loading large files into memory.

`c
#include "c_rest_multipart.h"
#include <stdio.h>

static int on_part_begin(c_rest_multipart_parser *parser) {
  printf("Part begin\n");
  return 0;
}

static int on_part_data(c_rest_multipart_parser *parser, const char *at, size_t length) {
  printf("Part data: %.*s\n", (int)length, at);
  return 0;
}

/* Inside your request handler / middleware: */
c_rest_multipart_parser *parser = NULL;
struct c_rest_multipart_callbacks callbacks = {0};
callbacks.on_part_begin = on_part_begin;
callbacks.on_part_data = on_part_data;

c_rest_multipart_parser_init(&parser, "my-boundary", &callbacks, NULL);

/* As data chunks arrive from the network: */
size_t parsed = 0;
c_rest_multipart_parser_execute(parser, chunk, chunk_len, &parsed);

/* When done: */
c_rest_multipart_parser_destroy(parser);
`


## Hot-Reloading / Auto-Restart API

When c-rest-framework is compiled with hot-reloading support, you can watch files and notify clients of changes.

```c
#include "c_rest_hot_reload.h"

int on_file_changed(void *user_data) {
  printf("File changed! Triggering reload...\\n");
  /* e.g., restart application, flush caches */
  return 0;
}

/* Initialize the hot reload context */
struct c_rest_hot_reload_ctx *ctx = NULL;
c_rest_hot_reload_init(&ctx, NULL);

/* Watch directories or files */
c_rest_hot_reload_add_watch(ctx, "./public");
c_rest_hot_reload_add_watch(ctx, "./templates");

/* Register an SSE endpoint to push reload events to clients (e.g. browser) */
c_rest_hot_reload_register_routes(router, "/_hot_reload");

/* Start a background watcher thread */
c_rest_hot_reload_start(ctx, on_file_changed, NULL);

/* When shutting down */
c_rest_hot_reload_destroy(ctx);
```

