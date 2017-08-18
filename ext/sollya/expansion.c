/*

Copyright 2006-20011 by

Laboratoire de l'Informatique du Parallelisme,
UMR CNRS - ENS Lyon - UCB Lyon 1 - INRIA 5668

and by

Laboratoire d'Informatique de Paris 6, equipe PEQUAN,
UPMC Universite Paris 06 - CNRS - UMR 7606 - LIP6, Paris, France.

Contributor Ch. Lauter

christoph.lauter@ens-lyon.org

This software is a computer program whose purpose is to provide an
environment for safe floating-point code development. It is
particularily targeted to the automatized implementation of
mathematical floating-point libraries (libm). Amongst other features,
it offers a certified infinity norm, an automatic polynomial
implementer and a fast Remez algorithm.

This software is governed by the CeCILL-C license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL-C
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL-C license and that you accept its terms.

This program is distributed WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

*/

#include <gmp.h>
#include <mpfr.h>
#include <fpu_control.h>
#include "expansion.h"
#include <sys/time.h>
#include <time.h>


void mpfr_to_double(double *dh, mpfr_t op) {
  *dh = mpfr_get_d(op, GMP_RNDN);
}

void mpfr_to_doubledouble(double *dh, double *dm, mpfr_t op) {
  mpfr_t rest, temp;

  mpfr_init2(rest,mpfr_get_prec(op));
  mpfr_init2(temp,mpfr_get_prec(op));

  mpfr_set(rest,op,GMP_RNDN);

  *dh = mpfr_get_d(rest, GMP_RNDN);
  mpfr_set_d(temp,*dh,GMP_RNDN);
  mpfr_sub(rest,rest,temp,GMP_RNDN);
  
  *dm = mpfr_get_d(rest, GMP_RNDN);
  mpfr_set_d(temp,*dm,GMP_RNDN);
  mpfr_sub(rest,rest,temp,GMP_RNDN);

  mpfr_clear(rest);
  mpfr_clear(temp);
}

void mpfr_to_tripledouble(double *dh, double *dm, double *dl, mpfr_t op) {
  mpfr_t rest, temp;

  mpfr_init2(rest,mpfr_get_prec(op));
  mpfr_init2(temp,mpfr_get_prec(op));

  mpfr_set(rest,op, GMP_RNDN);

  *dh = mpfr_get_d(rest, GMP_RNDN);
  mpfr_set_d(temp,*dh,GMP_RNDN);
  mpfr_sub(rest,rest,temp,GMP_RNDN);
  
  *dm = mpfr_get_d(rest, GMP_RNDN);
  mpfr_set_d(temp,*dm,GMP_RNDN);
  mpfr_sub(rest,rest,temp,GMP_RNDN);

  *dl = mpfr_get_d(rest, GMP_RNDN);
  mpfr_set_d(temp,*dl,GMP_RNDN);
  mpfr_sub(rest,rest,temp,GMP_RNDN);

  mpfr_clear(rest);
  mpfr_clear(temp);
}

void double_to_mpfr(mpfr_t rop, double dh) {
  mpfr_set_d(rop,dh,GMP_RNDN);
}

void doubledouble_to_mpfr(mpfr_t rop, double dh, double dm) {
  mpfr_t temp;
  mp_prec_t prec;

  prec = mpfr_get_prec(rop);
  
  if (prec < 53) prec = 53;

  mpfr_init2(temp, prec);
  
  mpfr_set_d(rop, dh, GMP_RNDN);
  
  mpfr_set_d(temp, dm, GMP_RNDN);
  mpfr_add(rop, rop, temp, GMP_RNDN);
  
  mpfr_clear(temp);
}

void tripledouble_to_mpfr(mpfr_t rop, double dh, double dm, double dl) {
  mpfr_t temp;
  mp_prec_t prec;

  prec = mpfr_get_prec(rop);
  
  if (prec < 53) prec = 53;

  mpfr_init2(temp, prec);
  
  mpfr_set_d(rop, dh, GMP_RNDN);
  
  mpfr_set_d(temp, dm, GMP_RNDN);
  mpfr_add(rop, rop, temp, GMP_RNDN);

  mpfr_set_d(temp, dl, GMP_RNDN);
  mpfr_add(rop, rop, temp, GMP_RNDN);

  
  mpfr_clear(temp);
}


