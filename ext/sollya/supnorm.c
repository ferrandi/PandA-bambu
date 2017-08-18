/*

  Copyright 2010-2011 by 

  Laboratoire d'Informatique de Paris 6, equipe PEQUAN,
  UPMC Universite Paris 06 - CNRS - UMR 7606 - LIP6, Paris, France,

  Laboratoire de l'Informatique du Parallelisme, 
  UMR CNRS - ENS Lyon - UCB Lyon 1 - INRIA 5668

  and by

  Centre de recherche INRIA Sophia-Antipolis Mediterranee, equipe APICS,
  Sophia Antipolis, France.

  Contributors Ch. Lauter, M. Joldes, S. Chevillard

  christoph.lauter@ens-lyon.org
  mioara.joldes@ens-lyon.fr
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

#include "supnorm.h"
#include <mpfr.h>
#include "mpfi-compat.h"
#include "execute.h"
#include <stdio.h> 
#include <stdlib.h>
#include "expression.h"
#include "infnorm.h"
#include "autodiff.h"
#include "taylorform.h"
#include "chain.h"
#include "sturm.h"
#include "general.h"
#include "infnorm.h"
#include "remez.h"
#include "external.h"

/* Add error codes here as needed. 

   When adding error codes, add warning messages below (in
   supremumNormBisect()).

   In case of an error, return SUPNORM_SOME_ERROR unless
   a precise understanding which phase failed is available.

*/
#define SUPNORM_NO_ERROR                         0  /* No error */
#define SUPNORM_SOME_ERROR                      -1  /* Some default error, don't know what caused it */
#define SUPNORM_NO_TAYLOR                        1  /* Couldn't compute a Taylor */
#define SUPNORM_NOT_ENOUGH_WORKING_PRECISION     2  /* Got the impression the precision was not enough */
#define SUPNORM_SINGULARITY_NOT_REMOVED          3  /* Couldn't divide poly by (x-x0)^k */
#define SUPNORM_COULD_NOT_SHOW_POSITIVITY        4  /* Could not validate everything by showing positivity */ 
#define SUPNORM_SINGULARITY_NOT_DETECTED         5  /* Failed to compute an approximate value to a singularity */
#define SUPNORM_ANOTHER_SINGULARITY_IN_DOM       6  /* There's at least two singularities, more bisection needed */
#define SUPNORM_CANNOT_COMPUTE_LOWER_BOUND       7  /* For some reason, we cannot compute a valid lower bound */
#define SUPNORM_CANNOT_COMPUTE_ABSOLUTE_INF      8  /* For some reason, we cannot compute a valid lower bound on the absolute value of func */
#define SUPNORM_CANNOT_DETERMINE_SIGN_OF_T       9  /* For some reason, we cannot determine the sign of the Taylor polynomial at a point */
#define SUPNORM_CANNOT_DETERMINE_ORDER_OF_SINGU 10  /* Could not correctly determine the order of the pole (error intervals not [0]) */

/* Exactly add two polynomials

   For each monomial degree do:

   In the case when both monomials are floating-point numbers,
   return a monomial that is a floating-point number.

   In the case when one of the monomials is a ratio of two
   floating-point numbers and the other is a floating-point number or
   a ratio of floating-point numbers, return a ratio of floating-point
   numbers.

   Otherwise, return a monomial representing the sum of the two
   expressions.

   This version is a first, crude version. We can do better if need
   be.
*/
node *addPolynomialsExactly(node *p1, node *p2) {
  node *temp, *res, *temp2;

  if (!(isPolynomial(p1) && isPolynomial(p2))) {
    temp = makeAdd(copyTree(p1),copyTree(p2));
    res = simplifyTreeErrorfree(temp);
    free_memory(temp);
    return res;
  }

  /* Here, we know that we have polynomials */
  temp = makeAdd(copyTree(p1),copyTree(p2));
  temp2 = horner(temp);
  res = simplifyRationalErrorfree(temp2);
  free_memory(temp);
  free_memory(temp2);

  return res;
}

/* Exactly subtract two polynomials

   For each monomial degree do:

   In the case when both monomials are floating-point numbers,
   return a monomial that is a floating-point number.

   In the case when one of the monomials is a ratio of two
   floating-point numbers and the other is a floating-point number or
   a ratio of floating-point numbers, return a ratio of floating-point
   numbers.

   Otherwise, return a monomial representing the sum of the two
   expressions.

   This version is a first, crude version. We can do better if need
   be.
*/
node *subPolynomialsExactly(node *p1, node *p2) {
  node *temp, *res, *temp2;

  if (!(isPolynomial(p1) && isPolynomial(p2))) {
    temp = makeSub(copyTree(p1),copyTree(p2));
    res = simplifyTreeErrorfree(temp);
    free_memory(temp);
    return res;
  }

  /* Here, we know that we have polynomials */
  temp = makeSub(copyTree(p1),copyTree(p2));
  temp2 = horner(temp);
  res = simplifyRationalErrorfree(temp2);
  free_memory(temp);
  free_memory(temp2);

  return res;
}

/* Exactly scale a polynomial

   Return sum (s * c_i) * x^i for s and p(x) = sum c_i * x^i 

   Wherever possible (even when more precision is needed), simplify
   the expressions s * c_i in the monomials.

   This version is a first, crude version. We can do better if need
   be.
*/
node *scalePolynomialExactly(node *poly, mpfr_t scale) {
  node *temp, *res, *temp2;

  if (!(isPolynomial(poly))) {
    temp = makeMul(copyTree(poly),makeConstant(scale));
    res = simplifyTreeErrorfree(temp);
    free_memory(temp);
    return res;
  }

  /* Here, we know that we have a polynomial */
  temp = makeMul(copyTree(poly),makeConstant(scale));
  temp2 = horner(temp);
  res = simplifyRationalErrorfree(temp2);
  free_memory(temp);
  free_memory(temp2);

  return res;
}

/* Divide a polynomial by (x - x0)^k and check if rest is zero

   The function tries to divide the polynomial poly by (x - x0)^k
   through long division. If the division rest is zero, i.e.
   if poly/(x - x0)^k is a polynomial, the function returns a 
   non-zero value and sets pTilde to poly/(x - x0)^k. 
   Otherwise, if poly is not a polynomial or if poly/(x - x0)^k
   is not a polynomial because the division leaves a rest,
   the function returns zero and does not touch pTilde.

*/
int dividePolyByXMinusX0ToTheK(node **pTilde, node *poly, mpfr_t x0, int k, mp_prec_t prec) {
  int okay, degPoly, degQuotient;
  node *myPTilde;
  node *shifterForth, *shifterBack, *pShifted, *xToK, *pShiftedHorner, *quotient;
  node *quotientSimplified, *quotientShifted, *quotientShiftedHorner;
  mpfr_t kAsMpfr;

  /* Determine the degree of poly and, at the same time, if 
     it really is a polynomial 
  */
  degPoly = getDegree(poly);
  
  /* We can't do anything if poly isn't a polynomial */
  if (degPoly < 0) return 0;

  /* The division will fail if k is greater than the degree of poly */
  if (k > degPoly) return 0;

  /* Cannot divide if (x-x0)^k is not a polynomial */
  if (k < 0) return 0;
  
  /* Division by (x-x0)^0 = 1 is trivial */
  if (k == 0) {
    *pTilde = copyTree(poly);
    return 1;
  }

  /* Here we know that degPoly >= k >= 1 */
  okay = 0;
  myPTilde = NULL;

  /* We shift poly: pShifted(x) = poly(x + x0) */
  shifterForth = makeAdd(makeVariable(),makeConstant(x0));
  pShifted = substitute(poly,shifterForth);
  pShiftedHorner = horner(pShifted);
  
  /* Now build x^k */
  mpfr_init2(kAsMpfr,5 + 8 * sizeof(k));
  mpfr_set_si(kAsMpfr,k,GMP_RNDN); /* exact as per what precedes */
  xToK = makePow(makeVariable(),makeConstant(kAsMpfr));

  /* Now build quotient = pShifted/xToK */
  quotient = makeDiv(pShiftedHorner,xToK);

  /* Now simplify quotient */
  quotientSimplified = simplifyRationalErrorfree(quotient);

  /* Now shift the quotient back: quotientShifted(x) = quotientSimplified(x - x0) */
  shifterBack = makeSub(makeVariable(),makeConstant(x0));
  quotientShifted = substitute(quotientSimplified,shifterBack);
  quotientShiftedHorner = horner(quotientShifted);
  
  /* Try to get the degree of quotientShiftedHorner
     If it is non-negative, quotientShiftedHorner is a polynomial
     If it is equal to degPoly - k, the division worked
  */
  degQuotient = getDegree(quotientShiftedHorner);
  
  if ((degQuotient >= 0) && (degQuotient == (degPoly - k))) {
    okay = 1;
    myPTilde = copyTree(quotientShiftedHorner);
  }


  /* Free all locally used memory */
  free_memory(shifterForth);
  free_memory(pShifted);
  /* pSHiftedHorner gets freed by freeing quotient */
  /* xToK gets freed by freeing quotient */
  free_memory(quotient);
  free_memory(quotientSimplified);
  free_memory(shifterBack);
  free_memory(quotientShifted);
  free_memory(quotientShiftedHorner);
  mpfr_clear(kAsMpfr);

  /* Set result if appropriate and return */
  if (myPTilde == NULL) okay = 0;
  if (okay) *pTilde = myPTilde;
  return okay;
}

/* Show positivity of a polynomial using the Sturm algorithm 

   For a polynomial poly and a domain dom,

   return zero if there is a point in dom at which poly is non-positive,
   return a non-zero value otherwise.

   Return zero for expressions that are no polynomial.
   Return zero for domains that are not bounded by finite numbers.

   Return zero if something goes wrong in the Sturm routine.

   Generally, return zero in case of a doubt (not enough precision
   etc.)

*/
int showPositivity(node * poly, sollya_mpfi_t dom, mp_prec_t prec) {
  int res, positive, nbRoots;
  mpfr_t a, b, c, y, nbRootsMpfr;
  mp_prec_t pp;

  if (!isPolynomial(poly)) return 0;
  if (!sollya_mpfi_bounded_p(dom)) return 0;

  mpfr_init2(nbRootsMpfr,8 * sizeof(int));
  res = getNrRoots(nbRootsMpfr, poly, dom, prec);
  if (!mpfr_number_p(nbRootsMpfr)) {
    nbRoots = 1;
  } else {
    nbRoots = mpfr_get_si(nbRootsMpfr,GMP_RNDU);
  }
  mpfr_clear(nbRootsMpfr);
  if (!res) return 0;

  if (nbRoots != 0) return 0;

  /* Here, we know that we do not cross the abscissa

     We still have to establish that at some point (in the middle of
     the interval), we have a positive value.

  */
  pp = sollya_mpfi_get_prec(dom);
  mpfr_init2(a,pp);
  mpfr_init2(b,pp);
  mpfr_init2(c,pp+1);

  sollya_mpfi_get_left(a,dom);
  sollya_mpfi_get_right(b,dom);

  mpfr_add(c,a,b,GMP_RNDN);
  mpfr_div_2ui(c,c,1,GMP_RNDN);

  mpfr_init2(y,16);
  res = evaluateFaithful(y, poly, c, prec);

  positive = 1;
  if (!res) positive = 0;
  if (!mpfr_number_p(y)) positive = 0;
  if (mpfr_sgn(y) <= 0) positive = 0;

  mpfr_clear(a);
  mpfr_clear(b);
  mpfr_clear(c);
  mpfr_clear(y);

  return positive;
}

