#ifndef C_REST_OPENAPI_H
#define C_REST_OPENAPI_H

/* clang-format off */
#include <stddef.h>
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Reference to a schema struct
 */

/**
 * @brief Reference to a schema struct
 */
/**
 * @brief Reference to a schema struct
 */
struct c_rest_openapi_schema_ref {
  const char *ref_name; /**< Name of the C struct, e.g., "my_request_struct" */
};

/**
 * @brief OpenAPI Example Object
 */
struct c_rest_openapi_example {
  const char *summary;        /**< Short summary */
  const char *description;    /**< Description */
  const char *value;          /**< Raw JSON or string value */
  const char *external_value; /**< External value URL */
};

/**
 * @brief OpenAPI Header Object
 */
struct c_rest_openapi_header {
  const char *description;                 /**< Header description */
  int required;                            /**< Required flag */
  int deprecated;                          /**< Deprecated flag */
  const char *style;                       /**< Header style */
  int explode;                             /**< Explode array flag */
  struct c_rest_openapi_schema_ref schema; /**< Schema reference */
};

/**
 * @brief OpenAPI Media Type Object
 */
struct c_rest_openapi_media_type {
  struct c_rest_openapi_schema_ref schema; /**< Media type schema */
  const char *example;                     /**< Simple string example */
};

/**
 * @brief OpenAPI Request Body Object
 */
struct c_rest_openapi_request_body {
  const char *description; /**< Request body description */
  struct c_rest_openapi_media_type *content_values; /**< Array of media types */
  const char **content_keys; /**< Array of content keys */
  size_t n_content;          /**< Number of content types */
  int required;              /**< Required flag */
};

/**
 * @brief OpenAPI Response Object
 */
struct c_rest_openapi_response {
  const char *status_code;                          /**< Response status code */
  const char *description;                          /**< Response description */
  struct c_rest_openapi_media_type *content_values; /**< Array of media types */
  const char **content_keys;                   /**< Array of content keys */
  size_t n_content;                            /**< Number of content types */
  struct c_rest_openapi_header *header_values; /**< Array of header values */
  const char **header_keys;                    /**< Array of header keys */
  size_t n_headers;                            /**< Number of headers */
};

/**
 * @brief OpenAPI Parameter Object
 */
struct c_rest_openapi_parameter {
  const char *name; /**< Parameter name */
  const char
      *in; /**< Parameter location ("query", "header", "path", "cookie") */
  const char *description;                 /**< Parameter description */
  int required;                            /**< Required flag */
  int deprecated;                          /**< Deprecated flag */
  int allow_empty_value;                   /**< Allow empty value flag */
  const char *style;                       /**< Parameter style */
  int explode;                             /**< Explode array flag */
  int allow_reserved;                      /**< Allow reserved flag */
  struct c_rest_openapi_schema_ref schema; /**< Schema reference */
  const char *example;                     /**< Example value */
};
/**
 * @brief OpenAPI Contact object
 */
struct c_rest_openapi_contact {
  const char *name;  /**< Name of contact */
  const char *url;   /**< URL of contact */
  const char *email; /**< Email of contact */
};

/**
 * @brief OpenAPI License object
 */
struct c_rest_openapi_license {
  const char *name;       /**< License name */
  const char *identifier; /**< SPDX identifier */
  const char *url;        /**< License URL */
};

/**
 * @brief OpenAPI External Documentation object
 */
struct c_rest_openapi_external_doc {
  const char *description; /**< Short description */
  const char *url;         /**< Doc URL */
};

/**
 * @brief OpenAPI Server Variable object
 */
struct c_rest_openapi_server_variable {
  const char *name;          /**< Variable name used in template */
  const char **enum_values;  /**< Allowed enum values */
  size_t n_enum_values;      /**< Number of enums */
  const char *default_value; /**< Default string value */
  const char *description;   /**< Description of variable */
};

/**
 * @brief OpenAPI Server object
 */
