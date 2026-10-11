# c-rest-framework

[![License](https://img.shields.io/badge/license-Apache--2.0%20OR%20MIT-blue.svg)](https://opensource.org/licenses/Apache-2.0)
[![Doc Coverage](https://img.shields.io/badge/Doc_Coverage-100.0%25-green.svg)](#)
[![Test Coverage](https://img.shields.io/badge/Test_Coverage-100.0%25-green.svg)](#)
[![CI](https://github.com/SamuelMarks/c-rest-framework/actions/workflows/ci.yml/badge.svg)](https://github.com/SamuelMarks/c-rest-framework/actions/workflows/ci.yml)

Welcome to **c-rest-framework**, an ultra-lightweight, blazing-fast, and strictly C89-compliant web framework and HTTP/HTTPS server.

Designed for absolute portability, it thrives anywhere from constrained embedded hardware running DOS or Win32 to modern macOS, Linux, and Windows server environments. With a batteries-included approach, `c-rest-framework` brings modern web development features to classic C programming.

## 🚀 Why c-rest-framework?

- **Uncompromising Portability:** Written in strict ANSI C (C89). No dependencies on modern C standards, meaning if you have a working C compiler, you can run this framework.
- **Batteries Included:** Integrated flawlessly with `c-abstract-http`, `c-multiplatform`, and `../c-orm`.
- **Automatic OpenAPI & Swagger:** Generates OpenAPI specifications on the fly from your defined routes.
- **GraphQL Ready:** Native support for parsing and resolving GraphQL queries via built-in AST generation.
- **Rich I/O Capabilities:** Full support for WebSockets (RFC 6455), Server-Sent Events (SSE), and multipart/form-data streaming.
- **Enterprise Security:** Plug-and-play TLS/HTTPS support (OpenSSL, mbedTLS, wolfSSL, LibreSSL, BoringSSL, s2n-tls) alongside built-in OAuth2 and JWT authentication middlewares.
- **Dynamic Routing:** Build complex, parameterized routes with ease.
- **On-the-fly Compression:** Integrated Gzip and Brotli response compression.

## ⚙️ Execution Modalities

Adapt the server to your exact system architecture. `c-rest-framework` supports 8 different execution models out of the box:
1. **Synchronous (Blocking)** - Simple and deterministic.
2. **Asynchronous (Event Loop)** - High concurrency on a single thread.
3. **Single-threaded** - Minimal footprint.
4. **Multi-threaded (Thread Pool)** - Standard parallel execution.
5. **Single-process** - Isolated execution.
6. **Multi-process (Preforked Workers)** - Robust scaling akin to NGINX.
7. **Greenthreads (Coroutines)** - Lightweight cooperative multitasking.
8. **Message Passing (Actor-like queues)** - Safe, lock-free state management.

## 🛠️ Usage Example (c-orm SQL & CRUD)

Below is a robust C89-compliant quickstart example using `../c-orm` to automatically generate SQL-backed CRUD endpoints with rigorous error enum percolation.

```c
#include "c_rest_error.h"
#include "c_rest_modality.h"
#include "c_rest_orm_crud.h"
#include "c_rest_orm_middleware.h"
#include "c_rest_response.h"
#include "c_rest_router.h"
#include <stdio.h>

/*
 * Define the ORM model to interact with the database.
 * The ORM automatically maps these definitions to a fully-typed C struct
 * for type-safe row operations. For the schema below, the corresponding
 * generated struct is:
 *
 * struct note {
 *     int id;
 *     char *title;
 *     char *content;
 * };
 */
struct c_rest_orm_column note_columns[] = {
    {"id", C_REST_ORM_TYPE_INT},
    {"title", C_REST_ORM_TYPE_STRING},
    {"content", C_REST_ORM_TYPE_STRING}
};

struct c_rest_orm_model note_model = {
    "notes",
    "id",
    note_columns,
    sizeof(note_columns) / sizeof(note_columns[0])
};

int main(void) {
  struct c_rest_context *ctx = NULL;
  c_rest_router *router = NULL;
  c_rest_error_t rc;

  printf("Initializing Notes App...\n");

  /* 1. Initialize the Framework Modality */
  rc = c_rest_init(C_REST_MODALITY_SYNC, &ctx);
  if (rc != C_REST_OK) {
    return (int)rc;
  }

  /* 2. Configure ORM Connection to a local SQLite database */
  ctx->db_config.connection_string = "sqlite://notes.db";

  /* 3. Initialize Router */
  rc = c_rest_router_init(&router);
  if (rc != C_REST_OK) {
    c_rest_destroy(ctx);
    return (int)rc;
  }

  /* 4. Apply ORM Transaction Middlewares */
  rc = c_rest_router_use(router, "/api/v0",
                         c_rest_orm_transaction_start_middleware, ctx);
  if (rc != C_REST_OK) {
    c_rest_router_destroy(router);
    c_rest_destroy(ctx);
    return (int)rc;
  }

  rc = c_rest_router_use_post(router, "/api/v0",
                              c_rest_orm_transaction_end_middleware, ctx);
  if (rc != C_REST_OK) {
    c_rest_router_destroy(router);
    c_rest_destroy(ctx);
    return (int)rc;
  }

  /* 5. Set the Router into the context */
  rc = c_rest_set_router(ctx, router);
  if (rc != C_REST_OK) {
    c_rest_router_destroy(router);
    c_rest_destroy(ctx);
    return (int)rc;
  }

  /* 6. Auto-generate CRUD endpoints */
  rc = c_rest_router_add(router, "GET", "/api/v0/notes",
                         c_rest_orm_crud_get_list, &note_model);
  if (rc == C_REST_OK) {
    rc = c_rest_router_add(router, "POST", "/api/v0/notes",
                           c_rest_orm_crud_create, &note_model);
  }
  if (rc == C_REST_OK) {
    rc = c_rest_router_add(router, "GET", "/api/v0/notes/:id",
                           c_rest_orm_crud_get_one, &note_model);
  }
  if (rc == C_REST_OK) {
    rc = c_rest_router_add(router, "PUT", "/api/v0/notes/:id",
                           c_rest_orm_crud_update, &note_model);
  }
  if (rc == C_REST_OK) {
    rc = c_rest_router_add(router, "DELETE", "/api/v0/notes/:id",
                           c_rest_orm_crud_delete, &note_model);
  }

  if (rc != C_REST_OK) {
    c_rest_router_destroy(router);
    c_rest_destroy(ctx);
    return (int)rc;
  }

  /* 7. Start the server */
  printf("App configured successfully.\n");
  /* rc = c_rest_run(ctx); */ /* Blocks and serves HTTP requests indefinitely */

  c_rest_router_destroy(router);
  c_rest_destroy(ctx);

  return (int)rc;
}
```

### Interacting with the Example
Assuming the application is compiled and listening on `http://localhost:8080`:

**Create a note:**
```sh
curl -X POST http://localhost:8080/api/v0/notes \
     -H "Content-Type: application/json" \
     -d '{"title": "My first note", "content": "Hello world"}'
```

**List all notes:**
```sh
curl -X GET http://localhost:8080/api/v0/notes
```

## 🏗️ Building the Framework

We use CMake for cross-platform builds. Dependencies are automatically managed via `vcpkg` or `FetchContent`.

```bash
mkdir -p build && cd build

# Basic Build
cmake ..
cmake --build .

# Build with TLS Support (e.g., OpenSSL)
cmake .. -DC_REST_FRAMEWORK_USE_OPENSSL=ON
cmake --build .
```


For end-users, here is a sample `CMakeLists.txt` for integrating the framework into your own projects:

```cmake
cmake_minimum_required(VERSION 3.10)
project(notes_app C)

# Option 1: If building alongside the framework source
add_subdirectory(c-rest-framework)

# Option 2: If using FetchContent to pull the framework from GitHub
# include(FetchContent)
# FetchContent_Declare(
#     c-rest-framework
#     GIT_REPOSITORY https://github.com/SamuelMarks/c-rest-framework.git
#     GIT_TAG        main
# )
# FetchContent_MakeAvailable(c-rest-framework)

# Create the executable
add_executable(notes_app main.c)

# Link the framework (this automatically handles include paths and transitive dependencies like c-orm)
target_link_libraries(notes_app PRIVATE c-rest-framework)
```

For advanced configuration and architecture details, please consult our [USAGE.md](USAGE.md) and [ARCHITECTURE.md](ARCHITECTURE.md) files.

---

## License

Licensed under either of

- Apache License, Version 2.0 ([LICENSE-APACHE](LICENSE-APACHE) or <https://www.apache.org/licenses/LICENSE-2.0>)
- MIT license ([LICENSE-MIT](LICENSE-MIT) or <https://opensource.org/licenses/MIT>)

at your option.

### Contribution

Unless you explicitly state otherwise, any contribution intentionally submitted
for inclusion in the work by you, as defined in the Apache-2.0 license, shall be
dual licensed as above, without any additional terms or conditions.
