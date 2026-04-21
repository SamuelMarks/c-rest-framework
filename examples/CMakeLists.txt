add_subdirectory(app_node_style)
add_subdirectory(app_threaded)
add_subdirectory(app_multi_process)
add_subdirectory(app_https_server)
add_subdirectory(notes)
add_subdirectory(app_template_engine)
if(NOT CMAKE_SYSTEM_NAME STREQUAL "DOS")
    add_subdirectory(oauth2)
endif()
add_subdirectory(app_openapi)
add_subdirectory(app_websockets)
add_subdirectory(app_server_sent_events_sse)
add_subdirectory(app_graphql_api)

add_subdirectory(app_http23)
add_subdirectory(app_jwt_json_w)

add_subdirectory(app_full_multi)

if(C_REST_ENABLE_HOT_RELOADING_AUTO_RESTART)
  add_subdirectory(app_hot_reload)
endif()


