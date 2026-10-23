cmake_minimum_required(VERSION 3.14)
if(POLICY CMP0077)
  cmake_policy(SET CMP0077 NEW)
  set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
endif()
if(POLICY CMP0079)
  cmake_policy(SET CMP0079 NEW)
endif()
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()
if(POLICY CMP0091)
  cmake_policy(SET CMP0091 NEW)
  set(CMAKE_POLICY_DEFAULT_CMP0091 NEW)
endif()

if(DEFINED CMAKE_INTERPROCEDURAL_OPTIMIZATION)
    set(SAVE_IPO ${CMAKE_INTERPROCEDURAL_OPTIMIZATION})
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF CACHE BOOL "" FORCE)
endif()

if(CMAKE_CROSSCOMPILING_EMULATOR MATCHES "wine" AND CMAKE_C_COMPILER MATCHES "cl")
    find_program(MSVC_LIB_EXE NAMES lib.exe lib HINTS "${MSVC_WINE_PATH}/bin/x64" "$ENV{HOME}/my_msvc/bin/x64")
    find_program(MSVC_LINK_EXE NAMES link.exe link HINTS "${MSVC_WINE_PATH}/bin/x64" "$ENV{HOME}/my_msvc/bin/x64")
    if(MSVC_LIB_EXE)
        set(CMAKE_AR "${MSVC_LIB_EXE}" CACHE FILEPATH "Archiver" FORCE)
    else()
        set(CMAKE_AR "lib.exe" CACHE FILEPATH "Archiver" FORCE)
    endif()
    if(MSVC_LINK_EXE)
        set(CMAKE_LINKER "${MSVC_LINK_EXE}" CACHE FILEPATH "Linker" FORCE)
    else()
        set(CMAKE_LINKER "link.exe" CACHE FILEPATH "Linker" FORCE)
    endif()
    set(CMAKE_C_CREATE_STATIC_LIBRARY "<CMAKE_AR> /nologo /OUT:<TARGET> <LINK_FLAGS> <OBJECTS> && test -f <TARGET> || (sleep 1 && <CMAKE_AR> /nologo /OUT:<TARGET> <LINK_FLAGS> <OBJECTS> && test -f <TARGET>)" CACHE STRING "Create static lib using lib.exe" FORCE)
    set(CMAKE_CXX_CREATE_STATIC_LIBRARY "<CMAKE_AR> /nologo /OUT:<TARGET> <LINK_FLAGS> <OBJECTS> && test -f <TARGET> || (sleep 1 && <CMAKE_AR> /nologo /OUT:<TARGET> <LINK_FLAGS> <OBJECTS> && test -f <TARGET>)" CACHE STRING "Create static lib using lib.exe" FORCE)
    set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_LINKER> /nologo <OBJECTS> ${CMAKE_C_LINK_FLAGS} <LINK_FLAGS> <LINK_LIBRARIES> /out:<TARGET> /implib:<TARGET_IMPLIB> /pdb:<TARGET_PDB> /version:<TARGET_VERSION_MAJOR>.<TARGET_VERSION_MINOR> && test -f <TARGET> || (sleep 1 && <CMAKE_LINKER> /nologo <OBJECTS> ${CMAKE_C_LINK_FLAGS} <LINK_FLAGS> <LINK_LIBRARIES> /out:<TARGET> /implib:<TARGET_IMPLIB> /pdb:<TARGET_PDB> /version:<TARGET_VERSION_MAJOR>.<TARGET_VERSION_MINOR> && test -f <TARGET>)" CACHE STRING "" FORCE)
    set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_LINKER> /nologo <OBJECTS> ${CMAKE_CXX_LINK_FLAGS} <LINK_FLAGS> <LINK_LIBRARIES> /out:<TARGET> /implib:<TARGET_IMPLIB> /pdb:<TARGET_PDB> /version:<TARGET_VERSION_MAJOR>.<TARGET_VERSION_MINOR> && test -f <TARGET> || (sleep 1 && <CMAKE_LINKER> /nologo <OBJECTS> ${CMAKE_CXX_LINK_FLAGS} <LINK_FLAGS> <LINK_LIBRARIES> /out:<TARGET> /implib:<TARGET_IMPLIB> /pdb:<TARGET_PDB> /version:<TARGET_VERSION_MAJOR>.<TARGET_VERSION_MINOR> && test -f <TARGET>)" CACHE STRING "" FORCE)
    set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_LINKER> /nologo <OBJECTS> ${CMAKE_C_LINK_FLAGS} <LINK_FLAGS> <LINK_LIBRARIES> /out:<TARGET> /implib:<TARGET_IMPLIB> /pdb:<TARGET_PDB> /dll /version:<TARGET_VERSION_MAJOR>.<TARGET_VERSION_MINOR> && test -f <TARGET> || (sleep 1 && <CMAKE_LINKER> /nologo <OBJECTS> ${CMAKE_C_LINK_FLAGS} <LINK_FLAGS> <LINK_LIBRARIES> /out:<TARGET> /implib:<TARGET_IMPLIB> /pdb:<TARGET_PDB> /dll /version:<TARGET_VERSION_MAJOR>.<TARGET_VERSION_MINOR> && test -f <TARGET>)" CACHE STRING "" FORCE)
    set(CMAKE_CXX_CREATE_SHARED_LIBRARY "<CMAKE_LINKER> /nologo <OBJECTS> ${CMAKE_CXX_LINK_FLAGS} <LINK_FLAGS> <LINK_LIBRARIES> /out:<TARGET> /implib:<TARGET_IMPLIB> /pdb:<TARGET_PDB> /dll /version:<TARGET_VERSION_MAJOR>.<TARGET_VERSION_MINOR> && test -f <TARGET> || (sleep 1 && <CMAKE_LINKER> /nologo <OBJECTS> ${CMAKE_CXX_LINK_FLAGS} <LINK_FLAGS> <LINK_LIBRARIES> /out:<TARGET> /implib:<TARGET_IMPLIB> /pdb:<TARGET_PDB> /dll /version:<TARGET_VERSION_MAJOR>.<TARGET_VERSION_MINOR> && test -f <TARGET>)" CACHE STRING "" FORCE)
endif()

if(APPLE)
    if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "10.10" CACHE STRING "Minimum OS X deployment version")
    endif()
endif()

project(c-rest-framework C)

