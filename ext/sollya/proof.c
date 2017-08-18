/*

Copyright 2006-2009 by 

Laboratoire de l'Informatique du Parallelisme, 
UMR CNRS - ENS Lyon - UCB Lyon 1 - INRIA 5668

and by

LORIA (CNRS, INPL, INRIA, UHP, U-Nancy 2)

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
#include <gmp.h>
#include "proof.h"
#include <stdio.h> /* fprintf, fopen, fclose, */
#include <stdlib.h> /* exit, free, mktemp */
#include <string.h>
#include <errno.h>
#include "expression.h"
#include "infnorm.h"
#include "general.h"


void nothing(void *arg) {
  UNUSED_PARAM(arg);
  return;
}

void freeExprBoundTheo(exprBoundTheo *theo) {
  if (theo == NULL) return;
  if (theo->function != NULL) {
    free_memory(theo->function);
  }
  if (theo->x != NULL) {
    sollya_mpfi_clear(*(theo->x));
    free(theo->x);
  }
  if (theo->boundLeft != NULL) {
    sollya_mpfi_clear(*(theo->boundLeft));
    free(theo->boundLeft);
  }
  if (theo->boundRight != NULL) {
    sollya_mpfi_clear(*(theo->boundRight));
    free(theo->boundRight);
  }
  if (theo->y != NULL) {
    sollya_mpfi_clear(*(theo->y));
    free(theo->y);
  }
  if (theo->theoLeft != NULL) {
    freeExprBoundTheo(theo->theoLeft);
  }
  if (theo->theoRight != NULL) {
    freeExprBoundTheo(theo->theoRight);
  }
  if (theo->xZ != NULL) {
    sollya_mpfi_clear(*(theo->xZ));
    free(theo->xZ);
  }
  if (theo->xMXZ != NULL) {
    sollya_mpfi_clear(*(theo->xMXZ));
    free(theo->xMXZ);
  }
  if (theo->leftDerivative != NULL) {
    free_memory(theo->leftDerivative);
  }
  if (theo->rightDerivative != NULL) {
    free_memory(theo->rightDerivative);
  }
  if (theo->theoLeftLinear != NULL) {
    freeExprBoundTheo(theo->theoLeftLinear);
  }
  if (theo->theoRightLinear != NULL) {
    freeExprBoundTheo(theo->theoRightLinear);
  }
  if (theo->theoLeftConstant != NULL) {
    freeExprBoundTheo(theo->theoLeftConstant);
  }
  if (theo->theoRightConstant != NULL) {
    freeExprBoundTheo(theo->theoRightConstant);
  }
  if (theo->boundLeftLinear != NULL) {
    sollya_mpfi_clear(*(theo->boundLeftLinear));
    free(theo->boundLeftLinear);
  }
  if (theo->boundRightLinear != NULL) {
    sollya_mpfi_clear(*(theo->boundRightLinear));
    free(theo->boundRightLinear);
  }
  if (theo->boundLeftConstant != NULL) {
    sollya_mpfi_clear(*(theo->boundLeftConstant));
    free(theo->boundLeftConstant);
  }
  if (theo->boundRightConstant != NULL) {
    sollya_mpfi_clear(*(theo->boundRightConstant));
    free(theo->boundRightConstant);
  }
  free(theo);
}

void nullifyExprBoundTheo(exprBoundTheo *theo) {
  theo->function = NULL;
  theo->functionType = 0;
  theo->x = NULL;
  theo->boundLeft = NULL;
  theo->boundRight = NULL;
  theo->y = NULL;
  theo->theoLeft = NULL;
  theo->theoRight = NULL;
  theo->simplificationUsed = 0;
  theo->leftDerivative = NULL;
  theo->rightDerivative = NULL;
  theo->xZ = NULL;
  theo->xMXZ = NULL;
  theo->theoLeftConstant = NULL;
  theo->theoRightConstant = NULL;
  theo->boundLeftConstant = NULL;
  theo->boundRightConstant = NULL;
  theo->theoLeftLinear = NULL;
  theo->theoRightLinear = NULL;
  theo->boundLeftLinear = NULL;
  theo->boundRightLinear = NULL;
  theo->number = 0;
}


int exprBoundTheoIsTrivial(exprBoundTheo *theo) {
  if (theo->function == NULL) return 0;
  if ((theo->function->nodeType == CONSTANT) || (theo->function->nodeType == VARIABLE)) return 1;
  return 0;
}

void fprintDerivativeLemma(FILE *fd, node *func, node *deriv, int theoNumber, int subNumber) {
  int restoreNullPtr;
  char *var = "x";

  if (func == NULL) return;
  if (deriv == NULL) return;

  restoreNullPtr = 0;
  if (variablename == NULL) {
    variablename = var;
    restoreNullPtr = 1;
  }
  
  sollyaFprintf(fd,"Lemma %d.%d:\n",theoNumber,subNumber);
  sollyaFprintf(fd,"The first derivative of\nf(%s) = ",variablename);
  fprintTree(fd,func);
  sollyaFprintf(fd,"\nwith respect to %s is\nf\'(%s) = ",variablename,variablename);
  fprintTree(fd,deriv);
  sollyaFprintf(fd,"\n\n");

  if (restoreNullPtr) variablename = NULL;
}


