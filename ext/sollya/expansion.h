/*
 * This file contains parts of the CR-libm library. It is not
 * necessary for the compilation of Sollya.
 *
 * Author  : David Defour, Catherine Daramy, Florent de Dinechin, Christoph Lauter
 *
 * Contact : David.Defour@ens-lyon.fr, catherine_daramy@ens-lyon.fr,
 * florent.de.dinechin@ens-lyon.fr, christoph.lauter@ens-lyon.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or 
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  
 *
 * 
 */

#ifndef EXPANSION_H
#define EXPANSION_H

#include <mpfr.h>

void mpfr_to_double(double *dh, mpfr_t op);
void mpfr_to_doubledouble(double *dh, double *dm, mpfr_t op);
void mpfr_to_tripledouble(double *dh, double *dm, double *dl, mpfr_t op);
void double_to_mpfr(mpfr_t rop, double dh);
void doubledouble_to_mpfr(mpfr_t rop, double dh, double dm);
void tripledouble_to_mpfr(mpfr_t rop, double dh, double dm, double dl);
void f(mpfr_t y, mpfr_t xMpfr);

#if defined(D_TO_D)
void p(double *p_resh, double x);
#elif defined(D_TO_DD)
void p(double *p_resh, double *p_resm, double x);
#elif defined(D_TO_TD)
void p(double *p_resh, double *p_resm, double *p_resl, double x);
#elif defined(DD_TO_DD)
void p(double *p_resh, double *p_resm, double xh, double xm);
#elif defined(DD_TO_TD) 
void p(double *p_resh, double *p_resm, double *p_resl, double xh, double xm);
#elif defined(TD_TO_TD)
void p(double *p_resh, double *p_resm, double *p_resl, double xh, double xm, double xl);
#endif



#define Add12(s, r, a, b)          \
        {double _z, _a=a, _b=b;    \
         s = _a + _b;              \
         _z = s - _a;              \
         r = _b - _z;   }            


#define Add12Cond(s, r, a, b)      \
{                                  \
    double _u1, _u2, _u3, _u4;     \
    double  _a=a, _b=b;            \
                                   \
    s = _a + _b;                   \
    _u1 = s - _a;                  \
    _u2 = s - _u1;                 \
    _u3 = _b - _u1;                \
    _u4 = _a - _u2;                \
    r = _u4 + _u3;                 \
}

#define Add22(zh,zl,xh,xl,yh,yl)         \
do {                                     \
double _r,_s;                            \
_r = (xh)+(yh);                          \
_s = ((((xh)-_r) +(yh)) + (yl)) + (xl);  \
*zh = _r+_s;                             \
*zl = (_r - (*zh)) + _s;                 \
} while(0)


#define Add22Cond(zh,zl,xh,xl,yh,yl)                                                   \
do {                                                                                   \
  double _v1, _v2, _v3, _v4;                                                           \
                                                                                       \
  Add12Cond(_v1, _v2, (xh), (yh));                                                     \
  _v3 = (xl) + (yl);                                                                   \
  _v4 = _v2 + _v3;                                                                     \
  Add12((*(zh)),(*(zl)),_v1,_v4);                                                      \
} while (2+2==5) 


#define Mul12(rh,rl,u,v)                        \
{                                               \
  const double c  = 134217729.; /* 2^27 +1 */   \
  double up, u1, u2, vp, v1, v2;                \
  double _u =u, _v=v;                           \
                                                \
  up = _u*c;        vp = _v*c;                  \
  u1 = (_u-up)+up;  v1 = (_v-vp)+vp;            \
  u2 = _u-u1;       v2 = _v-v1;                 \
                                                \
  *rh = _u*_v;                                  \
  *rl = (((u1*v1-*rh)+(u1*v2))+(u2*v1))+(u2*v2);\
}


#define Mul22(zh,zl,xh,xl,yh,yl)                      \
{                                                     \
double mh, ml;                                        \
						      \
  const double c = 134217729.;			      \
  double up, u1, u2, vp, v1, v2;		      \
						      \
  up = (xh)*c;        vp = (yh)*c;		      \
  u1 = ((xh)-up)+up;  v1 = ((yh)-vp)+vp;	      \
  u2 = (xh)-u1;       v2 = (yh)-v1;                   \
  						      \
  mh = (xh)*(yh);				      \
  ml = (((u1*v1-mh)+(u1*v2))+(u2*v1))+(u2*v2);	      \
						      \
  ml += (xh)*(yl) + (xl)*(yh);			      \
  *zh = mh+ml;					      \
  *zl = mh - (*zh) + ml;                              \
}


/* Additional double-double operators */

/* Eps Mul122 <= 2^-102 */
#define Mul122(resh,resl,a,bh,bl)                 \
{                                                 \
    double _t1, _t2, _t3, _t4;                    \
                                                  \
    Mul12(&_t1,&_t2,(a),(bh));                    \
    _t3 = (a) * (bl);                             \
    _t4 = _t2 + _t3;                              \
    Add12((*(resh)),(*(resl)),_t1,_t4);           \
}

/* Eps MulAdd212 <= 2^-100 for |a * (bh + bl)| <= 1/4 * |ch + cl| */
#define MulAdd212(resh,resl,ch,cl,a,bh,bl)           \
{                                                    \
    double _t1, _t2, _t3, _t4, _t5, _t6, _t7, _t8;   \
                                                     \
    Mul12(&_t1,&_t2,(a),(bh));                       \
    Add12(_t3,_t4,(ch),_t1);                         \
    _t5 = (bl) * (a);                                \
    _t6 = (cl) + _t2;                                \
    _t7 = _t5 + _t6;                                 \
    _t8 = _t7 + _t4;                                 \
    Add12((*(resh)),(*(resl)),_t3,_t8);              \
}

