#define c_rest_platform_init win32_c_rest_platform_init
#define c_rest_platform_cleanup win32_c_rest_platform_cleanup
#define c_rest_socket_create win32_c_rest_socket_create
#define c_rest_socket_set_nonblocking win32_c_rest_socket_set_nonblocking
#define c_rest_socket_bind win32_c_rest_socket_bind
#define c_rest_socket_listen win32_c_rest_socket_listen
#define c_rest_socket_accept win32_c_rest_socket_accept
#define c_rest_socket_close win32_c_rest_socket_close
#define c_rest_socket_recv win32_c_rest_socket_recv
#define c_rest_socket_send win32_c_rest_socket_send
#define c_rest_thread_create win32_c_rest_thread_create
#define c_rest_thread_join win32_c_rest_thread_join
#define c_rest_mutex_create win32_c_rest_mutex_create
#define c_rest_mutex_lock win32_c_rest_mutex_lock
#define c_rest_mutex_unlock win32_c_rest_mutex_unlock
#define c_rest_mutex_destroy win32_c_rest_mutex_destroy
#define c_rest_cond_create win32_c_rest_cond_create
#define c_rest_cond_wait win32_c_rest_cond_wait
#define c_rest_cond_signal win32_c_rest_cond_signal
#define c_rest_cond_destroy win32_c_rest_cond_destroy
#define c_rest_process_create win32_c_rest_process_create
#define c_rest_process_wait win32_c_rest_process_wait
#define c_rest_timer_get_ms win32_c_rest_timer_get_ms
#define c_rest_random_get win32_c_rest_random_get
#define c_rest_get_last_error win32_c_rest_get_last_error

/* clang-format off */
#include "winsock2.h"
#include "process.h"
#include "c_rest_error.h"
#include "c_rest_platform.h"
#include "c_rest_testing_mocks.h"
#include "greatest.h"
#include "greatest_clean.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
/* clang-format on */

static int g_wsa_startup_fail = 0;
static int g_wsa_cleanup_fail = 0;
static int g_socket_fail = 0;
static int g_ioctlsocket_fail = 0;
static int g_bind_fail = 0;
static int g_listen_fail = 0;
static int g_accept_fail = 0;
static int g_closesocket_fail = 0;
static int g_recv_fail = 0;
static int g_send_fail = 0;
static int g_event_create_count = 0;
static int g_wait_fail = 0;
static int g_search_path_fail = 0;
static int g_create_proc_fail = 0;
static int g_exit_code_fail = 0;
static volatile int g_mock_win32_dummy = 0;

int WSAStartup(WORD wVersionRequired, WSADATA *lpWSAData) {
  g_mock_win32_dummy += (int)wVersionRequired + (int)(size_t)lpWSAData;
  return g_wsa_startup_fail ? -1 : 0;
}

int WSACleanup(void) { return g_wsa_cleanup_fail ? -1 : 0; }

SOCKET socket(int af, int type, int protocol) {
  g_mock_win32_dummy += af + type + protocol;
  return g_socket_fail ? INVALID_SOCKET : 100;
}

int ioctlsocket(SOCKET s, long cmd, u_long *argp) {
  g_mock_win32_dummy += (int)s + (int)cmd + (int)(size_t)argp;
  return g_ioctlsocket_fail ? SOCKET_ERROR : 0;
}

int bind(SOCKET s, const struct sockaddr *name, int namelen) {
  g_mock_win32_dummy += (int)s + (int)(size_t)name + namelen;
  return g_bind_fail ? SOCKET_ERROR : 0;
}

int listen(SOCKET s, int backlog) {
  g_mock_win32_dummy += (int)s + backlog;
  return g_listen_fail ? SOCKET_ERROR : 0;
}

SOCKET accept(SOCKET s, struct sockaddr *addr, int *addrlen) {
  g_mock_win32_dummy += (int)s + (int)(size_t)addr + (int)(size_t)addrlen;
  return g_accept_fail ? INVALID_SOCKET : 101;
}

int closesocket(SOCKET s) {
  g_mock_win32_dummy += (int)s;
  return g_closesocket_fail ? SOCKET_ERROR : 0;
}

int recv(SOCKET s, char *buf, int len, int flags) {
  g_mock_win32_dummy += (int)s + (int)(size_t)buf + flags;
  return g_recv_fail ? -1 : len;
}

int send(SOCKET s, const char *buf, int len, int flags) {
  g_mock_win32_dummy += (int)s + (int)(size_t)buf + flags;
  return g_send_fail ? -1 : len;
}

