/* clang-format off */
#include "greatest.h"
#include "oauth2_client.h"
#include "oauth2_server.h"
#include "c_orm_db.h"
#include "c_orm_sqlite.h"
#include "c_orm_oauth2.h"

#include <stdio.h>
#include <stdlib.h>
/* clang-format on */

SUITE(oauth2_suite);

TEST test_client_init_fail_nulls(void) {
  c_orm_db_t *db = NULL;
  c_orm_sqlite_connect(":memory:", &db);
  ASSERT_EQ(1, oauth2_client_init(NULL, "client_id", "client_secret", db));
  ASSERT_EQ(1, oauth2_client_init("http://localhost:8080/oauth/token", NULL,
                                  "client_secret", db));
  ASSERT_EQ(1, oauth2_client_init("http://localhost:8080/oauth/token",
                                  "client_id", NULL, db));
  PASS();
}

TEST test_server_init_fail_nulls(void) {
  c_orm_db_t *db = NULL;
  c_rest_router *router = NULL;
  c_orm_sqlite_connect(":memory:", &db);
  c_rest_router_init(&router);

  ASSERT_EQ(1, oauth2_server_init(NULL, db));
  ASSERT_EQ(1, oauth2_server_init(router, NULL));

  c_rest_router_destroy(router);
  PASS();
}

SUITE(oauth2_suite) {
  RUN_TEST(test_client_init_fail_nulls);
  RUN_TEST(test_server_init_fail_nulls);
  /* The client and server logic uses real requests.
     In a unit test we mock them or perform real network tests if the server is
     started in another thread. For now, we just exercise basic initialization
     failures for code coverage. */
}

/* Add definitions that need to be in the test runner */
GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
  GREATEST_MAIN_BEGIN();
  RUN_SUITE(oauth2_suite);
  GREATEST_MAIN_END();
}
