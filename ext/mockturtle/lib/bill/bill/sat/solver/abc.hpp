#pragma once

/*** satStore.cpp ***/

/**CFile****************************************************************

  FileName    [satStore.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT solver.]

  Synopsis    [Records the trace of SAT solving in the CNF form.]

  Author      [Alan Mishchenko]

  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: satStore.c,v 1.4 2005/09/16 22:55:03 casem Exp $]

***********************************************************************/

#include "abc/satStore.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Fetches memory.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
inline char* Sto_ManMemoryFetch(Sto_Man_t* p, int nBytes)
{
	char* pMem;
	if (p->pChunkLast == NULL || nBytes > p->nChunkSize - p->nChunkUsed) {
		pMem = (char*) ABC_ALLOC(char, p->nChunkSize);
		*(char**) pMem = p->pChunkLast;
		p->pChunkLast = pMem;
		p->nChunkUsed = sizeof(char*);
	}
	pMem = p->pChunkLast + p->nChunkUsed;
	p->nChunkUsed += nBytes;
	return pMem;
}

/**Function*************************************************************

  Synopsis    [Frees memory manager.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
inline void Sto_ManMemoryStop(Sto_Man_t* p)
{
	char *pMem, *pNext;
	if (p->pChunkLast == NULL)
		return;
	for (pMem = p->pChunkLast; (pNext = *(char**) pMem); pMem = pNext)
		ABC_FREE(pMem);
	ABC_FREE(pMem);
}

/**Function*************************************************************

  Synopsis    [Reports memory usage in bytes.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
inline int Sto_ManMemoryReport(Sto_Man_t* p)
{
	int Total;
	char *pMem, *pNext;
	if (p->pChunkLast == NULL)
		return 0;
	Total = p->nChunkUsed;
	for (pMem = p->pChunkLast; (pNext = *(char**) pMem); pMem = pNext)
		Total += p->nChunkSize;
	return Total;
}

/**Function*************************************************************

  Synopsis    [Allocate proof manager.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
inline Sto_Man_t* Sto_ManAlloc()
{
	Sto_Man_t* p;
	// allocate the manager
	p = (Sto_Man_t*) ABC_ALLOC(char, sizeof(Sto_Man_t));
	memset(p, 0, sizeof(Sto_Man_t));
	// memory management
	p->nChunkSize = (1 << 16); // use 64K chunks
	return p;
}

/**Function*************************************************************

  Synopsis    [Deallocate proof manager.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
inline void Sto_ManFree(Sto_Man_t* p)
{
	Sto_ManMemoryStop(p);
	ABC_FREE(p);
}

/**Function*************************************************************

  Synopsis    [Adds one clause to the manager.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
inline int Sto_ManAddClause(Sto_Man_t* p, lit* pBeg, lit* pEnd)
{
	Sto_Cls_t* pClause;
	lit Lit, *i, *j;
	int nSize;

	// process the literals
	if (pBeg < pEnd) {
		// insertion sort
		for (i = pBeg + 1; i < pEnd; i++) {
			Lit = *i;
			for (j = i; j > pBeg && *(j - 1) > Lit; j--)
				*j = *(j - 1);
			*j = Lit;
		}
		// make sure there is no duplicated variables
		for (i = pBeg + 1; i < pEnd; i++)
			if (lit_var(*(i - 1)) == lit_var(*i)) {
				printf("The clause contains two literals of the same variable: %d "
				       "and %d.\n",
				       *(i - 1), *i);
				return 0;
			}
		// check the largest var size
		p->nVars = STO_MAX(p->nVars, lit_var(*(pEnd - 1)) + 1);
	}

	// get memory for the clause
	nSize = sizeof(Sto_Cls_t) + sizeof(lit) * (pEnd - pBeg);
	nSize = (nSize / sizeof(char*) + ((nSize % sizeof(char*)) > 0))
	        * sizeof(char*); // added by Saurabh on Sep 3, 2009
	pClause = (Sto_Cls_t*) Sto_ManMemoryFetch(p, nSize);
	memset(pClause, 0, sizeof(Sto_Cls_t));

	// assign the clause
	pClause->Id = p->nClauses++;
	pClause->nLits = pEnd - pBeg;
	memcpy(pClause->pLits, pBeg, sizeof(lit) * (pEnd - pBeg));
	//    assert( pClause->pLits[0] >= 0 );

	// add the clause to the list
	if (p->pHead == NULL)
		p->pHead = pClause;
	if (p->pTail == NULL)
		p->pTail = pClause;
	else {
		p->pTail->pNext = pClause;
		p->pTail = pClause;
	}

	// add the empty clause
	if (pClause->nLits == 0) {
		if (p->pEmpty) {
			printf("More than one empty clause!\n");
			return 0;
		}
		p->pEmpty = pClause;
	}
	return 1;
}

/**Function*************************************************************

  Synopsis    [Mark all clauses added so far as root clauses.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
inline void Sto_ManMarkRoots(Sto_Man_t* p)
{
	Sto_Cls_t* pClause;
	p->nRoots = 0;
	Sto_ManForEachClause(p, pClause)
	{
		pClause->fRoot = 1;
		p->nRoots++;
	}
}

/**Function*************************************************************

  Synopsis    [Mark all clauses added so far as clause of A.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
inline void Sto_ManMarkClausesA(Sto_Man_t* p)
{
	Sto_Cls_t* pClause;
	p->nClausesA = 0;
	Sto_ManForEachClause(p, pClause)
	{
		pClause->fA = 1;
		p->nClausesA++;
	}
}

/**Function*************************************************************

  Synopsis    [Returns the literal of the last clause.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
inline int Sto_ManChangeLastClause(Sto_Man_t* p)
{
	Sto_Cls_t *pClause, *pPrev;
	pPrev = NULL;
	Sto_ManForEachClause(p, pClause) pPrev = pClause;
	assert(pPrev != NULL);
	assert(pPrev->fA == 1);
	assert(pPrev->nLits == 1);
	p->nClausesA--;
	pPrev->fA = 0;
	return pPrev->pLits[0] >> 1;
}

/**Function*************************************************************

  Synopsis    [Writes the stored clauses into a file.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
inline void Sto_ManDumpClauses(Sto_Man_t* p, char* pFileName)
{
	FILE* pFile;
	Sto_Cls_t* pClause;
	int i;
	// start the file
	pFile = fopen(pFileName, "w");
	if (pFile == NULL) {
		printf("Error: Cannot open output file (%s).\n", pFileName);
		return;
	}
	// write the data
	fprintf(pFile, "p %d %d %d %d\n", p->nVars, p->nClauses, p->nRoots, p->nClausesA);
	Sto_ManForEachClause(p, pClause)
	{
		for (i = 0; i < (int) pClause->nLits; i++)
			fprintf(pFile, " %d", lit_print(pClause->pLits[i]));
		fprintf(pFile, " 0\n");
	}
	//    fprintf( pFile, " 0\n" );
	fclose(pFile);
}

/**Function*************************************************************

  Synopsis    [Reads one literal from file.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
inline int Sto_ManLoadNumber(FILE* pFile, int* pNumber)
{
	int Char, Number = 0, Sign = 0;
	// skip space-like chars
	do {
		Char = fgetc(pFile);
		if (Char == EOF)
			return 0;
	} while (Char == ' ' || Char == '\t' || Char == '\r' || Char == '\n');
	// read the literal
	while (1) {
		// get the next character
		Char = fgetc(pFile);
		if (Char == ' ' || Char == '\t' || Char == '\r' || Char == '\n')
			break;
		// check that the char is a digit
		if ((Char < '0' || Char > '9') && Char != '-') {
			printf("Error: Wrong char (%c) in the input file.\n", Char);
			return 0;
		}
		// check if this is a minus
		if (Char == '-')
			Sign = 1;
		else
			Number = 10 * Number + Char;
	}
	// return the number
	*pNumber = Sign ? -Number : Number;
	return 1;
}

/**Function*************************************************************

  Synopsis    [Reads CNF from file.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
inline Sto_Man_t* Sto_ManLoadClauses(char* pFileName)
{
	FILE* pFile;
	Sto_Man_t* p;
	Sto_Cls_t* pClause;
	char pBuffer[1024];
	int nLits, nLitsAlloc, Counter, Number;
	lit* pLits;

	// start the file
	pFile = fopen(pFileName, "r");
	if (pFile == NULL) {
		printf("Error: Cannot open input file (%s).\n", pFileName);
		return NULL;
	}

	// create the manager
	p = Sto_ManAlloc();

	// alloc the array of literals
	nLitsAlloc = 1024;
	pLits = (lit*) ABC_ALLOC(char, sizeof(lit) * nLitsAlloc);

	// read file header
	p->nVars = p->nClauses = p->nRoots = p->nClausesA = 0;
	while (fgets(pBuffer, 1024, pFile)) {
		if (pBuffer[0] == 'c')
			continue;
		if (pBuffer[0] == 'p') {
			sscanf(pBuffer + 1, "%d %d %d %d", &p->nVars, &p->nClauses, &p->nRoots,
			       &p->nClausesA);
			break;
		}
		printf("Warning: Skipping line: \"%s\"\n", pBuffer);
	}

	// read the clauses
	nLits = 0;
	while (Sto_ManLoadNumber(pFile, &Number)) {
		if (Number == 0) {
			int RetValue;
			RetValue = Sto_ManAddClause(p, pLits, pLits + nLits);
			assert(RetValue);
			nLits = 0;
			continue;
		}
		if (nLits == nLitsAlloc) {
			nLitsAlloc *= 2;
			pLits = ABC_REALLOC(lit, pLits, nLitsAlloc);
		}
		pLits[nLits++] = lit_read(Number);
	}
	if (nLits > 0)
		printf("Error: The last clause was not saved.\n");

	// count clauses
	Counter = 0;
	Sto_ManForEachClause(p, pClause) Counter++;

	// check the number of clauses
	if (p->nClauses != Counter) {
		printf(
		    "Error: The actual number of clauses (%d) is different than declared (%d).\n",
		    Counter, p->nClauses);
		Sto_ManFree(p);
		return NULL;
	}

	ABC_FREE(pLits);
	fclose(pFile);
	return p;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_IMPL_END

/*** satSolver.cpp ***/

/**************************************************************************************************
MiniSat -- Copyright (c) 2005, Niklas Sorensson
http://www.cs.chalmers.se/Cs/Research/FormalMethods/MiniSat/

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/
// Modified to compile with MS Visual Studio 6.0 by Alan Mishchenko

#include "abc/satSolver.h"
#include "abc/satStore.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

ABC_NAMESPACE_IMPL_START

#define SAT_USE_ANALYZE_FINAL

//=================================================================================================
// Debug:

//#define VERBOSEDEBUG

/**Function*************************************************************

  Synopsis    [Merging two lists of entries.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
inline void Abc_MergeSortCostMerge(int* p1Beg, int* p1End, int* p2Beg, int* p2End, int* pOut)
{
	int nEntries = (p1End - p1Beg) + (p2End - p2Beg);
	int* pOutBeg = pOut;
	while (p1Beg < p1End && p2Beg < p2End) {
		if (p1Beg[1] == p2Beg[1])
			*pOut++ = *p1Beg++, *pOut++ = *p1Beg++, *pOut++ = *p2Beg++,
			*pOut++ = *p2Beg++;
		else if (p1Beg[1] < p2Beg[1])
			*pOut++ = *p1Beg++, *pOut++ = *p1Beg++;
		else // if ( p1Beg[1] > p2Beg[1] )
			*pOut++ = *p2Beg++, *pOut++ = *p2Beg++;
	}
	while (p1Beg < p1End)
		*pOut++ = *p1Beg++, *pOut++ = *p1Beg++;
	while (p2Beg < p2End)
		*pOut++ = *p2Beg++, *pOut++ = *p2Beg++;
	assert(pOut - pOutBeg == nEntries);
}

/**Function*************************************************************

  Synopsis    [Recursive sorting.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
inline void Abc_MergeSortCost_rec(int* pInBeg, int* pInEnd, int* pOutBeg)
{
	int nSize = (pInEnd - pInBeg) / 2;
	assert(nSize > 0);
	if (nSize == 1)
		return;
	if (nSize == 2) {
		if (pInBeg[1] > pInBeg[3]) {
			pInBeg[1] ^= pInBeg[3];
			pInBeg[3] ^= pInBeg[1];
			pInBeg[1] ^= pInBeg[3];
			pInBeg[0] ^= pInBeg[2];
			pInBeg[2] ^= pInBeg[0];
			pInBeg[0] ^= pInBeg[2];
		}
	} else if (nSize < 8) {
		int temp, i, j, best_i;
		for (i = 0; i < nSize - 1; i++) {
			best_i = i;
			for (j = i + 1; j < nSize; j++)
				if (pInBeg[2 * j + 1] < pInBeg[2 * best_i + 1])
					best_i = j;
			temp = pInBeg[2 * i];
			pInBeg[2 * i] = pInBeg[2 * best_i];
			pInBeg[2 * best_i] = temp;
			temp = pInBeg[2 * i + 1];
			pInBeg[2 * i + 1] = pInBeg[2 * best_i + 1];
			pInBeg[2 * best_i + 1] = temp;
		}
	} else {
		Abc_MergeSortCost_rec(pInBeg, pInBeg + 2 * (nSize / 2), pOutBeg);
		Abc_MergeSortCost_rec(pInBeg + 2 * (nSize / 2), pInEnd, pOutBeg + 2 * (nSize / 2));
		Abc_MergeSortCostMerge(pInBeg, pInBeg + 2 * (nSize / 2), pInBeg + 2 * (nSize / 2),
		                       pInEnd, pOutBeg);
		memcpy(pInBeg, pOutBeg, sizeof(int) * 2 * nSize);
	}
}

/**Function*************************************************************

  Synopsis    [Sorting procedure.]

  Description [Returns permutation for the non-decreasing order of costs.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
inline int* Abc_MergeSortCost(int* pCosts, int nSize)
{
	int i, *pResult, *pInput, *pOutput;
	pResult = (int*) calloc(sizeof(int), nSize);
	if (nSize < 2)
		return pResult;
	pInput = (int*) malloc(sizeof(int) * 2 * nSize);
	pOutput = (int*) malloc(sizeof(int) * 2 * nSize);
	for (i = 0; i < nSize; i++)
		pInput[2 * i] = i, pInput[2 * i + 1] = pCosts[i];
	Abc_MergeSortCost_rec(pInput, pInput + 2 * nSize, pOutput);
	for (i = 0; i < nSize; i++)
		pResult[i] = pInput[2 * i];
	free(pOutput);
	free(pInput);
	return pResult;
}

// For derivation output (verbosity level 2)
#define L_IND "%-*d"
#define L_ind sat_solver_dl(s) * 2 + 2, sat_solver_dl(s)
#define L_LIT "%sx%d"
#define L_lit(p) lit_sign(p) ? "~" : "", (lit_var(p))

// Just like 'assert()' but expression will be evaluated in the release version as well.
static inline void check(int expr)
{
	assert(expr);
}

// static inline void printlits(lit* begin, lit* end)
// {
//     int i;
//     for (i = 0; i < end - begin; i++)
//         printf(L_LIT" ",L_lit(begin[i]));
// }

//=================================================================================================
// Random numbers:

// Returns a random float 0 <= x < 1. Seed must never be 0.
static inline double drand(double* seed)
{
	int q;
	*seed *= 1389796;
	q = (int) (*seed / 2147483647);
	*seed -= (double) q * 2147483647;
	return *seed / 2147483647;
}

// Returns a random integer 0 <= x < size. Seed must never be 0.
static inline int irand(double* seed, int size)
{
	return (int) (drand(seed) * size);
}

//=================================================================================================
// Variable datatype + minor functions:

static const int var0 = 1;
static const int var1 = 0;
static const int varX = 3;

struct varinfo_t {
	unsigned val : 2;  // variable value
	unsigned pol : 1;  // last polarity
	unsigned tag : 1;  // conflict analysis tag
	unsigned lev : 28; // variable level
};

static inline int var_level(sat_solver* s, int v)
{
	return s->levels[v];
}
static inline int var_value(sat_solver* s, int v)
{
	return s->assigns[v];
}
static inline int var_polar(sat_solver* s, int v)
{
	return s->polarity[v];
}

static inline void var_set_level(sat_solver* s, int v, int lev)
{
	s->levels[v] = lev;
}
static inline void var_set_value(sat_solver* s, int v, int val)
{
	s->assigns[v] = val;
}
static inline void var_set_polar(sat_solver* s, int v, int pol)
{
	s->polarity[v] = pol;
}

// variable tags
static inline int var_tag(sat_solver* s, int v)
{
	return s->tags[v];
}
static inline void var_set_tag(sat_solver* s, int v, int tag)
{
	assert(tag > 0 && tag < 16);
	if (s->tags[v] == 0)
		veci_push(&s->tagged, v);
	s->tags[v] = tag;
}
static inline void var_add_tag(sat_solver* s, int v, int tag)
{
	assert(tag > 0 && tag < 16);
	if (s->tags[v] == 0)
		veci_push(&s->tagged, v);
	s->tags[v] |= tag;
}
static inline void solver2_clear_tags(sat_solver* s, int start)
{
	int i, *tagged = veci_begin(&s->tagged);
	for (i = start; i < veci_size(&s->tagged); i++)
		s->tags[tagged[i]] = 0;
	veci_resize(&s->tagged, start);
}

inline int sat_solver_get_var_value(sat_solver* s, int v)
{
	if (var_value(s, v) == var0)
		return l_False;
	if (var_value(s, v) == var1)
		return l_True;
	if (var_value(s, v) == varX)
		return l_Undef;
	assert(0);
	return 0;
}

//=================================================================================================
// Simple helpers:

static inline int sat_solver_dl(sat_solver* s)
{
	return veci_size(&s->trail_lim);
}
static inline veci* sat_solver_read_wlist(sat_solver* s, lit l)
{
	return &s->wlists[l];
}

//=================================================================================================
// Variable order functions:

static inline void order_update(sat_solver* s, int v) // updateorder
{
	int* orderpos = s->orderpos;
	int* heap = veci_begin(&s->order);
	int i = orderpos[v];
	int x = heap[i];
	int parent = (i - 1) / 2;

	assert(s->orderpos[v] != -1);

	while (i != 0 && s->activity[x] > s->activity[heap[parent]]) {
		heap[i] = heap[parent];
		orderpos[heap[i]] = i;
		i = parent;
		parent = (i - 1) / 2;
	}

	heap[i] = x;
	orderpos[x] = i;
}

static inline void order_assigned(sat_solver* s, int v)
{}

static inline void order_unassigned(sat_solver* s, int v) // undoorder
{
	int* orderpos = s->orderpos;
	if (orderpos[v] == -1) {
		orderpos[v] = veci_size(&s->order);
		veci_push(&s->order, v);
		order_update(s, v);
		// printf( "+%d ", v );
	}
}

static inline int order_select(sat_solver* s, float random_var_freq) // selectvar
{
	int* heap = veci_begin(&s->order);
	int* orderpos = s->orderpos;
	// Random decision:
	if (drand(&s->random_seed) < random_var_freq) {
		int next = irand(&s->random_seed, s->size);
		assert(next >= 0 && next < s->size);
		if (var_value(s, next) == varX)
			return next;
	}
	// Activity based decision:
	while (veci_size(&s->order) > 0) {
		int next = heap[0];
		int size = veci_size(&s->order) - 1;
		int x = heap[size];
		veci_resize(&s->order, size);
		orderpos[next] = -1;
		if (size > 0) {
			int i = 0;
			int child = 1;
			while (child < size) {

				if (child + 1 < size
				    && s->activity[heap[child]] < s->activity[heap[child + 1]])
					child++;
				assert(child < size);
				if (s->activity[x] >= s->activity[heap[child]])
					break;

				heap[i] = heap[child];
				orderpos[heap[i]] = i;
				i = child;
				child = 2 * child + 1;
			}
			heap[i] = x;
			orderpos[heap[i]] = i;
		}
		if (var_value(s, next) == varX)
			return next;
	}
	return var_Undef;
}

inline void sat_solver_set_var_activity(sat_solver* s, int* pVars, int nVars)
{
	int i;
	assert(s->VarActType == 1);
	for (i = 0; i < s->size; i++)
		s->activity[i] = 0;
	s->var_inc = Abc_Dbl2Word(1);
	for (i = 0; i < nVars; i++) {
		int iVar = pVars ? pVars[i] : i;
		s->activity[iVar] = Abc_Dbl2Word(nVars - i);
		order_update(s, iVar);
	}
}

//=================================================================================================
// variable activities

inline void solver_init_activities(sat_solver* s)
{
	// variable activities
	if (s->VarActType == 0) {
		s->var_inc = (1 << 5);
		s->var_decay = -1;
	} else if (s->VarActType == 1) {
		s->var_inc = Abc_Dbl2Word(1.0);
		s->var_decay = Abc_Dbl2Word(1.0 / 0.95);
	} else if (s->VarActType == 2) {
		s->var_inc = Xdbl_FromDouble(1.0);
		s->var_decay = Xdbl_FromDouble(1.0 / 0.950);
	} else
		assert(0);

	// clause activities
	if (s->ClaActType == 0) {
		s->cla_inc = (1 << 11);
		s->cla_decay = -1;
	} else {
		s->cla_inc = 1;
		s->cla_decay = (float) (1 / 0.999);
	}
}

static inline void act_var_rescale(sat_solver* s)
{
	if (s->VarActType == 0) {
		word* activity = s->activity;
		int i;
		for (i = 0; i < s->size; i++)
			activity[i] >>= 19;
		s->var_inc >>= 19;
		s->var_inc = Abc_MaxInt((unsigned) s->var_inc, (1 << 4));
	} else if (s->VarActType == 1) {
		double* activity = (double*) s->activity;
		int i;
		for (i = 0; i < s->size; i++)
			activity[i] *= 1e-100;
		s->var_inc = Abc_Dbl2Word(Abc_Word2Dbl(s->var_inc) * 1e-100);
		// printf( "Rescaling var activity...\n" );
	} else if (s->VarActType == 2) {
		xdbl* activity = s->activity;
		int i;
		for (i = 0; i < s->size; i++)
			activity[i] = Xdbl_Div(activity[i], 200); // activity[i] / 2^200
		s->var_inc = Xdbl_Div(s->var_inc, 200);
	} else
		assert(0);
}
static inline void act_var_bump(sat_solver* s, int v)
{
	if (s->VarActType == 0) {
		s->activity[v] += s->var_inc;
		if ((unsigned) s->activity[v] & 0x80000000)
			act_var_rescale(s);
		if (s->orderpos[v] != -1)
			order_update(s, v);
	} else if (s->VarActType == 1) {
		double act = Abc_Word2Dbl(s->activity[v]) + Abc_Word2Dbl(s->var_inc);
		s->activity[v] = Abc_Dbl2Word(act);
		if (act > 1e100)
			act_var_rescale(s);
		if (s->orderpos[v] != -1)
			order_update(s, v);
	} else if (s->VarActType == 2) {
		s->activity[v] = Xdbl_Add(s->activity[v], s->var_inc);
		if (s->activity[v] > ABC_CONST(0x014c924d692ca61b))
			act_var_rescale(s);
		if (s->orderpos[v] != -1)
			order_update(s, v);
	} else
		assert(0);
}
static inline void act_var_bump_global(sat_solver* s, int v)
{
	if (!s->pGlobalVars || !s->pGlobalVars[v])
		return;
	if (s->VarActType == 0) {
		s->activity[v] += (int) ((unsigned) s->var_inc * 3);
		if (s->activity[v] & 0x80000000)
			act_var_rescale(s);
		if (s->orderpos[v] != -1)
			order_update(s, v);
	} else if (s->VarActType == 1) {
		double act = Abc_Word2Dbl(s->activity[v]) + Abc_Word2Dbl(s->var_inc) * 3.0;
		s->activity[v] = Abc_Dbl2Word(act);
		if (act > 1e100)
			act_var_rescale(s);
		if (s->orderpos[v] != -1)
			order_update(s, v);
	} else if (s->VarActType == 2) {
		s->activity[v] = Xdbl_Add(s->activity[v], Xdbl_Mul(s->var_inc, Xdbl_FromDouble(3.0)));
		if (s->activity[v] > ABC_CONST(0x014c924d692ca61b))
			act_var_rescale(s);
		if (s->orderpos[v] != -1)
			order_update(s, v);
	} else
		assert(0);
}
static inline void act_var_bump_factor(sat_solver* s, int v)
{
	if (!s->factors)
		return;
	if (s->VarActType == 0) {
		s->activity[v] += (int) ((unsigned) s->var_inc * (float) s->factors[v]);
		if (s->activity[v] & 0x80000000)
			act_var_rescale(s);
		if (s->orderpos[v] != -1)
			order_update(s, v);
	} else if (s->VarActType == 1) {
		double act = Abc_Word2Dbl(s->activity[v]) + Abc_Word2Dbl(s->var_inc) * s->factors[v];
		s->activity[v] = Abc_Dbl2Word(act);
		if (act > 1e100)
			act_var_rescale(s);
		if (s->orderpos[v] != -1)
			order_update(s, v);
	} else if (s->VarActType == 2) {
		s->activity[v] = Xdbl_Add(s->activity[v],
		                          Xdbl_Mul(s->var_inc, Xdbl_FromDouble(s->factors[v])));
		if (s->activity[v] > ABC_CONST(0x014c924d692ca61b))
			act_var_rescale(s);
		if (s->orderpos[v] != -1)
			order_update(s, v);
	} else
		assert(0);
}

static inline void act_var_decay(sat_solver* s)
{
	if (s->VarActType == 0)
		s->var_inc += (s->var_inc >> 4);
	else if (s->VarActType == 1)
		s->var_inc = Abc_Dbl2Word(Abc_Word2Dbl(s->var_inc) * Abc_Word2Dbl(s->var_decay));
	else if (s->VarActType == 2)
		s->var_inc = Xdbl_Mul(s->var_inc, s->var_decay);
	else
		assert(0);
}

// clause activities
static inline void act_clause_rescale(sat_solver* s)
{
	if (s->ClaActType == 0) {
		unsigned* activity = (unsigned*) veci_begin(&s->act_clas);
		int i;
		for (i = 0; i < veci_size(&s->act_clas); i++)
			activity[i] >>= 14;
		s->cla_inc >>= 14;
		s->cla_inc = Abc_MaxInt(s->cla_inc, (1 << 10));
	} else {
		float* activity = (float*) veci_begin(&s->act_clas);
		int i;
		for (i = 0; i < veci_size(&s->act_clas); i++)
			activity[i] *= (float) 1e-20;
		s->cla_inc *= (float) 1e-20;
	}
}
static inline void act_clause_bump(sat_solver* s, clause* c)
{
	if (s->ClaActType == 0) {
		unsigned* act = (unsigned*) veci_begin(&s->act_clas) + c->lits[c->size];
		*act += s->cla_inc;
		if (*act & 0x80000000)
			act_clause_rescale(s);
	} else {
		float* act = (float*) veci_begin(&s->act_clas) + c->lits[c->size];
		*act += s->cla_inc;
		if (*act > 1e20)
			act_clause_rescale(s);
	}
}
static inline void act_clause_decay(sat_solver* s)
{
	if (s->ClaActType == 0)
		s->cla_inc += (s->cla_inc >> 10);
	else
		s->cla_inc *= s->cla_decay;
}

//=================================================================================================
// Sorting functions (sigh):

static inline void selectionsort(void** array, int size, int (*comp)(const void*, const void*))
{
	int i, j, best_i;
	void* tmp;

	for (i = 0; i < size - 1; i++) {
		best_i = i;
		for (j = i + 1; j < size; j++) {
			if (comp(array[j], array[best_i]) < 0)
				best_i = j;
		}
		tmp = array[i];
		array[i] = array[best_i];
		array[best_i] = tmp;
	}
}

static inline void sortrnd(void** array, int size, int (*comp)(const void*, const void*), double* seed)
{
	if (size <= 15)
		selectionsort(array, size, comp);

	else {
		void* pivot = array[irand(seed, size)];
		void* tmp;
		int i = -1;
		int j = size;

		for (;;) {
			do
				i++;
			while (comp(array[i], pivot) < 0);
			do
				j--;
			while (comp(pivot, array[j]) < 0);

			if (i >= j)
				break;

			tmp = array[i];
			array[i] = array[j];
			array[j] = tmp;
		}

		sortrnd(array, i, comp, seed);
		sortrnd(&array[i], size - i, comp, seed);
	}
}

//=================================================================================================
// Clause functions:

static inline int sat_clause_compute_lbd(sat_solver* s, clause* c)
{
	int i, lev, minl = 0, lbd = 0;
	for (i = 0; i < (int) c->size; i++) {
		lev = var_level(s, lit_var(c->lits[i]));
		if (!(minl & (1 << (lev & 31)))) {
			minl |= 1 << (lev & 31);
			lbd++;
			//            printf( "%d ", lev );
		}
	}
	//    printf( " -> %d\n", lbd );
	return lbd;
}

/* pre: size > 1 && no variable occurs twice
 */
