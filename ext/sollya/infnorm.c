/*

Copyright 2006-2011 by

Laboratoire de l'Informatique du Parallelisme,
UMR CNRS - ENS Lyon - UCB Lyon 1 - INRIA 5668,

LORIA (CNRS, INPL, INRIA, UHP, U-Nancy 2),

Laboratoire d'Informatique de Paris 6, equipe PEQUAN,
UPMC Universite Paris 06 - CNRS - UMR 7606 - LIP6, Paris, France

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

#include <mpfr.h>
#include "mpfi-compat.h"
#include "expression.h"
#include "infnorm.h"
#include "chain.h"
#include "double.h"
#include "general.h"
#include "proof.h"
#include "remez.h"
#include "execute.h"

#include <stdio.h> /* fprintf, fopen, fclose, */
#include <stdlib.h> /* exit, free, mktemp */
#include <errno.h>

#define DEBUG 0
#define DEBUGMPFI 0
#define DIFFSIZE 5000000


void printInterval(sollya_mpfi_t interval);

int sollya_mpfi_equal_p(sollya_mpfi_t r1, sollya_mpfi_t r2) {
  mpfr_t x1, x2;
  int test = 0;
  mpfr_init2(x1, sollya_mpfi_get_prec(r1));
  mpfr_init2(x2, sollya_mpfi_get_prec(r2));

  sollya_mpfi_get_left(x1, r1); sollya_mpfi_get_left(x2, r2); test = mpfr_equal_p(x1, x2);
  sollya_mpfi_get_right(x1, r1); sollya_mpfi_get_right(x2, r2); test = test && mpfr_equal_p(x1, x2);
  
  mpfr_clear(x1); mpfr_clear(x2);
  return test;
}

int sollya_mpfr_max(mpfr_t z, mpfr_t x, mpfr_t y, mp_rnd_t rnd) {
  int res = 2;
  if (mpfr_nan_p(x) || mpfr_nan_p(y)) mpfr_set_nan(z);
  else res = mpfr_max(z, x, y, rnd);
  return res;
}

int sollya_mpfr_min(mpfr_t z, mpfr_t x, mpfr_t y, mp_rnd_t rnd) {
  int res = 2;
  if (mpfr_nan_p(x) || mpfr_nan_p(y)) mpfr_set_nan(z);
  else res = mpfr_min(z, x, y, rnd);
  return res;
}

void sollya_mpfi_pow(sollya_mpfi_t z, sollya_mpfi_t x, sollya_mpfi_t y) {
  mpfr_t l,r,lx,rx;
  mp_prec_t prec, precx;
  int must_divide;
  sollya_mpfi_t res;

  if (sollya_mpfi_has_nan(x) ||sollya_mpfi_has_nan(y)) { sollya_mpfi_set_nan(z); return; }
    if (sollya_mpfi_is_empty(x) || sollya_mpfi_is_empty(y)) { sollya_mpfi_set_empty(z); return; }

  prec = sollya_mpfi_get_prec(y);
  mpfr_init2(l,prec); sollya_mpfi_get_left(l,y);
  mpfr_init2(r,prec); sollya_mpfi_get_right(r,y);

  sollya_mpfi_init2(res,sollya_mpfi_get_prec(z) + 10);

  /* Case x^k, k an integer */
  if ((mpfr_cmp(l,r) == 0) && (mpfr_integer_p(l))) {
    if (mpfr_zero_p(l)) { /* Case k=0 -> 1 except if x=+/-Inf */
                          /* Note, if x contains an infinity, but is not equal to infinity,
                             we return 1 also, by continuity */
      if (sollya_mpfi_is_infinity(x)) sollya_mpfi_set_nan(z);
      else sollya_mpfi_set_d(z,1.0);
    
      mpfr_clear(l); mpfr_clear(r); sollya_mpfi_clear(res);
      return;
    } else {
      precx = sollya_mpfi_get_prec(x);
      if (sollya_mpfi_get_prec(res) > precx) 
        precx = sollya_mpfi_get_prec(res);

      mpfr_init2(lx,precx);
      mpfr_init2(rx,precx);
      
      sollya_mpfi_get_right(rx,x);
      sollya_mpfi_get_left(lx,x);
     
      if (mpfr_sgn(l) < 0) {
	must_divide = 1;
	mpfr_neg(l,l,GMP_RNDN);
      } else {
	must_divide = 0;
      }
      
      mpfr_div_2ui(r,l,1,GMP_RNDN);
      if (sollya_mpfi_is_nonneg(x) || (!mpfr_integer_p(r))) { /* x-> x^k is increasing monotonic when x>=0
                                                               or when k is odd */
        mpfr_pow(lx,lx,l,GMP_RNDD);
        mpfr_pow(rx,rx,l,GMP_RNDU);
        sollya_mpfi_interv_fr(res,lx,rx);
      }
      else if (sollya_mpfi_is_nonpos(x)) { /* x^k is decreasing when x<=0 and k is even */
        mpfr_pow(rx,rx,l,GMP_RNDD);
        mpfr_pow(lx,lx,l,GMP_RNDU);
        sollya_mpfi_interv_fr(res,rx,lx);
      }
      else { /* when x contains 0 and k is even, return [0, max(lx^k, rx^k)] */
        mpfr_pow(lx,lx,l,GMP_RNDU);
        mpfr_pow(rx,rx,l,GMP_RNDU);
        sollya_mpfr_max(rx,lx,rx,GMP_RNDU);
        mpfr_set_d(lx,0.0,GMP_RNDD);
        sollya_mpfi_interv_fr(res,lx,rx);
      }

      if (must_divide) sollya_mpfi_inv(res,res);

      mpfr_clear(lx);
      mpfr_clear(rx);
    }
  } else {
    sollya_mpfi_log(res,x);
    sollya_mpfi_mul(res,res,y);
    sollya_mpfi_exp(res,res);
  }
  mpfr_clear(l);
  mpfr_clear(r);
  sollya_mpfi_set(z,res);
  sollya_mpfi_clear(res);
}


void sollya_mpfi_round_to_double(sollya_mpfi_t rop, sollya_mpfi_t op) {
  mpfr_t l,r, lres, rres;
  mp_prec_t prec;

  prec = sollya_mpfi_get_prec(op) + 10;
  mpfr_init2(l,prec);
  mpfr_init2(r,prec);
  mpfr_init2(lres,prec);
  mpfr_init2(rres,prec);

  sollya_mpfi_get_left(l,op);
  sollya_mpfi_get_right(r,op);

  mpfr_round_to_double(lres,l);
  mpfr_round_to_double(rres,r);

  sollya_mpfi_interv_fr(rop,lres,rres);

  mpfr_clear(l);
  mpfr_clear(r);
  mpfr_clear(lres);
  mpfr_clear(rres);
}


void sollya_mpfi_round_to_single(sollya_mpfi_t rop, sollya_mpfi_t op) {
  mpfr_t l,r, lres, rres;
  mp_prec_t prec;

  prec = sollya_mpfi_get_prec(op) + 10;
  mpfr_init2(l,prec);
  mpfr_init2(r,prec);
  mpfr_init2(lres,prec);
  mpfr_init2(rres,prec);

  sollya_mpfi_get_left(l,op);
  sollya_mpfi_get_right(r,op);

  mpfr_round_to_single(lres,l);
  mpfr_round_to_single(rres,r);

  sollya_mpfi_interv_fr(rop,lres,rres);

  mpfr_clear(l);
  mpfr_clear(r);
  mpfr_clear(lres);
  mpfr_clear(rres);
}

void sollya_mpfi_round_to_quad(sollya_mpfi_t rop, sollya_mpfi_t op) {
  mpfr_t l,r, lres, rres;
  mp_prec_t prec;

  prec = sollya_mpfi_get_prec(op) + 10;
  mpfr_init2(l,prec);
  mpfr_init2(r,prec);
  mpfr_init2(lres,prec);
  mpfr_init2(rres,prec);

  sollya_mpfi_get_left(l,op);
  sollya_mpfi_get_right(r,op);

  mpfr_round_to_quad(lres,l);
  mpfr_round_to_quad(rres,r);

  sollya_mpfi_interv_fr(rop,lres,rres);

  mpfr_clear(l);
  mpfr_clear(r);
  mpfr_clear(lres);
  mpfr_clear(rres);
}

void sollya_mpfi_round_to_halfprecision(sollya_mpfi_t rop, sollya_mpfi_t op) {
  mpfr_t l,r, lres, rres;
  mp_prec_t prec;

  prec = sollya_mpfi_get_prec(op) + 10;
  mpfr_init2(l,prec);
  mpfr_init2(r,prec);
  mpfr_init2(lres,prec);
  mpfr_init2(rres,prec);

  sollya_mpfi_get_left(l,op);
  sollya_mpfi_get_right(r,op);

  mpfr_round_to_halfprecision(lres,l);
  mpfr_round_to_halfprecision(rres,r);

  sollya_mpfi_interv_fr(rop,lres,rres);

  mpfr_clear(l);
  mpfr_clear(r);
  mpfr_clear(lres);
  mpfr_clear(rres);
}

void sollya_mpfi_round_to_doubledouble(sollya_mpfi_t rop, sollya_mpfi_t op) {
  mpfr_t l,r, lres, rres;
  mp_prec_t prec;

  prec = sollya_mpfi_get_prec(op) + 10;
  mpfr_init2(l,prec);
  mpfr_init2(r,prec);
  mpfr_init2(lres,prec);
  mpfr_init2(rres,prec);

  sollya_mpfi_get_left(l,op);
  sollya_mpfi_get_right(r,op);

  mpfr_round_to_doubledouble(lres,l);
  mpfr_round_to_doubledouble(rres,r);

  sollya_mpfi_interv_fr(rop,lres,rres);

  mpfr_clear(l);
  mpfr_clear(r);
  mpfr_clear(lres);
  mpfr_clear(rres);
}

void sollya_mpfi_round_to_tripledouble(sollya_mpfi_t rop, sollya_mpfi_t op) {
  mpfr_t l,r, lres, rres;
  mp_prec_t prec;

  prec = sollya_mpfi_get_prec(op) + 10;
  mpfr_init2(l,prec);
  mpfr_init2(r,prec);
  mpfr_init2(lres,prec);
  mpfr_init2(rres,prec);

  sollya_mpfi_get_left(l,op);
  sollya_mpfi_get_right(r,op);

  mpfr_round_to_tripledouble(lres,l);
  mpfr_round_to_tripledouble(rres,r);

  sollya_mpfi_interv_fr(rop,lres,rres);

  mpfr_clear(l);
  mpfr_clear(r);
  mpfr_clear(lres);
  mpfr_clear(rres);
}

void sollya_mpfi_round_to_doubleextended(sollya_mpfi_t rop, sollya_mpfi_t op) {
  mpfr_t l,r, lres, rres;
  mp_prec_t prec;

  prec = sollya_mpfi_get_prec(op) + 10;
  mpfr_init2(l,prec);
  mpfr_init2(r,prec);
  mpfr_init2(lres,prec);
  mpfr_init2(rres,prec);

  sollya_mpfi_get_left(l,op);
  sollya_mpfi_get_right(r,op);

  mpfr_round_to_doubleextended(lres,l);
  mpfr_round_to_doubleextended(rres,r);

  sollya_mpfi_interv_fr(rop,lres,rres);

  mpfr_clear(l);
  mpfr_clear(r);
  mpfr_clear(lres);
  mpfr_clear(rres);
}


void sollya_mpfi_erf(sollya_mpfi_t rop, sollya_mpfi_t op) {
  mpfr_t opl, opr, ropl, ropr;

  mpfr_init2(opl,sollya_mpfi_get_prec(op));
  mpfr_init2(opr,sollya_mpfi_get_prec(op));

  mpfr_init2(ropl,sollya_mpfi_get_prec(rop));
  mpfr_init2(ropr,sollya_mpfi_get_prec(rop));
  
  sollya_mpfi_get_left(opl,op);
  sollya_mpfi_get_right(opr,op);
  
  mpfr_erf(ropl,opl,GMP_RNDD);
  mpfr_erf(ropr,opr,GMP_RNDU);

  sollya_mpfi_interv_fr(rop,ropl,ropr);

  mpfr_clear(opl);
  mpfr_clear(opr);
  mpfr_clear(ropl);
  mpfr_clear(ropr);
}

void sollya_mpfi_erfc(sollya_mpfi_t rop, sollya_mpfi_t op) {
  mpfr_t opl, opr, ropl, ropr;

  mpfr_init2(opl,sollya_mpfi_get_prec(op));
  mpfr_init2(opr,sollya_mpfi_get_prec(op));

  mpfr_init2(ropl,sollya_mpfi_get_prec(rop));
  mpfr_init2(ropr,sollya_mpfi_get_prec(rop));
  
  sollya_mpfi_get_left(opl,op);
  sollya_mpfi_get_right(opr,op);
  
  mpfr_erfc(ropl,opr,GMP_RNDD);
  mpfr_erfc(ropr,opl,GMP_RNDU);

  sollya_mpfi_interv_fr(rop,ropl,ropr);

  mpfr_clear(opl);
  mpfr_clear(opr);
  mpfr_clear(ropl);
  mpfr_clear(ropr);
}

void sollya_mpfi_ceil(sollya_mpfi_t rop, sollya_mpfi_t op) {
  mpfr_t opl, opr, ropl, ropr;

  mpfr_init2(opl,sollya_mpfi_get_prec(op));
  mpfr_init2(opr,sollya_mpfi_get_prec(op));

  mpfr_init2(ropl,sollya_mpfi_get_prec(op));
  mpfr_init2(ropr,sollya_mpfi_get_prec(op));
  
  sollya_mpfi_get_left(opl,op);
  sollya_mpfi_get_right(opr,op);
  
  mpfr_ceil(ropl,opr);
  mpfr_ceil(ropr,opl);

  sollya_mpfi_interv_fr(rop,ropl,ropr);

  mpfr_clear(opl);
  mpfr_clear(opr);
  mpfr_clear(ropl);
  mpfr_clear(ropr);
}

void sollya_mpfi_floor(sollya_mpfi_t rop, sollya_mpfi_t op) {
  mpfr_t opl, opr, ropl, ropr;

  mpfr_init2(opl,sollya_mpfi_get_prec(op));
  mpfr_init2(opr,sollya_mpfi_get_prec(op));

  mpfr_init2(ropl,sollya_mpfi_get_prec(op));
  mpfr_init2(ropr,sollya_mpfi_get_prec(op));
  
  sollya_mpfi_get_left(opl,op);
  sollya_mpfi_get_right(opr,op);
  
  mpfr_floor(ropl,opr);
  mpfr_floor(ropr,opl);

  sollya_mpfi_interv_fr(rop,ropl,ropr);

  mpfr_clear(opl);
  mpfr_clear(opr);
  mpfr_clear(ropl);
  mpfr_clear(ropr);
}

void sollya_mpfi_nearestint(sollya_mpfi_t rop, sollya_mpfi_t op) {
  mpfr_t opl, opr, ropl, ropr;

  mpfr_init2(opl,sollya_mpfi_get_prec(op));
  mpfr_init2(opr,sollya_mpfi_get_prec(op));

  mpfr_init2(ropl,sollya_mpfi_get_prec(op));
  mpfr_init2(ropr,sollya_mpfi_get_prec(op));
  
  sollya_mpfi_get_left(opl,op);
  sollya_mpfi_get_right(opr,op);
  
  mpfr_nearestint(ropl,opr);
  mpfr_nearestint(ropr,opl);

  sollya_mpfi_interv_fr(rop,ropl,ropr);

  mpfr_clear(opl);
  mpfr_clear(opr);
  mpfr_clear(ropl);
  mpfr_clear(ropr);
}

/* Evaluate a library constant function into an interval */
void libraryConstantToInterval(sollya_mpfi_t res, node *tree) {
  mpfr_t approx, lbound, rbound;
  mp_prec_t prec = sollya_mpfi_get_prec(res);

  mpfr_init2(approx, prec + 20); /* some guard bits may avoid reinit in tree->libFun */
  tree->libFun->constant_code(approx, prec);
  mpfr_init2(lbound, prec-2);
  mpfr_init2(rbound, prec-2);
  mpfr_set(lbound, approx, GMP_RNDD);
  mpfr_set(rbound, approx, GMP_RNDU);
  mpfr_nextbelow(lbound);
  mpfr_nextabove(rbound);
  
  sollya_mpfi_interv_fr(res, lbound, rbound);
  return;
}

int newtonMPFRWithStartPoint(mpfr_t res, node *tree, node *diff_tree, mpfr_t a, mpfr_t b, mpfr_t start, mp_prec_t prec) {
  mpfr_t x, x2, temp1, temp2, am, bm;
  unsigned long int n=1;
  int okay, lucky, hasZero, i, freeTrees;
  node *myTree, *myDiffTree;

  freeTrees = 0;
  if (tree->nodeType == DIV) {
    freeTrees = 1;
    myTree = copyTree(tree->child1);
    myDiffTree = differentiate(myTree);
  } else {
    myTree = tree;
    myDiffTree = diff_tree;
  }

  mpfr_init2(x,prec);
  mpfr_init2(x2,prec);
  mpfr_init2(temp1,prec);
  mpfr_init2(temp2,prec);
  mpfr_init2(am,prec/2);
  mpfr_init2(bm,prec/2);
  mpfr_set(am,a,GMP_RNDN);
  mpfr_nextbelow(am);
  mpfr_nextbelow(am);
  mpfr_set(bm,b,GMP_RNDN);
  mpfr_nextabove(am);
  mpfr_nextabove(bm);

  okay = 0;

  if (mpfr_sgn(a) != mpfr_sgn(b)) {
    mpfr_set_d(x,0.0,GMP_RNDN);
    evaluate(temp1, myTree, x, prec);
    if (mpfr_zero_p(temp1)) {
      mpfr_set(res,x,GMP_RNDN);
      okay = 1;
    }
  }

  if (!okay) {
    evaluate(temp1, myTree, a, prec);
    if (mpfr_zero_p(temp1)) {
      mpfr_set(res,a,GMP_RNDN);
      okay = 1;
    } else {
      evaluate(temp2, myTree, b, prec);
      if (mpfr_zero_p(temp2)) {
	mpfr_set(res,b,GMP_RNDN);
	okay = 1;
      } else {
	
	mpfr_mul(temp1,temp1,temp2,GMP_RNDN);
	hasZero = (mpfr_sgn(temp1) <= 0);

	mpfr_set(x,start,GMP_RNDN);
	lucky = 0;
	
	i = 5000;
	while((n<=prec+25) && (mpfr_cmp(am,x) <= 0) && (mpfr_cmp(x,bm) <= 0) && (i > 0)) {
	  evaluate(temp1, myTree, x, prec);
	  if (mpfr_zero_p(temp1)) {
	    lucky = 1;
	    break;
	  }
	  evaluate(temp2, myDiffTree, x, prec);
	  mpfr_div(temp1, temp1, temp2, GMP_RNDN);
	  mpfr_sub(x2, x, temp1, GMP_RNDN);
	  if (mpfr_cmp(x2,x) == 0) break;
	  if (mpfr_zero_p(x) || mpfr_zero_p(x2)) {
	    n *= 2;
	  } else {
	    if (mpfr_get_exp(x) == mpfr_get_exp(x2)) {
	      n *= 2;
	    } else {
	      i--;
	    }
	  }
	  mpfr_set(x,x2,GMP_RNDN);
	}
	
	if (mpfr_cmp(x,a) < 0) {
	  mpfr_set(res,a,GMP_RNDN);
	  if (hasZero) {
	    okay = 1;
	  } else {
	    evaluate(temp1, myTree, x, prec);
	    evaluate(temp2, myDiffTree, x, prec);
	    mpfr_div(temp1, temp1, temp2, GMP_RNDN);
	    mpfr_sub(x, x, temp1, GMP_RNDN);
	    if (mpfr_cmp(x,a) >= 0) {
	      okay = 1;
	    } else {
	      okay = 0;
	    }
	  }
	} else {
	  if (mpfr_cmp(b,x) < 0) {
	    mpfr_set(res,b,GMP_RNDN);
	    if (hasZero) {
	      okay = 1;
	    } else {
	      evaluate(temp1, myTree, x, prec);
	      evaluate(temp2, myDiffTree, x, prec);
	      mpfr_div(temp1, temp1, temp2, GMP_RNDN);
	      mpfr_sub(x, x, temp1, GMP_RNDN);
	      if (mpfr_cmp(b,x) >= 0) {
		okay = 1;
	      } else {
		okay = 0;
	      }
	    }
	  } else {
	    mpfr_set(res,x,GMP_RNDN);
	    if (!lucky) {
	      evaluate(temp1, myTree, x, prec);
	      evaluate(temp2, myDiffTree, x, prec);
	      mpfr_div(temp1, temp1, temp2, GMP_RNDN);
	      mpfr_abs(temp1,temp1,GMP_RNDN);
	      mpfr_abs(x,x,GMP_RNDN);
	      mpfr_div_ui(x,x,1,GMP_RNDN);
	      okay = (mpfr_cmp(temp1,x) <= 0);
	    } else {
	      okay = 1;
	    }
	  }
	}
      }
    } 
  }

  if (freeTrees) {
    free_memory(myTree);
    free_memory(myDiffTree);
  }

  mpfr_clear(x); mpfr_clear(temp1); mpfr_clear(temp2); mpfr_clear(x2); mpfr_clear(am); mpfr_clear(bm);
  return okay;
}

int newtonMPFRWithStartPointFaithful(mpfr_t res, node *tree, node *diff_tree, mpfr_t a, mpfr_t b, mpfr_t start, mp_prec_t prec) {
  mpfr_t x, x2, temp1, temp2, am, bm;
  unsigned long int n=1;
  int okay, lucky, hasZero, i, freeTrees;
  node *myTree, *myDiffTree;

  freeTrees = 0;
  if (tree->nodeType == DIV) {
    freeTrees = 1;
    myTree = copyTree(tree->child1);
    myDiffTree = differentiate(myTree);
  } else {
    myTree = tree;
    myDiffTree = diff_tree;
  }

  mpfr_init2(x,prec);
  mpfr_init2(x2,prec);
  mpfr_init2(temp1,prec);
  mpfr_init2(temp2,prec);
  mpfr_init2(am,prec/2);
  mpfr_init2(bm,prec/2);
  mpfr_set(am,a,GMP_RNDN);
  mpfr_nextbelow(am);
  mpfr_nextbelow(am);
  mpfr_set(bm,b,GMP_RNDN);
  mpfr_nextabove(am);
  mpfr_nextabove(bm);

  okay = 0;

  if (mpfr_sgn(a) != mpfr_sgn(b)) {
    mpfr_set_d(x,0.0,GMP_RNDN);
    evaluateFaithful(temp1, myTree, x, prec);
    if (mpfr_zero_p(temp1)) {
      mpfr_set(res,x,GMP_RNDN);
      okay = 1;
    }
  }

  if (!okay) {
    evaluateFaithful(temp1, myTree, a, prec);
    if (mpfr_zero_p(temp1)) {
      mpfr_set(res,a,GMP_RNDN);
      okay = 1;
    } else {
      evaluateFaithful(temp2, myTree, b, prec);
      if (mpfr_zero_p(temp2)) {
	mpfr_set(res,b,GMP_RNDN);
	okay = 1;
      } else {
	
	mpfr_mul(temp1,temp1,temp2,GMP_RNDN);
	hasZero = (mpfr_sgn(temp1) <= 0);

	mpfr_set(x,start,GMP_RNDN);
	lucky = 0;
	
	i = 5000;
	while((n<=prec+25) && (mpfr_cmp(am,x) <= 0) && (mpfr_cmp(x,bm) <= 0) && (i > 0)) {
	  evaluateFaithful(temp1, myTree, x, prec);
	  if (mpfr_zero_p(temp1)) {
	    lucky = 1;
	    break;
	  }
	  evaluate(temp2, myDiffTree, x, prec);
	  mpfr_div(temp1, temp1, temp2, GMP_RNDN);
	  mpfr_sub(x2, x, temp1, GMP_RNDN);
	  if (mpfr_cmp(x2,x) == 0) break;
	  if (mpfr_zero_p(x) || mpfr_zero_p(x2)) {
	    n *= 2;
	  } else {
	    if (mpfr_get_exp(x) == mpfr_get_exp(x2)) {
	      n *= 2;
	    } else {
	      i--;
	    }
	  }
	  mpfr_set(x,x2,GMP_RNDN);
	}
	
	if (mpfr_cmp(x,a) < 0) {
	  mpfr_set(res,a,GMP_RNDN);
	  if (hasZero) {
	    okay = 1;
	  } else {
	    evaluateFaithful(temp1, myTree, x, prec);
	    evaluateFaithful(temp2, myDiffTree, x, prec);
	    mpfr_div(temp1, temp1, temp2, GMP_RNDN);
	    mpfr_sub(x, x, temp1, GMP_RNDN);
	    if (mpfr_cmp(x,a) >= 0) {
	      okay = 1;
	    } else {
	      okay = 0;
	    }
	  }
	} else {
	  if (mpfr_cmp(b,x) < 0) {
	    mpfr_set(res,b,GMP_RNDN);
	    if (hasZero) {
	      okay = 1;
	    } else {
	      evaluateFaithful(temp1, myTree, x, prec);
	      evaluateFaithful(temp2, myDiffTree, x, prec);
	      mpfr_div(temp1, temp1, temp2, GMP_RNDN);
	      mpfr_sub(x, x, temp1, GMP_RNDN);
	      if (mpfr_cmp(b,x) >= 0) {
		okay = 1;
	      } else {
		okay = 0;
	      }
	    }
	  } else {
	    mpfr_set(res,x,GMP_RNDN);
	    if (!lucky) {
	      evaluateFaithful(temp1, myTree, x, prec);
	      evaluateFaithful(temp2, myDiffTree, x, prec);
	      mpfr_div(temp1, temp1, temp2, GMP_RNDN);
	      mpfr_abs(temp1,temp1,GMP_RNDN);
	      mpfr_abs(x,x,GMP_RNDN);
	      mpfr_div_ui(x,x,1,GMP_RNDN);
	      okay = (mpfr_cmp(temp1,x) <= 0);
	    } else {
	      okay = 1;
	    }
	  }
	}
      }
    } 
  }

  if (freeTrees) {
    free_memory(myTree);
    free_memory(myDiffTree);
  }

  mpfr_clear(x); mpfr_clear(temp1); mpfr_clear(temp2); mpfr_clear(x2); mpfr_clear(am); mpfr_clear(bm);
  return okay;
}

int newtonMPFR(mpfr_t res, node *tree, node *diff_tree, mpfr_t a, mpfr_t b, mp_prec_t prec) {
  mpfr_t start;
  int result;

  mpfr_init2(start,prec);
  mpfr_add(start,a,b,GMP_RNDN);
  mpfr_div_2ui(start,start,1,GMP_RNDN);

  result = newtonMPFRWithStartPoint(res, tree, diff_tree, a, b, start, prec);

  mpfr_clear(start);

  return result;
}

int newtonMPFRFaithful(mpfr_t res, node *tree, node *diff_tree, mpfr_t a, mpfr_t b, mp_prec_t prec) {
  mpfr_t start;
  int result;

  mpfr_init2(start,prec);
  mpfr_add(start,a,b,GMP_RNDN);
  mpfr_div_2ui(start,start,1,GMP_RNDN);

  result = newtonMPFRWithStartPointFaithful(res, tree, diff_tree, a, b, start, prec);

  mpfr_clear(start);

  return result;
}


