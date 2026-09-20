#ifndef MOCK_WINSOCK2_H
#define MOCK_WINSOCK2_H

/* clang-format off */
#include <stddef.h>
#include <string.h>
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long SOCKET;
typedef void *HANDLE;
typedef unsigned long DWORD;
typedef unsigned short WORD;
typedef unsigned long u_long;

#define INVALID_SOCKET ((SOCKET)(~0UL))
#define INVALID_HANDLE_VALUE ((HANDLE)(size_t)(~0UL))
#define SOCKET_ERROR (-1)
#define FALSE 0
#define TRUE 1
#define AF_INET 2
#define SOCK_STREAM 1
#define IPPROTO_TCP 6
#define FIONBIO 0x8004667eL
#define INADDR_ANY 0x00000000UL
#define INADDR_NONE ((unsigned long)0xFFFFFFFFUL)
#define INFINITE 0xFFFFFFFFUL
#define WAIT_OBJECT_0 0UL
#define WAIT_TIMEOUT 258UL
#define MAX_PATH 260

#define NO_ERROR 0L
#define MAKEWORD(a, b)                                                         \
  ((WORD)(((unsigned char)(a)) | (((WORD)((unsigned char)(b))) << 8)))

typedef struct WSAData {
  WORD wVersion;
  WORD wHighVersion;
  char szDescription[257];
  char szSystemStatus[129];
  unsigned short iMaxSockets;
  unsigned short iMaxUdpDg;
  char *lpVendorInfo;
} WSADATA;

struct in_addr {
  unsigned long s_addr;
};

struct sockaddr_in {
  short sin_family;
  unsigned short sin_port;
  struct in_addr sin_addr;
  char sin_zero[8];
};

struct sockaddr {
  unsigned short sa_family;
  char sa_data[14];
};

typedef struct _CRITICAL_SECTION {
  void *DebugInfo;
  long LockCount;
  long RecursionCount;
  HANDLE OwningThread;
  HANDLE LockSemaphore;
  unsigned long SpinCount;
} CRITICAL_SECTION;

typedef struct _STARTUPINFOA {
  DWORD cb;
  char *lpReserved;
  char *lpDesktop;
  char *lpTitle;
  DWORD dwX;
  DWORD dwY;
  DWORD dwXSize;
  DWORD dwYSize;
  DWORD dwXCountChars;
  DWORD dwYCountChars;
  DWORD dwFillAttribute;
  DWORD dwFlags;
  WORD wShowWindow;
  WORD cbReserved2;
  unsigned char *lpReserved2;
  HANDLE hStdInput;
  HANDLE hStdOutput;
  HANDLE hStdError;
} STARTUPINFOA;

typedef struct _PROCESS_INFORMATION {
  HANDLE hProcess;
  HANDLE hThread;
  DWORD dwProcessId;
  DWORD dwThreadId;
} PROCESS_INFORMATION;

#ifndef __stdcall
#define __stdcall
#endif

int WSAStartup(WORD wVersionRequired, WSADATA *lpWSAData);
int WSACleanup(void);
SOCKET socket(int af, int type, int protocol);
int ioctlsocket(SOCKET s, long cmd, u_long *argp);
int bind(SOCKET s, const struct sockaddr *name, int namelen);
int listen(SOCKET s, int backlog);
SOCKET accept(SOCKET s, struct sockaddr *addr, int *addrlen);
int closesocket(SOCKET s);
int recv(SOCKET s, char *buf, int len, int flags);
int send(SOCKET s, const char *buf, int len, int flags);
unsigned short htons(unsigned short hostshort);
unsigned long inet_addr(const char *cp);

void InitializeCriticalSection(CRITICAL_SECTION *lpCriticalSection);
void EnterCriticalSection(CRITICAL_SECTION *lpCriticalSection);
void LeaveCriticalSection(CRITICAL_SECTION *lpCriticalSection);
void DeleteCriticalSection(CRITICAL_SECTION *lpCriticalSection);

HANDLE CreateEventA(void *lpEventAttributes, int bManualReset,
                    int bInitialState, const char *lpName);
int SetEvent(HANDLE hEvent);
int ResetEvent(HANDLE hEvent);
int PulseEvent(HANDLE hEvent);
DWORD WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds);
DWORD SignalObjectAndWait(HANDLE hObjectToSignal, HANDLE hObjectToWaitOn,
                          DWORD dwMilliseconds, int bAlertable);
int CloseHandle(HANDLE hObject);

DWORD SearchPathA(const char *lpPath, const char *lpFileName,
                  const char *lpExtension, DWORD nBufferLength, char *lpBuffer,
                  char **lpFilePart);
int CreateProcessA(const char *lpApplicationName, char *lpCommandLine,
                   void *lpProcessAttributes, void *lpThreadAttributes,
                   int bInheritHandles, DWORD dwCreationFlags,
                   void *lpEnvironment, const char *lpCurrentDirectory,
                   STARTUPINFOA *lpStartupInfo,
                   PROCESS_INFORMATION *lpProcessInformation);
int GetExitCodeProcess(HANDLE hProcess, DWORD *lpExitCode);
DWORD GetTickCount(void);
DWORD GetLastError(void);

#ifdef __cplusplus
}
#endif

#endif /* MOCK_WINSOCK2_H */
