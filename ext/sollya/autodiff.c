/*

Copyright 2008-2011 by

Laboratoire de l'Informatique du Parallelisme,
UMR CNRS - ENS Lyon - UCB Lyon 1 - INRIA 5668,

LORIA (CNRS, INPL, INRIA, UHP, U-Nancy 2),

Laboratoire d'Informatique de Paris 6, equipe PEQUAN,
UPMC Universite Paris 06 - CNRS - UMR 7606 - LIP6, Paris, France

and by

Centre de recherche INRIA Sophia-Antipolis Mediterranee, equipe APICS,
Sophia Antipolis, France.

Contributors S. Chevillard, M. Joldes, Ch. Lauter

sylvain.chevillard@ens-lyon.org
mioara.joldes@ens-lyon.fr
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

#include "autodiff.h"
#include "general.h"
#include "infnorm.h"
#include "execute.h"
#include <stdlib.h>

/* The functions in AD manipulate arrays of size (n+1): [u0...un] */
/* Each array is supposed to represent a function u at a given    */
/* point x0. The value ui is a small sollya_mpfi_t such that             */
/*            u^(i)(x0)/i!  belongs to ui                         */


/* Apply Leibniz' formula: (uv)_p = sum(i=0..p, u_i * v_(p-i))    */
void multiplication_AD(sollya_mpfi_t *res, sollya_mpfi_t *f, sollya_mpfi_t *g, int n) {
  int i,j,p;
  sollya_mpfi_t temp;
  mp_prec_t prec;
  sollya_mpfi_t *temp_array;

  prec = getToolPrecision();
  sollya_mpfi_init2(temp, prec);

  temp_array = (sollya_mpfi_t *)safeCalloc((n+1),sizeof(sollya_mpfi_t));
  for(p=0;p<=n;p++) sollya_mpfi_init2(temp_array[p], prec);

  for(p=0; p<=n; p++) {
    sollya_mpfi_set_ui(temp_array[p], 0);
    i=0; j=p; 
    while(i<=p) {
      sollya_mpfi_mul(temp, f[i], g[j]);
      sollya_mpfi_add(temp_array[p], temp_array[p], temp);
      i++; j--;
    }
  }

  for(p=0; p<=n; p++) {
    sollya_mpfi_set(res[p], temp_array[p]);
    sollya_mpfi_clear(temp_array[p]);
  }
  free(temp_array);
  sollya_mpfi_clear(temp);
  return;
}

/* Generic recursive algorithm for the successive derivation of (g o f)  */
/* The array [f0...fn] represents a function f at point x0               */
/* The array [g0...gn] represents a function g at point f(x0)            */
/* Algo:                                                                 */
/*    If n==0, return [g0]                                               */
/*    Else, (g o f)^(i+1) = ((g o f)')^(i) = (f' * (g' o f))^(i) = h^(i) */
/*      So (g o f)^(i+1) / (i+1)! = (1/(i+1)) * h^(i)/i!                 */
/*      So we compute the array [w0 ... w(n-1)] corresponding to         */
/*      (g' o f), up to order (n-1) (by a recursive call)                */
/*      We apply multiplication_AD to w and [(1*f1) ... (n*fn)]          */
/*      (we remark that [(1*f1) ... (n*fn)] corresponds to f')           */
/*      This leads to an array [h0...h(n-1)] corresponding to h          */
/*      Finally, we return [g0 (h0/1) ... (h(n-1)/n)]                    */
void composition_AD(sollya_mpfi_t *res, sollya_mpfi_t *g, sollya_mpfi_t *f, int n) {
  sollya_mpfi_t *fprime, *gprime;
  sollya_mpfi_t *temp_array;
  int i;
  mp_prec_t prec;

  prec = getToolPrecision();
  if(n==0) sollya_mpfi_set(res[0], g[0]);
  else {
    temp_array = (sollya_mpfi_t *)safeCalloc(n,sizeof(sollya_mpfi_t));
    fprime = (sollya_mpfi_t *)safeCalloc(n,sizeof(sollya_mpfi_t));
    gprime = (sollya_mpfi_t *)safeCalloc(n,sizeof(sollya_mpfi_t));
    for(i=0;i<=n-1;i++) {
      sollya_mpfi_init2(temp_array[i], prec);
      sollya_mpfi_init2(fprime[i], prec);
      sollya_mpfi_init2(gprime[i], prec);

      sollya_mpfi_mul_ui(fprime[i], f[i+1], i+1);
      sollya_mpfi_mul_ui(gprime[i], g[i+1], i+1);
    }
    
    composition_AD(temp_array, gprime, f, n-1);
    multiplication_AD(res+1, temp_array, fprime, n-1);

    sollya_mpfi_set(res[0], g[0]);
    for(i=1; i<=n; i++) sollya_mpfi_div_ui(res[i], res[i], i);
    for(i=0; i<=n-1; i++) {
      sollya_mpfi_clear(temp_array[i]);
      sollya_mpfi_clear(fprime[i]);
      sollya_mpfi_clear(gprime[i]);
    }

    free(temp_array);
    free(fprime);
    free(gprime);
  }

  return ;
}


void binary_function_diff(sollya_mpfi_t *res, int nodeType, sollya_mpfi_t x0, node *f, node *g, int n, int *silent) {
  int i;
  sollya_mpfi_t *res1, *res2, *temp_array;
  mpfr_t minusOne;
  mp_prec_t prec;

  prec = getToolPrecision();
  res1 = (sollya_mpfi_t *)safeCalloc((n+1),sizeof(sollya_mpfi_t));
  res2 = (sollya_mpfi_t *)safeCalloc((n+1),sizeof(sollya_mpfi_t));
  for(i=0;i<=n;i++) {
    sollya_mpfi_init2(res1[i], prec);
    sollya_mpfi_init2(res2[i], prec);
  }
  auto_diff_scaled(res1, f, x0, n);
  auto_diff_scaled(res2, g, x0, n);

  switch(nodeType) {
  case ADD: 
    for(i=0; i<=n; i++) sollya_mpfi_add(res[i], res1[i], res2[i]);
    break;
  case SUB:
    for(i=0; i<=n; i++) sollya_mpfi_sub(res[i], res1[i], res2[i]);
    break;
  case MUL:
    multiplication_AD(res, res1, res2, n);
    break;
  case DIV: /* We compute it by g/h = g * h^{-1} */
    temp_array = (sollya_mpfi_t *)safeCalloc((n+1),sizeof(sollya_mpfi_t));
    for(i=0;i<=n;i++) sollya_mpfi_init2(temp_array[i], prec);

    /* temp_array corresponds to x->1/x at point h(x0) */
    mpfr_init2(minusOne, prec);  mpfr_set_si(minusOne, -1, GMP_RNDN);
    constantPower_diff(temp_array, res2[0], minusOne, n, silent);
    mpfr_clear(minusOne);

    /* temp_array corresponds to (x->1/x)(h) = 1/h */
    composition_AD(temp_array, temp_array, res2, n);

    /* res corresponds to g * 1/h */
    multiplication_AD(res, res1, temp_array, n);

    for(i=0;i<=n;i++) sollya_mpfi_clear(temp_array[i]);
    free(temp_array);
    break;

  default:
    sollyaFprintf(stderr, "Error in autodiff: unknown binary operator (%d)\n", nodeType);
    return;
  }
  
  for(i=0;i<=n;i++) {
    sollya_mpfi_clear(res1[i]);
    sollya_mpfi_clear(res2[i]);
  }
  free(res1);
  free(res2);
}


