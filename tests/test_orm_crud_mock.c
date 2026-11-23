/* clang-format off */
#include "c_rest_export.h"
#include "c_rest_mem.h"
#undef C_REST_EXPORT
#define C_REST_EXPORT

#include "c_rest_error.h"
#include "c_rest_modality.h"
#include "c_rest_orm.h"
#include "c_rest_orm_crud.h"
#include "greatest.h"
#include "greatest_clean.h"
#include <string.h>

#include "c_rest_response.h"

static int g_mock_res_json_fail = 0;
static int g_mock_res_status_fail = 0;

static c_rest_error_t mock_c_rest_response_json(struct c_rest_response *res, const char *json_str) {
    if (g_mock_res_json_fail) return C_REST_ERROR_GENERIC;
    return c_rest_response_json(res, json_str);
}

static c_rest_error_t mock_c_rest_response_set_status(struct c_rest_response *res, int status_code) {
    if (g_mock_res_status_fail) return C_REST_ERROR_GENERIC;
    return c_rest_response_set_status(res, status_code);
}

#define c_rest_response_json mock_c_rest_response_json
#define c_rest_response_set_status mock_c_rest_response_set_status

static int g_mock_cfs_path_init_fail = 0;
static int g_mock_cfs_exists_fail = 0;
static int g_mock_cfs_dir_init_fail = 0;
static int g_mock_cfs_dir_next_fail = 0;
static int g_mock_cfs_filename_fail = 0;
static int g_mock_cfs_cstr_fail = 0;
static int g_mock_cfs_clone_fail = 0;
static int g_mock_cfs_append_fail = 0;
static int g_mock_sqlite_conn_fail = 0;
static int g_mock_fseek_fail = 0;
static int g_mock_ftell_fail = 0;

#include <cfs/cfs.h>
#ifdef C_REST_FRAMEWORK_USE_REAL_C_ORM
#include "c_orm_db.h"
#include "c_orm_sqlite.h"
static const c_orm_driver_vtable_t *g_saved_reinit_vt = NULL;
static c_orm_driver_vtable_t g_reinit_fail_disc_vt;
static c_orm_driver_vtable_t g_reinit_null_disc_vt;
static c_orm_error_t mock_reinit_disc_fail(c_orm_db_t *db) {
  c_orm_error_t drc;
  db->vtable = g_saved_reinit_vt;
  drc = g_saved_reinit_vt->disconnect(db);
  if (drc != C_ORM_OK) {
    return drc;
  }
  return C_ORM_ERROR_CONNECTION;
}
#endif

static cfs_errc mock_cfs_path_init_str(cfs_path *p, const cfs_char_t *src) {
  if (g_mock_cfs_path_init_fail) {
    return cfs_errc_not_enough_memory;
  }
  return cfs_path_init_str(p, src);
}

static cfs_errc mock_cfs_exists_path(const cfs_path *p, cfs_bool *out,
                                     cfs_error_code *ec) {
  if (g_mock_cfs_exists_fail) {
    return cfs_errc_io_error;
  }
  return cfs_exists_path(p, out, ec);
}

static cfs_errc mock_cfs_dir_itr_init(const cfs_path *p,
                                      cfs_directory_iterator **out,
                                      cfs_error_code *ec) {
  if (g_mock_cfs_dir_init_fail) {
    return cfs_errc_permission_denied;
  }
  return cfs_dir_itr_init(p, out, ec);
}

static cfs_errc mock_cfs_dir_itr_next(cfs_directory_iterator *it,
                                      const cfs_directory_entry **out_entry,
                                      cfs_error_code *ec) {
  if (g_mock_cfs_dir_next_fail) {
    return cfs_errc_io_error;
  }
  return cfs_dir_itr_next(it, out_entry, ec);
}

static cfs_errc mock_cfs_path_filename(const cfs_path *p, cfs_path *out) {
  if (g_mock_cfs_filename_fail) {
    return cfs_errc_not_enough_memory;
  }
  return cfs_path_filename(p, out);
}

static cfs_errc mock_cfs_path_c_str(const cfs_path *p,
                                    const cfs_char_t **out) {
  if (g_mock_cfs_cstr_fail == 1) {
    return cfs_errc_invalid_argument;
  }
  if (g_mock_cfs_cstr_fail > 1) {
    g_mock_cfs_cstr_fail--;
  }
  return cfs_path_c_str(p, out);
}

