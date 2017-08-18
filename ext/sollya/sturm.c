/*

Copyright 2008-2011 by 

Laboratoire de l'Informatique du Parallelisme, 
UMR CNRS - ENS Lyon - UCB Lyon 1 - INRIA 5668,

Laboratoire d'Informatique de Paris 6, equipe PEQUAN,
UPMC Universite Paris 06 - CNRS - UMR 7606 - LIP6, Paris, France,

and by

Centre de recherche INRIA Sophia-Antipolis Mediterranee, equipe APICS,
Sophia Antipolis, France.

Contributors M. Joldes, Ch. Lauter, S. Chevillard

mioara.joldes@ens-lyon.fr
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
#include "expression.h"
#include <stdio.h> /* fprintf, fopen, fclose, */
#include <stdlib.h> /* exit, free, mktemp */
#include <errno.h>


//these are the functions that work on mpq_t


int sturm_mpq(int *n, mpq_t *p, int p_degree, sollya_mpfi_t x);
int polynomialDivide_mpq(mpq_t *quotient, int *quotient_degree, mpq_t *rest, int *rest_degree, mpq_t *p, int p_degree, mpq_t *q, int q_degree) ;
int polynomialDeriv_mpq(mpq_t **derivCoeff, int *deriv_degree, mpq_t *p, int p_degree);
int polynomialEval_mpq( mpq_t *res, mpq_t x, mpq_t *p, int p_degree);
int sturm_mpfi(int *n, mpq_t *p, int p_degree, sollya_mpfi_t x, mp_prec_t precision);
int polynomialDivide_mpfi(sollya_mpfi_t *quotient, int *quotient_degree, sollya_mpfi_t *rest, int *rest_degree, sollya_mpfi_t *p, int p_degree, sollya_mpfi_t *q, int q_degree, mp_prec_t prec) ;
int polynomialDeriv_mpfi(sollya_mpfi_t **derivCoeff, int *deriv_degree, sollya_mpfi_t *p, int p_degree, mp_prec_t prec);
int polynomialEval_mpfi( sollya_mpfi_t *res, sollya_mpfi_t x, sollya_mpfi_t *p, int p_degree);


void printMpq(mpq_t x) {
  mpz_t num;
  mpz_t denom;
  mpfr_t numMpfr;
  mpfr_t denomMpfr;
  mp_prec_t prec;
  int p;
  unsigned int dyadicValue;

  mpz_init(num);
  mpz_init(denom);

  mpq_get_num(num,x);
  mpq_get_den(denom,x);
  
  prec = mpz_sizeinbase(num, 2);
  dyadicValue = mpz_scan1(num, 0);
  p = prec - dyadicValue;
  if (p < 12) prec = 12; else prec = p; 
  mpfr_init2(numMpfr,prec);
  mpfr_set_z(numMpfr,num,GMP_RNDN);

  prec = mpz_sizeinbase(denom, 2);
  dyadicValue = mpz_scan1(denom, 0);
  p = prec - dyadicValue;
  if (p < 12) prec = 12; else prec = p; 
  mpfr_init2(denomMpfr,prec);
  mpfr_set_z(denomMpfr,denom,GMP_RNDN);

  printValue(&numMpfr); sollyaPrintf(" / "); printValue(&denomMpfr);

  mpfr_clear(numMpfr);
  mpfr_clear(denomMpfr);

  mpz_clear(num);
  mpz_clear(denom);

}




int polynomialDeriv_mpq(mpq_t **derivCoeff, int *deriv_degree, mpq_t *p, int p_degree){
  int i;
  mpq_t aux;

  if ((mpq_cmp_ui(p[p_degree],0,1))==0) 
    return 0;
  if (p_degree>=1)
    *deriv_degree=p_degree-1;
  else if (p_degree==0) *deriv_degree=0;
  else return 0; 
 
  *derivCoeff= (mpq_t *)safeMalloc((*deriv_degree+1)*sizeof(mpq_t));  
  mpq_init((*derivCoeff)[0]);//is set to 0
  mpq_init(aux);
  
  for (i=1; i<=p_degree;i++){
    if (i-1!=0) mpq_init((*derivCoeff)[i-1]);
    mpq_set_ui(aux,i,1);
    mpq_mul((*derivCoeff)[i-1],p[i],aux);
  }
  mpq_clear(aux);
  return 1;
}  