/* Computes the successive derivatives of y -> y^p at point x          */ 
/* [x^p/0!    p*x^(p-1)/1!   ...   p*(p-1)*...*(p-n+1)*x^(p-n)/n! ]    */
void constantPower_diff(sollya_mpfi_t *res, sollya_mpfi_t x, mpfr_t p, int n, int *silent) {
  sollya_mpfi_t expo, acc;
  mp_prec_t prec_expo, prec;
  int i;
  
  prec = getToolPrecision();
  prec_expo = (prec > mpfr_get_prec(p))?prec:mpfr_get_prec(p);

  sollya_mpfi_init2(expo, prec_expo);
  sollya_mpfi_init2(acc, prec);
  
  sollya_mpfi_set_fr(expo, p);
  sollya_mpfi_set_ui(acc, 1);
  
  for(i=0; i<=n; i++) {
    if (sollya_mpfi_is_zero(acc)) sollya_mpfi_set_ui(res[i],0);
    else {
      sollya_mpfi_pow(res[i], x, expo);
      sollya_mpfi_mul(res[i], res[i], acc);
      
      sollya_mpfi_mul(acc, acc, expo);
      sollya_mpfi_div_ui(acc, acc, i+1);
      sollya_mpfi_sub_ui(expo, expo, 1);
    }
  }

  sollya_mpfi_clear(expo);
  sollya_mpfi_clear(acc);

  return;
}


/* the power function is: p^x, where p is a positive constant */
/* [p^x/0!, log(p)p^x/1!, ... , log(p)^n p^x / n! ] */
void powerFunction_diff(sollya_mpfi_t *res, mpfr_t p, sollya_mpfi_t x, int n, int *silent) {
  int i;
  sollya_mpfi_t temp1,temp2;
  mp_prec_t prec;

  prec = getToolPrecision();
  sollya_mpfi_init2(temp1, prec);
  sollya_mpfi_init2(temp2, prec);

  sollya_mpfi_set_fr(temp1,p);

  sollya_mpfi_pow(temp2, temp1, x); /* temp2 = p^x */
  sollya_mpfi_log(temp1,temp1); /* temp1 = log(p) */

  for(i=0;i<=n;i++) {
    sollya_mpfi_set(res[i], temp2);
    sollya_mpfi_mul(temp2,temp2,temp1);
    sollya_mpfi_div_ui(temp2, temp2, i+1);
  }

  sollya_mpfi_clear(temp1);
  sollya_mpfi_clear(temp2);
  return;
}

void exp_diff(sollya_mpfi_t *res, sollya_mpfi_t x, int n, int *silent) {
  int i;
  sollya_mpfi_t temp;
  mp_prec_t prec;

  prec = getToolPrecision();
  sollya_mpfi_init2(temp, prec);

  sollya_mpfi_exp(temp, x);
  for(i=0;i<=n;i++) {
    sollya_mpfi_set(res[i], temp);
    sollya_mpfi_div_ui(temp, temp, i+1);
  }

  sollya_mpfi_clear(temp);
  return;
}

void expm1_diff(sollya_mpfi_t *res, sollya_mpfi_t x, int n, int *silent) {
  exp_diff(res, x, n, silent);
  sollya_mpfi_sub_ui(res[0], res[0], 1);
  return;
}


void log_diff(sollya_mpfi_t *res, sollya_mpfi_t x, int n, int *silent) {
  mpfr_t minusOne;
  mp_prec_t prec;
  int i;

  prec = getToolPrecision();

  sollya_mpfi_log(res[0], x);

  if(n>=1) {
    mpfr_init2(minusOne, prec);
    mpfr_set_si(minusOne, -1, GMP_RNDN);
    constantPower_diff(res+1, x, minusOne, n-1, silent);
    mpfr_clear(minusOne);
  }
  for(i=1;i<=n;i++) sollya_mpfi_div_ui(res[i], res[i], i);
  return;
}

void log1p_diff(sollya_mpfi_t *res, sollya_mpfi_t x, int n, int *silent) {
  mpfr_t minusOne;
  sollya_mpfi_t u;
  int i;
  mp_prec_t prec;

  prec = getToolPrecision();
  
  sollya_mpfi_log1p(res[0], x);

  if(n>=1) {
    sollya_mpfi_init2(u, prec);
    sollya_mpfi_add_ui(u, x, 1);
    mpfr_init2(minusOne, prec);
    mpfr_set_si(minusOne, -1, GMP_RNDN);
    constantPower_diff(res+1, u, minusOne, n-1, silent);
    mpfr_clear(minusOne);
    sollya_mpfi_clear(u);
  }

  for(i=1;i<=n;i++) sollya_mpfi_div_ui(res[i], res[i], i);

  return;
}

/* log2(x) = log(x) * (1/log(2)) */
void log2_diff(sollya_mpfi_t *res, sollya_mpfi_t x, int n, int *silent) {
  int i;
  sollya_mpfi_t log2;
  mp_prec_t prec;

  prec = getToolPrecision();
  sollya_mpfi_init2(log2, prec);

  sollya_mpfi_set_ui(log2, 2); sollya_mpfi_log(log2, log2);
  log_diff(res,x,n,silent);
  for(i=0;i<=n;i++) sollya_mpfi_div(res[i], res[i], log2);

  sollya_mpfi_clear(log2);
  return;
}

/* idem */
void log10_diff(sollya_mpfi_t *res, sollya_mpfi_t x, int n, int *silent) {
  int i;
  sollya_mpfi_t log10;
  mp_prec_t prec;

  prec = getToolPrecision();
  sollya_mpfi_init2(log10, prec);

  sollya_mpfi_set_ui(log10, 10); sollya_mpfi_log(log10, log10);
  log_diff(res,x,n,silent);
  for(i=0;i<=n;i++) sollya_mpfi_div(res[i], res[i], log10);

  sollya_mpfi_clear(log10);
  return;
}

void sin_diff(sollya_mpfi_t *res, sollya_mpfi_t x, int n, int *silent) {
  int i;
  mp_prec_t prec;

  prec = getToolPrecision();

  sollya_mpfi_sin(res[0], x); 
  for(i=2; i<=n; i+=2) sollya_mpfi_div_ui(res[i], res[i-2], (i-1)*i);
  for(i=2; i<=n; i +=4) sollya_mpfi_neg(res[i], res[i]);

  if(n>=1) {
    sollya_mpfi_cos(res[1], x); 
    for(i=3; i<=n; i+=2) sollya_mpfi_div_ui(res[i], res[i-2], (i-1)*i);
    for(i=3; i<=n; i +=4) sollya_mpfi_neg(res[i], res[i]);
  }

  return;
}

void cos_diff(sollya_mpfi_t *res, sollya_mpfi_t x, int n, int *silent) {
  int i;
  mp_prec_t prec;

  prec = getToolPrecision();

  sollya_mpfi_cos(res[0], x); 
  for(i=2; i<=n; i+=2) sollya_mpfi_div_ui(res[i], res[i-2], (i-1)*i);
  for(i=2; i<=n; i +=4) sollya_mpfi_neg(res[i], res[i]);

  if(n>=1) {
    sollya_mpfi_sin(res[1], x); 
    for(i=3; i<=n; i+=2) sollya_mpfi_div_ui(res[i], res[i-2], (i-1)*i);
    for(i=1; i<=n; i +=4) sollya_mpfi_neg(res[i], res[i]);
  }

  return;
}

void sinh_diff(sollya_mpfi_t *res, sollya_mpfi_t x, int n, int *silent) {
  int i;
  mp_prec_t prec;

  prec = getToolPrecision();

  sollya_mpfi_sinh(res[0], x); 
  for(i=2; i<=n; i+=2) sollya_mpfi_div_ui(res[i], res[i-2], (i-1)*i);

  if(n>=1) {
    sollya_mpfi_cosh(res[1], x); 
    for(i=3; i<=n; i+=2) sollya_mpfi_div_ui(res[i], res[i-2], (i-1)*i);
  }

  return;
}

void cosh_diff(sollya_mpfi_t *res, sollya_mpfi_t x, int n, int *silent) {
  int i;
  mp_prec_t prec;

  prec = getToolPrecision();

  sollya_mpfi_cosh(res[0], x); 
  for(i=2; i<=n; i+=2) sollya_mpfi_div_ui(res[i], res[i-2], (i-1)*i);

  if(n>=1) {
    sollya_mpfi_sinh(res[1], x); 
    for(i=3; i<=n; i+=2) sollya_mpfi_div_ui(res[i], res[i-2], (i-1)*i);
  }

  return;
}