static cfs_errc mock_cfs_path_clone(cfs_path *dst, const cfs_path *src) {
  if (g_mock_cfs_clone_fail) {
    return cfs_errc_not_enough_memory;
  }
  return cfs_path_clone(dst, src);
}

static cfs_errc mock_cfs_path_append(cfs_path *p, const cfs_char_t *src) {
  if (g_mock_cfs_append_fail) {
    return cfs_errc_not_enough_memory;
  }
  return cfs_path_append(p, src);
}

#ifdef C_REST_FRAMEWORK_USE_REAL_C_ORM
static c_orm_error_t mock_c_orm_sqlite_connect(const char *url,
                                               c_orm_db_t **out_db) {
  c_orm_error_t rc;
  if (g_mock_sqlite_conn_fail == 1) {
    return C_ORM_ERROR_CONNECTION;
  }
  rc = c_orm_sqlite_connect(url, out_db);
  if (rc == C_ORM_OK && *out_db != NULL) {
    if (g_mock_sqlite_conn_fail == 2) {
      g_saved_reinit_vt = (*out_db)->vtable;
      g_reinit_fail_disc_vt = *g_saved_reinit_vt;
      g_reinit_fail_disc_vt.disconnect = mock_reinit_disc_fail;
      (*out_db)->vtable = &g_reinit_fail_disc_vt;
    } else if (g_mock_sqlite_conn_fail == 3) {
      g_saved_reinit_vt = (*out_db)->vtable;
      g_reinit_null_disc_vt = *g_saved_reinit_vt;
      g_reinit_null_disc_vt.disconnect = NULL;
      (*out_db)->vtable = &g_reinit_null_disc_vt;
    } else if (g_mock_sqlite_conn_fail == 4) {
      g_saved_reinit_vt = (*out_db)->vtable;
      (*out_db)->vtable = NULL;
    } else if (g_mock_sqlite_conn_fail == 5) {
      rc = (*out_db)->vtable->disconnect(*out_db);
      *out_db = NULL;
      if (rc != C_ORM_OK) {
        return rc;
      }
    }
  }
  return rc;
}
#endif

static int mock_fseek(FILE *fp, long offset, int origin) {
  if (g_mock_fseek_fail == 1 && origin == SEEK_END) {
    return -1;
  }
  if (g_mock_fseek_fail == 2 && origin == SEEK_SET) {
    return -1;
  }
  return fseek(fp, offset, origin);
}

static long mock_ftell(FILE *fp) {
  if (g_mock_ftell_fail) {
    return -1L;
  }
  return ftell(fp);
}

#define cfs_path_init_str mock_cfs_path_init_str
#define cfs_exists_path mock_cfs_exists_path
#define cfs_dir_itr_init mock_cfs_dir_itr_init
#define cfs_dir_itr_next mock_cfs_dir_itr_next
#define cfs_path_filename mock_cfs_path_filename
#define cfs_path_c_str mock_cfs_path_c_str
#define cfs_path_clone mock_cfs_path_clone
#define cfs_path_append mock_cfs_path_append
#ifdef C_REST_FRAMEWORK_USE_REAL_C_ORM
#define c_orm_sqlite_connect mock_c_orm_sqlite_connect
#endif
#define fseek mock_fseek
#define ftell mock_ftell

#include "../src/c_orm_crud.c"

#undef fseek
#undef ftell
#ifdef C_REST_FRAMEWORK_USE_REAL_C_ORM
#undef c_orm_sqlite_connect
#endif
#undef cfs_path_append
#undef cfs_path_clone
#undef cfs_path_c_str
#undef cfs_path_filename
#undef cfs_dir_itr_next
#undef cfs_dir_itr_init
#undef cfs_exists_path
#undef cfs_path_init_str
#undef c_rest_response_json
#undef c_rest_response_set_status

/* clang-format on */

static int g_orm_alloc_fail_at = 0;
static int g_orm_alloc_count = 0;

static void *orm_fail_malloc_at(size_t size) {
  g_orm_alloc_count++;
  if (g_orm_alloc_fail_at > 0 && g_orm_alloc_count == g_orm_alloc_fail_at) {
    return NULL;
  }
  return malloc(size);
}