#undef htons
unsigned short htons(unsigned short hostshort) { return hostshort; }

unsigned long inet_addr(const char *cp) {
  if (strcmp(cp, "invalid") == 0)
    return INADDR_NONE;
  if (strcmp(cp, "255.255.255.255") == 0)
    return INADDR_NONE;
  return 0x7f000001UL;
}

void InitializeCriticalSection(CRITICAL_SECTION *lpCriticalSection) {
  g_mock_win32_dummy += (int)(size_t)lpCriticalSection;
}

void EnterCriticalSection(CRITICAL_SECTION *lpCriticalSection) {
  g_mock_win32_dummy += (int)(size_t)lpCriticalSection;
}

void LeaveCriticalSection(CRITICAL_SECTION *lpCriticalSection) {
  g_mock_win32_dummy += (int)(size_t)lpCriticalSection;
}

void DeleteCriticalSection(CRITICAL_SECTION *lpCriticalSection) {
  g_mock_win32_dummy += (int)(size_t)lpCriticalSection;
}

HANDLE CreateEventA(void *lpEventAttributes, int bManualReset,
                    int bInitialState, const char *lpName) {
  g_mock_win32_dummy += (int)(size_t)lpEventAttributes + bManualReset +
                        bInitialState + (int)(size_t)lpName;
  return (HANDLE)(size_t)(++g_event_create_count + 200);
}

int SetEvent(HANDLE hEvent) {
  g_mock_win32_dummy += (int)(size_t)hEvent;
  return 1;
}

DWORD WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds) {
  g_mock_win32_dummy += (int)(size_t)hHandle + (int)dwMilliseconds;
  return g_wait_fail ? WAIT_TIMEOUT : WAIT_OBJECT_0;
}

int CloseHandle(HANDLE hObject) {
  g_mock_win32_dummy += (int)(size_t)hObject;
  return 1;
}

uintptr_t _beginthreadex(void *security, unsigned stack_size,
                         unsigned(__stdcall *start_address)(void *),
                         void *arglist, unsigned initflag, unsigned *thrdaddr) {
  g_mock_win32_dummy += (int)(size_t)security + (int)stack_size + (int)initflag;
  *thrdaddr = 1234;
  start_address(arglist);
  return 300;
}

DWORD SearchPathA(const char *lpPath, const char *lpFileName,
                  const char *lpExtension, DWORD nBufferLength, char *lpBuffer,
                  char **lpFilePart) {
  g_mock_win32_dummy += (int)(size_t)lpPath + (int)(size_t)lpExtension +
                        (int)nBufferLength + (int)(size_t)lpFilePart;
  if (g_search_path_fail)
    return 0;
  strcpy(lpBuffer, "/mock/bin/");
  strcat(lpBuffer, lpFileName);
  return (DWORD)strlen(lpBuffer);
}

int CreateProcessA(const char *lpApplicationName, char *lpCommandLine,
                   void *lpProcessAttributes, void *lpThreadAttributes,
                   int bInheritHandles, DWORD dwCreationFlags,
                   void *lpEnvironment, const char *lpCurrentDirectory,
                   STARTUPINFOA *lpStartupInfo,
                   PROCESS_INFORMATION *lpProcessInformation) {
  g_mock_win32_dummy +=
      (int)(size_t)lpApplicationName + (int)(size_t)lpCommandLine +
      (int)(size_t)lpProcessAttributes + (int)(size_t)lpThreadAttributes +
      bInheritHandles + (int)dwCreationFlags + (int)(size_t)lpEnvironment +
      (int)(size_t)lpCurrentDirectory + (int)(size_t)lpStartupInfo;
  if (g_create_proc_fail)
    return 0;
  lpProcessInformation->hProcess = (HANDLE)400;
  lpProcessInformation->hThread = (HANDLE)401;
  lpProcessInformation->dwProcessId = 500;
  lpProcessInformation->dwThreadId = 501;
  return 1;
}

int GetExitCodeProcess(HANDLE hProcess, DWORD *lpExitCode) {
  g_mock_win32_dummy += (int)(size_t)hProcess;
  if (g_exit_code_fail)
    return 0;
  *lpExitCode = 0;
  return 1;
}

DWORD GetTickCount(void) { return 1234567UL; }

DWORD GetLastError(void) { return 42UL; }

#include "../src/platform_win32.c"

