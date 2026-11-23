cmake_minimum_required(VERSION 3.14)

if(POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)
    set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
endif()
if(POLICY CMP0079)
    cmake_policy(SET CMP0079 NEW)
endif()
if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
    set(CMAKE_POLICY_DEFAULT_CMP0091 NEW)
endif()
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

if(DEFINED CMAKE_INTERPROCEDURAL_OPTIMIZATION)
    set(_CRF_SAVE_IPO ${CMAKE_INTERPROCEDURAL_OPTIMIZATION})
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

if(APPLE AND NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
        set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum OS X deployment version")
    else()
        set(CMAKE_OSX_DEPLOYMENT_TARGET "10.10" CACHE STRING "Minimum OS X deployment version")
    endif()
endif()

project(c-rest-framework C)

string(TOUPPER "${PROJECT_NAME}" PROJECT_NAME_UPPER)
string(REPLACE "-" "_" PROJECT_NAME_SAFE "${PROJECT_NAME_UPPER}")

# Isolate coverage instrumentation strictly to project targets via target_*
set(CRF_COVERAGE_ENABLED FALSE)
if(CMAKE_C_FLAGS MATCHES "--coverage|-fprofile-arcs|-ftest-coverage")
    set(CRF_COVERAGE_ENABLED TRUE)
    string(REPLACE "--coverage" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
    string(REPLACE "-fprofile-arcs" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
    string(REPLACE "-ftest-coverage" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
endif()

if(MSVC)
    string(REGEX REPLACE "[/-]W[0-4]" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
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
    endif()
    if(MSVC_VERSION LESS 1600)
        function(target_compile_features)
        endfunction()
    endif()
endif()

if(DEFINED _CRF_SAVE_IPO)
    if(CMAKE_CROSSCOMPILING_EMULATOR MATCHES "wine")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF CACHE BOOL "" FORCE)
    else()
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ${_CRF_SAVE_IPO} CACHE BOOL "" FORCE)
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
    if(MSVC)
        set(CMAKE_INCLUDE_SYSTEM_FLAG_C "-I")
        set(CMAKE_INCLUDE_SYSTEM_FLAG_CXX "-I")
    endif()
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# Core CMake options: CRT linkage, Charset, Threading, LTO, MSVC Runtime Checks
set(${PROJECT_NAME_SAFE}_CRT_LINKAGE "dynamic" CACHE STRING "CRT Linkage (static/dynamic)")
set_property(CACHE ${PROJECT_NAME_SAFE}_CRT_LINKAGE PROPERTY STRINGS "static" "dynamic")
set(${PROJECT_NAME_SAFE}_CHARSET "UNICODE" CACHE STRING "Character set (UNICODE/ANSI)")
set_property(CACHE ${PROJECT_NAME_SAFE}_CHARSET PROPERTY STRINGS "UNICODE" "ANSI")
if(CMAKE_SYSTEM_NAME STREQUAL "DOS")
    option(${PROJECT_NAME_SAFE}_THREADING "Enable multi-threading" OFF)
else()
    option(${PROJECT_NAME_SAFE}_THREADING "Enable multi-threading" ON)
endif()
set(${PROJECT_NAME_SAFE}_MSVC_RTC "OFF" CACHE STRING "MSVC Runtime Checks (OFF, 1, s, u)")
set_property(CACHE ${PROJECT_NAME_SAFE}_MSVC_RTC PROPERTY STRINGS "OFF" "1" "s" "u" "RTC1" "RTCs" "RTCu")
option(${PROJECT_NAME_SAFE}_LTO "Enable Link-Time Optimization (IPO)" OFF)

if(DEFINED ${PROJECT_NAME_SAFE}_BUILD_TESTING)
    set(BUILD_TESTING ${${PROJECT_NAME_SAFE}_BUILD_TESTING} CACHE BOOL "Build the testing tree." FORCE)
endif()
set(_CRF_TOP_BUILD_TESTING ${BUILD_TESTING})

if(DEFINED CDD_CHARSET AND NOT "${CDD_CHARSET}" STREQUAL "")
    set(${PROJECT_NAME_SAFE}_CHARSET "${CDD_CHARSET}" CACHE STRING "" FORCE)
endif()
if(DEFINED CDD_THREADING AND NOT "${CDD_THREADING}" STREQUAL "")
    set(${PROJECT_NAME_SAFE}_THREADING "${CDD_THREADING}" CACHE BOOL "" FORCE)
endif()
if(DEFINED CDD_MSVC_RTC AND NOT "${CDD_MSVC_RTC}" STREQUAL "")
    set(${PROJECT_NAME_SAFE}_MSVC_RTC "${CDD_MSVC_RTC}" CACHE STRING "" FORCE)
endif()
if(DEFINED _CRF_SAVE_IPO AND _CRF_SAVE_IPO)
    set(${PROJECT_NAME_SAFE}_LTO ON CACHE BOOL "" FORCE)
endif()

set(CDD_CHARSET "${${PROJECT_NAME_SAFE}_CHARSET}" CACHE STRING "" FORCE)
set(CFS_CHARSET "${${PROJECT_NAME_SAFE}_CHARSET}" CACHE STRING "" FORCE)
set(C_FS_CHARSET "${${PROJECT_NAME_SAFE}_CHARSET}" CACHE STRING "" FORCE)
set(C_ABSTRACT_HTTP_CHARSET "${${PROJECT_NAME_SAFE}_CHARSET}" CACHE STRING "" FORCE)
set(CDD_THREADING "${${PROJECT_NAME_SAFE}_THREADING}" CACHE BOOL "" FORCE)
set(CFS_THREADING "${${PROJECT_NAME_SAFE}_THREADING}" CACHE STRING "" FORCE)
set(C_FS_THREADING "${${PROJECT_NAME_SAFE}_THREADING}" CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_THREADING "${${PROJECT_NAME_SAFE}_THREADING}" CACHE BOOL "" FORCE)
set(C_ORM_THREADING "${${PROJECT_NAME_SAFE}_THREADING}" CACHE BOOL "" FORCE)
set(CDD_MSVC_RTC "${${PROJECT_NAME_SAFE}_MSVC_RTC}" CACHE STRING "" FORCE)

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
    else()
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

set(_CRF_LTO_ACTIVE FALSE)
if(MSVC AND NOT ${PROJECT_NAME_SAFE}_MSVC_RTC STREQUAL "OFF" AND NOT ${PROJECT_NAME_SAFE}_MSVC_RTC STREQUAL "")
    set(${PROJECT_NAME_SAFE}_LTO OFF CACHE BOOL "" FORCE)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION FALSE)
elseif(${PROJECT_NAME_SAFE}_LTO)
    if(CMAKE_CROSSCOMPILING_EMULATOR MATCHES "wine")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION FALSE)
    else()
        set(_CRF_LTO_ACTIVE TRUE)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
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
option(${PROJECT_NAME_SAFE}_USE_LIBSOUP3 "Use libsoup3" OFF)
option(${PROJECT_NAME_SAFE}_USE_LIBUV "Use libuv" OFF)
option(${PROJECT_NAME_SAFE}_USE_LIBEVENT "Use libevent" OFF)
option(${PROJECT_NAME_SAFE}_USE_LIBFETCH "Use libfetch" OFF)

option(${PROJECT_NAME_SAFE}_MULTIPLATFORM_INTEGRATION "Integrate with c-multiplatform" OFF)
option(${PROJECT_NAME_SAFE}_FETCH_CAH "Fetch c-abstract-http from GitHub" ON)
option(${PROJECT_NAME_SAFE}_FETCH_C_ORM "Fetch c-orm from GitHub" ON)
option(${PROJECT_NAME_SAFE}_FETCH_C_FS "Fetch c-fs from GitHub" ON)
option(${PROJECT_NAME_SAFE}_FETCH_CDD_C "Fetch cdd-c from GitHub" ON)
option(C_REST_ENABLE_HOT_RELOADING_AUTO_RESTART "Enable Hot-Reloading / Auto-Restart" ON)
option(${PROJECT_NAME_SAFE}_ENABLE_GRAPHQL "Enable GraphQL API Parsing & Resolving module" ON)
option(${PROJECT_NAME_SAFE}_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING "Enable Server-Side Template Engine" ON)
option(C_REST_ENABLE_RATE_LIMITING_THROTTLING_MIDDLEWARE "Enable Rate Limiting / Throttling Middleware" ON)
option(C_REST_ENABLE_JWT_JSON_WEB_TOKENS_AUTHENTICATION_MIDDLEWARE "Enable JWT Authentication Middleware" ON)
option(${PROJECT_NAME_SAFE}_ENABLE_RESPONSE_COMPRESSION_GZIP_BROTLI "Enable response compression (gzip/brotli)" ON)
option(C_REST_ENABLE_FULL_MULTIPART_FORM_STREAMING "Enable Full Multipart Form Streaming" ON)
option(C_REST_ENABLE_SERVER_SENT_EVENTS_SSE "Enable Server-Sent Events (SSE)" ON)
option(TRY_SYS_LIB "Fallback to system packages if vcpkg is not available" ON)

# Internal target-scoped build flags interface (zero global add_compile_options / add_link_options)
add_library(c_rest_framework_build_flags INTERFACE)

set(_CRF_APPLE_GNU_SDK "")
if(APPLE AND CMAKE_C_COMPILER_ID MATCHES "GNU")
    execute_process(COMMAND xcrun --show-sdk-path OUTPUT_VARIABLE _CRF_APPLE_GNU_SDK OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
        set(_CRF_MACOS_MIN "11.0")
    else()
        set(_CRF_MACOS_MIN "10.10")
    endif()
    if(_CRF_APPLE_GNU_SDK AND EXISTS "${_CRF_APPLE_GNU_SDK}")
        target_compile_options(c_rest_framework_build_flags INTERFACE
            "$<BUILD_INTERFACE:-iframework;${_CRF_APPLE_GNU_SDK}/System/Library/Frameworks>")
    endif()
    target_compile_options(c_rest_framework_build_flags INTERFACE
        "$<BUILD_INTERFACE:-mmacosx-version-min=${_CRF_MACOS_MIN}>")
    target_link_options(c_rest_framework_build_flags INTERFACE
        "$<BUILD_INTERFACE:-mmacosx-version-min=${_CRF_MACOS_MIN}>")
endif()

if(MSVC)
    target_compile_options(c_rest_framework_build_flags INTERFACE "$<BUILD_INTERFACE:/W4;/WX>")
    if(NOT CMAKE_CROSSCOMPILING_EMULATOR MATCHES "wine" AND MSVC_VERSION GREATER_EQUAL 1800)
        target_compile_options(c_rest_framework_build_flags INTERFACE "$<BUILD_INTERFACE:/FS>")
    endif()
    if(NOT ${PROJECT_NAME_SAFE}_MSVC_RTC STREQUAL "OFF" AND NOT ${PROJECT_NAME_SAFE}_MSVC_RTC STREQUAL "")
        string(REGEX REPLACE "^[/-]?RTC" "" _RTC_SUFFIX "${${PROJECT_NAME_SAFE}_MSVC_RTC}")
        foreach(flag_var
            CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
            CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
            string(REGEX REPLACE "[/-]RTC[1suc]+" "" ${flag_var} "${${flag_var}}")
            string(REGEX REPLACE "[/-]O[12x]" "/Od" ${flag_var} "${${flag_var}}")
        endforeach()
        target_compile_options(c_rest_framework_build_flags INTERFACE "$<BUILD_INTERFACE:/RTC${_RTC_SUFFIX}>")
    endif()
elseif(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(c_rest_framework_build_flags INTERFACE
        "$<BUILD_INTERFACE:-ansi;-pedantic;-Wall;-Wextra;-Werror;-Wformat=2;-Wshadow;-Wwrite-strings;-Wstrict-prototypes;-Wold-style-definition;-Wredundant-decls;-Wnested-externs>")
    if(CMAKE_C_COMPILER_ID MATCHES "Clang" AND _CRF_LTO_ACTIVE AND NOT APPLE)
        target_link_options(c_rest_framework_build_flags INTERFACE "$<BUILD_INTERFACE:-fuse-ld=lld>")
    endif()
    if(${PROJECT_NAME_SAFE}_USE_ASAN)
        target_compile_options(c_rest_framework_build_flags INTERFACE "$<BUILD_INTERFACE:-fsanitize=address;-fno-omit-frame-pointer>")
        target_link_options(c_rest_framework_build_flags INTERFACE "$<BUILD_INTERFACE:-fsanitize=address>")
    endif()
    if(${PROJECT_NAME_SAFE}_USE_TSAN)
        target_compile_options(c_rest_framework_build_flags INTERFACE "$<BUILD_INTERFACE:-fsanitize=thread>")
        target_link_options(c_rest_framework_build_flags INTERFACE "$<BUILD_INTERFACE:-fsanitize=thread>")
    endif()
endif()

function(c_rest_configure_target TARGET_NAME)
    target_link_libraries(${TARGET_NAME} PRIVATE "$<BUILD_INTERFACE:c_rest_framework_build_flags>")
    if(NOT MSVC)
        set_target_properties(${TARGET_NAME} PROPERTIES
            C_STANDARD 90
            C_STANDARD_REQUIRED ON
            C_EXTENSIONS ON
        )
    endif()
    if(_CRF_LTO_ACTIVE)
        set_target_properties(${TARGET_NAME} PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
endfunction()

function(c_rest_copy_runtime_dlls TARGET_NAME)
    if(WIN32 AND BUILD_SHARED_LIBS)
        foreach(_dep c-rest-framework cfs c89stringutils c-abstract-http c-orm c_cdd c_str_span)
            if(TARGET ${_dep})
                get_target_property(_dep_type ${_dep} TYPE)
                if(_dep_type STREQUAL "SHARED_LIBRARY")
                    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                            "$<TARGET_FILE:${_dep}>"
                            "$<TARGET_FILE_DIR:${TARGET_NAME}>"
                    )
                endif()
            endif()
        endforeach()
    endif()
endfunction()

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
        FetchContent_Declare(${PkgName} URL ${FetchURL})
        FetchContent_MakeAvailable(${PkgName})
        set(${PkgName}_RESOLVED ON)
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

if(IS_SYMLINK "${CMAKE_BINARY_DIR}/_deps")
    get_filename_component(_real_fc_base "${CMAKE_BINARY_DIR}/_deps" REALPATH)
    set(FETCHCONTENT_BASE_DIR "${_real_fc_base}" CACHE PATH "" FORCE)
endif()
include(FetchContent)
if(CMAKE_CROSSCOMPILING_EMULATOR MATCHES "wine")
    set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "" FORCE)
endif()

# Position-independent code for static subprojects when building shared library
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
if(NOT MSVC)
    set(CMAKE_C_STANDARD 90)
    set(CMAKE_C_STANDARD_REQUIRED ON)
    set(CMAKE_C_EXTENSIONS ON)
endif()

FetchContent_Declare(
    greatest
    GIT_REPOSITORY https://github.com/SamuelMarks/greatest.git
    GIT_TAG master
)
FetchContent_MakeAvailable(greatest)
if(NOT TARGET c_rest_greatest)
    add_library(c_rest_greatest INTERFACE)
    target_include_directories(c_rest_greatest SYSTEM INTERFACE "${greatest_SOURCE_DIR}")
endif()

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
set(C_ABSTRACT_HTTP_USE_LSQUIC ${${PROJECT_NAME_SAFE}_USE_LSQUIC} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_PICOQUIC ${${PROJECT_NAME_SAFE}_USE_PICOQUIC} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_NGHTTP3 ${${PROJECT_NAME_SAFE}_USE_NGHTTP3} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_MSH3 ${${PROJECT_NAME_SAFE}_USE_MSH3} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_LIBSOUP3 ${${PROJECT_NAME_SAFE}_USE_LIBSOUP3} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_LIBUV ${${PROJECT_NAME_SAFE}_USE_LIBUV} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_LIBEVENT ${${PROJECT_NAME_SAFE}_USE_LIBEVENT} CACHE BOOL "" FORCE)
set(C_ABSTRACT_HTTP_USE_LIBFETCH ${${PROJECT_NAME_SAFE}_USE_LIBFETCH} CACHE BOOL "" FORCE)

if(${PROJECT_NAME_SAFE}_FETCH_C_FS OR ${PROJECT_NAME_SAFE}_FETCH_CDD_C OR ${PROJECT_NAME_SAFE}_FETCH_CAH OR ${PROJECT_NAME_SAFE}_FETCH_C_ORM)
    set(CDD_DEPS "FETCHCONTENT" CACHE STRING "" FORCE)
    set(CFS_INSTALL ON CACHE BOOL "" FORCE)
    set(C_FS_INSTALL ON CACHE BOOL "" FORCE)
    set(C_STR_SPAN_INSTALL ON CACHE BOOL "" FORCE)
    set(C89STRINGUTILS_INSTALL ON CACHE BOOL "" FORCE)
    set(_OLD_BSL_DEPS ${BUILD_SHARED_LIBS})
    set(_OLD_BT_DEPS ${BUILD_TESTING})
    set(BUILD_SHARED_LIBS OFF)
    set(BUILD_TESTING OFF)

    if(NOT TARGET c89stringutils)
        FetchContent_Declare(
            c89stringutils
            GIT_REPOSITORY https://github.com/offscale/c89stringutils.git
            GIT_TAG master
        )
        FetchContent_MakeAvailable(c89stringutils)
        if(APPLE AND TARGET c89stringutils)
            target_compile_definitions(c89stringutils PRIVATE _DARWIN_C_SOURCE=1)
        endif()
    endif()

    if(NOT TARGET c_str_span)
        FetchContent_Declare(
            c-str-span
            GIT_REPOSITORY https://github.com/SamuelMarks/c-str-span.git
            GIT_TAG master
        )
        FetchContent_MakeAvailable(c-str-span)
    endif()

    if(${PROJECT_NAME_SAFE}_FETCH_C_FS AND NOT TARGET cfs)
        FetchContent_Declare(
            cfs
            GIT_REPOSITORY https://github.com/SamuelMarks/c-fs.git
            GIT_TAG master
        )
        FetchContent_MakeAvailable(cfs)
    endif()

    set(BUILD_TESTING ${_OLD_BT_DEPS})
    set(BUILD_SHARED_LIBS ${_OLD_BSL_DEPS})
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
            set(parson_RESOLVED ON)
            get_target_property(_parson_vcpkg_inc parson::parson INTERFACE_INCLUDE_DIRECTORIES)
            if(_parson_vcpkg_inc)
                target_include_directories(c-rest-framework BEFORE PUBLIC ${_parson_vcpkg_inc})
            endif()
            target_link_libraries(c-rest-framework PUBLIC parson::parson)
        endif()
    endif()

    if(NOT parson_RESOLVED AND TARGET parson)
        set(parson_RESOLVED ON)
        target_link_libraries(c-rest-framework PUBLIC parson)
        get_target_property(_parson_inc_dirs parson INTERFACE_INCLUDE_DIRECTORIES)
        if(NOT _parson_inc_dirs)
            get_target_property(_parson_inc_dirs parson INCLUDE_DIRECTORIES)
        endif()
    endif()

    if(NOT parson_RESOLVED)
        FetchContent_Declare(
            parson
            GIT_REPOSITORY https://github.com/SamuelMarks/parson.git
            GIT_TAG master
        )
        set(OLD_BSL ${BUILD_SHARED_LIBS})
        set(BUILD_SHARED_LIBS OFF)
        FetchContent_MakeAvailable(parson)
        if(TARGET parson AND NOT TARGET parson::parson)
            add_library(parson::parson ALIAS parson)
        endif()
        set(BUILD_SHARED_LIBS ${OLD_BSL})
        target_link_libraries(c-rest-framework PUBLIC parson)
        if(TARGET parson)
            target_include_directories(parson PUBLIC "$<BUILD_INTERFACE:${parson_SOURCE_DIR}>")
        endif()
        set(_parson_inc_dirs "${parson_SOURCE_DIR}")
        set(parson_RESOLVED ON)
    endif()

    foreach(_pdir ${_parson_vcpkg_inc} ${_parson_inc_dirs} "${parson_SOURCE_DIR}" "${CMAKE_CURRENT_LIST_DIR}/../parson")
        if(_pdir AND EXISTS "${_pdir}/parson.h")
            file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/crf_parson_inc")
            file(COPY "${_pdir}/parson.h" DESTINATION "${CMAKE_BINARY_DIR}/crf_parson_inc")
            target_include_directories(c-rest-framework BEFORE PUBLIC "${CMAKE_BINARY_DIR}/crf_parson_inc")
            break()
        endif()
    endforeach()
endmacro()

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

set_target_properties(c-rest-framework PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)
c_rest_configure_target(c-rest-framework)

if(CRF_COVERAGE_ENABLED)
    target_compile_options(c-rest-framework PRIVATE --coverage)
    target_link_options(c-rest-framework PUBLIC --coverage)
endif()

if(NOT EMSCRIPTEN AND NOT CMAKE_SYSTEM_NAME STREQUAL "DOS")
    target_sources(c-rest-framework PRIVATE src/modality_multi_process.c)
endif()

if(C_REST_ENABLE_HOT_RELOADING_AUTO_RESTART AND NOT EMSCRIPTEN)
    target_sources(c-rest-framework PRIVATE src/c_rest_hot_reload.c)
    target_compile_definitions(c-rest-framework PUBLIC C_REST_ENABLE_HOT_RELOADING_AUTO_RESTART)
endif()

if(C_REST_ENABLE_SERVER_SENT_EVENTS_SSE)
    target_sources(c-rest-framework PRIVATE src/c_rest_sse.c)
    target_compile_definitions(c-rest-framework PUBLIC C_REST_ENABLE_SERVER_SENT_EVENTS_SSE)
endif()

if(C_REST_ENABLE_RATE_LIMITING_THROTTLING_MIDDLEWARE)
    target_compile_definitions(c-rest-framework PUBLIC C_REST_ENABLE_RATE_LIMITING_THROTTLING_MIDDLEWARE)
endif()

if(C_REST_ENABLE_JWT_JSON_WEB_TOKENS_AUTHENTICATION_MIDDLEWARE)
    target_compile_definitions(c-rest-framework PUBLIC C_REST_ENABLE_JWT_JSON_WEB_TOKENS_AUTHENTICATION_MIDDLEWARE)
endif()

if(C_REST_ENABLE_FULL_MULTIPART_FORM_STREAMING)
    target_compile_definitions(c-rest-framework PUBLIC C_REST_ENABLE_FULL_MULTIPART_FORM_STREAMING)
endif()

if(${PROJECT_NAME_SAFE}_MULTIPLATFORM_INTEGRATION)
    target_compile_definitions(c-rest-framework PUBLIC ${PROJECT_NAME_SAFE}_MULTIPLATFORM_INTEGRATION)
endif()

if(${PROJECT_NAME_SAFE}_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING)
    target_compile_definitions(c-rest-framework PUBLIC C_REST_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING)
endif()

if(${PROJECT_NAME_SAFE}_ENABLE_GRAPHQL)
    target_compile_definitions(c-rest-framework PUBLIC ${PROJECT_NAME_SAFE}_ENABLE_GRAPHQL)
endif()

if(WIN32)
    if(${PROJECT_NAME_SAFE}_CHARSET STREQUAL "UNICODE")
        target_compile_definitions(c-rest-framework PUBLIC UNICODE _UNICODE)
    elseif(${PROJECT_NAME_SAFE}_CHARSET STREQUAL "ANSI")
        target_compile_definitions(c-rest-framework PUBLIC _MBCS)
    endif()
endif()

if(${PROJECT_NAME_SAFE}_ENABLE_RESPONSE_COMPRESSION_GZIP_BROTLI)
    target_compile_definitions(c-rest-framework PUBLIC
        ${PROJECT_NAME_SAFE}_ENABLE_RESPONSE_COMPRESSION_GZIP_BROTLI
        ${PROJECT_NAME_SAFE}_ENABLE_RESPONSE_COMPRESSION_GZIP
    )

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
            FetchContent_MakeAvailable(brotli)
            set(BUILD_SHARED_LIBS ${OLD_BSL_BROTLI})
            if(TARGET brotli)
                set_target_properties(brotli PROPERTIES EXCLUDE_FROM_ALL TRUE)
            endif()
            if(TARGET brotlienc-static)
                target_compile_definitions(brotlienc-static PRIVATE BROTLI_ENCODER_CLEANUP_ON_OOM)
                target_link_libraries(c-rest-framework PUBLIC brotlienc-static brotlidec-static brotlicommon-static)
            else()
                target_compile_definitions(brotlienc PRIVATE BROTLI_ENCODER_CLEANUP_ON_OOM)
                target_link_libraries(c-rest-framework PUBLIC brotlienc brotlidec brotlicommon)
            endif()
            target_compile_definitions(c-rest-framework PRIVATE C_REST_BROTLI_CLEANUP_ON_OOM=1)
            target_include_directories(c-rest-framework SYSTEM PUBLIC "${brotli_SOURCE_DIR}/c/include")
        endif()
    endif()
endif()

c_rest_framework_apply_crypto(c-rest-framework PUBLIC)

include(GenerateExportHeader)
generate_export_header(c-rest-framework EXPORT_MACRO_NAME C_REST_EXPORT EXPORT_FILE_NAME c_rest_export.h)
target_include_directories(c-rest-framework PUBLIC include "${CMAKE_CURRENT_BINARY_DIR}")
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
        set(CMAKE_REQUIRED_LIBRARIES dl)
        check_c_source_compiles("
            #include <dlfcn.h>
            int main(void) { void *p = dlopen(0, 0); (void)p; return 0; }
        " HAVE_DLOPEN)
        unset(CMAKE_REQUIRED_LIBRARIES)
        if(HAVE_DLOPEN)
            target_link_libraries(c-rest-framework PRIVATE dl)
            target_compile_definitions(c-rest-framework PRIVATE C_REST_HAS_DLOPEN=1)
        endif()
    endif()

    FetchContent_Declare(
        cdd-c
        GIT_REPOSITORY https://github.com/SamuelMarks/cdd-c.git
        GIT_TAG master
    )
    set(OLD_BSL_CDD ${BUILD_SHARED_LIBS})
    set(OLD_BUILD_TESTING_GLOBAL ${BUILD_TESTING})
    set(BUILD_SHARED_LIBS OFF)
    set(BUILD_TESTING OFF)
    set(C_CDD_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(CDD_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(cdd-c)
    set(BUILD_TESTING ${OLD_BUILD_TESTING_GLOBAL})
    set(BUILD_SHARED_LIBS ${OLD_BSL_CDD})

    if(TARGET c_cdd)
        foreach(_cdd_tgt c_cdd c_cdd_transform cdd-c cdd-c-transform)
            if(TARGET ${_cdd_tgt})
                foreach(_prop LINK_LIBRARIES INTERFACE_LINK_LIBRARIES)
                    get_target_property(_cdd_ll ${_cdd_tgt} ${_prop})
                    if(_cdd_ll)
                        list(REMOVE_ITEM _cdd_ll c_str_span_compiler_flags)
                        set_target_properties(${_cdd_tgt} PROPERTIES ${_prop} "${_cdd_ll}")
                    endif()
                endforeach()
            endif()
        endforeach()
        file(CONFIGURE OUTPUT "${CMAKE_BINARY_DIR}/cdd_json_namespace.h" CONTENT
"#ifndef CDD_JSON_NAMESPACE_H
#define CDD_JSON_NAMESPACE_H
#define json_array_append_boolean cdd_dom_json_array_append_boolean
#define json_array_append_null cdd_dom_json_array_append_null
#define json_array_append_number cdd_dom_json_array_append_number
#define json_array_append_string cdd_dom_json_array_append_string
#define json_array_append_value cdd_dom_json_array_append_value
#define json_array_clear cdd_dom_json_array_clear
#define json_array_get_boolean cdd_dom_json_get_array_boolean
#define json_array_remove cdd_dom_json_array_remove
#define json_array_replace_boolean cdd_dom_json_array_replace_boolean
#define json_array_replace_null cdd_dom_json_array_replace_null
#define json_array_replace_number cdd_dom_json_array_replace_number
#define json_array_replace_string cdd_dom_json_array_replace_string
#define json_array_replace_value cdd_dom_json_array_replace_value
#define json_free_serialized_string cdd_dom_json_free_serialized_string
#define json_object_clear cdd_dom_json_object_clear
#define json_object_dothas_value cdd_dom_json_object_dothas_value
#define json_object_dotremove cdd_dom_json_object_dotremove
#define json_object_dotset_boolean cdd_dom_json_object_dotset_boolean
#define json_object_dotset_null cdd_dom_json_object_dotset_null
#define json_object_dotset_number cdd_dom_json_object_dotset_number
#define json_object_dotset_string cdd_dom_json_object_dotset_string
#define json_object_dotset_value cdd_dom_json_object_dotset_value
#define json_object_get_boolean cdd_dom_json_object_get_boolean
#define json_object_has_value cdd_dom_json_object_has_value
#define json_object_has_value_of_type cdd_dom_json_object_has_value_of_type
#define json_object_remove cdd_dom_json_object_remove
#define json_object_set_boolean cdd_dom_json_object_set_boolean
#define json_object_set_null cdd_dom_json_object_set_null
#define json_object_set_number cdd_dom_json_object_set_number
#define json_object_set_string cdd_dom_json_object_set_string
#define json_object_set_value cdd_dom_json_object_set_value
#define json_serialize_to_file_pretty cdd_dom_json_serialize_to_file_pretty
#define json_set_allocation_functions cdd_dom_json_set_allocation_functions
#define json_validate cdd_dom_json_validate
#define json_value_equals cdd_dom_json_value_equals
#define json_value_free cdd_dom_json_value_free
#define json_value_get_boolean cdd_dom_json_value_get_boolean
#define json_value_get_type cdd_dom_json_value_get_type
#endif
" @ONLY)
        if(MSVC)
            target_compile_options(c_cdd PRIVATE "/FI${CMAKE_BINARY_DIR}/cdd_json_namespace.h")
            if(MSVC_VERSION GREATER_EQUAL 1800)
                target_compile_options(c_cdd PRIVATE /FIstdbool.h)
            endif()
        else()
            target_compile_options(c_cdd PRIVATE -include "${CMAKE_BINARY_DIR}/cdd_json_namespace.h")
        endif()
        if(DEFINED c89stringutils_SOURCE_DIR AND EXISTS "${c89stringutils_SOURCE_DIR}")
            target_include_directories(c_cdd PUBLIC
                "$<BUILD_INTERFACE:${c89stringutils_SOURCE_DIR}>"
                "$<BUILD_INTERFACE:${c89stringutils_SOURCE_DIR}/c89stringutils>"
                "$<BUILD_INTERFACE:${c89stringutils_SOURCE_DIR}/src>")
        endif()
        if(TARGET c89stringutils)
            target_link_libraries(c_cdd PUBLIC c89stringutils)
        endif()
        if(UNIX AND NOT APPLE AND CMAKE_C_COMPILER_ID MATCHES "Clang")
            target_compile_definitions(c_cdd PRIVATE "_Bool=unsigned char")
        endif()
        if(NOT WIN32 AND NOT APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "DOS")
            find_package(OpenSSL QUIET)
            if(OpenSSL_FOUND)
                target_link_libraries(c_cdd PUBLIC OpenSSL::SSL OpenSSL::Crypto)
            endif()
        endif()
        target_link_libraries(c-rest-framework PUBLIC c_cdd)
        target_compile_definitions(c-rest-framework PUBLIC ${PROJECT_NAME_SAFE}_USE_CDD_C)
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
    set(BUILD_TESTING OFF)
    FetchContent_MakeAvailable(c-abstract-http)
    set(BUILD_TESTING ${OLD_BUILD_TESTING_CAH})
    set(BUILD_SHARED_LIBS ${OLD_BSL_CAH})

    if(TARGET c-abstract-http)
        if(_CRF_APPLE_GNU_SDK AND EXISTS "${_CRF_APPLE_GNU_SDK}")
            target_compile_options(c-abstract-http PRIVATE
                "-iframework" "${_CRF_APPLE_GNU_SDK}/System/Library/Frameworks"
            )
        endif()
        if(EMSCRIPTEN)
            target_compile_definitions(c-abstract-http PRIVATE CDD_NO_UCONTEXT=1)
        endif()
        target_link_libraries(c-rest-framework PUBLIC c-abstract-http)
        target_compile_definitions(c-rest-framework PUBLIC ${PROJECT_NAME_SAFE}_USE_REAL_CAH)
    endif()
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
    set(BUILD_TESTING OFF)
    set(C_ORM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(C_ORM_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(c-orm)
    set(BUILD_TESTING ${OLD_BUILD_TESTING_CORM})
    set(BUILD_SHARED_LIBS ${OLD_BSL_CORM})

    if(TARGET c-orm)
        foreach(_corm_tgt c-orm c-orm-cli c_orm_legacy_cdd_db)
            if(TARGET ${_corm_tgt})
                if(MSVC)
                    target_compile_options(${_corm_tgt} PRIVATE "/FI${CMAKE_BINARY_DIR}/cdd_json_namespace.h")
                else()
                    target_compile_options(${_corm_tgt} PRIVATE -include "${CMAKE_BINARY_DIR}/cdd_json_namespace.h")
                endif()
            endif()
        endforeach()
        if(TARGET c_orm_legacy_cdd_db)
            set_target_properties(c_orm_legacy_cdd_db PROPERTIES EXCLUDE_FROM_ALL TRUE)
        endif()
        target_link_libraries(c-rest-framework PUBLIC c-orm)
        target_compile_definitions(c-rest-framework PUBLIC ${PROJECT_NAME_SAFE}_USE_REAL_C_ORM)
    endif()
endif()

if(${PROJECT_NAME_SAFE}_FETCH_C_FS AND TARGET cfs)
    target_link_libraries(c-rest-framework PUBLIC cfs)
endif()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set(BUILD_TESTING ${_CRF_TOP_BUILD_TESTING} CACHE BOOL "Build the testing tree." FORCE)
endif()

if(BUILD_TESTING)
    target_sources(c-rest-framework PRIVATE src/c_orm_mock.c)
    target_compile_definitions(c-rest-framework PUBLIC
        C_REST_TESTING_MALLOC_HOOK
        C_REST_TESTING_ENDIAN_HOOK
    )
endif()

if(EMSCRIPTEN)
    target_sources(c-rest-framework PRIVATE src/platform_posix.c)
    target_compile_options(c-rest-framework PUBLIC -pthread)
    target_link_options(c-rest-framework PUBLIC -pthread "-sASYNCIFY=1")
elseif(CMAKE_SYSTEM_NAME STREQUAL "DOS")
    target_sources(c-rest-framework PRIVATE src/platform_dos.c)
    target_compile_definitions(c-rest-framework PUBLIC C_REST_NO_THREADS=1)
    find_library(WATT32_LIBRARY NAMES wattcp wattcpwf watt)
    if(WATT32_LIBRARY)
        target_link_libraries(c-rest-framework PUBLIC "${WATT32_LIBRARY}")
    endif()
elseif(WIN32)
    target_sources(c-rest-framework PRIVATE src/platform_win32.c)
    target_link_libraries(c-rest-framework PUBLIC ws2_32)
    if(NOT ${PROJECT_NAME_SAFE}_THREADING)
        target_compile_definitions(c-rest-framework PUBLIC C_REST_NO_THREADS=1)
    endif()
else()
    target_sources(c-rest-framework PRIVATE src/platform_posix.c)
    if(${PROJECT_NAME_SAFE}_THREADING)
        find_package(Threads REQUIRED)
        target_link_libraries(c-rest-framework PUBLIC Threads::Threads)
    else()
        target_compile_definitions(c-rest-framework PUBLIC C_REST_NO_THREADS=1)
    endif()
endif()

# Strip any upstream coverage or cast-qual flags from fetched dependency interface targets
foreach(dep cdd-c c_cdd c-abstract-http c-orm c_str_span cfs c89stringutils parson)
    foreach(_tgt ${dep}_compiler_flags ${dep})
        if(TARGET ${_tgt})
            get_target_property(_opts ${_tgt} INTERFACE_COMPILE_OPTIONS)
            if(_opts)
                string(REPLACE "-fprofile-arcs" "" _opts "${_opts}")
                string(REPLACE "-ftest-coverage" "" _opts "${_opts}")
                string(REPLACE "--coverage" "" _opts "${_opts}")
                string(REPLACE "-Wcast-qual" "" _opts "${_opts}")
                if(MSVC)
                    string(REPLACE "-Zi" "/Z7" _opts "${_opts}")
                    string(REPLACE "/Zi" "/Z7" _opts "${_opts}")
                endif()
                set_target_properties(${_tgt} PROPERTIES INTERFACE_COMPILE_OPTIONS "${_opts}")
            endif()
            get_target_property(_lopts ${_tgt} INTERFACE_LINK_OPTIONS)
            if(_lopts)
                string(REPLACE "-fprofile-arcs" "" _lopts "${_lopts}")
                string(REPLACE "-ftest-coverage" "" _lopts "${_lopts}")
                string(REPLACE "--coverage" "" _lopts "${_lopts}")
                set_target_properties(${_tgt} PROPERTIES INTERFACE_LINK_OPTIONS "${_lopts}")
            endif()
        endif()
    endforeach()
endforeach()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    if(BUILD_TESTING)
        enable_testing()
        add_subdirectory(tests)
    endif()
    add_subdirectory(examples)
endif()
