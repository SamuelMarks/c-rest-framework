/* clang-format off */
#include "c_rest_error.h"
#include "greatest.h"
#include <string.h>

#undef C_REST_EXPORT
#define C_REST_EXPORT

#include "c_rest_http23.h"
#include "c_rest_request.h"

static int g_mock_req_cleanup_countdown = -1;

extern c_rest_error_t c_rest_request_cleanup(struct c_rest_request *req);

static c_rest_error_t mock_c_rest_request_cleanup(struct c_rest_request *req) {
    if (g_mock_req_cleanup_countdown >= 0) {
        if (g_mock_req_cleanup_countdown == 0) return C_REST_ERROR_GENERIC;
        g_mock_req_cleanup_countdown--;
    }
    return c_rest_request_cleanup(req);
}

#define c_rest_request_cleanup mock_c_rest_request_cleanup
#define c_rest_http23_ctx_destroy test_c_rest_http23_ctx_destroy

#include "../src/c_rest_http23.c"

#undef c_rest_request_cleanup

/* clang-format on */

static void reset_mocks(void *data) {
  (void)data;
  g_mock_req_cleanup_countdown = -1;
}

TEST test_http23_error_branches(void) {
  c_rest_http23_ctx_t ctx = {0};

  g_mock_req_cleanup_countdown = 0;
  /* This will fail to clean up, hitting the error branch */
  ASSERT_EQ(C_REST_ERROR_GENERIC, test_c_rest_http23_ctx_destroy(&ctx));

  PASS();
}

SUITE(http23_mock_suite) {
  SET_SETUP(reset_mocks, NULL);
  RUN_TEST(test_http23_error_branches);
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
  GREATEST_MAIN_BEGIN();
  RUN_SUITE(http23_mock_suite);
  GREATEST_MAIN_END();
}
