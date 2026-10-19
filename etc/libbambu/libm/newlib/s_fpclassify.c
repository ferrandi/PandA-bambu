/* Copyright (C) 2002, 2007 by  Red Hat, Incorporated. All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * is freely granted, provided that this notice is preserved.
 */

#include "fdlibm.h"
#include <stdbool.h>

int
__fpclassifyd (double x)
{
  __uint32_t msw, lsw;

  EXTRACT_WORDS(msw,lsw,x);

  if ((msw == 0x00000000 && lsw == 0x00000000) ||
      (msw == 0x80000000 && lsw == 0x00000000))
    return FP_ZERO;
  else if ((msw >= 0x00100000 && msw <= 0x7fefffff) ||
           (msw >= 0x80100000 && msw <= 0xffefffff))
    return FP_NORMAL;
  else if ((msw >= 0x00000000 && msw <= 0x000fffff) ||
           (msw >= 0x80000000 && msw <= 0x800fffff))
    /* zero is already handled above */
    return FP_SUBNORMAL;
  else if ((msw == 0x7ff00000 && lsw == 0x00000000) ||
           (msw == 0xfff00000 && lsw == 0x00000000))
    return FP_INFINITE;
  else
    return FP_NAN;
}

// Signaling NaN
#define __FPCLASS_SNAN 0x0001
// Quiet NaN
#define __FPCLASS_QNAN 0x0002 
// Negative infinity   
#define __FPCLASS_NEGINF 0x0004
// Negative normal
#define      __FPCLASS_NEGNORMAL 0x0008
// Negative subnormal
#define __FPCLASS_NEGSUBNORMAL 0x0010
// Negative zero
#define __FPCLASS_NEGZERO 0x0020
// Positive zero
#define __FPCLASS_POSZERO 0x0040
// Positive subnormal
#define __FPCLASS_POSSUBNORMAL 0x0080
// Positive normal
#define __FPCLASS_POSNORMAL 0x0100
// Positive infinity
#define __FPCLASS_POSINF 0x0200

bool _llvm_is_fpclass_d(double x, int mask)
{
  __uint32_t msw, lsw;
  __uint32_t abs_msw, frac_hi;

  EXTRACT_WORDS(msw, lsw, x);

  abs_msw = msw & 0x7fffffff;
  frac_hi = msw & 0x000fffff;

  return
     ((mask & __FPCLASS_SNAN) &&
      ((msw & 0x7ff00000) == 0x7ff00000) &&
      ((frac_hi != 0) || (lsw != 0)) &&
      ((msw & 0x00080000) == 0))
  || ((mask & __FPCLASS_QNAN) &&
      ((msw & 0x7ff00000) == 0x7ff00000) &&
      ((frac_hi != 0) || (lsw != 0)) &&
      ((msw & 0x00080000) != 0))
  || ((mask & __FPCLASS_NEGINF) &&
      (msw == 0xfff00000 && lsw == 0))
  || ((mask & __FPCLASS_NEGNORMAL) &&
      (msw >= 0x80100000 && msw <= 0xffefffff))
  || ((mask & __FPCLASS_NEGSUBNORMAL) &&
      ((msw & 0x80000000) != 0) &&
      ((abs_msw & 0x7ff00000) == 0) &&
      ((frac_hi != 0) || (lsw != 0)))
  || ((mask & __FPCLASS_NEGZERO) &&
      (msw == 0x80000000 && lsw == 0))
  || ((mask & __FPCLASS_POSZERO) &&
      (msw == 0x00000000 && lsw == 0))
  || ((mask & __FPCLASS_POSSUBNORMAL) &&
      ((msw & 0x80000000) == 0) &&
      ((msw & 0x7ff00000) == 0) &&
      ((frac_hi != 0) || (lsw != 0)))
  || ((mask & __FPCLASS_POSNORMAL) &&
      (msw >= 0x00100000 && msw <= 0x7fefffff))
  || ((mask & __FPCLASS_POSINF) &&
      (msw == 0x7ff00000 && lsw == 0));
}
