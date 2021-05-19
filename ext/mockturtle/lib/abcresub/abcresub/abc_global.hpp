/*!
  \file abc_global.hpp
  \brief Extracted from ABC
         https://github.com/berkeley-abc/abc

  \author Alan Mishchenko (UC Berkeley)
*/

#pragma once

namespace abcresub
{

typedef uint64_t word;
inline int      Abc_MaxInt( int a, int b )             { return a > b ?  a : b; }
inline int      Abc_MinInt( int a, int b )             { return a < b ?  a : b; }
inline int      Abc_LitIsCompl( int Lit )              { assert(Lit >= 0); return Lit & 1;                           }
inline int      Abc_Lit2Var( int Lit )                 { assert(Lit >= 0); return Lit >> 1;                          }
inline int      Abc_Var2Lit( int Var, int c )          { assert(Var >= 0 && !(c >> 1)); return Var + Var + c;        }
inline int      Abc_LitNot( int Lit )                  { assert(Lit >= 0); return Lit ^ 1;                           }
inline int      Abc_LitNotCond( int Lit, int c )       { assert(Lit >= 0); return Lit ^ (int)(c > 0);                }
inline int      Abc_Lit2LitL( int * pMap, int Lit )    { assert(Lit >= 0); return Abc_LitNotCond( pMap[Abc_Lit2Var(Lit)], Abc_LitIsCompl(Lit) );   }

#ifndef ABC_SWAP
#define ABC_SWAP(Type, a, b)  { Type t = a; a = b; b = t; }
#endif /* ABC_SWAP */
  
#ifndef ABC_CONST
#define ABC_CONST(number) number ## ULL
#endif /* ABC_CONST */

#ifndef ABC_ALLOC
#define ABC_ALLOC(type, num)     ((type *) malloc(sizeof(type) * (size_t)(num)))
#endif /* ABC_ALLOC */

#ifndef ABC_CALLOC
#define ABC_CALLOC(type, num)    ((type *) calloc((size_t)(num), sizeof(type)))
#endif /* ABC_CALLOC */

#ifndef ABC_FREE
#define ABC_FREE(obj)            ((obj) ? (free((char *) (obj)), (obj) = 0) : 0)
#endif /* ABC_FREE */

#ifndef ABC_REALLOC
#define ABC_REALLOC(type, obj, num) \
        ((obj) ? ((type *) realloc((char *)(obj), sizeof(type) * (size_t)(num))) : \
         ((type *) malloc(sizeof(type) * (size_t)(num))))
#endif /* ABC_REALLOC */

void Abc_SortMerge( int * p1Beg, int * p1End, int * p2Beg, int * p2End, int * pOut )
{
    int nEntries = (p1End - p1Beg) + (p2End - p2Beg);
    int * pOutBeg = pOut;
    (void)nEntries;
    (void)pOutBeg;
    while ( p1Beg < p1End && p2Beg < p2End )
    {
        if ( *p1Beg == *p2Beg )
            *pOut++ = *p1Beg++, *pOut++ = *p2Beg++; 
        else if ( *p1Beg < *p2Beg )
            *pOut++ = *p1Beg++; 
        else // if ( *p1Beg > *p2Beg )
            *pOut++ = *p2Beg++; 
    }
    while ( p1Beg < p1End )
        *pOut++ = *p1Beg++; 
    while ( p2Beg < p2End )
        *pOut++ = *p2Beg++;
    assert( pOut - pOutBeg == nEntries );
}

void Abc_Sort_rec( int * pInBeg, int * pInEnd, int * pOutBeg )
{
    int nSize = pInEnd - pInBeg;
    assert( nSize > 0 );
    if ( nSize == 1 )
        return;
    if ( nSize == 2 )
    {
         if ( pInBeg[0] > pInBeg[1] )
         {
             pInBeg[0] ^= pInBeg[1];
             pInBeg[1] ^= pInBeg[0];
             pInBeg[0] ^= pInBeg[1];
         }
    }
    else if ( nSize < 8 )
    {
        int temp, i, j, best_i;
        for ( i = 0; i < nSize-1; i++ )
        {
            best_i = i;
            for ( j = i+1; j < nSize; j++ )
                if ( pInBeg[j] < pInBeg[best_i] )
                    best_i = j;
            temp = pInBeg[i]; 
            pInBeg[i] = pInBeg[best_i]; 
            pInBeg[best_i] = temp;
        }
    }
    else
    {
        Abc_Sort_rec( pInBeg, pInBeg + nSize/2, pOutBeg );
        Abc_Sort_rec( pInBeg + nSize/2, pInEnd, pOutBeg + nSize/2 );
        Abc_SortMerge( pInBeg, pInBeg + nSize/2, pInBeg + nSize/2, pInEnd, pOutBeg );
        memcpy( pInBeg, pOutBeg, sizeof(int) * nSize );
    }
}

void Abc_MergeSort( int * pInput, int nSize )
{
    int * pOutput;
    if ( nSize < 2 )
        return;
    pOutput = (int *) malloc( sizeof(int) * nSize );
    Abc_Sort_rec( pInput, pInput + nSize, pOutput );
    free( pOutput );
}

void Abc_MergeSortCostMerge( int * p1Beg, int * p1End, int * p2Beg, int * p2End, int * pOut )
{
    int nEntries = (p1End - p1Beg) + (p2End - p2Beg);
    int * pOutBeg = pOut;
    (void)nEntries;
    (void)pOutBeg;
    while ( p1Beg < p1End && p2Beg < p2End )
    {
        if ( p1Beg[1] == p2Beg[1] )
            *pOut++ = *p1Beg++, *pOut++ = *p1Beg++, *pOut++ = *p2Beg++, *pOut++ = *p2Beg++; 
        else if ( p1Beg[1] < p2Beg[1] )
            *pOut++ = *p1Beg++, *pOut++ = *p1Beg++; 
        else // if ( p1Beg[1] > p2Beg[1] )
            *pOut++ = *p2Beg++, *pOut++ = *p2Beg++; 
    }
    while ( p1Beg < p1End )
        *pOut++ = *p1Beg++, *pOut++ = *p1Beg++; 
    while ( p2Beg < p2End )
        *pOut++ = *p2Beg++, *pOut++ = *p2Beg++;
    assert( pOut - pOutBeg == nEntries );
}

void Abc_MergeSortCost_rec( int * pInBeg, int * pInEnd, int * pOutBeg )
{
    int nSize = (pInEnd - pInBeg)/2;
    assert( nSize > 0 );
    if ( nSize == 1 )
        return;
    if ( nSize == 2 )
    {
         if ( pInBeg[1] > pInBeg[3] )
         {
             pInBeg[1] ^= pInBeg[3];
             pInBeg[3] ^= pInBeg[1];
             pInBeg[1] ^= pInBeg[3];
             pInBeg[0] ^= pInBeg[2];
             pInBeg[2] ^= pInBeg[0];
             pInBeg[0] ^= pInBeg[2];
         }
    }
    else if ( nSize < 8 )
    {
        int temp, i, j, best_i;
        for ( i = 0; i < nSize-1; i++ )
        {
            best_i = i;
            for ( j = i+1; j < nSize; j++ )
                if ( pInBeg[2*j+1] < pInBeg[2*best_i+1] )
                    best_i = j;
            temp = pInBeg[2*i]; 
            pInBeg[2*i] = pInBeg[2*best_i]; 
            pInBeg[2*best_i] = temp;
            temp = pInBeg[2*i+1]; 
            pInBeg[2*i+1] = pInBeg[2*best_i+1]; 
            pInBeg[2*best_i+1] = temp;
        }
    }
    else
    {
        Abc_MergeSortCost_rec( pInBeg, pInBeg + 2*(nSize/2), pOutBeg );
        Abc_MergeSortCost_rec( pInBeg + 2*(nSize/2), pInEnd, pOutBeg + 2*(nSize/2) );
        Abc_MergeSortCostMerge( pInBeg, pInBeg + 2*(nSize/2), pInBeg + 2*(nSize/2), pInEnd, pOutBeg );
        memcpy( pInBeg, pOutBeg, sizeof(int) * 2 * nSize );
    }
}

int * Abc_MergeSortCost( int * pCosts, int nSize )
{
    int i, * pResult, * pInput, * pOutput;
    pResult = (int *) calloc( sizeof(int), nSize );
    if ( nSize < 2 )
        return pResult;
    pInput  = (int *) malloc( sizeof(int) * 2 * nSize );
    pOutput = (int *) malloc( sizeof(int) * 2 * nSize );
    for ( i = 0; i < nSize; i++ )
        pInput[2*i] = i, pInput[2*i+1] = pCosts[i];
    Abc_MergeSortCost_rec( pInput, pInput + 2*nSize, pOutput );
    for ( i = 0; i < nSize; i++ )
        pResult[i] = pInput[2*i];
    free( pOutput );
    free( pInput );
    return pResult;
}

} /* namespace abcresub */