/* Eps MulAdd212 <= 2^-100 
   for |(ah + bh) * (bh + bl)| <= 1/4 * |ch + cl| 
*/
#define MulAdd22(resh,resl,ch,cl,ah,al,bh,bl)        \
{                                                    \
    double _t1, _t2, _t3, _t4, _t5, _t6, _t7, _t8;   \
    double _t9, _t10;                                \
                                                     \
    Mul12(&_t1,&_t2,(ah),(bh));                      \
    Add12(_t3,_t4,(ch),_t1);                         \
    _t5 = (ah) * (bl);                               \
    _t6 = (al) * (bh);                               \
    _t7 = _t2 + (cl);                                \
    _t8 = _t4 + _t7;                                 \
    _t9 = _t5 + _t6;                                 \
    _t10 = _t8 + _t9;                                \
    Add12((*(resh)),(*(resl)),_t3,_t10);             \
}

#define Add122(resh,resl,a,bh,bl)                    \
{                                                    \
    double _t1, _t2, _t3;                            \
                                                     \
    Add12(_t1,_t2,(a),(bh));                         \
    _t3 = _t2 + (bl);                                \
    Add12((*(resh)),(*(resl)),_t1,_t3);              \
}    


#define Add122Cond(resh,resl,a,bh,bl)                \
{                                                    \
    double _t1, _t2, _t3;                            \
                                                     \
    Add12Cond(_t1,_t2,(a),(bh));                     \
    _t3 = _t2 + (bl);                                \
    Add12((*(resh)),(*(resl)),_t1,_t3);              \
}    


#define Add212(resh,resl,ah,al,b)                    \
{                                                    \
    double _t1, _t2, _t3;                            \
                                                     \
    Add12(_t1,_t2,(ah),b);                           \
    _t3 = _t2 + (al);                                \
    Add12((*(resh)),(*(resl)),_t1,_t3);              \
}


#define  Div22(pzh,pzl,xh,xl,yh,yl)  {           \
  double _ch,_cl,_uh,_ul;                        \
  _ch=(xh)/(yh);   Mul12(&_uh,&_ul,_ch,(yh));    \
  _cl=((xh)-_uh);                                \
  _cl -= _ul;                                    \
  _cl += (xl);                                   \
  _cl -= _ch*(yl);                               \
  _cl /= (yh);                                   \
  *pzh=_ch+_cl;   *pzl=(_ch-(*pzh))+_cl;         \
}

/* 
   Coefficients for 1/sqrt(m) with 1/2 < m < 2
   The corresponding relative polynomial approximation error is less than
   eps < 2^(-8.3127) (cf. Maple file)
   The Itanium instruction frsqrta is slightly more accurate; it can
   therefore easily replace the polynomial evaluation.
*/
   
#define SQRTPOLYC0 2.50385236695888790947606139525305479764938354492188e+00   
#define SQRTPOLYC1 -3.29763389114324168005509818613063544034957885742188e+00  
#define SQRTPOLYC2 2.75726076139124520736345402838196605443954467773438e+00   
#define SQRTPOLYC3 -1.15233725777933848632983426796272397041320800781250e+00  
#define SQRTPOLYC4 1.86900066679800969104974228685023263096809387207031e-01   
#define SQRTTWO52 4.50359962737049600000000000000000000000000000000000e+15


