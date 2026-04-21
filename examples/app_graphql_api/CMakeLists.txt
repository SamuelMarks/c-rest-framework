cmake_minimum_required(VERSION 3.10)

if(C_REST_FRAMEWORK_ENABLE_GRAPHQL)
    add_executable(app_graphql_api main.c)
    target_link_libraries(app_graphql_api PRIVATE c-rest-framework)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_link_libraries(app_graphql_api PRIVATE pthread)
    endif()
endif()