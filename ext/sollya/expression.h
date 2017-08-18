/*

Copyright 2006-2011 by

Laboratoire de l'Informatique du Parallelisme,
UMR CNRS - ENS Lyon - UCB Lyon 1 - INRIA 5668,

Laboratoire d'Informatique de Paris 6, equipe PEQUAN,
UPMC Universite Paris 06 - CNRS - UMR 7606 - LIP6, Paris, France

and by

LORIA (CNRS, INPL, INRIA, UHP, U-Nancy 2).

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

#ifndef EXPRESSION_H
#define EXPRESSION_H


#include <mpfr.h>
#include <stdio.h>
#include "chain.h"
#include "library.h"


#define VARIABLE 0
#define CONSTANT 1
#define ADD 2
#define SUB 3
#define MUL 4
#define DIV 5
#define SQRT 6
#define EXP 7
#define LOG 8
#define LOG_2 9
#define LOG_10 10
#define SIN 11
#define COS 12
#define TAN 13
#define ASIN 14
#define ACOS 15
#define ATAN 16
#define SINH 17
#define COSH 18
#define TANH 19
#define ASINH 20
#define ACOSH 21
#define ATANH 22
#define POW 23
#define NEG 24
#define ABS 25
#define DOUBLE 26
#define DOUBLEDOUBLE 27
#define TRIPLEDOUBLE 28
#define POLYNOMIAL 29
#define ERF 30
#define ERFC 31
#define LOG_1P 32
#define EXP_M1 33
#define DOUBLEEXTENDED 34
#define LIBRARYFUNCTION 35
#define CEIL 36
#define FLOOR 37
#define PI_CONST 38
#define SINGLE 39
#define NEARESTINT 40
#define LIBRARYCONSTANT 41
#define PROCEDUREFUNCTION 42
#define HALFPRECISION 43
#define QUAD 44


typedef struct nodeStruct node;

struct nodeStruct 
{
  int nodeType;
  mpfr_t *value;
  node *child1;
  node *child2;
  libraryFunction *libFun;
  int libFunDeriv;
  char *string;
  chain *arguments;
  libraryProcedure *libProc;
};

/* HELPER TYPE FOR THE PARSER */
typedef struct doubleNodeStruct doubleNode;
struct doubleNodeStruct 
{
  node *a;
  node *b;
};

/* END HELPER TYPE */


typedef struct rangetypeStruct rangetype;

struct rangetypeStruct
{
  mpfr_t *a;
  mpfr_t *b;
};


void printTree(node *tree);
node* differentiate(node *tree);
node* simplifyTree(node *tree); 
void free_memory(node *tree);
int evaluateConstantExpression(mpfr_t result, node *tree, mp_prec_t prec);
void evaluate(mpfr_t result, node *tree, mpfr_t x, mp_prec_t prec);
void printValue(mpfr_t *value);
node* copyTree(node *tree);
node* horner(node *tree);
int getDegree(node *tree);
node* expand(node *tree);
node* simplifyTreeErrorfree(node *tree);
int getNumeratorDenominator(node **numerator, node **denominator, node *tree);
node *substitute(node* tree, node *t);
int readDyadic(mpfr_t res, char *c);
int readHexadecimal(mpfr_t res, char *c);
int isPolynomial(node *tree);
int isAffine(node *tree);
int arity(node *tree);
void fprintValue(FILE *fd, mpfr_t value);
void fprintTree(FILE *fd, node *tree);
int isSyntacticallyEqual(node *tree1, node *tree2);
void fprintHeadFunction(FILE *fd,node *tree, char *x, char *y);
int isConstant(node *tree);
void getCoefficients(int *degree, node ***coefficients, node *poly);
node *makePolynomial(mpfr_t *coefficients, int degree);
node *makePolynomialConstantExpressions(node **coeffs, int deg);
int treeSize(node *tree);
void printMpfr(mpfr_t x);
int highestDegreeOfPolynomialSubexpression(node *tree);
node *getIthCoefficient(node *poly, int i);
node *getSubpolynomial(node *poly, chain *monomials, int fillDegrees, mp_prec_t prec);
node *makeCanonical(node *func, mp_prec_t prec);
char *sprintTree(node *tree);
char *sprintValue(mpfr_t *value);
void printBinary(mpfr_t x);
int isHorner(node *tree);
int isCanonical(node *tree);
char *sprintMidpointMode(mpfr_t a, mpfr_t b);
void fprintValueForXml(FILE *, mpfr_t );
void fprintValueWithPrintMode(FILE *, mpfr_t );
void fprintTreeWithPrintMode(FILE *, node *);
int readDecimalConstant(mpfr_t ,char *);
int getMaxPowerDivider(node *tree);
void simplifyMpfrPrec(mpfr_t rop, mpfr_t op);
node *simplifyRationalErrorfree(node *tree);
int tryEvaluateConstantTermToMpq(mpq_t res, node *tree);
node *simplifyAllButDivision(node *tree);
int mpfr_to_mpq( mpq_t y, mpfr_t x);
mp_prec_t getMpzPrecision(mpz_t x);

node *makeVariable();
node *makeConstant(mpfr_t x);
node *makeConstantDouble(double x);
node *makeAdd(node *op1, node *op2);
node *makeSub(node *op1, node *op2);
node *makeMul(node *op1, node *op2);
node *makeDiv(node *op1, node *op2);
node *makeSqrt(node *op1);
node *makeExp(node *op1);
node *makeLog(node *op1);
node *makeLog2(node *op1);
node *makeLog10(node *op1);
node *makeSin(node *op1);
node *makeCos(node *op1);
node *makeTan(node *op1);
node *makeAsin(node *op1);
node *makeAcos(node *op1);
node *makeAtan(node *op1);
node *makePow(node *op1, node *op2);
node *makeNeg(node *op1);
node *makeAbs(node *op1);
node *makeDouble(node *op1);
node *makeSingle(node *op1);
node *makeQuad(node *op1);
node *makeHalfPrecision(node *op1);
node *makeDoubledouble(node *op1);
node *makeTripledouble(node *op1);
node *makeErf(node *op1);
node *makeErfc(node *op1);
node *makeLog1p(node *op1);
node *makeExpm1(node *op1);
node *makeDoubleextended(node *op1);
node *makeCeil(node *op1);
node *makeFloor(node *op1);
node *makeNearestInt(node *op1);
node *makePi();
node *makeSinh(node *op1);
node *makeCosh(node *op1);
node *makeTanh(node *op1);
node *makeAsinh(node *op1);
node *makeAcosh(node *op1);
node *makeAtanh(node *op1);
node *makeUnary(node *op1, int nodeType);
int mpfr_nearestint(mpfr_t rop, mpfr_t op);



#endif /* ifdef EXPRESSION_H*/
