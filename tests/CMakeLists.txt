cmake_minimum_required(VERSION 3.14)

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
      GIT_REPOSITORY https://github.com/SamuelMarks/greatest.git
      GIT_TAG master
    )
endif()
FetchContent_MakeAvailable(greatest)


set(TEST_SRCS
    test_main.c

    test_examples.c

    test_multiplatform.c
    test_parser.c
    test_router.c
    test_request_response.c
    test_tls_context.c
    test_base64.c
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

if(C_REST_ENABLE_HOT_RELOADING_AUTO_RESTART AND NOT EMSCRIPTEN)
    list(APPEND TEST_SRCS test_hot_reload.c)
endif()

if(C_REST_ENABLE_JWT_JSON_WEB_TOKENS_AUTHENTICATION_MIDDLEWARE)
    list(APPEND TEST_SRCS test_jwt_json_web_tokens_authentication_middleware.c)
endif()

if(NOT CMAKE_SYSTEM_NAME STREQUAL "DOS")
    list(APPEND TEST_SRCS test_openapi.c test_platform.c test_modality.c test_middleware.c test_orm_integration.c test_oauth2.c test_crypto.c test_time.c test_multipart.c test_full_multipart_form_streaming.c test_rate_limiting_throttling_middleware.c test_endian.c test_hashmap.c test_list.c test_str_utils.c test_mem.c test_ts_queue.c test_log.c test_string.c test_pool.c)
    if(NOT EMSCRIPTEN)
        list(APPEND TEST_SRCS test_client.c test_tls_integration.c test_websocket.c test_http23.c)
    endif()
endif()

add_executable(c-rest-framework-tests ${TEST_SRCS})

add_executable(test-response-mock test_response_mock.c)
target_include_directories(test-response-mock PRIVATE ../include ${CMAKE_CURRENT_SOURCE_DIR} ${greatest_SOURCE_DIR})
target_include_directories(test-response-mock PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/../)
target_link_libraries(test-response-mock PRIVATE c-rest-framework)
if(C_REST_GNU_FLAGS)
    target_compile_options(test-response-mock PRIVATE ${C_REST_GNU_FLAGS})
endif()
add_test(NAME test-response-mock COMMAND test-response-mock)

add_executable(test-hot-reload-mock test_hot_reload_mock.c)
target_include_directories(test-hot-reload-mock PRIVATE ../include ${CMAKE_CURRENT_SOURCE_DIR} ${greatest_SOURCE_DIR})
target_include_directories(test-hot-reload-mock PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/../)
target_link_libraries(test-hot-reload-mock PRIVATE c-rest-framework)
if(C_REST_GNU_FLAGS)
    target_compile_options(test-hot-reload-mock PRIVATE ${C_REST_GNU_FLAGS})
endif()
add_test(NAME test-hot-reload-mock COMMAND test-hot-reload-mock)

add_executable(test-crypto-mock test_crypto_mock.c)
target_include_directories(test-crypto-mock PRIVATE ../include ${CMAKE_CURRENT_SOURCE_DIR} ${greatest_SOURCE_DIR})
target_include_directories(test-crypto-mock PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/../)
target_link_libraries(test-crypto-mock PRIVATE c-rest-framework)
if(C_REST_GNU_FLAGS)
    target_compile_options(test-crypto-mock PRIVATE ${C_REST_GNU_FLAGS})
endif()
add_test(NAME test-crypto-mock COMMAND test-crypto-mock)

add_executable(test-orm-crud-mock test_orm_crud_mock.c)
target_include_directories(test-orm-crud-mock PRIVATE ../include ${CMAKE_CURRENT_SOURCE_DIR} ${greatest_SOURCE_DIR})
target_include_directories(test-orm-crud-mock PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/../)
target_link_libraries(test-orm-crud-mock PRIVATE c-rest-framework)
if(C_REST_GNU_FLAGS)
    target_compile_options(test-orm-crud-mock PRIVATE ${C_REST_GNU_FLAGS})
endif()
add_test(NAME test-orm-crud-mock COMMAND test-orm-crud-mock)

add_executable(test-graphql-mock test_graphql_mock.c)
target_include_directories(test-graphql-mock PRIVATE ../include ${CMAKE_CURRENT_SOURCE_DIR} ${greatest_SOURCE_DIR})
target_include_directories(test-graphql-mock PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/../)
target_link_libraries(test-graphql-mock PRIVATE c-rest-framework)
if(C_REST_GNU_FLAGS)
    target_compile_options(test-graphql-mock PRIVATE ${C_REST_GNU_FLAGS})
endif()
add_test(NAME test-graphql-mock COMMAND test-graphql-mock)

add_executable(test-http23-mock test_http23_mock.c)
target_include_directories(test-http23-mock PRIVATE ../include ${CMAKE_CURRENT_SOURCE_DIR} ${greatest_SOURCE_DIR})
target_include_directories(test-http23-mock PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/../)
target_link_libraries(test-http23-mock PRIVATE c-rest-framework)
if(C_REST_GNU_FLAGS)
    target_compile_options(test-http23-mock PRIVATE ${C_REST_GNU_FLAGS})
endif()
add_test(NAME test-http23-mock COMMAND test-http23-mock)

add_executable(test-jwt-middleware-mock test_jwt_middleware_mock.c)
target_include_directories(test-jwt-middleware-mock PRIVATE ../include ${CMAKE_CURRENT_SOURCE_DIR} ${greatest_SOURCE_DIR})
target_include_directories(test-jwt-middleware-mock PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/../)
target_link_libraries(test-jwt-middleware-mock PRIVATE c-rest-framework)
if(C_REST_GNU_FLAGS)
    target_compile_options(test-jwt-middleware-mock PRIVATE ${C_REST_GNU_FLAGS})
endif()
add_test(NAME test-jwt-middleware-mock COMMAND test-jwt-middleware-mock)

add_executable(test-openapi-mock test_openapi_mock.c)
target_include_directories(test-openapi-mock PRIVATE ../include ${CMAKE_CURRENT_SOURCE_DIR} ${greatest_SOURCE_DIR})
target_include_directories(test-openapi-mock PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/../)
target_link_libraries(test-openapi-mock PRIVATE c-rest-framework)
if(C_REST_GNU_FLAGS)
    target_compile_options(test-openapi-mock PRIVATE ${C_REST_GNU_FLAGS})
endif()
add_test(NAME test-openapi-mock COMMAND test-openapi-mock)

add_executable(test-sse-mock test_sse_mock.c)
target_include_directories(test-sse-mock PRIVATE ../include ${CMAKE_CURRENT_SOURCE_DIR} ${greatest_SOURCE_DIR})
target_include_directories(test-sse-mock PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/../)
target_link_libraries(test-sse-mock PRIVATE c-rest-framework)
if(C_REST_GNU_FLAGS)
    target_compile_options(test-sse-mock PRIVATE ${C_REST_GNU_FLAGS})
endif()
add_test(NAME test-sse-mock COMMAND test-sse-mock)

add_executable(test-rate-limit-mock test_rate_limit_mock.c)
target_include_directories(test-rate-limit-mock PRIVATE ../include ${CMAKE_CURRENT_SOURCE_DIR} ${greatest_SOURCE_DIR})
target_include_directories(test-rate-limit-mock PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/../)
target_link_libraries(test-rate-limit-mock PRIVATE c-rest-framework)
if(C_REST_GNU_FLAGS)
    target_compile_options(test-rate-limit-mock PRIVATE ${C_REST_GNU_FLAGS})
endif()
add_test(NAME test-rate-limit-mock COMMAND test-rate-limit-mock)

add_executable(test-misc-mock test_misc_mock.c)
target_include_directories(test-misc-mock PRIVATE ../include ${CMAKE_CURRENT_SOURCE_DIR} ${greatest_SOURCE_DIR})
target_include_directories(test-misc-mock PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/../)
target_link_libraries(test-misc-mock PRIVATE c-rest-framework)
if(C_REST_GNU_FLAGS)
    target_compile_options(test-misc-mock PRIVATE ${C_REST_GNU_FLAGS})
endif()
add_test(NAME test-misc-mock COMMAND test-misc-mock)

if(C_REST_GNU_FLAGS)
endif()

add_executable(test-mem-mock test_mem_mock.c)
target_include_directories(test-mem-mock PRIVATE ../include ${CMAKE_CURRENT_SOURCE_DIR} ${greatest_SOURCE_DIR})
target_include_directories(test-mem-mock PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/../)
target_link_libraries(test-mem-mock PRIVATE c-rest-framework)
if(C_REST_GNU_FLAGS)
    target_compile_options(test-mem-mock PRIVATE ${C_REST_GNU_FLAGS})
endif()
add_test(NAME test-mem-mock COMMAND test-mem-mock)


target_include_directories(c-rest-framework-tests PRIVATE ../include ${CMAKE_CURRENT_SOURCE_DIR} ${greatest_SOURCE_DIR})
target_include_directories(c-rest-framework-tests PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/../)
target_link_libraries(c-rest-framework-tests PRIVATE c-rest-framework)

if(EMSCRIPTEN)
    target_link_options(c-rest-framework-tests PRIVATE "-sALLOW_MEMORY_GROWTH=1")
endif()

if(C_REST_GNU_FLAGS)
    target_compile_options(c-rest-framework-tests PRIVATE ${C_REST_GNU_FLAGS})
endif()

if(WIN32 AND CMAKE_C_COMPILER_ID STREQUAL "MSVC" AND MSVC_VERSION LESS 1500)
    target_compile_options(c-rest-framework-tests PRIVATE /Od /Ob0 /Zi)
    target_link_options(c-rest-framework-tests PRIVATE /DEBUG /OPT:NOREF /OPT:NOICF)
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
    if(TARGET c_cdd)
        add_custom_command(TARGET c-rest-framework-tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:c_cdd>" "$<TARGET_FILE_DIR:c-rest-framework-tests>"
        )
    endif()
    if(TARGET c_str_span)
        add_custom_command(TARGET c-rest-framework-tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:c_str_span>" "$<TARGET_FILE_DIR:c-rest-framework-tests>"
        )
    endif()
endif()
add_definitions(-DC_REST_TESTING_ENDIAN_HOOK)