/* Compute a value result such that forall x in dom abs(func(x)) > result

   If such a minimum cannot easily be computed, set result to 0 and return 0.
   Otherwise return a non-zero value.

*/
int computeAbsoluteMinimum(mpfr_t result, node *func, sollya_mpfi_t dom, mp_prec_t prec) {
  sollya_mpfi_t y;
  mpfr_t reslt;
  mpfr_t yl, yr;
  int res;
  node *deriv;

  mpfr_init2(reslt,mpfr_get_prec(result));
  res = 0;
  mpfr_set_si(result,0,GMP_RNDN);
  sollya_mpfi_init2(y,prec);
  mpfr_init2(yl,prec);
  mpfr_init2(yr,prec);
  evaluateInterval(y, func, NULL, dom);
  sollya_mpfi_get_left(yl,y);
  sollya_mpfi_get_right(yr,y);

  if (mpfr_number_p(yl) && mpfr_number_p(yr)) {
    if (mpfr_sgn(yl) * mpfr_sgn(yr) > 0) {
      /* The interval does not cross the y-axis, we can take the absolute minimum */
      sollya_mpfi_abs(y,y);
      sollya_mpfi_get_left(reslt,y);
      res = 1;
    } else {
      /* The interval does cross the y-axis, we give it another try with a derivative */
      deriv = differentiate(func);
      evaluateInterval(y, func, deriv, dom);
      sollya_mpfi_get_left(yl,y);
      sollya_mpfi_get_right(yr,y);
      if ((mpfr_number_p(yl) && mpfr_number_p(yr)) &&  
	  (mpfr_sgn(yl) * mpfr_sgn(yr) > 0)) {
	/* Here, we have a result that does not cross the y-axis */
	sollya_mpfi_abs(y,y);
	sollya_mpfi_get_left(reslt,y);
	res = 1;
      }
      free_memory(deriv);
    }
  } 

  sollya_mpfi_clear(y);
  mpfr_clear(yr);
  mpfr_clear(yl);
  mpfr_set(result,reslt,GMP_RNDN); /* exact */
  mpfr_clear(reslt);
  return res;
}


/* Determine the sign of func at x

   Set sign to 1 if it can be shown that func(x) > 0, 
   set sign to -1 if it can be shown that func(x) < 0.
   Return a non-zero value in both cases.

   Otherwise, if the sign cannot be safely determined,
   return zero and set sign to zero.
   
*/
int determineSignAtPoint(int *sign, node *func, mpfr_t x, mp_prec_t prec) {
  int okay, mySign;
  sollya_mpfi_t yAsInterv, xAsInterv;
  mpfr_t yLeft, yRight;
  
  okay = 1;
  mySign = 0;
  
  sollya_mpfi_init2(xAsInterv,mpfr_get_prec(x));
  sollya_mpfi_init2(yAsInterv,prec);
  sollya_mpfi_set_fr(xAsInterv,x);
  mpfr_init2(yLeft,prec);
  mpfr_init2(yRight,prec);
  
  evaluateInterval(yAsInterv, func, NULL, xAsInterv);

  sollya_mpfi_get_left(yLeft,yAsInterv);
  sollya_mpfi_get_right(yRight,yAsInterv);

  if ((!mpfr_number_p(yLeft)) || (!mpfr_number_p(yRight)) || (mpfr_zero_p(yLeft)) || (mpfr_zero_p(yRight))) {
    okay = 0;
  }
  else {
    if (mpfr_sgn(yLeft) * mpfr_sgn(yRight) < 0) okay = 0;
    else mySign = (mpfr_sgn(yLeft) < 0) ? (-1) : 1;
  }

  sollya_mpfi_clear(xAsInterv);
  sollya_mpfi_clear(yAsInterv);
  mpfr_clear(yLeft);
  mpfr_clear(yRight);

  /* Return the result */
  if (mySign == 0) okay = 0;
  if (okay) *sign = mySign; else *sign = 0;
  return okay;
}


/* Compute a certified lower bound ell to the supremum norm of 
   eps = poly - func resp. eps = poly / func - 1

   If mode is zero, let eps = poly - func else let eps = poly / func - 1.

   Compute a value ell such that for all x in dom, abs(eps(x)) >= ell.

   Additionally, if things are pretty, make sure that 

   || eps || <= ell * (1 + abs(gamma)).

   If everything works fine, set ell to the computed value, ADAPTING
   THE PRECISION OF THE mpfr_t VARIABLE ell IF NECESSARY and return a
   non-zero value.

   Otherwise, set ell to zero and return zero. This case happens
   when gamma is not a non-zero number.

   The function assumes that poly is a polynomial but will work even
   if poly is not a polynomial. However, it will not try to ensure
   that ell approximates ||eps|| up to a relative error of abs(gamma).

   The procedure determines its working precision itself where
   possible. It hence disregards the prec parameter unless the
   determination of the working precision fails.
*/
int computeSupnormLowerBound(mpfr_t ell, node *poly, node *func, sollya_mpfi_t dom, mpfr_t gamma, int mode, mp_prec_t prec) {
  node *eps;
  node *epsPrime;
  int res;
  mpfr_t l, y;
  mpfr_t temp, absGamma;
  mp_prec_t pp, pr;
  int deg;
  chain *possibleExtrema;
  mpfr_t a, b, lMinusUlp;
  unsigned long int samplePoints;
  mpfr_t *aBound, *bBound;
  chain *curr;
  int resEval;

  if (mpfr_zero_p(gamma) || (!mpfr_number_p(gamma))) {
    mpfr_set_si(ell,0,GMP_RNDN);
    return 0;
  }

  pr = sollya_mpfi_get_prec(dom);
  mpfr_init2(a,pr);
  mpfr_init2(b,pr);
  sollya_mpfi_get_left(a,dom);
  sollya_mpfi_get_right(b,dom);  

  if (!(mpfr_number_p(a) && mpfr_number_p(b))) {
    mpfr_clear(a);
    mpfr_clear(b);
    mpfr_set_si(ell,0,GMP_RNDN);
    return 0;
  }

  if (mode == ABSOLUTE) {
    eps = makeSub(copyTree(poly),copyTree(func));
    epsPrime = makeSub(differentiate(poly),differentiate(func));
  } else {
    eps = makeSub(makeDiv(copyTree(poly),copyTree(func)),makeConstantDouble(1.0));
    epsPrime = makeSub(makeMul(differentiate(poly),copyTree(func)),makeMul(copyTree(poly),differentiate(func)));
  }

  mpfr_init2(temp,10 + 8 * ((sizeof(unsigned int) > sizeof(mp_prec_t)) ? sizeof(unsigned int) : sizeof(mp_prec_t)));
  mpfr_init2(absGamma,mpfr_get_prec(gamma));
  mpfr_abs(absGamma,gamma,GMP_RNDN);
  mpfr_log2(temp,absGamma,GMP_RNDD);
  mpfr_neg(temp,temp,GMP_RNDU);
  mpfr_ceil(temp,temp);
  if (mpfr_sgn(temp) > 0) {
    pp = 10 + mpfr_get_ui(temp,GMP_RNDU);
    if (pp < 12) pp = 12;
  } else {
    pp = prec;
    if (pp < 12) pp = 12;
  }
  
  deg = getDegree(poly);
  if (deg >= 0) {
    samplePoints = 4 * deg + 1;
  } else {
    samplePoints = getToolPoints();
  }

  possibleExtrema = uncertifiedFindZeros(epsPrime, a, b, samplePoints, 6 + (pp / 2));

  /* We now really define epsPrime as the derivative of eps (for further use with evaluateFaithful) */
  epsPrime = makeDiv(epsPrime, makePow(copyTree(func), makeConstantDouble(2.0)));

  aBound = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
  bBound = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
  mpfr_init2(*aBound,pr);
  mpfr_set(*aBound,a,GMP_RNDD); /* exact */
  mpfr_init2(*bBound,pr);
  mpfr_set(*bBound,b,GMP_RNDU); /* exact */

  possibleExtrema = addElement(addElement(possibleExtrema,aBound),bBound);

  mpfr_init2(l,pp + 5);
  mpfr_init2(lMinusUlp,pp);
  mpfr_init2(y,pp + 10);
  mpfr_set_si(l,0,GMP_RNDN);
  res = 1;
  for (curr=possibleExtrema;curr!=NULL;curr=curr->next) {
    mpfr_set(lMinusUlp,l,GMP_RNDD); /* Take latest maximum minus a couple ulps as a cut off for evaluation */
    if (!mpfr_zero_p(lMinusUlp)) mpfr_nextbelow(lMinusUlp);
    resEval = evaluateFaithfulWithCutOffFast(y, eps, epsPrime, *((mpfr_t *) (curr->value)), lMinusUlp, pp + 10);
    if ((resEval != 0) && (resEval != 3) && (mpfr_number_p(y))) { /* Evaluation okay ? */
      mpfr_abs(y,y,GMP_RNDN); /* exact */
      if (mpfr_sgn(y) > 0) {
	mpfr_nextbelow(y);  /* Compensate for faithful rounding */
	mpfr_nextbelow(y);
      }
      if (mpfr_cmp(y,l) > 0) { /* New absolute maximum */
	mpfr_set(l,y,GMP_RNDD); /* Round down because we produce a lower bound */
      }
    } else { /* Here, something went wrong with the evaluation */
      res = 0;
      break;
    }
  }

  if (res) {
    if (mpfr_get_prec(ell) < mpfr_get_prec(l)) {
      mpfr_set_prec(ell,mpfr_get_prec(l));
    }
    mpfr_set(ell,l,GMP_RNDN); /* exact */
  } else {
    mpfr_set_si(ell,0,GMP_RNDN);
  }

  freeChain(possibleExtrema, freeMpfrPtr);
  mpfr_clear(l);
  mpfr_clear(lMinusUlp);
  mpfr_clear(y);
  mpfr_clear(a);
  mpfr_clear(b);
  mpfr_clear(absGamma);
  mpfr_clear(temp);
  free_memory(eps);
  free_memory(epsPrime);
  return res;
}