#define  sqrt12(resh, resl, x)  {                                                            \
  db_number _xdb;                                                                            \
  int _E;                                                                                    \
  double _m, _r0, _r1, _r2, _r3h, _r3l, _r4h, _r4l, _srtmh, _srtml;                          \
  double _r2PHr2h, _r2PHr2l, _r2Sqh, _r2Sql;                                                 \
  double _mMr2h, _mMr2l, _mMr2Ch, _mMr2Cl;                                                   \
  double _MHmMr2Ch, _MHmMr2Cl;                                                               \
  double _r3Sqh, _r3Sql, _mMr3Sqh, _mMr3Sql;                                                 \
                                                                                             \
  /* Special case x = 0 */                                                                   \
  if ((x) == 0) {                                                                            \
    (*(resh)) = (x);                                                                         \
    (*(resl)) = 0;                                                                           \
  } else {                                                                                   \
                                                                                             \
    _E = 0;                                                                                  \
                                                                                             \
    /* Convert to integer format */                                                          \
    _xdb.d = (x);                                                                            \
                                                                                             \
    /* Handle subnormal case */                                                              \
    if (_xdb.i[HI] < 0x00100000) {                                                           \
      _E = -52;                                                                              \
      _xdb.d *= ((db_number) ((double) SQRTTWO52)).d; 	                                     \
                      /* make x a normal number */                                           \
    }                                                                                        \
                                                                                             \
    /* Extract exponent E and mantissa m */                                                  \
    _E += (_xdb.i[HI]>>20)-1023;                                                             \
    _xdb.i[HI] = (_xdb.i[HI] & 0x000fffff) | 0x3ff00000;                                     \
    _m = _xdb.d;                                                                             \
                                                                                             \
    /* Make exponent even */                                                                 \
    if (_E & 0x00000001) {                                                                   \
      _E++;                                                                                  \
      _m *= 0.5;    /* Suppose now 1/2 <= m <= 2 */                                          \
    }                                                                                        \
                                                                                             \
    /* Construct sqrt(2^E) = 2^(E/2) */                                                      \
    _xdb.i[HI] = (_E/2 + 1023) << 20;                                                        \
    _xdb.i[LO] = 0;                                                                          \
                                                                                             \
    /* Compute initial approximation to r = 1/sqrt(m) */                                     \
                                                                                             \
    _r0 = SQRTPOLYC0 +                                                                       \
         _m * (SQRTPOLYC1 + _m * (SQRTPOLYC2 + _m * (SQRTPOLYC3 + _m * SQRTPOLYC4)));        \
                                                                                             \
    /* Iterate two times on double precision */                                              \
                                                                                             \
    _r1 = 0.5 * _r0 * (3 - _m * (_r0 * _r0));                                                \
    _r2 = 0.5 * _r1 * (3 - _m * (_r1 * _r1));                                                \
                                                                                             \
    /* Iterate two times on double-double precision */                                       \
                                                                                             \
    Mul12(&_r2Sqh, &_r2Sql, _r2, _r2);                                                       \
    Add12(_r2PHr2h, _r2PHr2l, _r2, (0.5 * _r2));                                             \
    Mul12(&_mMr2h, &_mMr2l, _m, _r2);                                                        \
    Mul22(&_mMr2Ch, &_mMr2Cl, _mMr2h, _mMr2l, _r2Sqh, _r2Sql);                               \
                                                                                             \
    _MHmMr2Ch = -0.5 * _mMr2Ch;                                                              \
    _MHmMr2Cl = -0.5 * _mMr2Cl;                                                              \
                                                                                             \
    Add22(&_r3h, &_r3l, _r2PHr2h, _r2PHr2l, _MHmMr2Ch, _MHmMr2Cl);                           \
                                                                                             \
    Mul22(&_r3Sqh, &_r3Sql, _r3h, _r3l, _r3h, _r3l);                                         \
    Mul22(&_mMr3Sqh, &_mMr3Sql, _m, 0, _r3Sqh, _r3Sql);                                      \
    /* To prove: mMr3Sqh = 1.0 in each case */                                               \
                                                                                             \
    Mul22(&_r4h, &_r4l, _r3h, _r3l, 1, (-0.5 * _mMr3Sql));                                   \
                                                                                             \
    /* Multiply obtained reciprocal square root by m */                                      \
                                                                                             \
    Mul22(&_srtmh,&_srtml,_m,0,_r4h,_r4l);                                                   \
                                                                                             \
    /* Multiply componentwise by sqrt(2^E) */                                                \
    /* which is an integer power of 2 that may not produce a subnormal */                    \
                                                                                             \
    (*(resh)) = _xdb.d * _srtmh;                                                             \
    (*(resl)) = _xdb.d * _srtml;                                                             \
                                                                                             \
  } /* End: special case 0 */                                                                \
}


/* Renormalize3

   Procedure for renormalizing a triple double number, i.e.
   computing exactly an equivalent sum of three non-overlapping
   double numbers


   Arguments:       a triple double number ah, am, al
   
   Results:         a triple double number resh, resm, resl

   Preconditions:   abs(ah) > abs(am) > abs(al)
                    ah and am are overlapping not more than 51 bits
                    am and al are overlapping not more than 51 bits

   Guarantees:      abs(resh) > abs(resm) > abs(resl)
                    resh and resm are non-overlapping
		    resm and resl are non-overlapping
		    resm = round-to-nearest(resm + resl)

   Details:         resh, resm and resl are considered to be pointers

*/
#define Renormalize3(resh, resm, resl, ah, am, al)     \
{                                                      \
    double _t1h, _t1l, _t2l;                           \
                                                       \
    Add12(_t1h, _t1l, (am), (al));                     \
    Add12((*(resh)), _t2l, (ah), (_t1h));              \
    Add12((*(resm)), (*(resl)), _t2l, _t1l);           \
}


/* Mul23

   Procedure for multiplying two double double numbers resulting
   in a triple double number


   Arguments:       two double double numbers:
                    ah, al and
		    bh, bl
   
   Results:         a triple double number resh, resm, resl

   Preconditions:   abs(ah) > abs(al) 
                    ah and al do not overlap
		    ah = round-to-nearest(ah + al)
		    abs(bh) > abs(bl) 
                    bh and bl do not overlap
		    bh = round-to-nearest(bh + bl)
		    
   Guarantees:      resm and resl are non-overlapping
                    resm = round-to-nearest(resm + resl)
		    abs(resm) <= 2^(-49) * abs(resh)
		    resh+resm+resl = (ah+al) * (bh+bl) * (1 + eps)
		    where
		    abs(eps) <= 2^(-149)

   Details:         resh, resm and resl are considered to be pointers
*/
#define Mul23(resh, resm, resl, ah, al, bh, bl)                \
{                                                              \
    double _t1, _t2, _t3, _t4, _t5, _t6, _t7, _t8, _t9, _t10;  \
                                                               \
    Mul12((resh),&_t1,(ah),(bh));                              \
    Mul12(&_t2,&_t3,(ah),(bl));                                \
    Mul12(&_t4,&_t5,(al),(bh));                                \
    _t6 = (al) * (bl);                                         \
    Add22Cond(&_t7,&_t8,_t2,_t3,_t4,_t5);                      \
    Add12(_t9,_t10,_t1,_t6);                                   \
    Add22Cond((resm),(resl),_t7,_t8,_t9,_t10);                 \
}

