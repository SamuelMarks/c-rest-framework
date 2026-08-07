# DOS Platform Support

The C REST Framework includes experimental support for DOS platforms, primarily targeted through cross-compilation with Open Watcom or DJGPP.

## Limitations

DOS does not have a native threading model or a built-in TCP/IP stack (like BSD sockets). As a result, the following limitations apply:
- **No Multithreading:** The `C_REST_MODALITY_MULTI_THREAD` modality is not supported and will fail initialization.
- **No Multiprocessing:** The `C_REST_MODALITY_MULTI_PROCESS` modality is not supported and will fail initialization.
- **Networking:** Network socket support is provided via the **WATT-32 TCP/IP** stack. To build for DOS with networking, you must have WATT-32 installed (`wattcp.lib` / `wattcp.a`). Without it, all socket operations will return `C_REST_ERROR_NOT_SUPPORTED`.
- **Timing and Randomness:** Resolution for timing relies on the standard `clock()` or DJGPP `gettimeofday()`. Randomness is initialized via `srand(time(NULL) ^ clock())`, which may not be cryptographically secure for production environments.

## Building for DOS

Use the provided CMake toolchain files:
- **DJGPP**: `cmake -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/DJGPP.cmake ..`
- **Open Watcom**: `cmake -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/OpenWatcom.cmake ..`

Ensure the `WATT-32` library is available in your DOS build environment (e.g. `$DJDIR/lib` or `$WATCOM/lib386/dos`) so it can be dynamically detected by CMake.