inline int sat_solver_clause_new(sat_solver* s, lit* begin, lit* end, int learnt)
{
	int fUseBinaryClauses = 1;
	int size;
	clause* c;
	int h;

	assert(end - begin > 1);
	assert(learnt >= 0 && learnt < 2);
	size = end - begin;

	// do not allocate memory for the two-literal problem clause
	if (fUseBinaryClauses && size == 2 && !learnt) {
		veci_push(sat_solver_read_wlist(s, lit_neg(begin[0])), (clause_from_lit(begin[1])));
		veci_push(sat_solver_read_wlist(s, lit_neg(begin[1])), (clause_from_lit(begin[0])));
		s->stats.clauses++;
		s->stats.clauses_literals += size;
		return 0;
	}

	// create new clause
	//    h = Vec_SetAppend( &s->Mem, NULL, size + learnt + 1 + 1 ) << 1;
	h = Sat_MemAppend(&s->Mem, begin, size, learnt, 0);
	assert(!(h & 1));
	if (s->hLearnts == -1 && learnt)
		s->hLearnts = h;
	if (learnt) {
		c = clause_read(s, h);
		c->lbd = sat_clause_compute_lbd(s, c);
		assert(clause_id(c) == veci_size(&s->act_clas));
		//        veci_push(&s->learned, h);
		//        act_clause_bump(s,clause_read(s, h));
		if (s->ClaActType == 0)
			veci_push(&s->act_clas, (1 << 10));
		else
			veci_push(&s->act_clas, s->cla_inc);
		s->stats.learnts++;
		s->stats.learnts_literals += size;
	} else {
		s->stats.clauses++;
		s->stats.clauses_literals += size;
	}

	assert(begin[0] >= 0);
	assert(begin[0] < s->size * 2);
	assert(begin[1] >= 0);
	assert(begin[1] < s->size * 2);

	assert(lit_neg(begin[0]) < s->size * 2);
	assert(lit_neg(begin[1]) < s->size * 2);

	// veci_push(sat_solver_read_wlist(s,lit_neg(begin[0])),c);
	// veci_push(sat_solver_read_wlist(s,lit_neg(begin[1])),c);
	veci_push(sat_solver_read_wlist(s, lit_neg(begin[0])),
	          (size > 2 ? h : clause_from_lit(begin[1])));
	veci_push(sat_solver_read_wlist(s, lit_neg(begin[1])),
	          (size > 2 ? h : clause_from_lit(begin[0])));

	return h;
}

//=================================================================================================
// Minor (solver) functions:

static inline int sat_solver_enqueue(sat_solver* s, lit l, int from)
{
	int v = lit_var(l);
	if (s->pFreqs[v] == 0)
		//    {
		s->pFreqs[v] = 1;
		//        s->nVarUsed++;
		//    }

#ifdef VERBOSEDEBUG
	printf(L_IND "enqueue(" L_LIT ")\n", L_ind, L_lit(l));
#endif
	if (var_value(s, v) != varX)
		return var_value(s, v) == lit_sign(l);
	else {
		/*
		        if ( s->pCnfFunc )
		        {
		            if ( lit_sign(l) )
		            {
		                if ( (s->loads[v] & 1) == 0 )
		                {
		                    s->loads[v] ^= 1;
		                    s->pCnfFunc( s->pCnfMan, l );
		                }
		            }
		            else
		            {
		                if ( (s->loads[v] & 2) == 0 )
		                {
		                    s->loads[v] ^= 2;
		                    s->pCnfFunc( s->pCnfMan, l );
		                }
		            }
		        }
		*/
		// New fact -- store it.
#ifdef VERBOSEDEBUG
		printf(L_IND "bind(" L_LIT ")\n", L_ind, L_lit(l));
#endif
		var_set_value(s, v, lit_sign(l));
		var_set_level(s, v, sat_solver_dl(s));
		s->reasons[v] = from;
		s->trail[s->qtail++] = l;
		order_assigned(s, v);
		return true;
	}
}

static inline int sat_solver_decision(sat_solver* s, lit l)
{
	assert(s->qtail == s->qhead);
	assert(var_value(s, lit_var(l)) == varX);
#ifdef VERBOSEDEBUG
	printf(L_IND "assume(" L_LIT ")  ", L_ind, L_lit(l));
	printf("act = %.20f\n", s->activity[lit_var(l)]);
#endif
	veci_push(&s->trail_lim, s->qtail);
	return sat_solver_enqueue(s, l, 0);
}

static void sat_solver_canceluntil(sat_solver* s, int level)
{
	int bound;
	int lastLev;
	int c;

	if (sat_solver_dl(s) <= level)
		return;

	assert(veci_size(&s->trail_lim) > 0);
	bound = (veci_begin(&s->trail_lim))[level];
	lastLev = (veci_begin(&s->trail_lim))[veci_size(&s->trail_lim) - 1];

	////////////////////////////////////////
	// added to cancel all assignments
	//    if ( level == -1 )
	//        bound = 0;
	////////////////////////////////////////

	for (c = s->qtail - 1; c >= bound; c--) {
		int x = lit_var(s->trail[c]);
		var_set_value(s, x, varX);
		s->reasons[x] = 0;
		if (c < lastLev)
			var_set_polar(s, x, !lit_sign(s->trail[c]));
	}
	// printf( "\n" );

	for (c = s->qhead - 1; c >= bound; c--)
		order_unassigned(s, lit_var(s->trail[c]));

	s->qhead = s->qtail = bound;
	veci_resize(&s->trail_lim, level);
}

static void sat_solver_canceluntil_rollback(sat_solver* s, int NewBound)
{
	int c, x;

	assert(sat_solver_dl(s) == 0);
	assert(s->qtail == s->qhead);
	assert(s->qtail >= NewBound);

	for (c = s->qtail - 1; c >= NewBound; c--) {
		x = lit_var(s->trail[c]);
		var_set_value(s, x, varX);
		s->reasons[x] = 0;
	}

	for (c = s->qhead - 1; c >= NewBound; c--)
		order_unassigned(s, lit_var(s->trail[c]));

	s->qhead = s->qtail = NewBound;
}

static void sat_solver_record(sat_solver* s, veci* cls)
{
	lit* begin = veci_begin(cls);
	lit* end = begin + veci_size(cls);
	int h = (veci_size(cls) > 1) ? sat_solver_clause_new(s, begin, end, 1) : 0;
	sat_solver_enqueue(s, *begin, h);
	assert(veci_size(cls) > 0);
	if (h == 0)
		veci_push(&s->unit_lits, *begin);

	///////////////////////////////////
	// add clause to internal storage
	if (s->pStore) {
		int RetValue = Sto_ManAddClause((Sto_Man_t*) s->pStore, begin, end);
		assert(RetValue);
		(void) RetValue;
	}
	///////////////////////////////////
	/*
	    if (h != 0) {
	        act_clause_bump(s,clause_read(s, h));
	        s->stats.learnts++;
	        s->stats.learnts_literals += veci_size(cls);
	    }
	*/
}

inline int sat_solver_count_assigned(sat_solver* s)
{
	// count top-level assignments
	int i, Count = 0;
	assert(sat_solver_dl(s) == 0);
	for (i = 0; i < s->size; i++)
		if (var_value(s, i) != varX)
			Count++;
	return Count;
}

static double sat_solver_progress(sat_solver* s)
{
	int i;
	double progress = 0;
	double F = 1.0 / s->size;
	for (i = 0; i < s->size; i++)
		if (var_value(s, i) != varX)
			progress += pow(F, var_level(s, i));
	return progress / s->size;
}

//=================================================================================================
// Major methods:

static int sat_solver_lit_removable(sat_solver* s, int x, int minl)
{
	int top = veci_size(&s->tagged);

	assert(s->reasons[x] != 0);
	veci_resize(&s->stack, 0);
	veci_push(&s->stack, x);

	while (veci_size(&s->stack)) {
		int v = veci_pop(&s->stack);
		assert(s->reasons[v] != 0);
		if (clause_is_lit(s->reasons[v])) {
			v = lit_var(clause_read_lit(s->reasons[v]));
			if (!var_tag(s, v) && var_level(s, v)) {
				if (s->reasons[v] != 0 && ((1 << (var_level(s, v) & 31)) & minl)) {
					veci_push(&s->stack, v);
					var_set_tag(s, v, 1);
				} else {
					solver2_clear_tags(s, top);
					return 0;
				}
			}
		} else {
			clause* c = clause_read(s, s->reasons[v]);
			lit* lits = clause_begin(c);
			int i;
			for (i = 1; i < clause_size(c); i++) {
				int v = lit_var(lits[i]);
				if (!var_tag(s, v) && var_level(s, v)) {
					if (s->reasons[v] != 0
					    && ((1 << (var_level(s, v) & 31)) & minl)) {
						veci_push(&s->stack, lit_var(lits[i]));
						var_set_tag(s, v, 1);
					} else {
						solver2_clear_tags(s, top);
						return 0;
					}
				}
			}
		}
	}
	return 1;
}

/*_________________________________________________________________________________________________
|
|  analyzeFinal : (p : Lit)  ->  [void]
|
|  Description:
|    Specialized analysis procedure to express the final conflict in terms of assumptions.
|    Calculates the (possibly empty) set of assumptions that led to the assignment of 'p', and
|    stores the result in 'out_conflict'.
|________________________________________________________________________________________________@*/
/*
void Solver::analyzeFinal(Clause* confl, bool skip_first)
{
    // -- NOTE! This code is relatively untested. Please report bugs!
    conflict.clear();
    if (root_level == 0) return;

    vec<char>& seen  = analyze_seen;
    for (int i = skip_first ? 1 : 0; i < confl->size(); i++){
        Var x = var((*confl)[i]);
        if (level[x] > 0)
            seen[x] = 1;
    }

    int start = (root_level >= trail_lim.size()) ? trail.size()-1 : trail_lim[root_level];
    for (int i = start; i >= trail_lim[0]; i--){
        Var     x = var(trail[i]);
        if (seen[x]){
            GClause r = reason[x];
            if (r == GClause_NULL){
                assert(level[x] > 0);
                conflict.push(~trail[i]);
            }else{
                if (r.isLit()){
                    Lit p = r.lit();
                    if (level[var(p)] > 0)
                        seen[var(p)] = 1;
                }else{
                    Clause& c = *r.clause();
                    for (int j = 1; j < c.size(); j++)
                        if (level[var(c[j])] > 0)
                            seen[var(c[j])] = 1;
                }
            }
            seen[x] = 0;
        }
    }
}
*/

#ifdef SAT_USE_ANALYZE_FINAL

static void sat_solver_analyze_final(sat_solver* s, int hConf, int skip_first)
{
	clause* conf = clause_read(s, hConf);
	int i, j, start;
	veci_resize(&s->conf_final, 0);
	if (s->root_level == 0)
		return;
	assert(veci_size(&s->tagged) == 0);
	//    assert( s->tags[lit_var(p)] == l_Undef );
	//    s->tags[lit_var(p)] = l_True;
	for (i = skip_first ? 1 : 0; i < clause_size(conf); i++) {
		int x = lit_var(clause_begin(conf)[i]);
		if (var_level(s, x) > 0)
			var_set_tag(s, x, 1);
	}

	start = (s->root_level >= veci_size(&s->trail_lim)) ?
	            s->qtail - 1 :
	            (veci_begin(&s->trail_lim))[s->root_level];
	for (i = start; i >= (veci_begin(&s->trail_lim))[0]; i--) {
		int x = lit_var(s->trail[i]);
		if (var_tag(s, x)) {
			if (s->reasons[x] == 0) {
				assert(var_level(s, x) > 0);
				veci_push(&s->conf_final, lit_neg(s->trail[i]));
			} else {
				if (clause_is_lit(s->reasons[x])) {
					lit q = clause_read_lit(s->reasons[x]);
					assert(lit_var(q) >= 0 && lit_var(q) < s->size);
					if (var_level(s, lit_var(q)) > 0)
						var_set_tag(s, lit_var(q), 1);
				} else {
					clause* c = clause_read(s, s->reasons[x]);
					int* lits = clause_begin(c);
					for (j = 1; j < clause_size(c); j++)
						if (var_level(s, lit_var(lits[j])) > 0)
							var_set_tag(s, lit_var(lits[j]), 1);
				}
			}
		}
	}
	solver2_clear_tags(s, 0);
}

#endif

static void sat_solver_analyze(sat_solver* s, int h, veci* learnt)
{
	lit* trail = s->trail;
	int cnt = 0;
	lit p = lit_Undef;
	int ind = s->qtail - 1;
	lit* lits;
	int i, j, minl;
	veci_push(learnt, lit_Undef);
	do {
		assert(h != 0);
		if (clause_is_lit(h)) {
			int x = lit_var(clause_read_lit(h));
			if (var_tag(s, x) == 0 && var_level(s, x) > 0) {
				var_set_tag(s, x, 1);
				act_var_bump(s, x);
				if (var_level(s, x) == sat_solver_dl(s))
					cnt++;
				else
					veci_push(learnt, clause_read_lit(h));
			}
		} else {
			clause* c = clause_read(s, h);

			if (clause_learnt(c))
				act_clause_bump(s, c);
			lits = clause_begin(c);
			// printlits(lits,lits+clause_size(c)); printf("\n");
			for (j = (p == lit_Undef ? 0 : 1); j < clause_size(c); j++) {
				int x = lit_var(lits[j]);
				if (var_tag(s, x) == 0 && var_level(s, x) > 0) {
					var_set_tag(s, x, 1);
					act_var_bump(s, x);
					// bump variables propaged by the LBD=2 clause
					//                    if ( s->reasons[x] && clause_read(s,
					//                    s->reasons[x])->lbd <= 2 )
					//                        act_var_bump(s,x);
					if (var_level(s, x) == sat_solver_dl(s))
						cnt++;
					else
						veci_push(learnt, lits[j]);
				}
			}
		}

		while (!var_tag(s, lit_var(trail[ind--])))
			;

		p = trail[ind + 1];
		h = s->reasons[lit_var(p)];
		cnt--;

	} while (cnt > 0);

	*veci_begin(learnt) = lit_neg(p);

	lits = veci_begin(learnt);
	minl = 0;
	for (i = 1; i < veci_size(learnt); i++) {
		int lev = var_level(s, lit_var(lits[i]));
		minl |= 1 << (lev & 31);
	}

	// simplify (full)
	for (i = j = 1; i < veci_size(learnt); i++) {
		if (s->reasons[lit_var(lits[i])] == 0
		    || !sat_solver_lit_removable(s, lit_var(lits[i]), minl))
			lits[j++] = lits[i];
	}

	// update size of learnt + statistics
	veci_resize(learnt, j);
	s->stats.tot_literals += j;

	// clear tags
	solver2_clear_tags(s, 0);

#ifdef DEBUG
	for (i = 0; i < s->size; i++)
		assert(!var_tag(s, i));
#endif

#ifdef VERBOSEDEBUG
	printf(L_IND "Learnt {", L_ind);
	for (i = 0; i < veci_size(learnt); i++)
		printf(" " L_LIT, L_lit(lits[i]));
#endif
	if (veci_size(learnt) > 1) {
		int max_i = 1;
		int max = var_level(s, lit_var(lits[1]));
		lit tmp;

		for (i = 2; i < veci_size(learnt); i++)
			if (var_level(s, lit_var(lits[i])) > max) {
				max = var_level(s, lit_var(lits[i]));
				max_i = i;
			}

		tmp = lits[1];
		lits[1] = lits[max_i];
		lits[max_i] = tmp;
	}
#ifdef VERBOSEDEBUG
	{
		int lev = veci_size(learnt) > 1 ? var_level(s, lit_var(lits[1])) : 0;
		printf(" } at level %d\n", lev);
	}
#endif
}

//#define TEST_CNF_LOAD

inline int sat_solver_propagate(sat_solver* s)
{
	int hConfl = 0;
	lit* lits;
	lit false_lit;

	// printf("sat_solver_propagate\n");
	while (hConfl == 0 && s->qtail - s->qhead > 0) {
		lit p = s->trail[s->qhead++];

#ifdef TEST_CNF_LOAD
		int v = lit_var(p);
		if (s->pCnfFunc) {
			if (lit_sign(p)) {
				if ((s->loads[v] & 1) == 0) {
					s->loads[v] ^= 1;
					s->pCnfFunc(s->pCnfMan, p);
				}
			} else {
				if ((s->loads[v] & 2) == 0) {
					s->loads[v] ^= 2;
					s->pCnfFunc(s->pCnfMan, p);
				}
			}
		}
		{
#endif

			veci* ws = sat_solver_read_wlist(s, p);
			int* begin = veci_begin(ws);
			int* end = begin + veci_size(ws);
			int *i, *j;

			s->stats.propagations++;
			//        s->simpdb_props--;

			// printf("checking lit %d: "L_LIT"\n", veci_size(ws), L_lit(p));
			for (i = j = begin; i < end;) {
				if (clause_is_lit(*i)) {

					int Lit = clause_read_lit(*i);
					if (var_value(s, lit_var(Lit)) == lit_sign(Lit)) {
						*j++ = *i++;
						continue;
					}

					*j++ = *i;
					if (!sat_solver_enqueue(s, clause_read_lit(*i),
					                        clause_from_lit(p))) {
						hConfl = s->hBinary;
						(clause_begin(s->binary))[1] = lit_neg(p);
						(clause_begin(s->binary))[0] = clause_read_lit(*i++);
						// Copy the remaining watches:
						while (i < end)
							*j++ = *i++;
					}
				} else {

					clause* c = clause_read(s, *i);
					lits = clause_begin(c);

					// Make sure the false literal is data[1]:
					false_lit = lit_neg(p);
					if (lits[0] == false_lit) {
						lits[0] = lits[1];
						lits[1] = false_lit;
					}
					assert(lits[1] == false_lit);

					// If 0th watch is true, then clause is already satisfied.
					if (var_value(s, lit_var(lits[0])) == lit_sign(lits[0]))
						*j++ = *i;
					else {
						// Look for new watch:
						lit* stop = lits + clause_size(c);
						lit* k;
						for (k = lits + 2; k < stop; k++) {
							if (var_value(s, lit_var(*k))
							    != !lit_sign(*k)) {
								lits[1] = *k;
								*k = false_lit;
								veci_push(sat_solver_read_wlist(
								              s, lit_neg(lits[1])),
								          *i);
								goto next;
							}
						}

						*j++ = *i;
						// Clause is unit under assignment:
						if (c->lrn)
							c->lbd = sat_clause_compute_lbd(s, c);
						if (!sat_solver_enqueue(s, lits[0], *i)) {
							hConfl = *i++;
							// Copy the remaining watches:
							while (i < end)
								*j++ = *i++;
						}
					}
				}
			next:
				i++;
			}

			s->stats.inspects += j - veci_begin(ws);
			veci_resize(ws, j - veci_begin(ws));
#ifdef TEST_CNF_LOAD
		}
#endif
	}

	return hConfl;
}