struct c_rest_openapi_server {
  const char *url;                                  /**< Server URL template */
  const char *description;                          /**< Server description */
  struct c_rest_openapi_server_variable *variables; /**< Variables mapped */
  size_t n_variables;                               /**< Number of variables */
};

/**
 * @brief OpenAPI Tag object
 */
struct c_rest_openapi_tag {
  const char *name;                                 /**< Tag name */
  const char *summary;                              /**< Tag summary */
  const char *description;                          /**< Tag description */
  struct c_rest_openapi_external_doc external_docs; /**< Additional docs */
  const char *parent; /**< Nested hierarchy parent */
  const char *kind;   /**< Tag kind logic */
};

/**
 * @brief OpenAPI Operation object
 */

/**
 * @brief OpenAPI OAuth Flow object
 */
struct c_rest_openapi_oauth_flow {
  const char *authorization_url; /**< Authorization URL */
  const char *token_url;         /**< Token URL */
  const char *refresh_url;       /**< Refresh URL */
  const char **scopes_keys;      /**< Scope keys */
  const char **scopes_values;    /**< Scope descriptions */
  size_t n_scopes;               /**< Number of scopes */
};

/**
 * @brief OpenAPI OAuth Flows object
 */
struct c_rest_openapi_oauth_flows {
  struct c_rest_openapi_oauth_flow *implicit; /**< Implicit flow */
  struct c_rest_openapi_oauth_flow *password; /**< Password flow */
  struct c_rest_openapi_oauth_flow
      *client_credentials; /**< Client credentials flow */
  struct c_rest_openapi_oauth_flow
      *authorization_code; /**< Authorization code flow */
};

/**
 * @brief OpenAPI Security Scheme object
 */
struct c_rest_openapi_security_scheme {
  const char *name_key;      /**< Internal component key */
  const char *type;          /**< "oauth2", "http", "apiKey", "openIdConnect" */
  const char *description;   /**< Description */
  const char *name;          /**< Header/query/cookie name for apiKey */
  const char *in;            /**< "query", "header", "cookie" */
  const char *scheme;        /**< "bearer", "basic", etc */
  const char *bearer_format; /**< e.g., "JWT" */
  struct c_rest_openapi_oauth_flows flows; /**< OAuth2 flows */
  const char *open_id_connect_url;         /**< OpenID Connect URL */
};

/**
 * @brief OpenAPI Security Requirement object
 */
struct c_rest_openapi_security_requirement {
  const char *name;    /**< Security scheme key */
  const char **scopes; /**< Required scopes */
  size_t n_scopes;     /**< Number of scopes */
};

/**
 * @brief OpenAPI Operation object
 */
struct c_rest_openapi_operation {
  const char *operation_id;                         /**< Unique ID */
  const char *summary;                              /**< Short summary */
  const char *description;                          /**< Verbose description */
  const char **tags;                                /**< Tags bound */
  size_t n_tags;                                    /**< Number of tags */
  struct c_rest_openapi_parameter *parameters;      /**< Parameters */
  size_t n_parameters;                              /**< Number of parameters */
  struct c_rest_openapi_request_body *request_body; /**< Request body */
  struct c_rest_openapi_response *responses;        /**< Responses array */
  size_t n_responses;                               /**< Number of responses */
  int deprecated;                                   /**< Is deprecated (1/0) */
  struct c_rest_openapi_external_doc external_docs; /**< Additional docs */
  struct c_rest_openapi_security_requirement
      *security;     /**< Security requirements */
  size_t n_security; /**< Number of security requirements */
};

/**
 * @brief OpenAPI Path Item object
 */
struct c_rest_openapi_path {
  const char *route;                       /**< The path route template */
  const char *summary;                     /**< Path summary */
  const char *description;                 /**< Path description */
  struct c_rest_openapi_server *servers;   /**< Path specific servers */
  size_t n_servers;                        /**< Path server count */
  struct c_rest_openapi_operation get;     /**< GET Operation */
  struct c_rest_openapi_operation post;    /**< POST Operation */
  struct c_rest_openapi_operation put;     /**< PUT Operation */
  struct c_rest_openapi_operation del;     /**< DELETE Operation */
  struct c_rest_openapi_operation patch;   /**< PATCH Operation */
  struct c_rest_openapi_operation options; /**< OPTIONS Operation */
  struct c_rest_openapi_operation head;    /**< HEAD Operation */
  struct c_rest_openapi_operation trace;   /**< TRACE Operation */
  struct c_rest_openapi_operation query;   /**< QUERY Operation */
};

