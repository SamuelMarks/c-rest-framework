/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "c_rest_modality.h"
#include "c_rest_orm.h"
#include "c_rest_orm_crud.h"
#include "c_rest_request.h"
#include "c_rest_response.h"
#include <cfs/cfs.h>
#ifdef C_REST_FRAMEWORK_USE_REAL_C_ORM
#include "c_orm_db.h"
#include "c_orm_sqlite.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* clang-format on */

#define C_REST_ORM_POOL_MAGIC 0x43524650UL

struct c_orm_pool {
  unsigned long magic;
#ifdef C_REST_FRAMEWORK_USE_REAL_C_ORM
  c_orm_db_t *db;
#else
  void *db;
#endif
};

c_rest_error_t c_rest_orm_init(struct c_rest_db_config *config,
                               struct c_orm_pool **pool) {
  struct c_orm_pool *new_pool = NULL;
  const char *sqlite_path = ":memory:";
  c_rest_error_t rc;
#ifdef C_REST_FRAMEWORK_USE_REAL_C_ORM
  c_orm_db_t *db = NULL;
  c_orm_error_t orm_rc;
#endif

  if (!config || !pool) {
    return C_REST_ERROR_INVALID_ARG;
  }

  if (config->connection_string != NULL &&
      config->connection_string[0] != '\0') {
    if (strncmp(config->connection_string, "sqlite://", 9) == 0) {
      if (config->connection_string[9] != '\0') {
        sqlite_path = config->connection_string + 9;
      }
    } else if (strstr(config->connection_string, "://") != NULL) {
      return C_REST_ERROR_NOT_SUPPORTED;
    } else {
      sqlite_path = config->connection_string;
    }
  }

  rc = C_REST_MALLOC(sizeof(struct c_orm_pool), (void **)&new_pool);
  if (rc != C_REST_OK) {
    return rc;
  }
  memset(new_pool, 0, sizeof(struct c_orm_pool));

#ifdef C_REST_FRAMEWORK_USE_REAL_C_ORM
  orm_rc = c_orm_sqlite_connect(sqlite_path, &db);
  if (orm_rc != C_ORM_OK) {
    C_REST_FREE(new_pool);
    return c_rest_error_from_orm((int)orm_rc);
  }
  new_pool->db = db;
#else
  (void)sqlite_path;
#endif
  new_pool->magic = C_REST_ORM_POOL_MAGIC;

  if (*pool != NULL) {
    c_rest_error_t clean_rc = c_rest_orm_cleanup(*pool);
    if (clean_rc != C_REST_OK) {
#ifdef C_REST_FRAMEWORK_USE_REAL_C_ORM
      if (new_pool->db && new_pool->db->vtable &&
          new_pool->db->vtable->disconnect) {
        c_orm_error_t disc_rc = new_pool->db->vtable->disconnect(new_pool->db);
        if (disc_rc != C_ORM_OK) {
          C_REST_FREE(new_pool);
          return c_rest_error_from_orm((int)disc_rc);
        }
      }
#endif
      C_REST_FREE(new_pool);
      return clean_rc;
    }
  }

  *pool = new_pool;
  return C_REST_OK;
}

c_rest_error_t c_rest_orm_cleanup(struct c_orm_pool *pool) {
  if (!pool || pool == (struct c_orm_pool *)1) {
    return C_REST_OK;
  }
  if ((size_t)pool <= 4096U || pool->magic != C_REST_ORM_POOL_MAGIC) {
    return C_REST_ERROR_INVALID_ARG;
  }
#ifdef C_REST_FRAMEWORK_USE_REAL_C_ORM
  if (pool->db && pool->db->vtable && pool->db->vtable->disconnect) {
    c_orm_error_t orm_rc = pool->db->vtable->disconnect(pool->db);
    if (orm_rc != C_ORM_OK) {
      pool->magic = 0;
      C_REST_FREE(pool);
      return c_rest_error_from_orm((int)orm_rc);
    }
  }
#endif
  pool->magic = 0;
  C_REST_FREE(pool);
  return C_REST_OK;
}

