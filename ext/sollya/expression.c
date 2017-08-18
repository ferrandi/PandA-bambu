/*

Copyright 2006-2011 by

Laboratoire de l'Informatique du Parallelisme,
UMR CNRS - ENS Lyon - UCB Lyon 1 - INRIA 5668,

LORIA (CNRS, INPL, INRIA, UHP, U-Nancy 2)

and by

Laboratoire d'Informatique de Paris 6, equipe PEQUAN,
UPMC Universite Paris 06 - CNRS - UMR 7606 - LIP6, Paris, France.

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
#include <gmp.h>
#include "expression.h"
#include <stdio.h> /* fprinft, fopen, fclose, */
#include <stdlib.h> /* exit, free, mktemp */
#include <string.h>
#include <errno.h>
#include "general.h"
#include "double.h"
#include "chain.h"
#include "execute.h"
#include "infnorm.h"

#define MAXDIFFSIMPLSIZE 100
#define MAXDIFFSIMPLDEGREE 25

void simplifyMpfrPrec(mpfr_t rop, mpfr_t op) {
  mpz_t mant;
  mp_exp_t expo;
  mp_prec_t prec;
  mpfr_t x;
  unsigned int dyadicValue;
  int p;

  if (mpfr_number_p(op) && (!mpfr_zero_p(op))) {
    mpz_init(mant);
    expo = mpfr_get_z_exp(mant,op);
    prec = mpz_sizeinbase(mant, 2);
    dyadicValue = mpz_scan1(mant, 0);
    p = prec - dyadicValue;
    if (p < 12) prec = 12; else prec = p; 
    mpfr_init2(x,prec);
    mpfr_set_z(x,mant,GMP_RNDN);
    mpfr_mul_2si(x,x,expo,GMP_RNDN);
    if (mpfr_cmp(x,op) == 0) {
      mpfr_set_prec(rop,prec);
      mpfr_set(rop,x,GMP_RNDN);
    } else {
      prec = mpfr_get_prec(op);
      mpfr_set_prec(x,prec);
      mpfr_set(x,op,GMP_RNDN);
      mpfr_set_prec(rop,prec);
      mpfr_set(rop,x,GMP_RNDN);
    }
    mpfr_clear(x);
    mpz_clear(mant);
  } else {
    if (mpfr_zero_p(op)) {
      mpfr_set_prec(rop,12);
      mpfr_set(rop,op,GMP_RNDN);
    } else {
      prec = mpfr_get_prec(op);
      mpfr_init2(x,prec);
      mpfr_set(x,op,GMP_RNDN);
      mpfr_set_prec(rop,prec);
      mpfr_set(rop,x,GMP_RNDN);
      mpfr_clear(x);
    } 
  }
}



void mpfr_from_mpfi(mpfr_t rop, mpfr_t op, int n, int (*mpfifun)(sollya_mpfi_t, sollya_mpfi_t, int)) {
  sollya_mpfi_t opI, ropI;

  sollya_mpfi_init2(opI,mpfr_get_prec(op));
  sollya_mpfi_init2(ropI,mpfr_get_prec(rop)+2);
  sollya_mpfi_set_fr(opI,op);

  mpfifun(ropI,opI,n);
  
  sollya_mpfi_mid(rop,ropI);

  sollya_mpfi_clear(opI);
  sollya_mpfi_clear(ropI);
}

void free_memory(node *tree) {
  if (tree == NULL) return;
  switch (tree->nodeType) {
  case VARIABLE:
    free(tree);
    break;
  case CONSTANT:
    mpfr_clear(*(tree->value));
    free(tree->value);
    free(tree);
    break;
  case ADD:
    free_memory(tree->child1);
    free_memory(tree->child2);
    free(tree);
    break;
  case SUB:
    free_memory(tree->child1);
    free_memory(tree->child2);
    free(tree);
    break;
  case MUL:
    free_memory(tree->child1);
    free_memory(tree->child2);
    free(tree);
    break;
  case DIV:
    free_memory(tree->child1);
    free_memory(tree->child2);
    free(tree);
    break;
  case SQRT:
    free_memory(tree->child1);
    free(tree);
    break;
  case EXP:
    free_memory(tree->child1);
    free(tree);
    break;
  case LOG:
    free_memory(tree->child1);
    free(tree);
    break;
  case LOG_2:
    free_memory(tree->child1);
    free(tree);
    break;
  case LOG_10:
    free_memory(tree->child1);
    free(tree);
    break;
  case SIN:
    free_memory(tree->child1);
    free(tree);
    break;
  case COS:
    free_memory(tree->child1);
    free(tree);
    break;
  case TAN:
    free_memory(tree->child1);
    free(tree);
    break;
  case ASIN:
    free_memory(tree->child1);
    free(tree);
    break;
  case ACOS:
    free_memory(tree->child1);
    free(tree);
    break;
  case ATAN:
    free_memory(tree->child1);
    free(tree);
    break;
  case SINH:
    free_memory(tree->child1);
    free(tree);
    break;
  case COSH:
    free_memory(tree->child1);
    free(tree);
    break;
  case TANH:
    free_memory(tree->child1);
    free(tree);
    break;
  case ASINH:
    free_memory(tree->child1);
    free(tree);
    break;
  case ACOSH:
    free_memory(tree->child1);
    free(tree);
    break;
  case ATANH:
    free_memory(tree->child1);
    free(tree);
    break;
  case POW:
    free_memory(tree->child1);
    free_memory(tree->child2);
    free(tree);
    break;
  case NEG:
    free_memory(tree->child1);
    free(tree);
    break;
  case ABS:
    free_memory(tree->child1);
    free(tree);
    break;
  case DOUBLE:
    free_memory(tree->child1);
    free(tree);
    break;
  case SINGLE:
    free_memory(tree->child1);
    free(tree);
    break;
  case QUAD:
    free_memory(tree->child1);
    free(tree);
    break;  
  case HALFPRECISION:
    free_memory(tree->child1);
    free(tree);
    break;
  case DOUBLEDOUBLE:
    free_memory(tree->child1);
    free(tree);
    break;
  case TRIPLEDOUBLE:
    free_memory(tree->child1);
    free(tree);
    break;
  case ERF: 
    free_memory(tree->child1);
    free(tree);
    break;
  case ERFC:
    free_memory(tree->child1);
    free(tree);
    break;
  case LOG_1P:
    free_memory(tree->child1);
    free(tree);
    break;
  case EXP_M1:
    free_memory(tree->child1);
    free(tree);
    break;
  case DOUBLEEXTENDED:
    free_memory(tree->child1);
    free(tree);
    break;
  case LIBRARYFUNCTION:
    free_memory(tree->child1);
    free(tree);
    break;
  case PROCEDUREFUNCTION:
    free_memory(tree->child1);
    freeThing(tree->child2);
    free(tree);
    break;
  case CEIL:
    free_memory(tree->child1);
    free(tree);
    break;
  case FLOOR:
    free_memory(tree->child1);
    free(tree);
    break;
  case NEARESTINT:
    free_memory(tree->child1);
    free(tree);
    break;
  case PI_CONST:
    free(tree);
    break;
  case LIBRARYCONSTANT:
    free(tree);
    break;
  default:
   sollyaFprintf(stderr,"Error: free_memory: unknown identifier (%d) in the tree\n",tree->nodeType);
   exit(1);
  }
  return;
}




void fprintHeadFunction(FILE *fd,node *tree, char *x, char *y) {
  int i;

  if (tree == NULL) return;
  switch (tree->nodeType) {
  case VARIABLE:
    if (x != NULL) sollyaFprintf(fd,"%s",x); else sollyaFprintf(fd,"x");
    break;
  case CONSTANT:
    fprintValue(fd,*(tree->value));
    break;
  case ADD:
    sollyaFprintf(fd,"%s + %s",x,y);
    break;
  case SUB:
    sollyaFprintf(fd,"%s - %s",x,y);
    break;
  case MUL:
    sollyaFprintf(fd,"%s * %s",x,y);
    break;
  case DIV:
    sollyaFprintf(fd,"%s / %s",x,y);
    break;
  case SQRT:
    sollyaFprintf(fd,"sqrt(%s)",x);
    break;
  case EXP:
    sollyaFprintf(fd,"exp(%s)",x);
    break;
  case LOG:
    sollyaFprintf(fd,"log(%s)",x);
    break;
  case LOG_2:
    sollyaFprintf(fd,"log2(%s)",x);
    break;
  case LOG_10:
    sollyaFprintf(fd,"log10(%s)",x);
    break;
  case SIN:
    sollyaFprintf(fd,"sin(%s)",x);
    break;
  case COS:
    sollyaFprintf(fd,"cos(%s)",x);
    break;
  case TAN:
    sollyaFprintf(fd,"tan(%s)",x);
    break;
  case ASIN:
    sollyaFprintf(fd,"asin(%s)",x);
    break;
  case ACOS:
    sollyaFprintf(fd,"acos(%s)",x);
    break;
  case ATAN:
    sollyaFprintf(fd,"atan(%s)",x);
    break;
  case SINH:
    sollyaFprintf(fd,"sinh(%s)",x);
    break;
  case COSH:
    sollyaFprintf(fd,"cosh(%s)",x);
    break;
  case TANH:
    sollyaFprintf(fd,"tanh(%s)",x);
    break;
  case ASINH:
    sollyaFprintf(fd,"asinh(%s)",x);
    break;
  case ACOSH:
    sollyaFprintf(fd,"acosh(%s)",x);
    break;
  case ATANH:
    sollyaFprintf(fd,"atanh(%s)",x);
    break;
  case POW:
    sollyaFprintf(fd,"%s^%s",x,y);
    break;
  case NEG:
    sollyaFprintf(fd,"-%s",x);
    break;
  case ABS:
    sollyaFprintf(fd,"abs(%s)",x);
    break;
  case DOUBLE:
    sollyaFprintf(fd,"double(%s)",x);
    break;
  case SINGLE:
    sollyaFprintf(fd,"single(%s)",x);
    break;
  case QUAD:
    sollyaFprintf(fd,"quad(%s)",x);
    break;
  case HALFPRECISION:
    sollyaFprintf(fd,"halfprecision(%s)",x);
    break;
  case DOUBLEDOUBLE:
    sollyaFprintf(fd,"doubledouble(%s)",x);
    break;
  case TRIPLEDOUBLE:
    sollyaFprintf(fd,"tripledouble(%s)",x);
    break;
  case ERF: 
    sollyaFprintf(fd,"erf(%s)",x);
    break;
  case ERFC:
    sollyaFprintf(fd,"erfc(%s)",x);
    break;
  case LOG_1P:
    sollyaFprintf(fd,"log1p(%s)",x);
    break;
  case EXP_M1:
    sollyaFprintf(fd,"expm1(%s)",x);
    break;
  case DOUBLEEXTENDED:
    sollyaFprintf(fd,"doubleextended(%s)",x);
    break;
  case LIBRARYFUNCTION:
    {
      sollyaFprintf(fd,"(");
      for (i=1;i<=tree->libFunDeriv;i++) {
	sollyaFprintf(fd,"diff(");
      }
      sollyaFprintf(fd,"%s",tree->libFun->functionName);
      for (i=1;i<=tree->libFunDeriv;i++) {
	sollyaFprintf(fd,")");
      }
      sollyaFprintf(fd,")(%s)",x);
    }
    break;
  case PROCEDUREFUNCTION:
    {
      sollyaFprintf(fd,"(");
      for (i=1;i<=tree->libFunDeriv;i++) {
	sollyaFprintf(fd,"diff(");
      }
      sollyaFprintf(fd,"function(");
      fPrintThing(fd,tree->child2);
      sollyaFprintf(fd,")");
      for (i=1;i<=tree->libFunDeriv;i++) {
	sollyaFprintf(fd,")");
      }
      sollyaFprintf(fd,")(%s)",x);
    }
    break;
  case CEIL:
    sollyaFprintf(fd,"ceil(%s)",x);
    break;
  case FLOOR:
    sollyaFprintf(fd,"floor(%s)",x);
    break;
  case NEARESTINT:
    sollyaFprintf(fd,"nearestint(%s)",x);
    break;
  case PI_CONST:
    sollyaFprintf(fd,"pi");
    break;
  case LIBRARYCONSTANT:
    sollyaFprintf(fd,"%s",tree->libFun->functionName);
    break;
  default:
   sollyaFprintf(stderr,"fprintHeadFunction: unknown identifier (%d) in the tree\n",tree->nodeType);
   exit(1);
  }
  return;
}

int precedence(node *tree) {
  switch (tree->nodeType) {
  case CONSTANT:
  case VARIABLE:
  case PI_CONST:
    return 1;
    break;
  case ADD:
  case SUB:
    return 2;
    break;
  case MUL:
    return 3;
    break;
  case DIV:
    return 4;
    break;
  case NEG:
    return 5;
    break;
  case POW:
    return 6;
    break;
  default:
    return 7; 
  }
  return 0;
}


int isInfix(node *tree) {
  char *str;
  int res;
  switch(tree->nodeType) {
  case CONSTANT: 
    if (mpfr_sgn(*(tree->value)) < 0) return 1;
    if ((dyadic == 2) || (dyadic == 3)) {
      str = sprintValue(tree->value);
      res = (strchr(str,'*') != NULL);
      free(str);
      return res;
    }
    break;
  case PI_CONST:
  case ADD:
  case SUB:
  case MUL:
  case DIV:
  case POW:
  case NEG:
    return 1;
    break;
  default: return 0;
  }
  return 0;
}

void removeTrailingZeros(char *outbuf, char *inbuf) {
  char *temp, *temp2, *temp3;

  temp = inbuf; temp2 = outbuf; temp3 = outbuf;
  while ((temp != NULL) && (*temp != '\0')) {
    *temp2 = *temp;
    if (*temp2 != '0') {
      temp3 = temp2;
    }
    temp2++;
    temp++;
  }
  temp3++;
  *temp3 = '\0';
}

void printHexadecimalValue(mpfr_t x);

void printValue(mpfr_t *value) {
  char *str;

  str = sprintValue(value);
  sollyaPrintf("%s",str);
  free(str);
}

char *sprintMidpointMode(mpfr_t a, mpfr_t b) {
  mp_exp_t e1, e2;
  char *str1, *str2, *str, *str3;
  mpfr_t aP, bP;
  int sign, len1, len2, len, i;
  mp_prec_t prec, p;
  char *s1, *s2;
  int possibleLength;

  if (mpfr_sgn(a) != mpfr_sgn(b)) return NULL;

  if (mpfr_zero_p(a)) {
    str = safeCalloc(7,sizeof(char));
    sprintf(str,"[0]");
    return str;
  }

  prec = mpfr_get_prec(a);
  p = mpfr_get_prec(b);

  if (p > prec) prec = p;

  mpfr_init2(aP,prec);
  mpfr_init2(bP,prec);

  sign = mpfr_sgn(a);
  if (sign > 0) {
    mpfr_set(aP,a,GMP_RNDN);
    mpfr_set(bP,b,GMP_RNDN);
  } else {
    mpfr_neg(aP,b,GMP_RNDN);
    mpfr_neg(bP,a,GMP_RNDN);
  }

  str1 = mpfr_get_str(NULL,&e1,10,0,aP,GMP_RNDD);  
  str2 = mpfr_get_str(NULL,&e2,10,0,bP,GMP_RNDU);  

  str3 = safeCalloc(strlen(str1) + 1, sizeof(char));
  removeTrailingZeros(str3,str1);
  free(str1);
  str1 = str3;

  str3 = safeCalloc(strlen(str2) + 1, sizeof(char));
  removeTrailingZeros(str3,str2);
  free(str2);
  str2 = str3;

  if (e1 == e2) {
    if (strcmp(str1,str2) == 0) {
      mpfr_set(aP,a,GMP_RNDN);
      str3 = sprintValue(&aP);
      str = (char *) safeCalloc(strlen(str3) + 3, sizeof(char));
      sprintf(str,"[%s]",str3);
      free(str3);
    } else { 

      if (str1[0] == str2[0]) {
	len1 = strlen(str1);
	len2 = strlen(str2);
	len = len1;
	if (len2 < len) len = len2;
	i = 0;
	while ((i < len) && (str1[i] == str2[i])) {
	  i++;
	}
	possibleLength = i;
	s1 = mpfr_get_str(NULL,&e1,10,possibleLength+1,aP,GMP_RNDD);  
	s2 = mpfr_get_str(NULL,&e2,10,possibleLength+1,bP,GMP_RNDU); 
	
	if (e1 == e2) {
	  if (s1[0] == s2[0]) {
	    len1 = strlen(s1);
	    len2 = strlen(s2);
	    len = len1;
	    if (len2 < len) len = len2;
	    str = (char *) safeCalloc(len+6,sizeof(char));
	    i = 0;
	    while ((i < len) && (s1[i] == s2[i])) {
	      str[i] = s1[i];
	      i++;
	    }
	    str[i] = '~';
	    if (sign > 0) 
	      str[i+1] = s1[i];
	    else
	      str[i+1] = s2[i];
	    str[i+2] = '/';
	    if (sign > 0) 
	      str[i+3] = s2[i];
	    else 
	      str[i+3] = s1[i];
	    str[i+4] = '~';
	    str3 = (char *) safeCalloc(strlen(str)+1,sizeof(char));
	    removeTrailingZeros(str3,str);
	    free(str);
	    str = str3;
	    str3 = (char *) safeCalloc(strlen(str)+69,sizeof(char));
	    if (sign < 0) {
	      if (e1 == 0) {
		sprintf(str3,"-0.%s",str);
	      } else {
		sprintf(str3,"-0.%se%d",str,(int)e1);
	      }
	    } else {
	      if (e1 == 0) {
		sprintf(str3,"0.%s",str);
	      } else {
		sprintf(str3,"0.%se%d",str,(int)e1);
	      }
	    }
	    free(str);
	    str = str3;
	    str3 = (char *) safeCalloc(strlen(str)+1,sizeof(char));
	    sprintf(str3,"%s",str);
	    free(str);
	    str = str3;
	  } else {
	    str = NULL;
	  }
	} else {
	  str = NULL;
	}
	free(s1);
	free(s2);
      } else {
	str = NULL;
      }
    }
  } else {
    str = NULL;
  }

  mpfr_free_str(str1);      
  mpfr_free_str(str2);      

  mpfr_clear(aP);
  mpfr_clear(bP);
  return str;
}

char *sPrintBinary(mpfr_t x) {
  mpfr_t xx;
  int negative;
  mp_prec_t prec;
  mp_exp_t expo;
  char *raw, *formatted, *temp1, *temp2, *str3;
  char *temp3=NULL;
  char *resultStr;
  int len;

  prec = mpfr_get_prec(x);
  mpfr_init2(xx,prec);
  mpfr_abs(xx,x,GMP_RNDN);
  negative = 0;
  if (mpfr_sgn(x) < 0) negative = 1;
  raw = mpfr_get_str(NULL,&expo,2,0,xx,GMP_RNDN);
  if (raw == NULL) {
    sollyaPrintf("Error: unable to get a string for the given number.\n");
    recoverFromError();
  } else {
    formatted = safeCalloc(strlen(raw) + 3, sizeof(char));
    temp1 = raw; temp2 = formatted;
    if (negative) {
      *temp2 = '-';
      temp2++;
    }
    *temp2 = *temp1; temp2++; temp1++;
    if (*temp1 != '\0') { 
      *temp2 = '.'; 
      temp2++;
    }
    while (*temp1 != '\0') {
      *temp2 = *temp1;
      temp2++; temp1++;
    }
    str3 = (char *) safeCalloc(strlen(formatted)+2,sizeof(char));
    removeTrailingZeros(str3,formatted);
    len = strlen(str3) - 1;
    if (str3[len] == '.') {
      str3[len] = '\0';
    }
    if (!mpfr_zero_p(x)) {
      if (mpfr_number_p(x)) {
	temp3 = (char *) safeCalloc(strlen(str3)+74,sizeof(char));
	if ((((int) expo)-1) != 0) 
	  sprintf(temp3,"%s_2 * 2^(%d)",str3,((int)expo)-1);  
	else
	  sprintf(temp3,"%s_2",str3);  
      } else {
	temp3 = (char *) safeCalloc(strlen(raw) + 2,sizeof(char));
	if (negative) 
	  sprintf(temp3,"-%s",raw); 
	else 
	  sprintf(temp3,"%s",raw); 
      }
    }
    else {
      temp3 = (char *) safeCalloc(2,sizeof(char));
      sprintf(temp3,"0");
    }
    free(formatted);
    free(str3);
  }
  mpfr_free_str(raw);  
  mpfr_clear(xx);
  resultStr = (char *) safeCalloc(strlen(temp3) + 1,sizeof(char));
  sprintf(resultStr,"%s",temp3);
  free(temp3);
  return resultStr;
}

char *sPrintHexadecimal(mpfr_t x) {
  mpfr_t xx;
  int negative;
  mp_prec_t prec;
  mp_exp_t expo;
  char *raw, *formatted, *temp1, *temp2, *str3;
  char *temp3 = NULL;
  char *resultStr;

  prec = mpfr_get_prec(x);
  mpfr_init2(xx,prec);
  mpfr_abs(xx,x,GMP_RNDN);
  negative = 0;
  if (mpfr_sgn(x) < 0) negative = 1;
  raw = mpfr_get_str(NULL,&expo,16,0,xx,GMP_RNDN);
  if (raw == NULL) {
    sollyaPrintf("Error: unable to get a string for the given number.\n");
    recoverFromError();
  } else {
    formatted = safeCalloc(strlen(raw) + 3, sizeof(char));
    temp1 = raw; temp2 = formatted;
    *temp2 = *temp1; temp2++; temp1++;
    *temp2 = '.'; temp2++;
    while (*temp1 != '\0') {
      *temp2 = *temp1;
      temp2++; temp1++;
    }
    str3 = (char *) safeCalloc(strlen(formatted)+2,sizeof(char));
    removeTrailingZeros(str3,formatted);    
    if (!mpfr_zero_p(x)) {
      if (mpfr_number_p(x)) {
	temp3 = (char *) safeCalloc(strlen(str3)+74,sizeof(char));
	if (negative) {
	  sprintf(temp3,"-0x%sp%d",str3,4 * (((int)expo)-1)); 
	} else {
	  sprintf(temp3,"0x%sp%d",str3,4 * (((int)expo)-1)); 
	}
      } else {
	temp3 = (char *) safeCalloc(strlen(raw) + 2,sizeof(char));
	if (negative) 
	  sprintf(temp3,"-%s",raw); 
	else 
	  sprintf(temp3,"%s",raw); 
      }
    }
    else {
      temp3 = (char *) safeCalloc(2,sizeof(char));
      sprintf(temp3,"0");
    }
    free(formatted);
    free(str3);
  }
  mpfr_free_str(raw);  
  mpfr_clear(xx);
  resultStr = (char *) safeCalloc(strlen(temp3) + 1,sizeof(char));
  sprintf(resultStr,"%s",temp3);
  free(temp3);
  return resultStr;
}


void printBinary(mpfr_t x) {
  char *str;

  str = sPrintBinary(x);
  sollyaPrintf("%s",str);
  free(str);
}

void printHexadecimalValue(mpfr_t x) {
  char *str;

  str = sPrintHexadecimal(x);
  sollyaPrintf("%s",str);
  free(str);
}



char *sprintValue(mpfr_t *aValue) {
  mpfr_t y;
  char *str, *str2, *str3;
  mp_exp_t e, expo;
  int l, i, len;
  char *buffer, *tempBuf, *finalBuffer;
  char *tempBufOld;
  char *str4;
  mpfr_t temp;
  mp_prec_t prec2, prec, p, pp, prec3;
  mpfr_t *value, myValue;
  char *res;
  mpfr_t two128;
  
  p = mpfr_get_prec(*aValue);  
  pp = p;
  mpfr_init2(myValue,p);
  simplifyMpfrPrec(myValue, *aValue);
  if ((p > tools_precision) && (mpfr_get_prec(myValue) < tools_precision)) {
    if (tools_precision < p) p = tools_precision;
    mpfr_set_prec(myValue,p);
    mpfr_set(myValue,*aValue,GMP_RNDN);
  }
  value = &myValue;
  
  if (dyadic == 4) {
    res = sPrintHexadecimal(*value);
    mpfr_clear(myValue);
    return res;
  }

  if (dyadic == 3) {
    res = sPrintBinary(*value);
    mpfr_clear(myValue);
    return res;
  } 

  mpfr_init2(two128,12);
  mpfr_set_ui(two128,2,GMP_RNDN);
  mpfr_mul_2ui(two128,two128,128,GMP_RNDN);

  prec = mpfr_get_prec(*value);

  if (mpfr_number_p(*value)) {
    prec2 = prec;
    while (prec2 >= tools_precision) {
      mpfr_init2(temp,prec2);
      mpfr_set(temp,*value,GMP_RNDN);
      if (mpfr_cmp(temp,*value) != 0) {
	mpfr_clear(temp);
	prec2++;
	break;
      }
      mpfr_clear(temp);
      prec2--;
    }
    if (prec2 > prec) prec2 = prec;
    if (prec2 < tools_precision) prec2 = tools_precision;
    prec = prec2;
  }
  prec3 = prec;
  if (mpfr_get_prec(*value) > prec3) prec3 = mpfr_get_prec(*value);
  buffer = safeCalloc(3 * prec3 + 7 + (sizeof(mp_exp_t) * 4) + 1, sizeof(char));
  tempBuf = buffer;
  mpfr_init2(y,prec);

  if (mpfr_zero_p(*value) || 
      (mpfr_number_p(*value) && 
       mpfr_integer_p(*value) &&
       (mpfr_cmpabs(*value,two128) < 0))) {
    if (mpfr_zero_p(*value)) {
      tempBuf += sprintf(tempBuf,"0");
    } else {
      mpfr_set(y,*value,GMP_RNDN);
      if (mpfr_sgn(y) < 0) {
        tempBuf += sprintf(tempBuf,"-"); mpfr_neg(y,y,GMP_RNDN);
      }
      str = mpfr_get_str(NULL,&e,10,0,y,GMP_RNDN);
      len = strlen(str);
      if (len >= e) {
        for (i=0;i<e;i++) {
          *tempBuf = str[i];
          tempBuf++;
        }
      } else {
        for (i=0;i<len;i++) {
          *tempBuf = str[i];
          tempBuf++;
        }
        for (i=0;i<e-len;i++) {
          *tempBuf = '0';
          tempBuf++;
        }
      }
      mpfr_free_str(str);
    }
  } else { 
    mpfr_set(y,*value,GMP_RNDN);
    if (mpfr_sgn(y) < 0) {
      tempBuf += sprintf(tempBuf,"-"); mpfr_neg(y,y,GMP_RNDN);
    }
    if ((dyadic == 1) || (dyadic == 2)) {
      if (!mpfr_number_p(*value)) {
	str = mpfr_get_str(NULL,&e,10,0,y,GMP_RNDN);
	tempBuf += sprintf(tempBuf,"%s",str);
      } else {
	expo = mpfr_get_exp(y);
	if (mpfr_set_exp(y,prec)) {
	  printMessage(1,"\nWarning: %d is not in the current exponent range of a variable. Values displayed may be wrong.\n",(int)(prec));
	}
	expo -= prec;
	while (mpfr_integer_p(y)) {
	  mpfr_div_2ui(y,y,1,GMP_RNDN);
	  expo += 1;
	}
	expo--;
	if (mpfr_mul_2ui(y,y,1,GMP_RNDN) != 0) {
	  if (!noRoundingWarnings) {
	    printMessage(1,"\nWarning: rounding occurred during displaying a value. Values displayed may be wrong.\n");
	  }
	}
	str = mpfr_get_str(NULL,&e,10,0,y,GMP_RNDN);
	str2 = (char *) safeCalloc(strlen(str)+1,sizeof(char));
	strncpy(str2,str,e);
	if (dyadic == 1) {
	  if (expo != 0) {
	    tempBuf += sprintf(tempBuf,"%sb%d",str2,(int)expo);
	  } else {
	    tempBuf += sprintf(tempBuf,"%s",str2);
	  }
	} else {
	  if (expo != 0) {
	    tempBuf += sprintf(tempBuf,"%s * 2^(%d)",str2,(int)expo);
	  } else {
	    tempBuf += sprintf(tempBuf,"%s",str2);
	  }
	}
	free(str2);
      }
      mpfr_free_str(str);      
    } else {
      str = mpfr_get_str(NULL,&e,10,0,y,GMP_RNDN);
      if (mpfr_number_p(*value)) {
	str3 = (char *) safeCalloc(strlen(str)+1,sizeof(char));
	removeTrailingZeros(str3,str);
	if (e == 0) {
	  tempBufOld = tempBuf;
	  str4 = (char *) safeCalloc(strlen(str3)+1,sizeof(char));
	  mpfr_init2(temp,prec);
	  for (i=0;i<(int)strlen(str3);i++) {
	    str4[i] = str3[i];
	    tempBuf = tempBufOld;
	    tempBuf += sprintf(tempBuf,"0.%s",str4);   
	    mpfr_set_str(temp,buffer,10,GMP_RNDN);
	    if (mpfr_cmp(temp,*value) == 0) break;
	  }
	  free(str4);
	  mpfr_clear(temp);
	} else {
	  l = strlen(str3);
	  if ((e > 0) && (l <= e) && (e <= 16)) {
	    tempBuf += sprintf(tempBuf,"%s",str3);
	    for (i=l;i<e;i++) tempBuf += sprintf(tempBuf,"0");
	  } else {
	    tempBufOld = tempBuf;
	    str4 = (char *) safeCalloc(strlen(str3)+1,sizeof(char));
	    mpfr_init2(temp,prec);
	    for (i=0;i<(int)strlen(str3);i++) {
	      str4[i] = str3[i];
	      tempBuf = tempBufOld;
	      if (e-1 == 0) {
		if (strlen(str4) > 1) {
		  tempBuf += sprintf(tempBuf,"%c.%s",*str4,str4+1);   
		} else {
		  tempBuf += sprintf(tempBuf,"%c",*str4);
		}
	      } else {
		if (strlen(str4) > 1) {
		  tempBuf += sprintf(tempBuf,"%c.%se%d",*str4,str4+1,(int)e-1);   
		} else {
		  tempBuf += sprintf(tempBuf,"%ce%d",*str4,(int)e-1);   
		}
	      }
	      mpfr_set_str(temp,buffer,10,GMP_RNDN);
	      if (mpfr_cmp(temp,*value) == 0) break;
	    }
	    free(str4);
	    mpfr_clear(temp);
	  }
	}
	free(str3);
      } else {
	tempBuf += sprintf(tempBuf,"%s",str);
      }
      mpfr_free_str(str);
    }
  }
  mpfr_clear(y);

  finalBuffer = (char *) safeCalloc(strlen(buffer)+1,sizeof(char));
  sprintf(finalBuffer,"%s",buffer);
  free(buffer);
  mpfr_clear(myValue);
  mpfr_clear(two128);
  return finalBuffer;
}


void printMpfr(mpfr_t x) {
  mpfr_t tmp;
  mp_prec_t prec;

  prec = mpfr_get_prec(x);
  mpfr_init2(tmp,prec);
  mpfr_set(tmp,x,GMP_RNDN);

  printValue(&tmp);
  sollyaPrintf("\n");

  mpfr_clear(tmp);
}


void fprintValueWithPrintMode(FILE *fd, mpfr_t value) {
  char *str;
  mpfr_t temp;
  mp_prec_t p;

  p = mpfr_get_prec(value);
  mpfr_init2(temp,p);
  mpfr_set(temp,value,GMP_RNDN);
  str = sprintValue(&temp);
  mpfr_clear(temp);
  sollyaFprintf(fd,"%s",str);
  free(str);
  
}

void fprintTreeWithPrintMode(FILE *fd, node *tree) {
  int pred, i;

  if (fullParentheses) pred = 100; else pred = precedence(tree);

  switch (tree->nodeType) {
  case VARIABLE:
    if (variablename != NULL) {
      sollyaFprintf(fd,"%s",variablename);
    } else {
      printMessage(1,"Warning: the current free variable has not been bound. Nevertheless it must be printed.\n");
      printMessage(1,"Will bind the current free variable to \"x\".\n");
      variablename = (char *) safeCalloc(2,sizeof(char));
      variablename[0] = 'x';
      sollyaFprintf(fd,"%s",variablename);
    }
    break;
  case CONSTANT:
    fprintValueWithPrintMode(fd,*(tree->value));
    break;
  case ADD:
    if (isInfix(tree->child1) && (precedence(tree->child1) < pred) && (tree->child1->nodeType != CONSTANT)) 
      sollyaFprintf(fd,"(");
    fprintTreeWithPrintMode(fd,tree->child1);
    if (isInfix(tree->child1) && (precedence(tree->child1) < pred) && (tree->child1->nodeType != CONSTANT)) 
      sollyaFprintf(fd,")");
    sollyaFprintf(fd," + ");
    if (isInfix(tree->child2) && (precedence(tree->child2) < pred)) 
      sollyaFprintf(fd,"(");
    fprintTreeWithPrintMode(fd,tree->child2);
    if (isInfix(tree->child2) && (precedence(tree->child2) < pred)) 
      sollyaFprintf(fd,")");
    break;
  case SUB:
    if (isInfix(tree->child1) && (precedence(tree->child1) < pred) && (tree->child1->nodeType != CONSTANT)) 
      sollyaFprintf(fd,"(");
    fprintTreeWithPrintMode(fd,tree->child1);
    if (isInfix(tree->child1) && (precedence(tree->child1) < pred) && (tree->child1->nodeType != CONSTANT)) 
      sollyaFprintf(fd,")");
    sollyaFprintf(fd," - ");
    if (isInfix(tree->child2) && (precedence(tree->child2) <= pred)) 
      sollyaFprintf(fd,"(");
    fprintTreeWithPrintMode(fd,tree->child2);
    if (isInfix(tree->child2) && (precedence(tree->child2) <= pred)) 
      sollyaFprintf(fd,")");
    break;
  case MUL:
    if (isInfix(tree->child1) && (precedence(tree->child1) < pred) && (tree->child1->nodeType != CONSTANT)) 
      sollyaFprintf(fd,"(");
    fprintTreeWithPrintMode(fd,tree->child1);
    if (isInfix(tree->child1) && (precedence(tree->child1) < pred) && (tree->child1->nodeType != CONSTANT)) 
      sollyaFprintf(fd,")");
    sollyaFprintf(fd," * ");
    if (isInfix(tree->child2) && (precedence(tree->child2) < pred)) 
      sollyaFprintf(fd,"(");
    fprintTreeWithPrintMode(fd,tree->child2);
    if (isInfix(tree->child2) && (precedence(tree->child2) < pred)) 
      sollyaFprintf(fd,")");
    break;
  case DIV:
    if (isInfix(tree->child1) && (precedence(tree->child1) < pred) && (tree->child1->nodeType != CONSTANT)) 
      sollyaFprintf(fd,"(");
    fprintTreeWithPrintMode(fd,tree->child1);
    if (isInfix(tree->child1) && (precedence(tree->child1) < pred) && (tree->child1->nodeType != CONSTANT)) 
      sollyaFprintf(fd,")");
    sollyaFprintf(fd," / ");
    if (isInfix(tree->child2) && (precedence(tree->child2) <= pred)) 
      sollyaFprintf(fd,"(");
    fprintTreeWithPrintMode(fd,tree->child2);
    if (isInfix(tree->child2) && (precedence(tree->child2) <= pred)) 
      sollyaFprintf(fd,")");
    break;
  case SQRT:
    sollyaFprintf(fd,"sqrt(");
    fprintTreeWithPrintMode(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case EXP:
    sollyaFprintf(fd,"exp(");
    fprintTreeWithPrintMode(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case LOG:
    sollyaFprintf(fd,"log(");
    fprintTreeWithPrintMode(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case LOG_2:
    sollyaFprintf(fd,"log2(");
    fprintTreeWithPrintMode(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case LOG_10:
    sollyaFprintf(fd,"log10(");
    fprintTreeWithPrintMode(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case SIN:
    sollyaFprintf(fd,"sin(");
    fprintTreeWithPrintMode(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case COS:
    sollyaFprintf(fd,"cos(");
    fprintTreeWithPrintMode(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case TAN:
    sollyaFprintf(fd,"tan(");
    fprintTreeWithPrintMode(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case ASIN:
    sollyaFprintf(fd,"asin(");
    fprintTreeWithPrintMode(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case ACOS:
    sollyaFprintf(fd,"acos(");
    fprintTreeWithPrintMode(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case ATAN:
    sollyaFprintf(fd,"atan(");
    fprintTreeWithPrintMode(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case SINH:
    sollyaFprintf(fd,"sinh(");
    fprintTreeWithPrintMode(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case COSH:
    sollyaFprintf(fd,"cosh(");
    fprintTreeWithPrintMode(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case TANH:
    sollyaFprintf(fd,"tanh(");
    fprintTreeWithPrintMode(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case ASINH:
    sollyaFprintf(fd,"asinh(");
    fprintTreeWithPrintMode(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case ACOSH:
    sollyaFprintf(fd,"acosh(");
    fprintTreeWithPrintMode(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case ATANH:
    sollyaFprintf(fd,"atanh(");
    fprintTreeWithPrintMode(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case POW:
    if (isInfix(tree->child1) && (precedence(tree->child1) <= pred)) 
      sollyaFprintf(fd,"(");
    fprintTreeWithPrintMode(fd,tree->child1);
    if (isInfix(tree->child1) && (precedence(tree->child1) <= pred)) 
      sollyaFprintf(fd,")");
    sollyaFprintf(fd,"^");
    if (isInfix(tree->child2) && ((precedence(tree->child2) <= pred)
				  || ((tree->child2->nodeType == CONSTANT) 
				      && ((dyadic == 2) || (dyadic == 3))))) {
      sollyaFprintf(fd,"(");
    }
    fprintTreeWithPrintMode(fd,tree->child2);
    if (isInfix(tree->child2) && ((precedence(tree->child2) <= pred)
				  || ((tree->child2->nodeType == CONSTANT) 
				      && ((dyadic == 2) || (dyadic == 3))))) {
      sollyaFprintf(fd,")");
    }
    break;
  case NEG:
    sollyaFprintf(fd,"-");
    if (isInfix(tree->child1) && (precedence(tree->child1) <= pred)) 
      sollyaFprintf(fd,"(");
    fprintTreeWithPrintMode(fd,tree->child1);
    if (isInfix(tree->child1) && (precedence(tree->child1) <= pred)) 
      sollyaFprintf(fd,")");
    break;
  case ABS:
    sollyaFprintf(fd,"abs(");
    fprintTreeWithPrintMode(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case DOUBLE:
    sollyaFprintf(fd,"double(");
    fprintTreeWithPrintMode(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case SINGLE:
    sollyaFprintf(fd,"single(");
    fprintTreeWithPrintMode(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case HALFPRECISION:
    sollyaFprintf(fd,"halfprecision(");
    fprintTreeWithPrintMode(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case QUAD:
    sollyaFprintf(fd,"quad(");
    fprintTreeWithPrintMode(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case DOUBLEDOUBLE:
    sollyaFprintf(fd,"doubledouble(");
    fprintTreeWithPrintMode(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case TRIPLEDOUBLE:
    sollyaFprintf(fd,"tripledouble(");
    fprintTreeWithPrintMode(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case ERF: 
    sollyaFprintf(fd,"erf(");
    fprintTreeWithPrintMode(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case ERFC:
    sollyaFprintf(fd,"erfc(");
    fprintTreeWithPrintMode(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case LOG_1P:
    sollyaFprintf(fd,"log1p(");
    fprintTreeWithPrintMode(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case EXP_M1:
    sollyaFprintf(fd,"expm1(");
    fprintTreeWithPrintMode(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case DOUBLEEXTENDED:
    sollyaFprintf(fd,"doubleextended(");
    fprintTreeWithPrintMode(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case LIBRARYFUNCTION:
    {
      if (tree->child1->nodeType == VARIABLE) {
	for (i=1;i<=tree->libFunDeriv;i++) {
	  sollyaFprintf(fd,"diff(");
	}
	sollyaFprintf(fd,"%s",tree->libFun->functionName);
	for (i=1;i<=tree->libFunDeriv;i++) {
	  sollyaFprintf(fd,")");
	}
      } else {
	if (tree->libFunDeriv == 0) {
	  sollyaFprintf(fd,"%s(",tree->libFun->functionName);
	  fprintTreeWithPrintMode(fd,tree->child1);
	  sollyaFprintf(fd,")");
	} else {
	  sollyaFprintf(fd,"(");
	  for (i=1;i<=tree->libFunDeriv;i++) {
	    sollyaFprintf(fd,"diff(");
	  }
	  sollyaFprintf(fd,"%s",tree->libFun->functionName);
	  for (i=1;i<=tree->libFunDeriv;i++) {
	    sollyaFprintf(fd,")");
	  }
	  sollyaFprintf(fd,")(");
	  fprintTreeWithPrintMode(fd,tree->child1);
	  sollyaFprintf(fd,")");
	}
      }
    }
    break;
  case PROCEDUREFUNCTION:
    {
      if (tree->child1->nodeType == VARIABLE) {
	for (i=1;i<=tree->libFunDeriv;i++) {
	  sollyaFprintf(fd,"diff(");
	}
	sollyaFprintf(fd,"function(");
	fPrintThing(fd,tree->child2);
	sollyaFprintf(fd,")");
	for (i=1;i<=tree->libFunDeriv;i++) {
	  sollyaFprintf(fd,")");
	}
      } else {
	if (tree->libFunDeriv == 0) {
	  sollyaFprintf(fd,"(function(");
	  fPrintThing(fd,tree->child2);
	  sollyaFprintf(fd,"))(");
	  fprintTreeWithPrintMode(fd,tree->child1);
	  sollyaFprintf(fd,")");
	} else {
	  sollyaFprintf(fd,"(");
	  for (i=1;i<=tree->libFunDeriv;i++) {
	    sollyaFprintf(fd,"diff(");
	  }
	  sollyaFprintf(fd,"function(");
	  fPrintThing(fd,tree->child2);
	  sollyaFprintf(fd,")");
	  for (i=1;i<=tree->libFunDeriv;i++) {
	    sollyaFprintf(fd,")");
	  }
	  sollyaFprintf(fd,")(");
	  fprintTreeWithPrintMode(fd,tree->child1);
	  sollyaFprintf(fd,")");
	}
      }
    }
    break;
  case CEIL:
    sollyaFprintf(fd,"ceil(");
    fprintTreeWithPrintMode(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case FLOOR:
    sollyaFprintf(fd,"floor(");
    fprintTreeWithPrintMode(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case NEARESTINT:
    sollyaFprintf(fd,"nearestint(");
    fprintTreeWithPrintMode(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case PI_CONST:
    sollyaFprintf(fd,"pi");
    break;
  case LIBRARYCONSTANT:
    sollyaFprintf(fd,"%s",tree->libFun->functionName);
    break;
  default:
   sollyaFprintf(stderr,"Error: fprintTreeWithPrintMode: unknown identifier in the tree\n");
   exit(1);
  }
  return;
}






void fprintValue(FILE *fd, mpfr_t value) {
  mpfr_t y;
  char *str, *str2;
  mp_exp_t e, expo;
  mp_prec_t prec;

  if (mpfr_zero_p(value)) {
    sollyaFprintf(fd,"0");
  } else {
    prec = mpfr_get_prec(value);
    mpfr_init2(y,prec+10);
    mpfr_set(y,value,GMP_RNDN);
    if (mpfr_sgn(y) < 0) {
      sollyaFprintf(fd,"-"); mpfr_neg(y,y,GMP_RNDN);
    }
    if (!mpfr_number_p(value)) {
      str = mpfr_get_str(NULL,&e,10,0,y,GMP_RNDN);
      sollyaFprintf(fd,"%s",str);
    } else {
      expo = mpfr_get_exp(y);
      if (mpfr_set_exp(y,prec+10)) {
	printMessage(1,"\nWarning: upon printing to a file: %d is not in the current exponent range of a variable. Values printed may be wrong.\n",(int)(prec+10));
      }
      expo -= prec+10;
      while (mpfr_integer_p(y)) {
	mpfr_div_2ui(y,y,1,GMP_RNDN);
	expo += 1;
      }
      expo--;
      if (mpfr_mul_2ui(y,y,1,GMP_RNDN) != 0) {
	if (!noRoundingWarnings) {
	  printMessage(1,"\nWarning: upon printing to a file: rounding occurred. Values printed may be wrong.\n");
	}
      }
      str = mpfr_get_str(NULL,&e,10,0,y,GMP_RNDN);
      str2 = (char *) safeCalloc(strlen(str)+1,sizeof(char));
      strncpy(str2,str,e);
      sollyaFprintf(fd,"%sb%d",str2,(int)expo);
      free(str2);
    }
    free(str);
    mpfr_clear(y);
  }
}

void fprintValueForXml(FILE *fd, mpfr_t value) {
  mpfr_t y, h;
  char *str, *str2;
  mp_exp_t e, expo;
  mp_prec_t prec;
  int negate, val;

  if (mpfr_zero_p(value)) {
    sollyaFprintf(fd,"<cn type=\"integer\" base=\"10\"> 0 </cn>\n");
  } else {
    prec = mpfr_get_prec(value);
    mpfr_init2(y,prec+10);
    mpfr_set(y,value,GMP_RNDN);
    val = mpfr_get_si(y,GMP_RNDN);
    mpfr_init2(h,prec);
    mpfr_set_si(h,val,GMP_RNDN);
    if (mpfr_number_p(y) && (mpfr_cmp(h,y) == 0)) {
      mpfr_clear(h);
      sollyaFprintf(fd,"<cn type=\"integer\" base=\"10\"> %d </cn>\n",val);
    } else {
      mpfr_clear(h);
      negate = 0;
      if (mpfr_sgn(y) < 0) {
	negate = 1; mpfr_neg(y,y,GMP_RNDN);
      } 
      if (!mpfr_number_p(value)) {
	str = mpfr_get_str(NULL,&e,10,0,y,GMP_RNDN);
	if (!negate) 
	  sollyaFprintf(fd,"<cn type=\"real\"> %s </cn>\n",str);
	else 
	  sollyaFprintf(fd,"<cn type=\"real\"> -%s </cn>\n",str);
      } else {
	expo = mpfr_get_exp(y);
	if (mpfr_set_exp(y,prec+10)) {
	  printMessage(1,"\nWarning: upon printing to a file: %d is not in the current exponent range of a variable. Values printed may be wrong.\n",(int)(prec+10));
	}
	expo -= prec+10;
	while (mpfr_integer_p(y)) {
	  mpfr_div_2ui(y,y,1,GMP_RNDN);
	  expo += 1;
	}
	expo--;
	if (mpfr_mul_2ui(y,y,1,GMP_RNDN) != 0) {
	  if (!noRoundingWarnings) {
	    printMessage(1,"\nWarning: upon printing to a file: rounding occurred. Values printed may be wrong.\n");
	  }
	}
	str = mpfr_get_str(NULL,&e,10,0,y,GMP_RNDN);
	str2 = (char *) safeCalloc(strlen(str)+1,sizeof(char));
	strncpy(str2,str,e);
	if (!negate) {
	  sollyaFprintf(fd,"<apply>\n");
	  sollyaFprintf(fd,"<times/>\n");
	  sollyaFprintf(fd,"<cn type=\"integer\" base=\"10\"> %s </cn>\n",str2);
	  sollyaFprintf(fd,"<apply>\n");
	  sollyaFprintf(fd,"<power/>\n");
	  sollyaFprintf(fd,"<cn type=\"integer\" base=\"10\"> 2 </cn>\n");
	  sollyaFprintf(fd,"<cn type=\"integer\" base=\"10\"> %d </cn>\n",(int) expo);
	  sollyaFprintf(fd,"</apply>\n");
	  sollyaFprintf(fd,"</apply>\n");
	} else {
	  sollyaFprintf(fd,"<apply>\n");
	  sollyaFprintf(fd,"<times/>\n");
	  sollyaFprintf(fd,"<cn type=\"integer\" base=\"10\"> -%s </cn>\n",str2);
	  sollyaFprintf(fd,"<apply>\n");
	  sollyaFprintf(fd,"<power/>\n");
	  sollyaFprintf(fd,"<cn type=\"integer\" base=\"10\"> 2 </cn>\n");
	  sollyaFprintf(fd,"<cn type=\"integer\" base=\"10\"> %d </cn>\n",(int) expo);
	  sollyaFprintf(fd,"</apply>\n");
	  sollyaFprintf(fd,"</apply>\n");
	}
	free(str2);
      }
    }
    mpfr_clear(y);
  }
}



void printTree(node *tree) {
  int pred, i;

  if (fullParentheses) pred = 100; else pred = precedence(tree);

  switch (tree->nodeType) {
  case VARIABLE:
    if (variablename != NULL) {
      sollyaPrintf("%s",variablename);
    } else {
      printMessage(1,"Warning: the current free variable has not been bound. Nevertheless it must be printed.\n");
      printMessage(1,"Will bind the current free variable to \"x\".\n");
      variablename = (char *) safeCalloc(2,sizeof(char));
      variablename[0] = 'x';
      sollyaPrintf("%s",variablename);
    }
    break;
  case CONSTANT:
    printValue(tree->value);
    break;
  case ADD:
    if (isInfix(tree->child1) && (precedence(tree->child1) < pred) && (tree->child1->nodeType != CONSTANT)) 
      sollyaPrintf("(");
    printTree(tree->child1);
    if (isInfix(tree->child1) && (precedence(tree->child1) < pred) && (tree->child1->nodeType != CONSTANT)) 
      sollyaPrintf(")");
    sollyaPrintf(" + ");
    if (isInfix(tree->child2) && (precedence(tree->child2) < pred)) 
      sollyaPrintf("(");
    printTree(tree->child2);
    if (isInfix(tree->child2) && (precedence(tree->child2) < pred)) 
      sollyaPrintf(")");
    break;
  case SUB:
    if (isInfix(tree->child1) && (precedence(tree->child1) < pred) && (tree->child1->nodeType != CONSTANT)) 
      sollyaPrintf("(");
    printTree(tree->child1);
    if (isInfix(tree->child1) && (precedence(tree->child1) < pred) && (tree->child1->nodeType != CONSTANT)) 
      sollyaPrintf(")");
    sollyaPrintf(" - ");
    if (isInfix(tree->child2) && (precedence(tree->child2) <= pred)) 
      sollyaPrintf("(");
    printTree(tree->child2);
    if (isInfix(tree->child2) && (precedence(tree->child2) <= pred)) 
      sollyaPrintf(")");
    break;
  case MUL:
    if (isInfix(tree->child1) && (precedence(tree->child1) < pred) && (tree->child1->nodeType != CONSTANT)) 
      sollyaPrintf("(");
    printTree(tree->child1);
    if (isInfix(tree->child1) && (precedence(tree->child1) < pred) && (tree->child1->nodeType != CONSTANT)) 
      sollyaPrintf(")");
    sollyaPrintf(" * ");
    if (isInfix(tree->child2) && (precedence(tree->child2) < pred)) 
      sollyaPrintf("(");
    printTree(tree->child2);
    if (isInfix(tree->child2) && (precedence(tree->child2) < pred)) 
      sollyaPrintf(")");
    break;
  case DIV:
    if (isInfix(tree->child1) && (precedence(tree->child1) < pred) && (tree->child1->nodeType != CONSTANT)) 
      sollyaPrintf("(");
    printTree(tree->child1);
    if (isInfix(tree->child1) && (precedence(tree->child1) < pred) && (tree->child1->nodeType != CONSTANT)) 
      sollyaPrintf(")");
    sollyaPrintf(" / ");
    if (isInfix(tree->child2) && (precedence(tree->child2) <= pred)) 
      sollyaPrintf("(");
    printTree(tree->child2);
    if (isInfix(tree->child2) && (precedence(tree->child2) <= pred)) 
      sollyaPrintf(")");
    break;
  case SQRT:
    sollyaPrintf("sqrt(");
    printTree(tree->child1);
    sollyaPrintf(")");
    break;
  case EXP:
    sollyaPrintf("exp(");
    printTree(tree->child1);
    sollyaPrintf(")");
    break;
  case LOG:
    sollyaPrintf("log(");
    printTree(tree->child1);
    sollyaPrintf(")");
    break;
  case LOG_2:
    sollyaPrintf("log2(");
    printTree(tree->child1);
    sollyaPrintf(")");
    break;
  case LOG_10:
    sollyaPrintf("log10(");
    printTree(tree->child1);
    sollyaPrintf(")");
    break;
  case SIN:
    sollyaPrintf("sin(");
    printTree(tree->child1);
    sollyaPrintf(")");
    break;
  case COS:
    sollyaPrintf("cos(");
    printTree(tree->child1);
    sollyaPrintf(")");
    break;
  case TAN:
    sollyaPrintf("tan(");
    printTree(tree->child1);
    sollyaPrintf(")");
    break;
  case ASIN:
    sollyaPrintf("asin(");
    printTree(tree->child1);
    sollyaPrintf(")");
    break;
  case ACOS:
    sollyaPrintf("acos(");
    printTree(tree->child1);
    sollyaPrintf(")");
    break;
  case ATAN:
    sollyaPrintf("atan(");
    printTree(tree->child1);
    sollyaPrintf(")");
    break;
  case SINH:
    sollyaPrintf("sinh(");
    printTree(tree->child1);
    sollyaPrintf(")");
    break;
  case COSH:
    sollyaPrintf("cosh(");
    printTree(tree->child1);
    sollyaPrintf(")");
    break;
  case TANH:
    sollyaPrintf("tanh(");
    printTree(tree->child1);
    sollyaPrintf(")");
    break;
  case ASINH:
    sollyaPrintf("asinh(");
    printTree(tree->child1);
    sollyaPrintf(")");
    break;
  case ACOSH:
    sollyaPrintf("acosh(");
    printTree(tree->child1);
    sollyaPrintf(")");
    break;
  case ATANH:
    sollyaPrintf("atanh(");
    printTree(tree->child1);
    sollyaPrintf(")");
    break;
  case POW:
    if (isInfix(tree->child1) && (precedence(tree->child1) <= pred)) 
      sollyaPrintf("(");
    printTree(tree->child1);
    if (isInfix(tree->child1) && (precedence(tree->child1) <= pred)) 
      sollyaPrintf(")");
    sollyaPrintf("^");
    if (isInfix(tree->child2) && ((precedence(tree->child2) <= pred)
				  || ((tree->child2->nodeType == CONSTANT) 
				      && ((dyadic == 2) || (dyadic == 3))))) {
      sollyaPrintf("(");
    }
    printTree(tree->child2);
    if (isInfix(tree->child2) && ((precedence(tree->child2) <= pred)
				  || ((tree->child2->nodeType == CONSTANT) 
				      && ((dyadic == 2) || (dyadic == 3))))) {
      sollyaPrintf(")");
    }
    break;
  case NEG:
    sollyaPrintf("-");
    if (isInfix(tree->child1) && (precedence(tree->child1) <= pred)) 
      sollyaPrintf("(");
    printTree(tree->child1);
    if (isInfix(tree->child1) && (precedence(tree->child1) <= pred)) 
      sollyaPrintf(")");
    break;
  case ABS:
    sollyaPrintf("abs(");
    printTree(tree->child1);
    sollyaPrintf(")");
    break;
  case DOUBLE:
    sollyaPrintf("double(");
    printTree(tree->child1);
    sollyaPrintf(")");
    break;
  case SINGLE:
    sollyaPrintf("single(");
    printTree(tree->child1);
    sollyaPrintf(")");
    break;
  case HALFPRECISION:
    sollyaPrintf("halfprecision(");
    printTree(tree->child1);
    sollyaPrintf(")");
    break;
  case QUAD:
    sollyaPrintf("quad(");
    printTree(tree->child1);
    sollyaPrintf(")");
    break;
  case DOUBLEDOUBLE:
    sollyaPrintf("doubledouble(");
    printTree(tree->child1);
    sollyaPrintf(")");
    break;
  case TRIPLEDOUBLE:
    sollyaPrintf("tripledouble(");
    printTree(tree->child1);
    sollyaPrintf(")");
    break;
  case ERF: 
    sollyaPrintf("erf(");
    printTree(tree->child1);
    sollyaPrintf(")");
    break;
  case ERFC:
    sollyaPrintf("erfc(");
    printTree(tree->child1);
    sollyaPrintf(")");
    break;
  case LOG_1P:
    sollyaPrintf("log1p(");
    printTree(tree->child1);
    sollyaPrintf(")");
    break;
  case EXP_M1:
    sollyaPrintf("expm1(");
    printTree(tree->child1);
    sollyaPrintf(")");
    break;
  case DOUBLEEXTENDED:
    sollyaPrintf("doubleextended(");
    printTree(tree->child1);
    sollyaPrintf(")");
    break;
  case LIBRARYFUNCTION:
    {
      if (tree->child1->nodeType == VARIABLE) {
	for (i=1;i<=tree->libFunDeriv;i++) {
	  sollyaPrintf("diff(");
	}
	sollyaPrintf("%s",tree->libFun->functionName);
	for (i=1;i<=tree->libFunDeriv;i++) {
	  sollyaPrintf(")");
	}
      } else {
	if (tree->libFunDeriv == 0) {
	  sollyaPrintf("%s(",tree->libFun->functionName);
	  printTree(tree->child1);
	  sollyaPrintf(")");
	} else {
	  sollyaPrintf("(");
	  for (i=1;i<=tree->libFunDeriv;i++) {
	    sollyaPrintf("diff(");
	  }
	  sollyaPrintf("%s",tree->libFun->functionName);
	  for (i=1;i<=tree->libFunDeriv;i++) {
	    sollyaPrintf(")");
	  }
	  sollyaPrintf(")(");
	  printTree(tree->child1);
	  sollyaPrintf(")");
	}
      }
    }
    break;
  case PROCEDUREFUNCTION:
    {
      if (tree->child1->nodeType == VARIABLE) {
	for (i=1;i<=tree->libFunDeriv;i++) {
	  sollyaPrintf("diff(");
	}
	sollyaPrintf("function(");
	printThing(tree->child2);
	sollyaPrintf(")");
	for (i=1;i<=tree->libFunDeriv;i++) {
	  sollyaPrintf(")");
	}
      } else {
	if (tree->libFunDeriv == 0) {
	  sollyaPrintf("(function(");
	  printThing(tree->child2);
	  sollyaPrintf("))(");
	  printTree(tree->child1);
	  sollyaPrintf(")");
	} else {
	  sollyaPrintf("(");
	  for (i=1;i<=tree->libFunDeriv;i++) {
	    sollyaPrintf("diff(");
	  }
	  sollyaPrintf("function(");
	  printThing(tree->child2);
	  sollyaPrintf(")");
	  for (i=1;i<=tree->libFunDeriv;i++) {
	    sollyaPrintf(")");
	  }
	  sollyaPrintf(")(");
	  printTree(tree->child1);
	  sollyaPrintf(")");
	}
      }
    }
    break;
  case CEIL:
    sollyaPrintf("ceil(");
    printTree(tree->child1);
    sollyaPrintf(")");
    break;
  case FLOOR:
    sollyaPrintf("floor(");
    printTree(tree->child1);
    sollyaPrintf(")");
    break;
  case NEARESTINT:
    sollyaPrintf("nearestint(");
    printTree(tree->child1);
    sollyaPrintf(")");
    break;
  case PI_CONST:
    sollyaPrintf("pi");
    break;
  case LIBRARYCONSTANT:
    sollyaPrintf("%s",tree->libFun->functionName);
    break;
  default:
   sollyaFprintf(stderr,"Error: printTree: unknown identifier in the tree\n");
   exit(1);
  }
  return;
}

char *sprintTree(node *tree) {
  int pred, i;
  char *buffer, *buffer1, *buffer2, *finalBuffer, *tempBuf;

  buffer1 = NULL;
  buffer2 = NULL;
  if (fullParentheses) pred = 100; else pred = precedence(tree);
  switch (tree->nodeType) {
  case VARIABLE:
    if (variablename == NULL) {
      printMessage(1,"Warning: the current free variable has not been bound. Nevertheless it must be printed.\n");
      printMessage(1,"Will bind the current free variable to \"x\".\n");
      variablename = (char *) safeCalloc(2,sizeof(char));
      variablename[0] = 'x';
    }
    buffer = (char *) safeCalloc(strlen(variablename)+1,sizeof(char));
    sprintf(buffer,"%s",variablename);
    break;
  case CONSTANT:
    buffer = sprintValue(tree->value);
    break;
  case ADD:
    buffer1 = sprintTree(tree->child1);
    buffer2 = sprintTree(tree->child2);
    buffer = (char *) safeCalloc(strlen(buffer1) + strlen(buffer2) + 9, sizeof(char));
    tempBuf = buffer;
    if (isInfix(tree->child1) && (precedence(tree->child1) < pred) && (tree->child1->nodeType != CONSTANT)) 
      tempBuf += sprintf(tempBuf,"(");
    tempBuf += sprintf(tempBuf,"%s",buffer1);
    if (isInfix(tree->child1) && (precedence(tree->child1) < pred) && (tree->child1->nodeType != CONSTANT)) 
      tempBuf += sprintf(tempBuf,")");
    tempBuf += sprintf(tempBuf," + ");
    if (isInfix(tree->child2) && (precedence(tree->child2) < pred)) 
      tempBuf += sprintf(tempBuf,"(");
    tempBuf += sprintf(tempBuf,"%s",buffer2);
    if (isInfix(tree->child2) && (precedence(tree->child2) < pred)) 
      tempBuf += sprintf(tempBuf,")");
    break;
  case SUB:
    buffer1 = sprintTree(tree->child1);
    buffer2 = sprintTree(tree->child2);
    buffer = (char *) safeCalloc(strlen(buffer1) + strlen(buffer2) + 9, sizeof(char));
    tempBuf = buffer;
    if (isInfix(tree->child1) && (precedence(tree->child1) < pred) && (tree->child1->nodeType != CONSTANT)) 
      tempBuf += sprintf(tempBuf,"(");
    tempBuf += sprintf(tempBuf,"%s",buffer1);
    if (isInfix(tree->child1) && (precedence(tree->child1) < pred) && (tree->child1->nodeType != CONSTANT)) 
      tempBuf += sprintf(tempBuf,")");
    tempBuf += sprintf(tempBuf," - ");
    if (isInfix(tree->child2) && (precedence(tree->child2) <= pred)) 
      tempBuf += sprintf(tempBuf,"(");
    tempBuf += sprintf(tempBuf,"%s",buffer2);
    if (isInfix(tree->child2) && (precedence(tree->child2) <= pred)) 
      tempBuf += sprintf(tempBuf,")");
    break;
  case MUL:
    buffer1 = sprintTree(tree->child1);
    buffer2 = sprintTree(tree->child2);
    buffer = (char *) safeCalloc(strlen(buffer1) + strlen(buffer2) + 9, sizeof(char));
    tempBuf = buffer;
    if (isInfix(tree->child1) && (precedence(tree->child1) < pred) && (tree->child1->nodeType != CONSTANT)) 
      tempBuf += sprintf(tempBuf,"(");
    tempBuf += sprintf(tempBuf,"%s",buffer1);
    if (isInfix(tree->child1) && (precedence(tree->child1) < pred) && (tree->child1->nodeType != CONSTANT)) 
      tempBuf += sprintf(tempBuf,")");
    tempBuf += sprintf(tempBuf," * ");
    if (isInfix(tree->child2) && (precedence(tree->child2) < pred)) 
      tempBuf += sprintf(tempBuf,"(");
    tempBuf += sprintf(tempBuf,"%s",buffer2);
    if (isInfix(tree->child2) && (precedence(tree->child2) < pred)) 
      tempBuf += sprintf(tempBuf,")");
    break;
  case DIV:
    buffer1 = sprintTree(tree->child1);
    buffer2 = sprintTree(tree->child2);
    buffer = (char *) safeCalloc(strlen(buffer1) + strlen(buffer2) + 9, sizeof(char));
    tempBuf = buffer;
    if (isInfix(tree->child1) && (precedence(tree->child1) < pred) && (tree->child1->nodeType != CONSTANT)) 
      tempBuf += sprintf(tempBuf,"(");
    tempBuf += sprintf(tempBuf,"%s",buffer1);
    if (isInfix(tree->child1) && (precedence(tree->child1) < pred) && (tree->child1->nodeType != CONSTANT)) 
      tempBuf += sprintf(tempBuf,")");
    tempBuf += sprintf(tempBuf," / ");
    if (isInfix(tree->child2) && (precedence(tree->child2) <= pred)) 
      tempBuf += sprintf(tempBuf,"(");
    tempBuf += sprintf(tempBuf,"%s",buffer2);
    if (isInfix(tree->child2) && (precedence(tree->child2) <= pred)) 
      tempBuf += sprintf(tempBuf,")");
    break;
  case SQRT:
    buffer1 = sprintTree(tree->child1);
    buffer = (char *) safeCalloc(strlen(buffer1) + 8, sizeof(char));
    sprintf(buffer,"sqrt(%s)",buffer1);
    break;
  case EXP:
    buffer1 = sprintTree(tree->child1);
    buffer = (char *) safeCalloc(strlen(buffer1) + 8, sizeof(char));
    sprintf(buffer,"exp(%s)",buffer1);
    break;
  case LOG:
    buffer1 = sprintTree(tree->child1);
    buffer = (char *) safeCalloc(strlen(buffer1) + 8, sizeof(char));
    sprintf(buffer,"log(%s)",buffer1);
    break;
  case LOG_2:
    buffer1 = sprintTree(tree->child1);
    buffer = (char *) safeCalloc(strlen(buffer1) + 8, sizeof(char));
    sprintf(buffer,"log2(%s)",buffer1);
    break;
  case LOG_10:
    buffer1 = sprintTree(tree->child1);
    buffer = (char *) safeCalloc(strlen(buffer1) + 9, sizeof(char));
    sprintf(buffer,"log10(%s)",buffer1);
    break;
  case SIN:
    buffer1 = sprintTree(tree->child1);
    buffer = (char *) safeCalloc(strlen(buffer1) + 8, sizeof(char));
    sprintf(buffer,"sin(%s)",buffer1);
    break;
  case COS:
    buffer1 = sprintTree(tree->child1);
    buffer = (char *) safeCalloc(strlen(buffer1) + 8, sizeof(char));
    sprintf(buffer,"cos(%s)",buffer1);
    break;
  case TAN:
    buffer1 = sprintTree(tree->child1);
    buffer = (char *) safeCalloc(strlen(buffer1) + 8, sizeof(char));
    sprintf(buffer,"tan(%s)",buffer1);
    break;
  case ASIN:
    buffer1 = sprintTree(tree->child1);
    buffer = (char *) safeCalloc(strlen(buffer1) + 8, sizeof(char));
    sprintf(buffer,"asin(%s)",buffer1);
    break;
  case ACOS:
    buffer1 = sprintTree(tree->child1);
    buffer = (char *) safeCalloc(strlen(buffer1) + 8, sizeof(char));
    sprintf(buffer,"acos(%s)",buffer1);
    break;
  case ATAN:
    buffer1 = sprintTree(tree->child1);
    buffer = (char *) safeCalloc(strlen(buffer1) + 8, sizeof(char));
    sprintf(buffer,"atan(%s)",buffer1);
    break;
  case SINH:
    buffer1 = sprintTree(tree->child1);
    buffer = (char *) safeCalloc(strlen(buffer1) + 8, sizeof(char));
    sprintf(buffer,"sinh(%s)",buffer1);
    break;
  case COSH:
    buffer1 = sprintTree(tree->child1);
    buffer = (char *) safeCalloc(strlen(buffer1) + 8, sizeof(char));
    sprintf(buffer,"cosh(%s)",buffer1);
    break;
  case TANH:
    buffer1 = sprintTree(tree->child1);
    buffer = (char *) safeCalloc(strlen(buffer1) + 8, sizeof(char));
    sprintf(buffer,"tanh(%s)",buffer1);
    break;
  case ASINH:
    buffer1 = sprintTree(tree->child1);
    buffer = (char *) safeCalloc(strlen(buffer1) + 9, sizeof(char));
    sprintf(buffer,"asinh(%s)",buffer1);
    break;
  case ACOSH:
    buffer1 = sprintTree(tree->child1);
    buffer = (char *) safeCalloc(strlen(buffer1) + 9, sizeof(char));
    sprintf(buffer,"acosh(%s)",buffer1);
    break;
  case ATANH:
    buffer1 = sprintTree(tree->child1);
    buffer = (char *) safeCalloc(strlen(buffer1) + 9, sizeof(char));
    sprintf(buffer,"atanh(%s)",buffer1);
    break;
  case POW:
    buffer1 = sprintTree(tree->child1);
    buffer2 = sprintTree(tree->child2);
    buffer = (char *) safeCalloc(strlen(buffer1) + strlen(buffer2) + 9, sizeof(char));
    tempBuf = buffer;
    if (isInfix(tree->child1) && (precedence(tree->child1) <= pred)) 
      tempBuf += sprintf(tempBuf,"(");
    tempBuf += sprintf(tempBuf,"%s",buffer1);
    if (isInfix(tree->child1) && (precedence(tree->child1) <= pred)) 
      tempBuf += sprintf(tempBuf,")");
    tempBuf += sprintf(tempBuf,"^");
    if (isInfix(tree->child2) && ((precedence(tree->child2) <= pred)
				  || ((tree->child2->nodeType == CONSTANT) 
				      && ((dyadic == 2) || (dyadic == 3))))) 
      tempBuf += sprintf(tempBuf,"(");
    tempBuf += sprintf(tempBuf,"%s",buffer2);
    if (isInfix(tree->child2) && ((precedence(tree->child2) <= pred)
				  || ((tree->child2->nodeType == CONSTANT) 
				      && ((dyadic == 2) || (dyadic == 3))))) 
      tempBuf += sprintf(tempBuf,")");
    break;
  case NEG:
    buffer1 = sprintTree(tree->child1);
    buffer = (char *) safeCalloc(strlen(buffer1) + 4, sizeof(char));
    if (isInfix(tree->child1)) sprintf(buffer,"-(%s)",buffer1); else sprintf(buffer,"-%s",buffer1);
    break;
  case ABS:
    buffer1 = sprintTree(tree->child1);
    buffer = (char *) safeCalloc(strlen(buffer1) + 8, sizeof(char));
    sprintf(buffer,"abs(%s)",buffer1);
    break;
  case DOUBLE:
    buffer1 = sprintTree(tree->child1);
    buffer = (char *) safeCalloc(strlen(buffer1) + 10, sizeof(char));
    sprintf(buffer,"double(%s)",buffer1);
    break;
  case SINGLE:
    buffer1 = sprintTree(tree->child1);
    buffer = (char *) safeCalloc(strlen(buffer1) + 10, sizeof(char));
    sprintf(buffer,"single(%s)",buffer1);
    break;
  case HALFPRECISION:
    buffer1 = sprintTree(tree->child1);
    buffer = (char *) safeCalloc(strlen(buffer1) + 17, sizeof(char));
    sprintf(buffer,"halfprecision(%s)",buffer1);
    break;
  case QUAD:
    buffer1 = sprintTree(tree->child1);
    buffer = (char *) safeCalloc(strlen(buffer1) + 8, sizeof(char));
    sprintf(buffer,"quad(%s)",buffer1);
    break;
  case DOUBLEDOUBLE:
    buffer1 = sprintTree(tree->child1);
    buffer = (char *) safeCalloc(strlen(buffer1) + 16, sizeof(char));
    sprintf(buffer,"doubledouble(%s)",buffer1);
    break;
  case TRIPLEDOUBLE:
    buffer1 = sprintTree(tree->child1);
    buffer = (char *) safeCalloc(strlen(buffer1) + 16, sizeof(char));
    sprintf(buffer,"tripledouble(%s)",buffer1);
    break;
  case ERF: 
    buffer1 = sprintTree(tree->child1);
    buffer = (char *) safeCalloc(strlen(buffer1) + 8, sizeof(char));
    sprintf(buffer,"erf(%s)",buffer1);
    break;
  case ERFC:
    buffer1 = sprintTree(tree->child1);
    buffer = (char *) safeCalloc(strlen(buffer1) + 8, sizeof(char));
    sprintf(buffer,"erfc(%s)",buffer1);
    break;
  case LOG_1P:
    buffer1 = sprintTree(tree->child1);
    buffer = (char *) safeCalloc(strlen(buffer1) + 9, sizeof(char));
    sprintf(buffer,"log1p(%s)",buffer1);
    break;
  case EXP_M1:
    buffer1 = sprintTree(tree->child1);
    buffer = (char *) safeCalloc(strlen(buffer1) + 9, sizeof(char));
    sprintf(buffer,"expm1(%s)",buffer1);
    break;
  case DOUBLEEXTENDED:
    buffer1 = sprintTree(tree->child1);
    buffer = (char *) safeCalloc(strlen(buffer1) + 19, sizeof(char));
    sprintf(buffer,"doubleextended(%s)",buffer1);
    break;
  case LIBRARYFUNCTION:
    {
      buffer1 = sprintTree(tree->child1);
      if (tree->child1->nodeType == VARIABLE) {
	buffer = (char *) safeCalloc(strlen(tree->libFun->functionName) + 6 * tree->libFunDeriv + 1, sizeof(char));
	tempBuf = buffer;
	for (i=1;i<=tree->libFunDeriv;i++) {
	  tempBuf += sprintf(tempBuf,"diff(");
	}
	tempBuf += sprintf(tempBuf,"%s",tree->libFun->functionName);
	for (i=1;i<=tree->libFunDeriv;i++) {
	  tempBuf += sprintf(tempBuf,")");
	}
      } else {
	if (tree->libFunDeriv == 0) {
	  buffer = (char *) safeCalloc(strlen(tree->libFun->functionName) + strlen(buffer1) + 2 + 1, sizeof(char));
	  tempBuf = buffer;
	  tempBuf += sprintf(tempBuf,"%s(",tree->libFun->functionName);
	  tempBuf += sprintf(tempBuf,"%s",buffer1);
	  tempBuf += sprintf(tempBuf,")");
	} else {
	  buffer = (char *) safeCalloc(strlen(tree->libFun->functionName) + strlen(buffer1) + 6 * tree->libFunDeriv + 3 + 1, sizeof(char));
	  tempBuf = buffer;
	  tempBuf += sprintf(tempBuf,"(");
	  for (i=1;i<=tree->libFunDeriv;i++) {
	    tempBuf += sprintf(tempBuf,"diff(");
	  }
	  tempBuf += sprintf(tempBuf,"%s",tree->libFun->functionName);
	  for (i=1;i<=tree->libFunDeriv;i++) {
	    tempBuf += sprintf(tempBuf,")");
	  }
	  tempBuf += sprintf(tempBuf,")(");
	  tempBuf += sprintf(tempBuf,"%s",buffer1);
	  tempBuf += sprintf(tempBuf,")");
	}
      }
    }
    break;
  case PROCEDUREFUNCTION:
    {
      buffer1 = sprintTree(tree->child1);
      buffer2 = sPrintThing(tree->child2);
      if (tree->child1->nodeType == VARIABLE) {
	buffer = (char *) safeCalloc(strlen(buffer2) + 6 * tree->libFunDeriv + 10 + 1, sizeof(char));
	tempBuf = buffer;
	for (i=1;i<=tree->libFunDeriv;i++) {
	  tempBuf += sprintf(tempBuf,"diff(");
	}
	tempBuf += sprintf(tempBuf,"function(");
	tempBuf += sprintf(tempBuf,"%s",buffer2);
	tempBuf += sprintf(tempBuf,")");
	for (i=1;i<=tree->libFunDeriv;i++) {
	  tempBuf += sprintf(tempBuf,")");
	}
      } else {
	if (tree->libFunDeriv == 0) {
	  buffer = (char *) safeCalloc(strlen(buffer1) + strlen(buffer2) + 14 + 1, sizeof(char));
	  tempBuf = buffer;
	  tempBuf += sprintf(tempBuf,"(function(");
	  tempBuf += sprintf(tempBuf,"%s",buffer2);
	  tempBuf += sprintf(tempBuf,"))(");
	  tempBuf += sprintf(tempBuf,"%s",buffer1);
	  tempBuf += sprintf(tempBuf,")");
	} else {
	  buffer = (char *) safeCalloc(strlen(buffer1) + strlen(buffer2) + 6 * tree->libFunDeriv + 14 + 1, sizeof(char));
	  tempBuf = buffer;
	  tempBuf += sprintf(tempBuf,"(");
	  for (i=1;i<=tree->libFunDeriv;i++) {
	    tempBuf += sprintf(tempBuf,"diff(");
	  }
	  tempBuf += sprintf(tempBuf,"function(");
	  tempBuf += sprintf(tempBuf,"%s",buffer2); 
	  tempBuf += sprintf(tempBuf,")");
	  for (i=1;i<=tree->libFunDeriv;i++) {
	    tempBuf += sprintf(tempBuf,")");
	  }
	  tempBuf += sprintf(tempBuf,")(");
	  tempBuf += sprintf(tempBuf,"%s",buffer1);
	  tempBuf += sprintf(tempBuf,")");
	}
      }
    }
    break;
  case CEIL:
    buffer1 = sprintTree(tree->child1);
    buffer = (char *) safeCalloc(strlen(buffer1) + 8, sizeof(char));
    sprintf(buffer,"ceil(%s)",buffer1);
    break;
  case FLOOR:
    buffer1 = sprintTree(tree->child1);
    buffer = (char *) safeCalloc(strlen(buffer1) + 9, sizeof(char));
    sprintf(buffer,"floor(%s)",buffer1);
    break;
  case NEARESTINT:
    buffer1 = sprintTree(tree->child1);
    buffer = (char *) safeCalloc(strlen(buffer1) + 12, sizeof(char));
    sprintf(buffer,"nearestint(%s)",buffer1);
    break;
  case PI_CONST:
    buffer = (char *) safeCalloc(3, sizeof(char));
    sprintf(buffer,"pi");
    break;
  case LIBRARYCONSTANT:
    buffer = (char *) safeCalloc(strlen(tree->libFun->functionName) + 1,sizeof(char));
    sprintf(buffer,"%s", tree->libFun->functionName);
    break;
  default:
   sollyaFprintf(stderr,"Error: sprintTree: unknown identifier in the tree\n");
   exit(1);
  }

  finalBuffer = (char *) safeCalloc(strlen(buffer)+1,sizeof(char));
  sprintf(finalBuffer,"%s",buffer);
  free(buffer);
  if (buffer1 != NULL) free(buffer1);
  if (buffer2 != NULL) free(buffer2);
  return finalBuffer;
}


void fprintTree(FILE *fd, node *tree) {
  int i;

  if (tree == NULL) return;
  switch (tree->nodeType) {
  case VARIABLE:
    if (variablename == NULL) {
      printMessage(1,"Warning: the current free variable has not been bound. Nevertheless it must be printed.\n");
      printMessage(1,"Will bind the current free variable to \"x\".\n");
      variablename = (char *) safeCalloc(2,sizeof(char));
      variablename[0] = 'x';
    }
    sollyaFprintf(fd,"%s",variablename);
    break;
  case CONSTANT:
    fprintValue(fd,*(tree->value));
    break;
  case ADD:
    if (isInfix(tree->child1)) 
      sollyaFprintf(fd,"(");
    fprintTree(fd,tree->child1);
    if (isInfix(tree->child1)) 
      sollyaFprintf(fd,")");
    sollyaFprintf(fd," + ");
    if (isInfix(tree->child2)) 
      sollyaFprintf(fd,"(");
    fprintTree(fd,tree->child2);
    if (isInfix(tree->child2)) 
      sollyaFprintf(fd,")");
    break;
  case SUB:
    if (isInfix(tree->child1)) 
      sollyaFprintf(fd,"(");
    fprintTree(fd,tree->child1);
    if (isInfix(tree->child1)) 
      sollyaFprintf(fd,")");
    sollyaFprintf(fd," - ");
    if (isInfix(tree->child2)) 
      sollyaFprintf(fd,"(");
    fprintTree(fd,tree->child2);
    if (isInfix(tree->child2)) 
      sollyaFprintf(fd,")");
    break;
  case MUL:
    if (isInfix(tree->child1)) 
      sollyaFprintf(fd,"(");
    fprintTree(fd,tree->child1);
    if (isInfix(tree->child1)) 
      sollyaFprintf(fd,")");
    sollyaFprintf(fd," * ");
    if (isInfix(tree->child2)) 
      sollyaFprintf(fd,"(");
    fprintTree(fd,tree->child2);
    if (isInfix(tree->child2)) 
      sollyaFprintf(fd,")");
    break;
  case DIV:
    if (isInfix(tree->child1)) 
      sollyaFprintf(fd,"(");
    fprintTree(fd,tree->child1);
    if (isInfix(tree->child1)) 
      sollyaFprintf(fd,")");
    sollyaFprintf(fd," / ");
    if (isInfix(tree->child2)) 
      sollyaFprintf(fd,"(");
    fprintTree(fd,tree->child2);
    if (isInfix(tree->child2)) 
      sollyaFprintf(fd,")");
    break;
  case SQRT:
    sollyaFprintf(fd,"sqrt(");
    fprintTree(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case EXP:
    sollyaFprintf(fd,"exp(");
    fprintTree(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case LOG:
    sollyaFprintf(fd,"log(");
    fprintTree(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case LOG_2:
    sollyaFprintf(fd,"log2(");
    fprintTree(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case LOG_10:
    sollyaFprintf(fd,"log10(");
    fprintTree(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case SIN:
    sollyaFprintf(fd,"sin(");
    fprintTree(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case COS:
    sollyaFprintf(fd,"cos(");
    fprintTree(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case TAN:
    sollyaFprintf(fd,"tan(");
    fprintTree(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case ASIN:
    sollyaFprintf(fd,"asin(");
    fprintTree(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case ACOS:
    sollyaFprintf(fd,"acos(");
    fprintTree(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case ATAN:
    sollyaFprintf(fd,"atan(");
    fprintTree(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case SINH:
    sollyaFprintf(fd,"sinh(");
    fprintTree(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case COSH:
    sollyaFprintf(fd,"cosh(");
    fprintTree(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case TANH:
    sollyaFprintf(fd,"tanh(");
    fprintTree(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case ASINH:
    sollyaFprintf(fd,"asinh(");
    fprintTree(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case ACOSH:
    sollyaFprintf(fd,"acosh(");
    fprintTree(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case ATANH:
    sollyaFprintf(fd,"atanh(");
    fprintTree(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case POW:
    if (isInfix(tree->child1)) 
      sollyaFprintf(fd,"(");
    fprintTree(fd,tree->child1);
    if (isInfix(tree->child1)) 
      sollyaFprintf(fd,")");
    sollyaFprintf(fd,"^(");
    fprintTree(fd,tree->child2);
    sollyaFprintf(fd,")");
    break;
  case NEG:
    sollyaFprintf(fd,"-");
    if (isInfix(tree->child1)) 
      sollyaFprintf(fd,"(");
    fprintTree(fd,tree->child1);
    if (isInfix(tree->child1)) 
      sollyaFprintf(fd,")");
    break;
  case ABS:
    sollyaFprintf(fd,"abs(");
    fprintTree(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case DOUBLE:
    sollyaFprintf(fd,"double(");
    fprintTree(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case SINGLE:
    sollyaFprintf(fd,"single(");
    fprintTree(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case QUAD:
    sollyaFprintf(fd,"quad(");
    fprintTree(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case HALFPRECISION:
    sollyaFprintf(fd,"halfprecision(");
    fprintTree(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case DOUBLEDOUBLE:
    sollyaFprintf(fd,"doubledouble(");
    fprintTree(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case TRIPLEDOUBLE:
    sollyaFprintf(fd,"tripledouble(");
    fprintTree(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case ERF: 
    sollyaFprintf(fd,"erf(");
    fprintTree(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case ERFC:
    sollyaFprintf(fd,"erfc(");
    fprintTree(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case LOG_1P:
    sollyaFprintf(fd,"log1p(");
    fprintTree(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case EXP_M1:
    sollyaFprintf(fd,"expm1(");
    fprintTree(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case DOUBLEEXTENDED:
    sollyaFprintf(fd,"doubleextended(");
    fprintTree(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
    case LIBRARYFUNCTION:
    {
      if (tree->child1->nodeType == VARIABLE) {
	for (i=1;i<=tree->libFunDeriv;i++) {
	  sollyaFprintf(fd,"diff(");
	}
	sollyaFprintf(fd,"%s",tree->libFun->functionName);
	for (i=1;i<=tree->libFunDeriv;i++) {
	  sollyaFprintf(fd,")");
	}
      } else {
	if (tree->libFunDeriv == 0) {
	  sollyaFprintf(fd,"%s(",tree->libFun->functionName);
	  fprintTree(fd,tree->child1);
	  sollyaFprintf(fd,")");
	} else {
	  sollyaFprintf(fd,"(");
	  for (i=1;i<=tree->libFunDeriv;i++) {
	    sollyaFprintf(fd,"diff(");
	  }
	  sollyaFprintf(fd,"%s",tree->libFun->functionName);
	  for (i=1;i<=tree->libFunDeriv;i++) {
	    sollyaFprintf(fd,")");
	  }
	  sollyaFprintf(fd,")(");
	  fprintTree(fd,tree->child1);
	  sollyaFprintf(fd,")");
	}
      }
    }
    break;
  case PROCEDUREFUNCTION:
    {
      if (tree->child1->nodeType == VARIABLE) {
	for (i=1;i<=tree->libFunDeriv;i++) {
	  sollyaFprintf(fd,"diff(");
	}
	sollyaFprintf(fd,"function(");
	fPrintThing(fd,tree->child2);
	sollyaFprintf(fd,")");
	for (i=1;i<=tree->libFunDeriv;i++) {
	  sollyaFprintf(fd,")");
	}
      } else {
	if (tree->libFunDeriv == 0) {
	  sollyaFprintf(fd,"(function(");
	  fPrintThing(fd,tree->child2);
	  sollyaFprintf(fd,"))(");
	  fprintTree(fd,tree->child1);
	  sollyaFprintf(fd,")");
	} else {
	  sollyaFprintf(fd,"(");
	  for (i=1;i<=tree->libFunDeriv;i++) {
	    sollyaFprintf(fd,"diff(");
	  }
	  sollyaFprintf(fd,"function(");
	  fPrintThing(fd,tree->child2);
	  sollyaFprintf(fd,")");
	  for (i=1;i<=tree->libFunDeriv;i++) {
	    sollyaFprintf(fd,")");
	  }
	  sollyaFprintf(fd,")(");
	  fprintTree(fd,tree->child1);
	  sollyaFprintf(fd,")");
	}
      }
    }
    break;
  case CEIL:
    sollyaFprintf(fd,"ceil(");
    fprintTree(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case FLOOR:
    sollyaFprintf(fd,"floor(");
    fprintTree(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case NEARESTINT:
    sollyaFprintf(fd,"nearestint(");
    fprintTree(fd,tree->child1);
    sollyaFprintf(fd,")");
    break;
  case PI_CONST:
    sollyaFprintf(fd,"pi");
    break;
  case LIBRARYCONSTANT:
    sollyaFprintf(fd,"%s",tree->libFun->functionName);
    break;
  default:
   sollyaFprintf(stderr,"Error: fprintTree: unknown identifier in the tree\n");
   exit(1);
  }
  return;
}




node* copyTree(node *tree) {
  node *copy;
  mpfr_t *value;
  mp_prec_t prec, p;
  mpfr_t temp;

  switch (tree->nodeType) {
  case VARIABLE:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = VARIABLE;
    break;
  case CONSTANT:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = CONSTANT;
    value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
    prec = tools_precision;
    p = mpfr_get_prec(*(tree->value));
    if (p > prec) prec = p;
    mpfr_init2(temp,prec);
    simplifyMpfrPrec(temp,*(tree->value));
    mpfr_init2(*value,mpfr_get_prec(temp));
    mpfr_set(*value,temp,GMP_RNDN);
    mpfr_clear(temp);
    copy->value = value;
    break;
  case ADD:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ADD;
    copy->child1 = copyTree(tree->child1);
    copy->child2 = copyTree(tree->child2);
    break;
  case SUB:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = SUB;
    copy->child1 = copyTree(tree->child1);
    copy->child2 = copyTree(tree->child2);
    break;
  case MUL:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = MUL;
    copy->child1 = copyTree(tree->child1);
    copy->child2 = copyTree(tree->child2);
    break;
  case DIV:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = DIV;
    copy->child1 = copyTree(tree->child1);
    copy->child2 = copyTree(tree->child2);
    break;
  case SQRT:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = SQRT;
    copy->child1 = copyTree(tree->child1);
    break;
  case EXP:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = EXP;
    copy->child1 = copyTree(tree->child1);
    break;
  case LOG:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = LOG;
    copy->child1 = copyTree(tree->child1);
    break;
  case LOG_2:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = LOG_2;
    copy->child1 = copyTree(tree->child1);
    break;
  case LOG_10:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = LOG_10;
    copy->child1 = copyTree(tree->child1);
    break;
  case SIN:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = SIN;
    copy->child1 = copyTree(tree->child1);
    break;
  case COS:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = COS;
    copy->child1 = copyTree(tree->child1);
    break;
  case TAN:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = TAN;
    copy->child1 = copyTree(tree->child1);
    break;
  case ASIN:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ASIN;
    copy->child1 = copyTree(tree->child1);
    break;
  case ACOS:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ACOS;
    copy->child1 = copyTree(tree->child1);
    break;
  case ATAN:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ATAN;
    copy->child1 = copyTree(tree->child1);
    break;
  case SINH:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = SINH;
    copy->child1 = copyTree(tree->child1);
    break;
  case COSH:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = COSH;
    copy->child1 = copyTree(tree->child1);
    break;
  case TANH:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = TANH;
    copy->child1 = copyTree(tree->child1);
    break;
  case ASINH:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ASINH;
    copy->child1 = copyTree(tree->child1);
    break;
  case ACOSH:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ACOSH;
    copy->child1 = copyTree(tree->child1);
    break;
  case ATANH:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ATANH;
    copy->child1 = copyTree(tree->child1);
    break;
  case POW:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = POW;
    copy->child1 = copyTree(tree->child1);
    copy->child2 = copyTree(tree->child2);
    break;
  case NEG:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = NEG;
    copy->child1 = copyTree(tree->child1);
    break;
  case ABS:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ABS;
    copy->child1 = copyTree(tree->child1);
    break;
  case DOUBLE:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = DOUBLE;
    copy->child1 = copyTree(tree->child1);
    break;
  case SINGLE:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = SINGLE;
    copy->child1 = copyTree(tree->child1);
    break;
  case QUAD:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = QUAD;
    copy->child1 = copyTree(tree->child1);
    break;
  case HALFPRECISION:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = HALFPRECISION;
    copy->child1 = copyTree(tree->child1);
    break;
  case DOUBLEDOUBLE:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = DOUBLEDOUBLE;
    copy->child1 = copyTree(tree->child1);
    break;
  case TRIPLEDOUBLE:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = TRIPLEDOUBLE;
    copy->child1 = copyTree(tree->child1);
    break;
  case ERF: 
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ERF;
    copy->child1 = copyTree(tree->child1);
    break;
  case ERFC:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ERFC;
    copy->child1 = copyTree(tree->child1);
    break;
  case LOG_1P:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = LOG_1P;
    copy->child1 = copyTree(tree->child1);
    break;
  case EXP_M1:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = EXP_M1;
    copy->child1 = copyTree(tree->child1);
    break;
  case DOUBLEEXTENDED:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = DOUBLEEXTENDED;
    copy->child1 = copyTree(tree->child1);
    break;
  case LIBRARYFUNCTION:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = LIBRARYFUNCTION;
    copy->libFun = tree->libFun;
    copy->libFunDeriv = tree->libFunDeriv;
    copy->child1 = copyTree(tree->child1);
    break;
  case PROCEDUREFUNCTION:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = PROCEDUREFUNCTION;
    copy->libFunDeriv = tree->libFunDeriv;
    copy->child1 = copyTree(tree->child1);
    copy->child2 = copyThing(tree->child2);
    break;
  case CEIL:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = CEIL;
    copy->child1 = copyTree(tree->child1);
    break;
  case FLOOR:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = FLOOR;
    copy->child1 = copyTree(tree->child1);
    break;
  case NEARESTINT:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = NEARESTINT;
    copy->child1 = copyTree(tree->child1);
    break;
  case PI_CONST:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = PI_CONST;
    break;
  case LIBRARYCONSTANT:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = LIBRARYCONSTANT;
    copy->libFun = tree->libFun;
    break;
  default:
   sollyaFprintf(stderr,"Error: copyTree: unknown identifier in the tree\n");
   exit(1);
  }
  return copy;
}

int mpfr_nearestint(mpfr_t rop, mpfr_t op) {
  return mpfr_rint(rop,op,GMP_RNDN);
}

int mpfr_to_mpq( mpq_t y, mpfr_t x){
  mpz_t mant;
  mp_exp_t expo;
  mpq_t aux;
  if (mpfr_number_p(x)) {
    mpz_init(mant);
    expo = mpfr_get_z_exp(mant,x);
    mpq_init(aux);    
    mpq_set_z(aux,mant);
    
    if (expo>=0)
      mpq_mul_2exp(aux,aux,(unsigned int)expo);
    else
      mpq_div_2exp(aux,aux,(unsigned int)(-expo));
    mpq_set(y,aux);
    mpq_clear(aux);
    mpz_clear(mant);
    return 1;
  }
  else return 0;
}

mp_prec_t getMpzPrecision(mpz_t x) {
  mp_prec_t prec;
  int p, dyadicValue;

  prec = mpz_sizeinbase(x, 2);
  dyadicValue = mpz_scan1(x, 0);
  p = prec - dyadicValue;
  if (p < 12) prec = 12; else prec = p; 

  return prec;
}


int tryEvaluateConstantTermToMpq(mpq_t res, node *tree) {
  mpq_t resA, resB, resC;
  int result;
  mpz_t num, denom;
  mpz_t num2, denom2;
  signed long int expo;

  if (tree == NULL) return 0;

  mpq_init(resA);
  mpq_init(resB);
  mpq_init(resC);

  result = 1;
  switch (tree->nodeType) {
  case CONSTANT:
    if (mpfr_number_p(*(tree->value))) {
      mpfr_to_mpq(resC,*(tree->value));
    } else result = 0;
    break;
  case ADD:
    if (tryEvaluateConstantTermToMpq(resA, tree->child1) && 
	tryEvaluateConstantTermToMpq(resB, tree->child2)) {
      mpq_add(resC,resA,resB);
    } else result = 0;
    break;
  case SUB:
    if (tryEvaluateConstantTermToMpq(resA, tree->child1) && 
	tryEvaluateConstantTermToMpq(resB, tree->child2)) {
      mpq_sub(resC,resA,resB);
    } else result = 0;
    break;
  case MUL:
    if (tryEvaluateConstantTermToMpq(resA, tree->child1) && 
	tryEvaluateConstantTermToMpq(resB, tree->child2)) {
      mpq_mul(resC,resA,resB);
    } else result = 0;
    break;
  case DIV:
    if (tryEvaluateConstantTermToMpq(resA, tree->child1) && 
	tryEvaluateConstantTermToMpq(resB, tree->child2) && 
	(mpq_sgn(resB) != 0)) {
      mpq_div(resC,resA,resB);
    } else result = 0;
    break;
  case SQRT:
    if (tryEvaluateConstantTermToMpq(resA, tree->child1)) {
      if (mpq_sgn(resA) >= 0) {
	mpz_init(num);
	mpz_init(denom);
	mpq_get_num(num,resA);
	mpq_get_den(denom,resA);
	if (mpz_root(num,num,2) && mpz_root(denom,denom,2)) {
	  mpq_set_num(resC,num);
	  mpq_set_den(resC,denom);
	  mpq_canonicalize(resC);
	} else {
	  result = 0;
	}
	mpz_clear(num);
	mpz_clear(denom);
      } else result = 0;
    } else result = 0;
    break;
  case POW:
    if (tryEvaluateConstantTermToMpq(resA, tree->child1) && 
	tryEvaluateConstantTermToMpq(resB, tree->child2)) {
      mpz_init(num);
      mpz_init(denom);
      mpq_get_num(num,resB);
      mpq_get_den(denom,resB);
      if (mpz_cmp_ui(denom,1) == 0) {
	/* Must set resC to resA^num */
	mpz_init(num2);
	mpz_init(denom2);
	if (mpz_sgn(num) < 0) {
	  mpq_get_num(denom2,resA);
	  mpq_get_den(num2,resA);
	  mpz_neg(num,num);
	} else {
	  mpq_get_num(num2,resA);
	  mpq_get_den(denom2,resA);
	}
	/* Must set resC to (num2^num)/(denom2^num), num is positive */
	if ((mpz_sgn(denom2) != 0) &&
	    mpz_fits_slong_p(num)) {
	  expo = mpz_get_si(num);
	  /* Must set resC to (num2^expo)/(denom2^expo), expo is positive */
	  mpz_pow_ui(num2, num2, (unsigned long int) expo);
	  mpz_pow_ui(denom2, denom2, (unsigned long int) expo);
	  mpq_set_num(resC,num2);
	  mpq_set_den(resC,denom2);
	  mpq_canonicalize(resC);
	} else result = 0;
	mpz_clear(num2);
	mpz_clear(denom2);
      } else result = 0;
      mpz_clear(num);
      mpz_clear(denom);
    } else result = 0;
    break;
  case NEG:
    if (tryEvaluateConstantTermToMpq(resA, tree->child1)) {
      mpq_neg(resC,resA);
    } else result = 0;
    break;
  default:
    result = 0;
  }

  if (result) mpq_set(res,resC);

  mpq_clear(resA);
  mpq_clear(resB);
  mpq_clear(resC);

  return result;
}


node *dividePolynomialByPowerOfVariableUnsafe(node *tree, int alpha);

int containsNotANumbers(node * tree) {
  int numberChilds;

  if (tree->nodeType == CONSTANT) {
    if (mpfr_nan_p(*(tree->value))) 
      return 1;
    else
      return 0;
  }

  if (tree->nodeType == VARIABLE) {
    return 0;
  }

  numberChilds = arity(tree);
  switch (numberChilds) {
  case 0:
    return 0;
    break;
  case 1:
    return containsNotANumbers(tree->child1);
    break;
  case 2:
    return (containsNotANumbers(tree->child1) ||
	    containsNotANumbers(tree->child2));
    break;
  default:
    sollyaFprintf(stderr,"Error: containsNotANumbers: unknown arity of tree node symbol.\n");
    exit(1);
  }

  return 1;
}

node* simplifyTreeErrorfreeInner(node *tree, int rec, int doRational) {
  node *simplChild1, *simplChild2, *simplified, *recsimplified;
  mpfr_t *value;
  mpfr_t temp;
  mp_prec_t prec, p;
  int alpha, beta;
  node *temp1, *temp2, *temp3, *temp4;
  rangetype xrange, yrange;
  mpq_t resMpq;
  mpfr_t num, denom, resDiv, resA, resB;
  int numberChilds;

  if ((tree->nodeType == CONSTANT) && (mpfr_nan_p(*(tree->value)))) return copyTree(tree);
  if (tree->nodeType != VARIABLE) {
    numberChilds = arity(tree);
    switch (numberChilds) {
    case 0:
      break;
    case 1:
      if ((tree->child1->nodeType == CONSTANT) && (mpfr_nan_p(*(tree->child1->value)))) return copyTree(tree->child1);
      break;
    case 2:
      if ((tree->child1->nodeType == CONSTANT) && (mpfr_nan_p(*(tree->child1->value)))) {
	if (isConstant(tree)) return copyTree(tree->child1);
	return copyTree(tree);
      }
      if ((tree->child2->nodeType == CONSTANT) && (mpfr_nan_p(*(tree->child2->value)))) {
	if (isConstant(tree)) return copyTree(tree->child2);
	return copyTree(tree);
      }
      break;
    default:
      sollyaFprintf(stderr,"Error: simplifyTreeErrorfreeInner: unknown arity of tree node symbol.\n");
      exit(1);
    }
  }
  
  if (doRational && isConstant(tree) && (tree->nodeType != CONSTANT)) {
    mpq_init(resMpq);
    if (tryEvaluateConstantTermToMpq(resMpq, tree)) {
      mpfr_init2(num,getMpzPrecision(mpq_numref(resMpq)));
      mpfr_init2(denom,getMpzPrecision(mpq_denref(resMpq)));
      mpfr_set_z(num,mpq_numref(resMpq),GMP_RNDN); /* exact */
      mpfr_set_z(denom,mpq_denref(resMpq),GMP_RNDN); /* exact */
      mpfr_init2(resDiv,defaultprecision);
      if ((mpfr_div(resDiv,num,denom,GMP_RNDN) == 0) &&  
	  mpfr_number_p(resDiv)) {
	mpfr_init2(resA,mpfr_get_prec(resDiv)+10);
	mpfr_set(resA,resDiv,GMP_RNDN); /* exact */
	simplifyMpfrPrec(resA, resDiv);
	simplified = makeConstant(resA);
	mpfr_clear(resA);
      } else {
	mpfr_init2(resA,mpfr_get_prec(num)+10);
	mpfr_set(resA,num,GMP_RNDN); /* exact */
	simplifyMpfrPrec(resA, num);
	mpfr_init2(resB,mpfr_get_prec(denom)+10);
	mpfr_set(resB,denom,GMP_RNDN); /* exact */
	simplifyMpfrPrec(resB, denom);
	simplified = makeDiv(makeConstant(resA),makeConstant(resB));
	mpfr_clear(resA);
	mpfr_clear(resB);
      }
      
      mpfr_clear(num);
      mpfr_clear(denom);
      mpfr_clear(resDiv);
      mpq_clear(resMpq);
      return simplified;
    }
    mpq_clear(resMpq);
  }

  if ((tree->nodeType == DIV) && 
      (!containsNotANumbers(tree)) &&
      (isPolynomial(tree->child1)) &&
      (isPolynomial(tree->child1)) &&
      ((alpha = getMaxPowerDivider(tree->child1)) > 0) && 
      ((beta = getMaxPowerDivider(tree->child2)) > 0)) {
    if (alpha == beta) {
      temp1 = dividePolynomialByPowerOfVariableUnsafe(tree->child1, alpha);
      temp2 = dividePolynomialByPowerOfVariableUnsafe(tree->child2, alpha);
      temp3 = (node *) safeMalloc(sizeof(node));
      temp3->nodeType = DIV;
      temp3->child1 = temp1;
      temp3->child2 = temp2;
      temp4 = simplifyTreeErrorfreeInner(temp3,rec,doRational);
      free_memory(temp3);
      return temp4;
    } else {
      temp1 = (node *) safeMalloc(sizeof(node));
      temp1->nodeType = DIV;
      temp1->child1 = copyTree(tree->child1);
      temp2 = (node *) safeMalloc(sizeof(node));
      temp1->child2 = temp2;
      temp2->nodeType = POW;
      temp2->child1 = (node *) safeMalloc(sizeof(node));
      temp2->child1->nodeType = VARIABLE;
      temp2->child2 = (node *) safeMalloc(sizeof(node));
      temp2->child2->nodeType = CONSTANT;
      temp2->child2->value = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*(temp2->child2->value),8 * sizeof(int) + 10);
      mpfr_set_si(*(temp2->child2->value),alpha,GMP_RNDN);
      temp3 = simplifyTreeErrorfreeInner(temp1, rec, doRational);
      free_memory(temp1);
      temp1 = (node *) safeMalloc(sizeof(node));
      temp1->nodeType = DIV;
      temp1->child1 = copyTree(tree->child2);
      temp2 = (node *) safeMalloc(sizeof(node));
      temp1->child2 = temp2;
      temp2->nodeType = POW;
      temp2->child1 = (node *) safeMalloc(sizeof(node));
      temp2->child1->nodeType = VARIABLE;
      temp2->child2 = (node *) safeMalloc(sizeof(node));
      temp2->child2->nodeType = CONSTANT;
      temp2->child2->value = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*(temp2->child2->value),8 * sizeof(int) + 10);
      mpfr_set_si(*(temp2->child2->value),beta,GMP_RNDN);
      temp4 = simplifyTreeErrorfreeInner(temp1, rec, doRational);
      free_memory(temp1);
      temp1 = (node *) safeMalloc(sizeof(node));
      temp1->nodeType = DIV;
      temp1->child1 = temp3;
      temp1->child2 = temp4;
      temp2 = simplifyTreeErrorfreeInner(temp1, rec, doRational);
      free_memory(temp1);
      if (alpha > beta) {
	temp1 = (node *) safeMalloc(sizeof(node));
	temp1->nodeType = POW;
	temp1->child1 = (node *) safeMalloc(sizeof(node));
	temp1->child1->nodeType = VARIABLE;
	temp1->child2 = (node *) safeMalloc(sizeof(node));
	temp1->child2->nodeType = CONSTANT;
	temp1->child2->value = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*(temp1->child2->value),8 * sizeof(int) + 10);
	mpfr_set_si(*(temp1->child2->value),alpha - beta,GMP_RNDN);
      } else {
	temp3 = (node *) safeMalloc(sizeof(node));
	temp3->nodeType = POW;
	temp3->child1 = (node *) safeMalloc(sizeof(node));
	temp3->child1->nodeType = VARIABLE;
	temp3->child2 = (node *) safeMalloc(sizeof(node));
	temp3->child2->nodeType = CONSTANT;
	temp3->child2->value = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*(temp3->child2->value),8 * sizeof(int) + 10);
	mpfr_set_si(*(temp3->child2->value),beta - alpha,GMP_RNDN);
	temp1 = (node *) safeMalloc(sizeof(node));
	temp1->nodeType = DIV;
	temp1->child2 = temp3;
	temp1->child1 = (node *) safeMalloc(sizeof(node));
	temp1->child1->nodeType = CONSTANT;
	temp1->child1->value = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*(temp1->child1->value),12);
	mpfr_set_si(*(temp1->child1->value),1,GMP_RNDN);
      }
      temp4 = (node *) safeMalloc(sizeof(node));
      temp4->nodeType = MUL;
      temp4->child1 = temp2;
      temp4->child2 = simplifyTreeErrorfreeInner(temp1,rec, doRational);
      free_memory(temp1);
      return temp4;
    }
  }

  switch (tree->nodeType) {
  case VARIABLE:
    simplified = (node*) safeMalloc(sizeof(node));
    simplified->nodeType = VARIABLE;
    break;
  case CONSTANT:
    simplified = (node*) safeMalloc(sizeof(node));
    simplified->nodeType = CONSTANT;
    value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
    mpfr_init2(temp,mpfr_get_prec(*(tree->value)) + 10);
    simplifyMpfrPrec(temp, *(tree->value));
    mpfr_init2(*value,mpfr_get_prec(temp));
    mpfr_set(*value,temp,GMP_RNDN);
    mpfr_clear(temp);
    simplified->value = value;
    break;
  case ADD:
    simplChild1 = simplifyTreeErrorfreeInner(tree->child1,rec, doRational);
    simplChild2 = simplifyTreeErrorfreeInner(tree->child2,rec, doRational);
    simplified = (node*) safeMalloc(sizeof(node));
    if ((simplChild1->nodeType == CONSTANT) && (simplChild2->nodeType == CONSTANT)) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      prec = 2 * tools_precision;
      p = 2 * mpfr_get_prec(*(simplChild1->value));
      if (p > prec) prec = p;
      p = 2 * mpfr_get_prec(*(simplChild2->value));
      if (p > prec) prec = p;
      prec += 10;
      if (prec > 256 * tools_precision) prec = 256 * tools_precision;
      mpfr_init2(*value,prec);
      simplified->value = value;
      if ((mpfr_add(*value, *(simplChild1->value), *(simplChild2->value), GMP_RNDN) != 0) || 
	  (!mpfr_number_p(*value))) {
	simplified->nodeType = ADD;
	simplified->child1 = simplChild1;
	simplified->child2 = simplChild2;
	mpfr_clear(*value);
	free(value);
      } else {
	free_memory(simplChild1);
	free_memory(simplChild2);
      }
    } else {
      if ((simplChild1->nodeType == CONSTANT) && (mpfr_zero_p(*(simplChild1->value)))) {
	free_memory(simplChild1);
	free(simplified);
	simplified = simplChild2;
      } else {
	if ((simplChild2->nodeType == CONSTANT) && (mpfr_zero_p(*(simplChild2->value)))) {
	  free_memory(simplChild2);
	  free(simplified);
	  simplified = simplChild1;
	} else {
	  if (simplChild1->nodeType == NEG) {
	    simplified->nodeType = SUB;
	    simplified->child1 = simplChild2;
	    simplified->child2 = copyTree(simplChild1->child1);
	    free_memory(simplChild1);
	    if (rec > 0) {
	      recsimplified = simplifyTreeErrorfreeInner(simplified,rec-1, doRational);
	      free_memory(simplified);
	      simplified = recsimplified;
	    }
	  } else {
	    if (simplChild2->nodeType == NEG) {
	      simplified->nodeType = SUB;
	      simplified->child1 = simplChild1;
	      simplified->child2 = copyTree(simplChild2->child1);
	      free_memory(simplChild2);
	      if (rec > 0) {
		recsimplified = simplifyTreeErrorfreeInner(simplified,rec-1, doRational);
		free_memory(simplified);
		simplified = recsimplified;
	      }
	    } else {
	      if (isSyntacticallyEqual(simplChild1,simplChild2)) {
		simplified->nodeType = MUL;
		simplified->child1 = (node *) safeMalloc(sizeof(node));
		simplified->child1->nodeType = CONSTANT;
		simplified->child1->value = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
		mpfr_init2(*(simplified->child1->value),tools_precision);
		mpfr_set_d(*(simplified->child1->value),2.0,GMP_RNDN);
		simplified->child2 = simplChild1;
		free_memory(simplChild2);
		if (rec > 0) {
		  recsimplified = simplifyTreeErrorfreeInner(simplified,rec-1, doRational);
		  free_memory(simplified);
		  simplified = recsimplified;
		}
	      } else {
		simplified->nodeType = ADD;
		simplified->child1 = simplChild1;
		simplified->child2 = simplChild2;
	      }
	    }
	  }
	}
      }
    }
    break;
  case SUB:
    simplChild1 = simplifyTreeErrorfreeInner(tree->child1,rec, doRational);
    simplChild2 = simplifyTreeErrorfreeInner(tree->child2,rec, doRational);
    simplified = (node*) safeMalloc(sizeof(node));
    if ((simplChild1->nodeType == CONSTANT) && (simplChild2->nodeType == CONSTANT)) {
      simplified->nodeType = CONSTANT;
      prec = 2 * tools_precision;
      p = 2 * mpfr_get_prec(*(simplChild1->value));
      if (p > prec) prec = p;
      p = 2 * mpfr_get_prec(*(simplChild2->value));
      if (p > prec) prec = p;
      prec += 10;
      if (prec > 256 * tools_precision) prec = 256 * tools_precision;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,prec);
      simplified->value = value;
      if ((mpfr_sub(*value, *(simplChild1->value), *(simplChild2->value), GMP_RNDN) != 0) || 
	  (!mpfr_number_p(*value))) {
	simplified->nodeType = SUB;
	simplified->child1 = simplChild1;
	simplified->child2 = simplChild2;
	mpfr_clear(*value);
	free(value);
      } else {
	free_memory(simplChild1);
	free_memory(simplChild2);
      }
    } else {
      if ((simplChild1->nodeType == CONSTANT) && (mpfr_zero_p(*(simplChild1->value)))) {
	free_memory(simplChild1);
	simplified->nodeType = NEG;
	simplified->child1 = simplChild2;
      } else {
	if ((simplChild2->nodeType == CONSTANT) && (mpfr_zero_p(*(simplChild2->value)))) {
	  free_memory(simplChild2);
	  free(simplified);
	  simplified = simplChild1;
	} else {
	  if (isSyntacticallyEqual(simplChild1,simplChild2)) {
	    free_memory(simplChild1);
	    free_memory(simplChild2);
	    simplified->nodeType = CONSTANT;
	    value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	    mpfr_init2(*value,tools_precision);
	    simplified->value = value;
	    mpfr_set_d(*value,0.0,GMP_RNDN);
	  } else {
	    if (simplChild2->nodeType == NEG) {
	      simplified->nodeType = ADD;
	      simplified->child1 = simplChild1;
	      simplified->child2 = copyTree(simplChild2->child1);
	      free_memory(simplChild2);
	    } else {
	      if ((simplChild1->nodeType == EXP) &&
		  (simplChild2->nodeType == CONSTANT) &&
		  (mpfr_cmp_d(*(simplChild2->value),1.0) == 0) &&
		  (!mpfr_nan_p(*(simplChild2->value)))) {
		simplified->nodeType = EXP_M1;
		simplified->child1 = copyTree(simplChild1->child1);
		free_memory(simplChild1);
		free_memory(simplChild2);
	      } else {
		simplified->nodeType = SUB;
		simplified->child1 = simplChild1;
		simplified->child2 = simplChild2;
	      }
	    }
	  }
	}
      }
    }
    break;
  case MUL:
    simplChild1 = simplifyTreeErrorfreeInner(tree->child1,rec, doRational);
    simplChild2 = simplifyTreeErrorfreeInner(tree->child2,rec, doRational);
    simplified = (node*) safeMalloc(sizeof(node));
    if ((simplChild1->nodeType == CONSTANT) && (simplChild2->nodeType == CONSTANT)) {
      simplified->nodeType = CONSTANT;
      prec = 2 * tools_precision;
      p = 2 * mpfr_get_prec(*(simplChild1->value));
      if (p > prec) prec = p;
      p = 2 * mpfr_get_prec(*(simplChild2->value));
      if (p > prec) prec = p;
      prec += 10;
      if (prec > 256 * tools_precision) prec = 256 * tools_precision;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,prec);
      simplified->value = value;
      if ((mpfr_mul(*value, *(simplChild1->value), *(simplChild2->value), GMP_RNDN) != 0) || 
	  (!mpfr_number_p(*value))) {
	simplified->nodeType = MUL;
	simplified->child1 = simplChild1;
	simplified->child2 = simplChild2;
	mpfr_clear(*value);
	free(value);
      } else {
	free_memory(simplChild1);
	free_memory(simplChild2);
      }
    } else {
      if (((simplChild1->nodeType == CONSTANT) && (mpfr_zero_p(*(simplChild1->value)))) ||
	  ((simplChild2->nodeType == CONSTANT) && (mpfr_zero_p(*(simplChild2->value))))) {
	free_memory(simplChild1);
	free_memory(simplChild2);
	simplified->nodeType = CONSTANT;
	value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*value,tools_precision);
	simplified->value = value;
	mpfr_set_d(*value,0.0,GMP_RNDN);
      } else {
	if ((simplChild1->nodeType == CONSTANT) && (mpfr_cmp_d(*(simplChild1->value),1.0) == 0) && (!mpfr_nan_p(*(simplChild1->value)))) {
	  free_memory(simplChild1);
	  free(simplified);
	  simplified = simplChild2;
	} else {
	  if ((simplChild2->nodeType == CONSTANT) && (mpfr_cmp_d(*(simplChild2->value),1.0) == 0) && (!mpfr_nan_p(*(simplChild2->value)))) {
	    free_memory(simplChild2);
	    free(simplified);
	    simplified = simplChild1;
	  } else {
	    if ((simplChild1->nodeType == DIV) &&
		(simplChild1->child1->nodeType == CONSTANT) &&
		(mpfr_cmp_d(*(simplChild1->child1->value),1.0) == 0) &&
		(!mpfr_nan_p(*(simplChild1->child1->value)))) {
	      simplified->nodeType = DIV;
	      simplified->child1 = simplChild2;
	      simplified->child2 = copyTree(simplChild1->child2);
	      free_memory(simplChild1);
	    } else {
	      if ((simplChild2->nodeType == DIV) &&
		  (simplChild2->child1->nodeType == CONSTANT) &&
		  (mpfr_cmp_d(*(simplChild2->child1->value),1.0) == 0) &&
		  (!mpfr_nan_p(*(simplChild2->child1->value)))) {
		simplified->nodeType = DIV;
		simplified->child1 = simplChild1;
		simplified->child2 = copyTree(simplChild2->child2);
		free_memory(simplChild2);
	      } else {
		if ((simplChild1->nodeType == NEG) &&
		    (simplChild2->nodeType == NEG)) {
		  simplified->nodeType = MUL;
		  simplified->child1 = copyTree(simplChild1->child1);
		  simplified->child2 = copyTree(simplChild2->child1);
		  free_memory(simplChild1);
		  free_memory(simplChild2);
		} else {
		  if (simplChild1->nodeType == NEG) {
		    simplified->nodeType = NEG;
		    simplified->child1 = (node *) safeMalloc(sizeof(node));
		    simplified->child1->nodeType = MUL;
		    simplified->child1->child1 = copyTree(simplChild1->child1);
		    simplified->child1->child2 = simplChild2;
		    free_memory(simplChild1);
		  } else {
		    if (simplChild2->nodeType == NEG) {
		      simplified->nodeType = NEG;
		      simplified->child1 = (node *) safeMalloc(sizeof(node));
		      simplified->child1->nodeType = MUL;
		      simplified->child1->child2 = copyTree(simplChild2->child1);
		      simplified->child1->child1 = simplChild1;
		      free_memory(simplChild2);
		    } else {
		      if ((simplChild1->nodeType == MUL) &&
			  (isConstant(simplChild2)) &&
			  (isConstant(simplChild1->child1))) {
			simplified->nodeType = MUL;
			simplified->child2 = simplChild1->child2;
			simplified->child1 = simplChild1;
			simplChild1->child2 = simplChild2;
			if (rec > 0) {
			  recsimplified = simplifyTreeErrorfreeInner(simplified,rec-1, doRational);
			  free_memory(simplified);
			  simplified = recsimplified;
			}
		      } else {
			if ((simplChild1->nodeType == MUL) &&
			    (isConstant(simplChild2)) &&
			    (isConstant(simplChild1->child2))) {
			  simplified->nodeType = MUL;
			  simplified->child2 = simplChild1->child1;
			  simplified->child1 = simplChild1;
			  simplChild1->child1 = simplChild2;
			  if (rec > 0) {
			    recsimplified = simplifyTreeErrorfreeInner(simplified,rec-1, doRational);
			    free_memory(simplified);
			    simplified = recsimplified;
			  }
			} else {
			  if ((simplChild2->nodeType == MUL) &&
			      (isConstant(simplChild1)) &&
			      (isConstant(simplChild2->child1))) {
			    simplified->nodeType = MUL;
			    simplified->child1 = simplChild2->child2;
			    simplified->child2 = simplChild2;
			    simplChild2->child2 = simplChild1;
			    if (rec > 0) {
			      recsimplified = simplifyTreeErrorfreeInner(simplified,rec-1, doRational);
			      free_memory(simplified);
			      simplified = recsimplified;
			    }
			  } else {
			    if ((simplChild2->nodeType == MUL) &&
				(isConstant(simplChild1)) &&
				(isConstant(simplChild2->child2))) {
			      simplified->nodeType = MUL;
			      simplified->child1 = simplChild2->child1;
			      simplified->child2 = simplChild2;
			      simplChild2->child1 = simplChild1;
			      if (rec > 0) {
				recsimplified = simplifyTreeErrorfreeInner(simplified,rec-1, doRational);
				free_memory(simplified);
				simplified = recsimplified;
			      }
			    } else {
			      if ((simplChild2->nodeType == DIV) && 
				  (isSyntacticallyEqual(simplChild1,simplChild2->child2))) {
				free(simplified);
				free_memory(simplChild1);
				free_memory(simplChild2->child2);
				simplified = simplChild2->child1;
				free(simplChild2);
			      } else {
				if ((simplChild1->nodeType == DIV) && 
				    (isSyntacticallyEqual(simplChild2,simplChild1->child2))) {
				  free(simplified);
				  free_memory(simplChild2);
				  free_memory(simplChild1->child2);
				  simplified = simplChild1->child1;
				  free(simplChild1);
				} else {
				  simplified->nodeType = MUL;
				  simplified->child1 = simplChild1;
				  simplified->child2 = simplChild2;
				}
			      }
			    }
			  }
			}
		      }
		    }
		  }
		}
	      }
	    }
	  }
	}
      }
    }
    break;
  case DIV:
    simplChild1 = simplifyTreeErrorfreeInner(tree->child1,rec, doRational);
    simplChild2 = simplifyTreeErrorfreeInner(tree->child2,rec, doRational);
    simplified = (node*) safeMalloc(sizeof(node));
    if ((simplChild1->nodeType == CONSTANT) && (simplChild2->nodeType == CONSTANT)) {
      simplified->nodeType = CONSTANT;
      prec = 2 * tools_precision;
      p = 2 * mpfr_get_prec(*(simplChild1->value));
      if (p > prec) prec = p;
      p = 2 * mpfr_get_prec(*(simplChild2->value));
      if (p > prec) prec = p;
      prec += 10;
      if (prec > 256 * tools_precision) prec = 256 * tools_precision;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,prec);
      simplified->value = value;
      if ((mpfr_div(*value, *(simplChild1->value), *(simplChild2->value), GMP_RNDN) != 0) || 
	  (!mpfr_number_p(*value))) {
	simplified->nodeType = DIV;
	simplified->child1 = simplChild1;
	simplified->child2 = simplChild2;
	mpfr_clear(*value);
	free(value);
      } else {
	free_memory(simplChild1);
	free_memory(simplChild2);
      }
    } else {
      if ((simplChild1->nodeType == CONSTANT) && (mpfr_zero_p(*(simplChild1->value)))) {
	free_memory(simplChild1);
	free_memory(simplChild2);
	simplified->nodeType = CONSTANT;
	value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*value,tools_precision);
	simplified->value = value;
	mpfr_set_d(*value,0.0,GMP_RNDN);
      } else {
	if ((simplChild2->nodeType == CONSTANT) && (mpfr_cmp_d(*(simplChild2->value),1.0) == 0) && (!mpfr_nan_p(*(simplChild2->value)))) {
	  free_memory(simplChild2);
	  free(simplified);
	  simplified = simplChild1;
	} else {
	  if (isSyntacticallyEqual(simplChild1,simplChild2)) {
	    simplified->nodeType = CONSTANT;
	    value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	    mpfr_init2(*value,tools_precision);
	    simplified->value = value;
	    mpfr_set_d(*value,1.0,GMP_RNDN);
	    free_memory(simplChild1);
	    free_memory(simplChild2);
	  } else {
	    if ((simplChild1->nodeType == NEG) &&
		(simplChild2->nodeType == NEG)) {
	      simplified->nodeType = DIV;
	      simplified->child1 = copyTree(simplChild1->child1);
	      simplified->child2 = copyTree(simplChild2->child1);
	      free_memory(simplChild1);
	      free_memory(simplChild2);
	    } else {
	      if (simplChild1->nodeType == NEG) {
		simplified->nodeType = NEG;
		simplified->child1 = (node *) safeMalloc(sizeof(node));
		simplified->child1->nodeType = DIV;
		simplified->child1->child1 = copyTree(simplChild1->child1);
		simplified->child1->child2 = simplChild2;
		free_memory(simplChild1);
	      } else {
		if (simplChild2->nodeType == NEG) {
		  simplified->nodeType = NEG;
		  simplified->child1 = (node *) safeMalloc(sizeof(node));
		  simplified->child1->nodeType = DIV;
		  simplified->child1->child2 = copyTree(simplChild2->child1);
		  simplified->child1->child1 = simplChild1;
		  free_memory(simplChild2);
		} else {
		  if (simplChild2->nodeType == DIV) {
		    simplified->nodeType = MUL;
		    simplified->child1 = simplChild1;
		    simplified->child2 = simplChild2->child2;
		    simplChild2->child2 = simplChild2->child1;
		    simplChild2->child1 = simplified->child2;
		    simplified->child2 = simplChild2;
		    if (rec > 0) {
		      recsimplified = simplifyTreeErrorfreeInner(simplified,rec, doRational);
		      free_memory(simplified);
		      simplified = recsimplified;
		    }
		  } else {
		    if (simplChild1->nodeType == DIV) {
		      simplified->nodeType = DIV;
		      simplified->child1 = simplChild1->child1;
		      simplified->child2 = simplChild1;
		      simplChild1->nodeType = MUL;
		      simplChild1->child1 = simplChild2;
		      if (rec > 0) {
			recsimplified = simplifyTreeErrorfreeInner(simplified,rec, doRational);
			free_memory(simplified);
			simplified = recsimplified;
		      }
		    } else {
		      if ((simplChild1->nodeType == SIN) &&
			  (simplChild2->nodeType == COS) &&
			  (isSyntacticallyEqual(simplChild1->child1,simplChild2->child1))) {
			simplified->nodeType = TAN;
			simplified->child1 = copyTree(simplChild1->child1);
			free_memory(simplChild1);
			free_memory(simplChild2);
		      } else {
			simplified->nodeType = DIV;
			simplified->child1 = simplChild1;
			simplified->child2 = simplChild2;
		      }
		    }
		  }
		}
	      }
	    }
	  }
	}
      }
    }
    break;
  case SQRT:
    simplChild1 = simplifyTreeErrorfreeInner(tree->child1,rec, doRational);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      prec = tools_precision;
      p = mpfr_get_prec(*(simplChild1->value));
      if (p > prec) prec = p;
      prec += 10;
      if (prec > 256 * tools_precision) prec = 256 * tools_precision;
      mpfr_init2(*value,prec);
      simplified->value = value;
      if ((mpfr_sqrt(*value, *(simplChild1->value), GMP_RNDN) != 0) || 
	  (!mpfr_number_p(*value))) {
	simplified->nodeType = SQRT;
	simplified->child1 = simplChild1;
	mpfr_clear(*value);
	free(value);
      } else {
	free_memory(simplChild1);
      }
    } else {
      if ((simplChild1->nodeType == POW) &&
	  (simplChild1->child2->nodeType == CONSTANT) &&
	  (mpfr_cmp_d(*(simplChild1->child2->value),2.0) == 0.0) && 
	  (!mpfr_nan_p(*(simplChild1->child2->value)))) {
	simplified->nodeType = ABS;
	simplified->child1 = copyTree(simplChild1->child1);
	free_memory(simplChild1);
      } else {
	simplified->nodeType = SQRT;
	simplified->child1 = simplChild1;
      }
    }
    break;
  case EXP:
    simplChild1 = simplifyTreeErrorfreeInner(tree->child1,rec, doRational);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      if ((mpfr_exp(*value, *(simplChild1->value), GMP_RNDN) != 0) || 
	  (!mpfr_number_p(*value))) {
	simplified->nodeType = EXP;
	simplified->child1 = simplChild1;
	mpfr_clear(*value);
	free(value);
      } else {
	free_memory(simplChild1);
      }
    } else {
      simplified->nodeType = EXP;
      simplified->child1 = simplChild1;
    }
    break;
  case LOG:
    simplChild1 = simplifyTreeErrorfreeInner(tree->child1,rec, doRational);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      if ((mpfr_log(*value, *(simplChild1->value), GMP_RNDN) != 0) || 
	  (!mpfr_number_p(*value))) {
	simplified->nodeType = LOG;
	simplified->child1 = simplChild1;
	mpfr_clear(*value);
	free(value);
      } else {
	free_memory(simplChild1);
      }
    } else {
      if ((simplChild1->nodeType == ADD) &&
	  (simplChild1->child1->nodeType == CONSTANT) &&
	  (mpfr_cmp_d(*(simplChild1->child1->value),1.0) == 0.0) &&
	  (!mpfr_nan_p(*(simplChild1->child1->value)))) {
	simplified->nodeType = LOG_1P;
	simplified->child1 = copyTree(simplChild1->child2);
	free_memory(simplChild1);
      } else {
	if ((simplChild1->nodeType == ADD) &&
	    (simplChild1->child2->nodeType == CONSTANT) &&
	    (mpfr_cmp_d(*(simplChild1->child2->value),1.0) == 0.0) &&
	    (!mpfr_nan_p(*(simplChild1->child2->value)))) {
	  simplified->nodeType = LOG_1P;
	  simplified->child1 = copyTree(simplChild1->child1);
	  free_memory(simplChild1);
	} else {
	  if (simplChild1->nodeType == EXP) {
	    free(simplified);
	    simplified = copyTree(simplChild1->child1);
	    free_memory(simplChild1);
	  } else {
	    simplified->nodeType = LOG;
	    simplified->child1 = simplChild1;
	  }
	}
      }
    }
    break;
  case LOG_2:
    simplChild1 = simplifyTreeErrorfreeInner(tree->child1,rec, doRational);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      if ((mpfr_log2(*value, *(simplChild1->value), GMP_RNDN) != 0) || 
	  (!mpfr_number_p(*value))) {
	simplified->nodeType = LOG_2;
	simplified->child1 = simplChild1;
	mpfr_clear(*value);
	free(value);
      } else {
	free_memory(simplChild1);
      }
    } else {
      simplified->nodeType = LOG_2;
      simplified->child1 = simplChild1;
    }
    break;
  case LOG_10:
    simplChild1 = simplifyTreeErrorfreeInner(tree->child1,rec, doRational);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      if ((mpfr_log10(*value, *(simplChild1->value), GMP_RNDN) != 0) || 
	  (!mpfr_number_p(*value))) {
	simplified->nodeType = LOG_10;
	simplified->child1 = simplChild1;
	mpfr_clear(*value);
	free(value);
      } else {
	free_memory(simplChild1);
      }
    } else {
      simplified->nodeType = LOG_10;
      simplified->child1 = simplChild1;
    }
    break;
  case SIN:
    simplChild1 = simplifyTreeErrorfreeInner(tree->child1,rec, doRational);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      if ((mpfr_sin(*value, *(simplChild1->value), GMP_RNDN) != 0) || 
	  (!mpfr_number_p(*value))) {
	simplified->nodeType = SIN;
	simplified->child1 = simplChild1;
	mpfr_clear(*value);
	free(value);
      } else {
	free_memory(simplChild1);
      }
    } else {
      simplified->nodeType = SIN;
      simplified->child1 = simplChild1;
    }
    break;
  case COS:
    simplChild1 = simplifyTreeErrorfreeInner(tree->child1,rec, doRational);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      if ((mpfr_cos(*value, *(simplChild1->value), GMP_RNDN) != 0) || 
	  (!mpfr_number_p(*value))) {
	simplified->nodeType = COS;
	simplified->child1 = simplChild1;
	mpfr_clear(*value);
	free(value);
      } else {
	free_memory(simplChild1);
      }
    } else {
      simplified->nodeType = COS;
      simplified->child1 = simplChild1;
    }
    break;
  case TAN:
    simplChild1 = simplifyTreeErrorfreeInner(tree->child1,rec, doRational);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      if ((mpfr_tan(*value, *(simplChild1->value), GMP_RNDN) != 0) || 
	  (!mpfr_number_p(*value))) {
	simplified->nodeType = TAN;
	simplified->child1 = simplChild1;
	mpfr_clear(*value);
	free(value);
      } else {
	free_memory(simplChild1);
      }
    } else {
      simplified->nodeType = TAN;
      simplified->child1 = simplChild1;
    }
    break;
  case ASIN:
    simplChild1 = simplifyTreeErrorfreeInner(tree->child1,rec, doRational);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      mpfr_init2(temp,53);
      mpfr_set_ui(temp,1,GMP_RNDN);
      if ((mpfr_cmp(temp, *(simplChild1->value)) == 0) && (!mpfr_nan_p(*(simplChild1->value)))) {
	mpfr_set_ui(*(simplChild1->value),2,GMP_RNDN);
	simplified->child2 = simplChild1;
	simplified->nodeType = DIV;
	simplified->child1 = (node *) safeMalloc(sizeof(node));
	simplified->child1->nodeType = PI_CONST;
      } else {
	mpfr_set_si(temp,-1,GMP_RNDN);
	if ((mpfr_cmp(temp, *(simplChild1->value)) == 0) && (!mpfr_nan_p(*(simplChild1->value)))) {
	  mpfr_set_si(*(simplChild1->value),-2,GMP_RNDN);
	  simplified->child2 = simplChild1;
	  simplified->nodeType = DIV;
	  simplified->child1 = (node *) safeMalloc(sizeof(node));
	  simplified->child1->nodeType = PI_CONST;
	} else {
	  simplified->nodeType = CONSTANT;
	  value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	  mpfr_init2(*value,tools_precision);
	  simplified->value = value;
	  if ((mpfr_asin(*value, *(simplChild1->value), GMP_RNDN) != 0) || 
	      (!mpfr_number_p(*value))) {
	    simplified->nodeType = ASIN;
	    simplified->child1 = simplChild1;
	    mpfr_clear(*value);
	    free(value);
	  } else {
	    free_memory(simplChild1);
	  }
	}
      }
      mpfr_clear(temp);
    } else {
      simplified->nodeType = ASIN;
      simplified->child1 = simplChild1;
    }
    break;
  case ACOS:
    simplChild1 = simplifyTreeErrorfreeInner(tree->child1,rec, doRational);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      mpfr_init2(temp,53);
      mpfr_set_si(temp,-1,GMP_RNDN);
      if ((mpfr_cmp(temp, *(simplChild1->value)) == 0) && (!mpfr_nan_p(*(simplChild1->value)))) {
	mpfr_set_ui(*(simplChild1->value),1,GMP_RNDN);
	simplified->child2 = simplChild1;
	simplified->nodeType = DIV;
	simplified->child1 = (node *) safeMalloc(sizeof(node));
	simplified->child1->nodeType = PI_CONST;
      } else {
	simplified->nodeType = CONSTANT;
	value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*value,tools_precision);
	simplified->value = value;
	if ((mpfr_acos(*value, *(simplChild1->value), GMP_RNDN) != 0) || 
	    (!mpfr_number_p(*value))) {
	  simplified->nodeType = ACOS;
	  simplified->child1 = simplChild1;
	  mpfr_clear(*value);
	  free(value);
	} else {
	  free_memory(simplChild1);
	}
      }
      mpfr_clear(temp);
    } else {
      simplified->nodeType = ACOS;
      simplified->child1 = simplChild1;
    }
    break;
  case ATAN:
    simplChild1 = simplifyTreeErrorfreeInner(tree->child1,rec, doRational);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      mpfr_init2(temp,53);
      mpfr_set_ui(temp,1,GMP_RNDN);
      if ((mpfr_cmp(temp, *(simplChild1->value)) == 0) && (!mpfr_nan_p(*(simplChild1->value)))) {
	mpfr_set_ui(*(simplChild1->value),4,GMP_RNDN);
	simplified->child2 = simplChild1;
	simplified->nodeType = DIV;
	simplified->child1 = (node *) safeMalloc(sizeof(node));
	simplified->child1->nodeType = PI_CONST;
      } else {
	mpfr_set_si(temp,-1,GMP_RNDN);
	if ((mpfr_cmp(temp, *(simplChild1->value)) == 0) && (!mpfr_nan_p(*(simplChild1->value)))) {
	  mpfr_set_si(*(simplChild1->value),-4,GMP_RNDN);
	  simplified->child2 = simplChild1;
	  simplified->nodeType = DIV;
	  simplified->child1 = (node *) safeMalloc(sizeof(node));
	  simplified->child1->nodeType = PI_CONST;
	} else {
	  simplified->nodeType = CONSTANT;
	  value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	  mpfr_init2(*value,tools_precision);
	  simplified->value = value;
	  if ((mpfr_atan(*value, *(simplChild1->value), GMP_RNDN) != 0) || 
	      (!mpfr_number_p(*value))) {
	    simplified->nodeType = ATAN;
	    simplified->child1 = simplChild1;
	    mpfr_clear(*value);
	    free(value);
	  } else {
	    free_memory(simplChild1);
	  }
	}
      }
      mpfr_clear(temp);
    } else {
      simplified->nodeType = ATAN;
      simplified->child1 = simplChild1;
    }
    break;
  case SINH:
    simplChild1 = simplifyTreeErrorfreeInner(tree->child1,rec, doRational);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      if ((mpfr_sinh(*value, *(simplChild1->value), GMP_RNDN) != 0) || 
	  (!mpfr_number_p(*value))) {
	simplified->nodeType = SINH;
	simplified->child1 = simplChild1;
	mpfr_clear(*value);
	free(value);
      } else {
	free_memory(simplChild1);
      }
    } else {
      simplified->nodeType = SINH;
      simplified->child1 = simplChild1;
    }
    break;
  case COSH:
    simplChild1 = simplifyTreeErrorfreeInner(tree->child1,rec, doRational);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      if ((mpfr_cosh(*value, *(simplChild1->value), GMP_RNDN) != 0) || 
	  (!mpfr_number_p(*value))) {
	simplified->nodeType = COSH;
	simplified->child1 = simplChild1;
	mpfr_clear(*value);
	free(value);
      } else {
	free_memory(simplChild1);
      }
    } else {
      simplified->nodeType = COSH;
      simplified->child1 = simplChild1;
    }
    break;
  case TANH:
    simplChild1 = simplifyTreeErrorfreeInner(tree->child1,rec, doRational);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      if ((mpfr_tanh(*value, *(simplChild1->value), GMP_RNDN) != 0) || 
	  (!mpfr_number_p(*value))) {
	simplified->nodeType = TANH;
	simplified->child1 = simplChild1;
	mpfr_clear(*value);
	free(value);
      } else {
	free_memory(simplChild1);
      }
    } else {
      simplified->nodeType = TANH;
      simplified->child1 = simplChild1;
    }
    break;
  case ASINH:
    simplChild1 = simplifyTreeErrorfreeInner(tree->child1,rec, doRational);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      if ((mpfr_asinh(*value, *(simplChild1->value), GMP_RNDN) != 0) || 
	  (!mpfr_number_p(*value))) {
	simplified->nodeType = ASINH;
	simplified->child1 = simplChild1;
	mpfr_clear(*value);
	free(value);
      } else {
	free_memory(simplChild1);
      }
    } else {
      simplified->nodeType = ASINH;
      simplified->child1 = simplChild1;
    }
    break;
  case ACOSH:
    simplChild1 = simplifyTreeErrorfreeInner(tree->child1,rec, doRational);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      if ((mpfr_acosh(*value, *(simplChild1->value), GMP_RNDN) != 0) || 
	  (!mpfr_number_p(*value))) {
	simplified->nodeType = ACOSH;
	simplified->child1 = simplChild1;
	mpfr_clear(*value);
	free(value);
      } else {
	free_memory(simplChild1);
      }
    } else {
      simplified->nodeType = ACOSH;
      simplified->child1 = simplChild1;
    }
    break;
  case ATANH:
    simplChild1 = simplifyTreeErrorfreeInner(tree->child1,rec, doRational);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      if ((mpfr_atanh(*value, *(simplChild1->value), GMP_RNDN) != 0) || 
	  (!mpfr_number_p(*value))) {
	simplified->nodeType = ATANH;
	simplified->child1 = simplChild1;
	mpfr_clear(*value);
	free(value);
      } else {
	free_memory(simplChild1);
      }
    } else {
      simplified->nodeType = ATANH;
      simplified->child1 = simplChild1;
    }
    break;
  case POW:
    simplChild1 = simplifyTreeErrorfreeInner(tree->child1,rec, doRational);
    simplChild2 = simplifyTreeErrorfreeInner(tree->child2,rec, doRational);
    simplified = (node*) safeMalloc(sizeof(node));
    if ((simplChild1->nodeType == CONSTANT) && (simplChild2->nodeType == CONSTANT)) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      if ((mpfr_pow(*value, *(simplChild1->value), *(simplChild2->value), GMP_RNDN) != 0) || 
	  (!mpfr_number_p(*value)) || (!mpfr_number_p(*(simplChild1->value)))) {
	simplified->nodeType = POW;
	simplified->child1 = simplChild1;
	simplified->child2 = simplChild2;
	mpfr_clear(*value);
	free(value);
      } else {
	free_memory(simplChild1);
	free_memory(simplChild2);
      }
    } else {
      if ((simplChild2->nodeType == CONSTANT) && (mpfr_cmp_d(*(simplChild2->value),1.0) == 0) && (!mpfr_nan_p(*(simplChild2->value)))) {
	free(simplified);
	free_memory(simplChild2);
	simplified = simplChild1;
      } else {
	value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*value,tools_precision);
	if ((simplChild2->nodeType == CONSTANT) &&
	    (simplChild1->nodeType == POW) && 
	    (simplChild1->child2->nodeType == CONSTANT) &&
	    (mpfr_mul(*value,*(simplChild2->value),*(simplChild1->child2->value),GMP_RNDN) == 0)) {
	  free(simplified);
	  simplified = simplChild1;
	  mpfr_clear(*(simplified->child2->value));
	  free(simplified->child2->value);
	  simplified->child2->value = value;
	  free_memory(simplChild2);
	} else {
	  mpfr_clear(*value);
	  free(value);
	  if ((simplChild2->nodeType == CONSTANT) &&
	      (mpfr_cmp_d(*(simplChild2->value),2.0) == 0) &&
	      (!mpfr_nan_p(*(simplChild2->value))) &&
	      (simplChild1->nodeType == SQRT)) {	    
	    free(simplified);
	    simplified = copyTree(simplChild1->child1);
	    free_memory(simplChild1);
	    free_memory(simplChild2);
	  } else {
	    simplified->nodeType = POW;
	    simplified->child1 = simplChild1;
	    simplified->child2 = simplChild2;
	  }
	}
      }
    }
    break;
  case NEG:
    simplChild1 = simplifyTreeErrorfreeInner(tree->child1,rec, doRational);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      prec = tools_precision;
      p = mpfr_get_prec(*(simplChild1->value));
      if (p > prec) prec = p;
      prec += 10;
      if (prec > 256 * tools_precision) prec = 256 * tools_precision;
      mpfr_init2(*value,prec);
      simplified->value = value;
      if ((mpfr_neg(*value, *(simplChild1->value), GMP_RNDN) != 0) || 
	  (mpfr_nan_p(*value))) {
	simplified->nodeType = NEG;
	simplified->child1 = simplChild1;
	mpfr_clear(*value);
	free(value);
      } else {
	free_memory(simplChild1);
      }
    } else {
      if (simplChild1->nodeType == NEG) {
	free(simplified);
	simplified = copyTree(simplChild1->child1);
	free_memory(simplChild1);
      } else {
	simplified->nodeType = NEG;
	simplified->child1 = simplChild1;
      }
    }
    break;
  case ABS:
    simplChild1 = simplifyTreeErrorfreeInner(tree->child1,rec, doRational);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      if ((mpfr_abs(*value, *(simplChild1->value), GMP_RNDN) != 0) || 
	  (!mpfr_number_p(*value))) {
	simplified->nodeType = ABS;
	simplified->child1 = simplChild1;
	mpfr_clear(*value);
	free(value);
      } else {
	free_memory(simplChild1);
      }
    } else {
      if (simplChild1->nodeType == ABS) {
	simplified->nodeType = ABS;
	simplified->child1 = copyTree(simplChild1->child1);
	free_memory(simplChild1);
      } else {
	simplified->nodeType = ABS;
	simplified->child1 = simplChild1;
      }
    }
    break;
  case DOUBLE:
    simplChild1 = simplifyTreeErrorfreeInner(tree->child1,rec, doRational);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_round_to_double(*value, *(simplChild1->value)); 
      if (!mpfr_number_p(*value)) {
	simplified->nodeType = DOUBLE;
	simplified->child1 = simplChild1;
	mpfr_clear(*value);
	free(value);
      } else {
	free_memory(simplChild1);
      }
    } else {     
      if (isConstant(simplChild1)) {
        xrange.a = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
        xrange.b = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
        yrange.a = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
        yrange.b = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
        mpfr_init2(*(yrange.a),4* tools_precision);
        mpfr_init2(*(yrange.b),4 * tools_precision);
        mpfr_init2(*(xrange.a),tools_precision);
        mpfr_init2(*(xrange.b),tools_precision);
        mpfr_set_ui(*(xrange.a),1,GMP_RNDD);
        mpfr_set_ui(*(xrange.b),1,GMP_RNDU);
        evaluateRangeFunction(yrange, simplChild1, xrange, 8 * tools_precision);
        mpfr_round_to_double(*(xrange.a),*(yrange.a));
        mpfr_round_to_double(*(xrange.b),*(yrange.b));
        if (mpfr_number_p(*(xrange.a)) && 
            mpfr_number_p(*(xrange.b)) &&
            (mpfr_cmp(*(xrange.a),*(xrange.b)) == 0)) {
          simplified->nodeType = CONSTANT;
          value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
          mpfr_init2(*value,tools_precision);
          simplified->value = value;
          mpfr_set(*value,*(xrange.a),GMP_RNDN); /* Exact */
	  free_memory(simplChild1);
        } else {
          simplified->nodeType = DOUBLE;
          simplified->child1 = simplChild1;
        }
        mpfr_clear(*(xrange.a));
        mpfr_clear(*(xrange.b));
        mpfr_clear(*(yrange.a));
        mpfr_clear(*(yrange.b));
        free(xrange.a);
        free(xrange.b);
        free(yrange.a);
        free(yrange.b);
      } else {
        simplified->nodeType = DOUBLE;
        simplified->child1 = simplChild1;
      }
    }
    break;
  case SINGLE:
    simplChild1 = simplifyTreeErrorfreeInner(tree->child1,rec, doRational);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_round_to_single(*value, *(simplChild1->value)); 
      if (!mpfr_number_p(*value)) {
	simplified->nodeType = SINGLE;
	simplified->child1 = simplChild1;
	mpfr_clear(*value);
	free(value);
      } else {
	free_memory(simplChild1);
      }
    } else {     
      if (isConstant(simplChild1)) {
        xrange.a = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
        xrange.b = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
        yrange.a = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
        yrange.b = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
        mpfr_init2(*(yrange.a),4* tools_precision);
        mpfr_init2(*(yrange.b),4 * tools_precision);
        mpfr_init2(*(xrange.a),tools_precision);
        mpfr_init2(*(xrange.b),tools_precision);
        mpfr_set_ui(*(xrange.a),1,GMP_RNDD);
        mpfr_set_ui(*(xrange.b),1,GMP_RNDU);
        evaluateRangeFunction(yrange, simplChild1, xrange, 8 * tools_precision);
        mpfr_round_to_single(*(xrange.a),*(yrange.a));
        mpfr_round_to_single(*(xrange.b),*(yrange.b));
        if (mpfr_number_p(*(xrange.a)) && 
            mpfr_number_p(*(xrange.b)) &&
            (mpfr_cmp(*(xrange.a),*(xrange.b)) == 0)) {
          simplified->nodeType = CONSTANT;
          value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
          mpfr_init2(*value,tools_precision);
          simplified->value = value;
          mpfr_set(*value,*(xrange.a),GMP_RNDN); /* Exact */
	  free_memory(simplChild1);
        } else {
          simplified->nodeType = SINGLE;
          simplified->child1 = simplChild1;
        }
        mpfr_clear(*(xrange.a));
        mpfr_clear(*(xrange.b));
        mpfr_clear(*(yrange.a));
        mpfr_clear(*(yrange.b));
        free(xrange.a);
        free(xrange.b);
        free(yrange.a);
        free(yrange.b);
      } else {
        simplified->nodeType = SINGLE;
        simplified->child1 = simplChild1;
      }
    }
    break;
  case QUAD:
    simplChild1 = simplifyTreeErrorfreeInner(tree->child1,rec, doRational);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_round_to_quad(*value, *(simplChild1->value)); 
      if (!mpfr_number_p(*value)) {
	simplified->nodeType = QUAD;
	simplified->child1 = simplChild1;
	mpfr_clear(*value);
	free(value);
      } else {
	free_memory(simplChild1);
      }
    } else {     
      if (isConstant(simplChild1)) {
        xrange.a = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
        xrange.b = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
        yrange.a = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
        yrange.b = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
        mpfr_init2(*(yrange.a),4* tools_precision);
        mpfr_init2(*(yrange.b),4 * tools_precision);
        mpfr_init2(*(xrange.a),tools_precision);
        mpfr_init2(*(xrange.b),tools_precision);
        mpfr_set_ui(*(xrange.a),1,GMP_RNDD);
        mpfr_set_ui(*(xrange.b),1,GMP_RNDU);
        evaluateRangeFunction(yrange, simplChild1, xrange, 8 * tools_precision);
        mpfr_round_to_quad(*(xrange.a),*(yrange.a));
        mpfr_round_to_quad(*(xrange.b),*(yrange.b));
        if (mpfr_number_p(*(xrange.a)) && 
            mpfr_number_p(*(xrange.b)) &&
            (mpfr_cmp(*(xrange.a),*(xrange.b)) == 0)) {
          simplified->nodeType = CONSTANT;
          value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
          mpfr_init2(*value,tools_precision);
          simplified->value = value;
          mpfr_set(*value,*(xrange.a),GMP_RNDN); /* Exact */
	  free_memory(simplChild1);
        } else {
          simplified->nodeType = QUAD;
          simplified->child1 = simplChild1;
        }
        mpfr_clear(*(xrange.a));
        mpfr_clear(*(xrange.b));
        mpfr_clear(*(yrange.a));
        mpfr_clear(*(yrange.b));
        free(xrange.a);
        free(xrange.b);
        free(yrange.a);
        free(yrange.b);
      } else {
        simplified->nodeType = QUAD;
        simplified->child1 = simplChild1;
      }
    }
    break;
  case HALFPRECISION:
    simplChild1 = simplifyTreeErrorfreeInner(tree->child1,rec, doRational);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_round_to_halfprecision(*value, *(simplChild1->value)); 
      if (!mpfr_number_p(*value)) {
	simplified->nodeType = HALFPRECISION;
	simplified->child1 = simplChild1;
	mpfr_clear(*value);
	free(value);
      } else {
	free_memory(simplChild1);
      }
    } else {     
      if (isConstant(simplChild1)) {
        xrange.a = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
        xrange.b = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
        yrange.a = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
        yrange.b = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
        mpfr_init2(*(yrange.a),4* tools_precision);
        mpfr_init2(*(yrange.b),4 * tools_precision);
        mpfr_init2(*(xrange.a),tools_precision);
        mpfr_init2(*(xrange.b),tools_precision);
        mpfr_set_ui(*(xrange.a),1,GMP_RNDD);
        mpfr_set_ui(*(xrange.b),1,GMP_RNDU);
        evaluateRangeFunction(yrange, simplChild1, xrange, 8 * tools_precision);
        mpfr_round_to_halfprecision(*(xrange.a),*(yrange.a));
        mpfr_round_to_halfprecision(*(xrange.b),*(yrange.b));
        if (mpfr_number_p(*(xrange.a)) && 
            mpfr_number_p(*(xrange.b)) &&
            (mpfr_cmp(*(xrange.a),*(xrange.b)) == 0)) {
          simplified->nodeType = CONSTANT;
          value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
          mpfr_init2(*value,tools_precision);
          simplified->value = value;
          mpfr_set(*value,*(xrange.a),GMP_RNDN); /* Exact */
	  free_memory(simplChild1);
        } else {
          simplified->nodeType = HALFPRECISION;
          simplified->child1 = simplChild1;
        }
        mpfr_clear(*(xrange.a));
        mpfr_clear(*(xrange.b));
        mpfr_clear(*(yrange.a));
        mpfr_clear(*(yrange.b));
        free(xrange.a);
        free(xrange.b);
        free(yrange.a);
        free(yrange.b);
      } else {
        simplified->nodeType = HALFPRECISION;
        simplified->child1 = simplChild1;
      }
    }
    break;
  case DOUBLEDOUBLE:
    simplChild1 = simplifyTreeErrorfreeInner(tree->child1,rec, doRational);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_round_to_doubledouble(*value, *(simplChild1->value)); 
      if (!mpfr_number_p(*value)) {
	simplified->nodeType = DOUBLEDOUBLE;
	simplified->child1 = simplChild1;
	mpfr_clear(*value);
	free(value);
      } else {
	free_memory(simplChild1);
      }
    } else {
      if (isConstant(simplChild1)) {
        xrange.a = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
        xrange.b = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
        yrange.a = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
        yrange.b = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
        mpfr_init2(*(yrange.a),4* tools_precision);
        mpfr_init2(*(yrange.b),4 * tools_precision);
        mpfr_init2(*(xrange.a),tools_precision);
        mpfr_init2(*(xrange.b),tools_precision);
        mpfr_set_ui(*(xrange.a),1,GMP_RNDD);
        mpfr_set_ui(*(xrange.b),1,GMP_RNDU);
        evaluateRangeFunction(yrange, simplChild1, xrange, 8 * tools_precision);
        mpfr_round_to_doubledouble(*(xrange.a),*(yrange.a));
        mpfr_round_to_doubledouble(*(xrange.b),*(yrange.b));
        if (mpfr_number_p(*(xrange.a)) && 
            mpfr_number_p(*(xrange.b)) &&
            (mpfr_cmp(*(xrange.a),*(xrange.b)) == 0)) {
          simplified->nodeType = CONSTANT;
          value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
          mpfr_init2(*value,tools_precision);
          simplified->value = value;
          mpfr_set(*value,*(xrange.a),GMP_RNDN); /* Exact */
	  free_memory(simplChild1);
        } else {
          simplified->nodeType = DOUBLEDOUBLE;
          simplified->child1 = simplChild1;
        }
        mpfr_clear(*(xrange.a));
        mpfr_clear(*(xrange.b));
        mpfr_clear(*(yrange.a));
        mpfr_clear(*(yrange.b));
        free(xrange.a);
        free(xrange.b);
        free(yrange.a);
        free(yrange.b);
      } else {
        simplified->nodeType = DOUBLEDOUBLE;
        simplified->child1 = simplChild1;
      }
    }
    break;
  case TRIPLEDOUBLE:
    simplChild1 = simplifyTreeErrorfreeInner(tree->child1,rec, doRational);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_round_to_tripledouble(*value, *(simplChild1->value)); 
      if (!mpfr_number_p(*value)) {
	simplified->nodeType = TRIPLEDOUBLE;
	simplified->child1 = simplChild1;
	mpfr_clear(*value);
	free(value);
      } else {
	free_memory(simplChild1);
      }
    } else {
      if (isConstant(simplChild1)) {
        xrange.a = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
        xrange.b = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
        yrange.a = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
        yrange.b = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
        mpfr_init2(*(yrange.a),4* tools_precision);
        mpfr_init2(*(yrange.b),4 * tools_precision);
        mpfr_init2(*(xrange.a),tools_precision);
        mpfr_init2(*(xrange.b),tools_precision);
        mpfr_set_ui(*(xrange.a),1,GMP_RNDD);
        mpfr_set_ui(*(xrange.b),1,GMP_RNDU);
        evaluateRangeFunction(yrange, simplChild1, xrange, 8 * tools_precision);
        mpfr_round_to_tripledouble(*(xrange.a),*(yrange.a));
        mpfr_round_to_tripledouble(*(xrange.b),*(yrange.b));
        if (mpfr_number_p(*(xrange.a)) && 
            mpfr_number_p(*(xrange.b)) &&
            (mpfr_cmp(*(xrange.a),*(xrange.b)) == 0)) {
          simplified->nodeType = CONSTANT;
          value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
          mpfr_init2(*value,tools_precision);
          simplified->value = value;
          mpfr_set(*value,*(xrange.a),GMP_RNDN); /* Exact */
	  free_memory(simplChild1);
        } else {
          simplified->nodeType = TRIPLEDOUBLE;
          simplified->child1 = simplChild1;
        }
        mpfr_clear(*(xrange.a));
        mpfr_clear(*(xrange.b));
        mpfr_clear(*(yrange.a));
        mpfr_clear(*(yrange.b));
        free(xrange.a);
        free(xrange.b);
        free(yrange.a);
        free(yrange.b);
      } else {
        simplified->nodeType = TRIPLEDOUBLE;
        simplified->child1 = simplChild1;
      }
    }
    break;
  case ERF: 
    simplChild1 = simplifyTreeErrorfreeInner(tree->child1,rec, doRational);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      if ((mpfr_erf(*value, *(simplChild1->value), GMP_RNDN) != 0) || 
	  (!mpfr_number_p(*value))) {
	simplified->nodeType = ERF;
	simplified->child1 = simplChild1;
	mpfr_clear(*value);
	free(value);
      } else {
	free_memory(simplChild1);
      }
    } else {
      simplified->nodeType = ERF;
      simplified->child1 = simplChild1;
    }
    break;
  case ERFC:
    simplChild1 = simplifyTreeErrorfreeInner(tree->child1,rec, doRational);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      if ((mpfr_erfc(*value, *(simplChild1->value), GMP_RNDN) != 0) || 
	  (!mpfr_number_p(*value))) {
	simplified->nodeType = ERFC;
	simplified->child1 = simplChild1;
	mpfr_clear(*value);
	free(value);
      } else {
	free_memory(simplChild1);
      }
    } else {
      simplified->nodeType = ERFC;
      simplified->child1 = simplChild1;
    }
    break;
  case LOG_1P:
    simplChild1 = simplifyTreeErrorfreeInner(tree->child1,rec, doRational);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      if ((mpfr_log1p(*value, *(simplChild1->value), GMP_RNDN) != 0) || 
	  (!mpfr_number_p(*value))) {
	simplified->nodeType = LOG_1P;
	simplified->child1 = simplChild1;
	mpfr_clear(*value);
	free(value);
      } else {
	free_memory(simplChild1);
      }
    } else {
      simplified->nodeType = LOG_1P;
      simplified->child1 = simplChild1;
    }
    break;
  case EXP_M1:
    simplChild1 = simplifyTreeErrorfreeInner(tree->child1,rec, doRational);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      if ((mpfr_expm1(*value, *(simplChild1->value), GMP_RNDN) != 0) || 
	  (!mpfr_number_p(*value))) {
	simplified->nodeType = EXP_M1;
	simplified->child1 = simplChild1;
	mpfr_clear(*value);
	free(value);
      } else {
	free_memory(simplChild1);
      }
    } else {
      simplified->nodeType = EXP_M1;
      simplified->child1 = simplChild1;
    }
    break;
  case DOUBLEEXTENDED:
    simplChild1 = simplifyTreeErrorfreeInner(tree->child1,rec, doRational);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_round_to_doubleextended(*value, *(simplChild1->value)); 
      if (!mpfr_number_p(*value)) {
	simplified->nodeType = DOUBLEEXTENDED;
	simplified->child1 = simplChild1;
	mpfr_clear(*value);
	free(value);
      } else {
	free_memory(simplChild1);
      }
    } else {
      if (isConstant(simplChild1)) {
        xrange.a = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
        xrange.b = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
        yrange.a = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
        yrange.b = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
        mpfr_init2(*(yrange.a),4* tools_precision);
        mpfr_init2(*(yrange.b),4 * tools_precision);
        mpfr_init2(*(xrange.a),tools_precision);
        mpfr_init2(*(xrange.b),tools_precision);
        mpfr_set_ui(*(xrange.a),1,GMP_RNDD);
        mpfr_set_ui(*(xrange.b),1,GMP_RNDU);
        evaluateRangeFunction(yrange, simplChild1, xrange, 8 * tools_precision);
        mpfr_round_to_doubleextended(*(xrange.a),*(yrange.a));
        mpfr_round_to_doubleextended(*(xrange.b),*(yrange.b));
        if (mpfr_number_p(*(xrange.a)) && 
            mpfr_number_p(*(xrange.b)) &&
            (mpfr_cmp(*(xrange.a),*(xrange.b)) == 0)) {
          simplified->nodeType = CONSTANT;
          value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
          mpfr_init2(*value,tools_precision);
          simplified->value = value;
          mpfr_set(*value,*(xrange.a),GMP_RNDN); /* Exact */
	  free_memory(simplChild1);
        } else {
          simplified->nodeType = DOUBLEEXTENDED;
          simplified->child1 = simplChild1;
        }
        mpfr_clear(*(xrange.a));
        mpfr_clear(*(xrange.b));
        mpfr_clear(*(yrange.a));
        mpfr_clear(*(yrange.b));
        free(xrange.a);
        free(xrange.b);
        free(yrange.a);
        free(yrange.b);
      } else {
        simplified->nodeType = DOUBLEEXTENDED;
        simplified->child1 = simplChild1;
      }
    }
    break;
  case LIBRARYFUNCTION:
    simplified = (node*) safeMalloc(sizeof(node));
    simplified->nodeType = LIBRARYFUNCTION;
    simplified->libFun = tree->libFun;
    simplified->libFunDeriv = tree->libFunDeriv;
    simplified->child1 = simplifyTreeErrorfreeInner(tree->child1,rec, doRational);
    break;
  case PROCEDUREFUNCTION:
    simplified = (node*) safeMalloc(sizeof(node));
    simplified->nodeType = PROCEDUREFUNCTION;
    simplified->libFunDeriv = tree->libFunDeriv;
    simplified->child2 = copyThing(tree->child2);
    simplified->child1 = simplifyTreeErrorfreeInner(tree->child1,rec, doRational);
    break;
  case CEIL:
    simplChild1 = simplifyTreeErrorfreeInner(tree->child1,rec, doRational);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_ceil(*value, *(simplChild1->value));
      if ((!mpfr_number_p(*value))) {
	simplified->nodeType = CEIL;
	simplified->child1 = simplChild1;
	mpfr_clear(*value);
	free(value);
      } else {
	free_memory(simplChild1);
      }
    } else {
      if (isConstant(simplChild1)) {
        xrange.a = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
        xrange.b = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
        yrange.a = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
        yrange.b = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
        mpfr_init2(*(yrange.a),4* tools_precision);
        mpfr_init2(*(yrange.b),4 * tools_precision);
        mpfr_init2(*(xrange.a),tools_precision);
        mpfr_init2(*(xrange.b),tools_precision);
        mpfr_set_ui(*(xrange.a),1,GMP_RNDD);
        mpfr_set_ui(*(xrange.b),1,GMP_RNDU);
        evaluateRangeFunction(yrange, simplChild1, xrange, 8 * tools_precision);
        mpfr_ceil(*(xrange.a),*(yrange.a));
        mpfr_ceil(*(xrange.b),*(yrange.b));
        if (mpfr_number_p(*(xrange.a)) && 
            mpfr_number_p(*(xrange.b)) &&
            (mpfr_cmp(*(xrange.a),*(xrange.b)) == 0)) {
          simplified->nodeType = CONSTANT;
          value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
          mpfr_init2(*value,tools_precision);
          simplified->value = value;
          mpfr_set(*value,*(xrange.a),GMP_RNDN); /* Exact */
	  free_memory(simplChild1);
        } else {
          simplified->nodeType = CEIL;
          simplified->child1 = simplChild1;
        }
        mpfr_clear(*(xrange.a));
        mpfr_clear(*(xrange.b));
        mpfr_clear(*(yrange.a));
        mpfr_clear(*(yrange.b));
        free(xrange.a);
        free(xrange.b);
        free(yrange.a);
        free(yrange.b);
      } else {
        simplified->nodeType = CEIL;
        simplified->child1 = simplChild1;
      }
    }
    break;
  case FLOOR:
    simplChild1 = simplifyTreeErrorfreeInner(tree->child1,rec, doRational);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_floor(*value, *(simplChild1->value));
      if ((!mpfr_number_p(*value))) {
	simplified->nodeType = FLOOR;
	simplified->child1 = simplChild1;
	mpfr_clear(*value);
	free(value);
      } else {
	free_memory(simplChild1);
      }
    } else {
      if (isConstant(simplChild1)) {
        xrange.a = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
        xrange.b = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
        yrange.a = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
        yrange.b = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
        mpfr_init2(*(yrange.a),tools_precision);
        mpfr_init2(*(yrange.b),tools_precision);
        mpfr_init2(*(xrange.a),4 * tools_precision);
        mpfr_init2(*(xrange.b),4 * tools_precision);
        mpfr_set_ui(*(xrange.a),1,GMP_RNDD);
        mpfr_set_ui(*(xrange.b),1,GMP_RNDU);
        evaluateRangeFunction(yrange, simplChild1, xrange, 8 * tools_precision);
        mpfr_floor(*(xrange.a),*(yrange.a));
        mpfr_floor(*(xrange.b),*(yrange.b));
        if (mpfr_number_p(*(xrange.a)) && 
            mpfr_number_p(*(xrange.b)) &&
            (mpfr_cmp(*(xrange.a),*(xrange.b)) == 0)) {
          simplified->nodeType = CONSTANT;
          value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
          mpfr_init2(*value,tools_precision);
          simplified->value = value;
          mpfr_set(*value,*(xrange.a),GMP_RNDN); /* Exact */
	  free_memory(simplChild1);
        } else {
          simplified->nodeType = FLOOR;
          simplified->child1 = simplChild1;
        }
        mpfr_clear(*(xrange.a));
        mpfr_clear(*(xrange.b));
        mpfr_clear(*(yrange.a));
        mpfr_clear(*(yrange.b));
        free(xrange.a);
        free(xrange.b);
        free(yrange.a);
        free(yrange.b);
      } else {
        simplified->nodeType = FLOOR;
        simplified->child1 = simplChild1;
      }
    }
    break;
  case NEARESTINT:
    simplChild1 = simplifyTreeErrorfreeInner(tree->child1,rec, doRational);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_nearestint(*value, *(simplChild1->value));
      if ((!mpfr_number_p(*value))) {
	simplified->nodeType = NEARESTINT;
	simplified->child1 = simplChild1;
	mpfr_clear(*value);
	free(value);
      } else {
	free_memory(simplChild1);
      }
    } else {
      if (isConstant(simplChild1)) {
        xrange.a = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
        xrange.b = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
        yrange.a = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
        yrange.b = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
        mpfr_init2(*(yrange.a),tools_precision);
        mpfr_init2(*(yrange.b),tools_precision);
        mpfr_init2(*(xrange.a),4 * tools_precision);
        mpfr_init2(*(xrange.b),4 * tools_precision);
        mpfr_set_ui(*(xrange.a),1,GMP_RNDD);
        mpfr_set_ui(*(xrange.b),1,GMP_RNDU);
        evaluateRangeFunction(yrange, simplChild1, xrange, 8 * tools_precision);
        mpfr_nearestint(*(xrange.a),*(yrange.a));
        mpfr_nearestint(*(xrange.b),*(yrange.b));
        if (mpfr_number_p(*(xrange.a)) && 
            mpfr_number_p(*(xrange.b)) &&
            (mpfr_cmp(*(xrange.a),*(xrange.b)) == 0)) {
          simplified->nodeType = CONSTANT;
          value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
          mpfr_init2(*value,tools_precision);
          simplified->value = value;
          mpfr_set(*value,*(xrange.a),GMP_RNDN); /* Exact */
	  free_memory(simplChild1);
        } else {
          simplified->nodeType = NEARESTINT;
          simplified->child1 = simplChild1;
        }
        mpfr_clear(*(xrange.a));
        mpfr_clear(*(xrange.b));
        mpfr_clear(*(yrange.a));
        mpfr_clear(*(yrange.b));
        free(xrange.a);
        free(xrange.b);
        free(yrange.a);
        free(yrange.b);
      } else {
        simplified->nodeType = NEARESTINT;
        simplified->child1 = simplChild1;
      }
    }
    break;
  case PI_CONST:
    simplified = (node*) safeMalloc(sizeof(node));
    simplified->nodeType = PI_CONST;
    break;
  case LIBRARYCONSTANT:
    simplified = (node*) safeMalloc(sizeof(node));
    simplified->nodeType = LIBRARYCONSTANT;
    simplified->libFun = tree->libFun;
    break;
  default:
    sollyaFprintf(stderr,"Error: simplifyTreeErrorfreeInner: unknown identifier in the tree\n");
    exit(1);
  }

  return simplified;
}

node *simplifyRationalErrorfree(node *tree) {
  return simplifyTreeErrorfreeInner(tree,1,1);
}

node *simplifyTreeErrorfree(node *tree) {
  node *temp;

  temp = simplifyTreeErrorfreeInner(tree,1,rationalMode);
  if (verbosity >= 7) {
    if (!isSyntacticallyEqual(temp,tree)) {
      if (verbosity < 9) {
	printMessage(7,"Information: an expression has been simplified.\n");
      } else {
	changeToWarningMode();
	printMessage(9,"Information: expression '");
	printTree(tree);
	printMessage(9,"' has been simplified to expression '");
	printTree(temp);
	printMessage(9,"'.\n");
	restoreMode();
      }
    }
  }
  return temp;
}


int isPolynomial(node *tree);
node *differentiatePolynomialUnsafe(node *tree);

node* differentiateUnsimplified(node *tree) {
  node *derivative;
  mpfr_t *mpfr_temp;
  node *temp_node, *temp_node2, *temp_node3, *f_diff, *g_diff, *f_copy, *g_copy, *g_copy2, *h_copy, *h_copy2;
  node *temp_node4, *f_copy2, *temp_node5;

  if (isConstant(tree)) {
    mpfr_temp = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
    mpfr_init2(*mpfr_temp,tools_precision);
    mpfr_set_d(*mpfr_temp,0.0,GMP_RNDN);
    temp_node = (node*) safeMalloc(sizeof(node));
    temp_node->nodeType = CONSTANT;
    temp_node->value = mpfr_temp;
    derivative = temp_node;    
  } else {
    if (isPolynomial(tree)) {
      derivative = differentiatePolynomialUnsafe(tree);
    } else {
      
      switch (tree->nodeType) {
      case VARIABLE:
	mpfr_temp = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*mpfr_temp,tools_precision);
	mpfr_set_d(*mpfr_temp,1.0,GMP_RNDN);
	temp_node = (node*) safeMalloc(sizeof(node));
	temp_node->nodeType = CONSTANT;
	temp_node->value = mpfr_temp;
	derivative = temp_node;
	break;
      case CONSTANT:
	mpfr_temp = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*mpfr_temp,tools_precision);
	mpfr_set_d(*mpfr_temp,0.0,GMP_RNDN);
	temp_node = (node*) safeMalloc(sizeof(node));
	temp_node->nodeType = CONSTANT;
	temp_node->value = mpfr_temp;
	derivative = temp_node;    
	break;
      case ADD:
	temp_node = (node*) safeMalloc(sizeof(node));
	temp_node->nodeType = ADD;
	temp_node->child1 = differentiateUnsimplified(tree->child1);
	temp_node->child2 = differentiateUnsimplified(tree->child2);
	derivative = temp_node;
	break;
      case SUB:
	temp_node = (node*) safeMalloc(sizeof(node));
	temp_node->nodeType = SUB;
	temp_node->child1 = differentiateUnsimplified(tree->child1);
	temp_node->child2 = differentiateUnsimplified(tree->child2);
	derivative = temp_node;
	break;
      case MUL:
	if (tree->child1->nodeType == CONSTANT) {
	  f_diff = differentiateUnsimplified(tree->child2);
	  g_copy = copyTree(tree->child1);
	  temp_node = (node*) safeMalloc(sizeof(node));
	  temp_node->nodeType = MUL;
	  temp_node->child1 = g_copy;
	  temp_node->child2 = f_diff;
	  derivative = temp_node;
	} else {
	  if (tree->child2->nodeType == CONSTANT) {
	    f_diff = differentiateUnsimplified(tree->child1);
	    g_copy = copyTree(tree->child2);
	    temp_node = (node*) safeMalloc(sizeof(node));
	    temp_node->nodeType = MUL;
	    temp_node->child2 = g_copy;
	    temp_node->child1 = f_diff;
	    derivative = temp_node;
	  } else {
	    f_copy = copyTree(tree->child1);
	    g_copy = copyTree(tree->child2);
	    f_diff = differentiateUnsimplified(tree->child1);
	    g_diff = differentiateUnsimplified(tree->child2);
	    temp_node = (node*) safeMalloc(sizeof(node));
	    temp_node->nodeType = ADD;
	    temp_node2 = (node*) safeMalloc(sizeof(node));
	    temp_node2->nodeType = MUL;
	    temp_node3 = (node*) safeMalloc(sizeof(node));
	    temp_node3->nodeType = MUL;
	    temp_node->child1 = temp_node2;
	    temp_node->child2 = temp_node3;
	    temp_node2->child1 = f_copy;
	    temp_node2->child2 = g_diff;
	    temp_node3->child1 = g_copy;
	    temp_node3->child2 = f_diff;
	    derivative = temp_node;
	  }
	}
	break;
      case DIV:
	f_copy = copyTree(tree->child1);
	g_copy = copyTree(tree->child2);
	f_diff = differentiateUnsimplified(tree->child1);
	g_diff = differentiateUnsimplified(tree->child2);
	temp_node = (node*) safeMalloc(sizeof(node));
	temp_node->nodeType = SUB;
	temp_node2 = (node*) safeMalloc(sizeof(node));
	temp_node2->nodeType = MUL;
	temp_node3 = (node*) safeMalloc(sizeof(node));
	temp_node3->nodeType = MUL;
	temp_node->child1 = temp_node2;
	temp_node->child2 = temp_node3;
	temp_node2->child1 = g_copy;
	temp_node2->child2 = f_diff;
	temp_node3->child1 = f_copy;
	temp_node3->child2 = g_diff;
	g_copy = copyTree(tree->child2);
	temp_node2 = (node*) safeMalloc(sizeof(node));
	temp_node2->nodeType = POW;
	temp_node2->child1 = g_copy;
	temp_node4 = (node*) safeMalloc(sizeof(node));
	temp_node4->nodeType = CONSTANT;
	mpfr_temp = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*mpfr_temp,tools_precision);
	mpfr_set_d(*mpfr_temp,2.0,GMP_RNDN);
	temp_node4->value = mpfr_temp;
	temp_node2->child2 = temp_node4;
	temp_node3 = (node*) safeMalloc(sizeof(node));
	temp_node3->nodeType = DIV;
	temp_node3->child1 = temp_node;
	temp_node3->child2 = temp_node2;
	derivative = temp_node3;
	break;
      case SQRT:
	h_copy = copyTree(tree);
	g_diff = differentiateUnsimplified(tree->child1);
	temp_node3 = (node*) safeMalloc(sizeof(node));
	temp_node3->nodeType = CONSTANT;
	mpfr_temp = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*mpfr_temp,tools_precision);
	mpfr_set_d(*mpfr_temp,2.0,GMP_RNDN);
	temp_node3->value = mpfr_temp;
	temp_node2 = (node*) safeMalloc(sizeof(node));
	temp_node2->nodeType = MUL;
	temp_node2->child1 = temp_node3;
	temp_node2->child2 = h_copy;
	temp_node = (node*) safeMalloc(sizeof(node));
	temp_node->nodeType = DIV;
	temp_node->child1 = g_diff;
	temp_node->child2 = temp_node2;
	derivative = temp_node;
	break;
      case EXP:
	g_copy = copyTree(tree->child1);
	g_diff = differentiateUnsimplified(tree->child1);
	temp_node = (node*) safeMalloc(sizeof(node));
	temp_node->nodeType = MUL;
	temp_node->child2 = g_diff;
	temp_node2 = (node*) safeMalloc(sizeof(node));
	temp_node2->nodeType = EXP;
	temp_node->child1 = temp_node2;
	temp_node2->child1 = g_copy;    
	derivative = temp_node;
	break;
      case LOG:
	g_copy = copyTree(tree->child1);
	g_diff = differentiateUnsimplified(tree->child1);
	temp_node = (node*) safeMalloc(sizeof(node));
	temp_node->nodeType = MUL;
	temp_node->child2 = g_diff;
	temp_node2 = (node*) safeMalloc(sizeof(node));
	temp_node2->nodeType = DIV;
	temp_node->child1 = temp_node2;
	temp_node2->child2 = g_copy;    
	temp_node3 = (node*) safeMalloc(sizeof(node));
	temp_node3->nodeType = CONSTANT;
	mpfr_temp = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*mpfr_temp,tools_precision);
	mpfr_set_d(*mpfr_temp,1.0,GMP_RNDN);
	temp_node3->value = mpfr_temp;
	temp_node2->child1 = temp_node3;
	derivative = temp_node;
	break;
      case LOG_2:
	g_copy = copyTree(tree->child1);
	g_diff = differentiateUnsimplified(tree->child1);
	temp_node = (node*) safeMalloc(sizeof(node));
	temp_node->nodeType = MUL;
	temp_node->child2 = g_diff;
	temp_node2 = (node*) safeMalloc(sizeof(node));
	temp_node2->nodeType = DIV;
	temp_node->child1 = temp_node2;
	temp_node3 = (node*) safeMalloc(sizeof(node));
	temp_node3->nodeType = CONSTANT;
	mpfr_temp = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*mpfr_temp,tools_precision);
	mpfr_set_d(*mpfr_temp,1.0,GMP_RNDN);
	temp_node3->value = mpfr_temp;
	temp_node2->child1 = temp_node3;
	temp_node3 = (node*) safeMalloc(sizeof(node));
	temp_node2->child2 = temp_node3;
	temp_node3->nodeType = MUL;
	temp_node3->child1 = g_copy;
	temp_node2 = (node*) safeMalloc(sizeof(node));
	temp_node2->nodeType = LOG;
	temp_node3->child2 = temp_node2;
	temp_node3 = (node*) safeMalloc(sizeof(node));
	temp_node3->nodeType = CONSTANT;
	mpfr_temp = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*mpfr_temp,tools_precision);
	mpfr_set_d(*mpfr_temp,2.0,GMP_RNDN);
	temp_node3->value = mpfr_temp;
	temp_node2->child1 = temp_node3;
	derivative = temp_node;
	break;
      case LOG_10:
	g_copy = copyTree(tree->child1);
	g_diff = differentiateUnsimplified(tree->child1);
	temp_node = (node*) safeMalloc(sizeof(node));
	temp_node->nodeType = MUL;
	temp_node->child2 = g_diff;
	temp_node2 = (node*) safeMalloc(sizeof(node));
	temp_node2->nodeType = DIV;
	temp_node->child1 = temp_node2;
	temp_node3 = (node*) safeMalloc(sizeof(node));
	temp_node3->nodeType = CONSTANT;
	mpfr_temp = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*mpfr_temp,tools_precision);
	mpfr_set_d(*mpfr_temp,1.0,GMP_RNDN);
	temp_node3->value = mpfr_temp;
	temp_node2->child1 = temp_node3;
	temp_node3 = (node*) safeMalloc(sizeof(node));
	temp_node2->child2 = temp_node3;
	temp_node3->nodeType = MUL;
	temp_node3->child1 = g_copy;
	temp_node2 = (node*) safeMalloc(sizeof(node));
	temp_node2->nodeType = LOG;
	temp_node3->child2 = temp_node2;
	temp_node3 = (node*) safeMalloc(sizeof(node));
	temp_node3->nodeType = CONSTANT;
	mpfr_temp = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*mpfr_temp,tools_precision);
	mpfr_set_d(*mpfr_temp,10.0,GMP_RNDN);
	temp_node3->value = mpfr_temp;
	temp_node2->child1 = temp_node3;
	derivative = temp_node;
	break;
      case SIN:
	g_copy = copyTree(tree->child1);
	g_diff = differentiateUnsimplified(tree->child1);
	temp_node = (node*) safeMalloc(sizeof(node));
	temp_node->nodeType = MUL;
	temp_node->child2 = g_diff;
	temp_node2 = (node*) safeMalloc(sizeof(node));
	temp_node2->nodeType = COS;
	temp_node->child1 = temp_node2;
	temp_node2->child1 = g_copy;
	derivative = temp_node;
	break;
      case COS:
	g_copy = copyTree(tree->child1);
	g_diff = differentiateUnsimplified(tree->child1);
	temp_node = (node*) safeMalloc(sizeof(node));
	temp_node->nodeType = MUL;
	temp_node->child2 = g_diff;
	temp_node2 = (node*) safeMalloc(sizeof(node));
	temp_node2->nodeType = SIN;
	temp_node2->child1 = g_copy;
	temp_node3 = (node*) safeMalloc(sizeof(node));
	temp_node3->nodeType = NEG;
	temp_node3->child1 = temp_node2;
	temp_node->child1 = temp_node3;
	derivative = temp_node;
	break;
      case TAN:
	g_diff = differentiateUnsimplified(tree->child1);
	temp_node = (node*) safeMalloc(sizeof(node));
	temp_node->nodeType = MUL;
	temp_node->child2 = g_diff;
	temp_node2 = (node*) safeMalloc(sizeof(node));
	temp_node2->nodeType = ADD;
	temp_node->child1 = temp_node2;
	temp_node3 = (node*) safeMalloc(sizeof(node));
	temp_node3->nodeType = CONSTANT;
	mpfr_temp = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*mpfr_temp,tools_precision);
	mpfr_set_d(*mpfr_temp,1.0,GMP_RNDN);
	temp_node3->value = mpfr_temp;
	temp_node2->child1 = temp_node3;
	h_copy = copyTree(tree);
	temp_node4 = (node*) safeMalloc(sizeof(node));
	temp_node4->nodeType = CONSTANT;
	mpfr_temp = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*mpfr_temp,tools_precision);
	mpfr_set_d(*mpfr_temp,2.0,GMP_RNDN);
	temp_node4->value = mpfr_temp;
	temp_node3 = (node*) safeMalloc(sizeof(node));
	temp_node3->nodeType = POW;
	temp_node2->child2 = temp_node3;
	temp_node3->child1 = h_copy;
	temp_node3->child2 = temp_node4;
	derivative = temp_node;
	break;
      case ASIN:
	g_copy = copyTree(tree->child1);
	g_diff = differentiateUnsimplified(tree->child1);
	temp_node = (node*) safeMalloc(sizeof(node));
	temp_node->nodeType = MUL;
	temp_node->child2 = g_diff;
	temp_node4 = (node*) safeMalloc(sizeof(node));
	temp_node4->nodeType = DIV;
	temp_node->child1 = temp_node4;
	temp_node3 = (node*) safeMalloc(sizeof(node));
	temp_node3->nodeType = CONSTANT;
	mpfr_temp = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*mpfr_temp,tools_precision);
	mpfr_set_d(*mpfr_temp,1.0,GMP_RNDN);
	temp_node3->value = mpfr_temp;
	temp_node4->child1 = temp_node3;
	temp_node3 = (node*) safeMalloc(sizeof(node));
	temp_node3->nodeType = POW;
	temp_node3->child1 = g_copy;
	temp_node5 = (node*) safeMalloc(sizeof(node));
	temp_node5->nodeType = CONSTANT;
	mpfr_temp = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*mpfr_temp,tools_precision);
	mpfr_set_d(*mpfr_temp,2.0,GMP_RNDN);
	temp_node5->value = mpfr_temp;
	temp_node3->child2 = temp_node5;
	temp_node2 = (node*) safeMalloc(sizeof(node));
	temp_node2->nodeType = SUB;
	temp_node2->child2 = temp_node3;
	temp_node3 = (node*) safeMalloc(sizeof(node));
	temp_node3->nodeType = CONSTANT;
	mpfr_temp = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*mpfr_temp,tools_precision);
	mpfr_set_d(*mpfr_temp,1.0,GMP_RNDN);
	temp_node3->value = mpfr_temp;
	temp_node2->child1 = temp_node3;
	temp_node3 = (node*) safeMalloc(sizeof(node));
	temp_node3->nodeType = SQRT;
	temp_node3->child1 = temp_node2;
	temp_node4->child2 = temp_node3;
	derivative = temp_node;
	break;
      case ACOS:
	g_copy = copyTree(tree->child1);
	g_copy2 = copyTree(tree->child1);
	g_diff = differentiateUnsimplified(tree->child1);
	temp_node = (node*) safeMalloc(sizeof(node));
	temp_node->nodeType = MUL;
	temp_node->child2 = g_diff;
	temp_node4 = (node*) safeMalloc(sizeof(node));
	temp_node4->nodeType = DIV;
	temp_node->child1 = temp_node4;
	temp_node3 = (node*) safeMalloc(sizeof(node));
	temp_node3->nodeType = CONSTANT;
	mpfr_temp = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*mpfr_temp,tools_precision);
	mpfr_set_d(*mpfr_temp,-1.0,GMP_RNDN);
	temp_node3->value = mpfr_temp;
	temp_node4->child1 = temp_node3;
	temp_node3 = (node*) safeMalloc(sizeof(node));
	temp_node3->nodeType = MUL;
	temp_node3->child1 = g_copy;
	temp_node3->child2 = g_copy2;
	temp_node2 = (node*) safeMalloc(sizeof(node));
	temp_node2->nodeType = SUB;
	temp_node2->child2 = temp_node3;
	temp_node3 = (node*) safeMalloc(sizeof(node));
	temp_node3->nodeType = CONSTANT;
	mpfr_temp = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*mpfr_temp,tools_precision);
	mpfr_set_d(*mpfr_temp,1.0,GMP_RNDN);
	temp_node3->value = mpfr_temp;
	temp_node2->child1 = temp_node3;
	temp_node3 = (node*) safeMalloc(sizeof(node));
	temp_node3->nodeType = SQRT;
	temp_node3->child1 = temp_node2;
	temp_node4->child2 = temp_node3;
	derivative = temp_node;
	break;
      case ATAN:
	g_copy = copyTree(tree->child1);
	g_copy2 = copyTree(tree->child1);
	g_diff = differentiateUnsimplified(tree->child1);
	temp_node = (node*) safeMalloc(sizeof(node));
	temp_node->nodeType = MUL;
	temp_node->child2 = g_diff;
	temp_node2 = (node*) safeMalloc(sizeof(node));
	temp_node2->nodeType = MUL;
	temp_node2->child1 = g_copy;
	temp_node2->child2 = g_copy2;
	temp_node3 = (node*) safeMalloc(sizeof(node));
	temp_node3->nodeType = CONSTANT;
	mpfr_temp = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*mpfr_temp,tools_precision);
	mpfr_set_d(*mpfr_temp,1.0,GMP_RNDN);
	temp_node3->value = mpfr_temp;
	temp_node4 = (node*) safeMalloc(sizeof(node));
	temp_node4->nodeType = ADD;
	temp_node4->child1 = temp_node3;
	temp_node4->child2 = temp_node2;
	temp_node2 = (node*) safeMalloc(sizeof(node));
	temp_node2->nodeType = DIV;
	temp_node3 = (node*) safeMalloc(sizeof(node));
	temp_node3->nodeType = CONSTANT;
	mpfr_temp = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*mpfr_temp,tools_precision);
	mpfr_set_d(*mpfr_temp,1.0,GMP_RNDN);
	temp_node3->value = mpfr_temp;
	temp_node2->child1 = temp_node3;
	temp_node2->child2 = temp_node4;
	temp_node->child1 = temp_node2;
	derivative = temp_node;
	break;
      case SINH:
	g_copy = copyTree(tree->child1);
	g_diff = differentiateUnsimplified(tree->child1);
	temp_node = (node*) safeMalloc(sizeof(node));
	temp_node->nodeType = MUL;
	temp_node->child2 = g_diff;
	temp_node2 = (node*) safeMalloc(sizeof(node));
	temp_node2->nodeType = COSH;
	temp_node2->child1 = g_copy;
	temp_node->child1 = temp_node2;
	derivative = temp_node;
	break;
      case COSH:
	g_copy = copyTree(tree->child1);
	g_diff = differentiateUnsimplified(tree->child1);
	temp_node = (node*) safeMalloc(sizeof(node));
	temp_node->nodeType = MUL;
	temp_node->child2 = g_diff;
	temp_node2 = (node*) safeMalloc(sizeof(node));
	temp_node2->nodeType = SINH;
	temp_node2->child1 = g_copy;
	temp_node->child1 = temp_node2;
	derivative = temp_node;
	break;
      case TANH:
	g_diff = differentiateUnsimplified(tree->child1);
	temp_node = (node*) safeMalloc(sizeof(node));
	temp_node->nodeType = MUL;
	temp_node->child2 = g_diff;
	temp_node2 = (node*) safeMalloc(sizeof(node));
	temp_node2->nodeType = SUB;
	temp_node->child1 = temp_node2;
	temp_node3 = (node*) safeMalloc(sizeof(node));
	temp_node3->nodeType = CONSTANT;
	mpfr_temp = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*mpfr_temp,tools_precision);
	mpfr_set_d(*mpfr_temp,1.0,GMP_RNDN);
	temp_node3->value = mpfr_temp;
	temp_node2->child1 = temp_node3;
	h_copy = copyTree(tree);
	h_copy2 = copyTree(tree);
	temp_node3 = (node*) safeMalloc(sizeof(node));
	temp_node3->nodeType = MUL;
	temp_node2->child2 = temp_node3;
	temp_node3->child1 = h_copy;
	temp_node3->child2 = h_copy2;
	derivative = temp_node;
	break;
      case ASINH:
	g_copy = copyTree(tree->child1);
	g_copy2 = copyTree(tree->child1);
	g_diff = differentiateUnsimplified(tree->child1);
	temp_node = (node*) safeMalloc(sizeof(node));
	temp_node->nodeType = MUL;
	temp_node->child2 = g_diff;
	temp_node4 = (node*) safeMalloc(sizeof(node));
	temp_node4->nodeType = DIV;
	temp_node->child1 = temp_node4;
	temp_node3 = (node*) safeMalloc(sizeof(node));
	temp_node3->nodeType = CONSTANT;
	mpfr_temp = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*mpfr_temp,tools_precision);
	mpfr_set_d(*mpfr_temp,1.0,GMP_RNDN);
	temp_node3->value = mpfr_temp;
	temp_node4->child1 = temp_node3;
	temp_node3 = (node*) safeMalloc(sizeof(node));
	temp_node3->nodeType = MUL;
	temp_node3->child1 = g_copy;
	temp_node3->child2 = g_copy2;
	temp_node2 = (node*) safeMalloc(sizeof(node));
	temp_node2->nodeType = ADD;
	temp_node2->child2 = temp_node3;
	temp_node3 = (node*) safeMalloc(sizeof(node));
	temp_node3->nodeType = CONSTANT;
	mpfr_temp = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*mpfr_temp,tools_precision);
	mpfr_set_d(*mpfr_temp,1.0,GMP_RNDN);
	temp_node3->value = mpfr_temp;
	temp_node2->child1 = temp_node3;
	temp_node3 = (node*) safeMalloc(sizeof(node));
	temp_node3->nodeType = SQRT;
	temp_node3->child1 = temp_node2;
	temp_node4->child2 = temp_node3;
	derivative = temp_node;
	break;
      case ACOSH:
	g_copy = copyTree(tree->child1);
	g_copy2 = copyTree(tree->child1);
	g_diff = differentiateUnsimplified(tree->child1);
	temp_node = (node*) safeMalloc(sizeof(node));
	temp_node->nodeType = MUL;
	temp_node->child2 = g_diff;
	temp_node4 = (node*) safeMalloc(sizeof(node));
	temp_node4->nodeType = DIV;
	temp_node->child1 = temp_node4;
	temp_node3 = (node*) safeMalloc(sizeof(node));
	temp_node3->nodeType = CONSTANT;
	mpfr_temp = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*mpfr_temp,tools_precision);
	mpfr_set_d(*mpfr_temp,1.0,GMP_RNDN);
	temp_node3->value = mpfr_temp;
	temp_node4->child1 = temp_node3;
	temp_node3 = (node*) safeMalloc(sizeof(node));
	temp_node3->nodeType = MUL;
	temp_node3->child1 = g_copy;
	temp_node3->child2 = g_copy2;
	temp_node2 = (node*) safeMalloc(sizeof(node));
	temp_node2->nodeType = ADD;
	temp_node2->child2 = temp_node3;
	temp_node3 = (node*) safeMalloc(sizeof(node));
	temp_node3->nodeType = CONSTANT;
	mpfr_temp = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*mpfr_temp,tools_precision);
	mpfr_set_d(*mpfr_temp,-1.0,GMP_RNDN);
	temp_node3->value = mpfr_temp;
	temp_node2->child1 = temp_node3;
	temp_node3 = (node*) safeMalloc(sizeof(node));
	temp_node3->nodeType = SQRT;
	temp_node3->child1 = temp_node2;
	temp_node4->child2 = temp_node3;
	derivative = temp_node;
	break;
      case ATANH:
	g_copy = copyTree(tree->child1);
	g_copy2 = copyTree(tree->child1);
	g_diff = differentiateUnsimplified(tree->child1);
	temp_node = (node*) safeMalloc(sizeof(node));
	temp_node->nodeType = MUL;
	temp_node->child2 = g_diff;
	temp_node2 = (node*) safeMalloc(sizeof(node));
	temp_node2->nodeType = MUL;
	temp_node2->child1 = g_copy;
	temp_node2->child2 = g_copy2;
	temp_node3 = (node*) safeMalloc(sizeof(node));
	temp_node3->nodeType = CONSTANT;
	mpfr_temp = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*mpfr_temp,tools_precision);
	mpfr_set_d(*mpfr_temp,1.0,GMP_RNDN);
	temp_node3->value = mpfr_temp;
	temp_node4 = (node*) safeMalloc(sizeof(node));
	temp_node4->nodeType = SUB;
	temp_node4->child1 = temp_node3;
	temp_node4->child2 = temp_node2;
	temp_node2 = (node*) safeMalloc(sizeof(node));
	temp_node2->nodeType = DIV;
	temp_node3 = (node*) safeMalloc(sizeof(node));
	temp_node3->nodeType = CONSTANT;
	mpfr_temp = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*mpfr_temp,tools_precision);
	mpfr_set_d(*mpfr_temp,1.0,GMP_RNDN);
	temp_node3->value = mpfr_temp;
	temp_node2->child1 = temp_node3;
	temp_node2->child2 = temp_node4;
	temp_node->child1 = temp_node2;
	derivative = temp_node;
	break;
      case POW:
	if (tree->child2->nodeType == CONSTANT) {
	  g_copy = copyTree(tree->child2);
	  g_copy2 = copyTree(tree->child2);
	  f_diff = differentiateUnsimplified(tree->child1);      
	  f_copy = copyTree(tree->child1);
	  temp_node2 = (node*) safeMalloc(sizeof(node));
	  temp_node2->nodeType = MUL;
	  temp_node2->child1 = g_copy;
	  temp_node2->child2 = f_diff;
	  temp_node3 = (node*) safeMalloc(sizeof(node));
	  temp_node3->nodeType = SUB;
	  temp_node4 = (node*) safeMalloc(sizeof(node));
	  temp_node4->nodeType = CONSTANT;
	  mpfr_temp = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	  mpfr_init2(*mpfr_temp,tools_precision);
	  mpfr_set_d(*mpfr_temp,1.0,GMP_RNDN);
	  temp_node4->value = mpfr_temp;
	  temp_node3->child2 = temp_node4;
	  temp_node3->child1 = g_copy2;
	  temp_node4 = (node*) safeMalloc(sizeof(node));
	  temp_node4->nodeType = POW;
	  temp_node4->child1 = f_copy;
	  temp_node4->child2 = temp_node3;
	  temp_node = (node*) safeMalloc(sizeof(node));
	  temp_node->nodeType = MUL;
	  temp_node->child1 = temp_node4;
	  temp_node->child2 = temp_node2;
	  derivative = temp_node;
	} else {
	  h_copy = copyTree(tree);
	  f_diff = differentiateUnsimplified(tree->child1);
	  f_copy = copyTree(tree->child1);
	  f_copy2 = copyTree(tree->child1);
	  g_copy = copyTree(tree->child2);
	  g_diff = differentiateUnsimplified(tree->child2);
	  temp_node4 = (node*) safeMalloc(sizeof(node));
	  temp_node4->nodeType = LOG;
	  temp_node4->child1 = f_copy;
	  temp_node3 = (node*) safeMalloc(sizeof(node));
	  temp_node3->nodeType = MUL;
	  temp_node3->child1 = g_diff;
	  temp_node3->child2 = temp_node4;
	  temp_node2 = (node*) safeMalloc(sizeof(node));
	  temp_node2->nodeType = MUL;
	  temp_node2->child1 = f_diff;
	  temp_node2->child2 = g_copy;
	  temp_node = (node*) safeMalloc(sizeof(node));
	  temp_node->nodeType = DIV;
	  temp_node->child1 = temp_node2;
	  temp_node->child2 = f_copy2;
	  temp_node4 = (node*) safeMalloc(sizeof(node));
	  temp_node4->nodeType = ADD;
	  temp_node4->child1 = temp_node;
	  temp_node4->child2 = temp_node3;
	  temp_node = (node*) safeMalloc(sizeof(node));
	  temp_node->nodeType = MUL;
	  temp_node->child1 = h_copy;
	  temp_node->child2 = temp_node4;
	  derivative = temp_node;
	}
	break;
      case NEG:
	g_diff = differentiateUnsimplified(tree->child1);
	temp_node = (node*) safeMalloc(sizeof(node));
	temp_node->nodeType = NEG;
	temp_node->child1 = g_diff;
	derivative = temp_node;
	break;
      case ABS:
	g_copy = copyTree(tree->child1);
	h_copy = copyTree(tree);
	g_diff = differentiateUnsimplified(tree->child1);
	temp_node = (node*) safeMalloc(sizeof(node));
	temp_node->nodeType = MUL;
	temp_node->child2 = g_diff;
	temp_node2 = (node*) safeMalloc(sizeof(node));
	temp_node2->nodeType = DIV;
	temp_node->child1 = temp_node2;
	temp_node2->child1 = g_copy;
	temp_node2->child2 = h_copy;
	derivative = temp_node;
	break;
      case DOUBLE:
	printMessage(1,
		     "Warning: the double rounding operator is not differentiable.\nReplacing it by a constant function when differentiating.\n");
	mpfr_temp = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*mpfr_temp,tools_precision);
	mpfr_set_d(*mpfr_temp,0.0,GMP_RNDN);
	temp_node = (node*) safeMalloc(sizeof(node));
	temp_node->nodeType = CONSTANT;
	temp_node->value = mpfr_temp;
	derivative = temp_node;
	break;
      case SINGLE:
	printMessage(1,
		     "Warning: the single rounding operator is not differentiable.\nReplacing it by a constant function when differentiating.\n");
	mpfr_temp = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*mpfr_temp,tools_precision);
	mpfr_set_d(*mpfr_temp,0.0,GMP_RNDN);
	temp_node = (node*) safeMalloc(sizeof(node));
	temp_node->nodeType = CONSTANT;
	temp_node->value = mpfr_temp;
	derivative = temp_node;
	break;
      case QUAD:
	printMessage(1,
		     "Warning: the quad rounding operator is not differentiable.\nReplacing it by a constant function when differentiating.\n");
	mpfr_temp = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*mpfr_temp,tools_precision);
	mpfr_set_d(*mpfr_temp,0.0,GMP_RNDN);
	temp_node = (node*) safeMalloc(sizeof(node));
	temp_node->nodeType = CONSTANT;
	temp_node->value = mpfr_temp;
	derivative = temp_node;
	break;
      case HALFPRECISION:
	printMessage(1,
		     "Warning: the half-precision rounding operator is not differentiable.\nReplacing it by a constant function when differentiating.\n");
	mpfr_temp = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*mpfr_temp,tools_precision);
	mpfr_set_d(*mpfr_temp,0.0,GMP_RNDN);
	temp_node = (node*) safeMalloc(sizeof(node));
	temp_node->nodeType = CONSTANT;
	temp_node->value = mpfr_temp;
	derivative = temp_node;
	break;
      case DOUBLEDOUBLE:
	printMessage(1,
		     "Warning: the double-double rounding operator is not differentiable.\nReplacing it by a constant function when differentiating.\n");
	mpfr_temp = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*mpfr_temp,tools_precision);
	mpfr_set_d(*mpfr_temp,0.0,GMP_RNDN);
	temp_node = (node*) safeMalloc(sizeof(node));
	temp_node->nodeType = CONSTANT;
	temp_node->value = mpfr_temp;
	derivative = temp_node;
	break;
      case TRIPLEDOUBLE:
	printMessage(1,
		     "Warning: the triple-double rounding operator is not differentiable.\nReplacing it by a constant function when differentiating.\n");
	mpfr_temp = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*mpfr_temp,tools_precision);
	mpfr_set_d(*mpfr_temp,0.0,GMP_RNDN);
	temp_node = (node*) safeMalloc(sizeof(node));
	temp_node->nodeType = CONSTANT;
	temp_node->value = mpfr_temp;
	derivative = temp_node;
	break;
      case ERF: 
	g_copy = copyTree(tree->child1);
	g_diff = differentiateUnsimplified(tree->child1);
	temp_node = (node*) safeMalloc(sizeof(node));
	temp_node->nodeType = MUL;
	temp_node->child2 = g_diff;
	temp_node2 = (node*) safeMalloc(sizeof(node));
	temp_node2->nodeType = DIV;
	temp_node3 = (node *) safeMalloc(sizeof(node));
	mpfr_temp = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*mpfr_temp,tools_precision);
	mpfr_set_d(*mpfr_temp,1.0,GMP_RNDN);
	temp_node3->nodeType = CONSTANT;
	temp_node3->value = mpfr_temp;
	temp_node4 = (node *) safeMalloc(sizeof(node));
	temp_node4->nodeType = ATAN;
	temp_node4->child1 = temp_node3;
	temp_node3 = (node *) safeMalloc(sizeof(node));
	temp_node3->nodeType = SQRT;
	temp_node3->child1 = temp_node4;
	temp_node2->child2 = temp_node3;
	temp_node3 = (node *) safeMalloc(sizeof(node));
	mpfr_temp = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*mpfr_temp,tools_precision);
	mpfr_set_d(*mpfr_temp,2.0,GMP_RNDN);
	temp_node3->nodeType = CONSTANT;
	temp_node3->value = mpfr_temp;
	temp_node4 = (node *) safeMalloc(sizeof(node));
	temp_node4->nodeType = POW;
	temp_node4->child1 = g_copy;
	temp_node4->child2 = temp_node3;
	temp_node3 = (node *) safeMalloc(sizeof(node));
	temp_node3->nodeType = NEG;
	temp_node3->child1 = temp_node4;
	temp_node4 = (node *) safeMalloc(sizeof(node));
	temp_node4->nodeType = EXP;
	temp_node4->child1 = temp_node3;
	temp_node2->child1 = temp_node4;
	temp_node->child1 = temp_node2;
	derivative = temp_node;
	break;
      case ERFC:
	g_copy = copyTree(tree->child1);
	g_diff = differentiateUnsimplified(tree->child1);
	temp_node = (node*) safeMalloc(sizeof(node));
	temp_node->nodeType = MUL;
	temp_node->child2 = g_diff;
	temp_node2 = (node*) safeMalloc(sizeof(node));
	temp_node2->nodeType = DIV;
	temp_node3 = (node *) safeMalloc(sizeof(node));
	mpfr_temp = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*mpfr_temp,tools_precision);
	mpfr_set_d(*mpfr_temp,1.0,GMP_RNDN);
	temp_node3->nodeType = CONSTANT;
	temp_node3->value = mpfr_temp;
	temp_node4 = (node *) safeMalloc(sizeof(node));
	temp_node4->nodeType = ATAN;
	temp_node4->child1 = temp_node3;
	temp_node3 = (node *) safeMalloc(sizeof(node));
	temp_node3->nodeType = SQRT;
	temp_node3->child1 = temp_node4;
	temp_node2->child2 = temp_node3;
	temp_node3 = (node *) safeMalloc(sizeof(node));
	mpfr_temp = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*mpfr_temp,tools_precision);
	mpfr_set_d(*mpfr_temp,2.0,GMP_RNDN);
	temp_node3->nodeType = CONSTANT;
	temp_node3->value = mpfr_temp;
	temp_node4 = (node *) safeMalloc(sizeof(node));
	temp_node4->nodeType = POW;
	temp_node4->child1 = g_copy;
	temp_node4->child2 = temp_node3;
	temp_node3 = (node *) safeMalloc(sizeof(node));
	temp_node3->nodeType = NEG;
	temp_node3->child1 = temp_node4;
	temp_node4 = (node *) safeMalloc(sizeof(node));
	temp_node4->nodeType = EXP;
	temp_node4->child1 = temp_node3;
	temp_node3 = (node *) safeMalloc(sizeof(node));
	temp_node3->nodeType = NEG;
	temp_node3->child1 = temp_node4;
	temp_node2->child1 = temp_node3;
	temp_node->child1 = temp_node2;
	derivative = temp_node;
	break;
      case LOG_1P:
	g_copy = copyTree(tree->child1);
	g_diff = differentiateUnsimplified(tree->child1);
	temp_node = (node*) safeMalloc(sizeof(node));
	temp_node->nodeType = MUL;
	temp_node->child2 = g_diff;
	temp_node2 = (node*) safeMalloc(sizeof(node));
	temp_node2->nodeType = DIV;
	temp_node->child1 = temp_node2;
	temp_node3 = (node*) safeMalloc(sizeof(node));
	temp_node3->nodeType = CONSTANT;
	mpfr_temp = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*mpfr_temp,tools_precision);
	mpfr_set_d(*mpfr_temp,1.0,GMP_RNDN);
	temp_node3->value = mpfr_temp;
	temp_node4 = (node*) safeMalloc(sizeof(node));
	temp_node4->nodeType = ADD;
	temp_node4->child1 = temp_node3;
	temp_node4->child2 = g_copy;
	temp_node2->child2 = temp_node4;    
	temp_node3 = (node*) safeMalloc(sizeof(node));
	temp_node3->nodeType = CONSTANT;
	mpfr_temp = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*mpfr_temp,tools_precision);
	mpfr_set_d(*mpfr_temp,1.0,GMP_RNDN);
	temp_node3->value = mpfr_temp;
	temp_node2->child1 = temp_node3;
	derivative = temp_node;
	break;
      case EXP_M1:
	g_copy = copyTree(tree->child1);
	g_diff = differentiateUnsimplified(tree->child1);
	temp_node = (node*) safeMalloc(sizeof(node));
	temp_node->nodeType = MUL;
	temp_node->child2 = g_diff;
	temp_node2 = (node*) safeMalloc(sizeof(node));
	temp_node2->nodeType = EXP;
	temp_node->child1 = temp_node2;
	temp_node2->child1 = g_copy;    
	derivative = temp_node;
	break;
      case DOUBLEEXTENDED:
	printMessage(1,
		     "Warning: the double-extended rounding operator is not differentiable.\nReplacing it by a constant function when differentiating.\n");
	mpfr_temp = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*mpfr_temp,tools_precision);
	mpfr_set_d(*mpfr_temp,0.0,GMP_RNDN);
	temp_node = (node*) safeMalloc(sizeof(node));
	temp_node->nodeType = CONSTANT;
	temp_node->value = mpfr_temp;
	derivative = temp_node;
	break;
      case LIBRARYFUNCTION:
	g_copy = copyTree(tree->child1);
	g_diff = differentiateUnsimplified(tree->child1);
	temp_node = (node*) safeMalloc(sizeof(node));
	temp_node->nodeType = MUL;
	temp_node->child2 = g_diff;
	temp_node2 = (node*) safeMalloc(sizeof(node));
	temp_node2->nodeType = LIBRARYFUNCTION;
	temp_node2->libFun = tree->libFun;
	temp_node2->libFunDeriv = tree->libFunDeriv + 1;
	temp_node->child1 = temp_node2;
	temp_node2->child1 = g_copy;    
	derivative = temp_node;
	break;
      case PROCEDUREFUNCTION:
	g_copy = copyTree(tree->child1);
	g_diff = differentiateUnsimplified(tree->child1);
	temp_node = (node*) safeMalloc(sizeof(node));
	temp_node->nodeType = MUL;
	temp_node->child2 = g_diff;
	temp_node2 = (node*) safeMalloc(sizeof(node));
	temp_node2->nodeType = PROCEDUREFUNCTION;
        temp_node2->child2 = copyThing(tree->child2);
	temp_node2->libFunDeriv = tree->libFunDeriv + 1;
	temp_node->child1 = temp_node2;
	temp_node2->child1 = g_copy;    
	derivative = temp_node;
	break;
      case CEIL:
	printMessage(1,
		     "Warning: the ceil operator is not differentiable.\nReplacing it by a constant function when differentiating.\n");
	mpfr_temp = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*mpfr_temp,tools_precision);
	mpfr_set_d(*mpfr_temp,0.0,GMP_RNDN);
	temp_node = (node*) safeMalloc(sizeof(node));
	temp_node->nodeType = CONSTANT;
	temp_node->value = mpfr_temp;
	derivative = temp_node;
	break;
      case FLOOR:
	printMessage(1,
		     "Warning: the floor operator is not differentiable.\nReplacing it by a constant function when differentiating.\n");
	mpfr_temp = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*mpfr_temp,tools_precision);
	mpfr_set_d(*mpfr_temp,0.0,GMP_RNDN);
	temp_node = (node*) safeMalloc(sizeof(node));
	temp_node->nodeType = CONSTANT;
	temp_node->value = mpfr_temp;
	derivative = temp_node;
	break;
      case NEARESTINT:
	printMessage(1,
		     "Warning: the nearestint operator is not differentiable.\nReplacing it by a constant function when differentiating.\n");
	mpfr_temp = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*mpfr_temp,tools_precision);
	mpfr_set_d(*mpfr_temp,0.0,GMP_RNDN);
	temp_node = (node*) safeMalloc(sizeof(node));
	temp_node->nodeType = CONSTANT;
	temp_node->value = mpfr_temp;
	derivative = temp_node;
	break;
      case PI_CONST:
      case LIBRARYCONSTANT:
	mpfr_temp = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*mpfr_temp,tools_precision);
	mpfr_set_d(*mpfr_temp,0.0,GMP_RNDN);
	temp_node = (node*) safeMalloc(sizeof(node));
	temp_node->nodeType = CONSTANT;
	temp_node->value = mpfr_temp;
	derivative = temp_node;    
	break;
      default:
	sollyaFprintf(stderr,"Error: differentiateUnsimplified: unknown identifier in the tree\n");
	exit(1);
      }
    }
  }
  return derivative;
}

int isHorner(node *);
int isCanonical(node *);

node* differentiate(node *tree) {
  node *temp, *temp3;

  printMessage(10,"Information: formally differentiating a function.\n");
  
  if (verbosity >= 11) {
    changeToWarningMode();
    printMessage(11,"Information: differentiating the expression '");
    printTree(tree);
    sollyaPrintf("'\n");
    restoreMode();
  }

  if (isPolynomial(tree) && (isHorner(tree) || isCanonical(tree))) {
    temp = differentiateUnsimplified(tree);
  } else {
    if ((treeSize(tree) > MAXDIFFSIMPLSIZE) || (getDegree(tree) > MAXDIFFSIMPLDEGREE)) {
      printMessage(7,"Information: will not simplify the given expression before differentiating because it is too big.\n");
      temp = differentiateUnsimplified(tree);
    } else {
      temp3 = simplifyTreeErrorfree(tree);
      temp = differentiateUnsimplified(temp3);
      free_memory(temp3);
    }
  }
  return temp;
}


int evaluateConstantExpression(mpfr_t result, node *tree, mp_prec_t prec) {
  mpfr_t stack1, stack2;
  sollya_mpfi_t stackI;
  int isConstant;

  mpfr_init2(stack1, prec);
  mpfr_init2(stack2, prec);

  switch (tree->nodeType) {
  case VARIABLE:
    isConstant = 0;
    break;
  case CONSTANT:
    mpfr_set(result, *(tree->value), GMP_RNDN);
    isConstant = 1;
    break;
  case ADD:
    isConstant = evaluateConstantExpression(stack1, tree->child1, prec);
    if (!isConstant) break;
    isConstant = evaluateConstantExpression(stack2, tree->child2, prec);
    if (!isConstant) break;
    mpfr_add(result, stack1, stack2, GMP_RNDN);
    break;
  case SUB:
    isConstant = evaluateConstantExpression(stack1, tree->child1, prec);
    if (!isConstant) break;
    isConstant = evaluateConstantExpression(stack2, tree->child2, prec);
    if (!isConstant) break;
    mpfr_sub(result, stack1, stack2, GMP_RNDN);
    break;
  case MUL:
    isConstant = evaluateConstantExpression(stack1, tree->child1, prec);
    if (!isConstant) break;
    isConstant = evaluateConstantExpression(stack2, tree->child2, prec);
    if (!isConstant) break;
    mpfr_mul(result, stack1, stack2, GMP_RNDN);
    break;
  case DIV:
    isConstant = evaluateConstantExpression(stack1, tree->child1, prec);
    if (!isConstant) break;
    isConstant = evaluateConstantExpression(stack2, tree->child2, prec);
    if (!isConstant) break;
    mpfr_div(result, stack1, stack2, GMP_RNDN);
    break;
  case SQRT:
    isConstant = evaluateConstantExpression(stack1, tree->child1, prec);
    if (!isConstant) break;
    mpfr_sqrt(result, stack1, GMP_RNDN);
    break;
  case EXP:
    isConstant = evaluateConstantExpression(stack1, tree->child1, prec);
    if (!isConstant) break;
    mpfr_exp(result, stack1, GMP_RNDN);
    break;
  case LOG:
    isConstant = evaluateConstantExpression(stack1, tree->child1, prec);
    if (!isConstant) break;
    mpfr_log(result, stack1, GMP_RNDN);
    break;
  case LOG_2:
    isConstant = evaluateConstantExpression(stack1, tree->child1, prec);
    if (!isConstant) break;
    mpfr_log2(result, stack1, GMP_RNDN);
    break;
  case LOG_10:
    isConstant = evaluateConstantExpression(stack1, tree->child1, prec);
    if (!isConstant) break;
    mpfr_log10(result, stack1, GMP_RNDN);
    break;
  case SIN:
    isConstant = evaluateConstantExpression(stack1, tree->child1, prec);
    if (!isConstant) break;
    mpfr_sin(result, stack1, GMP_RNDN);
    break;
  case COS:
    isConstant = evaluateConstantExpression(stack1, tree->child1, prec);
    if (!isConstant) break;
    mpfr_cos(result, stack1, GMP_RNDN);
    break;
  case TAN:
    isConstant = evaluateConstantExpression(stack1, tree->child1, prec);
    if (!isConstant) break;
    mpfr_tan(result, stack1, GMP_RNDN);
    break;
  case ASIN:
    isConstant = evaluateConstantExpression(stack1, tree->child1, prec);
    if (!isConstant) break;
    mpfr_asin(result, stack1, GMP_RNDN);
    break;
  case ACOS:
    isConstant = evaluateConstantExpression(stack1, tree->child1, prec);
    if (!isConstant) break;
    mpfr_acos(result, stack1, GMP_RNDN);
    break;
  case ATAN:
    isConstant = evaluateConstantExpression(stack1, tree->child1, prec);
    if (!isConstant) break;
    mpfr_atan(result, stack1, GMP_RNDN);
    break;
  case SINH:
    isConstant = evaluateConstantExpression(stack1, tree->child1, prec);
    if (!isConstant) break;
    mpfr_sinh(result, stack1, GMP_RNDN);
    break;
  case COSH:
    isConstant = evaluateConstantExpression(stack1, tree->child1, prec);
    if (!isConstant) break;
    mpfr_cosh(result, stack1, GMP_RNDN);
    break;
  case TANH:
    isConstant = evaluateConstantExpression(stack1, tree->child1, prec);
    if (!isConstant) break;
    mpfr_tanh(result, stack1, GMP_RNDN);
    break;
  case ASINH:
    isConstant = evaluateConstantExpression(stack1, tree->child1, prec);
    if (!isConstant) break;
    mpfr_asinh(result, stack1, GMP_RNDN);
    break;
  case ACOSH:
    isConstant = evaluateConstantExpression(stack1, tree->child1, prec);
    if (!isConstant) break;
    mpfr_acosh(result, stack1, GMP_RNDN);
    break;
  case ATANH:
    isConstant = evaluateConstantExpression(stack1, tree->child1, prec);
    if (!isConstant) break;
    mpfr_atanh(result, stack1, GMP_RNDN);
    break;
  case POW:
    isConstant = evaluateConstantExpression(stack1, tree->child1, prec);
    if (!isConstant) break;
    isConstant = evaluateConstantExpression(stack2, tree->child2, prec);
    if (!isConstant) break;
    mpfr_pow(result, stack1, stack2, GMP_RNDN);
    break;
  case NEG:
    isConstant = evaluateConstantExpression(stack1, tree->child1, prec);
    if (!isConstant) break;
    mpfr_neg(result, stack1, GMP_RNDN);
    break;
  case ABS:
    isConstant = evaluateConstantExpression(stack1, tree->child1, prec);
    if (!isConstant) break;
    mpfr_abs(result, stack1, GMP_RNDN);
    break;
  case DOUBLE:
    isConstant = evaluateConstantExpression(stack1, tree->child1, prec);
    if (!isConstant) break;
    mpfr_round_to_double(result, stack1);
    break;
  case SINGLE:
    isConstant = evaluateConstantExpression(stack1, tree->child1, prec);
    if (!isConstant) break;
    mpfr_round_to_single(result, stack1);
    break;
  case QUAD:
    isConstant = evaluateConstantExpression(stack1, tree->child1, prec);
    if (!isConstant) break;
    mpfr_round_to_quad(result, stack1);
    break;
  case HALFPRECISION:
    isConstant = evaluateConstantExpression(stack1, tree->child1, prec);
    if (!isConstant) break;
    mpfr_round_to_halfprecision(result, stack1);
    break;
  case DOUBLEDOUBLE:
    isConstant = evaluateConstantExpression(stack1, tree->child1, prec);
    if (!isConstant) break;
    mpfr_round_to_doubledouble(result, stack1);
  case TRIPLEDOUBLE:
    isConstant = evaluateConstantExpression(stack1, tree->child1, prec);
    if (!isConstant) break;
    mpfr_round_to_tripledouble(result, stack1);
    break;
  case ERF:
    isConstant = evaluateConstantExpression(stack1, tree->child1, prec);
    if (!isConstant) break;
    mpfr_erf(result, stack1, GMP_RNDN);
    break;
  case ERFC:
    isConstant = evaluateConstantExpression(stack1, tree->child1, prec);
    if (!isConstant) break;
    mpfr_erfc(result, stack1, GMP_RNDN);
    break;
  case LOG_1P:
    isConstant = evaluateConstantExpression(stack1, tree->child1, prec);
    if (!isConstant) break;
    mpfr_log1p(result, stack1, GMP_RNDN);
    break;
  case EXP_M1:
    isConstant = evaluateConstantExpression(stack1, tree->child1, prec);
    if (!isConstant) break;
    mpfr_expm1(result, stack1, GMP_RNDN);
    break;
  case DOUBLEEXTENDED:
    isConstant = evaluateConstantExpression(stack1, tree->child1, prec);
    if (!isConstant) break;
    mpfr_round_to_doubleextended(result, stack1);
    break;
  case LIBRARYFUNCTION:
    isConstant = evaluateConstantExpression(stack1, tree->child1, prec);
    if (!isConstant) break;
    mpfr_from_mpfi(result, stack1, tree->libFunDeriv, tree->libFun->code);
    break;
  case PROCEDUREFUNCTION:
    isConstant = evaluateConstantExpression(stack1, tree->child1, prec);
    if (!isConstant) break;
    computeFunctionWithProcedureMpfr(result, tree->child2, stack1, (unsigned int) tree->libFunDeriv);
    break;
  case CEIL:
    isConstant = evaluateConstantExpression(stack1, tree->child1, prec);
    if (!isConstant) break;
    mpfr_ceil(result, stack1);
    break;
  case FLOOR:
    isConstant = evaluateConstantExpression(stack1, tree->child1, prec);
    if (!isConstant) break;
    mpfr_floor(result, stack1);
    break;
  case NEARESTINT:
    isConstant = evaluateConstantExpression(stack1, tree->child1, prec);
    if (!isConstant) break;
    mpfr_nearestint(result, stack1);
    break;
  case PI_CONST:
    mpfr_const_pi(result, GMP_RNDN);
    isConstant = 1;
    break;
  case LIBRARYCONSTANT:
    sollya_mpfi_init2(stackI, mpfr_get_prec(result));
    libraryConstantToInterval(stackI, tree);
    sollya_mpfi_get_fr(result, stackI);
    sollya_mpfi_clear(stackI);
    isConstant = 1;
    break;
  default:
    sollyaFprintf(stderr,"Error: evaluateConstantExpression: unknown identifier in the tree\n");
    exit(1);
  }

  mpfr_clear(stack1); mpfr_clear(stack2);
  return isConstant;
} 


node* simplifyTreeInner(node *tree) {
  node *simplChild1, *simplChild2, *simplified;
  mpfr_t *value;
  mpfr_t temp;
  sollya_mpfi_t tempI;
  int numberChilds;

  if ((tree->nodeType == CONSTANT) && (mpfr_nan_p(*(tree->value)))) return copyTree(tree);
  if (tree->nodeType != VARIABLE) {
    numberChilds = arity(tree);
    switch (numberChilds) {
    case 0:
      break;
    case 1:
      if ((tree->child1->nodeType == CONSTANT) && (mpfr_nan_p(*(tree->child1->value)))) return copyTree(tree);
      break;
    case 2:
      if ((tree->child1->nodeType == CONSTANT) && (mpfr_nan_p(*(tree->child1->value)))) {
	if (isConstant(tree)) return copyTree(tree->child1);
	return copyTree(tree);
      }
      if ((tree->child2->nodeType == CONSTANT) && (mpfr_nan_p(*(tree->child2->value)))) {
	if (isConstant(tree)) return copyTree(tree->child2);
	return copyTree(tree);
      }
      break;
    default:
      sollyaFprintf(stderr,"Error: simplifyTreeInner: unknown arity of tree node symbol.\n");
      exit(1);
    }
  }

  switch (tree->nodeType) {
  case VARIABLE:
    simplified = (node*) safeMalloc(sizeof(node));
    simplified->nodeType = VARIABLE;
    break;
  case CONSTANT:
    simplified = (node*) safeMalloc(sizeof(node));
    simplified->nodeType = CONSTANT;
    value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
    mpfr_init2(temp,tools_precision);
    simplifyMpfrPrec(temp,*(tree->value));
    mpfr_init2(*value,mpfr_get_prec(temp));
    mpfr_set(*value,temp,GMP_RNDN);
    mpfr_clear(temp);
    simplified->value = value;
    break;
  case ADD:
    simplChild1 = simplifyTreeInner(tree->child1);
    simplChild2 = simplifyTreeInner(tree->child2);
    simplified = (node*) safeMalloc(sizeof(node));
    if ((simplChild1->nodeType == CONSTANT) && (simplChild2->nodeType == CONSTANT)) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_add(*value, *(simplChild1->value), *(simplChild2->value), GMP_RNDN);
      free_memory(simplChild1);
      free_memory(simplChild2);
    } else {
      if ((simplChild1->nodeType == CONSTANT) && (mpfr_zero_p(*(simplChild1->value)))) {
	free_memory(simplChild1);
	free(simplified);
	simplified = simplChild2;
      } else {
	if ((simplChild2->nodeType == CONSTANT) && (mpfr_zero_p(*(simplChild2->value)))) {
	  free_memory(simplChild2);
	  free(simplified);
	  simplified = simplChild1;
	} else {
	  simplified->nodeType = ADD;
	  simplified->child1 = simplChild1;
	  simplified->child2 = simplChild2;
	}
      }
    }
    break;
  case SUB:
    simplChild1 = simplifyTreeInner(tree->child1);
    simplChild2 = simplifyTreeInner(tree->child2);
    simplified = (node*) safeMalloc(sizeof(node));
    if ((simplChild1->nodeType == CONSTANT) && (simplChild2->nodeType == CONSTANT)) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_sub(*value, *(simplChild1->value), *(simplChild2->value), GMP_RNDN);
      free_memory(simplChild1);
      free_memory(simplChild2);
    } else {
      if ((simplChild1->nodeType == CONSTANT) && (mpfr_zero_p(*(simplChild1->value)))) {
	free_memory(simplChild1);
	simplified->nodeType = NEG;
	simplified->child1 = simplChild2;
      } else {
	if ((simplChild2->nodeType == CONSTANT) && (mpfr_zero_p(*(simplChild2->value)))) {
	  free_memory(simplChild2);
	  free(simplified);
	  simplified = simplChild1;
	} else {
	  simplified->nodeType = SUB;
	  simplified->child1 = simplChild1;
	  simplified->child2 = simplChild2;
	}
      }
    }
    break;
  case MUL:
    simplChild1 = simplifyTreeInner(tree->child1);
    simplChild2 = simplifyTreeInner(tree->child2);
    simplified = (node*) safeMalloc(sizeof(node));
    if ((simplChild1->nodeType == CONSTANT) && (simplChild2->nodeType == CONSTANT)) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_mul(*value, *(simplChild1->value), *(simplChild2->value), GMP_RNDN);
      free_memory(simplChild1);
      free_memory(simplChild2);
    } else {
      if (((simplChild1->nodeType == CONSTANT) && (mpfr_zero_p(*(simplChild1->value)))) ||
	  ((simplChild2->nodeType == CONSTANT) && (mpfr_zero_p(*(simplChild2->value))))) {
	free_memory(simplChild1);
	free_memory(simplChild2);
	simplified->nodeType = CONSTANT;
	value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*value,tools_precision);
	simplified->value = value;
	mpfr_set_d(*value,0.0,GMP_RNDN);
      } else {
	if ((simplChild1->nodeType == CONSTANT) && (mpfr_cmp_d(*(simplChild1->value),1.0) == 0) && (!mpfr_nan_p(*(simplChild1->value)))) {
	  free_memory(simplChild1);
	  free(simplified);
	  simplified = simplChild2;
	} else {
	  if ((simplChild2->nodeType == CONSTANT) && (mpfr_cmp_d(*(simplChild2->value),1.0) == 0) && (!mpfr_nan_p(*(simplChild2->value)))) {
	    free_memory(simplChild2);
	    free(simplified);
	    simplified = simplChild1;
	  } else {
	    simplified->nodeType = MUL;
	    simplified->child1 = simplChild1;
	    simplified->child2 = simplChild2;
	  }
	}
      }
    }
    break;
  case DIV:
    simplChild1 = simplifyTreeInner(tree->child1);
    simplChild2 = simplifyTreeInner(tree->child2);
    simplified = (node*) safeMalloc(sizeof(node));
    if ((simplChild1->nodeType == CONSTANT) && (simplChild2->nodeType == CONSTANT)) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_div(*value, *(simplChild1->value), *(simplChild2->value), GMP_RNDN);
      free_memory(simplChild1);
      free_memory(simplChild2);
    } else {
      if ((simplChild1->nodeType == CONSTANT) && (mpfr_zero_p(*(simplChild1->value)))) {
	free_memory(simplChild1);
	free_memory(simplChild2);
	simplified->nodeType = CONSTANT;
	value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*value,tools_precision);
	simplified->value = value;
	mpfr_set_d(*value,0.0,GMP_RNDN);
      } else {
	if ((simplChild2->nodeType == CONSTANT) && (mpfr_cmp_d(*(simplChild2->value),1.0) == 0)) {
	  if (mpfr_nan_p(*(simplChild2->value))) {
	    simplified->nodeType = MUL;
	    simplified->child1 = simplChild1;
	    simplified->child2 = simplChild2;
	  } else {
	    free_memory(simplChild2);
	    free(simplified);
	    simplified = simplChild1;
	  }
	} else {
	  simplified->nodeType = DIV;
	  simplified->child1 = simplChild1;
	  simplified->child2 = simplChild2;
	}
      }
    }
    break;
  case SQRT:
    simplChild1 = simplifyTreeInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_sqrt(*value, *(simplChild1->value), GMP_RNDN);
      free_memory(simplChild1);
    } else {
      simplified->nodeType = SQRT;
      simplified->child1 = simplChild1;
    }
    break;
  case EXP:
    simplChild1 = simplifyTreeInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_exp(*value, *(simplChild1->value), GMP_RNDN);
      free_memory(simplChild1);
    } else {
      simplified->nodeType = EXP;
      simplified->child1 = simplChild1;
    }
    break;
  case LOG:
    simplChild1 = simplifyTreeInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_log(*value, *(simplChild1->value), GMP_RNDN);
      free_memory(simplChild1);
    } else {
      simplified->nodeType = LOG;
      simplified->child1 = simplChild1;
    }
    break;
  case LOG_2:
    simplChild1 = simplifyTreeInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_log2(*value, *(simplChild1->value), GMP_RNDN);
      free_memory(simplChild1);
    } else {
      simplified->nodeType = LOG_2;
      simplified->child1 = simplChild1;
    }
    break;
  case LOG_10:
    simplChild1 = simplifyTreeInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_log10(*value, *(simplChild1->value), GMP_RNDN);
      free_memory(simplChild1);
    } else {
      simplified->nodeType = LOG_10;
      simplified->child1 = simplChild1;
    }
    break;
  case SIN:
    simplChild1 = simplifyTreeInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_sin(*value, *(simplChild1->value), GMP_RNDN);
      free_memory(simplChild1);
    } else {
      simplified->nodeType = SIN;
      simplified->child1 = simplChild1;
    }
    break;
  case COS:
    simplChild1 = simplifyTreeInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_cos(*value, *(simplChild1->value), GMP_RNDN);
      free_memory(simplChild1);
    } else {
      simplified->nodeType = COS;
      simplified->child1 = simplChild1;
    }
    break;
  case TAN:
    simplChild1 = simplifyTreeInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_tan(*value, *(simplChild1->value), GMP_RNDN);
      free_memory(simplChild1);
    } else {
      simplified->nodeType = TAN;
      simplified->child1 = simplChild1;
    }
    break;
  case ASIN:
    simplChild1 = simplifyTreeInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_asin(*value, *(simplChild1->value), GMP_RNDN);
      free_memory(simplChild1);
    } else {
      simplified->nodeType = ASIN;
      simplified->child1 = simplChild1;
    }
    break;
  case ACOS:
    simplChild1 = simplifyTreeInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_acos(*value, *(simplChild1->value), GMP_RNDN);
      free_memory(simplChild1);
    } else {
      simplified->nodeType = ACOS;
      simplified->child1 = simplChild1;
    }
    break;
  case ATAN:
    simplChild1 = simplifyTreeInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_atan(*value, *(simplChild1->value), GMP_RNDN);
      free_memory(simplChild1);
    } else {
      simplified->nodeType = ATAN;
      simplified->child1 = simplChild1;
    }
    break;
  case SINH:
    simplChild1 = simplifyTreeInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_sinh(*value, *(simplChild1->value), GMP_RNDN);
      free_memory(simplChild1);
    } else {
      simplified->nodeType = SINH;
      simplified->child1 = simplChild1;
    }
    break;
  case COSH:
    simplChild1 = simplifyTreeInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_cosh(*value, *(simplChild1->value), GMP_RNDN);
      free_memory(simplChild1);
    } else {
      simplified->nodeType = COSH;
      simplified->child1 = simplChild1;
    }
    break;
  case TANH:
    simplChild1 = simplifyTreeInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_tanh(*value, *(simplChild1->value), GMP_RNDN);
      free_memory(simplChild1);
    } else {
      simplified->nodeType = TANH;
      simplified->child1 = simplChild1;
    }
    break;
  case ASINH:
    simplChild1 = simplifyTreeInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_asinh(*value, *(simplChild1->value), GMP_RNDN);
      free_memory(simplChild1);
    } else {
      simplified->nodeType = ASINH;
      simplified->child1 = simplChild1;
    }
    break;
  case ACOSH:
    simplChild1 = simplifyTreeInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_acosh(*value, *(simplChild1->value), GMP_RNDN);
      free_memory(simplChild1);
    } else {
      simplified->nodeType = ACOSH;
      simplified->child1 = simplChild1;
    }
    break;
  case ATANH:
    simplChild1 = simplifyTreeInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_atanh(*value, *(simplChild1->value), GMP_RNDN);
      free_memory(simplChild1);
    } else {
      simplified->nodeType = ATANH;
      simplified->child1 = simplChild1;
    }
    break;
  case POW:
    simplChild1 = simplifyTreeInner(tree->child1);
    simplChild2 = simplifyTreeInner(tree->child2);
    simplified = (node*) safeMalloc(sizeof(node));
    if ((simplChild1->nodeType == CONSTANT) && (simplChild2->nodeType == CONSTANT)) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_pow(*value, *(simplChild1->value), *(simplChild2->value), GMP_RNDN);
      free_memory(simplChild1);
      free_memory(simplChild2);
    } else {
      if ((simplChild2->nodeType == CONSTANT) && (mpfr_cmp_d(*(simplChild2->value),1.0) == 0) && (!mpfr_nan_p(*(simplChild2->value)))) {
	free(simplified);
	free_memory(simplChild2);
	simplified = simplChild1;
      } else {
	simplified->nodeType = POW;
	simplified->child1 = simplChild1;
	simplified->child2 = simplChild2;
      }
    }
    break;
  case NEG:
    simplChild1 = simplifyTreeInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_neg(*value, *(simplChild1->value), GMP_RNDN);
      free_memory(simplChild1);
    } else {
      simplified->nodeType = NEG;
      simplified->child1 = simplChild1;
    }
    break;
  case ABS:
    simplChild1 = simplifyTreeInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_abs(*value, *(simplChild1->value), GMP_RNDN);
      free_memory(simplChild1);
    } else {
      simplified->nodeType = ABS;
      simplified->child1 = simplChild1;
    }
    break;
  case DOUBLE:
    simplChild1 = simplifyTreeInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_round_to_double(*value, *(simplChild1->value));
      free_memory(simplChild1);
    } else {
      simplified->nodeType = DOUBLE;
      simplified->child1 = simplChild1;
    }
    break;
  case SINGLE:
    simplChild1 = simplifyTreeInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_round_to_single(*value, *(simplChild1->value));
      free_memory(simplChild1);
    } else {
      simplified->nodeType = SINGLE;
      simplified->child1 = simplChild1;
    }
    break;
  case QUAD:
    simplChild1 = simplifyTreeInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_round_to_quad(*value, *(simplChild1->value));
      free_memory(simplChild1);
    } else {
      simplified->nodeType = QUAD;
      simplified->child1 = simplChild1;
    }
    break;
  case HALFPRECISION:
    simplChild1 = simplifyTreeInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_round_to_halfprecision(*value, *(simplChild1->value));
      free_memory(simplChild1);
    } else {
      simplified->nodeType = HALFPRECISION;
      simplified->child1 = simplChild1;
    }
    break;
  case DOUBLEDOUBLE:
    simplChild1 = simplifyTreeInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_round_to_doubledouble(*value, *(simplChild1->value));
      free_memory(simplChild1);
    } else {
      simplified->nodeType = DOUBLEDOUBLE;
      simplified->child1 = simplChild1;
    }
    break;
  case TRIPLEDOUBLE:
    simplChild1 = simplifyTreeInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_round_to_tripledouble(*value, *(simplChild1->value));
      free_memory(simplChild1);
    } else {
      simplified->nodeType = TRIPLEDOUBLE;
      simplified->child1 = simplChild1;
    }
    break;
  case ERF:
    simplChild1 = simplifyTreeInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_erf(*value, *(simplChild1->value), GMP_RNDN);
      free_memory(simplChild1);
    } else {
      simplified->nodeType = ERF;
      simplified->child1 = simplChild1;
    }
    break;
  case ERFC:
    simplChild1 = simplifyTreeInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_erfc(*value, *(simplChild1->value), GMP_RNDN);
      free_memory(simplChild1);
    } else {
      simplified->nodeType = ERFC;
      simplified->child1 = simplChild1;
    }
    break;
  case LOG_1P:
    simplChild1 = simplifyTreeInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_log1p(*value, *(simplChild1->value), GMP_RNDN);
      free_memory(simplChild1);
    } else {
      simplified->nodeType = LOG_1P;
      simplified->child1 = simplChild1;
    }
    break;
  case EXP_M1:
    simplChild1 = simplifyTreeInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_expm1(*value, *(simplChild1->value), GMP_RNDN);
      free_memory(simplChild1);
    } else {
      simplified->nodeType = EXP_M1;
      simplified->child1 = simplChild1;
    }
    break;
  case DOUBLEEXTENDED:
    simplChild1 = simplifyTreeInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_round_to_doubleextended(*value, *(simplChild1->value));
      free_memory(simplChild1);
    } else {
      simplified->nodeType = DOUBLEEXTENDED;
      simplified->child1 = simplChild1;
    }
    break;
  case LIBRARYFUNCTION:
    simplChild1 = simplifyTreeInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_from_mpfi(*value, *(simplChild1->value), tree->libFunDeriv, tree->libFun->code);
      free_memory(simplChild1);
    } else {
      simplified->nodeType = LIBRARYFUNCTION;
      simplified->child1 = simplChild1;
      simplified->libFun = tree->libFun;
      simplified->libFunDeriv = tree->libFunDeriv;
    }
    break;
  case PROCEDUREFUNCTION:
    simplChild1 = simplifyTreeInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      computeFunctionWithProcedureMpfr(*value, tree->child2, *(simplChild1->value), (unsigned int) tree->libFunDeriv);
      free_memory(simplChild1);
    } else {
      simplified->nodeType = PROCEDUREFUNCTION;
      simplified->child1 = simplChild1;
      simplified->child2 = copyThing(tree->child2);
      simplified->libFunDeriv = tree->libFunDeriv;
    }
    break;
  case CEIL:
    simplChild1 = simplifyTreeInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_ceil(*value, *(simplChild1->value));
      free_memory(simplChild1);
    } else {
      simplified->nodeType = CEIL;
      simplified->child1 = simplChild1;
    }
    break;
  case FLOOR:
    simplChild1 = simplifyTreeInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_floor(*value, *(simplChild1->value));
      free_memory(simplChild1);
    } else {
      simplified->nodeType = FLOOR;
      simplified->child1 = simplChild1;
    }
    break;
  case NEARESTINT:
    simplChild1 = simplifyTreeInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_nearestint(*value, *(simplChild1->value));
      free_memory(simplChild1);
    } else {
      simplified->nodeType = NEARESTINT;
      simplified->child1 = simplChild1;
    }
    break;
  case PI_CONST:
    simplified = (node*) safeMalloc(sizeof(node));
    simplified->nodeType = CONSTANT;
    value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
    mpfr_init2(*value,tools_precision);
    mpfr_const_pi(*value,GMP_RNDN);
    simplified->value = value;
    break;
  case LIBRARYCONSTANT:
    simplified = (node*) safeMalloc(sizeof(node));
    simplified->nodeType = CONSTANT;
    value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
    mpfr_init2(*value,tools_precision);
    sollya_mpfi_init2(tempI, tools_precision);
    libraryConstantToInterval(tempI, tree);
    sollya_mpfi_get_fr(*value, tempI);
    sollya_mpfi_clear(tempI);
    simplified->value = value;
    break;
  default:
    sollyaFprintf(stderr,"Error: simplifyTreeInner: unknown identifier (%d) in the tree\n",tree->nodeType);
    exit(1);
  }

  return simplified;
}

node* simplifyAllButDivisionInner(node *tree) {
  node *simplChild1, *simplChild2, *simplified;
  mpfr_t *value;
  mpfr_t temp;
  sollya_mpfi_t tempI;

  switch (tree->nodeType) {
  case VARIABLE:
    simplified = (node*) safeMalloc(sizeof(node));
    simplified->nodeType = VARIABLE;
    break;
  case CONSTANT:
    simplified = (node*) safeMalloc(sizeof(node));
    simplified->nodeType = CONSTANT;
    value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
    mpfr_init2(temp,tools_precision);
    simplifyMpfrPrec(temp,*(tree->value));
    mpfr_init2(*value,mpfr_get_prec(temp));
    mpfr_set(*value,temp,GMP_RNDN);
    mpfr_clear(temp);
    simplified->value = value;
    break;
  case ADD:
    simplChild1 = simplifyAllButDivisionInner(tree->child1);
    simplChild2 = simplifyAllButDivisionInner(tree->child2);
    simplified = (node*) safeMalloc(sizeof(node));
    if ((simplChild1->nodeType == CONSTANT) && (simplChild2->nodeType == CONSTANT)) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_add(*value, *(simplChild1->value), *(simplChild2->value), GMP_RNDN);
      free_memory(simplChild1);
      free_memory(simplChild2);
    } else {
      if ((simplChild1->nodeType == CONSTANT) && (mpfr_zero_p(*(simplChild1->value)))) {
	free_memory(simplChild1);
	free(simplified);
	simplified = simplChild2;
      } else {
	if ((simplChild2->nodeType == CONSTANT) && (mpfr_zero_p(*(simplChild2->value)))) {
	  free_memory(simplChild2);
	  free(simplified);
	  simplified = simplChild1;
	} else {
	  simplified->nodeType = ADD;
	  simplified->child1 = simplChild1;
	  simplified->child2 = simplChild2;
	}
      }
    }
    break;
  case SUB:
    simplChild1 = simplifyAllButDivisionInner(tree->child1);
    simplChild2 = simplifyAllButDivisionInner(tree->child2);
    simplified = (node*) safeMalloc(sizeof(node));
    if ((simplChild1->nodeType == CONSTANT) && (simplChild2->nodeType == CONSTANT)) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_sub(*value, *(simplChild1->value), *(simplChild2->value), GMP_RNDN);
      free_memory(simplChild1);
      free_memory(simplChild2);
    } else {
      if ((simplChild1->nodeType == CONSTANT) && (mpfr_zero_p(*(simplChild1->value)))) {
	free_memory(simplChild1);
	simplified->nodeType = NEG;
	simplified->child1 = simplChild2;
      } else {
	if ((simplChild2->nodeType == CONSTANT) && (mpfr_zero_p(*(simplChild2->value)))) {
	  free_memory(simplChild2);
	  free(simplified);
	  simplified = simplChild1;
	} else {
	  simplified->nodeType = SUB;
	  simplified->child1 = simplChild1;
	  simplified->child2 = simplChild2;
	}
      }
    }
    break;
  case MUL:
    simplChild1 = simplifyAllButDivisionInner(tree->child1);
    simplChild2 = simplifyAllButDivisionInner(tree->child2);
    simplified = (node*) safeMalloc(sizeof(node));
    if ((simplChild1->nodeType == CONSTANT) && (simplChild2->nodeType == CONSTANT)) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_mul(*value, *(simplChild1->value), *(simplChild2->value), GMP_RNDN);
      free_memory(simplChild1);
      free_memory(simplChild2);
    } else {
      if (((simplChild1->nodeType == CONSTANT) && (mpfr_zero_p(*(simplChild1->value)))) ||
	  ((simplChild2->nodeType == CONSTANT) && (mpfr_zero_p(*(simplChild2->value))))) {
	free_memory(simplChild1);
	free_memory(simplChild2);
	simplified->nodeType = CONSTANT;
	value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*value,tools_precision);
	simplified->value = value;
	mpfr_set_d(*value,0.0,GMP_RNDN);
      } else {
	if ((simplChild1->nodeType == CONSTANT) && (mpfr_cmp_d(*(simplChild1->value),1.0) == 0) && (!mpfr_nan_p(*(simplChild1->value)))) {
	  free_memory(simplChild1);
	  free(simplified);
	  simplified = simplChild2;
	} else {
	  if ((simplChild2->nodeType == CONSTANT) && (mpfr_cmp_d(*(simplChild2->value),1.0) == 0) && (!mpfr_nan_p(*(simplChild2->value)))) {
	    free_memory(simplChild2);
	    free(simplified);
	    simplified = simplChild1;
	  } else {
	    simplified->nodeType = MUL;
	    simplified->child1 = simplChild1;
	    simplified->child2 = simplChild2;
	  }
	}
      }
    }
    break;
  case DIV:
    simplChild1 = simplifyAllButDivisionInner(tree->child1);
    simplChild2 = simplifyAllButDivisionInner(tree->child2);
    simplified = (node*) safeMalloc(sizeof(node));
    simplified->nodeType = DIV;
    simplified->child1 = simplChild1;
    simplified->child2 = simplChild2;
    break;
  case SQRT:
    simplChild1 = simplifyAllButDivisionInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_sqrt(*value, *(simplChild1->value), GMP_RNDN);
      free_memory(simplChild1);
    } else {
      simplified->nodeType = SQRT;
      simplified->child1 = simplChild1;
    }
    break;
  case EXP:
    simplChild1 = simplifyAllButDivisionInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_exp(*value, *(simplChild1->value), GMP_RNDN);
      free_memory(simplChild1);
    } else {
      simplified->nodeType = EXP;
      simplified->child1 = simplChild1;
    }
    break;
  case LOG:
    simplChild1 = simplifyAllButDivisionInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_log(*value, *(simplChild1->value), GMP_RNDN);
      free_memory(simplChild1);
    } else {
      simplified->nodeType = LOG;
      simplified->child1 = simplChild1;
    }
    break;
  case LOG_2:
    simplChild1 = simplifyAllButDivisionInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_log2(*value, *(simplChild1->value), GMP_RNDN);
      free_memory(simplChild1);
    } else {
      simplified->nodeType = LOG_2;
      simplified->child1 = simplChild1;
    }
    break;
  case LOG_10:
    simplChild1 = simplifyAllButDivisionInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_log10(*value, *(simplChild1->value), GMP_RNDN);
      free_memory(simplChild1);
    } else {
      simplified->nodeType = LOG_10;
      simplified->child1 = simplChild1;
    }
    break;
  case SIN:
    simplChild1 = simplifyAllButDivisionInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_sin(*value, *(simplChild1->value), GMP_RNDN);
      free_memory(simplChild1);
    } else {
      simplified->nodeType = SIN;
      simplified->child1 = simplChild1;
    }
    break;
  case COS:
    simplChild1 = simplifyAllButDivisionInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_cos(*value, *(simplChild1->value), GMP_RNDN);
      free_memory(simplChild1);
    } else {
      simplified->nodeType = COS;
      simplified->child1 = simplChild1;
    }
    break;
  case TAN:
    simplChild1 = simplifyAllButDivisionInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_tan(*value, *(simplChild1->value), GMP_RNDN);
      free_memory(simplChild1);
    } else {
      simplified->nodeType = TAN;
      simplified->child1 = simplChild1;
    }
    break;
  case ASIN:
    simplChild1 = simplifyAllButDivisionInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_asin(*value, *(simplChild1->value), GMP_RNDN);
      free_memory(simplChild1);
    } else {
      simplified->nodeType = ASIN;
      simplified->child1 = simplChild1;
    }
    break;
  case ACOS:
    simplChild1 = simplifyAllButDivisionInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_acos(*value, *(simplChild1->value), GMP_RNDN);
      free_memory(simplChild1);
    } else {
      simplified->nodeType = ACOS;
      simplified->child1 = simplChild1;
    }
    break;
  case ATAN:
    simplChild1 = simplifyAllButDivisionInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_atan(*value, *(simplChild1->value), GMP_RNDN);
      free_memory(simplChild1);
    } else {
      simplified->nodeType = ATAN;
      simplified->child1 = simplChild1;
    }
    break;
  case SINH:
    simplChild1 = simplifyAllButDivisionInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_sinh(*value, *(simplChild1->value), GMP_RNDN);
      free_memory(simplChild1);
    } else {
      simplified->nodeType = SINH;
      simplified->child1 = simplChild1;
    }
    break;
  case COSH:
    simplChild1 = simplifyAllButDivisionInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_cosh(*value, *(simplChild1->value), GMP_RNDN);
      free_memory(simplChild1);
    } else {
      simplified->nodeType = COSH;
      simplified->child1 = simplChild1;
    }
    break;
  case TANH:
    simplChild1 = simplifyAllButDivisionInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_tanh(*value, *(simplChild1->value), GMP_RNDN);
      free_memory(simplChild1);
    } else {
      simplified->nodeType = TANH;
      simplified->child1 = simplChild1;
    }
    break;
  case ASINH:
    simplChild1 = simplifyAllButDivisionInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_asinh(*value, *(simplChild1->value), GMP_RNDN);
      free_memory(simplChild1);
    } else {
      simplified->nodeType = ASINH;
      simplified->child1 = simplChild1;
    }
    break;
  case ACOSH:
    simplChild1 = simplifyAllButDivisionInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_acosh(*value, *(simplChild1->value), GMP_RNDN);
      free_memory(simplChild1);
    } else {
      simplified->nodeType = ACOSH;
      simplified->child1 = simplChild1;
    }
    break;
  case ATANH:
    simplChild1 = simplifyAllButDivisionInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_atanh(*value, *(simplChild1->value), GMP_RNDN);
      free_memory(simplChild1);
    } else {
      simplified->nodeType = ATANH;
      simplified->child1 = simplChild1;
    }
    break;
  case POW:
    simplChild1 = simplifyAllButDivisionInner(tree->child1);
    simplChild2 = simplifyAllButDivisionInner(tree->child2);
    simplified = (node*) safeMalloc(sizeof(node));
    if ((simplChild1->nodeType == CONSTANT) && (simplChild2->nodeType == CONSTANT)) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_pow(*value, *(simplChild1->value), *(simplChild2->value), GMP_RNDN);
      free_memory(simplChild1);
      free_memory(simplChild2);
    } else {
      if ((simplChild2->nodeType == CONSTANT) && (mpfr_cmp_d(*(simplChild2->value),1.0) == 0) && (!mpfr_nan_p(*(simplChild2->value)))) {
	free(simplified);
	free_memory(simplChild2);
	simplified = simplChild1;
      } else {
	simplified->nodeType = POW;
	simplified->child1 = simplChild1;
	simplified->child2 = simplChild2;
      }
    }
    break;
  case NEG:
    simplChild1 = simplifyAllButDivisionInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_neg(*value, *(simplChild1->value), GMP_RNDN);
      free_memory(simplChild1);
    } else {
      simplified->nodeType = NEG;
      simplified->child1 = simplChild1;
    }
    break;
  case ABS:
    simplChild1 = simplifyAllButDivisionInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_abs(*value, *(simplChild1->value), GMP_RNDN);
      free_memory(simplChild1);
    } else {
      simplified->nodeType = ABS;
      simplified->child1 = simplChild1;
    }
    break;
  case DOUBLE:
    simplChild1 = simplifyAllButDivisionInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_round_to_double(*value, *(simplChild1->value));
      free_memory(simplChild1);
    } else {
      simplified->nodeType = DOUBLE;
      simplified->child1 = simplChild1;
    }
    break;
  case SINGLE:
    simplChild1 = simplifyAllButDivisionInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_round_to_single(*value, *(simplChild1->value));
      free_memory(simplChild1);
    } else {
      simplified->nodeType = SINGLE;
      simplified->child1 = simplChild1;
    }
    break;
  case HALFPRECISION:
    simplChild1 = simplifyAllButDivisionInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_round_to_halfprecision(*value, *(simplChild1->value));
      free_memory(simplChild1);
    } else {
      simplified->nodeType = HALFPRECISION;
      simplified->child1 = simplChild1;
    }
    break;
  case QUAD:
    simplChild1 = simplifyAllButDivisionInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_round_to_quad(*value, *(simplChild1->value));
      free_memory(simplChild1);
    } else {
      simplified->nodeType = QUAD;
      simplified->child1 = simplChild1;
    }
    break;
  case DOUBLEDOUBLE:
    simplChild1 = simplifyAllButDivisionInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_round_to_doubledouble(*value, *(simplChild1->value));
      free_memory(simplChild1);
    } else {
      simplified->nodeType = DOUBLEDOUBLE;
      simplified->child1 = simplChild1;
    }
    break;
  case TRIPLEDOUBLE:
    simplChild1 = simplifyAllButDivisionInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_round_to_tripledouble(*value, *(simplChild1->value));
      free_memory(simplChild1);
    } else {
      simplified->nodeType = TRIPLEDOUBLE;
      simplified->child1 = simplChild1;
    }
    break;
  case ERF:
    simplChild1 = simplifyAllButDivisionInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_erf(*value, *(simplChild1->value), GMP_RNDN);
      free_memory(simplChild1);
    } else {
      simplified->nodeType = ERF;
      simplified->child1 = simplChild1;
    }
    break;
  case ERFC:
    simplChild1 = simplifyAllButDivisionInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_erfc(*value, *(simplChild1->value), GMP_RNDN);
      free_memory(simplChild1);
    } else {
      simplified->nodeType = ERFC;
      simplified->child1 = simplChild1;
    }
    break;
  case LOG_1P:
    simplChild1 = simplifyAllButDivisionInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_log1p(*value, *(simplChild1->value), GMP_RNDN);
      free_memory(simplChild1);
    } else {
      simplified->nodeType = LOG_1P;
      simplified->child1 = simplChild1;
    }
    break;
  case EXP_M1:
    simplChild1 = simplifyAllButDivisionInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_expm1(*value, *(simplChild1->value), GMP_RNDN);
      free_memory(simplChild1);
    } else {
      simplified->nodeType = EXP_M1;
      simplified->child1 = simplChild1;
    }
    break;
  case DOUBLEEXTENDED:
    simplChild1 = simplifyAllButDivisionInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_round_to_doubleextended(*value, *(simplChild1->value));
      free_memory(simplChild1);
    } else {
      simplified->nodeType = DOUBLEEXTENDED;
      simplified->child1 = simplChild1;
    }
    break;
  case LIBRARYFUNCTION:
    simplChild1 = simplifyAllButDivisionInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_from_mpfi(*value, *(simplChild1->value), tree->libFunDeriv, tree->libFun->code);
      free_memory(simplChild1);
    } else {
      simplified->nodeType = LIBRARYFUNCTION;
      simplified->child1 = simplChild1;
      simplified->libFun = tree->libFun;
      simplified->libFunDeriv = tree->libFunDeriv;
    }
    break;
  case PROCEDUREFUNCTION:
    simplChild1 = simplifyAllButDivisionInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      computeFunctionWithProcedureMpfr(*value, tree->child2, *(simplChild1->value), (unsigned int) tree->libFunDeriv);
      free_memory(simplChild1);
    } else {
      simplified->nodeType = PROCEDUREFUNCTION;
      simplified->child1 = simplChild1;
      simplified->child2 = copyThing(tree->child2);
      simplified->libFunDeriv = tree->libFunDeriv;
    }
    break;
  case CEIL:
    simplChild1 = simplifyAllButDivisionInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_ceil(*value, *(simplChild1->value));
      free_memory(simplChild1);
    } else {
      simplified->nodeType = CEIL;
      simplified->child1 = simplChild1;
    }
    break;
  case FLOOR:
    simplChild1 = simplifyAllButDivisionInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_floor(*value, *(simplChild1->value));
      free_memory(simplChild1);
    } else {
      simplified->nodeType = FLOOR;
      simplified->child1 = simplChild1;
    }
    break;
  case NEARESTINT:
    simplChild1 = simplifyAllButDivisionInner(tree->child1);
    simplified = (node*) safeMalloc(sizeof(node));
    if (simplChild1->nodeType == CONSTANT) {
      simplified->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      simplified->value = value;
      mpfr_nearestint(*value, *(simplChild1->value));
      free_memory(simplChild1);
    } else {
      simplified->nodeType = NEARESTINT;
      simplified->child1 = simplChild1;
    }
    break;
  case PI_CONST:
    simplified = (node*) safeMalloc(sizeof(node));
    simplified->nodeType = CONSTANT;
    value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
    mpfr_init2(*value,tools_precision);
    mpfr_const_pi(*value,GMP_RNDN);
    simplified->value = value;
    break;
  case LIBRARYCONSTANT:
    simplified = (node*) safeMalloc(sizeof(node));
    simplified->nodeType = CONSTANT;
    value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
    mpfr_init2(*value,tools_precision);
    sollya_mpfi_init2(tempI, tools_precision);
    libraryConstantToInterval(tempI, tree);
    sollya_mpfi_get_fr(*value, tempI);
    sollya_mpfi_clear(tempI);
    simplified->value = value;
    break;
  default:
    sollyaFprintf(stderr,"Error: simplifyAllButDivisionInner: unknown identifier (%d) in the tree\n",tree->nodeType);
    exit(1);
  }

  return simplified;
}


node *simplifyTree(node *tree) {
  node *temp, *temp2;

  temp = simplifyTreeErrorfree(tree);
  temp2 = simplifyTreeInner(temp);
  free_memory(temp);

  return temp2;
}

node *simplifyAllButDivision(node *tree) {
  node *temp, *temp2;

  temp = simplifyTreeErrorfree(tree);
  temp2 = simplifyAllButDivisionInner(temp);
  free_memory(temp);
  return temp2;
}


void evaluate(mpfr_t result, node *tree, mpfr_t x, mp_prec_t prec) {
  mpfr_t stack1, stack2;
  sollya_mpfi_t stackI;

  mpfr_init2(stack1, prec);
  mpfr_init2(stack2, prec);

  switch (tree->nodeType) {
  case VARIABLE:
    mpfr_set(result, x, GMP_RNDN);
    break;
  case CONSTANT:
    mpfr_set(result, *(tree->value), GMP_RNDN);
    break;
  case ADD:
    evaluate(stack1, tree->child1, x, prec);
    evaluate(stack2, tree->child2, x, prec);
    mpfr_add(result, stack1, stack2, GMP_RNDN);
    break;
  case SUB:
    evaluate(stack1, tree->child1, x, prec);
    evaluate(stack2, tree->child2, x, prec);
    mpfr_sub(result, stack1, stack2, GMP_RNDN);
    break;
  case MUL:
    evaluate(stack1, tree->child1, x, prec);
    evaluate(stack2, tree->child2, x, prec);
    mpfr_mul(result, stack1, stack2, GMP_RNDN);
    break;
  case DIV:
    evaluate(stack1, tree->child1, x, prec);
    evaluate(stack2, tree->child2, x, prec);
    mpfr_div(result, stack1, stack2, GMP_RNDN);
    break;
  case SQRT:
    evaluate(stack1, tree->child1, x, prec);
    mpfr_sqrt(result, stack1, GMP_RNDN);
    break;
  case EXP:
    evaluate(stack1, tree->child1, x, prec);
    mpfr_exp(result, stack1, GMP_RNDN);
    break;
  case LOG:
    evaluate(stack1, tree->child1, x, prec);
    mpfr_log(result, stack1, GMP_RNDN);
    break;
  case LOG_2:
    evaluate(stack1, tree->child1, x, prec);
    mpfr_log2(result, stack1, GMP_RNDN);
    break;
  case LOG_10:
    evaluate(stack1, tree->child1, x, prec);
    mpfr_log10(result, stack1, GMP_RNDN);
    break;
  case SIN:
    evaluate(stack1, tree->child1, x, prec);
    mpfr_sin(result, stack1, GMP_RNDN);
    break;
  case COS:
    evaluate(stack1, tree->child1, x, prec);
    mpfr_cos(result, stack1, GMP_RNDN);
    break;
  case TAN:
    evaluate(stack1, tree->child1, x, prec);
    mpfr_tan(result, stack1, GMP_RNDN);
    break;
  case ASIN:
    evaluate(stack1, tree->child1, x, prec);
    mpfr_asin(result, stack1, GMP_RNDN);
    break;
  case ACOS:
    evaluate(stack1, tree->child1, x, prec);
    mpfr_acos(result, stack1, GMP_RNDN);
    break;
  case ATAN:
    evaluate(stack1, tree->child1, x, prec);
    mpfr_atan(result, stack1, GMP_RNDN);
    break;
  case SINH:
    evaluate(stack1, tree->child1, x, prec);
    mpfr_sinh(result, stack1, GMP_RNDN);
    break;
  case COSH:
    evaluate(stack1, tree->child1, x, prec);
    mpfr_cosh(result, stack1, GMP_RNDN);
    break;
  case TANH:
    evaluate(stack1, tree->child1, x, prec);
    mpfr_tanh(result, stack1, GMP_RNDN);
    break;
  case ASINH:
    evaluate(stack1, tree->child1, x, prec);
    mpfr_asinh(result, stack1, GMP_RNDN);
    break;
  case ACOSH:
    evaluate(stack1, tree->child1, x, prec);
    mpfr_acosh(result, stack1, GMP_RNDN);
    break;
  case ATANH:
    evaluate(stack1, tree->child1, x, prec);
    mpfr_atanh(result, stack1, GMP_RNDN);
    break;
  case POW:
    evaluate(stack1, tree->child1, x, prec);
    evaluate(stack2, tree->child2, x, prec);
    mpfr_pow(result, stack1, stack2, GMP_RNDN);
    break;
  case NEG:
    evaluate(stack1, tree->child1, x, prec);
    mpfr_neg(result, stack1, GMP_RNDN);
    break;
  case ABS:
    evaluate(stack1, tree->child1, x, prec);
    mpfr_abs(result, stack1, GMP_RNDN);
    break;
  case DOUBLE:
    evaluate(stack1, tree->child1, x, prec);
    mpfr_round_to_double(result, stack1);
    break;
  case SINGLE:
    evaluate(stack1, tree->child1, x, prec);
    mpfr_round_to_single(result, stack1);
    break;
  case QUAD:
    evaluate(stack1, tree->child1, x, prec);
    mpfr_round_to_quad(result, stack1);
    break;
  case HALFPRECISION:
    evaluate(stack1, tree->child1, x, prec);
    mpfr_round_to_halfprecision(result, stack1);
    break;
  case DOUBLEDOUBLE:
    evaluate(stack1, tree->child1, x, prec);
    mpfr_round_to_doubledouble(result, stack1);
    break;
  case TRIPLEDOUBLE:
    evaluate(stack1, tree->child1, x, prec);
    mpfr_round_to_tripledouble(result, stack1);
    break;
  case ERF:
    evaluate(stack1, tree->child1, x, prec);
    mpfr_erf(result, stack1, GMP_RNDN);
    break;
  case ERFC:
    evaluate(stack1, tree->child1, x, prec);
    mpfr_erfc(result, stack1, GMP_RNDN);
    break;
  case LOG_1P:
    evaluate(stack1, tree->child1, x, prec);
    mpfr_log1p(result, stack1, GMP_RNDN);
    break;
  case EXP_M1:
    evaluate(stack1, tree->child1, x, prec);
    mpfr_expm1(result, stack1, GMP_RNDN);
    break;
  case DOUBLEEXTENDED:
    evaluate(stack1, tree->child1, x, prec);
    mpfr_round_to_doubleextended(result, stack1);
    break;
  case LIBRARYFUNCTION:
    evaluate(stack1, tree->child1, x, prec);
    mpfr_from_mpfi(result, stack1, tree->libFunDeriv, tree->libFun->code);
    break;
  case PROCEDUREFUNCTION:
    evaluate(stack1, tree->child1, x, prec);
    computeFunctionWithProcedureMpfr(result, tree->child2, stack1, (unsigned int) tree->libFunDeriv);
    break;
  case CEIL:
    evaluate(stack1, tree->child1, x, prec);
    mpfr_ceil(result, stack1);
    break;
  case FLOOR:
    evaluate(stack1, tree->child1, x, prec);
    mpfr_floor(result, stack1);
    break;
  case NEARESTINT:
    evaluate(stack1, tree->child1, x, prec);
    mpfr_nearestint(result, stack1);
    break;
  case PI_CONST:
    mpfr_const_pi(result, GMP_RNDN);
    break;
  case LIBRARYCONSTANT:
    sollya_mpfi_init2(stackI, mpfr_get_prec(result));
    libraryConstantToInterval(stackI, tree);
    sollya_mpfi_get_fr(result, stackI);
    sollya_mpfi_clear(stackI);
  default:
    sollyaFprintf(stderr,"Error: evaluate: unknown identifier in the tree\n");
    exit(1);
  }

  mpfr_clear(stack1); mpfr_clear(stack2);
  return;
}


int arity(node *tree) {
  switch (tree->nodeType) {
  case CONSTANT:
  case PI_CONST:
  case LIBRARYCONSTANT:
    return 0;
    break;

  case ADD:
  case SUB:
  case MUL:
  case DIV:
  case POW:
    return 2;
    break;

  case VARIABLE:
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
  case NEG:
  case ABS:
  case DOUBLE:
  case SINGLE:
  case QUAD:
  case HALFPRECISION:
  case DOUBLEDOUBLE:
  case TRIPLEDOUBLE:
  case ERF:
  case ERFC:
  case LOG_1P:
  case EXP_M1:
  case DOUBLEEXTENDED:
  case LIBRARYFUNCTION:
  case PROCEDUREFUNCTION:
    return 1;
    break;
  case CEIL:
  case FLOOR:
  case NEARESTINT:
    return 1;
    break;

 default:
    sollyaFprintf(stderr,"Error: arity: unknown identifier in the tree\n");
    exit(1);
  }
}


int isSyntacticallyEqual(node *tree1, node *tree2) {

  if (tree1->nodeType != tree2->nodeType) return 0;
  if (tree1->nodeType == PI_CONST) return 1;
  if ((tree1->nodeType == LIBRARYFUNCTION) && 
      ((tree1->libFun != tree2->libFun) ||
       (tree1->libFunDeriv != tree2->libFunDeriv))) return 0;
  if (tree1->nodeType == LIBRARYCONSTANT) 
      return (tree1->libFun == tree2->libFun);
  if ((tree1->nodeType == PROCEDUREFUNCTION) && 
      ((!isEqualThing(tree1->child2, tree2->child2)) ||
       (tree1->libFunDeriv != tree2->libFunDeriv))) return 0;
  if (tree1->nodeType == CONSTANT) {
    if (mpfr_equal_p(*(tree1->value),*(tree2->value))) 
      return 1;
    else 
      return 0;
  }
  if (tree1->nodeType == VARIABLE) return 1;

  if (arity(tree1) == 1) {
    if (!isSyntacticallyEqual(tree1->child1,tree2->child1)) return 0;
  } else {
    if (!isSyntacticallyEqual(tree1->child1,tree2->child1)) return 0;
    if (!isSyntacticallyEqual(tree1->child2,tree2->child2)) return 0;
  }

  return 1;
}



int isConstant(node *tree);

int isPolynomial(node *tree) {
  int res;
  node *temp;
  
  if (isConstant(tree)) return 1;

  switch (tree->nodeType) {
  case VARIABLE:
    res = 1;
    break;
  case CONSTANT:
    res = 1;
    break;
  case ADD:
    res = isPolynomial(tree->child1) && isPolynomial(tree->child2);
    break;
  case SUB:
    res = isPolynomial(tree->child1) && isPolynomial(tree->child2);
    break;
  case MUL:
    res = isPolynomial(tree->child1) && isPolynomial(tree->child2);
    break;
  case DIV:
    res = isPolynomial(tree->child1) && isConstant(tree->child2);
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
  case DOUBLE:
  case SINGLE:
  case QUAD:
  case HALFPRECISION:
  case DOUBLEDOUBLE:
  case TRIPLEDOUBLE:
  case ERF:
  case ERFC:
  case LOG_1P:
  case EXP_M1:
  case DOUBLEEXTENDED:
  case LIBRARYFUNCTION:
  case CEIL:
  case FLOOR:
  case NEARESTINT:
    res = 0;
    break;
 
  case POW:
    {
      res = 0;
      if (isConstant(tree)) {
	res = 1;
      } else {
	if (isPolynomial(tree->child1)) {
	  if (tree->child2->nodeType == CONSTANT) 
	    temp = tree->child2; 
	  else 
	    temp = simplifyTreeErrorfree(tree->child2);
	  if (temp->nodeType == CONSTANT) {
	    if (mpfr_integer_p(*(temp->value))) {
	      if (mpfr_sgn(*(temp->value)) >= 0) {
		res = 1;
	      }
	    }
	  }
	  if (tree->child2->nodeType != CONSTANT) free_memory(temp);
	}
      }
    }
    break;
  case NEG:
    res = isPolynomial(tree->child1);
    break;

  case PROCEDUREFUNCTION:
    res = 0;
    break;
  case PI_CONST:
  case LIBRARYCONSTANT:
    res = 1;
    break;
  default:
    sollyaFprintf(stderr,"Error: isPolynomial: unknown identifier in the tree\n");
    exit(1);
  }
 return res;
}

int isAffine(node *tree) {
  int res;
  node *temp;

  switch (tree->nodeType) {
  case VARIABLE:
    res = 1;
    break;
  case CONSTANT:
    res = 1;
    break;
  case ADD:
    res = isAffine(tree->child1) && isAffine(tree->child2);
    break;
  case SUB:
    res = isAffine(tree->child1) && isAffine(tree->child2);
    break;
  case MUL:
    res = isAffine(tree->child1) && isAffine(tree->child2);
    break;

  case DIV:
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
  case DOUBLE:
  case SINGLE:
  case QUAD:
  case HALFPRECISION:
  case DOUBLEDOUBLE:
  case TRIPLEDOUBLE:
  case ERF:
  case ERFC:
  case LOG_1P:
  case EXP_M1:
  case DOUBLEEXTENDED:
  case LIBRARYFUNCTION:
  case CEIL:
  case FLOOR:
  case NEARESTINT:
    res = 0;
    break;

  case POW:
    {
      res = 0;
      if (isAffine(tree->child1)) {
        if (tree->child2->nodeType == CONSTANT) 
          temp = tree->child2; 
        else 
          temp = simplifyTreeErrorfree(tree->child2);
        if (temp->nodeType == CONSTANT) {
          if (mpfr_number_p(*(temp->value)) && mpfr_integer_p(*(temp->value))) {
            if (mpfr_sgn(*(temp->value)) > 0) {
              res = 1;
            }
          }
        }
        if (tree->child2->nodeType != CONSTANT) free_memory(temp);
      }
    }
    break;
  case NEG:
    res = isAffine(tree->child1);
    break;

  case PROCEDUREFUNCTION:
    res = 0;
    break;
  case PI_CONST:
  case LIBRARYCONSTANT:
    res = 1;
    break;
  default:
    sollyaFprintf(stderr,"Error: isAffine: unknown identifier in the tree\n");
    exit(1);
  }
 return res;
}


#define MAX_MACRO(a,b) (a) > (b) ? (a) : (b)
#define MIN_MACRO(a,b) (a) < (b) ? (a) : (b)

int getDegreeUnsafe(node *tree) {
  int l, r;
  mpfr_t temp;
  node *simplifiedExponent;

  if (isConstant(tree)) return 0;

  switch (tree->nodeType) {
  case VARIABLE:
    return 1;
    break;
  case CONSTANT:
  case PI_CONST:
    return 0;
    break;
  case ADD:
    l = getDegreeUnsafe(tree->child1);
    r = getDegreeUnsafe(tree->child2);
    return MAX_MACRO(l,r);
    break;
  case SUB:
    l = getDegreeUnsafe(tree->child1);
    r = getDegreeUnsafe(tree->child2);
    return MAX_MACRO(l,r);
    break;
  case MUL:
    l = getDegreeUnsafe(tree->child1);
    r = getDegreeUnsafe(tree->child2);
    return l + r;
    break;
  case DIV:
    return getDegreeUnsafe(tree->child1);
    break;
  case POW:
    {
      l = getDegreeUnsafe(tree->child1);
      if (tree->child2->nodeType != CONSTANT) {
        simplifiedExponent = simplifyRationalErrorfree(tree->child2);
        if ((simplifiedExponent->nodeType == CONSTANT) && 
            mpfr_integer_p(*(simplifiedExponent->value)) &&
            (mpfr_sgn(*(simplifiedExponent->value)) >= 0)) {
          r = mpfr_get_si(*(simplifiedExponent->value),GMP_RNDN);
          mpfr_init2(temp,mpfr_get_prec(*(simplifiedExponent->value)) + 10);
          mpfr_set_si(temp,r,GMP_RNDN);
          if (mpfr_cmp(*(simplifiedExponent->value),temp) != 0) {
            printMessage(1, "Warning: tried to compute polynomial degree of an expression using a power operator with an exponent");
            printMessage(1," which cannot be represented on an integer variable.\n");
            mpfr_clear(temp);
            free_memory(simplifiedExponent);
            return -1;
          }
          mpfr_clear(temp);
          free_memory(simplifiedExponent);
          return l * r;
        } else {
          sollyaFprintf(stderr,"Error: getDegreeUnsafe: an error occurred. The exponent in a power operator is not constant, not integer or not non-negative.\n");
          exit(1);
        }
        free_memory(simplifiedExponent);
      } else {
        if (!mpfr_integer_p(*(tree->child2->value))) {
          sollyaFprintf(stderr,"Error: getDegreeUnsafe: an error occurred. The exponent in a power operator is not integer.\n");
          exit(1);
        }
        if (mpfr_sgn(*(tree->child2->value)) < 0) {
          sollyaFprintf(stderr,"Error: getDegreeUnsafe: an error occurred. The exponent in a power operator is negative.\n");
          exit(1);
        }

        r = mpfr_get_si(*(tree->child2->value),GMP_RNDN);
        mpfr_init2(temp,mpfr_get_prec(*(tree->child2->value)) + 10);
        mpfr_set_si(temp,r,GMP_RNDN);
        if (mpfr_cmp(*(tree->child2->value),temp) != 0) {
          printMessage(1, "Warning: tried to compute polynomial degree of an expression using a power operator with an exponent");
          printMessage(1," which cannot be represented on an integer variable.\n");
          mpfr_clear(temp);
          return -1;
        }
        mpfr_clear(temp);
        return l * r;
      }
    }
    break;
  case NEG:
    return getDegreeUnsafe(tree->child1);
    break;
  default:
    sollyaFprintf(stderr,"Error: getDegreeUnsafe: an error occurred on handling the expression tree\n");
    exit(1);
  }
}

int getDegree(node *tree) {
  if (!isPolynomial(tree)) return -1;
  return getDegreeUnsafe(tree);
}

int getMaxPowerDividerUnsafe(node *tree) {
  int l, r;
  mpfr_t temp;

  if (isConstant(tree)) return 0;

  switch (tree->nodeType) {
  case VARIABLE:
    return 1;
    break;
  case CONSTANT:
  case PI_CONST:
    return 0;
    break;
  case ADD:
    l = getMaxPowerDividerUnsafe(tree->child1);
    r = getMaxPowerDividerUnsafe(tree->child2);
    return MIN_MACRO(l,r);
    break;
  case SUB:
    l = getMaxPowerDividerUnsafe(tree->child1);
    r = getMaxPowerDividerUnsafe(tree->child2);
    return MIN_MACRO(l,r);
    break;
  case MUL:
    l = getMaxPowerDividerUnsafe(tree->child1);
    r = getMaxPowerDividerUnsafe(tree->child2);
    return l + r;
    break;
  case DIV:
    return getMaxPowerDividerUnsafe(tree->child1);
    break;
  case POW:
    {
      l = getMaxPowerDividerUnsafe(tree->child1);
      if (tree->child2->nodeType != CONSTANT) {
	sollyaFprintf(stderr,"Error: getMaxPowerDividerUnsafe: an error occurred. The exponent in a power operator is not constant.\n");
	exit(1);
      }
      if (!mpfr_integer_p(*(tree->child2->value))) {
	sollyaFprintf(stderr,"Error: getMaxPowerDividerUnsafe: an error occurred. The exponent in a power operator is not integer.\n");
	exit(1);
      }
      if (mpfr_sgn(*(tree->child2->value)) < 0) {
	sollyaFprintf(stderr,"Error: getMaxPowerDividerUnsafe: an error occurred. The exponent in a power operator is negative.\n");
	exit(1);
      }

      r = mpfr_get_si(*(tree->child2->value),GMP_RNDN);
      mpfr_init2(temp,mpfr_get_prec(*(tree->child2->value)) + 10);
      mpfr_set_si(temp,r,GMP_RNDN);
      if (mpfr_cmp(*(tree->child2->value),temp) != 0) {
	printMessage(1,
"Warning: tried to compute degree of maximal polynomial divider of a polynomial in an expression using a power operator with an exponent ");
	printMessage(1,"which cannot be represented on an integer variable.\n");
	mpfr_clear(temp);
	return -1;
      }
      mpfr_clear(temp);
      return l * r;
    }
    break;
  case NEG:
    return getMaxPowerDividerUnsafe(tree->child1);
    break;
  default:
    sollyaFprintf(stderr,"Error: getMaxPowerDividerUnsafe: an error occurred on handling the expression tree\n");
    exit(1);
  }
}

int getMaxPowerDivider(node *tree) {
  if (!isPolynomial(tree)) return -1;
  return getMaxPowerDividerUnsafe(tree);
}





node* makeBinomial(node *a, node *b, int n, int s) {
  node *tree, *coeff, *aPow, *bPow, *tempNode, *tempNode2;
  mpfr_t *coeffVal, *mpfr_temp;
  unsigned int i;
  mpz_t coeffGMP;
  mp_prec_t prec;
  
  tree = (node*) safeMalloc(sizeof(node));
  tree->nodeType = CONSTANT;
  mpfr_temp = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
  mpfr_init2(*mpfr_temp,tools_precision);
  mpfr_set_d(*mpfr_temp,0.0,GMP_RNDN);
  tree->value = mpfr_temp;
  mpz_init(coeffGMP);
  for (i=0;i<=((unsigned int) n);i++) {
    mpz_bin_uiui(coeffGMP,(unsigned int) n,i);
    prec = mpz_sizeinbase (coeffGMP, 2) + 10;
    if (prec < tools_precision) prec = tools_precision;
    coeffVal = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
    mpfr_init2(*coeffVal,prec);
    if(mpfr_set_z(*coeffVal,coeffGMP,GMP_RNDN) != 0) {
      if (!noRoundingWarnings) {
	printMessage(1,"Warning: on expanding a power operator a rounding occurred when calculating a binomial coefficient.\n");
	printMessage(1,"Try to increase the working precision.\n");
      }
    }
    if ((s < 0) && (((((unsigned int) n) - i) & 1) != 0)) { /* This is a modulo 2 to determine eveness */
      mpfr_neg(*coeffVal,*coeffVal,GMP_RNDN);
    }
    coeff = (node*) safeMalloc(sizeof(node));
    coeff->nodeType = CONSTANT;
    coeff->value = coeffVal;
    aPow = (node*) safeMalloc(sizeof(node));
    aPow->nodeType = POW;
    aPow->child1 = copyTree(a);
    tempNode = (node*) safeMalloc(sizeof(node));
    tempNode->nodeType = CONSTANT;
    mpfr_temp = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
    mpfr_init2(*mpfr_temp,tools_precision);
    if(mpfr_set_ui(*mpfr_temp,i,GMP_RNDN) != 0) {
      if (!noRoundingWarnings) {
	printMessage(1,"Warning: on expanding a power operator a rounding occurred when calculating an exponent constant.\n");
	printMessage(1,"Try to increase the working precision.\n");
      }
    }
    tempNode->value = mpfr_temp;
    aPow->child2 = tempNode;
    bPow = (node*) safeMalloc(sizeof(node));
    bPow->nodeType = POW;
    bPow->child1 = copyTree(b);
    tempNode = (node*) safeMalloc(sizeof(node));
    tempNode->nodeType = CONSTANT;
    mpfr_temp = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
    mpfr_init2(*mpfr_temp,tools_precision);
    if(mpfr_set_ui(*mpfr_temp,((unsigned int) n) - i,GMP_RNDN) != 0) {
      printMessage(1,"Warning: on expanding a power operator a rounding occurred when calculating an exponent constant.\n");
      printMessage(1,"Try to increase the working precision.\n");
    }
    tempNode->value = mpfr_temp;
    bPow->child2 = tempNode;
    tempNode = (node*) safeMalloc(sizeof(node));
    tempNode->nodeType = MUL;
    tempNode->child1 = coeff;
    tempNode->child2 = aPow;
    tempNode2 = (node*) safeMalloc(sizeof(node));
    tempNode2->nodeType = MUL;
    tempNode2->child1 = tempNode;
    tempNode2->child2 = bPow;
    tempNode = (node*) safeMalloc(sizeof(node));
    tempNode->nodeType = ADD;
    tempNode->child1 = tree;
    tempNode->child2 = tempNode2;
    tree = tempNode;
  }
  mpz_clear(coeffGMP);

  return tree;
}


node* expandPowerInPolynomialUnsafe(node *tree) {
  node *copy, *left, *tempTree, *tempTree2, *tempTree3;
  int r, i;
  mpfr_t temp;
  mpfr_t *value;

  if (isConstant(tree)) return copyTree(tree);

  switch (tree->nodeType) {
  case VARIABLE:
    return copyTree(tree);
    break;
  case CONSTANT:
  case PI_CONST:
    return copyTree(tree);
    break;
  case ADD:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ADD;
    copy->child1 = expandPowerInPolynomialUnsafe(tree->child1);
    copy->child2 = expandPowerInPolynomialUnsafe(tree->child2);
    return copy;
    break;
  case SUB:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = SUB;
    copy->child1 = expandPowerInPolynomialUnsafe(tree->child1);
    copy->child2 = expandPowerInPolynomialUnsafe(tree->child2);
    return copy;    
    break;
  case MUL:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = MUL;
    copy->child1 = expandPowerInPolynomialUnsafe(tree->child1);
    copy->child2 = expandPowerInPolynomialUnsafe(tree->child2);
    return copy;
    break;
  case DIV:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = DIV;
    copy->child1 = expandPowerInPolynomialUnsafe(tree->child1);
    copy->child2 = expandPowerInPolynomialUnsafe(tree->child2);
    return copy;
    break;
  case POW:
    {
      left = expandPowerInPolynomialUnsafe(tree->child1);
      if (tree->child2->nodeType != CONSTANT) {
	sollyaFprintf(stderr,"Error: expandPowerInPolynomialUnsafe: an error occurred. The exponent in a power operator is not constant.\n");
	exit(1);
      }
      if (!mpfr_integer_p(*(tree->child2->value))) {
	sollyaFprintf(stderr,"Error: expandPowerInPolynomialUnsafe: an error occurred. The exponent in a power operator is not integer.\n");
	exit(1);
      }
      if (mpfr_sgn(*(tree->child2->value)) < 0) {
	sollyaFprintf(stderr,"Error: expandPowerInPolynomialUnsafe: an error occurred. The exponent in a power operator is negative.\n");
	exit(1);
      }

      r = mpfr_get_si(*(tree->child2->value),GMP_RNDN);
      mpfr_init2(temp,mpfr_get_prec(*(tree->child2->value)) + 10);
      mpfr_set_si(temp,r,GMP_RNDN);
      if (mpfr_cmp(*(tree->child2->value),temp) != 0) {
	sollyaFprintf(stderr,"Error: expandPowerInPolynomialUnsafe: an error occurred. Tried to expand an expression using a power operator with an exponent ");
	sollyaFprintf(stderr,"which cannot be represented on an integer variable.\n");
	mpfr_clear(temp);
	exit(1);
      }
      mpfr_clear(temp);
      if (r > 1) {
	switch (left->nodeType) {
	case VARIABLE:
	case CONSTANT:
	case PI_CONST:
	  tempTree = copyTree(left);
	  for (i=1;i<r;i++) {
	    tempTree2 = (node *) safeMalloc(sizeof(node));
	    tempTree2->nodeType = MUL;
	    tempTree2->child1 = tempTree;
	    tempTree2->child2 = copyTree(left);
	    tempTree = tempTree2;
	  }
	  break;
	case ADD:
	  tempTree = makeBinomial(left->child1,left->child2,r,1);
	  break;
	case SUB:
	  tempTree = makeBinomial(left->child1,left->child2,r,-1);
	  break;
	case MUL:
	  tempTree = (node *) safeMalloc(sizeof(node));
	  tempTree->nodeType = MUL;
	  tempTree2 = (node *) safeMalloc(sizeof(node));
	  tempTree2->nodeType = POW;
	  tempTree2->child1 = copyTree(left->child1);
	  tempTree3 = (node*) safeMalloc(sizeof(node));
	  tempTree3->nodeType = CONSTANT;
	  value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	  mpfr_init2(*value,tools_precision);
	  mpfr_set_si(*value,r,GMP_RNDN);
	  tempTree3->value = value;
	  tempTree2->child2 = tempTree3;
	  tempTree->child1 = tempTree2;
	  tempTree2 = (node *) safeMalloc(sizeof(node));
	  tempTree2->nodeType = POW;
	  tempTree2->child1 = copyTree(left->child2);
	  tempTree3 = (node*) safeMalloc(sizeof(node));
	  tempTree3->nodeType = CONSTANT;
	  value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	  mpfr_init2(*value,tools_precision);
	  mpfr_set_si(*value,r,GMP_RNDN);
	  tempTree3->value = value;
	  tempTree2->child2 = tempTree3;
	  tempTree->child2 = tempTree2;
	  break;
	case DIV:
	  tempTree = (node *) safeMalloc(sizeof(node));
	  tempTree->nodeType = DIV;
	  tempTree2 = (node *) safeMalloc(sizeof(node));
	  tempTree2->nodeType = POW;
	  tempTree2->child1 = copyTree(left->child1);
	  tempTree3 = (node*) safeMalloc(sizeof(node));
	  tempTree3->nodeType = CONSTANT;
	  value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	  mpfr_init2(*value,tools_precision);
	  mpfr_set_si(*value,r,GMP_RNDN);
	  tempTree3->value = value;
	  tempTree2->child2 = tempTree3;
	  tempTree->child1 = tempTree2;
	  tempTree2 = (node *) safeMalloc(sizeof(node));
	  tempTree2->nodeType = POW;
	  tempTree2->child1 = copyTree(left->child2);
	  tempTree3 = (node*) safeMalloc(sizeof(node));
	  tempTree3->nodeType = CONSTANT;
	  value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	  mpfr_init2(*value,tools_precision);
	  mpfr_set_si(*value,r,GMP_RNDN);
	  tempTree3->value = value;
	  tempTree2->child2 = tempTree3;
	  tempTree->child2 = tempTree2;
	  break;
	case NEG:
	  if (r & 1) { /* This is a modulo 2 to determine eveness, not a logical test */
	    /* r is odd */
	    tempTree = (node *) safeMalloc(sizeof(node));
	    tempTree->nodeType = NEG;
	    tempTree2 = (node *) safeMalloc(sizeof(node));
	    tempTree2->nodeType = POW;
	    tempTree2->child1 = copyTree(left->child1);
	    tempTree3 = (node*) safeMalloc(sizeof(node));
	    tempTree3->nodeType = CONSTANT;
	    value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	    mpfr_init2(*value,tools_precision);
	    mpfr_set_si(*value,r,GMP_RNDN);
	    tempTree3->value = value;
	    tempTree2->child2 = tempTree3;
	    tempTree->child1 = tempTree2;
	  } else {
	    tempTree = (node *) safeMalloc(sizeof(node));
	    tempTree->nodeType = POW;
	    tempTree->child1 = copyTree(left->child1);
	    tempTree3 = (node*) safeMalloc(sizeof(node));
	    tempTree3->nodeType = CONSTANT;
	    value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	    mpfr_init2(*value,tools_precision);
	    mpfr_set_si(*value,r,GMP_RNDN);
	    tempTree3->value = value;
	    tempTree->child2 = tempTree3;
	  }
	  break;
	default:
	  if (isConstant(left)) return copyTree(tree);

	  sollyaFprintf(stderr,"Error: expandPowerInPolynomialUnsafe: an error occurred on handling the expanded expression subtree\n");
	  exit(1);
	}
	copy = expandPowerInPolynomialUnsafe(tempTree);
	free_memory(tempTree);
      } else {
	if (r == 1) {
	  copy = copyTree(left);
	} else {
	  copy = (node*) safeMalloc(sizeof(node));
	  copy->nodeType = CONSTANT;
	  value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	  mpfr_init2(*value,tools_precision);
	  mpfr_set_d(*value,1.0,GMP_RNDN);
	  copy->value = value;
	}
      }
      free_memory(left);
      return copy;
    }
    break;
  case NEG:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = NEG;
    copy->child1 = expandPowerInPolynomialUnsafe(tree->child1);
    return copy;
    break;
  default:

    if (isConstant(tree)) return copyTree(tree);

    sollyaFprintf(stderr,"Error: expandPowerInPolynomialUnsafe: an error occurred on handling the expression tree\n");
    exit(1);
  }
}

node* expandPowerInPolynomial(node *tree) {
  if (getDegree(tree) < 0) return copyTree(tree);
  return expandPowerInPolynomialUnsafe(tree);
}



node* expandDivision(node *tree) {
  node *copy, *left, *right, *tempNode;
  mpfr_t *value;
  mpfr_t temp;

  switch (tree->nodeType) {
  case VARIABLE:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = VARIABLE;
    break;
  case CONSTANT:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = CONSTANT;
    value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
    mpfr_init2(temp,tools_precision);
    simplifyMpfrPrec(temp,*(tree->value));
    mpfr_init2(*value,mpfr_get_prec(temp));
    mpfr_set(*value,temp,GMP_RNDN);
    mpfr_clear(temp);
    copy->value = value;
    break;
  case ADD:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ADD;
    copy->child1 = expandDivision(tree->child1);
    copy->child2 = expandDivision(tree->child2);
    break;
  case SUB:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = SUB;
    copy->child1 = expandDivision(tree->child1);
    copy->child2 = expandDivision(tree->child2);
    break;
  case MUL:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = MUL;
    copy->child1 = expandDivision(tree->child1);
    copy->child2 = expandDivision(tree->child2);
    break;
  case DIV:
    left = expandDivision(tree->child1);
    right = expandDivision(tree->child2);
    if (right->nodeType == DIV) {
      tempNode = right->child1;
      right->child1 = right->child2;
      right->child2 = tempNode;
      copy = (node*) safeMalloc(sizeof(node));
      copy->nodeType = MUL;
      copy->child1 = left;
      copy->child2 = right;
    } else {
      copy = (node*) safeMalloc(sizeof(node));
      copy->nodeType = DIV;
      copy->child1 = left;
      copy->child2 = right;
    }
    break;
  case SQRT:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = SQRT;
    copy->child1 = expandDivision(tree->child1);
    break;
  case EXP:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = EXP;
    copy->child1 = expandDivision(tree->child1);
    break;
  case LOG:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = LOG;
    copy->child1 = expandDivision(tree->child1);
    break;
  case LOG_2:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = LOG_2;
    copy->child1 = expandDivision(tree->child1);
    break;
  case LOG_10:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = LOG_10;
    copy->child1 = expandDivision(tree->child1);
    break;
  case SIN:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = SIN;
    copy->child1 = expandDivision(tree->child1);
    break;
  case COS:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = COS;
    copy->child1 = expandDivision(tree->child1);
    break;
  case TAN:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = TAN;
    copy->child1 = expandDivision(tree->child1);
    break;
  case ASIN:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ASIN;
    copy->child1 = expandDivision(tree->child1);
    break;
  case ACOS:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ACOS;
    copy->child1 = expandDivision(tree->child1);
    break;
  case ATAN:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ATAN;
    copy->child1 = expandDivision(tree->child1);
    break;
  case SINH:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = SINH;
    copy->child1 = expandDivision(tree->child1);
    break;
  case COSH:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = COSH;
    copy->child1 = expandDivision(tree->child1);
    break;
  case TANH:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = TANH;
    copy->child1 = expandDivision(tree->child1);
    break;
  case ASINH:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ASINH;
    copy->child1 = expandDivision(tree->child1);
    break;
  case ACOSH:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ACOSH;
    copy->child1 = expandDivision(tree->child1);
    break;
  case ATANH:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ATANH;
    copy->child1 = expandDivision(tree->child1);
    break;
  case POW:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = POW;
    copy->child1 = expandDivision(tree->child1);
    copy->child2 = expandDivision(tree->child2);
    break;
  case NEG:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = NEG;
    copy->child1 = expandDivision(tree->child1);
    break;
  case ABS:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ABS;
    copy->child1 = expandDivision(tree->child1);
    break;
  case DOUBLE:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = DOUBLE;
    copy->child1 = expandDivision(tree->child1);
    break;
  case SINGLE:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = SINGLE;
    copy->child1 = expandDivision(tree->child1);
    break;
  case QUAD:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = QUAD;
    copy->child1 = expandDivision(tree->child1);
    break;
  case HALFPRECISION:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = HALFPRECISION;
    copy->child1 = expandDivision(tree->child1);
    break;
  case DOUBLEDOUBLE:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = DOUBLEDOUBLE;
    copy->child1 = expandDivision(tree->child1);
    break;
  case TRIPLEDOUBLE:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = TRIPLEDOUBLE;
    copy->child1 = expandDivision(tree->child1);
    break;
  case ERF:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ERF;
    copy->child1 = expandDivision(tree->child1);
    break;
  case ERFC:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ERFC;
    copy->child1 = expandDivision(tree->child1);
    break;
  case LOG_1P:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = LOG_1P;
    copy->child1 = expandDivision(tree->child1);
    break;
  case EXP_M1:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = EXP_M1;
    copy->child1 = expandDivision(tree->child1);
    break;
  case DOUBLEEXTENDED:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = DOUBLEEXTENDED;
    copy->child1 = expandDivision(tree->child1);
    break;
  case LIBRARYFUNCTION:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = LIBRARYFUNCTION;
    copy->libFun = tree->libFun;
    copy->libFunDeriv = tree->libFunDeriv;
    copy->child1 = expandDivision(tree->child1);
    break;
  case PROCEDUREFUNCTION:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = PROCEDUREFUNCTION;
    copy->libFunDeriv = tree->libFunDeriv;
    copy->child1 = expandDivision(tree->child1);
    copy->child2 = copyThing(tree->child2);
    break;
  case CEIL:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = CEIL;
    copy->child1 = expandDivision(tree->child1);
    break;
  case FLOOR:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = FLOOR;
    copy->child1 = expandDivision(tree->child1);
    break;
  case NEARESTINT:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = NEARESTINT;
    copy->child1 = expandDivision(tree->child1);
    break;
  case PI_CONST:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = PI_CONST;
    break;
  case LIBRARYCONSTANT:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = LIBRARYCONSTANT;
    copy->libFun = tree->libFun;
    break;

  default:
   sollyaFprintf(stderr,"Error: expandDivision: unknown identifier in the tree\n");
   exit(1);
  }
  return copy;
}



node* expandPolynomialUnsafe(node *tree) {
  node *left, *right, *copy, *tempNode, *tempNode2, *tempNode3, *tempNode4; 
  mpfr_t *value;

  if (isConstant(tree)) return copyTree(tree);

  switch (tree->nodeType) {
  case VARIABLE:
    return copyTree(tree);
    break;
  case CONSTANT:
  case PI_CONST:
    return copyTree(tree);
    break;
  case ADD:
    left = expandPolynomialUnsafe(tree->child1);
    right = expandPolynomialUnsafe(tree->child2);
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ADD;
    copy->child1 = left;
    copy->child2 = right;
    return copy;    
    break;
  case SUB:
    left = expandPolynomialUnsafe(tree->child1);
    right = expandPolynomialUnsafe(tree->child2);
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = SUB;
    copy->child1 = left;
    copy->child2 = right;
    return copy;
    break;
  case MUL:
    left = expandPolynomialUnsafe(tree->child1);
    right = expandPolynomialUnsafe(tree->child2);
    switch (left->nodeType) {
    case VARIABLE:
    case CONSTANT:
    case PI_CONST:
      if (isConstant(right)) {
	copy = (node*) safeMalloc(sizeof(node));
	copy->nodeType = MUL;
	copy->child1 = left;
	copy->child2 = right;
      } else {
	switch (right->nodeType) {
	case VARIABLE:
	case CONSTANT:
	case PI_CONST:
	  copy = (node*) safeMalloc(sizeof(node));
	  copy->nodeType = MUL;
	  copy->child1 = left;
	  copy->child2 = right;
	  break;  
	default:
	  tempNode = (node*) safeMalloc(sizeof(node));
	  tempNode->nodeType = MUL;
	  tempNode->child1 = right;
	  tempNode->child2 = left;
	  copy = expandPolynomialUnsafe(tempNode);
	  free_memory(tempNode);
	}
      }
      break;
    case MUL:
      switch (right->nodeType) {
      case ADD:
      case SUB:
      case NEG:
      case DIV:      
	tempNode = (node*) safeMalloc(sizeof(node));
	tempNode->nodeType = MUL;
	tempNode->child1 = right;
	tempNode->child2 = left;
	copy = expandPolynomialUnsafe(tempNode);
	free_memory(tempNode);
	break;
      default:
	copy = (node*) safeMalloc(sizeof(node));
	copy->nodeType = MUL;
	copy->child1 = left;
	copy->child2 = right;
	break;  
      }
      break;  
    case ADD:
      tempNode = (node*) safeMalloc(sizeof(node));
      tempNode->nodeType = ADD;
      tempNode2 = (node*) safeMalloc(sizeof(node));
      tempNode2->nodeType = MUL;
      tempNode3 = (node*) safeMalloc(sizeof(node));
      tempNode3->nodeType = MUL;
      tempNode2->child1 = copyTree(left->child1);
      tempNode2->child2 = copyTree(right);
      tempNode3->child1 = copyTree(left->child2);
      tempNode3->child2 = right;
      tempNode->child1 = tempNode2;
      tempNode->child2 = tempNode3;
      free_memory(left);
      copy = expandPolynomialUnsafe(tempNode);
      free_memory(tempNode);
      break;
    case SUB:
      tempNode = (node*) safeMalloc(sizeof(node));
      tempNode->nodeType = SUB;
      tempNode2 = (node*) safeMalloc(sizeof(node));
      tempNode2->nodeType = MUL;
      tempNode3 = (node*) safeMalloc(sizeof(node));
      tempNode3->nodeType = MUL;
      tempNode2->child1 = copyTree(left->child1);
      tempNode2->child2 = copyTree(right);
      tempNode3->child1 = copyTree(left->child2);
      tempNode3->child2 = right;
      tempNode->child1 = tempNode2;
      tempNode->child2 = tempNode3;
      free_memory(left);
      copy = expandPolynomialUnsafe(tempNode);
      free_memory(tempNode);      
      break;
    case NEG:
      tempNode = (node*) safeMalloc(sizeof(node));
      tempNode->nodeType = NEG;
      tempNode2 = (node*) safeMalloc(sizeof(node));
      tempNode2->nodeType = MUL;
      tempNode2->child1 = copyTree(left->child1);
      tempNode2->child2 = right;
      tempNode->child1 = tempNode2;
      free_memory(left);
      copy = expandPolynomialUnsafe(tempNode);
      free_memory(tempNode);      
      break;
    case DIV:
      if (isConstant(left)) {
	if (isConstant(right)) {
	  copy = (node*) safeMalloc(sizeof(node));
	  copy->nodeType = MUL;
	  copy->child1 = left;
	  copy->child2 = right;
	} else {
	  switch (right->nodeType) {
	  case ADD:
	  case SUB:
	  case NEG:
	  case DIV:      
	    tempNode = (node*) safeMalloc(sizeof(node));
	    tempNode->nodeType = MUL;
	    tempNode->child1 = right;
	    tempNode->child2 = left;
	    copy = expandPolynomialUnsafe(tempNode);
	    free_memory(tempNode);
	    break;
	  default:
	    copy = (node*) safeMalloc(sizeof(node));
	    copy->nodeType = MUL;
	    copy->child1 = left;
	    copy->child2 = right;
	    break;  
	  }  
	}
      } else {
	tempNode = (node*) safeMalloc(sizeof(node));
	tempNode->nodeType = MUL;
	tempNode2 = (node*) safeMalloc(sizeof(node));
	tempNode2->nodeType = MUL;
	tempNode3 = (node*) safeMalloc(sizeof(node));
	tempNode3->nodeType = DIV;
	tempNode4 = (node*) safeMalloc(sizeof(node));
	tempNode4->nodeType = CONSTANT;
	value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*value,tools_precision);
	mpfr_set_d(*value,1.0,GMP_RNDN);
	tempNode4->value = value;
	tempNode3->child1 = tempNode4;
	tempNode3->child2 = copyTree(left->child2);
	tempNode2->child1 = copyTree(left->child1);
	tempNode2->child2 = right;
	tempNode->child1 = tempNode3;
	tempNode->child2 = tempNode2;
	free_memory(left);
	copy = expandPolynomialUnsafe(tempNode);
	free_memory(tempNode);
      }      
      break;
    default:
      if (isConstant(left)) {
	if (isConstant(right)) {
	  return copyTree(tree);
	} else {
	  tempNode = (node*) safeMalloc(sizeof(node));
	  tempNode->nodeType = MUL;
	  tempNode->child1 = right;
	  tempNode->child2 = left;
	  copy = expandPolynomialUnsafe(tempNode);
	  free_memory(tempNode);
	  return copy;
	}
      } else {
	sollyaFprintf(stderr,"Error: expandPolynomialUnsafe: an error occurred on handling the MUL left rewritten expression subtree\n");
	exit(1);
      }
    }
    return copy;
    break;
  case DIV:
    left = expandPolynomialUnsafe(tree->child1);
    right = expandPolynomialUnsafe(tree->child2);
    switch (left->nodeType) {
    case CONSTANT:
    case PI_CONST:
      copy = (node*) safeMalloc(sizeof(node));
      copy->nodeType = DIV;
      copy->child1 = left;
      copy->child2 = right;
      break;  
    case VARIABLE:
      copy = (node*) safeMalloc(sizeof(node));
      copy->nodeType = MUL;
      tempNode = (node*) safeMalloc(sizeof(node));
      tempNode->nodeType = DIV;
      tempNode->child2 = right;
      tempNode2 = (node*) safeMalloc(sizeof(node));
      tempNode2->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      mpfr_set_d(*value,1.0,GMP_RNDN);
      tempNode2->value = value;
      tempNode->child1 = tempNode2;
      copy->child2 = left;
      copy->child1 = tempNode;
      break;
    case ADD:
      tempNode = (node*) safeMalloc(sizeof(node));
      tempNode->nodeType = ADD;
      tempNode2 = (node*) safeMalloc(sizeof(node));
      tempNode2->nodeType = DIV;
      tempNode3 = (node*) safeMalloc(sizeof(node));
      tempNode3->nodeType = DIV;
      tempNode2->child1 = copyTree(left->child1);
      tempNode2->child2 = copyTree(right);
      tempNode3->child1 = copyTree(left->child2);
      tempNode3->child2 = right;
      tempNode->child1 = tempNode2;
      tempNode->child2 = tempNode3;
      free_memory(left);
      copy = expandPolynomialUnsafe(tempNode);
      free_memory(tempNode);
      break;
    case SUB:
      tempNode = (node*) safeMalloc(sizeof(node));
      tempNode->nodeType = SUB;
      tempNode2 = (node*) safeMalloc(sizeof(node));
      tempNode2->nodeType = DIV;
      tempNode3 = (node*) safeMalloc(sizeof(node));
      tempNode3->nodeType = DIV;
      tempNode2->child1 = copyTree(left->child1);
      tempNode2->child2 = copyTree(right);
      tempNode3->child1 = copyTree(left->child2);
      tempNode3->child2 = right;
      tempNode->child1 = tempNode2;
      tempNode->child2 = tempNode3;
      free_memory(left);
      copy = expandPolynomialUnsafe(tempNode);
      free_memory(tempNode);      
      break;
    case MUL:
      tempNode = (node*) safeMalloc(sizeof(node));
      tempNode->nodeType = MUL;
      tempNode2 = (node*) safeMalloc(sizeof(node));
      tempNode2->nodeType = DIV;
      tempNode3 = (node*) safeMalloc(sizeof(node));
      tempNode3->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      mpfr_set_d(*value,1.0,GMP_RNDN);
      tempNode3->value = value;
      tempNode4 = (node*) safeMalloc(sizeof(node));
      tempNode4->nodeType = MUL;
      tempNode2->child1 = tempNode3;
      tempNode2->child2 = right;
      tempNode->child1 = tempNode2;
      tempNode->child2 = tempNode4;
      tempNode4->child1 = copyTree(left->child1);
      tempNode4->child2 = copyTree(left->child2);
      free_memory(left);
      copy = expandPolynomialUnsafe(tempNode);
      free_memory(tempNode);      
      break;
    case DIV:
      tempNode = (node*) safeMalloc(sizeof(node));
      tempNode->nodeType = DIV;
      tempNode->child1 = copyTree(left->child1);
      tempNode2 = (node*) safeMalloc(sizeof(node));
      tempNode2->nodeType = MUL;
      tempNode2->child1 = copyTree(left->child2);
      tempNode2->child2 = right;
      tempNode->child2 = tempNode2;
      free_memory(left);
      copy = expandPolynomialUnsafe(tempNode);
      free_memory(tempNode);      
      break;
    case NEG:
      tempNode = (node*) safeMalloc(sizeof(node));
      tempNode->nodeType = NEG;
      tempNode2 = (node*) safeMalloc(sizeof(node));
      tempNode2->nodeType = DIV;
      tempNode2->child1 = copyTree(left->child1);
      tempNode2->child2 = right;
      tempNode->child1 = tempNode2;
      free_memory(left);
      copy = expandPolynomialUnsafe(tempNode);
      free_memory(tempNode);      
      break;
    default: 
      if (isConstant(left)) {
	return copyTree(tree);
      } else {
	sollyaFprintf(stderr,"Error: expandPolynomialUnsafe: an error occurred on handling the DIV left rewritten expression subtree\n");
	exit(1);
      }
    }
    return copy;
    break;
  case NEG:
    left = expandPolynomialUnsafe(tree->child1);
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = NEG;
    copy->child1 = left;
    return copy;
    break;
  default:
    if (isConstant(tree)) {
      return copyTree(tree);
    } else {
      sollyaFprintf(stderr,"Error: expandPolynomialUnsafe: an error occurred on handling the expression tree\n");
      exit(1);
    }
  }
}



node* expandPolynomial(node *tree) {
  node *temp, *temp2;
  if (getDegree(tree) < 0) return copyTree(tree);
  temp = expandPowerInPolynomialUnsafe(tree);
  temp2 = expandPolynomialUnsafe(temp);
  free_memory(temp);
  return temp2;
}

node* expandUnsimplified(node *tree) {
  node *copy;
  mpfr_t *value;
  mpfr_t temp;

  if (!isConstant(tree) && isPolynomial(tree)) return expandPolynomial(tree);

  switch (tree->nodeType) {
  case VARIABLE:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = VARIABLE;
    break;
  case CONSTANT:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = CONSTANT;
    value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
    mpfr_init2(temp,tools_precision);
    simplifyMpfrPrec(temp,*(tree->value));
    mpfr_init2(*value,mpfr_get_prec(temp));
    mpfr_set(*value,temp,GMP_RNDN);
    mpfr_clear(temp);
    copy->value = value;
    break;
  case ADD:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ADD;
    copy->child1 = expand(tree->child1);
    copy->child2 = expand(tree->child2);
    break;
  case SUB:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = SUB;
    copy->child1 = expand(tree->child1);
    copy->child2 = expand(tree->child2);
    break;
  case MUL:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = MUL;
    copy->child1 = expand(tree->child1);
    copy->child2 = expand(tree->child2);
    break;
  case DIV:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = DIV;
    copy->child1 = expand(tree->child1);
    copy->child2 = expand(tree->child2);
    break;
  case SQRT:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = SQRT;
    copy->child1 = expand(tree->child1);
    break;
  case EXP:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = EXP;
    copy->child1 = expand(tree->child1);
    break;
  case LOG:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = LOG;
    copy->child1 = expand(tree->child1);
    break;
  case LOG_2:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = LOG_2;
    copy->child1 = expand(tree->child1);
    break;
  case LOG_10:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = LOG_10;
    copy->child1 = expand(tree->child1);
    break;
  case SIN:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = SIN;
    copy->child1 = expand(tree->child1);
    break;
  case COS:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = COS;
    copy->child1 = expand(tree->child1);
    break;
  case TAN:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = TAN;
    copy->child1 = expand(tree->child1);
    break;
  case ASIN:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ASIN;
    copy->child1 = expand(tree->child1);
    break;
  case ACOS:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ACOS;
    copy->child1 = expand(tree->child1);
    break;
  case ATAN:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ATAN;
    copy->child1 = expand(tree->child1);
    break;
  case SINH:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = SINH;
    copy->child1 = expand(tree->child1);
    break;
  case COSH:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = COSH;
    copy->child1 = expand(tree->child1);
    break;
  case TANH:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = TANH;
    copy->child1 = expand(tree->child1);
    break;
  case ASINH:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ASINH;
    copy->child1 = expand(tree->child1);
    break;
  case ACOSH:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ACOSH;
    copy->child1 = expand(tree->child1);
    break;
  case ATANH:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ATANH;
    copy->child1 = expand(tree->child1);
    break;
  case POW:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = POW;
    copy->child1 = expand(tree->child1);
    copy->child2 = expand(tree->child2);
    break;
  case NEG:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = NEG;
    copy->child1 = expand(tree->child1);
    break;
  case ABS:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ABS;
    copy->child1 = expand(tree->child1);
    break;
  case DOUBLE:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = DOUBLE;
    copy->child1 = expand(tree->child1);
    break;
  case SINGLE:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = SINGLE;
    copy->child1 = expand(tree->child1);
    break;
  case HALFPRECISION:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = HALFPRECISION;
    copy->child1 = expand(tree->child1);
    break;
  case QUAD:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = QUAD;
    copy->child1 = expand(tree->child1);
    break;
  case DOUBLEDOUBLE:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = DOUBLEDOUBLE;
    copy->child1 = expand(tree->child1);
    break;
  case TRIPLEDOUBLE:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = TRIPLEDOUBLE;
    copy->child1 = expand(tree->child1);
    break;
  case ERF:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ERF;
    copy->child1 = expand(tree->child1);
    break;
  case ERFC:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ERFC;
    copy->child1 = expand(tree->child1);
    break;
  case LOG_1P:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = LOG_1P;
    copy->child1 = expand(tree->child1);
    break;
  case EXP_M1:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = EXP_M1;
    copy->child1 = expand(tree->child1);
    break;
  case DOUBLEEXTENDED:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = DOUBLEEXTENDED;
    copy->child1 = expand(tree->child1);
    break;
  case LIBRARYFUNCTION:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = LIBRARYFUNCTION;
    copy->libFun = tree->libFun;
    copy->libFunDeriv = tree->libFunDeriv;
    copy->child1 = expand(tree->child1);
    break;
  case PROCEDUREFUNCTION:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = PROCEDUREFUNCTION;
    copy->libFunDeriv = tree->libFunDeriv;
    copy->child1 = expand(tree->child1);
    copy->child2 = copyThing(tree->child2);
    break;
  case CEIL:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = CEIL;
    copy->child1 = expand(tree->child1);
    break;
  case FLOOR:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = FLOOR;
    copy->child1 = expand(tree->child1);
    break;
  case NEARESTINT:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = NEARESTINT;
    copy->child1 = expand(tree->child1);
    break;
  case PI_CONST:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = PI_CONST;
    break;
  case LIBRARYCONSTANT:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = LIBRARYCONSTANT;
    copy->libFun = tree->libFun;
    break;
  default:
   sollyaFprintf(stderr,"Error: expand: unknown identifier in the tree\n");
   exit(1);
  }
  return copy;
}

node* expand(node *tree) {
  node *temp, *temp2, *temp3;
  temp3 = expandDivision(tree);
  temp = expandUnsimplified(temp3);
  temp2 = simplifyTreeErrorfree(temp);
  free_memory(temp);
  free_memory(temp3);
  return temp2;
}


int isConstant(node *tree) {
  switch (tree->nodeType) {
  case VARIABLE:
    return 0;
    break;
  case CONSTANT:
  case PI_CONST:
  case LIBRARYCONSTANT:
    return 1;
    break;

  case ADD:
  case SUB:
  case MUL:
  case DIV:
  case POW:
    return (isConstant(tree->child1) & isConstant(tree->child2));
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
  case NEG:
  case ABS:
  case DOUBLE:
  case SINGLE:
  case QUAD:
  case HALFPRECISION:
  case DOUBLEDOUBLE:
  case TRIPLEDOUBLE:
  case ERF:
  case ERFC:
  case LOG_1P:
  case EXP_M1:
  case DOUBLEEXTENDED:
  case LIBRARYFUNCTION:
  case PROCEDUREFUNCTION:
  case CEIL:
  case FLOOR:
  case NEARESTINT:
    return isConstant(tree->child1);
    break;
  default:
   sollyaFprintf(stderr,"Error: isConstant: unknown identifier in the tree\n");
   exit(1);
  }
}


int isMonomial(node *tree) {

  switch (tree->nodeType) {
  case MUL:
    return (isMonomial(tree->child1) && isMonomial(tree->child2));
    break;
  case NEG:
    return isMonomial(tree->child1);
    break;
  case VARIABLE:
    return 1;
    break;
  case DIV:
    return (isConstant(tree->child2)) && isMonomial(tree->child1);
  default: 
    return isConstant(tree);
  }
  return 0;
}



node* getCoefficientsInMonomialUnsafe(node *polynom) {
  node *leftSub, *rightSub, *coeffs;
  mpfr_t *value;

  if (isConstant(polynom)) return copyTree(polynom);

  if (polynom->nodeType == VARIABLE) return NULL;
 
  if (polynom->nodeType == MUL) {
    leftSub = getCoefficientsInMonomialUnsafe(polynom->child1);
    rightSub = getCoefficientsInMonomialUnsafe(polynom->child2);
    if ((leftSub == NULL) && (rightSub == NULL)) return NULL;
    if (leftSub == NULL) return rightSub;
    if (rightSub == NULL) return leftSub;
    coeffs = (node*) safeMalloc(sizeof(node));
    coeffs->nodeType = MUL;
    coeffs->child1 = leftSub;
    coeffs->child2 = rightSub;
    return coeffs;
  }

  if (polynom->nodeType == DIV) {
    leftSub = getCoefficientsInMonomialUnsafe(polynom->child1);
    if (leftSub == NULL) {
      coeffs = (node*) safeMalloc(sizeof(node));
      coeffs->nodeType = DIV;
      coeffs->child1 = (node *) safeMalloc(sizeof(node));
      coeffs->child1->nodeType = CONSTANT;
      coeffs->child1->value = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*(coeffs->child1->value),10);
      mpfr_set_d(*(coeffs->child1->value),1.0,GMP_RNDN);
      coeffs->child2 = copyTree(polynom->child2);
    } else {
      coeffs = (node*) safeMalloc(sizeof(node));
      coeffs->nodeType = DIV;
      coeffs->child1 = leftSub;
      coeffs->child2 = copyTree(polynom->child2);
    }
    return coeffs;
  }


  if (polynom->nodeType == NEG) {
    leftSub = getCoefficientsInMonomialUnsafe(polynom->child1);
    rightSub = (node *) safeMalloc(sizeof(node));
    rightSub->nodeType = CONSTANT;
    value = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
    mpfr_init2(*value,tools_precision);
    mpfr_set_d(*value,-1.0,GMP_RNDN);
    rightSub->value = value;
    if (leftSub == NULL) return rightSub;
    coeffs = (node*) safeMalloc(sizeof(node));
    coeffs->nodeType = MUL;
    coeffs->child1 = leftSub;
    coeffs->child2 = rightSub;
    return coeffs;
  }

  sollyaFprintf(stderr,"Error: getCoefficientsInMonomialUnsafe: an error occurred. The expression does not have the correct monomial form.\n");
  exit(1);
  return NULL;
}


void getCoefficientsUnsafe(node **monomials, node *polynom, int sign) {
  int degree;
  node *temp, *coeff, *temp2;
  mpfr_t *value;
  node *simplified, *simplifiedTemp;

 
  if (isMonomial(polynom)) {
    degree = getDegree(polynom);
    coeff = getCoefficientsInMonomialUnsafe(polynom);
    if (coeff == NULL) {
      coeff = (node *) safeMalloc(sizeof(node));
      coeff->nodeType = CONSTANT;
      value = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      mpfr_set_d(*value,1.0,GMP_RNDN);
      coeff->value = value;
    }
    temp = monomials[degree];
    if (temp == NULL) {
      if (sign < 0) {
	temp2 = (node *) safeMalloc(sizeof(node));
	temp2->nodeType = NEG;
	temp2->child1 = coeff;
	coeff = temp2;
      }
      temp = coeff;
    } else {
      temp2 = (node*) safeMalloc(sizeof(node));
      if (sign > 0) temp2->nodeType = ADD; else temp2->nodeType = SUB;
      temp2->child1 = temp;
      temp2->child2 = coeff;
      temp = temp2;
    }
    monomials[degree] = temp;
    return;
  }

  if (polynom->nodeType == ADD) {
    getCoefficientsUnsafe(monomials,polynom->child1,sign);
    getCoefficientsUnsafe(monomials,polynom->child2,sign);
    return;
  }

  if (polynom->nodeType == SUB) {
    getCoefficientsUnsafe(monomials,polynom->child1,sign);
    getCoefficientsUnsafe(monomials,polynom->child2,-sign);
    return;
  }

  if (polynom->nodeType == NEG) {
    getCoefficientsUnsafe(monomials,polynom->child1,-sign);
    return;
  }  
 

  simplifiedTemp = expandPowerInPolynomialUnsafe(polynom);
  simplified = expandPolynomialUnsafe(simplifiedTemp);

  if (verbosity >= 7) {
    changeToWarningMode();
    printMessage(7,"Warning: recursion on coefficients extraction:\n");
    printTree(polynom);
    printMessage(7,"\ntransformed to\n");
    printTree(simplified); sollyaPrintf("\n");
    restoreMode();
  }

  getCoefficientsUnsafe(monomials, simplified, sign);  

  free_memory(simplifiedTemp);
  free_memory(simplified);

}

int isPowerOfVariable(node *);

void getCoefficientsHornerUnsafe(node **coefficients, node *poly, int offset, int sign) {
  int deg, newSign;
  node *newCoeff, *temp;

  if (isConstant(poly)) {
    newCoeff = copyTree(poly);
  } else {
    if (poly->nodeType == SUB) newSign = -1; else newSign = 1;
    newCoeff = copyTree(poly->child1);
    if ((poly->child2->nodeType == MUL) &&
	isConstant(poly->child2->child1) &&
	isPowerOfVariable(poly->child2->child2)) {
      deg = getDegree(poly->child2->child2);
      getCoefficientsHornerUnsafe(coefficients,poly->child2->child1,offset+deg,sign*newSign);
    } else {
      if ((poly->child2->nodeType == MUL) && 
	  (poly->child2->child1->nodeType == MUL) &&
	  isPowerOfVariable(poly->child2->child1->child1) &&
	  isConstant(poly->child2->child1->child2) &&
	  isConstant(poly->child2->child2)) {
	deg = getDegree(poly->child2->child1->child1);
	temp = (node *) safeMalloc(sizeof(node));
	temp->nodeType = MUL;
	temp->child1 = copyTree(poly->child2->child1->child2);
	temp->child2 = copyTree(poly->child2->child2);
	getCoefficientsHornerUnsafe(coefficients,temp,offset+deg,sign*newSign);
	free_memory(temp);
      } else {
	if (isPowerOfVariable(poly->child2)) {
	  deg = getDegree(poly->child2);
	  temp = (node *) safeMalloc(sizeof(node));
	  temp->nodeType = CONSTANT;
	  temp->value = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
	  mpfr_init2(*(temp->value),17);
	  mpfr_set_d(*(temp->value),1.0,GMP_RNDN);
	  getCoefficientsHornerUnsafe(coefficients,temp,offset+deg,sign*newSign);
	  free_memory(temp);
	} else {
	  deg = getDegree(poly->child2->child1);
	  getCoefficientsHornerUnsafe(coefficients,poly->child2->child2,offset+deg,sign*newSign);
	}
      }
    }
  }
  
  if (coefficients[offset] == NULL) {
    if (sign == -1) {
      temp = (node *) safeMalloc(sizeof(node));
      temp->nodeType = NEG;
      temp->child1 = newCoeff;
      coefficients[offset] = temp;
    } else {
      coefficients[offset] = newCoeff;
    }
  } else {
    temp = (node *) safeMalloc(sizeof(node));
    if (sign == 1) temp->nodeType = ADD; else temp->nodeType = SUB;
    temp->child1 = coefficients[offset];
    temp->child2 = newCoeff;
    coefficients[offset] = temp;
  }

}

void getCoefficientsHorner(node **coefficients, node *poly) {
  int offset;

  printMessage(7,"Information: extraction of coefficient terms from a polynomial uses a special algorithm for Horner forms.\n");

  if (poly->nodeType == MUL) {
    offset = getDegree(poly->child1);
    getCoefficientsHornerUnsafe(coefficients,poly->child2,offset,1);
    return;
  }
  getCoefficientsHornerUnsafe(coefficients,poly,0,1);
}

int isPowerOfVariable(node *);
int isCanonicalMonomial(node *);

void getCoefficientsCanonicalUnsafe(node **coefficients, node *poly) {
  int deg, sign;
  node *newCoeff, *temp;

  if (isConstant(poly)) {
    sign = 1;
    deg = 0;
    newCoeff = copyTree(poly);
  } else {
    if (isCanonicalMonomial(poly)) {
      deg = getDegree(poly);
      sign = 1;
      if (isPowerOfVariable(poly)) {
	  newCoeff = (node *) safeMalloc(sizeof(node));
	  newCoeff->nodeType = CONSTANT;
	  newCoeff->value = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
	  mpfr_init2(*(newCoeff->value),17);
	  mpfr_set_d(*(newCoeff->value),1.0,GMP_RNDN);
      } else {
	newCoeff = copyTree(poly->child1);
      }
    } else {
      getCoefficientsCanonicalUnsafe(coefficients,poly->child1);
      if (poly->nodeType == ADD) sign = 1; else sign = -1;
      if (isConstant(poly->child2)) {
	deg = 0;
	newCoeff = copyTree(poly->child2);
      } else {
	deg = getDegree(poly->child2);
	if (isPowerOfVariable(poly->child2)) {
	  newCoeff = (node *) safeMalloc(sizeof(node));
	  newCoeff->nodeType = CONSTANT;
	  newCoeff->value = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
	  mpfr_init2(*(newCoeff->value),17);
	  mpfr_set_d(*(newCoeff->value),1.0,GMP_RNDN);
	} else {
	  newCoeff = copyTree(poly->child2->child1);
	}
      }
    }
  }

  if (coefficients[deg] == NULL) {
    if (sign == -1) {
      temp = (node *) safeMalloc(sizeof(node));
      temp->nodeType = NEG;
      temp->child1 = newCoeff;
      coefficients[deg] = temp;
    } else {
      coefficients[deg] = newCoeff;
    }
  } else {
    temp = (node *) safeMalloc(sizeof(node));
    if (sign == 1) temp->nodeType = ADD; else temp->nodeType = SUB;
    temp->child1 = coefficients[deg];
    temp->child2 = newCoeff;
    coefficients[deg] = temp;
  }

}

void getCoefficientsCanonical(node **coefficients, node *poly) {

  printMessage(7,"Information: extraction of coefficient terms from a polynomial uses a special algorithm for canonical forms.\n");

  getCoefficientsCanonicalUnsafe(coefficients, poly);
}


int isHorner(node *);
int isCanonical(node *);
node *dividePolynomialByPowerOfVariableUnsafe(node *tree, int alpha);
node *makePowerOfConstant(node *constTree, int k);
node *makeBinomialCoefficient(unsigned int n, unsigned int k);

// Computes the coefficients of the polynomial p^k where p is 
// given by the coefficients in input
//
void computePowerOfPolynomialCoefficients(int *degreeRes, node ***coeffRes, 
                                          node **coeffs, int degree, int k) {
  int i, t;
  node **coeffsQ;
  int degreeQ;
  node *binom;
  node *constPow;
  node *factor;
  node *temp;

  *degreeRes = k * degree;
  *coeffRes = (node **) safeCalloc(*degreeRes+1,sizeof(node *));

  if (k == 0) {
    for (i=1;i<=*degreeRes;i++) {
      (*coeffRes)[i] = makeConstantDouble(0.0);
    }
    (*coeffRes)[0] = makeConstantDouble(1.0);
    return;
  }

  if (degree == 0) {
    for (i=1;i<=*degreeRes;i++) {
      (*coeffRes)[i] = makeConstantDouble(0.0);
    }
    if (coeffs[0] != NULL) {
      (*coeffRes)[0] = makePowerOfConstant(coeffs[0],k);
    } else {
      (*coeffRes)[0] = makeConstantDouble(0.0);
    }
    return;
  }

  if (k == 1) {
    for (i=1;i<=*degreeRes;i++) {
      if (coeffs[i] != NULL) {
        (*coeffRes)[i] = copyTree(coeffs[i]);
      } else {
        (*coeffRes)[i] = makeConstantDouble(0.0);
      }
    }
    return;
  }

  for (i=0;i<=*degreeRes;i++) {
    (*coeffRes)[i] = makeConstantDouble(0.0);
  }

  for (t=0;t<=k;t++) {
    if ((coeffs[0] != NULL) && 
        (!((coeffs[0]->nodeType == CONSTANT) &&
           (mpfr_zero_p(*(coeffs[0]->value)))))) {
      computePowerOfPolynomialCoefficients(&degreeQ, &coeffsQ, 
                                           &(coeffs[1]), degree - 1, t);
      binom = makeBinomialCoefficient(k, t);
      constPow = makePowerOfConstant(coeffs[0],k-t);
      factor = makeMul(binom,constPow);
      for (i=t;i<=t+degreeQ;i++) {
        if (coeffsQ[i-t] != NULL) {
          (*coeffRes)[i] = makeAdd((*coeffRes)[i],
                                   makeMul(copyTree(factor),
                                           coeffsQ[i-t]));
        }
      }
      free(coeffsQ);
      free_memory(factor);
    } else {
      if (k == t) {
        computePowerOfPolynomialCoefficients(&degreeQ, &coeffsQ, 
                                             &(coeffs[1]), degree - 1, t);
        factor = makeBinomialCoefficient(k, t);
        for (i=t;i<=t+degreeQ;i++) {
          if (coeffsQ[i-t] != NULL) {
            (*coeffRes)[i] = makeAdd((*coeffRes)[i],
                                     makeMul(copyTree(factor),
                                             coeffsQ[i-t]));
          }
        }
        free(coeffsQ);
        free_memory(factor);
      }
    }
  }

  for (i=0;i<=*degreeRes;i++) {
    if ((*coeffRes)[i] != NULL) {
      temp = simplifyTreeErrorfree((*coeffRes)[i]);
      free_memory((*coeffRes)[i]);
      (*coeffRes)[i] = temp;
    }
  }
}

void getCoefficients(int *degree, node ***coefficients, node *poly) {
  node *temp, *temp2, *temp3, *temp4;
  int i,k,j, mpd;
  node **coefficients1, **coefficients2;
  int degree1, degree2;
  mpfr_t y;

  *degree = getDegree(poly);
  if (*degree < 0) {
    printMessage(1,"Warning: Tried to get coefficients of an expression that is not a polynomial.\n");
    return;
  }

  *coefficients = (node**) safeCalloc((*degree + 1),sizeof(node*));
  for (i=0;i<=*degree;i++) (*coefficients)[i] = NULL;

  
  if (isHorner(poly)) {
    getCoefficientsHorner(*coefficients,poly);
    return;
  }

  if (isCanonical(poly)) {
    getCoefficientsCanonical(*coefficients,poly);
    return;
  }
  

  if ((poly->nodeType == ADD) || (poly->nodeType == SUB)) {
    getCoefficients(&degree1, &coefficients1, poly->child1);
    getCoefficients(&degree2, &coefficients2, poly->child2);
    for (i=0;i<=degree1;i++) {
      (*coefficients)[i] = coefficients1[i];
    }
    free(coefficients1);
    for (i=0;i<=degree2;i++) {
      if (coefficients2[i] != NULL) {
	if ((*coefficients)[i] == NULL) {
	  if (poly->nodeType == SUB) {
	    temp = (node *) safeMalloc(sizeof(node));
	    temp->nodeType = NEG;
	    temp->child1 = coefficients2[i];
	    (*coefficients)[i] = temp;
	  } else {
	    (*coefficients)[i] = coefficients2[i];
	  }
	  
	} else {
	  temp = (node *) safeMalloc(sizeof(node));
	  temp->nodeType = poly->nodeType;
	  temp->child1 = (*coefficients)[i];
	  temp->child2 = coefficients2[i];
	  (*coefficients)[i] = temp;
	}
      }
    }
    free(coefficients2);
    return;
  }

  if (poly->nodeType == MUL) {
    getCoefficients(&degree1, &coefficients1, poly->child1);
    getCoefficients(&degree2, &coefficients2, poly->child2);
    for (i=0;i<=degree1;i++) {
      for (k=0;k<=degree2;k++) {
	if ((coefficients1[i] != NULL) && (coefficients2[k] != NULL)) {
	  j = i + k;
	  temp = (node *) safeMalloc(sizeof(node));
	  temp->nodeType = MUL;
	  temp->child1 = copyTree(coefficients1[i]);
	  temp->child2 = copyTree(coefficients2[k]);
	  if ((*coefficients)[j] == NULL) {
	    (*coefficients)[j] = temp;
	  } else {
	    temp2 = (node *) safeMalloc(sizeof(node));
	    temp2->nodeType = ADD;
	    temp2->child1 = (*coefficients)[j];
	    temp2->child2 = temp;
	    (*coefficients)[j] = temp2;
	  }
	}
      }
    }
    for (i=0;i<=degree1;i++) free_memory(coefficients1[i]);
    for (i=0;i<=degree2;i++) free_memory(coefficients2[i]);
    free(coefficients1);
    free(coefficients2);
    return;
  }

  if ((poly->nodeType == POW) &&
      (poly->child2->nodeType == CONSTANT) &&
      (mpfr_integer_p(*(poly->child2->value)))) {
    k = mpfr_get_si(*(poly->child2->value),GMP_RNDN);
    mpfr_init2(y,8 * sizeof(int) + 10);
    mpfr_set_si(y,k,GMP_RNDN);
    if ((mpfr_cmp(y,*(poly->child2->value)) == 0) && (!mpfr_nan_p(*(poly->child2->value))) &&
	(k > 0)) {
      if ((mpd = getMaxPowerDivider(poly->child1)) > 0) {
        temp = dividePolynomialByPowerOfVariableUnsafe(poly->child1, mpd);
        temp2 = (node *) safeMalloc(sizeof(node));
        temp2->nodeType = POW;
        temp2->child1 = temp;
        temp2->child2 = copyTree(poly->child2);
        getCoefficients(&degree1, &coefficients1, temp2);
        free_memory(temp2);
        for (i=0;i<=degree1;i++) 
          (*coefficients)[i + k * mpd] = coefficients1[i];
        free(coefficients1);    
        mpfr_clear(y);
        return;
      }

      if (k == 2) {
        getCoefficients(&degree1, &coefficients1, poly->child1);
        getCoefficients(&degree2, &coefficients2, poly->child1);
        for (i=0;i<=degree1;i++) {
          for (k=0;k<=degree2;k++) {
            if ((coefficients1[i] != NULL) && (coefficients2[k] != NULL)) {
              j = i + k;
              temp = (node *) safeMalloc(sizeof(node));
              temp->nodeType = MUL;
              temp->child1 = copyTree(coefficients1[i]);
              temp->child2 = copyTree(coefficients2[k]);
              if ((*coefficients)[j] == NULL) {
                (*coefficients)[j] = temp;
              } else {
                temp2 = (node *) safeMalloc(sizeof(node));
                temp2->nodeType = ADD;
                temp2->child1 = (*coefficients)[j];
                temp2->child2 = temp;
                (*coefficients)[j] = temp2;
              }
            }
          }
        }
        for (i=0;i<=degree1;i++) free_memory(coefficients1[i]);
        for (i=0;i<=degree2;i++) free_memory(coefficients2[i]);
        free(coefficients1);
        free(coefficients2);
        mpfr_clear(y);
        return;
      }

      getCoefficients(&degree1, &coefficients1, poly->child1);
      for (i=0;i<=degree1;i++) {
        if (coefficients1[i] == NULL) 
          coefficients1[i] = makeConstantDouble(0.0);
      }

      computePowerOfPolynomialCoefficients(&degree2, &coefficients2, 
                                           coefficients1, degree1, 
                                           k);
      for (i=0;i<=degree2;i++) {
        if (coefficients2[i] != NULL) {
          temp = simplifyTreeErrorfree(coefficients2[i]);
          free_memory(coefficients2[i]);
          coefficients2[i] = temp;
        }
      }

      for (i=0;i<=degree2;i++) {
        if ((coefficients2[i] != NULL) && 
            (!((coefficients2[i]->nodeType == CONSTANT) &&
               (mpfr_zero_p(*(coefficients2[i]->value)))))) {
          (*coefficients)[i] = copyTree(coefficients2[i]);
        }
      }

      for (i=0;i<=degree1;i++) free_memory(coefficients1[i]);
      free(coefficients1);
      for (i=0;i<=degree2;i++) free_memory(coefficients2[i]);
      free(coefficients2);
      mpfr_clear(y);
      return;
    } 
    mpfr_clear(y);
  }

  temp = simplifyTreeErrorfree(poly);
  temp2 = expandPowerInPolynomialUnsafe(temp);
  temp3 = expandPolynomialUnsafe(temp2);
  temp4 = simplifyTreeErrorfree(temp3);

  getCoefficientsUnsafe(*coefficients,temp4,1);

  free_memory(temp);
  free_memory(temp2);
  free_memory(temp3);
  free_memory(temp4);
}



node* hornerPolynomialUnsafe(node *tree) {
  node *copy, *temp, *temp2, *temp3, *temp4, *simplified;
  node **monomials;
  int degree, i, k, e;
  mpfr_t *value;
  
  simplified = simplifyTreeErrorfree(tree);

  if (isHorner(simplified)) {
    degree = getDegree(simplified);
    monomials = (node**) safeCalloc((degree + 1),sizeof(node*));
    for (i=0;i<=degree;i++) monomials[i] = NULL;
    getCoefficientsHorner(monomials,simplified);
  } else {
    if (isCanonical(simplified)) {
      degree = getDegree(simplified);
      monomials = (node**) safeCalloc((degree + 1),sizeof(node*));
      for (i=0;i<=degree;i++) monomials[i] = NULL;
      getCoefficientsCanonical(monomials,simplified);
    } else {
      getCoefficients(&degree,&monomials,simplified);
    }
  }

  if (monomials[degree] == NULL) {
    sollyaFprintf(stderr,
"Error: hornerPolynomialUnsafe: an error occurred. The coefficient of a monomial with the polynomial's degree exponent is zero.\n");
    exit(1);
    return NULL;
  }

  copy = copyTree(monomials[degree]);

  for (i=degree-1;i>=0;i--) {
    if (monomials[i] == NULL) {
      if ((i == 0)) {
	temp = (node *) safeMalloc(sizeof(node));
	temp->nodeType = MUL;
	temp2 = (node *) safeMalloc(sizeof(node));
	temp2->nodeType = VARIABLE;
	temp->child1 = temp2;
	temp->child2 = copy;
	copy = temp;
      } else {
	for (k=i-1;((monomials[k]==NULL) && (k > 0));k--);
	e = (i - k) + 1;
	temp = (node *) safeMalloc(sizeof(node));
	temp->nodeType = MUL;
	temp2 = (node *) safeMalloc(sizeof(node));
	temp2->nodeType = VARIABLE;
	temp3 = (node *) safeMalloc(sizeof(node));
	temp3->nodeType = CONSTANT;
	value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*value,tools_precision);
	if (mpfr_set_si(*value,e,GMP_RNDN) != 0) {
	  if (!noRoundingWarnings) {
	    printMessage(1,"Warning: rounding occurred on representing a monomial power exponent with %d bits.\n",
			 (int) tools_precision);
	    printMessage(1,"Try to increase the precision.\n");
	  }
	}
	temp3->value = value;
	temp4 = (node *) safeMalloc(sizeof(node));
	temp4->nodeType = POW;
	temp4->child1 = temp2;
	temp4->child2 = temp3;
	temp->child1 = temp4;
	temp->child2 = copy;
	copy = temp;
	if (monomials[k] != NULL) {
	  temp = (node *) safeMalloc(sizeof(node));
	  temp->nodeType = ADD;
	  temp->child1 = copyTree(monomials[k]);
	  temp->child2 = copy;
	  copy = temp;
	}
	i = k;
      }
    } else {
      temp = (node *) safeMalloc(sizeof(node));
      temp->nodeType = MUL;
      temp2 = (node *) safeMalloc(sizeof(node));
      temp2->nodeType = VARIABLE;
      temp->child1 = temp2;
      temp->child2 = copy;
      copy = temp;
      temp = (node *) safeMalloc(sizeof(node));
      temp->nodeType = ADD;
      temp->child1 = copyTree(monomials[i]);
      temp->child2 = copy;
      copy = temp;
    }
  }
  

  for (i=0;i<=degree;i++) {
    if (monomials[i] != NULL) free_memory(monomials[i]);
  }
  free(monomials);

  free_memory(simplified);
  return copy;
}

node* dividePolynomialByPowerOfVariableUnsafe(node *tree, int alpha) {
  node *copy, *temp, *temp2, *temp3, *temp4, *simplified;
  node **monomials;
  int degree, i, k, e;
  mpfr_t *value;
  
  simplified = simplifyTreeErrorfree(tree);

  getCoefficients(&degree,&monomials,simplified);

  if (alpha > 0) {
    for (i=0;i<alpha;i++) 
      if (monomials[i] != NULL) free_memory(monomials[i]);
    for (i=alpha;i<=degree;i++) {
      monomials[i-alpha] = monomials[i];
    }
    degree = degree - alpha;
  }


  if (monomials[degree] == NULL) {
    sollyaFprintf(stderr,
"Error: hornerPolynomialUnsafe: an error occurred. The coefficient of a monomial with the polynomial's degree exponent is zero.\n");
    exit(1);
    return NULL;
  }

  copy = copyTree(monomials[degree]);

  for (i=degree-1;i>=0;i--) {
    if (monomials[i] == NULL) {
      if ((i == 0)) {
	temp = (node *) safeMalloc(sizeof(node));
	temp->nodeType = MUL;
	temp2 = (node *) safeMalloc(sizeof(node));
	temp2->nodeType = VARIABLE;
	temp->child1 = temp2;
	temp->child2 = copy;
	copy = temp;
      } else {
	for (k=i-1;((monomials[k]==NULL) && (k > 0));k--);
	e = (i - k) + 1;
	temp = (node *) safeMalloc(sizeof(node));
	temp->nodeType = MUL;
	temp2 = (node *) safeMalloc(sizeof(node));
	temp2->nodeType = VARIABLE;
	temp3 = (node *) safeMalloc(sizeof(node));
	temp3->nodeType = CONSTANT;
	value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*value,tools_precision);
	if (mpfr_set_si(*value,e,GMP_RNDN) != 0) {
	  if (!noRoundingWarnings) {
	    printMessage(1,"Warning: rounding occurred on representing a monomial power exponent with %d bits.\n",
			 (int) tools_precision);
	    printMessage(1,"Try to increase the precision.\n");
	  }
	}
	temp3->value = value;
	temp4 = (node *) safeMalloc(sizeof(node));
	temp4->nodeType = POW;
	temp4->child1 = temp2;
	temp4->child2 = temp3;
	temp->child1 = temp4;
	temp->child2 = copy;
	copy = temp;
	if (monomials[k] != NULL) {
	  temp = (node *) safeMalloc(sizeof(node));
	  temp->nodeType = ADD;
	  temp->child1 = copyTree(monomials[k]);
	  temp->child2 = copy;
	  copy = temp;
	}
	i = k;
      }
    } else {
      temp = (node *) safeMalloc(sizeof(node));
      temp->nodeType = MUL;
      temp2 = (node *) safeMalloc(sizeof(node));
      temp2->nodeType = VARIABLE;
      temp->child1 = temp2;
      temp->child2 = copy;
      copy = temp;
      temp = (node *) safeMalloc(sizeof(node));
      temp->nodeType = ADD;
      temp->child1 = copyTree(monomials[i]);
      temp->child2 = copy;
      copy = temp;
    }
  }
  

  for (i=0;i<=degree;i++) {
    if (monomials[i] != NULL) free_memory(monomials[i]);
  }
  free(monomials);

  free_memory(simplified);
  return copy;
}



node* hornerPolynomial(node *tree) {
  node *temp;
  
  if (isConstant(tree)) return copyTree(tree);
  if (getDegree(tree) < 0) return copyTree(tree);
  if (isHorner(tree)) {
    printMessage(7,"Information: no Horner simplification will be performed because the given tree is already in Horner form.\n");
    return copyTree(tree);
  }
  
  temp = hornerPolynomialUnsafe(tree);
  
  return temp;
}


node* hornerUnsimplified(node *tree) {
  node *copy;
  mpfr_t *value;
  mpfr_t temp;

  if (isPolynomial(tree)) return hornerPolynomial(tree);

  switch (tree->nodeType) {
  case VARIABLE:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = VARIABLE;
    break;
  case CONSTANT:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = CONSTANT;
    value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
    mpfr_init2(temp,tools_precision);
    simplifyMpfrPrec(temp,*(tree->value));
    mpfr_init2(*value,mpfr_get_prec(temp));
    mpfr_set(*value,temp,GMP_RNDN);
    mpfr_clear(temp);
    copy->value = value;
    break;
  case ADD:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ADD;
    copy->child1 = horner(tree->child1);
    copy->child2 = horner(tree->child2);
    break;
  case SUB:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = SUB;
    copy->child1 = horner(tree->child1);
    copy->child2 = horner(tree->child2);
    break;
  case MUL:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = MUL;
    copy->child1 = horner(tree->child1);
    copy->child2 = horner(tree->child2);
    break;
  case DIV:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = DIV;
    copy->child1 = horner(tree->child1);
    copy->child2 = horner(tree->child2);
    break;
  case SQRT:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = SQRT;
    copy->child1 = horner(tree->child1);
    break;
  case EXP:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = EXP;
    copy->child1 = horner(tree->child1);
    break;
  case LOG:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = LOG;
    copy->child1 = horner(tree->child1);
    break;
  case LOG_2:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = LOG_2;
    copy->child1 = horner(tree->child1);
    break;
  case LOG_10:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = LOG_10;
    copy->child1 = horner(tree->child1);
    break;
  case SIN:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = SIN;
    copy->child1 = horner(tree->child1);
    break;
  case COS:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = COS;
    copy->child1 = horner(tree->child1);
    break;
  case TAN:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = TAN;
    copy->child1 = horner(tree->child1);
    break;
  case ASIN:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ASIN;
    copy->child1 = horner(tree->child1);
    break;
  case ACOS:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ACOS;
    copy->child1 = horner(tree->child1);
    break;
  case ATAN:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ATAN;
    copy->child1 = horner(tree->child1);
    break;
  case SINH:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = SINH;
    copy->child1 = horner(tree->child1);
    break;
  case COSH:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = COSH;
    copy->child1 = horner(tree->child1);
    break;
  case TANH:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = TANH;
    copy->child1 = horner(tree->child1);
    break;
  case ASINH:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ASINH;
    copy->child1 = horner(tree->child1);
    break;
  case ACOSH:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ACOSH;
    copy->child1 = horner(tree->child1);
    break;
  case ATANH:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ATANH;
    copy->child1 = horner(tree->child1);
    break;
  case POW:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = POW;
    copy->child1 = horner(tree->child1);
    copy->child2 = horner(tree->child2);
    break;
  case NEG:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = NEG;
    copy->child1 = horner(tree->child1);
    break;
  case ABS:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ABS;
    copy->child1 = horner(tree->child1);
    break;
  case DOUBLE:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = DOUBLE;
    copy->child1 = horner(tree->child1);
    break;
  case SINGLE:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = SINGLE;
    copy->child1 = horner(tree->child1);
    break;
  case QUAD:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = QUAD;
    copy->child1 = horner(tree->child1);
    break;
  case HALFPRECISION:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = HALFPRECISION;
    copy->child1 = horner(tree->child1);
    break;
  case DOUBLEDOUBLE:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = DOUBLEDOUBLE;
    copy->child1 = horner(tree->child1);
    break;
  case TRIPLEDOUBLE:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = TRIPLEDOUBLE;
    copy->child1 = horner(tree->child1);
    break;
  case ERF:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ERF;
    copy->child1 = horner(tree->child1);
    break;
  case ERFC:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ERFC;
    copy->child1 = horner(tree->child1);
    break;
  case LOG_1P:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = LOG_1P;
    copy->child1 = horner(tree->child1);
    break;
  case EXP_M1:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = EXP_M1;
    copy->child1 = horner(tree->child1);
    break;
  case DOUBLEEXTENDED:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = DOUBLEEXTENDED;
    copy->child1 = horner(tree->child1);
    break;
  case LIBRARYFUNCTION:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = LIBRARYFUNCTION;
    copy->libFun = tree->libFun;
    copy->libFunDeriv = tree->libFunDeriv;
    copy->child1 = horner(tree->child1);
    break;
  case PROCEDUREFUNCTION:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = PROCEDUREFUNCTION;
    copy->libFunDeriv = tree->libFunDeriv;
    copy->child1 = horner(tree->child1);
    copy->child2 = copyThing(tree->child2);
    break;
  case CEIL:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = CEIL;
    copy->child1 = horner(tree->child1);
    break;
  case FLOOR:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = FLOOR;
    copy->child1 = horner(tree->child1);
    break;
  case NEARESTINT:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = NEARESTINT;
    copy->child1 = horner(tree->child1);
    break;
  case PI_CONST:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = PI_CONST;
    break;
  case LIBRARYCONSTANT:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = LIBRARYCONSTANT;
    copy->libFun = tree->libFun;
    break;
  default:
   sollyaFprintf(stderr,"Error: horner: unknown identifier in the tree\n");
   exit(1);
  }
  return copy;
}


int isPowerOfVariable(node *tree) {
  if (tree->nodeType == VARIABLE) return 1;
  if ((tree->nodeType == POW) &&
      (tree->child1->nodeType == VARIABLE) &&
      (tree->child2->nodeType == CONSTANT) && 
      mpfr_integer_p(*(tree->child2->value))) return 1;
  return 0;
}

int isHornerUnsafe(node *tree) {
  if (isConstant(tree)) return 1;
  if (((tree->nodeType == ADD) || (tree->nodeType == SUB)) &&
      isConstant(tree->child1) &&
      (tree->child2->nodeType == MUL) &&
      isPowerOfVariable(tree->child2->child1) &&
      isHornerUnsafe(tree->child2->child2)) return 1;
  if (((tree->nodeType == ADD) || (tree->nodeType == SUB)) &&
      isConstant(tree->child1) &&
      isPowerOfVariable(tree->child2)) return 1;
  if (((tree->nodeType == ADD) || (tree->nodeType == SUB)) &&
      isConstant(tree->child1) &&
      (tree->child2->nodeType == MUL) &&
      (tree->child2->child1->nodeType == MUL) &&
      isPowerOfVariable(tree->child2->child1->child1) &&
      isConstant(tree->child2->child1->child2) &&
      isConstant(tree->child2->child2)) return 1;
  if (((tree->nodeType == ADD) || (tree->nodeType == SUB)) &&
      isConstant(tree->child1) &&
      (tree->child2->nodeType == MUL) &&
      isConstant(tree->child2->child1) &&
      isPowerOfVariable(tree->child2->child2)) return 1;
  return 0;
}

int isHorner(node *tree) {
  if ((tree->nodeType == ADD) || (tree->nodeType == SUB)) 
    return isHornerUnsafe(tree);
  if (tree->nodeType == MUL) {
    return isPowerOfVariable(tree->child1) && isHornerUnsafe(tree->child2);
  }
  return 0;
}


node* horner(node *tree) {
  node *temp, *temp2, *temp3;
  int i;

  if (isHorner(tree)) {
    printMessage(7,"Information: no Horner simplification will be performed because the given tree is already in Horner form.\n");
    return copyTree(tree);
  }

  temp3 = simplifyTreeErrorfree(tree);

  i = 0;
  do {
    temp = hornerUnsimplified(temp3);
    temp2 = simplifyTreeErrorfree(temp);
    free_memory(temp);
    free_memory(temp3);
    temp3 = temp2;
    i++;
  } while ((!isHorner(temp3)) && (i < 2));

  return temp3;
}


node *differentiatePolynomialHornerUnsafe(node *tree) {
  int degree, i, k, e;
  node **monomials;
  node *temp, *temp2, *temp3, *temp4, *copy;
  mpfr_t *value;
  mp_prec_t prec;

  getCoefficients(&degree,&monomials,tree);

  if (monomials[0] != NULL) free_memory(monomials[0]);

  for (i=1;i<=degree;i++) {
    if (monomials[i] != NULL) {
      if (monomials[i]->nodeType == CONSTANT) {
	value = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*value,mpfr_get_prec(*(monomials[i]->value))+(sizeof(int)*8));
	if (mpfr_mul_si(*value,*(monomials[i]->value),i,GMP_RNDN) != 0)
	  printMessage(1,"Warning: rounding occurred while differentiating a polynomial in Horner form.\n");
	mpfr_clear(*(monomials[i]->value));
	free(monomials[i]->value);
	monomials[i]->value = value;
	temp = monomials[i];
      } else {
	temp = (node *) safeMalloc(sizeof(node));
	temp->nodeType = MUL;
	temp->child1 = (node *) safeMalloc(sizeof(node));
	temp->child1->nodeType = CONSTANT;
	temp->child1->value = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
	prec = 8 * sizeof(int) + 10; if (tools_precision > prec) prec = tools_precision;
	mpfr_init2(*(temp->child1->value),prec);
	if (mpfr_set_si(*(temp->child1->value),i,GMP_RNDN) != 0) {
	  if (!noRoundingWarnings) {
	    printMessage(1,"Warning: on differentiating a polynomial in Horner form rounding occurred while representing the degree of a monomial on a constant of the given precision\n");
	  }
	}
	temp->child2 = monomials[i];
      }
      monomials[i-1] = temp;
    } else {
      monomials[i-1] = NULL;
    }
  }
  
  degree--;

  copy = copyTree(monomials[degree]);

  for (i=degree-1;i>=0;i--) {
    if (monomials[i] == NULL) {
      if ((i == 0)) {
	temp = (node *) safeMalloc(sizeof(node));
	temp->nodeType = MUL;
	temp2 = (node *) safeMalloc(sizeof(node));
	temp2->nodeType = VARIABLE;
	temp->child1 = temp2;
	temp->child2 = copy;
	copy = temp;
      } else {
	for (k=i-1;((monomials[k]==NULL) && (k > 0));k--);
	e = (i - k) + 1;
	temp = (node *) safeMalloc(sizeof(node));
	temp->nodeType = MUL;
	temp2 = (node *) safeMalloc(sizeof(node));
	temp2->nodeType = VARIABLE;
	temp3 = (node *) safeMalloc(sizeof(node));
	temp3->nodeType = CONSTANT;
	value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*value,tools_precision);
	if (mpfr_set_si(*value,e,GMP_RNDN) != 0) {
	  if (!noRoundingWarnings) {
	    printMessage(1,"Warning: rounding occurred on representing a monomial power exponent with %d bits.\n",
			 (int) tools_precision);
	    printMessage(1,"Try to increase the precision.\n");
	  }
	}
	temp3->value = value;
	temp4 = (node *) safeMalloc(sizeof(node));
	temp4->nodeType = POW;
	temp4->child1 = temp2;
	temp4->child2 = temp3;
	temp->child1 = temp4;
	temp->child2 = copy;
	copy = temp;
	if (monomials[k] != NULL) {
	  temp = (node *) safeMalloc(sizeof(node));
	  temp->nodeType = ADD;
	  temp->child1 = copyTree(monomials[k]);
	  temp->child2 = copy;
	  copy = temp;
	}
	i = k;
      }
    } else {
      temp = (node *) safeMalloc(sizeof(node));
      temp->nodeType = MUL;
      temp2 = (node *) safeMalloc(sizeof(node));
      temp2->nodeType = VARIABLE;
      temp->child1 = temp2;
      temp->child2 = copy;
      copy = temp;
      temp = (node *) safeMalloc(sizeof(node));
      temp->nodeType = ADD;
      temp->child1 = copyTree(monomials[i]);
      temp->child2 = copy;
      copy = temp;
    }
  }
  

  for (i=0;i<=degree;i++) {
    if (monomials[i] != NULL) free_memory(monomials[i]);
  }
  free(monomials);

  return copy;
}

node *differentiatePolynomialUnsafe(node *tree) {
  node *copy, *temp, *temp2, *temp3, *temp4, *temp5;
  int degree, i;
  node **monomials;
  mpfr_t *value;

  if (isHorner(tree)) {
    printMessage(25,"Information: differentiating a polynomial in Horner form uses a special algorithm.\n");
    return differentiatePolynomialHornerUnsafe(tree);
  } 

  degree = getDegree(tree);

  if (degree == 0) {
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = CONSTANT;
    value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
    mpfr_init2(*value,tools_precision);
    mpfr_set_d(*value,0.0,GMP_RNDN);
    copy->value = value;
  } else {
    getCoefficients(&degree,&monomials,tree);
 
    if (monomials[degree] == NULL) {
      copy = (node*) safeMalloc(sizeof(node));
      copy->nodeType = CONSTANT;
      value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*value,tools_precision);
      mpfr_set_d(*value,0.0,GMP_RNDN);
      copy->value = value;
      monomials[degree] = copy;
    }
    
    if (degree >= 2) {
      if (degree > 2) {
	temp = copyTree(monomials[degree]);
	temp2 = (node*) safeMalloc(sizeof(node));
	temp2->nodeType = CONSTANT;
	value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*value,tools_precision);
	if (mpfr_set_si(*value,degree,GMP_RNDN) != 0) {
	  if (!noRoundingWarnings) {
	    printMessage(1,"Warning: rounding occurred on differentiating a polynomial. A constant could not be written on %d bits.\n",
			 (int) tools_precision);
	    printMessage(1,"Try to increase the precision.\n");
	  }
	}
	temp2->value = value;
	temp3 = (node*) safeMalloc(sizeof(node));
	temp3->nodeType = MUL;
	temp3->child1 = temp2;
	temp3->child2 = temp;
	temp2 = (node*) safeMalloc(sizeof(node));
	temp2->nodeType = CONSTANT;
	value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*value,tools_precision);
	if (mpfr_set_si(*value,degree-1,GMP_RNDN) != 0) {
	  printMessage(1,
		 "Warning: rounding occurred on differentiating a polynomial. An exponent constant could not be written on %d bits.\n",
		 (int) tools_precision);
	  printMessage(1,"Try to increase the precision.\n");
	}
	temp2->value = value;
	temp = (node*) safeMalloc(sizeof(node));
	temp->nodeType = VARIABLE;
	temp4 = (node*) safeMalloc(sizeof(node));
	temp4->nodeType = POW;
	temp4->child1 = temp;
	temp4->child2 = temp2;
	temp5 = (node*) safeMalloc(sizeof(node));
	temp5->nodeType = MUL;
	temp5->child1 = temp3;
	temp5->child2 = temp4;
	copy = temp5;
      } else {
	temp = copyTree(monomials[degree]);
	temp2 = (node*) safeMalloc(sizeof(node));
	temp2->nodeType = CONSTANT;
	value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*value,tools_precision);
	if (mpfr_set_si(*value,degree,GMP_RNDN) != 0) {
	  printMessage(1,"Warning: rounding occurred on differentiating a polynomial. A constant could not be written on %d bits.\n",
		 (int) tools_precision);
	  printMessage(1,"Try to increase the precision.\n");
	}
	temp2->value = value;
	temp3 = (node*) safeMalloc(sizeof(node));
	temp3->nodeType = MUL;
	temp3->child1 = temp2;
	temp3->child2 = temp;
	temp = (node*) safeMalloc(sizeof(node));
	temp->nodeType = VARIABLE;
	temp4 = (node*) safeMalloc(sizeof(node));
	temp4->nodeType = MUL;
	temp4->child1 = temp3;
	temp4->child2 = temp;
	copy = temp4;
      }
          
      for (i=degree-1;i>1;i--) {
	if (monomials[i] != NULL) {
	  temp = copyTree(monomials[i]);
	  temp2 = (node*) safeMalloc(sizeof(node));
	  temp2->nodeType = CONSTANT;
	  value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	  mpfr_init2(*value,tools_precision);
	  if (mpfr_set_si(*value,i,GMP_RNDN) != 0) {
	    printMessage(1,"Warning: rounding occurred on differentiating a polynomial. A constant could not be written on %d bits.\n",
		   (int) tools_precision);
	    printMessage(1,"Try to increase the precision.\n");
	  }
	  temp2->value = value;
	  temp3 = (node*) safeMalloc(sizeof(node));
	  temp3->nodeType = MUL;
	  temp3->child1 = temp2;
	  temp3->child2 = temp;
	  temp2 = (node*) safeMalloc(sizeof(node));
	  temp2->nodeType = CONSTANT;
	  value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	  mpfr_init2(*value,tools_precision);
	  if (mpfr_set_si(*value,i-1,GMP_RNDN) != 0) {
	    printMessage(1,
		   "Warning: rounding occurred on differentiating a polynomial. An exponent constant could not be written on %d bits.\n",
		   (int) tools_precision);
	    printMessage(1,"Try to increase the precision.\n");
	  }
	  temp2->value = value;
	  temp = (node*) safeMalloc(sizeof(node));
	  temp->nodeType = VARIABLE;
	  temp4 = (node*) safeMalloc(sizeof(node));
	  temp4->nodeType = POW;
	  temp4->child1 = temp;
	  temp4->child2 = temp2;
	  temp5 = (node*) safeMalloc(sizeof(node));
	  temp5->nodeType = MUL;
	  temp5->child1 = temp3;
	  temp5->child2 = temp4;
	  temp = copy;
	  temp2 = (node*) safeMalloc(sizeof(node));
	  temp2->nodeType = ADD;
	  temp2->child1 = temp5;
	  temp2->child2 = temp;
	  copy = temp2;      
	} 
      }
      
      if (monomials[1] != NULL) {
	temp = copyTree(monomials[1]);
	temp2 = (node*) safeMalloc(sizeof(node));
	temp2->nodeType = ADD;
	temp2->child1 = temp;
	temp2->child2 = copy;
	copy = temp2;      
      } 
    } else {
      copy = copyTree(monomials[1]);
    }
  
    for (i=0;i<=degree;i++) {
      if (monomials[i] != NULL) free_memory(monomials[i]);
    }
    free(monomials);
  } 

  return copy;
}


int getNumeratorDenominator(node **numerator, node **denominator, node *tree) {
  if (tree->nodeType == DIV) {
    *numerator = copyTree(tree->child1);
    *denominator = copyTree(tree->child2);
    return 1;
  }
  else {
    *numerator = copyTree(tree);
    *denominator = NULL;
    return 0;
  }
}

node *makeBinomialCoefficient(unsigned int n, unsigned int k) {
  mpz_t coeffGMP;
  mp_prec_t prec;
  mpfr_t *coeffVal;
  node *res;

  mpz_init(coeffGMP);
  mpz_bin_uiui(coeffGMP,n,k);
  prec = mpz_sizeinbase(coeffGMP, 2) + 10;
  if (prec < tools_precision) prec = tools_precision;
  coeffVal = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
  mpfr_init2(*coeffVal,prec);
  if(mpfr_set_z(*coeffVal,coeffGMP,GMP_RNDN) != 0) {
    if (!noRoundingWarnings) {
      printMessage(1,"Warning: rounding occurred when calculating a binomial coefficient.\n");
      printMessage(1,"Try to increase the working precision.\n");
    }
  }
  mpz_clear(coeffGMP);
  res = (node *) safeMalloc(sizeof(node));
  res->nodeType = CONSTANT;
  res->value = coeffVal;
  return res;
}

node *makePowerOfConstant(node *constTree, int k) {
  node *temp, *res;

  if (k == 1) {
    return copyTree(constTree);
  }
  temp = (node *) safeMalloc(sizeof(node));
  temp->nodeType = CONSTANT;
  temp->value = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
  mpfr_init2(*(temp->value),8 * sizeof(k) + 10);
  mpfr_set_si(*(temp->value),k,GMP_RNDN);
  res = (node *) safeMalloc(sizeof(node));
  res->nodeType = POW;
  res->child1 = copyTree(constTree);
  res->child2 = temp;

  return res;
}

// polynomialShiftAndScaleAbscissaUnsafe(poly, a, b) returns
//
// p(a + b * x)
// 
// for a polynomial p and a, b constant nodes
//
// If poly is not a polynomial the tool is terminated.
// If a or b is not a constant expression, the shifting and
// scaling is performed as if they were.
//
node *polynomialShiftAndScaleAbscissaUnsafe(node *poly, node *a, node *b) {
  node *res;
  node **coeffs;
  node **coeffsRes;
  node *temp;
  int degree;
  int i,k;
 
  getCoefficients(&degree, &coeffs, poly);
  if (degree < 0) {
    sollyaFprintf(stderr,"Error: polynomialShiftAndScaleAbscissaUnsafe: the given expression is not a polynomial\n");
    exit(1);
  }
  for (i=0;i<=degree;i++) {
    if (coeffs[i] == NULL) {
      coeffs[i] = makeConstantDouble(0.0);
    }
  }

  coeffsRes = (node **) safeCalloc(degree+1,sizeof(node *));
  for (i=0;i<=degree;i++) {
    coeffsRes[i] = makeConstantDouble(0.0);
  }

  for (i=0;i<=degree;i++) {
    for (k=0;k<=i;k++) {
      temp = makeMul(copyTree(coeffs[i]),
                     makeMul(makeBinomialCoefficient(i,k),
                             makeMul(makePowerOfConstant(a, i-k),
                                     makePowerOfConstant(b, k))));
      coeffsRes[k] = makeAdd(coeffsRes[k],temp);
    }
  }

  for (i=0;i<=degree;i++) {
    if (coeffsRes[i] != NULL) {
      temp = simplifyTreeErrorfree(coeffsRes[i]);
      free_memory(coeffsRes[i]);
      coeffsRes[i] = temp;
    }
  }

  res = makePolynomialConstantExpressions(coeffsRes, degree);

  for (i=0;i<=degree;i++) {
    if (coeffs[i] != NULL) free_memory(coeffs[i]);
    if (coeffsRes[i] != NULL) free_memory(coeffsRes[i]);
  }
  free(coeffs);
  free(coeffsRes);

  return res;
}

// Returns p(q)
//
node *substitutePolynomialUnsafe(node *p, node *q) {
  node *res;
  node **coeffsP, **coeffsQ, **coeffs, **coeffsQPi;
  int degP, degQ, deg, i, k, degQPi;
  node *temp;

  getCoefficients(&degP, &coeffsP, p);
  if (degP < 0) {
    sollyaFprintf(stderr,"Error: substitutePolynomialUnsafe: the given expression is not a polynomial\n");
    exit(1);
  }

  getCoefficients(&degQ, &coeffsQ, q);
  if (degQ < 0) {
    sollyaFprintf(stderr,"Error: substitutePolynomialUnsafe: the given expression is not a polynomial\n");
    exit(1);
  }
  for (i=0;i<=degQ;i++) {
    if (coeffsQ[i] == NULL) {
      coeffsQ[i] = makeConstantDouble(0.0);
    }
  }

  deg = degP * degQ;
  coeffs = (node **) safeCalloc(deg+1,sizeof(node *));
  for (i=0;i<=deg;i++) {
    coeffs[i] = makeConstantDouble(0.0);
  }

  for (i=0;i<=degP;i++) {
    if (coeffsP[i] != NULL) {
      computePowerOfPolynomialCoefficients(&degQPi, &coeffsQPi, 
                                           coeffsQ, degQ, i);
      for (k=0;k<=degQPi;k++) {
        if (coeffsQPi[k] != NULL) {
          coeffs[k] = makeAdd(coeffs[k],
                              makeMul(copyTree(coeffsP[i]),
                                      coeffsQPi[k]));
        }
      }
      free(coeffsQPi);
    }
  }

  for (i=0;i<=deg;i++) {
    if (coeffs[i] != NULL) {
      temp = simplifyTreeErrorfree(coeffs[i]);
      free_memory(coeffs[i]);
      coeffs[i] = temp;
    }
  }

  res = makePolynomialConstantExpressions(coeffs, deg);

  for (i=0;i<=degP;i++) {
    if (coeffsP[i] != NULL) free_memory(coeffsP[i]);
  }
  free(coeffsP);

  for (i=0;i<=degQ;i++) {
    if (coeffsQ[i] != NULL) free_memory(coeffsQ[i]);
  }
  free(coeffsQ);

  for (i=0;i<=deg;i++) {
    if (coeffs[i] != NULL) free_memory(coeffs[i]);
  }
  free(coeffs);

  return res;
}

node *substitute(node* tree, node *t) {
  node *copy;
  mpfr_t *value;
  mpfr_t temp;
  node **coeffs;
  int degree;
  int i;

  if (isPolynomial(tree) && 
      isPolynomial(t)) {
    if ((getDegree(t) == 1) &&
        (getDegree(tree) >= 2)) {
      getCoefficients(&degree, &coeffs, t);
      if (degree == 1) {
        for (i=0;i<=degree;i++) {
          if (coeffs[i] == NULL) {
            coeffs[i] = makeConstantDouble(0.0);
          }
        }

        copy = polynomialShiftAndScaleAbscissaUnsafe(tree, coeffs[0], coeffs[1]);

        for (i=0;i<=degree;i++) {
          if (coeffs[i] != NULL) free_memory(coeffs[i]);
        }
        free(coeffs);
        return copy;
      }
      for (i=0;i<=degree;i++) {
        if (coeffs[i] != NULL) free_memory(coeffs[i]);
      }
      free(coeffs);
    }

    if ((getDegree(t) >= 2) && 
        (getDegree(tree) >= 2)) {
      copy = substitutePolynomialUnsafe(tree,t);
      return copy;
    }
  }

  switch (tree->nodeType) {
  case VARIABLE:
    copy = copyTree(t);
    break;
  case CONSTANT:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = CONSTANT;
    value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
    mpfr_init2(temp,mpfr_get_prec(*(tree->value)));
    simplifyMpfrPrec(temp,*(tree->value));
    mpfr_init2(*value,mpfr_get_prec(temp));
    mpfr_set(*value,temp,GMP_RNDN);
    mpfr_clear(temp);
    copy->value = value;
    break;
  case ADD:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ADD;
    copy->child1 = substitute(tree->child1,t);
    copy->child2 = substitute(tree->child2,t);
    break;
  case SUB:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = SUB;
    copy->child1 = substitute(tree->child1,t);
    copy->child2 = substitute(tree->child2,t);
    break;
  case MUL:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = MUL;
    copy->child1 = substitute(tree->child1,t);
    copy->child2 = substitute(tree->child2,t);
    break;
  case DIV:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = DIV;
    copy->child1 = substitute(tree->child1,t);
    copy->child2 = substitute(tree->child2,t);
    break;
  case SQRT:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = SQRT;
    copy->child1 = substitute(tree->child1,t);
    break;
  case EXP:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = EXP;
    copy->child1 = substitute(tree->child1,t);
    break;
  case LOG:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = LOG;
    copy->child1 = substitute(tree->child1,t);
    break;
  case LOG_2:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = LOG_2;
    copy->child1 = substitute(tree->child1,t);
    break;
  case LOG_10:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = LOG_10;
    copy->child1 = substitute(tree->child1,t);
    break;
  case SIN:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = SIN;
    copy->child1 = substitute(tree->child1,t);
    break;
  case COS:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = COS;
    copy->child1 = substitute(tree->child1,t);
    break;
  case TAN:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = TAN;
    copy->child1 = substitute(tree->child1,t);
    break;
  case ASIN:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ASIN;
    copy->child1 = substitute(tree->child1,t);
    break;
  case ACOS:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ACOS;
    copy->child1 = substitute(tree->child1,t);
    break;
  case ATAN:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ATAN;
    copy->child1 = substitute(tree->child1,t);
    break;
  case SINH:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = SINH;
    copy->child1 = substitute(tree->child1,t);
    break;
  case COSH:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = COSH;
    copy->child1 = substitute(tree->child1,t);
    break;
  case TANH:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = TANH;
    copy->child1 = substitute(tree->child1,t);
    break;
  case ASINH:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ASINH;
    copy->child1 = substitute(tree->child1,t);
    break;
  case ACOSH:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ACOSH;
    copy->child1 = substitute(tree->child1,t);
    break;
  case ATANH:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ATANH;
    copy->child1 = substitute(tree->child1,t);
    break;
  case POW:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = POW;
    copy->child1 = substitute(tree->child1,t);
    copy->child2 = substitute(tree->child2,t);
    break;
  case NEG:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = NEG;
    copy->child1 = substitute(tree->child1,t);
    break;
  case ABS:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ABS;
    copy->child1 = substitute(tree->child1,t);
    break;
  case DOUBLE:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = DOUBLE;
    copy->child1 = substitute(tree->child1,t);
    break;
  case SINGLE:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = SINGLE;
    copy->child1 = substitute(tree->child1,t);
    break;
  case QUAD:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = QUAD;
    copy->child1 = substitute(tree->child1,t);
    break;
  case HALFPRECISION:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = HALFPRECISION;
    copy->child1 = substitute(tree->child1,t);
    break;
  case DOUBLEDOUBLE:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = DOUBLEDOUBLE;
    copy->child1 = substitute(tree->child1,t);
    break;
  case TRIPLEDOUBLE:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = TRIPLEDOUBLE;
    copy->child1 = substitute(tree->child1,t);
    break;
  case ERF:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ERF;
    copy->child1 = substitute(tree->child1,t);
    break;
  case ERFC:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ERFC;
    copy->child1 = substitute(tree->child1,t);
    break;
  case LOG_1P:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = LOG_1P;
    copy->child1 = substitute(tree->child1,t);
    break;
  case EXP_M1:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = EXP_M1;
    copy->child1 = substitute(tree->child1,t);
    break;
  case DOUBLEEXTENDED:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = DOUBLEEXTENDED;
    copy->child1 = substitute(tree->child1,t);
    break;
  case LIBRARYFUNCTION:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = LIBRARYFUNCTION;
    copy->libFun = tree->libFun;
    copy->libFunDeriv = tree->libFunDeriv;
    copy->child1 = substitute(tree->child1,t);
    break;
  case PROCEDUREFUNCTION:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = PROCEDUREFUNCTION;
    copy->libFunDeriv = tree->libFunDeriv;
    copy->child1 = substitute(tree->child1,t);
    copy->child2 = copyThing(tree->child2);
    break;
  case CEIL:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = CEIL;
    copy->child1 = substitute(tree->child1,t);
    break;
  case FLOOR:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = FLOOR;
    copy->child1 = substitute(tree->child1,t);
    break;
  case NEARESTINT:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = NEARESTINT;
    copy->child1 = substitute(tree->child1,t);
    break;
  case PI_CONST:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = PI_CONST;
    break;
  case LIBRARYCONSTANT:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = LIBRARYCONSTANT;
    copy->libFun = tree->libFun;
    break;

  default:
   sollyaFprintf(stderr,"Error: substitute: unknown identifier in the tree\n");
   exit(1);
  }
  return copy;
}

int readHexadecimal(mpfr_t rop, char *c) {
  mpfr_t vrd, vru;
  mp_prec_t p;
  int res, resA, resB;
  char *c2;
  
  c2 = (char *) safeCalloc(strlen(c) + 2, sizeof(char));
  strcpy(c2, c);

  if ((c2[strlen(c2) - 1] == 'p') || 
      (c2[strlen(c2) - 1] == 'P')) c2[strlen(c2)] = '0';


  p = mpfr_get_prec(rop);

  mpfr_init2(vrd, p);
  mpfr_init2(vru, p);
  
  resA = mpfr_set_str(vrd, c2, 16, GMP_RNDD);
  resB = mpfr_set_str(vru, c2, 16, GMP_RNDU);

  if (!resA && !resB) {
    if (mpfr_cmp(vrd,vru) == 0) {
      mpfr_set(rop,vrd,GMP_RNDN);
      res = 1;
    } else {
      resA = mpfr_set_str(vrd, c2, 16, GMP_RNDN);
      if (!resA) {
	mpfr_set(rop, vrd, GMP_RNDN);
	res = 0;
      } else {
	mpfr_set_nan(rop);
	res = 0;
      }
    }
  } else {
    mpfr_set_nan(rop);
    res = 0;
  }

  mpfr_clear(vrd);
  mpfr_clear(vru);
  free(c2);

  return res;
}


int readDyadic(mpfr_t res, char *c) {
  char *mantissa, *exponent, *curr, *curr2;  
  mpfr_t mant, expo, temp1, temp2;
  mp_prec_t prec;
  int rounding;

  mantissa = (char *) safeCalloc(strlen(c)+1,sizeof(char));
  exponent = (char *) safeCalloc(strlen(c)+1,sizeof(char));
  curr = c; curr2 = mantissa;
  while ((*curr != '\0') && (*curr != 'b') && (*curr != 'B')) {
    *curr2 = *curr;
    curr2++;
    curr++;
  }
  if (*curr != '\0') curr++;
  curr2 = exponent;
  while (*curr != '\0') {
    *curr2 = *curr;
    curr2++;
    curr++;
  }

  rounding = 1;

  prec = mpfr_get_prec(res);
  mpfr_init2(mant,prec);
  mpfr_init2(expo,prec);
  mpfr_init2(temp1,prec);
  mpfr_init2(temp2,prec); 

  mpfr_set_str(temp1,mantissa,10,GMP_RNDU);
  mpfr_set_str(temp2,mantissa,10,GMP_RNDD);
  if (mpfr_cmp(temp1,temp2) != 0) {
    rounding = 0;
    mpfr_set_str(temp1,mantissa,10,GMP_RNDN);
  }
  if (mpfr_set(mant,temp1,GMP_RNDN) != 0) {
    rounding = 0;
  }
  mpfr_set_str(temp1,exponent,10,GMP_RNDU);
  mpfr_set_str(temp2,exponent,10,GMP_RNDD);
  if (mpfr_cmp(temp1,temp2) != 0) {
    rounding = 0;
    mpfr_set_str(temp1,exponent,10,GMP_RNDN);
  }
  if (mpfr_exp2(expo,temp1,GMP_RNDN) != 0) {
    rounding = 0;
  }
  if (mpfr_mul(res,mant,expo,GMP_RNDN) != 0) {
    rounding = 0;
  }
 
  if (!mpfr_number_p(res)) rounding = 1;

  mpfr_clear(mant);
  mpfr_clear(expo);
  mpfr_clear(temp1);
  mpfr_clear(temp2);
  free(mantissa);
  free(exponent);
  return rounding;
}

node *makePolynomialConstantExpressions(node **coeffs, int deg) {
  node *copy;
  int i, degree, e, k;
  node *temp;
  node *temp2;
  node *temp3;
  mpfr_t *value;
  node *temp4;

  if (deg < 0) {
    sollyaFprintf(stderr,"Error: makePolynomialConstantExpressions: degree of polynomial to be built is negative\n");
    exit(1);
  }

  degree = deg;
  while ((degree > 0) && 
         ((coeffs[degree] == NULL) || 
          ((coeffs[degree]->nodeType == CONSTANT) && 
           (mpfr_zero_p(*(coeffs[degree]->value)))))) degree--;

  if (degree == 0) {
    if (coeffs[0] == NULL) return makeConstantDouble(0.0);
    return copyTree(coeffs[0]);
  }

  copy = copyTree(coeffs[degree]);
  for (i=degree-1;i>=0;i--) {
    if ((coeffs[i] == NULL) || 
        ((coeffs[i]->nodeType == CONSTANT) && 
         (mpfr_zero_p(*(coeffs[i]->value))))) {
      if ((i == 0)) {
	temp = (node *) safeMalloc(sizeof(node));
	temp->nodeType = MUL;
	temp2 = (node *) safeMalloc(sizeof(node));
	temp2->nodeType = VARIABLE;
	temp->child1 = temp2;
	temp->child2 = copy;
	copy = temp;
      } else {
	for (k=i-1;(((coeffs[k] == NULL) || 
                     ((coeffs[k]->nodeType == CONSTANT) && 
                      (mpfr_zero_p(*(coeffs[k]->value))))) && (k > 0));k--);
	e = (i - k) + 1;
	temp = (node *) safeMalloc(sizeof(node));
	temp->nodeType = MUL;
	temp2 = (node *) safeMalloc(sizeof(node));
	temp2->nodeType = VARIABLE;
	temp3 = (node *) safeMalloc(sizeof(node));
	temp3->nodeType = CONSTANT;
	value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*value,(tools_precision > (8 * sizeof(e) + 10) ? tools_precision : (8 * sizeof(e) + 10)));
	if (mpfr_set_si(*value,e,GMP_RNDN) != 0) {
	  if (!noRoundingWarnings) {
	    printMessage(1,"Warning: rounding occurred on representing a monomial power exponent with %d bits.\n",
			 (int) mpfr_get_prec(*value));
	    printMessage(1,"Try to increase the precision.\n");
	  }
	}
	temp3->value = value;
	temp4 = (node *) safeMalloc(sizeof(node));
	temp4->nodeType = POW;
	temp4->child1 = temp2;
	temp4->child2 = temp3;
	temp->child1 = temp4;
	temp->child2 = copy;
	copy = temp;
	if (!((coeffs[k] == NULL) || 
              ((coeffs[k]->nodeType == CONSTANT) && 
               (mpfr_zero_p(*(coeffs[k]->value)))))) {
	  temp = (node *) safeMalloc(sizeof(node));
	  temp->nodeType = ADD;
	  temp->child1 = copyTree(coeffs[k]);
	  temp->child2 = copy;
	  copy = temp;
	}
	i = k;
      }
    } else {
      temp = (node *) safeMalloc(sizeof(node));
      temp->nodeType = MUL;
      temp2 = (node *) safeMalloc(sizeof(node));
      temp2->nodeType = VARIABLE;
      temp->child1 = temp2;
      temp->child2 = copy;
      copy = temp;
      temp = (node *) safeMalloc(sizeof(node));
      temp->nodeType = ADD;
      temp->child1 = copyTree(coeffs[i]);
      temp->child2 = copy;
      copy = temp;
    }
  }
  return copy;
}

node *makePolynomial(mpfr_t *coefficients, int degree) {
  node *tempTree, *tempTree2, *tempTree3;
  int i;
  
  tempTree = (node *) safeMalloc(sizeof(node));
  tempTree->nodeType = CONSTANT;
  tempTree->value = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
  mpfr_init2(*(tempTree->value),mpfr_get_prec(coefficients[degree]));
  mpfr_set(*(tempTree->value),coefficients[degree],GMP_RNDN);
  for (i=degree-1;i>=0;i--) {
    tempTree2 = (node *) safeMalloc(sizeof(node));
    tempTree2->nodeType = MUL;
    tempTree3 = (node *) safeMalloc(sizeof(node));
    tempTree3->nodeType = VARIABLE;
    tempTree2->child1 = tempTree3;
    tempTree2->child2 = tempTree;
    tempTree = (node *) safeMalloc(sizeof(node));
    tempTree->nodeType = ADD;
    tempTree->child2 = tempTree2;
    tempTree3 = (node *) safeMalloc(sizeof(node));
    tempTree3->nodeType = CONSTANT;
    tempTree3->value = (mpfr_t *) safeMalloc(sizeof(node));
    mpfr_init2(*(tempTree3->value),mpfr_get_prec(coefficients[i]));
    mpfr_set(*(tempTree3->value),coefficients[i],GMP_RNDN);
    tempTree->child1 = tempTree3;
  }
  tempTree2 = horner(tempTree);
  free_memory(tempTree);
  return tempTree2;
}


int treeSize(node *tree) {
  if (tree == NULL) return 0;
  switch (tree->nodeType) {
  case VARIABLE:
  case CONSTANT:
  case PI_CONST:
  case LIBRARYCONSTANT:
    return 1;
    break;

  case ADD:
  case SUB:
  case MUL:
  case DIV:
  case POW:
    return treeSize(tree->child1) + treeSize(tree->child2) + 1;
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
  case NEG:
  case ABS:
  case DOUBLE:
  case SINGLE:
  case QUAD:
  case HALFPRECISION:
  case DOUBLEDOUBLE:
  case TRIPLEDOUBLE:
  case ERF:
  case ERFC:
  case LOG_1P:
  case EXP_M1:
  case DOUBLEEXTENDED:
  case LIBRARYFUNCTION:    
  case PROCEDUREFUNCTION:    
  case CEIL:
  case FLOOR:
  case NEARESTINT:
    return treeSize(tree->child1) + 1;
    break;

  default:
   sollyaFprintf(stderr,"Error: treeSize: unknown identifier (%d) in the tree\n",tree->nodeType);
   exit(1);
  }
  return -1;
}


int highestDegreeOfPolynomialSubexpression(node *tree) {
  int l, r;

  if (isPolynomial(tree)) return getDegree(tree);

  switch (arity(tree)) {
  case 2:
    l = highestDegreeOfPolynomialSubexpression(tree->child1);
    r = highestDegreeOfPolynomialSubexpression(tree->child2);
    return l > r ? l : r;
    break;
  case 1:
    return highestDegreeOfPolynomialSubexpression(tree->child1);
    break;
  case 0:
    return getDegree(tree);
    break;
  default: 
    sollyaFprintf(stderr,"Error: unknown arity of an operator.\n");
    exit(1);
  }

  return -1;
}



node *getIthCoefficient(node *poly, int i) {
  node *tempNode;
  node **coefficients;
  int degree, k;

  if ((!isPolynomial(poly)) || (i < 0)) {
    tempNode = (node *) safeMalloc(sizeof(node));
    tempNode->nodeType = CONSTANT;
    tempNode->value = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
    mpfr_init2(*(tempNode->value),10);
    mpfr_set_d(*(tempNode->value),0.0,GMP_RNDN);
    return tempNode;
  } 

  getCoefficients(&degree, &coefficients, poly);

  if ((i > degree) || (coefficients[i] == NULL)) {
    tempNode = (node *) safeMalloc(sizeof(node));
    tempNode->nodeType = CONSTANT;
    tempNode->value = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
    mpfr_init2(*(tempNode->value),10);
    mpfr_set_d(*(tempNode->value),0.0,GMP_RNDN);
  } else {
    tempNode = copyTree(coefficients[i]);
  }
  
  for (k=0;k<=degree;k++) {
    if (coefficients[k] != NULL) free_memory(coefficients[k]);
  }

  free(coefficients);

  return tempNode;
}


node *getSubpolynomial(node *poly, chain *monomials, int fillDegrees, mp_prec_t prec) {
  node *tempNode, *tempNode2, *tempNode3;
  node **coefficients;
  int degree, k, currDeg, maxDegree;
  chain *curr;

  tempNode = (node *) safeMalloc(sizeof(node));
  tempNode->nodeType = CONSTANT;
  tempNode->value = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
  mpfr_init2(*(tempNode->value),prec);
  mpfr_set_d(*(tempNode->value),0.0,GMP_RNDN);

  if (!isPolynomial(poly)) {
    return tempNode;
  } 

  getCoefficients(&degree, &coefficients, poly);

  curr = monomials;

  maxDegree = -1;

  while (curr != NULL) {
    currDeg = *((int *) (curr->value));
    if (currDeg > maxDegree) maxDegree = currDeg;
    if ((currDeg >= 0) && (currDeg <= degree) && (coefficients[currDeg] != NULL)) {
      tempNode2 = (node *) safeMalloc(sizeof(node));
      tempNode2->nodeType = POW;
      tempNode3 = (node *) safeMalloc(sizeof(node));
      tempNode3->nodeType = CONSTANT;
      tempNode3->value = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
      mpfr_init2(*(tempNode3->value),prec);
      if (mpfr_set_si(*(tempNode3->value),currDeg,GMP_RNDN) != 0) {
	printMessage(1,"Warning: during subpolynomial extraction, the exponent of a power could not be represented exactly on with the given precision.\n");
      }
      tempNode2->child2 = tempNode3;
      tempNode3 = (node *) safeMalloc(sizeof(node));
      tempNode3->nodeType = VARIABLE;
      tempNode2->child1 = tempNode3;
      tempNode3 = (node *) safeMalloc(sizeof(node));
      tempNode3->nodeType = MUL;
      tempNode3->child2 = tempNode2;
      tempNode3->child1 = copyTree(coefficients[currDeg]);
      tempNode2 = (node *) safeMalloc(sizeof(node));
      tempNode2->nodeType = ADD;
      tempNode2->child2 = tempNode3;
      tempNode2->child1 = tempNode;
      tempNode = tempNode2;
    }
    curr = curr->next;
  }

  if (fillDegrees) {
    for (k=maxDegree+1;k<=degree;k++) {
      if (coefficients[k] != NULL) {
	tempNode2 = (node *) safeMalloc(sizeof(node));
	tempNode2->nodeType = POW;
	tempNode3 = (node *) safeMalloc(sizeof(node));
	tempNode3->nodeType = CONSTANT;
	tempNode3->value = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
	mpfr_init2(*(tempNode3->value),prec);
	if (mpfr_set_si(*(tempNode3->value),k,GMP_RNDN) != 0) {
	  printMessage(1,"Warning: during subpolynomial extraction, the exponent of a power could not be represented exactly on with the given precision.\n");
	}
	tempNode2->child2 = tempNode3;
	tempNode3 = (node *) safeMalloc(sizeof(node));
	tempNode3->nodeType = VARIABLE;
	tempNode2->child1 = tempNode3;
	tempNode3 = (node *) safeMalloc(sizeof(node));
	tempNode3->nodeType = MUL;
	tempNode3->child2 = tempNode2;
	tempNode3->child1 = copyTree(coefficients[k]);
	tempNode2 = (node *) safeMalloc(sizeof(node));
	tempNode2->nodeType = ADD;
	tempNode2->child2 = tempNode3;
	tempNode2->child1 = tempNode;
	tempNode = tempNode2;
      }
    }
  }


  for (k=0;k<=degree;k++) {
    if (coefficients[k] != NULL) free_memory(coefficients[k]);
  }

  free(coefficients);

  tempNode2 = horner(tempNode);

  free_memory(tempNode);
  
  return tempNode2;
}

node *makeCanonicalPolyUnsafe(node *poly, mp_prec_t prec) {
  node **coefficients;
  int degree, k;
  node *tempNode, *tempNode2, *tempNode3;

  getCoefficients(&degree, &coefficients, poly);

  tempNode = (node *) safeMalloc(sizeof(node));
  tempNode->nodeType = CONSTANT;
  tempNode->value = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
  mpfr_init2(*(tempNode->value),prec);
  mpfr_set_d(*(tempNode->value),0.0,GMP_RNDN);
  for (k=0;k<=degree;k++) {
    if (coefficients[k] != NULL) {
      if (k == 0) {
	tempNode2 = (node *) safeMalloc(sizeof(node));
	  tempNode2->nodeType = ADD;
	  tempNode2->child2 = coefficients[k];
	  tempNode2->child1 = tempNode;
	  tempNode = tempNode2;
      } else {
	if (k == 1) {
	  tempNode3 = (node *) safeMalloc(sizeof(node));
	  tempNode3->nodeType = VARIABLE;
	  tempNode2 = (node *) safeMalloc(sizeof(node));
	  tempNode2->nodeType = MUL;
	  tempNode2->child2 = tempNode3;
	  tempNode2->child1 = coefficients[k];
	  tempNode3 = (node *) safeMalloc(sizeof(node));
	  tempNode3->nodeType = ADD;
	  tempNode3->child2 = tempNode2;
	  tempNode3->child1 = tempNode;
	  tempNode = tempNode3;
	} else {
	  tempNode2 = (node *) safeMalloc(sizeof(node));
	  tempNode2->nodeType = POW;
	  tempNode3 = (node *) safeMalloc(sizeof(node));
	  tempNode3->nodeType = CONSTANT;
	  tempNode3->value = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
	  mpfr_init2(*(tempNode3->value),prec);
	  if (mpfr_set_si(*(tempNode3->value),k,GMP_RNDN) != 0) {
	    printMessage(1,"Warning: during transformation to canonical form, the exponent of a power could not be represented exactly on with the given precision.\n");
	  }
	  tempNode2->child2 = tempNode3;
	  tempNode3 = (node *) safeMalloc(sizeof(node));
	  tempNode3->nodeType = VARIABLE;
	  tempNode2->child1 = tempNode3;
	  tempNode3 = (node *) safeMalloc(sizeof(node));
	  tempNode3->nodeType = MUL;
	  tempNode3->child2 = tempNode2;
	  tempNode3->child1 = coefficients[k];
	  tempNode2 = (node *) safeMalloc(sizeof(node));
	  tempNode2->nodeType = ADD;
	  tempNode2->child2 = tempNode3;
	  tempNode2->child1 = tempNode;
	  tempNode = tempNode2;
	}
      }
    }
  }

  free(coefficients);

  tempNode2 = simplifyTreeErrorfree(tempNode);

  free_memory(tempNode);

  return tempNode2;
}


int isCanonicalMonomial(node *tree) {

  if (isConstant(tree)) return 1;

  if (isPowerOfVariable(tree)) return 1;
  
  if ((tree->nodeType == MUL) && 
      isConstant(tree->child1) &&
      isPowerOfVariable(tree->child2)) return 1;

  return 0;
}


int isCanonicalUnsafe(node *tree) {
  int deg1, deg2;
  
  if (isConstant(tree) || isCanonicalMonomial(tree)) return 1;

  if ((tree->nodeType == ADD) || (tree->nodeType == SUB)) {
    if (!isCanonicalUnsafe(tree->child1)) return 0;
    if (!isCanonicalMonomial(tree->child2)) return 0;
    deg1 = getDegree(tree->child1);
    deg2 = getDegree(tree->child2);
    if (deg1 >= deg2) return 0;
    return 1;
  }

  return 0;
}


int isCanonical(node *tree) {
  return isCanonicalUnsafe(tree);
}



node *makeCanonical(node *tree, mp_prec_t prec) {
  node *copy;
  mpfr_t *value;
  mpfr_t temp;

  if (isCanonical(tree)) {
    printMessage(7,"Information: no canonical form simplification will be performed because the given tree is already canonical.\n");
    return copyTree(tree);
  }

  if (isPolynomial(tree)) return makeCanonicalPolyUnsafe(tree,prec);

  switch (tree->nodeType) {
  case VARIABLE:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = VARIABLE;
    break;
  case CONSTANT:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = CONSTANT;
    value = (mpfr_t*) safeMalloc(sizeof(mpfr_t));
    mpfr_init2(temp,tools_precision);
    simplifyMpfrPrec(temp,*(tree->value));
    mpfr_init2(*value,mpfr_get_prec(temp));
    mpfr_set(*value,temp,GMP_RNDN);
    mpfr_clear(temp);
    copy->value = value;
    break;
  case ADD:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ADD;
    copy->child1 = makeCanonical(tree->child1,prec);
    copy->child2 = makeCanonical(tree->child2,prec);
    break;
  case SUB:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = SUB;
    copy->child1 = makeCanonical(tree->child1,prec);
    copy->child2 = makeCanonical(tree->child2,prec);
    break;
  case MUL:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = MUL;
    copy->child1 = makeCanonical(tree->child1,prec);
    copy->child2 = makeCanonical(tree->child2,prec);
    break;
  case DIV:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = DIV;
    copy->child1 = makeCanonical(tree->child1,prec);
    copy->child2 = makeCanonical(tree->child2,prec);
    break;
  case SQRT:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = SQRT;
    copy->child1 = makeCanonical(tree->child1,prec);
    break;
  case EXP:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = EXP;
    copy->child1 = makeCanonical(tree->child1,prec);
    break;
  case LOG:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = LOG;
    copy->child1 = makeCanonical(tree->child1,prec);
    break;
  case LOG_2:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = LOG_2;
    copy->child1 = makeCanonical(tree->child1,prec);
    break;
  case LOG_10:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = LOG_10;
    copy->child1 = makeCanonical(tree->child1,prec);
    break;
  case SIN:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = SIN;
    copy->child1 = makeCanonical(tree->child1,prec);
    break;
  case COS:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = COS;
    copy->child1 = makeCanonical(tree->child1,prec);
    break;
  case TAN:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = TAN;
    copy->child1 = makeCanonical(tree->child1,prec);
    break;
  case ASIN:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ASIN;
    copy->child1 = makeCanonical(tree->child1,prec);
    break;
  case ACOS:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ACOS;
    copy->child1 = makeCanonical(tree->child1,prec);
    break;
  case ATAN:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ATAN;
    copy->child1 = makeCanonical(tree->child1,prec);
    break;
  case SINH:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = SINH;
    copy->child1 = makeCanonical(tree->child1,prec);
    break;
  case COSH:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = COSH;
    copy->child1 = makeCanonical(tree->child1,prec);
    break;
  case TANH:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = TANH;
    copy->child1 = makeCanonical(tree->child1,prec);
    break;
  case ASINH:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ASINH;
    copy->child1 = makeCanonical(tree->child1,prec);
    break;
  case ACOSH:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ACOSH;
    copy->child1 = makeCanonical(tree->child1,prec);
    break;
  case ATANH:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ATANH;
    copy->child1 = makeCanonical(tree->child1,prec);
    break;
  case POW:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = POW;
    copy->child1 = makeCanonical(tree->child1,prec);
    copy->child2 = makeCanonical(tree->child2,prec);
    break;
  case NEG:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = NEG;
    copy->child1 = makeCanonical(tree->child1,prec);
    break;
  case ABS:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ABS;
    copy->child1 = makeCanonical(tree->child1,prec);
    break;
  case DOUBLE:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = DOUBLE;
    copy->child1 = makeCanonical(tree->child1,prec);
    break;
  case SINGLE:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = SINGLE;
    copy->child1 = makeCanonical(tree->child1,prec);
    break;
  case QUAD:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = QUAD;
    copy->child1 = makeCanonical(tree->child1,prec);
    break;
  case HALFPRECISION:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = HALFPRECISION;
    copy->child1 = makeCanonical(tree->child1,prec);
    break;
  case DOUBLEDOUBLE:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = DOUBLEDOUBLE;
    copy->child1 = makeCanonical(tree->child1,prec);
    break;
  case TRIPLEDOUBLE:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = TRIPLEDOUBLE;
    copy->child1 = makeCanonical(tree->child1,prec);
    break;
  case ERF: 
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ERF;
    copy->child1 = makeCanonical(tree->child1,prec);
    break;
  case ERFC:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = ERFC;
    copy->child1 = makeCanonical(tree->child1,prec);
    break;
  case LOG_1P:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = LOG_1P;
    copy->child1 = makeCanonical(tree->child1,prec);
    break;
  case EXP_M1:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = EXP_M1;
    copy->child1 = makeCanonical(tree->child1,prec);
    break;
  case DOUBLEEXTENDED:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = DOUBLEEXTENDED;
    copy->child1 = makeCanonical(tree->child1,prec);
    break;
  case LIBRARYFUNCTION:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = LIBRARYFUNCTION;
    copy->libFun = tree->libFun;
    copy->libFunDeriv = tree->libFunDeriv;
    copy->child1 = makeCanonical(tree->child1,prec);
    break;
  case PROCEDUREFUNCTION:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = PROCEDUREFUNCTION;
    copy->libFunDeriv = tree->libFunDeriv;
    copy->child1 = makeCanonical(tree->child1,prec);
    copy->child2 = copyThing(tree->child2);
    break;
  case CEIL:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = CEIL;
    copy->child1 = makeCanonical(tree->child1,prec);
    break;
  case FLOOR:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = FLOOR;
    copy->child1 = makeCanonical(tree->child1,prec);
    break;
  case NEARESTINT:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = NEARESTINT;
    copy->child1 = makeCanonical(tree->child1,prec);
    break;
  case PI_CONST:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = PI_CONST;
    break;
  case LIBRARYCONSTANT:
    copy = (node*) safeMalloc(sizeof(node));
    copy->nodeType = LIBRARYCONSTANT;
    copy->libFun = tree->libFun;
    break;

  default:
   sollyaFprintf(stderr,"Error: makeCanonical: unknown identifier in the tree\n");
   exit(1);
  }
  return copy;
}


node *makeVariable() {
  node *res;

  res = (node *) safeMalloc(sizeof(node));
  res->nodeType = VARIABLE;

  return res;
}

node *makeConstantDouble(double d) {
  node *res;
  
  res = (node *) safeMalloc(sizeof(node));
  res->nodeType = CONSTANT;
  res->value = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
  mpfr_init2(*(res->value),53);
  mpfr_set_d(*(res->value),d,GMP_RNDN);

  return res;
}

node *makeConstant(mpfr_t x) {
  node *res;
  
  res = (node *) safeMalloc(sizeof(node));
  res->nodeType = CONSTANT;
  res->value = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
  mpfr_init2(*(res->value),mpfr_get_prec(x));
  mpfr_set(*(res->value),x,GMP_RNDN);

  return res;
}

node *makeBinary(node *op1, node *op2, int opType) {
  node *res;

  res = (node *) safeMalloc(sizeof(node));
  res->nodeType = opType;
  res->child1 = op1;
  res->child2 = op2;

  return res;
}

node *makeUnary(node *op1, int opType) {
  node *res;

  res = (node *) safeMalloc(sizeof(node));
  res->nodeType = opType;
  res->child1 = op1;

  return res;
}


node *makeAdd(node *op1, node *op2) {
  return makeBinary(op1,op2,ADD);
}

node *makeSub(node *op1, node *op2) {
  return makeBinary(op1,op2,SUB);
}

node *makeMul(node *op1, node *op2) {
  return makeBinary(op1,op2,MUL);
}

node *makeDiv(node *op1, node *op2) {
  return makeBinary(op1,op2,DIV);
}

node *makeSqrt(node *op1) {
  return makeUnary(op1,SQRT);
}

node *makeExp(node *op1) {
  return makeUnary(op1,EXP);
}

node *makeLog(node *op1) {
  return makeUnary(op1,LOG);
}

node *makeLog2(node *op1) {
  return makeUnary(op1,LOG_2);
}

node *makeLog10(node *op1) {
  return makeUnary(op1,LOG_10);
}

node *makeSin(node *op1) {
  return makeUnary(op1,SIN);
}

node *makeCos(node *op1) {
  return makeUnary(op1,COS);
}

node *makeTan(node *op1) {
  return makeUnary(op1,TAN);
}

node *makeAsin(node *op1) {
  return makeUnary(op1,ASIN);
}

node *makeAcos(node *op1) {
  return makeUnary(op1,ACOS);
}

node *makeAtan(node *op1) {
  return makeUnary(op1,ATAN);
}

node *makePow(node *op1, node *op2) {
  return makeBinary(op1,op2,POW);
}

node *makeNeg(node *op1) {
  return makeUnary(op1,NEG);
}

node *makeAbs(node *op1) {
  return makeUnary(op1,ABS);
}

node *makeDouble(node *op1) {
  return makeUnary(op1,DOUBLE);
}

node *makeSingle(node *op1) {
  return makeUnary(op1,SINGLE);
}

node *makeQuad(node *op1) {
  return makeUnary(op1,QUAD);
}

node *makeHalfPrecision(node *op1) {
  return makeUnary(op1,HALFPRECISION);
}

node *makeDoubledouble(node *op1) {
  return makeUnary(op1,DOUBLEDOUBLE);
}

node *makeTripledouble(node *op1) {
  return makeUnary(op1,TRIPLEDOUBLE);
}

node *makeErf(node *op1 ) {
  return makeUnary(op1,ERF);
}

node *makeErfc(node *op1) {
  return makeUnary(op1,ERFC);
}

node *makeLog1p(node *op1) {
  return makeUnary(op1,LOG_1P);
}

node *makeExpm1(node *op1) {
  return makeUnary(op1,EXP_M1);
}

node *makeDoubleextended(node *op1) {
  return makeUnary(op1,DOUBLEEXTENDED);
}

node *makeCeil(node *op1) {
  return makeUnary(op1,CEIL);
}

node *makeFloor(node *op1) {
  return makeUnary(op1,FLOOR);
}

node *makeNearestInt(node *op1) {
  return makeUnary(op1,NEARESTINT);
}

node *makePi() {
  node *res;
  
  res = (node *) safeMalloc(sizeof(node));
  res->nodeType = PI_CONST;

  return res;
}

node *makeSinh(node *op1) {
  return makeUnary(op1,SINH);
}

node *makeCosh(node *op1) {
  return makeUnary(op1,COSH);
}

node *makeTanh(node *op1) {
  return makeUnary(op1,TANH);
}

node *makeAsinh(node *op1) {
  return makeUnary(op1,ASINH);
}

node *makeAcosh(node *op1) {
  return makeUnary(op1,ACOSH);
}

node *makeAtanh(node *op1) {
  return makeUnary(op1,ATANH);
}


int readDecimalConstant(mpfr_t result, char *str) {
  mpfr_t a,b;
  int ternary;

  mpfr_init2(a,tools_precision);
  mpfr_init2(b,tools_precision);

  mpfr_set_str(a,str,10,GMP_RNDD);
  mpfr_set_str(b,str,10,GMP_RNDU);    
  if (mpfr_cmp(a,b) != 0) {
    if (!noRoundingWarnings) {
      printMessage(1,
		   "Warning: Rounding occurred when converting the constant \"%s\" to floating-point with %d bits.\n",
		   str,(int) tools_precision);
      printMessage(1,"If safe computation is needed, try to increase the precision.\n");
    }
    ternary = mpfr_set_str(a,str,10,GMP_RNDN);
  } else {
    ternary = 0;
  }

  mpfr_set_prec(result, tools_precision);
  mpfr_set(result,a,GMP_RNDN);

  mpfr_clear(a);
  mpfr_clear(b);

  return ternary;
}

