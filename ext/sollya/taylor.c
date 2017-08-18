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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "taylor.h"
#include "expression.h"
#include "general.h"




node *taylor(node* tree, int degree, node* point, mp_prec_t prec) {
  node *copy, *temp, *temp2, *fderiv, *fderivsubst, *denominator, *numerator, *expon, *variable, *term, *pointTemp;
  mpfr_t *value;
  mpz_t denominatorGMP;
  int i;

  if (!isConstant(point)) {
    printMessage(1,"Warning: the expression given for the development point is not constant.\n");
    printMessage(1,"Will evaluate the expression in %s = 0 before using it as development point.\n",variablename);
    temp = (node *) safeMalloc(sizeof(node));
    temp->nodeType = CONSTANT;
    value = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
    mpfr_init2(*value,prec);
    mpfr_set_d(*value,0.0,GMP_RNDN);
    temp->value = value;
    temp2 = substitute(point,temp);
    pointTemp = simplifyTreeErrorfree(temp2);
    free_memory(temp);
    free_memory(temp2);
  } else {
    pointTemp = copyTree(point);
  }

  value = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
  mpfr_init2(*value,prec);
  mpfr_set_d(*value,0.0,GMP_RNDN);
  copy = (node *) safeMalloc(sizeof(node));
  copy->nodeType = CONSTANT;
  copy->value = value;

  mpz_init(denominatorGMP);
  fderiv = copyTree(tree);
  for (i=0;i<=degree;i++) {
    temp = substitute(fderiv,pointTemp);
    fderivsubst = simplifyTreeErrorfree(temp);
    free_memory(temp);
    mpz_fac_ui(denominatorGMP,(unsigned int) i);
    value = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
    mpfr_init2(*value,prec);
    if(mpfr_set_z(*value,denominatorGMP,GMP_RNDN) != 0) {
      if (!noRoundingWarnings) {
	printMessage(1,"Warning: rounding occurred on computing a taylor constant factor.\n");
	printMessage(1,"Try to increase the working precision.\n");
      }
    }
    denominator = (node *) safeMalloc(sizeof(node));
    denominator->nodeType = CONSTANT;
    denominator->value = value;
    value = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
    mpfr_init2(*value,prec);
    mpfr_set_d(*value,1.0,GMP_RNDN);
    numerator = (node *) safeMalloc(sizeof(node));
    numerator->nodeType = CONSTANT;
    numerator->value = value;
    temp = (node *) safeMalloc(sizeof(node));
    temp->nodeType = DIV;
    temp->child1 = numerator;
    temp->child2 = denominator;
    temp2 = (node *) safeMalloc(sizeof(node));
    temp2->nodeType = MUL;
    temp2->child1 = temp;
    temp2->child2 = fderivsubst;
    variable = (node *) safeMalloc(sizeof(node));
    variable->nodeType = VARIABLE;
    value = (mpfr_t *) safeMalloc(sizeof(mpfr_t));
    mpfr_init2(*value,prec);
    if(mpfr_set_si(*value,i,GMP_RNDN) != 0) {
      if (!noRoundingWarnings) {
	printMessage(1,"Warning: rounding occurred on computing a taylor exponent.\n");
	printMessage(1,"Try to increase the working precision.\n");
      }
    }
    expon = (node *) safeMalloc(sizeof(node));
    expon->nodeType = CONSTANT;
    expon->value = value;
    temp = (node *) safeMalloc(sizeof(node));
    temp->nodeType = POW;
    temp->child1 = variable;
    temp->child2 = expon;
    term = (node *) safeMalloc(sizeof(node));
    term->nodeType = MUL;
    term->child1 = temp2;
    term->child2 = temp;
    temp = (node *) safeMalloc(sizeof(node));
    temp->nodeType = ADD;
    temp->child1 = copy;
    temp->child2 = term;
    copy = temp;
    temp = differentiate(fderiv);
    free_memory(fderiv);
    fderiv = temp;
  }
  mpz_clear(denominatorGMP);
  
  free_memory(fderiv);

  temp = horner(copy);
  free_memory(copy);
  free_memory(pointTemp);
  return temp;
}