/* A small wrapper around taylorform

   Compute a polynomial poly and a bounding interval delta
   such that ( func - poly ) in delta.

   In the case when singu is NULL, use the absolute technique for
   taylorform, developing func at x0 = mid(dom), otherwise use the
   relative technique at x0 = singu.

   In any case, return delta in absolute terms. Account for 
   all errors in the coefficients (as returned by taylorform).
   
   Check if the final interval delta is bounded, i.e. has finite
   real bounds (no NaNs, no Infs).
   
   No check is performed to know if the coefficients of the returned
   polynomial are finite real numbers. In the case when this happens
   whilst the bound delta is finite, the function returns success.

   Return a non-zero value on success, and a zero value otherwise. 
  
   If a zero value is returned, the pointer assigned to *poly is
   invalid and delta is left untouched. This means, on failure, the
   function frees all memory allocated.

   Use prec as the working precision.

   HACK ALERT: currently, taylorform does not take any prec argument.
   This means we have to modify the global precision of the tool.  We
   reset it correctly in the usual case but if a Ctrl-C pops in in the
   middle, it will not be reset. This should be changed in the future.
   
   IMPORTANT REMARK: as does the taylorform command, the computed 
   polynomial here must be translated in order to get the right
   polynomial (i.e. T = poly(x-x0) is the true Taylor polynomial).
*/
int computeTaylorModel(node **poly, sollya_mpfi_t delta,
		       node *func, sollya_mpfi_t dom, int n, mpfr_t *singu, mp_prec_t prec) {
  mp_prec_t oldToolPrec, ppp;
  int res;
  mpfr_t x0;
  int mode;
  sollya_mpfi_t *myDelta;
  sollya_mpfi_t myDom, temp, x0AsInterval, nAsInterval, lagrangeDelta, shiftedDom;
  chain *errors;
  chain *revertedErrors;
  chain *curr;
  node *myPoly;

  if (!sollya_mpfi_bounded_p(dom)) return 0;

  oldToolPrec = getToolPrecision();
  setToolPrecision(prec);
  
  if (singu == NULL) {
    mpfr_init2(x0,sollya_mpfi_get_prec(dom) + 1);
    sollya_mpfi_mid(x0,dom);
    mode = ABSOLUTE;
  } else {
    mpfr_init2(x0,mpfr_get_prec(*singu));
    mpfr_set(x0,*singu,GMP_RNDN); /* exact */
    mode = RELATIVE;
  }
  sollya_mpfi_init2(x0AsInterval,mpfr_get_prec(x0));
  sollya_mpfi_set_fr(x0AsInterval,x0); /* exact */

  sollya_mpfi_init2(myDom, sollya_mpfi_get_prec(dom));
  sollya_mpfi_set(myDom, dom);

  myDelta = NULL;
  errors = NULL;
  taylorform(&myPoly, &errors, &myDelta, func, n, &x0AsInterval, &myDom, mode);
  
  res = 1;
  if ( (myDelta == NULL) || (errors == NULL) || (!sollya_mpfi_bounded_p(*myDelta)) ) res = 0;
  else {
    if (mode == ABSOLUTE) {
      /* Set lagrangeDelta = myDelta */
      sollya_mpfi_init2(lagrangeDelta,sollya_mpfi_get_prec(*myDelta));
      sollya_mpfi_set(lagrangeDelta,*myDelta);
    }
    else {
      /* Compute lagrangeDelta = myDelta * (dom - x0)^n */
      ppp = sollya_mpfi_get_prec(dom);
      sollya_mpfi_init2(temp,ppp);
      sollya_mpfi_init2(lagrangeDelta,ppp);
      sollya_mpfi_sub(temp,dom,x0AsInterval);
      sollya_mpfi_init2(nAsInterval,5 + 8 * sizeof(int));
      sollya_mpfi_set_si(nAsInterval,n); /* exact */
      sollya_mpfi_pow(temp, temp, nAsInterval);
      sollya_mpfi_mul(temp,temp,*myDelta);
      if (!sollya_mpfi_bounded_p(temp)) res = 0;
      else sollya_mpfi_set(lagrangeDelta,temp);
      sollya_mpfi_clear(temp);
      sollya_mpfi_clear(nAsInterval);
    }
    if (res) {
      /* Here, we have lagrangeDelta such that 
         (func - (poly(x-x0) + sum errors[i] * (x-x0)^i)) in lagrangeDelta
         We now add (sum errors[i] * x^i)(dom-x0) to lagrangeDelta.
         We use a simple Horner to perform that evalutation/ bounding.
         The error list starts with c0, so we have to revert it 
         before the Horner. 
         
         HACK ALERT: We will allocate only the containers to the
         reverted list but we just copy over the pointers to the
         MPFIs. So when free'ing the reverted list, we must not 
         free the MPFIs, as they will be free'd when free'ing the 
         orginal list.
      */
      sollya_mpfi_init2(temp,sollya_mpfi_get_prec(delta));
      sollya_mpfi_init2(shiftedDom,sollya_mpfi_get_prec(dom));
      sollya_mpfi_sub(shiftedDom,dom,x0AsInterval);
      revertedErrors = NULL;
      for (curr=errors;curr!=NULL;curr=curr->next) 
        revertedErrors = addElement(revertedErrors,curr->value);
      /* Horner */
      curr = revertedErrors;
      sollya_mpfi_set(temp,*((sollya_mpfi_t *) (curr->value)));
      for (curr=curr->next; curr != NULL; curr = curr->next) {
        sollya_mpfi_mul(temp,temp,shiftedDom);
        sollya_mpfi_add(temp,temp,*((sollya_mpfi_t *) (curr->value)));
      }
      freeChain(revertedErrors,freeNoPointer);
      sollya_mpfi_add(lagrangeDelta,lagrangeDelta,temp);
      if (!sollya_mpfi_bounded_p(lagrangeDelta)) res = 0;
      else sollya_mpfi_set(delta,lagrangeDelta);
      sollya_mpfi_clear(temp);
      sollya_mpfi_clear(shiftedDom);
    }
    sollya_mpfi_clear(lagrangeDelta);
    sollya_mpfi_clear(*myDelta);
    free(myDelta);
  }

  if (!res) free_memory(myPoly);
  else *poly=myPoly;

  freeChain(errors,freeMpfiPtr);
  sollya_mpfi_clear(myDom);
  sollya_mpfi_clear(x0AsInterval);
  mpfr_clear(x0);
  setToolPrecision(oldToolPrec);
  return res;
}


/* Tries to compute a Taylor expansion poly of func with degree n such that 

   || poly(x-x0) - func || <= delta.

   If such an expansion can be computed using computeTaylorForm,
   assign that expansion to *poly and return a non-zero value.

   Otherwise, do not touch the pointer *poly and return zero.
   
   This means, when the computation fails, all memory allocated 
   for the computation has been freed.
   
   Be aware that poly is not checked to have finite, real
   coefficients.  It is just ensured that the Lagrange and coefficient
   approximation error of the Taylor form is finite and contained in
   [-delta, delta].
   
   The parameters singu and prec are passed directly to
   computeTaylorForm.
   IMPORTANT REMARK: this is an untranslated polynomial (see computeTaylorModel).

*/
int checkDegreeTaylorModel(node **poly, node *func, sollya_mpfi_t dom, mpfr_t delta, int n, mpfr_t *singu, mp_prec_t prec) {
  node *myPoly;
  int res, resCompute;
  sollya_mpfi_t computedDelta;
  mpfr_t supAbsComputedDelta;

  res = 0;

  sollya_mpfi_init2(computedDelta,prec);
  resCompute = computeTaylorModel(&myPoly, computedDelta, func, dom, n, singu, prec);
 
  if (resCompute) {
    
    /* Here, we have to check if sup(abs(computedDelta)) <= delta */
    sollya_mpfi_abs(computedDelta,computedDelta);
    mpfr_init2(supAbsComputedDelta,prec);
    sollya_mpfi_get_right(supAbsComputedDelta,computedDelta);
    if (mpfr_number_p(supAbsComputedDelta) && 
	mpfr_number_p(delta) &&
	(mpfr_cmp(supAbsComputedDelta,delta) <= 0)) {
      /* Here, we have a polynomial that satisfies the bound */
      *poly = myPoly;
      res = 1;
    } else {
      /* Here, we got a polynomial and a computedDelta, but the 
  	 error is too large. So we have to free the polynomial */
      free_memory(myPoly);
      res = 0;
    }
    mpfr_clear(supAbsComputedDelta);
  } else {
    /* Here, we could not compute a Taylor Model 

       We simply return zero, without touching at poly.

       The computeTaylorModel function ensures that myPoly has not
       been assigned a newly allocated polynomial.

    */
    res = 0;
  }

  sollya_mpfi_clear(computedDelta);

  return res;
}


/* Checks if poly is a polynomial with coefficients that are all fully
   evaluated MPFR constants each of which is finite and real (not NaN,
   not Inf).  

   Returns a non-zero value if poly fullfills the constraints and zero
   otherwise.

*/
int isPolynomialWithConstantDyadicFiniteRealCoefficients(node *poly) {
  node **coefficients;
  int degree, i, res;

  if (!isPolynomial(poly)) return 0;

  getCoefficients(&degree, &coefficients, poly);

  if (degree < 0) return 0;

  res = 1;
  for (i=0;i<=degree;i++) {
    if (coefficients[i] != NULL) {
      if ((!(coefficients[i]->nodeType == CONSTANT)) ||
	  (!mpfr_number_p(*(coefficients[i]->value)))) {
	res = 0;
	break;
      }
    }
  }

  for (i=0;i<=degree;i++) {
    if (coefficients[i] != NULL) 
      free_memory(coefficients[i]);
  }
  free(coefficients);

  return res;
}


/* Compute a Taylor Model poly for func such that 

   - for all x in dom, abs(func(x) - poly(x)) <= delta,
   - the degree of poly is the least possible of the polynomials
   computable with the tool that satisfy the bound,
   - the degree of poly is less than or equal to maximumAllowedN,
   - the polynomial has finite, real coefficients that are MPFR constants.

   If such a polynomial is found, assign it to *poly and return a
   non-zero value. Otherwise, do not touch *poly and return zero.

   If singu is NULL, develop func at the midpoint of dom, using the
   absolute error mode of the Taylor Model code suite. Otherwise,
   develop func at singu, using the relative error mode of the Taylor
   Model code.

   Perform the computations with a working precision prec.

*/
int computeTaylorModelOfLeastDegree(node **poly, node *func, sollya_mpfi_t dom, mpfr_t delta, int maximumAllowedN, mpfr_t *singu, mp_prec_t prec) {
  node *myPoly;
  node *bestPoly;
  node *shifterBack;
  node *pShifted;
  mpfr_t x0;
  int n, okay, resCompute, res, nMin, nMax;
  nMax = 1; nMin = 0; okay = 0;

  bestPoly = NULL;
  while (nMax < 2*maximumAllowedN) {
    if (nMax > maximumAllowedN) nMax = maximumAllowedN;
    resCompute = checkDegreeTaylorModel(&myPoly, func, dom, delta, nMax, singu, prec);
    if (resCompute) {
      free_memory(bestPoly); /* does nothing if bestPoly == NULL */
      bestPoly = myPoly;
      okay = 1;
      break;
    }
    nMin = nMax;
    nMax <<= 1;
  }

  if (!okay) return 0; /* We cannot find a suitable polynomial of degree <= maximumAllowedN */

  /* Here we know that with nMin, we cannot reach the bound but with
     nMax we can. So we refine by bisecting the interval [nMin,nMax]
     Also, we know that bestPoly contains the poly associated with nMax.
  */
  while (nMax - nMin > 1) {
    n = (nMin + nMax) / 2;
    resCompute = checkDegreeTaylorModel(&myPoly, func, dom, delta, n, singu, prec);
    if (resCompute) {  /* OK with n, so the new interval is [nMin,n] */
      nMax = n;
      free_memory(bestPoly);
      bestPoly = myPoly;
    }
    else nMin = n; /* Not OK with n, so the new interval is [n,nMax] */
  }

  /* Now we know that bestPoly is the polynomial that we want, but we need to translate it */
  if (singu == NULL) {
    mpfr_init2(x0,sollya_mpfi_get_prec(dom) + 1);
    sollya_mpfi_mid(x0,dom);
  }
  else {
    mpfr_init2(x0,mpfr_get_prec(*singu));
    mpfr_set(x0,*singu,GMP_RNDN); /* exact */
  }

  shifterBack = makeSub(makeVariable(),makeConstant(x0));
  pShifted = substitute(bestPoly,shifterBack);
  free_memory(shifterBack);
  free_memory(bestPoly);
  bestPoly = horner(pShifted);
  free_memory(pShifted);
  mpfr_clear(x0);

  /* Though unlikely, it could be possible that some coefficient of the polynomial be a NaN or Inf */
  /*   (despite a successful return of checkDegreeTaylorModel...). So we check it. */
  if (isPolynomialWithConstantDyadicFiniteRealCoefficients(bestPoly)) {
	*poly = bestPoly;
	res = 1;
  }
  else {
    /* Here, we are in the strange case that should almost never
       happen.  We indicate failure and hope the problem will go
       away by bisecting dom.
    */
    res = 0;
    if (bestPoly != NULL) free_memory(bestPoly);
  }

  return res;
}