c_rest_error_t c_rest_orm_crud_get_list(struct c_rest_request *req,
                                        struct c_rest_response *res,
                                        void *user_data) {
  c_rest_error_t rc;
  struct c_rest_orm_model *model = (struct c_rest_orm_model *)user_data;
  (void)req;

  if (!model || !req->db_conn) {
    rc = c_rest_response_set_status(res, 500);
    if (rc != C_REST_OK)
      return rc;
    rc = c_rest_response_json(res, "{\"error\": \"Internal Server Error\"}");
    if (rc != C_REST_OK)
      return rc;
    return C_REST_ERROR_GENERIC;
  }

  /* Stub: SELECT * FROM model->table_name */
  rc = c_rest_response_set_status(res, 200);
  if (rc != C_REST_OK)
    return rc;
  rc = c_rest_response_json(res, "[]");
  if (rc != C_REST_OK)
    return rc;
  return C_REST_OK;
}

c_rest_error_t c_rest_orm_crud_get_one(struct c_rest_request *req,
                                       struct c_rest_response *res,
                                       void *user_data) {
  c_rest_error_t rc;
  struct c_rest_orm_model *model = (struct c_rest_orm_model *)user_data;

  if (!model || !req->db_conn) {
    rc = c_rest_response_set_status(res, 500);
    if (rc != C_REST_OK)
      return rc;
    return C_REST_ERROR_GENERIC;
  }

  /* Stub: SELECT * FROM model->table_name WHERE model->primary_key =
   * req->path_vars */
  rc = c_rest_response_set_status(res, 200);
  if (rc != C_REST_OK)
    return rc;
  rc = c_rest_response_json(res, "{}");
  if (rc != C_REST_OK)
    return rc;
  return C_REST_OK;
}

c_rest_error_t c_rest_orm_crud_create(struct c_rest_request *req,
                                      struct c_rest_response *res,
                                      void *user_data) {
  c_rest_error_t rc;
  struct c_rest_orm_model *model = (struct c_rest_orm_model *)user_data;

  if (!model || !req->db_conn) {
    rc = c_rest_response_set_status(res, 500);
    if (rc != C_REST_OK)
      return rc;
    return C_REST_ERROR_GENERIC;
  }

  /* Stub: INSERT INTO model->table_name VALUES (req->body) */
  rc = c_rest_response_set_status(res, 201);
  if (rc != C_REST_OK)
    return rc;
  rc = c_rest_response_json(res, "{\"status\": \"created\"}");
  if (rc != C_REST_OK)
    return rc;
  return C_REST_OK;
}

c_rest_error_t c_rest_orm_crud_update(struct c_rest_request *req,
                                      struct c_rest_response *res,
                                      void *user_data) {
  c_rest_error_t rc;
  struct c_rest_orm_model *model = (struct c_rest_orm_model *)user_data;

  if (!model || !req->db_conn) {
    rc = c_rest_response_set_status(res, 500);
    if (rc != C_REST_OK)
      return rc;
    return C_REST_ERROR_GENERIC;
  }

  /* Stub: UPDATE model->table_name SET req->body WHERE id = req->path_vars */
  rc = c_rest_response_set_status(res, 200);
  if (rc != C_REST_OK)
    return rc;
  rc = c_rest_response_json(res, "{\"status\": \"updated\"}");
  if (rc != C_REST_OK)
    return rc;
  return C_REST_OK;
}

c_rest_error_t c_rest_orm_crud_delete(struct c_rest_request *req,
                                      struct c_rest_response *res,
                                      void *user_data) {
  c_rest_error_t rc;
  struct c_rest_orm_model *model = (struct c_rest_orm_model *)user_data;

  if (!model || !req->db_conn) {
    rc = c_rest_response_set_status(res, 500);
    if (rc != C_REST_OK)
      return rc;
    return C_REST_ERROR_GENERIC;
  }

  /* Stub: DELETE FROM model->table_name WHERE id = req->path_vars */
  rc = c_rest_response_set_status(res, 204);
  if (rc != C_REST_OK)
    return rc;
  return C_REST_OK;
}