//=================================================================================================
// External solver functions:

inline sat_solver* sat_solver_new(void)
{
	sat_solver* s = (sat_solver*) ABC_CALLOC(char, sizeof(sat_solver));

	//    Vec_SetAlloc_(&s->Mem, 15);
	Sat_MemAlloc_(&s->Mem, 17);
	s->hLearnts = -1;
	s->hBinary = Sat_MemAppend(&s->Mem, NULL, 2, 0, 0);
	s->binary = clause_read(s, s->hBinary);

	s->nLearntStart = LEARNT_MAX_START_DEFAULT; // starting learned clause limit
	s->nLearntDelta = LEARNT_MAX_INCRE_DEFAULT; // delta of learned clause limit
	s->nLearntRatio = LEARNT_MAX_RATIO_DEFAULT; // ratio of learned clause limit
	s->nLearntMax = s->nLearntStart;

	// initialize vectors
	veci_new(&s->order);
	veci_new(&s->trail_lim);
	veci_new(&s->tagged);
	//    veci_new(&s->learned);
	veci_new(&s->act_clas);
	veci_new(&s->stack);
	//    veci_new(&s->model);
	veci_new(&s->unit_lits);
	veci_new(&s->temp_clause);
	veci_new(&s->conf_final);

	// initialize arrays
	s->wlists = 0;
	s->activity = 0;
	s->orderpos = 0;
	s->reasons = 0;
	s->trail = 0;

	// initialize other vars
	s->size = 0;
	s->cap = 0;
	s->qhead = 0;
	s->qtail = 0;

	solver_init_activities(s);
	veci_new(&s->act_vars);

	s->root_level = 0;
	//    s->simpdb_assigns         = 0;
	//    s->simpdb_props           = 0;
	s->progress_estimate = 0;
	//    s->binary                 = (clause*)ABC_ALLOC( char, sizeof(clause) + sizeof(lit)*2);
	//    s->binary->size_learnt    = (2 << 1);
	s->verbosity = 0;

	s->stats.starts = 0;
	s->stats.decisions = 0;
	s->stats.propagations = 0;
	s->stats.inspects = 0;
	s->stats.conflicts = 0;
	s->stats.clauses = 0;
	s->stats.clauses_literals = 0;
	s->stats.learnts = 0;
	s->stats.learnts_literals = 0;
	s->stats.tot_literals = 0;
	return s;
}

inline sat_solver* zsat_solver_new_seed(double seed)
{
	sat_solver* s = (sat_solver*) ABC_CALLOC(char, sizeof(sat_solver));

	//    Vec_SetAlloc_(&s->Mem, 15);
	Sat_MemAlloc_(&s->Mem, 15);
	s->hLearnts = -1;
	s->hBinary = Sat_MemAppend(&s->Mem, NULL, 2, 0, 0);
	s->binary = clause_read(s, s->hBinary);

	s->nLearntStart = LEARNT_MAX_START_DEFAULT; // starting learned clause limit
	s->nLearntDelta = LEARNT_MAX_INCRE_DEFAULT; // delta of learned clause limit
	s->nLearntRatio = LEARNT_MAX_RATIO_DEFAULT; // ratio of learned clause limit
	s->nLearntMax = s->nLearntStart;

	// initialize vectors
	veci_new(&s->order);
	veci_new(&s->trail_lim);
	veci_new(&s->tagged);
	//    veci_new(&s->learned);
	veci_new(&s->act_clas);
	veci_new(&s->stack);
	//    veci_new(&s->model);
	veci_new(&s->unit_lits);
	veci_new(&s->temp_clause);
	veci_new(&s->conf_final);

	// initialize arrays
	s->wlists = 0;
	s->activity = 0;
	s->orderpos = 0;
	s->reasons = 0;
	s->trail = 0;

	// initialize other vars
	s->size = 0;
	s->cap = 0;
	s->qhead = 0;
	s->qtail = 0;

	solver_init_activities(s);
	veci_new(&s->act_vars);

	s->root_level = 0;
	//    s->simpdb_assigns         = 0;
	//    s->simpdb_props           = 0;
	s->random_seed = seed;
	s->progress_estimate = 0;
	//    s->binary                 = (clause*)ABC_ALLOC( char, sizeof(clause) + sizeof(lit)*2);
	//    s->binary->size_learnt    = (2 << 1);
	s->verbosity = 0;

	s->stats.starts = 0;
	s->stats.decisions = 0;
	s->stats.propagations = 0;
	s->stats.inspects = 0;
	s->stats.conflicts = 0;
	s->stats.clauses = 0;
	s->stats.clauses_literals = 0;
	s->stats.learnts = 0;
	s->stats.learnts_literals = 0;
	s->stats.tot_literals = 0;
	return s;
}

inline int sat_solver_addvar(sat_solver* s)
{
	sat_solver_setnvars(s, s->size + 1);
	return s->size - 1;
}
inline void sat_solver_setnvars(sat_solver* s, int n)
{
	int var;

	if (s->cap < n) {
		int old_cap = s->cap;
		while (s->cap < n)
			s->cap = s->cap * 2 + 1;
		if (s->cap < 50000)
			s->cap = 50000;

		s->wlists = ABC_REALLOC(veci, s->wlists, s->cap * 2);
		//        s->vi        = ABC_REALLOC(varinfo,s->vi,       s->cap);
		s->levels = ABC_REALLOC(int, s->levels, s->cap);
		s->assigns = ABC_REALLOC(char, s->assigns, s->cap);
		s->polarity = ABC_REALLOC(char, s->polarity, s->cap);
		s->tags = ABC_REALLOC(char, s->tags, s->cap);
		s->loads = ABC_REALLOC(char, s->loads, s->cap);
		s->activity = ABC_REALLOC(word, s->activity, s->cap);
		s->activity2 = ABC_REALLOC(word, s->activity2, s->cap);
		s->pFreqs = ABC_REALLOC(char, s->pFreqs, s->cap);

		if (s->factors)
			s->factors = ABC_REALLOC(double, s->factors, s->cap);
		s->orderpos = ABC_REALLOC(int, s->orderpos, s->cap);
		s->reasons = ABC_REALLOC(int, s->reasons, s->cap);
		s->trail = ABC_REALLOC(lit, s->trail, s->cap);
		s->model = ABC_REALLOC(int, s->model, s->cap);
		memset(s->wlists + 2 * old_cap, 0, 2 * (s->cap - old_cap) * sizeof(veci));
	}

	for (var = s->size; var < n; var++) {
		assert(!s->wlists[2 * var].size);
		assert(!s->wlists[2 * var + 1].size);
		if (s->wlists[2 * var].ptr == NULL)
			veci_new(&s->wlists[2 * var]);
		if (s->wlists[2 * var + 1].ptr == NULL)
			veci_new(&s->wlists[2 * var + 1]);

		if (s->VarActType == 0)
			s->activity[var] = (1 << 10);
		else if (s->VarActType == 1)
			s->activity[var] = 0;
		else if (s->VarActType == 2)
			s->activity[var] = 0;
		else
			assert(0);

		s->pFreqs[var] = 0;
		if (s->factors)
			s->factors[var] = 0;
		//        *((int*)s->vi + var) = 0; s->vi[var].val = varX;
		s->levels[var] = 0;
		s->assigns[var] = varX;
		s->polarity[var] = 0;
		s->tags[var] = 0;
		s->loads[var] = 0;
		s->orderpos[var] = veci_size(&s->order);
		s->reasons[var] = 0;
		s->model[var] = 0;

		/* does not hold because variables enqueued at top level will not be reinserted in
		   the heap assert(veci_size(&s->order) == var);
		 */
		veci_push(&s->order, var);
		order_update(s, var);
	}

	s->size = n > s->size ? n : s->size;
}

inline void sat_solver_delete(sat_solver* s)
{
	//    Vec_SetFree_( &s->Mem );
	Sat_MemFree_(&s->Mem);

	// delete vectors
	veci_delete(&s->order);
	veci_delete(&s->trail_lim);
	veci_delete(&s->tagged);
	//    veci_delete(&s->learned);
	veci_delete(&s->act_clas);
	veci_delete(&s->stack);
	//    veci_delete(&s->model);
	veci_delete(&s->act_vars);
	veci_delete(&s->unit_lits);
	veci_delete(&s->pivot_vars);
	veci_delete(&s->temp_clause);
	veci_delete(&s->conf_final);

	veci_delete(&s->user_vars);
	veci_delete(&s->user_values);

	// delete arrays
	if (s->reasons != 0) {
		int i;
		for (i = 0; i < s->cap * 2; i++)
			veci_delete(&s->wlists[i]);
		ABC_FREE(s->wlists);
		//        ABC_FREE(s->vi       );
		ABC_FREE(s->levels);
		ABC_FREE(s->assigns);
		ABC_FREE(s->polarity);
		ABC_FREE(s->tags);
		ABC_FREE(s->loads);
		ABC_FREE(s->activity);
		ABC_FREE(s->activity2);
		ABC_FREE(s->pFreqs);
		ABC_FREE(s->factors);
		ABC_FREE(s->orderpos);
		ABC_FREE(s->reasons);
		ABC_FREE(s->trail);
		ABC_FREE(s->model);
	}

	sat_solver_store_free(s);
	ABC_FREE(s);
}

inline void sat_solver_restart(sat_solver* s)
{
	int i;
	Sat_MemRestart(&s->Mem);
	s->hLearnts = -1;
	s->hBinary = Sat_MemAppend(&s->Mem, NULL, 2, 0, 0);
	s->binary = clause_read(s, s->hBinary);

	veci_resize(&s->trail_lim, 0);
	veci_resize(&s->order, 0);
	for (i = 0; i < s->size * 2; i++)
		s->wlists[i].size = 0;

	s->nDBreduces = 0;

	// initialize other vars
	s->size = 0;
	//    s->cap                    = 0;
	s->qhead = 0;
	s->qtail = 0;

	// variable activities
	solver_init_activities(s);
	veci_resize(&s->act_clas, 0);

	s->root_level = 0;
	//    s->simpdb_assigns         = 0;
	//    s->simpdb_props           = 0;
	s->progress_estimate = 0;
	s->verbosity = 0;

	s->stats.starts = 0;
	s->stats.decisions = 0;
	s->stats.propagations = 0;
	s->stats.inspects = 0;
	s->stats.conflicts = 0;
	s->stats.clauses = 0;
	s->stats.clauses_literals = 0;
	s->stats.learnts = 0;
	s->stats.learnts_literals = 0;
	s->stats.tot_literals = 0;
}

inline void zsat_solver_restart_seed(sat_solver* s, double seed)
{
	int i;
	Sat_MemRestart(&s->Mem);
	s->hLearnts = -1;
	s->hBinary = Sat_MemAppend(&s->Mem, NULL, 2, 0, 0);
	s->binary = clause_read(s, s->hBinary);

	veci_resize(&s->trail_lim, 0);
	veci_resize(&s->order, 0);
	for (i = 0; i < s->size * 2; i++)
		s->wlists[i].size = 0;

	s->nDBreduces = 0;

	// initialize other vars
	s->size = 0;
	//    s->cap                    = 0;
	s->qhead = 0;
	s->qtail = 0;

	solver_init_activities(s);
	veci_resize(&s->act_clas, 0);

	s->root_level = 0;
	//    s->simpdb_assigns         = 0;
	//    s->simpdb_props           = 0;
	s->random_seed = seed;
	s->progress_estimate = 0;
	s->verbosity = 0;

	s->stats.starts = 0;
	s->stats.decisions = 0;
	s->stats.propagations = 0;
	s->stats.inspects = 0;
	s->stats.conflicts = 0;
	s->stats.clauses = 0;
	s->stats.clauses_literals = 0;
	s->stats.learnts = 0;
	s->stats.learnts_literals = 0;
	s->stats.tot_literals = 0;
}

// returns memory in bytes used by the SAT solver
inline double sat_solver_memory(sat_solver* s)
{
	int i;
	double Mem = sizeof(sat_solver);
	for (i = 0; i < s->cap * 2; i++)
		Mem += s->wlists[i].cap * sizeof(int);
	Mem += s->cap * sizeof(veci); // ABC_FREE(s->wlists   );
	Mem += s->cap * sizeof(int);  // ABC_FREE(s->levels   );
	Mem += s->cap * sizeof(char); // ABC_FREE(s->assigns  );
	Mem += s->cap * sizeof(char); // ABC_FREE(s->polarity );
	Mem += s->cap * sizeof(char); // ABC_FREE(s->tags     );
	Mem += s->cap * sizeof(char); // ABC_FREE(s->loads    );
	Mem += s->cap * sizeof(word); // ABC_FREE(s->activity );
	if (s->activity2)
		Mem += s->cap * sizeof(word); // ABC_FREE(s->activity );
	if (s->factors)
		Mem += s->cap * sizeof(double); // ABC_FREE(s->factors  );
	Mem += s->cap * sizeof(int);            // ABC_FREE(s->orderpos );
	Mem += s->cap * sizeof(int);            // ABC_FREE(s->reasons  );
	Mem += s->cap * sizeof(lit);            // ABC_FREE(s->trail    );
	Mem += s->cap * sizeof(int);            // ABC_FREE(s->model    );

	Mem += s->order.cap * sizeof(int);
	Mem += s->trail_lim.cap * sizeof(int);
	Mem += s->tagged.cap * sizeof(int);
	//    Mem += s->learned.cap * sizeof(int);
	Mem += s->stack.cap * sizeof(int);
	Mem += s->act_vars.cap * sizeof(int);
	Mem += s->unit_lits.cap * sizeof(int);
	Mem += s->act_clas.cap * sizeof(int);
	Mem += s->temp_clause.cap * sizeof(int);
	Mem += s->conf_final.cap * sizeof(int);
	Mem += Sat_MemMemoryAll(&s->Mem);
	return Mem;
}

inline int sat_solver_simplify(sat_solver* s)
{
	assert(sat_solver_dl(s) == 0);
	if (sat_solver_propagate(s) != 0)
		return false;
	return true;
}

inline void sat_solver_reducedb(sat_solver* s)
{
	static abctime TimeTotal = 0;
	abctime clk = Abc_Clock();
	Sat_Mem_t* pMem = &s->Mem;
	int nLearnedOld = veci_size(&s->act_clas);
	int* act_clas = veci_begin(&s->act_clas);
	int *pPerm, *pArray, *pSortValues, nCutoffValue;
	int i, k, j, Id, Counter, CounterStart, nSelected;
	clause* c;

	assert(s->nLearntMax > 0);
	assert(nLearnedOld == Sat_MemEntryNum(pMem, 1));
	assert(nLearnedOld == (int) s->stats.learnts);

	s->nDBreduces++;

	// printf( "Calling reduceDB with %d learned clause limit.\n", s->nLearntMax );
	s->nLearntMax = s->nLearntStart + s->nLearntDelta * s->nDBreduces;
	//    return;

	// create sorting values
	pSortValues = ABC_ALLOC(int, nLearnedOld);
	Sat_MemForEachLearned(pMem, c, i, k)
	{
		Id = clause_id(c);
		//        pSortValues[Id] = act[Id];
		if (s->ClaActType == 0)
			pSortValues[Id] = ((7 - Abc_MinInt(c->lbd, 7)) << 28) | (act_clas[Id] >> 4);
		else
			pSortValues[Id] = ((7 - Abc_MinInt(c->lbd, 7))
			                   << 28); // | (act_clas[Id] >> 4);
		assert(pSortValues[Id] >= 0);
	}

	// preserve 1/20 of last clauses
	CounterStart = nLearnedOld - (s->nLearntMax / 20);

	// preserve 3/4 of most active clauses
	nSelected = nLearnedOld * s->nLearntRatio / 100;

	// find non-decreasing permutation
	pPerm = Abc_MergeSortCost(pSortValues, nLearnedOld);
	assert(pSortValues[pPerm[0]] <= pSortValues[pPerm[nLearnedOld - 1]]);
	nCutoffValue = pSortValues[pPerm[nLearnedOld - nSelected]];
	ABC_FREE(pPerm);
	//    ActCutOff = ABC_INFINITY;

	// mark learned clauses to remove
	Counter = j = 0;
	Sat_MemForEachLearned(pMem, c, i, k)
	{
		assert(c->mark == 0);
		if (Counter++ > CounterStart || clause_size(c) < 3
		    || pSortValues[clause_id(c)] > nCutoffValue
		    || s->reasons[lit_var(c->lits[0])] == Sat_MemHand(pMem, i, k))
			act_clas[j++] = act_clas[clause_id(c)];
		else // delete
		{
			c->mark = 1;
			s->stats.learnts_literals -= clause_size(c);
			s->stats.learnts--;
		}
	}
	assert(s->stats.learnts == (unsigned) j);
	assert(Counter == nLearnedOld);
	veci_resize(&s->act_clas, j);
	ABC_FREE(pSortValues);

	// update ID of each clause to be its new handle
	Counter = Sat_MemCompactLearned(pMem, 0);
	assert(Counter == (int) s->stats.learnts);

	// update reasons
	for (i = 0; i < s->size; i++) {
		if (!s->reasons[i]) // no reason
			continue;
		if (clause_is_lit(s->reasons[i])) // 2-lit clause
			continue;
		if (!clause_learnt_h(pMem, s->reasons[i])) // problem clause
			continue;
		c = clause_read(s, s->reasons[i]);
		assert(c->mark == 0);
		s->reasons[i] = clause_id(c); // updating handle here!!!
	}

	// update watches
	for (i = 0; i < s->size * 2; i++) {
		pArray = veci_begin(&s->wlists[i]);
		for (j = k = 0; k < veci_size(&s->wlists[i]); k++) {
			if (clause_is_lit(pArray[k])) // 2-lit clause
				pArray[j++] = pArray[k];
			else if (!clause_learnt_h(pMem, pArray[k])) // problem clause
				pArray[j++] = pArray[k];
			else {
				c = clause_read(s, pArray[k]);
				if (!c->mark)                       // useful learned clause
					pArray[j++] = clause_id(c); // updating handle here!!!
			}
		}
		veci_resize(&s->wlists[i], j);
	}

	// perform final move of the clauses
	Counter = Sat_MemCompactLearned(pMem, 1);
	assert(Counter == (int) s->stats.learnts);

	// report the results
	TimeTotal += Abc_Clock() - clk;
}

// reverses to the previously bookmarked point
inline void sat_solver_rollback(sat_solver* s)
{
	Sat_Mem_t* pMem = &s->Mem;
	int i, k, j;
	static int Count = 0;
	Count++;
	assert(s->iVarPivot >= 0 && s->iVarPivot <= s->size);
	assert(s->iTrailPivot >= 0 && s->iTrailPivot <= s->qtail);
	// reset implication queue
	sat_solver_canceluntil_rollback(s, s->iTrailPivot);
	// update order
	if (s->iVarPivot < s->size) {
		if (s->activity2) {
			s->var_inc = s->var_inc2;
			memcpy(s->activity, s->activity2, sizeof(word) * s->iVarPivot);
		}
		veci_resize(&s->order, 0);
		for (i = 0; i < s->iVarPivot; i++) {
			if (var_value(s, i) != varX)
				continue;
			s->orderpos[i] = veci_size(&s->order);
			veci_push(&s->order, i);
			order_update(s, i);
		}
	}
	// compact watches
	for (i = 0; i < s->iVarPivot * 2; i++) {
		cla* pArray = veci_begin(&s->wlists[i]);
		for (j = k = 0; k < veci_size(&s->wlists[i]); k++) {
			if (clause_is_lit(pArray[k])) {
				if (clause_read_lit(pArray[k]) < s->iVarPivot * 2)
					pArray[j++] = pArray[k];
			} else if (Sat_MemClauseUsed(pMem, pArray[k]))
				pArray[j++] = pArray[k];
		}
		veci_resize(&s->wlists[i], j);
	}
	// reset watcher lists
	for (i = 2 * s->iVarPivot; i < 2 * s->size; i++)
		s->wlists[i].size = 0;

	// reset clause counts
	s->stats.clauses = pMem->BookMarkE[0];
	s->stats.learnts = pMem->BookMarkE[1];
	// rollback clauses
	Sat_MemRollBack(pMem);

	// resize learned arrays
	veci_resize(&s->act_clas, s->stats.learnts);

	// initialize other vars
	s->size = s->iVarPivot;
	if (s->size == 0) {
		//    s->size                   = 0;
		//    s->cap                    = 0;
		s->qhead = 0;
		s->qtail = 0;

		solver_init_activities(s);

		s->root_level = 0;
		s->progress_estimate = 0;
		s->verbosity = 0;

		s->stats.starts = 0;
		s->stats.decisions = 0;
		s->stats.propagations = 0;
		s->stats.inspects = 0;
		s->stats.conflicts = 0;
		s->stats.clauses = 0;
		s->stats.clauses_literals = 0;
		s->stats.learnts = 0;
		s->stats.learnts_literals = 0;
		s->stats.tot_literals = 0;

		// initialize rollback
		s->iVarPivot = 0;   // the pivot for variables
		s->iTrailPivot = 0; // the pivot for trail
		s->hProofPivot = 1; // the pivot for proof records
	}
}

inline int sat_solver_addclause(sat_solver* s, lit* begin, lit* end)
{
	lit *i, *j;
	int maxvar;
	lit last;
	assert(begin < end);
	if (s->fPrintClause) {
		for (i = begin; i < end; i++)
			printf("%s%d ", (*i) & 1 ? "!" : "", (*i) >> 1);
		printf("\n");
	}

	veci_resize(&s->temp_clause, 0);
	for (i = begin; i < end; i++)
		veci_push(&s->temp_clause, *i);
	begin = veci_begin(&s->temp_clause);
	end = begin + veci_size(&s->temp_clause);

	// insertion sort
	maxvar = lit_var(*begin);
	for (i = begin + 1; i < end; i++) {
		lit l = *i;
		maxvar = lit_var(l) > maxvar ? lit_var(l) : maxvar;
		for (j = i; j > begin && *(j - 1) > l; j--)
			*j = *(j - 1);
		*j = l;
	}
	sat_solver_setnvars(s, maxvar + 1);

	///////////////////////////////////
	// add clause to internal storage
	if (s->pStore) {
		int RetValue = Sto_ManAddClause((Sto_Man_t*) s->pStore, begin, end);
		assert(RetValue);
		(void) RetValue;
	}
	///////////////////////////////////

	// delete duplicates
	last = lit_Undef;
	for (i = j = begin; i < end; i++) {
		// printf("lit: "L_LIT", value = %d\n", L_lit(*i), (lit_sign(*i) ?
		// -s->assignss[lit_var(*i)] : s->assignss[lit_var(*i)]));
		if (*i == lit_neg(last) || var_value(s, lit_var(*i)) == lit_sign(*i))
			return true; // tautology
		else if (*i != last && var_value(s, lit_var(*i)) == varX)
			last = *j++ = *i;
	}
	//    j = i;

	if (j == begin) // empty clause
		return false;

	if (j - begin == 1) // unit clause
		return sat_solver_enqueue(s, *begin, 0);

	// create new clause
	sat_solver_clause_new(s, begin, j, 0);
	return true;
}