/* Determines if func has a zero in dom.

   If, using a safe, validated algorithm, it can be shown that func
   does not vanish on dom, the function returns 0 and does not
   touch zero and bisect.

   Otherwise, the function tries to determine the least zero of func
   in dom and assigns that value to zero. If func appears to have no
   other zeros in dom, the function returns 1 and does not touch
   bisect. Otherwise, the function assigns the midpoint of the least
   and one-but-least zero of func in dom to bisect and returns 2.

   In the case when the function is not able to establish the 
   fact that func does not vanish on dom but is not able to
   find an approximation to a zero of func in dom, it does not
   touch zero and bisect and returns -1.

   This means: if the return value is 

   - 0: the calling function can safely assume that func does not have
   a zero in dom,

   - 1: the calling function can assume that zero is a good
   approximation to the least zero of func in dom and that it is
   probably the only zero of func in dom. Hence by overcoming the 
   problems due to this zero of func, the supnorm is likely to 
   be computable in one step,

   - 2: the calling function can assume that, for supnorm computation,
   it is a good idea to bisect in the indicated bisection point if
   the computation tackling only the first zero fails,

   - -1: the calling function is not provided with a zero that 
   might need to be overcome in supnorm computation but is likely
   to see the supnorm computation on the whole domain dom fail.
   
   The argument expectedZeros is an indication how many zeros of func
   are maximally expected in the given range.

*/
int determinePossibleZeroAndBisectPoint(mpfr_t zero, mpfr_t bisect,
					node *func, sollya_mpfi_t dom,
					int expectedZeros,
					mp_prec_t prec) {
  int res;
  sollya_mpfi_t y;
  mpfr_t yl, yr;
  unsigned long int points;
  mpfr_t a, b;
  mp_prec_t pp, ppp;
  chain *possibleZeros;
  chain *curr;
  mpfr_t *least, *second;
  mpfr_t myBisect;

  sollya_mpfi_init2(y,prec);
  mpfr_init2(yl,prec);
  mpfr_init2(yr,prec);
  evaluateInterval(y, func, NULL, dom);
  sollya_mpfi_get_left(yl,y);
  sollya_mpfi_get_right(yr,y);
  
  if (mpfr_number_p(yl) &&
      mpfr_number_p(yr) &&
      (mpfr_sgn(yl) * mpfr_sgn(yr) > 0)) {
    /* Here, we know by interval evaluation of func over dom 
       that there is no zero of func in the interval.

       We can return zero.

    */
    res = 0;
  } else {
    /* Here, we have to compute a list of approximations to 
       the zeros of func in dom.
    */
    points = 4 * expectedZeros + 1;

    pp = sollya_mpfi_get_prec(dom);
    mpfr_init2(a,pp);
    mpfr_init2(b,pp);
    sollya_mpfi_get_left(a,dom);
    sollya_mpfi_get_right(b,dom);

    if ( (!mpfr_number_p(a)) || (!mpfr_number_p(b)) )
      res=-1; /* an unbounded range, return -1. */
    else {
      /* Try to compute a list of zeros */
      pp = mpfr_get_prec(zero);
      if (prec > pp) pp = prec;
      possibleZeros = uncertifiedFindZeros(func, a, b, points, pp + 5);

      /* Remark: uncertifiedFindZeros always returns ordered values.
         Hence, the following code is uselessly complicated. */

      if (possibleZeros != NULL) { /* We found at least one zero */
	if (possibleZeros->next == NULL) {
	  /* Here, we have exactly one zero. */
	  res = 1;
	  mpfr_set(zero,*((mpfr_t *) (possibleZeros->value)),GMP_RNDN); /* It's an approx. anyway */
	}
        else {
	  /* Here, we have at least two zeros 
	     Start by exhibiting the least zero.
	  */
	  least = (mpfr_t *) (possibleZeros->value);
	  for (curr=possibleZeros;curr!=NULL;curr=curr->next) {
	    if (mpfr_cmp(*((mpfr_t *) (curr->value)),*least) < 0) {
	      least = (mpfr_t *) (curr->value);
	    }
	  }

	  /* Now find the second least zero */
	  second = least;
	  for (curr=possibleZeros;curr!=NULL;curr=curr->next) {
	    if (mpfr_cmp(*((mpfr_t *) (curr->value)),*second) > 0) {
	      second = (mpfr_t *) (curr->value);
	    }
	  }
	  /* Now second is the greatest zero.
	     In the next stanza we will go down on second
	     with all zeros in the list greater than the least zero.
	     Anyway, we first check if the greatest zero is greater
	     then the least zero. If it is not, we cannot find a 
	     second least zero.
	  */

	  if (mpfr_cmp(*least,*second) == 0) {
	    /* Here, we couldn't find any zero larger than the least 
	       This case should not happen but anyway, we found one zero.
	    */
	    res = 1;
	    mpfr_set(zero,*least,GMP_RNDN); /* It's an approx. anyway */
	  } else {
	    /* Here, we really found that there at least two different zeros 
	       We now go down on second with all zeros in the list greater 
	       than the least zero.
	     */
	    for (curr=possibleZeros;curr!=NULL;curr=curr->next) {
	      if ((mpfr_cmp(*((mpfr_t *) (curr->value)),*least) > 0) &&
		  (mpfr_cmp(*((mpfr_t *) (curr->value)),*second) < 0)) {
		second = (mpfr_t *) (curr->value);
	      }
	    }
	    /* Now, second is really the second least zero in the list */
	    res = 2;
	    mpfr_set(zero,*least,GMP_RNDN); /* It's an approx. anyway */

	    /* Compute midpoint between least and second least zero */
	    pp = mpfr_get_prec(*least);
	    ppp = mpfr_get_prec(*second);
	    if (ppp > pp) pp = ppp;
	    mpfr_init2(myBisect,pp + 1);
	    mpfr_add(myBisect, *least, *second, GMP_RNDN); 
	    mpfr_div_2ui(myBisect,myBisect,1,GMP_RNDN);
	    
	    /* Set bisection point to midpoint of the two least zeros */
	    mpfr_set(bisect,myBisect,GMP_RNDN); /* It's all an approx. anyway */
	    
	    mpfr_clear(myBisect);
	  }
	}
	
	freeChain(possibleZeros,freeMpfrPtr);
      } else {
	/* We did not find any zero, return -1. */
	res = -1;
      }
    } 
    mpfr_clear(a);
    mpfr_clear(b);
  }
  
  mpfr_clear(yr);
  mpfr_clear(yl);
  sollya_mpfi_clear(y);

  return res;
}


/* Determines the order of a zero of func at x0.

   The function computes an upper bound k for the order of a zero of
   func at x0, provided that the zero is of order less than n.

   In the case when an upper bound has been correctly determined, the
   function assigns the order to k and returns a non-zero value.

   Otherwise, if n is too low to correctly determine an upper bound,
   the function does not touch k and returns zero.

   In the context of supremum norm computation, initialize n 
   to the degree of the polynomial p in p/f - 1, as f might not 
   reasonably have a zero of a higher degree than that of p.

   HACK ALERT: currently, taylorform does not take any prec argument.
   This means we have to modify the global precision of the tool.  We
   reset it correctly in the usual case but if a Ctrl-C pops in in the
   middle, it will not be reset. This should be changed in the future.

*/
int determineOrderOfZero(int *k, node *func, mpfr_t x0, int n, mp_prec_t prec) {
  int myK, res;
  mp_prec_t oldToolPrec;
  node *poly;
  chain *errors;
  sollya_mpfi_t x0AsInterval;
  node **coefficients;
  int degree, i, len;
  chain *curr;
  sollya_mpfi_t **errorsAsArray;

  res = 0;

  /* Make compiler happy: */
  myK = -1;
  /* End of compiler happiness */

  sollya_mpfi_init2(x0AsInterval,mpfr_get_prec(x0));
  sollya_mpfi_set_fr(x0AsInterval,x0);

  oldToolPrec = getToolPrecision();
  setToolPrecision(prec);
  poly = NULL;
  errors = NULL;
  taylorform(&poly, &errors, NULL, func, n, &x0AsInterval, NULL, RELATIVE);
  setToolPrecision(oldToolPrec);

  if ((poly != NULL) && (errors != NULL)) {
    len = lengthChain(errors); /* Normally, len == n+1 */

    /*   HACK ALERT: We will allocate only the containers to the
         array of errors but we just copy over the pointers to the
         MPFIs. So when free'ing the array, we must not 
         free the MPFIs, as they will be free'd when free'ing the 
         orginal list.
    */
    errorsAsArray = (sollya_mpfi_t **) safeCalloc(len,sizeof(sollya_mpfi_t *));
    i = 0; 
    for (curr=errors;curr!=NULL;curr=curr->next) {
      errorsAsArray[i] = (sollya_mpfi_t *) (curr->value);
      i++;
    }
    coefficients = NULL;
    getCoefficients(&degree,&coefficients,poly);
    if ( (degree >= 0) && (coefficients != NULL) ) { 
      if (len == degree + 1) { /* Should always be true */
        myK = 0; 
	while ((myK <= degree) && (myK <= n-1) && (!res)) { /* Normally n==degree */
	  if (   (    (coefficients[myK] == NULL)
                      || 
                      ( (coefficients[myK]->nodeType == CONSTANT) && 
                        (mpfr_zero_p(*(coefficients[myK]->value)))
                      )
                 ) && 
                 (sollya_mpfi_is_zero(*(errorsAsArray[myK])))
                 ) {
	    myK++;
	  } else {
	    res = 1;
	  }
	}
      } 
      for (i=0;i<=degree;i++) {
	if (coefficients[i] != NULL) free_memory(coefficients[i]);
      }
    }
    if (coefficients != NULL) free(coefficients);
    free(errorsAsArray);
    freeChain(errors,freeMpfiPtr);
    free_memory(poly);
  }

  sollya_mpfi_clear(x0AsInterval);

  if (res) *k = myK;
  return res;
}


