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

#include "abc/system.h"
#include "abc/Dimacs.h"
#include "abc/SimpSolver.h"

#include "abc/AbcGlucose.h"
#include "abc/abc_global.h"

//#include "base/abc/abc.h"
//#include "sat/cnf/cnf.h"
//#include "misc/extra/extra.h"

ABC_NAMESPACE_IMPL_START

using namespace Gluco;

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
Gluco::SimpSolver * glucose_solver_start()
{
    SimpSolver * S = new SimpSolver;
    S->setIncrementalMode();
    return S;
}

void glucose_solver_stop(Gluco::SimpSolver* S)
{
    delete S;
}

void glucose_solver_reset(Gluco::SimpSolver* S)
{
    S->reset();
}

int glucose_solver_addclause(Gluco::SimpSolver* S, int * plits, int nlits)
{
    vec<Lit> lits;
    for ( int i = 0; i < nlits; i++,plits++)
    {
        // note: Glucose uses the same var->lit conventiaon as ABC
        while ((*plits)/2 >= S->nVars()) S->newVar();
        assert((*plits)/2 < S->nVars()); // NOTE: since we explicitely use new function bmc_add_var
        Lit p;
        p.x = *plits;
        lits.push(p);
    }
    return S->addClause(lits); // returns 0 if the problem is UNSAT
}

void glucose_solver_setcallback(Gluco::SimpSolver* S, void * pman, int(*pfunc)(void*, int, int*))
{
    S->pCnfMan = pman;
    S->pCnfFunc = pfunc;
    S->nCallConfl = 1000;
}

int glucose_solver_solve(Gluco::SimpSolver* S, int * plits, int nlits)
{
    vec<Lit> lits;
    for (int i=0;i<nlits;i++,plits++)
    {
        Lit p;
        p.x = *plits;
        lits.push(p);
    }
    Gluco::lbool res = S->solveLimited(lits, 0);
    return (res == l_True ? 1 : res == l_False ? -1 : 0);
}

int glucose_solver_addvar(Gluco::SimpSolver* S)
{
    S->newVar();
    return S->nVars() - 1;
}

int glucose_solver_read_cex_varvalue(Gluco::SimpSolver* S, int ivar)
{
    return S->model[ivar] == l_True;
}

void glucose_solver_setstop(Gluco::SimpSolver* S, int * pstop)
{
    S->pstop = pstop;
}