int fprintExprBoundTheo(FILE *fd, exprBoundTheo *theo, int start) {
  int nextnumber, restoreNullPtr;
  char *var = "x";
  char *fx, *gx;
  
  if (theo == NULL) return start;

  if (exprBoundTheoIsTrivial(theo)) return start;

  restoreNullPtr = 0;
  if (variablename == NULL) {
    variablename = var;
    restoreNullPtr = 1;
  }

  nextnumber = start;
  if (theo->theoLeft != NULL) nextnumber = fprintExprBoundTheo(fd,theo->theoLeft,nextnumber);
  if (theo->theoRight != NULL) nextnumber = fprintExprBoundTheo(fd,theo->theoRight,nextnumber);
  if (theo->theoLeftConstant != NULL) nextnumber = fprintExprBoundTheo(fd,theo->theoLeftConstant,nextnumber);
  if (theo->theoRightConstant != NULL) nextnumber = fprintExprBoundTheo(fd,theo->theoRightConstant,nextnumber);
  if (theo->theoLeftLinear != NULL) nextnumber = fprintExprBoundTheo(fd,theo->theoLeftLinear, nextnumber);
  if (theo->theoRightLinear != NULL) nextnumber = fprintExprBoundTheo(fd,theo->theoRightLinear,nextnumber);
  theo->number = nextnumber; nextnumber++; 
  if ((theo->simplificationUsed == TAYLORPROOF) || (theo->simplificationUsed == MONOTONOCITY)) {
    fprintDerivativeLemma(fd, theo->function, theo->leftDerivative, theo->number, 1);
  }
  if ((theo->simplificationUsed == DECORRELATE) || 
      (theo->simplificationUsed == HOPITAL) ||
      (theo->simplificationUsed == HOPITAL_ON_POINT)) {
    fprintDerivativeLemma(fd, theo->theoLeft->function, theo->leftDerivative, theo->number, 1);
    fprintDerivativeLemma(fd, theo->theoRight->function, theo->rightDerivative, theo->number, 2);
  }
  
  sollyaFprintf(fd,"Theorem %d:\nFor all %s in ",theo->number,variablename);
  if (theo->x != NULL) fprintInterval(fd,*(theo->x));
  sollyaFprintf(fd,". ");
  fprintTree(fd,theo->function);
  sollyaFprintf(fd," is in ");
  if (theo->y != NULL) fprintInterval(fd,*(theo->y));
  sollyaFprintf(fd,"\nProof:\n");
  if (theo->functionType == POLYNOMIAL) {
    sollyaFprintf(fd,"The given expression is a polynomial. The given bound can be verified by the Gappa tool.\n");
  } else {
    switch (theo->simplificationUsed) {
    case TAYLORPROOF:
      sollyaFprintf(fd,"Theorem %d shows that for all %s in the given domain the given expression is bounded by ",
	      theo->theoLeft->number, variablename);
      fprintInterval(fd,*(theo->boundLeft));
      sollyaFprintf(fd,".\nFurther lemma %d.%d shows that the derivative of the given expression is ",
	      theo->number,1);
      fprintTree(fd,theo->leftDerivative);
      sollyaFprintf(fd,".\nOn the given domain, the values taken by this derivative are bounded by ");
      fprintInterval(fd,*(theo->boundLeftLinear));
      if (!exprBoundTheoIsTrivial(theo->theoLeftLinear)) {
	sollyaFprintf(fd," as shown by theorem %d.\n",theo->theoLeftLinear->number);
      } else {
	sollyaFprintf(fd," as is trivially clear.\n");
      }
      sollyaFprintf(fd,"The (quasi-) point interval _xZ = ");
      fprintInterval(fd,*(theo->xZ));
      sollyaFprintf(fd,
	   " is contained in the given domain.\nFor values %s in this interval _xZ, the given expression is bounded by ",
	      variablename);
      fprintInterval(fd,*(theo->boundLeftConstant));
      if (!exprBoundTheoIsTrivial(theo->theoLeftConstant)) {
	sollyaFprintf(fd," as shown by theorem %d.\n",theo->theoLeftConstant->number);
      } else {
	sollyaFprintf(fd," as is trivially clear.\n");
      }
      sollyaFprintf(fd,"Let _X be the given domain. The interval evaluation of (_X - _xZ) is bounded by ");
      fprintInterval(fd,*(theo->xMXZ));
      sollyaFprintf(fd,".\nBy Taylor\'s theorem the given expression is therefore bounded by ");
      fprintInterval(fd,*(theo->boundRight));
      sollyaFprintf(fd,".\nThe bounding domain for the expression given in this theorem is the intersection of\n");
      sollyaFprintf(fd,"the bound obtained by directly bounding the expression given above and the bound by\n");
      sollyaFprintf(fd,"Taylor\'s theorem.\n");
      break;
    case DECORRELATE:
      fx = (char *) safeCalloc(strlen(variablename)+4,sizeof(char));
      gx = (char *) safeCalloc(strlen(variablename)+4,sizeof(char));
      sprintf(fx,"f(%s)",variablename);
      sprintf(gx,"g(%s)",variablename);
      sollyaFprintf(fd,"The given expression is of the form ");
      fprintHeadFunction(fd,theo->function,fx,gx);
      free(fx);
      free(gx);
      sollyaFprintf(fd,"\nwhere f(%s) = ",variablename);
      fprintTree(fd,theo->theoLeft->function);
      sollyaFprintf(fd,"\nand g(%s) = ",variablename);
      fprintTree(fd,theo->theoRight->function);
      if (!exprBoundTheoIsTrivial(theo->theoLeft)) {
	sollyaFprintf(fd,"\nAs per theorem %d, for %s in the given domain, f(%s) is bounded by ",
		theo->theoLeft->number,variablename,variablename);
      } else {
	sollyaFprintf(fd,"\nTrivially for %s in the given domain, f(%s) is bounded by ",
		variablename,variablename);
      }
      fprintInterval(fd,*(theo->boundLeft));
      if (!exprBoundTheoIsTrivial(theo->theoRight)) {
	sollyaFprintf(fd,"\nAs per theorem %d, for %s in the given domain, g(%s) is bounded by ",
		theo->theoRight->number,variablename,variablename);
      } else {
	sollyaFprintf(fd,"\nTrivially for %s in the given domain, g(%s) is bounded by ",
		variablename,variablename);
      }
      fprintInterval(fd,*(theo->boundRight));
      sollyaFprintf(fd,"\nAs shown by lemma %d.%d, the derivative of f(%s) is ",
	      theo->number,1,variablename);
      fprintTree(fd,theo->leftDerivative);
      sollyaFprintf(fd,"\nFurther by lemma %d.%d, we know that the derivative of g(%s) is ",
	      theo->number,2,variablename);
      fprintTree(fd,theo->rightDerivative);
      sollyaFprintf(fd,"\nThe (quasi-) point interval _xZ = ");
      fprintInterval(fd,*(theo->xZ));
      sollyaFprintf(fd," is contained in the given domain.\n");
      if (!exprBoundTheoIsTrivial(theo->theoLeftConstant)) {
	sollyaFprintf(fd,"As per theorem %d, for %s in this interval _xZ, f(%s) is bounded by\n_xC = ",
		theo->theoLeftConstant->number, variablename, variablename);
      } else {
	sollyaFprintf(fd,"Trivially, for %s in this interval _xZ, f(%s) is bounded by\n_xC = ",
		variablename,variablename);
      }
      fprintInterval(fd,*(theo->boundLeftConstant));
      sollyaFprintf(fd,"\n");
      if (!exprBoundTheoIsTrivial(theo->theoRightConstant)) {
	sollyaFprintf(fd,"As per theorem %d, for %s in the interval _xZ, g(%s) is bounded by\n_yC = ",
		theo->theoRightConstant->number, variablename,variablename);
      } else {
	sollyaFprintf(fd,"Trivially, for %s in this interval _xZ, g(%s) is bounded by\n_yC = ",
		variablename,variablename);
      }
      fprintInterval(fd,*(theo->boundRightConstant));
      sollyaFprintf(fd,".\nLet _X be the given domain. The interval evaluation of (_X - _xZ) gives ");
      fprintInterval(fd,*(theo->xMXZ));
      sollyaFprintf(fd,".\n");
      if (!exprBoundTheoIsTrivial(theo->theoLeftLinear)) {
	sollyaFprintf(fd,"As per theorem %d, for %s in the given domain, the derivative of f(%s) is bounded by\n_xL = ",
		theo->theoLeftLinear->number, variablename,variablename);
      } else {
	sollyaFprintf(fd,"Trivially, for %s in the given domain, the derivative of f(%s) is bounded by\n_xL = ",
		variablename,variablename);
      }
      fprintInterval(fd,*(theo->boundLeftLinear));
      sollyaFprintf(fd,".\n");
      if (!exprBoundTheoIsTrivial(theo->theoRightLinear)) {
	sollyaFprintf(fd,"As per theorem %d, for %s in the given domain, the derivative of g(%s) is bounded by\n_yL = ",
		theo->theoRightLinear->number, variablename, variablename);
      } else {
	sollyaFprintf(fd,"Trivially, for %s in the given domain, the derivative of g(%s) is bounded by\n_yL = ",
		variablename,variablename);
      }
      fprintInterval(fd,*(theo->boundRightLinear));
      sollyaFprintf(fd,".\nUsing Taylor\'s theorem, one verifies that the images of the given expression, function in %s,\n",
	      variablename);
      sollyaFprintf(fd,"for %s in the given domain, are contained in the intersection of the interval evaluation of\n",
	      variablename);
      fx = (char *) safeCalloc(strlen(variablename)+4,sizeof(char));
      gx = (char *) safeCalloc(strlen(variablename)+4,sizeof(char));
      sprintf(fx,"f(%s)",variablename);
      sprintf(gx,"g(%s)",variablename);
      fprintHeadFunction(fd,theo->function,fx,gx);
      free(fx);
      free(gx);
      sollyaFprintf(fd," and of (");
      fprintHeadFunction(fd,theo->function,"_xC","_yC");
      sollyaFprintf(fd,") + (X - _xZ) * (");
      fprintHeadFunction(fd,theo->function,"_xL","_yL");
      sollyaFprintf(fd,").\nThey are therefore included in the bound\n");
      fprintInterval(fd,*(theo->y));
      sollyaFprintf(fd,"\nthat has had to be shown.\n");
      break;
    case HOPITAL_ON_POINT:
      fx = (char *) safeCalloc(strlen(variablename)+4,sizeof(char));
      gx = (char *) safeCalloc(strlen(variablename)+4,sizeof(char));
      sprintf(fx,"f(%s)",variablename);
      sprintf(gx,"g(%s)",variablename);
      sollyaFprintf(fd,"The given expression is of the form ");
      fprintHeadFunction(fd,theo->function,fx,gx);
      free(fx);
      free(gx);
      sollyaFprintf(fd,"\nwhere f(%s) = ",variablename);
      fprintTree(fd,theo->theoLeft->function);
      sollyaFprintf(fd,"\nand g(%s) = ",variablename);
      fprintTree(fd,theo->theoRight->function);
      if (!exprBoundTheoIsTrivial(theo->theoLeft)) {
	sollyaFprintf(fd,"As per theorem %d, ",theo->theoLeft->number);
      } else {
	sollyaFprintf(fd,"Trivially, ");
      }
      sollyaFprintf(fd,"for all %s in the given domain, f(%s) is bounded by the point interval ",variablename,variablename);
      fprintInterval(fd,*(theo->boundLeft));
      sollyaFprintf(fd," and therefore constant zero.\n");
      if (!exprBoundTheoIsTrivial(theo->theoRight)) {
	sollyaFprintf(fd,"As per theorem %d, ",theo->theoRight->number);
      } else {
	sollyaFprintf(fd,"Trivially, ");
      }
      sollyaFprintf(fd,"for all %s in the given domain, g(%s) is bounded by the point interval ",variablename,variablename);
      fprintInterval(fd,*(theo->boundLeft));
      sollyaFprintf(fd," and therefore constant zero, too.\nBy Hopital's rule, the given expression");
      sollyaFprintf(fd," is bounded by the interval division of boundings\n");
      sollyaFprintf(fd,"of the derivatives of the numerator f(%s) and denominator g(%s) of the given expression.\n",
	      variablename,variablename);
      sollyaFprintf(fd,"As per lemma %d.%d, the derivative of f(%s) is ",theo->number,1,variablename);
      fprintTree(fd,theo->leftDerivative);
      sollyaFprintf(fd,"As per lemma %d.%d, the derivative of g(%s) is ",theo->number,2,variablename);
      fprintTree(fd,theo->rightDerivative);
      if (!exprBoundTheoIsTrivial(theo->theoLeftLinear)) {
	sollyaFprintf(fd,"As shown by theorem %d, ",theo->theoLeftLinear->number);
      } else {
	sollyaFprintf(fd,"Trivially, ");
      }
      sollyaFprintf(fd,"this derivative of f(%s) for %s in the given domain is bounded by ",variablename,variablename);
      fprintInterval(fd,*(theo->theoLeftLinear->y));
      sollyaFprintf(fd,".\n");
      if (!exprBoundTheoIsTrivial(theo->theoRightLinear)) {
	sollyaFprintf(fd,"As shown by theorem %d, ",theo->theoRightLinear->number);
      } else {
	sollyaFprintf(fd,"Trivially, ");
      }
      sollyaFprintf(fd,"the given derivative of g(%s) for %s in the given domain is bounded by ",variablename,variablename);
      fprintInterval(fd,*(theo->theoRightLinear->y));
      sollyaFprintf(fd,".\n");
      sollyaFprintf(fd,"This yields to the following bound for the given expression with %s in the given domain: ",
	      variablename);
      fprintInterval(fd,*(theo->y));
      sollyaFprintf(fd,".\nThis is the bound that has had to be shown.\n");
      break;
    case NUMERATOR_IS_ZERO:
      fx = (char *) safeCalloc(strlen(variablename)+4,sizeof(char));
      gx = (char *) safeCalloc(strlen(variablename)+4,sizeof(char));
      sprintf(fx,"f(%s)",variablename);
      sprintf(gx,"g(%s)",variablename);
      sollyaFprintf(fd,"The given expression is of the form ");
      fprintHeadFunction(fd,theo->function,fx,gx);
      free(fx);
      free(gx);
      sollyaFprintf(fd,"\nwhere f(%s) = ",variablename);
      fprintTree(fd,theo->theoLeft->function);
      sollyaFprintf(fd,"\nand g(%s) is some function in %s.\n",variablename,variablename);
      if (exprBoundTheoIsTrivial(theo->theoLeft)) {
	sollyaFprintf(fd,"Trivially, ");
      } else {
	sollyaFprintf(fd,"As per theorem %d, ",theo->theoLeft->number);
      }
      sollyaFprintf(fd,"for %s in the given domain, f(%s) is in ",variablename,variablename);
      fprintInterval(fd,*(theo->boundLeft));
      sollyaFprintf(fd,", i.e. constant zero.\n");
      sollyaFprintf(fd,"Therefore the given expression is constant zero, i.e. bounded by ");
      fprintInterval(fd,*(theo->y));
      sollyaFprintf(fd,".\n");
      break;
    case HOPITAL:
      sollyaFprintf(fd,"The given expression is of the form ");
      fx = (char *) safeCalloc(strlen(variablename)+4,sizeof(char));
      gx = (char *) safeCalloc(strlen(variablename)+4,sizeof(char));
      sprintf(fx,"f(%s)",variablename);
      sprintf(gx,"g(%s)",variablename);
      fprintHeadFunction(fd,theo->function,fx,gx);
      sollyaFprintf(fd,"\nwhere f(%s) = ",variablename);
      fprintTree(fd,theo->theoLeft->function);
      sollyaFprintf(fd,"\nand g(%s) = ",variablename);
      fprintTree(fd,theo->theoRight->function);
      sollyaFprintf(fd,"\nLet _xZ be the point interval _xZ = ");
      fprintInterval(fd,*(theo->xZ));
      sollyaFprintf(fd,". This interval is contained in the given domain.\n");
      if (!exprBoundTheoIsTrivial(theo->theoLeftConstant)) {
	sollyaFprintf(fd,"As per theorem %d, ",theo->theoLeftConstant->number);
      } else {
	sollyaFprintf(fd,"Trivially, ");
      }
      sollyaFprintf(fd,"the images of f(%s) for %s in _xZ are in the point interval [0;0].\n",
	      variablename,variablename);
      sollyaFprintf(fd,"f(%s) is therefore constant zero in this interval _xZ.\n", variablename);
      if (!exprBoundTheoIsTrivial(theo->theoRightConstant)) {
	sollyaFprintf(fd,"As per theorem %d, ",theo->theoRightConstant->number);
      } else {
	sollyaFprintf(fd,"Trivially, ");
      }
      sollyaFprintf(fd,"the images of g(%s) for %s in _xZ are in the point interval [0;0].\n",
	      variablename,variablename);
      sollyaFprintf(fd,"g(%s) is therefore constant zero in this interval _xZ.\n", variablename);
      sollyaFprintf(fd,"Hopital's rule, i.e. a first order Taylor expansion of f(%s) and g(%s), can therefore\n",
	      variablename,variablename);
      sollyaFprintf(fd,"be used for bounding the given expression, which will be replaced by f'(%s)/g'(%s).\n",
	      variablename,variablename);
      sollyaFprintf(fd,"Lemma %d.%d shows that f'(%s) is\nf'(%s) = ",theo->number,1,variablename,variablename);
      fprintTree(fd,theo->leftDerivative);
      sollyaFprintf(fd,"\nLemma %d.%d shows that g'(%s) is\ng'(%s) = ",theo->number,2,variablename,variablename);
      fprintTree(fd,theo->rightDerivative);
      sollyaFprintf(fd,"\n");
      if (!exprBoundTheoIsTrivial(theo->theoLeftLinear)) {
	sollyaFprintf(fd,"As per theorem %d, ",theo->theoLeftLinear->number);
      } else {
	sollyaFprintf(fd,"Trivially, ");
      }
      sollyaFprintf(fd,"f'(%s)/g'(%s) is bounded by ",variablename,variablename);
      fprintInterval(fd,*(theo->boundLeftLinear));
      sollyaFprintf(fd,".\nThis is bound that has had to be proven.\n");
      free(fx);
      free(gx);
      break;
    case IMPLICATION:
      sollyaFprintf(fd,"The theorem is a direct consequence of theorem %d.\n",theo->theoLeft->number);
      break;
    default:
      sollyaFprintf(fd,"The given expression is of the form ");
      fx = (char *) safeCalloc(strlen(variablename)+4,sizeof(char));
      gx = (char *) safeCalloc(strlen(variablename)+4,sizeof(char));
      sprintf(fx,"f(%s)",variablename);
      sprintf(gx,"g(%s)",variablename);
      fprintHeadFunction(fd,theo->function,fx,gx);
      free(fx);
      free(gx);
      sollyaFprintf(fd,".\n");
      switch (arity(theo->function)) {
      case 1:
	if (!exprBoundTheoIsTrivial(theo->theoLeft)) {
	  sollyaFprintf(fd,"As per theorem %d, for %s in the given domain, f(%s) = ",
		  theo->theoLeft->number,variablename,variablename);
	  fprintTree(fd,theo->theoLeft->function);
	  sollyaFprintf(fd," is bounded by ");
	} else {
	  sollyaFprintf(fd,"For %s in the given domain, the value of f(%s) = ",variablename,variablename);
	  fprintTree(fd,theo->theoLeft->function);
	  sollyaFprintf(fd," is trivially bounded by ");
	}
	fprintInterval(fd,*(theo->boundLeft));
	sollyaFprintf(fd,"\nUsing this bound for the argument f(%s) of ",variablename);
	break;
      case 2:
	if (!exprBoundTheoIsTrivial(theo->theoLeft)) {
	  sollyaFprintf(fd,"As per theorem %d, for %s in the given domain, f(%s) = ",
		  theo->theoLeft->number,variablename,variablename);
	  fprintTree(fd,theo->theoLeft->function);
	  sollyaFprintf(fd," is bounded by ");
	} else {
	  sollyaFprintf(fd,"For %s in the given domain, the value of f(%s) = ",variablename,variablename);
	  fprintTree(fd,theo->theoLeft->function);
	  sollyaFprintf(fd," is trivially bounded by ");
	}
	fprintInterval(fd,*(theo->boundLeft));
	sollyaFprintf(fd,".\n");
	if (!exprBoundTheoIsTrivial(theo->theoRight)) {
	  sollyaFprintf(fd,"As per theorem %d, for %s in the given domain, g(%s) = ",
		  theo->theoRight->number,variablename,variablename);
	  fprintTree(fd,theo->theoRight->function);
	  sollyaFprintf(fd," is bounded by ");
	} else {
	  sollyaFprintf(fd,"For %s in the given domain, the value of g(%s)  = ",variablename,variablename);
	  fprintTree(fd,theo->theoRight->function);
	  sollyaFprintf(fd," is trivially bounded by ");
	}
	fprintInterval(fd,*(theo->boundRight));
	sollyaFprintf(fd,"\nUsing these bounds for the arguments f(%s) and g(%s) of ",variablename,variablename);
	break;
      case MONOTONOCITY: 
	sollyaFprintf(fd,"Lemma %d.%d shows that the derivative of the given expression is ",
		theo->number,1);
	fprintTree(fd,theo->leftDerivative);
	sollyaFprintf(fd,".\nOn the given domain, the values taken by this derivative are bounded by ");
	fprintInterval(fd,*(theo->boundLeftLinear));
	if (!exprBoundTheoIsTrivial(theo->theoLeftLinear)) {
	  sollyaFprintf(fd," as shown by theorem %d.\n",theo->theoLeftLinear->number);
	} else {
	  sollyaFprintf(fd," as is trivially clear.\n");
	}
	sollyaFprintf(fd,"Zero does not lie in this interval and the function is thus monotone on this interval.\n");
	sollyaFprintf(fd,"On the left endpoint of the interval, the function is bounded by ");
	fprintInterval(fd,*(theo->boundLeft));
	if (!exprBoundTheoIsTrivial(theo->theoLeft)) {
	  sollyaFprintf(fd," as shown by theorem %d.\n",theo->theoLeft->number);
	} else {
	  sollyaFprintf(fd," as is trivially clear.\n");
	}
	sollyaFprintf(fd,"On the right endpoint of the interval, the function is bounded by ");
	fprintInterval(fd,*(theo->boundRight));
	if (!exprBoundTheoIsTrivial(theo->theoRight)) {
	  sollyaFprintf(fd," as shown by theorem %d.\n",theo->theoRight->number);
	} else {
	  sollyaFprintf(fd," as is trivially clear.\n");
	}
	sollyaFprintf(fd,"The given interval is the convex hull of these two intervals bounding the function on the endpoints.\n");

	break;
      default:
	sollyaFprintf(fd,"The expression is a constant. Its bounding is trivial. Using this constant value\n");
      }
      fx = (char *) safeCalloc(strlen(variablename)+4,sizeof(char));
      gx = (char *) safeCalloc(strlen(variablename)+4,sizeof(char));
      sprintf(fx,"f(%s)",variablename);
      sprintf(gx,"g(%s)",variablename);
      fprintHeadFunction(fd,theo->function,fx,gx);
      free(fx);
      free(gx);
      sollyaFprintf(fd,", one can show the given bound for the given function.\n");
    }
  }
  sollyaFprintf(fd,"\n");

  if (restoreNullPtr) {
    variablename = NULL;
  }

  return nextnumber;
}


