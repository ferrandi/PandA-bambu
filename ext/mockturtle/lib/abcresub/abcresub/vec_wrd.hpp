/*!
  \file vec_wrd.hpp
  \brief Extracted from ABC
         https://github.com/berkeley-abc/abc

  \author Alan Mishchenko (UC Berkeley)
*/

#pragma once

namespace abcresub
{

typedef struct Vec_Wrd_t_ Vec_Wrd_t;

struct Vec_Wrd_t_ 
{
    int              nCap;
    int              nSize;
    word *           pArray;
};

inline Vec_Wrd_t * Vec_WrdAlloc( int nCap )
{
    Vec_Wrd_t * p;
    p = ABC_ALLOC( Vec_Wrd_t, 1 );
    if ( nCap > 0 && nCap < 16 )
        nCap = 16;
    p->nSize  = 0;
    p->nCap   = nCap;
    p->pArray = p->nCap? ABC_ALLOC( word, p->nCap ) : NULL;
    return p;
}

inline void Vec_WrdErase( Vec_Wrd_t * p )
{
    ABC_FREE( p->pArray );
    p->nSize = 0;
    p->nCap = 0;
}

inline void Vec_WrdFree( Vec_Wrd_t * p )
{
    ABC_FREE( p->pArray );
    ABC_FREE( p );
}

inline word * Vec_WrdArray( Vec_Wrd_t * p )
{
    return p->pArray;
}

inline word Vec_WrdEntry( Vec_Wrd_t * p, int i )
{
    assert( i >= 0 && i < p->nSize );
    return p->pArray[i];
}

inline word * Vec_WrdEntryP( Vec_Wrd_t * p, int i )
{
    assert( i >= 0 && i < p->nSize );
    return p->pArray + i;
}

inline void Vec_WrdGrow( Vec_Wrd_t * p, int nCapMin )
{
    if ( p->nCap >= nCapMin )
        return;
    p->pArray = ABC_REALLOC( word, p->pArray, nCapMin ); 
    assert( p->pArray );
    p->nCap   = nCapMin;
}

inline void Vec_WrdFill( Vec_Wrd_t * p, int nSize, word Fill )
{
    int i;
    Vec_WrdGrow( p, nSize );
    for ( i = 0; i < nSize; i++ )
        p->pArray[i] = Fill;
    p->nSize = nSize;
}

inline void Vec_WrdClear( Vec_Wrd_t * p )
{
    p->nSize = 0;
}

inline void Vec_WrdPush( Vec_Wrd_t * p, word Entry )
{
    if ( p->nSize == p->nCap )
    {
        if ( p->nCap < 16 )
            Vec_WrdGrow( p, 16 );
        else
            Vec_WrdGrow( p, 2 * p->nCap );
    }
    p->pArray[p->nSize++] = Entry;
}

inline int Vec_WrdSize( Vec_Wrd_t * p )
{
    return p->nSize;
}

} /* namespace abcresub */
