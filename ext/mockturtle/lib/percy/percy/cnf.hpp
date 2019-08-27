#pragma once

#include <vector>
#include <abc/satSolver.h>
#include "solvers.hpp"
#include <cstdio>

namespace percy
{
    class cnf_formula : public solver_wrapper
    {
    private:
        std::vector<std::vector<int>> clauses;
        int _nr_vars;

    public:
        void restart() { _nr_vars = 0;  clauses.clear(); }
        void clear() { restart(); };
        int nr_vars() { return _nr_vars; }
        int nr_clauses() { return clauses.size(); }
        int nr_conflicts() { return 0; }
        void set_nr_vars(int nr_vars) { _nr_vars = nr_vars; }
        void add_var() { _nr_vars++; };
        
        int var_value(int) { return false; }
        synth_result solve(int) { return failure; }
        synth_result solve(pabc::lit*, pabc::lit*, int) { return failure; }

        int add_clause(pabc::lit* begin, pabc::lit* end) 
        {
            std::vector<int> clause;
            while (begin != end) {
                clause.push_back(*begin);
                begin++;
            }
            clauses.push_back(clause);

            return 1;
        }

        void to_dimacs(FILE* f) 
        {
            fprintf(f, "p cnf %d %d\n", nr_vars(), nr_clauses());
            for (const auto& clause : clauses) {
                for (const auto lit : clause) {
                    // NOTE: variable 0 does not exist in DIMACS format
                    const auto var = pabc::Abc_Lit2Var(lit) + 1;
                    const auto is_c = pabc::Abc_LitIsCompl(lit);
                    fprintf(f, "%d ", is_c ? -var : var);
                }
                fprintf(f, "0\n");
            }
        }

    };
}