/**
 * @brief OpenAPI Info object
 */
struct c_rest_openapi_info {
  const char *title;                     /**< Title of API */
  const char *summary;                   /**< Summary */
  const char *version;                   /**< Version */
  const char *description;               /**< Description */
  const char *terms_of_service;          /**< ToS URL */
  struct c_rest_openapi_contact contact; /**< Contact */
  struct c_rest_openapi_license license; /**< License */
};

/**
 * @brief Main OpenAPI Spec state structure
 */
struct c_rest_openapi_spec {
  const char *openapi_version;                      /**< Spec version string */
  const char *json_schema_dialect;                  /**< JSON dialect */
  struct c_rest_openapi_info info;                  /**< Info block */
  struct c_rest_openapi_server *servers;            /**< Global servers */
  size_t n_servers;                                 /**< Server count */
  struct c_rest_openapi_path *paths;                /**< Paths map */
  size_t n_paths;                                   /**< Path count */
  size_t capacity_paths;                            /**< Allocator capacity */
  char *swagger_openapi_url;                        /**< UI override path */
  char **component_schemas_keys;                    /**< Component keys */
  char **component_schemas_json;                    /**< Component jsons */
  size_t n_components;                              /**< Count components */
  size_t capacity_components;                       /**< Capacity components */
  struct c_rest_openapi_tag *tags;                  /**< Tags */
  size_t n_tags;                                    /**< Tag count */
  struct c_rest_openapi_external_doc external_docs; /**< Docs */
  struct c_rest_openapi_security_requirement
      *security;     /**< Global security requirements */
  size_t n_security; /**< Number of global security requirements */
  struct c_rest_openapi_security_scheme
      *security_schemes;     /**< Security schemes */
  size_t n_security_schemes; /**< Number of security schemes */
};

int c_rest_openapi_spec_init(struct c_rest_openapi_spec **out_spec);
int c_rest_openapi_spec_destroy(struct c_rest_openapi_spec *spec);

int c_rest_openapi_spec_add_path(struct c_rest_openapi_spec *spec,
                                 const char *route, const char *method,
                                 const struct c_rest_openapi_operation *op);

/**
 * @brief Add a JSON schema to the components/schemas section.
 * @param spec The spec object.
 * @param schema_name The name of the schema (e.g., "my_request_struct").
 * @param json_schema_str The raw JSON schema string. Will be parsed and merged.
 * @return 0 on success.
 */
int c_rest_openapi_spec_add_component_schema(struct c_rest_openapi_spec *spec,
                                             const char *schema_name,
                                             const char *json_schema_str);

struct c_rest_router;

/**
 * @brief Serialize the OpenAPI spec to a JSON string.
 * @param spec The spec object.
 * @param out_json Allocated string. Must be freed via
 * json_free_serialized_string() from parson.
 * @return 0 on success.
 */
int c_rest_openapi_spec_to_json(const struct c_rest_openapi_spec *spec,
                                char **out_json);

/**
 * @brief Add a route to serve the OpenAPI specification.
 * @param router The router instance.
 * @param path The URL path (e.g., "/openapi.json").
 * @return 0 on success.
 */
int c_rest_enable_openapi(struct c_rest_router *router, const char *path);

/**
 * @brief Add a route to serve Swagger UI documentation.
 * @param router The router instance.
 * @param docs_path The URL path for the UI (e.g., "/docs").
 * @param openapi_url The URL path to the OpenAPI JSON (e.g., "/openapi.json").
 * @return 0 on success.
 */
int c_rest_enable_swagger_ui(struct c_rest_router *router,
                             const char *docs_path, const char *openapi_url);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* C_REST_OPENAPI_H */