static void reset_mocks(void *data) {
  (void)data;
  g_mock_res_json_fail = 0;
  g_mock_res_status_fail = 0;
  g_mock_cfs_path_init_fail = 0;
  g_mock_cfs_exists_fail = 0;
  g_mock_cfs_dir_init_fail = 0;
  g_mock_cfs_dir_next_fail = 0;
  g_mock_cfs_cstr_fail = 0;
  g_mock_cfs_clone_fail = 0;
  g_mock_cfs_append_fail = 0;
  g_mock_sqlite_conn_fail = 0;
  g_mock_fseek_fail = 0;
  g_mock_ftell_fail = 0;
  g_orm_alloc_fail_at = 0;
  g_orm_alloc_count = 0;
  g_crf_malloc_hook = NULL;
}

static c_rest_error_t mock_logger_fail(const char *msg) {
  (void)msg;
  return C_REST_ERROR_GENERIC;
}

static c_rest_error_t mock_logger_ok(const char *msg) {
  (void)msg;
  return C_REST_OK;
}

TEST test_orm_crud_error_branches(void) {
  struct c_rest_request req;
  struct c_rest_response res;
  struct c_rest_orm_model model;
  struct c_rest_context ctx;
  char empty_json[] = "{}";

  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));
  memset(&model, 0, sizeof(model));
  memset(&ctx, 0, sizeof(ctx));

  req.body = empty_json;

  /* --- c_rest_orm_crud_get_list --- */
  /* model == NULL: status fail */
  g_mock_res_status_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_orm_crud_get_list(&req, &res, NULL));
  g_mock_res_status_fail = 0;
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));

  /* model == NULL: json fail */
  g_mock_res_json_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_orm_crud_get_list(&req, &res, NULL));
  g_mock_res_json_fail = 0;
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));

  /* model == NULL: default error return */
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_orm_crud_get_list(&req, &res, NULL));
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));

  /* model != NULL but req.db_conn == NULL */
  req.db_conn = NULL;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_orm_crud_get_list(&req, &res, &model));
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));

  /* valid model and req.db_conn */
  req.db_conn = (void *)1;
  g_mock_res_status_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_orm_crud_get_list(&req, &res, &model));
  g_mock_res_status_fail = 0;
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));

  g_mock_res_json_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_orm_crud_get_list(&req, &res, &model));
  g_mock_res_json_fail = 0;
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));

  ASSERT_EQ(C_REST_OK, c_rest_orm_crud_get_list(&req, &res, &model));
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));
  req.db_conn = NULL;

  /* --- c_rest_orm_crud_get_one --- */
  /* model == NULL: status fail */
  g_mock_res_status_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_orm_crud_get_one(&req, &res, NULL));
  g_mock_res_status_fail = 0;
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));

  /* model == NULL: json fail */
  g_mock_res_json_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_orm_crud_get_one(&req, &res, NULL));
  g_mock_res_json_fail = 0;
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));

  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_orm_crud_get_one(&req, &res, NULL));
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));

  /* model != NULL but req.db_conn == NULL */
  req.db_conn = NULL;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_orm_crud_get_one(&req, &res, &model));
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));

  /* valid model and req.db_conn */
  req.db_conn = (void *)1;
  g_mock_res_status_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_orm_crud_get_one(&req, &res, &model));
  g_mock_res_status_fail = 0;
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));

  g_mock_res_json_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_orm_crud_get_one(&req, &res, &model));
  g_mock_res_json_fail = 0;
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));

  ASSERT_EQ(C_REST_OK, c_rest_orm_crud_get_one(&req, &res, &model));
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));
  req.db_conn = NULL;

  /* --- c_rest_orm_crud_create --- */
  /* model == NULL: status fail */
  g_mock_res_status_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_orm_crud_create(&req, &res, NULL));
  g_mock_res_status_fail = 0;
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));

  /* model == NULL: json fail */
  g_mock_res_json_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_orm_crud_create(&req, &res, NULL));
  g_mock_res_json_fail = 0;
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));

  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_orm_crud_create(&req, &res, NULL));
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));

  /* model != NULL but req.db_conn == NULL */
  req.db_conn = NULL;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_orm_crud_create(&req, &res, &model));
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));

  /* valid model and req.db_conn */
  req.db_conn = (void *)1;
  g_mock_res_status_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_orm_crud_create(&req, &res, &model));
  g_mock_res_status_fail = 0;
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));

  g_mock_res_json_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_orm_crud_create(&req, &res, &model));
  g_mock_res_json_fail = 0;
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));

  ASSERT_EQ(C_REST_OK, c_rest_orm_crud_create(&req, &res, &model));
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));
  req.db_conn = NULL;

  /* --- c_rest_orm_crud_update --- */
  /* model == NULL: status fail */
  g_mock_res_status_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_orm_crud_update(&req, &res, NULL));
  g_mock_res_status_fail = 0;
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));

  /* model == NULL: json fail */
  g_mock_res_json_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_orm_crud_update(&req, &res, NULL));
  g_mock_res_json_fail = 0;
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));

  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_orm_crud_update(&req, &res, NULL));
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));

  /* model != NULL but req.db_conn == NULL */
  req.db_conn = NULL;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_orm_crud_update(&req, &res, &model));
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));

  /* valid model and req.db_conn */
  req.db_conn = (void *)1;
  g_mock_res_status_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_orm_crud_update(&req, &res, &model));
  g_mock_res_status_fail = 0;
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));

  g_mock_res_json_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_orm_crud_update(&req, &res, &model));
  g_mock_res_json_fail = 0;
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));

  ASSERT_EQ(C_REST_OK, c_rest_orm_crud_update(&req, &res, &model));
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));
  req.db_conn = NULL;

  /* --- c_rest_orm_crud_delete --- */
  /* model == NULL: status fail */
  g_mock_res_status_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_orm_crud_delete(&req, &res, NULL));
  g_mock_res_status_fail = 0;
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));

  /* model == NULL: json fail */
  g_mock_res_json_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_orm_crud_delete(&req, &res, NULL));
  g_mock_res_json_fail = 0;
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));

  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_orm_crud_delete(&req, &res, NULL));
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));

  /* model != NULL but req.db_conn == NULL */
  req.db_conn = NULL;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_orm_crud_delete(&req, &res, &model));
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));

  /* valid model and req.db_conn */
  req.db_conn = (void *)1;
  g_mock_res_status_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_orm_crud_delete(&req, &res, &model));
  g_mock_res_status_fail = 0;
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));

  ASSERT_EQ(C_REST_OK, c_rest_orm_crud_delete(&req, &res, &model));
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));
  req.db_conn = NULL;

  /* --- c_rest_orm_health_check --- */
  /* req == NULL */
  ASSERT_EQ(C_REST_OK, c_rest_orm_health_check(NULL, &res, NULL));
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));

  /* --- c_rest_orm_health_check --- */
  /* with db_conn */
  req.db_conn = (void *)1;
  g_mock_res_status_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_orm_health_check(&req, &res, NULL));
  g_mock_res_status_fail = 0;
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));

  g_mock_res_json_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_orm_health_check(&req, &res, NULL));
  g_mock_res_json_fail = 0;
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));

  ASSERT_EQ(C_REST_OK, c_rest_orm_health_check(&req, &res, NULL));
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));
  req.db_conn = NULL;

  /* without db_conn */
  g_mock_res_status_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_orm_health_check(&req, &res, NULL));
  g_mock_res_status_fail = 0;
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));

  g_mock_res_json_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_orm_health_check(&req, &res, NULL));
  g_mock_res_json_fail = 0;
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));

  ASSERT_EQ(C_REST_OK, c_rest_orm_health_check(&req, &res, NULL));
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));

  /* --- c_rest_orm_run_migrations --- */
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG,
            c_rest_orm_run_migrations(NULL, "migrations"));
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG,
            c_rest_orm_run_migrations(&ctx, "migrations"));
  ctx.db_pool = (struct c_orm_pool *)1;
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG, c_rest_orm_run_migrations(&ctx, NULL));

  /* logger callback failure */
  ctx.logger.log_cb = mock_logger_fail;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_orm_run_migrations(&ctx, "migrations_nonexistent_xyz"));
  ctx.logger.log_cb = NULL;

  /* cfs_path_init_str failure */
  g_mock_cfs_path_init_fail = 1;
  ASSERT_EQ(C_REST_ERROR_OOM,
            c_rest_orm_run_migrations(&ctx, "migrations_nonexistent_xyz"));
  g_mock_cfs_path_init_fail = 0;

  /* cfs_dir_itr_init failure on missing directory */
  ASSERT_EQ(C_REST_ERROR_NOT_FOUND,
            c_rest_orm_run_migrations(&ctx, "migrations_nonexistent_xyz"));

  PASS();
}

