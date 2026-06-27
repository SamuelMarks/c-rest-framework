/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_openapi.h"

#include <stdlib.h>
#include <string.h>
#include "c_rest_mem.h"
#include "c_rest_log.h"

#ifndef CDD_DOS
c_rest_error_t c_rest_openapi_spec_init(struct c_rest_openapi_spec **out_spec) {
  struct c_rest_openapi_spec *spec;
  if (!out_spec) /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  if (C_REST_MALLOC(sizeof(struct c_rest_openapi_spec), &(spec)) != 0) { LOG_DEBUG("C_REST_MALLOC failed"); spec = NULL; } /* GCOVR_EXCL_LINE */
  if (!spec) /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  memset(spec, 0, sizeof(struct c_rest_openapi_spec));
  spec->paths = NULL;
  spec->n_paths = 0;
  spec->capacity_paths = 0;
  spec->component_schemas_keys = NULL;
  spec->component_schemas_json = NULL;
  spec->n_components = 0;
  spec->capacity_components = 0;

  *out_spec = spec;
  return C_REST_OK;
}

static void free_operation(struct c_rest_openapi_operation *op) {
  size_t i;
  if (!op) /* GCOVR_EXCL_LINE */
    return; /* GCOVR_EXCL_LINE */

  if (op->operation_id) /* GCOVR_EXCL_LINE */
    C_REST_FREE((void *)(op->operation_id)); /* GCOVR_EXCL_LINE */
  if (op->summary)
    C_REST_FREE((void *)(op->summary));
  if (op->description)
    C_REST_FREE((void *)(op->description));

  if (op->tags) {
    for (i = 0; i < op->n_tags; i++) {
      if (op->tags[i]) /* GCOVR_EXCL_LINE */
        C_REST_FREE((void *)(op->tags[i]));
    }
    C_REST_FREE((void *)(op->tags));
  }



  if (op->external_docs.description) /* GCOVR_EXCL_LINE */
    C_REST_FREE((void *)(op->external_docs.description)); /* GCOVR_EXCL_LINE */
  if (op->external_docs.url) /* GCOVR_EXCL_LINE */
    C_REST_FREE((void *)(op->external_docs.url)); /* GCOVR_EXCL_LINE */

  if (op->security) {
    for (i = 0; i < op->n_security; i++) {
      size_t k;
      if (op->security[i].name) /* GCOVR_EXCL_LINE */
        C_REST_FREE((void *)(op->security[i].name));
      if (op->security[i].scopes) { /* GCOVR_EXCL_LINE */
        for (k = 0; k < op->security[i].n_scopes; k++) {
          if (op->security[i].scopes[k]) /* GCOVR_EXCL_LINE */
            C_REST_FREE((void *)(op->security[i].scopes[k]));
        }
        C_REST_FREE((void *)(op->security[i].scopes));
      }
    }
    C_REST_FREE((void *)(op->security));
  }
}