inline double luby(double y, int x)
{
	int size, seq;
	for (size = 1, seq = 0; size < x + 1; seq++, size = 2 * size + 1)
		;
	while (size - 1 != x) {
		size = (size - 1) >> 1;
		seq--;
		x = x % size;
	}
	return pow(y, (double) seq);
}

inline void luby_test()
{
	int i;
	for (i = 0; i < 20; i++)
		printf("%d ", (int) luby(2, i));
	printf("\n");
}

static lbool sat_solver_search(sat_solver* s, ABC_INT64_T nof_conflicts)
{
	//    double  var_decay       = 0.95;
	//    double  clause_decay    = 0.999;
	double random_var_freq = s->fNotUseRandom ? 0.0 : 0.02;
	ABC_INT64_T conflictC = 0;
	veci learnt_clause;
	int i;

	assert(s->root_level == sat_solver_dl(s));

	s->nRestarts++;
	s->stats.starts++;
	//    s->var_decay = (float)(1 / var_decay   );  // move this to sat_solver_new()
	//    s->cla_decay = (float)(1 / clause_decay);  // move this to sat_solver_new()
	//    veci_resize(&s->model,0);
	veci_new(&learnt_clause);

	// use activity factors in every even restart
	if ((s->nRestarts & 1) && veci_size(&s->act_vars) > 0)
		//    if ( veci_size(&s->act_vars) > 0 )
		for (i = 0; i < s->act_vars.size; i++)
			act_var_bump_factor(s, s->act_vars.ptr[i]);

	// use activity factors in every restart
	if (s->pGlobalVars && veci_size(&s->act_vars) > 0)
		for (i = 0; i < s->act_vars.size; i++)
			act_var_bump_global(s, s->act_vars.ptr[i]);

	for (;;) {
		int hConfl = sat_solver_propagate(s);
		if (hConfl != 0) {
			// CONFLICT
			int blevel;

#ifdef VERBOSEDEBUG
			printf(L_IND "**CONFLICT**\n", L_ind);
#endif
			s->stats.conflicts++;
			conflictC++;
			if (sat_solver_dl(s) == s->root_level) {
#ifdef SAT_USE_ANALYZE_FINAL
				sat_solver_analyze_final(s, hConfl, 0);
#endif
				veci_delete(&learnt_clause);
				return l_False;
			}

			veci_resize(&learnt_clause, 0);
			sat_solver_analyze(s, hConfl, &learnt_clause);
			blevel = veci_size(&learnt_clause) > 1 ?
			             var_level(s, lit_var(veci_begin(&learnt_clause)[1])) :
			             s->root_level;
			blevel = s->root_level > blevel ? s->root_level : blevel;
			sat_solver_canceluntil(s, blevel);
			sat_solver_record(s, &learnt_clause);
#ifdef SAT_USE_ANALYZE_FINAL
			//            if (learnt_clause.size() == 1) level[var(learnt_clause[0])] =
			//            0;    // (this is ugly (but needed for 'analyzeFinal()') -- in
			//            future versions, we will backtrack past the 'root_level' and redo the assumptions)
			if (learnt_clause.size == 1)
				var_set_level(s, lit_var(learnt_clause.ptr[0]), 0);
#endif
			act_var_decay(s);
			act_clause_decay(s);

		} else {
			// NO CONFLICT
			int next;

			// Reached bound on number of conflicts:
			if ((!s->fNoRestarts && nof_conflicts >= 0 && conflictC >= nof_conflicts)
			    || (s->nRuntimeLimit && (s->stats.conflicts & 63) == 0
			        && Abc_Clock() > s->nRuntimeLimit)) {
				s->progress_estimate = sat_solver_progress(s);
				sat_solver_canceluntil(s, s->root_level);
				veci_delete(&learnt_clause);
				return l_Undef;
			}

			// Reached bound on number of conflicts:
			if ((s->nConfLimit && s->stats.conflicts > s->nConfLimit)
			    || (s->nInsLimit && s->stats.propagations > s->nInsLimit)) {
				s->progress_estimate = sat_solver_progress(s);
				sat_solver_canceluntil(s, s->root_level);
				veci_delete(&learnt_clause);
				return l_Undef;
			}

			// Simplify the set of problem clauses:
			if (sat_solver_dl(s) == 0 && !s->fSkipSimplify)
				sat_solver_simplify(s);

			// Reduce the set of learnt clauses:
			//            if (s->nLearntMax && veci_size(&s->learned) - s->qtail >= s->nLearntMax)
			if (s->nLearntMax && veci_size(&s->act_clas) >= s->nLearntMax)
				sat_solver_reducedb(s);

			// New variable decision:
			s->stats.decisions++;
			next = order_select(s, (float) random_var_freq);

			if (next == var_Undef) {
				// Model found:
				int i;
				for (i = 0; i < s->size; i++)
					s->model[i] = (var_value(s, i) == var1 ? l_True : l_False);
				sat_solver_canceluntil(s, s->root_level);
				veci_delete(&learnt_clause);

				/*
				veci apa; veci_new(&apa);
				for (i = 0; i < s->size; i++)
				    veci_push(&apa,(int)(s->model.ptr[i] == l_True ? toLit(i) :
				lit_neg(toLit(i)))); printf("model: "); printlits((lit*)apa.ptr,
				(lit*)apa.ptr + veci_size(&apa)); printf("\n"); veci_delete(&apa);
				*/

				return l_True;
			}

			if (var_polar(s, next)) // positive polarity
				sat_solver_decision(s, toLit(next));
			else
				sat_solver_decision(s, lit_neg(toLit(next)));
		}
	}

	return l_Undef; // cannot happen
}

// internal call to the SAT solver
inline int sat_solver_solve_internal(sat_solver* s)
{
	lbool status = l_Undef;
	int restart_iter = 0;
	veci_resize(&s->unit_lits, 0);
	s->nCalls++;

	if (s->verbosity >= 1) {
		printf("==================================[MINISAT]================================"
		       "===\n");
		printf("| Conflicts |     ORIGINAL     |              LEARNT              | "
		       "Progress |\n");
		printf("|           | Clauses Literals |   Limit Clauses Literals  Lit/Cl |        "
		       "  |\n");
		printf("==========================================================================="
		       "===\n");
	}

	while (status == l_Undef) {
		ABC_INT64_T nof_conflicts;
		double Ratio = (s->stats.learnts == 0) ?
		                   0.0 :
		                   s->stats.learnts_literals / (double) s->stats.learnts;
		if (s->nRuntimeLimit && Abc_Clock() > s->nRuntimeLimit)
			break;
		if (s->verbosity >= 1) {
			printf("| %9.0f | %7.0f %8.0f | %7.0f %7.0f %8.0f %7.1f | %6.3f %% |\n",
			       (double) s->stats.conflicts, (double) s->stats.clauses,
			       (double) s->stats.clauses_literals, (double) 0,
			       (double) s->stats.learnts, (double) s->stats.learnts_literals, Ratio,
			       s->progress_estimate * 100);
			fflush(stdout);
		}
		nof_conflicts = (ABC_INT64_T)(100 * luby(2, restart_iter++));
		status = sat_solver_search(s, nof_conflicts);
		// quit the loop if reached an external limit
		if (s->nConfLimit && s->stats.conflicts > s->nConfLimit)
			break;
		if (s->nInsLimit && s->stats.propagations > s->nInsLimit)
			break;
		if (s->nRuntimeLimit && Abc_Clock() > s->nRuntimeLimit)
			break;
		if (s->pFuncStop && s->pFuncStop(s->RunId))
			break;
	}
	if (s->verbosity >= 1)
		printf("==========================================================================="
		       "===\n");

	sat_solver_canceluntil(s, s->root_level);
	// save variable values
	if (status == l_True && s->user_vars.size) {
		int v;
		for (v = 0; v < s->user_vars.size; v++)
			veci_push(&s->user_values, sat_solver_var_value(s, s->user_vars.ptr[v]));
	}
	return status;
}

// pushing one assumption to the stack of assumptions
inline int sat_solver_push(sat_solver* s, int p)
{
	assert(lit_var(p) < s->size);
	veci_push(&s->trail_lim, s->qtail);
	s->root_level++;
	if (!sat_solver_enqueue(s, p, 0)) {
		int h = s->reasons[lit_var(p)];
		if (h) {
			if (clause_is_lit(h)) {
				(clause_begin(s->binary))[1] = lit_neg(p);
				(clause_begin(s->binary))[0] = clause_read_lit(h);
				h = s->hBinary;
			}
			sat_solver_analyze_final(s, h, 1);
			veci_push(&s->conf_final, lit_neg(p));
		} else {
			veci_resize(&s->conf_final, 0);
			veci_push(&s->conf_final, lit_neg(p));
			// the two lines below are a bug fix by Siert Wieringa
			if (var_level(s, lit_var(p)) > 0)
				veci_push(&s->conf_final, p);
		}
		// sat_solver_canceluntil(s, 0);
		return false;
	} else {
		int fConfl = sat_solver_propagate(s);
		if (fConfl) {
			sat_solver_analyze_final(s, fConfl, 0);
			// assert(s->conf_final.size > 0);
			// sat_solver_canceluntil(s, 0);
			return false;
		}
	}
	return true;
}

// removing one assumption from the stack of assumptions
inline void sat_solver_pop(sat_solver* s)
{
	assert(sat_solver_dl(s) > 0);
	sat_solver_canceluntil(s, --s->root_level);
}

inline void sat_solver_set_resource_limits(sat_solver* s, ABC_INT64_T nConfLimit,
                                           ABC_INT64_T nInsLimit, ABC_INT64_T nConfLimitGlobal,
                                           ABC_INT64_T nInsLimitGlobal)
{
	// set the external limits
	s->nRestarts = 0;
	s->nConfLimit = 0;
	s->nInsLimit = 0;
	if (nConfLimit)
		s->nConfLimit = s->stats.conflicts + nConfLimit;
	if (nInsLimit)
		//        s->nInsLimit = s->stats.inspects + nInsLimit;
		s->nInsLimit = s->stats.propagations + nInsLimit;
	if (nConfLimitGlobal && (s->nConfLimit == 0 || s->nConfLimit > nConfLimitGlobal))
		s->nConfLimit = nConfLimitGlobal;
	if (nInsLimitGlobal && (s->nInsLimit == 0 || s->nInsLimit > nInsLimitGlobal))
		s->nInsLimit = nInsLimitGlobal;
}

inline int sat_solver_solve(sat_solver* s, lit* begin, lit* end, ABC_INT64_T nConfLimit,
                            ABC_INT64_T nInsLimit, ABC_INT64_T nConfLimitGlobal,
                            ABC_INT64_T nInsLimitGlobal)
{
	lbool status;
	lit* i;
	////////////////////////////////////////////////
	if (s->fSolved) {
		if (s->pStore) {
			int RetValue = Sto_ManAddClause((Sto_Man_t*) s->pStore, NULL, NULL);
			assert(RetValue);
			(void) RetValue;
		}
		return l_False;
	}
	////////////////////////////////////////////////

	if (s->fVerbose)
		printf("Running SAT solver with parameters %d and %d and %d.\n", s->nLearntStart,
		       s->nLearntDelta, s->nLearntRatio);

	sat_solver_set_resource_limits(s, nConfLimit, nInsLimit, nConfLimitGlobal, nInsLimitGlobal);

#ifdef SAT_USE_ANALYZE_FINAL
	// Perform assumptions:
	s->root_level = 0;
	for (i = begin; i < end; i++)
		if (!sat_solver_push(s, *i)) {
			sat_solver_canceluntil(s, 0);
			s->root_level = 0;
			return l_False;
		}
	assert(s->root_level == sat_solver_dl(s));
#else
	// printf("solve: "); printlits(begin, end); printf("\n");
	for (i = begin; i < end; i++) {
		//        switch (lit_sign(*i) ? -s->assignss[lit_var(*i)] : s->assignss[lit_var(*i)]){
		switch (var_value(s, *i)) {
		case var1: // l_True:
			break;
		case varX: // l_Undef
			sat_solver_decision(s, *i);
			if (sat_solver_propagate(s) == 0)
				break;
			// fallthrough
		case var0: // l_False
			sat_solver_canceluntil(s, 0);
			return l_False;
		}
	}
	s->root_level = sat_solver_dl(s);
#endif

	status = sat_solver_solve_internal(s);

	sat_solver_canceluntil(s, 0);
	s->root_level = 0;

	////////////////////////////////////////////////
	if (status == l_False && s->pStore) {
		int RetValue = Sto_ManAddClause((Sto_Man_t*) s->pStore, NULL, NULL);
		assert(RetValue);
		(void) RetValue;
	}
	////////////////////////////////////////////////
	return status;
}

// This LEXSAT procedure should be called with a set of literals (pLits, nLits),
// which defines both (1) variable order, and (2) assignment to begin search from.
// It retuns the LEXSAT assigment that is the same or larger than the given one.
// (It assumes that there is no smaller assignment than the one given!)
// The resulting assignment is returned in the same set of literals (pLits, nLits).
// It pushes/pops assumptions internally and will undo them before terminating.
inline int sat_solver_solve_lexsat(sat_solver* s, int* pLits, int nLits)
{
	int i, iLitFail = -1;
	lbool status;
	assert(nLits > 0);
	// help the SAT solver by setting desirable polarity
	sat_solver_set_literal_polarity(s, pLits, nLits);
	// check if there exists a satisfying assignment
	status = sat_solver_solve_internal(s);
	if (status != l_True) // no assignment
		return status;
	// there is at least one satisfying assignment
	assert(status == l_True);
	// find the first mismatching literal
	for (i = 0; i < nLits; i++)
		if (pLits[i] != sat_solver_var_literal(s, Abc_Lit2Var(pLits[i])))
			break;
	if (i == nLits) // no mismatch - the current assignment is the minimum one!
		return l_True;
	// mismatch happens in literal i
	iLitFail = i;
	// create assumptions up to this literal (as in pLits) - including this literal!
	for (i = 0; i <= iLitFail; i++)
		if (!sat_solver_push(s, pLits[i])) // can become UNSAT while adding the last assumption
			break;
	if (i < iLitFail + 1) // the solver became UNSAT while adding assumptions
		status = l_False;
	else // solve under the assumptions
		status = sat_solver_solve_internal(s);
	if (status == l_True) {
		// we proved that there is a sat assignment with literal (iLitFail) having polarity
		// as in pLits continue solving recursively
		if (iLitFail + 1 < nLits)
			status = sat_solver_solve_lexsat(s, pLits + iLitFail + 1,
			                                 nLits - iLitFail - 1);
	} else if (status == l_False) {
		// we proved that there is no assignment with iLitFail having polarity as in pLits
		assert(Abc_LitIsCompl(pLits[iLitFail])); // literal is 0
		// (this assert may fail only if there is a sat assignment smaller than one
		// originally given in pLits) now we flip this literal (make it 1), change the last
		// assumption and contiue looking for the 000...0-assignment of other literals
		sat_solver_pop(s);
		pLits[iLitFail] = Abc_LitNot(pLits[iLitFail]);
		if (!sat_solver_push(s, pLits[iLitFail]))
			printf(
			    "sat_solver_solve_lexsat(): A satisfying assignment should exist.\n"); // because we know that the problem is satisfiable
		// update other literals to be 000...0
		for (i = iLitFail + 1; i < nLits; i++)
			pLits[i] = Abc_LitNot(Abc_LitRegular(pLits[i]));
		// continue solving recursively
		if (iLitFail + 1 < nLits)
			status = sat_solver_solve_lexsat(s, pLits + iLitFail + 1,
			                                 nLits - iLitFail - 1);
		else
			status = l_True;
	}
	// undo the assumptions
	for (i = iLitFail; i >= 0; i--)
		sat_solver_pop(s);
	return status;
}

// This procedure is called on a set of assumptions to minimize their number.
// The procedure relies on the fact that the current set of assumptions is UNSAT.
// It receives and returns SAT solver without assumptions. It returns the number
// of assumptions after minimization. The set of assumptions is returned in pLits.
inline int sat_solver_minimize_assumptions(sat_solver* s, int* pLits, int nLits, int nConfLimit)
{
	int i, k, nLitsL, nLitsR, nResL, nResR, status;
	if (nLits == 1) {
		// since the problem is UNSAT, we will try to solve it without assuming the last literal
		// if the result is UNSAT, the last literal can be dropped; otherwise, it is needed
		if (nConfLimit)
			s->nConfLimit = s->stats.conflicts + nConfLimit;
		status = sat_solver_solve_internal(s);
		// printf( "%c", status == l_False ? 'u' : 's' );
		return (int) (status != l_False); // return 1 if the problem is not UNSAT
	}
	assert(nLits >= 2);
	nLitsL = nLits / 2;
	nLitsR = nLits - nLitsL;
	// assume the left lits
	for (i = 0; i < nLitsL; i++)
		if (!sat_solver_push(s, pLits[i])) {
			for (k = i; k >= 0; k--)
				sat_solver_pop(s);
			return sat_solver_minimize_assumptions(s, pLits, i + 1, nConfLimit);
		}
	// solve with these assumptions
	if (nConfLimit)
		s->nConfLimit = s->stats.conflicts + nConfLimit;
	status = sat_solver_solve_internal(s);
	if (status == l_False) // these are enough
	{
		for (i = 0; i < nLitsL; i++)
			sat_solver_pop(s);
		return sat_solver_minimize_assumptions(s, pLits, nLitsL, nConfLimit);
	}
	// solve for the right lits
	nResL = nLitsR == 1 ? 1 :
	                      sat_solver_minimize_assumptions(s, pLits + nLitsL, nLitsR, nConfLimit);
	for (i = 0; i < nLitsL; i++)
		sat_solver_pop(s);
	// swap literals
	//    assert( nResL <= nLitsL );
	//    for ( i = 0; i < nResL; i++ )
	//        ABC_SWAP( int, pLits[i], pLits[nLitsL+i] );
	veci_resize(&s->temp_clause, 0);
	for (i = 0; i < nLitsL; i++)
		veci_push(&s->temp_clause, pLits[i]);
	for (i = 0; i < nResL; i++)
		pLits[i] = pLits[nLitsL + i];
	for (i = 0; i < nLitsL; i++)
		pLits[nResL + i] = veci_begin(&s->temp_clause)[i];
	// assume the right lits
	for (i = 0; i < nResL; i++)
		if (!sat_solver_push(s, pLits[i])) {
			for (k = i; k >= 0; k--)
				sat_solver_pop(s);
			return sat_solver_minimize_assumptions(s, pLits, i + 1, nConfLimit);
		}
	// solve with these assumptions
	if (nConfLimit)
		s->nConfLimit = s->stats.conflicts + nConfLimit;
	status = sat_solver_solve_internal(s);
	if (status == l_False) // these are enough
	{
		for (i = 0; i < nResL; i++)
			sat_solver_pop(s);
		return nResL;
	}
	// solve for the left lits
	nResR = nLitsL == 1 ? 1 :
	                      sat_solver_minimize_assumptions(s, pLits + nResL, nLitsL, nConfLimit);
	for (i = 0; i < nResL; i++)
		sat_solver_pop(s);
	return nResL + nResR;
}

// This is a specialized version of the above procedure with several custom changes:
// - makes sure that at least one of the marked literals is preserved in the clause
// - sets literals to zero when they do not have to be used
// - sets literals to zero for disproved variables
inline int sat_solver_minimize_assumptions2(sat_solver* s, int* pLits, int nLits, int nConfLimit)
{
	int i, k, nLitsL, nLitsR, nResL, nResR;
	if (nLits == 1) {
		// since the problem is UNSAT, we will try to solve it without assuming the last literal
		// if the result is UNSAT, the last literal can be dropped; otherwise, it is needed
		int RetValue = 1, LitNot = Abc_LitNot(pLits[0]);
		int status = l_False;
		int Temp = s->nConfLimit;
		s->nConfLimit = nConfLimit;

		RetValue = sat_solver_push(s, LitNot);
		assert(RetValue);
		status = sat_solver_solve_internal(s);
		sat_solver_pop(s);

		// if the problem is UNSAT, add clause
		if (status == l_False) {
			RetValue = sat_solver_addclause(s, &LitNot, &LitNot + 1);
			assert(RetValue);
		}

		s->nConfLimit = Temp;
		return (int) (status != l_False); // return 1 if the problem is not UNSAT
	}
	assert(nLits >= 2);
	nLitsL = nLits / 2;
	nLitsR = nLits - nLitsL;
	// assume the left lits
	for (i = 0; i < nLitsL; i++)
		if (!sat_solver_push(s, pLits[i])) {
			for (k = i; k >= 0; k--)
				sat_solver_pop(s);

			// add clauses for these literal
			for (k = i + 1; k > nLitsL; k++) {
				int LitNot = Abc_LitNot(pLits[i]);
				int RetValue = sat_solver_addclause(s, &LitNot, &LitNot + 1);
				assert(RetValue);
			}

			return sat_solver_minimize_assumptions2(s, pLits, i + 1, nConfLimit);
		}
	// solve for the right lits
	nResL = sat_solver_minimize_assumptions2(s, pLits + nLitsL, nLitsR, nConfLimit);
	for (i = 0; i < nLitsL; i++)
		sat_solver_pop(s);
	// swap literals
	//    assert( nResL <= nLitsL );
	veci_resize(&s->temp_clause, 0);
	for (i = 0; i < nLitsL; i++)
		veci_push(&s->temp_clause, pLits[i]);
	for (i = 0; i < nResL; i++)
		pLits[i] = pLits[nLitsL + i];
	for (i = 0; i < nLitsL; i++)
		pLits[nResL + i] = veci_begin(&s->temp_clause)[i];
	// assume the right lits
	for (i = 0; i < nResL; i++)
		if (!sat_solver_push(s, pLits[i])) {
			for (k = i; k >= 0; k--)
				sat_solver_pop(s);

			// add clauses for these literal
			for (k = i + 1; k > nResL; k++) {
				int LitNot = Abc_LitNot(pLits[i]);
				int RetValue = sat_solver_addclause(s, &LitNot, &LitNot + 1);
				assert(RetValue);
			}

			return sat_solver_minimize_assumptions2(s, pLits, i + 1, nConfLimit);
		}
	// solve for the left lits
	nResR = sat_solver_minimize_assumptions2(s, pLits + nResL, nLitsL, nConfLimit);
	for (i = 0; i < nResL; i++)
		sat_solver_pop(s);
	return nResL + nResR;
}

inline int sat_solver_nvars(sat_solver* s)
{
	return s->size;
}

inline int sat_solver_nclauses(sat_solver* s)
{
	return s->stats.clauses;
}

inline int sat_solver_nconflicts(sat_solver* s)
{
	return (int) s->stats.conflicts;
}

//=================================================================================================
// Clause storage functions:

inline void sat_solver_store_alloc(sat_solver* s)
{
	assert(s->pStore == NULL);
	s->pStore = Sto_ManAlloc();
}

inline void sat_solver_store_write(sat_solver* s, char* pFileName)
{
	if (s->pStore)
		Sto_ManDumpClauses((Sto_Man_t*) s->pStore, pFileName);
}