/* Mul233

   Procedure for multiplying a double double number by 
   a triple double number resulting in a triple double number


   Arguments:       a double double number ah, al
                    a triple double number bh, bm, bl
   
   Results:         a triple double number resh, resm, resl

   Preconditions:   abs(ah) > abs(al)
                    ah and al do not overlap
		    ah = round-to-nearest(ah + al)
		    abs(bm) <= 2^(-b_o) * abs(bh)
		    abs(bl) <= 2^(-b_u) * abs(bm)
		    where
		    b_o >= 2
		    b_u >= 1
		    
   Guarantees:      resm and resl are non-overlapping
                    resm = round-to-nearest(resm + resl)
		    abs(resm) <= 2^(\gamma) * abs(resh)
		    where
		    \gamma >= min(48,b_o-4,b_o+b_u-4)
		    resh+resm+resl=(ah+al) * (bh+bm+bl) * (1+eps)
		    where
		    abs(eps) <= 
                       (2^(-99-b_o) + 2^(-99-b_o-b_u) + 2^(-152)) / 
		         (1 - 2^(-53) - 2^(-b_o+1) - 2^(-b_o-b_u+1))

   Details:         resh, resm and resl are considered to be pointers
*/
#define Mul233(resh, resm, resl, ah, al, bh, bm, bl)            \
{                                                               \
    double _t1, _t2, _t3, _t4, _t5, _t6, _t7, _t8, _t9, _t10;   \
    double _t11, _t12, _t13, _t14, _t15, _t16, _t17, _t18;      \
                                                                \
    Mul12((resh),&_t1,(ah),(bh));                               \
    Mul12(&_t2,&_t3,(ah),(bm));                                 \
    Mul12(&_t4,&_t5,(ah),(bl));                                 \
    Mul12(&_t6,&_t7,(al),(bh));                                 \
    Mul12(&_t8,&_t9,(al),(bm));                                 \
    _t10 = (al) * (bl);                                         \
    Add22Cond(&_t11,&_t12,_t2,_t3,_t4,_t5);                     \
    Add22Cond(&_t13,&_t14,_t6,_t7,_t8,_t9);                     \
    Add22Cond(&_t15,&_t16,_t11,_t12,_t13,_t14);                 \
    Add12Cond(_t17,_t18,_t1,_t10);                              \
    Add22Cond((resm),(resl),_t17,_t18,_t15,_t16);               \
}




/* Add33

   Procedure for adding two triple double numbers resulting
   in a triple double number


   Arguments:       two triple double numbers:
                    ah, am, al and
		    bh, bm, bl
   
   Results:         a triple double number resh, resm, resl

   Preconditions:   abs(bh) <= 0.75 * abs(ah)  OR  ( sign(bh) = sign(ah) AND abs(bh) <= abs(ah))  (i)
                    abs(am) <= 2^(-a_o) * abs(ah)
		    abs(al) <= 2^(-a_u) * abs(am)
		    abs(bm) <= 2^(-b_o) * abs(bh)
		    abs(bl) <= 2^(-b_u) * abs(bm)
		    where
		    b_o >= a_o >= 4
		    b_u >= a_u >= 4

		    Condition (i) may not be respected if 
		    one can assume in this case that ah=am=al=0
		    
   Guarantees:      resm and resl are non-overlapping
                    resm = round-to-nearest(resm + resl)
		    abs(resm) <= 2^(-min(a_o,b_o) + 5) * abs(resh)
		    resh+resm+resl = (ah+am+al + bh+bm+bl) * (1+eps)
                    where 
		    abs(eps) <= 2^(-min(a_o+a_u,b_o+b_u)-47) + 2^(-min(a_o,a_u)-98)

   Details:         resh, resm and resl are considered to be pointers
*/
#define Add33(resh, resm, resl, ah, am, al, bh, bm, bl)      \
{                                                            \
    double _t1, _t2, _t3, _t4, _t5, _t6, _t7, _t8;           \
                                                             \
    Add12((*(resh)),_t1,(ah),(bh));                          \
    Add12Cond(_t2,_t3,(am),(bm));                            \
    _t6 = (al) + (bl);                                       \
    Add12Cond(_t7,_t4,_t1,_t2);                              \
    _t5 = _t3 + _t4;                                         \
    _t8 = _t5 + _t6;                                         \
    Add12Cond((*(resm)),(*(resl)),_t7,_t8);                  \
}



/* Add233

   Procedure for adding a double double number to a triple 
   double number resulting in a triple double number


   Arguments:       a double double number ah, al
                    a triple double number bh, bm, bl
   
   Results:         a triple double number resh, resm, resl

   Preconditions:   abs(ah) > abs(al)
                    ah and al do not overlap
		    ah = round-to-nearest(ah + al)
		    abs(bh) <= 2^(-2) * abs(ah)
		    abs(bm) <= 2^(-b_o) * abs(bh)
		    abs(bl) <= 2^(-b_u) * abs(bm)
		    where
		    b_o >= 2
		    b_u >= 1
		    
   Guarantees:      resm and resl are non-overlapping
                    resm = round-to-nearest(resm + resl)
		    abs(resm) <= 2^(\gamma) * abs(resh)
		    where
		    \gamma >= min(45,b_o-4,b_o+b_u-2)
		    resh+resm+resl=((ah+al) + (bh+bm+bl)) * (1+eps)
		    where
		    abs(eps) <= 
                       <= 2^(-b_o-b_u-52) + 2^(-b_o-104) + 2^(-153)

   Details:         resh, resm and resl are considered to be pointers
*/
#define Add233(resh, resm, resl, ah, al, bh, bm, bl)            \
{                                                               \
    double _t1, _t2, _t3, _t4, _t5, _t6, _t7;                   \
                                                                \
    Add12((*(resh)),_t1,(ah),(bh));                             \
    Add12Cond(_t2,_t3,(al),(bm));                               \
    Add12Cond(_t4,_t5,_t1,_t2);                                 \
    _t6 = _t3 + (bl);                                           \
    _t7 = _t6 + _t5;                                            \
    Add12Cond((*(resm)),(*(resl)),_t4,_t7);                     \
}

