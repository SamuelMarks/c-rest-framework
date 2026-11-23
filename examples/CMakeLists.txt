add_subdirectory(app_node_style)
add_subdirectory(app_threaded)
if(NOT EMSCRIPTEN AND NOT CMAKE_SYSTEM_NAME STREQUAL "DOS")
    add_subdirectory(app_multi_process)
endif()
add_subdirectory(app_https_server)
add_subdirectory(notes)
add_subdirectory(app_template_engine)
if(NOT EMSCRIPTEN AND NOT CMAKE_SYSTEM_NAME STREQUAL "DOS")
    add_subdirectory(oauth2)
endif()
add_subdirectory(app_openapi)
add_subdirectory(app_websockets)
add_subdirectory(app_server_sent_events_sse)
add_subdirectory(app_graphql_api)
add_subdirectory(app_http23)
add_subdirectory(app_jwt_json_w)
add_subdirectory(app_full_multi)

if(C_REST_ENABLE_HOT_RELOADING_AUTO_RESTART AND NOT EMSCRIPTEN)
    add_subdirectory(app_hot_reload)
endif()

if(CMAKE_CROSSCOMPILING_EMULATOR MATCHES "wine")
    foreach(_demo_target
        app_node_style
        app_threaded
        app_multi_process
        app_https_server
        app_notes
        app_template_engine
        oauth2_cli
        app_openapi
        app_websockets
        app_server_sent_events_sse
        app_graphql_api
        app_http23
        app_jwt_json_w
        app_full_multi
        app_hot_reload)
        if(TARGET ${_demo_target})
            set_target_properties(${_demo_target} PROPERTIES EXCLUDE_FROM_ALL TRUE)
        endif()
    endforeach()
endif()
