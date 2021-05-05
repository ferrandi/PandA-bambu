/*!
  \file tt.hpp
  \brief Extracted from ABC
         https://github.com/berkeley-abc/abc

  \author Alan Mishchenko (UC Berkeley)
*/

#pragma once

namespace abcresub
{

// read/write/flip i-th bit of a bit string table:
inline int     Abc_TtGetBit( word * p, int i )         { return (int)(p[i>>6] >> (word)(i & 63)) & 1;        }
inline void    Abc_TtSetBit( word * p, int i )         { p[i>>6] |= (word)(((word)1)<<(i & 63));             }
inline void    Abc_TtXorBit( word * p, int i )         { p[i>>6] ^= (word)(((word)1)<<(i & 63));             }

// read/write k-th digit d of a quaternary number:
inline int     Abc_TtGetQua( word * p, int k )         { return (int)(p[k>>5] >> (word)((k<<1) & 63)) & 3;   }
inline void    Abc_TtSetQua( word * p, int k, int d )  { p[k>>5] |= (word)(((word)d)<<((k<<1) & 63));        }
inline void    Abc_TtXorQua( word * p, int k, int d )  { p[k>>5] ^= (word)(((word)d)<<((k<<1) & 63));        }

// read/write k-th digit d of a hexadecimal number:
inline int     Abc_TtGetHex( word * p, int k )         { return (int)(p[k>>4] >> (word)((k<<2) & 63)) & 15;  }
inline void    Abc_TtSetHex( word * p, int k, int d )  { p[k>>4] |= (word)(((word)d)<<((k<<2) & 63));        }
inline void    Abc_TtXorHex( word * p, int k, int d )  { p[k>>4] ^= (word)(((word)d)<<((k<<2) & 63));        }

// read/write k-th digit d of a 256-base number:
inline int     Abc_TtGet256( word * p, int k )         { return (int)(p[k>>3] >> (word)((k<<3) & 63)) & 255; }
inline void    Abc_TtSet256( word * p, int k, int d )  { p[k>>3] |= (word)(((word)d)<<((k<<3) & 63));        }
inline void    Abc_TtXor256( word * p, int k, int d )  { p[k>>3] ^= (word)(((word)d)<<((k<<3) & 63));        }

inline int Abc_TtCountOnes( word x )
{
    x = x - ((x >> 1) & ABC_CONST(0x5555555555555555));   
    x = (x & ABC_CONST(0x3333333333333333)) + ((x >> 2) & ABC_CONST(0x3333333333333333));    
    x = (x + (x >> 4)) & ABC_CONST(0x0F0F0F0F0F0F0F0F);    
    x = x + (x >> 8);
    x = x + (x >> 16);
    x = x + (x >> 32); 
    return (int)(x & 0xFF);
}

inline int Abc_TtCountOnesVec( word * x, int nWords )
{
    int w, Count = 0;
    for ( w = 0; w < nWords; w++ )
        Count += Abc_TtCountOnes( x[w] );
    return Count;
}

inline int Abc_TtCountOnesVecMask( word * x, word * pMask, int nWords, int fCompl )
{
    int w, Count = 0;
    if ( fCompl )
        for ( w = 0; w < nWords; w++ )
            Count += Abc_TtCountOnes( pMask[w] & ~x[w] );
    else
        for ( w = 0; w < nWords; w++ )
            Count += Abc_TtCountOnes( pMask[w] & x[w] );
    return Count;
}

inline int Abc_TtCountOnesVecMask2( word * x0, word * x1, int fComp0, int fComp1, word * pMask, int nWords )
{
    int w, Count = 0;
    if ( !fComp0 && !fComp1 )
        for ( w = 0; w < nWords; w++ )
            Count += Abc_TtCountOnes( pMask[w] &  x0[w] &  x1[w] );
    else if (  fComp0 && !fComp1 )
        for ( w = 0; w < nWords; w++ )
            Count += Abc_TtCountOnes( pMask[w] & ~x0[w] &  x1[w] );
    else if ( !fComp0 &&  fComp1 )
        for ( w = 0; w < nWords; w++ )
            Count += Abc_TtCountOnes( pMask[w] &  x0[w] & ~x1[w] );
    else 
        for ( w = 0; w < nWords; w++ )
            Count += Abc_TtCountOnes( pMask[w] & ~x0[w] & ~x1[w] );
    return Count;
}

inline int Abc_TtCountOnesVecXorMask( word * x, word * y, int fCompl, word * pMask, int nWords )
{
    int w, Count = 0;
    if ( fCompl )
        for ( w = 0; w < nWords; w++ )
            Count += Abc_TtCountOnes( pMask[w] & (x[w] ^ ~y[w]) );
    else
        for ( w = 0; w < nWords; w++ )
            Count += Abc_TtCountOnes( pMask[w] & (x[w] ^ y[w]) );
    return Count;
}

inline void Abc_TtCopy( word * pOut, word * pIn, int nWords, int fCompl )
{
    int w;
    if ( fCompl )
        for ( w = 0; w < nWords; w++ )
            pOut[w] = ~pIn[w];
    else
        for ( w = 0; w < nWords; w++ )
            pOut[w] = pIn[w];
}

inline int Abc_TtIsConst0( word * pIn1, int nWords )
{
    int w;
    for ( w = 0; w < nWords; w++ )
        if ( pIn1[w] )
            return 0;
    return 1;
}

inline void Abc_TtClear( word * pOut, int nWords )
{
    int w;
    for ( w = 0; w < nWords; w++ )
        pOut[w] = 0;
}

inline void Abc_TtFill( word * pOut, int nWords )
{
    int w;
    for ( w = 0; w < nWords; w++ )
        pOut[w] = ~(word)0;
}

inline void Abc_TtAndCompl( word * pOut, word * pIn1, int fCompl1, word * pIn2, int fCompl2, int nWords )
{
    int w;
    if ( fCompl1 )
    {
        if ( fCompl2 )
            for ( w = 0; w < nWords; w++ )
                pOut[w] = ~pIn1[w] & ~pIn2[w];
        else
            for ( w = 0; w < nWords; w++ )
                pOut[w] = ~pIn1[w] & pIn2[w];
    }
    else
    {
        if ( fCompl2 )
            for ( w = 0; w < nWords; w++ )
                pOut[w] = pIn1[w] & ~pIn2[w];
        else
            for ( w = 0; w < nWords; w++ )
                pOut[w] = pIn1[w] & pIn2[w];
    }
}

inline void Abc_TtAndSharp( word * pOut, word * pIn1, word * pIn2, int nWords, int fCompl )
{
    int w;
    if ( fCompl )
        for ( w = 0; w < nWords; w++ )
            pOut[w] = pIn1[w] & ~pIn2[w];
    else
        for ( w = 0; w < nWords; w++ )
            pOut[w] = pIn1[w] & pIn2[w];
}

inline void Abc_TtXor( word * pOut, word * pIn1, word * pIn2, int nWords, int fCompl )
{
    int w;
    if ( fCompl )
        for ( w = 0; w < nWords; w++ )
            pOut[w] = pIn1[w] ^ ~pIn2[w];
    else
        for ( w = 0; w < nWords; w++ )
            pOut[w] = pIn1[w] ^ pIn2[w];
}

inline int Abc_TtIntersectOne( word * pOut, int fComp, word * pIn, int fComp0, int nWords )
{
    int w;
    if ( fComp0 )
    {
        if ( fComp )
        {
            for ( w = 0; w < nWords; w++ )
                if ( ~pIn[w] & ~pOut[w] )
                    return 1;
        }
        else
        {
            for ( w = 0; w < nWords; w++ )
                if ( ~pIn[w] & pOut[w] )
                    return 1;
        }
    }
    else
    {
        if ( fComp )
        {
            for ( w = 0; w < nWords; w++ )
                if ( pIn[w] & ~pOut[w] )
                    return 1;
        }
        else
        {
            for ( w = 0; w < nWords; w++ )
                if ( pIn[w] & pOut[w] )
                    return 1;
        }
    }
    return 0;
}

inline int Abc_TtIntersectTwo( word * pOut, int fComp, word * pIn0, int fComp0, word * pIn1, int fComp1, int nWords )
{
    int w;
    if ( fComp0 && fComp1 )
    {
        if ( fComp )
        {
            for ( w = 0; w < nWords; w++ )
                if ( ~pIn0[w] & ~pIn1[w] & ~pOut[w] )
                    return 1;
        }
        else
        {
            for ( w = 0; w < nWords; w++ )
                if ( ~pIn0[w] & ~pIn1[w] & pOut[w] )
                    return 1;
        }
    }
    else if ( fComp0 )
    {
        if ( fComp )
        {
            for ( w = 0; w < nWords; w++ )
                if ( ~pIn0[w] & pIn1[w] & ~pOut[w] )
                    return 1;
        }
        else
        {
            for ( w = 0; w < nWords; w++ )
                if ( ~pIn0[w] & pIn1[w] & pOut[w] )
                    return 1;
        }
    }
    else if ( fComp1 )
    {
        if ( fComp )
        {
            for ( w = 0; w < nWords; w++ )
                if ( pIn0[w] & ~pIn1[w] & ~pOut[w] )
                    return 1;
        }
        else
        {
            for ( w = 0; w < nWords; w++ )
                if ( pIn0[w] & ~pIn1[w] & pOut[w] )
                    return 1;
        }
    }
    else 
    {
        if ( fComp )
        {
            for ( w = 0; w < nWords; w++ )
                if ( pIn0[w] & pIn1[w] & ~pOut[w] )
                    return 1;
        }
        else
        {
            for ( w = 0; w < nWords; w++ )
                if ( pIn0[w] & pIn1[w] & pOut[w] )
                    return 1;
        }
    }
    return 0;
}

inline int Abc_TtIntersectXor( word * pOut, int fComp, word * pIn0, word * pIn1, int fComp01, int nWords )
{
    int w;
    if ( fComp01 )
    {
        if ( fComp )
        {
            for ( w = 0; w < nWords; w++ )
                if ( ~(pIn0[w] ^ pIn1[w]) & ~pOut[w] )
                    return 1;
        }
        else 
        {
            for ( w = 0; w < nWords; w++ )
                if ( ~(pIn0[w] ^ pIn1[w]) & pOut[w] )
                    return 1;
        }
    }
    else
    {
        if ( fComp )
        {
            for ( w = 0; w < nWords; w++ )
                if ( (pIn0[w] ^ pIn1[w]) & ~pOut[w] )
                    return 1;
        }
        else 
        {
            for ( w = 0; w < nWords; w++ )
                if ( (pIn0[w] ^ pIn1[w]) & pOut[w] )
                    return 1;
        }
    }
    return 0;
}

inline word Abc_Tt6Stretch( word t, int nVars )
{
    assert( nVars >= 0 );
    if ( nVars == 0 )
        nVars++, t = (t & 0x1) | ((t & 0x1) << 1);
    if ( nVars == 1 )
        nVars++, t = (t & 0x3) | ((t & 0x3) << 2);
    if ( nVars == 2 )
        nVars++, t = (t & 0xF) | ((t & 0xF) << 4);
    if ( nVars == 3 )
        nVars++, t = (t & 0xFF) | ((t & 0xFF) << 8);
    if ( nVars == 4 )
        nVars++, t = (t & 0xFFFF) | ((t & 0xFFFF) << 16);
    if ( nVars == 5 )
        nVars++, t = (t & 0xFFFFFFFF) | ((t & 0xFFFFFFFF) << 32);
    assert( nVars == 6 );
    return t;
}

} /* abcresub */
