add_executable(app_full_multi main.c)
target_link_libraries(app_full_multi PRIVATE c-rest-framework)
c_rest_configure_target(app_full_multi)
c_rest_copy_runtime_dlls(app_full_multi)