#ifdef C_REST_FRAMEWORK_USE_REAL_C_ORM
static c_orm_driver_vtable_t g_orig_sqlite_vt;
static c_orm_driver_vtable_t g_fail_disc_vt;
static int g_mock_fin_fail = 0;

static c_orm_error_t mock_disconnect_fail(c_orm_db_t *db) {
  c_orm_error_t real_rc = g_orig_sqlite_vt.disconnect(db);
  if (real_rc != C_ORM_OK) {
    return real_rc;
  }
  return C_ORM_ERROR_CONNECTION;
}

static c_orm_error_t mock_step_fail(c_orm_query_t *query, int *out_has_row) {
  (void)query;
  (void)out_has_row;
  return C_ORM_ERROR_STEP;
}

static c_orm_error_t mock_finalize_maybe_fail(c_orm_query_t *query) {
  c_orm_error_t real_rc = g_orig_sqlite_vt.finalize(query);
  if (real_rc != C_ORM_OK) {
    return real_rc;
  }
  if (g_mock_fin_fail) {
    return C_ORM_ERROR_SQL;
  }
  return C_ORM_OK;
}
#endif

static void write_test_file(const char *path, const char *content) {
  FILE *fp = NULL;
#if defined(_MSC_VER)
  fopen_s(&fp, path, "wb");
#else
  fp = fopen(path, "wb");
#endif
  if (fp) {
    if (content && content[0] != '\0') {
      fwrite(content, 1U, strlen(content), fp);
    }
    fclose(fp);
  }
}

