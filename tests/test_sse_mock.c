/* clang-format off */
#include "c_rest_error.h"
#include "greatest.h"
#include <string.h>

#undef C_REST_EXPORT
#define C_REST_EXPORT

#include "c_rest_sse.h"
#include "c_rest_response.h"
#include "c_rest_string.h"

static int g_mock_str_init_countdown = -1;
static int g_mock_res_write_chunk_countdown = -1;
static int g_mock_res_status_countdown = -1;

extern c_rest_error_t c_rest_string_init(struct c_rest_string *str, size_t initial_capacity);
extern c_rest_error_t c_rest_response_write_chunk(struct c_rest_response *res, const char *chunk, size_t chunk_len);
extern c_rest_error_t c_rest_response_set_status(struct c_rest_response *res, int status);

static c_rest_error_t mock_c_rest_string_init(struct c_rest_string *str, size_t initial_capacity) {
    if (g_mock_str_init_countdown >= 0) {
        if (g_mock_str_init_countdown == 0) return C_REST_ERROR_GENERIC;
        g_mock_str_init_countdown--;
    }
    return c_rest_string_init(str, initial_capacity);
}

static c_rest_error_t mock_c_rest_response_write_chunk(struct c_rest_response *res, const char *chunk, size_t chunk_len) {
    if (g_mock_res_write_chunk_countdown >= 0) {
        if (g_mock_res_write_chunk_countdown == 0) return C_REST_ERROR_GENERIC;
        g_mock_res_write_chunk_countdown--;
    }
    return c_rest_response_write_chunk(res, chunk, chunk_len);
}

static c_rest_error_t mock_c_rest_response_set_status(struct c_rest_response *res, int status) {
    if (g_mock_res_status_countdown >= 0) {
        if (g_mock_res_status_countdown == 0) return C_REST_ERROR_GENERIC;
        g_mock_res_status_countdown--;
    }
    return c_rest_response_set_status(res, status);
}

#define c_rest_string_init mock_c_rest_string_init
#define c_rest_response_write_chunk mock_c_rest_response_write_chunk
#define c_rest_response_set_status mock_c_rest_response_set_status

#define c_rest_sse_serialize test_c_rest_sse_serialize
#define c_rest_sse_context_destroy test_c_rest_sse_context_destroy
#define c_rest_sse_init_response test_c_rest_sse_init_response
#define c_rest_sse_send_event test_c_rest_sse_send_event

#include "../src/c_rest_sse.c"

#undef c_rest_string_init
#undef c_rest_response_write_chunk
#undef c_rest_response_set_status

/* clang-format on */

static void reset_mocks(void *data) {
  (void)data;
  g_mock_str_init_countdown = -1;
  g_mock_res_write_chunk_countdown = -1;
  g_mock_res_status_countdown = -1;
}

TEST test_sse_error_branches(void) {
  struct c_rest_sse_event evt = {0};
  struct c_rest_response res = {0};
  char *out_buf = NULL;
  size_t out_len = 0;

  /* event_init fails on str_init */
  g_mock_str_init_countdown = 0;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_sse_serialize(&evt, &out_buf, &out_len));

  /* client_send_event fails on write_chunk */
  g_mock_res_write_chunk_countdown = 0;
  ASSERT_EQ(C_REST_ERROR_GENERIC, test_c_rest_sse_send_event(&res, &evt));

  /* init_response fails on set_status */
  g_mock_res_status_countdown = 0;
  ASSERT_EQ(C_REST_ERROR_GENERIC, test_c_rest_sse_init_response(&res));

  PASS();
}

SUITE(sse_mock_suite) {
  SET_SETUP(reset_mocks, NULL);
  RUN_TEST(test_sse_error_branches);
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
  GREATEST_MAIN_BEGIN();
  RUN_SUITE(sse_mock_suite);
  GREATEST_MAIN_END();
}
