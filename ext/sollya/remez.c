/*

Copyright 2006-2011 by

Laboratoire de l'Informatique du Parallelisme,
UMR CNRS - ENS Lyon - UCB Lyon 1 - INRIA 5668

LORIA (CNRS, INPL, INRIA, UHP, U-Nancy 2),

Laboratoire d'Informatique de Paris 6, equipe PEQUAN,
UPMC Universite Paris 06 - CNRS - UMR 7606 - LIP6, Paris, France,

and by

Centre de recherche INRIA Sophia-Antipolis Mediterranee, equipe APICS,
Sophia Antipolis, France.

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
herefore means  that it is reserved for developers  and  experienced
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
#include "chain.h"
#include "general.h"
#include "infnorm.h"
#include "execute.h"
#include <stdio.h> /* fprintf, fopen, fclose, */
#include <stdlib.h> /* exit, free, mktemp */
#include <errno.h>

#define coeff(i,j,n) ((i)-1)*(n)+(j)-1
#define NEWTON_STEPS 2

void myPrintValue(mpfr_t *x, mp_prec_t prec) {
  mpfr_t y;
  mpfr_init2(y,prec);
  mpfr_set(y,*x,GMP_RNDN);
  printValue(&y);
  mpfr_clear(y);
}

void printMatrix(mpfr_t *M, int n) {
  int i,j;
 sollyaPrintf("[");
  for(i=1;i<=n;i++) {
    for(j=1;j<=n;j++) {
      myPrintValue(&M[coeff(i,j,n)],53); if(j!=n) sollyaPrintf(", ");
    }
    if(i!=n) sollyaPrintf(";\n");
  }
  sollyaPrintf("]\n");
  return;
}

void system_solve(mpfr_t *res, mpfr_t *M, mpfr_t *b, int n, mp_prec_t prec) {
  chain *i_list=NULL;
  chain *j_list=NULL;
  chain *curri;
  chain *currj;
  int i0, j0, i, j, k;
  int *var;
  mpfr_t max,lambda;
  int *order_i = safeMalloc(n*sizeof(int));
  int *order_j = safeMalloc(n*sizeof(int));

  mpfr_init2(max, 53);
  mpfr_init2(lambda, prec);

  for(i=1;i<=n;i++) {
    var = safeMalloc(sizeof(int));
    *var = i;
    i_list = addElement(i_list, (void *)var);
  }
  for(j=1;j<=n;j++) {
    var = safeMalloc(sizeof(int));
    *var = j;
    j_list = addElement(j_list, (void *)var);
  }


  // Triangulation by Gaussian elimination
  i0 = j0 = -1;
  for(k=1;k<=n;k++) {
    mpfr_set_d(max, 0., GMP_RNDN); //exact

    // In this part, we search for the bigger element of the matrix
    curri = i_list;
    while(curri!=NULL) {
      currj = j_list;
      while(currj!=NULL) {
	i = *(int *)(curri->value);
	j = *(int *)(currj->value);
	if(mpfr_cmpabs(M[coeff(i,j,n)],max)>=0) {
	  i0 = i;
	  j0 = j;
	  mpfr_set(max, M[coeff(i,j,n)], GMP_RNDN);
	}
	currj = currj->next;
      }
      curri = curri->next;
    }

    i_list = removeInt(i_list, i0);
    j_list = removeInt(j_list, j0);

    order_i[k-1] = i0;
    order_j[k-1] = j0;

    // Here we update the matrix and the second member
    curri = i_list;
    while(curri!=NULL) {
      i = *(int *)(curri->value);
      mpfr_div(lambda, M[coeff(i,j0,n)], M[coeff(i0,j0,n)], GMP_RNDN);
      mpfr_neg(lambda, lambda, GMP_RNDN);

      currj = j_list;
      while(currj!=NULL) {
	j = *(int *)(currj->value);
	mpfr_fma(M[coeff(i,j,n)], lambda, M[coeff(i0,j,n)], M[coeff(i,j,n)], GMP_RNDN);
	currj = currj->next;
      }

      mpfr_fma(b[i-1], lambda, b[i0-1], b[i-1], GMP_RNDN);
      mpfr_set_d(M[coeff(i,j0,n)], 0., GMP_RNDN); // this line is not useful strictly speaking
      curri = curri->next;
    }
  }
  /*********************************************************************/


  // Resolution of the system itself
  for(i=1;i<=n;i++) {
    var = safeMalloc(sizeof(int));
    *var = i;
    i_list = addElement(i_list, (void *)var);
  }

  for(k=n;k>=1;k--) {
    i0 = order_i[k-1];
    j0 = order_j[k-1];
    mpfr_div(res[j0-1], b[i0-1], M[coeff(i0,j0,n)], GMP_RNDN);
    i_list = removeInt(i_list, i0);

    curri = i_list;
    while(curri!=NULL) {
      i = *(int *)(curri->value);
      mpfr_neg(M[coeff(i,j0,n)], M[coeff(i,j0,n)], GMP_RNDN);
      mpfr_fma(b[i-1], M[coeff(i,j0,n)], res[j0-1], b[i-1], GMP_RNDN);
      curri=curri->next;
    }
  }

  free(order_i);
  free(order_j);
  freeChain(i_list, freeIntPtr);
  freeChain(j_list, freeIntPtr);
  mpfr_clear(max);
  mpfr_clear(lambda);
  return;
}


// Bubble sort
void mpfr_sort(mpfr_t *vect, int n, mp_prec_t prec) {
  int i,j;
  mpfr_t var;

  mpfr_init2(var, prec);

  for(i=1;i<=n-1;i++) {
    for(j=n;j>=i+1;j--) {
      if(mpfr_cmp(vect[j-1], vect[j-2])<=0) {
	mpfr_set(var, vect[j-1], GMP_RNDN);
	mpfr_set(vect[j-1], vect[j-2], GMP_RNDN);
	mpfr_set(vect[j-2], var, GMP_RNDN);
      }
    }
  }

  mpfr_clear(var);
  return;
}



// Constructs the tree corresponding to p = sum(coeff(i) X^monomials[i])
// The array coeff is supposed to have at least as many elements as monomials
node *constructPolynomial(mpfr_t *coeff, chain *monomials, mp_prec_t prec) {
  int i=1;
  chain *curr;
  node *temp1;
  node *temp2;
  node *temp3;
  node *temp4;
  node *temp5;
  node *temp6;

  node *poly;
  mpfr_t *ptr;

  poly =  safeMalloc(sizeof(node));
  poly->nodeType = CONSTANT;
  ptr = safeMalloc(sizeof(mpfr_t));
  mpfr_init2(*ptr, prec);
  mpfr_set_d(*ptr, 0., GMP_RNDN);
  poly->value = ptr;

  curr = monomials;
  while(curr != NULL) {
    temp1 = safeMalloc(sizeof(node));
    temp1->nodeType = ADD;

    temp2 = safeMalloc(sizeof(node));
    temp2->nodeType = MUL;

    temp3 = safeMalloc(sizeof(node));
    temp3->nodeType = CONSTANT;
    ptr = safeMalloc(sizeof(mpfr_t));
    mpfr_init2(*ptr, prec);
    mpfr_set(*ptr, coeff[i-1], GMP_RNDN);
    temp3->value = ptr;

    temp4 = safeMalloc(sizeof(node));
    temp4->nodeType = POW;

    temp5 = safeMalloc(sizeof(node));
    temp5->nodeType = VARIABLE;

    temp6 = safeMalloc(sizeof(node));
    temp6->nodeType = CONSTANT;
    ptr = safeMalloc(sizeof(mpfr_t));
    mpfr_init2(*ptr, prec);
    mpfr_set_si(*ptr, *((int *)(curr->value)), GMP_RNDN);
    temp6->value = ptr;

    temp4->child1 = temp5;
    temp4->child2 = temp6;
    temp2->child1 = temp3;
    temp2->child2 = temp4;
    temp1->child1 = temp2;
    temp1->child2 = poly;
    poly = temp1;
    i++;
    curr = curr->next;
  }

  return poly;
}