int polynomialEval_mpq( mpq_t *res, mpq_t x, mpq_t *p, int p_degree){
  int i;
  mpq_t pow, aux;
  mpq_init(pow);
  mpq_set_ui(pow,1,1);
  mpq_init(aux);
  mpq_set_ui(aux,1,1); 
  mpq_set_ui(*res,0,1);//is set to 0/1
  for (i=0; i<=p_degree; i++) {
    mpq_mul(aux,p[i],pow);
    mpq_add(*res, aux, *res);
    mpq_mul(pow, pow, x); 
  }

  
  mpq_clear(aux);
  mpq_clear(pow);
  
  return 1;
}


int polynomialDivide_mpq(mpq_t *quotient, int *quotient_degree, mpq_t *rest, int *rest_degree, mpq_t *p, int p_degree, mpq_t *q, int q_degree) {
  
  int i,step,k;
  mpq_t aux;
 
  *quotient_degree=p_degree-q_degree;
  
  if (((mpq_cmp_ui(q[q_degree],0,1))==0) || ((mpq_cmp_ui(p[p_degree],0,1))==0))
    return 0;
 
  mpq_init(aux);
  
  step=0;
  for (k=*quotient_degree; k>=0;k--)
    {

      mpq_set(quotient[k],p[p_degree-step]);
      mpq_div(quotient[k],quotient[k],q[q_degree]);
      for (i=q_degree; i>=0;i--){    
        mpq_mul(aux,quotient[k],q[i]);
        mpq_sub(p[p_degree-step-(q_degree-i)],p[p_degree-step-(q_degree-i)],aux);  
      }
      step++; 
    }
  *rest_degree=0;
  for (i=q_degree-1; i>=0; i--)  
    {
      if ((mpq_cmp_ui(p[i],0,1))!=0){
	*rest_degree=i;
	break;
      }
    }
  for (i=*rest_degree; i>=0; i--){ 
    mpq_set(rest[i],p[i]);
  }
  mpq_clear(aux);
  return 1;
}  