/* Add123

   Procedure for adding a double number to a double 
   double number resulting in a triple double number


   Arguments:       a double number a 
                    a double double number bh, bl
   
   Results:         a triple double number resh, resm, resl

   Preconditions:   abs(bh) <= 2^(-2) * abs(a)
		    abs(bl) <= 2^(-53) * abs(bh)
		    
   Guarantees:      resm and resl are non-overlapping
                    resm = round-to-nearest(resm + resl)
		    abs(resm) <= 2^(-\gamma) * abs(resh)
		    where
		    
		    \gamma >= 52

		    resh+resm+resl=(a + (bh+bl)) exactly
		    

   Details:         resh, resm and resl are considered to be pointers
*/
#define Add123(resh, resm, resl, a, bh, bl)                     \
{                                                               \
    double _t1;                                                 \
                                                                \
    Add12((*(resh)),_t1,(a),(bh));                              \
    Add12((*(resm)),(*(resl)),_t1,(bl));                        \
}

/* Add213

   Procedure for adding a double double number to a double 
   number resulting in a triple double number


   Arguments:       a double double number ah, al 
                    a double number b
   
   Results:         a triple double number resh, resm, resl

   Preconditions:   abs(b) <= 2^(-2) * abs(ah)
		    abs(al) <= 2^(-53) * abs(ah)
		    
   Guarantees:      resm and resl are non-overlapping
                    resm = round-to-nearest(resm + resl)
		    abs(resm) <= 2^(-\gamma) * abs(resh)
		    where
		    
		    \gamma >= 52

		    resh+resm+resl=(a + (bh+bm+bl)) exactly
		    

   Details:         resh, resm and resl are considered to be pointers
*/
#define Add213(resh, resm, resl, ah, al, b)                     \
{                                                               \
    double _t1;                                                 \
                                                                \
    Add12((*(resh)),_t1,(ah),(b));                              \
    Add12Cond((*(resm)),(*(resl)),(al),(b));                    \
}



/* Add23

   Procedure for adding a double-double number to a double-double 
   number resulting in a triple double number


   Arguments:       a double double number ah, al
                    a double double number bh, bl
   
   Results:         a triple double number resh, resm, resl

   Preconditions:   abs(bh) <= 2^(-2) * abs(ah)
                    abs(al) <= 2^(-53) * abs(ah)
		    abs(bl) <= 2^(-53) * abs(bh)
		    
   Guarantees:      TO DO
		    

   Details:         resh, resm and resl are considered to be pointers
*/
#define Add23(resh, resm, resl, ah, al, bh, bl)                 \
{                                                               \
    double _t1, _t2, _t3, _t4, _t5, _t6;                        \
                                                                \
    Add12((*(resh)),_t1,(ah),(bh));                             \
    Add12Cond(_t2,_t3,(al),(bl));                               \
    Add12Cond(_t4,_t5,_t1,_t2);                                 \
    _t6 = _t3 + _t5;                                            \
    Add12Cond((*(resm)),(*(resl)),_t4,_t6);                     \
}




/* Add133

   Procedure for adding a double number to a triple 
   double number resulting in a triple double number


   Arguments:       a double number a 
                    a triple double number bh, bm, bl
   
   Results:         a triple double number resh, resm, resl

   Preconditions:   abs(bh) <= 2^(-2) * abs(a)
		    abs(bm) <= 2^(-b_o) * abs(bh)
		    abs(bl) <= 2^(-b_u) * abs(bm)
		    where
		    b_o >= 2
		    b_u >= 1
		    
   Guarantees:      resm and resl are non-overlapping
                    resm = round-to-nearest(resm + resl)
		    abs(resm) <= 2^(\gamma) * abs(resh)
		    where
		    \gamma >= min(47,2-b_o,1-b_o-b_u)
		    resh+resm+resl=(a + (bh+bm+bl)) * (1+eps)
		    where
		    abs(eps) <= 
                       <= 2^(-52-b_o-b_u) + 2^(-154)


   Details:         resh, resm and resl are considered to be pointers
*/
#define Add133(resh, resm, resl, a, bh, bm, bl)                 \
{                                                               \
    double _t1, _t2, _t3, _t4;                                  \
                                                                \
    Add12((*(resh)),_t1,(a),(bh));                              \
    Add12Cond(_t2,_t3,_t1,(bm));                                \
    _t4 = _t3 + (bl);                                           \
    Add12Cond((*(resm)),(*(resl)),_t2,_t4);                     \
}

/* Add133Cond

   Procedure for adding a double number to a triple 
   double number resulting in a triple double number


   Arguments:       a double number a 
                    a triple double number bh, bm, bl
   
   Results:         a triple double number resh, resm, resl

   Preconditions:   abs(bm) <= 2^(-b_o) * abs(bh)
		    abs(bl) <= 2^(-b_u) * abs(bm)
		    where
		    b_o >= 2
		    b_u >= 1
		    
   Guarantees:      resm and resl are non-overlapping
                    resm = round-to-nearest(resm + resl)
		    abs(resm) <= 2^(\gamma) * abs(resh)
		    where

		    TODO

		    resh+resm+resl=(a + (bh+bm+bl)) * (1+eps)
		    where
		    abs(eps) <= 

		    TODO


   Details:         resh, resm and resl are considered to be pointers
*/
#define Add133Cond(resh, resm, resl, a, bh, bm, bl)             \
{                                                               \
    double _t1, _t2, _t3, _t4;                                  \
                                                                \
    Add12Cond((*(resh)),_t1,(a),(bh));                          \
    Add12Cond(_t2,_t3,_t1,(bm));                                \
    _t4 = _t3 + (bl);                                           \
    Add12Cond((*(resm)),(*(resl)),_t2,_t4);                     \
}