void makeMpfiAroundMpfr(sollya_mpfi_t res, mpfr_t x, unsigned int thousandUlps) {
  mpfr_t xp, xs;
  mp_prec_t prec;
  sollya_mpfi_t xI;


  prec = mpfr_get_prec(x);
  mpfr_init2(xp,prec);
  mpfr_init2(xs,prec);
  sollya_mpfi_init2(xI,prec);
  
  mpfr_set(xp,x,GMP_RNDD);
  mpfr_set(xs,x,GMP_RNDU);

  mpfr_nextbelow(xp);
  mpfr_nextabove(xs);

  sollya_mpfi_interv_fr(xI,xp,xs);
  
  sollya_mpfi_blow(xI,xI,(((double) thousandUlps) * 250.0));

  sollya_mpfi_set(res,xI);
  
  sollya_mpfi_clear(xI);
  mpfr_clear(xp);
  mpfr_clear(xs);
}



chain* evaluateI(sollya_mpfi_t result, node *tree, sollya_mpfi_t x, mp_prec_t prec, int simplifiesA, int simplifiesB, mpfr_t *hopitalPoint, exprBoundTheo *theo, int noExcludes) {
  sollya_mpfi_t stack1, stack2;
  sollya_mpfi_t stack3, zI, numeratorInZI, denominatorInZI, newExcludeTemp, xMXZ, temp1, temp2, tempA, tempB;
  sollya_mpfi_t *newExclude;
  sollya_mpfi_t leftConstantTerm, rightConstantTerm;
  sollya_mpfi_t leftLinearTerm, rightLinearTerm;
  mpfr_t al, ar, bl, br, xl, xr, z, z2;
  mpfr_t *newHopitalPoint;
  node *derivNumerator, *derivDenominator, *tempNode;
  node *derivLeft, *derivRight;
  chain *leftExcludes, *rightExcludes, *excludes, *t1, *t2;
  chain *leftExcludesConstant, *rightExcludesConstant;
  chain *leftExcludesLinear, *rightExcludesLinear;
  exprBoundTheo *leftTheo, *rightTheo, *internalTheo; 
  exprBoundTheo *leftTheoConstant, *rightTheoConstant, *leftTheoLinear, *rightTheoLinear;
  int isPolynom;
  int xIsPoint;

  excludes = NULL;

  if (theo != NULL) nullifyExprBoundTheo(theo);

  isPolynom = 0;
  internalTheo = NULL;
  if (theo != NULL) {
    isPolynom = isPolynomial(tree);
    
    if (isPolynom) {
      theo->functionType = POLYNOMIAL;
      leftTheo = NULL;
      rightTheo = NULL;
    } else {
      internalTheo = theo;
      internalTheo->functionType = tree->nodeType;
      switch (arity(tree)) {
      case 1:
	leftTheo = (exprBoundTheo *) safeCalloc(1,sizeof(exprBoundTheo));
	rightTheo = NULL;
	internalTheo->boundLeft = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
	sollya_mpfi_init2(*(internalTheo->boundLeft),prec);
	break;
      case 2:
	leftTheo = (exprBoundTheo *) safeCalloc(1,sizeof(exprBoundTheo));
	rightTheo = (exprBoundTheo *) safeCalloc(1,sizeof(exprBoundTheo));
	internalTheo->boundLeft = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
	sollya_mpfi_init2(*(internalTheo->boundLeft),prec);
	internalTheo->boundRight = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
	sollya_mpfi_init2(*(internalTheo->boundRight),prec);
	break;
      default:
	leftTheo = NULL;
	rightTheo = NULL;
	break;
      }
      internalTheo->theoLeft = leftTheo;
      internalTheo->theoRight = rightTheo;
    }

    theo->x = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
    sollya_mpfi_init2(*(theo->x),sollya_mpfi_get_prec(x));
    sollya_mpfi_set(*(theo->x),x);
    theo->function = copyTree(tree);
  } else {
    leftTheo = NULL;
    rightTheo = NULL;
  }


  sollya_mpfi_init2(stack1, prec);
  sollya_mpfi_init2(stack2, prec);
  sollya_mpfi_init2(stack3, prec);
  mpfr_init2(al,prec);
  mpfr_init2(ar,prec);
  mpfr_init2(bl,prec);
  mpfr_init2(br,prec);

  sollya_mpfi_diam_abs(al,x);

  if (mpfr_zero_p(al)) xIsPoint = 1; else xIsPoint = 0;

  if (xIsPoint) printMessage(12,"Information: while evaluating no decorrelation test will be performed because the ordinate interval is point.\n");

  switch (tree->nodeType) {
  case VARIABLE:
    sollya_mpfi_set(stack3,x);
    excludes = NULL;
    break;
  case CONSTANT:
    sollya_mpfi_set_fr(stack3,*(tree->value));
    excludes = NULL;
    break;
  case ADD:
    leftExcludes = evaluateI(stack1, tree->child1, x, prec, simplifiesA, simplifiesB, NULL, leftTheo,noExcludes);
    rightExcludes = evaluateI(stack2, tree->child2, x, prec, simplifiesA, simplifiesB, NULL, rightTheo,noExcludes);
    sollya_mpfi_add(stack3, stack1, stack2);
    if (internalTheo != NULL) {
      sollya_mpfi_set(*(internalTheo->boundLeft),stack1);
      sollya_mpfi_set(*(internalTheo->boundRight),stack2);
    }
    if ((simplifiesA > 0) && (sollya_mpfi_has_zero(stack3)) && (!sollya_mpfi_has_zero(stack1)) && (!sollya_mpfi_has_zero(stack2)) && !xIsPoint) {

      if (internalTheo != NULL) {
	internalTheo->simplificationUsed = DECORRELATE;
	leftTheoConstant = (exprBoundTheo *) safeCalloc(1,sizeof(exprBoundTheo));
	nullifyExprBoundTheo(leftTheoConstant);
	rightTheoConstant = (exprBoundTheo *) safeCalloc(1,sizeof(exprBoundTheo));
	nullifyExprBoundTheo(rightTheoConstant);
	leftTheoLinear = (exprBoundTheo *) safeCalloc(1,sizeof(exprBoundTheo));
	nullifyExprBoundTheo(leftTheoLinear);
	rightTheoLinear = (exprBoundTheo *) safeCalloc(1,sizeof(exprBoundTheo));
	nullifyExprBoundTheo(rightTheoLinear);
	internalTheo->theoLeftConstant = leftTheoConstant;
	internalTheo->theoRightConstant = rightTheoConstant;
	internalTheo->theoLeftLinear = leftTheoLinear;
	internalTheo->theoRightLinear = rightTheoLinear;
      } else {
	leftTheoConstant = NULL;
	rightTheoConstant = NULL;
	leftTheoLinear = NULL;
	rightTheoLinear = NULL;
      }

      mpfr_init2(z,prec);
      sollya_mpfi_init2(zI,prec);
      sollya_mpfi_init2(leftConstantTerm,prec);
      sollya_mpfi_init2(rightConstantTerm,prec);
      sollya_mpfi_init2(leftLinearTerm,prec);
      sollya_mpfi_init2(rightLinearTerm,prec);
      sollya_mpfi_init2(xMXZ,prec);
      sollya_mpfi_init2(temp1,prec);
      sollya_mpfi_init2(temp2,prec);
      sollya_mpfi_init2(tempA,prec);
      sollya_mpfi_init2(tempB,prec);

      sollya_mpfi_mid(z,x);
      sollya_mpfi_set_fr(zI,z);

      leftExcludesConstant = evaluateI(leftConstantTerm, tree->child1, zI, prec, simplifiesA-1, simplifiesB, NULL, leftTheoConstant,noExcludes);
      rightExcludesConstant = evaluateI(rightConstantTerm, tree->child2, zI, prec, simplifiesA-1, simplifiesB, NULL, rightTheoConstant,noExcludes);

      printMessage(12,"Information: Differentiating while evaluating for decorrelation.\n");

      derivLeft = differentiate(tree->child1);
      derivRight = differentiate(tree->child2);

      leftExcludesLinear = evaluateI(leftLinearTerm, derivLeft, x, prec, simplifiesA-1, simplifiesB, NULL, leftTheoLinear,noExcludes);
      rightExcludesLinear = evaluateI(rightLinearTerm, derivRight, x, prec, simplifiesA-1, simplifiesB, NULL, rightTheoLinear,noExcludes);

      sollya_mpfi_add(tempA,leftConstantTerm,rightConstantTerm);
      sollya_mpfi_add(tempB,leftLinearTerm,rightLinearTerm);

      sollya_mpfi_sub(xMXZ,x,zI);

      sollya_mpfi_mul(temp2,xMXZ,tempB);
      sollya_mpfi_add(temp1,tempA,temp2);

      sollya_mpfi_get_left(al,temp1);
      sollya_mpfi_get_right(ar,temp1);

      if (mpfr_number_p(al) && mpfr_number_p(ar)) {

	printMessage(8,"Information: decorrelating an interval addition.\n");
	if (verbosity >= 12) {
	  changeToWarningMode();
	  sollyaPrintf("Decorrelating on function\n");
	  printTree(tree);
	  sollyaPrintf("\nconstant term:\n");
	  printInterval(tempA);
	  sollyaPrintf("\nlinear term:\n");
	  printInterval(tempB);
	  sollyaPrintf("\ntranslated interval:\n");
	  printInterval(xMXZ);
	  sollyaPrintf("\nTaylor evaluation:\n");
	  printInterval(temp1);
	  sollyaPrintf("\ndirect evaluation:\n");
	  printInterval(stack3);
	  sollyaPrintf("\n");
	  restoreMode();
	}

	sollya_mpfi_intersect(stack3,stack3,temp1);
	if (!noExcludes) {
	  excludes = concatChains(leftExcludes,rightExcludes);
	  excludes = concatChains(excludes,leftExcludesConstant);
	  excludes = concatChains(excludes,rightExcludesConstant);	
	  excludes = concatChains(excludes,leftExcludesLinear);
	  excludes = concatChains(excludes,rightExcludesLinear);
	}

	if (internalTheo != NULL) {
	  internalTheo->boundLeftConstant = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
	  sollya_mpfi_init2(*(internalTheo->boundLeftConstant),prec);
	  internalTheo->boundRightConstant = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
	  sollya_mpfi_init2(*(internalTheo->boundRightConstant),prec);
	  internalTheo->boundLeftLinear = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
	  sollya_mpfi_init2(*(internalTheo->boundLeftLinear),prec);
	  internalTheo->boundRightLinear = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
	  sollya_mpfi_init2(*(internalTheo->boundRightLinear),prec);
	  internalTheo->xZ = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
	  sollya_mpfi_init2(*(internalTheo->xZ),prec);
	  internalTheo->xMXZ = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
	  sollya_mpfi_init2(*(internalTheo->xMXZ),prec);
	  internalTheo->leftDerivative = copyTree(derivLeft);
	  internalTheo->rightDerivative = copyTree(derivRight);
	  sollya_mpfi_set(*(internalTheo->boundLeftConstant),leftConstantTerm);
	  sollya_mpfi_set(*(internalTheo->boundRightConstant),rightConstantTerm);
	  sollya_mpfi_set(*(internalTheo->boundLeftLinear),leftLinearTerm);
	  sollya_mpfi_set(*(internalTheo->boundRightLinear),rightLinearTerm);
	  sollya_mpfi_set(*(internalTheo->xZ),zI);
	  sollya_mpfi_set(*(internalTheo->xMXZ),xMXZ);
	}
      } else {
	if (internalTheo != NULL) {
	  internalTheo->simplificationUsed = 0;
	  freeExprBoundTheo(internalTheo->theoLeftConstant);
	  internalTheo->theoLeftConstant = NULL;
	  freeExprBoundTheo(internalTheo->theoRightConstant);
	  internalTheo->theoRightConstant = NULL;
	  freeExprBoundTheo(internalTheo->theoLeftLinear);
	  internalTheo->theoLeftLinear = NULL;
	  freeExprBoundTheo(internalTheo->theoRightLinear);
	  internalTheo->theoRightLinear = NULL;
	}
	excludes = concatChains(leftExcludes,rightExcludes);
	freeChain(leftExcludesLinear,freeMpfiPtr); 	
	freeChain(rightExcludesLinear,freeMpfiPtr); 	
	freeChain(leftExcludesConstant,freeMpfiPtr); 	
	freeChain(rightExcludesConstant,freeMpfiPtr); 	
      }

      free_memory(derivLeft);
      free_memory(derivRight);
      mpfr_clear(z);
      sollya_mpfi_clear(zI);
      sollya_mpfi_clear(xMXZ);
      sollya_mpfi_clear(temp1);
      sollya_mpfi_clear(temp2);
      sollya_mpfi_clear(tempA);
      sollya_mpfi_clear(tempB);
      sollya_mpfi_clear(leftConstantTerm);
      sollya_mpfi_clear(rightConstantTerm);
      sollya_mpfi_clear(leftLinearTerm);
      sollya_mpfi_clear(rightLinearTerm);
    } else {
      excludes = concatChains(leftExcludes,rightExcludes);
    }
    break;
  case SUB:
    leftExcludes = evaluateI(stack1, tree->child1, x, prec, simplifiesA, simplifiesB, NULL, leftTheo,noExcludes);
    rightExcludes = evaluateI(stack2, tree->child2, x, prec, simplifiesA, simplifiesB, NULL, rightTheo,noExcludes);
    sollya_mpfi_sub(stack3, stack1, stack2);
    if (internalTheo != NULL) {
      sollya_mpfi_set(*(internalTheo->boundLeft),stack1);
      sollya_mpfi_set(*(internalTheo->boundRight),stack2);
    }
    if ((simplifiesA > 0) && (sollya_mpfi_has_zero(stack3)) && (!sollya_mpfi_has_zero(stack1)) && (!sollya_mpfi_has_zero(stack2)) && !xIsPoint) {

      if (internalTheo != NULL) {
	internalTheo->simplificationUsed = DECORRELATE;
	leftTheoConstant = (exprBoundTheo *) safeCalloc(1,sizeof(exprBoundTheo));
	nullifyExprBoundTheo(leftTheoConstant);
	rightTheoConstant = (exprBoundTheo *) safeCalloc(1,sizeof(exprBoundTheo));
	nullifyExprBoundTheo(rightTheoConstant);
	leftTheoLinear = (exprBoundTheo *) safeCalloc(1,sizeof(exprBoundTheo));
	nullifyExprBoundTheo(leftTheoLinear);
	rightTheoLinear = (exprBoundTheo *) safeCalloc(1,sizeof(exprBoundTheo));
	nullifyExprBoundTheo(rightTheoLinear);
	internalTheo->theoLeftConstant = leftTheoConstant;
	internalTheo->theoRightConstant = rightTheoConstant;
	internalTheo->theoLeftLinear = leftTheoLinear;
	internalTheo->theoRightLinear = rightTheoLinear;
      } else {
	leftTheoConstant = NULL;
	rightTheoConstant = NULL;
	leftTheoLinear = NULL;
	rightTheoLinear = NULL;
      }

      mpfr_init2(z,prec);
      sollya_mpfi_init2(zI,prec);
      sollya_mpfi_init2(leftConstantTerm,prec);
      sollya_mpfi_init2(rightConstantTerm,prec);
      sollya_mpfi_init2(leftLinearTerm,prec);
      sollya_mpfi_init2(rightLinearTerm,prec);
      sollya_mpfi_init2(xMXZ,prec);
      sollya_mpfi_init2(temp1,prec);
      sollya_mpfi_init2(temp2,prec);
      sollya_mpfi_init2(tempA,prec);
      sollya_mpfi_init2(tempB,prec);


      sollya_mpfi_mid(z,x);
      sollya_mpfi_set_fr(zI,z);

      leftExcludesConstant = evaluateI(leftConstantTerm, tree->child1, zI, prec, simplifiesA-1, simplifiesB, NULL, leftTheoConstant,noExcludes);
      rightExcludesConstant = evaluateI(rightConstantTerm, tree->child2, zI, prec, simplifiesA-1, simplifiesB, NULL, rightTheoConstant,noExcludes);

      printMessage(12,"Information: Differentiating while evaluating for decorrelation.\n");

      derivLeft = differentiate(tree->child1);
      derivRight = differentiate(tree->child2);

      leftExcludesLinear = evaluateI(leftLinearTerm, derivLeft, x, prec, simplifiesA-1, simplifiesB, NULL, leftTheoLinear,noExcludes);
      rightExcludesLinear = evaluateI(rightLinearTerm, derivRight, x, prec, simplifiesA-1, simplifiesB, NULL, rightTheoLinear,noExcludes);

      sollya_mpfi_sub(tempA,leftConstantTerm,rightConstantTerm);
      sollya_mpfi_sub(tempB,leftLinearTerm,rightLinearTerm);

      sollya_mpfi_sub(xMXZ,x,zI);

      sollya_mpfi_mul(temp2,xMXZ,tempB);
      sollya_mpfi_add(temp1,tempA,temp2);

      sollya_mpfi_get_left(al,temp1);
      sollya_mpfi_get_right(ar,temp1);

      if (mpfr_number_p(al) && mpfr_number_p(ar)) {

	printMessage(8,"Information: decorrelating an interval substraction.\n");
	if (verbosity >= 12) {
	  changeToWarningMode();
	  sollyaPrintf("Decorrelating on function\n");
	  printTree(tree);
	  sollyaPrintf("\nconstant term:\n");
	  printInterval(tempA);
	  sollyaPrintf("\nlinear term:\n");
	  printInterval(tempB);
	  sollyaPrintf("\ntranslated interval:\n");
	  printInterval(xMXZ);
	  sollyaPrintf("\nTaylor evaluation:\n");
	  printInterval(temp1);
	  sollyaPrintf("\ndirect evaluation:\n");
	  printInterval(stack3);
	  sollyaPrintf("\n");
	  restoreMode();
	}

	sollya_mpfi_intersect(stack3,stack3,temp1);
	if (!noExcludes) {
	  excludes = concatChains(leftExcludes,rightExcludes);
	  excludes = concatChains(excludes,leftExcludesConstant);
	  excludes = concatChains(excludes,rightExcludesConstant);	
	  excludes = concatChains(excludes,leftExcludesLinear);
	  excludes = concatChains(excludes,rightExcludesLinear);
	}

	if (internalTheo != NULL) {
	  internalTheo->boundLeftConstant = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
	  sollya_mpfi_init2(*(internalTheo->boundLeftConstant),prec);
	  internalTheo->boundRightConstant = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
	  sollya_mpfi_init2(*(internalTheo->boundRightConstant),prec);
	  internalTheo->boundLeftLinear = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
	  sollya_mpfi_init2(*(internalTheo->boundLeftLinear),prec);
	  internalTheo->boundRightLinear = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
	  sollya_mpfi_init2(*(internalTheo->boundRightLinear),prec);
	  internalTheo->xZ = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
	  sollya_mpfi_init2(*(internalTheo->xZ),prec);
	  internalTheo->xMXZ = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
	  sollya_mpfi_init2(*(internalTheo->xMXZ),prec);
	  internalTheo->leftDerivative = copyTree(derivLeft);
	  internalTheo->rightDerivative = copyTree(derivRight);
	  sollya_mpfi_set(*(internalTheo->boundLeftConstant),leftConstantTerm);
	  sollya_mpfi_set(*(internalTheo->boundRightConstant),rightConstantTerm);
	  sollya_mpfi_set(*(internalTheo->boundLeftLinear),leftLinearTerm);
	  sollya_mpfi_set(*(internalTheo->boundRightLinear),rightLinearTerm);
	  sollya_mpfi_set(*(internalTheo->xZ),zI);
	  sollya_mpfi_set(*(internalTheo->xMXZ),xMXZ);
	}
      } else {
	if (internalTheo != NULL) {
	  internalTheo->simplificationUsed = 0;
	  freeExprBoundTheo(internalTheo->theoLeftConstant);
	  internalTheo->theoLeftConstant = NULL;
	  freeExprBoundTheo(internalTheo->theoRightConstant);
	  internalTheo->theoRightConstant = NULL;
	  freeExprBoundTheo(internalTheo->theoLeftLinear);
	  internalTheo->theoLeftLinear = NULL;
	  freeExprBoundTheo(internalTheo->theoRightLinear);
	  internalTheo->theoRightLinear = NULL;
	}
	excludes = concatChains(leftExcludes,rightExcludes);
	freeChain(leftExcludesLinear,freeMpfiPtr); 	
	freeChain(rightExcludesLinear,freeMpfiPtr); 	
	freeChain(leftExcludesConstant,freeMpfiPtr); 	
	freeChain(rightExcludesConstant,freeMpfiPtr); 	
      }

      free_memory(derivLeft);
      free_memory(derivRight);
      mpfr_clear(z);
      sollya_mpfi_clear(zI);
      sollya_mpfi_clear(xMXZ);
      sollya_mpfi_clear(temp1);
      sollya_mpfi_clear(temp2);
      sollya_mpfi_clear(tempA);
      sollya_mpfi_clear(tempB);
      sollya_mpfi_clear(leftConstantTerm);
      sollya_mpfi_clear(rightConstantTerm);
      sollya_mpfi_clear(leftLinearTerm);
      sollya_mpfi_clear(rightLinearTerm);
    } else {
      excludes = concatChains(leftExcludes,rightExcludes);
    }
    break;
  case MUL:
    leftExcludes = evaluateI(stack1, tree->child1, x, prec, simplifiesA, simplifiesB, NULL, leftTheo,noExcludes);
    rightExcludes = evaluateI(stack2, tree->child2, x, prec, simplifiesA, simplifiesB, NULL, rightTheo,noExcludes);
    sollya_mpfi_mul(stack3, stack1, stack2);
    if (internalTheo != NULL) {
      sollya_mpfi_set(*(internalTheo->boundLeft),stack1);
      sollya_mpfi_set(*(internalTheo->boundRight),stack2);
    }
    excludes = concatChains(leftExcludes,rightExcludes);
    break;
  case DIV:
    leftExcludes = evaluateI(stack1, tree->child1, x, prec, simplifiesA, simplifiesB, NULL, leftTheo,noExcludes);
    rightExcludes = evaluateI(stack2, tree->child2, x, prec, simplifiesA, simplifiesB, NULL, rightTheo,noExcludes);

    if (internalTheo != NULL) {
      sollya_mpfi_set(*(internalTheo->boundLeft),stack1);
      sollya_mpfi_set(*(internalTheo->boundRight),stack2);
    }

    sollya_mpfi_get_left(al,stack1);
    sollya_mpfi_get_right(ar,stack1);
    sollya_mpfi_get_left(bl,stack2);
    sollya_mpfi_get_right(br,stack2);
    if (mpfr_zero_p(al) &&
	mpfr_zero_p(ar)) {
      if (mpfr_zero_p(bl) &&
	  mpfr_zero_p(br) &&
	  (simplifiesB > 0)) {
	/* [0;0] / [0;0] */

	printMessage(12,"Information: Differentiating while evaluating for Hopital's rule.\n");
	derivNumerator = differentiate(tree->child1);
	derivDenominator = differentiate(tree->child2);
	
	freeChain(leftExcludes,freeMpfiPtr);
	freeChain(rightExcludes,freeMpfiPtr);

	printMessage(8,"Information: using Hopital's rule on point division.\n");
	if (verbosity >= 9) {
	  changeToWarningMode();
	  sollyaPrintf("Information: entering interval was \n");
	  printInterval(x);
	  sollyaPrintf("\n");
	  restoreMode();
	}

	if (verbosity >= 12) {
	  changeToWarningMode();
	  sollyaPrintf("Hopital's rule is used on function\n");
	  printTree(tree);
	  sollyaPrintf("\n");
	  restoreMode();
	}

	if (verbosity >= 15) {
	  changeToWarningMode();
	  sollyaPrintf("The derivative of the numerator is\n");
	  printTree(derivNumerator);
	  sollyaPrintf("\n");
	  sollyaPrintf("The derivative of the denominator is\n");
	  printTree(derivDenominator);
	  sollyaPrintf("\n");
	  restoreMode();
	}

	if (internalTheo != NULL) {
	  internalTheo->simplificationUsed = HOPITAL_ON_POINT;
	  internalTheo->leftDerivative = copyTree(derivNumerator);
	  internalTheo->rightDerivative = copyTree(derivDenominator);
	  leftTheoLinear = (exprBoundTheo *) safeCalloc(1,sizeof(exprBoundTheo));
	  rightTheoLinear = (exprBoundTheo *) safeCalloc(1,sizeof(exprBoundTheo));
	  internalTheo->theoLeftLinear = leftTheoLinear;
	  internalTheo->theoRightLinear = rightTheoLinear;
	} else {
	  leftTheoLinear = NULL;
	  rightTheoLinear = NULL;
	}

	leftExcludes = evaluateI(stack1, derivNumerator, x, prec, simplifiesA, simplifiesB-1, NULL, leftTheoLinear,noExcludes);
	rightExcludes = evaluateI(stack2, derivDenominator, x, prec, simplifiesA, simplifiesB-1, NULL, rightTheoLinear,noExcludes);
	
	free_memory(derivNumerator);
	free_memory(derivDenominator);
	sollya_mpfi_div(stack3, stack1, stack2);
	excludes = concatChains(leftExcludes,rightExcludes);
      } else {
	/* [0;0] / [bl;br], bl,br != 0 */
	freeChain(rightExcludes,freeMpfiPtr);

	printMessage(8,"Information: simplifying an interval division with 0 point numerator.\n");
	if (verbosity >= 12) {
	  changeToWarningMode();
	  sollyaPrintf("Simplification on function\n");
	  printTree(tree);
	  sollyaPrintf("\n");
	  restoreMode();
	}


	sollya_mpfi_interv_d(stack3,0.0,0.0);
	excludes = leftExcludes;
	if (internalTheo != NULL) {
	  internalTheo->simplificationUsed = NUMERATOR_IS_ZERO;
	  freeExprBoundTheo(internalTheo->theoRight);
	  internalTheo->theoRight = NULL;
	  freeMpfiPtr(internalTheo->boundRight);
	  internalTheo->boundRight = NULL;
	}
      }
    } else {
      if (sollya_mpfi_has_zero(stack2) && 
	  (simplifiesB > 0)) {
	mpfr_init2(xl,prec);
	mpfr_init2(xr,prec);

	sollya_mpfi_get_left(xl,x);
	sollya_mpfi_get_right(xr,x);

	if (mpfr_cmp(xl,xr) != 0) {
	  
	  printMessage(12,"Information: Differentiating while evaluating for Hopital's rule.\n");
	  derivDenominator = differentiate(tree->child2);
	  
	  if ((simplifiesB == (hopitalrecursions + 1)) || (hopitalPoint == NULL)){
	    mpfr_init2(z,prec);
	    newtonMPFR(z,tree->child2,derivDenominator,xl,xr,prec);
	    newHopitalPoint = &z;
	  } else {
	    mpfr_init2(z,mpfr_get_prec(*hopitalPoint));
	    mpfr_set(z,*hopitalPoint,GMP_RNDN);
	    newHopitalPoint = hopitalPoint;
	  }
	  
	  if (mpfr_number_p(z)) {
	    sollya_mpfi_init2(zI,prec);
	    sollya_mpfi_set_fr(zI,z);
	    sollya_mpfi_init2(numeratorInZI,prec);
	    sollya_mpfi_init2(denominatorInZI,prec);
	    
	    if (internalTheo != NULL) {
	      	  leftTheoConstant = (exprBoundTheo *) safeCalloc(1,sizeof(exprBoundTheo));
		  rightTheoConstant = (exprBoundTheo *) safeCalloc(1,sizeof(exprBoundTheo));
	    } else {
	      leftTheoConstant = NULL;
	      rightTheoConstant = NULL;
	    }

	    t1 = evaluateI(numeratorInZI, tree->child1, zI, prec, simplifiesA, simplifiesB-1, newHopitalPoint, leftTheoConstant,1);
	    t2 = evaluateI(denominatorInZI, tree->child2, zI, prec, simplifiesA, simplifiesB-1, newHopitalPoint, rightTheoConstant,1);
	  
	    freeChain(t1,freeMpfiPtr);
	    freeChain(t2,freeMpfiPtr);

	    sollya_mpfi_get_left(al,numeratorInZI);
	    sollya_mpfi_get_right(ar,numeratorInZI);
	    sollya_mpfi_get_left(bl,denominatorInZI);
	    sollya_mpfi_get_right(br,denominatorInZI);
	    
	    if (mpfr_zero_p(al) && mpfr_zero_p(ar) && mpfr_zero_p(bl) && mpfr_zero_p(br)) {
	      /* Hopital's rule can be applied */

	      printMessage(12,"Information: Differentiating while evaluating for Hopital's rule.\n");
	      derivNumerator = differentiate(tree->child1);
	      
	      tempNode = (node *) safeMalloc(sizeof(node));
	      tempNode->nodeType = DIV;
	      tempNode->child1 = derivNumerator;
	      tempNode->child2 = copyTree(derivDenominator);
	      
	      freeChain(leftExcludes,freeMpfiPtr);
	      freeChain(rightExcludes,freeMpfiPtr);
	      
	      if (internalTheo != NULL) {
		leftTheoLinear = (exprBoundTheo *) safeCalloc(1,sizeof(exprBoundTheo));
		nullifyExprBoundTheo(leftTheoLinear); 
		internalTheo->simplificationUsed = HOPITAL;
		internalTheo->leftDerivative = copyTree(tempNode->child1);
		internalTheo->rightDerivative = copyTree(tempNode->child2);
		internalTheo->theoLeftConstant = leftTheoConstant;
		internalTheo->theoRightConstant = rightTheoConstant;
		internalTheo->theoLeftLinear = leftTheoLinear;
		internalTheo->boundLeftLinear = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
		sollya_mpfi_init2(*(internalTheo->boundLeftLinear),prec);
		internalTheo->boundLeftConstant = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
		sollya_mpfi_init2(*(internalTheo->boundLeftConstant),prec);
		internalTheo->boundRightConstant = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
		sollya_mpfi_init2(*(internalTheo->boundRightConstant),prec);
		sollya_mpfi_set(*(internalTheo->boundLeftConstant),numeratorInZI);
		sollya_mpfi_set(*(internalTheo->boundRightConstant),denominatorInZI);
		internalTheo->xZ = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
		sollya_mpfi_init2(*(internalTheo->xZ),prec);
		sollya_mpfi_set(*(internalTheo->xZ),zI);
	      } else {
		leftTheoLinear = NULL;
	      }

	      if (simplifiesB == (hopitalrecursions + 1)) {
		printMessage(8,"Information: using Hopital's rule (general case) on denominator zero.\n");
		if (verbosity >= 10) {
		  changeToWarningMode();
		  sollyaPrintf("Hopital's rule is used on function\n");
		  printTree(tree);
		  sollyaPrintf(" in point ");
		  printMpfr(z);
		  restoreMode();
		}
	      }

	      if (simplifiesB != (hopitalrecursions + 1)) {
		printMessage(8,"Information: recursion on use of Hopital's rule\n");
		if (verbosity >= 10) {
		  changeToWarningMode();
		  sollyaPrintf("Reused Hopital's rule point is ");
		  printMpfr(*hopitalPoint);
		  restoreMode();
		}
	      }

	      if (verbosity >= 15) {
		changeToWarningMode();
		sollyaPrintf("Information in Hopital: The simplified function is\n");
		printTree(tempNode);
		sollyaPrintf("\n");
		restoreMode();
	      }
	      
	      excludes = evaluateI(stack3, tempNode, x, prec, simplifiesA, simplifiesB-1, newHopitalPoint, leftTheoLinear,noExcludes);
	      
	      if (internalTheo != NULL) sollya_mpfi_set(*(internalTheo->boundLeftLinear),stack3);
	      
	      free_memory(tempNode);
	    } else {
	      if (internalTheo != NULL) {
		freeExprBoundTheo(leftTheoConstant);
		freeExprBoundTheo(rightTheoConstant);
	      }
  
	      printMessage(12,"Information: Differentiating while evaluating for Hopital's rule.\n");
	      derivNumerator = differentiate(tree->child1);

	      if ((simplifiesB == (hopitalrecursions + 1)) || (hopitalPoint == NULL)) {
		mpfr_init2(z2,prec);
		newtonMPFR(z2,tree->child1,derivNumerator,xl,xr,prec);
		newHopitalPoint = &z2;
	      } else {
		mpfr_init2(z2,mpfr_get_prec(*hopitalPoint));
		mpfr_set(z2,*hopitalPoint,GMP_RNDN);
		newHopitalPoint = hopitalPoint;
	      }

	      if (mpfr_number_p(z2)) {
		sollya_mpfi_set_fr(zI,z2);

		if (internalTheo != NULL) {
		  leftTheoConstant = (exprBoundTheo *) safeCalloc(1,sizeof(exprBoundTheo));
		  rightTheoConstant = (exprBoundTheo *) safeCalloc(1,sizeof(exprBoundTheo));
		} else {
		  leftTheoConstant = NULL;
		  rightTheoConstant = NULL;
		}
	    
		t1 = evaluateI(numeratorInZI, tree->child1, zI, prec, simplifiesA, simplifiesB-1, newHopitalPoint, leftTheoConstant,1);
		t2 = evaluateI(denominatorInZI, tree->child2, zI, prec, simplifiesA, simplifiesB-1, newHopitalPoint, rightTheoConstant,1);

		freeChain(t1,freeMpfiPtr);
		freeChain(t2,freeMpfiPtr);

		sollya_mpfi_get_left(al,numeratorInZI);
		sollya_mpfi_get_right(ar,numeratorInZI);
		sollya_mpfi_get_left(bl,denominatorInZI);
		sollya_mpfi_get_right(br,denominatorInZI);
		
		if (mpfr_zero_p(al) && mpfr_zero_p(ar) && mpfr_zero_p(bl) && mpfr_zero_p(br)) {

		  tempNode = (node *) safeMalloc(sizeof(node));
		  tempNode->nodeType = DIV;
		  tempNode->child1 = derivNumerator;
		  tempNode->child2 = copyTree(derivDenominator);

		  freeChain(leftExcludes,freeMpfiPtr);
		  freeChain(rightExcludes,freeMpfiPtr);

		  if (internalTheo != NULL) {
		    leftTheoLinear = (exprBoundTheo *) safeCalloc(1,sizeof(exprBoundTheo));
		    nullifyExprBoundTheo(leftTheoLinear); 
		    internalTheo->simplificationUsed = HOPITAL;
		    internalTheo->leftDerivative = copyTree(tempNode->child1);
		    internalTheo->rightDerivative = copyTree(tempNode->child2);
		    internalTheo->theoLeftConstant = leftTheoConstant;
		    internalTheo->theoRightConstant = rightTheoConstant;
		    internalTheo->theoLeftLinear = leftTheoLinear;
		    internalTheo->boundLeftLinear = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
		    sollya_mpfi_init2(*(internalTheo->boundLeftLinear),prec);
		    internalTheo->boundLeftConstant = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
		    sollya_mpfi_init2(*(internalTheo->boundLeftConstant),prec);
		    internalTheo->boundRightConstant = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
		    sollya_mpfi_init2(*(internalTheo->boundRightConstant),prec);
		    sollya_mpfi_set(*(internalTheo->boundLeftConstant),numeratorInZI);
		    sollya_mpfi_set(*(internalTheo->boundRightConstant),denominatorInZI);
		    internalTheo->xZ = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
		    sollya_mpfi_init2(*(internalTheo->xZ),prec);
		    sollya_mpfi_set(*(internalTheo->xZ),zI);
		  } else {
		    leftTheoLinear = NULL;
		  }

		  if (simplifiesB == (hopitalrecursions + 1)) {
		    printMessage(8,"Information: using Hopital's rule (general case) on numerator zero.\n");
		    if (verbosity >= 10) {
		      changeToWarningMode();
		      sollyaPrintf("Hopital's rule is used on function\n");
		      printTree(tree);
		      sollyaPrintf(" in point ");
		      printMpfr(z);
		      restoreMode();
		    }
		  }

		  if (simplifiesB != (hopitalrecursions + 1)) {
		    printMessage(8,"Information: recursion on use of Hopital's rule\n");
		    if (verbosity >= 10) {
		      changeToWarningMode();
		      sollyaPrintf("Reused Hopital's rule point is ");
		      printMpfr(*hopitalPoint);
		      restoreMode();
		    }
		  }
		  
		  if (verbosity >= 15) {
		    changeToWarningMode();
		    sollyaPrintf("Information in Hopital: The simplified function is\n");
		    printTree(tempNode);
		    sollyaPrintf("\n");
		    restoreMode();
		  }


		  excludes = evaluateI(stack3, tempNode, x, prec, simplifiesA, simplifiesB-1, newHopitalPoint, leftTheoLinear,noExcludes);

		  if (internalTheo != NULL) sollya_mpfi_set(*(internalTheo->boundLeftLinear),stack3);
	      
		  free_memory(tempNode);
		} else {

		  if (internalTheo != NULL) {
		    freeExprBoundTheo(leftTheoConstant);
		    freeExprBoundTheo(rightTheoConstant);
		  }

		  if (!noExcludes) {
		    newExclude = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
		    sollya_mpfi_init2(*newExclude,prec);
		    makeMpfiAroundMpfr(*newExclude,z,16777216);
		    sollya_mpfi_init2(newExcludeTemp,prec);
		    makeMpfiAroundMpfr(newExcludeTemp,z2,16777216);
		    sollya_mpfi_union(*newExclude,*newExclude,newExcludeTemp);
		    sollya_mpfi_clear(newExcludeTemp);
		    
		    excludes = concatChains(leftExcludes,rightExcludes);
		    excludes = addElement(excludes,newExclude);
		  }

		  free_memory(derivNumerator);
		  mpfr_clear(z2);
		  sollya_mpfi_div(stack3, stack1, stack2);
		}
	      } else {

		mpfr_clear(z2);

		if (!noExcludes) {
		  newExclude = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
		  sollya_mpfi_init2(*newExclude,prec);
		  makeMpfiAroundMpfr(*newExclude,z,16777216);
		  excludes = concatChains(leftExcludes,rightExcludes);
		  excludes = addElement(excludes,newExclude);
		}

		sollya_mpfi_div(stack3, stack1, stack2);
	      }
	    }

	    sollya_mpfi_clear(numeratorInZI);
	    sollya_mpfi_clear(denominatorInZI);
	    sollya_mpfi_clear(zI);
	  } else {
	    sollya_mpfi_div(stack3, stack1, stack2);
	    excludes = concatChains(leftExcludes,rightExcludes);
	  }
	  free_memory(derivDenominator);
	  mpfr_clear(z);
	} else {
	  sollya_mpfi_div(stack3, stack1, stack2);
	  excludes = concatChains(leftExcludes,rightExcludes);
	}
	mpfr_clear(xl);
	mpfr_clear(xr);
      } else {
	sollya_mpfi_div(stack3, stack1, stack2);
	excludes = concatChains(leftExcludes,rightExcludes);
      }
    }
    break;
  case SQRT:
    excludes = evaluateI(stack1, tree->child1, x, prec, simplifiesA, simplifiesB, NULL, leftTheo,noExcludes);
    sollya_mpfi_sqrt(stack3, stack1);
    if (internalTheo != NULL) {
      sollya_mpfi_set(*(internalTheo->boundLeft),stack1);
    }
    break;
  case EXP:
    excludes = evaluateI(stack1, tree->child1, x, prec, simplifiesA, simplifiesB, NULL, leftTheo,noExcludes);
    sollya_mpfi_exp(stack3, stack1);
    if (internalTheo != NULL) {
      sollya_mpfi_set(*(internalTheo->boundLeft),stack1);
    }
    break;
  case LOG:
    excludes = evaluateI(stack1, tree->child1, x, prec, simplifiesA, simplifiesB, NULL, leftTheo,noExcludes);
    sollya_mpfi_log(stack3, stack1);
    if (internalTheo != NULL) {
      sollya_mpfi_set(*(internalTheo->boundLeft),stack1);
    }
    break;
  case LOG_2:
    excludes = evaluateI(stack1, tree->child1, x, prec, simplifiesA, simplifiesB, NULL, leftTheo,noExcludes);
    sollya_mpfi_log2(stack3, stack1);
    if (internalTheo != NULL) {
      sollya_mpfi_set(*(internalTheo->boundLeft),stack1);
    }
    break;
  case LOG_10:
    evaluateI(stack1, tree->child1, x, prec, simplifiesA, simplifiesB, NULL, leftTheo,noExcludes);
    sollya_mpfi_log10(stack3, stack1);
    if (internalTheo != NULL) {
      sollya_mpfi_set(*(internalTheo->boundLeft),stack1);
    }
    break;
  case SIN:
    excludes = evaluateI(stack1, tree->child1, x, prec, simplifiesA, simplifiesB, NULL, leftTheo,noExcludes);
    sollya_mpfi_sin(stack3, stack1);
    if (sollya_mpfi_inf_p(stack1)) {
      sollya_mpfi_set_nan(stack3);
    }
    if (internalTheo != NULL) {
      sollya_mpfi_set(*(internalTheo->boundLeft),stack1);
    }

#if DEBUGMPFI 
    sollyaPrintf("sollya_mpfi_sin(");
    printInterval(stack1);
    sollyaPrintf(") = ");
    printInterval(stack3);
    sollyaPrintf("\n");
#endif    

    break;
  case COS:
    excludes = evaluateI(stack1, tree->child1, x, prec, simplifiesA, simplifiesB, NULL, leftTheo,noExcludes);
    sollya_mpfi_cos(stack3, stack1);
    if (sollya_mpfi_inf_p(stack1)) {
      sollya_mpfi_set_nan(stack3);
    }
    if (internalTheo != NULL) {
      sollya_mpfi_set(*(internalTheo->boundLeft),stack1);
    }

#if DEBUGMPFI 
    sollyaPrintf("sollya_mpfi_cos(");
    printInterval(stack1);
    sollyaPrintf(") = ");
    printInterval(stack3);
    sollyaPrintf("\n");
#endif    

    break;
  case TAN:
    excludes = evaluateI(stack1, tree->child1, x, prec, simplifiesA, simplifiesB, NULL, leftTheo,noExcludes);
    sollya_mpfi_tan(stack3, stack1);
    if (internalTheo != NULL) {
      sollya_mpfi_set(*(internalTheo->boundLeft),stack1);
    }
    break;
  case ASIN:
    excludes = evaluateI(stack1, tree->child1, x, prec, simplifiesA, simplifiesB, NULL, leftTheo,noExcludes);
    sollya_mpfi_asin(stack3, stack1);
    if (internalTheo != NULL) {
      sollya_mpfi_set(*(internalTheo->boundLeft),stack1);
    }
    break;
  case ACOS:
    excludes = evaluateI(stack1, tree->child1, x, prec, simplifiesA, simplifiesB, NULL, leftTheo,noExcludes);
    sollya_mpfi_acos(stack3, stack1);
    if (internalTheo != NULL) {
      sollya_mpfi_set(*(internalTheo->boundLeft),stack1);
    }
    break;
  case ATAN:
    excludes = evaluateI(stack1, tree->child1, x, prec, simplifiesA, simplifiesB, NULL, leftTheo,noExcludes);
    sollya_mpfi_atan(stack3, stack1);
    if (internalTheo != NULL) {
      sollya_mpfi_set(*(internalTheo->boundLeft),stack1);
    }
    break;
  case SINH:
    excludes = evaluateI(stack1, tree->child1, x, prec, simplifiesA, simplifiesB, NULL, leftTheo,noExcludes);
    sollya_mpfi_sinh(stack3, stack1);
    if (internalTheo != NULL) {
      sollya_mpfi_set(*(internalTheo->boundLeft),stack1);
    }
    break;
  case COSH:
    excludes = evaluateI(stack1, tree->child1, x, prec, simplifiesA, simplifiesB, NULL, leftTheo,noExcludes);
    sollya_mpfi_cosh(stack3, stack1);
    if (internalTheo != NULL) {
      sollya_mpfi_set(*(internalTheo->boundLeft),stack1);
    }
    break;
  case TANH:
    excludes = evaluateI(stack1, tree->child1, x, prec, simplifiesA, simplifiesB, NULL, leftTheo,noExcludes);
    sollya_mpfi_tanh(stack3, stack1);
    if (internalTheo != NULL) {
      sollya_mpfi_set(*(internalTheo->boundLeft),stack1);
    }
    break;
  case ASINH:
    excludes = evaluateI(stack1, tree->child1, x, prec, simplifiesA, simplifiesB, NULL, leftTheo,noExcludes);
    sollya_mpfi_asinh(stack3, stack1);
    if (internalTheo != NULL) {
      sollya_mpfi_set(*(internalTheo->boundLeft),stack1);
    }
    break;
  case ACOSH:
    excludes = evaluateI(stack1, tree->child1, x, prec, simplifiesA, simplifiesB, NULL, leftTheo,noExcludes);
    sollya_mpfi_acosh(stack3, stack1);
    if (internalTheo != NULL) {
      sollya_mpfi_set(*(internalTheo->boundLeft),stack1);
    }
    break;
  case ATANH:
    excludes = evaluateI(stack1, tree->child1, x, prec, simplifiesA, simplifiesB, NULL, leftTheo,noExcludes);
    sollya_mpfi_atanh(stack3, stack1);
    if (internalTheo != NULL) {
      sollya_mpfi_set(*(internalTheo->boundLeft),stack1);
    }
    break;
  case POW:
    leftExcludes = evaluateI(stack1, tree->child1, x, prec, simplifiesA, simplifiesB, NULL, leftTheo,noExcludes);
    rightExcludes = evaluateI(stack2, tree->child2, x, prec, simplifiesA, simplifiesB, NULL, rightTheo,noExcludes);
    sollya_mpfi_pow(stack3, stack1, stack2);
    if (internalTheo != NULL) {
      sollya_mpfi_set(*(internalTheo->boundLeft),stack1);
      sollya_mpfi_set(*(internalTheo->boundRight),stack2);
    }
    excludes = concatChains(leftExcludes,rightExcludes);
    break;
  case NEG:
    excludes = evaluateI(stack1, tree->child1, x, prec, simplifiesA, simplifiesB, NULL, leftTheo,noExcludes);
    sollya_mpfi_neg(stack3, stack1);
    if (internalTheo != NULL) {
      sollya_mpfi_set(*(internalTheo->boundLeft),stack1);
    }
    break;
  case ABS:
    excludes = evaluateI(stack1, tree->child1, x, prec, simplifiesA, simplifiesB, NULL, leftTheo,noExcludes);
    sollya_mpfi_abs(stack3, stack1);  
    if (internalTheo != NULL) {
      sollya_mpfi_set(*(internalTheo->boundLeft),stack1);
    }
    break;
  case DOUBLE:
    excludes = evaluateI(stack1, tree->child1, x, prec, simplifiesA, simplifiesB, NULL, leftTheo,noExcludes);
    sollya_mpfi_round_to_double(stack3, stack1);
    if (internalTheo != NULL) {
      sollya_mpfi_set(*(internalTheo->boundLeft),stack1);
    }
    break;
  case SINGLE:
    excludes = evaluateI(stack1, tree->child1, x, prec, simplifiesA, simplifiesB, NULL, leftTheo,noExcludes);
    sollya_mpfi_round_to_single(stack3, stack1);
    if (internalTheo != NULL) {
      sollya_mpfi_set(*(internalTheo->boundLeft),stack1);
    }
    break;
  case HALFPRECISION:
    excludes = evaluateI(stack1, tree->child1, x, prec, simplifiesA, simplifiesB, NULL, leftTheo,noExcludes);
    sollya_mpfi_round_to_halfprecision(stack3, stack1);
    if (internalTheo != NULL) {
      sollya_mpfi_set(*(internalTheo->boundLeft),stack1);
    }
    break;
  case QUAD:
    excludes = evaluateI(stack1, tree->child1, x, prec, simplifiesA, simplifiesB, NULL, leftTheo,noExcludes);
    sollya_mpfi_round_to_quad(stack3, stack1);
    if (internalTheo != NULL) {
      sollya_mpfi_set(*(internalTheo->boundLeft),stack1);
    }
    break;
  case DOUBLEDOUBLE:
    excludes = evaluateI(stack1, tree->child1, x, prec, simplifiesA, simplifiesB, NULL, leftTheo,noExcludes);
    sollya_mpfi_round_to_doubledouble(stack3, stack1);
    if (internalTheo != NULL) {
      sollya_mpfi_set(*(internalTheo->boundLeft),stack1);
    }
    break;
  case TRIPLEDOUBLE:
    excludes = evaluateI(stack1, tree->child1, x, prec, simplifiesA, simplifiesB, NULL, leftTheo,noExcludes);
    sollya_mpfi_round_to_tripledouble(stack3, stack1);
    if (internalTheo != NULL) {
      sollya_mpfi_set(*(internalTheo->boundLeft),stack1);
    }
    break;
  case ERF:
    excludes = evaluateI(stack1, tree->child1, x, prec, simplifiesA, simplifiesB, NULL, leftTheo,noExcludes);
    sollya_mpfi_erf(stack3, stack1);
    if (internalTheo != NULL) {
      sollya_mpfi_set(*(internalTheo->boundLeft),stack1);
    }
    break;
  case ERFC:
    excludes = evaluateI(stack1, tree->child1, x, prec, simplifiesA, simplifiesB, NULL, leftTheo,noExcludes);
    sollya_mpfi_erfc(stack3, stack1);
    if (internalTheo != NULL) {
      sollya_mpfi_set(*(internalTheo->boundLeft),stack1);
    }
    break;
  case LOG_1P:
    excludes = evaluateI(stack1, tree->child1, x, prec, simplifiesA, simplifiesB, NULL, leftTheo,noExcludes);
    sollya_mpfi_log1p(stack3, stack1);
    if (internalTheo != NULL) {
      sollya_mpfi_set(*(internalTheo->boundLeft),stack1);
    }
    break;
  case EXP_M1:
    excludes = evaluateI(stack1, tree->child1, x, prec, simplifiesA, simplifiesB, NULL, leftTheo,noExcludes);
    sollya_mpfi_expm1(stack3, stack1);  
    if (internalTheo != NULL) {
      sollya_mpfi_set(*(internalTheo->boundLeft),stack1);
    }
    break;
  case DOUBLEEXTENDED:
    excludes = evaluateI(stack1, tree->child1, x, prec, simplifiesA, simplifiesB, NULL, leftTheo,noExcludes);
    sollya_mpfi_round_to_doubleextended(stack3, stack1);
    if (internalTheo != NULL) {
      sollya_mpfi_set(*(internalTheo->boundLeft),stack1);
    }
    break;
  case LIBRARYFUNCTION:
    excludes = evaluateI(stack1, tree->child1, x, prec, simplifiesA, simplifiesB, NULL, leftTheo,noExcludes);
    tree->libFun->code(stack3, stack1, tree->libFunDeriv);
    if (internalTheo != NULL) {
      sollya_mpfi_set(*(internalTheo->boundLeft),stack1);
    }
    break;
  case PROCEDUREFUNCTION:
    excludes = evaluateI(stack1, tree->child1, x, prec, simplifiesA, simplifiesB, NULL, leftTheo,noExcludes);
    computeFunctionWithProcedure(stack3, tree->child2, stack1, (unsigned int) tree->libFunDeriv);
    if (internalTheo != NULL) {
      sollya_mpfi_set(*(internalTheo->boundLeft),stack1);
    }
    break;
  case CEIL:
    excludes = evaluateI(stack1, tree->child1, x, prec, simplifiesA, simplifiesB, NULL, leftTheo,noExcludes);
    sollya_mpfi_ceil(stack3, stack1);
    if (internalTheo != NULL) {
      sollya_mpfi_set(*(internalTheo->boundLeft),stack1);
    }
    break;
  case FLOOR:
    excludes = evaluateI(stack1, tree->child1, x, prec, simplifiesA, simplifiesB, NULL, leftTheo,noExcludes);
    sollya_mpfi_floor(stack3, stack1);
    if (internalTheo != NULL) {
      sollya_mpfi_set(*(internalTheo->boundLeft),stack1);
    }
    break;
  case NEARESTINT:
    excludes = evaluateI(stack1, tree->child1, x, prec, simplifiesA, simplifiesB, NULL, leftTheo,noExcludes);
    sollya_mpfi_nearestint(stack3, stack1);
    if (internalTheo != NULL) {
      sollya_mpfi_set(*(internalTheo->boundLeft),stack1);
    }
    break;
  case PI_CONST:
    sollya_mpfi_const_pi(stack3);
    excludes = NULL;
    break;
  case LIBRARYCONSTANT:
    libraryConstantToInterval(stack3, tree);
    excludes = NULL;
    break;
  default:
    sollyaFprintf(stderr,"Error: evaluateI: unknown identifier in the tree\n");
    exit(1);
  }

  sollya_mpfi_set(result,stack3);

  if (theo != NULL) {
    theo->y = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
    sollya_mpfi_init2(*(theo->y),sollya_mpfi_get_prec(result));
    sollya_mpfi_set(*(theo->y),result);
  }
  sollya_mpfi_clear(stack1); 
  sollya_mpfi_clear(stack2); 
  sollya_mpfi_clear(stack3);
  mpfr_clear(al); 
  mpfr_clear(ar); 
  mpfr_clear(bl); 
  mpfr_clear(br);
  return excludes;
}