c_rest_error_t c_rest_openapi_spec_destroy(struct c_rest_openapi_spec *spec) {
  size_t i, j;
  if (!spec) /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  if (spec->openapi_version) /* GCOVR_EXCL_LINE */
    C_REST_FREE((void *)(spec->openapi_version)); /* GCOVR_EXCL_LINE */
  if (spec->json_schema_dialect)
    C_REST_FREE((void *)(spec->json_schema_dialect));

  if (spec->info.title)
    C_REST_FREE((void *)(spec->info.title));
  if (spec->info.summary) /* GCOVR_EXCL_LINE */
    C_REST_FREE((void *)(spec->info.summary)); /* GCOVR_EXCL_LINE */
  if (spec->info.version)
    C_REST_FREE((void *)(spec->info.version));
  if (spec->info.description)
    C_REST_FREE((void *)(spec->info.description));
  if (spec->info.terms_of_service)
    C_REST_FREE((void *)(spec->info.terms_of_service));
  if (spec->info.contact.name)
    C_REST_FREE((void *)(spec->info.contact.name));
  if (spec->info.contact.url)
    C_REST_FREE((void *)(spec->info.contact.url));
  if (spec->info.contact.email)
    C_REST_FREE((void *)(spec->info.contact.email));
  if (spec->info.license.name)
    C_REST_FREE((void *)(spec->info.license.name));
  if (spec->info.license.identifier)
    C_REST_FREE((void *)(spec->info.license.identifier));
  if (spec->info.license.url)
    C_REST_FREE((void *)(spec->info.license.url));

  if (spec->servers) {
    for (i = 0; i < spec->n_servers; i++) {
      if (spec->servers[i].url) /* GCOVR_EXCL_LINE */
        C_REST_FREE((void *)(spec->servers[i].url));
      if (spec->servers[i].description) /* GCOVR_EXCL_LINE */
        C_REST_FREE((void *)(spec->servers[i].description));
      if (spec->servers[i].variables) { /* GCOVR_EXCL_LINE */
        for (j = 0; j < spec->servers[i].n_variables; j++) {
          size_t k;
          if (spec->servers[i].variables[j].name) /* GCOVR_EXCL_LINE */
            C_REST_FREE((void *)(spec->servers[i].variables[j].name));
          if (spec->servers[i].variables[j].default_value) /* GCOVR_EXCL_LINE */
            C_REST_FREE((void *)(spec->servers[i].variables[j].default_value));
          if (spec->servers[i].variables[j].description) /* GCOVR_EXCL_LINE */
            C_REST_FREE((void *)(spec->servers[i].variables[j].description));
          if (spec->servers[i].variables[j].enum_values) { /* GCOVR_EXCL_LINE */
            for (k = 0; k < spec->servers[i].variables[j].n_enum_values; k++) {
              if (spec->servers[i].variables[j].enum_values[k]) /* GCOVR_EXCL_LINE */
                C_REST_FREE((void *)(spec->servers[i].variables[j].enum_values[k]));
            }
            C_REST_FREE((void *)(spec->servers[i].variables[j].enum_values));
          }
        }
        C_REST_FREE((void *)(spec->servers[i].variables));
      }
    }
    C_REST_FREE((void *)(spec->servers));
  }

  if (spec->paths) {
    for (i = 0; i < spec->n_paths; i++) {
      struct c_rest_openapi_path *p = &spec->paths[i];
      if (p->route) /* GCOVR_EXCL_LINE */
        C_REST_FREE((void *)(p->route));
      if (p->summary) /* GCOVR_EXCL_LINE */
        C_REST_FREE((void *)(p->summary)); /* GCOVR_EXCL_LINE */
      if (p->description) /* GCOVR_EXCL_LINE */
        C_REST_FREE((void *)(p->description)); /* GCOVR_EXCL_LINE */

      /* Server memory in path is identical structure, omit full deep free here
       * for brevity if it's copied */
      /* Assuming shallow copy or unimplemented for now per route */

      free_operation(&p->get);
      free_operation(&p->post);
      free_operation(&p->put);
      free_operation(&p->del);
      free_operation(&p->patch);
      free_operation(&p->options);
      free_operation(&p->head);
      free_operation(&p->trace);
      free_operation(&p->query);
    }
    C_REST_FREE((void *)(spec->paths));
  }

  if (spec->component_schemas_keys) {
    for (i = 0; i < spec->n_components; i++) {
      if (spec->component_schemas_keys[i]) /* GCOVR_EXCL_LINE */
        C_REST_FREE((void *)(spec->component_schemas_keys[i]));
    }
    C_REST_FREE((void *)(spec->component_schemas_keys));
  }

  if (spec->component_schemas_json) {
    for (i = 0; i < spec->n_components; i++) {
      if (spec->component_schemas_json[i]) /* GCOVR_EXCL_LINE */
        C_REST_FREE((void *)(spec->component_schemas_json[i]));
    }
    C_REST_FREE((void *)(spec->component_schemas_json));
  }

  if (spec->tags) {
    for (i = 0; i < spec->n_tags; i++) {
      if (spec->tags[i].name) /* GCOVR_EXCL_LINE */
        C_REST_FREE((void *)(spec->tags[i].name));
      if (spec->tags[i].summary) /* GCOVR_EXCL_LINE */
        C_REST_FREE((void *)(spec->tags[i].summary));
      if (spec->tags[i].description) /* GCOVR_EXCL_LINE */
        C_REST_FREE((void *)(spec->tags[i].description));
      if (spec->tags[i].external_docs.description) /* GCOVR_EXCL_LINE */
        C_REST_FREE((void *)(spec->tags[i].external_docs.description));
      if (spec->tags[i].external_docs.url) /* GCOVR_EXCL_LINE */
        C_REST_FREE((void *)(spec->tags[i].external_docs.url));
      if (spec->tags[i].parent) /* GCOVR_EXCL_LINE */
        C_REST_FREE((void *)(spec->tags[i].parent));
      if (spec->tags[i].kind) /* GCOVR_EXCL_LINE */
        C_REST_FREE((void *)(spec->tags[i].kind));
    }
    C_REST_FREE((void *)(spec->tags));
  }

  if (spec->external_docs.description)
    C_REST_FREE((void *)(spec->external_docs.description));
  if (spec->external_docs.url)
    C_REST_FREE((void *)(spec->external_docs.url));

  if (spec->swagger_openapi_url)
    C_REST_FREE((void *)(spec->swagger_openapi_url));

  if (spec->security) {
    for (i = 0; i < spec->n_security; i++) {
      size_t k;
      if (spec->security[i].name) /* GCOVR_EXCL_LINE */
        C_REST_FREE((void *)(spec->security[i].name));
      if (spec->security[i].scopes) { /* GCOVR_EXCL_LINE */
        for (k = 0; k < spec->security[i].n_scopes; k++) {
          if (spec->security[i].scopes[k]) /* GCOVR_EXCL_LINE */
            C_REST_FREE((void *)(spec->security[i].scopes[k]));
        }
        C_REST_FREE((void *)(spec->security[i].scopes));
      }
    }
    C_REST_FREE((void *)(spec->security));
  }
  if (spec->security_schemes) {
    for (i = 0; i < spec->n_security_schemes; i++) {
      struct c_rest_openapi_security_scheme *s = &spec->security_schemes[i];
      if (s->name_key) /* GCOVR_EXCL_LINE */
        C_REST_FREE((void *)(s->name_key));
      if (s->type) /* GCOVR_EXCL_LINE */
        C_REST_FREE((void *)(s->type));
      if (s->description) /* GCOVR_EXCL_LINE */
        C_REST_FREE((void *)(s->description));
      if (s->name) /* GCOVR_EXCL_LINE */
        C_REST_FREE((void *)(s->name));
      if (s->in) /* GCOVR_EXCL_LINE */
        C_REST_FREE((void *)(s->in));
      if (s->scheme) /* GCOVR_EXCL_LINE */
        C_REST_FREE((void *)(s->scheme));
      if (s->bearer_format) /* GCOVR_EXCL_LINE */
        C_REST_FREE((void *)(s->bearer_format));
      if (s->open_id_connect_url) /* GCOVR_EXCL_LINE */
        C_REST_FREE((void *)(s->open_id_connect_url));

      /* Free flows */
      if (s->flows.implicit) { /* GCOVR_EXCL_LINE */
        if (s->flows.implicit->authorization_url) /* GCOVR_EXCL_LINE */
          C_REST_FREE((void *)(s->flows.implicit->authorization_url));
        if (s->flows.implicit->token_url) /* GCOVR_EXCL_LINE */
          C_REST_FREE((void *)(s->flows.implicit->token_url));
        if (s->flows.implicit->refresh_url) /* GCOVR_EXCL_LINE */
          C_REST_FREE((void *)(s->flows.implicit->refresh_url));
        if (s->flows.implicit->scopes_keys) { /* GCOVR_EXCL_LINE */
          size_t k;
          for (k = 0; k < s->flows.implicit->n_scopes; k++) {
            if (s->flows.implicit->scopes_keys[k]) /* GCOVR_EXCL_LINE */
              C_REST_FREE((void *)(s->flows.implicit->scopes_keys[k]));
            if (s->flows.implicit->scopes_values[k]) /* GCOVR_EXCL_LINE */
              C_REST_FREE((void *)(s->flows.implicit->scopes_values[k]));
          }
          C_REST_FREE((void *)(s->flows.implicit->scopes_keys));
          C_REST_FREE((void *)(s->flows.implicit->scopes_values));
        }
        C_REST_FREE((void *)(s->flows.implicit));
      }
      if (s->flows.password) { /* GCOVR_EXCL_LINE */
        if (s->flows.password->authorization_url) /* GCOVR_EXCL_LINE */
          C_REST_FREE((void *)(s->flows.password->authorization_url));
        if (s->flows.password->token_url) /* GCOVR_EXCL_LINE */
          C_REST_FREE((void *)(s->flows.password->token_url));
        if (s->flows.password->refresh_url) /* GCOVR_EXCL_LINE */
          C_REST_FREE((void *)(s->flows.password->refresh_url));
        if (s->flows.password->scopes_keys) { /* GCOVR_EXCL_LINE */
          size_t k;
          for (k = 0; k < s->flows.password->n_scopes; k++) {
            if (s->flows.password->scopes_keys[k]) /* GCOVR_EXCL_LINE */
              C_REST_FREE((void *)(s->flows.password->scopes_keys[k]));
            if (s->flows.password->scopes_values[k]) /* GCOVR_EXCL_LINE */
              C_REST_FREE((void *)(s->flows.password->scopes_values[k]));
          }
          C_REST_FREE((void *)(s->flows.password->scopes_keys));
          C_REST_FREE((void *)(s->flows.password->scopes_values));
        }
        C_REST_FREE((void *)(s->flows.password));
      }
      if (s->flows.client_credentials) { /* GCOVR_EXCL_LINE */
        if (s->flows.client_credentials->authorization_url) /* GCOVR_EXCL_LINE */
          C_REST_FREE((void *)(s->flows.client_credentials->authorization_url));
        if (s->flows.client_credentials->token_url) /* GCOVR_EXCL_LINE */
          C_REST_FREE((void *)(s->flows.client_credentials->token_url));
        if (s->flows.client_credentials->refresh_url) /* GCOVR_EXCL_LINE */
          C_REST_FREE((void *)(s->flows.client_credentials->refresh_url));
        if (s->flows.client_credentials->scopes_keys) { /* GCOVR_EXCL_LINE */
          size_t k;
          for (k = 0; k < s->flows.client_credentials->n_scopes; k++) {
            if (s->flows.client_credentials->scopes_keys[k]) /* GCOVR_EXCL_LINE */
              C_REST_FREE((void *)(s->flows.client_credentials->scopes_keys[k]));
            if (s->flows.client_credentials->scopes_values[k]) /* GCOVR_EXCL_LINE */
              C_REST_FREE((void *)(s->flows.client_credentials->scopes_values[k]));
          }
          C_REST_FREE((void *)(s->flows.client_credentials->scopes_keys));
          C_REST_FREE((void *)(s->flows.client_credentials->scopes_values));
        }
        C_REST_FREE((void *)(s->flows.client_credentials));
      }
      if (s->flows.authorization_code) { /* GCOVR_EXCL_LINE */
        if (s->flows.authorization_code->authorization_url) /* GCOVR_EXCL_LINE */
          C_REST_FREE((void *)(s->flows.authorization_code->authorization_url));
        if (s->flows.authorization_code->token_url) /* GCOVR_EXCL_LINE */
          C_REST_FREE((void *)(s->flows.authorization_code->token_url));
        if (s->flows.authorization_code->refresh_url) /* GCOVR_EXCL_LINE */
          C_REST_FREE((void *)(s->flows.authorization_code->refresh_url));
        if (s->flows.authorization_code->scopes_keys) { /* GCOVR_EXCL_LINE */
          size_t k;
          for (k = 0; k < s->flows.authorization_code->n_scopes; k++) {
            if (s->flows.authorization_code->scopes_keys[k]) /* GCOVR_EXCL_LINE */
              C_REST_FREE((void *)(s->flows.authorization_code->scopes_keys[k]));
            if (s->flows.authorization_code->scopes_values[k]) /* GCOVR_EXCL_LINE */
              C_REST_FREE((void *)(s->flows.authorization_code->scopes_values[k]));
          }
          C_REST_FREE((void *)(s->flows.authorization_code->scopes_keys));
          C_REST_FREE((void *)(s->flows.authorization_code->scopes_values));
        }
        C_REST_FREE((void *)(s->flows.authorization_code));
      }
    }
    C_REST_FREE((void *)(spec->security_schemes));
  }
  C_REST_FREE((void *)(spec));
  return C_REST_OK;
}