/* Takes a polynomial given by the array of its coefficients [p0...pn] 
   and differentiates it (returns the array of the derivative) */

/* It IS safe to use the same pointer for res and coeff_array */
/* (in-place computation) */
void symbolic_poly_diff(sollya_mpfi_t *res, sollya_mpfi_t *coeff_array, int degree) {
  int i;

  for(i=0;i<=degree-1;i++) sollya_mpfi_mul_ui(res[i], coeff_array[i+1], i+1);
}

/* Evaluates a symbolic polynomial at point x by Horner scheme */
void symbolic_poly_evaluation_horner(sollya_mpfi_t res, sollya_mpfi_t *coeffs_array, sollya_mpfi_t x, int degree) {
  int i;
  sollya_mpfi_t temp;
  mp_prec_t prec;

  prec = getToolPrecision();
  sollya_mpfi_init2(temp, prec);

  sollya_mpfi_set(temp, coeffs_array[degree]);
  for(i=degree-1;i>=0;i--) {
    sollya_mpfi_mul(temp, temp, x);
    sollya_mpfi_add(temp, temp, coeffs_array[i]);
  }
  sollya_mpfi_set(res, temp);
  sollya_mpfi_clear(temp);
}

/* Evaluates a symbolic polynomial at point x by computing successive powers */
void symbolic_poly_evaluation_powers(sollya_mpfi_t res, sollya_mpfi_t *coeffs_array, sollya_mpfi_t *powers_array, sollya_mpfi_t x, int degree) {
  int i;
  sollya_mpfi_t temp, acc;
  mp_prec_t prec;

  prec = getToolPrecision();
  sollya_mpfi_init2(temp, prec);
  sollya_mpfi_init2(acc, prec);

  sollya_mpfi_set_ui(acc, 0);
  for(i=0;i<=degree;i++) {
    sollya_mpfi_mul(temp, coeffs_array[i], powers_array[i]);
    sollya_mpfi_add(acc, acc, temp);
  }
  sollya_mpfi_set(res, acc);

  sollya_mpfi_clear(temp);
  sollya_mpfi_clear(acc);
}


/*  u=tan(x), tan^(n) / n! = p_(n)(u) with
    p_0 = u;

recurrence formula: p_(n+1)(u) = (p_n(u))' / (n+1) = p'_n(u) * (1+u^2) / (n+1)
   -> p_n of degree n+1
*/

void tan_diff(sollya_mpfi_t *res, sollya_mpfi_t x, int n, int *silent) {
  int i,index;
  sollya_mpfi_t *coeffs_array;
  sollya_mpfi_t u;
  mp_prec_t prec;

  prec = getToolPrecision();
  coeffs_array = (sollya_mpfi_t *)safeCalloc( (n+2),sizeof(sollya_mpfi_t));

  for (index=0; index<=n+1; index++) {
    sollya_mpfi_init2(coeffs_array[index], prec);
    sollya_mpfi_set_ui(coeffs_array[index], 0);
  }
  sollya_mpfi_init2(u, prec);

  sollya_mpfi_tan(u, x);
  sollya_mpfi_set_ui(coeffs_array[0], 0);
  sollya_mpfi_set_ui(coeffs_array[1], 1);
  
  sollya_mpfi_set(res[0], u);

  for(index=1; index<=n; index++) {
    /* coeffs_array represents p_(index-1) */
    
    symbolic_poly_diff(coeffs_array, coeffs_array, index);
    sollya_mpfi_set_ui(coeffs_array[index], 0);
    /* now it represents p_(index-1)' */

    for(i=index+1; i>=2; i--) {
      sollya_mpfi_add(coeffs_array[i], coeffs_array[i], coeffs_array[i-2]);
      sollya_mpfi_div_ui(coeffs_array[i], coeffs_array[i], index);
    }
    sollya_mpfi_div_ui(coeffs_array[1], coeffs_array[1], index);
    sollya_mpfi_div_ui(coeffs_array[0], coeffs_array[0], index);
    /* now it represents p_(index) */

    symbolic_poly_evaluation_horner(res[index], coeffs_array, u, index+1);
  }
    
  for (index=0; index<=n+1; index++){
    sollya_mpfi_clear(coeffs_array[index]);
  }
  sollya_mpfi_clear(u);
  free(coeffs_array);
  
  return;
}

/*  u=tanh(x), tanh^(n) / n! = p_(n)(u) with
    p_0 = u;

recurrence formula: p_(n+1)(u) = (p_n(u))' / (n+1) = p'_n(u) * (1-u^2) / (n+1)
   -> p_n of degree n+1
*/

void tanh_diff(sollya_mpfi_t *res, sollya_mpfi_t x, int n, int *silent) {
  int i,index;
  sollya_mpfi_t *coeffs_array;
  sollya_mpfi_t u;
  mp_prec_t prec;

  prec = getToolPrecision();
  coeffs_array = (sollya_mpfi_t *)safeCalloc( (n+2),sizeof(sollya_mpfi_t));

  for (index=0; index<=n+1; index++) {
    sollya_mpfi_init2(coeffs_array[index], prec);
    sollya_mpfi_set_ui(coeffs_array[index], 0);
  }
  sollya_mpfi_init2(u, prec);

  sollya_mpfi_tanh(u, x);
  sollya_mpfi_set_ui(coeffs_array[0], 0);
  sollya_mpfi_set_ui(coeffs_array[1], 1);
  
  sollya_mpfi_set(res[0], u);

  for(index=1; index<=n; index++) {
    /* coeffs_array represents p_(index-1) */
    
    symbolic_poly_diff(coeffs_array, coeffs_array, index);
    sollya_mpfi_set_ui(coeffs_array[index], 0);
    /* now it represents p_(index-1)' */

    for(i=index+1; i>=2; i--) {
      sollya_mpfi_sub(coeffs_array[i], coeffs_array[i], coeffs_array[i-2]);
      sollya_mpfi_div_ui(coeffs_array[i], coeffs_array[i], index);
    }
    sollya_mpfi_div_ui(coeffs_array[1], coeffs_array[1], index);
    sollya_mpfi_div_ui(coeffs_array[0], coeffs_array[0], index);
    /* now it represents p_(index) */

    symbolic_poly_evaluation_horner(res[index], coeffs_array, u, index+1);
  }
    
  for (index=0; index<=n+1; index++){
    sollya_mpfi_clear(coeffs_array[index]);
  }
  sollya_mpfi_clear(u);
  free(coeffs_array);
  
  return;
}


