/*!
  \file vec_wec.hpp
  \brief Extracted from ABC
         https://github.com/berkeley-abc/abc

  \author Alan Mishchenko (UC Berkeley)
*/

#pragma once

namespace abcresub
{

typedef struct Vec_Wec_t_       Vec_Wec_t;
struct Vec_Wec_t_ 
{
    int              nCap;
    int              nSize;
    Vec_Int_t *      pArray;
};

#define Vec_WecForEachLevel( vGlob, vVec, i )                                              \
    for ( i = 0; (i < Vec_WecSize(vGlob)) && (((vVec) = Vec_WecEntry(vGlob, i)), 1); i++ )
#define Vec_WecForEachLevelReverse( vGlob, vVec, i )                                       \
    for ( i = Vec_WecSize(vGlob)-1; (i >= 0) && (((vVec) = Vec_WecEntry(vGlob, i)), 1); i-- )
  
inline Vec_Wec_t * Vec_WecAlloc( int nCap )
{
    Vec_Wec_t * p;
    p = ABC_ALLOC( Vec_Wec_t, 1 );
    if ( nCap > 0 && nCap < 8 )
        nCap = 8;
    p->nSize  = 0;
    p->nCap   = nCap;
    p->pArray = p->nCap? ABC_CALLOC( Vec_Int_t, p->nCap ) : NULL;
    return p;
}

inline void Vec_WecErase( Vec_Wec_t * p )
{
    int i;
    for ( i = 0; i < p->nCap; i++ )
        ABC_FREE( p->pArray[i].pArray );
    ABC_FREE( p->pArray );
    p->nSize = 0;
    p->nCap = 0;
}

inline void Vec_WecFree( Vec_Wec_t * p )
{
    Vec_WecErase( p );
    ABC_FREE( p );
}

inline int Vec_WecSize( Vec_Wec_t * p )
{
    return p->nSize;
}

inline void Vec_WecGrow( Vec_Wec_t * p, int nCapMin )
{
    if ( p->nCap >= nCapMin )
        return;
    p->pArray = ABC_REALLOC( Vec_Int_t, p->pArray, nCapMin ); 
    memset( p->pArray + p->nCap, 0, sizeof(Vec_Int_t) * (size_t)(nCapMin - p->nCap) );
    p->nCap   = nCapMin;
}

inline void Vec_WecInit( Vec_Wec_t * p, int nSize )
{
    Vec_WecGrow( p, nSize );
    p->nSize = nSize;
}

inline Vec_Int_t * Vec_WecEntry( Vec_Wec_t * p, int i )
{
    assert( i >= 0 && i < p->nSize );
    return p->pArray + i;
}

inline void Vec_WecClear( Vec_Wec_t * p )
{
    Vec_Int_t * vVec;
    int i;
    Vec_WecForEachLevel( p, vVec, i )
        Vec_IntClear( vVec );
    p->nSize = 0;
}

inline void Vec_WecPush( Vec_Wec_t * p, int Level, int Entry )
{
    if ( p->nSize < Level + 1 )
    {
        Vec_WecGrow( p, Abc_MaxInt(2*p->nSize, Level + 1) );
        p->nSize = Level + 1;
    }
    Vec_IntPush( Vec_WecEntry(p, Level), Entry );
}

} /* namespace abcresub */