int sturm_mpq(int *n, mpq_t *p, int p_degree, sollya_mpfi_t x){
  mpq_t *quotient, *rest, *dp;
  int quotient_degree, rest_degree, dp_degree;
  mpq_t *evalResA;
  mpq_t *evalResB;
  mpq_t *s0, *s1;
  mpq_t evalRes;
  int s0_degree, s1_degree;
  mpfr_t a,b;
  mpq_t aq,bq;
  int i,na,nb,prec,nrRoots;
  int varSignB,varSignA;


  prec=sollya_mpfi_get_prec(x);
  na=0; nb=0;  
  nrRoots=0;
  mpfr_init2(a,prec);
  mpfr_init2(b,prec);
  sollya_mpfi_get_left(a,x);
  sollya_mpfi_get_right(b,x);
 
  mpq_init(aq);
  mpq_init(bq);
    
  mpfr_to_mpq(aq,a);
  mpfr_to_mpq(bq,b);
  
  evalResA = (mpq_t *)safeMalloc((p_degree+1)*sizeof(mpq_t));  
  evalResB = (mpq_t *)safeMalloc((p_degree+1)*sizeof(mpq_t));
  for (i=0; i<=p_degree; i++){ 
    mpq_init(evalResA[i]);
    mpq_init(evalResB[i]);
  }
    
  s0=(mpq_t *)safeMalloc((p_degree+1)*sizeof(mpq_t));
  quotient=(mpq_t *)safeMalloc((p_degree+1)*sizeof(mpq_t));
  rest=(mpq_t *)safeMalloc((p_degree+1)*sizeof(mpq_t));
  for (i=0; i<=p_degree; i++){ 
    mpq_init(quotient[i]);
    mpq_init(rest[i]);
  }

  s0_degree=p_degree;

  for (i=0; i<=p_degree; i++){ 
    mpq_init(s0[i]); 
    mpq_set(s0[i],p[i]);
  }
        
  polynomialDeriv_mpq(&dp, &dp_degree, p, p_degree);
  mpq_init(evalRes);
    
  polynomialEval_mpq( &evalRes, aq, s0, s0_degree);
  if (mpq_cmp_ui(evalRes,0,1)!=0){
     mpq_set(evalResA[na],evalRes);
    na++;
  }
  else nrRoots++; /*if the left extremity is a root we count it here
                    since the number of sign changes gives the nr of distinct 
                    roots between a and b, a<b*/  
  polynomialEval_mpq( &evalRes, bq, s0, s0_degree);
  if (mpq_cmp_ui(evalRes,0,1)!=0){
    mpq_set(evalResB[nb],evalRes);
    nb++;
  }
  s1=dp;
  s1_degree=dp_degree;
  if (s0_degree>0){

    polynomialEval_mpq( &evalRes, aq, dp, dp_degree);
    if (mpq_cmp_ui(evalRes,0,1)!=0){
      mpq_set(evalResA[na],evalRes);
      na++;
    }
  
    polynomialEval_mpq( &evalRes, bq, dp, dp_degree);
    if (mpq_cmp_ui(evalRes,0,1)!=0){
      mpq_set(evalResB[nb],evalRes);
      nb++;
    }

  }
  
  while (s1_degree!=0){
          
    polynomialDivide_mpq(quotient, &quotient_degree, rest, &rest_degree, s0, s0_degree, s1, s1_degree) ;
    
    s0_degree=s1_degree; 
    for(i=0;i<=s0_degree;i++)
      mpq_set(s0[i],s1[i]);
    
    s1_degree=rest_degree; 
    
    for(i=0;i<=s1_degree;i++){
      mpq_set(s1[i],rest[i]);
    }
    
    for (i=0; i<=s1_degree; i++)
      mpq_neg(s1[i],s1[i]);
    
    polynomialEval_mpq( &evalRes, aq, s1, s1_degree);
    if (mpq_cmp_ui(evalRes,0,1)!=0){
      mpq_set(evalResA[na],evalRes);
      na++;
    }
    
  
    polynomialEval_mpq( &evalRes, bq, s1, s1_degree);
    if (mpq_cmp_ui(evalRes,0,1)!=0){
      mpq_set(evalResB[nb],evalRes);
      nb++;
    }
    
  }
  
  varSignA=0;
  for (i=1; i<na; i++){
    if ((mpq_cmp_ui(evalResA[i-1],0,1) * mpq_cmp_ui(evalResA[i],0,1))<0) 
      varSignA++;
    
  } 
  varSignB=0;
  for (i=1; i<nb; i++){
    if ((mpq_cmp_si(evalResB[i-1],0,1) * mpq_cmp_si(evalResB[i],0,1))<0) 
      varSignB++;
  }

  *n=(((varSignA-varSignB)>0)?(varSignA-varSignB+nrRoots):(varSignB-varSignA+nrRoots) );

  for (i=0; i<=p_degree; i++){
    mpq_clear(s0[i]);
    mpq_clear(quotient[i]);
    mpq_clear(rest[i]);
    mpq_clear(evalResA[i]);
    mpq_clear(evalResB[i]);
  }
  for (i=0; i<=dp_degree; i++){ 
    mpq_clear(dp[i]);
  }
  free(dp);
  free(s0);
  free(evalResA);
  free(evalResB);
  free(quotient); 
  free(rest); 
  mpfr_clear(a);
  mpfr_clear(b);
  mpq_clear(aq);
  mpq_clear(bq);
  mpq_clear(evalRes);

  return 1;
}

