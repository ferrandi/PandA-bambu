/*!
  \file vec_int.hpp
  \brief Extracted from ABC
         https://github.com/berkeley-abc/abc

  \author Alan Mishchenko (UC Berkeley)
*/

#pragma once

namespace abcresub
{

typedef struct Vec_Int_t_ Vec_Int_t;

struct Vec_Int_t_ 
{
    int              nCap;
    int              nSize;
    int *            pArray;
};

#define Vec_IntForEachEntry( vVec, Entry, i )                                               \
    for ( i = 0; (i < Vec_IntSize(vVec)) && (((Entry) = Vec_IntEntry(vVec, i)), 1); i++ )
#define Vec_IntForEachEntryStop( vVec, Entry, i, Stop )                                     \
    for ( i = 0; (i < Stop) && (((Entry) = Vec_IntEntry(vVec, i)), 1); i++ )
#define Vec_IntForEachEntryDouble( vVec, Entry1, Entry2, i )            \
    for ( i = 0; (i+1 < Vec_IntSize(vVec)) && (((Entry1) = Vec_IntEntry(vVec, i)), 1) && (((Entry2) = Vec_IntEntry(vVec, i+1)), 1); i += 2 )
#define Vec_IntForEachEntryTwo( vVec1, vVec2, Entry1, Entry2, i )                           \
    for ( i = 0; (i < Vec_IntSize(vVec1)) && (((Entry1) = Vec_IntEntry(vVec1, i)), 1) && (((Entry2) = Vec_IntEntry(vVec2, i)), 1); i++ )
#define Vec_IntForEachEntryTwoStart( vVec1, vVec2, Entry1, Entry2, i, Start )               \
    for ( i = Start; (i < Vec_IntSize(vVec1)) && (((Entry1) = Vec_IntEntry(vVec1, i)), 1) && (((Entry2) = Vec_IntEntry(vVec2, i)), 1); i++ )

inline Vec_Int_t * Vec_IntAlloc( int nCap )
{
    Vec_Int_t * p;
    p = ABC_ALLOC( Vec_Int_t, 1 );
    if ( nCap > 0 && nCap < 16 )
        nCap = 16;
    p->nSize  = 0;
    p->nCap   = nCap;
    p->pArray = p->nCap? ABC_ALLOC( int, p->nCap ) : NULL;
    return p;
}

inline Vec_Int_t * Vec_IntStartFull( int nSize )
{
    Vec_Int_t * p;
    p = Vec_IntAlloc( nSize );
    p->nSize = nSize;
    if ( p->pArray ) memset( p->pArray, 0xff, sizeof(int) * (size_t)nSize );
    return p;
}

inline void Vec_IntWriteEntry( Vec_Int_t * p, int i, int Entry )
{
    assert( i >= 0 && i < p->nSize );
    p->pArray[i] = Entry;
}

inline void Vec_IntClear( Vec_Int_t * p )
{
    p->nSize = 0;
}

inline void Vec_IntErase( Vec_Int_t * p )
{
    ABC_FREE( p->pArray );
    p->nSize = 0;
    p->nCap = 0;
}

inline void Vec_IntFree( Vec_Int_t * p )
{
    ABC_FREE( p->pArray );
    ABC_FREE( p );
}

inline int * Vec_IntArray( Vec_Int_t * p )
{
    return p->pArray;
}

inline int Vec_IntSize( Vec_Int_t * p )
{
    return p->nSize;
}

inline int Vec_IntEntry( Vec_Int_t * p, int i )
{
    assert( i >= 0 && i < p->nSize );
    return p->pArray[i];
}

inline int Vec_IntEntryLast( Vec_Int_t * p )
{
    assert( p->nSize > 0 );
    return p->pArray[p->nSize-1];
}

inline void Vec_IntGrow( Vec_Int_t * p, int nCapMin )
{
    if ( p->nCap >= nCapMin )
        return;
    p->pArray = ABC_REALLOC( int, p->pArray, nCapMin ); 
    assert( p->pArray );
    p->nCap   = nCapMin;
}

inline void Vec_IntShrink( Vec_Int_t * p, int nSizeNew )
{
    assert( p->nSize >= nSizeNew );
    p->nSize = nSizeNew;
}

inline void Vec_IntPush( Vec_Int_t * p, int Entry )
{
    if ( p->nSize == p->nCap )
    {
        if ( p->nCap < 16 )
            Vec_IntGrow( p, 16 );
        else
            Vec_IntGrow( p, 2 * p->nCap );
    }
    p->pArray[p->nSize++] = Entry;
}

inline void Vec_IntPushTwo( Vec_Int_t * p, int Entry1, int Entry2 )
{
    Vec_IntPush( p, Entry1 );
    Vec_IntPush( p, Entry2 );
}

inline void Vec_IntPushArray( Vec_Int_t * p, int * pEntries, int nEntries )
{
    int i;
    for ( i = 0; i < nEntries; i++ )
        Vec_IntPush( p, pEntries[i] );
}

inline int Vec_IntTwoFindCommon( Vec_Int_t * vArr1, Vec_Int_t * vArr2, Vec_Int_t * vArr )
{
    int * pBeg1 = vArr1->pArray;
    int * pBeg2 = vArr2->pArray;
    int * pEnd1 = vArr1->pArray + vArr1->nSize;
    int * pEnd2 = vArr2->pArray + vArr2->nSize;
    Vec_IntClear( vArr );
    while ( pBeg1 < pEnd1 && pBeg2 < pEnd2 )
    {
        if ( *pBeg1 == *pBeg2 )
            Vec_IntPush( vArr, *pBeg1 ), pBeg1++, pBeg2++;
        else if ( *pBeg1 < *pBeg2 )
            pBeg1++;
        else 
            pBeg2++;
    }
    return Vec_IntSize(vArr);
}

inline Vec_Int_t * Vec_IntDup( Vec_Int_t * pVec )
{
    Vec_Int_t * p;
    p = ABC_ALLOC( Vec_Int_t, 1 );
    p->nSize  = pVec->nSize;
    p->nCap   = pVec->nSize;
    p->pArray = p->nCap? ABC_ALLOC( int, p->nCap ) : NULL;
    memcpy( p->pArray, pVec->pArray, sizeof(int) * (size_t)pVec->nSize );
    return p;
}

inline void Vec_IntAppend( Vec_Int_t * vVec1, Vec_Int_t * vVec2 )
{
    int Entry, i;
    Vec_IntForEachEntry( vVec2, Entry, i )
        Vec_IntPush( vVec1, Entry );
}

inline int Vec_IntFind( Vec_Int_t * p, int Entry )
{
    int i;
    for ( i = 0; i < p->nSize; i++ )
        if ( p->pArray[i] == Entry )
            return i;
    return -1;
}

inline void Vec_IntZero( Vec_Int_t * p )
{
    p->pArray = NULL;
    p->nSize = 0;
    p->nCap = 0;
}

inline void Vec_IntFill( Vec_Int_t * p, int nSize, int Fill )
{
    int i;
    Vec_IntGrow( p, nSize );
    for ( i = 0; i < nSize; i++ )
        p->pArray[i] = Fill;
    p->nSize = nSize;
}

} /* namespace abcresub */