chain* evaluateITaylor(sollya_mpfi_t result, node *func, node *deriv, sollya_mpfi_t x, mp_prec_t prec, int recurse, exprBoundTheo *theo, int noExcludes);

chain* evaluateITaylorOnDiv(sollya_mpfi_t result, node *func, sollya_mpfi_t x, mp_prec_t prec, int recurse, exprBoundTheo *theo, int noExcludes) {
  node *numerator, *denominator, *derivNumerator, *derivDenominator;
  chain *excludes, *numeratorExcludes, *denominatorExcludes;
  exprBoundTheo *numeratorTheo, *denominatorTheo;
  sollya_mpfi_t resultNumerator, resultDenominator, resultIndirect;
  mpfr_t tempNaN;

  mpfr_init2(tempNaN, prec);

  if (func->nodeType == DIV) {
    numerator = func->child1;
    denominator = func->child2;
    derivNumerator = differentiate(numerator);
    derivDenominator = differentiate(denominator);
    sollya_mpfi_init2(resultNumerator, prec);
    sollya_mpfi_init2(resultDenominator, prec);
    sollya_mpfi_init2(resultIndirect, prec);

    if (theo != NULL) {
      numeratorTheo = (exprBoundTheo *) safeCalloc(1,sizeof(exprBoundTheo));
      denominatorTheo = (exprBoundTheo *) safeCalloc(1,sizeof(exprBoundTheo));
    } else {
      numeratorTheo = NULL;
      denominatorTheo = NULL;
    }
    
    numeratorExcludes = evaluateITaylor(resultNumerator, numerator, derivNumerator, x, prec, recurse, numeratorTheo,noExcludes);
    denominatorExcludes = evaluateITaylor(resultDenominator, denominator, derivDenominator, x, prec, recurse, denominatorTheo,noExcludes);
    excludes = concatChains(numeratorExcludes,denominatorExcludes);    
    sollya_mpfi_div(resultIndirect, resultNumerator, resultDenominator);
    if (sollya_mpfi_bounded_p(resultIndirect)) {
      sollya_mpfi_set(result, resultIndirect);
      sollya_mpfi_nan_normalize(result);

      if (theo != NULL) {
	theo->functionType = func->nodeType;
	theo->boundLeft = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
	sollya_mpfi_init2(*(theo->boundLeft),prec);
	theo->boundRight = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
	sollya_mpfi_init2(*(theo->boundRight),prec);
	theo->theoLeft = numeratorTheo;
	theo->theoRight = denominatorTheo;
	theo->x = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
	sollya_mpfi_init2(*(theo->x),sollya_mpfi_get_prec(x));
	sollya_mpfi_set(*(theo->x),x);
	theo->function = copyTree(func);
	theo->y = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
	sollya_mpfi_init2(*(theo->y),sollya_mpfi_get_prec(result));
	sollya_mpfi_set(*(theo->y),result);
      }
    } else {
      freeChain(excludes,freeMpfiPtr); 
      if (theo != NULL) {
	freeExprBoundTheo(numeratorTheo);
	freeExprBoundTheo(denominatorTheo);
      }
      excludes = evaluateI(result, func, x, prec, 0, hopitalrecursions+1, NULL, theo,noExcludes);
      sollya_mpfi_nan_normalize(result);
    }
    
    sollya_mpfi_clear(resultNumerator);
    sollya_mpfi_clear(resultDenominator);
    sollya_mpfi_clear(resultIndirect);     
    free_memory(derivNumerator);
    free_memory(derivDenominator);
    mpfr_clear(tempNaN);
    return excludes;
  }
  else {
    excludes = evaluateI(result, func, x, prec, 0, hopitalrecursions+1, NULL, theo,noExcludes);
    sollya_mpfi_nan_normalize(result);
    mpfr_clear(tempNaN);
    return excludes;
  }
}

