#ifndef MOCK_PROCESS_H
#define MOCK_PROCESS_H

/* clang-format off */
#include "winsock2.h"
#if (!defined(_MSC_VER) || _MSC_VER >= 1600) && !defined(__WATCOMC__)
#include <stdint.h>
#endif
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

#if (defined(_MSC_VER) && _MSC_VER < 1600) || defined(__WATCOMC__)
#if defined(_WIN64)
typedef unsigned __int64 crf_mock_uintptr_t;
#else
typedef unsigned long crf_mock_uintptr_t;
#endif
#else
typedef uintptr_t crf_mock_uintptr_t;
#endif

crf_mock_uintptr_t _beginthreadex(void *security, unsigned stack_size,
                                  unsigned(__stdcall *start_address)(void *),
                                  void *arglist, unsigned initflag,
                                  unsigned *thrdaddr);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MOCK_PROCESS_H */