TEST test_orm_pool_and_migrations_branches(void) {
  struct c_rest_db_config cfg;
  struct c_orm_pool *pool = NULL;
  struct c_orm_pool bad_pool;
  struct c_rest_context ctx;
  cfs_path mp = {0};
  cfs_error_code ec = {0};
  cfs_size_t removed = 0;
  int i;

  memset(&cfg, 0, sizeof(cfg));
  memset(&bad_pool, 0, sizeof(bad_pool));
  memset(&ctx, 0, sizeof(ctx));

  /* Pool init/cleanup branches */
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG, c_rest_orm_init(NULL, &pool));
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG, c_rest_orm_init(&cfg, NULL));
  ASSERT_EQ(C_REST_OK, c_rest_orm_cleanup(NULL));
  ASSERT_EQ(C_REST_OK, c_rest_orm_cleanup((struct c_orm_pool *)1));
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG,
            c_rest_orm_cleanup((struct c_orm_pool *)2));
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG, c_rest_orm_cleanup(&bad_pool));

  /* Connection string variants */
  cfg.connection_string = "postgres://unsupported";
  ASSERT_EQ(C_REST_ERROR_NOT_SUPPORTED, c_rest_orm_init(&cfg, &pool));

  cfg.connection_string = NULL;
  ASSERT_EQ(C_REST_OK, c_rest_orm_init(&cfg, &pool));
  cfg.connection_string = "";
  ASSERT_EQ(C_REST_OK, c_rest_orm_init(&cfg, &pool));
  cfg.connection_string = "sqlite://";
  ASSERT_EQ(C_REST_OK, c_rest_orm_init(&cfg, &pool));
  cfg.connection_string = "sqlite://:memory:";
  ASSERT_EQ(C_REST_OK, c_rest_orm_init(&cfg, &pool));
  cfg.connection_string = ":memory:";
  ASSERT_EQ(C_REST_OK, c_rest_orm_init(&cfg, &pool));
  ASSERT_EQ(C_REST_OK, c_rest_orm_cleanup(pool));
  pool = NULL;

  /* OOM on pool allocation */
  g_orm_alloc_count = 0;
  g_orm_alloc_fail_at = 1;
  g_crf_malloc_hook = orm_fail_malloc_at;
  ASSERT_EQ(C_REST_ERROR_OOM, c_rest_orm_init(&cfg, &pool));
  g_crf_malloc_hook = NULL;
  g_orm_alloc_fail_at = 0;