/**Function*************************************************************

  Synopsis    [Wrapper APIs to calling from ABC.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bmcg_sat_solver * bmcg_sat_solver_start() 
{
    return (bmcg_sat_solver *)glucose_solver_start();
}
void bmcg_sat_solver_stop(bmcg_sat_solver* s)
{
    glucose_solver_stop((Gluco::SimpSolver*)s);
}
void bmcg_sat_solver_reset(bmcg_sat_solver* s)
{
    glucose_solver_reset((Gluco::SimpSolver*)s);
}


int bmcg_sat_solver_addclause(bmcg_sat_solver* s, int * plits, int nlits)
{
    return glucose_solver_addclause((Gluco::SimpSolver*)s,plits,nlits);
}

void bmcg_sat_solver_setcallback(bmcg_sat_solver* s, void * pman, int(*pfunc)(void*, int, int*))
{
    glucose_solver_setcallback((Gluco::SimpSolver*)s,pman,pfunc);
}

int bmcg_sat_solver_solve(bmcg_sat_solver* s, int * plits, int nlits)
{
    return glucose_solver_solve((Gluco::SimpSolver*)s,plits,nlits);
}

int bmcg_sat_solver_final(bmcg_sat_solver* s, int ** ppArray)
{
    *ppArray = (int *)(Lit *)((Gluco::SimpSolver*)s)->conflict;
    return ((Gluco::SimpSolver*)s)->conflict.size();
}

int bmcg_sat_solver_addvar(bmcg_sat_solver* s)
{
    return glucose_solver_addvar((Gluco::SimpSolver*)s);
}

void bmcg_sat_solver_set_nvars( bmcg_sat_solver* s, int nvars )
{
    int i;
    for ( i = bmcg_sat_solver_varnum(s); i < nvars; i++ )
        bmcg_sat_solver_addvar(s);
}

int bmcg_sat_solver_eliminate( bmcg_sat_solver* s, int turn_off_elim )
{
//    return 1; 
    return ((Gluco::SimpSolver*)s)->eliminate(turn_off_elim != 0);
}

int bmcg_sat_solver_var_is_elim( bmcg_sat_solver* s, int v )
{
//    return 0; 
    return ((Gluco::SimpSolver*)s)->isEliminated(v);
}

void bmcg_sat_solver_var_set_frozen( bmcg_sat_solver* s, int v, int freeze )
{
    ((Gluco::SimpSolver*)s)->setFrozen(v, freeze != 0);
}

int bmcg_sat_solver_elim_varnum(bmcg_sat_solver* s)
{
//    return 0; 
    return ((Gluco::SimpSolver*)s)->eliminated_vars;
}

int bmcg_sat_solver_read_cex_varvalue(bmcg_sat_solver* s, int ivar)
{
    return glucose_solver_read_cex_varvalue((Gluco::SimpSolver*)s, ivar);
}

void bmcg_sat_solver_set_stop(bmcg_sat_solver* s, int * pstop)
{
    glucose_solver_setstop((Gluco::SimpSolver*)s, pstop);
}

abctime bmcg_sat_solver_set_runtime_limit(bmcg_sat_solver* s, abctime Limit)
{
    abctime nRuntimeLimit = ((Gluco::SimpSolver*)s)->nRuntimeLimit;
    ((Gluco::SimpSolver*)s)->nRuntimeLimit = Limit;
    return nRuntimeLimit;
}

void bmcg_sat_solver_set_conflict_budget(bmcg_sat_solver* s, int Limit)
{
    if ( Limit > 0 ) 
        ((Gluco::SimpSolver*)s)->setConfBudget( (int64_t)Limit );
    else 
        ((Gluco::SimpSolver*)s)->budgetOff();
}

int bmcg_sat_solver_varnum(bmcg_sat_solver* s)
{
    return ((Gluco::SimpSolver*)s)->nVars();
}
int bmcg_sat_solver_clausenum(bmcg_sat_solver* s)
{
    return ((Gluco::SimpSolver*)s)->nClauses();
}
int bmcg_sat_solver_learntnum(bmcg_sat_solver* s)
{
    return ((Gluco::SimpSolver*)s)->nLearnts();
}
int bmcg_sat_solver_conflictnum(bmcg_sat_solver* s)
{
    return ((Gluco::SimpSolver*)s)->conflicts;
}

int bmcg_sat_solver_minimize_assumptions( bmcg_sat_solver * s, int * plits, int nlits, int pivot )
{
    vec<int>*array = &((Gluco::SimpSolver*)s)->user_vec;
    int i, nlitsL, nlitsR, nresL, nresR, status;
    assert( pivot >= 0 );
//    assert( nlits - pivot >= 2 );
    assert( nlits - pivot >= 1 );
    if ( nlits - pivot == 1 )
    {
        // since the problem is UNSAT, we try to solve it without assuming the last literal
        // if the result is UNSAT, the last literal can be dropped; otherwise, it is needed
        status = bmcg_sat_solver_solve( s, plits, pivot );
        return status != GLUCOSE_UNSAT; // return 1 if the problem is not UNSAT
    }
    assert( nlits - pivot >= 2 );
    nlitsL = (nlits - pivot) / 2;
    nlitsR = (nlits - pivot) - nlitsL;
    assert( nlitsL + nlitsR == nlits - pivot );
    // solve with these assumptions
    status = bmcg_sat_solver_solve( s, plits, pivot + nlitsL );
    if ( status == GLUCOSE_UNSAT ) // these are enough
        return bmcg_sat_solver_minimize_assumptions( s, plits, pivot + nlitsL, pivot );
    // these are not enough
    // solve for the right lits
//    nResL = nLitsR == 1 ? 1 : sat_solver_minimize_assumptions( s, pLits + nLitsL, nLitsR, nConfLimit );
    nresL = nlitsR == 1 ? 1 : bmcg_sat_solver_minimize_assumptions( s, plits, nlits, pivot + nlitsL );
    // swap literals
    array->clear();
    for ( i = 0; i < nlitsL; i++ )
        array->push(plits[pivot + i]);
    for ( i = 0; i < nresL; i++ )
        plits[pivot + i] = plits[pivot + nlitsL + i];
    for ( i = 0; i < nlitsL; i++ )
        plits[pivot + nresL + i] = (*array)[i];
    // solve with these assumptions
    status = bmcg_sat_solver_solve( s, plits, pivot + nresL );
    if ( status == GLUCOSE_UNSAT ) // these are enough
        return nresL;
    // solve for the left lits
//    nResR = nLitsL == 1 ? 1 : sat_solver_minimize_assumptions( s, pLits + nResL, nLitsL, nConfLimit );
    nresR = nlitsL == 1 ? 1 : bmcg_sat_solver_minimize_assumptions( s, plits, pivot + nresL + nlitsL, pivot + nresL );
    return nresL + nresR;
}

int bmcg_sat_solver_add_and( bmcg_sat_solver * s, int iVar, int iVar0, int iVar1, int fCompl0, int fCompl1, int fCompl )
{
    int Lits[3];

    Lits[0] = Abc_Var2Lit( iVar, !fCompl );
    Lits[1] = Abc_Var2Lit( iVar0, fCompl0 );
    if ( !bmcg_sat_solver_addclause( s, Lits, 2 ) )
        return 0;

    Lits[0] = Abc_Var2Lit( iVar, !fCompl );
    Lits[1] = Abc_Var2Lit( iVar1, fCompl1 );
    if ( !bmcg_sat_solver_addclause( s, Lits, 2 ) )
        return 0;

    Lits[0] = Abc_Var2Lit( iVar,   fCompl );
    Lits[1] = Abc_Var2Lit( iVar0, !fCompl0 );
    Lits[2] = Abc_Var2Lit( iVar1, !fCompl1 );
    if ( !bmcg_sat_solver_addclause( s, Lits, 3 ) )
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
Gluco::Solver * glucose_solver_start()
{
    Solver * S = new Solver;
    S->setIncrementalMode();
    return S;
}

void glucose_solver_stop(Gluco::Solver* S)
{
    delete S;
}

int glucose_solver_addclause(Gluco::Solver* S, int * plits, int nlits)
{
    vec<Lit> lits;
    for ( int i = 0; i < nlits; i++,plits++)
    {
        // note: Glucose uses the same var->lit conventiaon as ABC
        while ((*plits)/2 >= S->nVars()) S->newVar();
        assert((*plits)/2 < S->nVars()); // NOTE: since we explicitely use new function bmc_add_var
        Lit p;
        p.x = *plits;
        lits.push(p);
    }
    return S->addClause(lits); // returns 0 if the problem is UNSAT
}

void glucose_solver_setcallback(Gluco::Solver* S, void * pman, int(*pfunc)(void*, int, int*))
{
    S->pCnfMan = pman;
    S->pCnfFunc = pfunc;
    S->nCallConfl = 1000;
}

int glucose_solver_solve(Gluco::Solver* S, int * plits, int nlits)
{
    vec<Lit> lits;
    for (int i=0;i<nlits;i++,plits++)
    {
        Lit p;
        p.x = *plits;
        lits.push(p);
    }
    Gluco::lbool res = S->solveLimited(lits);
    return (res == l_True ? 1 : res == l_False ? -1 : 0);
}

int glucose_solver_addvar(Gluco::Solver* S)
{
    S->newVar();
    return S->nVars() - 1;
}

int glucose_solver_read_cex_varvalue(Gluco::Solver* S, int ivar)
{
    return S->model[ivar] == l_True;
}

void glucose_solver_setstop(Gluco::Solver* S, int * pstop)
{
    S->pstop = pstop;
}


/**Function*************************************************************

  Synopsis    [Wrapper APIs to calling from ABC.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bmcg_sat_solver * bmcg_sat_solver_start() 
{
    return (bmcg_sat_solver *)glucose_solver_start();
}
void bmcg_sat_solver_stop(bmcg_sat_solver* s)
{
    glucose_solver_stop((Gluco::Solver*)s);
}

int bmcg_sat_solver_addclause(bmcg_sat_solver* s, int * plits, int nlits)
{
    return glucose_solver_addclause((Gluco::Solver*)s,plits,nlits);
}

void bmcg_sat_solver_setcallback(bmcg_sat_solver* s, void * pman, int(*pfunc)(void*, int, int*))
{
    glucose_solver_setcallback((Gluco::Solver*)s,pman,pfunc);
}

int bmcg_sat_solver_solve(bmcg_sat_solver* s, int * plits, int nlits)
{
    return glucose_solver_solve((Gluco::Solver*)s,plits,nlits);
}

int bmcg_sat_solver_final(bmcg_sat_solver* s, int ** ppArray)
{
    *ppArray = (int *)(Lit *)((Gluco::Solver*)s)->conflict;
    return ((Gluco::Solver*)s)->conflict.size();
}

int bmcg_sat_solver_addvar(bmcg_sat_solver* s)
{
    return glucose_solver_addvar((Gluco::Solver*)s);
}

void bmcg_sat_solver_set_nvars( bmcg_sat_solver* s, int nvars )
{
    int i;
    for ( i = bmcg_sat_solver_varnum(s); i < nvars; i++ )
        bmcg_sat_solver_addvar(s);
}

int bmcg_sat_solver_eliminate( bmcg_sat_solver* s, int turn_off_elim )
{
    return 1; 
//    return ((Gluco::SimpSolver*)s)->eliminate(turn_off_elim != 0);
}

int bmcg_sat_solver_var_is_elim( bmcg_sat_solver* s, int v )
{
    return 0; 
//    return ((Gluco::SimpSolver*)s)->isEliminated(v);
}

void bmcg_sat_solver_var_set_frozen( bmcg_sat_solver* s, int v, int freeze )
{
//    ((Gluco::SimpSolver*)s)->setFrozen(v, freeze);
}

int bmcg_sat_solver_elim_varnum(bmcg_sat_solver* s)
{
    return 0;
//    return ((Gluco::SimpSolver*)s)->eliminated_vars;
}

int bmcg_sat_solver_read_cex_varvalue(bmcg_sat_solver* s, int ivar)
{
    return glucose_solver_read_cex_varvalue((Gluco::Solver*)s, ivar);
}

void bmcg_sat_solver_set_stop(bmcg_sat_solver* s, int * pstop)
{
    glucose_solver_setstop((Gluco::Solver*)s, pstop);
}

abctime bmcg_sat_solver_set_runtime_limit(bmcg_sat_solver* s, abctime Limit)
{
    abctime nRuntimeLimit = ((Gluco::Solver*)s)->nRuntimeLimit;
    ((Gluco::Solver*)s)->nRuntimeLimit = Limit;
    return nRuntimeLimit;
}

void bmcg_sat_solver_set_conflict_budget(bmcg_sat_solver* s, int Limit)
{
    if ( Limit > 0 ) 
        ((Gluco::Solver*)s)->setConfBudget( (int64_t)Limit );
    else 
        ((Gluco::Solver*)s)->budgetOff();
}

int bmcg_sat_solver_varnum(bmcg_sat_solver* s)
{
    return ((Gluco::Solver*)s)->nVars();
}
int bmcg_sat_solver_clausenum(bmcg_sat_solver* s)
{
    return ((Gluco::Solver*)s)->nClauses();
}
int bmcg_sat_solver_learntnum(bmcg_sat_solver* s)
{
    return ((Gluco::Solver*)s)->nLearnts();
}
int bmcg_sat_solver_conflictnum(bmcg_sat_solver* s)
{
    return ((Gluco::Solver*)s)->conflicts;
}

int bmcg_sat_solver_minimize_assumptions( bmcg_sat_solver * s, int * plits, int nlits, int pivot )
{
    vec<int>*array = &((Gluco::Solver*)s)->user_vec;
    int i, nlitsL, nlitsR, nresL, nresR, status;
    assert( pivot >= 0 );
//    assert( nlits - pivot >= 2 );
    assert( nlits - pivot >= 1 );
    if ( nlits - pivot == 1 )
    {
        // since the problem is UNSAT, we try to solve it without assuming the last literal
        // if the result is UNSAT, the last literal can be dropped; otherwise, it is needed
        status = bmcg_sat_solver_solve( s, plits, pivot );
        return status != GLUCOSE_UNSAT; // return 1 if the problem is not UNSAT
    }
    assert( nlits - pivot >= 2 );
    nlitsL = (nlits - pivot) / 2;
    nlitsR = (nlits - pivot) - nlitsL;
    assert( nlitsL + nlitsR == nlits - pivot );
    // solve with these assumptions
    status = bmcg_sat_solver_solve( s, plits, pivot + nlitsL );
    if ( status == GLUCOSE_UNSAT ) // these are enough
        return bmcg_sat_solver_minimize_assumptions( s, plits, pivot + nlitsL, pivot );
    // these are not enough
    // solve for the right lits
//    nResL = nLitsR == 1 ? 1 : sat_solver_minimize_assumptions( s, pLits + nLitsL, nLitsR, nConfLimit );
    nresL = nlitsR == 1 ? 1 : bmcg_sat_solver_minimize_assumptions( s, plits, nlits, pivot + nlitsL );
    // swap literals
    array->clear();
    for ( i = 0; i < nlitsL; i++ )
        array->push(plits[pivot + i]);
    for ( i = 0; i < nresL; i++ )
        plits[pivot + i] = plits[pivot + nlitsL + i];
    for ( i = 0; i < nlitsL; i++ )
        plits[pivot + nresL + i] = (*array)[i];
    // solve with these assumptions
    status = bmcg_sat_solver_solve( s, plits, pivot + nresL );
    if ( status == GLUCOSE_UNSAT ) // these are enough
        return nresL;
    // solve for the left lits
//    nResR = nLitsL == 1 ? 1 : sat_solver_minimize_assumptions( s, pLits + nResL, nLitsL, nConfLimit );
    nresR = nlitsL == 1 ? 1 : bmcg_sat_solver_minimize_assumptions( s, plits, pivot + nresL + nlitsL, pivot + nresL );
    return nresL + nresR;
}

#endif


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void glucose_print_stats(SimpSolver& s, abctime clk)
{
    double cpu_time = (double)(unsigned)clk / CLOCKS_PER_SEC;
    printf("c restarts              : %d (%d conflicts on average)\n",         (int)s.starts, s.starts > 0 ? (int)(s.conflicts/s.starts) : 0);
    printf("c blocked restarts      : %d (multiple: %d) \n",                   (int)s.nbstopsrestarts, (int)s.nbstopsrestartssame);
    printf("c last block at restart : %d\n",                                   (int)s.lastblockatrestart);
    printf("c nb ReduceDB           : %-12d\n",                                (int)s.nbReduceDB);
    printf("c nb removed Clauses    : %-12d\n",                                (int)s.nbRemovedClauses);
    printf("c nb learnts DL2        : %-12d\n",                                (int)s.nbDL2);
    printf("c nb learnts size 2     : %-12d\n",                                (int)s.nbBin);
    printf("c nb learnts size 1     : %-12d\n",                                (int)s.nbUn);
    printf("c conflicts             : %-12d  (%.0f /sec)\n",                   (int)s.conflicts,    s.conflicts   /cpu_time);
    printf("c decisions             : %-12d  (%4.2f %% random) (%.0f /sec)\n", (int)s.decisions,    (float)s.rnd_decisions*100 / (float)s.decisions, s.decisions   /cpu_time);
    printf("c propagations          : %-12d  (%.0f /sec)\n",                   (int)s.propagations, s.propagations/cpu_time);
    printf("c conflict literals     : %-12d  (%4.2f %% deleted)\n",            (int)s.tot_literals, (s.max_literals - s.tot_literals)*100 / (double)s.max_literals);
    printf("c nb reduced Clauses    : %-12d\n", (int)s.nbReducedClauses);
    //printf("c CPU time              : %.2f sec\n", cpu_time);
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_IMPL_END