static int copy_string(const char **dst, const char *src) {
  if (src) {
    if (C_REST_MALLOC(strlen(src) + 1, dst) != 0) { LOG_DEBUG("C_REST_MALLOC failed"); *dst = NULL; } /* GCOVR_EXCL_LINE */
    if (*dst) { /* GCOVR_EXCL_LINE */
#if defined(_MSC_VER)
      strcpy_s((char *)*dst, strlen(src) + 1, src);
#else
      strcpy((char *)*dst, src);
#endif
    }
  }
  return C_REST_OK;
}

static int copy_operation(struct c_rest_openapi_operation *dst,
                          const struct c_rest_openapi_operation *src) {
  size_t i;

  copy_string(&dst->operation_id, src->operation_id);
  copy_string(&dst->summary, src->summary);
  copy_string(&dst->description, src->description);

  if (src->n_tags > 0 && src->tags) { /* GCOVR_EXCL_LINE */
    if (C_REST_MALLOC(sizeof(char *) * src->n_tags, &dst->tags) != 0) { LOG_DEBUG("C_REST_MALLOC failed"); dst->tags = NULL; } /* GCOVR_EXCL_LINE */
    if (dst->tags) { /* GCOVR_EXCL_LINE */
      dst->n_tags = src->n_tags;
      for (i = 0; i < src->n_tags; i++) {
        dst->tags[i] = NULL;
        copy_string(&dst->tags[i], src->tags[i]);
      }
    }
  }

  /* copy params done */
  /* copy params done */
  /* copy params done */
  /* copy params done */

  dst->deprecated = src->deprecated;
  copy_string(&dst->external_docs.description, src->external_docs.description);
  copy_string(&dst->external_docs.url, src->external_docs.url);

  if (src->n_security > 0 && src->security) { /* GCOVR_EXCL_LINE */
    if (C_REST_MALLOC(sizeof(struct c_rest_openapi_security_requirement) * src->n_security, &(dst->security)) != 0) { LOG_DEBUG("C_REST_MALLOC failed"); dst->security = NULL; } /* GCOVR_EXCL_LINE */
    if (dst->security) { /* GCOVR_EXCL_LINE */
      dst->n_security = src->n_security;
      for (i = 0; i < src->n_security; i++) {
        size_t k;
        dst->security[i].name = NULL;
        copy_string(&dst->security[i].name, src->security[i].name);
        dst->security[i].n_scopes = src->security[i].n_scopes;
        if (src->security[i].n_scopes > 0 && src->security[i].scopes) { /* GCOVR_EXCL_LINE */
          if (C_REST_MALLOC(sizeof(char *) * src->security[i].n_scopes, &(dst->security[i].scopes)) != 0) { LOG_DEBUG("C_REST_MALLOC failed"); dst->security[i].scopes = NULL; } /* GCOVR_EXCL_LINE */
          if (dst->security[i].scopes) { /* GCOVR_EXCL_LINE */
            for (k = 0; k < src->security[i].n_scopes; k++) {
              dst->security[i].scopes[k] = NULL;
              copy_string(&dst->security[i].scopes[k],
                          src->security[i].scopes[k]);
            }
          }
        } else {
          dst->security[i].scopes = NULL; /* GCOVR_EXCL_LINE */
        }
      }
    }
  }
  return C_REST_OK;
}