/* atan_diff : reccurence formula: p_(n+1) = (p'_n * (1+x^2) - 2nx * p_n) / (n+1)
   atan^(0) = atan(x)
   atan^(n) / n! = p_(n)(x)/((1+x^2)^n)
   p_1=1;

   --> degree of p_n is (n-1)
*/
void atan_diff(sollya_mpfi_t *res, sollya_mpfi_t x, int n, int *silent) {
  int i,index;
  sollya_mpfi_t *coeffs_array, *coeffs_array_diff;
  sollya_mpfi_t u, temp;

  mp_prec_t prec;
    
  prec = getToolPrecision();
  coeffs_array = (sollya_mpfi_t *)safeCalloc( n,sizeof(sollya_mpfi_t));
  coeffs_array_diff = (sollya_mpfi_t *)safeCalloc( n,sizeof(sollya_mpfi_t));

  for (index=0; index<=n-1; index++) {
    sollya_mpfi_init2(coeffs_array[index], prec);
    sollya_mpfi_init2(coeffs_array_diff[index], prec);

    sollya_mpfi_set_ui(coeffs_array[index], 0);
    sollya_mpfi_set_ui(coeffs_array_diff[index], 0);
  }

  sollya_mpfi_init2(u, prec);
  sollya_mpfi_init2(temp, prec);

  sollya_mpfi_atan(res[0], x);

  if(n>=1) {
    sollya_mpfi_sqr(u, x);
    sollya_mpfi_add_ui(u, u, 1);

    sollya_mpfi_inv(res[1], u);

    sollya_mpfi_set_ui(coeffs_array[0], 1);
  }

  for(index=2; index<=n; index++) {
    /* coeffs_array represents p_(index-1) */
    
    symbolic_poly_diff(coeffs_array_diff, coeffs_array, index-2);
    sollya_mpfi_set_ui(coeffs_array_diff[index-2], 0);
    /* now it represents p_(index-1)' */

    for(i=index-1; i>=2; i--) {
      sollya_mpfi_add(coeffs_array[i], coeffs_array_diff[i], coeffs_array_diff[i-2]);
      sollya_mpfi_mul_ui(temp, coeffs_array[i-1], 2*(index-1));
      sollya_mpfi_sub(coeffs_array[i], coeffs_array[i], temp);
      sollya_mpfi_div_ui(coeffs_array[i], coeffs_array[i], index);
    }

    sollya_mpfi_mul_ui(temp, coeffs_array[0], 2*(index-1));
    sollya_mpfi_sub(coeffs_array[1], coeffs_array_diff[1], temp);
    sollya_mpfi_div_ui(coeffs_array[1], coeffs_array[1], index);

    sollya_mpfi_set(coeffs_array[0], coeffs_array_diff[0]);
    sollya_mpfi_div_ui(coeffs_array[0], coeffs_array[0], index);
    /* now it represents p_(index) */

    symbolic_poly_evaluation_horner(res[index], coeffs_array, x, index-1);
    sollya_mpfi_set_ui(temp, index);
    sollya_mpfi_pow(temp, u, temp);
    sollya_mpfi_div(res[index], res[index], temp);
  }
    
  for (index=0; index<=n-1; index++){
    sollya_mpfi_clear(coeffs_array[index]);
    sollya_mpfi_clear(coeffs_array_diff[index]);
  }
  sollya_mpfi_clear(u);
  sollya_mpfi_clear(temp);
  free(coeffs_array);
  free(coeffs_array_diff);
  
  return;
}


/* atanh_diff : reccurence formula: p_(n+1) = (p'_n * (1-x^2) + 2nx * p_n)/ (n+1)
   atanh^(0) = atanh(x)
   atanh^(n)/n! = p_(n)(x)/((1-x^2)^n)
   p_1=1;

   --> degree of p_n is (n-1)
*/
void atanh_diff(sollya_mpfi_t *res, sollya_mpfi_t x, int n, int *silent) {
  int i,index;
  sollya_mpfi_t *coeffs_array, *coeffs_array_diff;
  sollya_mpfi_t u, temp;

  mp_prec_t prec;
    
  prec = getToolPrecision();
  coeffs_array = (sollya_mpfi_t *)safeCalloc( n,sizeof(sollya_mpfi_t));
  coeffs_array_diff = (sollya_mpfi_t *)safeCalloc( n,sizeof(sollya_mpfi_t));

  for (index=0; index<=n-1; index++) {
    sollya_mpfi_init2(coeffs_array[index], prec);
    sollya_mpfi_init2(coeffs_array_diff[index], prec);

    sollya_mpfi_set_ui(coeffs_array[index], 0);
    sollya_mpfi_set_ui(coeffs_array_diff[index], 0);
  }

  sollya_mpfi_init2(u, prec);
  sollya_mpfi_init2(temp, prec);

  sollya_mpfi_atanh(res[0], x);

  if(n>=1) {
    sollya_mpfi_sqr(u, x);
    sollya_mpfi_sub_ui(u, u, 1); /* TODO: FIX IT when MPFI is patched */
    sollya_mpfi_neg(u, u);

    sollya_mpfi_inv(res[1], u);

    sollya_mpfi_set_ui(coeffs_array[0], 1);
  }

  for(index=2; index<=n; index++) {
    /* coeffs_array represents p_(index-1) */
    
    symbolic_poly_diff(coeffs_array_diff, coeffs_array, index-2);
    sollya_mpfi_set_ui(coeffs_array_diff[index-2], 0);
    /* now it represents p_(index-1)' */

    for(i=index-1; i>=2; i--) {
      sollya_mpfi_sub(coeffs_array[i], coeffs_array_diff[i], coeffs_array_diff[i-2]);
      sollya_mpfi_mul_ui(temp, coeffs_array[i-1], 2*(index-1));
      sollya_mpfi_add(coeffs_array[i], coeffs_array[i], temp);
      sollya_mpfi_div_ui(coeffs_array[i], coeffs_array[i], index);
    }

    sollya_mpfi_mul_ui(temp, coeffs_array[0], 2*(index-1));
    sollya_mpfi_add(coeffs_array[1], coeffs_array_diff[1], temp);
    sollya_mpfi_div_ui(coeffs_array[1], coeffs_array[1], index);

    sollya_mpfi_set(coeffs_array[0], coeffs_array_diff[0]);
    sollya_mpfi_div_ui(coeffs_array[0], coeffs_array[0], index);
    /* now it represents p_(index) */

    symbolic_poly_evaluation_horner(res[index], coeffs_array, x, index-1);
    sollya_mpfi_set_ui(temp, index);
    sollya_mpfi_pow(temp, u, temp);
    sollya_mpfi_div(res[index], res[index], temp);
  }
    
  for (index=0; index<=n-1; index++){
    sollya_mpfi_clear(coeffs_array[index]);
    sollya_mpfi_clear(coeffs_array_diff[index]);
  }
  sollya_mpfi_clear(u);
  sollya_mpfi_clear(temp);
  free(coeffs_array);
  free(coeffs_array_diff);
  
  return;
}



/* asin_diff : recurrence formula: p_(n+1)= (p'_n * (1-x^2) + (2n-1)x * p_n)/(n+1)
   asin^(0) = asin(x)
   asin^(n) / n! = p_(n)(x) / (1-x^2)^((2n-1)/2)
   p_1=1;

   --> degree of p_n is (n-1)
*/
void asin_diff(sollya_mpfi_t *res, sollya_mpfi_t x, int n, int *silent) {
  int i,index;
  sollya_mpfi_t *coeffs_array, *coeffs_array_diff;
  sollya_mpfi_t u, temp;

  mp_prec_t prec;
    
  prec = getToolPrecision();
  coeffs_array = (sollya_mpfi_t *)safeCalloc( n,sizeof(sollya_mpfi_t));
  coeffs_array_diff = (sollya_mpfi_t *)safeCalloc( n,sizeof(sollya_mpfi_t));

  for (index=0; index<=n-1; index++) {
    sollya_mpfi_init2(coeffs_array[index], prec);
    sollya_mpfi_init2(coeffs_array_diff[index], prec);

    sollya_mpfi_set_ui(coeffs_array[index], 0);
    sollya_mpfi_set_ui(coeffs_array_diff[index], 0);
  }

  sollya_mpfi_init2(u, prec);
  sollya_mpfi_init2(temp, prec);

  sollya_mpfi_asin(res[0], x);

  if(n>=1) {
    sollya_mpfi_sqr(u, x);
    sollya_mpfi_sub_ui(u, u, 1); /* TODO: FIX IT when MPFI is patched */
    sollya_mpfi_neg(u, u);
    sollya_mpfi_sqrt(u, u);

    sollya_mpfi_inv(res[1], u);

    sollya_mpfi_set_ui(coeffs_array[0], 1);
  }

  for(index=2; index<=n; index++) {
    /* coeffs_array represents p_(index-1) */
    
    symbolic_poly_diff(coeffs_array_diff, coeffs_array, index-2);
    sollya_mpfi_set_ui(coeffs_array_diff[index-2], 0);
    /* now it represents p_(index-1)' */

    for(i=index-1; i>=2; i--) {
      sollya_mpfi_sub(coeffs_array[i], coeffs_array_diff[i], coeffs_array_diff[i-2]);
      sollya_mpfi_mul_ui(temp, coeffs_array[i-1], 2*(index-1)-1);
      sollya_mpfi_add(coeffs_array[i], coeffs_array[i], temp);
      sollya_mpfi_div_ui(coeffs_array[i], coeffs_array[i], index);
    }

    sollya_mpfi_mul_ui(temp, coeffs_array[0], 2*(index-1)-1);
    sollya_mpfi_add(coeffs_array[1], coeffs_array_diff[1], temp);
    sollya_mpfi_div_ui(coeffs_array[1], coeffs_array[1], index);

    sollya_mpfi_set(coeffs_array[0], coeffs_array_diff[0]);
    sollya_mpfi_div_ui(coeffs_array[0], coeffs_array[0], index);
    /* now it represents p_(index) */

    symbolic_poly_evaluation_horner(res[index], coeffs_array, x, index-1);
    sollya_mpfi_set_ui(temp, 2*index-1);
    sollya_mpfi_pow(temp, u, temp);
    sollya_mpfi_div(res[index], res[index], temp);
  }
    
  for (index=0; index<=n-1; index++){
    sollya_mpfi_clear(coeffs_array[index]);
    sollya_mpfi_clear(coeffs_array_diff[index]);
  }
  sollya_mpfi_clear(u);
  sollya_mpfi_clear(temp);
  free(coeffs_array);
  free(coeffs_array_diff);
  
  return;
}