inline void sat_solver_store_free(sat_solver* s)
{
	if (s->pStore)
		Sto_ManFree((Sto_Man_t*) s->pStore);
	s->pStore = NULL;
}

inline int sat_solver_store_change_last(sat_solver* s)
{
	if (s->pStore)
		return Sto_ManChangeLastClause((Sto_Man_t*) s->pStore);
	return -1;
}

inline void sat_solver_store_mark_roots(sat_solver* s)
{
	if (s->pStore)
		Sto_ManMarkRoots((Sto_Man_t*) s->pStore);
}

inline void sat_solver_store_mark_clauses_a(sat_solver* s)
{
	if (s->pStore)
		Sto_ManMarkClausesA((Sto_Man_t*) s->pStore);
}

inline void* sat_solver_store_release(sat_solver* s)
{
	void* pTemp;
	if (s->pStore == NULL)
		return NULL;
	pTemp = s->pStore;
	s->pStore = NULL;
	return pTemp;
}

ABC_NAMESPACE_IMPL_END

/*** SimpSolver.cpp */

/***********************************************************************************[SimpSolver.cc]
Copyright (c) 2006,      Niklas Een, Niklas Sorensson
Copyright (c) 2007-2010, Niklas Sorensson

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#include "abc/SimpSolver.h"
#include "abc/Sort.h"
#include "abc/system.h"

ABC_NAMESPACE_IMPL_START

namespace Gluco {

//=================================================================================================
// Options:

static const char* _cat = "SIMP";

//=================================================================================================
// Constructor/Destructor:

inline SimpSolver::SimpSolver()
    : grow(0)
    , clause_lim(20)
    , subsumption_lim(1000)
    , simp_garbage_frac(0.5)
    , use_asymm(false)
    , use_rcheck(false)
    , use_elim(true)
    , merges(0)
    , asymm_lits(0)
    , eliminated_vars(0)
    , eliminated_clauses(0)
    , elimorder(1)
    , use_simplification(true)
    , occurs(ClauseDeleted(ca))
    , elim_heap(ElimLt(n_occ))
    , bwdsub_assigns(0)
    , n_touched(0)
{
	vec<Lit> dummy(1, lit_Undef);
	ca.extra_clause_field = true; // NOTE: must happen before allocating the dummy clause below.
	bwdsub_tmpunit = ca.alloc(dummy);
	remove_satisfied = false;
}

inline SimpSolver::~SimpSolver()
{}

inline Var SimpSolver::newVar(bool sign, bool dvar)
{
	Var v = Solver::newVar(sign, dvar);

	frozen.push((char) false);
	eliminated.push((char) false);

	if (use_simplification) {
		n_occ.push(0);
		n_occ.push(0);
		occurs.init(v);
		touched.push(0);
		elim_heap.insert(v);
	}
	return v;
}

inline lbool SimpSolver::solve_(bool do_simp, bool turn_off_simp)
{
	vec<Var> extra_frozen;
	lbool result = l_True;

	do_simp &= use_simplification;

	if (do_simp) {
		// Assumptions must be temporarily frozen to run variable elimination:
		for (int i = 0; i < assumptions.size(); i++) {
			Var v = var(assumptions[i]);

			// If an assumption has been eliminated, remember it.
			assert(!isEliminated(v));

			if (!frozen[v]) {
				// Freeze and store.
				setFrozen(v, true);
				extra_frozen.push(v);
			}
		}

		result = lbool(eliminate(turn_off_simp));
	}

	if (result == l_True)
		result = Solver::solve_();
	else if (verbosity >= 1)
		printf("==========================================================================="
		       "====\n");

	if (result == l_True)
		extendModel();

	if (do_simp)
		// Unfreeze the assumptions that were frozen:
		for (int i = 0; i < extra_frozen.size(); i++)
			setFrozen(extra_frozen[i], false);

	return result;
}

inline bool SimpSolver::addClause_(vec<Lit>& ps)
{
#ifndef NDEBUG
	for (int i = 0; i < ps.size(); i++)
		assert(!isEliminated(var(ps[i])));
#endif
	int nclauses = clauses.size();

	if (use_rcheck && implied(ps))
		return true;

	if (!Solver::addClause_(ps))
		return false;

	if (use_simplification && clauses.size() == nclauses + 1) {
		CRef cr = clauses.last();
		const Clause& c = ca[cr];

		// NOTE: the clause is added to the queue immediately and then
		// again during 'gatherTouchedClauses()'. If nothing happens
		// in between, it will only be checked once. Otherwise, it may
		// be checked twice unnecessarily. This is an unfortunate
		// consequence of how backward subsumption is used to mimic
		// forward subsumption.
		subsumption_queue.insert(cr);
		for (int i = 0; i < c.size(); i++) {
			occurs[var(c[i])].push(cr);
			n_occ[toInt(c[i])]++;
			touched[var(c[i])] = 1;
			n_touched++;
			if (elim_heap.inHeap(var(c[i])))
				elim_heap.increase(var(c[i]));
		}
	}

	return true;
}

inline void SimpSolver::removeClause(CRef cr)
{
	const Clause& c = ca[cr];

	if (use_simplification)
		for (int i = 0; i < c.size(); i++) {
			n_occ[toInt(c[i])]--;
			updateElimHeap(var(c[i]));
			occurs.smudge(var(c[i]));
		}

	Solver::removeClause(cr);
}

inline bool SimpSolver::strengthenClause(CRef cr, Lit l)
{
	Clause& c = ca[cr];
	assert(decisionLevel() == 0);
	assert(use_simplification);

	// FIX: this is too inefficient but would be nice to have (properly implemented)
	// if (!find(subsumption_queue, &c))
	subsumption_queue.insert(cr);

	if (certifiedUNSAT) {
		for (int i = 0; i < c.size(); i++)
			if (c[i] != l)
				fprintf(certifiedOutput, "%i ",
				        (var(c[i]) + 1) * (-2 * sign(c[i]) + 1));
		fprintf(certifiedOutput, "0\n");
	}

	if (c.size() == 2) {
		removeClause(cr);
		c.strengthen(l);
	} else {
		if (certifiedUNSAT) {
			fprintf(certifiedOutput, "d ");
			for (int i = 0; i < c.size(); i++)
				fprintf(certifiedOutput, "%i ",
				        (var(c[i]) + 1) * (-2 * sign(c[i]) + 1));
			fprintf(certifiedOutput, "0\n");
		}

		detachClause(cr, true);
		c.strengthen(l);
		attachClause(cr);
		remove(occurs[var(l)], cr);
		n_occ[toInt(l)]--;
		updateElimHeap(var(l));
	}

	return c.size() == 1 ? enqueue(c[0]) && propagate() == CRef_Undef : true;
}

// Returns FALSE if clause is always satisfied ('out_clause' should not be used).
inline bool SimpSolver::merge(const Clause& _ps, const Clause& _qs, Var v, vec<Lit>& out_clause)
{
	merges++;
	out_clause.clear();

	bool ps_smallest = _ps.size() < _qs.size();
	const Clause& ps = ps_smallest ? _qs : _ps;
	const Clause& qs = ps_smallest ? _ps : _qs;

	int i, j;
	for (i = 0; i < qs.size(); i++) {
		if (var(qs[i]) != v) {
			for (j = 0; j < ps.size(); j++)
				if (var(ps[j]) == var(qs[i])) {
					if (ps[j] == ~qs[i])
						return false;
					else
						goto next;
				}
			out_clause.push(qs[i]);
		}
	next:;
	}

	for (i = 0; i < ps.size(); i++)
		if (var(ps[i]) != v)
			out_clause.push(ps[i]);

	return true;
}

// Returns FALSE if clause is always satisfied.
inline bool SimpSolver::merge(const Clause& _ps, const Clause& _qs, Var v, int& size)
{
	merges++;

	bool ps_smallest = _ps.size() < _qs.size();
	const Clause& ps = ps_smallest ? _qs : _ps;
	const Clause& qs = ps_smallest ? _ps : _qs;
	const Lit* __ps = (const Lit*) ps;
	const Lit* __qs = (const Lit*) qs;

	size = ps.size() - 1;

	for (int i = 0; i < qs.size(); i++) {
		if (var(__qs[i]) != v) {
			for (int j = 0; j < ps.size(); j++)
				if (var(__ps[j]) == var(__qs[i])) {
					if (__ps[j] == ~__qs[i])
						return false;
					else
						goto next;
				}
			size++;
		}
	next:;
	}

	return true;
}

inline void SimpSolver::gatherTouchedClauses()
{
	if (n_touched == 0)
		return;

	int i, j;
	for (i = j = 0; i < subsumption_queue.size(); i++)
		if (ca[subsumption_queue[i]].mark() == 0)
			ca[subsumption_queue[i]].mark(2);

	for (i = 0; i < touched.size(); i++)
		if (touched[i]) {
			const vec<CRef>& cs = occurs.lookup(i);
			for (j = 0; j < cs.size(); j++)
				if (ca[cs[j]].mark() == 0) {
					subsumption_queue.insert(cs[j]);
					ca[cs[j]].mark(2);
				}
			touched[i] = 0;
		}

	for (i = 0; i < subsumption_queue.size(); i++)
		if (ca[subsumption_queue[i]].mark() == 2)
			ca[subsumption_queue[i]].mark(0);

	n_touched = 0;
}

inline bool SimpSolver::implied(const vec<Lit>& c)
{
	assert(decisionLevel() == 0);

	trail_lim.push(trail.size());
	for (int i = 0; i < c.size(); i++)
		if (value(c[i]) == l_True) {
			cancelUntil(0);
			return false;
		} else if (value(c[i]) != l_False) {
			assert(value(c[i]) == l_Undef);
			uncheckedEnqueue(~c[i]);
		}

	bool result = propagate() != CRef_Undef;
	cancelUntil(0);
	return result;
}

// Backward subsumption + backward subsumption resolution
inline bool SimpSolver::backwardSubsumptionCheck(bool verbose)
{
	int cnt = 0;
	int subsumed = 0;
	int deleted_literals = 0;
	assert(decisionLevel() == 0);

	while (subsumption_queue.size() > 0 || bwdsub_assigns < trail.size()) {

		// Empty subsumption queue and return immediately on user-interrupt:
		if (asynch_interrupt) {
			subsumption_queue.clear();
			bwdsub_assigns = trail.size();
			break;
		}

		// Check top-level assignments by creating a dummy clause and placing it in the queue:
		if (subsumption_queue.size() == 0 && bwdsub_assigns < trail.size()) {
			Lit l = trail[bwdsub_assigns++];
			ca[bwdsub_tmpunit][0] = l;
			ca[bwdsub_tmpunit].calcAbstraction();
			subsumption_queue.insert(bwdsub_tmpunit);
		}

		CRef cr = subsumption_queue.peek();
		subsumption_queue.pop();
		Clause& c = ca[cr];

		if (c.mark())
			continue;

		if (verbose && verbosity >= 2 && cnt++ % 1000 == 0)
			printf("subsumption left: %10d (%10d subsumed, %10d deleted literals)\r",
			       subsumption_queue.size(), subsumed, deleted_literals);

		assert(c.size() > 1
		       || value(c[0])
		              == l_True); // Unit-clauses should have been propagated before this point.

		// Find best variable to scan:
		Var best = var(c[0]);
		for (int i = 1; i < c.size(); i++)
			if (occurs[var(c[i])].size() < occurs[best].size())
				best = var(c[i]);

		// Search all candidates:
		vec<CRef>& _cs = occurs.lookup(best);
		CRef* cs = (CRef*) _cs;

		for (int j = 0; j < _cs.size(); j++)
			if (c.mark())
				break;
			else if (!ca[cs[j]].mark() && cs[j] != cr
			         && (subsumption_lim == -1 || ca[cs[j]].size() < subsumption_lim)) {
				Lit l = c.subsumes(ca[cs[j]]);

				if (l == lit_Undef)
					subsumed++, removeClause(cs[j]);
				else if (l != lit_Error) {
					deleted_literals++;

					if (!strengthenClause(cs[j], ~l))
						return false;

					// Did current candidate get deleted from cs? Then check
					// candidate at index j again:
					if (var(l) == best)
						j--;
				}
			}
	}

	return true;
}

inline bool SimpSolver::asymm(Var v, CRef cr)
{
	Clause& c = ca[cr];
	assert(decisionLevel() == 0);

	if (c.mark() || satisfied(c))
		return true;

	trail_lim.push(trail.size());
	Lit l = lit_Undef;
	for (int i = 0; i < c.size(); i++)
		if (var(c[i]) != v && value(c[i]) != l_False)
			uncheckedEnqueue(~c[i]);
		else
			l = c[i];

	if (propagate() != CRef_Undef) {
		cancelUntil(0);
		asymm_lits++;
		if (!strengthenClause(cr, l))
			return false;
	} else
		cancelUntil(0);

	return true;
}

inline bool SimpSolver::asymmVar(Var v)
{
	assert(use_simplification);

	const vec<CRef>& cls = occurs.lookup(v);

	if (value(v) != l_Undef || cls.size() == 0)
		return true;

	for (int i = 0; i < cls.size(); i++)
		if (!asymm(v, cls[i]))
			return false;

	return backwardSubsumptionCheck();
}

static void mkElimClause(vec<uint32_t>& elimclauses, Lit x)
{
	elimclauses.push(toInt(x));
	elimclauses.push(1);
}

static void mkElimClause(vec<uint32_t>& elimclauses, Var v, Clause& c)
{
	int first = elimclauses.size();
	int v_pos = -1;

	// Copy clause to elimclauses-vector. Remember position where the
	// variable 'v' occurs:
	for (int i = 0; i < c.size(); i++) {
		elimclauses.push(toInt(c[i]));
		if (var(c[i]) == v)
			v_pos = i + first;
	}
	assert(v_pos != -1);

	// Swap the first literal with the 'v' literal, so that the literal
	// containing 'v' will occur first in the clause:
	uint32_t tmp = elimclauses[v_pos];
	elimclauses[v_pos] = elimclauses[first];
	elimclauses[first] = tmp;

	// Store the length of the clause last:
	elimclauses.push(c.size());
}

inline bool SimpSolver::eliminateVar(Var v)
{
	int i, j;
	assert(!frozen[v]);
	assert(!isEliminated(v));
	assert(value(v) == l_Undef);

	// Split the occurrences into positive and negative:
	//
	const vec<CRef>& cls = occurs.lookup(v);
	vec<CRef> pos, neg;
	for (i = 0; i < cls.size(); i++)
		(find(ca[cls[i]], mkLit(v)) ? pos : neg).push(cls[i]);

	// Check wether the increase in number of clauses stays within the allowed ('grow').
	// Moreover, no clause must exceed the limit on the maximal clause size (if it is set):
	//
	int cnt = 0;
	int clause_size = 0;

	for (i = 0; i < pos.size(); i++)
		for (j = 0; j < neg.size(); j++)
			if (merge(ca[pos[i]], ca[neg[j]], v, clause_size)
			    && (++cnt > cls.size() + grow
			        || (clause_lim != -1 && clause_size > clause_lim)))
				return true;

	// Delete and store old clauses:
	eliminated[v] = true;
	setDecisionVar(v, false);
	eliminated_vars++;

	if (pos.size() > neg.size()) {
		for (i = 0; i < neg.size(); i++)
			mkElimClause(elimclauses, v, ca[neg[i]]);
		mkElimClause(elimclauses, mkLit(v));
		eliminated_clauses += neg.size();
	} else {
		for (i = 0; i < pos.size(); i++)
			mkElimClause(elimclauses, v, ca[pos[i]]);
		mkElimClause(elimclauses, ~mkLit(v));
		eliminated_clauses += pos.size();
	}

	// Produce clauses in cross product:
	vec<Lit>& resolvent = add_tmp;
	for (i = 0; i < pos.size(); i++)
		for (j = 0; j < neg.size(); j++)
			if (merge(ca[pos[i]], ca[neg[j]], v, resolvent) && !addClause_(resolvent))
				return false;

	for (i = 0; i < cls.size(); i++)
		removeClause(cls[i]);

	// Free occurs list for this variable:
	occurs[v].clear(true);

	// Free watchers lists for this variable, if possible:
	if (watches[mkLit(v)].size() == 0)
		watches[mkLit(v)].clear(true);
	if (watches[~mkLit(v)].size() == 0)
		watches[~mkLit(v)].clear(true);

	return backwardSubsumptionCheck();
}

inline bool SimpSolver::substitute(Var v, Lit x)
{
	assert(!frozen[v]);
	assert(!isEliminated(v));
	assert(value(v) == l_Undef);

	if (!ok)
		return false;

	eliminated[v] = true;
	setDecisionVar(v, false);
	const vec<CRef>& cls = occurs.lookup(v);

	vec<Lit>& subst_clause = add_tmp;
	for (int i = 0; i < cls.size(); i++) {
		Clause& c = ca[cls[i]];

		subst_clause.clear();
		for (int j = 0; j < c.size(); j++) {
			Lit p = c[j];
			subst_clause.push(var(p) == v ? x ^ sign(p) : p);
		}

		if (!addClause_(subst_clause))
			return ok = false;

		removeClause(cls[i]);
	}

	return true;
}

inline void SimpSolver::extendModel()
{
	int i, j;
	Lit x;

	for (i = elimclauses.size() - 1; i > 0; i -= j) {
		for (j = elimclauses[i--]; j > 1; j--, i--)
			if (modelValue(toLit(elimclauses[i])) != l_False)
				goto next;

		x = toLit(elimclauses[i]);
		model[var(x)] = lbool(!sign(x));
	next:;
	}
}

inline bool SimpSolver::eliminate(bool turn_off_elim)
{
	// abctime clk = Abc_Clock();
	if (!simplify())
		return false;
	else if (!use_simplification)
		return true;

	// Main simplification loop:
	//

	int toPerform = clauses.size() <= 4800000;

	if (!toPerform) {
		printf("c Too many clauses... No preprocessing\n");
	}

	while (toPerform && (n_touched > 0 || bwdsub_assigns < trail.size() || elim_heap.size() > 0)) {

		gatherTouchedClauses();
		// printf("  ## (time = %6.2f s) BWD-SUB: queue = %d, trail = %d\n", cpuTime(),
		// subsumption_queue.size(), trail.size() - bwdsub_assigns);
		if ((subsumption_queue.size() > 0 || bwdsub_assigns < trail.size())
		    && !backwardSubsumptionCheck(true)) {
			ok = false;
			goto cleanup;
		}

		// Empty elim_heap and return immediately on user-interrupt:
		if (asynch_interrupt) {
			assert(bwdsub_assigns == trail.size());
			assert(subsumption_queue.size() == 0);
			assert(n_touched == 0);
			elim_heap.clear();
			goto cleanup;
		}

		// printf("  ## (time = %6.2f s) ELIM: vars = %d\n", cpuTime(), elim_heap.size());
		for (int cnt = 0; !elim_heap.empty(); cnt++) {
			Var elim = elim_heap.removeMin();

			if (asynch_interrupt)
				break;

			if (isEliminated(elim) || value(elim) != l_Undef)
				continue;

			if (verbosity >= 2 && cnt % 100 == 0)
				printf("elimination left: %10d\r", elim_heap.size());

			if (use_asymm) {
				// Temporarily freeze variable. Otherwise, it would immediately end up on the queue again:
				bool was_frozen = frozen[elim] != 0;
				frozen[elim] = true;
				if (!asymmVar(elim)) {
					ok = false;
					goto cleanup;
				}
				frozen[elim] = was_frozen;
			}

			// At this point, the variable may have been set by assymetric branching, so
			// check it again. Also, don't eliminate frozen variables:
			if (use_elim && value(elim) == l_Undef && !frozen[elim]
			    && !eliminateVar(elim)) {
				ok = false;
				goto cleanup;
			}

			checkGarbage(simp_garbage_frac);
		}

		assert(subsumption_queue.size() == 0);
	}
cleanup:

	// If no more simplification is needed, free all simplification-related data structures:
	if (turn_off_elim) {
		touched.clear(true);
		occurs.clear(true);
		n_occ.clear(true);
		elim_heap.clear(true);
		subsumption_queue.clear(true);

		use_simplification = false;
		remove_satisfied = true;
		ca.extra_clause_field = false;

		// Force full cleanup (this is safe and desirable since it only happens once):
		rebuildOrderHeap();
		garbageCollect();
	} else {
		// Cheaper cleanup:
		cleanUpClauses(); // TODO: can we make 'cleanUpClauses()' not be linear in the problem size somehow?
		checkGarbage();
	}

	if (verbosity >= 1 && elimclauses.size() > 0)
		printf("c |  Eliminated clauses:     %10.2f Mb                                     "
		       "                           |\n",
		       double(elimclauses.size() * sizeof(uint32_t)) / (1024 * 1024));
	return ok;
}

inline void SimpSolver::cleanUpClauses()
{
	occurs.cleanAll();
	int i, j;
	for (i = j = 0; i < clauses.size(); i++)
		if (ca[clauses[i]].mark() == 0)
			clauses[j++] = clauses[i];
	clauses.shrink(i - j);
}

//=================================================================================================
// Garbage Collection methods:

inline void SimpSolver::relocAll(ClauseAllocator& to)
{
	int i;
	if (!use_simplification)
		return;

	// All occurs lists:
	//
	for (i = 0; i < nVars(); i++) {
		vec<CRef>& cs = occurs[i];
		for (int j = 0; j < cs.size(); j++)
			ca.reloc(cs[j], to);
	}

	// Subsumption queue:
	//
	for (i = 0; i < subsumption_queue.size(); i++)
		ca.reloc(subsumption_queue[i], to);

	// Temporary clause:
	//
	ca.reloc(bwdsub_tmpunit, to);
}

inline void SimpSolver::garbageCollect()
{
	// Initialize the next region to a size corresponding to the estimated utilization degree.
	// This is not precise but should avoid some unnecessary reallocations for the new region:
	ClauseAllocator to(ca.size() - ca.wasted());

	cleanUpClauses();
	to.extra_clause_field
	    = ca.extra_clause_field; // NOTE: this is important to keep (or lose) the extra fields.
	relocAll(to);
	Solver::relocAll(to);
	if (verbosity >= 2)
		printf("|  Garbage collection:   %12d bytes => %12d bytes             |\n",
		       ca.size() * ClauseAllocator::Unit_Size,
		       to.size() * ClauseAllocator::Unit_Size);
	to.moveTo(ca);
}

inline void SimpSolver::reset()
{
	Solver::reset();
	grow = 0;
	asymm_lits = eliminated_vars = bwdsub_assigns = n_touched = 0;
	elimclauses.clear(false);
	touched.clear(false);
	occurs.clear(false);
	n_occ.clear(false);
	elim_heap.clear(false);
	subsumption_queue.clear(false);
	frozen.clear(false);
	eliminated.clear(false);
	vec<Lit> dummy(1, lit_Undef);
	ca.extra_clause_field = true; // NOTE: must happen before allocating the dummy clause below.
	bwdsub_tmpunit = ca.alloc(dummy);
	remove_satisfied = false;
}

} /* namespace Gluco */

ABC_NAMESPACE_IMPL_END

/*** Glucose.cpp ***/

