# WebAssembly (WASM) Support

The `c-rest-framework` includes full support for compiling to WebAssembly (WASM) using Emscripten. This allows you to run your REST endpoints, clients, and parsers securely within modern web browsers or in lightweight Node.js environments.

## How to Build

First, ensure you have installed and activated the [Emscripten SDK (emsdk)](https://emscripten.org/docs/getting_started/downloads.html).

You can easily compile the framework to WebAssembly using the provided build script:

```bash
./wasm_compile.sh
```

Alternatively, you can manually use `emcmake`:

```bash
mkdir build-wasm
cd build-wasm
emcmake cmake ..
cmake --build .
```

## Running the Tests (Node.js)

When compiled to WebAssembly, you can test the output using Node.js natively:

```bash
node build-wasm/tests/c-rest-framework-tests.js
```

## Networking Limitations

Because WebAssembly executes in a secure sandbox (such as a Web Browser), raw POSIX socket APIs like `bind()`, `listen()`, and `accept()` are restricted.

- **In Node.js:** Emscripten provides a Node POSIX socket bridge. Basic TCP networking should function as expected.
- **In the Browser:** You cannot bind to raw TCP ports or act as a traditional standalone web server.
  - To make network requests out of the browser, Emscripten automatically proxies `c_rest_socket_send`/`c_rest_socket_recv` over WebSockets.
  - If you need to expose a listening server to the outside world from within a browser, you must deploy an external proxy such as **WebSockify** and configure `-s SOCKETS=1` during your application compilation.
- **Processes:** Modalities utilizing `fork()` and `execvp()` are disabled since WebAssembly has no concept of OS-level process management. Thread-based modalities (using Web Workers via pthreads) remain fully functional thanks to Emscripten's `-pthread` and `USE_PTHREADS=1` support.

## Threading & Asyncify

We automatically compile the framework with `-s USE_PTHREADS=1` and `-s ASYNCIFY=1`.
- **Pthreads** are mapped to Web Workers.
- **Asyncify** allows standard synchronous blocking network operations (like `recv`) to safely yield back to the browser's main event loop without deadlocking the UI.