// This function finds an approximate zero of a function f
// (which derivative if f_diff) in the interval [a;b]
// using x0 as an initial guess of the zero
// If n=0 the algorithm stops when the computed zero is
// a precise estimation of the real zero.
// If n<>0, n steps are computed.
// The algorithm uses Newton's method
// It is assumed that f(a)f(b)<=0 and x0 in [a;b]
// If x0=NULL the algorithm is free to use any initial guess
void findZero(mpfr_t res, node *f, node *f_diff, mpfr_t a, mpfr_t b, int sgnfa, mpfr_t *x0, int n, mp_prec_t prec) {

  /* Sketch of the algorithm :
     During the algorithm, we will work with an interval [u,v]
     that surely contains a zero of f.
     Whenever a step of Newton's algorithm does not work, we perform a bisection step on [u,v]
     to ensure the convergence of the algorithm.

     1) It is necessary that [u,v] does not contain 0. This ensures that the range of
     possible exponents for x is bounded.

     So, if 0<a or b<0,  let [u,v]=[a,b] and go directly to step 2.

     Otherwise:
       Let epsa= max(-2^(-2p), a)    and   epsb = min(2^(-2p), b):
       hence a <= epsa <= 0 <= epsb <= b.
       Let signepsa = sign(f(epsa)), sign0 = sign(f(0)) and signepsb = sign(f(epsb)).
       Here sign can be -1,0,1 or NaN if the evaluation of f fails.
       We separate the different cases for (signepsa, sign0, signepsb):

	 (   *       0      *   ) -> f(0)=0 exactly
	 (   0       *      *   ) -> f(epsa)=0 exactly
	 (   *       *      0   ) -> f(epsb)=0 exactly
	 (-sgnfa     *      *   ) -> we can set [u,v] = [a, epsa] (note that epsa<0 in this case)
	 (   *       *     sgnfa) -> we can set [u,v] = [epsb, b] (note that epsb>0 in this case)

	 (   *     sgnfa  -sgnfa) -> f changes its sign in [0,epsb] but f(0) != 0.
	                             The zero of f cannot be determined accurately.
				     We can return epsb for instance with an error message.
	 ( sgnfa  -sgnfa    *   ) -> idem with [epsa,0]

	 ( sgnfa    NaN   -sgnfa) -> f changes its sign in [epsa, epsb].
	                             It is likely that f(0)=0 but we cannot prove it.
				     We can return 0 with an error message.

	 ( sgnfa    NaN     NaN ) -> This case is likely to be b=0 (hence epsb=0)
	                             We cannot prove much but it is reasonnable to return 0
	 (  NaN     NaN   -sgnfa) -> idem with a=0
	 (  NaN     NaN     NaN ) -> idem with a=0=b

	 (  *      sgnfa    NaN ) -> We are really in trouble here since f(0) is provably non zero
                                     but we do not know if f vanishes in [0,epsb] or [epsb, b].
				     We should better stop with a huge ERROR
	 (  NaN   -sgnfa     *  ) -> idem with [a, epsa] or [epsa, 0]


     2) Now we are sure that [u,v] does not contain 0.

     During the algorithm, we will consider an approximation x of the zero z of f.
     From x, we compute a new approximation xNew.
     We assume that xNew is a very good approximation of z. Thus we can estimate the
     relative error between x and z by |xNew-x|/|xNew|.
     If this error is smaller than 2^(-prec), x is an accurate estimation of z and a fortiori
     xNew is accurate as well.

     Let x be somewhere in [u,v] (in the beginning, x=x0 if x0 is defined and lies in [u,v]).
     We compute xNew = iterator(x) = x - f(x)/f'(x)

       If (xNew<u) or (xNew>v) or xNew=NaN, we perform a bisection step:
       replaced by either [u, m] of [m, u] where m=(u+v)/2 and xNew is defined as
       the middle of the new interval

        Otherwise: xNew gives a valid new value. We compute yNew=f(xNew).
       We have 4 cases depending on sgn(yNew):
         sgnfa -> we can replace [u,v] by [xNew,v]
	-sgnfa -> we can replace [u,v] by [u, xNew]
	   0   -> xNew is an exact zero of f. We can stop
	  NaN  -> we leave [u,v] as is. Most likely the iterator(xNew) will be NaN and the next
	          step will be a bisection step.
  */

  mpfr_t zero_mpfr;
  mpfr_t u, v, epsa, fepsa, epsb, fepsb, f0;
  int sgnfepsa, sgnfepsb, sgnf0;
  int codefa, codeNegfa;
  int stop_algo = 0;
  int skip_step1 = 0;
  mp_prec_t prec_bounds;
  int r;

  node *iterator;
  node *temp;
  mpfr_t x, xNew, yNew, tmp_mpfr;
  int estim_prec, estim_prec2;
  int nbr_iter;

  /* Make compiler happy: */
  estim_prec = 12;
  nbr_iter = 2;
  /* End of compiler happiness */

  if(verbosity>=8) {
    changeToWarningMode();
    sollyaPrintf("Information (Newton's algorithm): entering in Newton's algorithm. Parameters are:\n");
    sollyaPrintf("Information (Newton's algorithm): f = "); printTree(f); sollyaPrintf("\n");
    sollyaPrintf("Information (Newton's algorithm): a = "); printMpfr(a);
    sollyaPrintf("Information (Newton's algorithm): b = "); printMpfr(b);
    if (x0!=NULL) { sollyaPrintf("Information (Newton's algorithm): x0 = "); printMpfr(*x0);}
    restoreMode();
  }
  
  prec_bounds = (mpfr_get_prec(a)>mpfr_get_prec(b)) ? mpfr_get_prec(a) : mpfr_get_prec(b);
  if (prec>prec_bounds) prec_bounds = prec;

  mpfr_init2(u, prec_bounds);
  mpfr_init2(v, prec_bounds);
  mpfr_init2(zero_mpfr, prec);
  
  mpfr_set_si(zero_mpfr, 0, GMP_RNDN);

  if ( (mpfr_cmp_ui(a,0)>0) || mpfr_cmp_ui(b,0)<0 ) {
    mpfr_set(u, a, GMP_RNDU); // exact
    mpfr_set(v, b, GMP_RNDD); // exact
    skip_step1 = 1;
  }

  /**************    STEP 1 : removing zero from the range    **************/
  if (!skip_step1) {
    mpfr_init2(epsa, prec);
    mpfr_init2(epsb, prec);
    mpfr_init2(fepsa, prec);
    mpfr_init2(fepsb, prec);
    mpfr_init2(f0, prec);
    
    mpfr_set_si(epsa, -1, GMP_RNDU);
    mpfr_div_2ui(epsa, epsa, 2*prec, GMP_RNDU);
    if (mpfr_cmp(a,epsa)>0) mpfr_set(epsa, a, GMP_RNDU);

    mpfr_set_si(epsb, 1, GMP_RNDD);
    mpfr_div_2ui(epsb, epsb, 2*prec, GMP_RNDD);
    if (mpfr_cmp(epsb,b)>0) mpfr_set(epsb, b, GMP_RNDD);

    /* For the signs, we use the following convention:
        0 is coded by 0
	1 is coded by 1
       -1 is coded by 2
	NaN is coded by 3
    */ 
    r = evaluateFaithfulWithCutOffFast(fepsa, f, f_diff, epsa, zero_mpfr, prec);
    if(r==0) mpfr_set_d(fepsa,0,GMP_RNDN);
    if (!mpfr_number_p(fepsa)) sgnfepsa = 3;
    else {
      sgnfepsa = mpfr_sgn(fepsa);
      if (sgnfepsa!=0) sgnfepsa = (sgnfepsa>0) ? 1 : 2;
    }

    r = evaluateFaithfulWithCutOffFast(f0, f, f_diff, zero_mpfr, zero_mpfr, prec);
    if(r==0) mpfr_set_d(f0,0,GMP_RNDN);
    if (!mpfr_number_p(f0)) sgnf0 = 3;
    else {
      sgnf0 = mpfr_sgn(f0);
      if (sgnf0!=0) sgnf0 = (sgnf0>0) ? 1 : 2;
    }

    r = evaluateFaithfulWithCutOffFast(fepsb, f, f_diff, epsb, zero_mpfr, prec);
    if(r==0) mpfr_set_d(fepsb,0,GMP_RNDN);
    if (!mpfr_number_p(fepsb)) sgnfepsb = 3;
    else {
      sgnfepsb = mpfr_sgn(fepsb);
      if (sgnfepsb!=0) sgnfepsb = (sgnfepsb>0) ? 1 : 2;
    }

    if (sgnfa>0) { codefa = 1; codeNegfa = 2; }
    else { codefa = 2; codeNegfa = 1; }
    
    if ( ((sgnfepsa==0) && (sgnf0==0) && (sgnfepsb==0)) ||
	 ((sgnfepsa==1) && (sgnf0==0) && (sgnfepsb==0)) ||
	 ((sgnfepsa==2) && (sgnf0==0) && (sgnfepsb==0)) ||
	 ((sgnfepsa==3) && (sgnf0==0) && (sgnfepsb==0)) ||
	 ((sgnfepsa==0) && (sgnf0==0) && (sgnfepsb==1)) ||
	 ((sgnfepsa==1) && (sgnf0==0) && (sgnfepsb==1)) ||
	 ((sgnfepsa==2) && (sgnf0==0) && (sgnfepsb==1)) ||
	 ((sgnfepsa==3) && (sgnf0==0) && (sgnfepsb==1)) ||
	 ((sgnfepsa==0) && (sgnf0==0) && (sgnfepsb==2)) ||
	 ((sgnfepsa==1) && (sgnf0==0) && (sgnfepsb==2)) ||
	 ((sgnfepsa==2) && (sgnf0==0) && (sgnfepsb==2)) ||
	 ((sgnfepsa==3) && (sgnf0==0) && (sgnfepsb==2)) ||
	 ((sgnfepsa==0) && (sgnf0==0) && (sgnfepsb==3)) ||
	 ((sgnfepsa==1) && (sgnf0==0) && (sgnfepsb==3)) ||
	 ((sgnfepsa==2) && (sgnf0==0) && (sgnfepsb==3)) ||
	 ((sgnfepsa==3) && (sgnf0==0) && (sgnfepsb==3)) ) {
      printMessage(5, "Information (Newton's algorithm): 0 is an exact 0.\n");
      mpfr_set(res, zero_mpfr, GMP_RNDN);
      stop_algo = 1;
    }
    if ( ((sgnfepsa==0) && (sgnf0==1) && (sgnfepsb==0)) ||
	 ((sgnfepsa==0) && (sgnf0==2) && (sgnfepsb==0)) ||
	 ((sgnfepsa==0) && (sgnf0==3) && (sgnfepsb==0)) ||
	 ((sgnfepsa==0) && (sgnf0==1) && (sgnfepsb==1)) ||
	 ((sgnfepsa==0) && (sgnf0==2) && (sgnfepsb==1)) ||
	 ((sgnfepsa==0) && (sgnf0==3) && (sgnfepsb==1)) ||
	 ((sgnfepsa==0) && (sgnf0==1) && (sgnfepsb==2)) ||
	 ((sgnfepsa==0) && (sgnf0==2) && (sgnfepsb==2)) ||
	 ((sgnfepsa==0) && (sgnf0==3) && (sgnfepsb==2)) ||
	 ((sgnfepsa==0) && (sgnf0==1) && (sgnfepsb==3)) ||
	 ((sgnfepsa==0) && (sgnf0==2) && (sgnfepsb==3)) ||
	 ((sgnfepsa==0) && (sgnf0==3) && (sgnfepsb==3)) ) {
      printMessage(5, "Information (Newton's algorithm): an exact 0 has been discovered.\n");
      mpfr_set(res, epsa, GMP_RNDN);
      stop_algo = 1;
    }
    if ( ((sgnfepsa==1) && (sgnf0==1) && (sgnfepsb==0)) ||
	 ((sgnfepsa==2) && (sgnf0==1) && (sgnfepsb==0)) ||
	 ((sgnfepsa==3) && (sgnf0==1) && (sgnfepsb==0)) ||
	 ((sgnfepsa==1) && (sgnf0==2) && (sgnfepsb==0)) ||
	 ((sgnfepsa==2) && (sgnf0==2) && (sgnfepsb==0)) ||
	 ((sgnfepsa==3) && (sgnf0==2) && (sgnfepsb==0)) ||
	 ((sgnfepsa==1) && (sgnf0==3) && (sgnfepsb==0)) ||
	 ((sgnfepsa==2) && (sgnf0==3) && (sgnfepsb==0)) ||
	 ((sgnfepsa==3) && (sgnf0==3) && (sgnfepsb==0)) ) {
      printMessage(5, "Information (Newton's algorithm): an exact 0 has been discovered.\n");
      mpfr_set(res, epsb, GMP_RNDN);
      stop_algo = 1;
    }
    /* The cases (-sngfa, * *) and (* * sgnfa) can be separated into subcases : */
    if ((sgnfepsa==codeNegfa) && (sgnf0==3) && (sgnfepsb==codefa)) {
      printMessage(3, "Warning (Newton's algorithm): the function has more than one zero in the interval.\n");
      printMessage(3, "Warning (Newton's algorithm): 0 seems to be one of them but wa cannot prove it.\n");
      mpfr_set(res, zero_mpfr, GMP_RNDN);
      stop_algo = 1;
    }
    if ((sgnfepsa==codeNegfa) && (sgnf0==codefa) && (sgnfepsb==codefa)) {
      printMessage(3, "Warning (Newton's algorithm): the function has more than one zero in the interval.\n");
      printMessage(3, "Warning (Newton's algorithm): one of them is too close to zero for being accurately determined.\n");
      mpfr_set(res, epsa, GMP_RNDU);
      stop_algo = 1;
    }
    if ((sgnfepsa==codeNegfa) && (sgnf0==codeNegfa) && (sgnfepsb==codefa)) {
      printMessage(3, "Warning (Newton's algorithm): the function has more than one zero in the interval.\n");
      printMessage(3, "Warning (Newton's algorithm): one of them is too close to zero for being accurately determined.\n");
      mpfr_set(res, epsb, GMP_RNDD);
      stop_algo = 1;
    }
    if ( ((sgnfepsa==codeNegfa) && (sgnf0==1) && (sgnfepsb==codeNegfa)) ||
	 ((sgnfepsa==codeNegfa) && (sgnf0==2) && (sgnfepsb==codeNegfa)) ||
	 ((sgnfepsa==codeNegfa) && (sgnf0==3) && (sgnfepsb==codeNegfa)) ||
	 ((sgnfepsa==codeNegfa) && (sgnf0==1) && (sgnfepsb==3)) ||
	 ((sgnfepsa==codeNegfa) && (sgnf0==2) && (sgnfepsb==3)) ||
	 ((sgnfepsa==codeNegfa) && (sgnf0==3) && (sgnfepsb==3)) ) {
      mpfr_set(u, a, GMP_RNDD);
      mpfr_set(v, epsa, GMP_RNDU);
    }
    if ( ((sgnfepsa==codefa) && (sgnf0==1) && (sgnfepsb==codefa)) ||
	 ((sgnfepsa==codefa) && (sgnf0==2) && (sgnfepsb==codefa)) ||
	 ((sgnfepsa==codefa) && (sgnf0==3) && (sgnfepsb==codefa)) ||
	 ((sgnfepsa==3) && (sgnf0==1) && (sgnfepsb==codefa)) ||
	 ((sgnfepsa==3) && (sgnf0==2) && (sgnfepsb==codefa)) ||
	 ((sgnfepsa==3) && (sgnf0==3) && (sgnfepsb==codefa)) ) {
      mpfr_set(u, epsb, GMP_RNDD);
      mpfr_set(v, b, GMP_RNDU);
    }
    /* End of the subcases */
    if ( ((sgnfepsa==codefa) && (sgnf0==codefa) && (sgnfepsb==codeNegfa)) ||
	 ((sgnfepsa==3) && (sgnf0==codefa) && (sgnfepsb==codeNegfa)) ) {
      printMessage(2, "Warning (Newton's algorithm): the zero of f is too close to zero for being accurately determined.\n");
      mpfr_set(res, epsb, GMP_RNDN);
      stop_algo = 1;
    }
    if ( ((sgnfepsa==codefa) && (sgnf0==codeNegfa) && (sgnfepsb==codeNegfa)) ||
	 ((sgnfepsa==codefa) && (sgnf0==codeNegfa) && (sgnfepsb==3)) ) {
      printMessage(2, "Warning (Newton's algorithm): the zero of f is too close to zero for being accurately determined.\n");
      mpfr_set(res, epsa, GMP_RNDN);
      stop_algo = 1;
    }
    if ((sgnfepsa==codefa) && (sgnf0==3) && (sgnfepsb==codeNegfa)) {
      printMessage(2, "Warning (Newton's algorithm): 0 seems to be an exact zero but we cannot prove it.\n");
      mpfr_set(res, zero_mpfr, GMP_RNDN);
      stop_algo = 1;
    }
    if ((sgnfepsa==codefa) && (sgnf0==3) && (sgnfepsb==3)) {
      if (mpfr_cmp_ui(b,0)==0) {
	printMessage(2, "Warning (Newton's algorithm): 0 seems to be an exact zero but we cannot prove it.\n");
	mpfr_set(res, zero_mpfr, GMP_RNDN);
	stop_algo = 1;
      }
      else {
	sollyaFprintf(stderr, "Error (Newton's algorithm): numerical problems have been encountered. Failed.\n");
	mpfr_set_nan(res);
	stop_algo = 1;
      }
    }
    if ((sgnfepsa==3) && (sgnf0==3) && (sgnfepsb==codeNegfa)) {
      if (mpfr_cmp_ui(a,0)==0) {
	printMessage(2, "Warning (Newton's algorithm): 0 seems to be an exact zero but we cannot prove it.\n");
	mpfr_set(res, zero_mpfr, GMP_RNDN);
	stop_algo = 1;
      }
      else {
	sollyaFprintf(stderr, "Error (Newton's algorithm): numerical problems have been encountered. Failed.\n");
	mpfr_set_nan(res);
	stop_algo = 1;
      }
    }
    if ((sgnfepsa==3) && (sgnf0==3) && (sgnfepsb==3)) {
      if ( (mpfr_cmp_ui(a,0)==0)&&(mpfr_cmp_ui(b,0)==0) ) {
	printMessage(2, "Warning (Newton's algorithm): 0 seems to be an exact zero but we cannot prove it.\n");
	mpfr_set(res, zero_mpfr, GMP_RNDN);
	stop_algo = 1;
      }
      else {
	sollyaFprintf(stderr, "Error (Newton's algorithm): numerical problems have been encountered. Failed.\n");
	mpfr_set_nan(res);
	stop_algo = 1;
      }
    }
    if ( ((sgnfepsa==codefa) && (sgnf0==codefa) && (sgnfepsb==3)) ||
	 ((sgnfepsa==3) && (sgnf0==codefa) && (sgnfepsb==3)) ) { 
      sollyaFprintf(stderr, "Error (Newton's algorithm): failed to locate the zero\n");
      mpfr_set_nan(res);
      stop_algo = 1;
    }
    if ( ((sgnfepsa==3) && (sgnf0==codeNegfa) && (sgnfepsb==codeNegfa)) ||
	 ((sgnfepsa==3) && (sgnf0==codeNegfa) && (sgnfepsb==3)) ) { 
      sollyaFprintf(stderr, "Error (Newton's algorithm): failed to locate the zero\n");
      mpfr_set_nan(res);
      stop_algo = 1;
    }

    mpfr_clear(epsa);
    mpfr_clear(epsb);
    mpfr_clear(fepsa);
    mpfr_clear(fepsb);
    mpfr_clear(f0);
  }
  /*************************************************************************/


  
  /**************    STEP 2 : iterating Newton's algorithm    **************/
  if (!stop_algo) {
    mpfr_init2(x, prec);
    mpfr_init2(xNew, prec);
    mpfr_init2(yNew, prec);
    mpfr_init2(tmp_mpfr, 53);

    if(x0!=NULL) mpfr_set(x,*x0,GMP_RNDN);
    else { mpfr_add(x, u, v, GMP_RNDU); mpfr_div_2ui(x, x, 1, GMP_RNDN); }

    iterator = safeMalloc(sizeof(node));
    iterator->nodeType = SUB;
    temp = safeMalloc(sizeof(node));
    temp->nodeType = VARIABLE;
    iterator->child1 = temp;
    
    temp = safeMalloc(sizeof(node));
    temp->nodeType = DIV;
    temp->child1 = copyTree(f);
    temp->child2 = copyTree(f_diff);
    iterator->child2 = temp;

    temp = simplifyTreeErrorfree(iterator);
    free_memory(iterator);
    iterator = temp;

    /* Main loop */
    nbr_iter = 0;
    while(!stop_algo) {
      r = evaluateFaithfulWithCutOffFast(xNew, iterator, NULL, x, zero_mpfr, prec);
      if(r==0) mpfr_set_d(xNew,0,GMP_RNDN);
    
      if( (mpfr_cmp(u,xNew)>0) || (mpfr_cmp(xNew,v)>0) || ((!mpfr_number_p(xNew)) && (r==1)) ) {
	printMessage(5, "Information (Newton's algorithm): performing a bisection step\n");
	mpfr_add(xNew,u,v,GMP_RNDN);
	mpfr_div_2ui(xNew, xNew, 1, GMP_RNDN);
	if (mpfr_cmp(x, xNew)==0) {
	  printMessage(5, "Warning (Newton's algorithm): performing a trisection step.\n");
	  mpfr_sub(xNew,v,u,GMP_RNDN);
	  mpfr_div_ui(xNew, xNew, 3, GMP_RNDN);
	  mpfr_add(xNew, u, xNew, GMP_RNDN);
	}
      }
      
      r = evaluateFaithfulWithCutOffFast(yNew, f, f_diff, xNew, zero_mpfr, prec); /* yNew=f[xNew] */
      if(r==0) mpfr_set_d(yNew, 0, GMP_RNDN);
      
      if (mpfr_number_p(yNew)) {
	if (mpfr_cmp_ui(yNew, 0)==0) {
	  printMessage(5, "Information (Newton's algorithm): an exact 0 has been discovered.\n");
	  mpfr_set(res, xNew, GMP_RNDN);
	  stop_algo = 1;
	}
	else {
	  if (mpfr_cmp_ui(yNew, 0)*sgnfa>0) mpfr_set(u, xNew, GMP_RNDN);
	  else mpfr_set(v, xNew, GMP_RNDN);
	}
      }
      
      
      mpfr_sub(tmp_mpfr, xNew, x, GMP_RNDU);
      if (!mpfr_zero_p(tmp_mpfr)) estim_prec=(mpfr_get_exp(xNew)-mpfr_get_exp(tmp_mpfr));
      
      mpfr_sub(tmp_mpfr,v,u,GMP_RNDN);
      if(mpfr_cmp_abs(v,u)>0) estim_prec2 = mpfr_get_exp(u) - mpfr_get_exp(tmp_mpfr);
      else estim_prec2 = mpfr_get_exp(v) - mpfr_get_exp(tmp_mpfr);
      
      if(estim_prec2 > estim_prec) estim_prec = estim_prec2;
      
      nbr_iter++;
      if ( ((n!=0) && (nbr_iter==n)) || mpfr_equal_p(x,xNew) || (estim_prec>(int)prec)) {
	mpfr_set(res, xNew, GMP_RNDN);
	stop_algo=1;
      }
      
      mpfr_set(x,xNew,GMP_RNDN);
    }
 
    free_memory(iterator);
    mpfr_clear(x);
    mpfr_clear(xNew);
    mpfr_clear(yNew);
    mpfr_clear(tmp_mpfr);
  }

  /*************************************************************************/


  printMessage(7, "Information (Newton's algorithm): finished after %d steps.\n", nbr_iter);
  if(verbosity>=8) {
    changeToWarningMode();
    sollyaPrintf("Information (Newton's algorithm): x = "); printMpfr(res);
    restoreMode();
  }
  
  mpfr_clear(zero_mpfr);
  mpfr_clear(u);
  mpfr_clear(v);

  return;
}