c_rest_error_t c_rest_orm_health_check(struct c_rest_request *req,
                                       struct c_rest_response *res,
                                       void *user_data) {
  c_rest_error_t rc;
  (void)req;
  (void)user_data;

  if (req && req->db_conn) {
    /* Stub: SELECT 1 to verify health */
    rc = c_rest_response_set_status(res, 200);
    if (rc != C_REST_OK)
      return rc;
    rc = c_rest_response_json(res, "{\"status\": \"healthy\"}");
    if (rc != C_REST_OK)
      return rc;
  } else {
    rc = c_rest_response_set_status(res, 503);
    if (rc != C_REST_OK)
      return rc;
    rc =
        c_rest_response_json(res, "{\"status\": \"unhealthy\", \"error\": \"No "
                                  "database connection\"}");
    if (rc != C_REST_OK)
      return rc;
  }
  return C_REST_OK;
}

static void migration_list_free(char **files, size_t count) {
  size_t i;
  if (!files) {
    return;
  }
  for (i = 0; i < count; ++i) {
    C_REST_FREE(files[i]);
  }
  C_REST_FREE(files);
}

static NO_DISCARD c_rest_error_t migration_list_insert(char ***io_files,
                                                       size_t *io_count,
                                                       size_t *io_cap,
                                                       const char *filepath) {
  size_t len = strlen(filepath);
  char *dup_str = NULL;
  size_t pos;
  c_rest_error_t rc;

  if (*io_count == *io_cap) {
    size_t new_cap = (*io_cap == 0) ? 4U : (*io_cap * 2U);
    char **new_arr = NULL;
    rc = C_REST_MALLOC(new_cap * sizeof(char *), (void **)&new_arr);
    if (rc != C_REST_OK) {
      return rc;
    }
    if (*io_files) {
#if defined(_MSC_VER)
      memcpy_s(new_arr, new_cap * sizeof(char *), *io_files,
               (*io_count) * sizeof(char *));
#else
      memcpy(new_arr, *io_files, (*io_count) * sizeof(char *));
#endif
      C_REST_FREE(*io_files);
    }
    *io_files = new_arr;
    *io_cap = new_cap;
  }

  rc = C_REST_MALLOC(len + 1U, (void **)&dup_str);
  if (rc != C_REST_OK) {
    return rc;
  }
#if defined(_MSC_VER)
  strcpy_s(dup_str, len + 1U, filepath);
#else
  strcpy(dup_str, filepath);
#endif

  pos = *io_count;
  while (pos > 0U && strcmp((*io_files)[pos - 1U], dup_str) > 0) {
    (*io_files)[pos] = (*io_files)[pos - 1U];
    pos--;
  }
  (*io_files)[pos] = dup_str;
  *io_count = (*io_count) + 1U;
  return C_REST_OK;
}