/***************************************************************************************[Solver.cc]
 Glucose -- Copyright (c) 2013, Gilles Audemard, Laurent Simon
                                CRIL - Univ. Artois, France
                                LRI  - Univ. Paris Sud, France

Glucose sources are based on MiniSat (see below MiniSat copyrights). Permissions and copyrights of
Glucose are exactly the same as Minisat on which it is based on. (see below).

---------------

Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
Copyright (c) 2007-2010, Niklas Sorensson

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#include "abc/Constants.h"
#include "abc/Solver.h"
#include "abc/Sort.h"
#include "abc/system.h"

#include <math.h>

ABC_NAMESPACE_IMPL_START

//=================================================================================================
// Options:

static const char* _cat = "CORE";
static const char* _cr = "CORE -- RESTART";
static const char* _cred = "CORE -- REDUCE";
static const char* _cm = "CORE -- MINIMIZE";
static const char* _certified = "CORE -- CERTIFIED UNSAT";

const bool opt_incremental = false;
const double opt_K = 0.8;
const double opt_R = 1.4;
const int opt_size_lbd_queue = 50;

const int opt_first_reduce_db = 2000;
const int opt_inc_reduce_db = 300;
const int opt_spec_inc_reduce_db = 1000;
const int opt_lb_lbd_frozen_clause = 30;

const int opt_lb_size_minimzing_clause = 30;
const int opt_lb_lbd_minimzing_clause = 6;

const double opt_var_decay = 0.8;
const double opt_clause_decay = 0.999;
const double opt_random_var_freq = 0;
const double opt_random_seed = 91648253;
const int opt_ccmin_mode = 2;
const int opt_phase_saving = 2;
const bool opt_rnd_init_act = false;
/*
static IntOption     opt_restart_first     (_cat, "rfirst",      "The base restart interval", 100,
IntRange(1, INT32_MAX)); static DoubleOption  opt_restart_inc       (_cat, "rinc",        "Restart
interval increase factor",                                                         2, DoubleRange(1,
false, HUGE_VAL, false));
*/
const double opt_garbage_frac = 0.20;

const bool opt_certified_ = false;
const char* const opt_certified_file_ = "NULL";

namespace Gluco {

//=================================================================================================
// Constructor/Destructor:

inline Solver::Solver()
    :

    // Parameters (user settable):
    //
    SolverType(0)
    , pCnfFunc(NULL)
    , nCallConfl(1000)
    , terminate_search_early(false)
    , pstop(NULL)
    , nRuntimeLimit(0)

    , verbosity(0)
    , verbEveryConflicts(10000)
    , showModel(0)
    , K(opt_K)
    , R(opt_R)
    , sizeLBDQueue(50)
    , sizeTrailQueue(5000)
    , firstReduceDB(opt_first_reduce_db)
    , incReduceDB(opt_inc_reduce_db)
    , specialIncReduceDB(opt_spec_inc_reduce_db)
    , lbLBDFrozenClause(opt_lb_lbd_frozen_clause)
    , lbSizeMinimizingClause(opt_lb_size_minimzing_clause)
    , lbLBDMinimizingClause(opt_lb_lbd_minimzing_clause)
    , var_decay(opt_var_decay)
    , clause_decay(opt_clause_decay)
    , random_var_freq(opt_random_var_freq)
    , random_seed(opt_random_seed)
    , ccmin_mode(opt_ccmin_mode)
    , phase_saving(opt_phase_saving)
    , rnd_pol(false)
    , rnd_init_act(opt_rnd_init_act)
    , garbage_frac(opt_garbage_frac)
    , certifiedOutput(NULL)
    , certifiedUNSAT(opt_certified_)
    // Statistics: (formerly in 'SolverStats')
    //
    , nbRemovedClauses(0)
    , nbReducedClauses(0)
    , nbDL2(0)
    , nbBin(0)
    , nbUn(0)
    , nbReduceDB(0)
    , solves(0)
    , starts(0)
    , decisions(0)
    , rnd_decisions(0)
    , propagations(0)
    , conflicts(0)
    , conflictsRestarts(0)
    , nbstopsrestarts(0)
    , nbstopsrestartssame(0)
    , lastblockatrestart(0)
    , dec_vars(0)
    , clauses_literals(0)
    , learnts_literals(0)
    , max_literals(0)
    , tot_literals(0)
    , curRestart(1)

    , ok(true)
    , cla_inc(1)
    , var_inc(1)
    , watches(WatcherDeleted(ca))
    , watchesBin(WatcherDeleted(ca))
    , qhead(0)
    , simpDB_assigns(-1)
    , simpDB_props(0)
    , order_heap(VarOrderLt(activity))
    , progress_estimate(0)
    , remove_satisfied(true)

