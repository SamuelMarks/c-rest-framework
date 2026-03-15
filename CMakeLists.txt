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
option(TRY_SYS_LIB "Fallback to system packages if vcpkg is not available" ON)

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
set(C_ABSTRACT_HTTP_USE_LSQUIC ${C_REST_FRAMEWORK_USE_LSQUIC} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_PICOQUIC ${C_REST_FRAMEWORK_USE_PICOQUIC} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_NGHTTP3 ${C_REST_FRAMEWORK_USE_NGHTTP3} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_MSH3 ${C_REST_FRAMEWORK_USE_MSH3} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_ARIA2 ${C_REST_FRAMEWORK_USE_ARIA2} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_LIBSOUP3 ${C_REST_FRAMEWORK_USE_LIBSOUP3} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_LIBUV ${C_REST_FRAMEWORK_USE_LIBUV} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_LIBEVENT ${C_REST_FRAMEWORK_USE_LIBEVENT} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_LIBFETCH ${C_REST_FRAMEWORK_USE_LIBFETCH} CACHE BOOL "" FORCE)

if(C_REST_FRAMEWORK_FETCH_CAH)
    set(C_ABSTRACT_HTTP_BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(C_ORM_BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(C_FS_BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(C_CDD_BUILD_TESTING OFF CACHE BOOL "" FORCE)
    
    FetchContent_Declare(
      c_abstract_http
      GIT_REPOSITORY https://github.com/SamuelMarks/c-abstract-http.git
      GIT_TAG master
    )
    FetchContent_MakeAvailable(c_abstract_http)
    add_definitions(-DC_REST_FRAMEWORK_USE_REAL_CAH)
endif()

if(C_REST_FRAMEWORK_FETCH_C_ORM)
    FetchContent_Declare(
      c_orm
      GIT_REPOSITORY https://github.com/SamuelMarks/c-orm.git
      GIT_TAG master
    )
    FetchContent_MakeAvailable(c_orm)
    add_definitions(-DC_REST_FRAMEWORK_USE_REAL_C_ORM)
endif()

if(C_REST_FRAMEWORK_FETCH_C_FS)
    FetchContent_Declare(
      cfs
      GIT_REPOSITORY https://github.com/SamuelMarks/c-fs.git
      GIT_TAG master
    )
    FetchContent_MakeAvailable(cfs)
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
        FetchContent_Declare(
            parson
            GIT_REPOSITORY https://github.com/kgabis/parson.git
            GIT_TAG master
        )
        FetchContent_GetProperties(parson)
        if(NOT parson_POPULATED)
            FetchContent_Populate(parson)
        endif()
        target_sources(c-rest-framework PRIVATE ${parson_SOURCE_DIR}/parson.c)
        target_include_directories(c-rest-framework PUBLIC ${parson_SOURCE_DIR})
        set(parson_RESOLVED ON)
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
    add_compile_options(-ansi -pedantic -Wall)
    
    if(C_REST_FRAMEWORK_USE_ASAN)
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address)
    endif()

    if(C_REST_FRAMEWORK_USE_TSAN)
        add_compile_options(-fsanitize=thread)
        add_link_options(-fsanitize=thread)
    endif()
endif()

add_library(c-rest-framework STATIC 
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
)

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

if(WIN32)
    target_sources(c-rest-framework PRIVATE src/platform_win32.c)
    target_link_libraries(c-rest-framework PUBLIC ws2_32 advapi32)
elseif(CMAKE_SYSTEM_NAME STREQUAL "DOS")
    target_sources(c-rest-framework PRIVATE src/platform_dos.c)
else()
    target_sources(c-rest-framework PRIVATE src/platform_posix.c)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_link_libraries(c-rest-framework PUBLIC pthread)
    endif()
endif()


target_include_directories(c-rest-framework PUBLIC include)

enable_testing()
add_subdirectory(tests)
add_subdirectory(examples)