int equalityTheoIsTrivial(equalityTheo *theo) {
  return isSyntacticallyEqual(theo->expr1,theo->expr2);
}

int fprintEqualityTheo(FILE *fd, equalityTheo *theo, int start) {

  if (theo == NULL) return start;
  
  if (equalityTheoIsTrivial(theo)) return start;

  theo->number = start;
  sollyaFprintf(fd,"Theorem %d:\n",start);
  fprintTree(fd,theo->expr1);
  sollyaFprintf(fd," = ");
  fprintTree(fd,theo->expr2);
  sollyaFprintf(fd,"\n\n");
  return start+1;
}

void freeEqualityTheo(equalityTheo *theo) {
  if (theo == NULL) return;
  free_memory(theo->expr1);
  free_memory(theo->expr2);
  free(theo);
}


  
void fprintNumeratorSufficesLemma(FILE *fd, node *func, node *numerator, int theoNumber, int subNumber) {
  int restoreNullPtr;
  char *var = "x";
  
  if (func == NULL) return;
  if (numerator == NULL) return;

  restoreNullPtr = 0;
  if (variablename == NULL) {
    variablename = var;
    restoreNullPtr = 1;
  }

  
  sollyaFprintf(fd,"Lemma %d.%d:\n",theoNumber,subNumber);
  sollyaFprintf(fd,"The set of the zeros of the function\nf(%s) = ",variablename);
  fprintTree(fd,func);
  sollyaFprintf(fd,"\nis included in the set of the zeros of the function\ng(%s) = ",variablename);
  fprintTree(fd,numerator);
  sollyaFprintf(fd,"\n");
  sollyaFprintf(fd,"Proof:\n");
  if (func->nodeType == DIV) {
    sollyaFprintf(fd,"The function f(%s) is a fraction. The function g(%s) is the numerator of this fraction.\n",
	    variablename,variablename);
  } else {
    if (isSyntacticallyEqual(func,numerator)) 
      sollyaFprintf(fd,"The functions f(%s) and g(%s) are equal.\n",variablename,variablename);
    else 
      sollyaFprintf(fd,"The functions f(%s) and g(%s) can be shown to be equal.\n",variablename,variablename);
  }
  sollyaFprintf(fd,"\n");

  if (restoreNullPtr) variablename = NULL;
}

int fprintNoZeroTheo(FILE *fd, noZeroTheo *theo, int start) {
  int nextNumber, restoreNullPtr;
  chain *curr, *zeroFree, *joinedZeroFree, *temp;
  char *var = "x";

  if (theo == NULL) return start;

  restoreNullPtr = 0;
  if (variablename == NULL) {
    variablename = var;
    restoreNullPtr = 1;
  }

  nextNumber = start;

  nextNumber = fprintEqualityTheo(fd,theo->funcEqual,nextNumber);
  nextNumber = fprintEqualityTheo(fd,theo->derivEqual,nextNumber);

  curr = theo->exprBoundTheos;
  while (curr != NULL) {
    nextNumber = fprintExprBoundTheo(fd,((exprBoundTheo *) (curr->value)),nextNumber);
    curr = curr->next;
  }

  theo->number = nextNumber;
  fprintDerivativeLemma(fd,theo->function,theo->derivative,theo->number,1);
  nextNumber++;

  sollyaFprintf(fd,"Theorem %d:\n",theo->number);
  sollyaFprintf(fd,"The function f(%s) = ",variablename);
  fprintTree(fd,theo->function);
  sollyaFprintf(fd," has no zeros in the following domain(s):\n");
  curr = theo->exprBoundTheos;
  while (curr != NULL) {
    fprintInterval(fd,*(((exprBoundTheo *) (curr->value))->x));
    sollyaFprintf(fd,"\n");
    curr = curr->next;
  }
  sollyaFprintf(fd,"Further, more strictly speaking, the function f(%s) has no zero in the following domains:\n",variablename);
  zeroFree = NULL;
  curr = theo->exprBoundTheos;
  while (curr != NULL) {
    zeroFree = addElement(zeroFree,((exprBoundTheo *) (curr->value))->x);
    curr = curr->next;
  }
  temp = copyChain(zeroFree,copyMpfiPtr);
  freeChain(zeroFree,nothing);
  zeroFree = temp;
  joinedZeroFree = joinAdjacentIntervalsMaximally(zeroFree);
  freeChain(zeroFree,freeMpfiPtr);
  curr = joinedZeroFree;
  while (curr != NULL) {
    fprintInterval(fd,*((sollya_mpfi_t *) (curr->value)));
    sollyaFprintf(fd,"\n");
    curr = curr->next;
  }
  freeChain(joinedZeroFree,freeMpfiPtr);
  sollyaFprintf(fd,"\n");
  sollyaFprintf(fd,"Proof:\n");
  sollyaFprintf(fd,"As per lemma %d.%d, the derivative of f(%s) is f\'(%s) = ",theo->number,1,variablename,variablename);
  fprintTree(fd,theo->derivative);
  sollyaFprintf(fd,".\n");
  if (!equalityTheoIsTrivial(theo->derivEqual)) {
    sollyaFprintf(fd,"As per theorem %d, f'(%s) can be written also ",theo->derivEqual->number,variablename);
    fprintTree(fd,theo->derivEqual->expr2);
    sollyaFprintf(fd,"\nIn the following assume this equality.\n");
  }
  if (!equalityTheoIsTrivial(theo->funcEqual)) {
    sollyaFprintf(fd,"As per theorem %d, f(%s) can be written also ",theo->funcEqual->number,variablename);
    fprintTree(fd,theo->funcEqual->expr2);
    sollyaFprintf(fd,"\nIn the following assume this equality.\n");
  }
  sollyaFprintf(fd,"Theorem(s) ");
  curr = theo->exprBoundTheos;
  while (curr != NULL) {
    if ((curr->next == NULL) && (curr != theo->exprBoundTheos)) sollyaFprintf(fd,"and ");
    sollyaFprintf(fd,"%d",((exprBoundTheo *) (curr->value))->number);
    if (curr->next != NULL) sollyaFprintf(fd,", ");
    curr = curr->next;
  }
  sollyaFprintf(fd,"\nshow(s) (using f'(%s)) that all images f(%s) for %s in one of the domains\n",
	  variablename,variablename,variablename);
  sollyaFprintf(fd,"given in this theorem are contained in (the union of) the following interval(s)\n");
  curr = theo->exprBoundTheos;
  while (curr != NULL) {
    fprintInterval(fd,*(((exprBoundTheo *) (curr->value))->y));
    sollyaFprintf(fd,"\n");
    curr = curr->next;
  }
  sollyaFprintf(fd,"Clearly, none of these intervals (this interval) contains zero.\n");
  sollyaFprintf(fd,"Thus f(%s) has no zero in the given intervals.\n",variablename);
  sollyaFprintf(fd,"Concerning the second (shorter) list of intervals, on remarks that it is a union of the\n");
  sollyaFprintf(fd,"intervals in the first list.");
  sollyaFprintf(fd,"\n\n");

  if (restoreNullPtr) variablename = NULL;

  return nextNumber;
}


void freeExprBoundTheoOnVoid(void *theo) {
  freeExprBoundTheo((exprBoundTheo *) theo);
}

void freeNoZeroTheo(noZeroTheo *theo) {
  if (theo == NULL) return;
  free_memory(theo->function);
  free_memory(theo->derivative);
  freeEqualityTheo(theo->funcEqual);
  freeEqualityTheo(theo->derivEqual);
  freeChain(theo->exprBoundTheos,freeExprBoundTheoOnVoid);
  free(theo);
}


