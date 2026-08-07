#include "c_rest_request.h"
#include "c_rest_response.h"
#include "greatest.h"

int app_node_style_main(void);

#define main app_node_style_main
#include "../examples/app_node_style/main.c"
#undef main

TEST test_app_node_style(void) {
  struct c_rest_request req;
  struct c_rest_response res;
  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  handle_hello_world(&req, &res, NULL);

  req.query = "name=Alice";
  handle_echo(&req, &res, NULL);

  req.query = "other=Alice";
  handle_echo(&req, &res, NULL);

  app_node_style_main();

  /* Also test the failure cases of app_node_style_main */
  /* If we mock c_rest_init, it fails, but we don't have mock here. */
  PASS();
}

SUITE(examples_suite) { RUN_TEST(test_app_node_style); }