static NO_DISCARD c_rest_error_t execute_migration_file(struct c_orm_pool *pool,
                                                        const char *filepath) {
  FILE *fp = NULL;
  long fsize;
  size_t read_n;
  char *sql_buf = NULL;
  c_rest_error_t rc;

#if defined(_MSC_VER)
  if (fopen_s(&fp, filepath, "rb") != 0 || !fp) {
    return C_REST_ERROR_IO;
  }
#else
  fp = fopen(filepath, "rb");
  if (!fp) {
    return C_REST_ERROR_IO;
  }
#endif

  if (fseek(fp, 0L, SEEK_END) != 0) {
    fclose(fp);
    return C_REST_ERROR_IO;
  }
  fsize = ftell(fp);
  if (fsize < 0L || fseek(fp, 0L, SEEK_SET) != 0) {
    fclose(fp);
    return C_REST_ERROR_IO;
  }

  rc = C_REST_MALLOC((size_t)fsize + 1U, (void **)&sql_buf);
  if (rc != C_REST_OK) {
    fclose(fp);
    return rc;
  }

  read_n = (fsize > 0L) ? fread(sql_buf, 1U, (size_t)fsize, fp) : 0U;
  sql_buf[read_n] = '\0';
  fclose(fp);

#ifdef C_REST_FRAMEWORK_USE_REAL_C_ORM
  if ((size_t)pool > 4096U && pool->magic == C_REST_ORM_POOL_MAGIC &&
      pool->db != NULL && pool->db->vtable != NULL && read_n > 0U) {
    c_orm_query_t *query = NULL;
    int has_row = 0;
    c_orm_error_t orm_rc = pool->db->vtable->prepare(pool->db, sql_buf, &query);
    if (orm_rc != C_ORM_OK) {
      C_REST_FREE(sql_buf);
      return c_rest_error_from_orm((int)orm_rc);
    }
    orm_rc = pool->db->vtable->step(query, &has_row);
    if (orm_rc != C_ORM_OK) {
      c_orm_error_t fin_err;
      C_REST_FREE(sql_buf);
      fin_err = pool->db->vtable->finalize(query);
      if (fin_err != C_ORM_OK) {
        return c_rest_error_from_orm((int)fin_err);
      }
      return c_rest_error_from_orm((int)orm_rc);
    }
    orm_rc = pool->db->vtable->finalize(query);
    if (orm_rc != C_ORM_OK) {
      C_REST_FREE(sql_buf);
      return c_rest_error_from_orm((int)orm_rc);
    }
  }
#else
  (void)pool;
#endif

  C_REST_FREE(sql_buf);
  return C_REST_OK;
}

