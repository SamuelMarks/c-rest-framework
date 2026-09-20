cmake_minimum_required(VERSION 3.14)
remove_definitions(-D_CRT_SECURE_NO_WARNINGS)

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

function(add_mock_test TARGET_NAME SRC_FILE SUITE_NAME)
    set(RUNNER_SRC "${CMAKE_CURRENT_BINARY_DIR}/runner_${TARGET_NAME}.c")
    file(WRITE "${RUNNER_SRC}"
"/* clang-format off */\n#include \"greatest.h\"\n/* clang-format on */\nSUITE_EXTERN(${SUITE_NAME});\nGREATEST_MAIN_DEFS();\nint main(int argc, char **argv) {\n  GREATEST_MAIN_BEGIN();\n  RUN_SUITE(${SUITE_NAME});\n  GREATEST_MAIN_END();\n}\n")
    add_executable(${TARGET_NAME} ${SRC_FILE} "${RUNNER_SRC}")
    target_include_directories(${TARGET_NAME} PRIVATE ../include ${CMAKE_CURRENT_SOURCE_DIR} ${greatest_SOURCE_DIR})
    target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/../)
    target_link_libraries(${TARGET_NAME} PRIVATE c-rest-framework)
    if(C_REST_GNU_FLAGS)
        target_compile_options(${TARGET_NAME} PRIVATE ${C_REST_GNU_FLAGS})
    elseif(C_REST_MSVC_FLAGS)
        target_compile_options(${TARGET_NAME} PRIVATE ${C_REST_MSVC_FLAGS})
    endif()
    add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME})
endfunction()

add_mock_test(test-response-mock test_response_mock.c response_mock_suite)

if(C_REST_ENABLE_HOT_RELOADING_AUTO_RESTART AND NOT EMSCRIPTEN)
    add_mock_test(test-hot-reload-mock test_hot_reload_mock.c hot_reload_mock_suite)
endif()

add_mock_test(test-crypto-mock test_crypto_mock.c crypto_mock_suite)
add_mock_test(test-orm-crud-mock test_orm_crud_mock.c orm_crud_mock_suite)
add_mock_test(test-graphql-mock test_graphql_mock.c graphql_mock_suite)
add_mock_test(test-http23-mock test_http23_mock.c http23_mock_suite)
add_mock_test(test-jwt-middleware-mock test_jwt_middleware_mock.c jwt_middleware_mock_suite)
add_mock_test(test-openapi-mock test_openapi_mock.c openapi_mock_suite)
add_mock_test(test-sse-mock test_sse_mock.c sse_mock_suite)
add_mock_test(test-rate-limit-mock test_rate_limit_mock.c rate_limit_mock_suite)
add_mock_test(test-misc-mock test_misc_mock.c misc_mock_suite)
add_mock_test(test-platform-dos-mock test_platform_dos_mock.c platform_dos_suite)
add_mock_test(test-mem-mock test_mem_mock.c mem_mock_suite)

if(NOT WIN32)
    add_mock_test(test-platform-win32-mock test_platform_win32_mock.c platform_win32_suite)
    target_include_directories(test-platform-win32-mock BEFORE PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/mock_win32)
endif()


target_include_directories(c-rest-framework-tests PRIVATE ../include ${CMAKE_CURRENT_SOURCE_DIR} ${greatest_SOURCE_DIR})
target_include_directories(c-rest-framework-tests PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/../)
target_link_libraries(c-rest-framework-tests PRIVATE c-rest-framework)

if(EMSCRIPTEN)
    target_link_options(c-rest-framework-tests PRIVATE "-sALLOW_MEMORY_GROWTH=1" "-sSTACK_SIZE=1048576")
endif()

if(C_REST_GNU_FLAGS)
    target_compile_options(c-rest-framework-tests PRIVATE ${C_REST_GNU_FLAGS})
elseif(C_REST_MSVC_FLAGS)
    target_compile_options(c-rest-framework-tests PRIVATE ${C_REST_MSVC_FLAGS})
endif()

if(WIN32 AND CMAKE_C_COMPILER_ID STREQUAL "MSVC" AND MSVC_VERSION LESS 1500)
    target_compile_options(c-rest-framework-tests PRIVATE /Od /Ob0 /Zi)
    target_link_options(c-rest-framework-tests PRIVATE /DEBUG /OPT:NOREF /OPT:NOICF)
endif()

add_test(NAME c-rest-framework-tests COMMAND c-rest-framework-tests)
set_tests_properties(c-rest-framework-tests PROPERTIES TIMEOUT 300)

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