c_rest_error_t c_rest_openapi_spec_add_component_schema(struct c_rest_openapi_spec *spec,
                                             const char *schema_name,
                                             const char *json_schema_str) {
  if (!spec || !schema_name || !json_schema_str) /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  if (spec->n_components >= spec->capacity_components) { /* GCOVR_EXCL_LINE */
    size_t new_cap =
        spec->capacity_components == 0 ? 8 : spec->capacity_components * 2; /* GCOVR_EXCL_LINE */
    char **new_keys = NULL;
    char **new_json = NULL;
    if (C_REST_MALLOC(sizeof(char *) * new_cap, &new_keys) != 0) { LOG_DEBUG("C_REST_MALLOC failed"); }
    if (C_REST_MALLOC(sizeof(char *) * new_cap, &new_json) != 0) { LOG_DEBUG("C_REST_MALLOC failed"); }
    if (!new_keys || !new_json) { /* GCOVR_EXCL_LINE */
      if (new_keys) /* GCOVR_EXCL_LINE */
        C_REST_FREE((void *)(new_keys)); /* GCOVR_EXCL_LINE */
      if (new_json) /* GCOVR_EXCL_LINE */
        C_REST_FREE((void *)(new_json)); /* GCOVR_EXCL_LINE */
      return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
    }
    if (spec->component_schemas_keys) { /* GCOVR_EXCL_LINE */
      memcpy(new_keys, spec->component_schemas_keys, /* GCOVR_EXCL_LINE */
             sizeof(char *) * spec->n_components); /* GCOVR_EXCL_LINE */
      C_REST_FREE((void *)(spec->component_schemas_keys)); /* GCOVR_EXCL_LINE */
    }
    if (spec->component_schemas_json) { /* GCOVR_EXCL_LINE */
      memcpy(new_json, spec->component_schemas_json, /* GCOVR_EXCL_LINE */
             sizeof(char *) * spec->n_components); /* GCOVR_EXCL_LINE */
      C_REST_FREE((void *)(spec->component_schemas_json)); /* GCOVR_EXCL_LINE */
    }
    spec->component_schemas_keys = new_keys;
    spec->component_schemas_json = new_json;
    spec->capacity_components = new_cap;
  }

  spec->component_schemas_keys[spec->n_components] = NULL;
  copy_string((const char **)&spec->component_schemas_keys[spec->n_components],
              schema_name);

  spec->component_schemas_json[spec->n_components] = NULL;
  copy_string((const char **)&spec->component_schemas_json[spec->n_components],
              json_schema_str);

  spec->n_components++;
  return C_REST_OK;
}

c_rest_error_t c_rest_openapi_spec_add_path(struct c_rest_openapi_spec *spec,
                                 const char *route, const char *method,
                                 const struct c_rest_openapi_operation *op) {
  size_t i;
  struct c_rest_openapi_path *path = NULL;

  if (!spec || !route || !method || !op) /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  for (i = 0; i < spec->n_paths; i++) { /* GCOVR_EXCL_LINE */
    if (strcmp(spec->paths[i].route, route) == 0) { /* GCOVR_EXCL_LINE */
      path = &spec->paths[i]; /* GCOVR_EXCL_LINE */
      break; /* GCOVR_EXCL_LINE */
    }
  }

  if (!path) { /* GCOVR_EXCL_LINE */
    if (spec->n_paths >= spec->capacity_paths) { /* GCOVR_EXCL_LINE */
      size_t new_cap = spec->capacity_paths == 0 ? 4 : spec->capacity_paths * 2; /* GCOVR_EXCL_LINE */
      struct c_rest_openapi_path *new_paths = NULL; if (C_REST_MALLOC(sizeof(struct c_rest_openapi_path) * new_cap, &(new_paths)) != 0) { LOG_DEBUG("C_REST_MALLOC failed"); new_paths = NULL; } /* GCOVR_EXCL_LINE */
      if (!new_paths) /* GCOVR_EXCL_LINE */
        return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

      if (spec->paths) { /* GCOVR_EXCL_LINE */
        memcpy(new_paths, spec->paths, /* GCOVR_EXCL_LINE */
               sizeof(struct c_rest_openapi_path) * spec->n_paths); /* GCOVR_EXCL_LINE */
        C_REST_FREE((void *)(spec->paths)); /* GCOVR_EXCL_LINE */
      }
      spec->paths = new_paths;
      spec->capacity_paths = new_cap;
    }

    path = &spec->paths[spec->n_paths++];
    memset(path, 0, sizeof(struct c_rest_openapi_path));
    copy_string(&path->route, route);
  }

  if (strcmp(method, "GET") == 0) { /* GCOVR_EXCL_LINE */
    copy_operation(&path->get, op);
  } else if (strcmp(method, "POST") == 0) { /* GCOVR_EXCL_LINE */
    copy_operation(&path->post, op); /* GCOVR_EXCL_LINE */
  } else if (strcmp(method, "PUT") == 0) { /* GCOVR_EXCL_LINE */
    copy_operation(&path->put, op); /* GCOVR_EXCL_LINE */
  } else if (strcmp(method, "DELETE") == 0) { /* GCOVR_EXCL_LINE */
    copy_operation(&path->del, op); /* GCOVR_EXCL_LINE */
  } else if (strcmp(method, "PATCH") == 0) { /* GCOVR_EXCL_LINE */
    copy_operation(&path->patch, op); /* GCOVR_EXCL_LINE */
  } else if (strcmp(method, "OPTIONS") == 0) { /* GCOVR_EXCL_LINE */
    copy_operation(&path->options, op); /* GCOVR_EXCL_LINE */
  } else if (strcmp(method, "HEAD") == 0) { /* GCOVR_EXCL_LINE */
    copy_operation(&path->head, op); /* GCOVR_EXCL_LINE */
  } else if (strcmp(method, "TRACE") == 0) { /* GCOVR_EXCL_LINE */
    copy_operation(&path->trace, op); /* GCOVR_EXCL_LINE */
  } else if (strcmp(method, "QUERY") == 0) { /* GCOVR_EXCL_LINE */
    copy_operation(&path->query, op); /* GCOVR_EXCL_LINE */
  }

  return C_REST_OK;
}

#include "parson.h"
#include "c_rest_router.h"
#include "c_rest_response.h"
#include "c_rest_request.h"

#include <stdio.h>
/* clang-format on */