if(APPLE AND CMAKE_C_COMPILER_ID MATCHES "GNU")
    execute_process(COMMAND xcrun --show-sdk-path OUTPUT_VARIABLE _SDK_PATH OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(_SDK_PATH AND EXISTS "${_SDK_PATH}")
        add_compile_options(-iframework "${_SDK_PATH}/System/Library/Frameworks")
    endif()
    add_compile_options(-mmacosx-version-min=10.10)
endif()

if(MSVC)
    string(REGEX REPLACE "/W[0-4]" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
    string(REGEX REPLACE "-W[0-4]" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
    string(REGEX REPLACE "/OF" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
    string(REPLACE "/Zi" "/Z7" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
    string(REPLACE "-Zi" "/Z7" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
    string(REPLACE "/Zi" "/Z7" CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
    string(REPLACE "-Zi" "/Z7" CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
    if(CMAKE_CROSSCOMPILING_EMULATOR MATCHES "wine")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /MANIFEST:NO")
        string(REPLACE "/FS" "" CMAKE_C_COMPILE_OBJECT "${CMAKE_C_COMPILE_OBJECT}")
        if(NOT CMAKE_C_COMPILE_OBJECT MATCHES "test -s")
            set(_ORIG_C_COMPILE_OBJECT "${CMAKE_C_COMPILE_OBJECT}")
            set(CMAKE_C_COMPILE_OBJECT "${_ORIG_C_COMPILE_OBJECT} && test -s <OBJECT> || (rm -f <OBJECT> && sleep 1 && ${_ORIG_C_COMPILE_OBJECT} && test -s <OBJECT>) || (rm -f <OBJECT> && sleep 2 && ${_ORIG_C_COMPILE_OBJECT} && test -s <OBJECT>)")
        endif()
        set(CMAKE_C_ABI_COMPILED TRUE CACHE INTERNAL "")
        set(CMAKE_CXX_ABI_COMPILED TRUE CACHE INTERNAL "")
        set(COMPILER_HAS_DEPRECATED_ATTR 0 CACHE INTERNAL "")
        set(COMPILER_HAS_DEPRECATED 1 CACHE INTERNAL "")
        set(HAVE_SYS_TYPES_H 1 CACHE INTERNAL "")
        set(HAVE_STDINT_H 1 CACHE INTERNAL "")
        set(HAVE_STDDEF_H 1 CACHE INTERNAL "")
        set(HAVE_OFF64_T FALSE CACHE INTERNAL "")
        set(OFF64_T "" CACHE INTERNAL "")
        set(HAVE_FSEEKO 0 CACHE INTERNAL "")
        set(HAVE_STDARG_H 1 CACHE INTERNAL "")
        set(HAVE_UNISTD_H 0 CACHE INTERNAL "")
        set(HAVE___ATTR__VIS_HIDDEN 0 CACHE INTERNAL "")
        set(HAVE_LIB_M 0 CACHE INTERNAL "")
        set(HAVE_SIZEOF_LONG_LONG TRUE CACHE INTERNAL "")
        set(SIZEOF_LONG_LONG 8 CACHE INTERNAL "")
        set(HAVE_SIZEOF___INT64 TRUE CACHE INTERNAL "")
        set(SIZEOF___INT64 8 CACHE INTERNAL "")
        set(CMAKE_HAVE_LIBC_PTHREAD 0 CACHE INTERNAL "")
        set(CMAKE_HAVE_PTHREADS_CREATE 0 CACHE INTERNAL "")
        set(CMAKE_HAVE_PTHREAD_CREATE 0 CACHE INTERNAL "")
        set(CMAKE_USE_WIN32_THREADS_INIT 1 CACHE INTERNAL "")
        set(Threads_FOUND TRUE CACHE INTERNAL "")
    elseif(MSVC_VERSION GREATER_EQUAL 1800)
        add_compile_options(/FS)
    endif()
    function(target_compile_features)
    endfunction()
endif()

string(TOUPPER "${PROJECT_NAME}" PROJECT_NAME_UPPER)
string(REPLACE "-" "_" PROJECT_NAME_SAFE "${PROJECT_NAME_UPPER}")
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

if(DEFINED SAVE_IPO)
    if(CMAKE_CROSSCOMPILING_EMULATOR MATCHES "wine")
        # Under wine LTO produces broken binaries, keeping it off
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF CACHE BOOL "" FORCE)

    # Avoid C2220 errors in unreferenced vars inside submodules
    else()
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ${SAVE_IPO} CACHE BOOL "" FORCE)
    endif()
endif()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    option(BUILD_TESTING "Build tests" ON)
else()
    option(BUILD_TESTING "Build tests" OFF)
endif()
include(CTest)

if(CMAKE_CROSSCOMPILING)
    set(CMAKE_DISABLE_FIND_PACKAGE_SQLite3 ON)
    set(CMAKE_DISABLE_FIND_PACKAGE_CURL ON)
endif()

# Global fPIC
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Set C standards
if(NOT MSVC)
    set(CMAKE_C_STANDARD 90)
    set(CMAKE_C_STANDARD_REQUIRED ON)
    set(CMAKE_C_EXTENSIONS ON)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(${PROJECT_NAME_SAFE}_CRT_LINKAGE "dynamic" CACHE STRING "CRT Linkage (static/dynamic)")
set_property(CACHE ${PROJECT_NAME_SAFE}_CRT_LINKAGE PROPERTY STRINGS "static" "dynamic")
set(${PROJECT_NAME_SAFE}_CHARSET "UNICODE" CACHE STRING "Character set (UNICODE/ANSI)")
set_property(CACHE ${PROJECT_NAME_SAFE}_CHARSET PROPERTY STRINGS "UNICODE" "ANSI")
option(${PROJECT_NAME_SAFE}_THREADING "Enable multi-threading" ON)
set(${PROJECT_NAME_SAFE}_MSVC_RTC "OFF" CACHE STRING "MSVC Runtime Checks (OFF, 1, s, u)")
set_property(CACHE ${PROJECT_NAME_SAFE}_MSVC_RTC PROPERTY STRINGS "OFF" "1" "s" "u" "RTC1" "RTCs" "RTCu")
option(${PROJECT_NAME_SAFE}_LTO "Enable Link-Time Optimization (IPO)" OFF)

if(DEFINED ${PROJECT_NAME_SAFE}_BUILD_TESTING)
    set(BUILD_TESTING ${${PROJECT_NAME_SAFE}_BUILD_TESTING} CACHE BOOL "Build the testing tree." FORCE)
endif()
if(DEFINED CDD_CHARSET AND NOT "${CDD_CHARSET}" STREQUAL "")
    set(${PROJECT_NAME_SAFE}_CHARSET "${CDD_CHARSET}" CACHE STRING "" FORCE)
endif()
if(DEFINED CDD_THREADING AND NOT "${CDD_THREADING}" STREQUAL "")
    set(${PROJECT_NAME_SAFE}_THREADING "${CDD_THREADING}" CACHE BOOL "" FORCE)
endif()
if(DEFINED CDD_MSVC_RTC AND NOT "${CDD_MSVC_RTC}" STREQUAL "")
    set(${PROJECT_NAME_SAFE}_MSVC_RTC "${CDD_MSVC_RTC}" CACHE STRING "" FORCE)
endif()
if(DEFINED CMAKE_INTERPROCEDURAL_OPTIMIZATION)
    set(${PROJECT_NAME_SAFE}_LTO "${CMAKE_INTERPROCEDURAL_OPTIMIZATION}" CACHE BOOL "" FORCE)
endif()

if(MSVC)
    if(DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
        if(CMAKE_MSVC_RUNTIME_LIBRARY MATCHES "DLL")
            set(${PROJECT_NAME_SAFE}_CRT_LINKAGE "dynamic" CACHE STRING "" FORCE)
        else()
            set(${PROJECT_NAME_SAFE}_CRT_LINKAGE "static" CACHE STRING "" FORCE)
        endif()
    endif()
    if(${PROJECT_NAME_SAFE}_CRT_LINKAGE STREQUAL "static")
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        foreach(flag_var
            CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
            CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
            if(${flag_var} MATCHES "[/-]MD")
                string(REGEX REPLACE "[/-]MD" "/MT" ${flag_var} "${${flag_var}}")
            endif()
        endforeach()
        set(_SUB_CRT_LINKAGE_CAP "Static")
    elseif(${PROJECT_NAME_SAFE}_CRT_LINKAGE STREQUAL "dynamic")
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
        foreach(flag_var
            CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
            CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
            if(${flag_var} MATCHES "[/-]MT")
                string(REGEX REPLACE "[/-]MT" "/MD" ${flag_var} "${${flag_var}}")
            endif()
        endforeach()
        set(_SUB_CRT_LINKAGE_CAP "Shared")
    endif()
    set(CDD_CRT_LINKAGE "${_SUB_CRT_LINKAGE_CAP}" CACHE STRING "" FORCE)
    set(C_STR_SPAN_CRT_LINKAGE "${_SUB_CRT_LINKAGE_CAP}" CACHE STRING "" FORCE)
    set(C89STRINGUTILS_CRT_LINKAGE "${_SUB_CRT_LINKAGE_CAP}" CACHE STRING "" FORCE)
    set(C_FS_CRT_LINKAGE "${_SUB_CRT_LINKAGE_CAP}" CACHE STRING "" FORCE)
    set(C_ABSTRACT_HTTP_CRT_LINKAGE "${_SUB_CRT_LINKAGE_CAP}" CACHE STRING "" FORCE)
    set(CDD_C_CRT_LINKAGE "${_SUB_CRT_LINKAGE_CAP}" CACHE STRING "" FORCE)
    set(C_ORM_CRT_LINKAGE "${${PROJECT_NAME_SAFE}_CRT_LINKAGE}" CACHE STRING "" FORCE)
endif()

if(MSVC AND NOT ${PROJECT_NAME_SAFE}_MSVC_RTC STREQUAL "OFF" AND NOT ${PROJECT_NAME_SAFE}_MSVC_RTC STREQUAL "")
    # MSVC error D8016: '/GL' (LTO) and '/RTC*' command-line options are incompatible.
    set(${PROJECT_NAME_SAFE}_LTO OFF CACHE BOOL "" FORCE)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF CACHE BOOL "" FORCE)
elseif(${PROJECT_NAME_SAFE}_LTO)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    if(MSVC)
        foreach(flag_var
            CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
            CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
            string(REGEX REPLACE "[/-]RTC[1suc]+" "" ${flag_var} "${${flag_var}}")
        endforeach()
    endif()
else()
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION FALSE)
endif()
option(${PROJECT_NAME_SAFE}_USE_ASAN "Enable AddressSanitizer" OFF)
option(${PROJECT_NAME_SAFE}_USE_TSAN "Enable ThreadSanitizer" OFF)

option(${PROJECT_NAME_SAFE}_USE_MBEDTLS "Use mbedtls for cryptography" OFF)
option(${PROJECT_NAME_SAFE}_USE_OPENSSL "Use OpenSSL for cryptography" OFF)
option(${PROJECT_NAME_SAFE}_USE_LIBRESSL "Use LibreSSL for cryptography" OFF)
option(${PROJECT_NAME_SAFE}_USE_BORINGSSL "Use BoringSSL for cryptography" OFF)
option(${PROJECT_NAME_SAFE}_USE_WOLFSSL "Use wolfSSL for cryptography" OFF)
option(${PROJECT_NAME_SAFE}_USE_S2N "Use s2n-tls for cryptography" OFF)

option(${PROJECT_NAME_SAFE}_USE_BEARSSL "Use BearSSL for cryptography" OFF)
option(${PROJECT_NAME_SAFE}_USE_SCHANNEL "Use Schannel for cryptography" OFF)
option(${PROJECT_NAME_SAFE}_USE_GNUTLS "Use GnuTLS for cryptography" OFF)
option(${PROJECT_NAME_SAFE}_USE_BOTAN "Use Botan for cryptography" OFF)
option(${PROJECT_NAME_SAFE}_USE_COMMONCRYPTO "Use CommonCrypto for cryptography" OFF)
option(${PROJECT_NAME_SAFE}_USE_WINCRYPT "Use WinCrypt for cryptography" OFF)

option(${PROJECT_NAME_SAFE}_USE_LSQUIC "Use lsquic" OFF)
option(${PROJECT_NAME_SAFE}_USE_PICOQUIC "Use picoquic" OFF)
option(${PROJECT_NAME_SAFE}_USE_NGHTTP3 "Use nghttp3" OFF)
option(${PROJECT_NAME_SAFE}_USE_MSH3 "Use msh3" OFF)
option(${PROJECT_NAME_SAFE}_USE_ARIA2 "Use aria2" OFF)
option(${PROJECT_NAME_SAFE}_USE_LIBSOUP3 "Use libsoup3" OFF)
option(${PROJECT_NAME_SAFE}_USE_LIBUV "Use libuv" OFF)
option(${PROJECT_NAME_SAFE}_USE_LIBEVENT "Use libevent" OFF)
option(${PROJECT_NAME_SAFE}_USE_LIBFETCH "Use libfetch" OFF)


option(${PROJECT_NAME_SAFE}_MULTIPLATFORM_INTEGRATION "Integrate with c-multiplatform" OFF)
option(${PROJECT_NAME_SAFE}_FETCH_CAH "Fetch c-abstract-http from GitHub" ON)
option(${PROJECT_NAME_SAFE}_FETCH_C_ORM "Fetch c-orm from GitHub" ON)
option(${PROJECT_NAME_SAFE}_FETCH_C_FS "Fetch c-fs from GitHub" ON)
option(C_REST_ENABLE_HOT_RELOADING_AUTO_RESTART "Enable Hot-Reloading / Auto-Restart" ON)
option(${PROJECT_NAME_SAFE}_FETCH_CDD_C "Fetch cdd-c from GitHub" ON)
option(${PROJECT_NAME_SAFE}_ENABLE_GRAPHQL "Enable GraphQL API Parsing & Resolving module" ON)
option(${PROJECT_NAME_SAFE}_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING "Enable Server-Side Template Engine" ON)

option(C_REST_ENABLE_RATE_LIMITING_THROTTLING_MIDDLEWARE "Enable Rate Limiting / Throttling Middleware" ON)
option(C_REST_ENABLE_JWT_JSON_WEB_TOKENS_AUTHENTICATION_MIDDLEWARE "Enable JWT Authentication Middleware" ON)
option(${PROJECT_NAME_SAFE}_ENABLE_RESPONSE_COMPRESSION_GZIP_BROTLI "Enable response compression (gzip/brotli)" ON)
if(${PROJECT_NAME_SAFE}_ENABLE_RESPONSE_COMPRESSION_GZIP_BROTLI)
    add_definitions(-D${PROJECT_NAME_SAFE}_ENABLE_RESPONSE_COMPRESSION_GZIP_BROTLI)
endif()
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




macro(c_rest_framework_resolve_dependency PkgName FetchURL)
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
            URL ${FetchURL}
        )
        FetchContent_MakeAvailable(${PkgName})
        set(${PkgName}_RESOLVED ON)
    elseif(${PROJECT_NAME_SAFE}_USE_BEARSSL)
        target_compile_definitions(${tgt} ${scope} C_REST_HAS_TLS C_REST_USE_BEARSSL)
    elseif(${PROJECT_NAME_SAFE}_USE_SCHANNEL)
        target_compile_definitions(${tgt} ${scope} C_REST_HAS_TLS C_REST_USE_SCHANNEL)
    elseif(${PROJECT_NAME_SAFE}_USE_GNUTLS)
        target_compile_definitions(${tgt} ${scope} C_REST_HAS_TLS C_REST_USE_GNUTLS)
    elseif(${PROJECT_NAME_SAFE}_USE_BOTAN)
        target_compile_definitions(${tgt} ${scope} C_REST_HAS_TLS C_REST_USE_BOTAN)
    elseif(${PROJECT_NAME_SAFE}_USE_COMMONCRYPTO)
        target_compile_definitions(${tgt} ${scope} C_REST_HAS_TLS C_REST_USE_COMMONCRYPTO)
    elseif(${PROJECT_NAME_SAFE}_USE_WINCRYPT)
        target_compile_definitions(${tgt} ${scope} C_REST_HAS_TLS C_REST_USE_WINCRYPT)
    endif()
endmacro()

macro(c_rest_framework_apply_crypto tgt scope)
    if(${PROJECT_NAME_SAFE}_USE_MBEDTLS)
        c_rest_framework_resolve_dependency(mbedtls https://github.com/Mbed-TLS/mbedtls/archive/refs/tags/v3.4.0.zip)
        target_link_libraries(${tgt} ${scope} mbedtls mbedcrypto mbedx509)
        target_compile_definitions(${tgt} ${scope} C_REST_HAS_TLS C_REST_USE_MBEDTLS)
    elseif(${PROJECT_NAME_SAFE}_USE_OPENSSL)
        c_rest_framework_resolve_dependency(OpenSSL https://github.com/openssl/openssl/archive/refs/tags/openssl-3.1.2.zip)
        target_link_libraries(${tgt} ${scope} OpenSSL::SSL OpenSSL::Crypto)
        target_compile_definitions(${tgt} ${scope} C_REST_HAS_TLS C_REST_USE_OPENSSL)
    elseif(${PROJECT_NAME_SAFE}_USE_LIBRESSL)
        c_rest_framework_resolve_dependency(LibreSSL https://github.com/libressl/portable/archive/refs/tags/v3.8.0.zip)
        target_link_libraries(${tgt} ${scope} LibreSSL::TLS LibreSSL::Crypto)
        target_compile_definitions(${tgt} ${scope} C_REST_HAS_TLS C_REST_USE_LIBRESSL)
    elseif(${PROJECT_NAME_SAFE}_USE_BORINGSSL)
        c_rest_framework_resolve_dependency(BoringSSL https://github.com/google/boringssl/archive/refs/heads/master.zip)
        target_link_libraries(${tgt} ${scope} crypto ssl)
        target_compile_definitions(${tgt} ${scope} C_REST_HAS_TLS C_REST_USE_BORINGSSL)
    elseif(${PROJECT_NAME_SAFE}_USE_WOLFSSL)
        set(WOLFSSL_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(WOLFSSL_CRYPT_TESTS OFF CACHE BOOL "" FORCE)
        c_rest_framework_resolve_dependency(wolfssl https://github.com/wolfSSL/wolfssl/archive/refs/tags/v5.6.6-stable.zip)
        target_link_libraries(${tgt} ${scope} wolfssl)
        target_compile_definitions(${tgt} ${scope} C_REST_HAS_TLS C_REST_USE_WOLFSSL)
    elseif(${PROJECT_NAME_SAFE}_USE_S2N)
        c_rest_framework_resolve_dependency(s2n https://github.com/aws/s2n-tls/archive/refs/heads/main.zip)
        target_link_libraries(${tgt} ${scope} s2n::s2n)
        target_compile_definitions(${tgt} ${scope} C_REST_HAS_TLS C_REST_USE_S2N)
    elseif(${PROJECT_NAME_SAFE}_USE_BEARSSL)
        target_compile_definitions(${tgt} ${scope} C_REST_HAS_TLS C_REST_USE_BEARSSL)
    elseif(${PROJECT_NAME_SAFE}_USE_SCHANNEL)
        target_compile_definitions(${tgt} ${scope} C_REST_HAS_TLS C_REST_USE_SCHANNEL)
    elseif(${PROJECT_NAME_SAFE}_USE_GNUTLS)
        target_compile_definitions(${tgt} ${scope} C_REST_HAS_TLS C_REST_USE_GNUTLS)
    elseif(${PROJECT_NAME_SAFE}_USE_BOTAN)
        target_compile_definitions(${tgt} ${scope} C_REST_HAS_TLS C_REST_USE_BOTAN)
    elseif(${PROJECT_NAME_SAFE}_USE_COMMONCRYPTO)
        target_compile_definitions(${tgt} ${scope} C_REST_HAS_TLS C_REST_USE_COMMONCRYPTO)
    elseif(${PROJECT_NAME_SAFE}_USE_WINCRYPT)
        target_compile_definitions(${tgt} ${scope} C_REST_HAS_TLS C_REST_USE_WINCRYPT)
    endif()
endmacro()

include(FetchContent)
if(CMAKE_CROSSCOMPILING_EMULATOR MATCHES "wine")
    set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "" FORCE)
endif()
foreach(_fc_dep PARSON C-ABSTRACT-HTTP C_ABSTRACT_HTTP C89STRINGUTILS CDD-C CDD_C C-STR-SPAN C_STR_SPAN C-ORM C_ORM CFS C_FS)
    unset(FETCHCONTENT_SOURCE_DIR_${_fc_dep} CACHE)
    unset(FETCHCONTENT_SOURCE_DIR_${_fc_dep})
endforeach()
FetchContent_Declare(
  greatest
  GIT_REPOSITORY https://github.com/SamuelMarks/greatest.git
  GIT_TAG        master
)
FetchContent_MakeAvailable(greatest)

set(C_ABSTRACT_HTTP_USE_MBEDTLS ${${PROJECT_NAME_SAFE}_USE_MBEDTLS} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_OPENSSL ${${PROJECT_NAME_SAFE}_USE_OPENSSL} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_LIBRESSL ${${PROJECT_NAME_SAFE}_USE_LIBRESSL} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_BORINGSSL ${${PROJECT_NAME_SAFE}_USE_BORINGSSL} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_WOLFSSL ${${PROJECT_NAME_SAFE}_USE_WOLFSSL} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_S2N ${${PROJECT_NAME_SAFE}_USE_S2N} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_BEARSSL ${${PROJECT_NAME_SAFE}_USE_BEARSSL} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_SCHANNEL ${${PROJECT_NAME_SAFE}_USE_SCHANNEL} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_GNUTLS ${${PROJECT_NAME_SAFE}_USE_GNUTLS} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_BOTAN ${${PROJECT_NAME_SAFE}_USE_BOTAN} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_COMMONCRYPTO ${${PROJECT_NAME_SAFE}_USE_COMMONCRYPTO} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_WINCRYPT ${${PROJECT_NAME_SAFE}_USE_WINCRYPT} CACHE BOOL "" FORCE)

set(C_ABSTRACT_HTTP_USE_BEARSSL ${${PROJECT_NAME_SAFE}_USE_BEARSSL} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_SCHANNEL ${${PROJECT_NAME_SAFE}_USE_SCHANNEL} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_GNUTLS ${${PROJECT_NAME_SAFE}_USE_GNUTLS} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_BOTAN ${${PROJECT_NAME_SAFE}_USE_BOTAN} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_COMMONCRYPTO ${${PROJECT_NAME_SAFE}_USE_COMMONCRYPTO} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_WINCRYPT ${${PROJECT_NAME_SAFE}_USE_WINCRYPT} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_LSQUIC ${${PROJECT_NAME_SAFE}_USE_LSQUIC} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_PICOQUIC ${${PROJECT_NAME_SAFE}_USE_PICOQUIC} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_NGHTTP3 ${${PROJECT_NAME_SAFE}_USE_NGHTTP3} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_MSH3 ${${PROJECT_NAME_SAFE}_USE_MSH3} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_ARIA2 ${${PROJECT_NAME_SAFE}_USE_ARIA2} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_LIBSOUP3 ${${PROJECT_NAME_SAFE}_USE_LIBSOUP3} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_LIBUV ${${PROJECT_NAME_SAFE}_USE_LIBUV} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_LIBEVENT ${${PROJECT_NAME_SAFE}_USE_LIBEVENT} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_LIBFETCH ${${PROJECT_NAME_SAFE}_USE_LIBFETCH} CACHE BOOL "" FORCE)

if(${PROJECT_NAME_SAFE}_FETCH_C_FS)
    set(CDD_DEPS "FETCHCONTENT" CACHE STRING "" FORCE)
    FetchContent_Declare(
      cfs
      GIT_REPOSITORY https://github.com/SamuelMarks/c-fs.git
      GIT_TAG master
    )
    set(OLD_BSL_CFS ${BUILD_SHARED_LIBS})
    set(OLD_BUILD_TESTING_CFS ${BUILD_TESTING})
    set(BUILD_SHARED_LIBS OFF)
    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(cfs)
    set(BUILD_TESTING ${OLD_BUILD_TESTING_CFS} CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS ${OLD_BSL_CFS})
endif()

if(DEFINED CMAKE_TOOLCHAIN_FILE AND CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg.cmake")
    file(WRITE "${CMAKE_BINARY_DIR}/c_str_spanConfig.cmake" "set(c_str_span_FOUND TRUE)\n")

    FetchContent_Declare(
        c89stringutils
        URL https://github.com/offscale/c89stringutils/archive/refs/heads/master.zip
    )
    FetchContent_MakeAvailable(c89stringutils)
    file(WRITE "${CMAKE_BINARY_DIR}/c89stringutilsConfig.cmake" "set(c89stringutils_FOUND TRUE)\n")
    list(APPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}")
endif()





    FetchContent_Declare(
        c-str-span
        GIT_REPOSITORY https://github.com/SamuelMarks/c-str-span.git
        GIT_TAG master
    )
    set(OLD_BSL_CSS ${BUILD_SHARED_LIBS})
    set(BUILD_SHARED_LIBS OFF)
    FetchContent_MakeAvailable(c-str-span)
    if(TARGET c_str_span_compiler_flags AND NOT CMAKE_C_FLAGS MATCHES "--coverage")
        get_target_property(_css_opts c_str_span_compiler_flags INTERFACE_COMPILE_OPTIONS)
        if(_css_opts)
            string(REPLACE ";-fprofile-arcs;-ftest-coverage" "" _css_opts "${_css_opts}")
            set_target_properties(c_str_span_compiler_flags PROPERTIES INTERFACE_COMPILE_OPTIONS "${_css_opts}")
        endif()
        set_target_properties(c_str_span_compiler_flags PROPERTIES INTERFACE_LINK_OPTIONS "")
    endif()
    if(CMAKE_HOST_UNIX AND EXISTS "${CMAKE_BINARY_DIR}/_deps/c-str-span-build/c_str_span/c_str_span_export.h" AND EXISTS "${CMAKE_BINARY_DIR}/_deps/c-str-span-src/CMakeLists.txt")
        execute_process(COMMAND touch -r "${CMAKE_BINARY_DIR}/_deps/c-str-span-src/CMakeLists.txt" "${CMAKE_BINARY_DIR}/_deps/c-str-span-build/c_str_span/c_str_span_export.h")
    endif()
    set(BUILD_SHARED_LIBS ${OLD_BSL_CSS})




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

    if(TARGET parson)
        message(STATUS "parson already provided by parent")
        set(parson_RESOLVED ON)
        target_link_libraries(c-rest-framework PUBLIC parson)
    endif()

    if(NOT parson_RESOLVED)
        message(STATUS "Fetching parson via FetchContent")
        if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../../parson")
            message(STATUS "Using local parson")
            FetchContent_Declare(parson SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../parson")
        else()
            FetchContent_Declare(
                parson
                GIT_REPOSITORY https://github.com/SamuelMarks/parson.git
        GIT_TAG master
            )
        endif()
        set(OLD_BSL ${BUILD_SHARED_LIBS})
        set(BUILD_SHARED_LIBS OFF)
        FetchContent_MakeAvailable(parson)
        if(TARGET parson AND NOT TARGET parson::parson)
            add_library(parson::parson ALIAS parson)
        endif()
        set(BUILD_SHARED_LIBS ${OLD_BSL})
        target_link_libraries(c-rest-framework PUBLIC parson)
        if(TARGET parson)
            target_include_directories(parson PUBLIC $<BUILD_INTERFACE:${parson_SOURCE_DIR}>)
            if(UNIX AND NOT APPLE AND CMAKE_C_COMPILER_ID MATCHES "Clang")
                file(WRITE "${CMAKE_BINARY_DIR}/parson_c89_math_fix.h" "#include <math.h>\n#undef isinf\n#undef isnan\n")
                target_compile_options(parson PRIVATE -include "${CMAKE_BINARY_DIR}/parson_c89_math_fix.h")
            endif()
        endif()
        target_include_directories(c-rest-framework PUBLIC ${parson_SOURCE_DIR})
        set(parson_RESOLVED ON)
        list(APPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}")
    elseif(${PROJECT_NAME_SAFE}_USE_BEARSSL)
        target_compile_definitions(${tgt} ${scope} C_REST_HAS_TLS C_REST_USE_BEARSSL)
    elseif(${PROJECT_NAME_SAFE}_USE_SCHANNEL)
        target_compile_definitions(${tgt} ${scope} C_REST_HAS_TLS C_REST_USE_SCHANNEL)
    elseif(${PROJECT_NAME_SAFE}_USE_GNUTLS)
        target_compile_definitions(${tgt} ${scope} C_REST_HAS_TLS C_REST_USE_GNUTLS)
    elseif(${PROJECT_NAME_SAFE}_USE_BOTAN)
        target_compile_definitions(${tgt} ${scope} C_REST_HAS_TLS C_REST_USE_BOTAN)
    elseif(${PROJECT_NAME_SAFE}_USE_COMMONCRYPTO)
        target_compile_definitions(${tgt} ${scope} C_REST_HAS_TLS C_REST_USE_COMMONCRYPTO)
    elseif(${PROJECT_NAME_SAFE}_USE_WINCRYPT)
        target_compile_definitions(${tgt} ${scope} C_REST_HAS_TLS C_REST_USE_WINCRYPT)
    endif()
endmacro()

if(${PROJECT_NAME_SAFE}_MULTIPLATFORM_INTEGRATION)
    add_definitions(-D${PROJECT_NAME_SAFE}_MULTIPLATFORM_INTEGRATION)
endif()

if(MSVC)
    # Step 17: Define strict compilation flags for MSVC (/W4 /WX /Za)
    # We use /WX to treat warnings as errors and high warning level
    # Note: /Za strictly enforces ANSI C but might cause issues with Windows headers.
    set(C_REST_MSVC_FLAGS /W4 /WX)

    if(${PROJECT_NAME_SAFE}_CHARSET STREQUAL "UNICODE")
        add_definitions(-DUNICODE -D_UNICODE)
    elseif(${PROJECT_NAME_SAFE}_CHARSET STREQUAL "ANSI")
        add_definitions(-D_MBCS)
    endif()

    if(NOT ${PROJECT_NAME_SAFE}_MSVC_RTC STREQUAL "OFF" AND NOT ${PROJECT_NAME_SAFE}_MSVC_RTC STREQUAL "")
        string(REGEX REPLACE "^RTC" "" _RTC_SUFFIX "${${PROJECT_NAME_SAFE}_MSVC_RTC}")
        add_compile_options("/RTC${_RTC_SUFFIX}")
    endif()
elseif(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    # Step 16: Define C89 strict compilation flags for GCC (-ansi -pedantic -Wall -Wextra -Werror)
    set(C_REST_GNU_FLAGS -ansi -pedantic -Wall -Wextra -Werror)
    if(CMAKE_C_COMPILER_ID MATCHES "Clang" AND CMAKE_INTERPROCEDURAL_OPTIMIZATION AND NOT APPLE)
        add_link_options("-fuse-ld=lld")
    endif()


    if(${PROJECT_NAME_SAFE}_USE_ASAN)
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address)
    endif()

    if(${PROJECT_NAME_SAFE}_USE_TSAN)
        add_compile_options(-fsanitize=thread)
        add_link_options(-fsanitize=thread)
    endif()
endif()

if(BUILD_TESTING)
    add_definitions(-DC_REST_TESTING_MALLOC_HOOK)
endif()

add_library(c-rest-framework
    src/modality.c
    src/modality_sync.c
    src/modality_single_thread.c
    src/modality_async.c
    src/modality_multi_thread.c
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
        src/c_rest_log.c
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

if(NOT EMSCRIPTEN)
    target_sources(c-rest-framework PRIVATE src/modality_multi_process.c)
endif()

if(C_REST_GNU_FLAGS)
    target_compile_options(c-rest-framework PRIVATE ${C_REST_GNU_FLAGS})
elseif(C_REST_MSVC_FLAGS)
    target_compile_options(c-rest-framework PRIVATE ${C_REST_MSVC_FLAGS})
endif()

if(C_REST_ENABLE_HOT_RELOADING_AUTO_RESTART AND NOT EMSCRIPTEN)
    target_sources(c-rest-framework PRIVATE src/c_rest_hot_reload.c)
    target_compile_definitions(c-rest-framework PUBLIC C_REST_ENABLE_HOT_RELOADING_AUTO_RESTART)
endif()

if(C_REST_ENABLE_SERVER_SENT_EVENTS_SSE)
    target_sources(c-rest-framework PRIVATE src/c_rest_sse.c)
endif()

if(${PROJECT_NAME_SAFE}_ENABLE_RESPONSE_COMPRESSION_GZIP_BROTLI)
    target_compile_definitions(c-rest-framework PUBLIC ${PROJECT_NAME_SAFE}_ENABLE_RESPONSE_COMPRESSION_GZIP)

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
                URL https://github.com/madler/zlib/archive/refs/heads/master.zip
            )
            set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
            set(ZLIB_BUILD_TESTING OFF CACHE BOOL "" FORCE)
            set(SKIP_INSTALL_ALL ON CACHE BOOL "" FORCE)
            FetchContent_MakeAvailable(zlib)
            if(MSVC AND MSVC_VERSION LESS 1600)
                if(TARGET zlib)
                    target_compile_definitions(zlib PRIVATE EWOULDBLOCK=140)
                endif()
                if(TARGET zlibstatic)
                    target_compile_definitions(zlibstatic PRIVATE EWOULDBLOCK=140)
                endif()
            endif()
            if(TARGET zlibstatic)
                target_link_libraries(c-rest-framework PUBLIC zlibstatic)
                set_target_properties(zlibstatic PROPERTIES C_EXTENSIONS ON)
                if(TARGET zlib)
                    set_target_properties(zlib PROPERTIES EXCLUDE_FROM_ALL TRUE)
                endif()
            else()
                target_link_libraries(c-rest-framework PUBLIC zlib)
            endif()
            if(TARGET zlib)
                set_target_properties(zlib PROPERTIES C_EXTENSIONS ON)
            endif()
            target_include_directories(c-rest-framework SYSTEM PUBLIC ${zlib_SOURCE_DIR} ${zlib_BINARY_DIR})
        endif()

    if(NOT MSVC OR MSVC_VERSION GREATER_EQUAL 1600)
        target_compile_definitions(c-rest-framework PUBLIC ${PROJECT_NAME_SAFE}_ENABLE_RESPONSE_COMPRESSION_BROTLI)
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
                URL https://github.com/google/brotli/archive/refs/heads/master.zip
            )
            set(BROTLI_DISABLE_TESTS ON CACHE BOOL "" FORCE)
            set(OLD_BSL_BROTLI ${BUILD_SHARED_LIBS})
            set(BUILD_SHARED_LIBS OFF)
            set(CMAKE_POSITION_INDEPENDENT_CODE ON)
            FetchContent_MakeAvailable(brotli)
            set(BUILD_SHARED_LIBS ${OLD_BSL_BROTLI})
            if(TARGET brotli)
                set_target_properties(brotli PROPERTIES EXCLUDE_FROM_ALL TRUE)
            endif()


            if(TARGET brotlienc-static)
                target_link_libraries(c-rest-framework PUBLIC brotlienc-static brotlidec-static brotlicommon-static)
            else()
                target_link_libraries(c-rest-framework PUBLIC brotlienc brotlidec brotlicommon)
            endif()

            target_include_directories(c-rest-framework PUBLIC ${brotli_SOURCE_DIR}/c/include)
        endif()
    endif()
endif()


c_rest_framework_apply_crypto(c-rest-framework PUBLIC)

include(GenerateExportHeader)
generate_export_header(c-rest-framework EXPORT_MACRO_NAME C_REST_EXPORT EXPORT_FILE_NAME c_rest_export.h)
target_include_directories(c-rest-framework PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(c-rest-framework PUBLIC ${PROJECT_NAME_SAFE}_STATIC=1 C_REST_FRAMEWORK_STATIC_DEFINE=1)
endif()
c_rest_framework_resolve_parson()

if(${PROJECT_NAME_SAFE}_FETCH_CDD_C)
    if(MSVC)
        foreach(_c89_var
            C89STRINGUTILS_HAVE_PRINTF_ZU C89STRINGUTILS_HAVE_PRINTF_LLD
            C89STRINGUTILS_HAVE_PRINTF_I64D C89STRINGUTILS_HAVE_PRINTF_IU
            C89STRINGUTILS_HAVE_VSNPRINTF_NULL C89STRINGUTILS_HAVE_VA_COPY
            C89STRINGUTILS_HAVE_SAL C89STRINGUTILS_HAVE_ERRNO_T
            C89STRINGUTILS_HAVE_RSIZE_T C89STRINGUTILS_HAVE_WCHAR_H
            C89STRINGUTILS_HAVE_STRNCPY_S C89STRINGUTILS_HAVE_STRCPY_S
            C89STRINGUTILS_HAVE_STRNCAT_S C89STRINGUTILS_HAVE_STRCAT_S
            C89STRINGUTILS_HAVE_STRNLEN C89STRINGUTILS_HAVE_STRNLEN_S
            C89STRINGUTILS_HAVE_MEMCPY_S C89STRINGUTILS_HAVE_TMPFILE_S
            C89STRINGUTILS_HAVE_FOPEN_S C89STRINGUTILS_HAVE_FREOPEN_S
            C89STRINGUTILS_HAVE_VSSCANF C89STRINGUTILS_HAVE_VSSCANF_S
            C89STRINGUTILS_HAVE_VFSCANF C89STRINGUTILS_HAVE_VFSCANF_S
            C89STRINGUTILS_HAVE_WCSCPY C89STRINGUTILS_HAVE_WCSNCPY
            C89STRINGUTILS_HAVE_WCSCPY_S C89STRINGUTILS_HAVE_WCSNCPY_S
            C89STRINGUTILS_HAVE_VSWPRINTF C89STRINGUTILS_HAVE_VSWPRINTF_S
            C89STRINGUTILS_HAVE_SWPRINTF C89STRINGUTILS_HAVE_SWPRINTF_S
            C89STRINGUTILS_HAVE_SPRINTF_S C89STRINGUTILS_HAVE_VSNPRINTF
            C89STRINGUTILS_HAVE__VSNPRINTF C89STRINGUTILS_HAVE_VSNPRINTF_S
            C89STRINGUTILS_HAVE__VSNPRINTF_S C89STRINGUTILS_HAVE__SNPRINTF_S
            C89STRINGUTILS_HAVE__SNPRINTF C89STRINGUTILS_HAVE_VFPRINTF_S
            C89STRINGUTILS_HAVE_FPRINTF_S C89STRINGUTILS_HAVE__VSCPRINTF
            C89STRINGUTILS_HAVE_SNPRINTF C89STRINGUTILS_HAVE__STRNICMP
            C89STRINGUTILS_HAVE__STRICMP C89STRINGUTILS_HAVE_STRERROR_S
            C89STRINGUTILS_HAVE_LONG_LONG C89STRINGUTILS_HAVE___INT64)
            set(${_c89_var} 1 CACHE INTERNAL "")
        endforeach()
        foreach(_c89_var_off
            C89STRINGUTILS_HAVE___VA_COPY C89STRINGUTILS_HAVE_ATTR_FORMAT
            C89STRINGUTILS_HAVE_ATTR_NONNULL C89STRINGUTILS_HAVE_TYPEOF
            C89STRINGUTILS_HAVE_STRINGS_H C89STRINGUTILS_HAVE_SYS_PARAM_H
            C89STRINGUTILS_HAVE_STRLCPY C89STRINGUTILS_HAVE_STRLCAT
            C89STRINGUTILS_HAVE_SET_CONSTRAINT_HANDLER_S
            C89STRINGUTILS_HAVE_REALLOCARRAY C89STRINGUTILS_HAVE_SNPRINTF_S
            C89STRINGUTILS_HAVE_STRNCASECMP C89STRINGUTILS_HAVE_STRCASECMP
            C89STRINGUTILS_HAVE_STRNSTR C89STRINGUTILS_HAVE_STRCASESTR
            C89STRINGUTILS_HAVE_VASPRINTF C89STRINGUTILS_HAVE_ASPRINTF
            C89STRINGUTILS_HAVE_STRERROR_R)
            set(${_c89_var_off} 0 CACHE INTERNAL "")
        endforeach()
    endif()
    if(UNIX AND NOT APPLE)
        include(CheckCSourceCompiles)
        set(_OLD_REQ_DEFS "${CMAKE_REQUIRED_DEFINITIONS}")
        set(_OLD_REQ_FLAGS "${CMAKE_REQUIRED_FLAGS}")
        set(CMAKE_REQUIRED_DEFINITIONS "-D_GNU_SOURCE")
        set(CMAKE_REQUIRED_FLAGS "-Werror")
        check_c_source_compiles("
            #include <string.h>
            int main(void) {
                char buf[256];
                char *res = strerror_r(0, buf, sizeof(buf));
                return res != 0;
            }
        " C89STRINGUTILS_STRERROR_R_CHAR_P)
        set(CMAKE_REQUIRED_DEFINITIONS "${_OLD_REQ_DEFS}")
        set(CMAKE_REQUIRED_FLAGS "${_OLD_REQ_FLAGS}")
    endif()
    unset(FETCHCONTENT_SOURCE_DIR_CDD-C CACHE)
    unset(FETCHCONTENT_SOURCE_DIR_CDD_C CACHE)
    unset(FETCHCONTENT_SOURCE_DIR_CDD-C)
    unset(FETCHCONTENT_SOURCE_DIR_CDD_C)
    FetchContent_Declare(
      c89stringutils
      GIT_REPOSITORY https://github.com/offscale/c89stringutils.git
      GIT_TAG master
    )
    FetchContent_Declare(
      cdd-c
      GIT_REPOSITORY https://github.com/SamuelMarks/cdd-c.git
      GIT_TAG master
    )
    set(OLD_BSL_CDD ${BUILD_SHARED_LIBS})
    set(BUILD_SHARED_LIBS OFF)
    set(OLD_BUILD_TESTING_GLOBAL ${BUILD_TESTING})
    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(cdd-c)
    set(BUILD_TESTING ${OLD_BUILD_TESTING_GLOBAL} CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS ${OLD_BSL_CDD})



    if (TARGET c_cdd)
        target_compile_definitions(c_cdd PRIVATE CDD_BUILD_TESTS)
        if (UNIX AND NOT APPLE AND CMAKE_C_COMPILER_ID MATCHES "Clang")
            target_compile_definitions(c_cdd PRIVATE "_Bool=unsigned char")
        endif()
        if (NOT WIN32 AND NOT APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "DOS")
            find_package(OpenSSL QUIET)
            if (OpenSSL_FOUND)
                target_link_libraries(c_cdd PUBLIC OpenSSL::SSL OpenSSL::Crypto)

            endif()
        endif()
    endif()
endif()

if(NOT WIN32 AND NOT APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "DOS" AND NOT EMSCRIPTEN)
    find_package(CURL QUIET)
endif()

if(TARGET CURL::libcurl OR TARGET libcurl)
    set(CURL_FOUND TRUE CACHE BOOL "CURL already resolved" FORCE)
    set(curl_FOUND TRUE CACHE BOOL "CURL already resolved" FORCE)
    if(NOT TARGET CURL)
        add_library(CURL INTERFACE IMPORTED)
        if(TARGET CURL::libcurl)
            target_link_libraries(CURL INTERFACE CURL::libcurl)
        elseif(TARGET libcurl)
            target_link_libraries(CURL INTERFACE libcurl)
        endif()
    endif()
endif()

if(${PROJECT_NAME_SAFE}_FETCH_CAH)
    FetchContent_Declare(
      c-abstract-http
      GIT_REPOSITORY https://github.com/SamuelMarks/c-abstract-http.git
      GIT_TAG master
    )
    set(OLD_BSL_CAH ${BUILD_SHARED_LIBS})
    set(OLD_BUILD_TESTING_CAH ${BUILD_TESTING})
    set(BUILD_SHARED_LIBS OFF)
    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(c-abstract-http)
    set(BUILD_TESTING ${OLD_BUILD_TESTING_CAH} CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS ${OLD_BSL_CAH})

    add_definitions(-D${PROJECT_NAME_SAFE}_USE_REAL_CAH)
endif()

if(${PROJECT_NAME_SAFE}_FETCH_C_ORM)
    FetchContent_Declare(
      c-orm
      GIT_REPOSITORY https://github.com/SamuelMarks/c-orm.git
      GIT_TAG master
    )
    set(OLD_BSL_CORM ${BUILD_SHARED_LIBS})
    set(OLD_BUILD_TESTING_CORM ${BUILD_TESTING})
    set(BUILD_SHARED_LIBS OFF)
    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(C_ORM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(C_ORM_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(c-orm)
    set(BUILD_TESTING ${OLD_BUILD_TESTING_CORM} CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS ${OLD_BSL_CORM})
    add_definitions(-D${PROJECT_NAME_SAFE}_USE_REAL_C_ORM)
    if(TARGET sqlite3)
        set_target_properties(sqlite3 PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES $<TARGET_PROPERTY:sqlite3,INTERFACE_INCLUDE_DIRECTORIES>)
    endif()
endif()

if(${PROJECT_NAME_SAFE}_FETCH_CAH)
    target_link_libraries(c-rest-framework PUBLIC c-abstract-http)

endif()

if(${PROJECT_NAME_SAFE}_FETCH_C_ORM)
    target_link_libraries(c-rest-framework PUBLIC c-orm)
endif()
target_sources(c-rest-framework PRIVATE src/c_orm_mock.c)

if(${PROJECT_NAME_SAFE}_FETCH_C_FS)
    target_link_libraries(c-rest-framework PUBLIC cfs)
endif()

if(${PROJECT_NAME_SAFE}_FETCH_CDD_C)
    target_link_libraries(c-rest-framework PUBLIC c_cdd)
    add_definitions(-D${PROJECT_NAME_SAFE}_USE_CDD_C)
endif()

if(EMSCRIPTEN)
    target_sources(c-rest-framework PRIVATE src/platform_posix.c)
    target_compile_options(c-rest-framework PUBLIC -pthread)
    target_link_options(c-rest-framework PUBLIC -pthread "-sUSE_PTHREADS=1" "-sASYNCIFY=1")
elseif(WIN32)
    target_sources(c-rest-framework PRIVATE src/platform_win32.c)
    target_link_libraries(c-rest-framework PUBLIC ws2_32)
else()
    target_sources(c-rest-framework PRIVATE src/platform_posix.c)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_link_libraries(c-rest-framework PUBLIC pthread)
    endif()
endif()


target_include_directories(c-rest-framework PUBLIC include)

if(${PROJECT_NAME_SAFE}_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING)
    target_compile_definitions(c-rest-framework PUBLIC C_REST_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING)
endif()

if(${PROJECT_NAME_SAFE}_ENABLE_GRAPHQL)
    target_compile_definitions(c-rest-framework PUBLIC ${PROJECT_NAME_SAFE}_ENABLE_GRAPHQL)
endif()




if(EMSCRIPTEN AND TARGET c-abstract-http)
    target_compile_definitions(c-abstract-http PRIVATE CDD_NO_UCONTEXT=1)
endif()
if(BUILD_TESTING)
    target_compile_definitions(c-rest-framework PRIVATE C_REST_TESTING_ENDIAN_HOOK)
endif()

# Remove coverage flags from dependencies
foreach(dep cdd-c c_cdd c-abstract-http c-orm c_str_span cfs c89stringutils parson)
    if(TARGET ${dep}_compiler_flags)
        get_target_property(_opts ${dep}_compiler_flags INTERFACE_COMPILE_OPTIONS)
        if(_opts)
            string(REPLACE "-fprofile-arcs" "" _opts "${_opts}")
            string(REPLACE "-ftest-coverage" "" _opts "${_opts}")
            string(REPLACE "--coverage" "" _opts "${_opts}")
            string(REPLACE "-Wcast-qual" "" _opts "${_opts}")
            if(MSVC)
                string(REPLACE "-Zi" "/Z7" _opts "${_opts}")
                string(REPLACE "/Zi" "/Z7" _opts "${_opts}")
            endif()
            set_target_properties(${dep}_compiler_flags PROPERTIES INTERFACE_COMPILE_OPTIONS "${_opts}")
        endif()
    endif()
    if(TARGET ${dep})
        get_target_property(_opts ${dep} INTERFACE_COMPILE_OPTIONS)
        if(_opts)
            string(REPLACE "-fprofile-arcs" "" _opts "${_opts}")
            string(REPLACE "-ftest-coverage" "" _opts "${_opts}")
            string(REPLACE "--coverage" "" _opts "${_opts}")
            string(REPLACE "-Wcast-qual" "" _opts "${_opts}")
            if(MSVC)
                string(REPLACE "-Zi" "/Z7" _opts "${_opts}")
                string(REPLACE "/Zi" "/Z7" _opts "${_opts}")
            endif()
            set_target_properties(${dep} PROPERTIES INTERFACE_COMPILE_OPTIONS "${_opts}")
        endif()
    endif()
endforeach()

if(TARGET c_cdd_compiler_flags)
    get_target_property(_link_opts c_cdd_compiler_flags INTERFACE_LINK_OPTIONS)
    if(_link_opts)
        string(REPLACE "/NODEFAULTLIB:LIBCMTD" "" _link_opts "${_link_opts}")
        set_target_properties(c_cdd_compiler_flags PROPERTIES INTERFACE_LINK_OPTIONS "${_link_opts}")
    endif()
endif()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    if(BUILD_TESTING)
        enable_testing()
        message(STATUS "ADDING TESTS")
        add_subdirectory(tests)
    endif()
    add_subdirectory(examples)
endif()

if(MSVC)
    if(TARGET c-rest-framework-tests)
        target_compile_definitions(c-rest-framework-tests PRIVATE DISABLE_GZIP_BROTLI_OOM_TESTS)
    endif()
endif()
