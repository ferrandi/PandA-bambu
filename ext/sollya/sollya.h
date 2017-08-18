/*

Copyright 2007-2011 by 

Laboratoire de l'Informatique du Parallelisme, 
UMR CNRS - ENS Lyon - UCB Lyon 1 - INRIA 5668,

LORIA (CNRS, INPL, INRIA, UHP, U-Nancy 2)

and by

Laboratoire d'Informatique de Paris 6, equipe PEQUAN,
UPMC Universite Paris 06 - CNRS - UMR 7606 - LIP6, Paris, France.

Contributors Ch. Lauter, S. Chevillard, M. Joldes

christoph.lauter@ens-lyon.org
sylvain.chevillard@ens-lyon.org
mioara.joldes@ens-lyon.fr

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

#ifndef SOLLYA_H
#define SOLLYA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <mpfr.h>
#include <mpfi-compat.h>
#include <setjmp.h>

typedef struct chainStruct chain;
struct chainStruct 
{
  void *value;
  chain *next;
};


typedef struct libraryHandleStruct libraryHandle;
struct libraryHandleStruct 
{
  char *libraryName;
  void *libraryDescriptor;
  chain *functionList;
};

typedef struct libraryFunctionStruct libraryFunction;
struct libraryFunctionStruct 
{
  char *functionName;
  int (*code)(sollya_mpfi_t, sollya_mpfi_t, int); /* used for LIBRARYFUNCTION */
  void (*constant_code)(mpfr_t, mp_prec_t); /* used for LIBRARYCONSTANT */
};

typedef struct libraryProcedureStruct libraryProcedure;
struct libraryProcedureStruct 
{
  char *procedureName;
  void *code;
  chain *signature;
};


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

#define FIXED 236
#define FLOATING 237
#define ABSOLUTESYM 197
#define RELATIVESYM 198

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

typedef struct rangetypeStruct rangetype;
struct rangetypeStruct
{
  mpfr_t *a;
  mpfr_t *b;
};

#define DISPLAY_MODE_DECIMAL     0
#define DISPLAY_MODE_DYADIC      1
#define DISPLAY_MODE_POWERS      2
#define DISPLAY_MODE_BINARY      3
#define DISPLAY_MODE_HEXADECIMAL 4


extern char *getNameOfVariable();
extern int setNameOfVariable(char *);
extern mp_prec_t getToolPrecision();
extern void setToolPrecision(mp_prec_t);
extern int getToolPoints(); 
extern void setToolPoints(int);
extern int getToolTaylorRecursions();
extern void setToolTaylorRecursions(int);
extern int getToolHopitalRecursions();
extern void setToolHopitalRecursions(int);
extern int getToolDiameter(mpfr_t);
extern void setToolDiameter(mpfr_t);
extern int getDisplayMode();
extern int setDisplayMode(int);
extern int getVerbosity();
extern int setVerbosity(int);
extern int getCanonical();
extern void setCanonical(int);
extern int getAutosimplify();
extern void setAutosimplify(int);
extern int getFullParentheses();
extern void setFullParentheses(int);
extern int getMidpointMode();
extern void setMidpointMode(int);
extern int getDieOnErrorMode();
extern void setDieOnErrorMode(int);
extern int getTimecounting();
extern void setTimecounting(int);
extern int getRoundingWarnings();
extern void setRoundingWarnings(int);
extern int getRationalMode();
extern void setRationalMode(int);

extern void initTool();
extern void finishTool();
extern void setRecoverEnvironment(jmp_buf *);
extern void invalidateRecoverEnvironment();
extern void changeToWarningMode();
extern void restoreMode();

extern node *makeVariable();
extern node *makeConstant(mpfr_t x);
extern node *makeConstantDouble(double d);
extern node *makeAdd(node *op1, node *op2);
extern node *makeSub(node *op1, node *op2);
extern node *makeMul(node *op1, node *op2);
extern node *makeDiv(node *op1, node *op2);
extern node *makeSqrt(node *op1);
extern node *makeExp(node *op1);
extern node *makeLog(node *op1);
extern node *makeLog2(node *op1);
extern node *makeLog10(node *op1);
extern node *makeSin(node *op1);
extern node *makeCos(node *op1);
extern node *makeTan(node *op1);
extern node *makeAsin(node *op1);
extern node *makeAcos(node *op1);
extern node *makeAtan(node *op1);
extern node *makePow(node *op1, node *op2);
extern node *makeNeg(node *op1);
extern node *makeAbs(node *op1);
extern node *makeDouble(node *op1);
extern node *makeDoubledouble(node *op1);
extern node *makeTripledouble(node *op1);
extern node *makeErf(node *op1);
extern node *makeErfc(node *op1);
extern node *makeLog1p(node *op1);
extern node *makeExpm1(node *op1);
extern node *makeDoubleextended(node *op1);
extern node *makeCeil(node *op1);
extern node *makeFloor(node *op1);
extern node *makeNearestInt(node *op1);
extern node *makePi();
extern node *makeSinh(node *op1);
extern node *makeCosh(node *op1);
extern node *makeTanh(node *op1);
extern node *makeAsinh(node *op1);
extern node *makeAcosh(node *op1);
extern node *makeAtanh(node *op1);


