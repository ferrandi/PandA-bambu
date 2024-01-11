/**
 * umul64 primitive adapted to the PandA infrastructure by Fabrizio Ferrandi from Politecnico di Milano.
 * November, 10 2019.
 *
 */
/* Public domain.  */

typedef unsigned int __USItype __attribute__((mode(SI)));
typedef unsigned long long int __UDItype __attribute__((mode(DI)));
typedef int __SItype __attribute__((mode(SI)));
typedef long long int __DItype __attribute__((mode(DI)));

static __UDItype __umul3264(__USItype u, __USItype v)
{
   __USItype u0v0, u1v0, u0v1, u1v1, u0, u1, v0, v1;
   __UDItype t1, t2;

   u0 = u & 0xFFFF;
   u1 = u >> 16;
   v0 = v & 0xFFFF;
   v1 = v >> 16;
   u0v0 = u0 * v0;
   u1v0 = u1 * v0;
   u0v1 = u0 * v1;
   u1v1 = u1 * v1;

   t1 = (((__UDItype)u1v0) << 16) + u0v0;
   t2 = (((__UDItype)u1v1) << 16) + u0v1;
   return t1 + (t2 << 16);
}

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
   t = __umul3264(u0, v0);
   w0 = t;
   k = t >> 32;
   tlast = u1 * v0 + k;
   w1 = tlast;
   tlast = u0 * v1 + w1;
   return (((__UDItype)tlast) << 32) | ((__UDItype)w0);
}

__DItype __mul64(__DItype _u, __DItype _v)
{
   __UDItype t;
   __UDItype u, v;
   __USItype u0, u1, v0, v1, k;
   __USItype w0, w1;
   __USItype tlast;
   u = ((__UDItype)_u);
   v = ((__UDItype)_v);
   u1 = u >> 32;
   u0 = u;
   v1 = v >> 32;
   v0 = v;
   t = __umul3264(u0, v0);
   w0 = t;
   k = t >> 32;
   tlast = u1 * v0 + k;
   w1 = tlast;
   tlast = u0 * v1 + w1;
   return (__DItype)((((__UDItype)tlast) << 32) | ((__UDItype)w0));
}