/* Add233Cond

   Procedure for adding a double double number to a triple 
   double number resulting in a triple double number


   Arguments:       a double double number ah, al
                    a triple double number bh, bm, bl
   
   Results:         a triple double number resh, resm, resl

   Preconditions:   abs(ah) > abs(al)
                    ah and al do not overlap
		    ah = round-to-nearest(ah + al)
		    abs(bm) <= 2^(-b_o) * abs(bh)
		    abs(bl) <= 2^(-b_u) * abs(bm)
		    where
		    b_o >= 2
		    b_u >= 1
		    
   Guarantees:      resm and resl are non-overlapping
                    resm = round-to-nearest(resm + resl)
		    abs(resm) <= 2^(\gamma) * abs(resh)
		    where
		    \gamma >= ????
		    resh+resm+resl=((ah+al) + (bh+bm+bl)) * (1+eps)
		    where
		    abs(eps) <= 
                       <= ????

   Details:         resh, resm and resl are considered to be pointers
*/
#define Add233Cond(resh, resm, resl, ah, al, bh, bm, bl)        \
{                                                               \
    double _t1, _t2, _t3, _t4, _t5, _t6, _t7;                   \
                                                                \
    Add12Cond((*(resh)),_t1,(ah),(bh));                         \
    Add12Cond(_t2,_t3,(al),(bm));                               \
    Add12Cond(_t4,_t5,_t1,_t2);                                 \
    _t6 = _t3 + (bl);                                           \
    _t7 = _t6 + _t5;                                            \
    Add12Cond((*(resm)),(*(resl)),_t4,_t7);                     \
}




/* Mul33

   Procedure for multiplying two triple double numbers resulting
   in a triple double number


   Arguments:       two triple double numbers:
                    ah, am, al and
		    bh, bm, bl
   
   Results:         a triple double number resh, resm, resl

   Preconditions:   abs(am) <= 2^(-a_o) * abs(ah)
		    abs(al) <= 2^(-a_u) * abs(am)
		    abs(bm) <= 2^(-b_o) * abs(bh)
		    abs(bl) <= 2^(-b_u) * abs(bm)
		    where
		    b_o, a_o >= 5
		    b_u, a_u >= 5

		    
   Guarantees:      resm and resl are non-overlapping
                    resm = round-to-nearest(resm + resl)
		    abs(resm) <= 2^(-g_o) * abs(resh)
		    with
		    g_o > min(48,-4+a_o,-4+b_o,-4+a_o-b_o)
		    resh+resm+resl = (ah+am+al) * (bh+bm+bl) * (1+eps)
                    where 
		    abs(eps) <= 2^-151 + 2^-99-a_o + 2^-99-b_o +
		    + 2^-49-a_o-a_u + 2^-49-b_o-b_u + 2^50-a_o-b_o-b_u + 
		    + 2^50-a_o-b_o-b_u + 2^-101-a_o-b_o + 2^-52-a_o-a_u-b_o-b_u

   Details:         resh, resm and resl are considered to be pointers
*/

#define Mul33(resh, resm, resl, ah, am, al, bh, bm, bl)      \
{                                                            \
    double _t1, _t2, _t3, _t4, _t5, _t6, _t7, _t8, _t9;      \
    double _t10, _t11, _t12, _t13, _t14, _t15, _t16, _t17;   \
    double _t18, _t19, _t20, _t21, _t22;                     \
                                                             \
    Mul12((resh),&_t1,(ah),(bh));                            \
    Mul12(&_t2,&_t3,(ah),(bm));                              \
    Mul12(&_t4,&_t5,(am),(bh));                              \
    Mul12(&_t6,&_t7,(am),(bm));                              \
    _t8 = (ah) * (bl);                                       \
    _t9 = (al) * (bh);                                       \
    _t10 = (am) * (bl);                                      \
    _t11 = (al) * (bm);                                      \
    _t12 = _t8 + _t9;                                        \
    _t13 = _t10 + _t11;                                      \
    Add12Cond(_t14,_t15,_t1,_t6);                            \
    _t16 = _t7 + _t15;                                       \
    _t17 = _t12 + _t13;                                      \
    _t18 = _t16 + _t17;                                      \
    Add12Cond(_t19,_t20,_t14,_t18);                          \
    Add22Cond(&_t21,&_t22,_t2,_t3,_t4,_t5);                  \
    Add22Cond((resm),(resl),_t21,_t22,_t19,_t20);            \
}