/* Compute the supremum norm on eps = p - f over dom

   The supremum norm is computed with an enclosure error less than accuracy,
   i.e. the obtained interval [l,u] satisfies in the end:

   abs(u-l/l) <= accuracy

   If everything works fine, result is affected with an interval
   safely enclosing the supremum norm. The return value is then *zero*
   (i.e. SUPNORM_NO_ERROR).

   Otherwise, if an error occurs, the return value is non-zero. The
   number of the return value then corresponds to a special error
   message meaning (see #defines above).

   No warning message is ever displayed by this function.

   The computing precision is prec.

   We are ensured that this function is called only if 

   * poly is a polynomial
   * dom is a closed non-empty interval containing only numbers that is not reduced to a point,
   * accuracy is a positive number.

   In the case when the computation fails but there is hope in
   obtaining a result by bisection, the algorithm may assign a point
   in the interior of dom to bisectPoint. The global bisection code
   will then try to bisect at that point. If no value is assigned, the
   global bisection will be performed at the midpoint of dom. This
   means: if you just want default behavior for the bisection (in the
   midpoint), then do not touch bisectPoint.

   To begin with, we do not care about removable singularities in the
   expression of func. In such a case, the absolute supnorm may simply
   fail for now.

*/
int supnormAbsolute(sollya_mpfi_t result, node *poly, node *func, sollya_mpfi_t dom, mpfr_t accuracy, mp_prec_t prec, mpfr_t bisectPoint) {
  mpfr_t ell, gamma, fifthteenThirtySecond, errMax, bound, u, thirtyoneThirtySecond;
  mp_prec_t pr;
  node *T;
  int maximumAllowedN;
  node *boundAsNode, *s1, *s2, *pMinusT, *TMinusp;
  /* Compute ell such that ell <= || p - f || with an accuracy gamma = accuracy/32 */
  mpfr_init2(ell,prec);
  mpfr_init2(gamma,mpfr_get_prec(accuracy));
  mpfr_div_ui(gamma,accuracy,32,GMP_RNDN); /* exact, but it doesn't matter anyway */
  if (!computeSupnormLowerBound(ell, poly, func, dom, gamma, ABSOLUTE, prec)) { /*we compute a lower bound with expected accuracy gamma=accuracy/32*/
    /* Before returning, we do a quick heuristical check if we had a chance 
       with this level of working precision 
    */
    mpfr_abs(ell,accuracy,GMP_RNDD); /* heuristic anyway */
    mpfr_log2(ell,ell,GMP_RNDD); /* heuristic anyway */
    mpfr_floor(ell,ell);
    mpfr_neg(ell,ell,GMP_RNDU);
    pr = mpfr_get_ui(ell,GMP_RNDD);
    mpfr_clear(ell);
    mpfr_clear(gamma);
    if (pr > prec) {
      return SUPNORM_NOT_ENOUGH_WORKING_PRECISION;
    }
    return SUPNORM_CANNOT_COMPUTE_LOWER_BOUND;
  }
  /* Compute errMax such that errMax approximates ell * accuracy * 15/32 but is surely no greater */
  mpfr_init2(fifthteenThirtySecond,12); /* 15/32 can be written on 12 bits */
  mpfr_set_ui(fifthteenThirtySecond,15,GMP_RNDD); /* exact as 15 holds on 12 bits */
  mpfr_div_ui(fifthteenThirtySecond,fifthteenThirtySecond,32,GMP_RNDD); /* exact, 32 is a power of 2 */
  mpfr_init2(errMax,prec);
  mpfr_mul(errMax,ell,accuracy,GMP_RNDD); /* round down to get lesser value */
  mpfr_mul(errMax,errMax,fifthteenThirtySecond,GMP_RNDD); /* round down to get lesser value */
  mpfr_clear(fifthteenThirtySecond);

  /* Compute T such that the absolute error is <= errMax */
  maximumAllowedN = 16 * getDegree(poly);
  if (maximumAllowedN < 32) maximumAllowedN = 32;  
  T = NULL;
  if (!computeTaylorModelOfLeastDegree(&T, func, dom, errMax, maximumAllowedN, NULL, prec)) {
    mpfr_clear(ell);
    mpfr_clear(gamma); 
    mpfr_clear(errMax);
    return SUPNORM_NO_TAYLOR;
  }
  /* Compute bound that approximates bound = ell * (1 + accuracy/2) but surely no greater  */
  /* Hence, proving that ||p-T|| <= bound surely proves that ||p-T|| <= ell*(1+accuracy/2) */
  mpfr_init2(bound,prec);
  mpfr_div_ui(bound,accuracy,2,GMP_RNDD); /* exact, but round-down anyway */
  mpfr_add_ui(bound,bound,1,GMP_RNDD); /* round-down to get lesser value */
  mpfr_mul(bound,ell,bound,GMP_RNDD); /* round-down to get lesser value */

  /* Represent bound as a node * (a polynomial) that we can give to 
     subPolynomialsExactly(node *p1, node *p2)
  */
  boundAsNode = makeConstant(bound);
  /* Compute (build) s1 = bound - (p - T) and s2 = bound - (T - p) */
  pMinusT = subPolynomialsExactly(poly, T);
  TMinusp = subPolynomialsExactly(T, poly);
  s1 = subPolynomialsExactly(boundAsNode, pMinusT);
  s2 = subPolynomialsExactly(boundAsNode, TMinusp);
  
  /* Check now that s1 and s2 are both positive on the domain dom 

     The case when s1 or s2 have a point where they are not
     positive should never happen. The check just ensures the 
     safety of the algorithm, i.e. validates it.

  */
  if ((!showPositivity(s1, dom, prec)) ||
      (!showPositivity(s2, dom, prec))) {
    /* At least one of s1 or s2 has a point in dom where it is non-positive 
       
       We clear the used memory and return the appropriate failure code.

    */
    mpfr_clear(ell);
    mpfr_clear(gamma);
    mpfr_clear(errMax);
    mpfr_clear(bound);
    free_memory(T);
    free_memory(boundAsNode);
    free_memory(s1);
    free_memory(s2);
    free_memory(pMinusT);
    free_memory(TMinusp);
    return SUPNORM_COULD_NOT_SHOW_POSITIVITY;
  }
  
  /* If we are here, we know that s1 and s2 are positive and we can
     easily deduce the upper bound from the lower bound ell. 

     We take u approximating ell * (1 + 31/32 * accuracy) and surely no less
     than it.

  */
  mpfr_init2(u,prec);
  mpfr_init2(thirtyoneThirtySecond,12); /* 31/32 holds on 12 bits */
  mpfr_set_ui(thirtyoneThirtySecond,31,GMP_RNDU); /* exact, 31 holds on 12 bits */
  mpfr_div_ui(thirtyoneThirtySecond,thirtyoneThirtySecond,32,GMP_RNDU); /* exact, 32 is a power of 2 */
  mpfr_mul(u,thirtyoneThirtySecond,accuracy,GMP_RNDU); /* round-up as all quantities are positive and we want an upper bound */
  mpfr_add_ui(u,u,1,GMP_RNDU); /* round-up as all quantities are positive and we want an upper bound */
  mpfr_mul(u,ell,u,GMP_RNDU); /* round-up as all quantities are positive and we want an upper bound */
  /* Set the result */
  sollya_mpfi_interv_fr(result,ell,u);

  /* Clear all used memory */
  mpfr_clear(ell);
  mpfr_clear(gamma);
  mpfr_clear(errMax);
  mpfr_clear(bound);
  mpfr_clear(u);
  mpfr_clear(thirtyoneThirtySecond);
  free_memory(T);
  free_memory(boundAsNode);
  free_memory(s1);
  free_memory(s2);
  free_memory(pMinusT);
  free_memory(TMinusp);

  /* Return success */
  return SUPNORM_NO_ERROR;
}

/* Compute the supremum norm on eps = p/f - 1 over dom

   The supremum norm is computed with an enclosure error less than accuracy,
   i.e. the obtained interval [l,u] satisfies in the end:

   abs(u-l/l) <= accuracy

   If everything works fine, result is affected with an interval
   safely enclosing the supremum norm. The return value is then *zero*
   (i.e. SUPNORM_NO_ERROR).

   Otherwise, if an error occurs, the return value is non-zero. The
   number of the return value then corresponds to a special error
   message meaning (see #defines above).

   No warning message is ever displayed by this function.

   The computing precision is prec.

   We are ensured that this function is called only if 

   * poly is a polynomial
   * dom is a closed non-empty interval containing only numbers that is not reduced to a point,
   * accuracy is a positive number.

   In the case when the computation fails but there is hope in
   obtaining a result by bisection, the algorithm may assign a point
   in the interior of dom to bisectPoint. The global bisection code
   will then try to bisect at that point. If no value is assigned, the
   global bisection will be performed at the midpoint of dom. This
   means: if you just want default behavior for the bisection (in the
   midpoint), then do not touch bisectPoint.

   This function may assume that the expression poly / func - 1 is
   likely not have any removable singularities in dom, i.e. it can
   assume that func is likely not to vanish in dom.

   We do not care about removable singularities in the
   expression of func. In such a case, the relative supnorm may simply
   fail for now.

   The singularity parameter is just to be passed on to the 
   Taylor Form code.

*/
int supnormRelativeNoSingularity(sollya_mpfi_t result, node *poly, node *func, sollya_mpfi_t dom, mpfr_t accuracy, mp_prec_t prec, mpfr_t *singularity, mpfr_t bisectPoint) {
  mpfr_t F, ell, gamma, thirtyoneThirtySecond, u, errMax, midDom, signT, bound;
  mp_prec_t pr;
  sollya_mpfi_t ellInterval, accuracyInterval, fifthteenThirtySecondInterval, FInterval, errMaxInterval, onePlusUInterval, uInterval;
  node *T;
  int maximumAllowedN, signAsInt;
  node *s1, *s2, *boundTimesT, *pMinusT, *TMinusp;
  
  /* Makes compiler happy */
  signAsInt = -2;
  /* End of compiler happiness */

  /* Compute F such that forall x in dom |func(x)| >= F */
  mpfr_init2(F,prec);
  if (!computeAbsoluteMinimum(F, func, dom, prec)) {
    /* If we are here, we could not compute a valid lower bound on the absolute 
       value of func, i.e. a bound that seems to be non-zero. 
    */
    mpfr_clear(F);
    return SUPNORM_CANNOT_COMPUTE_ABSOLUTE_INF;
  }

  /* Check that F is really non-zero and a real */
  if ((!mpfr_number_p(F)) || mpfr_zero_p(F)) {
    mpfr_clear(F);
    return SUPNORM_CANNOT_COMPUTE_ABSOLUTE_INF;
  }

  /* Compute ell such that ell <= || p / f - 1 || with an accuracy gamma = accuracy/32 */
  mpfr_init2(ell,prec);
  mpfr_init2(gamma,mpfr_get_prec(accuracy));
  mpfr_div_ui(gamma,accuracy,32,GMP_RNDN); /* exact, but it doesn't matter anyway */
  if (!computeSupnormLowerBound(ell, poly, func, dom, gamma, RELATIVE, prec)) {
    /* Before returning, we do a quick heuristical check if we had a chance 
       with this level of working precision 
    */
    mpfr_abs(ell,accuracy,GMP_RNDD); /* heuristic anyway */
    mpfr_log2(ell,ell,GMP_RNDD); /* heuristic anyway */
    mpfr_floor(ell,ell);
    mpfr_neg(ell,ell,GMP_RNDU);
    pr = mpfr_get_ui(ell,GMP_RNDD);
    mpfr_clear(F);
    mpfr_clear(ell);
    mpfr_clear(gamma);
    if (pr > prec) {
      return SUPNORM_NOT_ENOUGH_WORKING_PRECISION;
    }
    return SUPNORM_CANNOT_COMPUTE_LOWER_BOUND;
  }

  /* Compute a presumed upper bound for the supremum norm that we will try to
     validate.

     Take u approximating ell * (1 + 31/32 * accuracy) but surely no less than it.
     
  */
  mpfr_init2(u,prec);
  mpfr_init2(thirtyoneThirtySecond,12); /* 31/32 holds on 12 bits */
  mpfr_set_ui(thirtyoneThirtySecond,31,GMP_RNDU); /* exact, 31 holds on 12 bits */
  mpfr_div_ui(thirtyoneThirtySecond,thirtyoneThirtySecond,32,GMP_RNDU); /* exact, 32 is a power of 2 */
  mpfr_mul(u,thirtyoneThirtySecond,accuracy,GMP_RNDU); /* round-up as all quantities are positive and we want an upper bound */
  mpfr_add_ui(u,u,1,GMP_RNDU); /* round-up as all quantities are positive and we want an upper bound */
  mpfr_mul(u,ell,u,GMP_RNDU); /* round-up as all quantities are positive and we want an upper bound */

  /* Compute a maxium absolute error for the Taylor polynomial 

     Take a value errMax approximating ell * accuracy * (15/32) * 1/(1+u) * F/(1+15*accuracy/32) but being 
     surely no greater than it (with u approximating ell*(1+31*accuracy/32) but surely no
     lesser than it).

     We just use interval arithmetic to be sure to have a bound no greater than the approximated
     quantity.

  */

  /* Get memory */
  mpfr_init2(errMax, prec);
  sollya_mpfi_init2(ellInterval,prec);
  sollya_mpfi_init2(accuracyInterval,mpfr_get_prec(accuracy));
  sollya_mpfi_init2(fifthteenThirtySecondInterval,12);
  sollya_mpfi_init2(FInterval,mpfr_get_prec(F));
  sollya_mpfi_init2(errMaxInterval, prec);
  sollya_mpfi_init2(uInterval,mpfr_get_prec(u));
  sollya_mpfi_init2(onePlusUInterval, prec);
  
  /* Initialize */
  sollya_mpfi_set_fr(accuracyInterval,accuracy); 
  sollya_mpfi_set_ui(fifthteenThirtySecondInterval,15);
  sollya_mpfi_div_ui(fifthteenThirtySecondInterval,fifthteenThirtySecondInterval,32);
  sollya_mpfi_set_fr(FInterval,F);
  sollya_mpfi_set_fr(uInterval,u);
  sollya_mpfi_set_fr(ellInterval,ell);
  
  /* Compute in IA */
  sollya_mpfi_mul(errMaxInterval,fifthteenThirtySecondInterval,accuracyInterval); /* errMaxInterval <- 15/32 * accuracy */
  sollya_mpfi_add_ui(errMaxInterval,errMaxInterval,1); /* errMaxInterval <- 1 + errMaxInterval = 1 + 15/32 * accuracy */
  sollya_mpfi_div(errMaxInterval,FInterval,errMaxInterval); /* errMaxInterval <- F/errMaxInterval = F/(1+15/32*accuracy) */
  sollya_mpfi_add_ui(onePlusUInterval,uInterval,1); /* onePlusUInterval <- 1 + u */
  sollya_mpfi_div(errMaxInterval,errMaxInterval,onePlusUInterval); /* errMaxInterval <- errMaxInterval/onePlusUInterval = 1/(1+u) * F/(1+15/32*accuracy) */

  sollya_mpfi_mul(errMaxInterval,errMaxInterval,fifthteenThirtySecondInterval); /* errMaxInterval <- 15/32 * errMaxInterval = 15/32 * 1/(1+u) * F/(1+15/32*accuracy) */
  sollya_mpfi_mul(errMaxInterval,errMaxInterval,accuracyInterval); /* errMaxInterval <- errMaxInterval * accuracy = accuracy * 15/32 * 1/(1+u) * F/(1+15/32*accuracy) */
  sollya_mpfi_mul(errMaxInterval,errMaxInterval,ellInterval); /* errMaxInterval <- ell * errMaxInterval = ell * accuracy * 15/32 * 1/(1+u) * F/(1+15/32*accuracy) */

  /* Get the lower bound of the result */
  sollya_mpfi_get_left(errMax,errMaxInterval);

  /* Clear memory */
  sollya_mpfi_clear(errMaxInterval);
  sollya_mpfi_clear(FInterval);
  sollya_mpfi_clear(fifthteenThirtySecondInterval);
  sollya_mpfi_clear(accuracyInterval);
  sollya_mpfi_clear(ellInterval);
  sollya_mpfi_clear(onePlusUInterval);
  sollya_mpfi_clear(uInterval);
  
  /* Compute T such that the absolute error is <= errMax */
  maximumAllowedN = 16 * getDegree(poly);
  if (maximumAllowedN < 32) maximumAllowedN = 32;
  T = NULL;
  if (!computeTaylorModelOfLeastDegree(&T, func, dom, errMax, maximumAllowedN, singularity, prec)) {
    mpfr_clear(F);
    mpfr_clear(ell);
    mpfr_clear(gamma);
    mpfr_clear(u);
    mpfr_clear(thirtyoneThirtySecond);
    mpfr_clear(errMax);
    return SUPNORM_NO_TAYLOR;
  }
  
  /* Determine the sign of T in the middle of dom

     We really need to be sure of the sign, so 
     if we can't determine it, we fail.

  */
  mpfr_init2(midDom,sollya_mpfi_get_prec(dom));
  sollya_mpfi_mid(midDom,dom);
  if (!determineSignAtPoint(&signAsInt,T,midDom,prec)) {
    mpfr_clear(F);
    mpfr_clear(ell);
    mpfr_clear(gamma);
    mpfr_clear(u);
    mpfr_clear(thirtyoneThirtySecond);
    mpfr_clear(errMax);
    mpfr_clear(midDom);
    free_memory(T);
    return SUPNORM_CANNOT_DETERMINE_SIGN_OF_T;
  }

  /* Here, we know the sign of T at mid(dom), 
     we just have to translate it to an mpfr value */
  mpfr_init2(signT,12);
  if (signAsInt < 0) mpfr_set_si(signT,-1,GMP_RNDN); /* exact */
  else mpfr_set_si(signT,1,GMP_RNDN); /* exact */

  /* Compute bound that approximates bound = ell * (1 + accuracy/2) but surely no greater */
  mpfr_init2(bound,prec);
  mpfr_div_ui(bound,accuracy,2,GMP_RNDD); /* exact, but round-down anyway */
  mpfr_add_ui(bound,bound,1,GMP_RNDD); /* round-down to get lesser value */
  mpfr_mul(bound,ell,bound,GMP_RNDD); /* round-down to get lesser value */

  /* Integrate the sign of T at mid(dom) into bound */
  mpfr_mul(bound,bound,signT,GMP_RNDN); /* exact as signT one of -1 or 1 */
  
  /* Scale T by bound */
  boundTimesT = scalePolynomialExactly(T, bound); /* boundTimesT = |T|*ell*(1+accuracy/2) */
  
  /* Compute (build) s1 = boundTimesT - (p - T) and s2 = boundTimesT - (T - p) */
  pMinusT = subPolynomialsExactly(poly, T);
  TMinusp = subPolynomialsExactly(T, poly);
  s1 = subPolynomialsExactly(boundTimesT, pMinusT);
  s2 = subPolynomialsExactly(boundTimesT, TMinusp);
  
  /* Check now that s1 and s2 are both positive on the domain dom 

     The case when s1 or s2 have a point where they are not
     positive should never happen. The check just ensures the 
     safety of the algorithm, i.e. validates it.

  */
  if ((!showPositivity(s1, dom, prec)) ||
      (!showPositivity(s2, dom, prec))) {
    /* At least one of s1 or s2 has a point in dom where it is non-positive 
       
       We clear the used memory and return the appropriate failure code.

    */
    mpfr_clear(F);
    mpfr_clear(ell);
    mpfr_clear(gamma);
    mpfr_clear(u);
    mpfr_clear(thirtyoneThirtySecond);
    mpfr_clear(errMax);
    mpfr_clear(midDom);
    mpfr_clear(bound);
    mpfr_clear(signT);
    free_memory(T);
    free_memory(boundTimesT);
    free_memory(pMinusT);
    free_memory(TMinusp);
    free_memory(s1);
    free_memory(s2);
    return SUPNORM_COULD_NOT_SHOW_POSITIVITY;
  }

  /* Here, we know the supnorm as [ell,u] and we have validated it. */
  
  /* Set the result */
  sollya_mpfi_interv_fr(result,ell,u);

  /* Clear all used memory */
  mpfr_clear(F);
  mpfr_clear(ell);
  mpfr_clear(gamma);
  mpfr_clear(u);
  mpfr_clear(thirtyoneThirtySecond);
  mpfr_clear(errMax);
  mpfr_clear(midDom);
  mpfr_clear(bound);
  mpfr_clear(signT);
  free_memory(T);
  free_memory(boundTimesT);
  free_memory(pMinusT);
  free_memory(TMinusp);
  free_memory(s1);
  free_memory(s2);

  /* Return success */
  return SUPNORM_NO_ERROR;
}