static c_rest_error_t mock_thread_worker(void *arg) {
  int *val = (int *)arg;
  *val = 77;
  return C_REST_OK;
}

static void *mock_fail_malloc(size_t sz) {
  g_mock_win32_dummy += (int)sz;
  return NULL;
}

TEST test_win32_lifecycle(void) {
  c_rest_error_t rc;

  rc = win32_c_rest_platform_init();
  ASSERT_EQ(C_REST_OK, rc);

  g_wsa_startup_fail = 1;
  rc = win32_c_rest_platform_init();
  ASSERT_EQ(C_REST_ERROR_NETWORK, rc);
  g_wsa_startup_fail = 0;

  rc = win32_c_rest_platform_cleanup();
  ASSERT_EQ(C_REST_OK, rc);

  g_wsa_cleanup_fail = 1;
  rc = win32_c_rest_platform_cleanup();
  ASSERT_EQ(C_REST_ERROR_NETWORK, rc);
  g_wsa_cleanup_fail = 0;

  g_mock_platform_cleanup_fail = 1;
  rc = win32_c_rest_platform_cleanup();
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);
  g_mock_platform_cleanup_fail = 0;

  PASS();
}

TEST test_win32_socket_ops(void) {
  c_rest_error_t rc;
  c_rest_socket_t s = 0;
  c_rest_socket_t client = 0;
  char buf[32];
  size_t n = 0;

  rc = win32_c_rest_socket_create(NULL);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);

  rc = win32_c_rest_socket_create(&s);
  ASSERT_EQ(C_REST_OK, rc);

  rc = win32_c_rest_socket_set_nonblocking(s, 1);
  ASSERT_EQ(C_REST_OK, rc);
  rc = win32_c_rest_socket_set_nonblocking(s, 0);
  ASSERT_EQ(C_REST_OK, rc);

  g_ioctlsocket_fail = 1;
  rc = win32_c_rest_socket_set_nonblocking(s, 1);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);
  g_ioctlsocket_fail = 0;

  rc = win32_c_rest_socket_bind(s, NULL, 8080);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);

  rc = win32_c_rest_socket_bind(s, "invalid", 8080);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);

  rc = win32_c_rest_socket_bind(s, "127.0.0.1", 8080);
  ASSERT_EQ(C_REST_OK, rc);

  rc = win32_c_rest_socket_bind(s, "255.255.255.255", 8080);
  ASSERT_EQ(C_REST_OK, rc);

  g_bind_fail = 1;
  rc = win32_c_rest_socket_bind(s, "127.0.0.1", 8080);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);
  g_bind_fail = 0;

  rc = win32_c_rest_socket_listen(s, 128);
  ASSERT_EQ(C_REST_OK, rc);

  g_listen_fail = 1;
  rc = win32_c_rest_socket_listen(s, 128);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);
  g_listen_fail = 0;

  rc = win32_c_rest_socket_accept(s, NULL);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);

  g_accept_fail = 1;
  rc = win32_c_rest_socket_accept(s, &client);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);
  g_accept_fail = 0;

  rc = win32_c_rest_socket_accept(s, &client);
  ASSERT_EQ(C_REST_OK, rc);

  rc = win32_c_rest_socket_recv(s, NULL, sizeof(buf), &n);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);
  rc = win32_c_rest_socket_recv(s, buf, 0, &n);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);
  rc = win32_c_rest_socket_recv(s, buf, sizeof(buf), NULL);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);

  g_recv_fail = 1;
  rc = win32_c_rest_socket_recv(s, buf, sizeof(buf), &n);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);
  g_recv_fail = 0;

  rc = win32_c_rest_socket_recv(s, buf, sizeof(buf), &n);
  ASSERT_EQ(C_REST_OK, rc);
  ASSERT_EQ(sizeof(buf), n);

  rc = win32_c_rest_socket_send(s, NULL, sizeof(buf), &n);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);
  rc = win32_c_rest_socket_send(s, buf, 0, &n);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);
  rc = win32_c_rest_socket_send(s, buf, sizeof(buf), NULL);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);

  g_send_fail = 1;
  rc = win32_c_rest_socket_send(s, buf, sizeof(buf), &n);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);
  g_send_fail = 0;

  rc = win32_c_rest_socket_send(s, buf, sizeof(buf), &n);
  ASSERT_EQ(C_REST_OK, rc);
  ASSERT_EQ(sizeof(buf), n);

  g_mock_socket_fail = 200;
  rc = win32_c_rest_socket_send(s, buf, sizeof(buf), &n);
  ASSERT_EQ(C_REST_OK, rc);

  g_mock_socket_fail = 201;
  rc = win32_c_rest_socket_send(s, buf, sizeof(buf), &n);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);

  g_mock_socket_fail = 202;
  rc = win32_c_rest_socket_send(s, buf, sizeof(buf), &n);
  ASSERT_EQ(C_REST_OK, rc);

  g_mock_socket_fail = 203;
  rc = win32_c_rest_socket_send(s, buf, sizeof(buf), &n);
  ASSERT_EQ(C_REST_OK, rc);
  g_mock_socket_fail = 0;

  rc = win32_c_rest_socket_close(s);
  ASSERT_EQ(C_REST_OK, rc);

  g_closesocket_fail = 1;
  rc = win32_c_rest_socket_close(s);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);
  g_closesocket_fail = 0;

  PASS();
}