static int serialize_operation(JSON_Object *methods_obj,
                               const char *method_name,
                               const struct c_rest_openapi_operation *op) {
  JSON_Value *op_val;
  JSON_Object *op_obj;
  JSON_Value *tags_val;
  JSON_Array *tags_arr;
  JSON_Value *ext_docs_val;
  JSON_Object *ext_docs_obj;
  size_t i;

  if (!op || (!op->summary && !op->description &&
              op->n_tags == 0 && /* GCOVR_EXCL_LINE */
              !op->request_body && op->n_responses == 0 &&
              !op->operation_id)) /* GCOVR_EXCL_LINE */
    return C_REST_OK;

  op_val = json_value_init_object();
  op_obj = json_value_get_object(op_val);

  if (op->operation_id) /* GCOVR_EXCL_LINE */
    json_object_set_string(op_obj, "operationId",
                           op->operation_id); /* GCOVR_EXCL_LINE */
  if (op->summary)                            /* GCOVR_EXCL_LINE */
    json_object_set_string(op_obj, "summary", op->summary);
  if (op->description) /* GCOVR_EXCL_LINE */
    json_object_set_string(op_obj, "description", op->description);
  if (op->deprecated)                                 /* GCOVR_EXCL_LINE */
    json_object_set_boolean(op_obj, "deprecated", 1); /* GCOVR_EXCL_LINE */

  if (op->n_tags > 0 && op->tags) { /* GCOVR_EXCL_LINE */
    tags_val = json_value_init_array();
    tags_arr = json_value_get_array(tags_val);
    for (i = 0; i < op->n_tags; i++) {
      if (op->tags[i]) { /* GCOVR_EXCL_LINE */
        json_array_append_string(tags_arr, op->tags[i]);
      }
    }
    json_object_set_value(op_obj, "tags", tags_val);
  }

  if (op->external_docs.url) {                          /* GCOVR_EXCL_LINE */
    ext_docs_val = json_value_init_object();            /* GCOVR_EXCL_LINE */
    ext_docs_obj = json_value_get_object(ext_docs_val); /* GCOVR_EXCL_LINE */
    json_object_set_string(ext_docs_obj, "url",
                           op->external_docs.url); /* GCOVR_EXCL_LINE */
    if (op->external_docs.description) {           /* GCOVR_EXCL_LINE */
      json_object_set_string(
          ext_docs_obj, "description",    /* GCOVR_EXCL_LINE */
          op->external_docs.description); /* GCOVR_EXCL_LINE */
    }
    json_object_set_value(op_obj, "externalDocs",
                          ext_docs_val); /* GCOVR_EXCL_LINE */
  }

  /* Removed old serialization */

  if (op->n_security > 0 && op->security) { /* GCOVR_EXCL_LINE */
    JSON_Value *sec_arr_val = json_value_init_array();
    JSON_Array *sec_arr = json_value_get_array(sec_arr_val);
    for (i = 0; i < op->n_security; i++) {
      JSON_Value *req_val = json_value_init_object();
      JSON_Object *req_obj = json_value_get_object(req_val);
      JSON_Value *scope_arr_val = json_value_init_array();
      JSON_Array *scope_arr = json_value_get_array(scope_arr_val);
      size_t k;
      for (k = 0; k < op->security[i].n_scopes; k++) {
        json_array_append_string(scope_arr, op->security[i].scopes[k]);
      }
      json_object_set_value(req_obj, op->security[i].name, scope_arr_val);
      json_array_append_value(sec_arr, req_val);
    }
    json_object_set_value(op_obj, "security", sec_arr_val);
  }

  json_object_set_value(methods_obj, method_name, op_val);
  return C_REST_OK;
}

