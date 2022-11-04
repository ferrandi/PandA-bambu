/**
 * umul64 primitive adapted to the PandA infrastructure by Fabrizio Ferrandi from Politecnico di Milano.
 * November, 10 2019.
 *
 */
/* Public domain.  */
#include <stdlib.h>

typedef unsigned int __USItype __attribute__((mode(SI)));
typedef unsigned int __UDItype __attribute__((mode(DI)));
typedef int __SItype __attribute__((mode(SI)));
typedef int __DItype __attribute__((mode(DI)));

__UDItype __umul64(__UDItype u, __UDItype v)
{
   __UDItype t;
   __USItype u0, u1, v0, v1, k;
   __USItype w0, w1;
   __USItype tlast;
   u1 = u >> 32;
   u0 = u;
   v1 = v >> 32;
   v0 = v;
   t = (__UDItype)u0 * v0;
   w0 = t;
   k = t >> 32;
   tlast = u1 * v0 + k;
   w1 = tlast;
   tlast = u0 * v1 + w1;
   return (((__UDItype)tlast) << 32) | ((__UDItype)w0);
}

__DItype __mul64(__DItype _u, __DItype _v)
{
   __UDItype u, v, t;
   __USItype u0, u1, v0, v1, k;
   __USItype w0, w1;
   __USItype tlast;
   u = ((__UDItype)_u);
   v = ((__UDItype)_v);
   u1 = u >> 32;
   u0 = u;
   v1 = v >> 32;
   v0 = v;
   t = (__UDItype)u0 * v0;
   w0 = t;
   k = t >> 32;
   tlast = u1 * v0 + k;
   w1 = tlast;
   tlast = u0 * v1 + w1;
   return (__DItype)((((__UDItype)tlast) << 32) | ((__UDItype)w0));
}