int fprintInfnormTheo(FILE *fd, infnormTheo *theo, int start) {
  int nextNumber, innerLeftNumber, innerRightNumber, num, restoreNullPtr;
  chain *curr, *zeroFree, *joinedZeroFree, *temp;
  mpfr_t a, b, l, u, fr, fl, tl, tr;
  mp_prec_t p, prec;
  sollya_mpfi_t *currMpfi;
  char *var = "x";

  if (theo == NULL) return start;

  restoreNullPtr = 0;
  if (variablename == NULL) {
    variablename = var;
    restoreNullPtr = 1;
  }

  nextNumber = start;
  nextNumber = fprintNoZeroTheo(fd,theo->noZeros,nextNumber);
  nextNumber = fprintExprBoundTheo(fd,theo->evalLeftBound,nextNumber);
  nextNumber = fprintExprBoundTheo(fd,theo->evalRightBound,nextNumber);

  curr = theo->evalOnZeros;
  while (curr != NULL) {
    nextNumber = fprintExprBoundTheo(fd,((exprBoundTheo *) (curr->value)),nextNumber);
    curr = curr->next;
  }

  theo->number = nextNumber;
  fprintDerivativeLemma(fd,theo->function,theo->derivative,theo->number,1);
  fprintNumeratorSufficesLemma(fd,theo->derivative,theo->numeratorOfDerivative,theo->number,2);
  fprintDerivativeLemma(fd,theo->numeratorOfDerivative,theo->derivativeOfNumeratorOfDerivative,theo->number,3);
  nextNumber++;

  sollyaFprintf(fd,"Theorem %d:\n",theo->number);
  sollyaFprintf(fd,"Assuming that f is C^2, the infinity norm of\nf(%s) = ",variablename);
  fprintTree(fd,theo->function);
  sollyaFprintf(fd,"\nfor %s in ",variablename);
  if (theo->domain != NULL) fprintInterval(fd,*(theo->domain));
  sollyaFprintf(fd," ");
  if (theo->excludedIntervals != NULL) {
    sollyaFprintf(fd,"without the (union of the) following interval(s)\n");
    curr = theo->excludedIntervals;
    while (curr != NULL) {
      fprintInterval(fd,*((sollya_mpfi_t *) (curr->value)));
      sollyaFprintf(fd,"\n");
      curr = curr->next;
    }
  }
  sollyaFprintf(fd,"is bounded by ");
  if (theo->infnorm != NULL)  fprintInterval(fd,*(theo->infnorm));
  sollyaFprintf(fd,"\n");
  sollyaFprintf(fd,"Proof:\n");
  sollyaFprintf(fd,"As per lemma %d.%d, the derivative f'(%s) of the given function f(%s) is\nf'(%s) = ",
	  theo->number,1,variablename,variablename,variablename);
  fprintTree(fd,theo->derivative);
  sollyaFprintf(fd,
    ".\nLemma %d.%d shows that the set of the zeros of f'(%s), i.e. of the local extrema of f(%s) (since f is C^2), is a\n",
	  theo->number,2,variablename,variablename);
  sollyaFprintf(fd,"subset of the zeros of\ng(%s) = ",variablename);
  fprintTree(fd,theo->numeratorOfDerivative);
  sollyaFprintf(fd,".\nThe derivative of g(%s) is g'(%s) = ",variablename,variablename);
  fprintTree(fd,theo->derivativeOfNumeratorOfDerivative);
  sollyaFprintf(fd," as shown by lemma %d.%d.\n",theo->number,3);
  sollyaFprintf(fd,"As per theorem %d, g(%s) has no zero in the following domains:\n",
	  theo->noZeros->number,variablename);
  zeroFree = NULL;
  curr = theo->noZeros->exprBoundTheos;
  while (curr != NULL) {
    zeroFree = addElement(zeroFree,((exprBoundTheo *) (curr->value))->x);
    curr = curr->next;
  }
  temp = copyChain(zeroFree,copyMpfiPtr);
  freeChain(zeroFree,nothing);
  zeroFree = temp;
  joinedZeroFree = joinAdjacentIntervalsMaximally(zeroFree);
  freeChain(zeroFree,freeMpfiPtr);
  curr = joinedZeroFree;
  while (curr != NULL) {
    fprintInterval(fd,*((sollya_mpfi_t *) (curr->value)));
    sollyaFprintf(fd,"\n");
    curr = curr->next;
  }
  freeChain(joinedZeroFree,freeMpfiPtr);
  sollyaFprintf(fd,"This intervals and the following, whose set will be notated Z,\n");
  curr = theo->evalOnZeros;
  while (curr != NULL) {
    fprintInterval(fd,*(((exprBoundTheo *) (curr->value))->x));
    sollyaFprintf(fd,"\n");
    curr = curr->next;
  }
  sollyaFprintf(fd,"are a complete partitioning of the domain ");
  fprintInterval(fd,*(theo->domain));
  if (theo->excludedIntervals != NULL) {
    sollyaFprintf(fd," without the (union of the) following interval(s)\n");
    curr = theo->excludedIntervals;
    while (curr != NULL) {
      fprintInterval(fd,*((sollya_mpfi_t *) (curr->value)));
      sollyaFprintf(fd,"\n");
      curr = curr->next;
    }
  }
  sollyaFprintf(fd,".\nTheorems %d and %d show that the absolute value of f(%s) on the bounds of the given domain a = ",
	  theo->evalLeftBound->number, theo->evalRightBound->number, variablename);
  mpfr_init2(a,sollya_mpfi_get_prec(*(theo->domain)));
  mpfr_init2(b,sollya_mpfi_get_prec(*(theo->domain)));  
  sollya_mpfi_get_left(a,*(theo->domain));
  sollya_mpfi_get_right(b,*(theo->domain));
  fprintValue(fd,a);
  sollyaFprintf(fd," and b = ");
  fprintValue(fd,b);
  sollyaFprintf(fd,"\nis less than or equal to the upper bound u = ");
  mpfr_init2(u,sollya_mpfi_get_prec(*(theo->infnorm)));
  mpfr_init2(l,sollya_mpfi_get_prec(*(theo->infnorm)));  
  sollya_mpfi_get_left(l,*(theo->infnorm));
  sollya_mpfi_get_right(u,*(theo->infnorm));
  fprintValue(fd,u);
  sollyaFprintf(fd," given for the infinity norm.\nTheorem(s) ");
  curr = theo->evalOnZeros;
  while (curr != NULL) {
    if ((curr->next == NULL) && (curr != theo->evalOnZeros)) sollyaFprintf(fd,"and ");
    sollyaFprintf(fd,"%d",((exprBoundTheo *) (curr->value))->number);
    if (curr->next != NULL) sollyaFprintf(fd,", ");
    curr = curr->next;
  }
  sollyaFprintf(fd," show(s) using f'(%s) that the absolute value of f(%s) is less than or equal to this upper bound u\n",
	  variablename,variablename);
  sollyaFprintf(fd,"on all domains Z where g(%s) may have a zero, i.e. where f'(%s) may have a zero and f(%s) a local extremum.\n",
	  variablename, variablename,variablename);
  sollyaFprintf(fd,"Theorem %d shows that there are no other domains with zeros of f'(%s) and we have shown that\n",
	  theo->noZeros->number,variablename);
  sollyaFprintf(fd,"the partitioning is complete.\n");
  prec = sollya_mpfi_get_prec(*(theo->evalLeftBound->y));
  p = sollya_mpfi_get_prec(*(theo->evalRightBound->y));
  if (p > prec) prec = p;
  curr = theo->evalOnZeros;
  while (curr != NULL) {
    p = sollya_mpfi_get_prec(*(((exprBoundTheo *) (curr->value))->y));
    if (p > prec) prec = p;
    curr = curr->next;
  }
  mpfr_init2(fl,prec);
  mpfr_init2(fr,prec);
  mpfr_init2(tl,prec);
  mpfr_init2(tr,prec);
  sollya_mpfi_get_left(fr,*(theo->evalLeftBound->y));
  sollya_mpfi_get_right(fl,*(theo->evalLeftBound->y));
  innerLeftNumber = theo->evalLeftBound->number;
  innerRightNumber = theo->evalLeftBound->number;
  sollya_mpfi_get_left(tl,*(theo->evalRightBound->y));
  sollya_mpfi_get_right(tr,*(theo->evalRightBound->y));
  if (mpfr_less_p(tr,fl)) {
    mpfr_set(fl,tr,GMP_RNDN);
    innerLeftNumber = theo->evalRightBound->number;
  }
  if (mpfr_greater_p(tl,fr)) {
    mpfr_set(fr,tl,GMP_RNDN);
    innerRightNumber = theo->evalRightBound->number;
  }
  curr = theo->evalOnZeros;
  while (curr != NULL) {
    currMpfi = ((exprBoundTheo *) curr->value)->y;
    sollya_mpfi_get_left(tl,*currMpfi);
    sollya_mpfi_get_right(tr,*currMpfi);
    if (mpfr_less_p(tr,fl)) {
      mpfr_set(fl,tr,GMP_RNDN);
      innerLeftNumber = ((exprBoundTheo *) curr->value)->number;
    }
    if (mpfr_greater_p(tl,fr)) {
      mpfr_set(fr,tl,GMP_RNDN);
      innerRightNumber = ((exprBoundTheo *) curr->value)->number;
    } 
    curr = curr->next;
  }
  if (mpfr_greater_p(fl,fr)) {
    sollyaFprintf(fd,"Theorem %d shows that on a domain bound or a domain containing a zero of f'(%s)\n",
	    innerLeftNumber,variablename);
    sollyaFprintf(fd,"the value of f(%s) can principally be as great as ",variablename);
    fprintValue(fd,fl);
    sollyaFprintf(fd,".\nTheorem %d shows that on a domain bound or a domain containing a zero of f'(%s)\n",
	    innerRightNumber,variablename);
    sollyaFprintf(fd,"the value of f(%s) can principally as small as ",variablename);
    fprintValue(fd,fr);
    sollyaFprintf(fd,".\nThe lower bound for the infinity norm on f(%s) in the given domain l = ",variablename);
    fprintValue(fd,l);
    sollyaFprintf(fd," is therefore the best lower bound that can be shown in this proof.\n");
  } else {
    mpfr_neg(fl,fl,GMP_RNDN);
    if (mpfr_greater_p(fl,fr)) {
      mpfr_set(fr,fl,GMP_RNDN);
      num = innerLeftNumber;
    } else {
      num = innerRightNumber;
    }
    sollyaFprintf(fd,"Theorem %d shows that on a domain bound or a domain containing a zero of f'(%s)\n",
	    num,variablename);
    sollyaFprintf(fd,"the absolute value of f(%s) is not less than the lower bound l = ",variablename);
    fprintValue(fd,l);
    sollyaFprintf(fd," given for the infinity norm.");
  }
  sollyaFprintf(fd,"\n\n");
  mpfr_clear(a);
  mpfr_clear(b);
  mpfr_clear(u);
  mpfr_clear(l);
  mpfr_clear(fr);
  mpfr_clear(fl);
  mpfr_clear(tr);
  mpfr_clear(tl);

  if (restoreNullPtr) variablename = NULL;

  return nextNumber;
}


void freeInfnormTheo(infnormTheo *theo) {
  if (theo == NULL) return;
  free_memory(theo->function);
  freeMpfiPtr(theo->domain);
  freeMpfiPtr(theo->infnorm);
  free_memory(theo->derivative);
  free_memory(theo->numeratorOfDerivative);
  free_memory(theo->derivativeOfNumeratorOfDerivative);
  freeChain(theo->excludedIntervals,freeMpfiPtr);
  freeNoZeroTheo(theo->noZeros);
  freeExprBoundTheo(theo->evalLeftBound);
  freeExprBoundTheo(theo->evalRightBound);
  freeChain(theo->evalOnZeros,freeExprBoundTheoOnVoid);
  free(theo);
}