chain* evaluateITaylor(sollya_mpfi_t result, node *func, node *deriv, sollya_mpfi_t x, mp_prec_t prec, int recurse, exprBoundTheo *theo, int noExcludes) {
  mpfr_t xZ, rTl, rTr, leftX, rightX;
  sollya_mpfi_t xZI, xZI2, constantTerm, linearTerm, resultTaylor, resultDirect, temp, temp2;
  chain *excludes, *directExcludes, *taylorExcludes, *taylorExcludesLinear, *taylorExcludesConstant;
  exprBoundTheo *constantTheo, *linearTheo, *directTheo;
  node *nextderiv;
  int size;

  mpfr_init2(leftX,sollya_mpfi_get_prec(x));
  mpfr_init2(rightX,sollya_mpfi_get_prec(x));
  
  sollya_mpfi_get_left(leftX,x);
  sollya_mpfi_get_right(rightX,x);

  if ((mpfr_cmp(leftX,rightX) == 0) || (deriv == NULL)) {
    if (deriv != NULL) 
      printMessage(25,"Information: avoiding using Taylor's formula on a point interval.\n");
    else 
      printMessage(25,"Warning: no Taylor evaluation is possible because no derivative has been given.\n");
    
    excludes = evaluateI(result, func, x, prec, 1, hopitalrecursions+1, NULL, theo,noExcludes);
    sollya_mpfi_nan_normalize(result);

    mpfr_clear(leftX);
    mpfr_clear(rightX);
    
    return excludes;
  }


  printMessage(13,"Information: evaluating a function in interval arithmetic using Taylor's formula.\n");
  if (verbosity >= 15) {
    changeToWarningMode();
    sollyaPrintf("Information: the function is\n");
    printTree(func);
    sollyaPrintf("\nIts derivative is\n");
    printTree(deriv);
    sollyaPrintf("\n");
    restoreMode();
  }

  if (theo != NULL) {
    nullifyExprBoundTheo(theo);

    if (!isPolynomial(func)) {
      constantTheo = (exprBoundTheo *) safeCalloc(1,sizeof(exprBoundTheo));
      nullifyExprBoundTheo(constantTheo);
      linearTheo = (exprBoundTheo *) safeCalloc(1,sizeof(exprBoundTheo));
      nullifyExprBoundTheo(linearTheo);
      directTheo = (exprBoundTheo *) safeCalloc(1,sizeof(exprBoundTheo));
      nullifyExprBoundTheo(directTheo);
      theo->functionType = func->nodeType;
    } else {
      constantTheo = NULL;
      linearTheo = NULL;
      directTheo = NULL;
      theo->functionType = POLYNOMIAL;
    }
    theo->function = copyTree(func);
    theo->x = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
    sollya_mpfi_init2(*(theo->x),prec);
    sollya_mpfi_set(*(theo->x),x);
    theo->y = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
    sollya_mpfi_init2(*(theo->y),prec);
  } else {
    constantTheo = NULL;
    linearTheo = NULL;
    directTheo = NULL;
  }

  mpfr_init2(xZ,prec);
  sollya_mpfi_init2(xZI,prec);
  sollya_mpfi_init2(temp,prec);
  sollya_mpfi_init2(temp2,prec);
  sollya_mpfi_init2(constantTerm,prec);
  sollya_mpfi_init2(linearTerm,prec);
  sollya_mpfi_init2(resultTaylor,prec);
  sollya_mpfi_init2(resultDirect,prec);

  mpfr_init2(rTl,prec);
  mpfr_init2(rTr,prec);

  sollya_mpfi_mid(xZ,x);
  sollya_mpfi_set_fr(xZI,xZ);


  if ((recurse > 0) && (func->nodeType != DIV)) {
    nextderiv = differentiate(deriv);
    size = treeSize(nextderiv);

    if (size > DIFFSIZE) {
	printMessage(1,"Waring: during recursive Taylor evaluation the expression of a derivative has become\n");
	printMessage(1,"as great that it contains more than %d nodes.\n",DIFFSIZE);
	printMessage(1,"Will now stop recursive Taylor evaluation on this expression.\n");
	printMessage(2,"Information: the size of the derivative is %d, we had %d recursion(s) left.\n",size,recurse-1);
	taylorExcludesLinear = evaluateI(linearTerm, deriv, x, prec, 1, hopitalrecursions+1, NULL, linearTheo,noExcludes);
    } else {
      taylorExcludesLinear = evaluateITaylor(linearTerm, deriv, nextderiv, x, prec, recurse - 1, linearTheo,noExcludes);
    }
    
    free_memory(nextderiv);
  } else {
    taylorExcludesLinear = evaluateI(linearTerm, deriv, x, prec, 1, hopitalrecursions+1, NULL, linearTheo,noExcludes);
  }

  if ((sollya_mpfi_is_nonneg(linearTerm) || sollya_mpfi_is_nonpos(linearTerm)) && sollya_mpfi_bounded_p(linearTerm)) {

    printMessage(12,"Information: the linear term during Taylor evaluation does not change its sign.\n");
    printMessage(12,"Simplifying by taking the convex hull of the evaluations on the endpoints.\n");

 
    sollya_mpfi_init2(xZI2,prec);
    
    sollya_mpfi_set_fr(xZI,leftX);
    sollya_mpfi_set_fr(xZI2,rightX);

    directExcludes = evaluateI(resultDirect, func, xZI, prec, 0, hopitalrecursions+1, NULL, directTheo,noExcludes);
    taylorExcludesConstant = evaluateI(constantTerm, func, xZI2, prec, 1, hopitalrecursions+1, NULL, constantTheo,noExcludes);

    sollya_mpfi_union(result,resultDirect,constantTerm);
    
    if (theo != NULL) {
      if (theo->functionType != POLYNOMIAL) {
	theo->simplificationUsed = MONOTONOCITY;
	theo->theoLeft = directTheo;
	theo->theoRight = constantTheo;
	theo->theoLeftLinear = linearTheo;
	theo->leftDerivative = copyTree(deriv);
	theo->boundLeft = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
	sollya_mpfi_init2(*(theo->boundLeft),prec);
	sollya_mpfi_set(*(theo->boundLeft),resultDirect);
	theo->theoRight = constantTheo;
	theo->boundRight = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
	sollya_mpfi_init2(*(theo->boundRight),prec);
	sollya_mpfi_set(*(theo->boundRight),constantTerm);
	theo->boundLeftLinear = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
	sollya_mpfi_init2(*(theo->boundLeftLinear),prec);
	sollya_mpfi_set(*(theo->boundLeftLinear),linearTerm);
      }
    }

    excludes = concatChains(directExcludes,taylorExcludesConstant);
    excludes = concatChains(taylorExcludesLinear,excludes);

    sollya_mpfi_clear(xZI2);

  } else {

    taylorExcludesConstant = evaluateI(constantTerm, func, xZI, prec, 1, hopitalrecursions+1, NULL, constantTheo,noExcludes);
    
    sollya_mpfi_sub(temp, x, xZI);
    sollya_mpfi_mul(temp2, temp, linearTerm);
    sollya_mpfi_add(resultTaylor, constantTerm, temp2);
    taylorExcludes = concatChains(taylorExcludesConstant, taylorExcludesLinear);
  
    if (deriv != NULL) 
      directExcludes = evaluateITaylorOnDiv(resultDirect, func, x, prec, recurse, directTheo,noExcludes);
    else 
      directExcludes = evaluateI(resultDirect, func, x, prec, 0, hopitalrecursions+1, NULL, directTheo,noExcludes);

    if (verbosity >= 15) {
      changeToWarningMode();
      sollyaPrintf("Information: Taylor evaluation: domain:\n");
      printInterval(x);
      sollyaPrintf("\nconstant term:\n");
      printInterval(constantTerm);
      sollyaPrintf("\nlinear term:\n");
      printInterval(linearTerm);
      sollyaPrintf("\ntranslated interval:\n");
      printInterval(temp);
      sollyaPrintf("\nmultiplied linear term:\n");
      printInterval(temp2);
      sollyaPrintf("\ndirect evaluation:\n");
      printInterval(resultDirect);
      sollyaPrintf("\n");
      restoreMode();
    }

    sollya_mpfi_get_left(rTl,resultTaylor);
    sollya_mpfi_get_right(rTr,resultTaylor);
    
    if (mpfr_number_p(rTl) && mpfr_number_p(rTr)) {
      sollya_mpfi_intersect(result,resultTaylor,resultDirect);
      excludes = concatChains(directExcludes,taylorExcludes);
      if (theo != NULL) {
	if (theo->functionType != POLYNOMIAL) {
	  theo->simplificationUsed = TAYLORPROOF;
	  theo->theoLeft = directTheo;
	  theo->theoLeftConstant = constantTheo;
	  theo->theoLeftLinear = linearTheo;
	  theo->leftDerivative = copyTree(deriv);
	  theo->boundLeft = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
	  sollya_mpfi_init2(*(theo->boundLeft),prec);
	  sollya_mpfi_set(*(theo->boundLeft),resultDirect);
	  theo->boundRight = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
	  sollya_mpfi_init2(*(theo->boundRight),prec);
	  sollya_mpfi_set(*(theo->boundRight),resultTaylor);
	  theo->boundLeftConstant = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
	  sollya_mpfi_init2(*(theo->boundLeftConstant),prec);
	  sollya_mpfi_set(*(theo->boundLeftConstant),constantTerm);
	  theo->boundLeftLinear = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
	  sollya_mpfi_init2(*(theo->boundLeftLinear),prec);
	  sollya_mpfi_set(*(theo->boundLeftLinear),linearTerm);
	  theo->xZ = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
	  sollya_mpfi_init2(*(theo->xZ),prec);
	  sollya_mpfi_set(*(theo->xZ),xZI);
	  theo->xMXZ = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
	  sollya_mpfi_init2(*(theo->xMXZ),prec);
	  sollya_mpfi_set(*(theo->xMXZ),temp);
	}
      }
    } else {
      sollya_mpfi_set(result,resultDirect);
      freeChain(taylorExcludes,freeMpfiPtr);
      excludes = directExcludes;
      if (theo != NULL) {
	if (theo->functionType != POLYNOMIAL) {
	  theo->simplificationUsed = IMPLICATION;
	  theo->theoLeft = directTheo;
	  theo->boundLeft = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
	  sollya_mpfi_init2(*(theo->boundLeft),prec);
	  sollya_mpfi_set(*(theo->boundLeft),resultDirect);
	  freeExprBoundTheo(constantTheo);
	  freeExprBoundTheo(linearTheo);
	}
      }
    }
  }

  sollya_mpfi_nan_normalize(result);

  if (theo != NULL) sollya_mpfi_set(*(theo->y),result);

  mpfr_clear(xZ);
  mpfr_clear(rTl);
  mpfr_clear(rTr);
  sollya_mpfi_clear(xZI);
  sollya_mpfi_clear(temp);
  sollya_mpfi_clear(temp2);
  sollya_mpfi_clear(constantTerm);
  sollya_mpfi_clear(linearTerm);
  sollya_mpfi_clear(resultTaylor);
  sollya_mpfi_clear(resultDirect);
  mpfr_clear(leftX);
  mpfr_clear(rightX);

  return excludes;
}



chain *findZerosUnsimplified(node *func, node *deriv, sollya_mpfi_t range, mp_prec_t prec, mpfr_t diam, chain **noZeroProofs) {
  mpfr_t rangeDiam, l,m,r;
  chain *res, *leftchain, *rightchain;
  sollya_mpfi_t *temp;
  sollya_mpfi_t lI, rI, y;
  chain *excludes;
  chain *leftProofs, *rightProofs;
  chain **leftProofsPtr, **rightProofsPtr;
  exprBoundTheo *theo;

  leftProofs = NULL;
  rightProofs = NULL;
  if (noZeroProofs != NULL) {
    leftProofsPtr = &leftProofs;
    rightProofsPtr = &rightProofs;
    theo = (exprBoundTheo *) safeCalloc(1,sizeof(exprBoundTheo));
  } else {
    leftProofsPtr = NULL;
    rightProofsPtr = NULL;
    theo = NULL;
  }

  mpfr_init2(rangeDiam,prec);
  sollya_mpfi_diam_abs(rangeDiam,range);

  mpfr_init2(l,prec);
  mpfr_init2(r,prec);
  sollya_mpfi_get_left(l,range);
  sollya_mpfi_get_right(r,range);
  mpfr_nextabove(l); mpfr_nextabove(l); mpfr_nextabove(l); mpfr_nextabove(l);

  if ((mpfr_cmp(rangeDiam,diam) <= 0) || (mpfr_cmp(l,r) >= 0)) {
    res = (chain *) safeMalloc(sizeof(chain));
    res->next = NULL;
    temp = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
    sollya_mpfi_init2(*temp,prec);
    sollya_mpfi_set(*temp,range);
    res->value = temp;
    if (theo != NULL) freeExprBoundTheo(theo);
  } else {
    sollya_mpfi_init2(y,prec);
    excludes = evaluateITaylor(y, func, deriv, range, prec, taylorrecursions, theo,1);
    freeChain(excludes,freeMpfiPtr);
    if (!sollya_mpfi_bounded_p(y)) {
      printMessage(1,"Warning: during zero-search the derivative of the function evaluated to NaN or Inf in the interval ");
      if (verbosity >= 1) { 	changeToWarningMode(); printInterval(range); restoreMode(); }
      printMessage(1,".\nThe function might not be continuously differentiable in this interval.\n");
    }
    if ((!sollya_mpfi_bounded_p(y)) || sollya_mpfi_has_zero(y)) {
      mpfr_init2(m,prec);
      sollya_mpfi_get_left(l,range);

      sollya_mpfi_mid(m,range);
      sollya_mpfi_init2(lI,prec);
      sollya_mpfi_init2(rI,prec);

      if (mpfr_cmp(l, m)<=0) sollya_mpfi_interv_fr(lI,l,m);
      else sollya_mpfi_interv_fr(lI,m,l);

      if (mpfr_cmp(m, r)<=0) sollya_mpfi_interv_fr(rI,m,r);
      else sollya_mpfi_interv_fr(rI,r,m);

      if (theo != NULL) freeExprBoundTheo(theo);
   
      leftchain = findZerosUnsimplified(func,deriv,lI,prec,diam,leftProofsPtr);
      rightchain = findZerosUnsimplified(func,deriv,rI,prec,diam,rightProofsPtr);

      res = concatChains(leftchain,rightchain);

      if (noZeroProofs != NULL) {
	*noZeroProofs = concatChains(leftProofs,rightProofs);
      }


      mpfr_clear(m);    
      sollya_mpfi_clear(lI);
      sollya_mpfi_clear(rI);
    } else {
      res = NULL;
      if (noZeroProofs != NULL) *noZeroProofs = addElement(*noZeroProofs,theo);
    }
    sollya_mpfi_clear(y);
  }
  mpfr_clear(l);
  mpfr_clear(r);
  mpfr_clear(rangeDiam);
  return res;
}

chain *findZeros(node *func, node *deriv, sollya_mpfi_t range, mp_prec_t prec, mpfr_t diam, noZeroTheo *theo) {
  node *funcSimplified, *derivSimplified;
  chain *temp;
  chain **noZeroProofs;

  funcSimplified = horner(func);
  derivSimplified = horner(deriv);

  if (theo != NULL) {
    theo->function = copyTree(func);
    theo->derivative = copyTree(deriv);
    theo->funcEqual = (equalityTheo *) safeMalloc(sizeof(equalityTheo));
    theo->funcEqual->expr1 = copyTree(func);
    theo->funcEqual->expr2 = copyTree(funcSimplified);
    theo->derivEqual = (equalityTheo *) safeMalloc(sizeof(equalityTheo));
    theo->derivEqual->expr1 = copyTree(deriv);
    theo->derivEqual->expr2 = copyTree(derivSimplified);
    noZeroProofs = &(theo->exprBoundTheos);
  } else {
    noZeroProofs = NULL;
  }

  printMessage(3,"Information: invoking the recursive interval zero search.\n");
  temp = findZerosUnsimplified(funcSimplified,derivSimplified,range,prec,diam,noZeroProofs);
  printMessage(3,"Information: the recursive interval zero search has finished.\n");
  
  free_memory(funcSimplified);
  free_memory(derivSimplified);

  return temp;
}



void printInterval(sollya_mpfi_t interval) {
  mpfr_t l,r;
  mp_prec_t prec;
  char *temp_string;

  prec = sollya_mpfi_get_prec(interval);
  mpfr_init2(l,prec);
  mpfr_init2(r,prec);
  sollya_mpfi_get_left(l,interval);
  sollya_mpfi_get_right(r,interval);

  if ((dyadic == 0) && (midpointMode == 1)) {
    temp_string = sprintMidpointMode(l, r);
    if (temp_string != NULL) {
      sollyaPrintf("%s ",temp_string);
      free(temp_string);
    } else {
      sollyaPrintf("[");
      printValue(&l);
      sollyaPrintf(";");
      printValue(&r);
      sollyaPrintf("]");
    }
  } else {
    sollyaPrintf("[");
    printValue(&l);
    sollyaPrintf(";");
    printValue(&r);
    sollyaPrintf("]");
  }

  mpfr_clear(l);
  mpfr_clear(r);
}


void fprintInterval(FILE *fd, sollya_mpfi_t interval) {
  mpfr_t l,r;
  mp_prec_t prec;

  
  prec = sollya_mpfi_get_prec(interval);
  mpfr_init2(l,prec);
  mpfr_init2(r,prec);
  sollya_mpfi_get_left(l,interval);
  sollya_mpfi_get_right(r,interval);
  sollyaFprintf(fd,"[");
  fprintValue(fd,l);
  sollyaFprintf(fd,";");
  fprintValue(fd,r);
  sollyaFprintf(fd,"]");

  mpfr_clear(l);
  mpfr_clear(r);
}


chain *joinAdjacentIntervals(chain *intervals, mpfr_t diam) {
  chain *newChain, *curr;
  sollya_mpfi_t *tempI;
  mp_prec_t prec, p;
  mpfr_t newLeft, newRight, l,r, mpfr_temp;

  if (intervals == NULL) return NULL;
  if (intervals->next == NULL) {
    tempI = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
    sollya_mpfi_init2(*tempI,sollya_mpfi_get_prec(*((sollya_mpfi_t *) (intervals->value))));
    sollya_mpfi_set(*tempI,*((sollya_mpfi_t *) (intervals->value)));
    newChain = addElement(NULL,tempI);
    return newChain;
  }

  prec = sollya_mpfi_get_prec(*((sollya_mpfi_t *) (intervals->value)));
  curr = intervals->next;
  while (curr != NULL) {
    p = sollya_mpfi_get_prec(*((sollya_mpfi_t *) (curr->value)));
    if (p > prec) prec = p;
    curr = curr->next;
  }


  mpfr_init2(newLeft,prec);
  mpfr_init2(newRight,prec);
  sollya_mpfi_get_left(newLeft,*((sollya_mpfi_t *) (intervals->value)));
  sollya_mpfi_get_right(newRight,*((sollya_mpfi_t *) (intervals->value)));
  mpfr_init2(l,prec);
  mpfr_init2(r,prec);
  mpfr_init2(mpfr_temp,prec);

  newChain = NULL;
  curr = intervals->next;
  while (curr != NULL) {
    sollya_mpfi_get_left(l,*((sollya_mpfi_t *) (curr->value)));
    sollya_mpfi_get_right(r,*((sollya_mpfi_t *) (curr->value)));
    mpfr_sub(mpfr_temp,r,newLeft,GMP_RNDN);
    if ((mpfr_cmp(l,newRight) == 0) && (mpfr_cmp(mpfr_temp,diam) < 0)) {
      mpfr_set(newRight,r,GMP_RNDN);
    } else {
      tempI = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
      sollya_mpfi_init2(*tempI,prec);
      sollya_mpfi_interv_fr(*tempI,newLeft,newRight);
      newChain = addElement(newChain,tempI);
      mpfr_set(newLeft,l,GMP_RNDN);
      mpfr_set(newRight,r,GMP_RNDN);
    }
    curr = curr->next;
  }
  tempI = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
  sollya_mpfi_init2(*tempI,prec);
  sollya_mpfi_interv_fr(*tempI,newLeft,newRight);
  newChain = addElement(newChain,tempI);

  mpfr_clear(l);
  mpfr_clear(r);
  mpfr_clear(newLeft);
  mpfr_clear(newRight);
  mpfr_clear(mpfr_temp);
  return newChain;
}

chain *joinAdjacentIntervalsMaximally(chain *intervals) {
  chain *newChain, *curr;
  sollya_mpfi_t *tempI;
  mp_prec_t prec, p;
  mpfr_t newLeft, newRight, l,r, mpfr_temp;

  if (intervals == NULL) return NULL;
  if (intervals->next == NULL) {
    tempI = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
    sollya_mpfi_init2(*tempI,sollya_mpfi_get_prec(*((sollya_mpfi_t *) (intervals->value))));
    sollya_mpfi_set(*tempI,*((sollya_mpfi_t *) (intervals->value)));
    newChain = addElement(NULL,tempI);
    return newChain;
  }

  prec = sollya_mpfi_get_prec(*((sollya_mpfi_t *) (intervals->value)));
  curr = intervals->next;
  while (curr != NULL) {
    p = sollya_mpfi_get_prec(*((sollya_mpfi_t *) (curr->value)));
    if (p > prec) prec = p;
    curr = curr->next;
  }


  mpfr_init2(newLeft,prec);
  mpfr_init2(newRight,prec);
  sollya_mpfi_get_left(newLeft,*((sollya_mpfi_t *) (intervals->value)));
  sollya_mpfi_get_right(newRight,*((sollya_mpfi_t *) (intervals->value)));
  mpfr_init2(l,prec);
  mpfr_init2(r,prec);
  mpfr_init2(mpfr_temp,prec);

  newChain = NULL;
  curr = intervals->next;
  while (curr != NULL) {
    sollya_mpfi_get_left(l,*((sollya_mpfi_t *) (curr->value)));
    sollya_mpfi_get_right(r,*((sollya_mpfi_t *) (curr->value)));
    mpfr_sub(mpfr_temp,r,newLeft,GMP_RNDN);
    if (mpfr_cmp(l,newRight) == 0) {
      mpfr_set(newRight,r,GMP_RNDN);
    } else {
      tempI = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
      sollya_mpfi_init2(*tempI,prec);
      sollya_mpfi_interv_fr(*tempI,newLeft,newRight);
      newChain = addElement(newChain,tempI);
      mpfr_set(newLeft,l,GMP_RNDN);
      mpfr_set(newRight,r,GMP_RNDN);
    }
    curr = curr->next;
  }
  tempI = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
  sollya_mpfi_init2(*tempI,prec);
  sollya_mpfi_interv_fr(*tempI,newLeft,newRight);
  newChain = addElement(newChain,tempI);

  mpfr_clear(l);
  mpfr_clear(r);
  mpfr_clear(newLeft);
  mpfr_clear(newRight);
  mpfr_clear(mpfr_temp);
  return newChain;
}



