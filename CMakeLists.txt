cmake_minimum_required(VERSION 3.10)
project(c-rest-framework C)

# Set C standards
set(CMAKE_C_STANDARD 90)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

option(C_REST_FRAMEWORK_STATIC_CRT "Use Static CRT (/MT, /MTd)" OFF)
option(C_REST_FRAMEWORK_SHARED_CRT "Use Shared CRT (/MD, /MDd) (default)" ON)
option(C_REST_FRAMEWORK_UNICODE "Use UNICODE" OFF)
option(C_REST_FRAMEWORK_ANSI "Use ANSI (Multi-Byte)" OFF)
option(C_REST_FRAMEWORK_USE_ASAN "Enable AddressSanitizer" OFF)
option(C_REST_FRAMEWORK_USE_TSAN "Enable ThreadSanitizer" OFF)

option(C_REST_FRAMEWORK_USE_MBEDTLS "Use mbedtls for cryptography" OFF)
option(C_REST_FRAMEWORK_USE_OPENSSL "Use OpenSSL for cryptography" OFF)
option(C_REST_FRAMEWORK_USE_LIBRESSL "Use LibreSSL for cryptography" OFF)
option(C_REST_FRAMEWORK_USE_BORINGSSL "Use BoringSSL for cryptography" OFF)
option(C_REST_FRAMEWORK_USE_WOLFSSL "Use wolfSSL for cryptography" OFF)
option(C_REST_FRAMEWORK_USE_S2N "Use s2n-tls for cryptography" OFF)

option(C_REST_FRAMEWORK_USE_BEARSSL "Use BearSSL for cryptography" OFF)
option(C_REST_FRAMEWORK_USE_SCHANNEL "Use Schannel for cryptography" OFF)
option(C_REST_FRAMEWORK_USE_GNUTLS "Use GnuTLS for cryptography" OFF)
option(C_REST_FRAMEWORK_USE_BOTAN "Use Botan for cryptography" OFF)
option(C_REST_FRAMEWORK_USE_COMMONCRYPTO "Use CommonCrypto for cryptography" OFF)
option(C_REST_FRAMEWORK_USE_WINCRYPT "Use WinCrypt for cryptography" OFF)

option(C_REST_FRAMEWORK_USE_LSQUIC "Use lsquic" OFF)
option(C_REST_FRAMEWORK_USE_PICOQUIC "Use picoquic" OFF)
option(C_REST_FRAMEWORK_USE_NGHTTP3 "Use nghttp3" OFF)
option(C_REST_FRAMEWORK_USE_MSH3 "Use msh3" OFF)
option(C_REST_FRAMEWORK_USE_ARIA2 "Use aria2" OFF)
option(C_REST_FRAMEWORK_USE_LIBSOUP3 "Use libsoup3" OFF)
option(C_REST_FRAMEWORK_USE_LIBUV "Use libuv" OFF)
option(C_REST_FRAMEWORK_USE_LIBEVENT "Use libevent" OFF)
option(C_REST_FRAMEWORK_USE_LIBFETCH "Use libfetch" OFF)


option(C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION "Integrate with c-multiplatform" OFF)
option(C_REST_FRAMEWORK_FETCH_CAH "Fetch c-abstract-http from GitHub" ON)
option(C_REST_FRAMEWORK_FETCH_C_ORM "Fetch c-orm from GitHub" OFF)
option(C_REST_FRAMEWORK_FETCH_C_FS "Fetch c-fs from GitHub" ON)
option(C_REST_ENABLE_HOT_RELOADING_AUTO_RESTART "Enable Hot-Reloading / Auto-Restart" OFF)
option(C_REST_FRAMEWORK_FETCH_CDD_C "Fetch cdd-c from GitHub" ON)
option(C_REST_FRAMEWORK_ENABLE_GRAPHQL "Enable GraphQL API Parsing & Resolving module" ON)
option(C_REST_FRAMEWORK_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING "Enable Server-Side Template Engine" ON)
option(C_REST_ENABLE_RATE_LIMITING_THROTTLING_MIDDLEWARE "Enable Rate Limiting / Throttling Middleware" ON)
option(C_REST_ENABLE_JWT_JSON_WEB_TOKENS_AUTHENTICATION_MIDDLEWARE "Enable JWT Authentication Middleware" ON)
option(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP_BROTLI "Enable response compression (gzip/brotli)" ON)
if(C_REST_ENABLE_RATE_LIMITING_THROTTLING_MIDDLEWARE)
    add_definitions(-DC_REST_ENABLE_RATE_LIMITING_THROTTLING_MIDDLEWARE)