extern node *parseString(char *str); 

extern void free_memory(node *tree);

extern int printMessage(int verb, const char *format, ...);
extern void printTree(node *tree);
extern void fprintTree(FILE *fd, node *tree);
extern char *sprintTree(node *tree);

extern node* copyTree(node *tree);
extern node* differentiate(node *tree);
extern node* simplifyTree(node *tree); 
extern node* simplifyTreeErrorfree(node *tree);
extern node* horner(node *tree);
extern node* expand(node *tree);
extern node *substitute(node* tree, node *t);
extern node *makeCanonical(node *func, mp_prec_t prec);

extern node *makePolynomial(mpfr_t *coefficients, int degree);
extern node *getIthCoefficient(node *poly, int i);
extern void getCoefficients(int *degree, node ***coefficients, node *poly);

extern void evaluateRangeFunction(rangetype y, node *f, rangetype x, mp_prec_t prec);
extern int evaluateFaithful(mpfr_t result, node *tree, mpfr_t x, mp_prec_t prec);
extern int evaluateFaithfulWithCutOff(mpfr_t result, node *func, mpfr_t x, mpfr_t cutoff, mp_prec_t startprec);
extern int evaluateFaithfulWithCutOffFast(mpfr_t result, node *func, node *deriv, mpfr_t x, mpfr_t cutoff, mp_prec_t startprec);

extern int getDegree(node *tree);
extern int isPolynomial(node *tree);
extern int isSyntacticallyEqual(node *tree1, node *tree2);
extern int isConstant(node *tree);
extern int isHorner(node *tree);
extern int isCanonical(node *tree);

extern rangetype infnorm(node *func, rangetype range, chain *excludes, mp_prec_t prec, mpfr_t diam, FILE *proof);
extern void uncertifiedInfnorm(mpfr_t result, node *tree, mpfr_t a, mpfr_t b, unsigned long int points, mp_prec_t prec);
extern int checkInfnorm(node *func, rangetype range, mpfr_t infnormval, mpfr_t diam, mp_prec_t prec);

extern int newtonMPFR(mpfr_t res, node *tree, node *diff_tree, mpfr_t a, mpfr_t b, mp_prec_t prec);
extern int newtonMPFRWithStartPoint(mpfr_t res, node *tree, node *diff_tree, mpfr_t a, mpfr_t b, mpfr_t start, mp_prec_t prec);
extern chain* findZerosFunction(node *func, rangetype range, mp_prec_t prec, mpfr_t diam);
extern chain* fpFindZerosFunction(node *func, rangetype range, mp_prec_t prec);

extern node* remez(node *func, node *weight, chain* monom, mpfr_t a, mpfr_t b, mpfr_t *requestedQuality, mp_prec_t prec);
extern rangetype guessDegree(node *func, node *weight, mpfr_t a, mpfr_t b, mpfr_t eps);

extern node *FPminimax(node *f,	chain *monomials, chain *formats, chain *points, mpfr_t a, mpfr_t b, int fp, int absrel, node *consPart, node *minimax);

extern int implementconst(node *, FILE *, char *);
extern void freeChain(chain *c, void (*f) (void *));
extern chain *addElement(chain *c, void *elem);
extern void *first(chain *c);
extern chain *tail(chain *c);
extern chain *copyChain(chain *c, void * (*f) (void *));
extern chain *concatChains(chain *c1, chain *c2);
extern int lengthChain(chain *c);
extern void sortChain(chain *c,  int (*f) (void *, void *));
extern chain *makeIntPtrChainFromTo(int m, int n);
extern void freeIntPtr(void *ptr);
extern void *accessInList(chain *, int);
extern chain *removeInt(chain *c, int n);

extern void *safeCalloc (size_t nmemb, size_t size);
extern void *safeMalloc (size_t size);

extern void printInterval(sollya_mpfi_t);
extern void printValue(mpfr_t *);
extern node* simplifyTreeErrorfree(node *tree);

#ifdef __cplusplus
}
#endif

#endif