// Just a wrapper
void newton(mpfr_t res, node *f, node *f_diff, mpfr_t a, mpfr_t b, int sgnfa, mp_prec_t prec) {
  findZero(res, f, f_diff, a, b, sgnfa, NULL, 0, prec);
  return;
}

// Yet another wrapper compatible with Christoph's old newtonMPFR routine
int newtonFaithful(mpfr_t res, node *f, node *f_diff, mpfr_t a, mpfr_t b, mp_prec_t prec) {
  mpfr_t yA;
  mpfr_init2(yA,prec);
  evaluateFaithful(yA,f,a,prec);
  findZero(res, f, f_diff, a, b, mpfr_sgn(yA), NULL, 0, prec);
  mpfr_clear(yA);
  return 1;
}

// Finds the zeros of a function on an interval.
chain *uncertifiedFindZeros(node *tree, mpfr_t a, mpfr_t b, unsigned long int points, mp_prec_t prec) {
  mpfr_t zero_mpfr, h, x1, x2, y1, y2;
  mpfr_t *temp;
  node *diff_tree;
  chain *result=NULL;

  mpfr_init2(h,prec);
  mpfr_init2(y1,prec);
  mpfr_init2(y2,prec);
  mpfr_init2(x1,prec);
  mpfr_init2(x2,prec);
  mpfr_init2(zero_mpfr,prec);

  diff_tree = differentiate(tree);

  mpfr_set_d(zero_mpfr, 0., GMP_RNDN);

  mpfr_sub(h,b,a,GMP_RNDD);
  mpfr_div_si(h,h,points,GMP_RNDD);

  mpfr_set(x1,b,GMP_RNDN);
  mpfr_sub(x2,b,h,GMP_RNDD);

  evaluateFaithfulWithCutOffFast(y1, tree, diff_tree, x1, zero_mpfr, prec);
  evaluateFaithfulWithCutOffFast(y2, tree, diff_tree, x2, zero_mpfr, prec);
  /* Little trick: if a=b, h=0, thus x1=x2=a=b */
  /* By doing this, we avoid entering the loop */
  if(mpfr_equal_p(a,b)) { mpfr_nextbelow(x2); }

  while(mpfr_greaterequal_p(x2,a)) {
    if((mpfr_sgn(y1)==0) || (mpfr_sgn(y2)==0) || (mpfr_sgn(y1) != mpfr_sgn(y2))) {
      if (mpfr_sgn(y1)==0) {
	  temp = safeMalloc(sizeof(mpfr_t));
	  mpfr_init2(*temp, prec);
	  mpfr_set(*temp, x1, GMP_RNDN);
	  result = addElement(result, temp);
      }
      else {
	if (mpfr_sgn(y2)!=0) {
	  temp = safeMalloc(sizeof(mpfr_t));
	  mpfr_init2(*temp, prec);
	  newton(*temp, tree, diff_tree, x2, x1, mpfr_sgn(y2), prec);
	  result = addElement(result, temp);
	}
      }
    }
    mpfr_set(x1,x2,GMP_RNDN);
    mpfr_sub(x2,x2,h,GMP_RNDD);
    mpfr_set(y1,y2,GMP_RNDN);
    evaluateFaithfulWithCutOffFast(y2, tree, diff_tree, x2, zero_mpfr, prec);
  }

  if(! mpfr_equal_p(x1,a)) {
    mpfr_set(x2,a,GMP_RNDU);
    evaluateFaithfulWithCutOffFast(y2, tree, diff_tree, x2, zero_mpfr, prec);
    if (mpfr_sgn(y1)==0) {
      temp = safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*temp, prec);
      mpfr_set(*temp, x1, GMP_RNDN);
      result = addElement(result, temp);
    }
    if (mpfr_sgn(y2)==0) {
      temp = safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*temp, prec);
      mpfr_set(*temp, x2, GMP_RNDN);
      result = addElement(result, temp);
    }
    if( (mpfr_sgn(y1)!=0) && (mpfr_sgn(y2)!=0) && (mpfr_sgn(y1) != mpfr_sgn(y2)) ) {
      temp = safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*temp, prec);
      newton(*temp, tree, diff_tree, x2, x1, mpfr_sgn(y2), prec);
      result = addElement(result, temp);
    }
  }
  else { // x1=a
    if (mpfr_sgn(y1)==0) {
      temp = safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*temp, prec);
      mpfr_set(*temp, x1, GMP_RNDN);
      result = addElement(result, temp);
    }
  }
  mpfr_clear(h);
  mpfr_clear(y1);
  mpfr_clear(y2);
  mpfr_clear(x1);
  mpfr_clear(x2);
  mpfr_clear(zero_mpfr);
  
  free_memory(diff_tree);

  return result;
}

// Perform a step of exchange algorithm
// newx is the point to be inserted
// err_newx is the corresponding error
// lambdai_vect is the vector of lambda_i corresponding to vector x
// epsilon is the current radius of reference
//   note that (p*w-f)(x_{n+1})= epsilon by definition
//   and lambda_{n+1} = -1 by definition
//   Thus the rule is
//            * take the max of mu/lambda if sgn(err_newx)*sgn(epsilon)=-1
//            * take the min otherwise
// n is the number of elements in x
void single_step_remez(mpfr_t newx, mpfr_t err_newx, mpfr_t *x,
		       node **monomials_tree,
		       node *w,
		       mpfr_t *lambdai_vect,
		       mpfr_t epsilon,
		       int n, mp_prec_t prec) {
  int freeDegrees = n-1;
  int test,i,j,r, argmaxi, argmini;
  mpfr_t *N;
  mpfr_t *c;
  mpfr_t *mui_vect;
  node *temp_tree;
  node *temp_tree2;
  mpfr_t zero_mpfr, var1, var2, var3;
  mpfr_t maxi;
  mpfr_t mini;
  
  // Initialisations and precomputations
  mpfr_init2(var1, prec);
  mpfr_init2(var2, prec);
  mpfr_init2(var3, prec);

  mpfr_init2(zero_mpfr, 53);
  mpfr_set_d(zero_mpfr, 0., GMP_RNDN);

  mpfr_init2(maxi, prec);
  mpfr_init2(mini, prec);
  
  N = safeMalloc(freeDegrees*freeDegrees*sizeof(mpfr_t));
  c = safeMalloc(freeDegrees*sizeof(mpfr_t));
  mui_vect = safeMalloc(freeDegrees*sizeof(mpfr_t));
  
  // Initialization of mui_vect
  for(j=1; j <= freeDegrees ; j++) {
    for(i=1; i<= freeDegrees; i++) {
      mpfr_init2(N[coeff(i,j,freeDegrees)],prec);
    }
    mpfr_init2(c[j-1], prec);
    mpfr_init2(mui_vect[j-1], prec);
  }

  // Computation of the matrix
  for (i=1 ; i <= freeDegrees ; i++) {
    r = evaluateFaithfulWithCutOffFast(var1, w, NULL, x[i-1], zero_mpfr, prec);
    if((r==1) && (mpfr_number_p(var1))) test=1;
    else test=0;
    
    for (j=1 ; j <= freeDegrees ; j++) {
      if(test==1) {
	r = evaluateFaithfulWithCutOffFast(var2, monomials_tree[j-1], NULL, x[i-1], zero_mpfr, prec);
	if((r==1) && (mpfr_number_p(var2))) {
	  mpfr_mul(var2, var1, var2, GMP_RNDN);
	  mpfr_set(N[coeff(j,i,freeDegrees)],var2,GMP_RNDN);
	}
      }
      if((test==0) || (r==0) || (!mpfr_number_p(var2))) {
	temp_tree = safeMalloc(sizeof(node));
	temp_tree->nodeType = MUL;
	temp_tree->child1 = copyTree(monomials_tree[j-1]);
	temp_tree->child2 = copyTree(w);
	
	temp_tree2 = simplifyTreeErrorfree(temp_tree);
	free_memory(temp_tree);
	temp_tree = temp_tree2; // temp_tree = x^(monomials[j])*w(x)
	
	r = evaluateFaithfulWithCutOffFast(var3, temp_tree, NULL, x[i-1], zero_mpfr, prec);
	
	if(r==0) mpfr_set_d(var3, 0., GMP_RNDN);
	mpfr_set(N[coeff(j,i,freeDegrees)],var3,GMP_RNDN);
	free_memory(temp_tree);
      }
    }
  }

  // Computation of the vector corresponding to the new point
  r = evaluateFaithfulWithCutOffFast(var1, w, NULL, newx, zero_mpfr, prec);
  if((r==1) && (mpfr_number_p(var1))) test=1;
  else test=0;
  
  for (j=1 ; j <= freeDegrees ; j++) {
    if(test==1) {
      r = evaluateFaithfulWithCutOffFast(var2, monomials_tree[j-1], NULL, newx, zero_mpfr, prec);
      if((r==1) && (mpfr_number_p(var2))) {
	mpfr_mul(var2, var1, var2, GMP_RNDN);
	mpfr_set(c[j-1],var2,GMP_RNDN);
      }
    }
    if((test==0) || (r==0) || (!mpfr_number_p(var2))) {
      temp_tree = safeMalloc(sizeof(node));
      temp_tree->nodeType = MUL;
      temp_tree->child1 = copyTree(monomials_tree[j-1]);
      temp_tree->child2 = copyTree(w);
      
      temp_tree2 = simplifyTreeErrorfree(temp_tree);
      free_memory(temp_tree);
      temp_tree = temp_tree2; // temp_tree = x^(monomials[j])*w(x)
      
      r = evaluateFaithfulWithCutOffFast(var3, temp_tree, NULL, newx, zero_mpfr, prec);
      
      if(r==0) mpfr_set_d(var3, 0., GMP_RNDN);
      mpfr_set(c[j-1], var3, GMP_RNDN);
      free_memory(temp_tree);
    }
  }



  // Resolution of the system
  system_solve(mui_vect , N, c, freeDegrees, prec);

  // Finding the maximum and minimum
  mpfr_set(maxi, zero_mpfr, GMP_RNDN);
  argmaxi=freeDegrees;
  mpfr_set(mini, zero_mpfr, GMP_RNDN);
  argmini=freeDegrees;

  for(i=freeDegrees-1;i>=0;i--) {
    mpfr_div(var1, mui_vect[i], lambdai_vect[i], GMP_RNDN);
    if (mpfr_cmp(var1, maxi)>0) {
      mpfr_set(maxi, var1, GMP_RNDN);
      argmaxi=i;
    }
    if (mpfr_cmp(var1, mini)<0) {
      mpfr_set(mini, var1, GMP_RNDN);
      argmini=i;
    }
  }


  // Introduce newx
  if(mpfr_sgn(err_newx)*mpfr_sgn(epsilon)==1) {
    if(verbosity>=3) {
      changeToWarningMode();
      sollyaPrintf("Remez: exchange algorithm takes the minimum (");
      myPrintValue(&mini, 53);
      sollyaPrintf(") at place %d\n",argmini);
      restoreMode();
    }
    mpfr_set(x[argmini], newx, GMP_RNDN);
  }
  else {
    if(verbosity>=3) {
      changeToWarningMode();
      sollyaPrintf("Remez: exchange algorithm takes the maximum (");
      myPrintValue(&maxi, 53);
      sollyaPrintf(") at place %d\n",argmaxi);
      restoreMode();
    }
    mpfr_set(x[argmaxi], newx, GMP_RNDN);
  }

  mpfr_sort(x, freeDegrees+1, prec);

  
  // Freeing the memory
  
  for(j=1; j <= freeDegrees ; j++) {
    for(i=1; i<= freeDegrees; i++) {
      mpfr_clear(N[coeff(i,j,freeDegrees)]);
    }
    mpfr_clear(c[j-1]);
    mpfr_clear(mui_vect[j-1]);
  }
  free(N);
  free(c);
  free(mui_vect);

  mpfr_clear(zero_mpfr);
  mpfr_clear(var1);
  mpfr_clear(var2);
  mpfr_clear(var3);
  mpfr_clear(maxi);
  mpfr_clear(mini);

  return;
}


