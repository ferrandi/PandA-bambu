/**
 * umul32 primitive adapted to the PandA infrastructure by Fabrizio Ferrandi from Politecnico di Milano.
 * January, 4 2024.
 *
 */
/* Public domain.  */

typedef unsigned int __UHItype __attribute__((mode(HI)));
typedef unsigned int __USItype __attribute__((mode(SI)));
typedef int __HItype __attribute__((mode(HI)));
typedef int __SItype __attribute__((mode(SI)));

__USItype __umul32(__USItype u, __USItype v)
{
   __USItype t;
   __UHItype u0, u1, v0, v1, k;
   __UHItype w0, w1;
   __UHItype tlast;
   u1 = u >> 16;
   u0 = u;
   v1 = v >> 16;
   v0 = v;
   t = ((__USItype)u0) * ((__USItype)v0);
   w0 = t;
   k = t >> 16;
   tlast = u1 * v0 + k;
   w1 = tlast;
   tlast = u0 * v1 + w1;
   return (((__USItype)tlast) << 16) | ((__USItype)w0);
}

__SItype __mul32(__SItype _u, __SItype _v)
{
   __USItype u, v, t;
   __UHItype u0, u1, v0, v1, k;
   __UHItype w0, w1;
   __UHItype tlast;
   u = ((__USItype)_u);
   v = ((__USItype)_v);
   u1 = u >> 16;
   u0 = u;
   v1 = v >> 16;
   v0 = v;
   t = ((__USItype)u0) * ((__USItype)v0);
   w0 = t;
   k = t >> 16;
   tlast = u1 * v0 + k;
   w1 = tlast;
   tlast = u0 * v1 + w1;
   return (__SItype)((((__USItype)tlast) << 16) | ((__USItype)w0));
}