/* acos_diff : except for the res[0], all the terms are equal to -asin^(n)(x)/n! */
void acos_diff(sollya_mpfi_t *res, sollya_mpfi_t x, int n, int *silent) {
  int i; 

  asin_diff(res,x,n,silent);

  sollya_mpfi_acos(res[0], x);

  for (i=1; i<=n;i++)  sollya_mpfi_neg(res[i], res[i]);

  return;
}


/* asinh_diff : recurrence formula: p_(n+1) = (p'_n * (1+x^2) - (2n-1)x * p_n) / (n+1) 
   asinh^(0) = asinh(x)
   asinh^(n)/n! = p_(n)(x) / (1+x^2)^((2n-1)/2)
   p_1=1;

   --> degree of p_n is (n-1)
*/
void asinh_diff(sollya_mpfi_t *res, sollya_mpfi_t x, int n, int *silent) {
  int i,index;
  sollya_mpfi_t *coeffs_array, *coeffs_array_diff;
  sollya_mpfi_t u, temp;

  mp_prec_t prec;
    
  prec = getToolPrecision();
  coeffs_array = (sollya_mpfi_t *)safeCalloc( n,sizeof(sollya_mpfi_t));
  coeffs_array_diff = (sollya_mpfi_t *)safeCalloc( n,sizeof(sollya_mpfi_t));

  for (index=0; index<=n-1; index++) {
    sollya_mpfi_init2(coeffs_array[index], prec);
    sollya_mpfi_init2(coeffs_array_diff[index], prec);

    sollya_mpfi_set_ui(coeffs_array[index], 0);
    sollya_mpfi_set_ui(coeffs_array_diff[index], 0);
  }

  sollya_mpfi_init2(u, prec);
  sollya_mpfi_init2(temp, prec);

  sollya_mpfi_asinh(res[0], x);

  if(n>=1) {
    sollya_mpfi_sqr(u, x);
    sollya_mpfi_add_ui(u, u, 1);
    sollya_mpfi_sqrt(u, u);

    sollya_mpfi_inv(res[1], u);

    sollya_mpfi_set_ui(coeffs_array[0], 1);
  }

  for(index=2; index<=n; index++) {
    /* coeffs_array represents p_(index-1) */
    
    symbolic_poly_diff(coeffs_array_diff, coeffs_array, index-2);
    sollya_mpfi_set_ui(coeffs_array_diff[index-2], 0);
    /* now it represents p_(index-1)' */

    for(i=index-1; i>=2; i--) {
      sollya_mpfi_add(coeffs_array[i], coeffs_array_diff[i], coeffs_array_diff[i-2]);
      sollya_mpfi_mul_ui(temp, coeffs_array[i-1], 2*(index-1)-1);
      sollya_mpfi_sub(coeffs_array[i], coeffs_array[i], temp);
      sollya_mpfi_div_ui(coeffs_array[i], coeffs_array[i], index);
    }

    sollya_mpfi_mul_ui(temp, coeffs_array[0], 2*(index-1)-1);
    sollya_mpfi_sub(coeffs_array[1], coeffs_array_diff[1], temp);
    sollya_mpfi_div_ui(coeffs_array[1], coeffs_array[1], index);

    sollya_mpfi_set(coeffs_array[0], coeffs_array_diff[0]);
    sollya_mpfi_div_ui(coeffs_array[0], coeffs_array[0], index);
    /* now it represents p_(index) */

    symbolic_poly_evaluation_horner(res[index], coeffs_array, x, index-1);
    sollya_mpfi_set_ui(temp, 2*index-1);
    sollya_mpfi_pow(temp, u, temp);
    sollya_mpfi_div(res[index], res[index], temp);
  }
    
  for (index=0; index<=n-1; index++){
    sollya_mpfi_clear(coeffs_array[index]);
    sollya_mpfi_clear(coeffs_array_diff[index]);
  }
  sollya_mpfi_clear(u);
  sollya_mpfi_clear(temp);
  free(coeffs_array);
  free(coeffs_array_diff);
  
  return;
}


/* acosh_diff : recurrence formula: p_(n+1) = (p'_n * (x^2-1) - (2n-1)x * p_n) / (n+1)
   acosh^(0) = acosh(x)
   acosh^(n)/n! = p_(n)(x) / (x^2-1)^((2n-1)/2)
   p_1=1;

   --> degree of p_n is (n-1)
*/
void acosh_diff(sollya_mpfi_t *res, sollya_mpfi_t x, int n, int *silent) {
  int i,index;
  sollya_mpfi_t *coeffs_array, *coeffs_array_diff;
  sollya_mpfi_t u, temp;

  mp_prec_t prec;
    
  prec = getToolPrecision();
  coeffs_array = (sollya_mpfi_t *)safeCalloc( n,sizeof(sollya_mpfi_t));
  coeffs_array_diff = (sollya_mpfi_t *)safeCalloc( n,sizeof(sollya_mpfi_t));

  for (index=0; index<=n-1; index++) {
    sollya_mpfi_init2(coeffs_array[index], prec);
    sollya_mpfi_init2(coeffs_array_diff[index], prec);

    sollya_mpfi_set_ui(coeffs_array[index], 0);
    sollya_mpfi_set_ui(coeffs_array_diff[index], 0);
  }

  sollya_mpfi_init2(u, prec);
  sollya_mpfi_init2(temp, prec);

  sollya_mpfi_acosh(res[0], x);

  if(n>=1) {
    sollya_mpfi_sqr(u, x);
    sollya_mpfi_sub_ui(u, u, 1);
    sollya_mpfi_sqrt(u, u);

    sollya_mpfi_inv(res[1], u);

    sollya_mpfi_set_ui(coeffs_array[0], 1);
  }

  for(index=2; index<=n; index++) {
    /* coeffs_array represents p_(index-1) */
    
    symbolic_poly_diff(coeffs_array_diff, coeffs_array, index-2);
    sollya_mpfi_set_ui(coeffs_array_diff[index-2], 0);
    /* now it represents p_(index-1)' */

    for(i=index-1; i>=2; i--) {
      sollya_mpfi_sub(coeffs_array[i], coeffs_array_diff[i-2], coeffs_array_diff[i]);
      sollya_mpfi_mul_ui(temp, coeffs_array[i-1], 2*(index-1)-1);
      sollya_mpfi_sub(coeffs_array[i], coeffs_array[i], temp);
      sollya_mpfi_div_ui(coeffs_array[i], coeffs_array[i], index);
    }

    sollya_mpfi_mul_ui(temp, coeffs_array[0], 2*(index-1)-1);
    sollya_mpfi_add(coeffs_array[1], temp, coeffs_array_diff[1]);
    sollya_mpfi_neg(coeffs_array[1], coeffs_array[1]);
    sollya_mpfi_div_ui(coeffs_array[1], coeffs_array[1], index);

    sollya_mpfi_neg(coeffs_array[0], coeffs_array_diff[0]);
    sollya_mpfi_div_ui(coeffs_array[0], coeffs_array[0], index);
    /* now it represents p_(index) */

    symbolic_poly_evaluation_horner(res[index], coeffs_array, x, index-1);
    sollya_mpfi_set_ui(temp, 2*index-1);
    sollya_mpfi_pow(temp, u, temp);
    sollya_mpfi_div(res[index], res[index], temp);
  }
    
  for (index=0; index<=n-1; index++){
    sollya_mpfi_clear(coeffs_array[index]);
    sollya_mpfi_clear(coeffs_array_diff[index]);
  }
  sollya_mpfi_clear(u);
  sollya_mpfi_clear(temp);
  free(coeffs_array);
  free(coeffs_array_diff);
  
  return;
}