// Returns a PARI array containing the zeros of tree on [a;b]
// deg+1 indicates the number of zeros which we are expecting.
// error' = tree  and tree' = diff_tree
void quickFindZeros(mpfr_t *res, mpfr_t *curr_points,
		    node *error, node *tree, node *diff_tree,
		    node **monomials_tree, node *w, mpfr_t *lambdai_vect, mpfr_t epsilon, int HaarCompliant,
		    int deg,
		    mpfr_t a, mpfr_t b, mp_prec_t prec) {
  long int n = 50*(deg+2);
  long int i=0;

  /* The variable test is used to check that the maximum (in absolute value)
     of the error will be inserted in the new list of points.
     If it is not the case, a step of the exchange algorithm is performed */
  int test=0;
  mpfr_t h, x1, x2, x, y1, y2, zero_mpfr, maxi, argmaxi, z, alpha1, alpha2, alpha;

  mpfr_init2(h,prec);
  mpfr_init2(y1,prec);
  mpfr_init2(y2,prec);
  mpfr_init2(x1,prec);
  mpfr_init2(x2,prec);
  mpfr_init2(x, prec);
  mpfr_init2(zero_mpfr,prec);
  mpfr_init2(z, prec);
  mpfr_init2(maxi, prec);
  mpfr_init2(argmaxi, prec);
  mpfr_init2(alpha1, 24);
  mpfr_init2(alpha2, 24);
  mpfr_init2(alpha, 24);

  mpfr_set_d(zero_mpfr, 0., GMP_RNDN);

  evaluateFaithfulWithCutOffFast(z, error, tree, a, zero_mpfr, prec);
  mpfr_set(maxi,z,GMP_RNDN);
  mpfr_set(argmaxi,a,GMP_RNDN);
  evaluateFaithfulWithCutOffFast(z, error, tree, b, zero_mpfr, prec);
  if (mpfr_cmpabs(z,maxi)>0) {
    mpfr_set(maxi,z,GMP_RNDN);
    mpfr_set(argmaxi,b,GMP_RNDN);
  }

  mpfr_sub(h,b,a,GMP_RNDD);
  mpfr_div_si(h,h,n,GMP_RNDD);

  mpfr_set(x1,a,GMP_RNDN);
  mpfr_add(x2,a,h,GMP_RNDN);

  evaluateFaithfulWithCutOffFast(y1, tree, diff_tree, x1, zero_mpfr, prec);
  evaluateFaithfulWithCutOffFast(y2, tree, diff_tree, x2, zero_mpfr, prec);
  evaluateFaithfulWithCutOffFast(alpha1, diff_tree, NULL, x1, zero_mpfr, prec);
  evaluateFaithfulWithCutOffFast(alpha2, diff_tree, NULL, x2, zero_mpfr, prec);

  while(mpfr_lessequal_p(x2,b)) {
    if( (mpfr_sgn(y1)==0) || (mpfr_sgn(y2)==0) || (mpfr_sgn(y1) != mpfr_sgn(y2))) {
      if (mpfr_sgn(y1)==0) {
	evaluateFaithfulWithCutOffFast(z, error, tree, x1, zero_mpfr, prec);
	if (mpfr_sgn(z)*mpfr_sgn(alpha1)<0) {
	  i++;
	  if(i>deg+2)
	    printMessage(1,"Warning: the function oscillates too much. Nevertheless, we try to continue.\n");
	  else mpfr_set(res[i-1], x1, GMP_RNDN);
	}
	if (mpfr_cmpabs(z,maxi)>0) {
	  if ( (i>deg+2)||(mpfr_sgn(z)*mpfr_sgn(alpha1)>=0) ) test=0;
	  else test=1;
	  mpfr_set(maxi,z,GMP_RNDN);
	  mpfr_set(argmaxi,x1,GMP_RNDN);
	}
	
	/* if(mpfr_sgn(y2)==0) {
	     evaluateFaithfulWithCutOffFast(z, error, tree, x2, zero_mpfr, prec);
	     if (mpfr_sgn(z)*mpfr_sgn(alpha2)<0) {
	       i++;
	       if(i>deg+2)
	         printMessage(1,"Warning: the function oscillates too much. Nevertheless, we try to continue.\n");
	       else mpfr_set(res[i-1], x2, GMP_RNDN);
	     }
	     if (mpfr_cmpabs(z,maxi)>0) {
	       if ( (i>deg+2)||(mpfr_sgn(z)*mpfr_sgn(alpha2)>=0) ) test=0;
	       else test=1;
	       mpfr_set(maxi,z,GMP_RNDN);
	       mpfr_set(argmaxi,x2,GMP_RNDN);
	     }
	   }
         */
      }
      else {
	if (mpfr_sgn(y2)==0) {
	  evaluateFaithfulWithCutOffFast(z, error, tree, x2, zero_mpfr, prec);
	  if (mpfr_sgn(z)*mpfr_sgn(alpha2)<0) {
	    i++;
	    if(i>deg+2)
	      printMessage(1,"Warning: the function oscillates too much. Nevertheless, we try to continue.\n");
	    else mpfr_set(res[i-1], x2, GMP_RNDN);
	  }
	  if (mpfr_cmpabs(z,maxi)>0) {
	    if ( (i>deg+2) || (mpfr_sgn(z)*mpfr_sgn(alpha2)>=0) ) test=0;
	    else test=1;	
	    mpfr_set(maxi,z,GMP_RNDN);
	    mpfr_set(argmaxi,x2,GMP_RNDN);
	  }
	}
	else {
	  newton(x, tree, diff_tree, x1, x2, mpfr_sgn(y1), prec);
	  evaluateFaithfulWithCutOffFast(z, error, tree, x , zero_mpfr, prec);
	  evaluateFaithfulWithCutOffFast(alpha, diff_tree, NULL, x , zero_mpfr, prec);
	  if (mpfr_sgn(z)*mpfr_sgn(alpha)<0) {
	    i++;
	    if(i>deg+2)
	      printMessage(1,"Warning: the function oscillates too much. Nevertheless, we try to continue.\n");
	    else mpfr_set(res[i-1], x, GMP_RNDN);
	  }
	  if (mpfr_cmpabs(z,maxi)>0) {
	    if ( (i>deg+2) || (mpfr_sgn(z)*mpfr_sgn(alpha)>=0) ) test=0;
	    else test=1;
	    mpfr_set(maxi,z,GMP_RNDN);
	    mpfr_set(argmaxi,x,GMP_RNDN);
	  }
	}
      }
    }
    
    mpfr_set(x1,x2,GMP_RNDN);
    mpfr_add(x2,x2,h,GMP_RNDN);
    mpfr_set(y1,y2,GMP_RNDN);
    evaluateFaithfulWithCutOffFast(y2, tree, diff_tree, x2, zero_mpfr, prec);
    evaluateFaithfulWithCutOffFast(alpha2, diff_tree, NULL, x2, zero_mpfr, prec);
  }
  
  if ((i<deg)||(i>deg+2)||(!HaarCompliant)||(!test)) {
    /* printMessage(1,"Warning: the function fails to oscillate enough.\n");
       printMessage(1,"Check Haar condition and/or increase precision.\n");
       *crash_report = -1; */
    test=0;
    printMessage(2, "Performing an exchange step...\n");
    if (verbosity>=4) {
      changeToWarningMode();
      sollyaPrintf("Computed infinity norm : "); printMpfr(maxi);
      sollyaPrintf("Reached at point "); printMpfr(argmaxi);
      restoreMode();
    }
    for(i=0;i<deg+2;i++) mpfr_set(res[i], curr_points[i], GMP_RNDN);
    single_step_remez(argmaxi, maxi, res, monomials_tree, w, lambdai_vect, epsilon, deg+2, prec);
  }
  else {
    // in this branch test=1
    if (i==deg) { 
      mpfr_set(res[deg], a, GMP_RNDN);
      mpfr_set(res[deg+1], b, GMP_RNDN);
      mpfr_sort(res, deg+2, prec);
    }
    else {
      if (i==deg+1) {
	evaluateFaithfulWithCutOffFast(y1, error, tree, a, zero_mpfr, prec);
	evaluateFaithfulWithCutOffFast(y2, error, tree, res[0], zero_mpfr, prec);
	if (mpfr_sgn(y1)==mpfr_sgn(y2))  mpfr_set(res[deg+1], b, GMP_RNDN);
	else {
	  evaluateFaithfulWithCutOffFast(y1,  error, tree, b, zero_mpfr, prec);
	  evaluateFaithfulWithCutOffFast(y2,  error, tree, res[deg], zero_mpfr, prec);
	  if (mpfr_sgn(y1)==mpfr_sgn(y2))  mpfr_set(res[deg+1], a, GMP_RNDN);
	  else {
	    evaluateFaithfulWithCutOffFast(y1,  error, tree, a, zero_mpfr, prec);
	    evaluateFaithfulWithCutOffFast(y2,  error, tree, b, zero_mpfr, prec);
	    if (mpfr_cmpabs(y1,y2)>0) mpfr_set(res[deg+1], a, GMP_RNDN);
	    else mpfr_set(res[deg+1], b, GMP_RNDN);
	  }
	}
	mpfr_sort(res, deg+2, prec);
      }
    }
  }

  if(test) {
    /* since we did not perform an exchange step, 
       we have to check that the pseudo-alternating condition 
       is fulfilled */

    test=1;
    for(i=0; (i<deg+2)&&test ;i++) {
      evaluateFaithfulWithCutOffFast(z, error, tree, res[i] , zero_mpfr, prec);
      if ( mpfr_sgn(z)*mpfr_sgn(lambdai_vect[i])*mpfr_sgn(epsilon) >= 0 ) test=0;
    }
    if(!test) {
      printMessage(2, "Failed to find pseudo-alternating points. Performing an exchange step...\n");
      if (verbosity>=4) {
	changeToWarningMode();
	sollyaPrintf("Computed infinity norm : "); printMpfr(maxi);
	sollyaPrintf("Reached at point "); printMpfr(argmaxi);
	restoreMode();
      }
      for(i=0;i<deg+2;i++) mpfr_set(res[i], curr_points[i], GMP_RNDN);
      single_step_remez(argmaxi, maxi, res, monomials_tree, w, lambdai_vect, epsilon, deg+2, prec);
    }
  }

  mpfr_clear(h); mpfr_clear(x1); mpfr_clear(x2); mpfr_clear(x); mpfr_clear(y1); mpfr_clear(y2); mpfr_clear(zero_mpfr); mpfr_clear(z); mpfr_clear(maxi); mpfr_clear(argmaxi); mpfr_clear(alpha1); mpfr_clear(alpha2); mpfr_clear(alpha);
  return;
}