gappaAssignment *newGappaOperation(int opType, int relErrBits, 
				   int resultType, int resultOverlap, char *resultVariable,
				   int operand1UsedType, int operand1ComingType, char *operand1Variable,
				   int operand2UsedType, int operand2ComingType, char *operand2Variable) {
  gappaAssignment *newAssignment;

  newAssignment = (gappaAssignment *) safeCalloc(1,sizeof(gappaAssignment));
  
  newAssignment->opType = opType;
  newAssignment->relErrBits = relErrBits;
  newAssignment->resultType = resultType;
  newAssignment->resultOverlap = resultOverlap;
  if (resultVariable != NULL) {
    newAssignment->resultVariable = (char *) safeCalloc(strlen(resultVariable)+1,sizeof(char));
    strcpy(newAssignment->resultVariable,resultVariable);
  } else {
    newAssignment->resultVariable = NULL;
  }
  newAssignment->operand1UsedType = operand1UsedType;
  newAssignment->operand1ComingType = operand1ComingType;
  if (operand1Variable != NULL) {
    newAssignment->operand1Variable = (char *) safeCalloc(strlen(operand1Variable)+1,sizeof(char));
    strcpy(newAssignment->operand1Variable,operand1Variable);
  } else {
    newAssignment->operand1Variable = NULL;
  }
  newAssignment->operand2UsedType = operand2UsedType;
  newAssignment->operand2ComingType = operand2ComingType;
  if (operand2Variable != NULL) {
    newAssignment->operand2Variable = (char *) safeCalloc(strlen(operand2Variable)+1,sizeof(char));
    strcpy(newAssignment->operand2Variable,operand2Variable);
  } else {
    newAssignment->operand2Variable = NULL;
  }


  return newAssignment;
}

gappaAssignment *newGappaConstant(int resultType, char *resultVariable, double constHi, double constMi, double constLo) {
  gappaAssignment *newAssignment;

  newAssignment = (gappaAssignment *) safeCalloc(1,sizeof(gappaAssignment));
  
  newAssignment->opType = GAPPA_CONST;
  newAssignment->resultType = resultType;
  newAssignment->resultOverlap = 53;
  newAssignment->resultVariable = (char *) safeCalloc(strlen(resultVariable)+1,sizeof(char));
  strcpy(newAssignment->resultVariable,resultVariable);
  newAssignment->constHi = constHi;
  newAssignment->constMi = constMi;
  newAssignment->constLo = constLo;

  return newAssignment;
}


void freeGappaAssignment(gappaAssignment *assign) {
  if (assign == NULL) return;
  free(assign->resultVariable);
  free(assign->operand1Variable);
  free(assign->operand2Variable);
  free(assign->operand3Variable);
  free(assign);
}

void freeGappaAssignmentOnVoid(void *assign) {
  freeGappaAssignment((gappaAssignment *) assign);
}

void freeGappaProof(gappaProof *proof) {
  int i;
  if (proof == NULL) return;
  mpfr_clear(proof->a);
  mpfr_clear(proof->b);
  free(proof->variableName);
  free(proof->resultName);
  free_memory(proof->polynomToImplement);
  free_memory(proof->polynomImplemented);
  for (i=0;i<proof->assignmentsNumber;i++) {
    freeGappaAssignment(proof->assignments[i]);
  }
  free(proof->assignments);
  free(proof);
}

void fprintGappaAssignmentAsMaths(FILE *fd, gappaAssignment *assign) {
  switch (assign->opType) {
  case GAPPA_CONST:   
    switch (assign->resultType) {
    case 3:
      sollyaFprintf(fd,"M%s = %shml;\n",assign->resultVariable,assign->resultVariable);
      break;
    case 2:
      sollyaFprintf(fd,"M%s = %shm;\n",assign->resultVariable,assign->resultVariable);
      break;
    case 1:
      sollyaFprintf(fd,"M%s = %sh;\n",assign->resultVariable,assign->resultVariable);
      break;
    default:
      sollyaFprintf(stderr,"Error: fprintGappaAssignmentAsMaths: unknown result type (%d) in the assignment\n",assign->opType);
      exit(1);
    }
    break;
  case GAPPA_ADD_EXACT: 
    sollyaFprintf(fd,"M%s = M%s + M%s;\n",assign->resultVariable,assign->operand1Variable,assign->operand2Variable);
    break;
  case GAPPA_MUL_EXACT: 
    sollyaFprintf(fd,"M%s = M%s * M%s;\n",assign->resultVariable,assign->operand1Variable,assign->operand2Variable);
    break;
  case GAPPA_ADD_DOUBLE: 
    sollyaFprintf(fd,"M%s = M%s + M%s;\n",assign->resultVariable,assign->operand1Variable,assign->operand2Variable);
    break;
  case GAPPA_MUL_DOUBLE: 
    sollyaFprintf(fd,"M%s = M%s * M%s;\n",assign->resultVariable,assign->operand1Variable,assign->operand2Variable);
    break;
  case GAPPA_RENORMALIZE: 
    sollyaFprintf(fd,"M%s = M%s;\n",assign->resultVariable,assign->operand1Variable);
    break;
  case GAPPA_ADD_REL: 
    sollyaFprintf(fd,"M%s = M%s + M%s;\n",assign->resultVariable,assign->operand1Variable,assign->operand2Variable);
    break;
  case GAPPA_MUL_REL: 
    sollyaFprintf(fd,"M%s = M%s * M%s;\n",assign->resultVariable,assign->operand1Variable,assign->operand2Variable);
    break;
  case GAPPA_FMA_REL: 
    sollyaFprintf(fd,"M%s = (M%s * M%s) + M%s;\n",
	    assign->resultVariable,assign->operand3Variable,assign->operand2Variable,assign->operand1Variable);
    break;
  case GAPPA_COPY: 
    sollyaFprintf(fd,"M%s = M%s;\n",assign->resultVariable,assign->operand1Variable);
    break;
  default:
    sollyaFprintf(stderr,"Error: fprintGappaAssignmentAsMaths: unknown operation type (%d) in the assignment\n",assign->opType);
    exit(1);
  }
}

void fprintExpansionSuffix(FILE *fd, int type) {
  switch (type) {
  case 3:
    sollyaFprintf(fd,"hml");
    break;
  case 2:
    sollyaFprintf(fd,"hm");
    break;
  case 1:
    sollyaFprintf(fd,"h");
    break;
  default:
    sollyaFprintf(stderr,"Error: fprintExpansionSuffix: unknown result type (%d) to print\n",type);
    exit(1);
  }
}

void fprintGappaAssignmentAsArith(FILE *fd, gappaAssignment *assign) {
  switch (assign->opType) {
  case GAPPA_CONST:   
    switch (assign->resultType) {
    case 3:
      sollyaFprintf(fd,"%sh = double(%1.80e);\n",assign->resultVariable,assign->constHi);
      sollyaFprintf(fd,"%sm = double(%1.80e);\n",assign->resultVariable,assign->constMi);
      sollyaFprintf(fd,"%sl = double(%1.80e);\n",assign->resultVariable,assign->constLo);
      sollyaFprintf(fd,"%shml = %sh + %sm + %sl;\n\n",assign->resultVariable,
	      assign->resultVariable,assign->resultVariable,assign->resultVariable);
      break;
    case 2:
      sollyaFprintf(fd,"%sh = double(%1.80e);\n",assign->resultVariable,assign->constHi);
      sollyaFprintf(fd,"%sm = double(%1.80e);\n",assign->resultVariable,assign->constMi);
      sollyaFprintf(fd,"%shm = %sh + %sm;\n\n",assign->resultVariable,
	      assign->resultVariable,assign->resultVariable);
      break;
    case 1:
      sollyaFprintf(fd,"%sh = double(%1.80e);\n\n",assign->resultVariable,assign->constHi);
      break;
    default:
      sollyaFprintf(stderr,"Error: fprintGappaAssignmentAsArith: unknown result type (%d) in the assignment\n",assign->resultType);
      exit(1);
    }
    break;
  case GAPPA_ADD_EXACT: 
    sollyaFprintf(fd,"%shm = %s",assign->resultVariable,assign->operand1Variable);
    fprintExpansionSuffix(fd,assign->operand1UsedType);
    sollyaFprintf(fd," + %s",assign->operand2Variable);
    fprintExpansionSuffix(fd,assign->operand2UsedType);
    sollyaFprintf(fd,";\n");
    sollyaFprintf(fd,"%sh = double(%shm);\n",assign->resultVariable,assign->resultVariable);
    sollyaFprintf(fd,"%sm = %shm - %sh;\n\n",assign->resultVariable,assign->resultVariable,assign->resultVariable);
    break;
  case GAPPA_MUL_EXACT: 
    sollyaFprintf(fd,"%shm = %s",assign->resultVariable,assign->operand1Variable);
    fprintExpansionSuffix(fd,assign->operand1UsedType);
    sollyaFprintf(fd," * %s",assign->operand2Variable);
    fprintExpansionSuffix(fd,assign->operand2UsedType);
    sollyaFprintf(fd,";\n");
    sollyaFprintf(fd,"%sh = double(%shm);\n",assign->resultVariable,assign->resultVariable);
    sollyaFprintf(fd,"%sm = %shm - %sh;\n\n",assign->resultVariable,assign->resultVariable,assign->resultVariable);
    break;
  case GAPPA_ADD_DOUBLE: 
    if ((assign->operand1UsedType == 2) && (assign->operand2UsedType == 2)) {
      sollyaFprintf(fd,"%sh double= (%sh + %sm) + (%sh + %sm);\n",
	      assign->resultVariable,
	      assign->operand1Variable,assign->operand1Variable,
	      assign->operand2Variable,assign->operand2Variable);
    } else {
      if (assign->operand1UsedType == 2) {
	sollyaFprintf(fd,"%sh double= (%sh + %sm) + %sm;\n",
		assign->resultVariable,assign->operand1Variable,assign->operand1Variable,assign->operand2Variable);
      } else {
	if (assign->operand2UsedType == 2) {
	  sollyaFprintf(fd,"%sh double= %sh + (%sh + %sm);\n",
		  assign->resultVariable,assign->operand1Variable,assign->operand2Variable,assign->operand2Variable);
	} else {
	  sollyaFprintf(fd,"%sh = double(%sh + %sh);\n",
		  assign->resultVariable,assign->operand1Variable,assign->operand2Variable);
	}
      }
    }
    break;
  case GAPPA_MUL_DOUBLE: 
    if ((assign->operand1UsedType == 2) && (assign->operand2UsedType == 2)) {
      sollyaFprintf(fd,"%sh double= (%sh + %sm) * (%sh + %sm);\n",
	      assign->resultVariable,
	      assign->operand1Variable,assign->operand1Variable,
	      assign->operand2Variable,assign->operand2Variable);
    } else {
      if (assign->operand1UsedType == 2) {
	sollyaFprintf(fd,"%sh double= (%sh + %sm) * %sm;\n",
		assign->resultVariable,assign->operand1Variable,assign->operand1Variable,assign->operand2Variable);
      } else {
	if (assign->operand2UsedType == 2) {
	  sollyaFprintf(fd,"%sh double= %sh * (%sh + %sm);\n",
		  assign->resultVariable,assign->operand1Variable,assign->operand2Variable,assign->operand2Variable);
	} else {
	  sollyaFprintf(fd,"%sh = double(%sh * %sh);\n",
		  assign->resultVariable,assign->operand1Variable,assign->operand2Variable);
	}
      }
    }
    break;
  case GAPPA_RENORMALIZE: 
    sollyaFprintf(fd,"%shml = %shml;\n",assign->resultVariable,assign->operand1Variable);
    sollyaFprintf(fd,"%sml = %shml - %sh;\n",assign->resultVariable,assign->resultVariable,assign->resultVariable);
    sollyaFprintf(fd,"%sm = double(%sml);\n",assign->resultVariable,assign->resultVariable);
    sollyaFprintf(fd,"%sl = %sml - %sm;\n",assign->resultVariable,assign->resultVariable,assign->resultVariable);
    sollyaFprintf(fd,"%shm = %sh + %sm;\n",assign->resultVariable,assign->resultVariable,assign->resultVariable);
    sollyaFprintf(fd,"overlap_%s = %sm / %sh;\n\n",assign->resultVariable,assign->resultVariable,assign->resultVariable);
    break;
  case GAPPA_ADD_REL: 
    sollyaFprintf(fd,"%s",assign->resultVariable);
    fprintExpansionSuffix(fd,assign->resultType);
    sollyaFprintf(fd," = add_rel<%d>(%s",assign->relErrBits,assign->operand1Variable);
    fprintExpansionSuffix(fd,assign->operand1UsedType);
    sollyaFprintf(fd,",%s",assign->operand2Variable);
    fprintExpansionSuffix(fd,assign->operand2UsedType);
    sollyaFprintf(fd,");\n");
    switch (assign->resultType) {
    case 3:
      sollyaFprintf(fd,"%sml = %shml - %sh;\n",assign->resultVariable,assign->resultVariable,assign->resultVariable);
      sollyaFprintf(fd,"%sm = double(%sml);\n",assign->resultVariable,assign->resultVariable);
      sollyaFprintf(fd,"%sl = %sml - %sm;\n",assign->resultVariable,assign->resultVariable,assign->resultVariable);
      sollyaFprintf(fd,"%shm = %sh + %sm;\n",assign->resultVariable,assign->resultVariable,assign->resultVariable);
      sollyaFprintf(fd,"overlap_%s = %sm / %sh;\n\n",assign->resultVariable,assign->resultVariable,assign->resultVariable);
      break;
    case 2:
      sollyaFprintf(fd,"%sh = double(%shm);\n",assign->resultVariable,assign->resultVariable);
      sollyaFprintf(fd,"%sm = %shm - %sh;\n\n",assign->resultVariable,assign->resultVariable,assign->resultVariable);
      break;
    default:
      sollyaFprintf(stderr,"Error: fprintGappaAssignmentAsArith: unhandlable result type (%d) in the assignment\n",assign->resultType);
      exit(1);
    }
    break;
  case GAPPA_MUL_REL: 
    sollyaFprintf(fd,"%s",assign->resultVariable);
    fprintExpansionSuffix(fd,assign->resultType);
    sollyaFprintf(fd," = mul_rel<%d>(%s",assign->relErrBits,assign->operand1Variable);
    fprintExpansionSuffix(fd,assign->operand1UsedType);
    sollyaFprintf(fd,",%s",assign->operand2Variable);
    fprintExpansionSuffix(fd,assign->operand2UsedType);
    sollyaFprintf(fd,");\n");
    switch (assign->resultType) {
    case 3:
      sollyaFprintf(fd,"%sml = %shml - %sh;\n",assign->resultVariable,assign->resultVariable,assign->resultVariable);
      sollyaFprintf(fd,"%sm = double(%sml);\n",assign->resultVariable,assign->resultVariable);
      sollyaFprintf(fd,"%sl = %sml - %sm;\n",assign->resultVariable,assign->resultVariable,assign->resultVariable);
      sollyaFprintf(fd,"%shm = %sh + %sm;\n",assign->resultVariable,assign->resultVariable,assign->resultVariable);
      sollyaFprintf(fd,"overlap_%s = %sm / %sh;\n\n",assign->resultVariable,assign->resultVariable,assign->resultVariable);
      break;
    case 2:
      sollyaFprintf(fd,"%sh = double(%shm);\n",assign->resultVariable,assign->resultVariable);
      sollyaFprintf(fd,"%sm = %shm - %sh;\n\n",assign->resultVariable,assign->resultVariable,assign->resultVariable);
      break;
    default:
      sollyaFprintf(stderr,"Error: fprintGappaAssignmentAsArith: unhandlable result type (%d) in the assignment\n",assign->resultType);
      exit(1);
    }
    break;
  case GAPPA_FMA_REL: 
    sollyaFprintf(fd,"%s",assign->resultVariable);
    fprintExpansionSuffix(fd,assign->resultType);
    sollyaFprintf(fd," = fma_rel<%d>(%s",assign->relErrBits,assign->operand3Variable);
    fprintExpansionSuffix(fd,assign->operand3UsedType);
    sollyaFprintf(fd,",%s",assign->operand2Variable);
    fprintExpansionSuffix(fd,assign->operand2UsedType);
    sollyaFprintf(fd,",%s",assign->operand1Variable);
    fprintExpansionSuffix(fd,assign->operand1UsedType);
    sollyaFprintf(fd,");\n");
    switch (assign->resultType) {
    case 3:
      sollyaFprintf(fd,"%sml = %shml - %sh;\n",assign->resultVariable,assign->resultVariable,assign->resultVariable);
      sollyaFprintf(fd,"%sm = double(%sml);\n",assign->resultVariable,assign->resultVariable);
      sollyaFprintf(fd,"%sl = %sml - %sm;\n",assign->resultVariable,assign->resultVariable,assign->resultVariable);
      sollyaFprintf(fd,"%shm = %sh + %sm;\n",assign->resultVariable,assign->resultVariable,assign->resultVariable);
      sollyaFprintf(fd,"overlap_%s = %sm / %sh;\n\n",assign->resultVariable,assign->resultVariable,assign->resultVariable);
      break;
    case 2:
      sollyaFprintf(fd,"%sh = double(%shm);\n",assign->resultVariable,assign->resultVariable);
      sollyaFprintf(fd,"%sm = %shm - %sh;\n\n",assign->resultVariable,assign->resultVariable,assign->resultVariable);
      break;
    default:
      sollyaFprintf(stderr,"Error: fprintGappaAssignmentAsArith: unhandlable result type (%d) in the assignment\n",assign->resultType);
      exit(1);
    }
    break;
  case GAPPA_COPY: 
    switch (assign->resultType) {
    case 3:
      sollyaFprintf(fd,"%shml = %shml;\n",assign->resultVariable,assign->operand1Variable);
      sollyaFprintf(fd,"%shm = %shm;\n",assign->resultVariable,assign->operand1Variable);
      sollyaFprintf(fd,"%sml = %sml;\n",assign->resultVariable,assign->operand1Variable);
      sollyaFprintf(fd,"%sh = %sh;\n",assign->resultVariable,assign->operand1Variable);
      sollyaFprintf(fd,"%sm = %sm;\n",assign->resultVariable,assign->operand1Variable);
      sollyaFprintf(fd,"%sl = %sl;\n",assign->resultVariable,assign->operand1Variable);
      sollyaFprintf(fd,"overlap_%s = overlap_%s;\n\n",assign->resultVariable,assign->operand1Variable);
      break;
    case 2:
      sollyaFprintf(fd,"%shm = %shm;\n",assign->resultVariable,assign->operand1Variable);
      sollyaFprintf(fd,"%sh = %sh;\n",assign->resultVariable,assign->operand1Variable);
      sollyaFprintf(fd,"%sm = %sm;\n\n",assign->resultVariable,assign->operand1Variable);
      break;
    case 1: 
      sollyaFprintf(fd,"%sh = %sh;\n\n",assign->resultVariable,assign->operand1Variable);    
      break;
    default:
      sollyaFprintf(stderr,"Error: fprintGappaAssignmentAsArith: unknown result type (%d) in the assignment\n",assign->resultType);
      exit(1);
    }
    break;
  default:
    sollyaFprintf(stderr,"Error: fprintGappaAssignmentAsArith: unknown operation type (%d) in the assignment\n",assign->opType);
    exit(1);
  }
}