#ifdef C_REST_FRAMEWORK_USE_REAL_C_ORM
  /* sqlite connect failure */
  g_mock_sqlite_conn_fail = 1;
  ASSERT_EQ(C_REST_ERROR_DB, c_rest_orm_init(&cfg, &pool));
  g_mock_sqlite_conn_fail = 0;

  /* Re-init when existing *pool fails cleanup */
  pool = (struct c_orm_pool *)2;
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG, c_rest_orm_init(&cfg, &pool));
  pool = NULL;

  /* Re-init cleanup fail + new_pool disconnect fail / null disconnect / null
   * vtable / null db */
  pool = (struct c_orm_pool *)2;
  g_mock_sqlite_conn_fail = 2;
  ASSERT_EQ(C_REST_ERROR_DB, c_rest_orm_init(&cfg, &pool));
  g_mock_sqlite_conn_fail = 3;
  pool = (struct c_orm_pool *)2;
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG, c_rest_orm_init(&cfg, &pool));
  g_mock_sqlite_conn_fail = 4;
  pool = (struct c_orm_pool *)2;
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG, c_rest_orm_init(&cfg, &pool));
  g_mock_sqlite_conn_fail = 5;
  pool = (struct c_orm_pool *)2;
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG, c_rest_orm_init(&cfg, &pool));
  g_mock_sqlite_conn_fail = 0;
  pool = NULL;

  /* Cleanup with NULL db / NULL vtable / NULL disconnect / failing disconnect
   */
  ASSERT_EQ(C_REST_OK, c_rest_orm_init(&cfg, &pool));
  {
    c_orm_db_t *saved_db = pool->db;
    const c_orm_driver_vtable_t *saved_vt = saved_db->vtable;
    g_orig_sqlite_vt = *saved_vt;
    g_fail_disc_vt = *saved_vt;
    g_fail_disc_vt.disconnect = mock_disconnect_fail;
    saved_db->vtable = &g_fail_disc_vt;
    ASSERT_EQ(C_REST_ERROR_DB, c_rest_orm_cleanup(pool));
    pool = NULL;
  }
  ASSERT_EQ(C_REST_OK, c_rest_orm_init(&cfg, &pool));
  {
    c_orm_db_t *saved_db = pool->db;
    const c_orm_driver_vtable_t *saved_vt = saved_db->vtable;
    c_orm_driver_vtable_t null_disc_vt = *saved_vt;
    null_disc_vt.disconnect = NULL;
    saved_db->vtable = &null_disc_vt;
    ASSERT_EQ(C_REST_OK, c_rest_orm_cleanup(pool));
    saved_db->vtable = saved_vt;
    ASSERT_EQ(C_ORM_OK, saved_db->vtable->disconnect(saved_db));
    pool = NULL;
  }
  ASSERT_EQ(C_REST_OK, c_rest_orm_init(&cfg, &pool));
  {
    c_orm_db_t *saved_db = pool->db;
    const c_orm_driver_vtable_t *saved_vt = saved_db->vtable;
    saved_db->vtable = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_orm_cleanup(pool));
    saved_db->vtable = saved_vt;
    ASSERT_EQ(C_ORM_OK, saved_db->vtable->disconnect(saved_db));
    pool = NULL;
  }
  ASSERT_EQ(C_REST_OK, c_rest_orm_init(&cfg, &pool));
  {
    c_orm_db_t *saved_db = pool->db;
    pool->db = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_orm_cleanup(pool));
    ASSERT_EQ(C_ORM_OK, saved_db->vtable->disconnect(saved_db));
    pool = NULL;
  }
#endif

  /* Prepare real temporary migration directory */
#if defined(CFS_OS_WINDOWS) && defined(CFS_UNICODE)
  ASSERT_EQ(cfs_errc_success, cfs_path_init_str(&mp, L"test_mig_mock_dir"));
#else
  ASSERT_EQ(cfs_errc_success,
            cfs_path_init_str(&mp, (const cfs_char_t *)"test_mig_mock_dir"));
