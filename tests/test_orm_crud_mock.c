/* clang-format off */
#include "c_rest_error.h"
#include "greatest.h"
#include <string.h>

#undef C_REST_EXPORT
#define C_REST_EXPORT

#include "c_rest_response.h"

static int g_mock_res_json_countdown = -1;
static int g_mock_res_status_countdown = -1;

extern c_rest_error_t c_rest_response_json(struct c_rest_response *res, const char *json_str);
extern c_rest_error_t c_rest_response_set_status(struct c_rest_response *res, int status_code);

static c_rest_error_t mock_c_rest_response_json(struct c_rest_response *res, const char *json_str) {
    if (g_mock_res_json_countdown >= 0) {
        if (g_mock_res_json_countdown == 0) return C_REST_ERROR_GENERIC;
        g_mock_res_json_countdown--;
    }
    return c_rest_response_json(res, json_str);
}

static c_rest_error_t mock_c_rest_response_set_status(struct c_rest_response *res, int status_code) {
    if (g_mock_res_status_countdown >= 0) {
        if (g_mock_res_status_countdown == 0) return C_REST_ERROR_GENERIC;
        g_mock_res_status_countdown--;
    }
    return c_rest_response_set_status(res, status_code);
}

#define c_rest_response_json mock_c_rest_response_json
#define c_rest_response_set_status mock_c_rest_response_set_status

#define c_rest_orm_crud_create test_c_rest_orm_crud_create
#define c_rest_orm_crud_get_one test_c_rest_orm_crud_get_one
#define c_rest_orm_crud_update test_c_rest_orm_crud_update
#define c_rest_orm_crud_delete test_c_rest_orm_crud_delete
#define c_rest_orm_crud_get_list test_c_rest_orm_crud_get_list

#include "../src/c_orm_crud.c"

#undef c_rest_response_json
#undef c_rest_response_set_status

/* clang-format on */

static void reset_mocks(void *data) {
  (void)data;
  g_mock_res_json_countdown = -1;
  g_mock_res_status_countdown = -1;
}

TEST test_orm_crud_error_branches(void) {
  struct c_rest_request req = {0};
  struct c_rest_response res = {0};

  req.body = "{}";

  /* test c_rest_response_json failure in c_rest_orm_crud_create */
  g_mock_res_json_countdown = 0;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_orm_crud_create(&req, &res, NULL));

  /* test c_rest_response_set_status failure in c_rest_orm_crud_get_one */
  g_mock_res_status_countdown = 0;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_orm_crud_get_one(&req, &res, NULL));

  /* test c_rest_response_set_status failure in c_rest_orm_crud_update */
  g_mock_res_status_countdown = 0;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_orm_crud_update(&req, &res, NULL));

  /* test c_rest_response_set_status failure in c_rest_orm_crud_delete */
  g_mock_res_status_countdown = 0;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_orm_crud_delete(&req, &res, NULL));

  /* test c_rest_response_set_status failure in c_rest_orm_crud_get_list */
  g_mock_res_status_countdown = 0;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_orm_crud_get_list(&req, &res, NULL));

  PASS();
}

SUITE(orm_crud_mock_suite) {
  SET_SETUP(reset_mocks, NULL);
  RUN_TEST(test_orm_crud_error_branches);
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
  GREATEST_MAIN_BEGIN();
  RUN_SUITE(orm_crud_mock_suite);
  GREATEST_MAIN_END();
}