/* Compute the supremum norm on eps = p/f - 1 over dom

   The supremum norm is computed with an enclosure error less than accuracy,
   i.e. the obtained interval [l,u] satisfies in the end:

   abs(u-l/l) <= accuracy

   If everything works fine, result is affected with an interval
   safely enclosing the supremum norm. The return value is then *zero*
   (i.e. SUPNORM_NO_ERROR).

   Otherwise, if an error occurs, the return value is non-zero. The
   number of the return value then corresponds to a special error
   message meaning (see #defines above).

   No warning message is ever displayed by this function.

   The computing precision is prec.

   We are ensured that this function is called only if 

   * poly is a polynomial
   * dom is a closed non-empty interval containing only numbers that is not reduced to a point,
   * accuracy is a positive number.

   In the case when the computation fails but there is hope in
   obtaining a result by bisection, the algorithm may assign a point
   in the interior of dom to bisectPoint. The global bisection code
   will then try to bisect at that point. If no value is assigned, the
   global bisection will be performed at the midpoint of dom. This
   means: if you just want default behavior for the bisection (in the
   midpoint), then do not touch bisectPoint.

   This function is supposed to overcome a removable singularity at
   singularity. There might be other singularities of poly/func-1 in
   the domain dom. In this case, the function may fail. It is not
   supposed to fail, though, if singularity is the only removable
   singularity of poly/func - 1 in the domain dom.

   However, we do not care about removable singularities in the
   expression of func. In such a case, the relative supnorm may simply
   fail for now. It may assign a new value to bisectPoint if desired.

*/
int supnormRelativeSingularity(sollya_mpfi_t result, node *poly, node *func, sollya_mpfi_t dom, mpfr_t accuracy, mpfr_t singularity, mp_prec_t prec, mpfr_t bisectPoint) {
  int deg, k, n, res;
  node *pTilde, *fTilde, *fTildeUnsimplified;
  mpfr_t kAsMpfr, mySingularity;

  /* Makes compiler happy */
  k = -1;
  /* End of compiler happiness */

  /* Determine the degree of poly */
  deg = getDegree(poly);
  if (deg < 0) {
    /* Strange things are happening, we return an error */
    return SUPNORM_SOME_ERROR;
  }
  
  /* Determine a maximum order */
  n = deg; 
  if (n < 2) n = 2;

  if (!determineOrderOfZero(&k, func, singularity, n, prec)) {
    /* We couldn't determine the order of the presumed singularity.
       Hence we fail with the appropriate error code.
    */
    return SUPNORM_CANNOT_DETERMINE_ORDER_OF_SINGU;
  }

  /* Now we know that func has a singularity at singularity of order k 
     We have to check now if we can divide poly by (x-singularity)^k
  */
  pTilde = NULL;
  if (!dividePolyByXMinusX0ToTheK(&pTilde,poly,singularity,k,prec)) {
    /* Here, we couldn't divide poly by (x-singularity)^k 
       We return the appropriate error code.
    */
    return SUPNORM_SINGULARITY_NOT_REMOVED;
  }

  /* If we are here, we know that pTilde = poly/((x-singularity)^k) 

     We now build fTilde = func/(x-singularity)^k

  */
  mpfr_init2(kAsMpfr,5 + 8 * sizeof(k));
  mpfr_set_si(kAsMpfr,k,GMP_RNDN); /* exact as per what precedes */
  fTildeUnsimplified = makeDiv(copyTree(func),
			       makePow(makeSub(makeVariable(),makeConstant(singularity)),
				       makeConstant(kAsMpfr)));
  fTilde = simplifyTreeErrorfree(fTildeUnsimplified);
  free_memory(fTildeUnsimplified);

  /* Now copy singularity into a local variable ('cause that stupid C,
     in some versions, prohibits taking a pointer on an argument)
  */
  mpfr_init2(mySingularity,mpfr_get_prec(singularity));
  mpfr_set(mySingularity,singularity,GMP_RNDN); /* exact, the precision is the same */

  /* Now call the relative supremum norm with pTilde and fTilde, passing on 
     the singularity as the development point for fTilde.
  */
  res = supnormRelativeNoSingularity(result, pTilde, fTilde, dom, accuracy, prec, &mySingularity, bisectPoint);

  /* Free all locally used memory */
  free_memory(pTilde);
  free_memory(fTilde);
  mpfr_clear(kAsMpfr);
  mpfr_clear(mySingularity);

  /* Return the result obtained */
  return res;
}


