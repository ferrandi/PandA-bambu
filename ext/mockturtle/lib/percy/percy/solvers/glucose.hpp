#pragma once

#if !defined(_WIN32) && !defined(_WIN64)

#undef var_Undef

#pragma GCC diagnostic push

#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wparentheses"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wpedantic"


#ifndef USE_GLUCOSE
#include <syrup/parallel/MultiSolvers.h>
#define GWType Glucose::MultiSolvers
#else
#include <glucose/core/Solver.h>
#define GWType Glucose::Solver
#endif

#pragma GCC diagnostic pop

#undef var_Undef

#define var_Undef (0xffffffffU >> 4)


namespace percy
{

    class glucose_wrapper : public solver_wrapper
    {
    private:
        GWType* solver;
        int nr_threads = 0;
        
    public:
        glucose_wrapper()
        {
            solver = new GWType;
        }

        ~glucose_wrapper()
        {
            delete solver;
            solver = NULL;
        }

        void restart()
        {
            delete solver;
#ifdef USE_SYRUP
            if (nr_threads > 0) {
                solver = new GWType(nr_threads);
            } else {
                solver = new GWType;
            }
#else
            solver = new GWType;
#endif
        }


        void set_nr_vars(int nr_vars)
        {
            while (nr_vars-- > 0) {
                solver->newVar();
            }
        }

        int nr_vars()
        {
            return solver->nVars();
        }

        int nr_clauses()
        {
            return solver->nClauses();
        }

        int nr_conflicts()
        {
#ifdef USE_GLUCOSE
            return solver->conflicts;
#else
            // Currently not supported by Glucose::MultiSolvers
            return 0;
#endif
        }

        int add_clause(pabc::lit* begin, pabc::lit* end)
        {
            Glucose::vec<Glucose::Lit> litvec;
            for (auto i = begin; i != end; i++) {
                litvec.push(Glucose::mkLit((*i >> 1), (*i & 1)));
            }
            return solver->addClause(litvec);
        }

        void add_var()
        {
            solver->newVar();
        }

        int var_value(int var)
        {
#ifdef USE_GLUCOSE
            return solver->modelValue(var) == l_True;
#else
            return solver->model[var] == l_True;
#endif
        }

        synth_result solve(int cl)
        {
#ifdef USE_GLUCOSE
            Glucose::vec<Glucose::Lit> litvec;
            if (cl) {
                solver->setConfBudget(cl);
            }
            auto res = solver->solveLimited(litvec);
            if (res == l_True) {
                return success;
            } else if (res == l_False) {
                return failure;
            } else {
                return timeout;
            }
#else
            int ret2 = solver->simplify();
            solver->use_simplification = false;
            if (ret2) {
                solver->eliminate();
            }

            if (!ret2 || !solver->okay()) {
                return failure;
            }

            // Conflict limits are currently not supported by 
            // Glucose::MultiSolvers.   
            auto res = solver->solve();
            if (res == l_True) {
                return success;
            } else if (res == l_False) {
                return failure;
            } else {
                return timeout;
            }
#endif
        }


#ifdef USE_GLUCOSE
        synth_result solve(pabc::lit* begin, pabc::lit* end, int cl)
        {
            Glucose::vec<Glucose::Lit> litvec;
            for (auto i = begin; i != end; i++) {
                litvec.push(Glucose::mkLit((*i >> 1), (*i & 1)));
            }
            if (cl) {
                solver->setConfBudget(cl);
            }
            auto res = solver->solveLimited(litvec);
            if (res == l_True) {
                return success;
            } else if (res == l_False) {
                return failure;
            } else {
                return timeout;
            }
        }
#else
        synth_result solve(pabc::lit*, pabc::lit*, int cl)
        {
            return solve(cl);
        }
#endif

#ifdef USE_SYRUP
        void set_nr_threads(int nr_threads)
        {
            delete solver;
            this->nr_threads = nr_threads;
            solver = new Glucose::MultiSolvers(nr_threads);
        }
#endif
    };
}

#endif
