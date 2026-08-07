set(CMAKE_SYSTEM_NAME DOS)
set(CMAKE_SYSTEM_PROCESSOR i386)

# Assuming DJGPP is installed and in the PATH, or DJDIR is set
if(DEFINED ENV{DJDIR})
    set(DJDIR $ENV{DJDIR})
else()
    # Default fallback, might need to be overridden by the user
    set(DJDIR /dev/env/DJDIR)
endif()

set(CMAKE_C_COMPILER i586-pc-msdosdjgpp-gcc)
set(CMAKE_CXX_COMPILER i586-pc-msdosdjgpp-g++)
set(CMAKE_AR i586-pc-msdosdjgpp-ar)
set(CMAKE_RANLIB i586-pc-msdosdjgpp-ranlib)

set(CMAKE_FIND_ROOT_PATH ${DJDIR})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# WATT-32 specific options for DJGPP
add_compile_definitions(C_REST_WATT32=1)

# Ensure strict C89 flags for DJGPP
set(CMAKE_C_FLAGS_INIT "-std=c89 -pedantic")