/* erf^(n)(x)/n! = p_n(x)*e^(-x^2)             */
/* with p_1(x) = 2/sqrt(pi)                    */
/* and p_(n+1)(x) = (p_n'(x) - 2xp_n(x))/(n+1) */
/*  -> degree of p_n is n-1                    */
void erf_diff(sollya_mpfi_t *res, sollya_mpfi_t x, int n, int *silent) {
  int i,index;
  sollya_mpfi_t *coeffs_array, *coeffs_array_diff;
  sollya_mpfi_t u, temp;

  mp_prec_t prec;
    
  prec = getToolPrecision();
  coeffs_array = (sollya_mpfi_t *)safeCalloc( n,sizeof(sollya_mpfi_t));
  coeffs_array_diff = (sollya_mpfi_t *)safeCalloc( n,sizeof(sollya_mpfi_t));

  for (index=0; index<=n-1; index++) {
    sollya_mpfi_init2(coeffs_array[index], prec);
    sollya_mpfi_init2(coeffs_array_diff[index], prec);

    sollya_mpfi_set_ui(coeffs_array[index], 0);
    sollya_mpfi_set_ui(coeffs_array_diff[index], 0);
  }

  sollya_mpfi_init2(u, prec);
  sollya_mpfi_init2(temp, prec);

  sollya_mpfi_erf(res[0], x);

  if(n>=1) {
    sollya_mpfi_const_pi(temp);
    sollya_mpfi_sqrt(temp, temp);
    sollya_mpfi_ui_div(temp, 2, temp);

    sollya_mpfi_sqr(u, x);
    sollya_mpfi_neg(u, u);
    sollya_mpfi_exp(u, u);

    sollya_mpfi_mul(res[1], temp, u);

    sollya_mpfi_set(coeffs_array[0], temp);
  }

  for(index=2; index<=n; index++) {
    /* coeffs_array represents p_(index-1) */
    
    symbolic_poly_diff(coeffs_array_diff, coeffs_array, index-2);
    sollya_mpfi_set_ui(coeffs_array_diff[index-2], 0);
    /* now it represents p_(index-1)' */

    for(i=index-1; i>=1; i--) {
      sollya_mpfi_mul_ui(temp, coeffs_array[i-1], 2);
      sollya_mpfi_sub(coeffs_array[i], coeffs_array_diff[i], temp);
      sollya_mpfi_div_ui(coeffs_array[i], coeffs_array[i], index);
    }

    sollya_mpfi_set(coeffs_array[0], coeffs_array_diff[0]);
    sollya_mpfi_div_ui(coeffs_array[0], coeffs_array[0], index);
    /* now it represents p_(index) */

    symbolic_poly_evaluation_horner(res[index], coeffs_array, x, index-1);
    sollya_mpfi_mul(res[index], res[index], u);
  }
    
  for (index=0; index<=n-1; index++){
    sollya_mpfi_clear(coeffs_array[index]);
    sollya_mpfi_clear(coeffs_array_diff[index]);
  }
  sollya_mpfi_clear(u);
  sollya_mpfi_clear(temp);
  free(coeffs_array);
  free(coeffs_array_diff);
  
  return;  
}

void erfc_diff(sollya_mpfi_t *res, sollya_mpfi_t x, int n, int *silent) {
  int i; 

  erf_diff(res, x, n, silent);

  sollya_mpfi_erfc(res[0], x);

  for (i=1; i<=n;i++)  sollya_mpfi_neg(res[i],res[i]);

  return;
}

void abs_diff(sollya_mpfi_t *res, sollya_mpfi_t x, int n, int *silent) {
  int i;
  mpfr_t temp2;
  mp_prec_t prec;

  prec = getToolPrecision();

  sollya_mpfi_abs(res[0], x);
  if(n >= 1) {
    if (sollya_mpfi_has_zero(x))  sollya_mpfi_interv_si(res[1], -1, 1);
    else sollya_mpfi_set_si(res[1], sollya_mpfi_is_nonneg(x) ? 1 : (-1));
  }

  if(n >= 2) {
    mpfr_init2(temp2, prec);
    mpfr_set_nan(temp2);

    if (!(*silent)) {
      *silent = 1;
      printMessage(1, "Warning: the absolute value is not twice differentiable.\n");
      printMessage(1, "Will return [@NaN@, @NaN@].\n");
    }
    for(i=2;i<=n;i++) sollya_mpfi_set_fr(res[i], temp2);
    mpfr_clear(temp2);
  }

  return;
}

void single_diff(sollya_mpfi_t *res, sollya_mpfi_t x, int n, int *silent){
  int i;
  mpfr_t temp;
  mp_prec_t prec;

  prec = getToolPrecision();
  mpfr_init2(temp, prec);
  mpfr_set_nan(temp);

  if (!(*silent)) {
    *silent = 1;
    printMessage(1, "Warning: the single rounding operator is not differentiable.\n");
    printMessage(1, "Will return [@NaN@, @NaN@].\n");
  }
  for(i=0;i<=n;i++) sollya_mpfi_set_fr(res[i], temp);
  mpfr_clear(temp);
}

void quad_diff(sollya_mpfi_t *res, sollya_mpfi_t x, int n, int *silent){
  int i;
  mpfr_t temp;
  mp_prec_t prec;

  prec = getToolPrecision();
  mpfr_init2(temp, prec);
  mpfr_set_nan(temp);

  if (!(*silent)) {
    *silent = 1;
    printMessage(1, "Warning: the quad rounding operator is not differentiable.\n");
    printMessage(1, "Will return [@NaN@, @NaN@].\n");
  }
  for(i=0;i<=n;i++) sollya_mpfi_set_fr(res[i], temp);
  mpfr_clear(temp);
}

void halfprecision_diff(sollya_mpfi_t *res, sollya_mpfi_t x, int n, int *silent){
  int i;
  mpfr_t temp;
  mp_prec_t prec;

  prec = getToolPrecision();
  mpfr_init2(temp, prec);
  mpfr_set_nan(temp);

  if (!(*silent)) {
    *silent = 1;
    printMessage(1, "Warning: the half-precision rounding operator is not differentiable.\n");
    printMessage(1, "Will return [@NaN@, @NaN@].\n");
  }
  for(i=0;i<=n;i++) sollya_mpfi_set_fr(res[i], temp);
  mpfr_clear(temp);
}

void double_diff(sollya_mpfi_t *res, sollya_mpfi_t x, int n, int *silent){
  int i;
  mpfr_t temp;
  mp_prec_t prec;

  prec = getToolPrecision();
  mpfr_init2(temp, prec);
  mpfr_set_nan(temp);

  if (!(*silent)) {
    *silent = 1;
    printMessage(1, "Warning: the double rounding operator is not differentiable.\n");
    printMessage(1, "Will return [@NaN@, @NaN@].\n");
  }
  for(i=0;i<=n;i++) sollya_mpfi_set_fr(res[i], temp);
  mpfr_clear(temp);
}