void f(mpfr_t y, mpfr_t xMpfr) {
  unsigned short oldcw, cw;
#if defined(D_TO_D)
  double x;
  double resh;
#elif defined(D_TO_DD)
  double x;
  double resh, resm;
#elif defined(D_TO_TD)
  double x;
  double resh, resm, resl;
#elif defined(DD_TO_DD)
  double xh, xm;
  double resh, resm;
#elif defined(DD_TO_TD)
  double xh, xm;
  double resh, resm, resl;
#elif defined (TD_TO_TD)
  double xh, xm, xl;
  double resh, resm, resl;
#endif

  _FPU_GETCW(oldcw);
  cw = (_FPU_DEFAULT & ~_FPU_EXTENDED)|_FPU_DOUBLE;
  _FPU_SETCW(cw);

#if defined(D_TO_D)
  mpfr_to_double(&x, xMpfr);     
  POLYNOMIALNAME(&resh, x);
  double_to_mpfr(y, resh);
#elif defined(D_TO_DD)
  mpfr_to_double(&x, xMpfr);
  POLYNOMIALNAME(&resh, &resm, x);
  doubledouble_to_mpfr(y, resh, resm);
#elif defined(D_TO_TD)
  mpfr_to_double(&x, xMpfr);
  POLYNOMIALNAME(&resh, &resm, &resl, x);
  tripledouble_to_mpfr(y, resh, resm, resl);
#elif defined(DD_TO_DD)
  mpfr_to_doubledouble(&xh, &xm, xMpfr);
  POLYNOMIALNAME(&resh, &resm, xh, xm);
  doubledouble_to_mpfr(y, resh, resm);
#elif defined(DD_TO_TD)
  mpfr_to_doubledouble(&xh, &xm, xMpfr);
  POLYNOMIALNAME(&resh, &resm, &resl, xh, xm);
  tripledouble_to_mpfr(y, resh, resm, resl);
#elif defined(TD_TO_TD)
  mpfr_to_tripledouble(&xh, &xm, &xl, xMpfr);
  POLYNOMIALNAME(&resh, &resm, &resl, xh, xm, xl);
  tripledouble_to_mpfr(y, resh, resm, resl);
#else
#warning You must define one of the macros for the argument and result formats
  mpfr_set(y,xMpfr,GMP_RNDN);
#endif

  _FPU_SETCW(oldcw);

}


int timefunc(int *timing, void **args) {
  unsigned short oldcw, cw;
#if defined(D_TO_D)
  double x;
  double resh;
#elif defined(D_TO_DD)
  double x;
  double resh, resm;
#elif defined(D_TO_TD)
  double x;
  double resh, resm, resl;
#elif defined(DD_TO_DD)
  double xh, xm;
  double resh, resm;
#elif defined(DD_TO_TD)
  double xh, xm;
  double resh, resm, resl;
#elif defined (TD_TO_TD)
  double xh, xm, xl;
  double resh, resm, resl;
#endif
  mpfr_t xMpfr;
  mpfr_t a, b;
  mpfr_t h;
  int steps, iterations;
  int i;
  struct timeval start, end;
  int usecs;
  double overalltime;

  mpfr_init2(xMpfr, 161);
  mpfr_init2(a, mpfr_get_prec(*((mpfr_t *) (args[0]))));
  mpfr_init2(b, mpfr_get_prec(*((mpfr_t *) (args[1]))));
  mpfr_set(a, *((mpfr_t *) (args[0])), GMP_RNDN);
  mpfr_set(b, *((mpfr_t *) (args[1])), GMP_RNDN);
  mpfr_init2(h, 161);

  mpfr_set(xMpfr, a, GMP_RNDU);

  steps = *((int *) (args[2]));
  iterations = *((int *) (args[3]));

  mpfr_sub(h, b, a, GMP_RNDU);
  mpfr_div_si(h, h, steps, GMP_RNDU);
  
  overalltime = 0;

  while (mpfr_cmp(xMpfr,b) <= 0) {

    _FPU_GETCW(oldcw);
    cw = (_FPU_DEFAULT & ~_FPU_EXTENDED)|_FPU_DOUBLE;
    _FPU_SETCW(cw);

#if defined(D_TO_D)
    mpfr_to_double(&x, xMpfr);     
    gettimeofday(&start,NULL);
    for (i=0;i<iterations;i++) POLYNOMIALNAME(&resh, x);
    gettimeofday(&end,NULL);
#elif defined(D_TO_DD)
    mpfr_to_double(&x, xMpfr);
    gettimeofday(&start,NULL);
    for (i=0;i<iterations;i++) POLYNOMIALNAME(&resh, &resm, x);
    gettimeofday(&end,NULL);
#elif defined(D_TO_TD)
    mpfr_to_double(&x, xMpfr);
    gettimeofday(&start,NULL);
    for (i=0;i<iterations;i++) POLYNOMIALNAME(&resh, &resm, &resl, x);
    gettimeofday(&end,NULL);
#elif defined(DD_TO_DD)
    mpfr_to_doubledouble(&xh, &xm, xMpfr);
    gettimeofday(&start,NULL);
    for (i=0;i<iterations;i++) POLYNOMIALNAME(&resh, &resm, xh, xm);
    gettimeofday(&end,NULL);
#elif defined(DD_TO_TD)
    mpfr_to_doubledouble(&xh, &xm, xMpfr);
    gettimeofday(&start,NULL);
    for (i=0;i<iterations;i++) POLYNOMIALNAME(&resh, &resm, &resl, xh, xm);
    gettimeofday(&end,NULL);
#elif defined(TD_TO_TD)
    mpfr_to_tripledouble(&xh, &xm, &xl, xMpfr);
    gettimeofday(&start,NULL);
    for (i=0;i<iterations;i++) POLYNOMIALNAME(&resh, &resm, &resl, xh, xm, xl);
    gettimeofday(&end,NULL);
#else
#warning You must define one of the macros for the argument and result formats
#endif

    _FPU_SETCW(oldcw);

    mpfr_add(xMpfr, xMpfr, h, GMP_RNDU);

    usecs = ((end.tv_sec - start.tv_sec) * 1000000) + (end.tv_usec - start.tv_usec);

    overalltime += usecs;
  }

  overalltime /= steps;

  mpfr_clear(b);
  mpfr_clear(a);
  mpfr_clear(xMpfr);
  mpfr_clear(h);

  *timing = overalltime;

  return 1;
}