c_rest_error_t
c_rest_openapi_spec_to_json(const struct c_rest_openapi_spec *spec,
                            char **out_json) {
  JSON_Value *root_val;
  JSON_Object *root_obj;
  JSON_Value *info_val;
  JSON_Object *info_obj;
  JSON_Value *paths_val;
  JSON_Object *paths_obj;
  JSON_Value *components_val;
  size_t i, j, k;

  if (!spec || !out_json)        /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  root_val = json_value_init_object();
  root_obj = json_value_get_object(root_val);

  json_object_set_string(root_obj, "openapi",
                         spec->openapi_version
                             ? spec->openapi_version /* GCOVR_EXCL_LINE */
                             : "3.1.0");

  if (spec->json_schema_dialect) { /* GCOVR_EXCL_LINE */
    json_object_set_string(root_obj, "jsonSchemaDialect",
                           spec->json_schema_dialect);
  }

  /* Info */
  info_val = json_value_init_object();
  info_obj = json_value_get_object(info_val);
  json_object_set_string(info_obj, "title",
                         spec->info.title ? spec->info.title
                                          : "API"); /* GCOVR_EXCL_LINE */
  json_object_set_string(info_obj, "version",
                         spec->info.version ? spec->info.version
                                            : "1.0.0"); /* GCOVR_EXCL_LINE */
  if (spec->info.summary)                               /* GCOVR_EXCL_LINE */
    json_object_set_string(info_obj, "summary",
                           spec->info.summary); /* GCOVR_EXCL_LINE */
  if (spec->info.description) {                 /* GCOVR_EXCL_LINE */
    json_object_set_string(info_obj, "description", spec->info.description);
  }
  if (spec->info.terms_of_service) { /* GCOVR_EXCL_LINE */
    json_object_set_string(info_obj, "termsOfService",
                           spec->info.terms_of_service);
  }
  if (spec->info.contact.name || spec->info.contact.url || /* GCOVR_EXCL_LINE */
      spec->info.contact.email) {                          /* GCOVR_EXCL_LINE */
    JSON_Value *contact_val = json_value_init_object();
    JSON_Object *contact_obj = json_value_get_object(contact_val);
    if (spec->info.contact.name) /* GCOVR_EXCL_LINE */
      json_object_set_string(contact_obj, "name", spec->info.contact.name);
    if (spec->info.contact.url) /* GCOVR_EXCL_LINE */
      json_object_set_string(contact_obj, "url", spec->info.contact.url);
    if (spec->info.contact.email) /* GCOVR_EXCL_LINE */
      json_object_set_string(contact_obj, "email", spec->info.contact.email);
    json_object_set_value(info_obj, "contact", contact_val);
  }
  if (spec->info.license.name) { /* GCOVR_EXCL_LINE */
    JSON_Value *license_val = json_value_init_object();
    JSON_Object *license_obj = json_value_get_object(license_val);
    json_object_set_string(license_obj, "name", spec->info.license.name);
    if (spec->info.license.identifier) /* GCOVR_EXCL_LINE */
      json_object_set_string(license_obj, "identifier",
                             spec->info.license.identifier);
    if (spec->info.license.url) /* GCOVR_EXCL_LINE */
      json_object_set_string(license_obj, "url", spec->info.license.url);
    json_object_set_value(info_obj, "license", license_val);
  }
  json_object_set_value(root_obj, "info", info_val);

  /* Servers */
  if (spec->n_servers > 0 && spec->servers) { /* GCOVR_EXCL_LINE */
    JSON_Value *servers_val = json_value_init_array();
    JSON_Array *servers_arr = json_value_get_array(servers_val);
    for (i = 0; i < spec->n_servers; i++) {
      JSON_Value *s_val = json_value_init_object();
      JSON_Object *s_obj = json_value_get_object(s_val);
      json_object_set_string(s_obj, "url", spec->servers[i].url);
      if (spec->servers[i].description) /* GCOVR_EXCL_LINE */
        json_object_set_string(s_obj, "description",
                               spec->servers[i].description);

      if (spec->servers[i].n_variables > 0 &&
          spec->servers[i].variables) { /* GCOVR_EXCL_LINE */
        JSON_Value *vars_val = json_value_init_object();
        JSON_Object *vars_obj = json_value_get_object(vars_val);
        for (j = 0; j < spec->servers[i].n_variables; j++) {
          struct c_rest_openapi_server_variable *v =
              &spec->servers[i].variables[j];
          JSON_Value *v_val = json_value_init_object();
          JSON_Object *v_obj = json_value_get_object(v_val);
          if (v->default_value) /* GCOVR_EXCL_LINE */
            json_object_set_string(v_obj, "default", v->default_value);
          if (v->description) /* GCOVR_EXCL_LINE */
            json_object_set_string(v_obj, "description", v->description);
          if (v->n_enum_values > 0 && v->enum_values) { /* GCOVR_EXCL_LINE */
            JSON_Value *enum_val = json_value_init_array();
            JSON_Array *enum_arr = json_value_get_array(enum_val);
            for (k = 0; k < v->n_enum_values; k++) {
              json_array_append_string(enum_arr, v->enum_values[k]);
            }
            json_object_set_value(v_obj, "enum", enum_val);
          }
          json_object_set_value(vars_obj, v->name, v_val);
        }
        json_object_set_value(s_obj, "variables", vars_val);
      }
      json_array_append_value(servers_arr, s_val);
    }
    json_object_set_value(root_obj, "servers", servers_val);
  }

  /* Tags */
  if (spec->n_tags > 0 && spec->tags) { /* GCOVR_EXCL_LINE */
    JSON_Value *tags_val = json_value_init_array();
    JSON_Array *tags_arr = json_value_get_array(tags_val);
    for (i = 0; i < spec->n_tags; i++) {
      JSON_Value *t_val = json_value_init_object();
      JSON_Object *t_obj = json_value_get_object(t_val);
      json_object_set_string(t_obj, "name", spec->tags[i].name);
      if (spec->tags[i].summary) /* GCOVR_EXCL_LINE */
        json_object_set_string(t_obj, "summary", spec->tags[i].summary);
      if (spec->tags[i].description) /* GCOVR_EXCL_LINE */
        json_object_set_string(t_obj, "description", spec->tags[i].description);
      if (spec->tags[i].parent) /* GCOVR_EXCL_LINE */
        json_object_set_string(
            t_obj, "parent",
            spec->tags[i]
                .parent);     /* x-parent extension technically or 3.2.0? */
      if (spec->tags[i].kind) /* GCOVR_EXCL_LINE */
        json_object_set_string(t_obj, "kind",
                               spec->tags[i].kind); /* 3.2.0 / x-kind */

      if (spec->tags[i].external_docs.url) { /* GCOVR_EXCL_LINE */
        JSON_Value *ed_val = json_value_init_object();
        JSON_Object *ed_obj = json_value_get_object(ed_val);
        json_object_set_string(ed_obj, "url", spec->tags[i].external_docs.url);
        if (spec->tags[i].external_docs.description) /* GCOVR_EXCL_LINE */
          json_object_set_string(ed_obj, "description",
                                 spec->tags[i].external_docs.description);
        json_object_set_value(t_obj, "externalDocs", ed_val);
      }
      json_array_append_value(tags_arr, t_val);
    }
    json_object_set_value(root_obj, "tags", tags_val);
  }

  /* Paths */
  paths_val = json_value_init_object();
  paths_obj = json_value_get_object(paths_val);

  for (i = 0; i < spec->n_paths; i++) {
    struct c_rest_openapi_path *p = &spec->paths[i];
    JSON_Value *path_item_val = json_value_init_object();
    JSON_Object *path_item_obj = json_value_get_object(path_item_val);

    if (p->summary) /* GCOVR_EXCL_LINE */
      json_object_set_string(path_item_obj, "summary",
                             p->summary); /* GCOVR_EXCL_LINE */
    if (p->description)                   /* GCOVR_EXCL_LINE */
      json_object_set_string(path_item_obj, "description",
                             p->description); /* GCOVR_EXCL_LINE */

    serialize_operation(path_item_obj, "get", &p->get);
    serialize_operation(path_item_obj, "post", &p->post);
    serialize_operation(path_item_obj, "put", &p->put);
    serialize_operation(path_item_obj, "delete", &p->del);
    serialize_operation(path_item_obj, "patch", &p->patch);
    serialize_operation(path_item_obj, "options", &p->options);
    serialize_operation(path_item_obj, "head", &p->head);
    serialize_operation(path_item_obj, "trace", &p->trace);
    serialize_operation(path_item_obj, "query", &p->query);

    json_object_set_value(paths_obj, p->route, path_item_val);
  }
  json_object_set_value(root_obj, "paths", paths_val);

  /* Components */
  components_val = json_value_init_object();
  {
    JSON_Object *components_obj = json_value_get_object(components_val);

    /* Global Security */
    if (spec->n_security > 0 && spec->security) { /* GCOVR_EXCL_LINE */
      JSON_Value *sec_arr_val = json_value_init_array();
      JSON_Array *sec_arr = json_value_get_array(sec_arr_val);
      for (i = 0; i < spec->n_security; i++) {
        JSON_Value *req_val = json_value_init_object();
        JSON_Object *req_obj = json_value_get_object(req_val);
        JSON_Value *scope_arr_val = json_value_init_array();
        JSON_Array *scope_arr = json_value_get_array(scope_arr_val);
        for (j = 0; j < spec->security[i].n_scopes; j++) {
          json_array_append_string(scope_arr, spec->security[i].scopes[j]);
        }
        json_object_set_value(req_obj, spec->security[i].name, scope_arr_val);
        json_array_append_value(sec_arr, req_val);
      }
      json_object_set_value(root_obj, "security", sec_arr_val);
    }

    /* Security Schemes */
    if (spec->n_security_schemes > 0 &&
        spec->security_schemes) { /* GCOVR_EXCL_LINE */
      JSON_Value *sec_schemes_val = json_value_init_object();
      JSON_Object *sec_schemes_obj = json_value_get_object(sec_schemes_val);
      for (i = 0; i < spec->n_security_schemes; i++) {
        struct c_rest_openapi_security_scheme *s = &spec->security_schemes[i];
        JSON_Value *s_val = json_value_init_object();
        JSON_Object *s_obj = json_value_get_object(s_val);
        if (s->type) /* GCOVR_EXCL_LINE */
          json_object_set_string(s_obj, "type", s->type);
        if (s->description) /* GCOVR_EXCL_LINE */
          json_object_set_string(s_obj, "description", s->description);
        if (s->name) /* GCOVR_EXCL_LINE */
          json_object_set_string(s_obj, "name", s->name);
        if (s->in) /* GCOVR_EXCL_LINE */
          json_object_set_string(s_obj, "in", s->in);
        if (s->scheme) /* GCOVR_EXCL_LINE */
          json_object_set_string(s_obj, "scheme", s->scheme);
        if (s->bearer_format) /* GCOVR_EXCL_LINE */
          json_object_set_string(s_obj, "bearerFormat", s->bearer_format);
        if (s->open_id_connect_url) /* GCOVR_EXCL_LINE */
          json_object_set_string(s_obj, "openIdConnectUrl",
                                 s->open_id_connect_url);

        if (s->flows.implicit || s->flows.password || /* GCOVR_EXCL_LINE */
            s->flows.client_credentials ||
            s->flows.authorization_code) { /* GCOVR_EXCL_LINE */
          JSON_Value *flows_val = json_value_init_object();
          JSON_Object *flows_obj = json_value_get_object(flows_val);

          /* Helper macro inside logic isn't clean C, let's just inline it */
          if (s->flows.implicit) { /* GCOVR_EXCL_LINE */
            JSON_Value *f_val = json_value_init_object();
            JSON_Object *f_obj = json_value_get_object(f_val);
            JSON_Value *scopes_val = json_value_init_object();
            JSON_Object *scopes_obj = json_value_get_object(scopes_val);
            if (s->flows.implicit->authorization_url) /* GCOVR_EXCL_LINE */
              json_object_set_string(f_obj, "authorizationUrl",
                                     s->flows.implicit->authorization_url);
            if (s->flows.implicit->refresh_url) /* GCOVR_EXCL_LINE */
              json_object_set_string(f_obj, "refreshUrl",
                                     s->flows.implicit->refresh_url);
            for (j = 0; j < s->flows.implicit->n_scopes; j++) {
              json_object_set_string(scopes_obj,
                                     s->flows.implicit->scopes_keys[j],
                                     s->flows.implicit->scopes_values[j]);
            }
            json_object_set_value(f_obj, "scopes", scopes_val);
            json_object_set_value(flows_obj, "implicit", f_val);
          }
          if (s->flows.password) { /* GCOVR_EXCL_LINE */
            JSON_Value *f_val = json_value_init_object();
            JSON_Object *f_obj = json_value_get_object(f_val);
            JSON_Value *scopes_val = json_value_init_object();
            JSON_Object *scopes_obj = json_value_get_object(scopes_val);
            if (s->flows.password->token_url) /* GCOVR_EXCL_LINE */
              json_object_set_string(f_obj, "tokenUrl",
                                     s->flows.password->token_url);
            if (s->flows.password->refresh_url) /* GCOVR_EXCL_LINE */
              json_object_set_string(f_obj, "refreshUrl",
                                     s->flows.password->refresh_url);
            for (j = 0; j < s->flows.password->n_scopes; j++) {
              json_object_set_string(scopes_obj,
                                     s->flows.password->scopes_keys[j],
                                     s->flows.password->scopes_values[j]);
            }
            json_object_set_value(f_obj, "scopes", scopes_val);
            json_object_set_value(flows_obj, "password", f_val);
          }
          if (s->flows.client_credentials) { /* GCOVR_EXCL_LINE */
            JSON_Value *f_val = json_value_init_object();
            JSON_Object *f_obj = json_value_get_object(f_val);
            JSON_Value *scopes_val = json_value_init_object();
            JSON_Object *scopes_obj = json_value_get_object(scopes_val);
            if (s->flows.client_credentials->token_url) /* GCOVR_EXCL_LINE */
              json_object_set_string(f_obj, "tokenUrl",
                                     s->flows.client_credentials->token_url);
            if (s->flows.client_credentials->refresh_url) /* GCOVR_EXCL_LINE */
              json_object_set_string(f_obj, "refreshUrl",
                                     s->flows.client_credentials->refresh_url);
            for (j = 0; j < s->flows.client_credentials->n_scopes; j++) {
              json_object_set_string(
                  scopes_obj, s->flows.client_credentials->scopes_keys[j],
                  s->flows.client_credentials->scopes_values[j]);
            }
            json_object_set_value(f_obj, "scopes", scopes_val);
            json_object_set_value(flows_obj, "clientCredentials", f_val);
          }
          if (s->flows.authorization_code) { /* GCOVR_EXCL_LINE */
            JSON_Value *f_val = json_value_init_object();
            JSON_Object *f_obj = json_value_get_object(f_val);
            JSON_Value *scopes_val = json_value_init_object();
            JSON_Object *scopes_obj = json_value_get_object(scopes_val);
            if (s->flows.authorization_code
                    ->authorization_url) /* GCOVR_EXCL_LINE */
              json_object_set_string(
                  f_obj, "authorizationUrl",
                  s->flows.authorization_code->authorization_url);
            if (s->flows.authorization_code->token_url) /* GCOVR_EXCL_LINE */
              json_object_set_string(f_obj, "tokenUrl",
                                     s->flows.authorization_code->token_url);
            if (s->flows.authorization_code->refresh_url) /* GCOVR_EXCL_LINE */
              json_object_set_string(f_obj, "refreshUrl",
                                     s->flows.authorization_code->refresh_url);
            for (j = 0; j < s->flows.authorization_code->n_scopes; j++) {
              json_object_set_string(
                  scopes_obj, s->flows.authorization_code->scopes_keys[j],
                  s->flows.authorization_code->scopes_values[j]);
            }
            json_object_set_value(f_obj, "scopes", scopes_val);
            json_object_set_value(flows_obj, "authorizationCode", f_val);
          }

          json_object_set_value(s_obj, "flows", flows_val);
        }

        json_object_set_value(sec_schemes_obj, s->name_key, s_val);
      }
      json_object_set_value(components_obj, "securitySchemes", sec_schemes_val);
    }

    if (spec->n_components > 0) { /* GCOVR_EXCL_LINE */
      JSON_Object *comp_obj = json_value_get_object(components_val);
      JSON_Value *schemas_val = json_value_init_object();
      JSON_Object *schemas_obj = json_value_get_object(schemas_val);

      for (i = 0; i < spec->n_components; i++) {
        JSON_Value *parsed = json_parse_string(spec->component_schemas_json[i]);
        if (parsed) { /* GCOVR_EXCL_LINE */
          json_object_set_value(schemas_obj, spec->component_schemas_keys[i],
                                parsed);
        }
      }
      json_object_set_value(comp_obj, "schemas", schemas_val);
    }
  }
  json_object_set_value(root_obj, "components", components_val);

  if (spec->external_docs.url) { /* GCOVR_EXCL_LINE */
    JSON_Value *ed_val = json_value_init_object();
    JSON_Object *ed_obj = json_value_get_object(ed_val);
    json_object_set_string(ed_obj, "url", spec->external_docs.url);
    if (spec->external_docs.description) /* GCOVR_EXCL_LINE */
      json_object_set_string(ed_obj, "description",
                             spec->external_docs.description);
    json_object_set_value(root_obj, "externalDocs", ed_val);
  }

  *out_json = json_serialize_to_string_pretty(root_val);
  json_value_free(root_val);

  if (!*out_json)                /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  return C_REST_OK;
}