/* Mul133

   Procedure for multiplying double by a triple double number resulting
   in a triple double number


   Arguments:       a double a
		    a triple double bh, bm, bl
   
   Results:         a triple double number resh, resm, resl

   Preconditions:   abs(bm) <= 2^(-b_o) * abs(bh)
		    abs(bl) <= 2^(-b_u) * abs(bm)
		    where
		    b_o >= 2
		    b_u >= 2

		    
   Guarantees:      resm and resl are non-overlapping
                    resm = round-to-nearest(resm + resl)
		    abs(resm) <= 2^(-g_o) * abs(resh)
		    with
		    g_o > min(47,-5-b_o,-5+b_o+b_u) 
		    resh+resm+resl = a * (bh+bm+bl) * (1+eps)
                    where 
		    abs(eps) <= 2^-49-b_o-b_u + 2^-101-b_o + 2^-156

   Details:         resh, resm and resl are considered to be pointers
*/
#define Mul133(resh, resm, resl, a, bh, bm, bl)            \
{                                                          \
    double _t2, _t3, _t4, _t5, _t7, _t8, _t9, _t10;        \
                                                           \
    Mul12((resh),&_t2,(a),(bh));                           \
    Mul12(&_t3,&_t4,(a),(bm));                             \
    _t5 = (a) * (bl);                                      \
    Add12Cond(_t9,_t7,_t2,_t3);                            \
    _t8 = _t4 + _t5;                                       \
    _t10 = _t7 + _t8;                                      \
    Add12Cond((*(resm)),(*(resl)),_t9,_t10);               \
}

/* Mul123

   Procedure for multiplying double by a double double number resulting
   in a triple double number


   Arguments:       a double a
		    a double double bh, bl
   
   Results:         a triple double number resh, resm, resl
		    
   Guarantees:      resm and resl are non-overlapping
                    resm = round-to-nearest(resm + resl)
		    abs(resm) <= 2^(-g_o) * abs(resh)
		    with
		    g_o > 47 
		    resh+resm+resl = a * (bh+bm) * (1+eps)
                    where 
		    abs(eps) <= 2^-154

   Details:         resh, resm and resl are considered to be pointers
*/
#define Mul123(resh, resm, resl, a, bh, bl)                \
{                                                          \
    double _t1, _t2, _t3, _t4, _t5, _t6;                   \
                                                           \
    Mul12((resh),&_t1,(a),(bh));                           \
    Mul12(&_t2,&_t3,(a),(bl));                             \
    Add12Cond(_t5,_t4,_t1,_t2);                            \
    _t6 = _t3 + _t4;                                       \
    Add12Cond((*(resm)),(*(resl)),_t5,_t6);                \
}



/* sqrt13

   Computes a triple-double approximation of sqrt(x)
   
   Should be provable to be exact to at least 140 bits.

   Only handles the following special cases:
   - x == 0
   - subnormal x 
   The following cases are not handled:
   - x < 0
   - x = +/-Infty, NaN

*/


