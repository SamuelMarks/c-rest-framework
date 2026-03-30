c-rest-framework
================

[![License](https://img.shields.io/badge/license-Apache--2.0%20OR%20MIT-blue.svg)](https://opensource.org/licenses/Apache-2.0)
[![CI](https://github.com/SamuelMarks/c-rest-framework/actions/workflows/ci.yml/badge.svg)](https://github.com/SamuelMarks/c-rest-framework/actions/workflows/ci.yml)

A highly portable, modality-driven REST framework written in strict C89 (ANSI C).

## Overview

`c-rest-framework` is an extremely lightweight, high-performance HTTP/HTTPS web framework and server built for C. It is designed to be fully embedded into applications across a vast array of operating systems and hardware platforms, from modern environments (Windows, macOS, Linux, BSD) to highly constrained legacy systems (DOS, Win32).

It integrates seamlessly with `c-abstract-http` (for parser/client abstractions), `c-multiplatform` (for threading/sockets), and `c-orm` (for database mapping and connection pooling), providing a comprehensive batteries-included stack.

### Key Features
- **Strict C89 Compliance:** Zero dependencies on newer C standards. Highly portable.
- **OpenAPI & Swagger UI:** Natively generates and serves OpenAPI JSON specs and Swagger UI documentation directly from registered routes (powered by `cdd-c`).
- **8 Execution Modalities:** Choose how your server executes at runtime:
  - Synchronous (Blocking)
  - Asynchronous (Event loop)
  - Single-threaded
  - Multi-threaded (Thread pool)
  - Single-process
  - Multi-process (Preforked workers)
  - Greenthreads (Coroutines)
  - Message Passing (Actor-like queues)
- **HTTPS & TLS Integration:** Built-in secure context handling wrapping multiple backends (OpenSSL, mbedTLS, wolfSSL, LibreSSL, BoringSSL, s2n-tls).
- **ORM & Database Support:** Native integration with `c-orm` for automatic connection pooling, transactional routing, and CRUD endpoint generation.
- **Dynamic Routing & Path Variables:** E.g., `GET /api/users/:id`
- **Response Compression:** Built-in dynamic Gzip and Brotli response compression support.
- **Hot-Reloading & Auto-Restart:** Watch files and automatically trigger restarts or push live updates to clients via Server-Sent Events (SSE).
- **GraphQL APIs:** Built-in GraphQL API Parsing and Resolving schema logic directly into the router via AST generation.
- **Authentication:** Built-in OAuth2 support, Basic Auth, and JWT (JSON Web Tokens) Authentication Middlewares.
- **Full Multipart Form Streaming:** Natively streams multipart/form-data uploads with state-machine parsing, avoiding loading large files into memory.
- **WebSockets (RFC 6455):** Native upgrade support, frame parsing, masking/unmasking, and payload management.  
- **Server-Sent Events (SSE):** Native streaming API for pushing continuous data to clients efficiently.
- **Built-in Middlewares:** Static file serving, CORS, Request Logging, HSTS, HTTP-to-HTTPS redirect.
---

## Usage Example (c-orm + c-rest-framework)

Below is a complete, working example of a simple Notes application. It uses `c-orm` to automatically generate RESTful endpoints backed by a SQLite database.

```c
#include "c_rest_modality.h"
#include "c_rest_router.h"
#include "c_rest_orm_crud.h"
#include "c_rest_orm_middleware.h"
#include "c_rest_response.h"
#include <stdio.h>

/* 1. Define your ORM model */
struct c_rest_orm_model note_model = {
    "notes", /* Table name */
    "id"     /* Primary key */
};

int main(void) {
  struct c_rest_context *ctx = NULL;
  c_rest_router *router = NULL;
  int res;

  printf("Initializing Notes App...\n");

  /* 2. Initialize the Framework Modality (e.g. Synchronous) */
  res = c_rest_init(C_REST_MODALITY_SYNC, &ctx);
  if (res != 0) return 1;

  /* 3. Configure the ORM Connection Pool */
  ctx->db_config.connection_string = "sqlite://notes.db";

  /* 4. Create the Router */
  c_rest_router_init(&router);

  /* 5. Add Transactional Middlewares to /api/v0 */
  c_rest_router_use(router, "/api/v0", c_rest_orm_transaction_start_middleware, ctx);
  c_rest_router_use_post(router, "/api/v0", c_rest_orm_transaction_end_middleware, ctx);

  /* 6. Auto-generate CRUD Endpoints for the Note Model */
  c_rest_router_add(router, "GET", "/api/v0/notes", c_rest_orm_crud_get_list, &note_model);
  c_rest_router_add(router, "POST", "/api/v0/notes", c_rest_orm_crud_create, &note_model);
  c_rest_router_add(router, "GET", "/api/v0/notes/:id", c_rest_orm_crud_get_one, &note_model);
  c_rest_router_add(router, "PUT", "/api/v0/notes/:id", c_rest_orm_crud_update, &note_model);
  c_rest_router_add(router, "DELETE", "/api/v0/notes/:id", c_rest_orm_crud_delete, &note_model);

  /* 7. Attach router and run server */
  ctx->internal_state = router;
  /* c_rest_run(ctx); */ /* Blocks and serves HTTP requests indefinitely */

  c_rest_router_destroy(router);
  c_rest_destroy(ctx);
  return 0;
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

---

## Building

The framework uses CMake and conditionally pulls dependencies (via `vcpkg` or `FetchContent` if unavailable locally).

```sh
mkdir build && cd build

# Standard build
cmake ..
cmake --build .

# Build with TLS (e.g. OpenSSL)
cmake .. -DC_REST_FRAMEWORK_USE_OPENSSL=ON
cmake --build .
```

See `ARCHITECTURE.md` for architectural concepts, `USAGE.md` for comprehensive API documentation, and `PLAN.md` for our roadmap.


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