#endif
  {
    cfs_bool ex = 0;
    ASSERT_EQ(cfs_errc_success, cfs_exists_path(&mp, &ex, &ec));
    if (ex) {
      ASSERT_EQ(cfs_errc_success, cfs_remove_all(&mp, &removed, &ec));
    }
  }
  ASSERT_EQ(cfs_errc_success, cfs_create_directories(&mp, &ec));

  /* Empty directory with sentinel pool & logger */
  ctx.db_pool = (struct c_orm_pool *)1;
  ctx.logger.log_cb = mock_logger_ok;
  ASSERT_EQ(C_REST_OK, c_rest_orm_run_migrations(&ctx, "test_mig_mock_dir"));
  ctx.logger.log_cb = NULL;

  /* Populate >4 .sql files out of order + short/non-sql files + empty .sql */
  write_test_file("test_mig_mock_dir/005_five.sql",
                  "INSERT INTO items (name) VALUES ('five');");
  write_test_file(
      "test_mig_mock_dir/001_one.sql",
      "CREATE TABLE IF NOT EXISTS items (id INTEGER PRIMARY KEY, name TEXT);");
  write_test_file("test_mig_mock_dir/003_three.sql",
                  "INSERT INTO items (name) VALUES ('three');");
  write_test_file("test_mig_mock_dir/002_two.sql",
                  "INSERT INTO items (name) VALUES ('two');");
  write_test_file("test_mig_mock_dir/004_four.sql",
                  "INSERT INTO items (name) VALUES ('four');");
  write_test_file("test_mig_mock_dir/000_empty.sql", "");
  write_test_file("test_mig_mock_dir/readme.txt", "ignore me");

  /* Run with sentinel pool */
  ASSERT_EQ(C_REST_OK, c_rest_orm_run_migrations(&ctx, "test_mig_mock_dir"));

  /* Run with real pool */
  ASSERT_EQ(C_REST_OK, c_rest_orm_init(&cfg, &pool));
  ctx.db_pool = pool;
  ASSERT_EQ(C_REST_OK, c_rest_orm_run_migrations(&ctx, "test_mig_mock_dir"));

  /* Directory iterator / path_c_str / exists / dir_init / clone / append
   * failure branches */
  g_mock_cfs_exists_fail = 1;
  ASSERT_EQ(C_REST_ERROR_IO,
            c_rest_orm_run_migrations(&ctx, "test_mig_mock_dir"));
  g_mock_cfs_exists_fail = 0;

  g_mock_cfs_dir_init_fail = 1;
  ASSERT_EQ(C_REST_ERROR_UNAUTHORIZED,
            c_rest_orm_run_migrations(&ctx, "test_mig_mock_dir"));
  g_mock_cfs_dir_init_fail = 0;

  g_mock_cfs_dir_next_fail = 1;
  ASSERT_EQ(C_REST_ERROR_IO,
            c_rest_orm_run_migrations(&ctx, "test_mig_mock_dir"));
  g_mock_cfs_dir_next_fail = 0;

  g_mock_cfs_filename_fail = 1;
  ASSERT_EQ(C_REST_ERROR_OOM,
            c_rest_orm_run_migrations(&ctx, "test_mig_mock_dir"));
  g_mock_cfs_filename_fail = 0;

  g_mock_cfs_cstr_fail = 1;
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG,
            c_rest_orm_run_migrations(&ctx, "test_mig_mock_dir"));
  g_mock_cfs_cstr_fail = 0;

  g_mock_cfs_clone_fail = 1;
  ASSERT_EQ(C_REST_ERROR_OOM,
            c_rest_orm_run_migrations(&ctx, "test_mig_mock_dir"));
  g_mock_cfs_clone_fail = 0;

  g_mock_cfs_append_fail = 1;
  ASSERT_EQ(C_REST_ERROR_OOM,
            c_rest_orm_run_migrations(&ctx, "test_mig_mock_dir"));
  g_mock_cfs_append_fail = 0;

  g_mock_cfs_cstr_fail = 2;
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG,
            c_rest_orm_run_migrations(&ctx, "test_mig_mock_dir"));
  g_mock_cfs_cstr_fail = 0;

  /* fseek / ftell failure branches */
  g_mock_fseek_fail = 1;
  ASSERT_EQ(C_REST_ERROR_IO,
            c_rest_orm_run_migrations(&ctx, "test_mig_mock_dir"));
  g_mock_fseek_fail = 2;
  ASSERT_EQ(C_REST_ERROR_IO,
            c_rest_orm_run_migrations(&ctx, "test_mig_mock_dir"));
  g_mock_fseek_fail = 0;

  g_mock_ftell_fail = 1;
  ASSERT_EQ(C_REST_ERROR_IO,
            c_rest_orm_run_migrations(&ctx, "test_mig_mock_dir"));
  g_mock_ftell_fail = 0;

  /* OOM sweep across list insertion, array growth, and SQL buffer read */
  for (i = 1; i <= 16; ++i) {
    c_rest_error_t oom_rc;
    g_orm_alloc_count = 0;
    g_orm_alloc_fail_at = i;
    g_crf_malloc_hook = orm_fail_malloc_at;
    oom_rc = c_rest_orm_run_migrations(&ctx, "test_mig_mock_dir");
    ASSERT(oom_rc == C_REST_OK || oom_rc == C_REST_ERROR_OOM);
    g_crf_malloc_hook = NULL;
    g_orm_alloc_fail_at = 0;
  }

  /* Subdirectory ending in .sql triggers fopen failure */
  {
    cfs_path bad_sub = {0};
#if defined(CFS_OS_WINDOWS) && defined(CFS_UNICODE)
    ASSERT_EQ(cfs_errc_success,
              cfs_path_init_str(&bad_sub, L"test_mig_mock_dir/999_dir.sql"));
#else
    ASSERT_EQ(
        cfs_errc_success,
        cfs_path_init_str(&bad_sub,
                          (const cfs_char_t *)"test_mig_mock_dir/999_dir.sql"));
#endif
    ASSERT_EQ(cfs_errc_success, cfs_create_directories(&bad_sub, &ec));
    /* On POSIX fopen("dir", "rb") succeeds while on Windows it fails;
     * test execute_migration_file directly with a missing file for
     * deterministic fopen failure on all platforms! */
    ASSERT_EQ(C_REST_ERROR_IO, execute_migration_file(
                                   pool, "test_mig_mock_dir/missing_999.sql"));
    ASSERT_EQ(cfs_errc_success, cfs_remove_all(&bad_sub, &removed, &ec));
    cfs_path_destroy(&bad_sub);
  }

  /* Invalid SQL syntax triggers prepare failure */
  write_test_file("test_mig_mock_dir/999_bad.sql", "NOT VALID SQL SYNTAX;;;");
  ASSERT_EQ(C_REST_ERROR_DB,
            c_rest_orm_run_migrations(&ctx, "test_mig_mock_dir"));

