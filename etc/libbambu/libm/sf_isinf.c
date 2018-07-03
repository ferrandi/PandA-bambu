/** 
 * Porting of the libm library to the PandA framework 
 * starting from the original FDLIBM 5.3 (Freely Distributable LIBM) developed by SUN
 * plus the newlib version 1.19 from RedHat and plus uClibc version 0.9.32.1 developed by Erik Andersen.
 * The author of this port is Fabrizio Ferrandi from Politecnico di Milano.
 * The porting fall under the LGPL v2.1, see the files COPYING.LIB and COPYING.LIBM_PANDA in this directory.
 * Date: September, 11 2013.
*/
/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

/*
 * isinff(x) returns 1 if x is inf, -1 if x is -inf, else 0;
 * no branching!
 */

#include "math_privatef.h"

int __isinff(float x)
{
    int ix,t;
    GET_FLOAT_WORD(ix,x);
    t = ix & 0x7fffffff;
    t ^= 0x7f800000;
    t |= -t;
    return ~(t >> 31) & (ix >> 30);
}


int isinff(float x)
{
   return __isinff(x);
}

#ifdef _DOUBLE_IS_32BITS

int isinf(double x)
{
    return isinff((float) x);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
