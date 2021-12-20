// This little program runs the macros in the SLJIT header file
// which auto-detect architecture. This enables us to only attempt
// to use SLJIT on architectures that support it
//
#define SLJIT_CONFIG_AUTO 1
#include "../sljit/sljitConfigInternal.h"

#ifndef SLJIT_CONFIG_UNSUPPORTED
  #warning JIT enabled
#else
  #error JIT not supported
#endif

int main(void)
{
  return 0;
}
