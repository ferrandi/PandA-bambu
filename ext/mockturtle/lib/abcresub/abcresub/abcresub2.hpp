/*!
  \file abcresub2.hpp
  \brief Extracted from ABC
         https://github.com/berkeley-abc/abc

  \author Alan Mishchenko (UC Berkeley)
*/

#pragma once

#include "abcresub.hpp"

namespace abcresub
{

static word s_Truths6[6] = {
    ABC_CONST(0xAAAAAAAAAAAAAAAA),
    ABC_CONST(0xCCCCCCCCCCCCCCCC),
    ABC_CONST(0xF0F0F0F0F0F0F0F0),
    ABC_CONST(0xFF00FF00FF00FF00),
    ABC_CONST(0xFFFF0000FFFF0000),
    ABC_CONST(0xFFFFFFFF00000000)
};

typedef struct Gia_Rsb2Man_t_ Gia_Rsb2Man_t;
struct Gia_Rsb2Man_t_
{
    // hyper-parameters
    int            nDivsMax;
    int            nLevelIncrease;
    int            fUseXor;
    int            fUseZeroCost;
    int            fDebug;
    int            fVerbose;
    // input AIG
    int            nObjs;
    int            nPis;
    int            nNodes;
    int            nPos;
    int            iFirstPo;
    int            Level;
    int            nMffc;
    // intermediate data
    Vec_Int_t      vObjs;
    Vec_Wrd_t      vSims;
    Vec_Ptr_t      vpDivs;
    Vec_Int_t      vDivs;
    Vec_Int_t      vLevels;
    Vec_Int_t      vRefs;
    Vec_Int_t      vCopies;
    Vec_Int_t      vTried;
    word           Truth0;
    word           Truth1;
    word           CareSet;
};

inline Gia_Rsb2Man_t * Gia_Rsb2ManAlloc()
{
    Gia_Rsb2Man_t * p = ABC_CALLOC( Gia_Rsb2Man_t, 1 );
    return p;
}

inline void Gia_Rsb2ManFree( Gia_Rsb2Man_t * p )
{
    Vec_IntErase( &p->vObjs   );
    Vec_WrdErase( &p->vSims   );
    Vec_PtrErase( &p->vpDivs  );
    Vec_IntErase( &p->vDivs   );
    Vec_IntErase( &p->vLevels );
    Vec_IntErase( &p->vRefs   );
    Vec_IntErase( &p->vCopies );
    Vec_IntErase( &p->vTried );
    ABC_FREE( p );
}

inline void Gia_Rsb2ManStart( Gia_Rsb2Man_t * p, int * pObjs, int nObjs, int nDivsMax, int nLevelIncrease, int fUseXor, int fUseZeroCost, int fDebug, int fVerbose )
{
    int i;
    // hyper-parameters
    p->nDivsMax       = nDivsMax;
    p->nLevelIncrease = nLevelIncrease;
    p->fUseXor        = fUseXor;
    p->fUseZeroCost   = fUseZeroCost;
    p->fDebug         = fDebug;
    p->fVerbose       = fVerbose;
    // user data
    Vec_IntClear( &p->vObjs );
    Vec_IntPushArray( &p->vObjs, pObjs, 2*nObjs );
    assert( pObjs[0] == 0 );
    assert( pObjs[1] == 0 );
    p->nObjs    = nObjs;
    p->nPis     = 0;
    p->nNodes   = 0;
    p->nPos     = 0;
    p->iFirstPo = 0;
    for ( i = 1; i < nObjs; i++ )
    {
        if ( pObjs[2*i+0] == 0 && pObjs[2*i+1] == 0 )
            p->nPis++;
        else if ( pObjs[2*i+0] == pObjs[2*i+1] )
            p->nPos++;
        else
            p->nNodes++;
    }
    assert( nObjs == 1 + p->nPis + p->nNodes + p->nPos );
    p->iFirstPo = nObjs - p->nPos;
    Vec_WrdClear( &p->vSims );
    Vec_WrdGrow( &p->vSims, 2*nObjs );
    Vec_WrdPush( &p->vSims, 0 );
    Vec_WrdPush( &p->vSims, 0 );
    for ( i = 0; i < p->nPis; i++ )
    {
        Vec_WrdPush( &p->vSims,  s_Truths6[i] );
        Vec_WrdPush( &p->vSims, ~s_Truths6[i] );
    }
    p->vSims.nSize = 2*p->nObjs;
    Vec_IntClear( &p->vDivs   );
    Vec_IntClear( &p->vLevels );
    Vec_IntClear( &p->vRefs   );
    Vec_IntClear( &p->vCopies );
    Vec_IntClear( &p->vTried  );
    Vec_PtrClear( &p->vpDivs  );
    Vec_IntGrow( &p->vDivs,   nObjs );
    Vec_IntGrow( &p->vLevels, nObjs );
    Vec_IntGrow( &p->vRefs,   nObjs );
    Vec_IntGrow( &p->vCopies, nObjs );
    Vec_IntGrow( &p->vTried,  nObjs );
    Vec_PtrGrow( &p->vpDivs,  nObjs );
}

inline void Gia_Rsb2ManPrint( Gia_Rsb2Man_t * p )
{
    int i, * pObjs = Vec_IntArray( &p->vObjs );
    printf( "PI = %d.  PO = %d.  Obj = %d.\n", p->nPis, p->nPos, p->nObjs );
    for ( i = p->nPis + 1; i < p->iFirstPo; i++ )
        printf( "%2d = %c%2d & %c%2d;\n", i,
            Abc_LitIsCompl(pObjs[2*i+0]) ? '!' : ' ', Abc_Lit2Var(pObjs[2*i+0]),
            Abc_LitIsCompl(pObjs[2*i+1]) ? '!' : ' ', Abc_Lit2Var(pObjs[2*i+1]) );
    for ( i = p->iFirstPo; i < p->nObjs; i++ )
        printf( "%2d = %c%2d;\n", i,
            Abc_LitIsCompl(pObjs[2*i+0]) ? '!' : ' ', Abc_Lit2Var(pObjs[2*i+0]) );
}

inline int Gia_Rsb2ManLevel( Gia_Rsb2Man_t * p )
{
    int i, * pLevs, Level = 0;
    Vec_IntClear( &p->vLevels );
    Vec_IntGrow( &p->vLevels, p->nObjs );
    pLevs = Vec_IntArray( &p->vLevels );
    for ( i = p->nPis + 1; i < p->iFirstPo; i++ )
        pLevs[i] = 1 + Abc_MaxInt( pLevs[2*i+0]/2, pLevs[2*i+1]/2 );
    for ( i = p->iFirstPo; i < p->nObjs; i++ )
        Level = Abc_MaxInt( Level, pLevs[i] = pLevs[2*i+0]/2 );
    return Level;
}

inline word Gia_Rsb2ManOdcs( Gia_Rsb2Man_t * p, int iNode )
{
    int i; word Res = 0;
    int  * pObjs = Vec_IntArray( &p->vObjs );
    word * pSims = Vec_WrdArray( &p->vSims );
    for ( i = p->nPis + 1; i < p->iFirstPo; i++ )
    {
        if ( pObjs[2*i+0] < pObjs[2*i+1] )
            pSims[2*i+0] = pSims[pObjs[2*i+0]] & pSims[pObjs[2*i+1]];
        else if ( pObjs[2*i+0] > pObjs[2*i+1] )
            pSims[2*i+0] = pSims[pObjs[2*i+0]] ^ pSims[pObjs[2*i+1]];
        else assert( 0 );
        pSims[2*i+1] = ~pSims[2*i+0];
    }
    for ( i = p->iFirstPo; i < p->nObjs; i++ )
        pSims[2*i+0] = pSims[pObjs[2*i+0]];
    ABC_SWAP( word, pSims[2*iNode+0], pSims[2*iNode+1] );
    for ( i = iNode + 1; i < p->iFirstPo; i++ )
    {
        if ( pObjs[2*i+0] < pObjs[2*i+1] )
            pSims[2*i+0] = pSims[pObjs[2*i+0]] & pSims[pObjs[2*i+1]];
        else if ( pObjs[2*i+0] < pObjs[2*i+1] )
            pSims[2*i+0] = pSims[pObjs[2*i+0]] ^ pSims[pObjs[2*i+1]];
        else assert( 0 );
        pSims[2*i+1] = ~pSims[2*i+0];
    }
    for ( i = p->iFirstPo; i < p->nObjs; i++ )
        Res |= pSims[2*i+0] ^ pSims[pObjs[2*i+0]];
    ABC_SWAP( word, pSims[2*iNode+0], pSims[2*iNode+1] );
    return Res;
}

// marks MFFC and returns its size
inline int Gia_Rsb2ManDeref_rec( Gia_Rsb2Man_t * p, int * pObjs, int * pRefs, int iNode )
{
    int Counter = 1;
    if ( iNode <= p->nPis )
        return 0;
    if ( --pRefs[Abc_Lit2Var(pObjs[2*iNode+0])] == 0 )
        Counter += Gia_Rsb2ManDeref_rec( p, pObjs, pRefs, Abc_Lit2Var(pObjs[2*iNode+0]) );
    if ( --pRefs[Abc_Lit2Var(pObjs[2*iNode+1])] == 0 )
        Counter += Gia_Rsb2ManDeref_rec( p, pObjs, pRefs, Abc_Lit2Var(pObjs[2*iNode+1]) );
    return Counter;
}

inline int Gia_Rsb2ManMffc( Gia_Rsb2Man_t * p, int iNode )
{
    int i, * pRefs, * pObjs;
    Vec_IntFill( &p->vRefs, p->nObjs, 0 );
    pRefs = Vec_IntArray( &p->vRefs );
    pObjs = Vec_IntArray( &p->vObjs );
    assert( pObjs[2*iNode+0] != pObjs[2*iNode+1] );
    for ( i = p->nPis + 1; i < p->iFirstPo; i++ )
        pRefs[Abc_Lit2Var(pObjs[2*i+0])]++,
        pRefs[Abc_Lit2Var(pObjs[2*i+1])]++;
    for ( i = p->iFirstPo; i < p->nObjs; i++ )
        pRefs[Abc_Lit2Var(pObjs[2*i+0])]++;
    for ( i = p->nPis + 1; i < p->iFirstPo; i++ )
        assert( pRefs[i] );
    pRefs[iNode] = 0;
    for ( i = iNode + 1; i < p->iFirstPo; i++ )
        if ( !pRefs[Abc_Lit2Var(pObjs[2*i+0])] || !pRefs[Abc_Lit2Var(pObjs[2*i+1])] )
            pRefs[i] = 0;
    return Gia_Rsb2ManDeref_rec( p, pObjs, pRefs, iNode );
}

// collects divisors and maps them into nodes
// assumes MFFC is already marked
inline int Gia_Rsb2ManDivs( Gia_Rsb2Man_t * p, int iNode )
{
    int i, iNodeLevel = 0;
    int * pRefs = Vec_IntArray( &p->vRefs );
    p->CareSet = Gia_Rsb2ManOdcs( p, iNode );
    p->Truth1 = p->CareSet & Vec_WrdEntry(&p->vSims, 2*iNode);
    p->Truth0 = p->CareSet & ~p->Truth1;
    Vec_PtrClear( &p->vpDivs );
    Vec_PtrPush( &p->vpDivs, &p->Truth0 );
    Vec_PtrPush( &p->vpDivs, &p->Truth1 );
    Vec_IntClear( &p->vDivs );
    Vec_IntPushTwo( &p->vDivs, -1, -1 );
    for ( i = 1; i <= p->nPis; i++ )
    {
        Vec_PtrPush( &p->vpDivs, Vec_WrdEntryP(&p->vSims, 2*i) );
        Vec_IntPush( &p->vDivs, i );
    }
    p->nMffc = Gia_Rsb2ManMffc( p, iNode );
    if ( p->nLevelIncrease >= 0 )
    {
        p->Level = Gia_Rsb2ManLevel(p);
        iNodeLevel = Vec_IntEntry(&p->vLevels, iNode);
    }
    for ( i = p->nPis + 1; i < p->iFirstPo; i++ )
    {
        if ( !pRefs[i] || (p->nLevelIncrease >= 0 && Vec_IntEntry(&p->vLevels, i) > iNodeLevel + p->nLevelIncrease) )
            continue;
        Vec_PtrPush( &p->vpDivs, Vec_WrdEntryP(&p->vSims, 2*i) );
        Vec_IntPush( &p->vDivs, i );
    }
    assert( Vec_IntSize(&p->vDivs) == Vec_PtrSize(&p->vpDivs) );
    return Vec_IntSize(&p->vDivs);
}

inline int Gia_Rsb2AddNode( Vec_Int_t * vRes, int iLit0, int iLit1, int iRes0, int iRes1 )
{
    int iLitMin = iRes0 < iRes1 ? Abc_LitNotCond(iRes0, Abc_LitIsCompl(iLit0)) : Abc_LitNotCond(iRes1, Abc_LitIsCompl(iLit1));
    int iLitMax = iRes0 < iRes1 ? Abc_LitNotCond(iRes1, Abc_LitIsCompl(iLit1)) : Abc_LitNotCond(iRes0, Abc_LitIsCompl(iLit0));
    int iLitRes = Vec_IntSize(vRes);
    if ( iLit0 < iLit1 ) // and
    {
        if ( iLitMin == 0 )
            return 0;
        if ( iLitMin == 1 )
            return iLitMax;
        if ( iLitMin == Abc_LitNot(iLitMax) )
            return 0;
    }
    else if ( iLit0 > iLit1 ) // xor
    {
        if ( iLitMin == 0 )
            return iLitMax;
        if ( iLitMin == 1 )
            return Abc_LitNot(iLitMax);
        if ( iLitMin == Abc_LitNot(iLitMax) )
            return 1;
    }
    else assert( 0 );
    assert( iLitMin >= 2 && iLitMax >= 2 );
    if ( iLit0 < iLit1 ) // and
    {
        Vec_IntPushTwo( vRes, iLitMin, iLitMax );
    }
    else if ( iLit0 > iLit1 ) // xor
    {
        assert( !Abc_LitIsCompl(iLit0) );
        assert( !Abc_LitIsCompl(iLit1) );
        Vec_IntPushTwo( vRes, iLitMax, iLitMin );
    }
    else assert( 0 );
    return iLitRes;
}

inline int Gia_Rsb2ManInsert_rec( Vec_Int_t * vRes, int nPis, Vec_Int_t * vObjs, int iNode, Vec_Int_t * vResub, Vec_Int_t * vDivs, Vec_Int_t * vCopies, int iObj )
{
    if ( Vec_IntEntry(vCopies, iObj) >= 0 )
        return Vec_IntEntry(vCopies, iObj);
    assert( iObj > nPis );
    if ( iObj == iNode )
    {
        int nVars = Vec_IntSize(vDivs);
        int iLitRes = -1, iTopLit = Vec_IntEntryLast( vResub );
        if ( Abc_Lit2Var(iTopLit) == 0 )
            iLitRes = 0;
        else if ( Abc_Lit2Var(iTopLit) < nVars )
            iLitRes = Gia_Rsb2ManInsert_rec( vRes, nPis, vObjs, -1, vResub, vDivs, vCopies, Vec_IntEntry(vDivs, Abc_Lit2Var(iTopLit)) );
        else
        {
            Vec_Int_t * vCopy = Vec_IntAlloc( 10 );
            int k, iLit, iLit0, iLit1;
            Vec_IntForEachEntryStop( vResub, iLit, k, Vec_IntSize(vResub)-1 )
                if ( Abc_Lit2Var(iLit) < nVars )
                    Gia_Rsb2ManInsert_rec( vRes, nPis, vObjs, -1, vResub, vDivs, vCopies, Vec_IntEntry(vDivs, Abc_Lit2Var(iLit)) );
            Vec_IntForEachEntryDouble( vResub, iLit0, iLit1, k )
            {
                int iVar0 = Abc_Lit2Var(iLit0);
                int iVar1 = Abc_Lit2Var(iLit1);
                int iRes0 = iVar0 < nVars ? Vec_IntEntry(vCopies, Vec_IntEntry(vDivs, iVar0)) : Vec_IntEntry(vCopy, iVar0 - nVars);
                int iRes1 = iVar1 < nVars ? Vec_IntEntry(vCopies, Vec_IntEntry(vDivs, iVar1)) : Vec_IntEntry(vCopy, iVar1 - nVars);
                iLitRes   = Gia_Rsb2AddNode( vRes, iLit0, iLit1, iRes0, iRes1 );
                Vec_IntPush( vCopy, iLitRes );
            }
            Vec_IntFree( vCopy );
        }
        iLitRes = Abc_LitNotCond( iLitRes, Abc_LitIsCompl(iTopLit) );
        Vec_IntWriteEntry( vCopies, iObj, iLitRes );
        return iLitRes;
    }
    else
    {
        int iLit0 = Vec_IntEntry( vObjs, 2*iObj+0 );
        int iLit1 = Vec_IntEntry( vObjs, 2*iObj+1 );
        int iRes0 = Gia_Rsb2ManInsert_rec( vRes, nPis, vObjs, iNode, vResub, vDivs, vCopies, Abc_Lit2Var(iLit0) );
        int iRes1 = Gia_Rsb2ManInsert_rec( vRes, nPis, vObjs, iNode, vResub, vDivs, vCopies, Abc_Lit2Var(iLit1) );
        int iLitRes = Gia_Rsb2AddNode( vRes, iLit0, iLit1, iRes0, iRes1 );
        Vec_IntWriteEntry( vCopies, iObj, iLitRes );
        return iLitRes;
    }
}

inline Vec_Int_t * Gia_Rsb2ManInsert( int nPis, int nPos, Vec_Int_t * vObjs, int iNode, Vec_Int_t * vResub, Vec_Int_t * vDivs, Vec_Int_t * vCopies )
{
    int i, nObjs = Vec_IntSize(vObjs)/2, iFirstPo = nObjs - nPos;
    Vec_Int_t * vRes = Vec_IntAlloc( Vec_IntSize(vObjs) );
//Vec_IntPrint( vDivs );
//Vec_IntPrint( vResub );
    Vec_IntFill( vCopies, Vec_IntSize(vObjs), -1 );
    Vec_IntFill( vRes, 2*(nPis + 1), 0 );
    for ( i = 0; i <= nPis; i++ )
        Vec_IntWriteEntry( vCopies, i, 2*i );
    for ( i = iFirstPo; i < nObjs; i++ )
        Gia_Rsb2ManInsert_rec( vRes, nPis, vObjs, iNode, vResub, vDivs, vCopies, Abc_Lit2Var( Vec_IntEntry(vObjs, 2*i) ) );
    for ( i = iFirstPo; i < nObjs; i++ )
    {
        int iLitNew = Abc_Lit2LitL( Vec_IntArray(vCopies), Vec_IntEntry(vObjs, 2*i) );
        Vec_IntPushTwo( vRes, iLitNew, iLitNew );
    }
    return vRes;
}

inline int Abc_ResubNodeToTry( Vec_Int_t * vTried, int iFirst, int iLast )
{
    int iNode;
    //for ( iNode = iFirst; iNode < iLast; iNode++ )
    for ( iNode = iLast - 1; iNode >= iFirst; iNode-- )
        if ( Vec_IntFind(vTried, iNode) == -1 )
            return iNode;
    return -1;
}

inline int Abc_ResubComputeWindow( int * pObjs, int nObjs, int nDivsMax, int nLevelIncrease, int fUseXor, int fUseZeroCost, int fDebug, int fVerbose, int ** ppArray, int * pnResubs )
{
    int iNode, nChanges = 0, RetValue = 0;
    Gia_Rsb2Man_t * p = Gia_Rsb2ManAlloc();
    Gia_Rsb2ManStart( p, pObjs, nObjs, nDivsMax, nLevelIncrease, fUseXor, fUseZeroCost, fDebug, fVerbose );
    *ppArray = NULL;
    while ( (iNode = Abc_ResubNodeToTry(&p->vTried, p->nPis+1, p->iFirstPo)) > 0 )
    {
        int nDivs = Gia_Rsb2ManDivs( p, iNode );
        int * pResub, nResub = Abc_ResubComputeFunction( Vec_PtrArray(&p->vpDivs), nDivs, 1, p->nMffc-1, nDivsMax, 0, fUseXor, fDebug, fVerbose, &pResub );
        if ( nResub == 0 )
            Vec_IntPush( &p->vTried, iNode );
        else
        {
            int i, k = 0, iTried;
            Vec_Int_t vResub = { nResub, nResub, pResub };
            Vec_Int_t * vRes = Gia_Rsb2ManInsert( p->nPis, p->nPos, &p->vObjs, iNode, &vResub, &p->vDivs, &p->vCopies );
            //printf( "\nResubing node %d:\n", iNode );
            //Gia_Rsb2ManPrint( p );
            p->nObjs    = Vec_IntSize(vRes)/2;
            p->iFirstPo = p->nObjs - p->nPos;
            Vec_IntClear( &p->vObjs );
            Vec_IntAppend( &p->vObjs, vRes );
            Vec_IntFree( vRes );
            Vec_IntForEachEntry( &p->vTried, iTried, i )
                if ( Vec_IntEntry(&p->vCopies, iTried) > Abc_Var2Lit(p->nPis, 0) ) // internal node
                    Vec_IntWriteEntry( &p->vTried, k++, Abc_Lit2Var(Vec_IntEntry(&p->vCopies, iTried)) );
            Vec_IntShrink( &p->vTried, k );
            nChanges++;
            //Gia_Rsb2ManPrint( p );
        }
    }
    if ( nChanges )
    {
        RetValue = p->nObjs;
        *ppArray = p->vObjs.pArray;
        Vec_IntZero( &p->vObjs );
    }
    Gia_Rsb2ManFree( p );
    if ( pnResubs )
        *pnResubs = nChanges;
    return RetValue;
}

} /* abcresub */
