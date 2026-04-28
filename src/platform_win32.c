/* clang-format off */
#include "c_rest_platform.h"

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdlib.h>
#include <string.h>
#include "c_rest_mem.h"
#include "c_rest_log.h"
#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>
/* clang-format on */

/*
 * Implementation of Windows platform layer.
 */

int c_rest_platform_init(void) {
  WSADATA wsaData;
  int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (res != 0) {
    return 1;
  }
  return 0;
}

int c_rest_platform_cleanup(void) {
  WSACleanup();
  return 0;
}

int c_rest_socket_create(c_rest_socket_t *out_sock) {
  SOCKET sock;
  if (!out_sock)
    return 1;

  sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock == INVALID_SOCKET) {
    return 1;
  }

  *out_sock = (c_rest_socket_t)sock;
  return 0;
}

int c_rest_socket_bind(c_rest_socket_t sock, const char *host,
                       unsigned short port) {
  struct sockaddr_in addr;
  SOCKET s = (SOCKET)sock;
  int res;

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr =
      inet_addr(host); /* Very basic, INADDR_ANY if "0.0.0.0" */

  res = bind(s, (struct sockaddr *)&addr, sizeof(addr));
  if (res == SOCKET_ERROR) {
    return 1;
  }

  return 0;
}

int c_rest_socket_listen(c_rest_socket_t sock, int backlog) {
  SOCKET s = (SOCKET)sock;
  int res = listen(s, backlog);
  if (res == SOCKET_ERROR) {
    return 1;
  }
  return 0;
}

int c_rest_socket_accept(c_rest_socket_t server_sock,
                         c_rest_socket_t *out_client_sock) {
  SOCKET s = (SOCKET)server_sock;
  SOCKET client;
  struct sockaddr_in client_addr;
  int addr_len = sizeof(client_addr);

  if (!out_client_sock)
    return 1;

  client = accept(s, (struct sockaddr *)&client_addr, &addr_len);
  if (client == INVALID_SOCKET) {
    return 1;
  }

  *out_client_sock = (c_rest_socket_t)client;
  return 0;
}

int c_rest_socket_set_nonblocking(c_rest_socket_t sock, int nonblocking) {
  SOCKET s = (SOCKET)sock;
  u_long mode = nonblocking ? 1 : 0;
  int res = ioctlsocket(s, FIONBIO, &mode);
  if (res != NO_ERROR) {
    return 1;
  }
  return 0;
}

int c_rest_socket_close(c_rest_socket_t sock) {
  SOCKET s = (SOCKET)sock;
  int res = closesocket(s);
  if (res == SOCKET_ERROR) {
    return 1;
  }
  return 0;
}

struct thread_wrapper_args {
  c_rest_thread_fn func;
  void *arg;
};

static unsigned __stdcall thread_wrapper(void *arg) {
  struct thread_wrapper_args *args = (struct thread_wrapper_args *)arg;
  args->func(args->arg);
  C_REST_FREE((void *)(args));
  return 0;
}

int c_rest_thread_create(c_rest_thread_t *out_thread, c_rest_thread_fn func,
                         void *arg) {
  HANDLE hThread;
  unsigned threadID;
  struct thread_wrapper_args *args;

  if (!out_thread || !func)
    return 1;

  if (C_REST_MALLOC(sizeof(struct thread_wrapper_args), (void **)&(args)) !=
      0) {
    LOG_DEBUG("C_REST_MALLOC failed");
    args = NULL;
  }
  if (!args)
    return 1;

  args->func = func;
  args->arg = arg;

  hThread = (HANDLE)_beginthreadex(NULL, 0, thread_wrapper, args, 0, &threadID);
  if (!hThread) {
    C_REST_FREE((void *)(args));
    return 1;
  }

  *out_thread = (c_rest_thread_t)hThread;
  return 0;
}

int c_rest_thread_join(c_rest_thread_t thread) {
  HANDLE hThread = (HANDLE)thread;
  DWORD res;

  if (!hThread)
    return 1;

  res = WaitForSingleObject(hThread, INFINITE);
  if (res != WAIT_OBJECT_0) {
    return 1;
  }

  CloseHandle(hThread);
  return 0;
}