/* Compute the supremum norm on eps = p/f - 1 over dom

   The supremum norm is computed with an enclosure error less than accuracy,
   i.e. the obtained interval [l,u] satisfies in the end:

   abs(u-l/l) <= accuracy

   If everything works fine, result is affected with an interval
   safely enclosing the supremum norm. The return value is then *zero*
   (i.e. SUPNORM_NO_ERROR).

   Otherwise, if an error occurs, the return value is non-zero. The
   number of the return value then corresponds to a special error
   message meaning (see #defines above).

   No warning message is ever displayed by this function.

   The computing precision is prec.

   We are ensured that this function is called only if 

   * poly is a polynomial
   * dom is a closed non-empty interval containing only numbers that is not reduced to a point,
   * accuracy is a positive number.

   In the case when the computation fails but there is hope in
   obtaining a result by bisection, the algorithm may assign a point
   in the interior of dom to bisectPoint. The global bisection code
   will then try to bisect at that point. If no value is assigned, the
   global bisection will be performed at the midpoint of dom. This
   means: if you just want default behavior for the bisection (in the
   midpoint), then do not touch bisectPoint.

   This function is supposed to detect and overcome false
   singularities. However, it is also supposed to perform a fast check
   first, i.e. it is not supposed to do a length detection of zeros of
   func if the (IA) image of func over dom does not contain zero. The
   use of void evaluateInterval(sollya_mpfi_t y, node *func, node
   *deriv, sollya_mpfi_t x); comes handy here (remark that deriv may
   be set to NULL).

   To begin with, we do not care about removable singularities in the
   expression of func. In such a case, the relative supnorm may simply
   fail for now.

   However, removable singularities in poly/func must be detected
   (after a fast check if there aren't any) and overcome. This must
   work for cases when there is one singularity in dom, though. If
   there are several singularities, bisection will eventually split
   the interval. If appropriate set bisectPoint to something
   reasonable. 

*/
int supnormRelative(sollya_mpfi_t result, node *poly, node *func, sollya_mpfi_t dom, mpfr_t accuracy, mp_prec_t prec, mpfr_t bisectPoint) {
  int numberOfSingularities;
  mpfr_t singularity, myBisect, oldBisect;
  int degree, res;

  /* Initialize the result to "Error" */
  res = SUPNORM_SOME_ERROR;

  /* We use the degree of the polynomial as an indication of how many 
     removable singularities poly/func might maximally have.
  */
  degree = getDegree(poly);
  if (degree < 5) degree = 5;

  mpfr_init2(myBisect,mpfr_get_prec(bisectPoint));
  mpfr_set(myBisect,bisectPoint,GMP_RNDN); /* exact */

  mpfr_init2(singularity, prec);
  
  /* Quickly determine if there is a possible singularity and if yes, where it is and if it is the only one */
  numberOfSingularities = determinePossibleZeroAndBisectPoint(singularity, myBisect, func, dom, degree, prec);

  /* Do the right thing depending on how many singularities have been determined */
  if ((numberOfSingularities == 0) || (numberOfSingularities == -1)) {
    mpfr_clear(singularity);
    mpfr_clear(myBisect);

    /* Launch computation with the conviction that there is no singularity */
    res = supnormRelativeNoSingularity(result, poly, func, dom, accuracy, prec, NULL, bisectPoint);

    if ((res == SUPNORM_SOME_ERROR) && (numberOfSingularities == -1)) res = SUPNORM_SINGULARITY_NOT_DETECTED;
  } else {
    mpfr_init2(oldBisect,mpfr_get_prec(bisectPoint));
    mpfr_set(oldBisect,bisectPoint,GMP_RNDN);

    /* Launch computation with the conviction that there is a singularity at singularity */
    res = supnormRelativeSingularity(result, poly, func, dom, accuracy, singularity, prec, oldBisect);

    if (res != SUPNORM_NO_ERROR) {
      if (numberOfSingularities == 2) {
	/* The supnorm failed and we know of another possible
	   singularity and hence of a good bisection point */
	mpfr_set(bisectPoint, myBisect, GMP_RNDN); /* That's all approximations */
	if (res == SUPNORM_SOME_ERROR) res = SUPNORM_ANOTHER_SINGULARITY_IN_DOM;
      } else {
	/* The supnorm failed but we do not know of another possible bisection point 
	   
	   We hence set bisectPoint to the value that value suggested to us by the 
	   supnormRelativeSingularity function.

	*/
	mpfr_set(bisectPoint, oldBisect, GMP_RNDN); /* bisectPoint does not change 
						       if supnormRelativeSingularity didn't 
						       touch oldBisect */
      }
    }
    mpfr_clear(singularity);
    mpfr_clear(myBisect);
    mpfr_clear(oldBisect);
  }

  return res;
}

/* Compute the supremum norm on eps = p - f resp. eps = p/f - 1 over dom

   eps is defined according to the mode parameter:
   if mode = ABSOLUTE then eps = p - f else eps = p/f -1

   The supremum norm is computed with an enclosure error less than accuracy,
   i.e. the obtained interval [l,u] satisfies in the end:

   abs(u-l/l) <= accuracy

   If everything works fine, result is affected with an interval
   safely enclosing the supremum norm. The return value is then *zero*
   (i.e. SUPNORM_NO_ERROR).

   Otherwise, if an error occurs, the return value is non-zero. The
   number of the return value then corresponds to a special error
   message meaning (see #defines above).

   No warning message is ever displayed by this function.

   The computing precision is prec.

   We are ensured that this function is called only if 

   * poly is a polynomial
   * dom is a closed non-empty interval containing only numbers that is not reduced to a point,
   * accuracy is a positive number.

   In the case when the computation fails but there is hope in
   obtaining a result by bisection, the algorithm may assign a point
   in the interior of dom to bisectPoint. The global bisection code
   will then try to bisect at that point. If no value is assigned, the
   global bisection will be performed at the midpoint of dom. This
   means: if you just want default behavior for the bisection (in the
   midpoint), then do not touch bisectPoint.

*/
int supremumNormInner(sollya_mpfi_t result, node *poly, node *func, sollya_mpfi_t dom, int mode, mpfr_t accuracy, mp_prec_t prec, mpfr_t bisectPoint) {
  int res;

  if (mode == ABSOLUTE) {
    res = supnormAbsolute(result,poly,func,dom,accuracy,prec,bisectPoint);
  } else {
    res = supnormRelative(result,poly,func,dom,accuracy,prec,bisectPoint);
  }

  return res;
}

/* Compute the supremum norm on eps = p - f resp. eps = p/f - 1 over [a,b]

   eps is defined according to the mode parameter:
   if mode = ABSOLUTE then eps = p - f else eps = p/f -1

   The supremum norm is computed with an enclosure error less than accuracy,
   i.e. the obtained interval [l,u] satisfies in the end:

   abs(u-l/l) <= accuracy

   If everything works fine, result is affected with an interval
   safely enclosing the supremum norm. The return value is then *zero*
   (i.e. SUPNORM_NO_ERROR).

   Otherwise, if an error occurs, the return value reflects the last
   error message of the recursive calls.

   We are ensured that this function is called only if 

   * poly is a polynomial
   * a and b, a < b, form an interval [a,b] that is not reduced to a point
   * accuracy is a positive number,
   * diameter is a non-negative number.

   The algorithm is allowed to stop if a result has been found or if
   abs(b-a) is less than diameter.

*/
int supremumNormBisectInner(sollya_mpfi_t result, node *poly, node *func, mpfr_t a, mpfr_t b, int mode, mpfr_t accuracy, mpfr_t diameter, mp_prec_t prec) {
  int resFirst, resLeft, resRight;
  mp_prec_t p1, p2;
  sollya_mpfi_t dom, resultLeft, resultRight;
  mpfr_t c, width;
  mpfr_t ll, lr, ul, ur;

  p1 = mpfr_get_prec(a);
  p2 = mpfr_get_prec(b);
  if (p2 > p1) p1 = p2;
  sollya_mpfi_init2(dom,p1);
  sollya_mpfi_interv_fr(dom,a,b);

  mpfr_init2(c, p1 + 1);
  mpfr_add(c,a,b,GMP_RNDN);
  mpfr_div_2ui(c,c,1,GMP_RNDN);

  /* Call inner supnorm algorithm on whole interval */
  resFirst = supremumNormInner(result, poly, func, dom, mode, accuracy, prec, c);

  sollya_mpfi_clear(dom);

  /* If everything worked fine on the whole interval, we do not need to bisect */
  if (resFirst == SUPNORM_NO_ERROR) {
    mpfr_clear(c);
    return SUPNORM_NO_ERROR;
  }

  /* Some error occured, check if we still need to bisect or if we have reached diameter */
  mpfr_init2(width,p1);
  mpfr_sub(width,b,a,GMP_RNDU);
  if (mpfr_cmp(width,diameter) < 0) {
    /* We have reached diameter, we return the latest error code */
    mpfr_clear(c);
    mpfr_clear(width);
    return resFirst;
  }

  mpfr_clear(width);

  /* Here, we have to bisect.

     We check that the bisection point is a number in the interior of [a,b].
     If it is not, we set it to the middle of [a,b].

  */
  if ((!mpfr_number_p(c)) ||
      (mpfr_cmp(c,a) <= 0) ||
      (mpfr_cmp(b,c) <= 0)) {
    mpfr_add(c,a,b,GMP_RNDN);
    mpfr_div_2ui(c,c,1,GMP_RNDN);
  }

  /* Bisect */
  p2 = sollya_mpfi_get_prec(result);
  sollya_mpfi_init2(resultLeft,p2);

  resLeft = supremumNormBisectInner(resultLeft, poly, func, a, c, mode, accuracy, diameter, prec);

  if (resLeft != SUPNORM_NO_ERROR) {
    /* The bisection recursively failed on the left sub-interval [a,c] 
       Return the error code returned by that recursive call.
    */
    mpfr_clear(c);
    sollya_mpfi_clear(resultLeft);
    return resLeft;
  }
  /* Here, resLeft == SUPNORM_NO_ERROR */

  sollya_mpfi_init2(resultRight,p2);

  resRight = supremumNormBisectInner(resultRight, poly, func, c, b, mode, accuracy, diameter, prec);

  if (resRight != SUPNORM_NO_ERROR) {
    /* The bisection recursively failed on the right sub-interval [c,b] 
       Return the error code returned by that recursive call.
    */
    mpfr_clear(c);
    sollya_mpfi_clear(resultLeft);
    sollya_mpfi_clear(resultRight);
    return resRight;
  }
  
  /* Here, resLeft == SUPNORM_NO_ERROR and resRight == SUPNORM_NO_ERROR

     This means both recursive calls worked without error. 
     
     We combine the results by taking the max of the lower resp. the
     upper bounds.
     
  */
  mpfr_init2(ll,p2);
  mpfr_init2(lr,p2);
  mpfr_init2(ul,p2);
  mpfr_init2(ur,p2);
  
  sollya_mpfi_get_left(ll,resultLeft);
  sollya_mpfi_get_right(ul,resultLeft);
  sollya_mpfi_get_left(lr,resultRight);
  sollya_mpfi_get_right(ur,resultRight);

  if (mpfr_cmp(ll,lr) > 0) mpfr_set(lr,ll,GMP_RNDN); /* exact */
  if (mpfr_cmp(ul,ur) > 0) mpfr_set(ur,ul,GMP_RNDN); /* exact */

  sollya_mpfi_interv_fr(result,lr,ur);

  mpfr_clear(ll);
  mpfr_clear(lr);
  mpfr_clear(ul);
  mpfr_clear(ur);
  sollya_mpfi_clear(resultLeft);
  sollya_mpfi_clear(resultRight);
  mpfr_clear(c);
  return SUPNORM_NO_ERROR;
}

