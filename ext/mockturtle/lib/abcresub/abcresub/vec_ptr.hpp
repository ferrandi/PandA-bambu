/*!
  \file vec_ptr.hpp
  \brief Extracted from ABC
         https://github.com/berkeley-abc/abc

  \author Alan Mishchenko (UC Berkeley)
*/

#pragma once

namespace abcresub
{

typedef struct Vec_Ptr_t_ Vec_Ptr_t;

struct Vec_Ptr_t_ 
{
    int              nCap;
    int              nSize;
    void **          pArray;
};

#define Vec_PtrForEachEntry( Type, vVec, pEntry, i )                                               \
    for ( i = 0; (i < Vec_PtrSize(vVec)) && (((pEntry) = (Type)Vec_PtrEntry(vVec, i)), 1); i++ )
#define Vec_PtrForEachEntryStart( Type, vVec, pEntry, i, Start )                                   \
    for ( i = Start; (i < Vec_PtrSize(vVec)) && (((pEntry) = (Type)Vec_PtrEntry(vVec, i)), 1); i++ )

inline Vec_Ptr_t * Vec_PtrAlloc( int nCap )
{
    Vec_Ptr_t * p;
    p = ABC_ALLOC( Vec_Ptr_t, 1 );
    if ( nCap > 0 && nCap < 8 )
        nCap = 8;
    p->nSize  = 0;
    p->nCap   = nCap;
    p->pArray = p->nCap? ABC_ALLOC( void *, p->nCap ) : NULL;
    return p;
}

inline void * Vec_PtrEntry( Vec_Ptr_t * p, int i )
{
    assert( i >= 0 && i < p->nSize );
    return p->pArray[i];
}

inline void Vec_PtrGrow( Vec_Ptr_t * p, int nCapMin )
{
    if ( p->nCap >= nCapMin )
        return;
    p->pArray = ABC_REALLOC( void *, p->pArray, nCapMin ); 
    p->nCap   = nCapMin;
}

inline void Vec_PtrClear( Vec_Ptr_t * p )
{
    p->nSize = 0;
}

inline void Vec_PtrErase( Vec_Ptr_t * p )
{
    ABC_FREE( p->pArray );
    p->nSize = 0;
    p->nCap = 0;
}

inline void Vec_PtrFree( Vec_Ptr_t * p )
{
    ABC_FREE( p->pArray );
    ABC_FREE( p );
}

inline void ** Vec_PtrArray( Vec_Ptr_t * p )
{
    return p->pArray;
}

inline int Vec_PtrSize( Vec_Ptr_t * p )
{
    return p->nSize;
}

inline void Vec_PtrPush( Vec_Ptr_t * p, void * Entry )
{
    if ( p->nSize == p->nCap )
    {
        if ( p->nCap < 16 )
            Vec_PtrGrow( p, 16 );
        else
            Vec_PtrGrow( p, 2 * p->nCap );
    }
    p->pArray[p->nSize++] = Entry;
}

inline void Vec_PtrAppend( Vec_Ptr_t * vVec1, Vec_Ptr_t * vVec2 )
{
    void * Entry; int i;
    Vec_PtrForEachEntry( void *, vVec2, Entry, i )
        Vec_PtrPush( vVec1, Entry );
}

} /* abcresub */