// This function finds the local extrema of a the error function poly*w-f.
// It uses the derivative and the second derivative of these functions to
// search the zeros of (poly*w-f)' by Newton's algorithm
// It expects to find at least freeDegrees+1 alternates extrema and
// returns it as a result. An intial estimation of these points is given
// in the vector x.
// Moreover, the quality of the approximation
// (defined by abs(err_max)/abs(err_min) - 1 where err_min and err_max 
// denote the minimal and maximal extrema in absolute value) is stored
// in computedQuality and the infinity norm is stored in infinityNorm
// if these parameters are non NULL.
int qualityOfError(mpfr_t computedQuality, mpfr_t infinityNorm, mpfr_t *x,
		   node *poly, node *f, node *w,
		   node **monomials_tree, mpfr_t *lambdai_vect, mpfr_t epsilon, int HaarCompliant,
		   int freeDegrees, mpfr_t a, mpfr_t b, mp_prec_t prec) {
  node *error;
  node *temp1;
  node *error_diff;
  node *error_diff2;
  int test, i, r;
  int case1, case2, case2b, case3;
  int *s;
  mpfr_t *y;
  mpfr_t var_mpfr, dummy_mpfr, dummy_mpfr2, max_val, min_val, zero_mpfr;
  mpfr_t *z;
  
  int crash_report = 0;
  int n = freeDegrees+1;
  
  mpfr_init2(var_mpfr, prec);
  mpfr_init2(zero_mpfr, 53);
  mpfr_init2(max_val, prec);
  mpfr_init2(min_val, prec);
  mpfr_init2(dummy_mpfr, 5);
  mpfr_init2(dummy_mpfr2, 53);

  mpfr_set_d(zero_mpfr, 0., GMP_RNDN);

  z = safeMalloc(n*sizeof(mpfr_t));
  for(i=1;i<=n;i++) mpfr_init2(z[i-1],prec);

  
  // Construction of the trees corresponding to (poly*w-f)' and (poly*w-f)''
  if(verbosity>=8) { 	changeToWarningMode(); sollyaPrintf("Constructing the error tree... \n"); restoreMode(); }
  error = safeMalloc(sizeof(node));
  error->nodeType = SUB;
  temp1 = safeMalloc(sizeof(node));
  temp1->nodeType = MUL;
  temp1->child1 = copyTree(poly);
  temp1->child2 = copyTree(w);
  error->child1 = temp1;
  error->child2 = copyTree(f);

  temp1 = simplifyTreeErrorfree(error);
  free_memory(error);
  error = temp1;

  if(verbosity>=8) { 	changeToWarningMode(); sollyaPrintf("Constructing the error' tree... \n"); restoreMode(); }
  error_diff = differentiate(error);
  temp1 = simplifyTreeErrorfree(error_diff);
  free_memory(error_diff);
  error_diff = temp1;

  if(verbosity>=8) { 	changeToWarningMode(); sollyaPrintf("Constructing the error'' trees... \n"); restoreMode(); }
  error_diff2 = differentiate(error_diff);
  temp1 = simplifyTreeErrorfree(error_diff2);
  free_memory(error_diff2);
  error_diff2 = temp1;
  
  if(verbosity>=6) { 	changeToWarningMode(); sollyaPrintf("Computing the yi... \n"); restoreMode(); }
  // If x = [x1 ... xn], we construct [y0 y1 ... yn] by
  // y0 = (a+x1)/2, yn = (xn+b)/2 and yi = (xi + x(i+1))/2
  y = (mpfr_t *)safeMalloc((n+1)*sizeof(mpfr_t));
  mpfr_init2(y[0], prec);
  mpfr_add(y[0], x[0], a, GMP_RNDN);
  mpfr_div_2ui(y[0], y[0], 1, GMP_RNDN);
  
  for(i=1; i<n; i++) {
    mpfr_init2(y[i], prec);
    mpfr_add(y[i], x[i-1], x[i], GMP_RNDN);
    mpfr_div_2ui(y[i], y[i], 1, GMP_RNDN);
  }

  mpfr_init2(y[n], prec);
  mpfr_add(y[n], x[n-1], b, GMP_RNDN);
  mpfr_div_2ui(y[n], y[n], 1, GMP_RNDN);

  if(verbosity>=6) {
    changeToWarningMode();
    sollyaPrintf("The computed yi are : ");
    for(i=0;i<=n;i++) {printMpfr(y[i]); sollyaPrintf(" ");}
    sollyaPrintf("\n");
    restoreMode();
  }
  

  // We call *case 1* the case where x1=a and xn=b
  // We call *case 2* the case where x1<>a and xn=b
  // and *case 2bis* the symetrical case
  // We call *case 3* the case where x1<>a and xn<>b


  if (mpfr_equal_p(y[0], a) &&  mpfr_equal_p(y[n],b)) {
    case1 = 1; case2 = 0; case2b = 0; case3 = 0;
  }
  else {
    if ((! mpfr_equal_p(y[0], a)) &&  mpfr_equal_p(y[n],b)) {
      case1 = 0; case2 = 1; case2b = 0; case3 = 0;
    }
    else {      
      if (mpfr_equal_p(y[0], a) &&  (! mpfr_equal_p(y[n],b))) {
	case1 = 0; case2 = 0; case2b = 1; case3 = 0;
      }
      else {
	case1 = 0; case2 = 0; case2b = 0; case3 = 1;
      }
    }
  }

  if(verbosity>=6) {
    changeToWarningMode();
    sollyaPrintf("We are in case : ");
    if(case1) sollyaPrintf("1\n");
    if(case2) sollyaPrintf("2\n");
    if(case2b) sollyaPrintf("2bis\n");
    if(case3) sollyaPrintf("3\n");
    restoreMode();
  }



  // If one of error_diff(y0) .... error_diff(yn) is a real NaN
  // (i.e. if evaluateFaithfulWithCutOffFast returns 1 and store a NaN)
  // this leads to numerical problems -> we use quikFindZeros

  // If sgn(error_diff(yi))*sgn(error_diff(y(i+1))) > 0 for some i=1..n-2
  // If we are in case 2 and sgn(error_diff(y0))*sgn(error_diff(y1)) > 0
  // If we are in case 2bis and sgn(error_diff(y(n-1)))*sgn(error_diff(yn)) > 0
  // If we are in case 3 and one of this two last condition is not fulfilled
  // this means the hypothesis (xi ~ zeros of error_diff) is false -> quickFindZeros

  // If we are in case 1 : if sgn(error_diff(y0))*sgn(error_diff(y1)) > 0 (the most probable)
  // we have a problem if error(y0) is a real NaN or if sgn(error(y0))*sgn(error_diff(y0))>0
  // if sgn(error_diff(y(n-1)))*sgn(error_diff(yn)) > 0 (the most probable)
  // we have a problem if error(yn) is a real NaN or if sgn(error(yn))*sgn(error_diff(yn))<0

  // If we are in case 2 if sgn(error_diff(y(n-1)))*sgn(error_diff(yn)) > 0 (the most probable)
  // we have a problem if error(yn) is a real NaN or if sgn(error(yn))*sgn(error_diff(yn))<0
  
  // If we are in case 2bis, if sgn(error_diff(y0))*sgn(error_diff(y1)) > 0 (the most probable)
  // we have a problem if error(y0) is a real NaN or if sgn(error(y0))*sgn(error_diff(y0))>0
  

  s = (int *)safeMalloc((n+1)*sizeof(int));
  test = 1;
  i = 0;
  while(test && (i<=n)) {
    r = evaluateFaithfulWithCutOffFast(dummy_mpfr, error_diff, error_diff2, y[i], zero_mpfr, prec);
    if((!mpfr_number_p(dummy_mpfr)) && (r==1)) test=0;
    else { 
      if(r==0) s[i]=0;
      else s[i] = mpfr_sgn(dummy_mpfr);
    }
    i++;
  }

  if(verbosity>=6) {
    changeToWarningMode();
    if(test) {
      sollyaPrintf("The computed signs are : ");
      for(i=0;i<=n;i++) sollyaPrintf("%d  ",s[i]);
      sollyaPrintf("\n");
    }
    else sollyaPrintf("Test is false because signs could not be evaluated\n");
    restoreMode();
  }
  

  if(test) {
    i = 1;
    while(test && (i<=n-2)) {
      if(s[i]*s[i+1] > 0) test=0;
      i++;
    }
  }
  if(test && case2 && (s[0]*s[1]>0)) test=0;
  if(test && case2b && (s[n-1]*s[n]>0)) test=0;
  if(test && case3 && ((s[0]*s[1]>0) || (s[n-1]*s[n]>0))) test=0;

  if(test && (case1 || case2b) && (s[0]*s[1]>0)) {
    r = evaluateFaithfulWithCutOffFast(dummy_mpfr, error, error_diff, y[0], zero_mpfr, prec);
    if((!mpfr_number_p(dummy_mpfr)) && (r==1)) test=0;
    else { 
      if((r!=0) && (mpfr_sgn(dummy_mpfr)*s[0] > 0)) test=0;
    }
  }

  if(test && (case1 || case2) && (s[n-1]*s[n]>0)) {
    r = evaluateFaithfulWithCutOffFast(dummy_mpfr, error, error_diff, y[n], zero_mpfr, prec);
    if((!mpfr_number_p(dummy_mpfr)) && (r==1)) test=0;
    else { 
      if((r!=0) && (mpfr_sgn(dummy_mpfr)*s[n] < 0)) test=0;
    }
  }

  if(test && HaarCompliant) {
    if((case1 || case2b) && (s[0]*s[1]<=0)) {
      if(s[0]==0) mpfr_set(z[0], a, GMP_RNDN);
      else {
	if(s[1]==0) mpfr_set(z[0], y[1], GMP_RNDN);
	else findZero(z[0], error_diff, error_diff2,y[0],y[1],s[0],NULL, NEWTON_STEPS,prec);
      }
    }
    if((case1 || case2b) && (s[0]*s[1]>0)) mpfr_set(z[0], a, GMP_RNDN);
    if(case2 || case3) findZero(z[0], error_diff, error_diff2, y[0], y[1], s[0], &x[0], NEWTON_STEPS, prec);
    
    for(i=1;i<=n-2;i++) findZero(z[i], error_diff, error_diff2, y[i], y[i+1], s[i], &x[i], NEWTON_STEPS, prec);

    if((case1 || case2) && (s[n-1]*s[n]<=0)) {
      if(s[n]==0) mpfr_set(z[n-1], b, GMP_RNDN);
      else {
	if(s[n-1]==0) mpfr_set(z[n-1], y[n-1], GMP_RNDN);
	else findZero(z[n-1], error_diff, error_diff2, y[n-1], y[n], s[n-1], NULL, NEWTON_STEPS, prec);
      }
    }

    if((case1 || case2) && (s[n-1]*s[n]>0)) mpfr_set(z[n-1],b,GMP_RNDN);
    if(case2b || case3) findZero(z[n-1], error_diff, error_diff2, y[n-1], y[n], s[n-1], &x[n-1], NEWTON_STEPS, prec);



    // We expect error(z[i]) ~ - sgn(lambda[i])*epsilon
    // We check the sign of error at these points and for z[i]<>a,b, we check the sign of error''
    test=1;
    for(i=1; (i<=n) && test; i++) {
      r = evaluateFaithfulWithCutOffFast(var_mpfr, error, error_diff, z[i-1], zero_mpfr, prec);
      if(r==0) mpfr_set_d(var_mpfr, 0., GMP_RNDN);

      if ( mpfr_sgn(var_mpfr)*mpfr_sgn(lambdai_vect[i-1])*mpfr_sgn(epsilon) >= 0 ) test=0;
    
      if ( (!mpfr_equal_p(z[i-1],a)) && (!mpfr_equal_p(z[i-1],b)) ) {
	r = evaluateFaithfulWithCutOffFast(var_mpfr, error_diff2, NULL, z[i-1], zero_mpfr, prec);
	if(r==0) mpfr_set_d(var_mpfr, 0., GMP_RNDN);
      
	if(-mpfr_sgn(var_mpfr)*mpfr_sgn(epsilon)*mpfr_sgn(lambdai_vect[i-1]) >= 0) test=0;
      }
    }
  
    if (!test) {
      printMessage(1,"Warning in Remez: main heuristic failed. A slower algorithm is used for this step.\n");
      quickFindZeros(z, x, error, error_diff, error_diff2, monomials_tree, w, lambdai_vect, epsilon, HaarCompliant, freeDegrees-1, a, b, prec);

      if(crash_report==-1) {
	free_memory(error);
	free_memory(error_diff);
	free_memory(error_diff2);
	mpfr_clear(var_mpfr);
	mpfr_clear(zero_mpfr);
	mpfr_clear(dummy_mpfr);
	mpfr_clear(dummy_mpfr2);
	mpfr_clear(max_val);
	mpfr_clear(min_val);
	free(s);
	  
	for(i=0;i<=n;i++)  mpfr_clear(y[i]);
	free(y);
	for(i=1;i<=n;i++) {
	  mpfr_clear(z[i-1]);
	}
	free(z);
	  
	return -1;
      }
    }
  }
  else {
    if(verbosity>=1) {
      changeToWarningMode();
      sollyaPrintf("Warning in Remez: a slower algorithm is used for this step");
      if(!HaarCompliant) sollyaPrintf(" (pseudo-alternation condition changed)");
      sollyaPrintf("\n");
      restoreMode();
    }

    quickFindZeros(z, x, error, error_diff, error_diff2, monomials_tree, w, lambdai_vect, epsilon, HaarCompliant, freeDegrees-1, a, b, prec);

    if(crash_report==-1) {
      free_memory(error);
      free_memory(error_diff);
      free_memory(error_diff2);
      mpfr_clear(var_mpfr);
      mpfr_clear(zero_mpfr);
      mpfr_clear(dummy_mpfr);
      mpfr_clear(dummy_mpfr2);
      mpfr_clear(max_val);
      mpfr_clear(min_val);
      free(s);

      for(i=0;i<=n;i++)  mpfr_clear(y[i]);
      free(y);
      for(i=1;i<=n;i++) {
	mpfr_clear(z[i-1]);
      }
      free(z);

      return -1;
    }
  }


  if(verbosity>=3) {
    changeToWarningMode();
    sollyaPrintf("The new points are : ");
    for(i=1; i<=n; i++) printMpfr(z[i-1]);
    restoreMode();
  }

  // Test the quality of the current error

  mpfr_set_d(max_val, 0., GMP_RNDN);
  mpfr_set_inf(min_val, 1);

  if((computedQuality!=NULL) || (infinityNorm != NULL)) {
    for(i=1;i<=n;i++) {
      r = evaluateFaithfulWithCutOffFast(var_mpfr, error, error_diff, z[i-1], zero_mpfr, prec);
      if(r==0) mpfr_set_d(var_mpfr, 0., GMP_RNDN);
      
      mpfr_abs(var_mpfr, var_mpfr, GMP_RNDN);
      if(mpfr_cmp(var_mpfr, max_val) > 0) mpfr_set(max_val, var_mpfr, GMP_RNDU);
      if(mpfr_cmp(var_mpfr, min_val) < 0) mpfr_set(min_val, var_mpfr, GMP_RNDD);
    }

    mpfr_div(var_mpfr, max_val, min_val, GMP_RNDU);
    mpfr_sub_ui(var_mpfr, var_mpfr, 1, GMP_RNDU);
  }

  if(computedQuality!=NULL) mpfr_set(computedQuality, var_mpfr, GMP_RNDU);
  if(infinityNorm!=NULL) mpfr_set(infinityNorm, max_val, GMP_RNDU);

  if(verbosity>=3) {
    changeToWarningMode();
    mpfr_set(dummy_mpfr2,max_val,GMP_RNDN);
    sollyaPrintf("Current norm: "); printValue(&max_val); //myPrintValue(&dummy_mpfr2, 5) ;
    mpfr_set(dummy_mpfr2,var_mpfr,GMP_RNDN);
    sollyaPrintf(" (1 +/- "); myPrintValue(&dummy_mpfr2, 5);
    sollyaPrintf(")\n");
    restoreMode();
  }


  free_memory(error);
  free_memory(error_diff);
  free_memory(error_diff2);
  mpfr_clear(var_mpfr);
  mpfr_clear(zero_mpfr);
  mpfr_clear(dummy_mpfr);
  mpfr_clear(dummy_mpfr2);
  mpfr_clear(max_val);
  mpfr_clear(min_val);
  free(s);

  for(i=0;i<=n;i++)  mpfr_clear(y[i]);
  free(y);
  for(i=1;i<=n;i++) {
    mpfr_set(x[i-1], z[i-1], GMP_RNDN);
    mpfr_clear(z[i-1]);
  }
  free(z);

  return 0;
}

