/* Copyright (C) 2002,2007 by  Red Hat, Incorporated. All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * is freely granted, provided that this notice is preserved.
 */

#include "fdlibm.h"
#include <stdbool.h>

int
__fpclassifyf (float x)
{
  __uint32_t w;

  GET_FLOAT_WORD(w,x);
  
  if (w == 0x00000000 || w == 0x80000000)
    return FP_ZERO;
  else if ((w >= 0x00800000 && w <= 0x7f7fffff) ||
           (w >= 0x80800000 && w <= 0xff7fffff))
    return FP_NORMAL;
  else if ((w >= 0x00000001 && w <= 0x007fffff) ||
           (w >= 0x80000001 && w <= 0x807fffff))
    return FP_SUBNORMAL;
  else if (w == 0x7f800000 || w == 0xff800000)
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

bool _llvm_is_fpclass_f(float x, int mask)
{
    __uint32_t w;
    GET_FLOAT_WORD(w, x);

    return
        ((mask & __FPCLASS_SNAN) &&
         ((w & 0x7f800000u) == 0x7f800000u) &&   
         ((w & 0x007fffffu) != 0) &&            
         ((w & 0x00400000u) == 0))              
     || ((mask & __FPCLASS_QNAN) &&
         ((w & 0x7f800000u) == 0x7f800000u) &&  
         ((w & 0x007fffffu) != 0) &&            
         ((w & 0x00400000u) != 0))              
     || ((mask & __FPCLASS_NEGINF) &&
         (w == 0xff800000u))
     || ((mask & __FPCLASS_NEGNORMAL) &&
         (w >= 0x80800000u && w <= 0xff7fffffu))
     || ((mask & __FPCLASS_NEGSUBNORMAL) &&
         (w >= 0x80000001u && w <= 0x807fffffu))
     || ((mask & __FPCLASS_NEGZERO) &&
         (w == 0x80000000u))
     || ((mask & __FPCLASS_POSZERO) &&
         (w == 0x00000000u))
     || ((mask & __FPCLASS_POSSUBNORMAL) &&
         (w >= 0x00000001u && w <= 0x007fffffu))
     || ((mask & __FPCLASS_POSNORMAL) &&
         (w >= 0x00800000u && w <= 0x7f7fffffu))
     || ((mask & __FPCLASS_POSINF) &&
         (w == 0x7f800000u));
}
