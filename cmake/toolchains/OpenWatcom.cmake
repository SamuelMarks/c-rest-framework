set(CMAKE_SYSTEM_NAME DOS)
set(CMAKE_SYSTEM_PROCESSOR i386)

# Assuming WATCOM environment variable is set
if(NOT DEFINED ENV{WATCOM})
    message(WARNING "WATCOM environment variable not set. Open Watcom may not be found.")
endif()

set(CMAKE_C_COMPILER wcc386)
set(CMAKE_CXX_COMPILER wpp386)
set(CMAKE_AR wlib)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Open Watcom specific compiler flags for DOS


# WATT-32 specific options for Open Watcom
add_compile_definitions(C_REST_WATT32=1)

# Ensure strict C89 flags for Open Watcom
# wcc386 uses -za to disable language extensions (strict ANSI C)
set(CMAKE_C_FLAGS_INIT "-bt=dos -ms -za")
