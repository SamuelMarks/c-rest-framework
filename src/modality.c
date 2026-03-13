/* clang-format off */
#include "c_rest_modality.h"

#include <stdlib.h>
#include <stdio.h>
/* clang-format on */

extern const struct c_rest_modality_vtable sync_vtable;
extern const struct c_rest_modality_vtable single_thread_vtable;
extern const struct c_rest_modality_vtable async_vtable;
extern const struct c_rest_modality_vtable multi_thread_vtable;
extern const struct c_rest_modality_vtable multi_process_vtable;
extern const struct c_rest_modality_vtable greenthread_vtable;
extern const struct c_rest_modality_vtable message_passing_vtable;

/* We will reuse a dummy vtable for the unimplemented modalities for now */
static int dummy_init(struct c_rest_context *ctx) {
  if (ctx && ctx->logger.log_cb) {
    ctx->logger.log_cb("Initializing Dummy modality");
  }
  return 0;
}

static int dummy_destroy(struct c_rest_context *ctx) {
  if (ctx && ctx->logger.log_cb) {
    ctx->logger.log_cb("Destroying Dummy modality");
  }
  return 0;
}

static int dummy_run(struct c_rest_context *ctx) {
  (void)ctx;
  return 0;
}

static const struct c_rest_modality_vtable dummy_vtable = {
    dummy_init, dummy_destroy, dummy_run};

static int get_vtable(enum c_rest_modality_type type,
                      const struct c_rest_modality_vtable **out_vtable) {
  if (!out_vtable)
    return 1;

  switch (type) {
  case C_REST_MODALITY_SYNC:
    *out_vtable = &sync_vtable;
    return 0;
  case C_REST_MODALITY_SINGLE_THREAD:
    *out_vtable = &single_thread_vtable;
    return 0;
  case C_REST_MODALITY_ASYNC:
    *out_vtable = &async_vtable;
    return 0;
  case C_REST_MODALITY_MULTI_THREAD:
    *out_vtable = &multi_thread_vtable;
    return 0;
  case C_REST_MODALITY_MULTI_PROCESS:
    *out_vtable = &multi_process_vtable;
    return 0;
  case C_REST_MODALITY_GREENTHREAD:
    *out_vtable = &greenthread_vtable;
    return 0;
  case C_REST_MODALITY_MESSAGE_PASSING:
    *out_vtable = &message_passing_vtable;
    return 0;
  case C_REST_MODALITY_SINGLE_PROCESS:
    *out_vtable = &dummy_vtable;
    return 0;
  default:
    *out_vtable = NULL;
    return 1;
  }
}

int c_rest_init(enum c_rest_modality_type type,
                struct c_rest_context **out_ctx) {
  struct c_rest_context *ctx;
  const struct c_rest_modality_vtable *vtable;
  int res;

  if (!out_ctx) {
    return 1;
  }

  *out_ctx = NULL;

  if (get_vtable(type, &vtable) != 0 || !vtable) {
    return 1; /* Unsupported modality or invalid enum */
  }

  ctx = (struct c_rest_context *)malloc(sizeof(struct c_rest_context));
  if (!ctx) {
    return 1; /* Out of memory */
  }

  ctx->modality = type;
  ctx->allocator.malloc_cb = malloc;
  ctx->allocator.free_cb = free;
  ctx->logger.log_cb =
      NULL; /* No default logger to avoid implicit stdout dependency if not
               desired, but we can allow users to set it later */
  ctx->vtable = vtable;
  ctx->internal_state = NULL;

  /* Initialize db fields to zero */
  ctx->db_config.connection_string = NULL;
  ctx->db_config.max_connections = 0;
  ctx->db_config.min_connections = 0;
  ctx->db_config.connect_timeout_ms = 0;
  ctx->db_pool = NULL;

#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
  ctx->cm_env = NULL;
#endif

  res = ctx->vtable->init(ctx);
  if (res != 0) {
    free(ctx);
    return res;
  }

  *out_ctx = ctx;
  return 0;
}

int c_rest_run(struct c_rest_context *ctx) {
  int res;
  if (!ctx) {
    return 1;
  }

  /* Initialize c-orm connection pool if configured */
  if (ctx->db_config.connection_string != NULL) {
    if (ctx->logger.log_cb) {
      ctx->logger.log_cb("Initializing c-orm database connection pool...");
    }
    res = c_rest_orm_init(&ctx->db_config, &ctx->db_pool);
    if (res != 0) {
      if (ctx->logger.log_cb) {
        ctx->logger.log_cb("Failed to initialize database connection pool.");
      }
      return res;
    }
  }

  if (ctx->vtable && ctx->vtable->run) {
    return ctx->vtable->run(ctx);
  }

  if (ctx->logger.log_cb) {
    ctx->logger.log_cb("No run loop implemented for the selected modality.");
  }
  return 1;
}

int c_rest_destroy(struct c_rest_context *ctx) {
  int res = 0;
  if (!ctx) {
    return 1;
  }

  if (ctx->db_pool) {
    if (ctx->logger.log_cb) {
      ctx->logger.log_cb("Cleaning up database connection pool...");
    }
    c_rest_orm_cleanup(ctx->db_pool);
    ctx->db_pool = NULL;
  }

  if (ctx->vtable && ctx->vtable->destroy) {
    res = ctx->vtable->destroy(ctx);
  }

  if (ctx->allocator.free_cb) {
    ctx->allocator.free_cb(ctx);
  } else {
    free(ctx);
  }

  return res;
}

#ifdef C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION
int c_rest_set_multiplatform_env(struct c_rest_context *ctx, cm_env_t env) {
  struct cm_allocator alloc;
  struct cm_logger logger;

  if (!ctx || !env) {
    return 1;
  }

  ctx->cm_env = env;

  alloc.malloc_cb = ctx->allocator.malloc_cb;
  alloc.free_cb = ctx->allocator.free_cb;
  cm_env_set_allocator(env, &alloc);

  logger.log_cb = ctx->logger.log_cb;
  cm_env_set_logger(env, &logger);

  return 0;
}
#endif