chain *excludeIntervals(chain *mainIntervals, chain *excludeIntervals) {
  chain *curr, *previous, *curr2, *temp;
  sollya_mpfi_t *interval, *exclude;
  mp_prec_t prec, p;
  mpfr_t il, ir, el, er;

  if (mainIntervals == NULL) return NULL;
  if (excludeIntervals == NULL) return mainIntervals;

  prec = 1;
  curr = mainIntervals;
  while (curr != NULL) {
    p = sollya_mpfi_get_prec(*((sollya_mpfi_t *) curr->value));
    if (p > prec) prec = p;
    curr = curr->next;
  }
  curr = excludeIntervals;
  while (curr != NULL) {
    p = sollya_mpfi_get_prec(*((sollya_mpfi_t *) curr->value));
    if (p > prec) prec = p;
    curr = curr->next;
  }

  prec += 5;

  mpfr_init2(il,prec);
  mpfr_init2(ir,prec);
  mpfr_init2(el,prec);
  mpfr_init2(er,prec);

  curr2 = excludeIntervals;
  while (curr2 != NULL) {
    exclude = (sollya_mpfi_t *) (curr2->value);
    sollya_mpfi_get_left(el,*exclude);
    sollya_mpfi_get_right(er,*exclude);
    curr = mainIntervals;
    previous = NULL;
    while (curr != NULL) {
      interval = (sollya_mpfi_t *) (curr->value);
      sollya_mpfi_get_left(il,*interval);
      sollya_mpfi_get_right(ir,*interval);
      if ((mpfr_cmp(el,ir) < 0) && (mpfr_cmp(il,er) < 0)) { /* [il;ir] inter [el;er] != empty */
	if ((mpfr_cmp(il,el) < 0) && (mpfr_cmp(er,ir) < 0)) {
	  /* We must produce two intervals [il;el] and [er;ir] */
	  sollya_mpfi_interv_fr(*interval,il,el);
	  interval = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
	  sollya_mpfi_init2(*interval,prec);
	  sollya_mpfi_interv_fr(*interval,er,ir);
	  temp = (chain *) safeMalloc(sizeof(chain));
	  temp->value = interval;
	  temp->next = curr->next;
	  curr->next = temp;
	} else {
	  if (mpfr_cmp(il,el) < 0) {
	    /* We must produce one interval [il;el] */
	    sollya_mpfi_interv_fr(*interval,il,el);
	  } else {
	    if (mpfr_cmp(er,ir) < 0) {
	      /* We must produce one interval [er;ir] */
	      sollya_mpfi_interv_fr(*interval,er,ir);
	    } else {
	      /* We must remove the interval completely */
	      if (previous != NULL) {
		/* We are not the first interval in the chain */
		previous->next = curr->next;
		sollya_mpfi_clear(*interval);
		free(interval);
		free(curr);
		curr = previous;
	      } else {
		/* We are the first interval in the chain */
		if (curr->next != NULL) {
		  /* We have a successor that will become the head of the chain */
		  mainIntervals = curr->next;
		  sollya_mpfi_clear(*interval);
		  free(interval);
		  free(curr);
		  curr = mainIntervals;
		} else {
		  /* We are the first and the last element in the chain, which will be empty */
		  
		  sollya_mpfi_clear(*interval);
		  free(interval);
		  free(curr);
		  mpfr_clear(il);
		  mpfr_clear(ir);
		  mpfr_clear(el);
		  mpfr_clear(er);
		  return NULL;
		}
	      }
	    }
	  }
	}
      }
      previous = curr;
      curr = curr->next;
    }
    curr2 = curr2->next;
  }

  mpfr_clear(il);
  mpfr_clear(ir);
  mpfr_clear(el);
  mpfr_clear(er);

  return mainIntervals;
}


void infnormI(sollya_mpfi_t infnormval, node *func, node *deriv, 
	      node *numeratorDeriv, node *derivNumeratorDeriv,
	      sollya_mpfi_t range, mp_prec_t prec, mpfr_t diam, 
	      chain *intervalsToExclude,
	      chain **mightExcludes,
	      infnormTheo *theo) {
  chain *curr, *zeros, *tempChain, *tempChain2, *tempChain3;
  sollya_mpfi_t *currInterval;
  sollya_mpfi_t evalFuncOnInterval, lInterv, rInterv;
  mpfr_t innerLeft, innerRight, outerLeft, outerRight, l, r, tl, tr;
  mpfr_t diamJoin;
  mp_prec_t rangePrec;
  chain *excludes, *excludesTemp;
  int i; 
  noZeroTheo *noZeros;
  exprBoundTheo *evalLeftBound, *evalRightBound, *currZeroTheo;

  currZeroTheo = NULL;
  if (theo != NULL) {
    theo->function = copyTree(func);
    theo->derivative = copyTree(deriv);
    theo->numeratorOfDerivative = copyTree(numeratorDeriv);
    theo->derivativeOfNumeratorOfDerivative = copyTree(derivNumeratorDeriv);
    theo->excludedIntervals = copyChain(intervalsToExclude,copyMpfiPtr);
    noZeros = (noZeroTheo *) safeCalloc(1,sizeof(noZeroTheo));
    theo->noZeros = noZeros;
    evalLeftBound = (exprBoundTheo *) safeCalloc(1,sizeof(exprBoundTheo));
    nullifyExprBoundTheo(evalLeftBound);
    evalRightBound = (exprBoundTheo *) safeCalloc(1,sizeof(exprBoundTheo));
    nullifyExprBoundTheo(evalRightBound);
    theo->evalLeftBound = evalLeftBound;
    theo->evalRightBound = evalRightBound;
    theo->domain = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
    theo->infnorm = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
    theo->evalOnZeros = NULL;
    sollya_mpfi_init2(*(theo->domain),sollya_mpfi_get_prec(range));
    sollya_mpfi_init2(*(theo->infnorm),sollya_mpfi_get_prec(infnormval));
    sollya_mpfi_set(*(theo->domain),range);
  } else {
    noZeros = NULL;
    evalLeftBound = NULL;
    evalRightBound = NULL;
  }
  
  mpfr_init2(innerLeft, prec);
  mpfr_init2(innerRight, prec);
  mpfr_init2(outerLeft, prec);
  mpfr_init2(outerRight, prec);
  mpfr_init2(tl, prec);
  mpfr_init2(tr, prec);
  sollya_mpfi_init2(evalFuncOnInterval,prec);

  rangePrec = sollya_mpfi_get_prec(range);
  mpfr_init2(l,rangePrec);
  mpfr_init2(r,rangePrec);
  sollya_mpfi_get_left(l,range);
  sollya_mpfi_get_right(r,range);
  sollya_mpfi_init2(rInterv, rangePrec);
  sollya_mpfi_init2(lInterv, rangePrec);
  sollya_mpfi_set_fr(rInterv,r);
  sollya_mpfi_set_fr(lInterv,l);

  excludes = evaluateITaylor(evalFuncOnInterval, func, deriv, lInterv, prec, taylorrecursions, evalLeftBound,0); 
  sollya_mpfi_get_left(outerLeft,evalFuncOnInterval);
  sollya_mpfi_get_right(outerRight,evalFuncOnInterval);
  mpfr_set(innerLeft,outerRight,GMP_RNDU);
  mpfr_set(innerRight,outerLeft,GMP_RNDD);
  excludesTemp = evaluateITaylor(evalFuncOnInterval, func, deriv, rInterv, prec, taylorrecursions, evalRightBound,0); 
  excludes = concatChains(excludes,excludesTemp);
  sollya_mpfi_get_left(tl,evalFuncOnInterval);
  sollya_mpfi_get_right(tr,evalFuncOnInterval);
  sollya_mpfr_min(outerLeft,outerLeft,tl,GMP_RNDD);
  sollya_mpfr_max(outerRight,outerRight,tr,GMP_RNDU);
  sollya_mpfr_min(innerLeft,innerLeft,tr,GMP_RNDU);
  sollya_mpfr_max(innerRight,innerRight,tl,GMP_RNDD); 
 
  printMessage(3,"Information: invoking interval zero search.\n");
  tempChain = findZeros(numeratorDeriv,derivNumeratorDeriv,range,prec,diam,noZeros); 

  printMessage(3,"Information: interval zero search is done.\n");
  mpfr_init2(diamJoin,prec);
  mpfr_mul_2ui(diamJoin,diam,3,GMP_RNDN);
  tempChain2 = joinAdjacentIntervals(tempChain,diamJoin);
  tempChain3 = copyChain(tempChain2,copyMpfiPtr);
  mpfr_mul_2ui(diamJoin,diamJoin,2,GMP_RNDN);
  zeros = joinAdjacentIntervals(tempChain3,diamJoin);
  mpfr_clear(diamJoin);

  zeros = excludeIntervals(zeros,intervalsToExclude);

  i = 0;
  for (curr=zeros;curr!=NULL;curr=curr->next) i++;
  printMessage(2,
	  "Information: %d interval(s) have (has) been found that possibly contain(s) the zeros of the derivative.\n",i);

  curr = zeros;
  while (curr != NULL) {

    if (verbosity >= 7) {
      changeToWarningMode();
      sollyaPrintf("Information:\nCurrent inner enclosure: [");
      printValue(&innerLeft);
      sollyaPrintf(";");
      printValue(&innerRight);
      sollyaPrintf("]\nCurrent outer enclosure: [");
      printValue(&outerLeft);
      sollyaPrintf(";");
      printValue(&outerRight);
      sollyaPrintf("]\n");
      restoreMode();
    }

    if (theo != NULL) {
      currZeroTheo = (exprBoundTheo *) safeCalloc(1,sizeof(exprBoundTheo));
      nullifyExprBoundTheo(currZeroTheo);
    } else {
      currZeroTheo = NULL;
    }
    currInterval = ((sollya_mpfi_t *) (curr->value));
    excludesTemp = evaluateITaylor(evalFuncOnInterval, func, deriv, *currInterval, prec, taylorrecursions, currZeroTheo,0); 

    if (verbosity >= 7) {
      changeToWarningMode();
      sollyaPrintf("Information: The function evaluates on\n");
      printInterval(*currInterval);
      sollyaPrintf(" to\n");
      printInterval(evalFuncOnInterval);
      sollyaPrintf("\n");
      restoreMode();
    }

    excludes = concatChains(excludes,excludesTemp);
    sollya_mpfi_get_left(tl,evalFuncOnInterval);
    sollya_mpfi_get_right(tr,evalFuncOnInterval);

    if (theo != NULL) {
      theo->evalOnZeros = addElement(theo->evalOnZeros,currZeroTheo);
    }

    if (mpfr_nan_p(tl) || mpfr_nan_p(tr)) {
      printMessage(1,"Warning: NaNs occurred during the interval evaluation of the zeros of the derivative.\n");
    }

    sollya_mpfr_min(outerLeft,outerLeft,tl,GMP_RNDD);
    sollya_mpfr_max(outerRight,outerRight,tr,GMP_RNDU);
    sollya_mpfr_min(innerLeft,innerLeft,tr,GMP_RNDU); 
    sollya_mpfr_max(innerRight,innerRight,tl,GMP_RNDD); 
    curr = curr->next;
  }

  freeChain(zeros,freeMpfiPtr);
  freeChain(tempChain,freeMpfiPtr);
  freeChain(tempChain2,freeMpfiPtr);
  freeChain(tempChain3,freeMpfiPtr);

  if (mpfr_cmp(innerLeft,innerRight) >= 0) {
    mpfr_neg(outerLeft,outerLeft,GMP_RNDN);
    sollya_mpfr_max(tr,outerLeft,outerRight,GMP_RNDU);
    mpfr_set_d(tl,0.0,GMP_RNDD);

    if (mpfr_cmp(tl, tr)<=0) sollya_mpfi_interv_fr(infnormval,tl,tr);
    else sollya_mpfi_interv_fr(infnormval,tr,tl);

  } else {
    mpfr_neg(innerLeft,innerLeft,GMP_RNDN);
    mpfr_neg(outerLeft,outerLeft,GMP_RNDN);
    sollya_mpfr_max(tl,innerLeft,innerRight,GMP_RNDD);
    sollya_mpfr_max(tr,outerLeft,outerRight,GMP_RNDU);
    if (mpfr_cmp(tl, tr)<=0) sollya_mpfi_interv_fr(infnormval,tl,tr);
    else sollya_mpfi_interv_fr(infnormval,tr,tl);
  }

  if (mightExcludes == NULL) {
    freeChain(excludes,freeMpfiPtr);
  } else {
    *mightExcludes = excludes;
  }

  if (theo != NULL) {
    sollya_mpfi_set(*(theo->infnorm),infnormval);
  }

  mpfr_clear(tl);
  mpfr_clear(tr);
  sollya_mpfi_clear(lInterv);
  sollya_mpfi_clear(rInterv);
  mpfr_clear(l);
  mpfr_clear(r);
  sollya_mpfi_clear(evalFuncOnInterval);
  mpfr_clear(innerLeft);
  mpfr_clear(innerRight);
  mpfr_clear(outerLeft);
  mpfr_clear(outerRight);
}


int isTrivialInfnormCase(rangetype result, node *func) {
  int isTrivial;
  node *simplifiedFunc, *numerator, *denominator;

  isTrivial = 0;
  simplifiedFunc = horner(func);

  if (simplifiedFunc->nodeType == CONSTANT) {
    mpfr_set(*(result.a),*(simplifiedFunc->value),GMP_RNDD);
    mpfr_set(*(result.b),*(simplifiedFunc->value),GMP_RNDU);
    isTrivial = 1;
  } else {
    if (simplifiedFunc->nodeType == SUB) {
      if (isSyntacticallyEqual(simplifiedFunc->child1,simplifiedFunc->child2)) {
	mpfr_set_d(*(result.a),0.0,GMP_RNDN);
	mpfr_set_d(*(result.b),0.0,GMP_RNDN);
	isTrivial = 1;
      }
    } else {
      if (getNumeratorDenominator(&numerator, &denominator, simplifiedFunc)) {
	if ((numerator->nodeType == CONSTANT) &&
	    mpfr_zero_p(*(numerator->value))) {
	  mpfr_set_d(*(result.a),0.0,GMP_RNDN);
	  mpfr_set_d(*(result.b),0.0,GMP_RNDN);
	  isTrivial = 1;
	} else {
	  if (isSyntacticallyEqual(numerator, denominator)) {
	    mpfr_set_d(*(result.a),1.0,GMP_RNDN);
	    mpfr_set_d(*(result.b),1.0,GMP_RNDN);
	    isTrivial = 1;
	  }
	}
	free_memory(denominator);
      } 
      free_memory(numerator);
    }
  }

  free_memory(simplifiedFunc);
  return isTrivial;
}

void uncertifiedInfnorm(mpfr_t result, node *f, mpfr_t a, mpfr_t b, unsigned long int points, mp_prec_t prec) {
  mpfr_t current_x, x1, x2, x3, step, y1, y2, y3, max, cutoff;
  mpfr_t ystar, y1diff, y3diff, xstar;
  mpfr_t zero_mpfr;
  mpfr_t perturb;

  mp_prec_t prec_bound = prec;
  node *f_diff, *f_diff2;
  int r;
  int count=0;
  int stop_algo = 0;
  const mp_prec_t INIT_PREC = 20;
  gmp_randstate_t random_state;


  if (mpfr_get_prec(a) > prec_bound) prec_bound = mpfr_get_prec(a);
  if (mpfr_get_prec(b) > prec_bound) prec_bound = mpfr_get_prec(b);


  /**************************** Dealing with special cases ****************************/
  if ( (!mpfr_number_p(a)) || (!mpfr_number_p(b)) ) {
    printMessage(1,"Warning: a bound of the interval is infinite or NaN.\n");
    printMessage(1,"This command cannot handle such intervals.\n");
    mpfr_set_nan(result);
    return;
  }

  if (mpfr_equal_p(a,b)) {
    printMessage(1,"Warning: the given interval is reduced to one point.\n");
    evaluateFaithful(result,f,a,prec);
    mpfr_abs(result,result,GMP_RNDU);
    return;
  }

  if (mpfr_greater_p(a,b)) {
    printMessage(1,"Warning: the interval is empty.\n");
    mpfr_set_d(result,0.,GMP_RNDN);
    return;
  }

  if (isConstant(f)) {
    printMessage(1,"Warning: the expression is constant.\n");
    evaluateFaithful(result,f,a,prec);
    mpfr_abs(result,result,GMP_RNDU);
    return;
  }
  /************************************************************************************/


  gmp_randinit_default(random_state);
  gmp_randseed_ui(random_state, 65845285);
  mpfr_init2(perturb, prec);

  mpfr_init2(zero_mpfr, 53);
  mpfr_set_d(zero_mpfr, 0., GMP_RNDN);

  mpfr_init2(step, prec);
  mpfr_sub(step, b, a, GMP_RNDU); /* since a<b and step is computed with rounding upwards, step>O */
  mpfr_div_ui(step, step, points, GMP_RNDN);

  mpfr_init2(current_x, prec_bound);
  mpfr_init2(x1, prec_bound);
  mpfr_init2(x2, prec_bound);
  mpfr_init2(x3, prec_bound);
  mpfr_init2(y1, prec);
  mpfr_init2(y2, prec);
  mpfr_init2(y3, prec);

  mpfr_init2(ystar, prec);
  mpfr_init2(y1diff, prec);
  mpfr_init2(y3diff, prec);
  mpfr_init2(xstar, prec_bound);

  mpfr_init2(max, prec);
  mpfr_set_d(max, 0., GMP_RNDN);
  mpfr_init2(cutoff, prec);
  mpfr_set_d(cutoff, 0., GMP_RNDN);

  f_diff = differentiate(f);
  f_diff2 = NULL;

  
  /* Initial value of x1 */
  mpfr_set(x1, a, GMP_RNDN); /* exact */
  mpfr_set(current_x, a, GMP_RNDN);
  do {
    count++;
    if (mpfr_greaterequal_p(x1,b)) {
      mpfr_set(x1, b, GMP_RNDN); /* exact */
      stop_algo = 1;
    }
    r = evaluateFaithfulWithCutOffFast(y1, f, f_diff, x1, cutoff, INIT_PREC);
    if (r==0) mpfr_set_d(y1, 0. , GMP_RNDN);

    if (!mpfr_number_p(y1)) {
      if(verbosity >= 1) {
        changeToWarningMode();
	sollyaPrintf("Warning: the evaluation of the given function in ");
	printValue(&x1); sollyaPrintf(" gives NaN.\n");
	sollyaPrintf("This (possibly maximum) point will be excluded from the infnorm result.\n");
        restoreMode();
      }
      mpfr_add(current_x, current_x, step, GMP_RNDU); /* rounding up ensures that x1(new) > x1(old) */
      mpfr_urandomb(perturb, random_state); mpfr_mul_2ui(perturb, perturb, 1, GMP_RNDN);
      mpfr_sub_ui(perturb, perturb, 1, GMP_RNDN); mpfr_div_2ui(perturb, perturb, 2, GMP_RNDN);
      mpfr_mul(perturb, perturb, step, GMP_RNDN); // perturb \in [-step/4; step/4]
      mpfr_add(x1, current_x, perturb, GMP_RNDU);
    }
  } while ( (!mpfr_number_p(y1)) && (!stop_algo) );

  mpfr_abs(max, y1, GMP_RNDU);
  if (verbosity >= 3) { 
    changeToWarningMode();
    sollyaPrintf("Information: current max is "); printValue(&max);
    sollyaPrintf(" and is reached at "); printMpfr(x1);
    restoreMode();
  }

  mpfr_div_2ui(cutoff, max, 1, GMP_RNDU);


  /* Initial value of x2 */
  do {
    count++;
    mpfr_add(current_x, current_x, step, GMP_RNDU); /* rounding up ensures that x2 > x1 */
    mpfr_urandomb(perturb, random_state); mpfr_mul_2ui(perturb, perturb, 1, GMP_RNDN);
    mpfr_sub_ui(perturb, perturb, 1, GMP_RNDN); mpfr_div_2ui(perturb, perturb, 2, GMP_RNDN);
    mpfr_mul(perturb, perturb, step, GMP_RNDN); // perturb \in [-step/4; step/4]
    mpfr_add(x2, current_x, perturb, GMP_RNDU);

    if (mpfr_greaterequal_p(x2,b)) {
      mpfr_set(x2, b, GMP_RNDN); /* exact */
      stop_algo = 1;
    }
    
    r = evaluateFaithfulWithCutOffFast(y2, f, f_diff, x2, cutoff, INIT_PREC);
    if (r==2) mpfr_set_d(y2, 0. , GMP_RNDN); /* under the cutoff */
    
    if (!mpfr_number_p(y2)) {
      if(verbosity >= 1) {
	changeToWarningMode();
	sollyaPrintf("Warning: the evaluation of the given function in ");
	printValue(&x2); sollyaPrintf(" gives NaN.\n");
	sollyaPrintf("This (possibly maximum) point will be excluded from the infnorm result.\n");
        restoreMode();
      }
    }
  } while ( ( (!mpfr_number_p(y2)) || mpfr_equal_p(y1,y2) )
	    &&
	    (!stop_algo)
	    );

  if (mpfr_cmpabs(y2, max) > 0) { /* evaluates to false when y2=NaN */
    mpfr_abs(max, y2, GMP_RNDU); 
    if (verbosity >= 3) { 
      changeToWarningMode();
      sollyaPrintf("Information: current max is "); printValue(&max);
      sollyaPrintf(" and is reached at "); printMpfr(x2);
      restoreMode();
    }
    mpfr_div_2ui(cutoff, max, 1, GMP_RNDU);
  }

  /* Remark: at this point, it is possible that y1=y2 and stop_algo is true. */
  /* In this case, it means that f is constant over the interval and it is   */
  /* hence not necessary to run Newton's algorithm anyway.                   */

  /* Main loop */
  while(!stop_algo) {
    do {
      count++;
      if (verbosity >= 2) {
          if( count % 100 == 0) printMessage(2,"Information: %d out of %d points have been handled.\n",count,points);
      }

      mpfr_add(current_x, current_x, step, GMP_RNDU); /* rounding up ensures that x3 > x2 */
      mpfr_urandomb(perturb, random_state); mpfr_mul_2ui(perturb, perturb, 1, GMP_RNDN);
      mpfr_sub_ui(perturb, perturb, 1, GMP_RNDN); mpfr_div_2ui(perturb, perturb, 2, GMP_RNDN);
      mpfr_mul(perturb, perturb, step, GMP_RNDN); // perturb \in [-step/4; step/4]
      mpfr_add(x3, current_x, perturb, GMP_RNDU);

      if (mpfr_greaterequal_p(x3,b)) {
	mpfr_set(x3, b, GMP_RNDN); /* exact */
	stop_algo = 1;
      }
      
      r = evaluateFaithfulWithCutOffFast(y3, f, f_diff, x3, cutoff, INIT_PREC);
      if (r==2) mpfr_set_d(y3, 0. , GMP_RNDN); /* under the cutoff */
      
      if (!mpfr_number_p(y3)) {
	if(verbosity >= 1) {
	  changeToWarningMode();
	  sollyaPrintf("Warning: the evaluation of the given function in ");
	  printValue(&x3); sollyaPrintf(" gives NaN.\n");
	  sollyaPrintf("This (possibly maximum) point will be excluded from the infnorm result.\n");
          restoreMode();
	}
      }
    } while ( ( (!mpfr_number_p(y3))|| mpfr_equal_p(y2,y3) )
	      &&
	      (!stop_algo)
	      );
    
    if (mpfr_cmpabs(y3, max) > 0) { /* evaluates to false when y3=NaN */
      mpfr_abs(max, y3, GMP_RNDU); 
      if (verbosity >= 3) { 
	changeToWarningMode();
	sollyaPrintf("Information: current max is "); printValue(&max);
	sollyaPrintf(" and is reached at "); printMpfr(x3);
	restoreMode();
      }
      mpfr_div_2ui(cutoff, max, 1, GMP_RNDU);
    }
    
    /* Call to Newton's algorithm if necessary */
    if ( (mpfr_cmpabs(y2,y1)>=0) && (mpfr_cmpabs(y2,y3)>=0) && (mpfr_cmp_d(y2,0.)!=0) ) {
      if (f_diff2 == NULL) f_diff2 = differentiate(f_diff);

      r = evaluateFaithfulWithCutOffFast(y1diff, f_diff, f_diff2, x1, zero_mpfr, prec+10);
      if (r==0) mpfr_set_d(y1diff, 0. , GMP_RNDN);

      r = evaluateFaithfulWithCutOffFast(y3diff, f_diff, f_diff2, x3, zero_mpfr, prec+10);
      if (r==0) mpfr_set_d(y3diff, 0. , GMP_RNDN);
      
      if ( (!mpfr_number_p(y1diff)) || (!mpfr_number_p(y3diff)) ) {
	if(verbosity >= 1) {
          changeToWarningMode();
	  sollyaPrintf("Warning: the evaluation of the derivative of the given function in ");
	  printValue(&x1); sollyaPrintf(" or "); printValue(&x3); sollyaPrintf(" gives NaN.\n");
	  sollyaPrintf("Newton's algorithm will not be used on this interval\n");
          restoreMode();
	}
      }
      else if(mpfr_sgn(y1diff)*mpfr_sgn(y3diff)<0) { /* If y1diff=0 or y3diff=0, there is no need to */
                                                     /* use Newton's algorithm since we already have */
                                                     /* the zero. Moreover, note that y1 and y3 have */
                                                     /* already been taken into account in max.      */
	findZero(xstar, f_diff, f_diff2, x1, x3, mpfr_sgn(y1diff), NULL, 0, prec_bound);
	
	/* If xstar = NaN, a warning has already been produced by Newton's algorithm. */
	/* There is no need to print a warning again here.                            */
	if(mpfr_number_p(xstar)) { 
	  r = evaluateFaithfulWithCutOffFast(ystar, f, f_diff, xstar, cutoff, prec+10);
	  if (r==2) mpfr_set_d(ystar, 0. , GMP_RNDN); /* under the cutoff */
	  
	  if (!mpfr_number_p(ystar)) {
	    if(verbosity >= 1) {
	      changeToWarningMode();
	      sollyaPrintf("Warning: the evaluation of the given function in ");
	      printValue(&xstar); sollyaPrintf(" gives NaN.\n");
	      sollyaPrintf("This (possibly maximum) point will be excluded from the infnorm result.\n");
              restoreMode();
	    }
	  }
	  if (mpfr_cmpabs(ystar, max) > 0) { /* evaluates to false when ystar=NaN */
	    mpfr_abs(max, ystar, GMP_RNDU); 
	    if (verbosity >= 3) { 
	      changeToWarningMode();
	      sollyaPrintf("Information: current max is "); printValue(&max);
	      sollyaPrintf(" and is reached at "); printMpfr(xstar);
	      restoreMode();
	    }
	    mpfr_div_2ui(cutoff, max, 1, GMP_RNDU);
	  }
	}
      }    
    }
    
    mpfr_set(x1,x2,GMP_RNDN); /* exact */
    mpfr_set(y1,y2,GMP_RNDN); /* exact */
    mpfr_set(x2,x3,GMP_RNDN); /* exact */
    mpfr_set(y2,y3,GMP_RNDN); /* exact */
  }

  mpfr_set(result, max, GMP_RNDU);

  free_memory(f_diff);
  free_memory(f_diff2);

  gmp_randclear(random_state);
  mpfr_clear(perturb);
  mpfr_clear(current_x);
  mpfr_clear(x1);
  mpfr_clear(x2);
  mpfr_clear(x3);
  mpfr_clear(step);
  mpfr_clear(y1);
  mpfr_clear(y2);
  mpfr_clear(y3);
  mpfr_clear(ystar);
  mpfr_clear(y1diff);
  mpfr_clear(y3diff);
  mpfr_clear(xstar);
  mpfr_clear(zero_mpfr);
  mpfr_clear(max);
  mpfr_clear(cutoff);
}