void fprintGappaAssignmentAsHint(FILE *fd, gappaAssignment *assign) {
  switch (assign->opType) {
  case GAPPA_CONST:   
    break;
  case GAPPA_ADD_EXACT: 
    sollyaFprintf(fd,"%sh ~ %shm;\n",assign->resultVariable,assign->resultVariable);
    break;
  case GAPPA_MUL_EXACT: 
    sollyaFprintf(fd,"%sh ~ %shm;\n",assign->resultVariable,assign->resultVariable);
    break;
  case GAPPA_ADD_DOUBLE: 
    break;
  case GAPPA_MUL_DOUBLE: 
    break;
  case GAPPA_RENORMALIZE: 
    sollyaFprintf(fd,"%shm ~ %shml;\n",assign->resultVariable,assign->resultVariable);
    sollyaFprintf(fd,"%sh ~ %shm;\n",assign->resultVariable,assign->resultVariable);
    sollyaFprintf(fd,"%sh ~ %shml;\n",assign->resultVariable,assign->resultVariable);
    sollyaFprintf(fd,"%sm -> %sh * overlap_%s;\n",assign->resultVariable,assign->resultVariable,assign->resultVariable);
    sollyaFprintf(fd,"%sl / %sm -> - ((%sm - %sml) / %sml) / (1 + ((%sm - %sml) / %sml));\n",
	    assign->resultVariable,assign->resultVariable,assign->resultVariable,assign->resultVariable,
	    assign->resultVariable,assign->resultVariable,assign->resultVariable,assign->resultVariable);
    sollyaFprintf(fd,"(%shm - %shml) / %shml -> - (%sl / %sm) * (1 / (1 / overlap_%s + 1 + (%sl / %sm)));\n",
	    assign->resultVariable,assign->resultVariable,assign->resultVariable,assign->resultVariable,
	    assign->resultVariable,assign->resultVariable,assign->resultVariable,assign->resultVariable);
    sollyaFprintf(fd,"%sml -> %shml / ((1 + ((%sm - %sml) / %sml)) / overlap_%s + 1);\n",
	    assign->resultVariable,assign->resultVariable,assign->resultVariable,assign->resultVariable,
	    assign->resultVariable,assign->resultVariable);
    sollyaFprintf(fd,"(%sh - %shm) / %shm -> - 1 / (1 / overlap_%s + 1);\n",
	    assign->resultVariable,assign->resultVariable,assign->resultVariable,assign->resultVariable);
    sollyaFprintf(fd,"%sh -> %shml / (overlap_%s / (1 + ((%sm - %sml) / %sml)) + 1);\n",
	    assign->resultVariable,assign->resultVariable,assign->resultVariable,assign->resultVariable,
	    assign->resultVariable,assign->resultVariable);
    break;
  case GAPPA_ADD_REL: 
    switch (assign->resultType) {
    case 3:
      sollyaFprintf(fd,"%shm ~ %shml;\n",assign->resultVariable,assign->resultVariable);
      sollyaFprintf(fd,"%sh ~ %shm;\n",assign->resultVariable,assign->resultVariable);
      sollyaFprintf(fd,"%sh ~ %shml;\n",assign->resultVariable,assign->resultVariable);
      sollyaFprintf(fd,"%sm -> %sh * overlap_%s;\n",assign->resultVariable,assign->resultVariable,assign->resultVariable);
      sollyaFprintf(fd,"%sl / %sm -> - ((%sm - %sml) / %sml) / (1 + ((%sm - %sml) / %sml));\n",
	      assign->resultVariable,assign->resultVariable,assign->resultVariable,assign->resultVariable,
	      assign->resultVariable,assign->resultVariable,assign->resultVariable,assign->resultVariable);
      sollyaFprintf(fd,"(%shm - %shml) / %shml -> - (%sl / %sm) * (1 / (1 / overlap_%s + 1 + (%sl / %sm)));\n",
	      assign->resultVariable,assign->resultVariable,assign->resultVariable,assign->resultVariable,
	      assign->resultVariable,assign->resultVariable,assign->resultVariable,assign->resultVariable);
      sollyaFprintf(fd,"%sml -> %shml / ((1 + ((%sm - %sml) / %sml)) / overlap_%s + 1);\n",
	      assign->resultVariable,assign->resultVariable,assign->resultVariable,assign->resultVariable,
	      assign->resultVariable,assign->resultVariable);
      sollyaFprintf(fd,"(%sh - %shm) / %shm -> - 1 / (1 / overlap_%s + 1);\n",
	      assign->resultVariable,assign->resultVariable,assign->resultVariable,assign->resultVariable);
      sollyaFprintf(fd,"%sh -> %shml / (overlap_%s / (1 + ((%sm - %sml) / %sml)) + 1);\n",
	      assign->resultVariable,assign->resultVariable,assign->resultVariable,assign->resultVariable,
	      assign->resultVariable,assign->resultVariable);
      break;
    case 2:
      sollyaFprintf(fd,"%sh ~ %shm;\n",assign->resultVariable,assign->resultVariable);
      break;
    default:
      sollyaFprintf(stderr,"Error: fprintGappaAssignmentAsHint: unhandlable result type (%d) in the assignment\n",assign->resultType);
      exit(1);
    }
    break;
  case GAPPA_MUL_REL: 
    switch (assign->resultType) {
    case 3:
      sollyaFprintf(fd,"%shm ~ %shml;\n",assign->resultVariable,assign->resultVariable);
      sollyaFprintf(fd,"%sh ~ %shm;\n",assign->resultVariable,assign->resultVariable);
      sollyaFprintf(fd,"%sh ~ %shml;\n",assign->resultVariable,assign->resultVariable);
      sollyaFprintf(fd,"%sm -> %sh * overlap_%s;\n",assign->resultVariable,assign->resultVariable,assign->resultVariable);
      sollyaFprintf(fd,"%sl / %sm -> - ((%sm - %sml) / %sml) / (1 + ((%sm - %sml) / %sml));\n",
	      assign->resultVariable,assign->resultVariable,assign->resultVariable,assign->resultVariable,
	      assign->resultVariable,assign->resultVariable,assign->resultVariable,assign->resultVariable);
      sollyaFprintf(fd,"(%shm - %shml) / %shml -> - (%sl / %sm) * (1 / (1 / overlap_%s + 1 + (%sl / %sm)));\n",
	      assign->resultVariable,assign->resultVariable,assign->resultVariable,assign->resultVariable,
	      assign->resultVariable,assign->resultVariable,assign->resultVariable,assign->resultVariable);
      sollyaFprintf(fd,"%sml -> %shml / ((1 + ((%sm - %sml) / %sml)) / overlap_%s + 1);\n",
	      assign->resultVariable,assign->resultVariable,assign->resultVariable,assign->resultVariable,
	      assign->resultVariable,assign->resultVariable);
      sollyaFprintf(fd,"(%sh - %shm) / %shm -> - 1 / (1 / overlap_%s + 1);\n",
	      assign->resultVariable,assign->resultVariable,assign->resultVariable,assign->resultVariable);
      sollyaFprintf(fd,"%sh -> %shml / (overlap_%s / (1 + ((%sm - %sml) / %sml)) + 1);\n",
	      assign->resultVariable,assign->resultVariable,assign->resultVariable,assign->resultVariable,
	      assign->resultVariable,assign->resultVariable);
      break;
    case 2:
      sollyaFprintf(fd,"%sh ~ %shm;\n",assign->resultVariable,assign->resultVariable);
      break;
    default:
      sollyaFprintf(stderr,"Error: fprintGappaAssignmentAsHint: unhandlable result type (%d) in the assignment\n",assign->resultType);
      exit(1);
    }
    break;
  case GAPPA_FMA_REL: 
    switch (assign->resultType) {
    case 3:
      sollyaFprintf(fd,"%shm ~ %shml;\n",assign->resultVariable,assign->resultVariable);
      sollyaFprintf(fd,"%sh ~ %shm;\n",assign->resultVariable,assign->resultVariable);
      sollyaFprintf(fd,"%sh ~ %shml;\n",assign->resultVariable,assign->resultVariable);
      sollyaFprintf(fd,"%sm -> %sh * overlap_%s;\n",assign->resultVariable,assign->resultVariable,assign->resultVariable);
      sollyaFprintf(fd,"%sl / %sm -> - ((%sm - %sml) / %sml) / (1 + ((%sm - %sml) / %sml));\n",
	      assign->resultVariable,assign->resultVariable,assign->resultVariable,assign->resultVariable,
	      assign->resultVariable,assign->resultVariable,assign->resultVariable,assign->resultVariable);
      sollyaFprintf(fd,"(%shm - %shml) / %shml -> - (%sl / %sm) * (1 / (1 / overlap_%s + 1 + (%sl / %sm)));\n",
	      assign->resultVariable,assign->resultVariable,assign->resultVariable,assign->resultVariable,
	      assign->resultVariable,assign->resultVariable,assign->resultVariable,assign->resultVariable);
      sollyaFprintf(fd,"%sml -> %shml / ((1 + ((%sm - %sml) / %sml)) / overlap_%s + 1);\n",
	      assign->resultVariable,assign->resultVariable,assign->resultVariable,assign->resultVariable,
	      assign->resultVariable,assign->resultVariable);
      sollyaFprintf(fd,"(%sh - %shm) / %shm -> - 1 / (1 / overlap_%s + 1);\n",
	      assign->resultVariable,assign->resultVariable,assign->resultVariable,assign->resultVariable);
      sollyaFprintf(fd,"%sh -> %shml / (overlap_%s / (1 + ((%sm - %sml) / %sml)) + 1);\n",
	      assign->resultVariable,assign->resultVariable,assign->resultVariable,assign->resultVariable,
	      assign->resultVariable,assign->resultVariable);
      break;
    case 2:
      sollyaFprintf(fd,"%sh ~ %shm;\n",assign->resultVariable,assign->resultVariable);
      break;
    default:
      sollyaFprintf(stderr,"Error: fprintGappaAssignmentAsHint: unhandlable result type (%d) in the assignment\n",assign->resultType);
      exit(1);
    }
    break;
  case GAPPA_COPY: 
    break;
  default:
    sollyaFprintf(stderr,"Error: fprintGappaAssignmentAsHint: unknown operation type (%d) in the assignment\n",assign->opType);
    exit(1);
  }
}