#define sqrt13(resh, resm, resl , x)                                                          \
{                                                                                             \
  db_number _xdb;                                                                             \
  int _E;                                                                                     \
  double _m, _r0, _r1, _r2, _r3h, _r3l, _r4h, _r4l;                                           \
  double _r5h, _r5m, _r5l, _srtmh, _srtml, _srtmm;                                            \
  double _r2PHr2h, _r2PHr2l, _r2Sqh, _r2Sql;                                                  \
  double _mMr2h, _mMr2l, _mMr2Ch, _mMr2Cl;                                                    \
  double _MHmMr2Ch, _MHmMr2Cl;                                                                \
  double _r3Sqh, _r3Sql, _mMr3Sqh, _mMr3Sql;                                                  \
  double _srtmhover,_srtmmover,_srtmlover;                                                    \
  double _HmMr4Sqm,_HmMr4Sql, _mMr4Sqhover, _mMr4Sqmover, _mMr4Sqlover;                       \
  double _mMr4Sqh, _mMr4Sqm, _mMr4Sql, _r4Sqh, _r4Sqm, _r4Sql;                                \
                                                                                              \
  /* Special case x = 0 */                                                                    \
  if ((x) == 0) {                                                                             \
    (*(resh)) = (x);                                                                          \
    (*(resm)) = 0;                                                                            \
    (*(resl)) = 0;                                                                            \
  } else {                                                                                    \
                                                                                              \
    _E = 0;                                                                                   \
                                                                                              \
    /* Convert to integer format */                                                           \
    _xdb.d = (x);                                                                             \
                                                                                              \
    /* Handle subnormal case */                                                               \
    if (_xdb.i[HI] < 0x00100000) {                                                            \
      _E = -52;                                                                               \
      _xdb.d *= ((db_number) ((double) SQRTTWO52)).d;                                         \
                        /* make x a normal number */                                          \
    }                                                                                         \
                                                                                              \
    /* Extract exponent E and mantissa m */                                                   \
    _E += (_xdb.i[HI]>>20)-1023;                                                              \
    _xdb.i[HI] = (_xdb.i[HI] & 0x000fffff) | 0x3ff00000;                                      \
    _m = _xdb.d;                                                                              \
                                                                                              \
    /* Make exponent even */                                                                  \
    if (_E & 0x00000001) {                                                                    \
      _E++;                                                                                   \
      _m *= 0.5;    /* Suppose now 1/2 <= m <= 2 */                                           \
    }                                                                                         \
                                                                                              \
    /* Construct sqrt(2^E) = 2^(E/2) */                                                       \
    _xdb.i[HI] = (_E/2 + 1023) << 20;                                                         \
    _xdb.i[LO] = 0;                                                                           \
                                                                                              \
    /* Compute initial approximation to r = 1/sqrt(m) */                                      \
                                                                                              \
    _r0 = SQRTPOLYC0 +                                                                        \
         _m * (SQRTPOLYC1 + _m * (SQRTPOLYC2 + _m * (SQRTPOLYC3 + _m * SQRTPOLYC4)));         \
                                                                                              \
    /* Iterate two times on double precision */                                               \
                                                                                              \
    _r1 = 0.5 * _r0 * (3 - _m * (_r0 * _r0));                                                 \
    _r2 = 0.5 * _r1 * (3 - _m * (_r1 * _r1));                                                 \
                                                                                              \
    /* Iterate two times on double-double precision */                                        \
                                                                                              \
    Mul12(&_r2Sqh, &_r2Sql, _r2, _r2);                                                        \
    Add12(_r2PHr2h, _r2PHr2l, _r2, (0.5 * _r2));                                              \
    Mul12(&_mMr2h, &_mMr2l, _m, _r2);                                                         \
    Mul22(&_mMr2Ch, &_mMr2Cl, _mMr2h, _mMr2l, _r2Sqh, _r2Sql);                                \
                                                                                              \
    _MHmMr2Ch = -0.5 * _mMr2Ch;                                                               \
    _MHmMr2Cl = -0.5 * _mMr2Cl;                                                               \
                                                                                              \
    Add22(&_r3h, &_r3l, _r2PHr2h, _r2PHr2l, _MHmMr2Ch, _MHmMr2Cl);                            \
                                                                                              \
    Mul22(&_r3Sqh, &_r3Sql, _r3h, _r3l, _r3h, _r3l);                                          \
    Mul22(&_mMr3Sqh, &_mMr3Sql, _m, 0, _r3Sqh, _r3Sql);                                       \
             /* To prove: mMr3Sqh = 1.0 in each case */                                       \
                                                                                              \
    Mul22(&_r4h, &_r4l, _r3h, _r3l, 1, (-0.5 * _mMr3Sql));                                    \
                                                                                              \
    /* Iterate once on triple-double precision */                                             \
                                                                                              \
    Mul23(&_r4Sqh, &_r4Sqm, &_r4Sql, _r4h, _r4l, _r4h, _r4l);                                 \
    Mul133(&_mMr4Sqhover, &_mMr4Sqmover, &_mMr4Sqlover, _m, _r4Sqh, _r4Sqm, _r4Sql);          \
    Renormalize3(&_mMr4Sqh, &_mMr4Sqm, &_mMr4Sql, _mMr4Sqhover, _mMr4Sqmover, _mMr4Sqlover);  \
    /* To prove: mMr4Sqh = 1.0 in each case */                                                \
                                                                                              \
    _HmMr4Sqm = -0.5 * _mMr4Sqm;                                                              \
    _HmMr4Sql = -0.5 * _mMr4Sql;                                                              \
                                                                                              \
    Mul233(&_r5h,&_r5m,&_r5l,_r4h,_r4l,1,_HmMr4Sqm,_HmMr4Sql);                                \
                                                                                              \
    /* Multiply obtained reciprocal square root by m */                                       \
                                                                                              \
    Mul133(&_srtmhover, &_srtmmover, &_srtmlover,_m,_r5h,_r5m,_r5l);                          \
                                                                                              \
    Renormalize3(&_srtmh,&_srtmm,&_srtml,_srtmhover,_srtmmover,_srtmlover);                   \
                                                                                              \
    /* Multiply componentwise by sqrt(2^E) */                                                 \
    /* which is an integer power of 2 that may not produce a subnormal */                     \
                                                                                              \
    (*(resh)) = _xdb.d * _srtmh;                                                              \
    (*(resm)) = _xdb.d * _srtmm;                                                              \
    (*(resl)) = _xdb.d * _srtml;                                                              \
                                                                                              \
  } /* End: special case 0 */                                                                 \
}


/* recpr33()

   Computes a triple-double reciprocal of a triple-double
   
   Should be provable to be exact to at least 140 bits

   No special case handling is done

   dh + dm + dl must be renormalized

   The result is renormalized

*/


#define recpr33(resh, resm, resl, dh, dm, dl)                                                 \
{                                                                                             \
    double _r1, _t1, _t2, _t3, _t4, _t5, _t6, _t7, _t8, _t9, _t10, _t11, _t12, _t13, _t14;    \
    double _r2h, _r2l, _t15, _t16, _t17, _t18, _t19, _t20, _t21, _t22, _t23;                  \
                                                                                              \
    _r1 = 1.0 / (dh);                                                                         \
    Mul12(&_t1,&_t2,_r1,(dh));                                                                \
    _t3 = _t1 - 1.0;                                                                          \
    Add12Cond(_t4,_t5,_t3,_t2);                                                               \
    Mul12(&_t6,&_t7,_r1,(dm));                                                                \
    Add12(_t8,_t9,-1.0,_t6);                                                                  \
    _t10 = _t9 + _t7;                                                                         \
    Add12(_t11,_t12,_t8,_t10);                                                                \
    _r1 = -_r1;                                                                               \
    Add22Cond(&_t13,&_t14,_t4,_t5,_t11,_t12);                                                 \
    Mul122(&_r2h,&_r2l,_r1,_t13,_t14);                                                        \
    Mul233(&_t15,&_t16,&t_17,_r2h,_r2l,(dh),(dm),(dl));                                       \
    Renormalize3(&_t18,&_t19,&_t20,_t15,_t16,_t17);                                           \
    _t18 = -1.0;                                                                              \
    Mul233(&_t21,&_t22,&_t23,_r2h,_r2l,_t18,_t19,_t20);                                       \
    _t21 = -_t21; _t22 = -_t22; _t23 = -_t23;                                                 \
    Renormalize3((resh),(resm),(resl),_t21,_t22,_t23);                                        \
}





#endif /* ifdef EXPANSION_H*/