node *remezAux(node *f, node *w, chain *monomials, mpfr_t u, mpfr_t v, mp_prec_t prec, mpfr_t quality) {
  int freeDegrees = lengthChain(monomials);
  int i,j, r, count, test, crash, HaarCompliant;
  mpfr_t zero_mpfr, var1, var2, var3, computedQuality, infinityNorm;
  mpfr_t *ptr;
  node *temp_tree;
  node *temp_tree2;
  node *temp_tree3;
  node *poly;
  node *poly_diff;
  node *poly_diff2;
  node *f_diff;
  node *f_diff2;
  node *w_diff;
  node *w_diff2;
  chain *curr;
  node **monomials_tree;
  mpfr_t *x;
  mpfr_t *M;
  mpfr_t *N;
  mpfr_t *b;
  mpfr_t *c;
  mpfr_t *ai_vect;
  mpfr_t *lambdai_vect;
  mpfr_t *previous_lambdai_vect;
  mpfr_t perturb;
  gmp_randstate_t random_state;

  gmp_randinit_default(random_state);
  gmp_randseed_ui(random_state, 65845285);

  HaarCompliant=1;

  if(verbosity>=3) {
    changeToWarningMode();
    sollyaPrintf("Entering in Remez function...\n");
    sollyaPrintf("Required quality :"); printMpfr(quality);
    restoreMode();
  }

  // Initialisations and precomputations
  mpfr_init2(var1, prec);
  mpfr_init2(var2, prec);
  mpfr_init2(var3, prec);

  mpfr_init2(zero_mpfr, 53);
  mpfr_set_d(zero_mpfr, 0., GMP_RNDN);

  if (mpfr_get_prec(quality)>prec) mpfr_init2(computedQuality, mpfr_get_prec(quality));
  else mpfr_init2(computedQuality, prec);

  mpfr_set_inf(computedQuality, 1);
  mpfr_init2(infinityNorm, prec);

  M = safeMalloc((freeDegrees+1)*(freeDegrees+1)*sizeof(mpfr_t));
  N = safeMalloc(freeDegrees*freeDegrees*sizeof(mpfr_t));
  b = safeMalloc((freeDegrees+1)*sizeof(mpfr_t));
  c = safeMalloc(freeDegrees*sizeof(mpfr_t));
  ai_vect = safeMalloc((freeDegrees+1)*sizeof(mpfr_t));
  lambdai_vect = safeMalloc((freeDegrees+1)*sizeof(mpfr_t));
  previous_lambdai_vect = safeMalloc((freeDegrees+1)*sizeof(mpfr_t));
  x = safeMalloc((freeDegrees+1)*sizeof(mpfr_t));
  
  for(j=1; j <= freeDegrees+1 ; j++) {
    for(i=1; i<= freeDegrees+1; i++) {
      mpfr_init2(M[coeff(i,j,freeDegrees+1)],prec);
    }
    mpfr_init2(b[j-1], prec);
    mpfr_init2(ai_vect[j-1], prec);
    mpfr_init2(x[j-1], prec);
  }

  for(j=1; j <= freeDegrees ; j++) {
    for(i=1; i<= freeDegrees; i++) {
      mpfr_init2(N[coeff(i,j,freeDegrees)],prec);
    }
    mpfr_init2(c[j-1], prec);
    mpfr_init2(lambdai_vect[j-1], prec);
    mpfr_init2(previous_lambdai_vect[j-1], prec);
  }
  mpfr_init2(lambdai_vect[freeDegrees], prec);
  mpfr_init2(previous_lambdai_vect[freeDegrees], prec);
  mpfr_set_si(lambdai_vect[freeDegrees],-1,GMP_RNDN);
  mpfr_set_si(previous_lambdai_vect[freeDegrees],-1,GMP_RNDN);


  if(verbosity>=8)  { changeToWarningMode(); sollyaPrintf("Differentiating functions...\n"); restoreMode(); }
  pushTimeCounter();
  poly = NULL;
  f_diff = differentiate(f);
  f_diff2 = differentiate(f_diff);
  w_diff = differentiate(w);
  w_diff2 = differentiate(w_diff);
  popTimeCounter("Remez: differentiating the functions");


  if(verbosity>=8)  { changeToWarningMode(); sollyaPrintf("Computing monomials...\n"); restoreMode(); }
  pushTimeCounter();
  monomials_tree = safeMalloc(freeDegrees*sizeof(node *));
  curr = monomials;
  for(j=0;j<freeDegrees;j++) {
    temp_tree = safeMalloc(sizeof(node));
    temp_tree->nodeType = VARIABLE;
    temp_tree2 = safeMalloc(sizeof(node));
    temp_tree2->nodeType = CONSTANT;
    ptr = safeMalloc(sizeof(mpfr_t));
    mpfr_init2(*ptr, prec);
    mpfr_set_si(*ptr, (long) (*((int *)(curr->value))), GMP_RNDN);
    temp_tree2->value = ptr;
    
    temp_tree3 = safeMalloc(sizeof(node));
    temp_tree3->nodeType = POW;
    temp_tree3->child1 = temp_tree;
    temp_tree3->child2 = temp_tree2;

    monomials_tree[j] = temp_tree3;
    curr=curr->next;
  }
  popTimeCounter("Remez: computing monomials");


  count = 0;

  // Definition of the array x of the n+2 Chebychev points

  if(verbosity>=8) {
    changeToWarningMode();
    sollyaPrintf("Computing an initial points set...\n");
    restoreMode();
  }
  pushTimeCounter();

  /*************************************************************/
  mpfr_const_pi(var1, GMP_RNDN);
  mpfr_div_si(var1, var1, (long)freeDegrees, GMP_RNDN); // var1 = Pi/freeDegrees
  mpfr_sub(var2, u, v, GMP_RNDN);
  mpfr_div_2ui(var2, var2, 1, GMP_RNDN); // var2 = (u-v)/2
  mpfr_add(var3, u, v, GMP_RNDN);
  mpfr_div_2ui(var3, var3, 1, GMP_RNDN); // var3 = (u+v)/2
  
  for (i=1 ; i <= freeDegrees+1 ; i++) {
    mpfr_mul_si(x[i-1], var1, i-1, GMP_RNDN);
    mpfr_cos(x[i-1], x[i-1], GMP_RNDN);
    mpfr_fma(x[i-1], x[i-1], var2, var3, GMP_RNDN); // x_i = [cos((i-1)*Pi/freeDegrees)]*(u-v)/2 + (u+v)/2
  }

  /* Random pertubration of the points... */
  mpfr_init2(perturb, prec);
  for(i=2;i<=freeDegrees;i++) {
    mpfr_urandomb(perturb, random_state);
    mpfr_mul_2ui(perturb, perturb, 1, GMP_RNDN);
    mpfr_sub_ui(perturb, perturb, 1, GMP_RNDN);
    mpfr_div_2ui(perturb, perturb, 2, GMP_RNDN); // perturb \in [-1/4; 1/4]
    /* mpfr_set_d(perturb,0.,GMP_RNDN); // if no perturbation is desired */
    
    mpfr_sub(var1, x[i-1], x[i-2], GMP_RNDN);
    mpfr_sub(var2, x[i], x[i-1], GMP_RNDN);
    if (mpfr_cmpabs(var1,var2)>0) mpfr_mul(var3, var2, perturb, GMP_RNDN);
    else mpfr_mul(var3, var1, perturb, GMP_RNDN);
    mpfr_add(x[i-1], x[i-1], var3, GMP_RNDN);
  }
  mpfr_clear(perturb);


  /*************************************************************/


  /*************************************************************/
  /*                  Manually chosen points                   */
  // assume the list of points to be stored in variable list:
  // run:   i=0; for t in list do {write("mpfr_set_str(x[",i,"],\"",t,"\", 10, GMP_RNDN);\n"); i=i+1;} ;

  /*   mpfr_set_str(x[0],"-0.3125e-1", 10, GMP_RNDN); */
  /*   mpfr_set_str(x[1],"-0.270866924296709954921192919457109841628145246303031e-1", 10, GMP_RNDN); */
  /*   mpfr_set_str(x[2],"-0.156792125481182037300460865714390558114871681447265e-1", 10, GMP_RNDN); */
  /*   mpfr_set_str(x[3],"0.270866923865259682207709736846537325622398567463872667e-1", 10, GMP_RNDN); */


  /*************************************************************/



  /*************************************************************/
  /*                 Evenly distributed points                 */
  //mpfr_sub(var1, v, u, GMP_RNDN);
  //mpfr_div_si(var1, var1, (long)(freeDegrees), GMP_RNDN); // var1 = (v-u)/freeDegrees
  
  //for (i=1 ; i <= freeDegrees+1 ; i++) {
  //  mpfr_mul_si(x[i-1], var1, i-1, GMP_RNDN);
  //  mpfr_add(x[i-1], x[i-1], u, GMP_RNDN);
  //}
  /*************************************************************/


  /*************************************************************/
  /*                  Alternative Cheb points                  */
  //mpfr_const_pi(var1, GMP_RNDN);
  //mpfr_div_si(var1, var1, 2*((long)freeDegrees+1), GMP_RNDN); // var1 = Pi/(2*freeDegrees+2)
  //mpfr_sub(var2, u, v, GMP_RNDN);
  //mpfr_div_2ui(var2, var2, 1, GMP_RNDN); // var2 = (u-v)/2
  //mpfr_add(var3, u, v, GMP_RNDN);
  //mpfr_div_2ui(var3, var3, 1, GMP_RNDN); // var3 = (u+v)/2

  //for (i=1 ; i <= freeDegrees+1 ; i++) {
  //  mpfr_mul_si(x[i-1], var1, 2*i-1, GMP_RNDN);
  //  mpfr_cos(x[i-1], x[i-1], GMP_RNDN);
  //  mpfr_fma(x[i-1], x[i-1], var2, var3, GMP_RNDN); // x_i=[cos((2i-1)*Pi/(2freeDegrees+2))]*(u-v)/2 + (u+v)/2
  //}
  /*************************************************************/

  popTimeCounter("Remez: computing initial points set");
  if(verbosity>=4) {
    changeToWarningMode();
    sollyaPrintf("Computed points set:\n");
    for(i=1;i<=freeDegrees+1;i++) printMpfr(x[i-1]);
    restoreMode();
  }

  while((mpfr_cmp(computedQuality, quality)>0)  && (count<200)) {  
    while((mpfr_cmp(computedQuality, quality)>0) && (count<200)) {
      free_memory(poly);

      // Definition of the matrices M and N of Remez algorithm
      // N lets us determine the modified alternation property
      // M lets us solve the interpolation problem
      if(verbosity>=3) {
	changeToWarningMode();
	sollyaPrintf("Step %d\n",count);
	sollyaPrintf("Computing the matrix...\n");
	restoreMode();
      }
      pushTimeCounter();

      for (i=1 ; i <= freeDegrees+1 ; i++) {
	r = evaluateFaithfulWithCutOffFast(var1, w, NULL, x[i-1], zero_mpfr, prec);
	if((r==1) && (mpfr_number_p(var1))) test=1;
	else test=0;
		 
	for (j=1 ; j <= freeDegrees ; j++) {
	  if(test==1) {
	    r = evaluateFaithfulWithCutOffFast(var2, monomials_tree[j-1], NULL, x[i-1], zero_mpfr, prec);
	    if((r==1) && (mpfr_number_p(var2))) {
	      mpfr_mul(var2, var1, var2, GMP_RNDN);
	      mpfr_set(M[coeff(i,j,freeDegrees+1)],var2,GMP_RNDN);
	      if (i<=freeDegrees) mpfr_set(N[coeff(j,i,freeDegrees)],var2,GMP_RNDN);
	      else mpfr_set(c[j-1],var2,GMP_RNDN);
	    }
	  }
	  if((test==0) || (r==0) || (!mpfr_number_p(var2))) {
	    printMessage(2,"Information: the construction of M[%d,%d] uses a slower algorithm\n",i,j);
	    temp_tree = safeMalloc(sizeof(node));
	    temp_tree->nodeType = MUL;
	    temp_tree->child1 = copyTree(monomials_tree[j-1]);
	    temp_tree->child2 = copyTree(w);
	  
	    temp_tree2 = simplifyTreeErrorfree(temp_tree);
	    free_memory(temp_tree);
	    temp_tree = temp_tree2; // temp_tree = x^(monomials[j])*w(x)
	  
	    r = evaluateFaithfulWithCutOffFast(var3, temp_tree, NULL, x[i-1], zero_mpfr, prec);

	    if(r==0) mpfr_set_d(var3, 0., GMP_RNDN);
	    mpfr_set(M[coeff(i,j,freeDegrees+1)],var3,GMP_RNDN);
	    if (i<=freeDegrees) mpfr_set(N[coeff(j,i,freeDegrees)],var3,GMP_RNDN);
	    else mpfr_set(c[j-1], var3, GMP_RNDN);
	    free_memory(temp_tree);
	  }
	}
      }

      system_solve( lambdai_vect , N, c, freeDegrees, prec);

      if(HaarCompliant==2) HaarCompliant=0; /* Too many oscillations: forcing exchange step */
      else {
	HaarCompliant=1;
	for(i=1; i<=freeDegrees+1; i++) {
	  if (mpfr_sgn(lambdai_vect[i-1])*mpfr_sgn(previous_lambdai_vect[i-1])<=0) HaarCompliant=0;
	}
	if(count==0) HaarCompliant=1;
      }

      for (i=1 ; i <= freeDegrees+1 ; i++) {
	if (mpfr_sgn(lambdai_vect[i-1])>0)
	  mpfr_set_si(M[coeff(i, freeDegrees+1, freeDegrees+1)], 1 ,GMP_RNDN);
	else {
	  if (mpfr_sgn(lambdai_vect[i-1])<0)
	    mpfr_set_si(M[coeff(i, freeDegrees+1, freeDegrees+1)], -1 ,GMP_RNDN);
	  else {
	    printMessage(1,"Warning: degenerated system in a non Haar context. The algorithm may be incorrect.\n");
	    mpfr_set_si(M[coeff(i, freeDegrees+1, freeDegrees+1)], 1 ,GMP_RNDN);
	  }
	}
      }

      if(verbosity>=4) {
	changeToWarningMode();
	sollyaPrintf("Signs for pseudo-alternating condition : [");
	for (i=1 ; i <= freeDegrees ; i++) {
	  myPrintValue(&M[coeff(i, freeDegrees+1, freeDegrees+1)],10);
	  sollyaPrintf(", ");
	}
	sollyaPrintf("-1]\n");
	restoreMode();
      }

      popTimeCounter("Remez: computing the matrix");

      if(verbosity>=7) {
	changeToWarningMode();
	sollyaPrintf("The computed matrix is "); printMatrix(M, freeDegrees+1);
	restoreMode();
      }
        
    
      // Determination of the polynomial corresponding to M and x
      for (i=1 ; i <= freeDegrees+1 ; i++) {
	r = evaluateFaithfulWithCutOffFast(var1, f, NULL, x[i-1], zero_mpfr, prec); // var1=f(x_i)
	if(r==0) mpfr_set_d(var1, 0., GMP_RNDN);

	mpfr_set(b[i-1],var1,GMP_RNDN);
      }

      if(verbosity>=8) { changeToWarningMode(); sollyaPrintf("Resolving the system...\n"); restoreMode(); }

      pushTimeCounter();
      system_solve(ai_vect, M, b, freeDegrees+1, prec);
      popTimeCounter("Remez: solving the system");

      poly = constructPolynomial(ai_vect, monomials, prec);

      if(verbosity>=4) {
	changeToWarningMode();
	sollyaPrintf("The computed polynomial is "); printTree(poly); sollyaPrintf("\n");
	restoreMode();
      }
      if(verbosity>=3) {
	changeToWarningMode();
	sollyaPrintf("Current value of epsilon : "); myPrintValue(&ai_vect[freeDegrees],53); sollyaPrintf("\n");
	restoreMode();
      }

      // Plotting the error curve
      /*     node *plotTemp; */
      /*     chain *plotList=NULL; */
      /*     plotTemp = makeSub(makeMul(copyTree(poly),copyTree(w)),copyTree(f)); */
      /*     plotList=addElement(plotList, plotTemp); */
      /*     plotTree(plotList,u,v,defaultpoints,prec,NULL,0); */
      /*     free_memory(plotTemp); */
      //    freeChain(plotList, nothing);

      // Computing the useful derivatives of functions
      if(verbosity>=8) {
	changeToWarningMode();
	sollyaPrintf("Differentiating the computed polynomial...\n");
	restoreMode();
      }
    
      pushTimeCounter();
    
      temp_tree = horner(poly);
      free_memory(poly);
      poly = temp_tree;

      poly_diff = differentiate(poly);
      poly_diff2 = differentiate(poly_diff);
    
      popTimeCounter("Remez: differentiating the polynomial");

      if(verbosity>=8) {
	changeToWarningMode();
	sollyaPrintf("Searching extrema of the error function...\n");
	restoreMode();
      }
    
      // Find extremas and tests the quality of the current approximation
      pushTimeCounter();

      crash = qualityOfError(computedQuality, infinityNorm, x,
			     poly, f, w,
			     monomials_tree, lambdai_vect, ai_vect[freeDegrees], HaarCompliant,
			     freeDegrees, u, v, prec);
      popTimeCounter("Remez: computing the quality of approximation");

      if(crash==-1) {

	// temporary check until I patch the algorithm in order to handle
	// correctly cases when the error oscillates too much
	mpfr_t ninf;
	mpfr_init2(ninf, prec);
      
	temp_tree = makeSub(makeMul(copyTree(poly), copyTree(w)), copyTree(f));
	uncertifiedInfnorm(ninf, temp_tree, u, v, getToolPoints(), prec);

	if(verbosity>=1) {
	  changeToWarningMode();
	  sollyaPrintf("The best polynomial obtained gives an error of ");
	  printMpfr(ninf);
	  sollyaPrintf("\n");
	  restoreMode();
	}

	free_memory(temp_tree);
	mpfr_clear(ninf);
	// end of the temporary check


	for(j=0;j<freeDegrees;j++) {
	  free_memory(monomials_tree[j]);
	}
	free(monomials_tree);
      
	for(j=1;j<=freeDegrees+1;j++) mpfr_clear(x[j-1]);
	free(x);

	free_memory(poly_diff);
	free_memory(poly_diff2);
	mpfr_clear(zero_mpfr);
	mpfr_clear(var1);
	mpfr_clear(var2);
	mpfr_clear(var3);
	free_memory(f_diff);
	free_memory(f_diff2);
	free_memory(w_diff);
	free_memory(w_diff2);
	mpfr_clear(computedQuality);
	mpfr_clear(infinityNorm);

	for(j=1; j <= freeDegrees+1 ; j++) {
	  for(i=1; i<= freeDegrees+1; i++) {
	    mpfr_clear(M[coeff(i,j,freeDegrees+1)]);
	  }
	  mpfr_clear(b[j-1]);
	  mpfr_clear(ai_vect[j-1]);
	}
	free(M);
	free(b);
	free(ai_vect);

	for(j=1; j <= freeDegrees ; j++) {
	  for(i=1; i<= freeDegrees; i++) {
	    mpfr_clear(N[coeff(i,j,freeDegrees)]);
	  }
	  mpfr_clear(c[j-1]);
	  mpfr_clear(lambdai_vect[j-1]);
	  mpfr_clear(previous_lambdai_vect[j-1]);
	}
	mpfr_clear(lambdai_vect[freeDegrees]);
	mpfr_clear(previous_lambdai_vect[freeDegrees]);
	free(N);
	free(c);
	free(lambdai_vect);
	free(previous_lambdai_vect);
	
	gmp_randclear(random_state);

	recoverFromError();
      }
      
      if(verbosity>=3) {
	changeToWarningMode();
	sollyaPrintf("Current quality: "); printMpfr(computedQuality);
	restoreMode();
      }

      count++;
      for(i=1; i<=freeDegrees+1; i++) {
	mpfr_set(previous_lambdai_vect[i-1], lambdai_vect[i-1], GMP_RNDN);
      }



      free_memory(poly_diff);
      free_memory(poly_diff2);
    }


    // temporary check until I patch the algorithm in order to handle
    // correctly cases when the error oscillates too much
    mpfr_t ninf;
    mpfr_init2(ninf, prec);

    temp_tree = makeSub(makeMul(copyTree(poly), copyTree(w)), copyTree(f));
    uncertifiedInfnorm(ninf, temp_tree, u, v, getToolPoints(), prec);
    free_memory(temp_tree);

    mpfr_add_ui(computedQuality, computedQuality, 1, GMP_RNDU);
    mpfr_mul(computedQuality, computedQuality, ninf, GMP_RNDU);
    mpfr_div(computedQuality, computedQuality, infinityNorm, GMP_RNDU);
    mpfr_sub_ui(computedQuality, computedQuality, 1, GMP_RNDU);

    mpfr_clear(ninf);

    if(mpfr_cmp(computedQuality, quality)>0) {
      changeToWarningMode();
      printMessage(2, "Warning: Remez algorithm failed (too many oscillations?)\n");
      printMessage(2, "Looping again\n");
      HaarCompliant=2;
      restoreMode();
    }
    
    // end of the temporary check
  }
  
  if(verbosity>=2) {
    changeToWarningMode();
    sollyaPrintf("Remez finished after %d steps\n",count);
    sollyaPrintf("The computed infnorm is "); myPrintValue(&infinityNorm, 53) ; sollyaPrintf("\n");
    sollyaPrintf("The polynomial is optimal within a factor 1 +/- "); myPrintValue(&computedQuality, 5); sollyaPrintf("\n");
    if(verbosity>=5) { sollyaPrintf("Computed poly: "); printTree(poly); sollyaPrintf("\n");}
    restoreMode();
  }


  for(j=0;j<freeDegrees;j++) {
    free_memory(monomials_tree[j]);
  }
  free(monomials_tree);

  for(i=1; i<=freeDegrees+1; i++) {
    mpfr_clear(x[i-1]);
  }
  free(x);


  mpfr_clear(zero_mpfr);
  mpfr_clear(var1);
  mpfr_clear(var2);
  mpfr_clear(var3);
  free_memory(f_diff);
  free_memory(f_diff2);
  free_memory(w_diff);
  free_memory(w_diff2);

  for(j=1; j <= freeDegrees+1 ; j++) {
    for(i=1; i<= freeDegrees+1; i++) {
      mpfr_clear(M[coeff(i,j,freeDegrees+1)]);
    }
    mpfr_clear(b[j-1]);
    mpfr_clear(ai_vect[j-1]);
  }
  free(M);
  free(b);
  free(ai_vect);

  for(j=1; j <= freeDegrees ; j++) {
    for(i=1; i<= freeDegrees; i++) {
      mpfr_clear(N[coeff(i,j,freeDegrees)]);
    }
    mpfr_clear(c[j-1]);
    mpfr_clear(lambdai_vect[j-1]);
    mpfr_clear(previous_lambdai_vect[j-1]);
  }
  mpfr_clear(lambdai_vect[freeDegrees]);
  mpfr_clear(previous_lambdai_vect[freeDegrees]);
  free(N);
  free(c);
  free(lambdai_vect);
  free(previous_lambdai_vect);

  gmp_randclear(random_state);

  if (mpfr_cmp(computedQuality, quality)>0) {
    sollyaFprintf(stderr, "Error in Remez: the algorithm does not converge.\n");
    mpfr_clear(computedQuality);
    mpfr_clear(infinityNorm);
    recoverFromError();
  }

  mpfr_clear(computedQuality);
  mpfr_clear(infinityNorm);

  return poly;
}


