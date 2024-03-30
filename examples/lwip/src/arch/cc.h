#ifndef __AGM_CC_H__
#define __AGM_CC_H__

#include "alta.h"

#define LWIP_PLATFORM_DIAG(...) printf __VA_ARGS__
#define LWIP_PLATFORM_ASSERT(x) do {printf("Assertion \"%s\" failed at line %d in %s\n", \
                                     x, __LINE__, __FILE__); asm("ebreak"); } while(0)

typedef uint32_t sys_prot_t;

#endif
