add_executable(app_threaded main.c)
target_link_libraries(app_threaded PRIVATE c-rest-framework)
c_rest_configure_target(app_threaded)
c_rest_copy_runtime_dlls(app_threaded)
