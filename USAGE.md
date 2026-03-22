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