int polynomialDeriv_mpfi(sollya_mpfi_t **derivCoeff, int *deriv_degree, sollya_mpfi_t *p, int p_degree, mp_prec_t prec){
  int i;
  sollya_mpfi_t aux;

  if (sollya_mpfi_is_zero(p[p_degree])) 
    return 0;
  if (p_degree>=1)
    *deriv_degree=p_degree-1;
  else if (p_degree==0) *deriv_degree=0;
  else return 0; 
 
  *derivCoeff= (sollya_mpfi_t *)safeMalloc((*deriv_degree+1)*sizeof(sollya_mpfi_t));  
  sollya_mpfi_init2((*derivCoeff)[0],prec);
  sollya_mpfi_set_ui((*derivCoeff)[0],0);
  sollya_mpfi_init2(aux,prec);
  
  for (i=1; i<=p_degree;i++){
    if (i-1!=0) sollya_mpfi_init2((*derivCoeff)[i-1],prec);
    sollya_mpfi_set_ui(aux,i);
    sollya_mpfi_mul((*derivCoeff)[i-1],p[i],aux);
  }
  sollya_mpfi_clear(aux);
  return 1;
}  


int polynomialEval_mpfi( sollya_mpfi_t *res, sollya_mpfi_t x, sollya_mpfi_t *p, int p_degree){
  int i;

  sollya_mpfi_set_ui(*res,0);
  sollya_mpfi_set(*res,p[p_degree]);
  for (i=p_degree-1;i>=0;i--) {
    sollya_mpfi_mul(*res,*res,x);
    sollya_mpfi_add(*res,*res,p[i]);
  }

  return 1;
}

int polynomialDivide_mpfi(sollya_mpfi_t *quotient, int *quotient_degree, sollya_mpfi_t *rest, int *rest_degree, sollya_mpfi_t *p, int p_degree, sollya_mpfi_t *q, int q_degree, mp_prec_t prec) {
  
  int i,step,k;
  sollya_mpfi_t aux;
  int okay;

  okay = 1;
 
  *quotient_degree=p_degree-q_degree;
  
  if (sollya_mpfi_is_zero(q[q_degree]) || sollya_mpfi_is_zero(p[p_degree]))
    return 0;
 
  sollya_mpfi_init2(aux,prec);
  
  step=0;
  for (k=*quotient_degree; (k>=0) && okay;k--)
    {

      sollya_mpfi_set(quotient[k],p[p_degree-step]);
      if (!sollya_mpfi_has_zero(q[q_degree])) 
	sollya_mpfi_div(quotient[k],quotient[k],q[q_degree]);
      else {
	okay = 0;
      }
      if (okay) {
	for (i=q_degree; i>=0;i--){    
	  sollya_mpfi_mul(aux,quotient[k],q[i]);
	  sollya_mpfi_sub(p[p_degree-step-(q_degree-i)],p[p_degree-step-(q_degree-i)],aux);  
	}
	step++; 
      }
    }
  *rest_degree=0;
  for (i=q_degree-1; i>=0; i--)  
    {
      if (!sollya_mpfi_is_zero(p[i])){
	*rest_degree=i;
	break;
      }
    }
  for (i=*rest_degree; i>=0; i--){ 
    sollya_mpfi_set(rest[i],p[i]);
  }
  sollya_mpfi_clear(aux);
  return okay;
}  

