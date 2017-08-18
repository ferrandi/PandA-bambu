/*

Copyright 2006-2009 by 

Laboratoire de l'Informatique du Parallelisme, 
UMR CNRS - ENS Lyon - UCB Lyon 1 - INRIA 5668

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

#ifndef PROOF_H
#define PROOF_H


#include <mpfr.h>
#include "mpfi-compat.h"
#include <stdio.h>
#include "expression.h"
#include "chain.h"


#define TAYLORPROOF 1
#define HOPITAL_ON_POINT 2
#define NUMERATOR_IS_ZERO 3
#define HOPITAL 4
#define IMPLICATION 5
#define DECORRELATE 6
#define MONOTONOCITY 7

typedef struct exprBoundTheoStruct exprBoundTheo;

struct exprBoundTheoStruct
{
  node *function;
  int functionType;
  sollya_mpfi_t *x;
  sollya_mpfi_t *boundLeft;
  sollya_mpfi_t *boundRight;
  sollya_mpfi_t *y;
  exprBoundTheo *theoLeft;
  exprBoundTheo *theoRight;
  int simplificationUsed;
  node *leftDerivative;
  node *rightDerivative;
  sollya_mpfi_t *xZ;
  sollya_mpfi_t *xMXZ;
  exprBoundTheo *theoLeftConstant;
  exprBoundTheo *theoRightConstant;
  sollya_mpfi_t *boundLeftConstant;
  sollya_mpfi_t *boundRightConstant;
  exprBoundTheo *theoLeftLinear;
  exprBoundTheo *theoRightLinear;
  sollya_mpfi_t *boundLeftLinear;
  sollya_mpfi_t *boundRightLinear;
  int number;
};

typedef struct equalityTheoStruct equalityTheo;

struct equalityTheoStruct
{
  node *expr1;
  node *expr2;
  int number;
};

typedef struct noZeroTheoStruct noZeroTheo;

struct noZeroTheoStruct
{
  node *function;
  node *derivative;
  equalityTheo *funcEqual;
  equalityTheo *derivEqual;
  chain *exprBoundTheos;
  int number;
};

typedef struct infnormTheoStruct infnormTheo;

struct infnormTheoStruct
{
  node *function;
  sollya_mpfi_t *domain;
  sollya_mpfi_t *infnorm;
  node *derivative;
  node *numeratorOfDerivative;
  node *derivativeOfNumeratorOfDerivative;
  chain *excludedIntervals;
  noZeroTheo *noZeros;
  exprBoundTheo *evalLeftBound;
  exprBoundTheo *evalRightBound;
  chain *evalOnZeros;
  int number;
};


#define GAPPA_CONST 1
#define GAPPA_ADD_EXACT 2
#define GAPPA_MUL_EXACT 3
#define GAPPA_ADD_DOUBLE 4
#define GAPPA_MUL_DOUBLE 5
#define GAPPA_RENORMALIZE 6
#define GAPPA_ADD_REL 7
#define GAPPA_MUL_REL 8
#define GAPPA_FMA_REL 9
#define GAPPA_COPY 10

typedef struct gappaAssignmentStruct gappaAssignment;

struct gappaAssignmentStruct
{
  int opType;
  int relErrBits;
  int resultType;
  int resultOverlap;
  char *resultVariable;
  int operand1UsedType;
  int operand1ComingType;
  char *operand1Variable;
  int operand2UsedType;
  int operand2ComingType;
  char *operand2Variable;
  int operand3UsedType;
  int operand3ComingType;
  char *operand3Variable;
  double constHi;
  double constMi;
  double constLo;
};

typedef struct gappaProofStruct gappaProof;

struct gappaProofStruct
{
  char *variableName;
  char *resultName;
  mpfr_t a, b;
  int variableType;
  int resultType;
  node *polynomToImplement;
  node *polynomImplemented;
  int assignmentsNumber;
  gappaAssignment **assignments;
};




int fprintExprBoundTheo(FILE *fd, exprBoundTheo *theo, int start);
void freeExprBoundTheo(exprBoundTheo *theo);
void nullifyExprBoundTheo(exprBoundTheo *theo);
int fprintNoZeroTheo(FILE *fd, noZeroTheo *theo, int start);
void freeNoZeroTheo(noZeroTheo *theo);
int fprintInfnormTheo(FILE *fd, infnormTheo *theo, int start);
void freeInfnormTheo(infnormTheo *theo);

gappaAssignment *newGappaOperation(int opType, int relErrBits, 
				   int resultType, int resultOverlap, char *resultVariable,
				   int operand1UsedType, int operand1ComingType, char *operand1Variable,
				   int operand2UsedType, int operand2ComingType, char *operand2Variable);
gappaAssignment *newGappaConstant(int resultType, char *resultVariable, double constHi, double constMi, double constLo);

void freeGappaProof(gappaProof *proof);
void fprintGappaProof(FILE *fd, gappaProof *proof);



#endif /* ifdef PROOF_H*/
