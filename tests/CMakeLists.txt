cmake_minimum_required(VERSION 3.10)

include(FetchContent)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../greatest/greatest.h")
    message(STATUS "Using local greatest for tests")
    FetchContent_Declare(
      greatest
      SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../greatest"
    )
else()
    FetchContent_Declare(
      greatest
      URL "https://raw.githubusercontent.com/SamuelMarks/greatest/refs/heads/cmake-and-msvc/greatest.h"
      DOWNLOAD_NO_EXTRACT TRUE
    )
endif()
FetchContent_MakeAvailable(greatest)


set(TEST_SRCS
    test_main.c
    test_multiplatform.c
    test_parser.c
    test_router.c
    test_request_response.c
    test_tls_context.c
)

if(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP_BROTLI)
    list(APPEND TEST_SRCS test_response_compression_gzip_brotli.c)
endif()

if(C_REST_FRAMEWORK_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING)
    list(APPEND TEST_SRCS test_template.c)
endif()

if(C_REST_FRAMEWORK_ENABLE_GRAPHQL)
    list(APPEND TEST_SRCS test_graphql.c)
endif()

if(C_REST_ENABLE_SERVER_SENT_EVENTS_SSE)
    list(APPEND TEST_SRCS test_server_sent_events_sse.c)
endif()

if(C_REST_ENABLE_HOT_RELOADING_AUTO_RESTART)
    list(APPEND TEST_SRCS test_hot_reload.c)
endif()

if(C_REST_ENABLE_JWT_JSON_WEB_TOKENS_AUTHENTICATION_MIDDLEWARE)
    list(APPEND TEST_SRCS test_jwt_json_web_tokens_authentication_middleware.c)
endif()

if(NOT CMAKE_SYSTEM_NAME STREQUAL "DOS")
    list(APPEND TEST_SRCS test_client.c test_tls_integration.c test_openapi.c test_orm_integration.c test_oauth2.c test_crypto.c test_time.c test_multipart.c test_full_multipart_form_streaming.c test_websocket.c test_http23.c test_rate_limiting_throttling_middleware.c)
endif()

add_executable(c-rest-framework-tests ${TEST_SRCS})


target_include_directories(c-rest-framework-tests PRIVATE ../include ${CMAKE_CURRENT_SOURCE_DIR} ${greatest_SOURCE_DIR})
target_link_libraries(c-rest-framework-tests PRIVATE c-rest-framework)

if(C_REST_GNU_FLAGS)
    target_compile_options(c-rest-framework-tests PRIVATE ${C_REST_GNU_FLAGS})
endif()

add_test(NAME c-rest-framework-tests COMMAND c-rest-framework-tests)

if(WIN32 AND BUILD_SHARED_LIBS)
    if(TARGET cfs)
        add_custom_command(TARGET c-rest-framework-tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:cfs>" "$<TARGET_FILE_DIR:c-rest-framework-tests>"
        )
    endif()
    if(TARGET c89stringutils)
        add_custom_command(TARGET c-rest-framework-tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:c89stringutils>" "$<TARGET_FILE_DIR:c-rest-framework-tests>"
        )
    endif()
    if(TARGET c-abstract-http)
        add_custom_command(TARGET c-rest-framework-tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:c-abstract-http>" "$<TARGET_FILE_DIR:c-rest-framework-tests>"
        )
    endif()
    if(TARGET c_orm)
        add_custom_command(TARGET c-rest-framework-tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:c_orm>" "$<TARGET_FILE_DIR:c-rest-framework-tests>"
        )
    endif()
    if(TARGET c-rest-framework)
        add_custom_command(TARGET c-rest-framework-tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:c-rest-framework>" "$<TARGET_FILE_DIR:c-rest-framework-tests>"
        )
    endif()
endif()


