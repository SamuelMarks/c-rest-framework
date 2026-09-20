#ifndef MOCK_PROCESS_H
#define MOCK_PROCESS_H

/* clang-format off */
#include "winsock2.h"
#include <stdint.h>
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

uintptr_t _beginthreadex(void *security, unsigned stack_size,
                         unsigned(__stdcall *start_address)(void *),
                         void *arglist, unsigned initflag, unsigned *thrdaddr);

#ifdef __cplusplus
}
#endif

#endif /* MOCK_PROCESS_H */
