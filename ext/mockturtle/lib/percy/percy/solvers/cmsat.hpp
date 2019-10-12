#pragma once

#include "solver_wrapper.hpp"

#ifdef USE_CMS

#pragma GCC diagnostic push

#pragma GCC diagnostic ignored "-Wall"

#include <cryptominisat5/cryptominisat.h>

#pragma GCC diagnostic pop

// A hack to undefine the CryptoMiniSat lbool definitions, 
// as they conflict with those defined by ABC.
#undef l_True
#undef l_False
#undef l_Undef

namespace percy
{
    class cmsat_wrapper : public solver_wrapper
    {
    private:
        CMSat::SATSolver * solver = NULL;

    public:
        cmsat_wrapper()
        {
            solver = new CMSat::SATSolver;
            auto nr_threads = std::thread::hardware_concurrency();
            solver->set_num_threads(nr_threads);
        }

        ~cmsat_wrapper()
        {
            delete solver;
            solver = NULL;
        }

        void restart()
        {
            delete solver;
            solver = new CMSat::SATSolver;
            auto nr_threads = std::thread::hardware_concurrency();
            solver->set_num_threads(nr_threads);
        }

        void set_nr_vars(int nr_vars)
        {
            solver->new_vars(nr_vars);
        }

        int add_clause(pabc::lit* begin, pabc::lit* end)
        {
            static std::vector<CMSat::Lit> clause;
            clause.clear();
            for (auto i = begin; i < end; i++) {
                clause.push_back(CMSat::Lit(pabc::Abc_Lit2Var(*i), pabc::Abc_LitIsCompl(*i)));
            }
            return solver->add_clause(clause);
        }

        int var_value(int var)
        {
            return solver->get_model()[var] == CMSat::boolToLBool(true);
        }

        synth_result solve(int cl) 
        {
            std::vector<CMSat::Lit> assumps;
            if (cl > 0) {
                solver->set_max_confl(cl);
            }
            auto res = solver->solve(&assumps);
            if (res == CMSat::boolToLBool(true)) {
                return success;
            } else if (res == CMSat::boolToLBool(false)) {
                return failure;
            } else {
                return timeout;
            }
        }

        synth_result solve(pabc::lit* begin, pabc::lit* end, int cl)
        {
            static std::vector<CMSat::Lit> assumps;
            assumps.clear();
            for (auto i = begin; i < end; i++) {
                assumps.push_back(CMSat::Lit(pabc::Abc_Lit2Var(*i), pabc::Abc_LitIsCompl(*i)));
            }
            if (cl > 0) {
                solver->set_max_confl(cl);
            }
            auto res = solver->solve(&assumps);
            if (res == CMSat::boolToLBool(true)) {
                return success;
            } else if (res == CMSat::boolToLBool(false)) {
                return failure;
            } else {
                return timeout;
            }
        }

        int nr_conflicts()
        {
            // TODO: check if we can now do this with CMSat.
            return 0u;
        }
        
        int nr_clauses()
        {
            // TODO: fix implementation.
            return 0;
        }

        int nr_vars()
        {
            return solver->nVars();
        }

        void add_var()
        {
            solver->new_var();
        }

    };
}

#endif