c_rest_error_t c_rest_orm_run_migrations(struct c_rest_context *ctx,
                                         const char *migration_dir) {
  c_rest_error_t rc;
  cfs_path dir_p = {0};
  cfs_directory_iterator *it = NULL;
  const cfs_directory_entry *entry = NULL;
  cfs_error_code ec = {0};
  cfs_errc cfs_rc;
  char **files = NULL;
  size_t count = 0U;
  size_t cap = 0U;
  size_t i;
#if defined(CFS_OS_WINDOWS) && defined(CFS_UNICODE)
  wchar_t wdir[CFS_MAX_PATH];
  cfs_size_t req_len = 0;
#endif

  if (!ctx || !ctx->db_pool || !migration_dir) {
    return C_REST_ERROR_INVALID_ARG;
  }

  if (ctx->logger.log_cb) {
    rc = ctx->logger.log_cb("Executing database migrations...");
    if (rc != C_REST_OK) {
      return rc;
    }
  }

#if defined(CFS_OS_WINDOWS) && defined(CFS_UNICODE)
  cfs_rc = cfs_utf8_to_utf16(migration_dir, wdir,
                             sizeof(wdir) / sizeof(wdir[0]), &req_len);
  if (cfs_rc != cfs_errc_success) {
    return c_rest_error_from_cfs((int)cfs_rc);
  }
  cfs_rc = cfs_path_init_str(&dir_p, wdir);
  if (cfs_rc != cfs_errc_success) {
    return c_rest_error_from_cfs((int)cfs_rc);
  }
#else
  cfs_rc = cfs_path_init_str(&dir_p, (const cfs_char_t *)migration_dir);
  if (cfs_rc != cfs_errc_success) {
    return c_rest_error_from_cfs((int)cfs_rc);
  }
#endif

  {
    cfs_bool exists = 0;
    cfs_rc = cfs_exists_path(&dir_p, &exists, &ec);
    if (cfs_rc != cfs_errc_success) {
      cfs_path_destroy(&dir_p);
      return c_rest_error_from_cfs((int)cfs_rc);
    }
    if (!exists) {
      cfs_path_destroy(&dir_p);
      return C_REST_ERROR_NOT_FOUND;
    }
  }

  cfs_rc = cfs_dir_itr_init(&dir_p, &it, &ec);
  if (cfs_rc != cfs_errc_success) {
    cfs_path_destroy(&dir_p);
    return c_rest_error_from_cfs((int)cfs_rc);
  }

  for (;;) {
    const cfs_char_t *raw_name = NULL;
    const cfs_char_t *raw_full = NULL;
    cfs_path fn_p = {0};
    cfs_path full_p = {0};
    const char *utf8_ptr = NULL;
    size_t plen;
#if defined(CFS_OS_WINDOWS) && defined(CFS_UNICODE)
    char utf8_buf[CFS_MAX_PATH * 3];
    cfs_size_t u8_len = 0;
#endif

    cfs_rc = cfs_dir_itr_next(it, &entry, &ec);
    if (cfs_rc != cfs_errc_success) {
      cfs_dir_itr_close(it);
      cfs_path_destroy(&dir_p);
      migration_list_free(files, count);
      return c_rest_error_from_cfs((int)cfs_rc);
    }
    if (!entry) {
      break;
    }

    cfs_rc = cfs_path_filename(&entry->path, &fn_p);
    if (cfs_rc != cfs_errc_success) {
      cfs_dir_itr_close(it);
      cfs_path_destroy(&dir_p);
      migration_list_free(files, count);
      return c_rest_error_from_cfs((int)cfs_rc);
    }

    cfs_rc = cfs_path_c_str(&fn_p, &raw_name);
    if (cfs_rc != cfs_errc_success) {
      cfs_path_destroy(&fn_p);
      cfs_dir_itr_close(it);
      cfs_path_destroy(&dir_p);
      migration_list_free(files, count);
      return c_rest_error_from_cfs((int)cfs_rc);
    }

    cfs_rc = cfs_path_clone(&full_p, &dir_p);
    if (cfs_rc != cfs_errc_success) {
      cfs_path_destroy(&fn_p);
      cfs_dir_itr_close(it);
      cfs_path_destroy(&dir_p);
      migration_list_free(files, count);
      return c_rest_error_from_cfs((int)cfs_rc);
    }

    cfs_rc = cfs_path_append(&full_p, raw_name);
    if (cfs_rc != cfs_errc_success) {
      cfs_path_destroy(&fn_p);
      cfs_path_destroy(&full_p);
      cfs_dir_itr_close(it);
      cfs_path_destroy(&dir_p);
      migration_list_free(files, count);
      return c_rest_error_from_cfs((int)cfs_rc);
    }
    cfs_path_destroy(&fn_p);

    cfs_rc = cfs_path_c_str(&full_p, &raw_full);
    if (cfs_rc != cfs_errc_success) {
      cfs_path_destroy(&full_p);
      cfs_dir_itr_close(it);
      cfs_path_destroy(&dir_p);
      migration_list_free(files, count);
      return c_rest_error_from_cfs((int)cfs_rc);
    }

#if defined(CFS_OS_WINDOWS) && defined(CFS_UNICODE)
    cfs_rc = cfs_utf16_to_utf8(raw_full, utf8_buf, sizeof(utf8_buf), &u8_len);
    if (cfs_rc != cfs_errc_success) {
      cfs_path_destroy(&full_p);
      cfs_dir_itr_close(it);
      cfs_path_destroy(&dir_p);
      migration_list_free(files, count);
      return c_rest_error_from_cfs((int)cfs_rc);
    }
    utf8_ptr = utf8_buf;
#else
    utf8_ptr = (const char *)raw_full;
#endif

    plen = strlen(utf8_ptr);
    if (plen > 4U && strcmp(utf8_ptr + plen - 4U, ".sql") == 0) {
      rc = migration_list_insert(&files, &count, &cap, utf8_ptr);
      if (rc != C_REST_OK) {
        cfs_path_destroy(&full_p);
        cfs_dir_itr_close(it);
        cfs_path_destroy(&dir_p);
        migration_list_free(files, count);
        return rc;
      }
    }
    cfs_path_destroy(&full_p);
  }

  cfs_dir_itr_close(it);
  cfs_path_destroy(&dir_p);

  for (i = 0U; i < count; ++i) {
    rc = execute_migration_file(ctx->db_pool, files[i]);
    if (rc != C_REST_OK) {
      migration_list_free(files, count);
      return rc;
    }
  }

  migration_list_free(files, count);
  return C_REST_OK;
}
