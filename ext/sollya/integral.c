/*

Copyright 2006-2009 by 

Laboratoire de l'Informatique du Parallelisme, 
UMR CNRS - ENS Lyon - UCB Lyon 1 - INRIA 5668

Contributors Ch. Lauter, S. Chevillard

christoph.lauter@ens-lyon.org
sylvain.chevillard@ens-lyon.org

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

#include <stdio.h> /* fprintf, fopen, fclose, */
#include <stdlib.h> /* exit, free, mktemp */
#include <errno.h>

#include <mpfr.h>
#include "mpfi-compat.h"
#include "expression.h"
#include "infnorm.h"
#include "integral.h"
#include "general.h"


#define DEBUG 0
#define DEBUGMPFI 0



rangetype integral(node *func, rangetype interval, mp_prec_t prec, mpfr_t diam) {
  node *deriv;

  deriv = differentiate(func);
  
  rangetype x,y;
  mpfr_t x1,x2,y1,y2,delta;
  sollya_mpfi_t temp, val;

  rangetype sum;
  sum.a = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
  sum.b = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
  mpfr_init2(*(sum.a),prec);
  mpfr_init2(*(sum.b),prec);
  mpfr_set_d(*(sum.a),0.,GMP_RNDD);
  mpfr_set_d(*(sum.b),0.,GMP_RNDU);
  
  
  if (mpfr_equal_p(*(interval.a),*(interval.b))) {
    printMessage(1,"Warning: the given interval is reduced to one point.\n");
    return sum;
  }

  if (mpfr_less_p(*(interval.b),*(interval.a))) {
    printMessage(1,"Warning: the interval is empty.\n");
    return sum;
  }

  mpfr_init2(delta,53);
  mpfr_sub(delta, *(interval.b), *(interval.a), GMP_RNDN);
  mpfr_mul(delta, delta, diam, GMP_RNDN);

  sollya_mpfi_init2(temp,prec);
  sollya_mpfi_init2(val,prec);

  mpfr_init2(x1,prec);
  mpfr_init2(x2,prec);
  mpfr_set(x1, *(interval.a),GMP_RNDD);
  mpfr_add(x2, x1, delta, GMP_RNDN);
  x.a = &x1;
  x.b = &x2;
  
  mpfr_init2(y1,prec);
  mpfr_init2(y2,prec);
  y.a = &y1;
  y.b = &y2;

  while(mpfr_less_p(x2,*(interval.b))) {
    evaluateRangeFunctionFast(y, func, deriv, x, prec);

    sollya_mpfi_set_fr(temp, x1);
    sollya_mpfi_set_fr(val, x2);
    sollya_mpfi_sub(temp, val, temp);
    
    sollya_mpfi_interv_fr(val, *(y.a), *(y.b));
    sollya_mpfi_mul(temp, temp, val);
    
    sollya_mpfi_get_left(y1, temp);
    sollya_mpfi_get_right(y2, temp);
    mpfr_add(*(sum.a), *(sum.a), y1, GMP_RNDD);
    mpfr_add(*(sum.b), *(sum.b), y2, GMP_RNDU);
    
    mpfr_set(x1,x2,GMP_RNDD); // exact
    mpfr_add(x2, x1, delta, GMP_RNDN);
  }

  mpfr_set(x2,*(interval.b),GMP_RNDU);
  evaluateRangeFunction(y, func, x, prec);

  sollya_mpfi_set_fr(temp, x1);
  sollya_mpfi_set_fr(val, x2);
  sollya_mpfi_sub(temp, val, temp);
    
  sollya_mpfi_interv_fr(val, *(y.a), *(y.b));
  sollya_mpfi_mul(temp, temp, val);
    
  sollya_mpfi_get_left(y1, temp);
  sollya_mpfi_get_right(y2, temp);
  mpfr_add(*(sum.a), *(sum.a), y1, GMP_RNDD);
  mpfr_add(*(sum.b), *(sum.b), y2, GMP_RNDU);
  
 
  free_memory(deriv);
  sollya_mpfi_clear(val); sollya_mpfi_clear(temp);
  mpfr_clear(x1); mpfr_clear(x2);  
  mpfr_clear(y1); mpfr_clear(y2);
  mpfr_clear(delta);

  return sum;
}

void uncertifiedIntegral(mpfr_t result, node *tree, mpfr_t a, mpfr_t b, unsigned long int points, mp_prec_t prec) {
  mpfr_t sum, temp, x, y1, y2, step;

  mpfr_init2(step, prec);

  mpfr_sub(step, b, a, GMP_RNDN);
  mpfr_div_ui(step, step, points, GMP_RNDN);
 
  if (mpfr_sgn(step) == 0) {
    printMessage(1,"Warning: the given interval is reduced to one point.\n");
    mpfr_set_d(result,0.,GMP_RNDN);
    mpfr_clear(step);
    return;
  }

  if (mpfr_sgn(step) < 0) {
    printMessage(1,"Warning: the interval is empty.\n");
    mpfr_set_d(result,0.,GMP_RNDN);
    mpfr_clear(step);
    return;
  }

  mpfr_init2(x, prec);
  mpfr_init2(y1, prec);
  mpfr_init2(y2, prec);
  mpfr_init2(temp, prec);
  mpfr_init2(sum, prec);

  mpfr_set_d(sum,0.,GMP_RNDN);

  mpfr_set(x,a,GMP_RNDN);
  evaluateFaithful(y1,tree,x,prec);

  mpfr_add(x,x,step,GMP_RNDU);
  if (mpfr_greater_p(x,b)) {
    mpfr_sub(x, x, step, GMP_RNDN);
    mpfr_sub(step, b, x, GMP_RNDN);
    mpfr_set(x,b,GMP_RNDN);
  }
  evaluateFaithful(y2,tree,x,prec);

  while(mpfr_lessequal_p(x,b)) {
    mpfr_add(temp, y1, y2, GMP_RNDN);
    mpfr_div_2ui(temp, temp, 1, GMP_RNDN);
    mpfr_mul(temp, temp, step, GMP_RNDN);
    mpfr_add(sum,sum,temp, GMP_RNDN);

    mpfr_set(y1, y2, GMP_RNDN);

    if (mpfr_equal_p(x,b)) break;

    mpfr_add(x,x,step,GMP_RNDU);
    if (mpfr_greater_p(x,b)) {
      mpfr_sub(x, x, step, GMP_RNDN);
      mpfr_sub(step, b, x, GMP_RNDN);
      mpfr_set(x,b,GMP_RNDN);
    }
    evaluateFaithful(y2,tree,x,prec);
  }

  mpfr_set(result,sum,GMP_RNDU);

  mpfr_clear(x); mpfr_clear(y1); mpfr_clear(y2); mpfr_clear(step);
  mpfr_clear(sum); mpfr_clear(temp); 
  return;
}
