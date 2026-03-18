# c-rest-framework Examples

This document provides overviews and instructions for the example applications located in the `examples/` directory.

## OAuth2

The `oauth2` directory contains a standalone, RFC 6749 compliant OAuth 2.0 server and client implementation utilizing `c-rest-framework` and `c-orm`. Currently, it demonstrates the **Password Grant** flow and initializes an in-memory SQLite database to manage users, clients, and tokens.

It is designed to be easily integrated into other applications, providing `static`, `shared`, and `header-only` library targets, alongside a standalone CLI executable.

### Building

The OAuth2 example is configured so that it can be built completely standalone. If `c-rest-framework` and `c-orm` are not available in your CMake tree, it will automatically fetch them from GitHub via CMake's `FetchContent`.

```bash
cd examples/oauth2
cmake -S . -B build
cmake --build build
```

### Running the Server

Once built, you can start the demonstration server, which will listen on port `8080` by default:

```bash
# From the build directory (on Windows, it might be inside Debug/ or Release/)
./oauth2_cli
```

Alternatively, you can provide configuration via command line arguments or environment variables:

```bash
./oauth2_cli --listen-port 9090 --db-url "sqlite.db"
```

**Environment Variables:**
* `OAUTH2_LISTEN_ADDR`
* `OAUTH2_LISTEN_PORT`
* `OAUTH2_DB_URL`
* `OAUTH2_TLS_CERT`
* `OAUTH2_TLS_KEY`

### Endpoints

The server exposes the following routes, prefixed with `/api/v0`:

* `POST /api/v0/oauth/clients`: Register a new client application by sending `client_id` and `client_secret` via `application/x-www-form-urlencoded`.
* `POST /api/v0/users`: Register a new user by sending `username` and `password` via `application/x-www-form-urlencoded`.
* `POST /api/v0/oauth/token`: Standard RFC 6749 endpoint. Test the password grant flow by sending a `application/x-www-form-urlencoded` request with `grant_type=password`, along with your `username`, `password`, `client_id`, and `client_secret`.
* `POST /api/v0/login`: Directly submit `username` and `password` as `application/x-www-form-urlencoded` to receive an access token pair.
* `POST /api/v0/logout`: Revoke an active access token by supplying it in the `Authorization: Bearer <token>` header.
* `GET /api/v0/secret`: A protected route that verifies the provided `Authorization: Bearer <token>` header against the database before returning a secret message.

### Quick Start: Password Grant Flow

Here is a complete example of creating a client, creating a user, and authenticating using either **HTTPie** or **cURL**.

**1. Register a Client**
```bash
# HTTPie
http -f POST :8080/api/v0/oauth/clients client_id=my_app client_secret=my_secret

# cURL
curl -X POST http://127.0.0.1:8080/api/v0/oauth/clients \
  -d "client_id=my_app" \
  -d "client_secret=my_secret"
```

**2. Register a User**
```bash
# HTTPie
http -f POST :8080/api/v0/users username=alice password=supersecret

# cURL
curl -X POST http://127.0.0.1:8080/api/v0/users \
  -d "username=alice" \
  -d "password=supersecret"
```

**3. Obtain an Access Token**
```bash
# HTTPie
http -f POST :8080/api/v0/oauth/token grant_type=password username=alice password=supersecret client_id=my_app client_secret=my_secret

# cURL
curl -X POST http://127.0.0.1:8080/api/v0/oauth/token \
  -d "grant_type=password" \
  -d "username=alice" \
  -d "password=supersecret" \
  -d "client_id=my_app" \
  -d "client_secret=my_secret"
```

### Testing

The example includes a unit test suite built using the `greatest` testing framework. Tests are enabled by default but can be toggled via the `OAUTH2_BUILD_TESTS` CMake option.

To build and execute the test suite:

```bash
cd examples/oauth2
mkdir build
cd build
cmake -DOAUTH2_BUILD_TESTS=ON ..
cmake --build .

# Run the tests via CTest
ctest --output-on-failure

# Or run the test executable directly
./oauth2_tests
```