rangetype infnorm(node *func, rangetype range, chain *excludes, 
		  mp_prec_t prec, mpfr_t diam, FILE *proof) {
  rangetype res;
  sollya_mpfi_t rangeI, resI;
  sollya_mpfi_t *excludeI;
  node *deriv, *numeratorDeriv, *derivNumeratorDeriv, *denominatorDeriv, *derivDenominatorDeriv;
  mpfr_t rangeDiameter, z, ya,yb;
  chain *mightExcludes, *curr, *secondMightExcludes, *initialExcludes;
  int newtonWorked;
  mp_prec_t p, p2;
  infnormTheo *theo;
  int freeInitialExcludes;



  freeInitialExcludes = 1;
  res.a = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
  res.b = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
  mpfr_init2(*(res.a),prec);
  mpfr_init2(*(res.b),prec);

  if ((!mpfr_number_p(*(range.a))) || (!mpfr_number_p(*(range.b)))) {
    printMessage(1,"Warning: the bounds of the range an infinity norm is to be computed on are not numbers.\n");
    if (proof != NULL) {
      printMessage(1,"Warning: no proof will be generated.\n");
    }
    mpfr_set_d(*(res.a),0.0,GMP_RNDN);
    mpfr_set_inf(*(res.b),1);
    return res;
  }
  
  if ((mpfr_cmp(*(range.a),*(range.b)) == 0) && (proof == NULL)) {
    evaluateRangeFunctionFast(res, func, NULL, range, prec);
    mpfr_abs(*(res.a),*(res.a),GMP_RNDN);
    mpfr_abs(*(res.b),*(res.b),GMP_RNDN);
    if (mpfr_cmp(*(res.a),*(res.b)) > 0) {
      mpfr_init2(z,prec);
      mpfr_set(z,*(res.b),GMP_RNDN);
      mpfr_set(*(res.b),*(res.a),GMP_RNDN);
      mpfr_set(*(res.a),z,GMP_RNDN);
      mpfr_clear(z);
    }
    return res;
  }

  if (isTrivialInfnormCase(res, func)) {
    if (proof != NULL) {
      printMessage(1,"Warning: the infnorm on the given function is trivially calculable.\n");
      printMessage(1,"No proof will be generated.\n");
    }
    return res;
  }

  curr = excludes;
  initialExcludes = NULL;
  while (curr != NULL) {
    excludeI = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
    p = mpfr_get_prec(*(((rangetype *) curr->value)->a));
    p2 = mpfr_get_prec(*(((rangetype *) curr->value)->b));
    if (p2 > p) p = p2;
    if (prec > p) p = prec;
    sollya_mpfi_init2(*excludeI,p);
    sollya_mpfi_interv_fr(*excludeI,*(((rangetype *) curr->value)->a),*(((rangetype *) curr->value)->b));
    initialExcludes = addElement(initialExcludes,(void *) excludeI);
    curr = curr->next;
  }

  sollya_mpfi_init2(rangeI,prec);
  sollya_mpfi_init2(resI,prec);
  mpfr_init2(rangeDiameter,prec);
  mpfr_sub(rangeDiameter,*(range.b),*(range.a),GMP_RNDD);
  mpfr_mul(rangeDiameter,rangeDiameter,diam,GMP_RNDD);
  sollya_mpfi_interv_fr(rangeI,*(range.a),*(range.b));
  deriv = differentiate(func);

  if (getNumeratorDenominator(&numeratorDeriv,&denominatorDeriv,deriv)) {
    printMessage(1,"Warning: the derivative of the function is a quotient, thus possibly not continuous in the interval.\n");
    printMessage(1,"Only the zeros of the numerator will be searched and pole detection may fail.\n");
    printMessage(1,"Be sure that the function is twice continuously differentiable if trusting the infnorm result.\n");

    mpfr_init2(z,prec);
    mpfr_init2(ya,prec);
    mpfr_init2(yb,prec);

    derivDenominatorDeriv = differentiate(denominatorDeriv);

    newtonWorked = newtonMPFR(z, denominatorDeriv, derivDenominatorDeriv, *(range.a), *(range.b), prec);

    if (newtonWorked && mpfr_number_p(z)) {
      evaluate(ya,numeratorDeriv,z,prec);
      evaluate(yb,denominatorDeriv,z,prec);

      mpfr_abs(ya,ya,GMP_RNDN);
      mpfr_abs(yb,yb,GMP_RNDN);

      mpfr_mul_ui(yb,yb,2,GMP_RNDN);

      if (mpfr_cmp(ya,yb) <= 0) {
	if (verbosity >= 1) {
	  changeToWarningMode();
	  sollyaPrintf("Warning: the derivative of the function seems to have a extensible singularity in ");
	  printValue(&z); 
	  sollyaPrintf(".\n");
	  sollyaPrintf("The infnorm result might not be trustful if the derivative cannot actually\n");
	  sollyaPrintf("be extended in this point.\n");
	  restoreMode(); 
	}
      } else {
	if (verbosity >= 1) {
	  changeToWarningMode(); 
	  sollyaPrintf("Warning: the derivative of the function seems to have a singularity in ");
	  printValue(&z); 
	  sollyaPrintf(".\n");
	  sollyaPrintf("The infnorm result is likely to be wrong.\n");
	  restoreMode(); 
	}
      }
    } else {
      evaluate(ya,denominatorDeriv,*(range.a),prec);
      evaluate(yb,denominatorDeriv,*(range.b),prec);

      if (mpfr_sgn(ya) != mpfr_sgn(yb)) {
	printMessage(1,"Warning: the derivative of the function seems to have a (extensible) singularity in the considered interval.\n");
	printMessage(1,"The infnorm result might be not trustful if the function is not continuously differentiable.\n");
      } else {
	printMessage(2,"Information: the derivative seems to have no (false) pole in the considered interval.\n");
      }
    }

    mpfr_clear(z);
    mpfr_clear(ya);
    mpfr_clear(yb);
    free_memory(derivDenominatorDeriv);
    free_memory(denominatorDeriv);
  }
  derivNumeratorDeriv = differentiate(numeratorDeriv);
  mightExcludes = NULL;

  if (proof != NULL) {
    theo = (infnormTheo *) safeCalloc(1,sizeof(infnormTheo));
  } else {
    theo = NULL;
  }

  printMessage(3,"Information: invoking the interval infnorm subfunction.\n");

  infnormI(resI,func,deriv,numeratorDeriv,derivNumeratorDeriv,rangeI,
	   prec,rangeDiameter,initialExcludes,&mightExcludes,theo);

  printMessage(3,"Information: interval infnorm subfunction has finished.\n");

  secondMightExcludes = NULL;

  if (mightExcludes != NULL) {
    printMessage(1,"Warning: to get better infnorm quality, the following domains will be excluded additionally:\n");
    if (verbosity >= 1) {
      changeToWarningMode();
      curr = mightExcludes;
      while(curr != NULL) {
	printInterval(*((sollya_mpfi_t *) (curr->value)));
	sollyaPrintf("\n");
	curr = curr->next;
      }
      sollyaPrintf("\n");
      restoreMode();
    }
    mightExcludes = concatChains(mightExcludes,initialExcludes);
    freeInitialExcludes = 0;

    if (theo != NULL) freeInfnormTheo(theo);
    if (proof != NULL) {
      theo = (infnormTheo *) safeCalloc(1,sizeof(infnormTheo));
    } else {
      theo = NULL;
    }
    
    printMessage(3,"Information: invoking the interval infnorm subfunction on additional excludes.\n");

    infnormI(resI,func,deriv,numeratorDeriv,derivNumeratorDeriv,rangeI,
	     2*prec,rangeDiameter,mightExcludes,&secondMightExcludes,theo);

    printMessage(3,"Information: interval infnorm subfunction on additional excludes has finished.\n");

    if (secondMightExcludes != NULL) {
      printMessage(1,"Warning: the following domains remain the exclusion of which could improve the result.\n");
      if (verbosity >= 1) {
	changeToWarningMode();
	curr = secondMightExcludes;
	while(curr != NULL) {
	  printInterval(*((sollya_mpfi_t *) (curr->value)));
	  sollyaPrintf("\n");
	  curr = curr->next;
	}
	sollyaPrintf("\n");
	restoreMode();
      }
    }
  }

  if (proof != NULL) {
    printMessage(2,"Information: started writing the proof.\n");
    fprintInfnormTheo(proof,theo,1);
    printMessage(2,"Information: proof written.\n");
  }
  
  if (theo != NULL) freeInfnormTheo(theo);
  freeChain(mightExcludes,freeMpfiPtr);
  freeChain(secondMightExcludes,freeMpfiPtr);
  if (freeInitialExcludes) freeChain(initialExcludes,freeMpfiPtr);
  sollya_mpfi_get_left(*(res.a),resI);
  sollya_mpfi_get_right(*(res.b),resI);
  free_memory(deriv);
  free_memory(numeratorDeriv);
  free_memory(derivNumeratorDeriv);
  sollya_mpfi_clear(rangeI);
  sollya_mpfi_clear(resI);
  mpfr_clear(rangeDiameter);
  return res;
}


void evaluateRangeFunctionFast(rangetype yrange, node *func, node *deriv, rangetype xrange, mp_prec_t prec) {
  sollya_mpfi_t x, y;
  chain *tempChain;
  mp_prec_t p, p2;

  p = prec;
  p2 = mpfr_get_prec(*(xrange.a));
  if (p2 > p) p = p2;
  p2 = mpfr_get_prec(*(xrange.b));
  if (p2 > p) p = p2;

  sollya_mpfi_init2(x,p);
  sollya_mpfi_init2(y,prec);
  sollya_mpfi_interv_fr(x,*(xrange.a),*(xrange.b));

  tempChain = evaluateITaylor(y, func, deriv, x, prec, taylorrecursions, NULL, 1);

  sollya_mpfi_get_left(*(yrange.a),y);
  sollya_mpfi_get_right(*(yrange.b),y);

  freeChain(tempChain,freeMpfiPtr);
  sollya_mpfi_clear(x);
  sollya_mpfi_clear(y);
}

void evaluateInterval(sollya_mpfi_t y, node *func, node *deriv, sollya_mpfi_t x) {
  mp_prec_t prec;

  prec = sollya_mpfi_get_prec(y);

  // We need more precision in the first steps to get the precision in the end.
  prec <<= 1;

  // Let's use at least the precision of the tool, that gives us
  // additional 10% on the check examples
  if (prec < tools_precision) prec = tools_precision;

  evaluateITaylor(y, func, deriv, x, prec, taylorrecursions, NULL, 1);
}

void evaluateConstantExpressionToInterval(sollya_mpfi_t y, node *func) {
  sollya_mpfi_t x;

  if (!isConstant(func)) {
    printMessage(1,"Warning: the given expression is not constant. Evaluating it at 1.\n");
  }

  sollya_mpfi_init2(x,12);
  sollya_mpfi_set_si(x,1);

  evaluateInterval(y, func, NULL, x);

  sollya_mpfi_clear(x);
}

void evaluateRangeFunction(rangetype yrange, node *func, rangetype xrange, mp_prec_t prec) {
  node *deriv, *temp, *temp2, *numerator, *denominator, *f;
  rangetype myrange;
  node *myderiv;

  temp = differentiate(func);
  deriv = horner(temp);

  if ((func->nodeType == POW) && 
      (func->child1->nodeType == VARIABLE) &&
      (func->child2->nodeType == CONSTANT) &&
      mpfr_zero_p(*(func->child2->value))) {
    temp2 = copyTree(func);
  } else {
    temp2 = horner(func);
  }

  f = NULL;

  if (getNumeratorDenominator(&numerator,&denominator,temp2)) {
    if (isSyntacticallyEqual(numerator, denominator)) {
      if (!isConstant(numerator)) {
        myderiv = differentiate(numerator);
        myrange.a = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
        myrange.b = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
        mpfr_init2(*(myrange.a),mpfr_get_prec(*(yrange.a)));
        mpfr_init2(*(myrange.b),mpfr_get_prec(*(yrange.b)));
        evaluateRangeFunctionFast(myrange,numerator,myderiv,xrange,prec);
        free_memory(myderiv);
        if (mpfr_sgn(*(myrange.a)) * mpfr_sgn(*(myrange.b)) == 1) {
          mpfr_clear(*(myrange.a));
          mpfr_clear(*(myrange.b));
          free(myrange.a);
          free(myrange.b);
          mpfr_set_d(*(yrange.a),1.0,GMP_RNDD);
          mpfr_set_d(*(yrange.b),1.0,GMP_RNDU);
          free_memory(numerator);
          free_memory(denominator);
          free_memory(deriv);
          free_memory(temp);
          free_memory(temp2);
          return;
        } else {
          f = copyTree(temp2);
        }
      } else {
        myderiv = differentiate(numerator);
        myrange.a = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
        myrange.b = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
        mpfr_init2(*(myrange.a),mpfr_get_prec(*(yrange.a)));
        mpfr_init2(*(myrange.b),mpfr_get_prec(*(yrange.b)));
        evaluateRangeFunctionFast(myrange,numerator,myderiv,xrange,prec);
        free_memory(myderiv);
        if (mpfr_sgn(*(myrange.a)) * mpfr_sgn(*(myrange.b)) == 1) {
          mpfr_clear(*(myrange.a));
          mpfr_clear(*(myrange.b));
          free(myrange.a);
          free(myrange.b);
          mpfr_set_d(*(yrange.a),1.0,GMP_RNDD);
          mpfr_set_d(*(yrange.b),1.0,GMP_RNDU);
          free_memory(numerator);
          free_memory(denominator);
          free_memory(deriv);
          free_memory(temp);
          free_memory(temp2);
          return;
        } else {
          if (mpfr_zero_p(*(myrange.a)) && mpfr_zero_p(*(myrange.b))) {
            mpfr_clear(*(myrange.a));
            mpfr_clear(*(myrange.b));
            free(myrange.a);
            free(myrange.b);
            mpfr_set_nan(*(yrange.a));
            mpfr_set_nan(*(yrange.b));
            free_memory(numerator);
            free_memory(denominator);
            free_memory(deriv);
            free_memory(temp);
            free_memory(temp2);
            return;
          } else {
            mpfr_clear(*(myrange.a));
            mpfr_clear(*(myrange.b));
            free(myrange.a);
            free(myrange.b);
            f = copyTree(temp2);
          }
        }
      }
    } else {
      f = copyTree(temp2);
    }
    free_memory(numerator);
    free_memory(denominator);
  } else {
    f = numerator;
  }

  evaluateRangeFunctionFast(yrange,f,deriv,xrange,prec);
  free_memory(deriv);
  free_memory(temp);
  free_memory(temp2);
  free_memory(f);
}



chain* findZerosFunction(node *func, rangetype range, mp_prec_t prec, mpfr_t diam) {
  sollya_mpfi_t rangeI;
  node *deriv, *numerator, *denominator;
  mpfr_t rangeDiameter, diamJoin;
  chain *zerosI, *zeros, *curr;
  rangetype *tempRange;
  chain *tempChain, *tempChain2, *tempChain3;

  sollya_mpfi_init2(rangeI,prec);
  mpfr_init2(rangeDiameter,prec);
  mpfr_sub(rangeDiameter,*(range.b),*(range.a),GMP_RNDD);
  mpfr_mul(rangeDiameter,rangeDiameter,diam,GMP_RNDD);
  sollya_mpfi_interv_fr(rangeI,*(range.a),*(range.b));

  if (getNumeratorDenominator(&numerator,&denominator,func)) free_memory(denominator);

  deriv = differentiate(numerator);

  tempChain = findZeros(numerator,deriv,rangeI,prec,rangeDiameter,NULL);
  mpfr_init2(diamJoin,prec);
  mpfr_mul_2ui(diamJoin,diam,3,GMP_RNDN);
  tempChain2 = joinAdjacentIntervals(tempChain,diamJoin);
  tempChain3 = copyChain(tempChain2,copyMpfiPtr);
  mpfr_mul_2ui(diamJoin,diamJoin,2,GMP_RNDN);
  zerosI = joinAdjacentIntervals(tempChain3,diamJoin);
  mpfr_clear(diamJoin);

  zeros = NULL;
  curr = zerosI;
  while (curr != NULL) {
    tempRange = (rangetype *) safeMalloc(sizeof(rangetype));
    tempRange->a = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
    tempRange->b = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
    mpfr_init2(*(tempRange->a),prec);
    mpfr_init2(*(tempRange->b),prec);
    sollya_mpfi_get_left(*(tempRange->a),*((sollya_mpfi_t *) (curr->value)));
    sollya_mpfi_get_right(*(tempRange->b),*((sollya_mpfi_t *) (curr->value)));
    zeros = addElement(zeros,tempRange);
    curr = curr->next;
  }

  freeChain(zerosI,freeMpfiPtr);
  freeChain(tempChain,freeMpfiPtr);
  freeChain(tempChain2,freeMpfiPtr);
  freeChain(tempChain3,freeMpfiPtr);
  free_memory(numerator);
  free_memory(deriv);
  sollya_mpfi_clear(rangeI);
  mpfr_clear(rangeDiameter);
  return zeros;
}


int checkInfnormI(node *func, node *deriv, sollya_mpfi_t infnormval, sollya_mpfi_t range, mpfr_t diam, mp_prec_t prec) {
  sollya_mpfi_t evaluateOnRange, rangeLeft, rangeRight;
  chain *tempChain;
  mpfr_t l,m,r, diamRange;
  int resultLeft, resultRight;

  sollya_mpfi_init2(evaluateOnRange,prec);

  tempChain = evaluateITaylor(evaluateOnRange, func, deriv, range, prec, taylorrecursions, NULL, 1);

  freeChain(tempChain,freeMpfiPtr);

  if (sollya_mpfi_is_inside(evaluateOnRange, infnormval)) {
    /* Simple end case: the interval evaluation is contained in the given interval for infnorm */
    sollya_mpfi_clear(evaluateOnRange);
    return 1;
  }

  mpfr_init2(diamRange,prec);
  sollya_mpfi_diam_abs(diamRange,range);

  if (mpfr_cmp(diamRange,diam) <= 0) {
    /* Simple end case: the range to test is already smaller than diam but we could not check */
    if (verbosity >= 2) {
      changeToWarningMode();
      sollyaPrintf("Information: could not check the infinity norm on the domain\n");
      printInterval(range);
      sollyaPrintf("\nThe function evaluates here to\n");
      printInterval(evaluateOnRange);
      sollyaPrintf("\n");
      restoreMode();
    }
    sollya_mpfi_clear(evaluateOnRange);
    mpfr_clear(diamRange);
    return 0;
  }

  mpfr_init2(l,prec);
  mpfr_init2(m,prec);
  mpfr_init2(r,prec);
  sollya_mpfi_init2(rangeLeft,prec);
  sollya_mpfi_init2(rangeRight,prec);
  
  sollya_mpfi_get_left(l,range);
  sollya_mpfi_mid(m,range);
  sollya_mpfi_get_right(r,range);
  
  sollya_mpfi_interv_fr(rangeLeft,l,m);
  sollya_mpfi_interv_fr(rangeRight,m,r);

  /* Recurse on half the range */
  
  resultLeft = 0;
  resultRight = 0;

  resultLeft = checkInfnormI(func, deriv, infnormval, rangeLeft, diam, prec);
  if (resultLeft || (verbosity >= 4)) resultRight = checkInfnormI(func, deriv, infnormval, rangeRight, diam, prec);

  sollya_mpfi_clear(rangeRight);
  sollya_mpfi_clear(rangeLeft);
  mpfr_clear(r);
  mpfr_clear(m);
  mpfr_clear(l);
  sollya_mpfi_clear(evaluateOnRange);
  mpfr_clear(diamRange);

  return (resultLeft && resultRight);
}


int checkInfnorm(node *func, rangetype range, mpfr_t infnormval, mpfr_t diam, mp_prec_t prec) {
  node *deriv;
  sollya_mpfi_t rangeI, infnormvalI;
  mpfr_t rangeDiameter, tempLeft, tempRight;
  int result;

  sollya_mpfi_init2(rangeI,prec);
  sollya_mpfi_init2(infnormvalI,prec);
  mpfr_init2(rangeDiameter,prec);
  mpfr_init2(tempLeft,prec);
  mpfr_init2(tempRight,prec);

  mpfr_sub(rangeDiameter,*(range.b),*(range.a),GMP_RNDD);
  mpfr_mul(rangeDiameter,rangeDiameter,diam,GMP_RNDD);
  sollya_mpfi_interv_fr(rangeI,*(range.a),*(range.b));
  mpfr_abs(tempRight,infnormval,GMP_RNDU);
  mpfr_neg(tempLeft,tempRight,GMP_RNDD);
  sollya_mpfi_interv_fr(infnormvalI,tempLeft,tempRight);
  deriv = differentiate(func);

  result = checkInfnormI(func, deriv, infnormvalI, rangeI, rangeDiameter, prec);

  free_memory(deriv);
  mpfr_clear(tempLeft);
  mpfr_clear(tempRight);
  mpfr_clear(rangeDiameter);
  sollya_mpfi_clear(infnormvalI);
  sollya_mpfi_clear(rangeI);

  return result;
}


void evaluateConstantWithErrorEstimate(mpfr_t res, mpfr_t err, node *func, mpfr_t x, mp_prec_t prec) {
  rangetype xrange, yrange;
  mpfr_t temp;

  xrange.a = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
  xrange.b = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
  yrange.a = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
  yrange.b = (mpfr_t *) safeMalloc(sizeof(mpfr_t));

  mpfr_init2(*(xrange.a), prec);
  mpfr_init2(*(xrange.b), prec);
  mpfr_init2(*(yrange.a), prec);
  mpfr_init2(*(yrange.b), prec);
  mpfr_init2(temp,prec + 10);
  
  mpfr_set(*(xrange.a),x,GMP_RNDD);
  mpfr_set(*(xrange.b),x,GMP_RNDU);
  
  evaluateRangeFunction(yrange, func, xrange, prec);

  mpfr_add(temp,*(yrange.a),*(yrange.b),GMP_RNDN);
  mpfr_div_2ui(temp,temp,1,GMP_RNDN);
  mpfr_set(res,temp,GMP_RNDN);

  if (mpfr_zero_p(res)) {
    if (mpfr_zero_p(*(yrange.a)) && mpfr_zero_p(*(yrange.b))) {
      mpfr_set_d(err,0.0,GMP_RNDN);
    } else {
      mpfr_set_d(temp,1.0,GMP_RNDN);
      mpfr_div(temp,temp,res,GMP_RNDN);
      mpfr_set(err,temp,GMP_RNDU);
    }
  } else {
    mpfr_abs(*(yrange.a),*(yrange.a),GMP_RNDN);
    mpfr_abs(*(yrange.b),*(yrange.b),GMP_RNDN);
    if (mpfr_cmp(*(yrange.b),*(yrange.a)) > 0) {
      mpfr_set(*(yrange.a),*(yrange.b),GMP_RNDN);
    }
    mpfr_abs(temp,temp,GMP_RNDN);
    mpfr_sub(*(yrange.a),*(yrange.a),temp,GMP_RNDU);
    mpfr_div(err,*(yrange.a),temp,GMP_RNDU);
  }


  mpfr_clear(*(xrange.a));
  mpfr_clear(*(xrange.b));
  mpfr_clear(*(yrange.a));
  mpfr_clear(*(yrange.b));
  mpfr_init2(temp,prec);
  free(xrange.a);
  free(xrange.b);
  free(yrange.a);
  free(yrange.b);
}

chain* findZerosByNewton(node *func, mpfr_t a, mpfr_t b, mp_prec_t prec) {
  node *deriv;
  mpfr_t resNewtonStep, ap, bp, step, yAp, yBp;
  int newtonOkay;
  mpfr_t *newZero;
  chain *fpZeros;

  fpZeros = NULL;
  deriv = differentiate(func);
  mpfr_init2(resNewtonStep,prec);
  mpfr_init2(ap,prec);
  mpfr_init2(bp,prec);
  mpfr_init2(step,prec);
  mpfr_init2(yAp,prec);
  mpfr_init2(yBp,prec);
  
  mpfr_sub(step,b,a,GMP_RNDU);
  if (mpfr_zero_p(step)) {
    evaluate(resNewtonStep,func,a,prec);
    newZero = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
    mpfr_init2(*newZero,prec);
    mpfr_set(*newZero,resNewtonStep,GMP_RNDN);
    fpZeros = addElement(fpZeros,newZero);
  } else {
    mpfr_div_ui(step,step,defaultpoints,GMP_RNDU);    
    mpfr_set(ap,a,GMP_RNDD);
    while (mpfr_cmp(ap,b) < 0) {
      mpfr_add(bp,ap,step,GMP_RNDN);
      sollya_mpfr_min(bp,bp,b,GMP_RNDU);
      
      newtonOkay = newtonMPFR(resNewtonStep, func, deriv, ap, bp, prec);
      
      if (newtonOkay) {
	newZero = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*newZero,prec);
	mpfr_set(*newZero,resNewtonStep,GMP_RNDN);
	fpZeros = addElement(fpZeros,newZero);
      } else {
	evaluateFaithful(yAp, func, ap, prec);
	evaluateFaithful(yBp, func, bp, prec);
	if (mpfr_number_p(yAp) && mpfr_number_p(yBp) &&
	    (mpfr_sgn(yAp) != mpfr_sgn(yBp))) {
	  newZero = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
	  mpfr_init2(*newZero,prec);
	  mpfr_set(*newZero,ap,GMP_RNDN);
	  mpfr_add(*newZero,*newZero,bp,GMP_RNDN);
	  mpfr_div_2ui(*newZero,*newZero,1,GMP_RNDN);
	  fpZeros = addElement(fpZeros,newZero);
	}
      }
      
      mpfr_set(ap,bp,GMP_RNDN);
    }
  }
  mpfr_clear(step);
  mpfr_clear(bp);
  mpfr_clear(ap);
  mpfr_clear(yAp);
  mpfr_clear(yBp);
  mpfr_clear(resNewtonStep);
  free_memory(deriv);
  return fpZeros;
}



