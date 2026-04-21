# c-rest-framework OAuth2 Example

This project serves as a standalone reference implementation of an OAuth 2.0 server and client SDK built using the lightweight `c-rest-framework` and `c-orm`.

It demonstrates building secure, database-backed HTTP endpoints entirely in C, and can be integrated into your own CMake projects either as a standalone CLI executable, or consumed directly via library targets (STATIC, SHARED, and INTERFACE).

## Requirements
* **CMake 3.10+**
* A working C compiler (GCC, Clang, or MSVC)
* Note: Dependencies like `c-rest-framework` and `c-orm` are automatically fetched by CMake if they are not detected in your project structure.

## Building

Generate build files and compile the `oauth2_example` suite using CMake:

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

## Running the Server

The compiled executable `oauth2_cli` provides a fully-functional (mock-populated) OAuth 2.0 API. 

```bash
./oauth2_cli --listen-port 9090 --db-url "sqlite.db"
```

You can alternatively configure the server using environment variables:
* `OAUTH2_LISTEN_ADDR` (Default: `0.0.0.0`)
* `OAUTH2_LISTEN_PORT` (Default: `8080`)
* `OAUTH2_DB_URL` (Default: `:memory:`)
* `OAUTH2_TLS_CERT` (Optional: Path to `server.crt` to enable HTTPS)
* `OAUTH2_TLS_KEY`  (Optional: Path to `server.key` to enable HTTPS)

## Available Endpoints

The server exposes the following routes for demonstration:

| Method | Route | Description | 
|--------|-------|-------------|
| **POST** | `/api/v0/oauth/clients` | Register a new client application (`client_id` and `client_secret`). |
| **POST** | `/api/v0/users` | Register a new user (`username` and `password`). |
| **POST** | `/api/v0/oauth/token` | Standard RFC 6749 Token endpoint. Submit `grant_type=password` along with your credentials via `application/x-www-form-urlencoded`. |
| **POST** | `/api/v0/login` | A simplified login handler that accepts `username` and `password` and immediately returns a new access token pair. |
| **POST** | `/api/v0/logout` | Revokes the current session token provided via the `Authorization: Bearer <token>` HTTP header. |
| **GET** | `/api/v0/secret` | A protected resource that validates the `Authorization: Bearer` header before returning secure data. |
| **GET** | `/api/v0/openapi.json` | Generated OpenAPI 3 specification of the endpoints. |
| **GET** | `/api/v0/docs` | Swagger UI portal to browse and interact with the endpoints. |

## Quick Start: Password Grant Flow

Here is a complete example of creating a client, creating a user, and authenticating to receive an access token using either **HTTPie** or **cURL**.

### 1. Register a Client
Create a new client application that will request the tokens.

**HTTPie:**
```bash
http -f POST :8080/api/v0/oauth/clients client_id=my_app client_secret=my_secret
```

**cURL:**
```bash
curl -X POST http://127.0.0.1:8080/api/v0/oauth/clients \
  -d "client_id=my_app" \
  -d "client_secret=my_secret"
```

### 2. Register a User
Create a user account that will log into the application.

**HTTPie:**
```bash
http -f POST :8080/api/v0/users username=alice password=supersecret
```

**cURL:**
```bash
curl -X POST http://127.0.0.1:8080/api/v0/users \
  -d "username=alice" \
  -d "password=supersecret"
```

### 3. Obtain an Access Token
Exchange the user's credentials and the client's credentials for an OAuth 2.0 Access Token using the Password Grant flow.

**HTTPie:**
```bash
http -f POST :8080/api/v0/oauth/token grant_type=password username=alice password=supersecret client_id=my_app client_secret=my_secret
```

**cURL:**
```bash
curl -X POST http://127.0.0.1:8080/api/v0/oauth/token \
  -d "grant_type=password" \
  -d "username=alice" \
  -d "password=supersecret" \
  -d "client_id=my_app" \
  -d "client_secret=my_secret"
```

## Extending with Your Own Logic

To add your own models and routes, you can initialize `c-orm` alongside `c-rest-framework`:

1. Define your data models in `c-orm`.
2. Create new route handler functions conforming to the `c_rest_router_handler` signature:
   ```c
   int my_custom_handler(struct c_rest_request *req, struct c_rest_response *res, void *user_data);
   ```
3. Use the `c_rest_request_*` APIs to safely parse bodies, headers, and query parameters.
4. Pass your `c_orm_db_t` handle as the `user_data` to interact securely with your database:
   ```c
   c_rest_router_add(router, "GET", "/api/v0/my_route", my_custom_handler, (void *)db);
   ```

## Testing

A suite of unit tests using the `greatest` testing framework is built alongside the server by default.

To run the tests:
```bash
cd build
ctest --output-on-failure
# Or execute the runner manually
./oauth2_tests
```