#ifdef C_REST_FRAMEWORK_USE_REAL_C_ORM
  {
    const c_orm_driver_vtable_t *saved_vt = pool->db->vtable;
    c_orm_driver_vtable_t custom_vt = *saved_vt;
    g_orig_sqlite_vt = *saved_vt;

    /* step fails, finalize succeeds */
    custom_vt.step = mock_step_fail;
    custom_vt.finalize = mock_finalize_maybe_fail;
    g_mock_fin_fail = 0;
    pool->db->vtable = &custom_vt;
    ASSERT_EQ(C_REST_ERROR_DB,
              execute_migration_file(pool, "test_mig_mock_dir/001_one.sql"));

    /* step fails AND finalize fails */
    g_mock_fin_fail = 1;
    ASSERT_EQ(C_REST_ERROR_DB,
              execute_migration_file(pool, "test_mig_mock_dir/001_one.sql"));

    /* step succeeds, finalize fails */
    custom_vt.step = g_orig_sqlite_vt.step;
    g_mock_fin_fail = 1;
    ASSERT_EQ(C_REST_ERROR_DB,
              execute_migration_file(pool, "test_mig_mock_dir/001_one.sql"));
    g_mock_fin_fail = 0;

    /* NULL vtable / NULL db / bad magic in execute_migration_file */
    pool->db->vtable = NULL;
    ASSERT_EQ(C_REST_OK,
              execute_migration_file(pool, "test_mig_mock_dir/001_one.sql"));
    pool->db->vtable = saved_vt;

    {
      c_orm_db_t *saved_db_ptr = pool->db;
      pool->db = NULL;
      ASSERT_EQ(C_REST_OK,
                execute_migration_file(pool, "test_mig_mock_dir/001_one.sql"));
      pool->db = saved_db_ptr;
    }
    ASSERT_EQ(C_REST_OK, execute_migration_file(
                             &bad_pool, "test_mig_mock_dir/001_one.sql"));
  }
#endif

  /* Short relative path ("m/a", plen == 3 <= 4) branch */
  {
    cfs_path sp = {0};
#if defined(CFS_OS_WINDOWS) && defined(CFS_UNICODE)
    ASSERT_EQ(cfs_errc_success, cfs_path_init_str(&sp, L"m"));
#else
    ASSERT_EQ(cfs_errc_success,
              cfs_path_init_str(&sp, (const cfs_char_t *)"m"));
#endif
    ASSERT_EQ(cfs_errc_success, cfs_create_directories(&sp, &ec));
    write_test_file("m/a", "-- short");
    ASSERT_EQ(C_REST_OK, c_rest_orm_run_migrations(&ctx, "m"));
    ASSERT_EQ(cfs_errc_success, cfs_remove_all(&sp, &removed, &ec));
    cfs_path_destroy(&sp);
  }

  ASSERT_EQ(C_REST_OK, c_rest_orm_cleanup(pool));
  ASSERT_EQ(cfs_errc_success, cfs_remove_all(&mp, &removed, &ec));
  cfs_path_destroy(&mp);
  migration_list_free(NULL, 0U);
  PASS();
}

SUITE(orm_crud_mock_suite) {
  SET_SETUP(reset_mocks, NULL);
  RUN_TEST(test_orm_crud_error_branches);
  RUN_TEST(test_orm_pool_and_migrations_branches);
}
