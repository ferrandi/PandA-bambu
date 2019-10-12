#pragma once

#ifndef DISABLE_SATOKO

#include "solver_wrapper.hpp"

#pragma GCC diagnostic push

#pragma GCC diagnostic ignored "-Wtype-limits"
#pragma GCC diagnostic ignored "-Wpedantic"
#include <satoko/satoko.h>
#include <satoko/solver.h>
#pragma GCC diagnostic pop

namespace percy
{
    class satoko_wrapper : public solver_wrapper
    {
    private:
        satoko::satoko_t * solver = NULL;

    public:
        satoko_wrapper()
        {
            solver = satoko::satoko_create();
        }

        ~satoko_wrapper()
        {
            satoko::satoko_destroy(solver);
            solver = NULL;
        }

        void restart()
        {
            satoko::satoko_reset(solver);
        }

        void set_nr_vars(int nr_vars)
        {
            satoko::satoko_setnvars(solver, nr_vars);
        }

        int nr_vars()
        {
            return satoko::satoko_varnum(solver);
        }

        int nr_clauses()
        {
            return satoko::satoko_clausenum(solver);
        }

        int nr_conflicts()
        {
            return satoko::satoko_conflictnum(solver);
        }

        int add_clause(pabc::lit* begin, pabc::lit* end)
        {
            return satoko::satoko_add_clause(solver, begin, end - begin);
        }

        void add_var()
        {
            satoko::satoko_add_variable(solver, 0);
        }

        int var_value(int var)
        {
            return satoko::satoko_read_cex_varvalue(solver, var);
        }

        synth_result solve(int cl)
        {
            auto res = satoko::satoko_solve_assumptions_limit(solver, 0, 0, cl);
            if (res == satoko::SATOKO_SAT) {
                return success;
            } else if (res == satoko::SATOKO_UNSAT) {
                return failure;
            } else {
                return timeout;
            }
        }

        synth_result solve(pabc::lit* begin, pabc::lit* end, int cl)
        {
            auto res = satoko::satoko_solve_assumptions_limit(solver, begin, end - begin, cl);
            if (res == satoko::SATOKO_SAT) {
                return success;
            } else if (res == satoko::SATOKO_UNSAT) {
                return failure;
            } else {
                return timeout;
            }
        }

        void set_no_simplify(char no_simplify)
        {
            solver->opts.no_simplify = no_simplify;
        }

        void set_f_rst(double f_rst)
        {
            solver->opts.f_rst = f_rst;
        }

        void set_b_rst(double b_rst)
        {
            solver->opts.b_rst = b_rst;
        }

        void set_fst_block_rst(unsigned fst_block_rst)
        {
            solver->opts.fst_block_rst = fst_block_rst;
        }

        void set_sz_lbd_bqueue(unsigned sz_lbd_bqueue)
        {
            solver->opts.sz_lbd_bqueue = sz_lbd_bqueue;
        }

        void set_sz_trail_bqueue(unsigned sz_trail_bqueue)
        {
            solver->opts.sz_trail_bqueue = sz_trail_bqueue;
        }

        void set_n_conf_fst_reduce(unsigned n_conf_fst_reduce)
        {
            solver->opts.n_conf_fst_reduce = n_conf_fst_reduce;
        }

        void set_inc_reduce(unsigned inc_reduce)
        {
            solver->opts.inc_reduce = inc_reduce;
        }

        void set_inc_special_reduce(unsigned inc_special_reduce)
        {
            solver->opts.inc_special_reduce = inc_special_reduce;
        }

        void set_lbd_freeze_clause(unsigned lbd_freeze_clause)
        {
            solver->opts.lbd_freeze_clause = lbd_freeze_clause;
        }

        void set_learnt_ratio(float learnt_ratio)
        {
            solver->opts.learnt_ratio = learnt_ratio;
        }

        void set_var_decay(double var_decay)
        {
            solver->opts.var_decay = var_decay;
        }

        void set_clause_decay(float clause_decay)
        {
            solver->opts.clause_decay = clause_decay;
        }

        void set_var_act_rescale(unsigned var_act_rescale)
        {
            solver->opts.var_act_rescale = var_act_rescale;
        }

        void set_var_act_limit(satoko::act_t var_act_limit)
        {
            solver->opts.var_act_limit = var_act_limit;
        }

        void set_clause_max_sz_bin_resol(unsigned clause_max_sz_bin_resol)
        {
            solver->opts.clause_max_sz_bin_resol = clause_max_sz_bin_resol;
        }

        void set_clause_min_lbd_bin_resol(unsigned clause_min_lbd_bin_resol)
        {
            solver->opts.clause_min_lbd_bin_resol = clause_min_lbd_bin_resol;
        }

        void set_garbage_max_ratio(float garbage_max_ratio)
        {
            solver->opts.garbage_max_ratio = garbage_max_ratio;
        }
    };
}

#endif // !DISABLE_SATOKO