    // Resource constraints:
    //
    , conflict_budget(-1)
    , propagation_budget(-1)
    , asynch_interrupt(false)
    , incremental(opt_incremental)
    , nbVarsInitialFormula(INT32_MAX)
{
	MYFLAG = 0;
	// Initialize only first time. Useful for incremental solving, useless otherwise
	lbdQueue.initSize(sizeLBDQueue);
	trailQueue.initSize(sizeTrailQueue);
	sumLBD = 0;
	nbclausesbeforereduce = firstReduceDB;
	totalTime4Sat = 0;
	totalTime4Unsat = 0;
	nbSatCalls = 0;
	nbUnsatCalls = 0;

	if (certifiedUNSAT) {
		if (!strcmp(opt_certified_file_, "NULL")) {
			certifiedOutput = fopen("/dev/stdout", "wb");
		} else {
			certifiedOutput = fopen(opt_certified_file_, "wb");
		}
		//    fprintf(certifiedOutput,"o proof DRUP\n");
	}
}

inline Solver::~Solver()
{}

/****************************************************************
 Set the incremental mode
****************************************************************/

// This function set the incremental mode to true.
// You can add special code for this mode here.

inline void Solver::setIncrementalMode()
{
	incremental = true;
}

// Number of variables without selectors
inline void Solver::initNbInitialVars(int nb)
{
	nbVarsInitialFormula = nb;
}

//=================================================================================================
// Minor methods:

// Creates a new SAT variable in the solver. If 'decision' is cleared, variable will not be
// used as a decision variable (NOTE! This has effects on the meaning of a SATISFIABLE result).
//
inline Var Solver::newVar(bool sign, bool dvar)
{
	int v = nVars();
	watches.init(mkLit(v, false));
	watches.init(mkLit(v, true));
	watchesBin.init(mkLit(v, false));
	watchesBin.init(mkLit(v, true));
	assigns.push(l_Undef);
	vardata.push(mkVarData(CRef_Undef, 0));
	// activity .push(0);
	activity.push(rnd_init_act ? drand(random_seed) * 0.00001 : 0);
	seen.push(0);
	permDiff.push(0);
	polarity.push(sign);
	decision.push();
	trail.capacity(v + 1);
	setDecisionVar(v, dvar);
	return v;
}

inline bool Solver::addClause_(vec<Lit>& ps)
{
	assert(decisionLevel() == 0);
	if (!ok)
		return false;

	if (0) {
		for (int i = 0; i < ps.size(); i++)
			printf("%s%d ", (toInt(ps[i]) & 1) ? "-" : "", toInt(ps[i]) >> 1);
		printf("\n");
	}

	// Check if clause is satisfied and remove false/duplicate literals:
	sort(ps);

	vec<Lit> oc;
	oc.clear();

	Lit p;
	int i, j, flag = 0;
	if (certifiedUNSAT) {
		for (i = j = 0, p = lit_Undef; i < ps.size(); i++) {
			oc.push(ps[i]);
			if (value(ps[i]) == l_True || ps[i] == ~p || value(ps[i]) == l_False)
				flag = 1;
		}
	}

	for (i = j = 0, p = lit_Undef; i < ps.size(); i++)
		if (value(ps[i]) == l_True || ps[i] == ~p)
			return true;
		else if (value(ps[i]) != l_False && ps[i] != p)
			ps[j++] = p = ps[i];
	ps.shrink(i - j);

	if (0) {
		for (int i = 0; i < ps.size(); i++)
			printf("%s%d ", (toInt(ps[i]) & 1) ? "-" : "", toInt(ps[i]) >> 1);
		printf("\n");
	}

	if (flag && (certifiedUNSAT)) {
		for (i = j = 0, p = lit_Undef; i < ps.size(); i++)
			fprintf(certifiedOutput, "%i ", (var(ps[i]) + 1) * (-2 * sign(ps[i]) + 1));
		fprintf(certifiedOutput, "0\n");

		fprintf(certifiedOutput, "d ");
		for (i = j = 0, p = lit_Undef; i < oc.size(); i++)
			fprintf(certifiedOutput, "%i ", (var(oc[i]) + 1) * (-2 * sign(oc[i]) + 1));
		fprintf(certifiedOutput, "0\n");
	}

	if (ps.size() == 0)
		return ok = false;
	else if (ps.size() == 1) {
		uncheckedEnqueue(ps[0]);
		return ok = (propagate() == CRef_Undef);
	} else {
		CRef cr = ca.alloc(ps, false);
		clauses.push(cr);
		attachClause(cr);
	}

	return true;
}

inline void Solver::attachClause(CRef cr)
{
	const Clause& c = ca[cr];

	assert(c.size() > 1);
	if (c.size() == 2) {
		watchesBin[~c[0]].push(Watcher(cr, c[1]));
		watchesBin[~c[1]].push(Watcher(cr, c[0]));
	} else {
		watches[~c[0]].push(Watcher(cr, c[1]));
		watches[~c[1]].push(Watcher(cr, c[0]));
	}
	if (c.learnt())
		learnts_literals += c.size();
	else
		clauses_literals += c.size();
}

inline void Solver::detachClause(CRef cr, bool strict)
{
	const Clause& c = ca[cr];

	assert(c.size() > 1);
	if (c.size() == 2) {
		if (strict) {
			remove(watchesBin[~c[0]], Watcher(cr, c[1]));
			remove(watchesBin[~c[1]], Watcher(cr, c[0]));
		} else {
			// Lazy detaching: (NOTE! Must clean all watcher lists before garbage collecting this clause)
			watchesBin.smudge(~c[0]);
			watchesBin.smudge(~c[1]);
		}
	} else {
		if (strict) {
			remove(watches[~c[0]], Watcher(cr, c[1]));
			remove(watches[~c[1]], Watcher(cr, c[0]));
		} else {
			// Lazy detaching: (NOTE! Must clean all watcher lists before garbage collecting this clause)
			watches.smudge(~c[0]);
			watches.smudge(~c[1]);
		}
	}
	if (c.learnt())
		learnts_literals -= c.size();
	else
		clauses_literals -= c.size();
}

inline void Solver::removeClause(CRef cr)
{

	Clause& c = ca[cr];

	if (certifiedUNSAT) {
		fprintf(certifiedOutput, "d ");
		for (int i = 0; i < c.size(); i++)
			fprintf(certifiedOutput, "%i ", (var(c[i]) + 1) * (-2 * sign(c[i]) + 1));
		fprintf(certifiedOutput, "0\n");
	}

	detachClause(cr);
	// Don't leave pointers to free'd memory!
	if (locked(c))
		vardata[var(c[0])].reason = CRef_Undef;
	c.mark(1);
	ca.free_(cr);
}

inline bool Solver::satisfied(const Clause& c) const
{
	if (incremental) // Check clauses with many selectors is too time consuming
		return (value(c[0]) == l_True) || (value(c[1]) == l_True);

	// Default mode.
	for (int i = 0; i < c.size(); i++)
		if (value(c[i]) == l_True)
			return true;
	return false;
}

/************************************************************
 * Compute LBD functions
 *************************************************************/

inline unsigned int Solver::computeLBD(const vec<Lit>& lits, int end)
{
	int nblevels = 0;
	MYFLAG++;

	if (incremental) { // ----------------- INCREMENTAL MODE
		if (end == -1)
			end = lits.size();
		unsigned int nbDone = 0;
		for (int i = 0; i < lits.size(); i++) {
			if (nbDone >= end)
				break;
			if (isSelector(var(lits[i])))
				continue;
			nbDone++;
			int l = level(var(lits[i]));
			if (permDiff[l] != MYFLAG) {
				permDiff[l] = MYFLAG;
				nblevels++;
			}
		}
	} else { // -------- DEFAULT MODE. NOT A LOT OF DIFFERENCES... BUT EASIER TO READ
		for (int i = 0; i < lits.size(); i++) {
			int l = level(var(lits[i]));
			if (permDiff[l] != MYFLAG) {
				permDiff[l] = MYFLAG;
				nblevels++;
			}
		}
	}

	return nblevels;
}

inline unsigned int Solver::computeLBD(const Clause& c)
{
	int nblevels = 0;
	MYFLAG++;

	if (incremental) { // ----------------- INCREMENTAL MODE
		int nbDone = 0;
		for (int i = 0; i < c.size(); i++) {
			if (nbDone >= c.sizeWithoutSelectors())
				break;
			if (isSelector(var(c[i])))
				continue;
			nbDone++;
			int l = level(var(c[i]));
			if (permDiff[l] != MYFLAG) {
				permDiff[l] = MYFLAG;
				nblevels++;
			}
		}
	} else { // -------- DEFAULT MODE. NOT A LOT OF DIFFERENCES... BUT EASIER TO READ
		for (int i = 0; i < c.size(); i++) {
			int l = level(var(c[i]));
			if (permDiff[l] != MYFLAG) {
				permDiff[l] = MYFLAG;
				nblevels++;
			}
		}
	}
	return nblevels;
}

/******************************************************************
 * Minimisation with binary reolution
 ******************************************************************/
inline void Solver::minimisationWithBinaryResolution(vec<Lit>& out_learnt)
{

	// Find the LBD measure
	unsigned int lbd = computeLBD(out_learnt);
	Lit p = ~out_learnt[0];

	if (lbd <= lbLBDMinimizingClause) {
		MYFLAG++;

		for (int i = 1; i < out_learnt.size(); i++) {
			permDiff[var(out_learnt[i])] = MYFLAG;
		}

		vec<Watcher>& wbin = watchesBin[p];
		int nb = 0;
		for (int k = 0; k < wbin.size(); k++) {
			Lit imp = wbin[k].blocker;
			if (permDiff[var(imp)] == MYFLAG && value(imp) == l_True) {
				nb++;
				permDiff[var(imp)] = MYFLAG - 1;
			}
		}
		int l = out_learnt.size() - 1;
		if (nb > 0) {
			nbReducedClauses++;
			for (int i = 1; i < out_learnt.size() - nb; i++) {
				if (permDiff[var(out_learnt[i])] != MYFLAG) {
					Lit p = out_learnt[l];
					out_learnt[l] = out_learnt[i];
					out_learnt[i] = p;
					l--;
					i--;
				}
			}

			out_learnt.shrink(nb);
		}
	}
}

// Revert to the state at given level (keeping all assignment at 'level' but not beyond).
//
inline void Solver::cancelUntil(int level)
{
	if (decisionLevel() > level) {
		for (int c = trail.size() - 1; c >= trail_lim[level]; c--) {
			Var x = var(trail[c]);
			assigns[x] = l_Undef;
			if (phase_saving > 1 || ((phase_saving == 1) && c > trail_lim.last()))
				polarity[x] = sign(trail[c]);
			insertVarOrder(x);
		}
		qhead = trail_lim[level];
		trail.shrink(trail.size() - trail_lim[level]);
		trail_lim.shrink(trail_lim.size() - level);
	}
}

//=================================================================================================
// Major methods:

inline Lit Solver::pickBranchLit()
{
	Var next = var_Undef;

	// Random decision:
	if (drand(random_seed) < random_var_freq && !order_heap.empty()) {
		next = order_heap[irand(random_seed, order_heap.size())];
		if (value(next) == l_Undef && decision[next])
			rnd_decisions++;
	}

	// Activity based decision:
	while (next == var_Undef || value(next) != l_Undef || !decision[next])
		if (order_heap.empty()) {
			next = var_Undef;
			break;
		} else
			next = order_heap.removeMin();

	return next == var_Undef ?
	           lit_Undef :
	           mkLit(next, rnd_pol ? drand(random_seed) < 0.5 : (polarity[next] != 0));
}

/*_________________________________________________________________________________________________
|
|  analyze : (confl : Clause*) (out_learnt : vec<Lit>&) (out_btlevel : int&)  ->  [void]
|
|  Description:
|    Analyze conflict and produce a reason clause.
|
|    Pre-conditions:
|      * 'out_learnt' is assumed to be cleared.
|      * Current decision level must be greater than root level.
|
|    Post-conditions:
|      * 'out_learnt[0]' is the asserting literal at level 'out_btlevel'.
|      * If out_learnt.size() > 1 then 'out_learnt[1]' has the greatest decision level of the
|        rest of literals. There may be others from the same level though.
|
|________________________________________________________________________________________________@*/
inline void Solver::analyze(CRef confl, vec<Lit>& out_learnt, vec<Lit>& selectors, int& out_btlevel,
                            unsigned int& lbd, unsigned int& szWithoutSelectors)
{
	int pathC = 0;
	Lit p = lit_Undef;

	// Generate conflict clause:
	//
	out_learnt.push(); // (leave room for the asserting literal)
	int index = trail.size() - 1;

	do {
		assert(confl != CRef_Undef); // (otherwise should be UIP)
		Clause& c = ca[confl];

		// Special case for binary clauses
		// The first one has to be SAT
		if (p != lit_Undef && c.size() == 2 && value(c[0]) == l_False) {

			assert(value(c[1]) == l_True);
			Lit tmp = c[0];
			c[0] = c[1], c[1] = tmp;
		}

		if (c.learnt())
			claBumpActivity(c);

#ifdef DYNAMICNBLEVEL
		// DYNAMIC NBLEVEL trick (see competition'09 companion paper)
		if (c.learnt() && c.lbd() > 2) {
			unsigned int nblevels = computeLBD(c);
			if (nblevels + 1 < c.lbd()) { // improve the LBD
				if (c.lbd() <= lbLBDFrozenClause) {
					c.setCanBeDel(false);
				}
				// seems to be interesting : keep it for the next round
				c.setLBD(nblevels); // Update it
			}
		}
#endif

		for (int j = (p == lit_Undef) ? 0 : 1; j < c.size(); j++) {
			Lit q = c[j];

			if (!seen[var(q)] && level(var(q)) > 0) {
				if (!isSelector(var(q)))
					varBumpActivity(var(q));
				seen[var(q)] = 1;
				if (level(var(q)) >= decisionLevel()) {
					pathC++;
#ifdef UPDATEVARACTIVITY
					// UPDATEVARACTIVITY trick (see competition'09 companion paper)
					if (!isSelector(var(q)) && (reason(var(q)) != CRef_Undef)
					    && ca[reason(var(q))].learnt())
						lastDecisionLevel.push(q);
#endif

				} else {
					if (isSelector(var(q))) {
						assert(value(q) == l_False);
						selectors.push(q);
					} else
						out_learnt.push(q);
				}
			}
		}

		// Select next clause to look at:
		while (!seen[var(trail[index--])])
			;
		p = trail[index + 1];
		confl = reason(var(p));
		seen[var(p)] = 0;
		pathC--;

	} while (pathC > 0);
	out_learnt[0] = ~p;

	// Simplify conflict clause:
	//
	int i, j;

	for (i = 0; i < selectors.size(); i++)
		out_learnt.push(selectors[i]);

	out_learnt.copyTo(analyze_toclear);
	if (ccmin_mode == 2) {
		uint32_t abstract_level = 0;
		for (i = 1; i < out_learnt.size(); i++)
			abstract_level |= abstractLevel(var(
			    out_learnt[i])); // (maintain an abstraction of levels involved in conflict)

		for (i = j = 1; i < out_learnt.size(); i++)
			if (reason(var(out_learnt[i])) == CRef_Undef
			    || !litRedundant(out_learnt[i], abstract_level))
				out_learnt[j++] = out_learnt[i];

	} else if (ccmin_mode == 1) {
		for (i = j = 1; i < out_learnt.size(); i++) {
			Var x = var(out_learnt[i]);

			if (reason(x) == CRef_Undef)
				out_learnt[j++] = out_learnt[i];
			else {
				Clause& c = ca[reason(var(out_learnt[i]))];
				// Thanks to Siert Wieringa for this bug fix!
				for (int k = ((c.size() == 2) ? 0 : 1); k < c.size(); k++)
					if (!seen[var(c[k])] && level(var(c[k])) > 0) {
						out_learnt[j++] = out_learnt[i];
						break;
					}
			}
		}
	} else
		i = j = out_learnt.size();

	max_literals += out_learnt.size();
	out_learnt.shrink(i - j);
	tot_literals += out_learnt.size();

	/* ***************************************
	  Minimisation with binary clauses of the asserting clause
	  First of all : we look for small clauses
	  Then, we reduce clauses with small LBD.
	  Otherwise, this can be useless
	 */
	if (!incremental && out_learnt.size() <= lbSizeMinimizingClause) {
		minimisationWithBinaryResolution(out_learnt);
	}
	// Find correct backtrack level:
	//
	if (out_learnt.size() == 1)
		out_btlevel = 0;
	else {
		int max_i = 1;
		// Find the first literal assigned at the next-highest level:
		for (int i = 2; i < out_learnt.size(); i++)
			if (level(var(out_learnt[i])) > level(var(out_learnt[max_i])))
				max_i = i;
		// Swap-in this literal at index 1:
		Lit p = out_learnt[max_i];
		out_learnt[max_i] = out_learnt[1];
		out_learnt[1] = p;
		out_btlevel = level(var(p));
	}

	// Compute the size of the clause without selectors (incremental mode)
	if (incremental) {
		szWithoutSelectors = 0;
		for (int i = 0; i < out_learnt.size(); i++) {
			if (!isSelector(var((out_learnt[i]))))
				szWithoutSelectors++;
			else if (i > 0)
				break;
		}
	} else
		szWithoutSelectors = out_learnt.size();

	// Compute LBD
	lbd = computeLBD(out_learnt, out_learnt.size() - selectors.size());

#ifdef UPDATEVARACTIVITY
	// UPDATEVARACTIVITY trick (see competition'09 companion paper)
	if (lastDecisionLevel.size() > 0) {
		for (int i = 0; i < lastDecisionLevel.size(); i++) {
			if (ca[reason(var(lastDecisionLevel[i]))].lbd() < lbd)
				varBumpActivity(var(lastDecisionLevel[i]));
		}
		lastDecisionLevel.clear();
	}
#endif

	for (j = 0; j < analyze_toclear.size(); j++)
		seen[var(analyze_toclear[j])] = 0; // ('seen[]' is now cleared)
	for (j = 0; j < selectors.size(); j++)
		seen[var(selectors[j])] = 0;
}

// Check if 'p' can be removed. 'abstract_levels' is used to abort early if the algorithm is
// visiting literals at levels that cannot be removed later.
inline bool Solver::litRedundant(Lit p, uint32_t abstract_levels)
{
	analyze_stack.clear();
	analyze_stack.push(p);
	int top = analyze_toclear.size();
	while (analyze_stack.size() > 0) {
		assert(reason(var(analyze_stack.last())) != CRef_Undef);
		Clause& c = ca[reason(var(analyze_stack.last()))];
		analyze_stack.pop();
		if (c.size() == 2 && value(c[0]) == l_False) {
			assert(value(c[1]) == l_True);
			Lit tmp = c[0];
			c[0] = c[1], c[1] = tmp;
		}

		for (int i = 1; i < c.size(); i++) {
			Lit p = c[i];
			if (!seen[var(p)] && level(var(p)) > 0) {
				if (reason(var(p)) != CRef_Undef
				    && (abstractLevel(var(p)) & abstract_levels) != 0) {
					seen[var(p)] = 1;
					analyze_stack.push(p);
					analyze_toclear.push(p);
				} else {
					for (int j = top; j < analyze_toclear.size(); j++)
						seen[var(analyze_toclear[j])] = 0;
					analyze_toclear.shrink(analyze_toclear.size() - top);
					return false;
				}
			}
		}
	}

	return true;
}

/*_________________________________________________________________________________________________
|
|  analyzeFinal : (p : Lit)  ->  [void]
|
|  Description:
|    Specialized analysis procedure to express the final conflict in terms of assumptions.
|    Calculates the (possibly empty) set of assumptions that led to the assignment of 'p', and
|    stores the result in 'out_conflict'.
|________________________________________________________________________________________________@*/
inline void Solver::analyzeFinal(Lit p, vec<Lit>& out_conflict)
{
	out_conflict.clear();
	out_conflict.push(p);

	if (decisionLevel() == 0)
		return;

	seen[var(p)] = 1;

	for (int i = trail.size() - 1; i >= trail_lim[0]; i--) {
		Var x = var(trail[i]);
		if (seen[x]) {
			if (reason(x) == CRef_Undef) {
				assert(level(x) > 0);
				out_conflict.push(~trail[i]);
			} else {
				Clause& c = ca[reason(x)];
				//                for (int j = 1; j < c.size(); j++) Minisat (glucose 2.0) loop
				// Bug in case of assumptions due to special data structures for Binary.
				// Many thanks to Sam Bayless (sbayless@cs.ubc.ca) for discover this bug.
				for (int j = ((c.size() == 2) ? 0 : 1); j < c.size(); j++)
					if (level(var(c[j])) > 0)
						seen[var(c[j])] = 1;
			}

			seen[x] = 0;
		}
	}

	seen[var(p)] = 0;
}

inline void Solver::uncheckedEnqueue(Lit p, CRef from)
{
	assert(value(p) == l_Undef);
	assigns[var(p)] = lbool(!sign(p));
	vardata[var(p)] = mkVarData(from, decisionLevel());
	trail.push_(p);
}

/*_________________________________________________________________________________________________
|
|  propagate : [void]  ->  [Clause*]
|
|  Description:
|    Propagates all enqueued facts. If a conflict arises, the conflicting clause is returned,
|    otherwise CRef_Undef.
|
|    Post-conditions:
|      * the propagation queue is empty, even if there was a conflict.
|________________________________________________________________________________________________@*/
inline CRef Solver::propagate()
{
	CRef confl = CRef_Undef;
	int num_props = 0;
	watches.cleanAll();
	watchesBin.cleanAll();
	while (qhead < trail.size()) {
		Lit p = trail[qhead++]; // 'p' is enqueued fact to propagate.
		vec<Watcher>& ws = watches[p];
		Watcher *i, *j, *end;
		num_props++;

		// First, Propagate binary clauses
		vec<Watcher>& wbin = watchesBin[p];
		for (int k = 0; k < wbin.size(); k++) {
			Lit imp = wbin[k].blocker;
			if (value(imp) == l_False) {
				return wbin[k].cref;
			}
			if (value(imp) == l_Undef) {
				uncheckedEnqueue(imp, wbin[k].cref);
			}
		}

		for (i = j = (Watcher*) ws, end = i + ws.size(); i != end;) {
			// Try to avoid inspecting the clause:
			Lit blocker = i->blocker;
			if (value(blocker) == l_True) {
				*j++ = *i++;
				continue;
			}

			// Make sure the false literal is data[1]:
			CRef cr = i->cref;
			Clause& c = ca[cr];
			Lit false_lit = ~p;
			if (c[0] == false_lit)
				c[0] = c[1], c[1] = false_lit;
			assert(c[1] == false_lit);
			i++;

			// If 0th watch is true, then clause is already satisfied.
			Lit first = c[0];
			Watcher w = Watcher(cr, first);
			if (first != blocker && value(first) == l_True) {
				*j++ = w;
				continue;
			}

			// Look for new watch:
			if (incremental) { // ----------------- INCREMENTAL MODE
				int choosenPos = -1;
				for (int k = 2; k < c.size(); k++) {

					if (value(c[k]) != l_False) {
						if (decisionLevel() > assumptions.size()) {
							choosenPos = k;
							break;
						} else {
							choosenPos = k;

							if (value(c[k]) == l_True
							    || !isSelector(var(c[k]))) {
								break;
							}
						}
					}
				}
				if (choosenPos != -1) {
					c[1] = c[choosenPos];
					c[choosenPos] = false_lit;
					watches[~c[1]].push(w);
					goto NextClause;
				}
			} else { // ----------------- DEFAULT  MODE (NOT INCREMENTAL)
				for (int k = 2; k < c.size(); k++) {

					if (value(c[k]) != l_False) {
						c[1] = c[k];
						c[k] = false_lit;
						watches[~c[1]].push(w);
						goto NextClause;
					}
				}
			}

			// Did not find watch -- clause is unit under assignment:
			*j++ = w;
			if (value(first) == l_False) {
				confl = cr;
				qhead = trail.size();
				// Copy the remaining watches:
				while (i < end)
					*j++ = *i++;
			} else {
				uncheckedEnqueue(first, cr);
			}
		NextClause:;
		}
		ws.shrink(i - j);
	}
	propagations += num_props;
	simpDB_props -= num_props;

	return confl;
}

/*_________________________________________________________________________________________________
|
|  reduceDB : ()  ->  [void]
|
|  Description:
|    Remove half of the learnt clauses, minus the clauses locked by the current assignment. Locked
|    clauses are clauses that are reason to some assignment. Binary clauses are never removed.
|________________________________________________________________________________________________@*/
struct reduceDB_lt {
	ClauseAllocator& ca;
	reduceDB_lt(ClauseAllocator& ca_)
	    : ca(ca_)
	{}
	bool operator()(CRef x, CRef y)
	{

		// Main criteria... Like in MiniSat we keep all binary clauses
		if (ca[x].size() > 2 && ca[y].size() == 2)
			return 1;

		if (ca[y].size() > 2 && ca[x].size() == 2)
			return 0;
		if (ca[x].size() == 2 && ca[y].size() == 2)
			return 0;

		// Second one  based on literal block distance
		if (ca[x].lbd() > ca[y].lbd())
			return 1;
		if (ca[x].lbd() < ca[y].lbd())
			return 0;

		// Finally we can use old activity or size, we choose the last one
		return ca[x].activity() < ca[y].activity();
		// return x->size() < y->size();
		// return ca[x].size() > 2 && (ca[y].size() == 2 || ca[x].activity() < ca[y].activity()); }
	}
};

inline void Solver::reduceDB()
{
	int i, j;
	nbReduceDB++;
	sort(learnts, reduceDB_lt(ca));

	// We have a lot of "good" clauses, it is difficult to compare them. Keep more !
	if (ca[learnts[learnts.size() / RATIOREMOVECLAUSES]].lbd() <= 3)
		nbclausesbeforereduce += specialIncReduceDB;
	// Useless :-)
	if (ca[learnts.last()].lbd() <= 5)
		nbclausesbeforereduce += specialIncReduceDB;

	// Don't delete binary or locked clauses. From the rest, delete clauses from the first half
	// Keep clauses which seem to be usefull (their lbd was reduce during this sequence)

	int limit = learnts.size() / 2;
	for (i = j = 0; i < learnts.size(); i++) {
		Clause& c = ca[learnts[i]];
		if (c.lbd() > 2 && c.size() > 2 && c.canBeDel() && !locked(c) && (i < limit)) {
			removeClause(learnts[i]);
			nbRemovedClauses++;
		} else {
			if (!c.canBeDel())
				limit++;     // we keep c, so we can delete an other clause
			c.setCanBeDel(true); // At the next step, c can be delete
			learnts[j++] = learnts[i];
		}
	}
	learnts.shrink(i - j);
	checkGarbage();
}

inline void Solver::removeSatisfied(vec<CRef>& cs)
{
	int i, j;
	for (i = j = 0; i < cs.size(); i++) {
		Clause& c = ca[cs[i]];
		if (satisfied(c))
			removeClause(cs[i]);
		else
			cs[j++] = cs[i];
	}
	cs.shrink(i - j);
}

inline void Solver::rebuildOrderHeap()
{
	vec<Var> vs;
	for (Var v = 0; v < nVars(); v++)
		if (decision[v] && value(v) == l_Undef)
			vs.push(v);
	order_heap.build(vs);
}

/*_________________________________________________________________________________________________
|
|  simplify : [void]  ->  [bool]
|
|  Description:
|    Simplify the clause database according to the current top-level assigment. Currently, the only
|    thing done here is the removal of satisfied clauses, but more things can be put here.
|________________________________________________________________________________________________@*/
inline bool Solver::simplify()
{
	assert(decisionLevel() == 0);

	if (!ok || propagate() != CRef_Undef)
		return ok = false;

	if (nAssigns() == simpDB_assigns || (simpDB_props > 0))
		return true;

	// Remove satisfied clauses:
	removeSatisfied(learnts);
	if (remove_satisfied) // Can be turned off.
		removeSatisfied(clauses);

	checkGarbage();

	rebuildOrderHeap();

	simpDB_assigns = nAssigns();
	simpDB_props = clauses_literals
	               + learnts_literals; // (shouldn't depend on stats really, but it will do for now)

	return true;
}

/*_________________________________________________________________________________________________
|
|  search : (nof_conflicts : int) (params : const SearchParams&)  ->  [lbool]
|
|  Description:
|    Search for a model the specified number of conflicts.
|    NOTE! Use negative value for 'nof_conflicts' indicate infinity.
|
|  Output:
|    'l_True' if a partial assigment that is consistent with respect to the clauseset is found. If
|    all variables are decision variables, this means that the clause set is satisfiable. 'l_False'
|    if the clause set is unsatisfiable. 'l_Undef' if the bound on number of conflicts is reached.
|________________________________________________________________________________________________@*/
inline lbool Solver::search(int nof_conflicts)
{
	assert(ok);
	int backtrack_level;
	int conflictC = 0;
	vec<Lit> learnt_clause, selectors;
	unsigned int nblevels, szWoutSelectors;
	bool blocked = false;
	starts++;
	for (;;) {
		CRef confl = propagate();
		if (confl != CRef_Undef) {
			// CONFLICT
			conflicts++;
			conflictC++;
			conflictsRestarts++;
			if (conflicts % 5000 == 0 && var_decay < 0.95)
				var_decay += 0.01;

			if (verbosity >= 1 && conflicts % verbEveryConflicts == 0) {
				printf("c | %8d   %7d    %5d | %7d %8d %8d | %5d %8d   %6d %8d | "
				       "%6.3f %% |\n",
				       (int) starts, (int) nbstopsrestarts,
				       (int) (conflicts / starts),
				       (int) dec_vars
				           - (trail_lim.size() == 0 ? trail.size() : trail_lim[0]),
				       nClauses(), (int) clauses_literals, (int) nbReduceDB,
				       nLearnts(), (int) nbDL2, (int) nbRemovedClauses,
				       progressEstimate() * 100);
			}
			if (decisionLevel() == 0) {
				return l_False;
			}

			trailQueue.push(trail.size());
			// BLOCK RESTART (CP 2012 paper)
			if (conflictsRestarts > LOWER_BOUND_FOR_BLOCKING_RESTART
			    && lbdQueue.isvalid() && trail.size() > R * trailQueue.getavg()) {
				lbdQueue.fastclear();
				nbstopsrestarts++;
				if (!blocked) {
					lastblockatrestart = starts;
					nbstopsrestartssame++;
					blocked = true;
				}
			}

			learnt_clause.clear();
			selectors.clear();
			analyze(confl, learnt_clause, selectors, backtrack_level, nblevels,
			        szWoutSelectors);

			lbdQueue.push(nblevels);
			sumLBD += nblevels;

			cancelUntil(backtrack_level);

			if (certifiedUNSAT) {
				for (int i = 0; i < learnt_clause.size(); i++)
					fprintf(certifiedOutput, "%i ",
					        (var(learnt_clause[i]) + 1)
					            * (-2 * sign(learnt_clause[i]) + 1));
				fprintf(certifiedOutput, "0\n");
			}

			if (learnt_clause.size() == 1) {
				uncheckedEnqueue(learnt_clause[0]);
				nbUn++;
			} else {
				CRef cr = ca.alloc(learnt_clause, true);
				ca[cr].setLBD(nblevels);
				ca[cr].setSizeWithoutSelectors(szWoutSelectors);
				if (nblevels <= 2)
					nbDL2++; // stats
				if (ca[cr].size() == 2)
					nbBin++; // stats
				learnts.push(cr);
				attachClause(cr);

				claBumpActivity(ca[cr]);
				uncheckedEnqueue(learnt_clause[0], cr);
			}
			varDecayActivity();
			claDecayActivity();

		} else {

			// Our dynamic restart, see the SAT09 competition compagnion paper
			if ((conflictsRestarts && lbdQueue.isvalid()
			     && lbdQueue.getavg() * K > sumLBD / conflictsRestarts)
			    || (pstop && *pstop)) {
				lbdQueue.fastclear();
				progress_estimate = progressEstimate();
				int bt = 0;
				if (incremental) { // DO NOT BACKTRACK UNTIL 0.. USELESS
					bt = (decisionLevel() < assumptions.size()) ?
					         decisionLevel() :
					         assumptions.size();
				}
				cancelUntil(bt);
				return l_Undef;
			}

			// Simplify the set of problem clauses:
			if (decisionLevel() == 0 && !simplify()) {
				return l_False;
			}
			// Perform clause database reduction !
			if (conflicts >= curRestart * nbclausesbeforereduce) {

				assert(learnts.size() > 0);
				curRestart = (conflicts / nbclausesbeforereduce) + 1;
				reduceDB();
				nbclausesbeforereduce += incReduceDB;
			}

			Lit next = lit_Undef;
			while (decisionLevel() < assumptions.size()) {
				// Perform user provided assumption:
				Lit p = assumptions[decisionLevel()];
				if (value(p) == l_True) {
					// Dummy decision level:
					newDecisionLevel();
				} else if (value(p) == l_False) {
					analyzeFinal(~p, conflict);
					return l_False;
				} else {
					next = p;
					break;
				}
			}

			if (next == lit_Undef) {
				// New variable decision:
				decisions++;
				next = pickBranchLit();

				if (next == lit_Undef) {
					// printf("c last restart ## conflicts  :  %d %d
					// \n",conflictC,decisionLevel());
					// Model found:
					return l_True;
				}
			}

			// Increase decision level and enqueue 'next'
			newDecisionLevel();
			uncheckedEnqueue(next);
		}
	}
}

inline double Solver::progressEstimate() const
{
	double progress = 0;
	double F = 1.0 / nVars();

	for (int i = 0; i <= decisionLevel(); i++) {
		int beg = i == 0 ? 0 : trail_lim[i - 1];
		int end = i == decisionLevel() ? trail.size() : trail_lim[i];
		progress += pow(F, i) * (end - beg);
	}

	return progress / nVars();
}

inline void Solver::printIncrementalStats()
{

	printf("c---------- Glucose Stats -------------------------\n");
	printf("c restarts              : %lld\n", starts);
	printf("c nb ReduceDB           : %lld\n", nbReduceDB);
	printf("c nb removed Clauses    : %lld\n", nbRemovedClauses);
	printf("c nb learnts DL2        : %lld\n", nbDL2);
	printf("c nb learnts size 2     : %lld\n", nbBin);
	printf("c nb learnts size 1     : %lld\n", nbUn);

	printf("c conflicts             : %lld\n", conflicts);
	printf("c decisions             : %lld\n", decisions);
	printf("c propagations          : %lld\n", propagations);

	printf("c SAT Calls             : %d in %g seconds\n", nbSatCalls, totalTime4Sat);
	printf("c UNSAT Calls           : %d in %g seconds\n", nbUnsatCalls, totalTime4Unsat);
	printf("c--------------------------------------------------\n");
}

// NOTE: assumptions passed in member-variable 'assumptions'.
inline lbool Solver::solve_()
{

	if (incremental && certifiedUNSAT) {
		printf("Can not use incremental and certified unsat in the same time\n");
		exit(-1);
	}
	model.clear();
	conflict.clear();
	if (!ok)
		return l_False;
	double curTime = cpuTime();

	solves++;

	lbool status = l_Undef;
	if (!incremental && verbosity >= 1) {
		printf("c ========================================[ MAGIC CONSTANTS "
		       "]==============================================\n");
		printf("c | Constants are supposed to work well together :-)                       "
		       "                               |\n");
		printf("c | however, if you find better choices, please let us known...            "
		       "                               |\n");
		printf("c "
		       "|--------------------------------------------------------------------------"
		       "-----------------------------|\n");
		printf("c |                                |                                |      "
		       "                               |\n");
		printf("c | - Restarts:                    | - Reduce Clause DB:            | - "
		       "Minimize Asserting:               |\n");
		printf("c |   * LBD Queue    : %6d      |   * First     : %6d         |    * size "
		       "< %3d                     |\n",
		       lbdQueue.maxSize(), nbclausesbeforereduce, lbSizeMinimizingClause);
		printf("c |   * Trail  Queue : %6d      |   * Inc       : %6d         |    * lbd  "
		       "< %3d                     |\n",
		       trailQueue.maxSize(), incReduceDB, lbLBDMinimizingClause);
		printf("c |   * K            : %6.2f      |   * Special   : %6d         |          "
		       "                           |\n",
		       K, specialIncReduceDB);
		printf("c |   * R            : %6.2f      |   * Protected :  (lbd)< %2d     |      "
		       "                               |\n",
		       R, lbLBDFrozenClause);
		printf("c |                                |                                |      "
		       "                               |\n");
		printf("c ==================================[ Search Statistics (every %6d "
		       "conflicts) ]=========================\n",
		       verbEveryConflicts);
		printf("c |                                                                        "
		       "                               |\n");

		printf("c |          RESTARTS           |          ORIGINAL         |              "
		       "LEARNT              | Progress |\n");
		printf("c |       NB   Blocked  Avg Cfc |    Vars  Clauses Literals |   Red   "
		       "Learnts    LBD2  Removed |          |\n");
		printf("c "
		       "==========================================================================="
		       "==============================\n");
	}

	// Search:
	int curr_restarts = 0;
	while (status == l_Undef) {
		status = search(0); // the parameter is useless in glucose, kept to allow modifications
		if (!withinBudget() || terminate_search_early || (pstop && *pstop))
			break;
		if (nRuntimeLimit && Abc_Clock() > nRuntimeLimit)
			break;
		curr_restarts++;
	}

	if (!incremental && verbosity >= 1)
		printf("c "
		       "==========================================================================="
		       "==============================\n");

	if (certifiedUNSAT) { // Want certified output
		if (status == l_False)
			fprintf(certifiedOutput, "0\n");
		fclose(certifiedOutput);
	}

	if (status == l_True) {
		// Extend & copy model:
		model.growTo(nVars());
		for (int i = 0; i < nVars(); i++)
			model[i] = value(i);
	} else if (status == l_False && conflict.size() == 0)
		ok = false;

	cancelUntil(0);

	double finalTime = cpuTime();
	if (status == l_True) {
		nbSatCalls++;
		totalTime4Sat += (finalTime - curTime);
	}
	if (status == l_False) {
		nbUnsatCalls++;
		totalTime4Unsat += (finalTime - curTime);
	}

	// ABC callback
	if (pCnfFunc
	    && !terminate_search_early) { // hack to avoid calling callback twise if the solver was terminated early
		int* pCex = NULL;
		int message = (status == l_True ? 1 : status == l_False ? 0 : -1);
		if (status == l_True) {
			pCex = new int[nVars()];
			for (int i = 0; i < nVars(); i++)
				pCex[i] = (model[i] == l_True);
		}

		int callback_result = pCnfFunc(pCnfMan, message, pCex);
		assert(callback_result == 0);
	} else if (pCnfFunc)
		terminate_search_early = false; // for next run

	return status;
}

//=================================================================================================
// Writing CNF to DIMACS:
//
// FIXME: this needs to be rewritten completely.

static Var mapVar(Var x, vec<Var>& map, Var& max)
{
	if (map.size() <= x || map[x] == -1) {
		map.growTo(x + 1, -1);
		map[x] = max++;
	}
	return map[x];
}

inline void Solver::toDimacs(FILE* f, Clause& c, vec<Var>& map, Var& max)
{
	if (satisfied(c))
		return;

	for (int i = 0; i < c.size(); i++)
		if (value(c[i]) != l_False)
			fprintf(f, "%s%d ", sign(c[i]) ? "-" : "", mapVar(var(c[i]), map, max) + 1);
	fprintf(f, "0\n");
}

inline void Solver::toDimacs(const char* file, const vec<Lit>& assumps)
{
	FILE* f = fopen(file, "wr");
	if (f == NULL)
		fprintf(stderr, "could not open file %s\n", file), exit(1);
	toDimacs(f, assumps);
	fclose(f);
}

inline void Solver::toDimacs(FILE* f, const vec<Lit>& assumps)
{
	// Handle case when solver is in contradictory state:
	if (!ok) {
		fprintf(f, "p cnf 1 2\n1 0\n-1 0\n");
		return;
	}

	vec<Var> map;
	Var max = 0;

	// Cannot use removeClauses here because it is not safe
	// to deallocate them at this point. Could be improved.
	int i, cnt = 0;
	for (i = 0; i < clauses.size(); i++)
		if (!satisfied(ca[clauses[i]]))
			cnt++;

	for (i = 0; i < clauses.size(); i++)
		if (!satisfied(ca[clauses[i]])) {
			Clause& c = ca[clauses[i]];
			for (int j = 0; j < c.size(); j++)
				if (value(c[j]) != l_False)
					mapVar(var(c[j]), map, max);
		}

	// Assumptions are added as unit clauses:
	cnt += assumptions.size();

	fprintf(f, "p cnf %d %d\n", max, cnt);

	for (i = 0; i < assumptions.size(); i++) {
		assert(value(assumptions[i]) != l_False);
		fprintf(f, "%s%d 0\n", sign(assumptions[i]) ? "-" : "",
		        mapVar(var(assumptions[i]), map, max) + 1);
	}

	for (i = 0; i < clauses.size(); i++)
		toDimacs(f, ca[clauses[i]], map, max);

	if (verbosity > 0)
		printf("Wrote %d clauses with %d variables.\n", cnt, max);
}

//=================================================================================================
// Garbage Collection methods:

inline void Solver::relocAll(ClauseAllocator& to)
{
	int v, s, i, j;
	// All watchers:
	//
	// for (int i = 0; i < watches.size(); i++)
	watches.cleanAll();
	watchesBin.cleanAll();
	for (v = 0; v < nVars(); v++)
		for (s = 0; s < 2; s++) {
			Lit p = mkLit(v, s != 0);
			// printf(" >>> RELOCING: %s%d\n", sign(p)?"-":"", var(p)+1);
			vec<Watcher>& ws = watches[p];
			for (j = 0; j < ws.size(); j++)
				ca.reloc(ws[j].cref, to);
			vec<Watcher>& ws2 = watchesBin[p];
			for (j = 0; j < ws2.size(); j++)
				ca.reloc(ws2[j].cref, to);
		}

	// All reasons:
	//
	for (i = 0; i < trail.size(); i++) {
		Var v = var(trail[i]);

		if (reason(v) != CRef_Undef && (ca[reason(v)].reloced() || locked(ca[reason(v)])))
			ca.reloc(vardata[v].reason, to);
	}

	// All learnt:
	//
	for (i = 0; i < learnts.size(); i++)
		ca.reloc(learnts[i], to);

	// All original:
	//
	for (i = 0; i < clauses.size(); i++)
		ca.reloc(clauses[i], to);
}

inline void Solver::garbageCollect()
{
	// Initialize the next region to a size corresponding to the estimated utilization degree.
	// This is not precise but should avoid some unnecessary reallocations for the new region:
	ClauseAllocator to(ca.size() - ca.wasted());

	relocAll(to);
	if (verbosity >= 2)
		printf("|  Garbage collection:   %12d bytes => %12d bytes             |\n",
		       ca.size() * ClauseAllocator::Unit_Size,
		       to.size() * ClauseAllocator::Unit_Size);
	to.moveTo(ca);
}

inline void Solver::reset()
{
	// Reset everything
	ok = true;
	K = (double) opt_K;
	R = (double) opt_R;
	firstReduceDB = opt_first_reduce_db;
	var_decay = (double) opt_var_decay;
	// max_var_decay = opt_max_var_decay;
	solves = starts = decisions = propagations = conflicts = conflictsRestarts = 0;
	curRestart = 1;
	cla_inc = var_inc = 1;
	watches.clear(false); // We don't free the memory, new calls should be of the same size order.
	watchesBin.clear(false);
	// unaryWatches.clear(false);
	qhead = 0;
	simpDB_assigns = -1;
	simpDB_props = 0;
	order_heap.clear(false);
	progress_estimate = 0;
	// lastLearntClause = CRef_Undef;
	conflict_budget = -1;
	propagation_budget = -1;
	nbVarsInitialFormula = INT32_MAX;
	totalTime4Sat = 0.;
	totalTime4Unsat = 0.;
	nbSatCalls = nbUnsatCalls = 0;
	MYFLAG = 0;
	lbdQueue.clear(false);
	lbdQueue.initSize(sizeLBDQueue);
	trailQueue.clear(false);
	trailQueue.initSize(sizeTrailQueue);
	sumLBD = 0;
	nbclausesbeforereduce = firstReduceDB;
	// stats.clear();
	// stats.growTo(coreStatsSize, 0);
	clauses.clear(false);
	learnts.clear(false);
	// permanentLearnts.clear(false);
	// unaryWatchedClauses.clear(false);
	model.clear(false);
	conflict.clear(false);
	activity.clear(false);
	assigns.clear(false);
	polarity.clear(false);
	// forceUNSAT.clear(false);
	decision.clear(false);
	trail.clear(false);
	nbpos.clear(false);
	trail_lim.clear(false);
	vardata.clear(false);
	assumptions.clear(false);
	permDiff.clear(false);
	lastDecisionLevel.clear(false);
	ca.clear();
	seen.clear(false);
	analyze_stack.clear(false);
	analyze_toclear.clear(false);
	add_tmp.clear(false);
	assumptionPositions.clear(false);
	initialPositions.clear(false);
}

} /* namespace Gluco */