int sturm_mpfi(int *n, mpq_t *pMpq, int p_degree, sollya_mpfi_t x, mp_prec_t precision){
  sollya_mpfi_t *quotient, *rest, *dp;
  int quotient_degree, rest_degree, dp_degree;
  sollya_mpfi_t *evalResA;
  sollya_mpfi_t *evalResB;
  sollya_mpfi_t *s0, *s1;
  sollya_mpfi_t evalRes;
  int s0_degree, s1_degree;
  mpfr_t a,b;
  sollya_mpfi_t aq,bq;
  int i,na,nb,nrRoots;
  int varSignB,varSignA;
  sollya_mpfi_t *p;
  int resultat;
  int resDiv;
  mp_prec_t prec, tempprec, prec2;


  resultat = 1;

  prec = precision;
  for (i=0;i<=p_degree;i++) {
    tempprec = getMpzPrecision(mpq_numref(pMpq[i])) + 10;
    if (tempprec > prec) prec = tempprec;
    tempprec = getMpzPrecision(mpq_denref(pMpq[i])) + 10;
    if (tempprec > prec) prec = tempprec;
  }
  prec = 2 * prec;

  printMessage(2,"Information: in sturm_mpfi: chosen working precision is %d\n",(int) prec);

  p = (sollya_mpfi_t *) safeCalloc(p_degree+1,sizeof(sollya_mpfi_t));
  for (i=0;i<=p_degree;i++) {
    sollya_mpfi_init2(p[i],prec);
    sollya_mpfi_set_q(p[i],pMpq[i]);
  }

  prec2=sollya_mpfi_get_prec(x);
  if (prec > prec2) prec2 = prec;
  na=0; nb=0;  
  nrRoots=0;
  mpfr_init2(a,prec2);
  mpfr_init2(b,prec2);
  sollya_mpfi_get_left(a,x);
  sollya_mpfi_get_right(b,x);
 
  sollya_mpfi_init2(aq,prec2);
  sollya_mpfi_init2(bq,prec2);
    
  sollya_mpfi_set_fr(aq,a);
  sollya_mpfi_set_fr(bq,b);
  
  sollya_mpfi_init2(evalRes,prec);

  evalResA = (sollya_mpfi_t *)safeMalloc((p_degree+1)*sizeof(sollya_mpfi_t));  
  evalResB = (sollya_mpfi_t *)safeMalloc((p_degree+1)*sizeof(sollya_mpfi_t));    
  s0=(sollya_mpfi_t *)safeMalloc((p_degree+1)*sizeof(sollya_mpfi_t));
  quotient=(sollya_mpfi_t *)safeMalloc((p_degree+1)*sizeof(sollya_mpfi_t));
  rest=(sollya_mpfi_t *)safeMalloc((p_degree+1)*sizeof(sollya_mpfi_t));
  for (i=0; i<=p_degree; i++){ 
    sollya_mpfi_init2(evalResA[i],prec);
    sollya_mpfi_init2(evalResB[i],prec);
    sollya_mpfi_init2(quotient[i],prec);
    sollya_mpfi_init2(rest[i],prec);
  }

  s0_degree=p_degree;

  for (i=0; i<=p_degree; i++){ 
    sollya_mpfi_init2(s0[i],prec); 
    sollya_mpfi_set(s0[i],p[i]);
  }
        
  polynomialDeriv_mpfi(&dp, &dp_degree, p, p_degree, prec);
    
  polynomialEval_mpfi( &evalRes, aq, s0, s0_degree);
  if (!sollya_mpfi_has_zero(evalRes)){
    sollya_mpfi_set(evalResA[na],evalRes);
    na++;
  }
  else {
    if (sollya_mpfi_is_zero(evalRes)) 
    nrRoots++; /*if the left extremity is a root we count it here
                    since the number of sign changes gives the nr of distinct 
                    roots between a and b, a<b*/  
    else resultat = 0;
  }

  polynomialEval_mpfi( &evalRes, bq, s0, s0_degree);
  if (!sollya_mpfi_has_zero(evalRes)){
    sollya_mpfi_set(evalResB[nb],evalRes);
    nb++;
  } else {
    if (!sollya_mpfi_is_zero(evalRes)) resultat = 0;
  }

  s1=dp;
  s1_degree=dp_degree;
  if (s0_degree>0){

    polynomialEval_mpfi( &evalRes, aq, dp, dp_degree);
    if (!sollya_mpfi_has_zero(evalRes)){
      sollya_mpfi_set(evalResA[na],evalRes);
      na++;
    } else {
      if (!sollya_mpfi_is_zero(evalRes)) resultat = 0;
    }
  
    polynomialEval_mpfi( &evalRes, bq, dp, dp_degree);
    if (!sollya_mpfi_has_zero(evalRes)){
      sollya_mpfi_set(evalResB[nb],evalRes);
      nb++;
    } else {
      if (!sollya_mpfi_is_zero(evalRes)) resultat = 0;
    }

  }
  
  while ((s1_degree!=0) && resultat) {
          
    resDiv = polynomialDivide_mpfi(quotient, &quotient_degree, rest, &rest_degree, s0, s0_degree, s1, s1_degree, prec);

    if (!resDiv) 
      resultat = 0; 
    else {

      s0_degree=s1_degree; 
      for(i=0;i<=s0_degree;i++) {
	sollya_mpfi_set(s0[i],s1[i]);
      }

      s1_degree=rest_degree; 
      
      for(i=0;i<=s1_degree;i++){
	sollya_mpfi_set(s1[i],rest[i]);
      }
      
      for (i=0; i<=s1_degree; i++)
	sollya_mpfi_neg(s1[i],s1[i]);
      
      polynomialEval_mpfi( &evalRes, aq, s1, s1_degree);
      if (!sollya_mpfi_has_zero(evalRes)){
	sollya_mpfi_set(evalResA[na],evalRes);
	na++;
      } else {
	if (!sollya_mpfi_is_zero(evalRes)) resultat = 0;
      }
      
      polynomialEval_mpfi( &evalRes, bq, s1, s1_degree);
      if (!sollya_mpfi_has_zero(evalRes)){
	sollya_mpfi_set(evalResB[nb],evalRes);
	nb++;
      } else {
	if (!sollya_mpfi_is_zero(evalRes)) resultat = 0;
      }
    }
  }
  
  varSignA=0;
  for (i=1; i<na; i++){
    if ((!(!sollya_mpfi_is_nonneg(evalResA[i-1]))) ^ (!(!sollya_mpfi_is_nonneg(evalResA[i])))) varSignA++;    
  } 
  varSignB=0;
  for (i=1; i<nb; i++){
    if ((!(!sollya_mpfi_is_nonneg(evalResB[i-1]))) ^ (!(!sollya_mpfi_is_nonneg(evalResB[i])))) varSignB++;    
  }

  *n=(((varSignA-varSignB)>0)?(varSignA-varSignB+nrRoots):(varSignB-varSignA+nrRoots) );

  for (i=0; i<=p_degree; i++){ 
    sollya_mpfi_clear(evalResA[i]);
    sollya_mpfi_clear(evalResB[i]);
    sollya_mpfi_clear(s0[i]);
    sollya_mpfi_clear(quotient[i]);
    sollya_mpfi_clear(rest[i]);
  }
  for (i=0; i<=dp_degree; i++){ 
    sollya_mpfi_clear(dp[i]);
  }
  free(dp);
  free(evalResA);
  free(evalResB);
  free(s0); 
  free(quotient); 
  free(rest); 
  sollya_mpfi_clear(evalRes);
  mpfr_clear(a);
  mpfr_clear(b);
  sollya_mpfi_clear(aq);
  sollya_mpfi_clear(bq);

  for (i=0;i<=p_degree;i++) {
    sollya_mpfi_clear(p[i]);
  }
  free(p);
  
  return resultat;
}