TEST test_win32_threading_and_mutex(void) {
  c_rest_error_t rc;
  c_rest_thread_t th = 0;
  c_rest_mutex_t mtx = 0;
  int worker_arg = 0;

  rc = win32_c_rest_thread_create(NULL, mock_thread_worker, &worker_arg);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);
  rc = win32_c_rest_thread_create(&th, NULL, &worker_arg);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);

  g_crf_malloc_hook = mock_fail_malloc;
  rc = win32_c_rest_thread_create(&th, mock_thread_worker, &worker_arg);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);
  g_crf_malloc_hook = NULL;

  rc = win32_c_rest_thread_create(&th, mock_thread_worker, &worker_arg);
  ASSERT_EQ(C_REST_OK, rc);
  ASSERT_EQ(77, worker_arg);

  rc = win32_c_rest_thread_join(0);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);
  rc = win32_c_rest_thread_join((c_rest_thread_t)(size_t)INVALID_HANDLE_VALUE);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);

  g_wait_fail = 1;
  rc = win32_c_rest_thread_join(th);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);
  g_wait_fail = 0;

  rc = win32_c_rest_thread_join(th);
  ASSERT_EQ(C_REST_OK, rc);

  rc = win32_c_rest_mutex_create(NULL);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);

  g_crf_malloc_hook = mock_fail_malloc;
  rc = win32_c_rest_mutex_create(&mtx);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);
  g_crf_malloc_hook = NULL;

  rc = win32_c_rest_mutex_create(&mtx);
  ASSERT_EQ(C_REST_OK, rc);

  rc = win32_c_rest_mutex_lock(0);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);
  rc = win32_c_rest_mutex_lock(mtx);
  ASSERT_EQ(C_REST_OK, rc);

  rc = win32_c_rest_mutex_unlock(0);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);
  rc = win32_c_rest_mutex_unlock(mtx);
  ASSERT_EQ(C_REST_OK, rc);

  rc = win32_c_rest_mutex_destroy(0);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);
  rc = win32_c_rest_mutex_destroy(mtx);
  ASSERT_EQ(C_REST_OK, rc);

  PASS();
}

TEST test_win32_cond_ops(void) {
  c_rest_error_t rc;
  c_rest_cond_t cond = 0;
  c_rest_mutex_t mtx = 0;

  rc = win32_c_rest_mutex_create(&mtx);
  ASSERT_EQ(C_REST_OK, rc);

  rc = win32_c_rest_cond_create(NULL);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);

  g_crf_malloc_hook = mock_fail_malloc;
  rc = win32_c_rest_cond_create(&cond);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);
  g_crf_malloc_hook = NULL;

  g_event_create_count = 0;
  rc = win32_c_rest_cond_create(&cond);
  ASSERT_EQ(C_REST_OK, rc);

  rc = win32_c_rest_cond_wait(0, mtx);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);
  rc = win32_c_rest_cond_wait(cond, 0);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);

  /* have_waiters == 0 branch */
  rc = win32_c_rest_cond_signal(0);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);
  rc = win32_c_rest_cond_signal(cond);
  ASSERT_EQ(C_REST_OK, rc);

  /* Set waiters_count > 0 to cover have_waiters > 0 branch */
  {
    struct cond_impl *ci = (struct cond_impl *)cond;
    ci->waiters_count = 2;
    rc = win32_c_rest_cond_signal(cond);
    ASSERT_EQ(C_REST_OK, rc);

    /* Wait with have_waiters = 1 and have_waiters = 0 in wait cleanup */
    ci->waiters_count = 1;
    rc = win32_c_rest_cond_wait(cond, mtx);
    ASSERT_EQ(C_REST_OK, rc);
  }

  rc = win32_c_rest_cond_destroy(0);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);
  rc = win32_c_rest_cond_destroy(cond);
  ASSERT_EQ(C_REST_OK, rc);

  rc = win32_c_rest_mutex_destroy(mtx);
  ASSERT_EQ(C_REST_OK, rc);

  PASS();
}

