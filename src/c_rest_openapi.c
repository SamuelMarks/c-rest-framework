/* clang-format off */
#include "c_rest_openapi.h"

#include <stdlib.h>
#include <string.h>

#ifndef CDD_DOS
int c_rest_openapi_spec_init(struct c_rest_openapi_spec **out_spec) {
  struct c_rest_openapi_spec *spec;
  if (!out_spec)
    return 1;

  spec =
      (struct c_rest_openapi_spec *)malloc(sizeof(struct c_rest_openapi_spec));
  if (!spec)
    return 1;

  memset(spec, 0, sizeof(struct c_rest_openapi_spec));
  spec->paths = NULL;
  spec->n_paths = 0;
  spec->capacity_paths = 0;
  spec->component_schemas_keys = NULL;
  spec->component_schemas_json = NULL;
  spec->n_components = 0;
  spec->capacity_components = 0;

  *out_spec = spec;
  return 0;
}

static void free_operation(struct c_rest_openapi_operation *op) {
  size_t i;
  if (!op)
    return;

  if (op->operation_id)
    free((void *)op->operation_id);
  if (op->summary)
    free((void *)op->summary);
  if (op->description)
    free((void *)op->description);

  if (op->tags) {
    for (i = 0; i < op->n_tags; i++) {
      if (op->tags[i])
        free((void *)op->tags[i]);
    }
    free((void *)op->tags);
  }



  if (op->external_docs.description)
    free((void *)op->external_docs.description);
  if (op->external_docs.url)
    free((void *)op->external_docs.url);

  if (op->security) {
    for (i = 0; i < op->n_security; i++) {
      size_t k;
      if (op->security[i].name)
        free((void *)op->security[i].name);
      if (op->security[i].scopes) {
        for (k = 0; k < op->security[i].n_scopes; k++) {
          if (op->security[i].scopes[k])
            free((void *)op->security[i].scopes[k]);
        }
        free(op->security[i].scopes);
      }
    }
    free(op->security);
  }
}