int c_rest_mutex_create(c_rest_mutex_t *out_mutex) {
  CRITICAL_SECTION *cs;

  if (!out_mutex)
    return 1;

  if (C_REST_MALLOC(sizeof(CRITICAL_SECTION), (void **)&cs) != 0) {
    LOG_DEBUG("C_REST_MALLOC failed");
    cs = NULL;
  }
  if (!cs)
    return 1;

  InitializeCriticalSection(cs);
  *out_mutex = (c_rest_mutex_t)cs;
  return 0;
}

int c_rest_mutex_lock(c_rest_mutex_t mutex) {
  CRITICAL_SECTION *cs = (CRITICAL_SECTION *)mutex;
  if (!cs)
    return 1;

  EnterCriticalSection(cs);
  return 0;
}

int c_rest_mutex_unlock(c_rest_mutex_t mutex) {
  CRITICAL_SECTION *cs = (CRITICAL_SECTION *)mutex;
  if (!cs)
    return 1;

  LeaveCriticalSection(cs);
  return 0;
}

int c_rest_mutex_destroy(c_rest_mutex_t mutex) {
  CRITICAL_SECTION *cs = (CRITICAL_SECTION *)mutex;
  if (!cs)
    return 1;

  DeleteCriticalSection(cs);
  C_REST_FREE((void *)(cs));
  return 0;
}

/* Cond vars are not trivial on pre-Vista windows natively.
 * We can implement a naive approach for MSVC 2005 compatibility.
 */
struct cond_impl {
  HANDLE events[2];
  unsigned int waiters_count;
  CRITICAL_SECTION waiters_count_lock;
};

int c_rest_cond_create(c_rest_cond_t *out_cond) {
  struct cond_impl *cond;

  if (!out_cond)
    return 1;

  if (C_REST_MALLOC(sizeof(struct cond_impl), (void **)&cond) != 0) {
    LOG_DEBUG("C_REST_MALLOC failed");
    cond = NULL;
  }
  if (!cond)
    return 1;

  cond->waiters_count = 0;
  InitializeCriticalSection(&cond->waiters_count_lock);

  cond->events[0] = CreateEventA(NULL, FALSE, FALSE, NULL); /* Signal */
  cond->events[1] = CreateEventA(NULL, TRUE, FALSE, NULL);  /* Broadcast */

  if (!cond->events[0] || !cond->events[1]) {
    if (cond->events[0])
      CloseHandle(cond->events[0]);
    if (cond->events[1])
      CloseHandle(cond->events[1]);
    DeleteCriticalSection(&cond->waiters_count_lock);
    C_REST_FREE((void *)(cond));
    return 1;
  }

  *out_cond = (c_rest_cond_t)cond;
  return 0;
}

int c_rest_cond_wait(c_rest_cond_t c, c_rest_mutex_t m) {
  struct cond_impl *cond = (struct cond_impl *)c;
  int wait_res;
  int last_waiter;

  if (!cond)
    return 1;

  EnterCriticalSection(&cond->waiters_count_lock);
  cond->waiters_count++;
  LeaveCriticalSection(&cond->waiters_count_lock);

  c_rest_mutex_unlock(m);

  wait_res = WaitForMultipleObjects(2, cond->events, FALSE, INFINITE);

  EnterCriticalSection(&cond->waiters_count_lock);
  cond->waiters_count--;
  last_waiter = (wait_res == WAIT_OBJECT_0 + 1) && (cond->waiters_count == 0);
  LeaveCriticalSection(&cond->waiters_count_lock);

  if (last_waiter) {
    ResetEvent(cond->events[1]);
  }

  c_rest_mutex_lock(m);

  return 0;
}

int c_rest_cond_signal(c_rest_cond_t c) {
  struct cond_impl *cond = (struct cond_impl *)c;
  int have_waiters;

  if (!cond)
    return 1;

  EnterCriticalSection(&cond->waiters_count_lock);
  have_waiters = cond->waiters_count > 0;
  LeaveCriticalSection(&cond->waiters_count_lock);

  if (have_waiters) {
    SetEvent(cond->events[0]);
  }

  return 0;
}