/* Compute the supremum norm on eps = p - f resp. eps = p/f - 1 over [a,b]

   eps is defined according to the mode parameter:
   if mode = ABSOLUTE then eps = p - f else eps = p/f -1

   The supremum norm is computed with an enclosure error less than accuracy,
   i.e. the obtained interval [l,u] satisfies in the end:

   abs(u-l/l) <= accuracy

   If everything works fine, result is affected with an interval 
   safely enclosing the supremum norm. The return value is then non-zero.

   Otherwise, if an error occurs, the return value is 0.

   We are ensured that this function is called only if 

   * poly is a polynomial
   * a and b, a < b, form an interval [a,b] that is not reduced to a point
   * accuracy is a positive number,
   * diameter is a non-negative number.

   The algorithm is allowed to stop if a result has been found or if
   abs(b-a) is less than diameter.

*/
int supremumNormBisect(sollya_mpfi_t result, node *poly, node *func, mpfr_t a, mpfr_t b, int mode, mpfr_t accuracy, mpfr_t diameter) {
  int res;
  mp_prec_t prec, p;
  mpfr_t temp;
  prec = getToolPrecision() + 25;
  //p = sollya_mpfi_get_prec(result);
 
  /* Compute p = -floor(log2(accuracy)) to get the number of bits we need
     in the end
  */
  mpfr_init2(temp, 8 * sizeof(mp_prec_t) + 10);
  mpfr_log2(temp,accuracy,GMP_RNDD);
  mpfr_floor(temp,temp);
  mpfr_neg(temp,temp,GMP_RNDU);
  p = mpfr_get_ui(temp,GMP_RNDD);
  mpfr_clear(temp);
  
  /*if the requested accuracy (p) is close to prec, increase prec*/
  if (abs(p-prec)<(p/2)) {
    if (p>prec)  prec = p + (p/2);
    if (p<=prec)  prec = prec + (p/2);
  }
  res = supremumNormBisectInner(result, poly, func, a, b, mode, accuracy, diameter, prec);

  if (res == 0) return 1; /* everything's fine */
 
  /* In the following, perform error handling (messaging and return 0) */
  switch (res) {
  case SUPNORM_NO_TAYLOR:
    printMessage(1,"Warning: during supnorm computation, no suitable Taylor form could be found.\n");
    break;
  case SUPNORM_NOT_ENOUGH_WORKING_PRECISION:
    printMessage(1,"Warning: during supnorm computation, no result could be found as the working precision seems to be too low.\n");
    break;
  case SUPNORM_SINGULARITY_NOT_REMOVED:
    printMessage(1,"Warning: during supnorm computation, a singularity in the expression tree could not be removed.\n");
    break;
  case SUPNORM_COULD_NOT_SHOW_POSITIVITY:
    printMessage(1,"Warning: during supnorm computation, the positivity of a polynomial could not be established.\n");
    break;
  case SUPNORM_SINGULARITY_NOT_DETECTED:
    printMessage(1,"Warning: during supnorm computation, a false singularity could not be detected.\n");
    break;
  case SUPNORM_ANOTHER_SINGULARITY_IN_DOM:
    printMessage(1,"Warning: during supnorm computation, there appeared to be at least two singularities in the domain. More bisection is needed.\n");
    break;
  case SUPNORM_CANNOT_COMPUTE_LOWER_BOUND:
    printMessage(1,"Warning: during supnorm computation, it was not possible to determine a valid lower bound for the error function.\n");
    break;
  case SUPNORM_CANNOT_COMPUTE_ABSOLUTE_INF:
    printMessage(1,"Warning: during supnorm computation, it was not possible to determine a valid lower bound for the absolute value of the function.\n");
    break;
  case SUPNORM_CANNOT_DETERMINE_SIGN_OF_T:
    printMessage(1,"Warning: during supnorm computation, it was not possible to safely determine the sign of the Taylor polynomial.\n");
    break;
  case SUPNORM_CANNOT_DETERMINE_ORDER_OF_SINGU:
    printMessage(1,"Warning: during supnorm computation, it was not possible to safely determine the order of a presume zero of the given function.\n");
    break;    
  default:
    printMessage(1,"Warning: during supnorm computation, some generic error occured. No further description is available.\n");
  }

  return 0;
}

/* Compute the supremum norm on eps = p - f resp. eps = p/f - 1 at a,
   i.e. evaluate abs(eps) at a.

   eps is defined according to the mode parameter:
   if mode = ABSOLUTE then eps = p - f else eps = p/f -1

   The supremum norm is computed with an enclosure error less than accuracy,
   i.e. the obtained interval [l,u] satisfies in the end:

   abs(u-l/l) <= accuracy

   If everything works fine, result is affected with an interval 
   safely enclosing the supremum norm. The return value is then non-zero.

   Otherwise, if an error occurs, the return value is 0.

   We are ensured that this function is called only if 

   * poly is a polynomial
   * a is a number,
   * accuracy is a positive number,
   * diameter is a non-negative number.

   */
int supremumNormDegenerate(sollya_mpfi_t result, node *poly, node *func, mpfr_t a, int mode, mpfr_t accuracy) {
  node *absEps;
  int res;
  mpfr_t temp, y, ya, yb;
  unsigned int pr;
  mp_prec_t prec, pp;
  int tempRes;
  mpfr_t absAccuracy;

  if (mode == ABSOLUTE) {
    /* Construct absEps = abs(poly - func) */
    absEps = makeAbs(makeSub(copyTree(poly),copyTree(func)));
  } else {
    /* Construct absEps = abs(poly/func - 1) */
    absEps = makeAbs(makeSub(makeDiv(copyTree(poly),copyTree(func)),makeConstantDouble(1.0)));
  }

  /* Compute pr = -floor(log2(accuracy)) to get the number of bits we need
     in the end
  */
  
  mpfr_init2(temp, 8 * sizeof(mp_prec_t) + 10);
  mpfr_init2(absAccuracy,mpfr_get_prec(accuracy));
  mpfr_abs(absAccuracy,accuracy,GMP_RNDN); /* exact */
  mpfr_log2(temp,absAccuracy,GMP_RNDD);
  mpfr_clear(absAccuracy);
  mpfr_floor(temp,temp);
  mpfr_neg(temp,temp,GMP_RNDU);
  pr = mpfr_get_ui(temp,GMP_RNDD);
  mpfr_clear(temp);

  prec = getToolPrecision();
  if (pr > 2048 * prec) {
    printMessage(1,"Warning: the given accuracy depasses the current maximum precision of %d bits.\n",2048 * prec);
    printMessage(1,"Try to increase the precision of the tool.\n");
    sollya_mpfi_set_nan(result);
    free_memory(absEps);
    return 0;
  }

  if (pr < prec) pp = prec; else pp = prec;
  pp += 10;
 
  mpfr_init2(y,pp);

  tempRes = evaluateFaithful(y, absEps, a, pp);

  res = 0;
  if (tempRes == 1) {
    res = 1;
    pp = mpfr_get_prec(y) - 5;
    mpfr_init2(ya,pp);
    mpfr_init2(yb,pp); 
    mpfr_set(ya,y,GMP_RNDD);
    mpfr_set(yb,y,GMP_RNDU);
    mpfr_nextbelow(ya);
    mpfr_nextbelow(ya);
    mpfr_nextabove(yb);
    mpfr_nextabove(yb);
    if (mpfr_sgn(ya) < 0) {
      mpfr_set_si(ya,0,GMP_RNDN);
    }

    sollya_mpfi_interv_fr(result,ya,yb);

    mpfr_clear(ya);
    mpfr_clear(yb);
  } else {
    printMessage(1,"Warning: could not perform a faithful evaluation of the error function between the given polynomial and the given function at the given point.\n");
    sollya_mpfi_set_nan(result);
  }

  free_memory(absEps);
  mpfr_clear(y);

  return res;
}


/* Checks if all coefficients of poly can be written as ratios of
   floating-point numbers 

   Returns 0 if poly is not a polynomial or is a polynomial that does
   contain irrational coefficients.

   Returns a non-zero value otherwise.

*/
int hasOnlyMpqCoefficients(node *poly) {
  node **coefficients;
  int degree, i, res, okay;
  node *simplified;

  if (!isPolynomial(poly)) return 0;

  getCoefficients(&degree,&coefficients,poly);
  if (degree < 0) return 0;

  res = 1;
  for (i=0;i<=degree;i++) {
    if (coefficients[i] != NULL) {
      simplified = simplifyRationalErrorfree(coefficients[i]);
      okay = 0;
      if ((simplified->nodeType == CONSTANT) &&
	  (mpfr_number_p(*(simplified->value)))) okay = 1;
      else {
	if ((simplified->nodeType == DIV) &&
	    (((simplified->child1->nodeType == CONSTANT) && 
	      (mpfr_number_p(*(simplified->child1->value)))) && 
	     ((simplified->child2->nodeType == CONSTANT) && 
	      (mpfr_number_p(*(simplified->child2->value)))))) okay = 1;
      }
      free_memory(simplified);
      if (!okay) {
	res = 0;
	break;
      }
    }
  }

  for (i=0;i<=degree;i++) {
    if (coefficients[i] != NULL) free_memory(coefficients[i]);
  }
  free(coefficients);

  return res;
}



/* Compute the supremum norm on eps = p - f resp. eps = p/f - 1 over dom

   eps is defined according to the mode parameter:
   if mode = ABSOLUTE then eps = p - f else eps = p/f -1

   The supremum norm is computed with an enclosure error less than accuracy,
   i.e. the interval [l,u] obtained satisfies in the end:

   abs(u-l/l) <= abs(accuracy)

   If everything works fine, result is affected with an interval 
   safely enclosing the supremum norm. The return value is then non-zero.

   Otherwise, if an error occurs, the return value is 0 and result is
   affected with [NaN;NaN]. A warning is printed in this case.
*/
int supremumnorm(sollya_mpfi_t result, node *poly, node *func, sollya_mpfi_t dom, int mode, mpfr_t accuracy) {
  mpfr_t a, b, diameter, temp, absAccuracy;
  mp_prec_t tempPrec;
  int res;

  if (!isPolynomial(poly)) {
    printMessage(1,"Warning: the given expression is not a polynomial.\n");
    sollya_mpfi_set_nan(result);
    return 0;
  }

  tempPrec = sollya_mpfi_get_prec(dom);
  mpfr_init2(a,tempPrec);
  mpfr_init2(b,tempPrec);
  sollya_mpfi_get_left(a,dom);
  sollya_mpfi_get_right(b,dom);

  if (!(mpfr_number_p(a) && mpfr_number_p(b))) {
    printMessage(1,"Warning: the given domain is not a closed interval on the reals.\n");
    sollya_mpfi_set_nan(result);
    mpfr_clear(a);
    mpfr_clear(b);
    return 0;
  }

  if (mpfr_cmp(a,b) > 0) {
    printMessage(1,"Warning: the given domain is empty.\n");
    sollya_mpfi_set_nan(result);
    mpfr_clear(a);
    mpfr_clear(b);
    return 0;
  }

  if (mpfr_cmp(a,b) == 0) {
    printMessage(1,"Warning: the given domain is reduced to a point. Replacing the supremum norm with an evaluation.\n");
    res = supremumNormDegenerate(result,poly,func,a,mode,accuracy);
    if (!res) {
      printMessage(1,"Warning: could not evaluate the error function between the given polynomial and the given function at this point.\n");
      sollya_mpfi_set_nan(result);
    } 
    mpfr_clear(a);
    mpfr_clear(b);
    return 1;
  }

  if (!mpfr_number_p(accuracy)) {
    printMessage(1,"Warning: the given accuracy is not a real number.\n");
    sollya_mpfi_set_nan(result);
    mpfr_clear(a);
    mpfr_clear(b);
    return 0;
  }

  if (mpfr_zero_p(accuracy)) {
    printMessage(1,"Warning: the given accuracy is zero. In order to ensure the termination of the supremum norm algorithm, the accuracy parameter must be non-zero.\n");
    sollya_mpfi_set_nan(result);
    mpfr_clear(a);
    mpfr_clear(b);
    return 0;
  }

  if (!hasOnlyMpqCoefficients(poly)) {
    printMessage(1,"Warning: the coefficients of the given polynomial cannot all be written as ratios of floating-point numbers.\nSupremum norm computation is only possible on such polynomials. Try to use roundcoefficients().\n");
    sollya_mpfi_set_nan(result);
    mpfr_clear(a);
    mpfr_clear(b);
    return 0;
  }

  /* Here, we know that the interval is proper (no NaNs, no Infs) and
     that it is not reduced to a point. We know that accuracy is a
     non-zero number and that poly is a polynomial whose coefficients 
     can be written in floating-point numbers or ratios of floating-point
     numbers.

     We will call supremumNormBisect with diam * width(dom) and abs(accuracy).

  */

  mpfr_init2(temp,tempPrec * 4);
  mpfr_init2(diameter, tempPrec * 4 + 53);
  mpfr_sub(temp,b,a,GMP_RNDU);
  getToolDiameter(diameter);
  mpfr_mul(diameter,diameter,temp,GMP_RNDU);
  mpfr_abs(diameter,diameter,GMP_RNDN);

  mpfr_init2(absAccuracy,mpfr_get_prec(accuracy));
  mpfr_abs(absAccuracy,accuracy,GMP_RNDN); /* exact */

  res = supremumNormBisect(result,poly,func,a,b,mode,absAccuracy,diameter);
  if (!res) {
    printMessage(1,"Warning: an error occured during supremum norm computation. A safe enclosure of the supremum norm could not be computed.\n");
    sollya_mpfi_set_nan(result);
  } 

  mpfr_clear(a);
  mpfr_clear(b);
  mpfr_clear(temp);
  mpfr_clear(diameter);
  mpfr_clear(absAccuracy);

  return res;
}