int getNrRoots(mpfr_t res, node *f, sollya_mpfi_t range, mp_prec_t precision) {
  sollya_mpfi_t x;
  int degree,i,nr;
  node **coefficients;
  mpq_t  *qCoefficients;
  int r;
  mpfr_t tempValue, tempValue2;
  node *tempTree;
  int deg;
  int resMpfi;
  mp_prec_t prec;
  
  if (!isPolynomial(f)) {
      printMessage(1,"Warning: the given function must be a polynomial in this context.\n");
      return 0;
  }

  if (!sollya_mpfi_bounded_p(range)) {
      printMessage(1,"Warning: the given interval must have finite bounds.\n");
      return 0;
  }

  prec=precision;
  
  sollya_mpfi_init2(x, sollya_mpfi_get_prec(range));
  sollya_mpfi_set(x, range);

  getCoefficients(&degree,&coefficients,f);

  if (degree < 0) {
    printMessage(1,"Warning: the given function is not a polynomial.\n");
    sollya_mpfi_clear(x);
    return 0;
  }
  
  qCoefficients = (mpq_t *) safeCalloc(degree+1,sizeof(mpq_t));
  for (i=0;i<=degree;i++) {
    mpq_init(qCoefficients[i]);
  }  

  mpfr_init2(tempValue,prec);
  mpfr_set_d(tempValue,1.0,GMP_RNDN);
  mpfr_init2(tempValue2,prec);
  for (i=0;i<=degree;i++) {
    if (coefficients[i] != NULL) {
      tempTree = simplifyTreeErrorfree(coefficients[i]);
      free_memory(coefficients[i]);
      if (!isConstant(tempTree)) {
	sollyaFprintf(stderr,"Error: getNrRoots: an error occurred. A polynomial coefficient is not constant.\n");
	exit(1);
      }
      if (tempTree->nodeType != CONSTANT) {
	if (tryEvaluateConstantTermToMpq(qCoefficients[i], tempTree)) {
	  if (verbosity >= 3) {
	    changeToWarningMode();
	    sollyaPrintf("Information: in getNrRoots: evaluated the %dth coefficient to ",i);
	    printMpq(qCoefficients[i]);
	    sollyaPrintf("\n");
	    restoreMode();
	  }
	} else {
	  if (!noRoundingWarnings) {
	    printMessage(1,"Warning: the %dth coefficient of the polynomial is neither a floating point\n",i);
	    printMessage(1,"constant nor can be evaluated without rounding to a floating point constant.\n");
	    printMessage(1,"Will faithfully evaluate it with the current precision (%d bits) \n",prec);
	  }
	  r=evaluateFaithful(tempValue2, tempTree, tempValue, prec);
	  if (!r){
	    mpfr_set_ui(tempValue2,0,GMP_RNDN);
	    if (!noRoundingWarnings) {
	      printMessage(1,"Warning: Rounded the coefficient %d to 0.\n",i);
	    }
	  }
	  mpfr_to_mpq(qCoefficients[i], tempValue2);
	  if (verbosity >= 3) {
	    changeToWarningMode();
	    sollyaPrintf("Information: evaluated the %dth coefficient to ",i);
	    printMpq(qCoefficients[i]);
	    sollyaPrintf("\n");
	    restoreMode();
	  }
	}
      } 
      else {
	mpfr_to_mpq(qCoefficients[i], *(tempTree->value));
      }
      free_memory(tempTree);
    } else {
      mpq_set_ui(qCoefficients[i],0,1);
    }
  }
  free(coefficients);
  mpfr_clear(tempValue); 
  mpfr_clear(tempValue2);

  for(deg = degree; deg >= 0 && (mpq_sgn(qCoefficients[deg]) == 0); deg--); 

  if (deg >= 0) {
    resMpfi = sturm_mpfi(&nr, qCoefficients, deg,x,precision);
    if (!resMpfi) {
      printMessage(1,"Warning: using slower GMP MPQ version\n");
      sturm_mpq(&nr, qCoefficients, deg,x);
    }
    mpfr_set_si(res,nr,GMP_RNDN);
  } else {
    printMessage(1,"Warning: the given polynomial is the zero polynomial. Its number of zeros is infinite.\n");
    mpfr_set_inf(res,1);
  }

  sollya_mpfi_clear(x);
  for (i=0;i<=degree;i++) {
    mpq_clear(qCoefficients[i]);
  }
  free(qCoefficients);
  
  return 1;
}