void double_double_diff(sollya_mpfi_t *res, sollya_mpfi_t x, int n, int *silent){
  int i;
  mpfr_t temp;
  mp_prec_t prec;

  prec = getToolPrecision();
  mpfr_init2(temp, prec);
  mpfr_set_nan(temp);

  if (!(*silent)) {
    *silent = 1;
    printMessage(1, "Warning: the doubledouble rounding operator is not differentiable.\n");
    printMessage(1, "Will return [@NaN@, @NaN@].\n");
  }
  for(i=0;i<=n;i++) sollya_mpfi_set_fr(res[i], temp);
  mpfr_clear(temp);
}

void triple_double_diff(sollya_mpfi_t *res, sollya_mpfi_t x, int n, int *silent){
  int i;
  mpfr_t temp;
  mp_prec_t prec;

  prec = getToolPrecision();
  mpfr_init2(temp, prec);
  mpfr_set_nan(temp);

  if (!(*silent)) {
    *silent = 1;
    printMessage(1, "Warning: the tripledouble rounding operator is not differentiable.\n");
    printMessage(1, "Will return [@NaN@, @NaN@].\n");
  }
  for(i=0;i<=n;i++) sollya_mpfi_set_fr(res[i], temp);
  mpfr_clear(temp);
}

void double_extended_diff(sollya_mpfi_t *res, sollya_mpfi_t x, int n, int *silent){
  int i;
  mpfr_t temp;
  mp_prec_t prec;

  prec = getToolPrecision();
  mpfr_init2(temp, prec);
  mpfr_set_nan(temp);

  if (!(*silent)) {
    *silent = 1;
    printMessage(1, "Warning: the doubleextended rounding operator is not differentiable.\n");
    printMessage(1, "Will return [@NaN@, @NaN@].\n");
  }
  for(i=0;i<=n;i++) sollya_mpfi_set_fr(res[i], temp);
  mpfr_clear(temp);
}

void ceil_diff(sollya_mpfi_t *res, sollya_mpfi_t x, int n, int *silent){
  int i;
  mpfr_t temp;
  mp_prec_t prec;

  prec = getToolPrecision();
  mpfr_init2(temp, prec);
  mpfr_set_nan(temp);

  if (!(*silent)) {
    *silent = 1;
    printMessage(1, "Warning: the ceil operator is not differentiable.\n");
    printMessage(1, "Will return [@NaN@, @NaN@].\n");
  }
  for(i=0;i<=n;i++) sollya_mpfi_set_fr(res[i], temp);
  mpfr_clear(temp);
}

void floor_diff(sollya_mpfi_t *res, sollya_mpfi_t x, int n, int *silent){
  int i;
  mpfr_t temp;
  mp_prec_t prec;

  prec = getToolPrecision();
  mpfr_init2(temp, prec);
  mpfr_set_nan(temp);

  if (!(*silent)) {
    *silent = 1;
    printMessage(1, "Warning: the floor operator is not differentiable.\n");
    printMessage(1, "Will return [@NaN@, @NaN@].\n");
  }
  for(i=0;i<=n;i++) sollya_mpfi_set_fr(res[i], temp);
  mpfr_clear(temp);
}

void nearestint_diff(sollya_mpfi_t *res, sollya_mpfi_t x, int n, int *silent){
  int i;
  mpfr_t temp;
  mp_prec_t prec;

  prec = getToolPrecision();
  mpfr_init2(temp, prec);
  mpfr_set_nan(temp);

  if (!(*silent)) {
    *silent = 1;
    printMessage(1, "Warning: the nearestint operator is not differentiable.\n");
    printMessage(1, "Will return [@NaN@, @NaN@].\n");
  }
  for(i=0;i<=n;i++) sollya_mpfi_set_fr(res[i], temp);
  mpfr_clear(temp);
}

void libraryFunction_diff(sollya_mpfi_t *res, node *f, sollya_mpfi_t x, int n, int *silent) {
  sollya_mpfi_t fact;
  mp_prec_t prec;
  int i;

  prec = getToolPrecision();
  sollya_mpfi_init2(fact, prec);
  sollya_mpfi_set_ui(fact, 1);

  for(i=0;i<=n;i++) {
    f->libFun->code(res[i], x, f->libFunDeriv + i);
    sollya_mpfi_div(res[i], res[i], fact);
    sollya_mpfi_mul_ui(fact, fact, i+1);
  }
  sollya_mpfi_clear(fact);
}

void procedureFunction_diff(sollya_mpfi_t *res, node *f, sollya_mpfi_t x, int n, int *silent) {
  sollya_mpfi_t fact;
  mp_prec_t prec;
  int i;

  prec = getToolPrecision();
  sollya_mpfi_init2(fact, prec);
  sollya_mpfi_set_ui(fact, 1);

  for(i=0;i<=n;i++) {
    computeFunctionWithProcedure(res[i], f->child2, x, (unsigned int) (f->libFunDeriv + i));
    sollya_mpfi_div(res[i], res[i], fact);
    sollya_mpfi_mul_ui(fact, fact, i+1);
  }
  sollya_mpfi_clear(fact);
}

void baseFunction_diff(sollya_mpfi_t *res, int nodeType, sollya_mpfi_t x, int n, int *silent) {
  mpfr_t oneHalf;
  mp_prec_t prec;
  prec = getToolPrecision();

  int i;
  switch(nodeType) {
  case NEG: /* This case is useless since it is handled separately in auto_diff_scaled */
            /* However, since x->(-x) could be seen as a base function, and since      */
            /* baseFunction_diff could be used in another context, we implement it.    */
    sollya_mpfi_neg(res[0], x);
    if(n>=1) {
      sollya_mpfi_set_si(res[1], -1);
      for(i=2;i<=n;i++) sollya_mpfi_set_ui(res[i], 0);
    }
    break;
  case SQRT:
    mpfr_init2(oneHalf, prec);
    mpfr_set_d(oneHalf, 0.5, GMP_RNDN);
    constantPower_diff(res, x, oneHalf, n, silent);
    mpfr_clear(oneHalf);
    break;
  case ERF:
    erf_diff(res, x, n, silent);
    break;
  case ERFC:
    erfc_diff(res, x, n, silent);
    break;
  case EXP:
    exp_diff(res, x, n, silent);
    break;
  case EXP_M1:
    expm1_diff(res, x, n, silent);
    break;
  case LOG_1P:
    log1p_diff(res, x, n, silent);
    break;
  case LOG:
    log_diff(res, x, n, silent);
    break;
  case LOG_2:
    log2_diff(res, x, n, silent);
    break;
  case LOG_10:
    log10_diff(res, x, n, silent);
    break;
  case SIN:
    sin_diff(res, x, n, silent);
    break;
  case COS:
    cos_diff(res, x, n, silent);
    break;
  case TAN:
    tan_diff(res, x, n, silent);
    break;
  case ASIN:
    asin_diff(res, x, n, silent);
    break;
  case ACOS:
    acos_diff(res, x, n, silent);
    break;
  case ATAN:
     atan_diff(res, x, n, silent);
    break;
  case SINH:
    sinh_diff(res, x, n, silent);
    break;
  case COSH:
    cosh_diff(res, x, n, silent);
    break;
  case TANH:
    tanh_diff(res, x, n, silent);
    break;
  case ASINH:
    asinh_diff(res, x, n, silent);
    break;
  case ACOSH:
    acosh_diff(res, x, n, silent);
    break;
  case ATANH:
    atanh_diff(res, x, n, silent);
    break;
  case ABS:
    abs_diff(res, x, n, silent);
    break;
  case SINGLE:
    single_diff(res, x, n, silent);
    break;
  case QUAD:
    quad_diff(res, x, n, silent);
    break;
  case HALFPRECISION:
    halfprecision_diff(res, x, n, silent);
    break;
  case DOUBLE:
    double_diff(res, x, n, silent);
    break;
  case DOUBLEDOUBLE:
    double_double_diff(res, x, n, silent);
    break;
  case TRIPLEDOUBLE:
    triple_double_diff(res, x, n, silent);
    break;
  case DOUBLEEXTENDED:
    double_extended_diff(res, x, n, silent);
    break;
  case CEIL:
    ceil_diff(res, x, n, silent);
    break;
  case FLOOR:
    floor_diff(res, x, n, silent);
    break;
  case NEARESTINT:
    nearestint_diff(res, x, n, silent);
    break;
  default:
    sollyaFprintf(stderr,"Error in autodiff: unknown unary function (%d) in the tree\n", nodeType);
  }

  return;
}


