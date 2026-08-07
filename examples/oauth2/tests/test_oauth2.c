/* clang-format off */
#include "c_rest_error.h"
#include "greatest.h"
#include "oauth2_client.h"
#include "oauth2_server.h"
#include "c_orm_db.h"
#include "c_orm_sqlite.h"
#include "c_orm_oauth2.h"
#include "c_rest_request.h"
#include "c_rest_response.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

  /* Missing c_rest_client_init mock will cause this to fail gracefully */
  ASSERT_EQ(0, oauth2_client_init("http://localhost:8080/oauth/token",
                                  "client_id", "client_secret", db));
  oauth2_client_cleanup();

  if (db && db->vtable && db->vtable->disconnect)
    db->vtable->disconnect(db);
  PASS();
}

TEST test_server_init_fail_nulls(void) {
  c_orm_db_t *db = NULL;
  c_rest_router *router = NULL;
  c_orm_sqlite_connect(":memory:", &db);
  (void)!c_rest_router_init(&router);

  ASSERT_EQ(1, oauth2_server_init(NULL, db));
  ASSERT_EQ(1, oauth2_server_init(router, NULL));

  ASSERT_EQ(0, oauth2_server_init(router, db));

  (void)!c_rest_router_destroy(router);
  if (db && db->vtable && db->vtable->disconnect)
    db->vtable->disconnect(db);
  PASS();
}

TEST test_client_password_grant(void) {
  char *access_token = NULL;
  int expires_in = 0;

  ASSERT_EQ(1, oauth2_client_password_grant(NULL, "pass", &access_token,
                                            &expires_in));
  ASSERT_EQ(1, oauth2_client_password_grant("user", NULL, &access_token,
                                            &expires_in));
  ASSERT_EQ(1, oauth2_client_password_grant("user", "pass", NULL, &expires_in));
  ASSERT_EQ(1,
            oauth2_client_password_grant("user", "pass", &access_token, NULL));

  /* We don't have a real server, so it should fail */
  ASSERT_EQ(1, oauth2_client_password_grant("user", "pass", &access_token,
                                            &expires_in));
  PASS();
}

TEST test_server_handlers(void) {
  c_orm_db_t *db = NULL;
  struct c_rest_request req;
  struct c_rest_response res;
  struct c_rest_header hdr;

  c_orm_sqlite_connect(":memory:", &db);

  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  /* token handler: fail if db is null */
  ASSERT_EQ(1, oauth2_token_handler(&req, &res, NULL));
  ASSERT_EQ(1, oauth2_token_handler(NULL, &res, db));
  ASSERT_EQ(1, oauth2_token_handler(&req, NULL, db));

  /* valid args but missing body */
  req.body = NULL;
  req.body_len = 0;
  ASSERT_EQ(0, oauth2_token_handler(&req, &res, db));

  /* login handler */
  ASSERT_EQ(1, oauth2_login_handler(&req, &res, NULL));
  ASSERT_EQ(0, oauth2_login_handler(&req, &res, db));

  /* logout handler */
  ASSERT_EQ(0, oauth2_logout_handler(&req, &res, db));
  hdr.key = "Authorization";
  hdr.value = "Bearer mytoken";
  hdr.next = NULL;
  req.headers = &hdr;
  ASSERT_EQ(0, oauth2_logout_handler(&req, &res, db));

  /* secret handler */
  req.headers = NULL;
  ASSERT_EQ(0, oauth2_secret_handler(&req, &res, db));
  req.headers = &hdr;
  ASSERT_EQ(0, oauth2_secret_handler(&req, &res, db));

  /* register client */
  ASSERT_EQ(1, oauth2_register_client_handler(&req, &res, NULL));
  ASSERT_EQ(0, oauth2_register_client_handler(&req, &res, db));
  req.body = "client_id=myid&client_secret=mysecret";
  req.body_len = strlen(req.body);
  ASSERT_EQ(0, oauth2_register_client_handler(&req, &res, db));

  /* register user */
  ASSERT_EQ(1, oauth2_register_user_handler(&req, &res, NULL));
  req.body = NULL;
  req.body_len = 0;
  ASSERT_EQ(0, oauth2_register_user_handler(&req, &res, db));
  req.body = "username=myuser&password=mypassword";
  req.body_len = strlen(req.body);
  ASSERT_EQ(0, oauth2_register_user_handler(&req, &res, db));

  if (db && db->vtable && db->vtable->disconnect)
    db->vtable->disconnect(db);
  PASS();
}

SUITE(oauth2_suite) {
  RUN_TEST(test_client_init_fail_nulls);
  RUN_TEST(test_server_init_fail_nulls);
  RUN_TEST(test_client_password_grant);
  RUN_TEST(test_server_handlers);
}

/* Add definitions that need to be in the test runner */
GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
  GREATEST_MAIN_BEGIN();
  RUN_SUITE(oauth2_suite);
  GREATEST_MAIN_END();
}
