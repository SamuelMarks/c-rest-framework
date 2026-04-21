# c-rest-framework Architecture

## Modality System
The framework is built around a modality execution model, allowing the consumer to select the processing paradigm at runtime or initialization time.

### Supported Modalities
- **Synchronous**: Blocks on I/O. Simplest model, suitable for DOS and basic constrained environments.
- **Asynchronous**: Uses an event loop. Non-blocking I/O.
- **Single-threading**: Runs everything on the main thread.
- **Multi-threading**: Utilizes a thread pool to handle concurrent requests.
- **Single-process**: All logic happens in a single process.
- **Multi-process**: Preforks worker processes to handle requests.
- **Greenthreads**: Coroutine-based scheduling, offering apparent concurrent execution without OS threads.
- **Message Passing**: Uses actor-like queues to pass requests and responses.

## Memory & C89 Compliance
- Strict ANSI C (C89).
- No VLA (Variable Length Arrays).
- Memory allocations are abstracted and bound to the framework context.

## Multiplatform Boundary Layers
`c-rest-framework` explicitly supports integration with `c-multiplatform` acting as the frontend. When built with `C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION=ON`:
- **Context Injection**: Users can inject a `cm_env_t` environment using `c_rest_set_multiplatform_env()`.
- **Primitives Overridden**: All internal network handles (`c_rest_socket_t`) and threading primitives morph into their `c-multiplatform` equivalents at compile-time via `include/c_rest_platform.h`.
- **Logging & Allocation**: `c-rest-framework` forwards its allocator and logging callbacks to the injected multiplatform environment automatically, ensuring cross-boundary unified memory tracking and tracing without leaks.
- **Polling Hooks**: In asynchronous and threading modalities, the framework will bypass standard POSIX/Win32 I/O calls and directly leverage the `c-multiplatform` API (`cm_socket_accept`, `cm_socket_close`, etc.).

## HTTPS & TLS Integration
c-rest-framework provides native HTTPS support through integration with multiple cryptographic backends via c_rest_tls.h.

- **Backends:** OpenSSL, LibreSSL, BoringSSL, mbedTLS, wolfSSL, and s2n-tls.
- **Context Management:** TLS states are managed via struct c_rest_tls_context attached to the main framework modality.
- **Handshakes:** TLS handshakes are seamlessly processed inside synchronous (c_rest_tls_accept blocking mode) and asynchronous (C_REST_TLS_WANT_READ/WRITE state machine) modalities.
- **Strict Settings:** The framework enforces a minimum of TLS 1.2+ configuration internally.
- **Middlewares:** Includes built-in middlewares for Strict-Transport-Security (HSTS) and HTTP-to-HTTPS redirections.

## OpenAPI and Swagger Documentation
The framework natively supports generating dynamic OpenAPI 3.0.0 JSON specifications and interactive Swagger UI interfaces.
- **Route Metadata**: Handlers can be registered with `c_rest_router_add_openapi()`, providing rich metadata such as `summary`, `description`, `tags`, and expected JSON schema definitions for requests and responses.
- **Content-Type Overrides**: Handles varied encoding strategies (e.g., `application/json` and `application/x-www-form-urlencoded`).
- **Dynamic JSON Export**: Internally utilizes the bundled `parson` library to serialize the registered C structs and OpenAPI configurations into a single accessible endpoint (via `c_rest_enable_openapi()`).
- **Interactive Documentation**: A static, completely embedded HTML Swagger UI instance can be mounted at any route (via `c_rest_enable_swagger_ui()`) for instantaneous, web-based API testing.