ABC_NAMESPACE_IMPL_END

/*** AbcGlucose.cpp */

/**CFile****************************************************************

  FileName    [AbcGlucose.cpp]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT solver Glucose 3.0 by Gilles Audemard and Laurent Simon.]

  Synopsis    [Interface to Glucose.]

  Author      [Alan Mishchenko]

  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 6, 2017.]

  Revision    [$Id: AbcGlucose.cpp,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "abc/AbcGlucose.h"
#include "abc/Dimacs.h"
#include "abc/SimpSolver.h"
#include "abc/abc_global.h"
#include "abc/system.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define USE_SIMP_SOLVER 1

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

#ifdef USE_SIMP_SOLVER

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
inline Gluco::SimpSolver* glucose_solver_start()
{
	Gluco::SimpSolver* S = new Gluco::SimpSolver;
	S->setIncrementalMode();
	return S;
}

inline void glucose_solver_stop(Gluco::SimpSolver* S)
{
	delete S;
}

inline void glucose_solver_reset(Gluco::SimpSolver* S)
{
	S->reset();
}

inline int glucose_solver_addclause(Gluco::SimpSolver* S, int* plits, int nlits)
{
	Gluco::vec<Gluco::Lit> lits;
	for (int i = 0; i < nlits; i++, plits++) {
		// note: Glucose uses the same var->lit conventiaon as ABC
		while ((*plits) / 2 >= S->nVars())
			S->newVar();
		assert((*plits) / 2
		       < S->nVars()); // NOTE: since we explicitely use new function bmc_add_var
		Gluco::Lit p;
		p.x = *plits;
		lits.push(p);
	}
	return S->addClause(lits); // returns 0 if the problem is UNSAT
}

inline void glucose_solver_setcallback(Gluco::SimpSolver* S, void* pman,
                                       int (*pfunc)(void*, int, int*))
{
	S->pCnfMan = pman;
	S->pCnfFunc = pfunc;
	S->nCallConfl = 1000;
}

inline int glucose_solver_solve(Gluco::SimpSolver* S, int* plits, int nlits)
{
	Gluco::vec<Gluco::Lit> lits;
	for (int i = 0; i < nlits; i++, plits++) {
		Gluco::Lit p;
		p.x = *plits;
		lits.push(p);
	}
	Gluco::lbool res = S->solveLimited(lits, 0);
	return (res == Gluco::l_True ? 1 : res == Gluco::l_False ? -1 : 0);
}

inline int glucose_solver_addvar(Gluco::SimpSolver* S)
{
	S->newVar();
	return S->nVars() - 1;
}

inline int glucose_solver_read_cex_varvalue(Gluco::SimpSolver* S, int ivar)
{
	return S->model[ivar] == Gluco::l_True;
}

inline void glucose_solver_setstop(Gluco::SimpSolver* S, int* pstop)
{
	S->pstop = pstop;
}

/**Function*************************************************************

  Synopsis    [Wrapper APIs to calling from ABC.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
inline bmcg_sat_solver* bmcg_sat_solver_start()
{
	return (bmcg_sat_solver*) glucose_solver_start();
}
inline void bmcg_sat_solver_stop(bmcg_sat_solver* s)
{
	glucose_solver_stop((Gluco::SimpSolver*) s);
}
inline void bmcg_sat_solver_reset(bmcg_sat_solver* s)
{
	glucose_solver_reset((Gluco::SimpSolver*) s);
}

inline int bmcg_sat_solver_addclause(bmcg_sat_solver* s, int* plits, int nlits)
{
	return glucose_solver_addclause((Gluco::SimpSolver*) s, plits, nlits);
}

inline void bmcg_sat_solver_setcallback(bmcg_sat_solver* s, void* pman,
                                        int (*pfunc)(void*, int, int*))
{
	glucose_solver_setcallback((Gluco::SimpSolver*) s, pman, pfunc);
}

inline int bmcg_sat_solver_solve(bmcg_sat_solver* s, int* plits, int nlits)
{
	return glucose_solver_solve((Gluco::SimpSolver*) s, plits, nlits);
}

inline int bmcg_sat_solver_final(bmcg_sat_solver* s, int** ppArray)
{
	*ppArray = (int*) (Gluco::Lit*) ((Gluco::SimpSolver*) s)->conflict;
	return ((Gluco::SimpSolver*) s)->conflict.size();
}

inline int bmcg_sat_solver_addvar(bmcg_sat_solver* s)
{
	return glucose_solver_addvar((Gluco::SimpSolver*) s);
}

inline void bmcg_sat_solver_set_nvars(bmcg_sat_solver* s, int nvars)
{
	int i;
	for (i = bmcg_sat_solver_varnum(s); i < nvars; i++)
		bmcg_sat_solver_addvar(s);
}

inline int bmcg_sat_solver_eliminate(bmcg_sat_solver* s, int turn_off_elim)
{
	//    return 1;
	return ((Gluco::SimpSolver*) s)->eliminate(turn_off_elim != 0);
}

inline int bmcg_sat_solver_var_is_elim(bmcg_sat_solver* s, int v)
{
	//    return 0;
	return ((Gluco::SimpSolver*) s)->isEliminated(v);
}

inline void bmcg_sat_solver_var_set_frozen(bmcg_sat_solver* s, int v, int freeze)
{
	((Gluco::SimpSolver*) s)->setFrozen(v, freeze != 0);
}

inline int bmcg_sat_solver_elim_varnum(bmcg_sat_solver* s)
{
	//    return 0;
	return ((Gluco::SimpSolver*) s)->eliminated_vars;
}

inline int bmcg_sat_solver_read_cex_varvalue(bmcg_sat_solver* s, int ivar)
{
	return glucose_solver_read_cex_varvalue((Gluco::SimpSolver*) s, ivar);
}

inline void bmcg_sat_solver_set_stop(bmcg_sat_solver* s, int* pstop)
{
	glucose_solver_setstop((Gluco::SimpSolver*) s, pstop);
}

inline abctime bmcg_sat_solver_set_runtime_limit(bmcg_sat_solver* s, abctime Limit)
{
	abctime nRuntimeLimit = ((Gluco::SimpSolver*) s)->nRuntimeLimit;
	((Gluco::SimpSolver*) s)->nRuntimeLimit = Limit;
	return nRuntimeLimit;
}

inline void bmcg_sat_solver_set_conflict_budget(bmcg_sat_solver* s, int Limit)
{
	if (Limit > 0)
		((Gluco::SimpSolver*) s)->setConfBudget((int64_t) Limit);
	else
		((Gluco::SimpSolver*) s)->budgetOff();
}

inline int bmcg_sat_solver_varnum(bmcg_sat_solver* s)
{
	return ((Gluco::SimpSolver*) s)->nVars();
}
inline int bmcg_sat_solver_clausenum(bmcg_sat_solver* s)
{
	return ((Gluco::SimpSolver*) s)->nClauses();
}
inline int bmcg_sat_solver_learntnum(bmcg_sat_solver* s)
{
	return ((Gluco::SimpSolver*) s)->nLearnts();
}
inline int bmcg_sat_solver_conflictnum(bmcg_sat_solver* s)
{
	return ((Gluco::SimpSolver*) s)->conflicts;
}

inline int bmcg_sat_solver_minimize_assumptions(bmcg_sat_solver* s, int* plits, int nlits, int pivot)
{
	Gluco::vec<int>* array = &((Gluco::SimpSolver*) s)->user_vec;
	int i, nlitsL, nlitsR, nresL, nresR, status;
	assert(pivot >= 0);
	//    assert( nlits - pivot >= 2 );
	assert(nlits - pivot >= 1);
	if (nlits - pivot == 1) {
		// since the problem is UNSAT, we try to solve it without assuming the last literal
		// if the result is UNSAT, the last literal can be dropped; otherwise, it is needed
		status = bmcg_sat_solver_solve(s, plits, pivot);
		return status != GLUCOSE_UNSAT; // return 1 if the problem is not UNSAT
	}
	assert(nlits - pivot >= 2);
	nlitsL = (nlits - pivot) / 2;
	nlitsR = (nlits - pivot) - nlitsL;
	assert(nlitsL + nlitsR == nlits - pivot);
	// solve with these assumptions
	status = bmcg_sat_solver_solve(s, plits, pivot + nlitsL);
	if (status == GLUCOSE_UNSAT) // these are enough
		return bmcg_sat_solver_minimize_assumptions(s, plits, pivot + nlitsL, pivot);
	// these are not enough
	// solve for the right lits
	//    nResL = nLitsR == 1 ? 1 : sat_solver_minimize_assumptions( s, pLits + nLitsL, nLitsR, nConfLimit );
	nresL = nlitsR == 1 ? 1 :
	                      bmcg_sat_solver_minimize_assumptions(s, plits, nlits, pivot + nlitsL);
	// swap literals
	array->clear();
	for (i = 0; i < nlitsL; i++)
		array->push(plits[pivot + i]);
	for (i = 0; i < nresL; i++)
		plits[pivot + i] = plits[pivot + nlitsL + i];
	for (i = 0; i < nlitsL; i++)
		plits[pivot + nresL + i] = (*array)[i];
	// solve with these assumptions
	status = bmcg_sat_solver_solve(s, plits, pivot + nresL);
	if (status == GLUCOSE_UNSAT) // these are enough
		return nresL;
	// solve for the left lits
	//    nResR = nLitsL == 1 ? 1 : sat_solver_minimize_assumptions( s, pLits + nResL, nLitsL, nConfLimit );
	nresR = nlitsL == 1 ? 1 :
	                      bmcg_sat_solver_minimize_assumptions(s, plits, pivot + nresL + nlitsL,
	                                                           pivot + nresL);
	return nresL + nresR;
}

inline int bmcg_sat_solver_add_and(bmcg_sat_solver* s, int iVar, int iVar0, int iVar1, int fCompl0,
                                   int fCompl1, int fCompl)
{
	int Lits[3];

	Lits[0] = Abc_Var2Lit(iVar, !fCompl);
	Lits[1] = Abc_Var2Lit(iVar0, fCompl0);
	if (!bmcg_sat_solver_addclause(s, Lits, 2))
		return 0;

	Lits[0] = Abc_Var2Lit(iVar, !fCompl);
	Lits[1] = Abc_Var2Lit(iVar1, fCompl1);
	if (!bmcg_sat_solver_addclause(s, Lits, 2))
		return 0;

	Lits[0] = Abc_Var2Lit(iVar, fCompl);
	Lits[1] = Abc_Var2Lit(iVar0, !fCompl0);
	Lits[2] = Abc_Var2Lit(iVar1, !fCompl1);
	if (!bmcg_sat_solver_addclause(s, Lits, 3))
		return 0;

	return 1;
}

#else

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
inline Gluco::Solver* glucose_solver_start()
{
	Gluco::Solver* S = new Gluco::Solver;
	S->setIncrementalMode();
	return S;
}

inline void glucose_solver_stop(Gluco::Solver* S)
{
	delete S;
}

inline int glucose_solver_addclause(Gluco::Solver* S, int* plits, int nlits)
{
	Gluco::vec<Gluco::Lit> lits;
	for (int i = 0; i < nlits; i++, plits++) {
		// note: Glucose uses the same var->lit conventiaon as ABC
		while ((*plits) / 2 >= S->nVars())
			S->newVar();
		assert((*plits) / 2
		       < S->nVars()); // NOTE: since we explicitely use new function bmc_add_var
		Gluco::Lit p;
		p.x = *plits;
		lits.push(p);
	}
	return S->addClause(lits); // returns 0 if the problem is UNSAT
}

inline void glucose_solver_setcallback(Gluco::Solver* S, void* pman, int (*pfunc)(void*, int, int*))
{
	S->pCnfMan = pman;
	S->pCnfFunc = pfunc;
	S->nCallConfl = 1000;
}

inline int glucose_solver_solve(Gluco::Solver* S, int* plits, int nlits)
{
	vec<Lit> lits;
	for (int i = 0; i < nlits; i++, plits++) {
		Lit p;
		p.x = *plits;
		lits.push(p);
	}
	Gluco::lbool res = S->solveLimited(lits);
	return (res == l_True ? 1 : res == l_False ? -1 : 0);
}

inline int glucose_solver_addvar(Gluco::Solver* S)
{
	S->newVar();
	return S->nVars() - 1;
}

inline int glucose_solver_read_cex_varvalue(Gluco::Solver* S, int ivar)
{
	return S->model[ivar] == l_True;
}

inline void glucose_solver_setstop(Gluco::Solver* S, int* pstop)
{
	S->pstop = pstop;
}

/**Function*************************************************************

  Synopsis    [Wrapper APIs to calling from ABC.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
inline bmcg_sat_solver* bmcg_sat_solver_start()
{
	return (bmcg_sat_solver*) glucose_solver_start();
}
inline void bmcg_sat_solver_stop(bmcg_sat_solver* s)
{
	glucose_solver_stop((Gluco::Solver*) s);
}

inline int bmcg_sat_solver_addclause(bmcg_sat_solver* s, int* plits, int nlits)
{
	return glucose_solver_addclause((Gluco::Solver*) s, plits, nlits);
}

inline void bmcg_sat_solver_setcallback(bmcg_sat_solver* s, void* pman,
                                        int (*pfunc)(void*, int, int*))
{
	glucose_solver_setcallback((Gluco::Solver*) s, pman, pfunc);
}

inline int bmcg_sat_solver_solve(bmcg_sat_solver* s, int* plits, int nlits)
{
	return glucose_solver_solve((Gluco::Solver*) s, plits, nlits);
}

inline int bmcg_sat_solver_final(bmcg_sat_solver* s, int** ppArray)
{
	*ppArray = (int*) (Lit*) ((Gluco::Solver*) s)->conflict;
	return ((Gluco::Solver*) s)->conflict.size();
}

inline int bmcg_sat_solver_addvar(bmcg_sat_solver* s)
{
	return glucose_solver_addvar((Gluco::Solver*) s);
}

inline void bmcg_sat_solver_set_nvars(bmcg_sat_solver* s, int nvars)
{
	int i;
	for (i = bmcg_sat_solver_varnum(s); i < nvars; i++)
		bmcg_sat_solver_addvar(s);
}

inline int bmcg_sat_solver_eliminate(bmcg_sat_solver* s, int turn_off_elim)
{
	return 1;
	//    return ((Gluco::SimpSolver*)s)->eliminate(turn_off_elim != 0);
}

inline int bmcg_sat_solver_var_is_elim(bmcg_sat_solver* s, int v)
{
	return 0;
	//    return ((Gluco::SimpSolver*)s)->isEliminated(v);
}

inline void bmcg_sat_solver_var_set_frozen(bmcg_sat_solver* s, int v, int freeze)
{
	//    ((Gluco::SimpSolver*)s)->setFrozen(v, freeze);
}

inline int bmcg_sat_solver_elim_varnum(bmcg_sat_solver* s)
{
	return 0;
	//    return ((Gluco::SimpSolver*)s)->eliminated_vars;
}

inline int bmcg_sat_solver_read_cex_varvalue(bmcg_sat_solver* s, int ivar)
{
	return glucose_solver_read_cex_varvalue((Gluco::Solver*) s, ivar);
}

inline void bmcg_sat_solver_set_stop(bmcg_sat_solver* s, int* pstop)
{
	glucose_solver_setstop((Gluco::Solver*) s, pstop);
}

inline abctime bmcg_sat_solver_set_runtime_limit(bmcg_sat_solver* s, abctime Limit)
{
	abctime nRuntimeLimit = ((Gluco::Solver*) s)->nRuntimeLimit;
	((Gluco::Solver*) s)->nRuntimeLimit = Limit;
	return nRuntimeLimit;
}

inline void bmcg_sat_solver_set_conflict_budget(bmcg_sat_solver* s, int Limit)
{
	if (Limit > 0)
		((Gluco::Solver*) s)->setConfBudget((int64_t) Limit);
	else
		((Gluco::Solver*) s)->budgetOff();
}

inline int bmcg_sat_solver_varnum(bmcg_sat_solver* s)
{
	return ((Gluco::Solver*) s)->nVars();
}
inline int bmcg_sat_solver_clausenum(bmcg_sat_solver* s)
{
	return ((Gluco::Solver*) s)->nClauses();
}
inline int bmcg_sat_solver_learntnum(bmcg_sat_solver* s)
{
	return ((Gluco::Solver*) s)->nLearnts();
}
inline int bmcg_sat_solver_conflictnum(bmcg_sat_solver* s)
{
	return ((Gluco::Solver*) s)->conflicts;
}

inline int bmcg_sat_solver_minimize_assumptions(bmcg_sat_solver* s, int* plits, int nlits, int pivot)
{
	vec<int>* array = &((Gluco::Solver*) s)->user_vec;
	int i, nlitsL, nlitsR, nresL, nresR, status;
	assert(pivot >= 0);
	//    assert( nlits - pivot >= 2 );
	assert(nlits - pivot >= 1);
	if (nlits - pivot == 1) {
		// since the problem is UNSAT, we try to solve it without assuming the last literal
		// if the result is UNSAT, the last literal can be dropped; otherwise, it is needed
		status = bmcg_sat_solver_solve(s, plits, pivot);
		return status != GLUCOSE_UNSAT; // return 1 if the problem is not UNSAT
	}
	assert(nlits - pivot >= 2);
	nlitsL = (nlits - pivot) / 2;
	nlitsR = (nlits - pivot) - nlitsL;
	assert(nlitsL + nlitsR == nlits - pivot);
	// solve with these assumptions
	status = bmcg_sat_solver_solve(s, plits, pivot + nlitsL);
	if (status == GLUCOSE_UNSAT) // these are enough
		return bmcg_sat_solver_minimize_assumptions(s, plits, pivot + nlitsL, pivot);
	// these are not enough
	// solve for the right lits
	//    nResL = nLitsR == 1 ? 1 : sat_solver_minimize_assumptions( s, pLits + nLitsL, nLitsR, nConfLimit );
	nresL = nlitsR == 1 ? 1 :
	                      bmcg_sat_solver_minimize_assumptions(s, plits, nlits, pivot + nlitsL);
	// swap literals
	array->clear();
	for (i = 0; i < nlitsL; i++)
		array->push(plits[pivot + i]);
	for (i = 0; i < nresL; i++)
		plits[pivot + i] = plits[pivot + nlitsL + i];
	for (i = 0; i < nlitsL; i++)
		plits[pivot + nresL + i] = (*array)[i];
	// solve with these assumptions
	status = bmcg_sat_solver_solve(s, plits, pivot + nresL);
	if (status == GLUCOSE_UNSAT) // these are enough
		return nresL;
	// solve for the left lits
	//    nResR = nLitsL == 1 ? 1 : sat_solver_minimize_assumptions( s, pLits + nResL, nLitsL, nConfLimit );
	nresR = nlitsL == 1 ? 1 :
	                      bmcg_sat_solver_minimize_assumptions(s, plits, pivot + nresL + nlitsL,
	                                                           pivot + nresL);
	return nresL + nresR;
}

#endif

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
inline void glucose_print_stats(Gluco::SimpSolver& s, abctime clk)
{
	double cpu_time = (double) (unsigned) clk / CLOCKS_PER_SEC;
	printf("c restarts              : %d (%d conflicts on average)\n", (int) s.starts,
	       s.starts > 0 ? (int) (s.conflicts / s.starts) : 0);
	printf("c blocked restarts      : %d (multiple: %d) \n", (int) s.nbstopsrestarts,
	       (int) s.nbstopsrestartssame);
	printf("c last block at restart : %d\n", (int) s.lastblockatrestart);
	printf("c nb ReduceDB           : %-12d\n", (int) s.nbReduceDB);
	printf("c nb removed Clauses    : %-12d\n", (int) s.nbRemovedClauses);
	printf("c nb learnts DL2        : %-12d\n", (int) s.nbDL2);
	printf("c nb learnts size 2     : %-12d\n", (int) s.nbBin);
	printf("c nb learnts size 1     : %-12d\n", (int) s.nbUn);
	printf("c conflicts             : %-12d  (%.0f /sec)\n", (int) s.conflicts,
	       s.conflicts / cpu_time);
	printf("c decisions             : %-12d  (%4.2f %% random) (%.0f /sec)\n", (int) s.decisions,
	       (float) s.rnd_decisions * 100 / (float) s.decisions, s.decisions / cpu_time);
	printf("c propagations          : %-12d  (%.0f /sec)\n", (int) s.propagations,
	       s.propagations / cpu_time);
	printf("c conflict literals     : %-12d  (%4.2f %% deleted)\n", (int) s.tot_literals,
	       (s.max_literals - s.tot_literals) * 100 / (double) s.max_literals);
	printf("c nb reduced Clauses    : %-12d\n", (int) s.nbReducedClauses);
	// printf("c CPU time              : %.2f sec\n", cpu_time);
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_IMPL_END

#undef SAT_USE_ANALYZE_FINAL
#undef L_IND
#undef L_ind
#undef L_LIT
#undef L_lit
#undef USE_SIMP_SOLVER