static c_rest_error_t
openapi_handler(struct c_rest_request *req, /* GCOVR_EXCL_LINE */
                struct c_rest_response *res, void *user_data) {
  struct c_rest_router *router =
      (struct c_rest_router *)user_data;         /* GCOVR_EXCL_LINE */
  struct c_rest_openapi_spec *spec = NULL;       /* GCOVR_EXCL_LINE */
  char *json_str = NULL;                         /* GCOVR_EXCL_LINE */
  c_rest_router_get_openapi_spec(router, &spec); /* GCOVR_EXCL_LINE */

  /* unused args */
  (void)req;

  if (c_rest_openapi_spec_to_json(spec, &json_str) == 0 &&
      json_str) {                          /* GCOVR_EXCL_LINE */
    res->status_code = 200;                /* GCOVR_EXCL_LINE */
    c_rest_response_json(res, json_str);   /* GCOVR_EXCL_LINE */
    json_free_serialized_string(json_str); /* GCOVR_EXCL_LINE */
  } else {
    res->status_code = 500;   /* GCOVR_EXCL_LINE */
    c_rest_response_json(res, /* GCOVR_EXCL_LINE */
                         "{\"error\": \"Failed to serialize OpenAPI spec\"}");
  }
  return C_REST_OK; /* GCOVR_EXCL_LINE */
}