void fprintGappaAssignmentAsDichotomy(FILE *fd, gappaAssignment *assign) {
  switch (assign->opType) {
  case GAPPA_CONST:   
    break;
  case GAPPA_ADD_EXACT: 
    break;
  case GAPPA_MUL_EXACT: 
    break;
  case GAPPA_ADD_DOUBLE: 
    break;
  case GAPPA_MUL_DOUBLE: 
    break;
  case GAPPA_RENORMALIZE: 
    sollyaFprintf(fd,"$ %shml in (0);\n",assign->resultVariable);
    sollyaFprintf(fd,"$ %sml in (0);\n",assign->resultVariable);
    break;
  case GAPPA_ADD_REL: 
    switch (assign->resultType) {
    case 3:
      sollyaFprintf(fd,"$ %shml in (0);\n",assign->resultVariable);
      sollyaFprintf(fd,"$ %sml in (0);\n",assign->resultVariable);
      break;
    case 2:
      break;
    default:
      sollyaFprintf(stderr,"Error: fprintGappaAssignmentAsDichotomy: unhandlable result type (%d) in the assignment\n",assign->resultType);
      exit(1);
    }
    break;
  case GAPPA_MUL_REL: 
    switch (assign->resultType) {
    case 3:
      sollyaFprintf(fd,"$ %shml in (0);\n",assign->resultVariable);
      sollyaFprintf(fd,"$ %sml in (0);\n",assign->resultVariable);
      break;
    case 2:
      break;
    default:
      sollyaFprintf(stderr,"Error: fprintGappaAssignmentAsDichotomy: unhandlable result type (%d) in the assignment\n",assign->resultType);
      exit(1);
    }
    break;
  case GAPPA_FMA_REL: 
    switch (assign->resultType) {
    case 3:
      sollyaFprintf(fd,"$ %shml in (0);\n",assign->resultVariable);
      sollyaFprintf(fd,"$ %sml in (0);\n",assign->resultVariable);
      break;
    case 2:
      break;
    default:
      sollyaFprintf(stderr,"Error: fprintGappaAssignmentAsDichotomy: unhandlable result type (%d) in the assignment\n",assign->resultType);
      exit(1);
    }
    break;
  case GAPPA_COPY: 
    break;
  default:
    sollyaFprintf(stderr,"Error: fprintGappaAssignmentAsDichtomy: unknown operation type (%d) in the assignment\n",assign->opType);
    exit(1);
  }
}



void fprintGappaAssignmentAsMetaHint(FILE *fd, gappaAssignment *assign) {
  switch (assign->opType) {
  case GAPPA_CONST:   

    break;
  case GAPPA_ADD_EXACT: 
    sollyaFprintf(fd,"%shm ~ M%s;\n",assign->resultVariable,assign->resultVariable);
    break;
  case GAPPA_MUL_EXACT: 
    sollyaFprintf(fd,"%shm ~ M%s;\n",assign->resultVariable,assign->resultVariable);
    break;
  case GAPPA_ADD_DOUBLE: 
    sollyaFprintf(fd,"%sh ~ M%s;\n",assign->resultVariable,assign->resultVariable);
    break;
  case GAPPA_MUL_DOUBLE: 
    sollyaFprintf(fd,"%sh ~ M%s;\n",assign->resultVariable,assign->resultVariable);
    break;
  case GAPPA_RENORMALIZE: 
    sollyaFprintf(fd,"%shml ~ M%s;\n",assign->resultVariable,assign->resultVariable);
    break;
  case GAPPA_ADD_REL: 
    sollyaFprintf(fd,"%s",assign->resultVariable);
    fprintExpansionSuffix(fd,assign->resultType);
    sollyaFprintf(fd," ~ M%s;\n",assign->resultVariable);
    break;
  case GAPPA_MUL_REL: 
    sollyaFprintf(fd,"%s",assign->resultVariable);
    fprintExpansionSuffix(fd,assign->resultType);
    sollyaFprintf(fd," ~ M%s;\n",assign->resultVariable);
    break;
  case GAPPA_FMA_REL: 
    sollyaFprintf(fd,"%s",assign->resultVariable);
    fprintExpansionSuffix(fd,assign->resultType);
    sollyaFprintf(fd," ~ M%s;\n",assign->resultVariable);
    break;
  case GAPPA_COPY: 
    sollyaFprintf(fd,"%s",assign->resultVariable);
    fprintExpansionSuffix(fd,assign->resultType);
    sollyaFprintf(fd," ~ M%s;\n",assign->resultVariable);
    break;
  default:
    sollyaFprintf(stderr,"Error: fprintGappaAssignmentAsMetaHint: unknown operation type (%d) in the assignment\n",assign->opType);
    exit(1);
  }
}



void fprintGappaAssignmentAsOverlapBound(FILE *fd, gappaAssignment *assign) {
  switch (assign->opType) {
  case GAPPA_CONST:   
    break;
  case GAPPA_ADD_EXACT: 
    break;
  case GAPPA_MUL_EXACT: 
    break;
  case GAPPA_ADD_DOUBLE: 
    break;
  case GAPPA_MUL_DOUBLE: 
    break;
  case GAPPA_RENORMALIZE: 
    sollyaFprintf(fd,"/\\ |overlap_%s| in [1b-400,1b-%d]    # Verify the lower bound\n",
	    assign->resultVariable,assign->resultOverlap);
    sollyaFprintf(fd,"/\\ |%sml| in [1b-1021,1b1023]\n",
	    assign->resultVariable);
    break;
  case GAPPA_ADD_REL: 
    if (assign->resultType == 3) {
      sollyaFprintf(fd,"/\\ |overlap_%s| in [1b-400,1b-%d]    # Verify the lower bound\n",
	      assign->resultVariable,assign->resultOverlap);
      sollyaFprintf(fd,"/\\ |%sml| in [1b-1021,1b1023]\n",
	      assign->resultVariable);
    }
    break;
  case GAPPA_MUL_REL: 
    if (assign->resultType == 3) {
      sollyaFprintf(fd,"/\\ |overlap_%s| in [1b-400,1b-%d]    # Verify the lower bound\n",
	      assign->resultVariable,assign->resultOverlap);
      sollyaFprintf(fd,"/\\ |%sml| in [1b-1021,1b1023]\n",
	      assign->resultVariable);
    }
    break;
  case GAPPA_FMA_REL: 
    if (assign->resultType == 3) {
      sollyaFprintf(fd,"/\\ |overlap_%s| in [1b-400,1b-%d]    # Verify the lower bound\n",
	      assign->resultVariable,assign->resultOverlap);
      sollyaFprintf(fd,"/\\ |%sml| in [1b-1021,1b1023]\n",
	      assign->resultVariable);
    }
    break;
  case GAPPA_COPY: 
    break;
  default:
    sollyaFprintf(stderr,"Error: fprintGappaAssignmentAsOverlapBound: unknown operation type (%d) in the assignment\n",assign->opType);
    exit(1);
  }
}




