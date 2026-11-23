add_executable(app_websockets main.c)
target_link_libraries(app_websockets PRIVATE c-rest-framework)
c_rest_configure_target(app_websockets)
c_rest_copy_runtime_dlls(app_websockets)
