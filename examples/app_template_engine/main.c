/* clang-format off */
#include "c_rest_router.h"
#include "c_rest_request.h"
#include "c_rest_response.h"
#include "c_rest_modality.h"
#include "c_rest_template.h"
#include <stdio.h>
#include <stdlib.h>
/* clang-format on */

#ifdef C_REST_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING

static int provide_template_data(struct c_rest_request *req,
                                 const char ***out_keys,
                                 const char ***out_values, size_t *out_count,
                                 void *user_data) {
  /* Hardcoded for example purposes, could fetch from DB using user_data */
  static const char *keys[] = {"title", "heading", "message"};
  static const char *values[] = {"My Webpage", "Welcome to C-REST",
                                 "HTML rendering in C is fast!"};

  (void)req;
  (void)user_data;

  *out_keys = keys;
  *out_values = values;
  *out_count = 3;

  return 0;
}

int main(void) {
  struct c_rest_context *ctx = NULL;
  c_rest_router *router = NULL;
  struct c_rest_template_context tpl_ctx;

  /* Initialize framework context in Single Thread mode */
  if (c_rest_init(C_REST_MODALITY_SINGLE_THREAD, &ctx) != 0) {
    printf("Failed to init framework\n");
    return 1;
  }

  /* Initialize Router */
  if (c_rest_router_init(&router) != 0) {
    printf("Failed to init router\n");
    c_rest_destroy(ctx);
    return 1;
  }

  /* Initialize Template */
  if (c_rest_template_init(
          &tpl_ctx,
          "<html><head><title>{{title}}</title></head>"
          "<body><h1>{{heading}}</h1><p>{{message}}</p></body></html>") != 0) {
    printf("Failed to init template\n");
    c_rest_router_destroy(router);
    c_rest_destroy(ctx);
    return 1;
  }

  /* Register Template Route */
  c_rest_router_add_template(router, "GET", "/", &tpl_ctx,
                             provide_template_data, NULL);

  /* Run Server */
  printf("Starting server at http://127.0.0.1:8080/\n");
  c_rest_set_router(ctx, router);
  /* c_rest_run(ctx); Uncomment to block and run server */

  /* Cleanup */
  c_rest_template_destroy(&tpl_ctx);
  c_rest_router_destroy(router);
  c_rest_destroy(ctx);

  return 0;
}

#else

int main(void) {
  printf("Server-Side Template Engine is disabled.\n");
  return 0;
}

#endif
