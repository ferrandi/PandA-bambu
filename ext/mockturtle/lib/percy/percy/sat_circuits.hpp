#pragma once

#include <vector>
#include <percy/solvers.hpp>

namespace percy
{
    /// Given a solver, a list of variables, and a cardinality C, encodes a circuit in the solver which
    /// can be used to check if the binary sum of the variables is less than, equal to, or greater
    /// than C.
    /// This encoding is due to Andrey Mokhov.
    inline void create_cardinality_circuit(
        solver_wrapper * solver,
        const std::vector<int>& sum_vars,
        const std::vector<int>& res_vars,
        const int C)
    {
        int lits[3];

        // We have C + 2 result variables per addition step: (res[0], res[1], ..., res[C], res[C+1]).
        assert(res_vars.size() == ((C + 2) * (sum_vars.size() + 1)));

        // Initialize res[0] = 1, res[1] == res[2] == ... == res[C+1] == 0.
        for (int i = 0; i < C + 2; i++) {
            if (i == 0) {
                lits[0] = pabc::Abc_Var2Lit(res_vars[i], 0);
            } else {
                lits[0] = pabc::Abc_Var2Lit(res_vars[i], 1);
            }
            (void)solver->add_clause(lits, lits + 1);
        }

        for (auto i = 0u; i < sum_vars.size(); i++) {
            const auto x_k = sum_vars[i];
            for (int j = 0; j < C + 2; j++) {
                const auto res_j = res_vars[i * (C + 2) + j];
                const auto res_jp = res_vars[(i + 1) * (C + 2) + j];
                if (j == 0) {
                    // res'[0] = MUX(x[k], res[0], 0) = ~x[k] /\ res[0]
                    // Tseytin encoding:
                    // (x[k] \/ ~res[0] \/ res'[0]) /\ (~x[k] \/ ~res'[0]) /\ (res[0] \/ ~res'[0])
                    lits[0] = pabc::Abc_Var2Lit(x_k, 0);
                    lits[1] = pabc::Abc_Var2Lit(res_j, 1);
                    lits[2] = pabc::Abc_Var2Lit(res_jp, 0);
                    (void)solver->add_clause(lits, lits + 3);
                    lits[0] = pabc::Abc_Var2Lit(x_k, 1);
                    lits[1] = pabc::Abc_Var2Lit(res_jp, 1);
                    (void)solver->add_clause(lits, lits + 2);
                    lits[0] = pabc::Abc_Var2Lit(res_j, 0);
                    lits[1] = pabc::Abc_Var2Lit(res_jp, 1);
                    (void)solver->add_clause(lits, lits + 2);
                } else if (j == C + 1) {
                    // res'[C+2] = MUX(x[k], res[C+2], res[C+1] \/ res[C+1]) = 
                    // (~x[k] /\ res[C+2]) \/ (x[k] /\ res[C+1]) \/ (x[k] /\ res[C+2]) =
                    // (res[C+1] \/ res[C+2]) /\ (x[k] \/ res[C+2])
                    // Tseytin encoding:
                    // (~res'[C+2] \/ res[C+1] \/ res[C+2]) /\ (~res'[C+2] \/ x[k] \/ res[C+2]) /\ (res'[C+2] \/ ~x[k] \/ ~res[C+1]) /\ (res'[C+2] \/ ~res[C+2])
                    lits[0] = pabc::Abc_Var2Lit(res_jp, 1);
                    lits[1] = pabc::Abc_Var2Lit(res_j - 1, 0);
                    lits[2] = pabc::Abc_Var2Lit(res_j, 0);
                    (void)solver->add_clause(lits, lits + 3);
                    lits[0] = pabc::Abc_Var2Lit(res_jp, 1);
                    lits[1] = pabc::Abc_Var2Lit(x_k, 0);
                    lits[2] = pabc::Abc_Var2Lit(res_j, 0);
                    (void)solver->add_clause(lits, lits + 3);
                    lits[0] = pabc::Abc_Var2Lit(res_jp, 0);
                    lits[1] = pabc::Abc_Var2Lit(x_k, 1);
                    lits[2] = pabc::Abc_Var2Lit(res_j - 1, 1);
                    (void)solver->add_clause(lits, lits + 3);
                    lits[0] = pabc::Abc_Var2Lit(res_jp, 0);
                    lits[1] = pabc::Abc_Var2Lit(res_j, 1);
                    (void)solver->add_clause(lits, lits + 2);
                } else {
                    // res'[i] = MUX(x[k], res[i], res[i-1]).
                    // Tseytin encoding:
                    // ((~Z OR A OR S) AND (~Z OR B OR ~S)) AND (Z OR ~A OR S) AND (Z OR ~B OR ~S))) 
                    lits[0] = pabc::Abc_Var2Lit(res_jp, 1);
                    lits[1] = pabc::Abc_Var2Lit(res_j, 0);
                    lits[2] = pabc::Abc_Var2Lit(x_k, 0);
                    (void)solver->add_clause(lits, lits + 3);
                    lits[0] = pabc::Abc_Var2Lit(res_jp, 1);
                    lits[1] = pabc::Abc_Var2Lit(res_j - 1, 0);
                    lits[2] = pabc::Abc_Var2Lit(x_k, 1);
                    (void)solver->add_clause(lits, lits + 3);
                    lits[0] = pabc::Abc_Var2Lit(res_jp, 0);
                    lits[1] = pabc::Abc_Var2Lit(res_j, 1);
                    lits[2] = pabc::Abc_Var2Lit(x_k, 0);
                    (void)solver->add_clause(lits, lits + 3);
                    lits[0] = pabc::Abc_Var2Lit(res_jp, 0);
                    lits[1] = pabc::Abc_Var2Lit(res_j - 1, 1);
                    lits[2] = pabc::Abc_Var2Lit(x_k, 1);
                    (void)solver->add_clause(lits, lits + 3);
                }
            }
        }
    }
}