int c_rest_openapi_spec_destroy(struct c_rest_openapi_spec *spec) {
  size_t i, j;
  if (!spec)
    return 1;

  if (spec->openapi_version)
    free((void *)spec->openapi_version);
  if (spec->json_schema_dialect)
    free((void *)spec->json_schema_dialect);

  if (spec->info.title)
    free((void *)spec->info.title);
  if (spec->info.summary)
    free((void *)spec->info.summary);
  if (spec->info.version)
    free((void *)spec->info.version);
  if (spec->info.description)
    free((void *)spec->info.description);
  if (spec->info.terms_of_service)
    free((void *)spec->info.terms_of_service);
  if (spec->info.contact.name)
    free((void *)spec->info.contact.name);
  if (spec->info.contact.url)
    free((void *)spec->info.contact.url);
  if (spec->info.contact.email)
    free((void *)spec->info.contact.email);
  if (spec->info.license.name)
    free((void *)spec->info.license.name);
  if (spec->info.license.identifier)
    free((void *)spec->info.license.identifier);
  if (spec->info.license.url)
    free((void *)spec->info.license.url);

  if (spec->servers) {
    for (i = 0; i < spec->n_servers; i++) {
      if (spec->servers[i].url)
        free((void *)spec->servers[i].url);
      if (spec->servers[i].description)
        free((void *)spec->servers[i].description);
      if (spec->servers[i].variables) {
        for (j = 0; j < spec->servers[i].n_variables; j++) {
          size_t k;
          if (spec->servers[i].variables[j].name)
            free((void *)spec->servers[i].variables[j].name);
          if (spec->servers[i].variables[j].default_value)
            free((void *)spec->servers[i].variables[j].default_value);
          if (spec->servers[i].variables[j].description)
            free((void *)spec->servers[i].variables[j].description);
          if (spec->servers[i].variables[j].enum_values) {
            for (k = 0; k < spec->servers[i].variables[j].n_enum_values; k++) {
              if (spec->servers[i].variables[j].enum_values[k])
                free((void *)spec->servers[i].variables[j].enum_values[k]);
            }
            free(spec->servers[i].variables[j].enum_values);
          }
        }
        free(spec->servers[i].variables);
      }
    }
    free(spec->servers);
  }

  if (spec->paths) {
    for (i = 0; i < spec->n_paths; i++) {
      struct c_rest_openapi_path *p = &spec->paths[i];
      if (p->route)
        free((void *)p->route);
      if (p->summary)
        free((void *)p->summary);
      if (p->description)
        free((void *)p->description);

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
    free(spec->paths);
  }

  if (spec->component_schemas_keys) {
    for (i = 0; i < spec->n_components; i++) {
      if (spec->component_schemas_keys[i])
        free(spec->component_schemas_keys[i]);
    }
    free(spec->component_schemas_keys);
  }

  if (spec->component_schemas_json) {
    for (i = 0; i < spec->n_components; i++) {
      if (spec->component_schemas_json[i])
        free(spec->component_schemas_json[i]);
    }
    free(spec->component_schemas_json);
  }

  if (spec->tags) {
    for (i = 0; i < spec->n_tags; i++) {
      if (spec->tags[i].name)
        free((void *)spec->tags[i].name);
      if (spec->tags[i].summary)
        free((void *)spec->tags[i].summary);
      if (spec->tags[i].description)
        free((void *)spec->tags[i].description);
      if (spec->tags[i].external_docs.description)
        free((void *)spec->tags[i].external_docs.description);
      if (spec->tags[i].external_docs.url)
        free((void *)spec->tags[i].external_docs.url);
      if (spec->tags[i].parent)
        free((void *)spec->tags[i].parent);
      if (spec->tags[i].kind)
        free((void *)spec->tags[i].kind);
    }
    free(spec->tags);
  }

  if (spec->external_docs.description)
    free((void *)spec->external_docs.description);
  if (spec->external_docs.url)
    free((void *)spec->external_docs.url);

  if (spec->swagger_openapi_url)
    free(spec->swagger_openapi_url);

  if (spec->security) {
    for (i = 0; i < spec->n_security; i++) {
      size_t k;
      if (spec->security[i].name)
        free((void *)spec->security[i].name);
      if (spec->security[i].scopes) {
        for (k = 0; k < spec->security[i].n_scopes; k++) {
          if (spec->security[i].scopes[k])
            free((void *)spec->security[i].scopes[k]);
        }
        free(spec->security[i].scopes);
      }
    }
    free(spec->security);
  }
  if (spec->security_schemes) {
    for (i = 0; i < spec->n_security_schemes; i++) {
      struct c_rest_openapi_security_scheme *s = &spec->security_schemes[i];
      if (s->name_key)
        free((void *)s->name_key);
      if (s->type)
        free((void *)s->type);
      if (s->description)
        free((void *)s->description);
      if (s->name)
        free((void *)s->name);
      if (s->in)
        free((void *)s->in);
      if (s->scheme)
        free((void *)s->scheme);
      if (s->bearer_format)
        free((void *)s->bearer_format);
      if (s->open_id_connect_url)
        free((void *)s->open_id_connect_url);

      /* Free flows */
      if (s->flows.implicit) {
        if (s->flows.implicit->authorization_url)
          free((void *)s->flows.implicit->authorization_url);
        if (s->flows.implicit->token_url)
          free((void *)s->flows.implicit->token_url);
        if (s->flows.implicit->refresh_url)
          free((void *)s->flows.implicit->refresh_url);
        if (s->flows.implicit->scopes_keys) {
          size_t k;
          for (k = 0; k < s->flows.implicit->n_scopes; k++) {
            if (s->flows.implicit->scopes_keys[k])
              free((void *)s->flows.implicit->scopes_keys[k]);
            if (s->flows.implicit->scopes_values[k])
              free((void *)s->flows.implicit->scopes_values[k]);
          }
          free(s->flows.implicit->scopes_keys);
          free(s->flows.implicit->scopes_values);
        }
        free(s->flows.implicit);
      }
      if (s->flows.password) {
        if (s->flows.password->authorization_url)
          free((void *)s->flows.password->authorization_url);
        if (s->flows.password->token_url)
          free((void *)s->flows.password->token_url);
        if (s->flows.password->refresh_url)
          free((void *)s->flows.password->refresh_url);
        if (s->flows.password->scopes_keys) {
          size_t k;
          for (k = 0; k < s->flows.password->n_scopes; k++) {
            if (s->flows.password->scopes_keys[k])
              free((void *)s->flows.password->scopes_keys[k]);
            if (s->flows.password->scopes_values[k])
              free((void *)s->flows.password->scopes_values[k]);
          }
          free(s->flows.password->scopes_keys);
          free(s->flows.password->scopes_values);
        }
        free(s->flows.password);
      }
      if (s->flows.client_credentials) {
        if (s->flows.client_credentials->authorization_url)
          free((void *)s->flows.client_credentials->authorization_url);
        if (s->flows.client_credentials->token_url)
          free((void *)s->flows.client_credentials->token_url);
        if (s->flows.client_credentials->refresh_url)
          free((void *)s->flows.client_credentials->refresh_url);
        if (s->flows.client_credentials->scopes_keys) {
          size_t k;
          for (k = 0; k < s->flows.client_credentials->n_scopes; k++) {
            if (s->flows.client_credentials->scopes_keys[k])
              free((void *)s->flows.client_credentials->scopes_keys[k]);
            if (s->flows.client_credentials->scopes_values[k])
              free((void *)s->flows.client_credentials->scopes_values[k]);
          }
          free(s->flows.client_credentials->scopes_keys);
          free(s->flows.client_credentials->scopes_values);
        }
        free(s->flows.client_credentials);
      }
      if (s->flows.authorization_code) {
        if (s->flows.authorization_code->authorization_url)
          free((void *)s->flows.authorization_code->authorization_url);
        if (s->flows.authorization_code->token_url)
          free((void *)s->flows.authorization_code->token_url);
        if (s->flows.authorization_code->refresh_url)
          free((void *)s->flows.authorization_code->refresh_url);
        if (s->flows.authorization_code->scopes_keys) {
          size_t k;
          for (k = 0; k < s->flows.authorization_code->n_scopes; k++) {
            if (s->flows.authorization_code->scopes_keys[k])
              free((void *)s->flows.authorization_code->scopes_keys[k]);
            if (s->flows.authorization_code->scopes_values[k])
              free((void *)s->flows.authorization_code->scopes_values[k]);
          }
          free(s->flows.authorization_code->scopes_keys);
          free(s->flows.authorization_code->scopes_values);
        }
        free(s->flows.authorization_code);
      }
    }
    free(spec->security_schemes);
  }
  free(spec);
  return 0;
}

static int copy_string(const char **dst, const char *src) {
  if (src) {
    *dst = (char *)malloc(strlen(src) + 1);
    if (*dst) {
#if defined(_MSC_VER)
      strcpy_s((char *)*dst, strlen(src) + 1, src);
#else
      strcpy((char *)*dst, src);
#endif
    }
  }
  return 0;
}

static int copy_operation(struct c_rest_openapi_operation *dst,
                          const struct c_rest_openapi_operation *src) {
  size_t i;

  copy_string(&dst->operation_id, src->operation_id);
  copy_string(&dst->summary, src->summary);
  copy_string(&dst->description, src->description);

  if (src->n_tags > 0 && src->tags) {
    dst->tags = (const char **)malloc(sizeof(char *) * src->n_tags);
    if (dst->tags) {
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

  if (src->n_security > 0 && src->security) {
    dst->security = (struct c_rest_openapi_security_requirement *)malloc(
        sizeof(struct c_rest_openapi_security_requirement) * src->n_security);
    if (dst->security) {
      dst->n_security = src->n_security;
      for (i = 0; i < src->n_security; i++) {
        size_t k;
        dst->security[i].name = NULL;
        copy_string(&dst->security[i].name, src->security[i].name);
        dst->security[i].n_scopes = src->security[i].n_scopes;
        if (src->security[i].n_scopes > 0 && src->security[i].scopes) {
          dst->security[i].scopes =
              (const char **)malloc(sizeof(char *) * src->security[i].n_scopes);
          if (dst->security[i].scopes) {
            for (k = 0; k < src->security[i].n_scopes; k++) {
              dst->security[i].scopes[k] = NULL;
              copy_string(&dst->security[i].scopes[k],
                          src->security[i].scopes[k]);
            }
          }
        } else {
          dst->security[i].scopes = NULL;
        }
      }
    }
  }
  return 0;
}

int c_rest_openapi_spec_add_component_schema(struct c_rest_openapi_spec *spec,
                                             const char *schema_name,
                                             const char *json_schema_str) {
  if (!spec || !schema_name || !json_schema_str)
    return 1;

  if (spec->n_components >= spec->capacity_components) {
    size_t new_cap =
        spec->capacity_components == 0 ? 8 : spec->capacity_components * 2;
    char **new_keys = (char **)malloc(sizeof(char *) * new_cap);
    char **new_json = (char **)malloc(sizeof(char *) * new_cap);
    if (!new_keys || !new_json) {
      if (new_keys)
        free(new_keys);
      if (new_json)
        free(new_json);
      return 1;
    }
    if (spec->component_schemas_keys) {
      memcpy(new_keys, spec->component_schemas_keys,
             sizeof(char *) * spec->n_components);
      free(spec->component_schemas_keys);
    }
    if (spec->component_schemas_json) {
      memcpy(new_json, spec->component_schemas_json,
             sizeof(char *) * spec->n_components);
      free(spec->component_schemas_json);
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
  return 0;
}

int c_rest_openapi_spec_add_path(struct c_rest_openapi_spec *spec,
                                 const char *route, const char *method,
                                 const struct c_rest_openapi_operation *op) {
  size_t i;
  struct c_rest_openapi_path *path = NULL;

  if (!spec || !route || !method || !op)
    return 1;

  for (i = 0; i < spec->n_paths; i++) {
    if (strcmp(spec->paths[i].route, route) == 0) {
      path = &spec->paths[i];
      break;
    }
  }

  if (!path) {
    if (spec->n_paths >= spec->capacity_paths) {
      size_t new_cap = spec->capacity_paths == 0 ? 4 : spec->capacity_paths * 2;
      struct c_rest_openapi_path *new_paths =
          (struct c_rest_openapi_path *)malloc(
              sizeof(struct c_rest_openapi_path) * new_cap);
      if (!new_paths)
        return 1;

      if (spec->paths) {
        memcpy(new_paths, spec->paths,
               sizeof(struct c_rest_openapi_path) * spec->n_paths);
        free(spec->paths);
      }
      spec->paths = new_paths;
      spec->capacity_paths = new_cap;
    }

    path = &spec->paths[spec->n_paths++];
    memset(path, 0, sizeof(struct c_rest_openapi_path));
    copy_string(&path->route, route);
  }

  if (strcmp(method, "GET") == 0) {
    copy_operation(&path->get, op);
  } else if (strcmp(method, "POST") == 0) {
    copy_operation(&path->post, op);
  } else if (strcmp(method, "PUT") == 0) {
    copy_operation(&path->put, op);
  } else if (strcmp(method, "DELETE") == 0) {
    copy_operation(&path->del, op);
  } else if (strcmp(method, "PATCH") == 0) {
    copy_operation(&path->patch, op);
  } else if (strcmp(method, "OPTIONS") == 0) {
    copy_operation(&path->options, op);
  } else if (strcmp(method, "HEAD") == 0) {
    copy_operation(&path->head, op);
  } else if (strcmp(method, "TRACE") == 0) {
    copy_operation(&path->trace, op);
  } else if (strcmp(method, "QUERY") == 0) {
    copy_operation(&path->query, op);
  }

  return 0;
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

  if (!op || (!op->summary && !op->description && op->n_tags == 0 &&
              !op->request_body && op->n_responses == 0 && !op->operation_id))
    return 0;

  op_val = json_value_init_object();
  op_obj = json_value_get_object(op_val);

  if (op->operation_id)
    json_object_set_string(op_obj, "operationId", op->operation_id);
  if (op->summary)
    json_object_set_string(op_obj, "summary", op->summary);
  if (op->description)
    json_object_set_string(op_obj, "description", op->description);
  if (op->deprecated)
    json_object_set_boolean(op_obj, "deprecated", 1);

  if (op->n_tags > 0 && op->tags) {
    tags_val = json_value_init_array();
    tags_arr = json_value_get_array(tags_val);
    for (i = 0; i < op->n_tags; i++) {
      if (op->tags[i]) {
        json_array_append_string(tags_arr, op->tags[i]);
      }
    }
    json_object_set_value(op_obj, "tags", tags_val);
  }

  if (op->external_docs.url) {
    ext_docs_val = json_value_init_object();
    ext_docs_obj = json_value_get_object(ext_docs_val);
    json_object_set_string(ext_docs_obj, "url", op->external_docs.url);
    if (op->external_docs.description) {
      json_object_set_string(ext_docs_obj, "description",
                             op->external_docs.description);
    }
    json_object_set_value(op_obj, "externalDocs", ext_docs_val);
  }

  /* Removed old serialization */

  if (op->n_security > 0 && op->security) {
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
  return 0;
}

int c_rest_openapi_spec_to_json(const struct c_rest_openapi_spec *spec,
                                char **out_json) {
  JSON_Value *root_val;
  JSON_Object *root_obj;
  JSON_Value *info_val;
  JSON_Object *info_obj;
  JSON_Value *paths_val;
  JSON_Object *paths_obj;
  JSON_Value *components_val;
  size_t i, j, k;

  if (!spec || !out_json)
    return 1;

  root_val = json_value_init_object();
  root_obj = json_value_get_object(root_val);

  json_object_set_string(root_obj, "openapi",
                         spec->openapi_version ? spec->openapi_version
                                               : "3.1.0");

  if (spec->json_schema_dialect) {
    json_object_set_string(root_obj, "jsonSchemaDialect",
                           spec->json_schema_dialect);
  }

  /* Info */
  info_val = json_value_init_object();
  info_obj = json_value_get_object(info_val);
  json_object_set_string(info_obj, "title",
                         spec->info.title ? spec->info.title : "API");
  json_object_set_string(info_obj, "version",
                         spec->info.version ? spec->info.version : "1.0.0");
  if (spec->info.summary)
    json_object_set_string(info_obj, "summary", spec->info.summary);
  if (spec->info.description) {
    json_object_set_string(info_obj, "description", spec->info.description);
  }
  if (spec->info.terms_of_service) {
    json_object_set_string(info_obj, "termsOfService",
                           spec->info.terms_of_service);
  }
  if (spec->info.contact.name || spec->info.contact.url ||
      spec->info.contact.email) {
    JSON_Value *contact_val = json_value_init_object();
    JSON_Object *contact_obj = json_value_get_object(contact_val);
    if (spec->info.contact.name)
      json_object_set_string(contact_obj, "name", spec->info.contact.name);
    if (spec->info.contact.url)
      json_object_set_string(contact_obj, "url", spec->info.contact.url);
    if (spec->info.contact.email)
      json_object_set_string(contact_obj, "email", spec->info.contact.email);
    json_object_set_value(info_obj, "contact", contact_val);
  }
  if (spec->info.license.name) {
    JSON_Value *license_val = json_value_init_object();
    JSON_Object *license_obj = json_value_get_object(license_val);
    json_object_set_string(license_obj, "name", spec->info.license.name);
    if (spec->info.license.identifier)
      json_object_set_string(license_obj, "identifier",
                             spec->info.license.identifier);
    if (spec->info.license.url)
      json_object_set_string(license_obj, "url", spec->info.license.url);
    json_object_set_value(info_obj, "license", license_val);
  }
  json_object_set_value(root_obj, "info", info_val);

  /* Servers */
  if (spec->n_servers > 0 && spec->servers) {
    JSON_Value *servers_val = json_value_init_array();
    JSON_Array *servers_arr = json_value_get_array(servers_val);
    for (i = 0; i < spec->n_servers; i++) {
      JSON_Value *s_val = json_value_init_object();
      JSON_Object *s_obj = json_value_get_object(s_val);
      json_object_set_string(s_obj, "url", spec->servers[i].url);
      if (spec->servers[i].description)
        json_object_set_string(s_obj, "description",
                               spec->servers[i].description);

      if (spec->servers[i].n_variables > 0 && spec->servers[i].variables) {
        JSON_Value *vars_val = json_value_init_object();
        JSON_Object *vars_obj = json_value_get_object(vars_val);
        for (j = 0; j < spec->servers[i].n_variables; j++) {
          struct c_rest_openapi_server_variable *v =
              &spec->servers[i].variables[j];
          JSON_Value *v_val = json_value_init_object();
          JSON_Object *v_obj = json_value_get_object(v_val);
          if (v->default_value)
            json_object_set_string(v_obj, "default", v->default_value);
          if (v->description)
            json_object_set_string(v_obj, "description", v->description);
          if (v->n_enum_values > 0 && v->enum_values) {
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
  if (spec->n_tags > 0 && spec->tags) {
    JSON_Value *tags_val = json_value_init_array();
    JSON_Array *tags_arr = json_value_get_array(tags_val);
    for (i = 0; i < spec->n_tags; i++) {
      JSON_Value *t_val = json_value_init_object();
      JSON_Object *t_obj = json_value_get_object(t_val);
      json_object_set_string(t_obj, "name", spec->tags[i].name);
      if (spec->tags[i].summary)
        json_object_set_string(t_obj, "summary", spec->tags[i].summary);
      if (spec->tags[i].description)
        json_object_set_string(t_obj, "description", spec->tags[i].description);
      if (spec->tags[i].parent)
        json_object_set_string(
            t_obj, "parent",
            spec->tags[i]
                .parent); /* x-parent extension technically or 3.2.0? */
      if (spec->tags[i].kind)
        json_object_set_string(t_obj, "kind",
                               spec->tags[i].kind); /* 3.2.0 / x-kind */

      if (spec->tags[i].external_docs.url) {
        JSON_Value *ed_val = json_value_init_object();
        JSON_Object *ed_obj = json_value_get_object(ed_val);
        json_object_set_string(ed_obj, "url", spec->tags[i].external_docs.url);
        if (spec->tags[i].external_docs.description)
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

    if (p->summary)
      json_object_set_string(path_item_obj, "summary", p->summary);
    if (p->description)
      json_object_set_string(path_item_obj, "description", p->description);

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
    if (spec->n_security > 0 && spec->security) {
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
    if (spec->n_security_schemes > 0 && spec->security_schemes) {
      JSON_Value *sec_schemes_val = json_value_init_object();
      JSON_Object *sec_schemes_obj = json_value_get_object(sec_schemes_val);
      for (i = 0; i < spec->n_security_schemes; i++) {
        struct c_rest_openapi_security_scheme *s = &spec->security_schemes[i];
        JSON_Value *s_val = json_value_init_object();
        JSON_Object *s_obj = json_value_get_object(s_val);
        if (s->type)
          json_object_set_string(s_obj, "type", s->type);
        if (s->description)
          json_object_set_string(s_obj, "description", s->description);
        if (s->name)
          json_object_set_string(s_obj, "name", s->name);
        if (s->in)
          json_object_set_string(s_obj, "in", s->in);
        if (s->scheme)
          json_object_set_string(s_obj, "scheme", s->scheme);
        if (s->bearer_format)
          json_object_set_string(s_obj, "bearerFormat", s->bearer_format);
        if (s->open_id_connect_url)
          json_object_set_string(s_obj, "openIdConnectUrl",
                                 s->open_id_connect_url);

        if (s->flows.implicit || s->flows.password ||
            s->flows.client_credentials || s->flows.authorization_code) {
          JSON_Value *flows_val = json_value_init_object();
          JSON_Object *flows_obj = json_value_get_object(flows_val);

          /* Helper macro inside logic isn't clean C, let's just inline it */
          if (s->flows.implicit) {
            JSON_Value *f_val = json_value_init_object();
            JSON_Object *f_obj = json_value_get_object(f_val);
            JSON_Value *scopes_val = json_value_init_object();
            JSON_Object *scopes_obj = json_value_get_object(scopes_val);
            if (s->flows.implicit->authorization_url)
              json_object_set_string(f_obj, "authorizationUrl",
                                     s->flows.implicit->authorization_url);
            if (s->flows.implicit->refresh_url)
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
          if (s->flows.password) {
            JSON_Value *f_val = json_value_init_object();
            JSON_Object *f_obj = json_value_get_object(f_val);
            JSON_Value *scopes_val = json_value_init_object();
            JSON_Object *scopes_obj = json_value_get_object(scopes_val);
            if (s->flows.password->token_url)
              json_object_set_string(f_obj, "tokenUrl",
                                     s->flows.password->token_url);
            if (s->flows.password->refresh_url)
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
          if (s->flows.client_credentials) {
            JSON_Value *f_val = json_value_init_object();
            JSON_Object *f_obj = json_value_get_object(f_val);
            JSON_Value *scopes_val = json_value_init_object();
            JSON_Object *scopes_obj = json_value_get_object(scopes_val);
            if (s->flows.client_credentials->token_url)
              json_object_set_string(f_obj, "tokenUrl",
                                     s->flows.client_credentials->token_url);
            if (s->flows.client_credentials->refresh_url)
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
          if (s->flows.authorization_code) {
            JSON_Value *f_val = json_value_init_object();
            JSON_Object *f_obj = json_value_get_object(f_val);
            JSON_Value *scopes_val = json_value_init_object();
            JSON_Object *scopes_obj = json_value_get_object(scopes_val);
            if (s->flows.authorization_code->authorization_url)
              json_object_set_string(
                  f_obj, "authorizationUrl",
                  s->flows.authorization_code->authorization_url);
            if (s->flows.authorization_code->token_url)
              json_object_set_string(f_obj, "tokenUrl",
                                     s->flows.authorization_code->token_url);
            if (s->flows.authorization_code->refresh_url)
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

    if (spec->n_components > 0) {
      JSON_Object *comp_obj = json_value_get_object(components_val);
      JSON_Value *schemas_val = json_value_init_object();
      JSON_Object *schemas_obj = json_value_get_object(schemas_val);

      for (i = 0; i < spec->n_components; i++) {
        JSON_Value *parsed = json_parse_string(spec->component_schemas_json[i]);
        if (parsed) {
          json_object_set_value(schemas_obj, spec->component_schemas_keys[i],
                                parsed);
        }
      }
      json_object_set_value(comp_obj, "schemas", schemas_val);
    }
  }
  json_object_set_value(root_obj, "components", components_val);

  if (spec->external_docs.url) {
    JSON_Value *ed_val = json_value_init_object();
    JSON_Object *ed_obj = json_value_get_object(ed_val);
    json_object_set_string(ed_obj, "url", spec->external_docs.url);
    if (spec->external_docs.description)
      json_object_set_string(ed_obj, "description",
                             spec->external_docs.description);
    json_object_set_value(root_obj, "externalDocs", ed_val);
  }

  *out_json = json_serialize_to_string_pretty(root_val);
  json_value_free(root_val);

  if (!*out_json)
    return 1;
  return 0;
}

static int openapi_handler(struct c_rest_request *req,
                           struct c_rest_response *res, void *user_data) {
  struct c_rest_router *router = (struct c_rest_router *)user_data;
  struct c_rest_openapi_spec *spec = NULL;
  char *json_str = NULL;
  c_rest_router_get_openapi_spec(router, &spec);

  /* unused args */
  (void)req;

  if (c_rest_openapi_spec_to_json(spec, &json_str) == 0 && json_str) {
    res->status_code = 200;
    c_rest_response_json(res, json_str);
    json_free_serialized_string(json_str);
  } else {
    res->status_code = 500;
    c_rest_response_json(res,
                         "{\"error\": \"Failed to serialize OpenAPI spec\"}");
  }
  return 0;
}

int c_rest_enable_openapi(struct c_rest_router *router, const char *path) {
  if (!router || !path)
    return 1;
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

static const char *swagger_html_template_2 =
    "  <script>\n"
    "    window.onload = function() {\n"
    "      SwaggerUIBundle({\n"
    "        url: \"%s\",\n"
    "        dom_id: '#swagger-ui',\n"
    "        presets: [SwaggerUIBundle.presets.apis, "
    "SwaggerUIBundle.SwaggerUIStandalonePreset],\n"
    "        layout: \"BaseLayout\"\n"
    "      });\n"
    "    };\n"
    "  </script>\n"
    "</body>\n"
    "</html>";

static int swagger_ui_handler(struct c_rest_request *req,
                              struct c_rest_response *res, void *user_data) {
  struct c_rest_router *router = (struct c_rest_router *)user_data;
  struct c_rest_openapi_spec *spec = NULL;
  char *html_buf;
  c_rest_router_get_openapi_spec(router, &spec);
  size_t html_len;
  const char *openapi_url = "/openapi.json";

  /* unused args */
  (void)req;

  if (spec && spec->swagger_openapi_url) {
    openapi_url = spec->swagger_openapi_url;
  }

  html_len = strlen(swagger_html_template_1) + strlen(swagger_html_template_2) +
             strlen(openapi_url) + 1;
  html_buf = (char *)malloc(html_len);
  if (!html_buf) {
    res->status_code = 500;
    return c_rest_response_html(res, "Internal Server Error");
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
  free(html_buf);
  return 0;
}

int c_rest_enable_swagger_ui(struct c_rest_router *router,
                             const char *docs_path, const char *openapi_url) {
  struct c_rest_openapi_spec *spec = NULL;
  if (!router || !docs_path || !openapi_url)
    return 1;

  c_rest_router_get_openapi_spec(router, &spec);
  if (spec) {
    if (spec->swagger_openapi_url)
      free(spec->swagger_openapi_url);
    spec->swagger_openapi_url = (char *)malloc(strlen(openapi_url) + 1);
    if (spec->swagger_openapi_url) {
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
int c_rest_openapi_spec_init(struct c_rest_openapi_spec **out_spec) {
  if (out_spec)
    *out_spec = NULL;
  return 1;
}
int c_rest_openapi_spec_destroy(struct c_rest_openapi_spec *spec) {
  (void)spec;
  return 1;
}
int c_rest_openapi_spec_add_component_schema(struct c_rest_openapi_spec *spec,
                                             const char *schema_name,
                                             const char *json_schema_str) {
  (void)spec;
  (void)schema_name;
  (void)json_schema_str;
  return 1;
}
int c_rest_openapi_spec_add_path(struct c_rest_openapi_spec *spec,
                                 const char *route, const char *method,
                                 const struct c_rest_openapi_operation *op) {
  (void)spec;
  (void)route;
  (void)method;
  (void)op;
  return 1;
}
int c_rest_openapi_spec_to_json(const struct c_rest_openapi_spec *spec,
                                char **out_json) {
  (void)spec;
  if (out_json)
    *out_json = NULL;
  return 1;
}
int c_rest_enable_openapi(struct c_rest_router *router, const char *path) {
  (void)router;
  (void)path;
  return 1;
}
int c_rest_enable_swagger_ui(struct c_rest_router *router,
                             const char *docs_path, const char *openapi_url) {
  (void)router;
  (void)docs_path;
  (void)openapi_url;
  return 1;
}
#endif