int c_rest_cond_destroy(c_rest_cond_t c) {
  struct cond_impl *cond = (struct cond_impl *)c;

  if (!cond)
    return 1;

  CloseHandle(cond->events[0]);
  CloseHandle(cond->events[1]);
  DeleteCriticalSection(&cond->waiters_count_lock);
  C_REST_FREE((void *)(cond));

  return 0;
}

int c_rest_process_create(c_rest_process_t *out_proc, const char *executable,
                          char *const argv[]) {
  STARTUPINFOA si;
  PROCESS_INFORMATION pi;
  char cmdline[1024];
  int i = 0;
  size_t len = 0;

  if (!out_proc || !executable)
    return 1;

  memset(&si, 0, sizeof(si));
  si.cb = sizeof(si);
  memset(&pi, 0, sizeof(pi));

  cmdline[0] = '\0';

  /* Naive command line building - real implementation needs escaping */
  if (argv) {
    while (argv[i]) {
      size_t arg_len = strlen(argv[i]);
      if (len + arg_len + 2 < sizeof(cmdline)) {
#if defined(_MSC_VER) && _MSC_VER >= 1400
        strcat_s(cmdline, sizeof(cmdline), argv[i]);
        strcat_s(cmdline, sizeof(cmdline), " ");
#else
        strcat(cmdline, argv[i]);
        strcat(cmdline, " ");
#endif
        len += arg_len + 1;
      }
      i++;
    }
  }

  if (!CreateProcessA(executable, cmdline[0] ? cmdline : NULL, NULL, NULL,
                      FALSE, 0, NULL, NULL, &si, &pi)) {
    return 1;
  }

  CloseHandle(pi.hThread);
  *out_proc = (c_rest_process_t)pi.hProcess;
  return 0;
}

int c_rest_process_wait(c_rest_process_t proc, int *out_exit_code) {
  HANDLE hProc = (HANDLE)proc;
  DWORD res;
  DWORD exit_code = 0;

  if (!hProc)
    return 1;

  res = WaitForSingleObject(hProc, INFINITE);
  if (res != WAIT_OBJECT_0) {
    return 1;
  }

  if (out_exit_code) {
    if (GetExitCodeProcess(hProc, &exit_code)) {
      *out_exit_code = (int)exit_code;
    } else {
      *out_exit_code = 1;
    }
  }

  CloseHandle(hProc);
  return 0;
}

int c_rest_timer_get_ms(unsigned long *out_ms) {
  if (!out_ms)
    return 1;
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 28159)
#endif
  *out_ms = GetTickCount();
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
  return 0;
}

int c_rest_random_get(void *buffer, size_t size) {
  if (!buffer || size == 0)
    return 1;
  /* Basic placeholder for RNG - we can use rand for now or proper
   * CryptGenRandom */
  /* Strict C89 fallback: */
  {
    size_t i;
    char *buf = (char *)buffer;
    for (i = 0; i < size; ++i) {
      buf[i] = (char)(rand() % 256);
    }
  }
  return 0;
}

int c_rest_get_last_error(int *out_error) {
  if (!out_error)
    return 1;
  *out_error = (int)GetLastError();
  return 0;
}

int c_rest_socket_recv(c_rest_socket_t sock, void *buf, size_t len,
                       size_t *out_read) {
  int ret;
  if (!buf || !out_read)
    return 1;
  *out_read = 0;
  ret = recv((SOCKET)sock, (char *)buf, (int)len, 0);
  if (ret > 0) {
    *out_read = (size_t)ret;
    return 0;
  }
  return 1;
}

int c_rest_socket_send(c_rest_socket_t sock, const void *buf, size_t len,
                       size_t *out_written) {
  int ret;
  if (!buf || !out_written)
    return 1;
  *out_written = 0;
  ret = send((SOCKET)sock, (const char *)buf, (int)len, 0);
  if (ret > 0) {
    *out_written = (size_t)ret;
    return 0;
  }
  return 1;
}
