/* clang-format off */
#include "c_rest_modality.h"
#include "c_rest_jwt_middleware.h"
#include "c_rest_crypto.h"
#include "c_rest_router.h"
#include "c_rest_request.h"
#include "c_rest_response.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
/* clang-format on */

#ifdef C_REST_ENABLE_JWT_JSON_WEB_TOKENS_AUTHENTICATION_MIDDLEWARE

static int my_verify_payload(const char *payload, void **out_auth_context) {
  /* Simple mock payload verifier. In real app, parse the JSON payload here */
  if (strstr(payload, "\"sub\":\"user123\"") != NULL) {
    *out_auth_context = (void *)1; /* User ID or struct */
    return 0;
  }
  return 1;
}

static int protected_route_handler(struct c_rest_request *req,
                                   struct c_rest_response *res,
                                   void *user_data) {
  (void)user_data;
  if (req->auth_context == (void *)1) {
    c_rest_response_html(res,
                         "Hello, User 123! Welcome to the protected route.");
  } else {
    c_rest_response_html(
        res, "Hello, Unknown User! Welcome to the protected route.");
  }
  return 0;
}

static int generate_token_handler(struct c_rest_request *req,
                                  struct c_rest_response *res,
                                  void *user_data) {
  const unsigned char *secret = (const unsigned char *)user_data;
  char *token = NULL;
  char response_buf[1024];

  (void)req;
  if (c_rest_jwt_sign_hs256("{\"sub\":\"user123\"}", secret,
                            strlen((const char *)secret), &token) == 0) {
#if defined(_MSC_VER)
    sprintf_s(response_buf, sizeof(response_buf), "Your token is: %s", token);
#else
    sprintf(response_buf, "Your token is: %s", token);
#endif
    c_rest_response_html(res, response_buf);
    free(token);
  } else {
    c_rest_response_set_status(res, 500);
    c_rest_response_html(res, "Failed to generate token");
  }

  return 0;
}

int main(void) {
  struct c_rest_context *ctx = NULL;
  struct c_rest_router *router = NULL;
  struct c_rest_jwt_middleware_config jwt_config;
  const unsigned char secret[] = "my_super_secret_key";
  int res;

  printf("Initializing c-rest-framework...\n");
  res = c_rest_init(C_REST_MODALITY_SYNC, &ctx);
  if (res != 0) {
    printf("Failed to initialize framework.\n");
    return 1;
  }

  res = c_rest_router_init(&router);
  if (res != 0) {
    printf("Failed to initialize router.\n");
    c_rest_destroy(ctx);
    return 1;
  }

  /* Init JWT middleware config */
  c_rest_jwt_middleware_config_init(&jwt_config, secret, sizeof(secret) - 1,
                                    my_verify_payload);

  /* Set up routes */
  c_rest_router_add(router, "GET", "/token", generate_token_handler,
                    (void *)secret);

  /* Add middleware to the protected route */
  c_rest_router_use(router, "/protected", c_rest_jwt_middleware, &jwt_config);
  c_rest_router_add(router, "GET", "/protected", protected_route_handler, NULL);

  /* Attach router to context */
  ctx->router = router;

  printf("Server is ready (Simulated). In a real application, you would call "
         "c_rest_listen().\n");
  printf("Run the application and navigate to /token to get a token, then pass "
         "it to /protected as a Bearer token.\n");

  c_rest_router_destroy(router);
  c_rest_destroy(ctx);
  return 0;
}

#else

int main(void) {
  printf("JWT Middleware is not enabled. Compile with "
         "-DC_REST_ENABLE_JWT_JSON_WEB_TOKENS_AUTHENTICATION_MIDDLEWARE=ON\n");
  return 0;
}

#endif
