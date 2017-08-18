/*

Copyright 2008 by 

Laboratoire de l'Informatique du Parallélisme, 
UMR CNRS - ENS Lyon - UCB Lyon 1 - INRIA 5668

Contributors Ch. Lauter, S. Chevillard, N. Jourdan

christoph.lauter@ens-lyon.fr
sylvain.chevillard@ens-lyon.fr
nicolas.jourdan@ens-lyon.fr

This software is a computer program whose purpose is to provide an
environment for safe floating-point code development. It is
particularily targeted to the automatized implementation of
mathematical floating-point libraries (libm). Amongst other features,
it offers a certified infinite norm, an automatic polynomial
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

*/

#ifndef SOLLYA_H
#define SOLLYA_H

#include <mpfr.h>
#include <setjmp.h>

#ifdef __cplusplus
extern "C" {
#endif
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
#define FIXED 236
#define FLOATING 237
#define ABSOLUTESYM 197
#define RELATIVESYM 198
#define SINGLE 39

typedef struct sollya_chain* sollya_chain_t;
typedef struct sollya_node* sollya_node_t;
typedef struct sollya_range* sollya_range_t;

char *getNameOfVariable();
int setNameOfVariable(char *);
mp_prec_t getToolPrecision();
void setToolPrecision(mp_prec_t);
int getToolPoints(); 
void setToolPoints(int);
int getToolTaylorRecursions();
void setToolTaylorRecursions(int);
int getToolHopitalRecursions();
void setToolHopitalRecursions(int);
int getToolDiameter(mpfr_t);
void setToolDiameter(mpfr_t);

void initTool();
void finishTool();
void setRecoverEnvironment(jmp_buf *);
void invalidateRecoverEnvironment();

sollya_node_t makeVariable();
sollya_node_t makeConstant(mpfr_t x);
sollya_node_t makeConstantDouble(double d);
sollya_node_t makeAdd(sollya_node_t op1, sollya_node_t op2);
sollya_node_t makeSub(sollya_node_t op1, sollya_node_t op2);
sollya_node_t makeMul(sollya_node_t op1, sollya_node_t op2);
sollya_node_t makeDiv(sollya_node_t op1, sollya_node_t op2);
sollya_node_t makeSqrt(sollya_node_t op1);
sollya_node_t makeExp(sollya_node_t op1);
sollya_node_t makeLog(sollya_node_t op1);
sollya_node_t makeLog2(sollya_node_t op1);
sollya_node_t makeLog10(sollya_node_t op1);
sollya_node_t makeSin(sollya_node_t op1);
sollya_node_t makeCos(sollya_node_t op1);
sollya_node_t makeTan(sollya_node_t op1);
sollya_node_t makeAsin(sollya_node_t op1);
sollya_node_t makeAcos(sollya_node_t op1);
sollya_node_t makeAtan(sollya_node_t op1);
sollya_node_t makePow(sollya_node_t op1, sollya_node_t op2);
sollya_node_t makeNeg(sollya_node_t op1);
sollya_node_t makeAbs(sollya_node_t op1);
sollya_node_t makeDouble(sollya_node_t op1);
sollya_node_t makeDoubledouble(sollya_node_t op1);
sollya_node_t makeTripledouble(sollya_node_t op1);
sollya_node_t makeErf(sollya_node_t op1);
sollya_node_t makeErfc(sollya_node_t op1);
sollya_node_t makeLog1p(sollya_node_t op1);
sollya_node_t makeExpm1(sollya_node_t op1);
sollya_node_t makeDoubleextended(sollya_node_t op1);
sollya_node_t makeCeil(sollya_node_t op1);
sollya_node_t makeFloor(sollya_node_t op1);
sollya_node_t makePi();
sollya_node_t makeSinh(sollya_node_t op1);
sollya_node_t makeCosh(sollya_node_t op1);
sollya_node_t makeTanh(sollya_node_t op1);
sollya_node_t makeAsinh(sollya_node_t op1);
sollya_node_t makeAcosh(sollya_node_t op1);
sollya_node_t makeAtanh(sollya_node_t op1);

sollya_node_t parseString(const char *str); 

void free_memory(sollya_node_t tree);

void printTree(sollya_node_t tree);
void fprintTree(FILE *fd, sollya_node_t tree);
char *sprintTree(sollya_node_t tree);

sollya_node_t copyTree(sollya_node_t tree);
sollya_node_t differentiate(sollya_node_t tree);
sollya_node_t simplifyTree(sollya_node_t tree); 
sollya_node_t simplifyTreeErrorfree(sollya_node_t tree);
sollya_node_t horner(sollya_node_t tree);
sollya_node_t expand(sollya_node_t tree);
sollya_node_t substitute(sollya_node_t tree, sollya_node_t t);
sollya_node_t makeCanonical(sollya_node_t func, mp_prec_t prec);

sollya_node_t makePolynomial(mpfr_t *coefficients, int degree);
sollya_node_t getIthCoefficient(sollya_node_t poly, int i);
void getCoefficients(int *degree, sollya_node_t **coefficients, sollya_node_t poly);

void evaluateConstantExpression(mpfr_t result, sollya_node_t tree, mp_prec_t prec);
void evaluateFaithful(mpfr_t result, sollya_node_t tree, mpfr_t x, mp_prec_t prec);
int evaluateFaithfulWithCutOff(mpfr_t result, sollya_node_t func, mpfr_t x, mpfr_t cutoff, mp_prec_t startprec);

int getDegree(sollya_node_t tree);
int isPolynomial(sollya_node_t tree);
int isSyntacticallyEqual(sollya_node_t tree1, sollya_node_t tree2);
int isConstant(sollya_node_t tree);
int isHorner(sollya_node_t tree);
int isCanonical(sollya_node_t tree);

sollya_range_t infnorm(sollya_node_t func, sollya_range_t range, sollya_chain_t *excludes, mp_prec_t prec, mpfr_t diam, FILE *proof);
void uncertifiedInfnorm(mpfr_t result, sollya_node_t tree, mpfr_t a, mpfr_t b, unsigned long int points, mp_prec_t prec);
int checkInfnorm(sollya_node_t func, sollya_range_t range, mpfr_t infnormval, mpfr_t diam, mp_prec_t prec);

int newtonMPFR(mpfr_t res, sollya_node_t tree, sollya_node_t diff_tree, mpfr_t a, mpfr_t b, mp_prec_t prec);
int newtonMPFRWithStartPoint(mpfr_t res, sollya_node_t tree, sollya_node_t diff_tree, mpfr_t a, mpfr_t b, mpfr_t start, mp_prec_t prec);
sollya_chain_t findZerosFunction(sollya_node_t func, sollya_range_t range, mp_prec_t prec, mpfr_t diam);
sollya_chain_t fpFindZerosFunction(sollya_node_t func, sollya_range_t range, mp_prec_t prec);

sollya_node_t remez(sollya_node_t func, sollya_node_t weight, sollya_chain_t monom, mpfr_t a, mpfr_t b, mpfr_t *requestedQuality, mp_prec_t prec);
sollya_range_t guessDegree(sollya_node_t func, sollya_node_t weight, mpfr_t a, mpfr_t b, mpfr_t eps);

sollya_node_t FPminimax(sollya_node_t f,	sollya_chain_t monomials, sollya_chain_t formats,  sollya_chain_t points, mpfr_t a, mpfr_t b, int fp, int absrel, sollya_node_t consPart, sollya_node_t minimax);


void uncertifiedInfnorm(mpfr_t result, sollya_node_t tree, mpfr_t a, mpfr_t b, unsigned long int points, mp_prec_t prec);

sollya_node_t taylor(sollya_node_t tree, int degree, sollya_node_t point, mp_prec_t prec);

void freeChain(sollya_chain_t c, void (*f) (void *));
sollya_chain_t addElement(sollya_chain_t c, void *elem);
void *first(sollya_chain_t c);
sollya_chain_t tail(sollya_chain_t c);
sollya_chain_t copyChain(sollya_chain_t c, void * (*f) (void *));
sollya_chain_t concatChains(sollya_chain_t c1, sollya_chain_t c2);
int lengthChain(sollya_chain_t c);
void sortChain(sollya_chain_t c,  int (*f) (void *, void *));
sollya_chain_t makeIntPtrChainFromTo(int m, int n);
void freeIntPtr(void *ptr);

void *safeCalloc (size_t nmemb, size_t size);
void *safeMalloc (size_t size);



#ifdef __cplusplus
}
#endif

#endif