TEST test_win32_process_and_misc(void) {
  c_rest_error_t rc;
  c_rest_process_t proc = 0;
  int exit_code = -1;
  unsigned long ms = 0;
  char rnd[16];
  int last_err = 0;
  char *const argv[] = {"app.exe", "--test", "arg2", NULL};

  rc = win32_c_rest_process_create(NULL, "app.exe", argv);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);
  rc = win32_c_rest_process_create(&proc, NULL, argv);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);

  g_mock_fork_fail = 1;
  rc = win32_c_rest_process_create(&proc, "app.exe", argv);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);
  g_mock_fork_fail = 0;

  g_create_proc_fail = 1;
  rc = win32_c_rest_process_create(&proc, "app.exe", argv);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);
  g_create_proc_fail = 0;

  /* SearchPath fail branch */
  g_search_path_fail = 1;
  rc = win32_c_rest_process_create(&proc, "app.exe", argv);
  ASSERT_EQ(C_REST_OK, rc);
  g_search_path_fail = 0;

  /* SearchPath success branch, argv = NULL */
  rc = win32_c_rest_process_create(&proc, "app.exe", NULL);
  ASSERT_EQ(C_REST_OK, rc);

  /* Multi-argument argv */
  {
    char *const argv_multi[] = {"app.exe", "a1", "a2", NULL};
    rc = win32_c_rest_process_create(&proc, "app.exe", argv_multi);
    ASSERT_EQ(C_REST_OK, rc);
  }

  /* Long argument to trigger cmdline overflow branch */
  {
    char long_arg[1100];
    char *argv_long[3];
    memset(long_arg, 'x', sizeof(long_arg) - 1);
    long_arg[sizeof(long_arg) - 1] = '\0';
    argv_long[0] = "app.exe";
    argv_long[1] = long_arg;
    argv_long[2] = NULL;
    rc = win32_c_rest_process_create(&proc, "app.exe", argv_long);
    ASSERT_EQ(C_REST_OK, rc);
  }

  rc = win32_c_rest_process_wait(0, &exit_code);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);
  rc = win32_c_rest_process_wait((c_rest_process_t)(size_t)INVALID_HANDLE_VALUE,
                                 &exit_code);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);

  g_wait_fail = 1;
  rc = win32_c_rest_process_wait(proc, &exit_code);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);
  g_wait_fail = 0;

  g_exit_code_fail = 1;
  rc = win32_c_rest_process_wait(proc, &exit_code);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);
  g_exit_code_fail = 0;

  /* NULL exit_code */
  rc = win32_c_rest_process_wait(proc, NULL);
  ASSERT_EQ(C_REST_OK, rc);

  /* Normal exit_code */
  rc = win32_c_rest_process_wait(proc, &exit_code);
  ASSERT_EQ(C_REST_OK, rc);
  ASSERT_EQ(0, exit_code);

  rc = win32_c_rest_timer_get_ms(NULL);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);
  rc = win32_c_rest_timer_get_ms(&ms);
  ASSERT_EQ(C_REST_OK, rc);
  ASSERT_EQ(1234567UL, ms);

  rc = win32_c_rest_random_get(NULL, sizeof(rnd));
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);
  rc = win32_c_rest_random_get(rnd, 0);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);
  rc = win32_c_rest_random_get(rnd, sizeof(rnd));
  ASSERT_EQ(C_REST_OK, rc);

  rc = win32_c_rest_get_last_error(NULL);
  ASSERT_EQ(C_REST_ERROR_GENERIC, rc);
  rc = win32_c_rest_get_last_error(&last_err);
  ASSERT_EQ(C_REST_OK, rc);
  ASSERT_EQ(42, last_err);

  PASS();
}

SUITE_EXTERN(platform_win32_suite);
SUITE(platform_win32_suite) {
  RUN_TEST(test_win32_lifecycle);
  RUN_TEST(test_win32_socket_ops);
  RUN_TEST(test_win32_threading_and_mutex);
  RUN_TEST(test_win32_cond_ops);
  RUN_TEST(test_win32_process_and_misc);
}