/* res is a reserved space for n+1 sollya_mpfi_t such that: */
/*               res_i = f^(i)(x0)/i!                */
/* We proceed recursively on the structure.          */
void auto_diff_scaled(sollya_mpfi_t* res, node *f, sollya_mpfi_t x0, int n) {
  int i;
  sollya_mpfi_t *res1, *res2;
  node *simplifiedChild1, *simplifiedChild2, *tempTree;
  sollya_mpfi_t temp1, temp2;
  mp_prec_t prec;
  int silent = 0;

  prec = getToolPrecision();
  switch (f->nodeType) {
  case VARIABLE:
    sollya_mpfi_set(res[0], x0);
    if(n>=1) {
      sollya_mpfi_set_ui(res[1], 1);
      for(i=2; i<=n; i++) sollya_mpfi_set_ui(res[i], 0);
    }
    break;

  case PI_CONST:
    sollya_mpfi_const_pi(res[0]);
    for(i=1; i<=n; i++) sollya_mpfi_set_ui(res[i], 0);
    break;

  case LIBRARYCONSTANT:
    libraryConstantToInterval(res[0], f);
    for(i=1; i<=n; i++) sollya_mpfi_set_ui(res[i], 0);
    break;

  case CONSTANT:
    sollya_mpfi_set_fr(res[0], *(f->value));
    for(i=1; i<=n; i++) sollya_mpfi_set_ui(res[i], 0);
    break;

  case NEG:
    auto_diff_scaled(res, f->child1, x0, n);
    for(i=0;i<=n;i++) sollya_mpfi_neg(res[i], res[i]);
    break;

  case ADD:
  case SUB:
  case MUL:
  case DIV:
    binary_function_diff(res, f->nodeType, x0, f->child1, f->child2, n, &silent);
    break;

  case SQRT:
  case EXP:
  case LOG:
  case LOG_2:
  case LOG_10:
  case SIN:
  case COS:
  case TAN:
  case ASIN:
  case ACOS:
  case ATAN:
  case SINH:
  case COSH:
  case TANH:
  case ASINH:
  case ACOSH:
  case ATANH:
  case ABS:
  case SINGLE:
  case QUAD:
  case HALFPRECISION:
  case DOUBLE:
  case DOUBLEEXTENDED:
  case DOUBLEDOUBLE:
  case TRIPLEDOUBLE:
  case ERF: 
  case ERFC:
  case LOG_1P:
  case EXP_M1:
  case CEIL:
  case FLOOR:
  case NEARESTINT:
  case LIBRARYFUNCTION:
  case PROCEDUREFUNCTION:
    res1 = (sollya_mpfi_t *)safeCalloc((n+1),sizeof(sollya_mpfi_t));
    res2 = (sollya_mpfi_t *)safeCalloc((n+1),sizeof(sollya_mpfi_t));
    for(i=0;i<=n;i++) {
      sollya_mpfi_init2(res1[i], prec);
      sollya_mpfi_init2(res2[i], prec);
    }

    auto_diff_scaled(res1, f->child1, x0, n);
    if(f->nodeType==LIBRARYFUNCTION) libraryFunction_diff(res2, f, res1[0], n, &silent); 
    else if(f->nodeType==PROCEDUREFUNCTION) procedureFunction_diff(res2, f, res1[0], n, &silent);
    else baseFunction_diff(res2, f->nodeType, res1[0], n, &silent);
    composition_AD(res, res2, res1, n);

    for(i=0;i<=n;i++) {
      sollya_mpfi_clear(res1[i]);
      sollya_mpfi_clear(res2[i]);
    }
    free(res1);
    free(res2);
    break;

  case POW:
    simplifiedChild2 = simplifyTreeErrorfree(f->child2);
    simplifiedChild1 = simplifyTreeErrorfree(f->child1);
    
    /* x^p case */
    if ( (simplifiedChild1->nodeType == VARIABLE) &&
	 (simplifiedChild2->nodeType == CONSTANT) ) {
      constantPower_diff(res, x0, *(simplifiedChild2->value), n, &silent);
    }

    /* p^x case */
    else if ( (simplifiedChild1->nodeType == CONSTANT) &&
	      (simplifiedChild2->nodeType == VARIABLE) ) {
      powerFunction_diff(res, *(simplifiedChild1->value), x0, n, &silent);
    }

    /* p^q case */
    else if ( (simplifiedChild1->nodeType == CONSTANT) &&
	      (simplifiedChild2->nodeType == CONSTANT) ) {
      sollya_mpfi_init2(temp1, prec);
      sollya_mpfi_set_fr(temp1, *(simplifiedChild1->value));
      sollya_mpfi_init2(temp2, prec);
      sollya_mpfi_set_fr(temp2, *(simplifiedChild2->value));
      sollya_mpfi_pow(res[0], temp1, temp2);
      for(i=1; i<=n; i++) sollya_mpfi_set_ui(res[i], 0);
      
      sollya_mpfi_clear(temp1);
      sollya_mpfi_clear(temp2);
    }
    
    /* p^f or f^p case */
    else if ( (simplifiedChild1->nodeType==CONSTANT) ||
	      (simplifiedChild2->nodeType==CONSTANT) ) {
      
      res1 = (sollya_mpfi_t *)safeCalloc((n+1),sizeof(sollya_mpfi_t));
      res2 = (sollya_mpfi_t *)safeCalloc((n+1),sizeof(sollya_mpfi_t));
      for(i=0;i<=n;i++) {
	sollya_mpfi_init2(res1[i], prec);
	sollya_mpfi_init2(res2[i], prec);
      }
      
      if (simplifiedChild1->nodeType == CONSTANT) { /* p^f */
	auto_diff_scaled(res1, simplifiedChild2, x0, n);
	powerFunction_diff(res2, *(simplifiedChild1->value), res1[0], n, &silent);
      }
      else { /* f^p */
	auto_diff_scaled(res1, simplifiedChild1, x0, n);
	constantPower_diff(res2, res1[0], *(simplifiedChild2->value), n, &silent);
      }
      
      composition_AD(res, res2, res1, n); 
      
      for(i=0; i<=n; i++) {
	sollya_mpfi_clear(res1[i]);
	sollya_mpfi_clear(res2[i]); 
      }
      free(res1);
      free(res2);    
    } 

    /*  f^g case */
    /* f^g = exp(g*log(f)) */
    else {
      tempTree = makeExp(makeMul(copyTree(simplifiedChild2), makeLog(copyTree(simplifiedChild1))));
      auto_diff_scaled(res, tempTree, x0, n);
      free_memory(tempTree);
    }

    free_memory(simplifiedChild1);
    free_memory(simplifiedChild2);
    break;
    
  default:
   sollyaFprintf(stderr,"Error in autodiff: unknown identifier (%d) in the tree\n",f->nodeType);
   exit(1);
  }

  return;
}

/* res is a reserved space for n+1 sollya_mpfi_t such that: */
/*               res_i = f^(i)(x0)                   */
void auto_diff(sollya_mpfi_t* res, node *f, sollya_mpfi_t x0, int n) {
  int i;
  sollya_mpfi_t fact;
  mp_prec_t prec;

  prec = getToolPrecision();

  sollya_mpfi_init2(fact, prec);
  sollya_mpfi_set_ui(fact, 1);

  auto_diff_scaled(res, f, x0, n);
  for(i=1;i<=n;i++) {
    sollya_mpfi_mul_ui(fact, fact, i);
    sollya_mpfi_mul(res[i], res[i], fact);
  }
  
  sollya_mpfi_clear(fact);
}