chain* fpFindZerosFunction(node *func, rangetype range, mp_prec_t prec) {
  mpfr_t diam;
  chain *intervalZeros, *fpZeros, *temp, *fpZerosOnInterval, *fpZeros2, *curr;
  mpfr_t *newZero;
  mpfr_t before, after, yBefore, yAfter, y, compare;
  int addToList, removedFromList;
  unsigned int oldDefaultPoints;

  
  oldDefaultPoints = defaultpoints;
  defaultpoints = defaultpoints >> 4;

  mpfr_init2(diam,prec+50);
  mpfr_set_d(diam,DEFAULTDIAM2,GMP_RNDN);

  intervalZeros = findZerosFunction(func, range, prec, diam);
  
  mpfr_clear(diam);

  fpZeros = NULL;

  while (intervalZeros != NULL) {
    fpZerosOnInterval = findZerosByNewton(func, 
					  *(((rangetype *) (intervalZeros->value))->a), 
					  *(((rangetype *) (intervalZeros->value))->b), 
					  4*prec);
    fpZeros = concatChains(fpZeros, fpZerosOnInterval);
    mpfr_clear(*(((rangetype *) (intervalZeros->value))->a));
    mpfr_clear(*(((rangetype *) (intervalZeros->value))->b));
    free(((rangetype *) (intervalZeros->value))->a);
    free(((rangetype *) (intervalZeros->value))->b);
    free(intervalZeros->value);
    temp = intervalZeros->next;
    free(intervalZeros);
    intervalZeros = temp;
  }
  

  mpfr_init2(compare,prec);
  mpfr_set_d(compare,1.0,GMP_RNDN);
  mpfr_div_ui(compare,compare,prec,GMP_RNDN);
  fpZeros2 = NULL;
  curr = fpZeros;
  while (curr != NULL) {
    while ((curr->next != NULL) && (mpfr_cmp(*((mpfr_t *) (curr->value)),*((mpfr_t *) (curr->next->value))) == 0))
      curr = curr->next;
    newZero = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
    mpfr_init2(*newZero,prec);
    mpfr_set(*newZero,*((mpfr_t *) (curr->value)),GMP_RNDN);
    fpZeros2 = addElement(fpZeros2,newZero);
    if ((!mpfr_zero_p(*newZero)) && (mpfr_cmpabs(*newZero,compare) <= 0)) {
      newZero = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*newZero,prec);
      mpfr_set_d(*newZero,0.0,GMP_RNDN);
      fpZeros2 = addElement(fpZeros2,newZero);
    }
    curr = curr->next;
  }
  mpfr_clear(compare);

  while (fpZeros != NULL) {
    mpfr_clear(*((mpfr_t*) (fpZeros->value)));
    free((fpZeros->value));
    temp = fpZeros->next;
    free(fpZeros);
    fpZeros = temp;
  }

  mpfr_init2(before,prec);
  mpfr_init2(after,prec);
  mpfr_init2(yAfter,prec);
  mpfr_init2(yBefore,prec);
  mpfr_init2(y,prec);

  removedFromList = 0;
  fpZeros = NULL;
  while (fpZeros2 != NULL) {
    
    addToList = 0;

    evaluateFaithful(y, func, *((mpfr_t *) (fpZeros2->value)), prec);

    if (mpfr_zero_p(y) || (!mpfr_number_p(y)) || mpfr_zero_p(*((mpfr_t *) (fpZeros2->value)))) {
      addToList = 1;
    } else {

      mpfr_set(before,*((mpfr_t *) (fpZeros2->value)),GMP_RNDN);
      mpfr_set(after,*((mpfr_t *) (fpZeros2->value)),GMP_RNDN);
      mpfr_nextabove(after);
      mpfr_nextbelow(before);

      evaluateFaithful(yAfter, func, after, prec);
      evaluateFaithful(yBefore, func, before, prec);
      
      if ((!mpfr_number_p(yAfter)) || (!mpfr_number_p(yBefore))) {
	addToList = 1;
      } else {
	if (mpfr_sgn(yAfter) != mpfr_sgn(yBefore)) {
	  addToList = 1;
	} else {
	  if (mpfr_number_p(y)) {
	    if (mpfr_sgn(y) != mpfr_sgn(yAfter)) {
	      addToList = 1;
	    } else {
	      removedFromList = 1;
	      if (verbosity >= 2) {
		changeToWarningMode();
		printMessage(2,"Information: removing possible zero in ");
		printMpfr(*((mpfr_t *) (fpZeros2->value)));
		printMessage(3,"Information: removing because all signs are equal.\n");
		restoreMode();
	      }
	    }
	  } else {
	    removedFromList = 1;
	    if (verbosity >= 2) {
	      changeToWarningMode();
	      printMessage(2,"Information: removing possible zero in ");
	      printMpfr(*((mpfr_t *) (fpZeros2->value)));
	      printMessage(3,"Information: removing because predecessor and successor signs are equal.\n");
	      restoreMode();
	    }
	  }
	}
      }
    }

    if (addToList) {
      fpZeros = addElement(fpZeros,fpZeros2->value);
    }
    temp = fpZeros2->next;
    free(fpZeros2);
    fpZeros2 = temp;
  }

  if (removedFromList) {
    printMessage(1,"Warning: actual zero filter has removed at least one possible zero of higher order.\n");
  }

  mpfr_clear(before);
  mpfr_clear(after);
  mpfr_clear(yBefore);
  mpfr_clear(yAfter);
  mpfr_clear(y);

  sortChain(fpZeros,  cmpMpfrPtr);

  fpZeros2 = NULL;
  curr = fpZeros;
  while (curr != NULL) {
    while ((curr->next != NULL) && (mpfr_cmp(*((mpfr_t *) (curr->value)),*((mpfr_t *) (curr->next->value))) == 0))
      curr = curr->next;
    newZero = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
    mpfr_init2(*newZero,mpfr_get_prec(*((mpfr_t *) (curr->value))));
    mpfr_set(*newZero,*((mpfr_t *) (curr->value)),GMP_RNDN);
    fpZeros2 = addElement(fpZeros2,newZero);
    curr = curr->next;
  }

  while (fpZeros != NULL) {
    mpfr_clear(*((mpfr_t*) (fpZeros->value)));
    free((fpZeros->value));
    temp = fpZeros->next;
    free(fpZeros);
    fpZeros = temp;
  }

  defaultpoints = oldDefaultPoints;

  return fpZeros2;
}


chain *uncertifiedZeroDenominators(node *tree, mpfr_t a, mpfr_t b, mp_prec_t prec) {
  chain *leftPoles, *rightPoles, *newZeros;
  rangetype range;

  if (tree == NULL) return NULL;
  switch (tree->nodeType) {
  case VARIABLE:
    return NULL;
    break;
  case CONSTANT:
  case PI_CONST:
  case LIBRARYCONSTANT:
    return NULL;
    break;
  case ADD:
    leftPoles = uncertifiedZeroDenominators(tree->child1,a,b,prec);
    rightPoles = uncertifiedZeroDenominators(tree->child2,a,b,prec);
    return concatChains(leftPoles,rightPoles);
    break;
  case SUB:
    leftPoles = uncertifiedZeroDenominators(tree->child1,a,b,prec);
    rightPoles = uncertifiedZeroDenominators(tree->child2,a,b,prec);
    return concatChains(leftPoles,rightPoles);
    break;
  case MUL:
    leftPoles = uncertifiedZeroDenominators(tree->child1,a,b,prec);
    rightPoles = uncertifiedZeroDenominators(tree->child2,a,b,prec);
    return concatChains(leftPoles,rightPoles);
    break;
  case DIV:
    leftPoles = uncertifiedZeroDenominators(tree->child1,a,b,prec);
    rightPoles = uncertifiedZeroDenominators(tree->child2,a,b,prec);
    range.a = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
    range.b = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
    mpfr_init2(*(range.a),prec);
    mpfr_init2(*(range.b),prec);
    mpfr_set(*(range.a),a,GMP_RNDD);
    mpfr_set(*(range.b),b,GMP_RNDU);
    newZeros = fpFindZerosFunction(tree->child2, range, prec);
    mpfr_clear(*(range.a));
    mpfr_clear(*(range.b));
    free(range.a);
    free(range.b);
    leftPoles = concatChains(leftPoles,rightPoles);
    return concatChains(leftPoles,newZeros);
    break;
  case SQRT:
    return uncertifiedZeroDenominators(tree->child1,a,b,prec);
    break;
  case EXP:
    return uncertifiedZeroDenominators(tree->child1,a,b,prec);
    break;
  case LOG:
    return uncertifiedZeroDenominators(tree->child1,a,b,prec);
    break;
  case LOG_2:
    return uncertifiedZeroDenominators(tree->child1,a,b,prec);
    break;
  case LOG_10:
    return uncertifiedZeroDenominators(tree->child1,a,b,prec);
    break;
  case SIN:
    return uncertifiedZeroDenominators(tree->child1,a,b,prec);
    break;
  case COS:
    return uncertifiedZeroDenominators(tree->child1,a,b,prec);
    break;
  case TAN:
    return uncertifiedZeroDenominators(tree->child1,a,b,prec);
    break;
  case ASIN:
    return uncertifiedZeroDenominators(tree->child1,a,b,prec);
    break;
  case ACOS:
    return uncertifiedZeroDenominators(tree->child1,a,b,prec);
    break;
  case ATAN:
    return uncertifiedZeroDenominators(tree->child1,a,b,prec);
    break;
  case SINH:
    return uncertifiedZeroDenominators(tree->child1,a,b,prec);
    break;
  case COSH:
    return uncertifiedZeroDenominators(tree->child1,a,b,prec);
    break;
  case TANH:
    return uncertifiedZeroDenominators(tree->child1,a,b,prec);
    break;
  case ASINH:
    return uncertifiedZeroDenominators(tree->child1,a,b,prec);
    break;
  case ACOSH:
    return uncertifiedZeroDenominators(tree->child1,a,b,prec);
    break;
  case ATANH:
    return uncertifiedZeroDenominators(tree->child1,a,b,prec);
    break;
  case POW:
    leftPoles = uncertifiedZeroDenominators(tree->child1,a,b,prec);
    rightPoles = uncertifiedZeroDenominators(tree->child2,a,b,prec);
    return concatChains(leftPoles,rightPoles);
    break;
  case NEG:
    return uncertifiedZeroDenominators(tree->child1,a,b,prec);
    break;
  case ABS:
    return uncertifiedZeroDenominators(tree->child1,a,b,prec);
    break;
  case DOUBLE:
    return uncertifiedZeroDenominators(tree->child1,a,b,prec);
    break;
  case SINGLE:
    return uncertifiedZeroDenominators(tree->child1,a,b,prec);
    break;
  case QUAD:
    return uncertifiedZeroDenominators(tree->child1,a,b,prec);
    break;
  case HALFPRECISION:
    return uncertifiedZeroDenominators(tree->child1,a,b,prec);
    break;
  case DOUBLEDOUBLE:
    return uncertifiedZeroDenominators(tree->child1,a,b,prec);
    break;
  case TRIPLEDOUBLE:
    return uncertifiedZeroDenominators(tree->child1,a,b,prec);
    break;
  case ERF:
    return uncertifiedZeroDenominators(tree->child1,a,b,prec);
    break;
  case ERFC:
    return uncertifiedZeroDenominators(tree->child1,a,b,prec);
    break;
  case LOG_1P:
    return uncertifiedZeroDenominators(tree->child1,a,b,prec);
    break;
  case EXP_M1:
    return uncertifiedZeroDenominators(tree->child1,a,b,prec);
    break;
  case DOUBLEEXTENDED:
    return uncertifiedZeroDenominators(tree->child1,a,b,prec);
    break;
  case LIBRARYFUNCTION:
    return uncertifiedZeroDenominators(tree->child1,a,b,prec);
    break;
  case PROCEDUREFUNCTION:
    return uncertifiedZeroDenominators(tree->child1,a,b,prec);
    break;
  case CEIL:
    return uncertifiedZeroDenominators(tree->child1,a,b,prec);
    break;
  case FLOOR:
    return uncertifiedZeroDenominators(tree->child1,a,b,prec);
    break;
  case NEARESTINT:
    return uncertifiedZeroDenominators(tree->child1,a,b,prec);
    break;
  default:
   sollyaFprintf(stderr,"Error: uncertifiedZeroDenominators: unknown identifier (%d) in the tree\n",tree->nodeType);
   exit(1);
  }
  return NULL;
}


int isEvaluable(node *func, mpfr_t x, mpfr_t *y, mp_prec_t prec) {
  mpfr_t val;
  rangetype xrange, yrange;

  mpfr_init2(val,prec);
  evaluate(val,func,x,prec);
  if (mpfr_number_p(val)) {
    if (y != NULL) {
      mpfr_set(*y,val,GMP_RNDN);
    }
    mpfr_clear(val);
    return ISFLOATINGPOINTEVALUABLE;
  }

  xrange.a = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
  xrange.b = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
  yrange.a = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
  yrange.b = (mpfr_t *) safeMalloc(sizeof(mpfr_t));

  mpfr_init2(*(xrange.a),prec);
  mpfr_init2(*(xrange.b),prec);
  mpfr_init2(*(yrange.a),prec);
  mpfr_init2(*(yrange.b),prec);

  mpfr_set(*(xrange.a),x,GMP_RNDD);
  mpfr_set(*(xrange.b),x,GMP_RNDU);

  evaluateRangeFunction(yrange, func, xrange, prec);

  if (mpfr_number_p(*(yrange.a)) && mpfr_number_p(*(yrange.b))) {
    mpfr_add(val,*(yrange.a),*(yrange.b),GMP_RNDN);
    mpfr_div_2ui(val,val,1,GMP_RNDN);
    if (!mpfr_number_p(val)) {
      mpfr_clear(val);
      mpfr_clear(*(xrange.a));
      mpfr_clear(*(xrange.b));
      mpfr_clear(*(yrange.a));
      mpfr_clear(*(yrange.b));
      free(xrange.a);
      free(xrange.b);
      free(yrange.a);
      free(yrange.b);
      return ISNOTEVALUABLE;
    }
    if (y != NULL) {
      mpfr_set(*y,val,GMP_RNDN);
    }
    mpfr_clear(val);
    mpfr_clear(*(xrange.a));
    mpfr_clear(*(xrange.b));
    mpfr_clear(*(yrange.a));
    mpfr_clear(*(yrange.b));
    free(xrange.a);
    free(xrange.b);
    free(yrange.a);
    free(yrange.b);
    return ISHOPITALEVALUABLE;
  } 

  mpfr_clear(val);
  mpfr_clear(*(xrange.a));
  mpfr_clear(*(xrange.b));
  mpfr_clear(*(yrange.a));
  mpfr_clear(*(yrange.b));
  free(xrange.a);
  free(xrange.b);
  free(yrange.a);
  free(yrange.b);
  return ISNOTEVALUABLE;
}

int evaluateWithAccuracyEstimate(node *func, mpfr_t x, mpfr_t y, mpfr_t accur, mp_prec_t prec) {
  rangetype xrange, yrange;

  xrange.a = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
  xrange.b = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
  yrange.a = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
  yrange.b = (mpfr_t *) safeMalloc(sizeof(mpfr_t));

  mpfr_init2(*(xrange.a),prec);
  mpfr_init2(*(xrange.b),prec);
  mpfr_init2(*(yrange.a),prec);
  mpfr_init2(*(yrange.b),prec);

  mpfr_set(*(xrange.a),x,GMP_RNDD);
  mpfr_set(*(xrange.b),x,GMP_RNDU);

  evaluateRangeFunction(yrange, func, xrange, prec);

  mpfr_add(*(xrange.a),*(yrange.a),*(yrange.b),GMP_RNDN);
  mpfr_div_2ui(*(xrange.a),*(xrange.a),1,GMP_RNDN);
  mpfr_set(y,*(xrange.a),GMP_RNDN);

  /* We have a zero in the output interval or are not a number */
  if ((mpfr_sgn(*(yrange.a)) != mpfr_sgn(*(yrange.b))) || (!mpfr_number_p(y))) {
    mpfr_clear(*(xrange.a));
    mpfr_clear(*(xrange.b));
    mpfr_clear(*(yrange.a));
    mpfr_clear(*(yrange.b));
    free(xrange.a);
    free(xrange.b);
    free(yrange.a);
    free(yrange.b);
    return 0;
  }
  
  /* We have the exact interval [0;0] with an error of 0 */
  if (mpfr_zero_p(*(yrange.a)) && mpfr_zero_p(*(yrange.b))) {
    mpfr_set_d(accur,0.0,GMP_RNDN);
    mpfr_clear(*(xrange.a));
    mpfr_clear(*(xrange.b));
    mpfr_clear(*(yrange.a));
    mpfr_clear(*(yrange.b));
    free(xrange.a);
    free(xrange.b);
    free(yrange.a);
    free(yrange.b);
    return 1;
  }

  if (mpfr_cmp(y,*(yrange.b)) > 0) {
    mpfr_set(*(yrange.b),y,GMP_RNDU);
  }

  if (mpfr_cmp(y,*(yrange.a)) < 0) {
    mpfr_set(*(yrange.a),y,GMP_RNDD);
  }

  mpfr_abs(*(yrange.a),*(yrange.a),GMP_RNDD);
  mpfr_abs(*(yrange.b),*(yrange.b),GMP_RNDD);
  sollya_mpfr_min(*(xrange.a),*(yrange.a),*(yrange.b),GMP_RNDD);
  sollya_mpfr_max(*(xrange.b),*(yrange.a),*(yrange.b),GMP_RNDU);

  mpfr_sub(*(xrange.b),*(xrange.b),*(xrange.a),GMP_RNDU);

  mpfr_div(*(xrange.b),*(xrange.b),*(xrange.a),GMP_RNDU);
  mpfr_mul_2ui(*(xrange.b),*(xrange.b),1,GMP_RNDU);
  mpfr_set(accur,*(xrange.b),GMP_RNDU);

  mpfr_clear(*(xrange.a));
  mpfr_clear(*(xrange.b));
  mpfr_clear(*(yrange.a));
  mpfr_clear(*(yrange.b));
  free(xrange.a);
  free(xrange.b);
  free(yrange.a);
  free(yrange.b);

  return 1;
}


int evaluateWithAccuracy(node *func, mpfr_t x, mpfr_t y, mpfr_t accur, 
			 mp_prec_t minprec, mp_prec_t maxprec, mp_prec_t *needPrec) {
  mpfr_t temp, currY, currX, currAccur, resY;
  mp_prec_t p;
  int res, okay;

  if (mpfr_sgn(accur) <= 0) return 0;

  mpfr_init2(temp,minprec);
  mpfr_set_d(temp,1.0,GMP_RNDN);
  p = mpfr_get_prec(y);
  if (maxprec < p) p = maxprec;
  mpfr_div_2ui(temp,temp,p,GMP_RNDN);
  if (mpfr_cmp(accur,temp) < 0) {
    mpfr_clear(temp);
    return 0;
  }
  mpfr_clear(temp);

  mpfr_init2(currAccur,mpfr_get_prec(accur));

  p = minprec;
  if ((mpfr_get_prec(y) + 10) > minprec) minprec = mpfr_get_prec(y) + 10;

  res = 0; okay = 0;
  while (p <= maxprec) {
    mpfr_init2(currY,p);
    mpfr_init2(currX,p);
    mpfr_set(currX,x,GMP_RNDN);
    res = evaluateWithAccuracyEstimate(func, currX, currY, currAccur, p);
    mpfr_clear(currX);

    if (res && (mpfr_cmp(currAccur,accur) <= 0)) {
      mpfr_init2(resY,mpfr_get_prec(currY));
      mpfr_set(resY,currY,GMP_RNDN);
      mpfr_clear(currY);
      okay = 1;
      break;
    }
    mpfr_clear(currY);
    p *= 2;
  }
  
  if (okay) {
    mpfr_set(y,resY,GMP_RNDN);
    mpfr_clear(resY);
    if (needPrec != NULL) {
      *needPrec = p;
    }
  }

  mpfr_clear(currAccur);
  return okay;
}


int evaluateFaithfulOrFail(node *func, mpfr_t x, mpfr_t y, unsigned int precFactor, mp_prec_t *needPrec) {
  mp_prec_t startPrec, endPrec, p;
  mpfr_t accur;
  int res;

  p = mpfr_get_prec(y);
  startPrec = p + 10;
  endPrec = startPrec * precFactor;

  mpfr_init2(accur,startPrec);
  mpfr_set_d(accur,1.0,GMP_RNDN);
  mpfr_div_2ui(accur,accur,p,GMP_RNDD);

  res = evaluateWithAccuracy(func, x, y, accur, startPrec, endPrec, needPrec);
  
  mpfr_clear(accur);
  
  return res;
}

int evaluateFaithful(mpfr_t result, node *tree, mpfr_t x, mp_prec_t prec) {
  mp_prec_t startPrec, p;
  mpfr_t cutoff;
  int res;

  p = mpfr_get_prec(result);
  startPrec = p + 10;
  if (prec > startPrec) startPrec = prec;

  mpfr_init2(cutoff,startPrec);
  mpfr_set_si(cutoff,0,GMP_RNDN);

  res = evaluateFaithfulWithCutOffFast(result, tree, NULL, x, cutoff, startPrec);
  if (res==3) res=0;

  mpfr_clear(cutoff);

  if (!res) {
    printMessage(4,"Warning: evaluateFaithful returned NaN.\n");
    mpfr_set_nan(result);
  }

  return res;

}

int determineHeuristicTaylorRecursions(node *func) {
  int highestDegree, sizeOfFunc, sizeOfCurrDeriv, i;
  node *temp, *temp2;

  highestDegree = highestDegreeOfPolynomialSubexpression(func);

  sizeOfFunc = treeSize(func);

  temp = differentiate(func);
  sizeOfCurrDeriv = treeSize(temp);
  i = -1;

  while ((highestDegree >= 0) && (((double) sizeOfCurrDeriv) <= ((double) 4) * ((double) sizeOfFunc))) {
    temp2 = differentiate(temp);
    free_memory(temp);
    temp = temp2;
    sizeOfCurrDeriv = treeSize(temp);
    i++;
    highestDegree--;
  }
  
  free_memory(temp);

  return i < 0 ? 0 : i;
}



int accurateInfnorm(mpfr_t result, node *func, rangetype range, chain *excludes, mp_prec_t startPrec) {
  rangetype res;
  sollya_mpfi_t rangeI, resI;
  sollya_mpfi_t *excludeI;
  node *deriv, *numeratorDeriv, *derivNumeratorDeriv, *denominatorDeriv, *derivDenominatorDeriv;
  mpfr_t rangeDiameter, z, ya,yb;
  chain *curr, *initialExcludes;
  int newtonWorked;
  mp_prec_t p, p2, prec;
  mpfr_t startDiam, currDiameter, resultUp, resultDown, stopDiameter;
  int okay, oldtaylorrecursions, t;

  
  prec = startPrec;
  p = mpfr_get_prec(result);

  mpfr_init2(resultUp,p);
  mpfr_init2(resultDown,p);

  if (p > prec) {
    prec = p;
    printMessage(1,"Warning: starting intermediate precision increased to %d bits.\n",prec);
  }

  res.a = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
  res.b = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
  mpfr_init2(*(res.a),prec);
  mpfr_init2(*(res.b),prec);

  if (isTrivialInfnormCase(res, func)) {
    printMessage(2,"Information: the infnorm on the given function is trivially calculable.\n");
    mpfr_set(result,*(res.a),GMP_RNDU);
    mpfr_clear(*(res.a));
    mpfr_clear(*(res.b));
    free(res.a);
    free(res.b);
    return 1;
  }

  oldtaylorrecursions = taylorrecursions;
  t = determineHeuristicTaylorRecursions(func);
  if ((t > oldtaylorrecursions) && (t < ((oldtaylorrecursions + 1) * 2))) {
    taylorrecursions = t;
    printMessage(3,"Information: the number of Taylor recursions has temporarily been set to %d.\n",taylorrecursions);
  }

  curr = excludes;
  initialExcludes = NULL;
  while (curr != NULL) {
    excludeI = (sollya_mpfi_t *) safeMalloc(sizeof(sollya_mpfi_t));
    p = mpfr_get_prec(*(((rangetype *) curr->value)->a));
    p2 = mpfr_get_prec(*(((rangetype *) curr->value)->b));
    if (p2 > p) p = p2;
    if (prec > p) p = prec;
    sollya_mpfi_init2(*excludeI,p);
    sollya_mpfi_interv_fr(*excludeI,*(((rangetype *) curr->value)->a),*(((rangetype *) curr->value)->b));
    initialExcludes = addElement(initialExcludes,(void *) excludeI);
    curr = curr->next;
  }

  sollya_mpfi_init2(rangeI,prec);
  sollya_mpfi_init2(resI,prec);
  mpfr_init2(rangeDiameter,prec);
  mpfr_sub(rangeDiameter,*(range.b),*(range.a),GMP_RNDD);

  mpfr_init2(startDiam,prec);
  mpfr_set_d(startDiam,DEFAULTDIAM,GMP_RNDD);

  mpfr_mul(rangeDiameter,rangeDiameter,startDiam,GMP_RNDD);
  
  mpfr_clear(startDiam);

  sollya_mpfi_interv_fr(rangeI,*(range.a),*(range.b));
  deriv = differentiate(func);

  if (getNumeratorDenominator(&numeratorDeriv,&denominatorDeriv,deriv)) {
    printMessage(1,"Warning: the derivative of the function is a quotient, thus possibly not continuous in the interval.\n");
    printMessage(1,"Only the zeros of the numerator will be searched and pole detection may fail.\n");
    printMessage(1,"Be sure that the function is twice continuously differentiable if trusting the infnorm result.\n");

    mpfr_init2(z,prec);
    mpfr_init2(ya,prec);
    mpfr_init2(yb,prec);

    derivDenominatorDeriv = differentiate(denominatorDeriv);

    newtonWorked = newtonMPFR(z, denominatorDeriv, derivDenominatorDeriv, *(range.a), *(range.b), prec);

    if (newtonWorked && mpfr_number_p(z)) {
      evaluate(ya,numeratorDeriv,z,prec);
      evaluate(yb,denominatorDeriv,z,prec);

      mpfr_abs(ya,ya,GMP_RNDN);
      mpfr_abs(yb,yb,GMP_RNDN);

      mpfr_mul_ui(yb,yb,2,GMP_RNDN);

      if (mpfr_cmp(ya,yb) <= 0) {
	printMessage(1,"Warning: the derivative of the function seems to have a extensible singularity in ");
	if (verbosity >= 1) { 	changeToWarningMode(); printValue(&z); restoreMode(); }
	printMessage(1,".\n");
	printMessage(1,"The infnorm result might not be trustful if the derivative cannot actually\n");
	printMessage(1,"be extended in this point.\n");
      } else {
	printMessage(1,"Warning: the derivative of the function seems to have a singularity in ");
	if (verbosity >= 1) { 	changeToWarningMode(); printValue(&z); restoreMode(); }
	printMessage(1,".\n");
	printMessage(1,"The infnorm result is likely to be wrong.\n");
      }
    } else {
      evaluate(ya,denominatorDeriv,*(range.a),prec);
      evaluate(yb,denominatorDeriv,*(range.b),prec);

      if (mpfr_sgn(ya) != mpfr_sgn(yb)) {
	printMessage(1,"Warning: the derivative of the function seems to have a (extensible) singularity in the considered interval.\n");
	printMessage(1,"The infnorm result might be not trustful if the function is not continuously differentiable.\n");
      } else {
	printMessage(2,"Information: the derivative seems to have no (false) pole in the considered interval.\n");
      }
    }

    mpfr_clear(z);
    mpfr_clear(ya);
    mpfr_clear(yb);
    free_memory(derivDenominatorDeriv);
    free_memory(denominatorDeriv);
  }
  derivNumeratorDeriv = differentiate(numeratorDeriv);

  mpfr_init2(currDiameter, prec);
  mpfr_init2(stopDiameter, prec);

  mpfr_div_2ui(stopDiameter,rangeDiameter,20,GMP_RNDD);

  okay = 0;

  while (prec <= 512 * startPrec) {

    mpfr_set(currDiameter,rangeDiameter,GMP_RNDD);

    while (mpfr_cmp(currDiameter,stopDiameter) >= 0) {

      infnormI(resI,func,deriv,numeratorDeriv,derivNumeratorDeriv,rangeI,
	       prec,currDiameter,initialExcludes,NULL,NULL);
    
      sollya_mpfi_get_left(resultDown,resI);
      sollya_mpfi_get_right(resultUp,resI);

      if (mpfr_cmp(resultDown,resultUp) == 0) {
	okay = 1;
	break;
      }

      mpfr_nextabove(resultDown);

      if (mpfr_cmp(resultDown,resultUp) == 0) {
	okay = 1;
	break;
      }
    
      mpfr_div_2ui(currDiameter,currDiameter,2,GMP_RNDD);

      printMessage(4,"Information: the absolute diameter is now ");
      if (verbosity >= 4) {
	changeToWarningMode();
	printMpfr(currDiameter);
	restoreMode();
      }
      printMessage(4,"The current intermediate precision is %d bits.\n",(int) prec);

    }

    if (okay) break;

    prec *= 2;

    printMessage(4,"Information: the intermediate precision is now %d bits.\n",(int) prec);

  }

  if (okay) mpfr_set(result,resultUp,GMP_RNDU);

  mpfr_clear(*(res.a));
  mpfr_clear(*(res.b));
  free(res.a);
  free(res.b);

  mpfr_clear(stopDiameter);
  mpfr_clear(currDiameter);
  free_memory(deriv);
  free_memory(numeratorDeriv);
  free_memory(derivNumeratorDeriv);
  sollya_mpfi_clear(rangeI);
  sollya_mpfi_clear(resI);
  mpfr_clear(rangeDiameter);
  mpfr_clear(resultUp);
  mpfr_clear(resultDown);

  taylorrecursions = oldtaylorrecursions;

  return okay;
}