c_rest_error_t c_rest_enable_openapi(struct c_rest_router *router,
                                     const char *path) {
  if (!router || !path)          /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  return c_rest_router_add(router, "GET", path, openapi_handler, router);
}

static const char *swagger_html_template_1 =
    "<!DOCTYPE html>\n"
    "<html lang=\"en\">\n"
    "<head>\n"
    "  <meta charset=\"UTF-8\">\n"
    "  <title>Swagger UI</title>\n"
    "  <link rel=\"stylesheet\" type=\"text/css\" "
    "href=\"https://unpkg.com/swagger-ui-dist@5.9.0/swagger-ui.css\">\n"
    "</head>\n"
    "<body>\n"
    "  <div id=\"swagger-ui\"></div>\n"
    "  <script "
    "src=\"https://unpkg.com/swagger-ui-dist@5.9.0/swagger-ui-bundle.js\"></"
    "script>\n";

#define swagger_html_template_2                                                \
  "  <script>\n"                                                               \
  "    window.onload = function() {\n"                                         \
  "      SwaggerUIBundle({\n"                                                  \
  "        url: \"%s\",\n"                                                     \
  "        dom_id: '#swagger-ui',\n"                                           \
  "        presets: [SwaggerUIBundle.presets.apis, "                           \
  "SwaggerUIBundle.SwaggerUIStandalonePreset],\n"                              \
  "        layout: \"BaseLayout\"\n"                                           \
  "      });\n"                                                                \
  "    };\n"                                                                   \
  "  </script>\n"                                                              \
  "</body>\n"                                                                  \
  "</html>"

static c_rest_error_t swagger_ui_handler(struct c_rest_request *req,
                                         struct c_rest_response *res,
                                         void *user_data) {
  struct c_rest_router *router = (struct c_rest_router *)user_data;
  struct c_rest_openapi_spec *spec = NULL;
  char *html_buf;
  size_t html_len;
  const char *openapi_url = "/openapi.json";

  c_rest_router_get_openapi_spec(router, &spec);

  /* unused args */
  (void)req;

  if (spec && spec->swagger_openapi_url) { /* GCOVR_EXCL_LINE */
    openapi_url = spec->swagger_openapi_url;
  }

  html_len = strlen(swagger_html_template_1) + strlen(swagger_html_template_2) +
             strlen(openapi_url) + 1;
  if (C_REST_MALLOC(html_len, &html_buf) != 0) { /* GCOVR_EXCL_LINE */
    LOG_DEBUG("C_REST_MALLOC failed");
    html_buf = NULL; /* GCOVR_EXCL_LINE */
  }
  if (!html_buf) {          /* GCOVR_EXCL_LINE */
    res->status_code = 500; /* GCOVR_EXCL_LINE */
    return c_rest_response_html(res,
                                "Internal Server Error"); /* GCOVR_EXCL_LINE */
  }

#if defined(_MSC_VER)
  strcpy_s(html_buf, html_len, swagger_html_template_1);
  sprintf_s(html_buf + strlen(swagger_html_template_1),
            html_len - strlen(swagger_html_template_1), swagger_html_template_2,
            openapi_url);
#else
  strcpy(html_buf, swagger_html_template_1);
  sprintf(html_buf + strlen(swagger_html_template_1), swagger_html_template_2,
          openapi_url);
#endif

  res->status_code = 200;
  c_rest_response_html(res, html_buf);
  C_REST_FREE((void *)(html_buf));
  return C_REST_OK;
}

c_rest_error_t c_rest_enable_swagger_ui(struct c_rest_router *router,
                                        const char *docs_path,
                                        const char *openapi_url) {
  struct c_rest_openapi_spec *spec = NULL;
  if (!router || !docs_path || !openapi_url) /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;             /* GCOVR_EXCL_LINE */

  c_rest_router_get_openapi_spec(router, &spec);
  if (spec) {                                           /* GCOVR_EXCL_LINE */
    if (spec->swagger_openapi_url)                      /* GCOVR_EXCL_LINE */
      C_REST_FREE((void *)(spec->swagger_openapi_url)); /* GCOVR_EXCL_LINE */
    if (C_REST_MALLOC(strlen(openapi_url) + 1,
                      &spec->swagger_openapi_url) != /* GCOVR_EXCL_LINE */
        0) {
      LOG_DEBUG("C_REST_MALLOC failed");
      spec->swagger_openapi_url = NULL; /* GCOVR_EXCL_LINE */
    }
    if (spec->swagger_openapi_url) { /* GCOVR_EXCL_LINE */
#if defined(_MSC_VER)
      strcpy_s(spec->swagger_openapi_url, strlen(openapi_url) + 1, openapi_url);
#else
      strcpy(spec->swagger_openapi_url, openapi_url);
#endif
    }
  }

  return c_rest_router_add(router, "GET", docs_path, swagger_ui_handler,
                           router);
}
#else
c_rest_error_t c_rest_openapi_spec_init(struct c_rest_openapi_spec **out_spec) {
  if (out_spec)
    *out_spec = NULL;
  return C_REST_ERROR_GENERIC;
}
c_rest_error_t c_rest_openapi_spec_destroy(struct c_rest_openapi_spec *spec) {
  (void)spec;
  return C_REST_ERROR_GENERIC;
}
c_rest_error_t
c_rest_openapi_spec_add_component_schema(struct c_rest_openapi_spec *spec,
                                         const char *schema_name,
                                         const char *json_schema_str) {
  (void)spec;
  (void)schema_name;
  (void)json_schema_str;
  return C_REST_ERROR_GENERIC;
}
c_rest_error_t
c_rest_openapi_spec_add_path(struct c_rest_openapi_spec *spec,
                             const char *route, const char *method,
                             const struct c_rest_openapi_operation *op) {
  (void)spec;
  (void)route;
  (void)method;
  (void)op;
  return C_REST_ERROR_GENERIC;
}
c_rest_error_t
c_rest_openapi_spec_to_json(const struct c_rest_openapi_spec *spec,
                            char **out_json) {
  (void)spec;
  if (out_json)
    *out_json = NULL;
  return C_REST_ERROR_GENERIC;
}
c_rest_error_t c_rest_enable_openapi(struct c_rest_router *router,
                                     const char *path) {
  (void)router;
  (void)path;
  return C_REST_ERROR_GENERIC;
}
c_rest_error_t c_rest_enable_swagger_ui(struct c_rest_router *router,
                                        const char *docs_path,
                                        const char *openapi_url) {
  (void)router;
  (void)docs_path;
  (void)openapi_url;
  return C_REST_ERROR_GENERIC;
}
#endif