void fprintGappaProof(FILE *fd, gappaProof *proof) {
  int i;
  mpfr_t temp;
  mp_prec_t prec, p;

  prec = mpfr_get_prec(proof->a);
  p = mpfr_get_prec(proof->b);
  if (p > prec) prec = p;

  mpfr_init2(temp,prec);

  sollyaFprintf(fd,"# The polynomial to implement is: ");
  fprintTree(fd, proof->polynomToImplement);
  sollyaFprintf(fd,"\n# The polynomial implemented is: ");
  fprintTree(fd, proof->polynomImplemented);
  sollyaFprintf(fd,"\n# The domain is [");
  fprintValue(fd, proof->a);
  sollyaFprintf(fd,";");
  fprintValue(fd, proof->b);
  sollyaFprintf(fd,"]\n# The free variable %s is a ",proof->variableName);
  switch (proof->variableType) {
  case 3:
    sollyaFprintf(fd,"triple-double");
    break;
  case 2:
    sollyaFprintf(fd,"double-double");
    break;
  case 1:
    sollyaFprintf(fd,"double precision");
    break;
  default:
    sollyaFprintf(fd,"unknown precision");
  }
  sollyaFprintf(fd," number, the result %s* is stored on a ",proof->resultName);
  switch (proof->resultType) {
  case 3:
    sollyaFprintf(fd,"triple-double");
    break;
  case 2:
    sollyaFprintf(fd,"double-double");
    break;
  case 1:
    sollyaFprintf(fd,"double precision");
    break;
  default:
    sollyaFprintf(fd,"unknown precision");
  }
  sollyaFprintf(fd," number.\n");
  sollyaFprintf(fd,"# The code produces %d intermediate and final arithmetical approximations.\n\n",proof->assignmentsNumber);

  sollyaFprintf(fd,"# Double precision rounding operator:\n@double = float<ieee_64,ne>;\n\n");
  sollyaFprintf(fd,"# Disable some annoying warnings:\n#@-Wno-dichotomy-failure\n\n");

  sollyaFprintf(fd,"# Helper definitions for decomposing the free variable\n");
  switch (proof->variableType) {
  case 3:
    sollyaFprintf(fd,"%sml = %shml - %sh;\n",proof->variableName,proof->variableName,proof->variableName);
    sollyaFprintf(fd,"%sm = double(%sml);\n",proof->variableName,proof->variableName);
    sollyaFprintf(fd,"%sl = %sml - %sm;\n",proof->variableName,proof->variableName,proof->variableName);
    sollyaFprintf(fd,"%shm = %sh + %sm;\n",proof->variableName,proof->variableName,proof->variableName);
    sollyaFprintf(fd,"overlap_%s = %sm / %sh;\n",proof->variableName,proof->variableName,proof->variableName);
    break;
  case 2:
    sollyaFprintf(fd,"%sh = double(%shm);\n",proof->variableName,proof->variableName);
    sollyaFprintf(fd,"%sm = %shm - %sh;\n",proof->variableName,proof->variableName,proof->variableName);
    break;
  case 1:
    sollyaFprintf(fd,"%sh = %s;\n",proof->variableName,proof->variableName);
    break;
  default:
    sollyaFprintf(stderr,"Error: fprintGappaProof: unknown variable type (%d) in the proof\n",proof->variableType);
    exit(1);
  }
  sollyaFprintf(fd,"\n");
  

  sollyaFprintf(fd,"# Transcription of the C code\n");
  for (i=0;i<proof->assignmentsNumber;i++) {
    fprintGappaAssignmentAsArith(fd, proof->assignments[i]);
  }
  sollyaFprintf(fd,"\n");

  sollyaFprintf(fd,"# Mathematical equivalents\n");
  switch (proof->variableType) {
  case 3:
    sollyaFprintf(fd,"M%s = %shml;\n",proof->variableName,proof->variableName);
    break;
  case 2:
    sollyaFprintf(fd,"M%s = %shm;\n",proof->variableName,proof->variableName);
    break;
  case 1:
    sollyaFprintf(fd,"M%s = %s;\n",proof->variableName,proof->variableName);
    break;
  default:
    sollyaFprintf(stderr,"Error: fprintGappaProof: unknown variable type (%d) in the proof\n",proof->variableType);
    exit(1);
  }
  
  for (i=0;i<proof->assignmentsNumber;i++) {
    fprintGappaAssignmentAsMaths(fd, proof->assignments[i]);
  }
  sollyaFprintf(fd,"\n");


  sollyaFprintf(fd,"# Definition of the relative arithmetical error\n");
  switch (proof->resultType) {
  case 3:
    sollyaFprintf(fd,"epsilon = (%shml - M%s) / M%s;\n",proof->resultName,proof->resultName,proof->resultName);
    break;
  case 2:
    sollyaFprintf(fd,"epsilon = (%shm - M%s) / M%s;\n",proof->resultName,proof->resultName,proof->resultName);
    break;
  case 1:
    sollyaFprintf(fd,"epsilon = (%sh - M%s) / M%s;\n",proof->resultName,proof->resultName,proof->resultName);
    break;
  default:
    sollyaFprintf(stderr,"Error: fprintGappaProof: unknown result type (%d) in the proof\n",proof->resultType);
    exit(1);
  }
  sollyaFprintf(fd,"\n");

  sollyaFprintf(fd,"# Implication to prove\n");

  if ((mpfr_sgn(proof->a) != mpfr_sgn(proof->b)) && (!mpfr_zero_p(proof->a)) && (!mpfr_zero_p(proof->b))) {
    
    sollyaFprintf(fd,"{((\n");
    switch (proof->variableType) {
    case 3:
      sollyaFprintf(fd,"   %shml",proof->variableName);
      break;
    case 2:
      sollyaFprintf(fd,"   %shm",proof->variableName);
      break;
    case 1:
      sollyaFprintf(fd,"   %s",proof->variableName);
      break;
    default:
      sollyaFprintf(stderr,"Error: fprintGappaProof: unknown variable type (%d) in the proof\n",proof->variableType);
      exit(1);
    }
    sollyaFprintf(fd," in [");
    fprintValue(fd, proof->a);
    sollyaFprintf(fd,",");
    mpfr_set(temp,proof->a,GMP_RNDN);
    mpfr_div_2ui(temp,temp,400,GMP_RNDN);
    fprintValue(fd, temp);
    sollyaFprintf(fd,"]\n");
    if (proof->variableType == 3) {
      sollyaFprintf(fd,"/\\ |overlap_%s| in [1b-400, 1b-52]  # Verify the lower bound for the overlap interval\n",proof->variableName);
      sollyaFprintf(fd,"/\\ |%sml| in [1b-1021, 1b1023]\n",proof->variableName);
    }
    for (i=0;i<proof->assignmentsNumber;i++) {
      fprintGappaAssignmentAsOverlapBound(fd, proof->assignments[i]);
    }

    sollyaFprintf(fd,") \\/ (\n");

    switch (proof->variableType) {
    case 3:
      sollyaFprintf(fd,"   %shml",proof->variableName);
      break;
    case 2:
      sollyaFprintf(fd,"   %shm",proof->variableName);
      break;
    case 1:
      sollyaFprintf(fd,"   %s",proof->variableName);
      break;
    default:
      sollyaFprintf(stderr,"Error: fprintGappaProof: unknown variable type (%d) in the proof\n",proof->variableType);
      exit(1);
    }
    sollyaFprintf(fd," in [");
    mpfr_set(temp,proof->b,GMP_RNDN);
    mpfr_div_2ui(temp,temp,400,GMP_RNDN);
    fprintValue(fd, temp);
    sollyaFprintf(fd,",");
    fprintValue(fd, proof->b);
    sollyaFprintf(fd,"]\n");
    if (proof->variableType == 3) {
      sollyaFprintf(fd,"/\\ |overlap_%s| in [1b-400, 1b-52]  # Verify the lower bound for the overlap interval\n",proof->variableName);
      sollyaFprintf(fd,"/\\ |%sml| in [1b-1021, 1b1023]\n",proof->variableName);
    }
    for (i=0;i<proof->assignmentsNumber;i++) {
      fprintGappaAssignmentAsOverlapBound(fd, proof->assignments[i]);
    }
    
    sollyaFprintf(fd,"))\n->\n(\n   epsilon in ?\n)}\n");
  } else {
    sollyaFprintf(fd,"{(\n");
    switch (proof->variableType) {
    case 3:
      sollyaFprintf(fd,"   %shml",proof->variableName);
      break;
    case 2:
      sollyaFprintf(fd,"   %shm",proof->variableName);
      break;
    case 1:
      sollyaFprintf(fd,"   %s",proof->variableName);
      break;
    default:
      sollyaFprintf(stderr,"Error: fprintGappaProof: unknown variable type (%d) in the proof\n",proof->variableType);
      exit(1);
    }
    sollyaFprintf(fd," in [");
    fprintValue(fd, proof->a);
    sollyaFprintf(fd,",");
    fprintValue(fd, proof->b);
    sollyaFprintf(fd,"]\n");
    if (proof->variableType == 3) {
      sollyaFprintf(fd,"/\\ |overlap_%s| in [1b-400, 1b-52]  # Verify the lower bound for the overlap interval\n",proof->variableName);
      sollyaFprintf(fd,"/\\ |%sml| in [1b-1021, 1b1023]\n",proof->variableName);
    }
    for (i=0;i<proof->assignmentsNumber;i++) {
      fprintGappaAssignmentAsOverlapBound(fd, proof->assignments[i]);
    }
    
    sollyaFprintf(fd,")\n->\n(\n   epsilon in ?\n)}\n");
  }
  sollyaFprintf(fd,"\n");
  
  sollyaFprintf(fd,"# Hints and Meta-Hints for expansion decomposition\n");


  switch (proof->variableType) {
  case 3:
    sollyaFprintf(fd,"%sh ~ %shm;\n",proof->variableName,proof->variableName);
    sollyaFprintf(fd,"%shm ~ %shml;\n",proof->variableName,proof->variableName);
    sollyaFprintf(fd,"%sm -> %sh * overlap_%s;\n",proof->variableName,proof->variableName,proof->variableName);
    sollyaFprintf(fd,"%sl / %sm -> - ((%sm - %sml) / %sml) / (1 + ((%sm - %sml) / %sml));\n",
	    proof->variableName,proof->variableName,proof->variableName,proof->variableName,
	    proof->variableName,proof->variableName,proof->variableName,proof->variableName);
    sollyaFprintf(fd,"(%shm - %shml) / %shml -> - (%sl / %sm) * (1 / (1 / overlap_%s + 1 + (%sl / %sm)));\n",
	    proof->variableName,proof->variableName,proof->variableName,proof->variableName,
	    proof->variableName,proof->variableName,proof->variableName,proof->variableName);
    sollyaFprintf(fd,"%sml -> %shml / ((1 + ((%sm - %sml) / %sml)) / overlap_%s + 1);\n",
	    proof->variableName,proof->variableName,proof->variableName,proof->variableName,
	    proof->variableName,proof->variableName);
    sollyaFprintf(fd,"(%sh - %shm) / %shm -> - 1 / (1 / overlap_%s + 1);\n",
	    proof->variableName,proof->variableName,proof->variableName,proof->variableName);
    sollyaFprintf(fd,"%sh -> %shml / (overlap_%s / (1 + ((%sm - %sml) / %sml)) + 1);\n",
	    proof->variableName,proof->variableName,proof->variableName,proof->variableName,
	    proof->variableName,proof->variableName);
    break;
  case 2:
    sollyaFprintf(fd,"%sh ~ %shm;\n",proof->variableName,proof->variableName);
    break;
  case 1:
    break;
  default:
    sollyaFprintf(stderr,"Error: fprintGappaProof: unknown variable type (%d) in the proof\n",proof->variableType);
    exit(1);
  }
  sollyaFprintf(fd,"\n");

  for (i=0;i<proof->assignmentsNumber;i++) {
    fprintGappaAssignmentAsHint(fd, proof->assignments[i]);
  }  
  sollyaFprintf(fd,"\n");

  sollyaFprintf(fd,"# Meta-Hints for Horner scheme\n");

  for (i=0;i<proof->assignmentsNumber;i++) {
    fprintGappaAssignmentAsMetaHint(fd, proof->assignments[i]);
  }

  sollyaFprintf(fd,"\n");

  sollyaFprintf(fd,"# Dichotomies for triple-double decomposition\n");

  if (proof->variableType == 3) {
    sollyaFprintf(fd,"$ %shml in (0);\n",proof->variableName);
    sollyaFprintf(fd,"$ %sml in (0);\n",proof->variableName);
    sollyaFprintf(fd,"\n");
  }

  for (i=0;i<proof->assignmentsNumber;i++) {
    fprintGappaAssignmentAsDichotomy(fd, proof->assignments[i]);
  }
  sollyaFprintf(fd,"\n");

  sollyaFprintf(fd,"# Dichotomy for the error bound\n");
  sollyaFprintf(fd,"epsilon $ %s",proof->variableName);
  fprintExpansionSuffix(fd,proof->resultType);
  sollyaFprintf(fd,";\n\n");

  mpfr_clear(temp);

}