// Suppose that the list monom is sorted.
// Tests whether monom contains two equal ints.
int testMonomials(chain *monom) {
  chain *curr;

  if (monom == NULL) return 1;
  
  curr = monom;
  while (curr->next != NULL) {
    if (*((int *) (curr->value)) == *((int *) (curr->next->value))) return 0;
    curr = curr->next;
  }

  return 1;
}

  
node *remez(node *func, node *weight, chain *monomials, mpfr_t a, mpfr_t b, mpfr_t *requestedQuality, mp_prec_t prec) {
  mpfr_t quality;
  node *res;
  chain *monomials2;
  
  if (requestedQuality==NULL) {
    mpfr_init2(quality, 53);
    mpfr_set_d(quality, 0.00001, GMP_RNDN);
  }
  else {
    mpfr_init2(quality, mpfr_get_prec(*requestedQuality));
    mpfr_abs(quality, *requestedQuality, GMP_RNDN);
  }
  
  monomials2 = copyChain(monomials, copyIntPtrOnVoid);

  sortChain(monomials2,cmpIntPtr);

  if (!testMonomials(monomials2)) {
    sollyaFprintf(stderr,"Error: monomial degree is given twice in argument to Remez algorithm.\n");
    recoverFromError();
  }

  if (mpfr_equal_p(a,b))
    printMessage(1,"Warning: the input interval is reduced to a single point. The algorithm may not converge.\n");

  res = remezAux(func, weight, monomials2, a, b, prec, quality);

  freeChain(monomials2, freeIntPtr);
  mpfr_clear(quality);
  return res;
}






node *constructPolynomialFromArray(mpfr_t *coeff, node **monomials_tree, int n) {
  int i;
  node *poly;

  poly = makeConstantDouble(0.0);
  for(i=0;i<n;i++) poly = makeAdd(makeMul(makeConstant(coeff[i]), copyTree(monomials_tree[i])),poly);

  return poly;
}


/* Construct a set of p Chebychev's points corresponding to the extrema
   of the Chebychev polynomial */
mpfr_t *chebychevsPoints(mpfr_t u, mpfr_t v, int p, mp_prec_t *currentPrec) {
  mpfr_t var1, var2, var3;
  mpfr_t *x;
  int i;
  mpfr_init2(var1, *currentPrec);
  mpfr_init2(var2, *currentPrec);
  mpfr_init2(var3, *currentPrec);

  x = (mpfr_t *)safeMalloc(p*sizeof(mpfr_t));

  mpfr_const_pi(var1, GMP_RNDN);
  mpfr_div_si(var1, var1, (long)(p-1), GMP_RNDN); // var1 = Pi/(p-1)
  mpfr_sub(var2, u, v, GMP_RNDN);
  mpfr_div_2ui(var2, var2, 1, GMP_RNDN); // var2 = (u-v)/2
  mpfr_add(var3, u, v, GMP_RNDN);
  mpfr_div_2ui(var3, var3, 1, GMP_RNDN); // var3 = (u+v)/2
  
  for (i=1 ; i <= p ; i++) {
    mpfr_init2(x[i-1], *currentPrec);
    mpfr_mul_si(x[i-1], var1, i-1, GMP_RNDN);
    mpfr_cos(x[i-1], x[i-1], GMP_RNDN);
    mpfr_fma(x[i-1], x[i-1], var2, var3, GMP_RNDN); // x_i = [cos((i-1)*Pi/(p-1))]*(u-v)/2 + (u+v)/2
  }

  mpfr_clear(var1);
  mpfr_clear(var2);
  mpfr_clear(var3);
  
  return x;
}


/* Random pertubration of the points... */
/* There is p points : x0 ... x(p-1)    */
void perturbPoints(mpfr_t *x, int p, mp_prec_t *currentPrec) {
  mpfr_t perturb;
  mpfr_t var1, var2, var3;
  int i;
  gmp_randstate_t random_state;

  gmp_randinit_default(random_state);
  gmp_randseed_ui(random_state, 65845285);

  mpfr_init2(perturb, *currentPrec);
  mpfr_init2(var1, *currentPrec);
  mpfr_init2(var2, *currentPrec);
  mpfr_init2(var3, *currentPrec);

  for(i=1;i<=p-2;i++) {
    mpfr_urandomb(perturb, random_state);
    mpfr_mul_2ui(perturb, perturb, 1, GMP_RNDN);
    mpfr_sub_ui(perturb, perturb, 1, GMP_RNDN);
    mpfr_div_2ui(perturb, perturb, 2, GMP_RNDN); // perturb \in [-1/4; 1/4]

    mpfr_sub(var1, x[i], x[i-1], GMP_RNDN);
    mpfr_sub(var2, x[i+1], x[i], GMP_RNDN);
    if (mpfr_cmpabs(var1,var2)>0) mpfr_mul(var3, var2, perturb, GMP_RNDN);
    else mpfr_mul(var3, var1, perturb, GMP_RNDN);
    mpfr_add(x[i], x[i], var3, GMP_RNDN);
  }

  gmp_randclear(random_state);
  mpfr_clear(perturb);
  mpfr_clear(var1);
  mpfr_clear(var2);
  mpfr_clear(var3);
  return;
}