endif()
if(C_REST_ENABLE_JWT_JSON_WEB_TOKENS_AUTHENTICATION_MIDDLEWARE)
    add_definitions(-DC_REST_ENABLE_JWT_JSON_WEB_TOKENS_AUTHENTICATION_MIDDLEWARE)
endif()
option(TRY_SYS_LIB "Fallback to system packages if vcpkg is not available" ON)
option(C_REST_ENABLE_FULL_MULTIPART_FORM_STREAMING "Enable Full Multipart Form Streaming" ON)
if(C_REST_ENABLE_FULL_MULTIPART_FORM_STREAMING)
    add_definitions(-DC_REST_ENABLE_FULL_MULTIPART_FORM_STREAMING)
endif()
option(C_REST_ENABLE_SERVER_SENT_EVENTS_SSE "Enable Server-Sent Events (SSE)" ON)
if(C_REST_ENABLE_SERVER_SENT_EVENTS_SSE)
    add_definitions(-DC_REST_ENABLE_SERVER_SENT_EVENTS_SSE)
endif()

macro(c_rest_framework_resolve_dependency PkgName FetchRepo FetchTag)
    set(_VCPKG_APPLICABLE ON)
    if(MINGW OR CYGWIN OR (MSVC AND MSVC_VERSION LESS 1500))
        set(_VCPKG_APPLICABLE OFF)
    endif()
    
    set(${PkgName}_RESOLVED OFF)

    if(_VCPKG_APPLICABLE AND DEFINED CMAKE_TOOLCHAIN_FILE AND CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg.cmake")
        find_package(${PkgName} CONFIG QUIET)
        if(${PkgName}_FOUND)
            message(STATUS "Found ${PkgName} via vcpkg")
            set(${PkgName}_RESOLVED ON)
        endif()
    endif()

    if(NOT ${PkgName}_RESOLVED AND TRY_SYS_LIB)
        find_package(${PkgName} QUIET NO_CMAKE_BUILDS_PATH)
        if(${PkgName}_FOUND)
            message(STATUS "Found ${PkgName} via system packages")
            set(${PkgName}_RESOLVED ON)
        endif()
    endif()

    if(NOT ${PkgName}_RESOLVED)
        message(STATUS "Fetching ${PkgName} via FetchContent")
        FetchContent_Declare(${PkgName}
            GIT_REPOSITORY ${FetchRepo}
            GIT_TAG ${FetchTag}
            GIT_CONFIG "core.autocrlf=false"
            UPDATE_COMMAND ""
        )
        FetchContent_MakeAvailable(${PkgName})
        set(${PkgName}_RESOLVED ON)
    elseif(C_REST_FRAMEWORK_USE_BEARSSL)
        target_compile_definitions(${tgt} ${scope} C_REST_HAS_TLS C_REST_USE_BEARSSL)
    elseif(C_REST_FRAMEWORK_USE_SCHANNEL)
        target_compile_definitions(${tgt} ${scope} C_REST_HAS_TLS C_REST_USE_SCHANNEL)
    elseif(C_REST_FRAMEWORK_USE_GNUTLS)
        target_compile_definitions(${tgt} ${scope} C_REST_HAS_TLS C_REST_USE_GNUTLS)
    elseif(C_REST_FRAMEWORK_USE_BOTAN)
        target_compile_definitions(${tgt} ${scope} C_REST_HAS_TLS C_REST_USE_BOTAN)
    elseif(C_REST_FRAMEWORK_USE_COMMONCRYPTO)
        target_compile_definitions(${tgt} ${scope} C_REST_HAS_TLS C_REST_USE_COMMONCRYPTO)
    elseif(C_REST_FRAMEWORK_USE_WINCRYPT)
        target_compile_definitions(${tgt} ${scope} C_REST_HAS_TLS C_REST_USE_WINCRYPT)
    endif()
endmacro()

macro(c_rest_framework_apply_crypto tgt scope)
    if(C_REST_FRAMEWORK_USE_MBEDTLS)
        c_rest_framework_resolve_dependency(mbedtls https://github.com/Mbed-TLS/mbedtls.git v3.4.0)
        target_link_libraries(${tgt} ${scope} mbedtls mbedcrypto mbedx509)
        target_compile_definitions(${tgt} ${scope} C_REST_HAS_TLS C_REST_USE_MBEDTLS)
    elseif(C_REST_FRAMEWORK_USE_OPENSSL)
        c_rest_framework_resolve_dependency(OpenSSL https://github.com/openssl/openssl.git openssl-3.1.2)
        target_link_libraries(${tgt} ${scope} OpenSSL::SSL OpenSSL::Crypto)
        target_compile_definitions(${tgt} ${scope} C_REST_HAS_TLS C_REST_USE_OPENSSL)
    elseif(C_REST_FRAMEWORK_USE_LIBRESSL)
        c_rest_framework_resolve_dependency(LibreSSL https://github.com/libressl/portable.git v3.8.0)
        target_link_libraries(${tgt} ${scope} LibreSSL::TLS LibreSSL::Crypto)
        target_compile_definitions(${tgt} ${scope} C_REST_HAS_TLS C_REST_USE_LIBRESSL)
    elseif(C_REST_FRAMEWORK_USE_BORINGSSL)
        c_rest_framework_resolve_dependency(BoringSSL https://github.com/google/boringssl.git master)
        target_link_libraries(${tgt} ${scope} crypto ssl)
        target_compile_definitions(${tgt} ${scope} C_REST_HAS_TLS C_REST_USE_BORINGSSL)
    elseif(C_REST_FRAMEWORK_USE_WOLFSSL)
        set(WOLFSSL_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(WOLFSSL_CRYPT_TESTS OFF CACHE BOOL "" FORCE)
        c_rest_framework_resolve_dependency(wolfssl https://github.com/wolfSSL/wolfssl.git v5.6.6-stable)
        target_link_libraries(${tgt} ${scope} wolfssl)
        target_compile_definitions(${tgt} ${scope} C_REST_HAS_TLS C_REST_USE_WOLFSSL)
    elseif(C_REST_FRAMEWORK_USE_S2N)
        set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
        c_rest_framework_resolve_dependency(s2n https://github.com/aws/s2n-tls.git main)
        target_link_libraries(${tgt} ${scope} s2n::s2n)
        target_compile_definitions(${tgt} ${scope} C_REST_HAS_TLS C_REST_USE_S2N)
    elseif(C_REST_FRAMEWORK_USE_BEARSSL)
        target_compile_definitions(${tgt} ${scope} C_REST_HAS_TLS C_REST_USE_BEARSSL)
    elseif(C_REST_FRAMEWORK_USE_SCHANNEL)
        target_compile_definitions(${tgt} ${scope} C_REST_HAS_TLS C_REST_USE_SCHANNEL)
    elseif(C_REST_FRAMEWORK_USE_GNUTLS)
        target_compile_definitions(${tgt} ${scope} C_REST_HAS_TLS C_REST_USE_GNUTLS)
    elseif(C_REST_FRAMEWORK_USE_BOTAN)
        target_compile_definitions(${tgt} ${scope} C_REST_HAS_TLS C_REST_USE_BOTAN)
    elseif(C_REST_FRAMEWORK_USE_COMMONCRYPTO)
        target_compile_definitions(${tgt} ${scope} C_REST_HAS_TLS C_REST_USE_COMMONCRYPTO)
    elseif(C_REST_FRAMEWORK_USE_WINCRYPT)
        target_compile_definitions(${tgt} ${scope} C_REST_HAS_TLS C_REST_USE_WINCRYPT)
    endif()
endmacro()

include(FetchContent)
set(C_ABSTRACT_HTTP_USE_MBEDTLS ${C_REST_FRAMEWORK_USE_MBEDTLS} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_OPENSSL ${C_REST_FRAMEWORK_USE_OPENSSL} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_LIBRESSL ${C_REST_FRAMEWORK_USE_LIBRESSL} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_BORINGSSL ${C_REST_FRAMEWORK_USE_BORINGSSL} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_WOLFSSL ${C_REST_FRAMEWORK_USE_WOLFSSL} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_S2N ${C_REST_FRAMEWORK_USE_S2N} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_BEARSSL ${C_REST_FRAMEWORK_USE_BEARSSL} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_SCHANNEL ${C_REST_FRAMEWORK_USE_SCHANNEL} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_GNUTLS ${C_REST_FRAMEWORK_USE_GNUTLS} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_BOTAN ${C_REST_FRAMEWORK_USE_BOTAN} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_COMMONCRYPTO ${C_REST_FRAMEWORK_USE_COMMONCRYPTO} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_WINCRYPT ${C_REST_FRAMEWORK_USE_WINCRYPT} CACHE BOOL "" FORCE)

set(C_ABSTRACT_HTTP_USE_BEARSSL ${C_REST_FRAMEWORK_USE_BEARSSL} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_SCHANNEL ${C_REST_FRAMEWORK_USE_SCHANNEL} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_GNUTLS ${C_REST_FRAMEWORK_USE_GNUTLS} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_BOTAN ${C_REST_FRAMEWORK_USE_BOTAN} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_COMMONCRYPTO ${C_REST_FRAMEWORK_USE_COMMONCRYPTO} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_WINCRYPT ${C_REST_FRAMEWORK_USE_WINCRYPT} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_LSQUIC ${C_REST_FRAMEWORK_USE_LSQUIC} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_PICOQUIC ${C_REST_FRAMEWORK_USE_PICOQUIC} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_NGHTTP3 ${C_REST_FRAMEWORK_USE_NGHTTP3} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_MSH3 ${C_REST_FRAMEWORK_USE_MSH3} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_ARIA2 ${C_REST_FRAMEWORK_USE_ARIA2} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_LIBSOUP3 ${C_REST_FRAMEWORK_USE_LIBSOUP3} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_LIBUV ${C_REST_FRAMEWORK_USE_LIBUV} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_LIBEVENT ${C_REST_FRAMEWORK_USE_LIBEVENT} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_LIBFETCH ${C_REST_FRAMEWORK_USE_LIBFETCH} CACHE BOOL "" FORCE)

if(C_REST_FRAMEWORK_FETCH_C_FS)
    set(CDD_DEPS "FETCHCONTENT" CACHE STRING "" FORCE)
    if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../c-fs/CMakeLists.txt")
        FetchContent_Declare(
          cfs
          SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../c-fs"
        )
    else()
        FetchContent_Declare(
          cfs
          GIT_REPOSITORY https://github.com/SamuelMarks/c-fs.git
          GIT_TAG master
          GIT_CONFIG "core.autocrlf=false"
        )
    endif()
    FetchContent_MakeAvailable(cfs)
endif()

if(C_REST_FRAMEWORK_FETCH_CDD_C)
    if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../cdd-c/CMakeLists.txt")
        FetchContent_Declare(
          cdd_c
          SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../cdd-c"
        )
    else()
        FetchContent_Declare(
          cdd_c
          GIT_REPOSITORY https://github.com/SamuelMarks/cdd-c.git
          GIT_TAG master
          GIT_CONFIG "core.autocrlf=false"
        )
    endif()
    FetchContent_MakeAvailable(cdd_c)
endif()

if(C_REST_FRAMEWORK_FETCH_CAH)
    set(C_ABSTRACT_HTTP_BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(C_ORM_BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(C_FS_BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(C_CDD_BUILD_TESTING OFF CACHE BOOL "" FORCE)
    
    if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../c-abstract-http/CMakeLists.txt")
        FetchContent_Declare(
          c_abstract_http
          SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../c-abstract-http"
        )
    else()
        FetchContent_Declare(
          c_abstract_http
          GIT_REPOSITORY https://github.com/SamuelMarks/c-abstract-http.git
          GIT_TAG master
          GIT_CONFIG "core.autocrlf=false"
        )
    endif()
    FetchContent_MakeAvailable(c_abstract_http)
    add_definitions(-DC_REST_FRAMEWORK_USE_REAL_CAH)
endif()

if(C_REST_FRAMEWORK_FETCH_C_ORM)
    if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../c-orm/CMakeLists.txt")
        FetchContent_Declare(
          c_orm
          SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../c-orm"
        )
    else()
        FetchContent_Declare(
          c_orm
          GIT_REPOSITORY https://github.com/SamuelMarks/c-orm.git
          GIT_TAG master
          GIT_CONFIG "core.autocrlf=false"
        )
    endif()
    FetchContent_MakeAvailable(c_orm)
    add_definitions(-DC_REST_FRAMEWORK_USE_REAL_C_ORM)
endif()

macro(c_rest_framework_resolve_parson)
    set(_VCPKG_APPLICABLE ON)
    if(MINGW OR CYGWIN OR (MSVC AND MSVC_VERSION LESS 1500))
        set(_VCPKG_APPLICABLE OFF)
    endif()

    set(parson_RESOLVED OFF)

    if(_VCPKG_APPLICABLE AND DEFINED CMAKE_TOOLCHAIN_FILE AND CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg.cmake") 
        find_package(parson CONFIG QUIET)
        if(parson_FOUND)
            message(STATUS "Found parson via vcpkg")
            set(parson_RESOLVED ON)
            target_link_libraries(c-rest-framework PUBLIC parson::parson)
        endif()
    endif()

    if(NOT parson_RESOLVED AND TRY_SYS_LIB)
        find_package(parson QUIET NO_CMAKE_BUILDS_PATH)
        if(parson_FOUND)
            message(STATUS "Found parson via system packages")
            set(parson_RESOLVED ON)
            target_link_libraries(c-rest-framework PUBLIC parson)
        endif()
    endif()

    if(NOT parson_RESOLVED)
        message(STATUS "Fetching parson via FetchContent")
        if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../../parson/CMakeLists.txt")
            message(STATUS "Using local parson")
            FetchContent_Declare(parson SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../parson")
        else()
            FetchContent_Declare(
                parson
                GIT_REPOSITORY https://github.com/kgabis/parson.git
                GIT_TAG master
          GIT_CONFIG "core.autocrlf=false"
            )
        endif()
        set(OLD_BSL ${BUILD_SHARED_LIBS})
        set(BUILD_SHARED_LIBS OFF)
        FetchContent_MakeAvailable(parson)
        set(BUILD_SHARED_LIBS ${OLD_BSL})
        FetchContent_GetProperties(parson)
        target_sources(c-rest-framework PRIVATE ${parson_SOURCE_DIR}/parson.c)
        if(MSVC)
            set_source_files_properties(${parson_SOURCE_DIR}/parson.c PROPERTIES COMPILE_FLAGS "/wd4232")
        endif()
        target_include_directories(c-rest-framework PUBLIC ${parson_SOURCE_DIR})
        set(parson_RESOLVED ON)
    elseif(C_REST_FRAMEWORK_USE_BEARSSL)
        target_compile_definitions(${tgt} ${scope} C_REST_HAS_TLS C_REST_USE_BEARSSL)
    elseif(C_REST_FRAMEWORK_USE_SCHANNEL)
        target_compile_definitions(${tgt} ${scope} C_REST_HAS_TLS C_REST_USE_SCHANNEL)
    elseif(C_REST_FRAMEWORK_USE_GNUTLS)
        target_compile_definitions(${tgt} ${scope} C_REST_HAS_TLS C_REST_USE_GNUTLS)
    elseif(C_REST_FRAMEWORK_USE_BOTAN)
        target_compile_definitions(${tgt} ${scope} C_REST_HAS_TLS C_REST_USE_BOTAN)
    elseif(C_REST_FRAMEWORK_USE_COMMONCRYPTO)
        target_compile_definitions(${tgt} ${scope} C_REST_HAS_TLS C_REST_USE_COMMONCRYPTO)
    elseif(C_REST_FRAMEWORK_USE_WINCRYPT)
        target_compile_definitions(${tgt} ${scope} C_REST_HAS_TLS C_REST_USE_WINCRYPT)
    endif()
endmacro()

if(C_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION)
    add_definitions(-DC_REST_FRAMEWORK_MULTIPLATFORM_INTEGRATION)
endif()

if(MSVC)
    # Step 17: Define strict compilation flags for MSVC (/W4 /WX- /Za)
    # We use /WX- to disable treating warnings as errors by default but use high warning level
    # Note: /Za strictly enforces ANSI C but might cause issues with Windows headers.
    add_compile_options(/W4 /WX-)

    # We apply /Za only to specific source files since windows.h is not ANSI compliant
    set_source_files_properties(
        src/c_rest_hot_reload.c
        src/modality.c
        src/modality_sync.c
        src/modality_single_thread.c
        src/modality_async.c
        src/modality_multi_thread.c
        src/modality_multi_process.c
        src/modality_greenthread.c
        src/modality_message_passing.c
        src/parser.c
        src/client.c
        src/tls.c
        src/multipart.c
        src/router.c
        src/middleware.c
        src/request.c
        src/response.c
        src/c_orm_middleware.c
        src/c_orm_crud.c
        src/c_rest_string.c
        src/c_rest_hashmap.c
        src/c_rest_list.c
        src/c_rest_pool.c
        src/c_rest_ts_queue.c
        src/c_rest_mem.c
        src/c_rest_str_utils.c
        src/c_rest_endian.c
        src/c_rest_base64.c
        src/c_rest_crypto.c
        src/c_rest_time.c
        src/c_rest_graphql.c
        src/c_rest_template.c
        src/c_rest_http23.c
        src/c_rest_rate_limit.c
        src/c_rest_jwt_middleware.c
        src/websocket.c
        PROPERTIES COMPILE_OPTIONS "/Za"
    )

    

    if(C_REST_FRAMEWORK_STATIC_CRT)
        foreach(flag_var
            CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
            CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
            if(${flag_var} MATCHES "/MD")
                set(_tmp "${${flag_var}}")
                string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${_tmp}")
            endif()
        endforeach()
    endif()


    if(C_REST_FRAMEWORK_UNICODE)
        add_definitions(-DUNICODE -D_UNICODE)
    elseif(C_REST_FRAMEWORK_ANSI)
        add_definitions(-D_MBCS)
    endif()
elseif(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    # Step 16: Define C89 strict compilation flags for GCC (-ansi -pedantic -Wall)
    set(C_REST_GNU_FLAGS -ansi -pedantic -Wall)
    
    if(C_REST_FRAMEWORK_USE_ASAN)
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address)
    endif()

    if(C_REST_FRAMEWORK_USE_TSAN)
        add_compile_options(-fsanitize=thread)
        add_link_options(-fsanitize=thread)
    endif()
endif()

add_library(c-rest-framework
    src/modality.c
    src/modality_sync.c
    src/modality_single_thread.c
    src/modality_async.c
    src/modality_multi_thread.c
    src/modality_multi_process.c
    src/modality_greenthread.c
    src/modality_message_passing.c
    src/parser.c
    src/client.c
    src/tls.c
    src/multipart.c
    src/router.c
    src/middleware.c
    src/request.c
    src/response.c
    src/c_orm_middleware.c
    src/c_orm_crud.c
    src/c_rest_string.c
    src/c_rest_hashmap.c
    src/c_rest_list.c
    src/c_rest_pool.c
    src/c_rest_ts_queue.c
    src/c_rest_mem.c
    src/c_rest_str_utils.c
    src/c_rest_endian.c
    src/c_rest_base64.c
    src/c_rest_crypto.c
    src/c_rest_time.c
    src/c_rest_openapi.c
    src/c_rest_graphql.c
    src/c_rest_template.c
    src/c_rest_http23.c
    src/c_rest_rate_limit.c
    src/c_rest_jwt_middleware.c
    src/websocket.c
    src/c_rest_compression.c
)

if(C_REST_GNU_FLAGS)
    target_compile_options(c-rest-framework PRIVATE ${C_REST_GNU_FLAGS})
endif()

if(C_REST_ENABLE_HOT_RELOADING_AUTO_RESTART)
    target_sources(c-rest-framework PRIVATE src/c_rest_hot_reload.c)
    target_compile_definitions(c-rest-framework PUBLIC C_REST_ENABLE_HOT_RELOADING_AUTO_RESTART)
endif()

if(C_REST_ENABLE_SERVER_SENT_EVENTS_SSE)
    target_sources(c-rest-framework PRIVATE src/c_rest_sse.c)
endif()

if(C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP_BROTLI)
    target_compile_definitions(c-rest-framework PUBLIC C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_GZIP)
    
    set(_VCPKG_APPLICABLE ON)
    if(MINGW OR CYGWIN OR (MSVC AND MSVC_VERSION LESS 1500))
        set(_VCPKG_APPLICABLE OFF)
    endif()
    
    set(ZLIB_RESOLVED OFF)
    if(_VCPKG_APPLICABLE AND DEFINED CMAKE_TOOLCHAIN_FILE AND CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg.cmake")
        find_package(ZLIB CONFIG QUIET)
        if(ZLIB_FOUND)
            set(ZLIB_RESOLVED ON)
            target_link_libraries(c-rest-framework PUBLIC ZLIB::ZLIB)
        endif()
    endif()
    if(NOT ZLIB_RESOLVED AND NOT MINGW)
        find_package(ZLIB QUIET)
        if(ZLIB_FOUND)
            set(ZLIB_RESOLVED ON)
            target_link_libraries(c-rest-framework PUBLIC ZLIB::ZLIB)
        endif()
    endif()
    if(NOT ZLIB_RESOLVED)
            FetchContent_Declare(
                zlib
                GIT_REPOSITORY https://github.com/madler/zlib.git
                GIT_TAG master
                GIT_CONFIG "core.autocrlf=false"
            )
            FetchContent_MakeAvailable(zlib)
            if(MSVC AND MSVC_VERSION LESS 1600)
                if(TARGET zlib)
                    target_compile_definitions(zlib PRIVATE EWOULDBLOCK=140)
                endif()
                if(TARGET zlibstatic)
                    target_compile_definitions(zlibstatic PRIVATE EWOULDBLOCK=140)
                endif()
            endif()
            target_link_libraries(c-rest-framework PUBLIC zlib)
            target_include_directories(c-rest-framework PUBLIC ${zlib_SOURCE_DIR})
        endif()

    if(NOT MSVC OR MSVC_VERSION GREATER_EQUAL 1600)
        target_compile_definitions(c-rest-framework PUBLIC C_REST_FRAMEWORK_ENABLE_RESPONSE_COMPRESSION_BROTLI)
        set(BROTLI_RESOLVED OFF)
        if(_VCPKG_APPLICABLE AND DEFINED CMAKE_TOOLCHAIN_FILE AND CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg.cmake")
            find_package(unofficial-brotli CONFIG QUIET)
            if(unofficial-brotli_FOUND)
                set(BROTLI_RESOLVED ON)
                target_link_libraries(c-rest-framework PUBLIC unofficial::brotli::brotlienc)
            endif()
        endif()
        if(NOT BROTLI_RESOLVED)
            FetchContent_Declare(
                brotli
                GIT_REPOSITORY https://github.com/google/brotli.git
                GIT_TAG master
                GIT_CONFIG "core.autocrlf=false"
                UPDATE_COMMAND ""
            )
            FetchContent_MakeAvailable(brotli)
            
            if(TARGET brotlienc-static)
                target_link_libraries(c-rest-framework PUBLIC brotlienc-static brotlidec-static brotlicommon-static)
            else()
                target_link_libraries(c-rest-framework PUBLIC brotlienc brotlidec brotlicommon)
            endif()
            
            target_include_directories(c-rest-framework PUBLIC ${brotli_SOURCE_DIR}/c/include)
        endif()
    endif()
endif()

if(WIN32)
    set_target_properties(c-rest-framework PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

c_rest_framework_apply_crypto(c-rest-framework PUBLIC)
c_rest_framework_resolve_parson()


if(C_REST_FRAMEWORK_FETCH_CAH)
    target_link_libraries(c-rest-framework PUBLIC c-abstract-http)

endif()

if(C_REST_FRAMEWORK_FETCH_C_ORM)
    target_link_libraries(c-rest-framework PUBLIC c-orm)
else()
    target_sources(c-rest-framework PRIVATE src/c_orm_mock.c)
endif()

if(C_REST_FRAMEWORK_FETCH_C_FS)
    target_link_libraries(c-rest-framework PUBLIC cfs)
endif()

if(C_REST_FRAMEWORK_FETCH_CDD_C)
    target_link_libraries(c-rest-framework PUBLIC c_cdd)
    add_definitions(-DC_REST_FRAMEWORK_USE_CDD_C)
endif()

if(WIN32)
    target_sources(c-rest-framework PRIVATE src/platform_win32.c)
    target_link_libraries(c-rest-framework PUBLIC ws2_32 advapi32)
elseif(CMAKE_SYSTEM_NAME STREQUAL "DOS")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ml")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ml")
    add_compile_definitions(CDD_DOS=1)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} option stack=32k")
    target_sources(c-rest-framework PRIVATE src/platform_dos.c)
else()
    target_sources(c-rest-framework PRIVATE src/platform_posix.c)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_link_libraries(c-rest-framework PUBLIC pthread)
    endif()
endif()


target_include_directories(c-rest-framework PUBLIC include)

if(C_REST_FRAMEWORK_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING)
    target_compile_definitions(c-rest-framework PUBLIC C_REST_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING)
endif()

if(C_REST_FRAMEWORK_ENABLE_GRAPHQL)
    target_compile_definitions(c-rest-framework PUBLIC C_REST_FRAMEWORK_ENABLE_GRAPHQL)
endif()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    enable_testing()
    add_subdirectory(tests)
    add_subdirectory(examples)
endif()







