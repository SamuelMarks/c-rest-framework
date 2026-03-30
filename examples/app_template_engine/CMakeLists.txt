if(C_REST_FRAMEWORK_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING)
    add_executable(app_template_engine main.c)
    target_link_libraries(app_template_engine PRIVATE c-rest-framework)

    if(WIN32)
        target_link_libraries(app_template_engine PRIVATE ws2_32)
    endif()

    if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(app_template_engine PRIVATE -Wall -Wextra -Werror)
    endif()
endif()