mpfr_t *remezMatrix(node *w, mpfr_t *x, node **monomials_tree, int n, mp_prec_t *currentPrec) {
  mpfr_t var1, var2, var3, zero_mpfr;
  mpfr_t *M;
  int i,j,r, test;
  node *temp_tree, *temp_tree2;
  mp_prec_t prec= *currentPrec;

  M = (mpfr_t *)(safeMalloc((n+1)*(n+1)*sizeof(mpfr_t)));
  
  mpfr_init2(var1, prec);
  mpfr_init2(var2, prec);
  mpfr_init2(var3, prec);
  mpfr_init2(zero_mpfr, 53);
  mpfr_set_d(zero_mpfr, 0., GMP_RNDN);

  for (i=1 ; i <= n+1 ; i++) {
    mpfr_init2(M[coeff(i,n+1,n+1)], prec);
    if(i%2==0) mpfr_set_si(M[coeff(i,n+1,n+1)], 1, GMP_RNDN);
    else mpfr_set_si(M[coeff(i,n+1,n+1)], -1, GMP_RNDN);

    r = evaluateFaithfulWithCutOffFast(var1, w, NULL, x[i-1], zero_mpfr, prec);
    if((r==1) && (mpfr_number_p(var1))) test=1;
    else test=0;

    for (j=1 ; j <= n ; j++) {
      mpfr_init2(M[coeff(i,j,n+1)], prec);

      if(test==1) {
	r = evaluateFaithfulWithCutOffFast(var2, monomials_tree[j-1], NULL, x[i-1], zero_mpfr, prec);
	if((r==1) && (mpfr_number_p(var2))) {
	  mpfr_mul(var2, var1, var2, GMP_RNDN);
	  mpfr_set(M[coeff(i,j,n+1)],var2,GMP_RNDN);
	}
      }
      if((test==0) || (r==0) || (!mpfr_number_p(var2))) {
	printMessage(2,"Information: the construction of M[%d,%d] uses a slower algorithm\n",i,j);
	temp_tree = safeMalloc(sizeof(node));
	temp_tree->nodeType = MUL;
	temp_tree->child1 = copyTree(monomials_tree[j-1]);
	temp_tree->child2 = copyTree(w);
	
	temp_tree2 = simplifyTreeErrorfree(temp_tree);
	free_memory(temp_tree);
	temp_tree = temp_tree2; // temp_tree = x^(monomials[j])*w(x)
	
	r = evaluateFaithfulWithCutOffFast(var3, temp_tree, NULL, x[i-1], zero_mpfr, prec);
	
	if(r==0) mpfr_set_d(var3, 0., GMP_RNDN);
	mpfr_set(M[coeff(i,j,n+1)],var3,GMP_RNDN);

	free_memory(temp_tree);
      }
    }
  }

  
  mpfr_clear(zero_mpfr);
  mpfr_clear(var1);
  mpfr_clear(var2);
  mpfr_clear(var3);

  return M;
}


node *elementaryStepRemezAlgorithm(mpfr_t *h,
				   node *func, node *weight, mpfr_t *x,
				   node **monomials_tree, int n,
				   mp_prec_t *currentPrec) {
  mpfr_t *M;
  mpfr_t *b, *ai_vect;
  int i,j;
  mpfr_t zero_mpfr;
  node *poly;
  int r;

  mpfr_init2(zero_mpfr, 53);
  mpfr_set_d(zero_mpfr, 0., GMP_RNDN);

  b = (mpfr_t *)(safeMalloc((n+1)*sizeof(mpfr_t)));
  ai_vect = (mpfr_t *)(safeMalloc((n+1)*sizeof(mpfr_t)));

  for(i=0;i<=n;i++) {
    mpfr_init2(b[i], *currentPrec);
    r=evaluateFaithfulWithCutOffFast(b[i], func, NULL, x[i], zero_mpfr, *currentPrec);
    if (r == 0) mpfr_set_d(b[i], 0., GMP_RNDN);
  }

  for(i=0;i<=n;i++) mpfr_init2(ai_vect[i], *currentPrec);

  M = remezMatrix(weight, x, monomials_tree, n, currentPrec);
  system_solve(ai_vect, M, b, n+1, *currentPrec);
  
  poly = constructPolynomialFromArray(ai_vect, monomials_tree, n);
  if (h!=NULL)   mpfr_set(*h, ai_vect[n], GMP_RNDU);

  for(i=0;i<=n;i++) mpfr_clear(b[i]);
  free(b);

  for(i=0;i<=n;i++) mpfr_clear(ai_vect[i]);
  free(ai_vect);

  for(i=1;i<=n+1;i++) {
    for(j=1;j<=n+1;j++) mpfr_clear(M[coeff(i,j,n+1)]);
  }
  free(M);

  mpfr_clear(zero_mpfr);
  return poly;
}

void radiusBasicMinimaxChebychevsPoints(mpfr_t *h, node *func, node *weight, mpfr_t a, mpfr_t b, int n, mp_prec_t *currentPrec) {
  mpfr_t *x;
  node **monomials_tree;
  int i;
  node *poly;

  monomials_tree = (node **)safeMalloc(n*sizeof(node *));
  monomials_tree[0] = makeConstantDouble(1.);
  for(i=1;i<n;i++) monomials_tree[i] = makePow(makeVariable(), makeConstantDouble((double)i));


  x = chebychevsPoints(a,b,n+1,currentPrec);
  perturbPoints(x, n+1, currentPrec);
  poly = elementaryStepRemezAlgorithm(h, func, weight, x, monomials_tree, n, currentPrec);
  mpfr_abs(*h, *h, GMP_RNDN);

  free_memory(poly);

  for(i=0;i<n;i++) free_memory(monomials_tree[i]);
  free(monomials_tree);

  for(i=0;i<=n;i++) mpfr_clear(x[i]);
  free(x);

  return;
}

void firstStepContinuousMinimaxChebychevsPoints(mpfr_t *h, node *func, node *weight, mpfr_t a, mpfr_t b, int n, mp_prec_t *currentPrec) {
  mpfr_t *x;
  node **monomials_tree;
  int i;
  node *poly;
  node *error;
  monomials_tree = (node **)(safeMalloc(n*sizeof(node *)));
  monomials_tree[0] = makeConstantDouble(1.);
  for(i=1;i<n;i++) monomials_tree[i] = makePow(makeVariable(), makeConstantDouble((double)i));

  x = chebychevsPoints(a,b,n+1,currentPrec);
  perturbPoints(x, n+1, currentPrec);
  poly = elementaryStepRemezAlgorithm(NULL, func, weight, x, monomials_tree, n, currentPrec);

  error = makeSub(makeMul(copyTree(poly), copyTree(weight)), copyTree(func));
  uncertifiedInfnorm(*h, error, a, b, 3*n, getToolPrecision());

  free_memory(error);
  free_memory(poly);

  for(i=0;i<n;i++) free_memory(monomials_tree[i]);
  free(monomials_tree);

  for(i=0;i<=n;i++) mpfr_clear(x[i]);
  free(x);

  return;
}

rangetype guessDegree(node *func, node *weight, mpfr_t a, mpfr_t b, mpfr_t eps, int bound) {
  int n=1;
  int n_min, n_max;
  mp_prec_t prec = getToolPrecision();
  mpfr_t h;
  rangetype range;
  mpfr_t *tempMpfr;

  mpfr_init2(h, prec);


  /* n reprensents the number of unknowns: n=1 corresponds to degree 0 */
  /* bound represents the maximal value allowed for n. If we do not find a
     suitable n<=bound, we return an interval [*, +inf]. We assume bound>=1 */

  /* We try n=1, 2, 4, 8, etc. until we find one for which the basic
     minimax problem achieve the required bound eps */
  pushTimeCounter();
  radiusBasicMinimaxChebychevsPoints(&h, func, weight, a, b, n, &prec);
  if(verbosity>=4) {
    changeToWarningMode();
    sollyaPrintf("Information: guessdegree: trying degree %d. Found radius: ",n-1);
    printMpfr(h);
    restoreMode();
  }


  /* If h<eps, we may be in a degenerated case (for instance, an even
     function on a symetrical interval, that leads to a huge difference
     between the radius of the basic minimax problem and of the continuous
     minimax problem. We try, n=2 for a confirmation. */
  if(mpfr_cmp(h,eps)<0) {
    n=2;
    radiusBasicMinimaxChebychevsPoints(&h, func, weight, a, b, n, &prec);
    if(verbosity>=4) {
      changeToWarningMode();
      sollyaPrintf("Information: guessdegree: trying degree %d. Found radius: ",n-1);
      printMpfr(h);
      restoreMode();
    }

    if (mpfr_cmp(h,eps)<0) n=1; /* OK. Sorry. The system seems to be normal */
  }

  /* Here, if n=1, it means that we are allowed to think that n=1 achieves the
     continuous problem (at least, based on what we can say from the discrete
     problem.)
     If n=2, it means that we know for sure that n=2 does not achieve the
     continuous problem.
  */

  while(mpfr_cmp(h,eps) >= 0) {
    n *= 2;
    if (n<bound)
      radiusBasicMinimaxChebychevsPoints(&h, func, weight, a, b, n, &prec);
    else {
      radiusBasicMinimaxChebychevsPoints(&h, func, weight, a, b, bound, &prec);
      break;
    }
    if(verbosity>=4) {
      changeToWarningMode();
      sollyaPrintf("Information: guessdegree: trying degree %d. Found radius: ",n-1);
      printMpfr(h);
      restoreMode();
    }
  }

  if (mpfr_cmp(h,eps) >=0) { /* Even n=bound does not achieve the discrete
                                problem, a fortiori it does not achieve the
                                continuous problem. Return [bound+1, +Inf]
                             */
    printMessage(1, "Warning: guessdegree: none of the degrees smaller than %d satisfies the required error.\n", bound-1);
    mpfr_clear(h);
    tempMpfr = (mpfr_t *)safeMalloc(sizeof(mpfr_t));
    mpfr_init2(*tempMpfr,128);
    mpfr_set_ui(*tempMpfr, bound, GMP_RNDN); /* n=bound+1 converts to degree bound */
    range.a = tempMpfr;

    tempMpfr = (mpfr_t *)safeMalloc(sizeof(mpfr_t));
    mpfr_init2(*tempMpfr,128);
    mpfr_set_inf(*tempMpfr, 1);
    range.b = tempMpfr;

    return range;
  }

  /* Now, the basic minimax problem achieves eps for min(n,bound) but not
     for n/2.
     We use a bisection to obtain a thinner bound.
     Note that if n=1 achieves the bound eps, we have nothing to do */
  if(n!=1) {
    n_min = n/2;
    n_max = (n<=bound)?n:bound;

    n = (n_min + n_max)/2;

    while(n != n_min) {
      radiusBasicMinimaxChebychevsPoints(&h, func, weight, a, b, n, &prec);
      if(verbosity>=4) {
	changeToWarningMode();
	sollyaPrintf("Information: guessdegree: trying degree %d (current bounds: [%d, %d]). Found radius: ",n-1,n_min-1,n_max-1);
	printMpfr(h);
	restoreMode();
      }
      if(mpfr_cmp(h,eps) >= 0) n_min = n;
      else n_max = n;

      n = (n_min + n_max)/2;
    }
  }
  else n_max = 1;
  popTimeCounter("finding a lower bound for guessdegree");


  /* Now n_min = n = n_max - 1
     What we know for sure is: n_min is not sufficient to achieve
     the basic minimax problem. A fortiori, it is not sufficient for
     the continuous minimax problem.
     n_max is a possible candidate for the continuous minimax problem */
  n = n_max;

  pushTimeCounter();
  firstStepContinuousMinimaxChebychevsPoints(&h, func, weight, a, b, n, &prec);
  if(verbosity>=4) {
    changeToWarningMode();
    sollyaPrintf("Information: guessdegree: trying degree %d. Found infnorm: ",n-1);
    printMpfr(h);
    restoreMode();
  }

  while(mpfr_cmp(h,eps) > 0) {
    n++;
    if (n>bound) break;
    firstStepContinuousMinimaxChebychevsPoints(&h, func, weight, a, b, n, &prec);
    if(verbosity>=4) {
      changeToWarningMode();
      sollyaPrintf("Information: guessdegree: trying degree %d. Found infnorm: ",n-1);
      printMpfr(h);
      restoreMode();
    }
  }
  popTimeCounter("finding an upper bound for guessdegree");

  /* if (n>bound), it is possible that n_max suffices, but we did not manage to
     prove that n suffices for any n_max<=n<=bound. So we return [n_max, +Inf].
     Otherwise, we are sure that n is sufficient to achieve eps in the
     continuous problem. We return [n_max, n];
  */
  if (n>bound)
    printMessage(2, "Warning: guessdegree: we did not find a degree less than %d for which we can prove that the errror is satisfied.\n", bound-1);

  mpfr_clear(h);
  tempMpfr = (mpfr_t *)safeMalloc(sizeof(mpfr_t));
  mpfr_init2(*tempMpfr,128);
  mpfr_set_ui(*tempMpfr, n_max-1, GMP_RNDN);
  range.a = tempMpfr;

  tempMpfr = (mpfr_t *)safeMalloc(sizeof(mpfr_t));
  mpfr_init2(*tempMpfr,128);
  if (n<=bound) mpfr_set_ui(*tempMpfr, n-1, GMP_RNDN);
  else mpfr_set_inf(*tempMpfr, 1);
  range.b = tempMpfr;

  return range;
}