/* Tries to evaluate func(x) with faithful rounding to the precision of result                            */
/* If it can be proven that |func(x)|<cutoff, the returned value is 2 and result ~ func(x)                */
/* If the faithful value can successfully be computed, it is stored in result and the returned value is 1 */
/* If after increasing the precision many times, a satisfying value has not been obtained, result is set  */
/* to NaN and the returned value is:                                                                      */
/*    3 if the last computed value was NaN, or Inf                                                        */
/*    0 if the last computed value was a regular number (in which case, func(x) might be an exact 0)      */
int evaluateFaithfulWithCutOffFastOld(mpfr_t result, node *func, node *deriv, mpfr_t x, mpfr_t cutoff, mp_prec_t startprec) {
  mp_prec_t p, prec, oldPrec, oldPrec2;
  rangetype xrange, yrange;
  int okay;
  mpfr_t resUp, resDown;


  prec = mpfr_get_prec(result);
  mpfr_init2(resUp,prec);
  mpfr_init2(resDown,prec);

  p = mpfr_get_prec(x);
  xrange.a = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
  xrange.b = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
  mpfr_init2(*(xrange.a),p);
  mpfr_init2(*(xrange.b),p);
  mpfr_set(*(xrange.a),x,GMP_RNDD);
  mpfr_set(*(xrange.b),x,GMP_RNDU);

  if (p > prec) prec = p;
  if (startprec > prec) prec = startprec;

  oldPrec = tools_precision;
  oldPrec2 = defaultprecision;

  // tools_precision = prec;
  // defaultprecision = prec;


  yrange.a = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
  yrange.b = (mpfr_t *) safeMalloc(sizeof(mpfr_t));

  p=startprec;
  okay = 0;
  while (p < prec * 512) {
    mpfr_init2(*(yrange.a),p);
    mpfr_init2(*(yrange.b),p);
    evaluateRangeFunctionFast(yrange, func, deriv, xrange, p);
    mpfr_set(resDown,*(yrange.a),GMP_RNDN);
    mpfr_set(resUp,*(yrange.b),GMP_RNDN);
    if ((mpfr_number_p(resDown)) &&
	(mpfr_number_p(resUp)) &&
	(mpfr_number_p(*(yrange.a))) &&
	(mpfr_number_p(*(yrange.b)))
	) {
      if (mpfr_cmp(resDown,resUp) == 0)
	okay = 1;
      mpfr_nextabove(resDown);
      if (mpfr_cmp(resDown,resUp) == 0)
	okay = 1;
      if (okay == 0) {
	if ((mpfr_cmpabs(*(yrange.a),cutoff) < 0) && (mpfr_cmpabs(*(yrange.b),cutoff) < 0)) {
	  mpfr_add(*(yrange.a),*(yrange.b),*(yrange.a),GMP_RNDN);
	  mpfr_div_2ui(*(yrange.a),*(yrange.a),1,GMP_RNDN);
	  mpfr_set(resUp,*(yrange.a),GMP_RNDN);
	  okay = 2;
	}
      }
    }
    mpfr_clear(*(yrange.a));
    mpfr_clear(*(yrange.b));
    if (okay > 0) break;
    p *= 2;
  }
  mpfr_clear(*(xrange.a));
  mpfr_clear(*(xrange.b));

  if (okay > 0) {
    mpfr_set(result,resUp,GMP_RNDN);
  } else {
    mpfr_set_nan(result);
    if( (!mpfr_number_p(resUp)) || (!mpfr_number_p(resDown))) okay=3;
  }
  
  free(yrange.a);
  free(yrange.b);
  free(xrange.a);
  free(xrange.b);

  mpfr_clear(resUp);
  mpfr_clear(resDown);


  // tools_precision = oldPrec;
  // defaultprecision = oldPrec2;

  return okay;
}

int evaluateFaithfulWithCutOffFast(mpfr_t result, node *func, node *deriv, mpfr_t x, mpfr_t cutoff, mp_prec_t startprec) {
  mp_prec_t p, prec;
  sollya_mpfi_t yI, xI, cutoffI;
  int okay;
  mpfr_t resUp, resDown;
  mpfr_t cutoffLeft, cutoffRight;
  mpfr_t yILeft, yIRight;
  int testCutOff;
  
  /* We test the cutoff only if it is not zero */
  testCutOff = 1;

  prec = mpfr_get_prec(cutoff);
  mpfr_init2(cutoffLeft, prec);
  mpfr_init2(cutoffRight, prec);
  sollya_mpfi_init2(cutoffI,prec);
  mpfr_abs(cutoffRight,cutoff,GMP_RNDU);

  /* We test the cutoff only if it is not zero */
  mpfr_set_si(cutoffLeft,0,GMP_RNDN);
  if (mpfr_cmp(cutoffRight,cutoffLeft) == 0) testCutOff = 0; 

  mpfr_neg(cutoffLeft,cutoffRight,GMP_RNDD);
  sollya_mpfi_interv_fr(cutoffI,cutoffLeft,cutoffRight);
  mpfr_clear(cutoffLeft);
  mpfr_clear(cutoffRight);

  /* Need a little more because we always take the upper value below */
  prec = mpfr_get_prec(result)+5;
  mpfr_init2(resUp,prec);
  mpfr_init2(resDown,prec);

  /* Copy x into an interval with its own precision */
  p = mpfr_get_prec(x);
  sollya_mpfi_init2(xI,p);
  sollya_mpfi_interv_fr(xI,x,x);

  if (startprec > prec) prec = startprec;

  /* Testing (comparing the final prec with the startprec on the examples in the check files)
     shows we should take anyway a little more in the beginning.

     Let's take it times 1.25, because that's easy to compute on integer.
  */
  startprec = startprec + (startprec >> 2);

  /* Use up a little more memory with the first malloc 
     The starting subsequent mpf*_set_prec will not malloc
  */
  sollya_mpfi_init2(yI,startprec*16);
  mpfr_init2(yILeft,startprec*16);
  mpfr_init2(yIRight,startprec*16);
  p=startprec;
  okay = 0;
  while (p < prec * 512) {
    sollya_mpfi_set_prec(yI,p);
    mpfr_set_prec(yILeft,p);
    mpfr_set_prec(yIRight,p);
    evaluateInterval(yI, func, deriv, xI);
    sollya_mpfi_get_left(yILeft,yI);
    sollya_mpfi_get_right(yIRight,yI);
    mpfr_set(resDown,yILeft,GMP_RNDN);
    mpfr_set(resUp,yIRight,GMP_RNDN);
    /*
      No, two sollya_mpfi_get_left/right on resDown and resUp directly do not work:

      ----|-----------|-----------|----
                   |--yI-|
          left        RN          right
     */
    if ((mpfr_number_p(resDown)) &&
	(mpfr_number_p(resUp)) &&
	(sollya_mpfi_bounded_p(yI))
	) {
      if (mpfr_cmp(resDown,resUp) == 0) 
	okay = 1;
      mpfr_nextabove(resDown);
      if (mpfr_cmp(resDown,resUp) == 0) 
	okay = 1;
      if (testCutOff && (okay == 0)) {
	if (sollya_mpfi_is_inside(yI,cutoffI)) {
	  sollya_mpfi_mid(resUp,yI);
	  okay = 2;
	} 
      }
    }
    if (okay > 0) break;
    p <<= 1;
  }
  sollya_mpfi_clear(xI);
  sollya_mpfi_clear(yI);
  mpfr_clear(yILeft);
  mpfr_clear(yIRight);

  if (okay > 0) {
    /* This rouning annihilates the effect of taking always the upper value */
    mpfr_set(result,resUp,GMP_RNDN);
  } else {
    mpfr_set_nan(result);
    if( (!mpfr_number_p(resUp)) || (!mpfr_number_p(resDown))) okay=3;
  }
 
  mpfr_clear(resUp);
  mpfr_clear(resDown);
  sollya_mpfi_clear(cutoffI);
  return okay;
}


int evaluateFaithfulWithCutOff(mpfr_t result, node *func, mpfr_t x, mpfr_t cutoff, mp_prec_t startprec) {
  node *deriv;
  int res;

  if ((2*startprec) < (mpfr_get_prec(result) + 10)) {
    printMessage(12,"Information: Differentiating while evaluating because start precision (%d bits) too low.\n",
		 (int)startprec);
    deriv = differentiate(func); 
  }else deriv = NULL;
  res = evaluateFaithfulWithCutOffFast(result, func, deriv, x, cutoff, startprec);
  if (res==3) res=0;
  if (deriv != NULL) free_memory(deriv);
  return res;
}


node *makeDoubleConstant(double d) {
  node *tempNode;

  tempNode = (node *) safeMalloc(sizeof(node));
  tempNode->nodeType = CONSTANT;
  tempNode->value = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
  mpfr_init2(*(tempNode->value),53);
  mpfr_set_d(*(tempNode->value),d,GMP_RNDN);
  
  return tempNode;
}

int evaluateSign(int *s, node *func);

node *convertConstantToFunctionInPiInner(node *tree) {
  node *res;
  int a;

  if (tree->nodeType == PI_CONST) {
    res = (node *) safeMalloc(sizeof(node));
    res->nodeType = VARIABLE;
    return res;
  } 

  a = arity(tree);
  switch (a) {
  case 0:
    res = copyTree(tree);
    break;
  case 1:
    res = (node *) safeMalloc(sizeof(node));
    res->nodeType = tree->nodeType;
    if (tree->nodeType == LIBRARYFUNCTION) {
      res->libFun = tree->libFun;
      res->libFunDeriv = tree->libFunDeriv;
    }
    if (tree->nodeType == PROCEDUREFUNCTION) {
      res->libFunDeriv = tree->libFunDeriv;
      res->child2 = copyThing(tree->child1);
    }
    res->child1 = convertConstantToFunctionInPiInner(tree->child1);
    break;
  case 2:
    res = (node *) safeMalloc(sizeof(node));
    res->nodeType = tree->nodeType;
    res->child1 = convertConstantToFunctionInPiInner(tree->child1);
    res->child2 = convertConstantToFunctionInPiInner(tree->child2);
    break;
  default:
    sollyaFprintf(stderr,"Error: convertConstantToFunctionInPiInner: unknown arity (%d).\n",a);
    exit(1);
  }

  return res;
}

node *convertConstantToFunctionInPi(node *tree) {
  if (!isConstant(tree)) return NULL;
  return convertConstantToFunctionInPiInner(tree);
}


int containsPi(node *tree) {
  int a;

  if (tree->nodeType == PI_CONST) return 1;
  
  a = arity(tree);
  switch (a) {
  case 0:
    return 0;
    break;
  case 1:
    return containsPi(tree->child1);
    break;
  case 2:
    return (containsPi(tree->child1) || containsPi(tree->child2));
  default:
    sollyaFprintf(stderr,"Error: containsPi: unknown arity (%d).\n",a);
    exit(1);
  }
  return 0;
}


int compareConstant(int *cmp, node *func1, node *func2) {
  node *diff, *rawDiff, *rawDiff2;
  int res, okay;
  mpfr_t value, dummyX;
  int okayA, okayB, signA, signB;
  node *tempNode;
  node **coefficients;
  int degree, i;
  int allZero, allOkay;

  okay = 0;
  rawDiff = makeSub(copyTree(func1),copyTree(func2));
  rawDiff2 = simplifyRationalErrorfree(rawDiff);
  diff = simplifyTreeErrorfree(rawDiff2);
  free_memory(rawDiff);
  free_memory(rawDiff2);
  mpfr_init2(value,12);
  mpfr_init2(dummyX,12);
  mpfr_set_ui(dummyX,1,GMP_RNDN);
  if (evaluateFaithful(value, diff, dummyX, defaultprecision) &&
      mpfr_number_p(value)) { 
    res = mpfr_sgn(value);
    okay = 1;
  } else {
    if ((func1->nodeType == DIV) && 
	evaluateSign(&signA,func1->child2) && 
	(signA != 0)) {
      tempNode = makeMul(copyTree(func1->child2),copyTree(func2));
      okayB = compareConstant(&signB, func1->child1, tempNode);
      if (okayB) {
	okay = 1;
	res = signB;
      }
      free_memory(tempNode);
    } 
    if (!okay) {
      if ((func2->nodeType == DIV) && 
	  evaluateSign(&signA,func2->child2) && 
	  (signA != 0)) {
	tempNode = makeMul(copyTree(func2->child2),copyTree(func1));
	okayB = compareConstant(&signB, tempNode, func2->child1);
	if (okayB) {
	  okay = 1;
	  res = signB;
	}
	free_memory(tempNode);
      } 
      if (!okay) {
	if (((func1->nodeType == EXP) && (func2->nodeType == EXP)) ||
	    ((func1->nodeType == SINH) && (func2->nodeType == SINH)) || 
	    ((func1->nodeType == TANH) && (func2->nodeType == TANH)) || 
	    ((func1->nodeType == ASINH) && (func2->nodeType == ASINH)) || 
	    ((func1->nodeType == ERF) && (func2->nodeType == ERF)) || 
	    ((func1->nodeType == EXP_M1) && (func2->nodeType == EXP_M1))) {
	  okayA = compareConstant(&signA, func1->child1, func2->child1);
	  if (okayA) {
	    okay = 1;
	    res = signA;
	  }
	}
	if (!okay) {
	  if (((func1->nodeType == ERFC) && (func2->nodeType == ERFC)) ||
	      ((func1->nodeType == NEG) && (func2->nodeType == NEG))) {
	    okayA = compareConstant(&signA, func1->child1, func2->child1);
	    if (okayA) {
	      okay = 1;
	      res = -signA;
	    }
	  }
	  if (!okay) {
	    if (containsPi(diff)) {
	      if ((tempNode = convertConstantToFunctionInPi(diff)) != NULL) {
		if (isPolynomial(tempNode)) {
		  // Here we have tempNode(pi) = diff and tempNode a polynomial
		  //
		  getCoefficients(&degree, &coefficients, tempNode);
		  if (degree >= 0) {
		    allZero = 1; allOkay = 1;
		    for (i=0;i<=degree;i++) {
		      if (coefficients[i] != NULL) {
			if (evaluateSign(&signA, coefficients[i])) {
			  if (signA != 0) {
			    allZero = 0;
			    break;
			  }
			} else {
			  allOkay = 0;
			  break;
			}
		      }
		    }
		    if (allOkay && allZero) {
		      okay = 1;
		      res = 0;
		    }
		    for (i=0;i<=degree;i++) 
		      if (coefficients[i] != NULL) free_memory(coefficients[i]);
		    free(coefficients);
		  }
		}
		free_memory(tempNode);
	      }
	    }
	    // Put next case here
	  }
	}
      }
    }
  }

  mpfr_clear(dummyX);
  mpfr_clear(value);
  free_memory(diff);

  if (okay) *cmp = res;
  return okay;
}

int evaluateSignTrigoUnsafe(int *s, node *child, int nodeType) {
  mpfr_t value, value2;
  mpfr_t piHalf;
  mpfr_t dummyX;
  int okay, res;
  node *tempNode;
  int signA;

  okay = 0;

  mpfr_init2(value,defaultprecision);
  mpfr_init2(piHalf,defaultprecision);
  mpfr_init2(dummyX,12);
  mpfr_set_ui(dummyX,1,GMP_RNDN);
  if (evaluateFaithful(value, child, dummyX, defaultprecision) &&
      mpfr_number_p(value)) { 
    mpfr_const_pi(piHalf,GMP_RNDN);
    mpfr_div_2ui(piHalf,piHalf,1,GMP_RNDN);
    mpfr_div(value,value,piHalf,GMP_RNDN);
    mpfr_rint(value,value,GMP_RNDN);
    mpfr_div_2ui(value,value,1,GMP_RNDN);
    // Here, diff is approximately value * pi
    // and value * 2 is an integer
    tempNode = makeMul(makeConstant(value),makePi());
    if (compareConstant(&signA, child, tempNode)) {
      if (signA == 0) {
	// Here, we have proven that child is equal to value * pi
	//
	mpfr_init2(value2,defaultprecision);
	mpfr_rint(value2,value,GMP_RNDN);      // exact, same precision
	mpfr_sub(value,value,value2,GMP_RNDN); // exact, Sterbenz
	// Here, we know that child is equal to (n + value) * pi for
	// some integer n. We know that value can only be 0 or +/- 0.5
	switch (nodeType) {
	case SIN:
	  // sin is zero for all n * pi, n in Z
	  if (mpfr_zero_p(value)) {
	    okay = 1;
	    res = 0;
	  }
	  break;
	case COS:
	  // cos is zero for all (n + 1/2) * pi, n in Z
	  if (!mpfr_zero_p(value)) {
	    okay = 1;
	    res = 0;
	  }
	  break;
	case TAN:
	  // tan is zero for all n * pi, n in Z
	  if (mpfr_zero_p(value)) {
	    okay = 1;
	    res = 0;
	  }
	  break;
	default:
	  sollyaFprintf(stderr,"Error: evaluateSignTrigoUnsafe: unknown identifier (%d) in the tree\n",nodeType);
	  exit(1);
	}
	mpfr_clear(value2);
      }
    }
    free_memory(tempNode);
  }
  mpfr_clear(dummyX);
  mpfr_clear(piHalf);
  mpfr_clear(value);

  if (okay) *s = res;
  return okay;
}



int evaluateSign(int *s, node *rawFunc) {
  int sign, okay, okayA, okayB, okayC;
  mpfr_t value, dummyX;
  sollya_mpfi_t valueI;
  int signA, signB, signC;
  node *tempNode, *tempNode2;
  node *func, *rawFunc2;
  
  okay = 0;
  if (!isConstant(rawFunc)) return 0;

  if ((rawFunc->nodeType == CONSTANT) &&
      (!mpfr_number_p(*(rawFunc->value)))) return 0;

  mpfr_init2(value,12);
  mpfr_init2(dummyX,12);
  mpfr_set_ui(dummyX,1,GMP_RNDN);
  if (evaluateFaithful(value, rawFunc, dummyX, defaultprecision) &&
      mpfr_number_p(value)) { 
    sign = mpfr_sgn(value);
    okay = 1;
  } else {
    rawFunc2 = simplifyRationalErrorfree(rawFunc);
    func = simplifyTreeErrorfree(rawFunc2);
    free_memory(rawFunc2);
    if (evaluateFaithful(value, func, dummyX, defaultprecision) &&
	mpfr_number_p(value)) {
      sign = mpfr_sgn(value);
      okay = 1;
    } else {
      switch (func->nodeType) {
      case CONSTANT:
	sign = mpfr_sgn(*(func->value));
	okay = 1;
	break;
      case ADD:
	tempNode = makeNeg(copyTree(func->child2));
	okay = compareConstant(&sign, func->child1, func->child2);
	free_memory(tempNode);
	break;
      case SUB:
	okay = compareConstant(&sign, func->child1, func->child2);
	break;
      case MUL:
	okay = (evaluateSign(&signA, func->child1) && evaluateSign(&signB, func->child2));
	sign = signA * signB;
	break;
      case DIV:
	okayA = (evaluateSign(&signA, func->child1) && evaluateSign(&signB, func->child2));
	if (okayA && (signB != 0)) {
	  okay = 1;
	  sign = signA * signB;
	}
	break;
      case SQRT:
	okayA = evaluateSign(&signA, func->child1);
	if (okayA && (signA >= 0)) {
	  okay = 1;
	  sign = signA;
	}
	break;
      case EXP:
	okayA = evaluateSign(&signA, func->child1);
	if (okayA) {
	  okay = 1;
	  sign = 1;
	}
	break;
      case LOG:
	// fall-through
      case LOG_2:
	// fall-through
      case LOG_10:
	tempNode = makeDoubleConstant(1.0);
	okayA = compareConstant(&signA, func->child1, tempNode);
	okayB = evaluateSign(&signB, func->child1);
	if (okayA && okayB && (signB > 0)) {
	  okay = 1;
	  sign = signA;
	}
	free_memory(tempNode);
	break;
      case SIN:
	// fall-through
      case COS:
	// fall-through
      case TAN:
	okayA = evaluateSign(&signA, func->child1);
	if (okayA && (signA == 0)) {
	  okay = 1;
	  sign = 0;
	} else {
	  okay = evaluateSignTrigoUnsafe(&sign, func->child1, func->nodeType);
	}
	break;
      case ASIN:
	okayA = evaluateSign(&signA, func->child1);
	tempNode = makeAbs(copyTree(func->child1));
	tempNode2 = makeDoubleConstant(1.0);
	okayB = compareConstant(&signB, tempNode, tempNode2);
	if (okayA && okayB && (signB <= 0)) {
	  okay = 1;
	  sign = signA;
	}
	free_memory(tempNode);
	free_memory(tempNode2);
	break;
      case ACOS:
	okayA = evaluateSign(&signA, func->child1);
	tempNode = makeAbs(copyTree(func->child1));
	tempNode2 = makeDoubleConstant(1.0);
	okayB = compareConstant(&signB, tempNode, tempNode2);
	okayC = compareConstant(&signC, func->child1, tempNode2);
	if (okayA && okayB && okayC && (signB <= 0)) {
	  okay = 1;
	  if (signC == 0) sign = 0; else sign = 1;
	}
	free_memory(tempNode);
	free_memory(tempNode2);
	break;
      case ATAN:
	okay = evaluateSign(&sign, func->child1);
	break;
      case SINH:
	okay = evaluateSign(&sign, func->child1);
	break;
      case COSH:
	okay = 1;
	sign = 1;
	break;
      case TANH:
	okay = evaluateSign(&sign, func->child1);
	break;
      case ASINH:
	okay = evaluateSign(&sign, func->child1);
	break;
      case ACOSH:
	tempNode = makeDoubleConstant(1.0);
	okayA = compareConstant(&signA, func->child1, tempNode);
	if (okayA && (signA >= 0)) {
	  okay = 1;
	  sign = 1;
	}
	free_memory(tempNode);
	break;
      case ATANH:
	okayA = evaluateSign(&signA, func->child1);
	tempNode = makeAbs(copyTree(func->child1));
	tempNode2 = makeDoubleConstant(1.0);
	okayB = compareConstant(&signB, tempNode, tempNode2);
	if (okayA && okayB && (signB < 0)) {
	  okay = 1;
	  sign = signA;
	}
	free_memory(tempNode);
	free_memory(tempNode2);
	break;
      case POW:
	okayA = evaluateSign(&signA, func->child1);
	okayB = evaluateSign(&signB, func->child1);
	if (okayA && okayB) {
	  if (okayB == 0) {
	    if (okayA != 0) {
	      okay = 1;
	      sign = 1;
	    }
	  } else {
	    if (okayA == 0) {
	      okay = 1;
	      sign = 0;
	    }
	  }
	}
	break;
      case NEG:
	okay = evaluateSign(&signA, func->child1);
	sign = -1 * signA;
	break;
      case ABS:
	okayA = evaluateSign(&signA, func->child1);
	if (okayA) {
	  okay = 1;
	  if (signA == 0) sign = 0; else sign = 1;
	}
	break;
      case DOUBLE:
	break;
      case SINGLE:
	break;
      case QUAD:
	break;
      case HALFPRECISION:
	break;
      case DOUBLEDOUBLE:
	break;
      case TRIPLEDOUBLE:
	break;
      case ERF:
	okay = evaluateSign(&sign, func->child1); 
	break;
      case ERFC:
	okay = 1;
	sign = 1;
	break;
      case LOG_1P:
	tempNode = makeDoubleConstant(-1.0);
	okayA = compareConstant(&signA, func->child1, tempNode);
	okayB = evaluateSign(&signB, func->child1);
	if (okayA && okayB && (signA > 0)) {
	  okay = 1;
	  sign = signB;
	}
	free_memory(tempNode);
	break;
      case EXP_M1:
	okay = evaluateSign(&sign, func->child1);
	break;
      case DOUBLEEXTENDED:
	break;
      case LIBRARYFUNCTION:
	break;
      case PROCEDUREFUNCTION:
	break;
      case CEIL:
	okayA = evaluateSign(&signA, func->child1);
	tempNode = makeDoubleConstant(-1.0);
	if (okayA) 
	  okayB = compareConstant(&signB, func->child1, tempNode);
	else
	  okayB = 0;
	if (okayA && okayB) {
	  okay = 1;
	  if (signB <= 0) {
	    sign = -1;
	  } else {
	    if (signA <= 0) {
	      sign = 0;
	    } else {
	      sign = 1;
	    }
	  }
	}
	free_memory(tempNode);
	break;
      case FLOOR:
	okayA = evaluateSign(&signA, func->child1);
	tempNode = makeDoubleConstant(1.0);
	if (okayA) 
	  okayB = compareConstant(&signB, func->child1, tempNode);
	else
	  okayB = 0;
	if (okayA && okayB) {
	  okay = 1;
	  if (signA < 0) {
	    sign = -1;
	  } else {
	    if (signB < 0) {
	      sign = 0;
	    } else {
	      sign = 1;
	    }
	  }
	}
	free_memory(tempNode);
	break;
      case NEARESTINT:
	okayA = evaluateSign(&signA, func->child1);
	tempNode = makeDoubleConstant(1.0);
	if (okayA) 
	  okayB = compareConstant(&signB, func->child1, tempNode);
	else
	  okayB = 0;
	if (okayA && okayB) {
	  okay = 1;
	  if (signA < 0) {
	    sign = -1;
	  } else {
	    if (signB < 0) {
	      sign = 0;
	    } else {
	      sign = 1;
	    }
	  }
	}
	free_memory(tempNode);
	break;
      case PI_CONST:
	okay = 1;
	sign = 1;
	break;
      case LIBRARYCONSTANT:
        /* By definition, a library constant is known with a relative error
           smaller that ~ 2^(-prec). So we can decide the sign, based on low
           approximation of the constant. */
        sollya_mpfi_init2(valueI, 12); 
        libraryConstantToInterval(valueI, func);
        if (sollya_mpfi_is_zero(valueI)) {
          okay = 1;
          sign = 0;
        }
        else {
          if (sollya_mpfi_has_zero(valueI)) {
            okay = 0;
            sign = 0;
          }
          else {
            okay = 1;
            sign = (sollya_mpfi_is_nonneg(valueI))?1:(-1);
          }
        }
        break;
      default:
	sollyaFprintf(stderr,"Error: evaluateSign: unknown identifier (%d) in the tree\n",func->nodeType);
	exit(1);
      }
    }
    free_memory(func);
  }
  mpfr_clear(value);
  mpfr_clear(dummyX);

  if (okay) *s = sign;
  return okay;
}
