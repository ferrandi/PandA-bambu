#pragma once

#include <vector>
#include <kitty/kitty.hpp>
#include "encoder.hpp"
#include "../partial_dag.hpp"

namespace percy
{
    class mig_encoder
    {
    private:
        int level_dist[32]; // How many steps are below a certain level
        int nr_levels; // The number of levels in the Boolean fence
        int nr_sel_vars;
        int nr_op_vars;
        int nr_sim_vars;
        int total_nr_vars;
        int sel_offset;
        int ops_offset;
        int sim_offset;
        bool dirty = false;
        pabc::lit pLits[2048];
        //pabc::Vec_Int_t* vLits = NULL;
        solver_wrapper* solver;

        int svars[16][16][16][16];

        // There are 4 possible operators for each MIG node:
        // <abc>        (0)
        // <!abc>       (1)
        // <a!bc>       (2)
        // <ab!c>       (3)
        // All other input patterns can be obained from these
        // by output inversion. Therefore we consider
        // them symmetries and do not encode them.
        const int MIG_OP_VARS_PER_STEP = 4;

        const int NR_SIM_TTS = 32;
        std::vector<kitty::dynamic_truth_table> sim_tts {32};

        int get_sim_var(const spec& spec, int step_idx, int t) const
        {
            return sim_offset + spec.tt_size * step_idx + t;
        }

        int get_op_var(const spec& spec, int step_idx, int var_idx) const
        {
            return ops_offset + step_idx * MIG_OP_VARS_PER_STEP + var_idx;
        }

        bool fix_output_sim_vars(const spec& spec, int t)
        {
            const auto ilast_step = spec.nr_steps - 1;
            auto outbit = kitty::get_bit(
                spec[spec.synth_func(0)], t + 1);
            if ((spec.out_inv >> spec.synth_func(0)) & 1) {
                outbit = 1 - outbit;
            }
            const auto sim_var = get_sim_var(spec, ilast_step, t);
            pabc::lit sim_lit = pabc::Abc_Var2Lit(sim_var, 1 - outbit);
            return solver->add_clause(&sim_lit, &sim_lit + 1);
        }

        void vfix_output_sim_vars(const spec& spec, int t)
        {
            const auto ilast_step = spec.nr_steps - 1;

            auto outbit = kitty::get_bit(
                spec[spec.synth_func(0)], t + 1);
            if ((spec.out_inv >> spec.synth_func(0)) & 1) {
                outbit = 1 - outbit;
            }
            const auto sim_var = get_sim_var(spec, ilast_step, t);
            pabc::lit sim_lit = pabc::Abc_Var2Lit(sim_var, 1 - outbit);
            const auto ret = solver->add_clause(&sim_lit, &sim_lit + 1);
            assert(ret);
            if (spec.verbosity) {
                printf("forcing bit %d=%d\n", t + 1, outbit);
            }
        }

        int get_sel_var(const spec& spec, int idx, int var_idx) const
        {
            assert(idx < spec.nr_steps);
            const auto nr_svars_for_idx = nr_svars_for_step(spec, idx);
            assert(var_idx < nr_svars_for_idx);
            auto offset = 0;
            for (int i = 0; i < idx; i++) {
                offset += nr_svars_for_step(spec, i);
            }
            return sel_offset + offset + var_idx;
        }

    public:
        mig_encoder(solver_wrapper& solver)
        {
            this->solver = &solver;
        }

        ~mig_encoder()
        {
        }

        void create_variables(const spec& spec)
        {
            nr_op_vars = spec.nr_steps * MIG_OP_VARS_PER_STEP;
            nr_sim_vars = spec.nr_steps * spec.tt_size;

            nr_sel_vars = 0;
            for (int i = 0; i < spec.nr_steps; i++) {
                for (int l = 2; l <= spec.nr_in + i; l++) {
                    for (int k = 1; k < l; k++) {
                        for (int j = 0; j < k; j++) {
                            svars[i][j][k][l] = nr_sel_vars++;
                        }
                    }
                }
            }

            sel_offset = 0;
            ops_offset = nr_sel_vars;
            sim_offset = nr_sel_vars + nr_op_vars;
            total_nr_vars = nr_sel_vars + nr_op_vars + nr_sim_vars;

            if (spec.verbosity) {
                printf("Creating variables (MIG)\n");
                printf("nr steps = %d\n", spec.nr_steps);
                printf("nr_sel_vars=%d\n", nr_sel_vars);
                printf("nr_op_vars = %d\n", nr_op_vars);
                printf("nr_sim_vars = %d\n", nr_sim_vars);
                printf("creating %d total variables\n", total_nr_vars);
            }

            solver->set_nr_vars(total_nr_vars);
        }

        int first_step_on_level(int level) const
        {
            if (level == 0) { return 0; }
            return level_dist[level-1];
        }

        int nr_svars_for_step(const spec& spec, int i) const
        {
            // Determine the level of this step.
            const auto level = get_level(spec, i + spec.nr_in + 1);
            auto nr_svars_for_i = 0;
            assert(level > 0);
            for (auto l = first_step_on_level(level - 1);
                l < first_step_on_level(level); l++) {
                // We select l as fanin 3, so have (l choose 2) options 
                // (j,k in {0,...,(l-1)}) left for fanin 1 and 2.
                nr_svars_for_i += (l * (l - 1)) / 2;
            }

            return nr_svars_for_i;
        }

        void fence_create_variables(const spec& spec)
        {
            nr_op_vars = spec.nr_steps * MIG_OP_VARS_PER_STEP;
            nr_sim_vars = spec.nr_steps * spec.tt_size;

            nr_sel_vars = 0;
            for (int i = 0; i < spec.nr_steps; i++) {
                nr_sel_vars += nr_svars_for_step(spec, i);
            }

            sel_offset = 0;
            ops_offset = nr_sel_vars;
            sim_offset = nr_sel_vars + nr_op_vars;
            total_nr_vars = nr_sel_vars + nr_op_vars + nr_sim_vars;

            if (spec.verbosity) {
                printf("Creating variables (MIG)\n");
                printf("nr steps = %d\n", spec.nr_steps);
                printf("nr_sel_vars=%d\n", nr_sel_vars);
                printf("nr_op_vars = %d\n", nr_op_vars);
                printf("nr_sim_vars = %d\n", nr_sim_vars);
                printf("creating %d total variables\n", total_nr_vars);
            }

            solver->set_nr_vars(total_nr_vars);
        }

        

        /// Ensures that each gate has the proper number of fanins.
        bool create_fanin_clauses(const spec& spec)
        {
            auto status = true;

            if (spec.verbosity > 2) {
                printf("Creating fanin clauses (MIG)\n");
                printf("Nr. clauses = %d (PRE)\n", solver->nr_clauses());
            }

            for (int i = 0; i < spec.nr_steps; i++) {
                auto ctr = 0;
                for (int l = 2; l <= spec.nr_in + i; l++) {
                    for (int k = 1; k < l; k++) {
                        for (int j = 0; j < k; j++) {
                            pLits[ctr++] = pabc::Abc_Var2Lit(svars[i][j][k][l], 0);
                        }
                    }
                }
                status &= solver->add_clause(pLits, pLits + ctr);
            }

            // We need to select one of the possible operators for this step.
            /*
            for (int i = 0; i < spec.nr_steps; i++) {
                pLits[0] = pabc::Abc_Var2Lit(get_op_var(spec, i, 0), 0);
                pLits[1] = pabc::Abc_Var2Lit(get_op_var(spec, i, 1), 0);
                pLits[2] = pabc::Abc_Var2Lit(get_op_var(spec, i, 2), 0);
                pLits[3] = pabc::Abc_Var2Lit(get_op_var(spec, i, 3), 0);
                status &= solver->add_clause(pLits, pLits + 4);
            }
            */
            
            if (spec.verbosity > 2) {
                printf("Nr. clauses = %d (POST)\n", solver->nr_clauses());
            }

            return status;
        }

        

        int maj3(int a, int ca, int b, int cb, int c, int cc) const
        {
            a = ca ? ~a : a;
            a = a & 1;
            b = cb ? ~b : b;
            b = b & 1;
            c = cc ? ~c : c;
            c = c & 1;
            return (a & b) | (a & c) | (b & c);
        }

        bool add_simulation_clause(
            const spec& spec,
            const int t,
            const int i,
            const int j,
            const int k,
            const int l,
            const int a,
            const int b,
            const int c,
            const int d
            )
        {
            int ctr = 0;

            if (j == 0) {
                // Constant zero input
                if (k <= spec.nr_in) {
                    if ((((t + 1) & (1 << (k - 1))) ? 1 : 0) != c) {
                        return true;
                    }
                } else {
                    pLits[ctr++] = pabc::Abc_Var2Lit(
                        get_sim_var(spec, k - spec.nr_in - 1, t), c);
                }

                if (l <= spec.nr_in) {
                    if ((((t + 1) & (1 << (l - 1))) ? 1 : 0) != d) {
                        return true;
                    }
                } else {
                    pLits[ctr++] = pabc::Abc_Var2Lit(
                        get_sim_var(spec, l - spec.nr_in - 1, t), d);
                }

                pLits[ctr++] = pabc::Abc_Var2Lit(svars[i][j][k][l], 1);
                pLits[ctr++] = pabc::Abc_Var2Lit(get_sim_var(spec, i, t), a);

                if (c | d) {
                    if (maj3(0, 0, c, 0, d, 0) == a) {
                        pLits[ctr++] =
                            pabc::Abc_Var2Lit(get_op_var(spec, i, 0), 0);
                    }
                    if (maj3(0, 1, c, 0, d, 0) == a) {
                        pLits[ctr++] =
                            pabc::Abc_Var2Lit(get_op_var(spec, i, 1), 0);
                    }
                    if (maj3(0, 0, c, 1, d, 0) == a) {
                        pLits[ctr++] =
                            pabc::Abc_Var2Lit(get_op_var(spec, i, 2), 0);
                    }
                    if (maj3(0, 0, c, 0, d, 1) == a) {
                        pLits[ctr++] =
                            pabc::Abc_Var2Lit(get_op_var(spec, i, 3), 0);
                    }
                }

                solver->add_clause(pLits, pLits + ctr);
                const auto ret = solver->add_clause(pLits, pLits + ctr);
                assert(ret);
                return ret;
            } 
            
            if (j <= spec.nr_in) {
                if ((((t + 1) & (1 << (j - 1))) ? 1 : 0) != b) {
                    return true;
                }
            } else {
                pLits[ctr++] = pabc::Abc_Var2Lit(
                    get_sim_var(spec, j - spec.nr_in - 1, t), b);
            }

            if (k <= spec.nr_in) {
                if ((((t + 1) & (1 << (k - 1))) ? 1 : 0) != c) {
                    return true;
                }
            } else {
                pLits[ctr++] = pabc::Abc_Var2Lit(
                    get_sim_var(spec, k - spec.nr_in - 1, t), c);
            }

            if (l <= spec.nr_in) {
                if ((((t + 1) & (1 << (l - 1))) ? 1 : 0) != d) {
                    return true;
                }
            } else {
                pLits[ctr++] = pabc::Abc_Var2Lit(
                    get_sim_var(spec, l - spec.nr_in - 1, t), d);
            }

            pLits[ctr++] = pabc::Abc_Var2Lit(svars[i][j][k][l], 1);
            pLits[ctr++] = pabc::Abc_Var2Lit(get_sim_var(spec, i, t), a);

            if (b | c | d) {
                if (maj3(b, 0, c, 0, d, 0) == a) {
                    pLits[ctr++] = 
                        pabc::Abc_Var2Lit(get_op_var(spec, i, 0), 0);
                }
                if (maj3(b, 1, c, 0, d, 0) == a) {
                    pLits[ctr++] = 
                        pabc::Abc_Var2Lit(get_op_var(spec, i, 1), 0);
                }
                if (maj3(b, 0, c, 1, d, 0) == a) {
                    pLits[ctr++] = 
                        pabc::Abc_Var2Lit(get_op_var(spec, i, 2), 0);
                }
                if (maj3(b, 0, c, 0, d, 1) == a) {
                    pLits[ctr++] = 
                        pabc::Abc_Var2Lit(get_op_var(spec, i, 3), 0);
                }
            }

            const auto ret = solver->add_clause(pLits, pLits + ctr);
            assert(ret);
            return ret;
        }

        bool add_consistency_clause(
            const spec& spec,
            const int t,
            const int i,
            const int j,
            const int k,
            const int l,
            const int d,
            const int c,
            const int b,
            const int true_opvar1,
            const int true_opvar2,
            const int false_opvar1,
            const int false_opvar2
            )
        {
            int ctr = 0;
            assert(j >= 1);

             
            if (j <= spec.nr_in) {
                if ((((t + 1) & (1 << (j - 1))) ? 1 : 0) != b) {
                    return true;
                }
            } else {
                pLits[ctr++] = pabc::Abc_Var2Lit(
                    get_sim_var(spec, j - spec.nr_in - 1, t), b);
            }

            if (k <= spec.nr_in) {
                if ((((t + 1) & (1 << (k - 1))) ? 1 : 0) != c) {
                    return true;
                }
            } else {
                pLits[ctr++] = pabc::Abc_Var2Lit(
                    get_sim_var(spec, k - spec.nr_in - 1, t), c);
            }

            if (l <= spec.nr_in) {
                if ((((t + 1) & (1 << (l - 1))) ? 1 : 0) != d) {
                    return true;
                }
            } else {
                pLits[ctr++] = pabc::Abc_Var2Lit(
                    get_sim_var(spec, l - spec.nr_in - 1, t), d);
            }

            pLits[ctr++] = pabc::Abc_Var2Lit(svars[i][j][k][l], 1);
            pLits[ctr++] = pabc::Abc_Var2Lit(get_sim_var(spec, i, t), 1);
            pLits[ctr++] = pabc::Abc_Var2Lit(get_op_var(spec, i, true_opvar1), 0);
            pLits[ctr++] = pabc::Abc_Var2Lit(get_op_var(spec, i, true_opvar2), 0);
            auto ret = solver->add_clause(pLits, pLits + ctr);

            pLits[ctr - 2] = pabc::Abc_Var2Lit(get_op_var(spec, i, false_opvar1), 1);
            ret &= solver->add_clause(pLits, pLits + ctr - 1);
            pLits[ctr - 2] = pabc::Abc_Var2Lit(get_op_var(spec, i, false_opvar2), 1);
            ret &= solver->add_clause(pLits, pLits + ctr - 1);
            
            pLits[ctr - 3] = pabc::Abc_Var2Lit(get_sim_var(spec, i, t), 0);
            pLits[ctr - 2] = pabc::Abc_Var2Lit(get_op_var(spec, i, false_opvar1), 0);
            pLits[ctr - 1] = pabc::Abc_Var2Lit(get_op_var(spec, i, false_opvar2), 0);
            ret &= solver->add_clause(pLits, pLits + ctr);
            
            pLits[ctr - 2] = pabc::Abc_Var2Lit(get_op_var(spec, i, true_opvar1), 1);
            ret &= solver->add_clause(pLits, pLits + ctr - 1);
            pLits[ctr - 2] = pabc::Abc_Var2Lit(get_op_var(spec, i, true_opvar2), 1);
            ret &= solver->add_clause(pLits, pLits + ctr - 1);

            assert(ret);


            
            return ret;
        }

        bool fence_add_consistency_clause(
            const spec& spec,
            const int t,
            const int i,
            const int j,
            const int k,
            const int l,
            const int d,
            const int c,
            const int b,
            const int true_opvar1,
            const int true_opvar2,
            const int false_opvar1,
            const int false_opvar2,
            const int sel_var)
        {
            int ctr = 0;
            assert(j >= 1);

             
            if (j <= spec.nr_in) {
                if ((((t + 1) & (1 << (j - 1))) ? 1 : 0) != b) {
                    return true;
                }
            } else {
                pLits[ctr++] = pabc::Abc_Var2Lit(
                    get_sim_var(spec, j - spec.nr_in - 1, t), b);
            }

            if (k <= spec.nr_in) {
                if ((((t + 1) & (1 << (k - 1))) ? 1 : 0) != c) {
                    return true;
                }
            } else {
                pLits[ctr++] = pabc::Abc_Var2Lit(
                    get_sim_var(spec, k - spec.nr_in - 1, t), c);
            }

            if (l <= spec.nr_in) {
                if ((((t + 1) & (1 << (l - 1))) ? 1 : 0) != d) {
                    return true;
                }
            } else {
                pLits[ctr++] = pabc::Abc_Var2Lit(
                    get_sim_var(spec, l - spec.nr_in - 1, t), d);
            }

            pLits[ctr++] = pabc::Abc_Var2Lit(sel_var, 1);
            pLits[ctr++] = pabc::Abc_Var2Lit(get_sim_var(spec, i, t), 1);
            pLits[ctr++] = pabc::Abc_Var2Lit(get_op_var(spec, i, true_opvar1), 0);
            pLits[ctr++] = pabc::Abc_Var2Lit(get_op_var(spec, i, true_opvar2), 0);
            auto ret = solver->add_clause(pLits, pLits + ctr);

            pLits[ctr - 2] = pabc::Abc_Var2Lit(get_op_var(spec, i, false_opvar1), 1);
            ret &= solver->add_clause(pLits, pLits + ctr - 1);
            pLits[ctr - 2] = pabc::Abc_Var2Lit(get_op_var(spec, i, false_opvar2), 1);
            ret &= solver->add_clause(pLits, pLits + ctr - 1);
            
            pLits[ctr - 3] = pabc::Abc_Var2Lit(get_sim_var(spec, i, t), 0);
            pLits[ctr - 2] = pabc::Abc_Var2Lit(get_op_var(spec, i, false_opvar1), 0);
            pLits[ctr - 1] = pabc::Abc_Var2Lit(get_op_var(spec, i, false_opvar2), 0);
            ret &= solver->add_clause(pLits, pLits + ctr);
            
            pLits[ctr - 2] = pabc::Abc_Var2Lit(get_op_var(spec, i, true_opvar1), 1);
            ret &= solver->add_clause(pLits, pLits + ctr - 1);
            pLits[ctr - 2] = pabc::Abc_Var2Lit(get_op_var(spec, i, true_opvar2), 1);
            ret &= solver->add_clause(pLits, pLits + ctr - 1);

            assert(ret);


            
            return ret;
        }


        bool add_impossibility_clause(
            const spec& spec,
            const int t,
            const int i,
            const int j,
            const int k,
            const int l,
            const int d,
            const int c,
            const int b,
            const int a)
        {
            int ctr = 0;
            assert(j >= 1);

             
            if (j <= spec.nr_in) {
                if ((((t + 1) & (1 << (j - 1))) ? 1 : 0) != b) {
                    return true;
                }
            } else {
                pLits[ctr++] = pabc::Abc_Var2Lit(
                    get_sim_var(spec, j - spec.nr_in - 1, t), b);
            }

            if (k <= spec.nr_in) {
                if ((((t + 1) & (1 << (k - 1))) ? 1 : 0) != c) {
                    return true;
                }
            } else {
                pLits[ctr++] = pabc::Abc_Var2Lit(
                    get_sim_var(spec, k - spec.nr_in - 1, t), c);
            }

            if (l <= spec.nr_in) {
                if ((((t + 1) & (1 << (l - 1))) ? 1 : 0) != d) {
                    return true;
                }
            } else {
                pLits[ctr++] = pabc::Abc_Var2Lit(
                    get_sim_var(spec, l - spec.nr_in - 1, t), d);
            }

            pLits[ctr++] = pabc::Abc_Var2Lit(svars[i][j][k][l], 1);
            pLits[ctr++] = pabc::Abc_Var2Lit(get_sim_var(spec, i, t), a);
            auto ret = solver->add_clause(pLits, pLits + ctr);

            assert(ret);
            
            return ret;
        }

        bool fence_add_impossibility_clause(
            const spec& spec,
            const int t,
            const int i,
            const int j,
            const int k,
            const int l,
            const int d,
            const int c,
            const int b,
            const int a,
            const int sel_var)
        {
            int ctr = 0;
            assert(j >= 1);

             
            if (j <= spec.nr_in) {
                if ((((t + 1) & (1 << (j - 1))) ? 1 : 0) != b) {
                    return true;
                }
            } else {
                pLits[ctr++] = pabc::Abc_Var2Lit(
                    get_sim_var(spec, j - spec.nr_in - 1, t), b);
            }

            if (k <= spec.nr_in) {
                if ((((t + 1) & (1 << (k - 1))) ? 1 : 0) != c) {
                    return true;
                }
            } else {
                pLits[ctr++] = pabc::Abc_Var2Lit(
                    get_sim_var(spec, k - spec.nr_in - 1, t), c);
            }

            if (l <= spec.nr_in) {
                if ((((t + 1) & (1 << (l - 1))) ? 1 : 0) != d) {
                    return true;
                }
            } else {
                pLits[ctr++] = pabc::Abc_Var2Lit(
                    get_sim_var(spec, l - spec.nr_in - 1, t), d);
            }

            pLits[ctr++] = pabc::Abc_Var2Lit(sel_var, 1);
            pLits[ctr++] = pabc::Abc_Var2Lit(get_sim_var(spec, i, t), a);
            auto ret = solver->add_clause(pLits, pLits + ctr);

            assert(ret);
            
            return ret;
        }

        bool add_const_consistency_clause(
            const spec& spec,
            const int t,
            const int i,
            const int k,
            const int l,
            const int d,
            const int c,
            const int true_opvar1,
            const int true_opvar2,
            const int false_opvar1,
            const int false_opvar2
            )
        {
            int ctr = 0;
             
            if (k <= spec.nr_in) {
                if ((((t + 1) & (1 << (k - 1))) ? 1 : 0) != c) {
                    return true;
                }
            } else {
                pLits[ctr++] = pabc::Abc_Var2Lit(
                    get_sim_var(spec, k - spec.nr_in - 1, t), c);
            }

            if (l <= spec.nr_in) {
                if ((((t + 1) & (1 << (l - 1))) ? 1 : 0) != d) {
                    return true;
                }
            } else {
                pLits[ctr++] = pabc::Abc_Var2Lit(
                    get_sim_var(spec, l - spec.nr_in - 1, t), d);
            }

            pLits[ctr++] = pabc::Abc_Var2Lit(svars[i][0][k][l], 1);
            pLits[ctr++] = pabc::Abc_Var2Lit(get_sim_var(spec, i, t), 1);
            pLits[ctr++] = pabc::Abc_Var2Lit(get_op_var(spec, i, true_opvar1), 0);
            pLits[ctr++] = pabc::Abc_Var2Lit(get_op_var(spec, i, true_opvar2), 0);
            auto ret = solver->add_clause(pLits, pLits + ctr);

            pLits[ctr - 2] = pabc::Abc_Var2Lit(get_op_var(spec, i, false_opvar1), 1);
            ret &= solver->add_clause(pLits, pLits + ctr - 1);
            pLits[ctr - 2] = pabc::Abc_Var2Lit(get_op_var(spec, i, false_opvar2), 1);
            ret &= solver->add_clause(pLits, pLits + ctr - 1);
            
            pLits[ctr - 3] = pabc::Abc_Var2Lit(get_sim_var(spec, i, t), 0);
            pLits[ctr - 2] = pabc::Abc_Var2Lit(get_op_var(spec, i, false_opvar1), 0);
            pLits[ctr - 1] = pabc::Abc_Var2Lit(get_op_var(spec, i, false_opvar2), 0);
            ret &= solver->add_clause(pLits, pLits + ctr);
            
            pLits[ctr - 2] = pabc::Abc_Var2Lit(get_op_var(spec, i, true_opvar1), 1);
            ret &= solver->add_clause(pLits, pLits + ctr - 1);
            pLits[ctr - 2] = pabc::Abc_Var2Lit(get_op_var(spec, i, true_opvar2), 1);
            ret &= solver->add_clause(pLits, pLits + ctr - 1);

            assert(ret);

            return ret;
        }

        bool fence_add_const_consistency_clause(
            const spec& spec,
            const int t,
            const int i,
            const int k,
            const int l,
            const int d,
            const int c,
            const int true_opvar1,
            const int true_opvar2,
            const int false_opvar1,
            const int false_opvar2,
            int sel_var)
        {
            int ctr = 0;
             
            if (k <= spec.nr_in) {
                if ((((t + 1) & (1 << (k - 1))) ? 1 : 0) != c) {
                    return true;
                }
            } else {
                pLits[ctr++] = pabc::Abc_Var2Lit(
                    get_sim_var(spec, k - spec.nr_in - 1, t), c);
            }

            if (l <= spec.nr_in) {
                if ((((t + 1) & (1 << (l - 1))) ? 1 : 0) != d) {
                    return true;
                }
            } else {
                pLits[ctr++] = pabc::Abc_Var2Lit(
                    get_sim_var(spec, l - spec.nr_in - 1, t), d);
            }

            pLits[ctr++] = pabc::Abc_Var2Lit(sel_var, 1);
            pLits[ctr++] = pabc::Abc_Var2Lit(get_sim_var(spec, i, t), 1);
            pLits[ctr++] = pabc::Abc_Var2Lit(get_op_var(spec, i, true_opvar1), 0);
            pLits[ctr++] = pabc::Abc_Var2Lit(get_op_var(spec, i, true_opvar2), 0);
            auto ret = solver->add_clause(pLits, pLits + ctr);

            pLits[ctr - 2] = pabc::Abc_Var2Lit(get_op_var(spec, i, false_opvar1), 1);
            ret &= solver->add_clause(pLits, pLits + ctr - 1);
            pLits[ctr - 2] = pabc::Abc_Var2Lit(get_op_var(spec, i, false_opvar2), 1);
            ret &= solver->add_clause(pLits, pLits + ctr - 1);
            
            pLits[ctr - 3] = pabc::Abc_Var2Lit(get_sim_var(spec, i, t), 0);
            pLits[ctr - 2] = pabc::Abc_Var2Lit(get_op_var(spec, i, false_opvar1), 0);
            pLits[ctr - 1] = pabc::Abc_Var2Lit(get_op_var(spec, i, false_opvar2), 0);
            ret &= solver->add_clause(pLits, pLits + ctr);
            
            pLits[ctr - 2] = pabc::Abc_Var2Lit(get_op_var(spec, i, true_opvar1), 1);
            ret &= solver->add_clause(pLits, pLits + ctr - 1);
            pLits[ctr - 2] = pabc::Abc_Var2Lit(get_op_var(spec, i, true_opvar2), 1);
            ret &= solver->add_clause(pLits, pLits + ctr - 1);

            assert(ret);

            return ret;
        }

        bool add_const_impossibility_clause(
            const spec& spec,
            const int t,
            const int i,
            const int k,
            const int l,
            const int d,
            const int c)
        {
            int ctr = 0;
             
            if (k <= spec.nr_in) {
                if ((((t + 1) & (1 << (k - 1))) ? 1 : 0) != c) {
                    return true;
                }
            } else {
                pLits[ctr++] = pabc::Abc_Var2Lit(
                    get_sim_var(spec, k - spec.nr_in - 1, t), c);
            }

            if (l <= spec.nr_in) {
                if ((((t + 1) & (1 << (l - 1))) ? 1 : 0) != d) {
                    return true;
                }
            } else {
                pLits[ctr++] = pabc::Abc_Var2Lit(
                    get_sim_var(spec, l - spec.nr_in - 1, t), d);
            }

            pLits[ctr++] = pabc::Abc_Var2Lit(svars[i][0][k][l], 1);
            pLits[ctr++] = pabc::Abc_Var2Lit(get_sim_var(spec, i, t), 1);
            auto ret = solver->add_clause(pLits, pLits + ctr);

            assert(ret);

            return ret;
        }

        bool fence_add_const_impossibility_clause(
            const spec& spec,
            const int t,
            const int i,
            const int k,
            const int l,
            const int d,
            const int c,
            const int sel_var)
        {
            int ctr = 0;
             
            if (k <= spec.nr_in) {
                if ((((t + 1) & (1 << (k - 1))) ? 1 : 0) != c) {
                    return true;
                }
            } else {
                pLits[ctr++] = pabc::Abc_Var2Lit(
                    get_sim_var(spec, k - spec.nr_in - 1, t), c);
            }

            if (l <= spec.nr_in) {
                if ((((t + 1) & (1 << (l - 1))) ? 1 : 0) != d) {
                    return true;
                }
            } else {
                pLits[ctr++] = pabc::Abc_Var2Lit(
                    get_sim_var(spec, l - spec.nr_in - 1, t), d);
            }

            pLits[ctr++] = pabc::Abc_Var2Lit(sel_var, 1);
            pLits[ctr++] = pabc::Abc_Var2Lit(get_sim_var(spec, i, t), 1);
            auto ret = solver->add_clause(pLits, pLits + ctr);

            assert(ret);

            return ret;
        }

        bool create_tt_clauses(const spec& spec, const int t)
        {
            bool ret = true;
            for (int i = 0; i < spec.nr_steps; i++) {
                for (int l = 2; l <= spec.nr_in + i; l++) {
                    for (int k = 1; k < l; k++) {
                        for (int j = 1; j < k; j++) {
                            ret &= add_consistency_clause(spec, t, i, j, k, l, 0, 0, 1, 2, 3, 0, 1);
                            ret &= add_consistency_clause(spec, t, i, j, k, l, 0, 1, 0, 1, 3, 0, 2);
                            ret &= add_consistency_clause(spec, t, i, j, k, l, 0, 1, 1, 0, 3, 1, 2);
                            ret &= add_consistency_clause(spec, t, i, j, k, l, 1, 0, 0, 1, 2, 0, 3);
                            ret &= add_consistency_clause(spec, t, i, j, k, l, 1, 0, 1, 0, 2, 1, 3);
                            ret &= add_consistency_clause(spec, t, i, j, k, l, 1, 1, 0, 0, 1, 2, 3);
                            ret &= add_impossibility_clause(spec, t, i, j, k, l, 0, 0, 0, 1);
                            ret &= add_impossibility_clause(spec, t, i, j, k, l, 1, 1, 1, 0);
                        }
                        ret &= add_const_impossibility_clause(spec, t, i, k, l, 0, 0);
                        ret &= add_const_consistency_clause(spec, t, i, k, l, 0, 1, 1, 3, 0, 2);
                        ret &= add_const_consistency_clause(spec, t, i, k, l, 1, 0, 1, 2, 0, 3);
                        ret &= add_const_consistency_clause(spec, t, i, k, l, 1, 1, 0, 1, 2, 3);
                    }
                }
                assert(ret);
            }

            ret &= fix_output_sim_vars(spec, t);

            return ret;
        }

        bool fence_create_tt_clauses(const spec& spec, const int t)
        {
            bool ret = true;
            for (int i = 0; i < spec.nr_steps; i++) {
                const auto level = get_level(spec, i + spec.nr_in + 1);
                int ctr = 0;
                for (int l = first_step_on_level(level - 1);
                    l < first_step_on_level(level); l++) {
                    for (int k = 1; k < l; k++) {
                        for (int j = 0; j < k; j++) {
                            const auto sel_var = get_sel_var(spec, i, ctr++);
                            if (j == 0) {
                                ret &= fence_add_const_impossibility_clause(spec, t, i, k, l, 0, 0, sel_var);
                                ret &= fence_add_const_consistency_clause(spec, t, i, k, l, 0, 1, 1, 3, 0, 2, sel_var);
                                ret &= fence_add_const_consistency_clause(spec, t, i, k, l, 1, 0, 1, 2, 0, 3, sel_var);
                                ret &= fence_add_const_consistency_clause(spec, t, i, k, l, 1, 1, 0, 1, 2, 3, sel_var);
                            } else {
                                ret &= fence_add_consistency_clause(spec, t, i, j, k, l, 0, 0, 1, 2, 3, 0, 1, sel_var);
                                ret &= fence_add_consistency_clause(spec, t, i, j, k, l, 0, 1, 0, 1, 3, 0, 2, sel_var);
                                ret &= fence_add_consistency_clause(spec, t, i, j, k, l, 0, 1, 1, 0, 3, 1, 2, sel_var);
                                ret &= fence_add_consistency_clause(spec, t, i, j, k, l, 1, 0, 0, 1, 2, 0, 3, sel_var);
                                ret &= fence_add_consistency_clause(spec, t, i, j, k, l, 1, 0, 1, 0, 2, 1, 3, sel_var);
                                ret &= fence_add_consistency_clause(spec, t, i, j, k, l, 1, 1, 0, 0, 1, 2, 3, sel_var);
                                ret &= fence_add_impossibility_clause(spec, t, i, j, k, l, 0, 0, 0, 1, sel_var);
                                ret &= fence_add_impossibility_clause(spec, t, i, j, k, l, 1, 1, 1, 0, sel_var);
                            }
                        }
                    }
                }
                assert(ret);
            }

            ret &= fix_output_sim_vars(spec, t);

            return ret;
        }

        void create_main_clauses(const spec& spec)
        {
            for (int t = 0; t < spec.tt_size; t++) {
                (void)create_tt_clauses(spec, t);
            }
        }

        bool fence_create_main_clauses(const spec& spec)
        {
            bool ret = true;
            for (int t = 0; t < spec.tt_size; t++) {
                ret &= fence_create_tt_clauses(spec, t);
            }
            return ret;
        }

        void create_alonce_clauses(const spec& spec)
        {
            for (int i = 0; i < spec.nr_steps - 1; i++) {
                int ctr = 0;
                for (int ip = i + 1; ip < spec.nr_steps; ip++) {
                    for (int l = spec.nr_in + i; l <= spec.nr_in + ip; l++) {
                        for (int k = 1; k < l; k++) {
                            for (int j = 0; j < k; j++) {
                                pLits[ctr++] = pabc::Abc_Var2Lit(svars[ip][j][k][l], 0);
                            }
                        }
                    }
                }
                const auto res = solver->add_clause(pLits, pLits + ctr);
                assert(res);
            }
        }

        void fence_create_alonce_clauses(const spec& spec)
        {
            for (int i = 0; i < spec.nr_steps - 1; i++) {
                auto ctr = 0;
                const auto idx = spec.nr_in + i + 1;
                const auto level = get_level(spec, idx);
                for (int ip = i + 1; ip < spec.nr_steps; ip++) {
                    auto levelp = get_level(spec, ip + spec.nr_in + 1);
                    assert(levelp >= level);
                    if (levelp == level) {
                        continue;
                    }
                    auto svctr = 0;
                    for (int l = first_step_on_level(levelp - 1);
                        l < first_step_on_level(levelp); l++) {
                        for (int k = 1; k < l; k++) {
                            for (int j = 0; j < k; j++) {
                                if (j == idx || k == idx || l == idx) {
                                    const auto sel_var = get_sel_var(spec, ip, svctr);
                                    pLits[ctr++] = pabc::Abc_Var2Lit(sel_var, 0);
                                }
                                svctr++;
                            }
                        }
                    }
                    assert(svctr == nr_svars_for_step(spec, ip));
                }
                solver->add_clause(pLits, pLits + ctr);
            }
        }

        bool create_noreapply_clauses(const spec& spec)
        {
            // There seems to be no good analogy for this in MIGs
            assert(false);
            return false;
        }

        void create_lex_func_clauses(const spec& spec)
        {
            for (int i = 0; i < spec.nr_steps - 1; i++) {
                for (int l = 2; l <= spec.nr_in + i; l++) {
                    for (int k = 1; k < l; k++) {
                        for (int j = 0; j < k; j++) {
                            pLits[0] = pabc::Abc_Var2Lit(svars[i][j][k][l], 1);
                            pLits[1] = pabc::Abc_Var2Lit(svars[i + 1][j][k][l], 1);
                            pLits[2] = pabc::Abc_Var2Lit(get_op_var(spec, i, 3), 1);
                            pLits[3] = pabc::Abc_Var2Lit(get_op_var(spec, i + 1, 3), 1);
                            auto status = solver->add_clause(pLits, pLits + 4);
                            assert(status);
                            pLits[2] = pabc::Abc_Var2Lit(get_op_var(spec, i, 2), 1);
                            pLits[3] = pabc::Abc_Var2Lit(get_op_var(spec, i + 1, 0), 0);
                            pLits[4] = pabc::Abc_Var2Lit(get_op_var(spec, i + 1, 1), 0);
                            status = solver->add_clause(pLits, pLits + 5);
                            assert(status);
                            pLits[2] = pabc::Abc_Var2Lit(get_op_var(spec, i, 1), 1);
                            pLits[3] = pabc::Abc_Var2Lit(get_op_var(spec, i + 1, 0), 0);
                            status = solver->add_clause(pLits, pLits + 4);
                            assert(status);
                            pLits[2] = pabc::Abc_Var2Lit(get_op_var(spec, i, 0), 1);
                            status = solver->add_clause(pLits, pLits + 3);
                            assert(status);
                        }
                    }
                }
            }
        }

        void fence_create_lex_func_clauses(const spec& spec)
        {
            for (int i = 0; i < spec.nr_steps - 1; i++) {
                const auto level = get_level(spec, spec.nr_in + i + 1);
                const auto levelp = get_level(spec, spec.nr_in + i + 2);
                int svar_ctr = 0;
                for (int l = first_step_on_level(level - 1); 
                    l < first_step_on_level(level); l++) {
                    for (int k = 1; k < l; k++) {
                        for (int j = 0; j < k; j++) {
                            const auto sel_var = get_sel_var(spec, i, svar_ctr++);
                            pLits[0] = pabc::Abc_Var2Lit(sel_var, 1);

                            int svar_ctrp = 0;
                            for (int lp = first_step_on_level(levelp - 1);
                                lp < first_step_on_level(levelp); lp++) {
                                for (int kp = 1; kp < lp; kp++) {
                                    for (int jp = 0; jp < kp; jp++) {
                                        const auto sel_varp = get_sel_var(spec, i + 1, svar_ctrp++);
                                        if (j != jp || k != kp || l != lp) {
                                            continue;
                                        }
                                        pLits[1] = pabc::Abc_Var2Lit(sel_varp, 1);
                                        pLits[2] = pabc::Abc_Var2Lit(get_op_var(spec, i, 3), 1);
                                        pLits[3] = pabc::Abc_Var2Lit(get_op_var(spec, i + 1, 3), 1);
                                        auto status = solver->add_clause(pLits, pLits + 4);
                                        assert(status);
                                        pLits[2] = pabc::Abc_Var2Lit(get_op_var(spec, i, 2), 1);
                                        pLits[3] = pabc::Abc_Var2Lit(get_op_var(spec, i + 1, 0), 0);
                                        pLits[4] = pabc::Abc_Var2Lit(get_op_var(spec, i + 1, 1), 0);
                                        status = solver->add_clause(pLits, pLits + 5);
                                        assert(status);
                                        pLits[2] = pabc::Abc_Var2Lit(get_op_var(spec, i, 1), 1);
                                        pLits[3] = pabc::Abc_Var2Lit(get_op_var(spec, i + 1, 0), 0);
                                        status = solver->add_clause(pLits, pLits + 4);
                                        assert(status);
                                        pLits[2] = pabc::Abc_Var2Lit(get_op_var(spec, i, 0), 1);
                                        status = solver->add_clause(pLits, pLits + 3);
                                        assert(status);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        void create_colex_clauses(const spec& spec)
        {
            for (int i = 0; i < spec.nr_steps - 1; i++) {
                for (int l = 2; l <= spec.nr_in + i; l++) {
                    for (int k = 1; k < l; k++) {
                        for (int j = 0; j < k; j++) {
                            pLits[0] = pabc::Abc_Var2Lit(svars[i][j][k][l], 1);

                            // Cannot have lp < l
                            for (int lp = 2; lp < l; lp++) {
                                for (int kp = 1; kp < lp; kp++) {
                                    for (int jp = 0; jp < kp; jp++) {
                                        pLits[1] = pabc::Abc_Var2Lit(svars[i + 1][jp][kp][lp], 1);
                                        const auto res = solver->add_clause(pLits, pLits + 2);
                                        assert(res);
                                    }
                                }
                            }

                            // May have lp == l and kp > k
                            for (int kp = 1; kp < k; kp++) {
                                for (int jp = 0; jp < kp; jp++) {
                                    pLits[1] = pabc::Abc_Var2Lit(svars[i + 1][jp][kp][l], 1);
                                    const auto res = solver->add_clause(pLits, pLits + 2);
                                    assert(res);
                                }
                            }
                            // OR lp == l and kp == k
                            for (int jp = 0; jp < j; jp++) {
                                pLits[1] = pabc::Abc_Var2Lit(svars[i + 1][jp][k][l], 1);
                                const auto res = solver->add_clause(pLits, pLits + 2);
                                assert(res);
                            }
                        }
                    }
                }
            }
        }

        void fence_create_colex_clauses(const spec& spec)
        {
            for (int i = 0; i < spec.nr_steps - 1; i++) {
                const auto level = get_level(spec, i + spec.nr_in + 1);
                const auto levelp = get_level(spec, i + 1 + spec.nr_in + 1);
                int svar_ctr = 0;
                for (int l = first_step_on_level(level-1); 
                        l < first_step_on_level(level); l++) {
                    for (int k = 1; k < l; k++) {
                        for (int j = 0; j < k; j++) {
                            if (l < 3) {
                                svar_ctr++;
                                continue;
                            }
                            const auto sel_var = get_sel_var(spec, i, svar_ctr);
                            pLits[0] = pabc::Abc_Var2Lit(sel_var, 1);
                            int svar_ctrp = 0;
                            for (int lp = first_step_on_level(levelp - 1);
                                lp < first_step_on_level(levelp); lp++) {
                                for (int kp = 1; kp < lp; kp++) {
                                    for (int jp = 0; jp < kp; jp++) {
                                        if ((lp == l && kp == k && jp < j) || (lp == l && kp < k) || (lp < l)) {
                                            const auto sel_varp = get_sel_var(spec, i + 1, svar_ctrp);
                                            pLits[1] = pabc::Abc_Var2Lit(sel_varp, 1);
                                            (void)solver->add_clause(pLits, pLits + 2);
                                        }
                                        svar_ctrp++;
                                    }
                                }
                            }
                            svar_ctr++;
                        }
                    }
                }
            }
        }

        bool create_symvar_clauses(const spec& spec)
        {
            for (int q = 2; q <= spec.nr_in; q++) {
                for (int p = 1; p < q; p++) {
                    auto symm = true;
                    for (int i = 0; i < spec.nr_nontriv; i++) {
                        auto f = spec[spec.synth_func(i)];
                        if (!(swap(f, p - 1, q - 1) == f)) {
                            symm = false;
                            break;
                        }
                    }
                    if (!symm) {
                        continue;
                    }

                    for (int i = 1; i < spec.nr_steps; i++) {
                        for (int l = 2; l <= spec.nr_in + i; l++) {
                            for (int k = 1; k < l; k++) {
                                for (int j = 0; j < k; j++) {
                                    if (!(j == q || k == q || l == q) || (j == p || k == p)) {
                                        continue;
                                    }
                                    pLits[0] = pabc::Abc_Var2Lit(svars[i][j][k][l], 1);
                                    auto ctr = 1;
                                    for (int ip = 0; ip < i; ip++) {
                                        for (int lp = 2; lp <= spec.nr_in + ip; lp++) {
                                            for (int kp = 1; kp < lp; kp++) {
                                                for (int jp = 0; jp < kp; jp++) {
                                                    if (jp == p || kp == p || lp == p) {
                                                        pLits[ctr++] = pabc::Abc_Var2Lit(svars[ip][jp][kp][lp], 0);
                                                    }
                                                }
                                            }
                                        }
                                    }
                                    if (!solver->add_clause(pLits, pLits + ctr)) {
                                        return false;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            return true;
        }

        void fence_create_symvar_clauses(const spec& spec)
        {
            for (int q = 2; q <= spec.nr_in; q++) {
                for (int p = 1; p < q; p++) {
                    auto symm = true;
                    for (int i = 0; i < spec.nr_nontriv; i++) {
                        auto& f = spec[spec.synth_func(i)];
                        if (!(swap(f, p - 1, q - 1) == f)) {
                            symm = false;
                            break;
                        }
                    }
                    if (!symm) {
                        continue;
                    }
                    for (int i = 1; i < spec.nr_steps; i++) {
                        const auto level = get_level(spec, i + spec.nr_in + 1);
                        int svar_ctr = 0;
                        for (int l = first_step_on_level(level - 1);
                            l < first_step_on_level(level); l++) {
                            for (int k = 1; k < l; k++) {
                                for (int j = 0; j < k; j++) {
                                    if (!(j == q || k == q || l == q) || (j == p || k == p)) {
                                        svar_ctr++;
                                        continue;
                                    }
                                    const auto sel_var = get_sel_var(spec, i, svar_ctr);
                                    pLits[0] = pabc::Abc_Var2Lit(sel_var, 1);
                                    auto ctr = 1;
                                    for (int ip = 0; ip < i; ip++) {
                                        const auto levelp = get_level(spec, spec.nr_in + ip + 1);
                                        auto svar_ctrp = 0;
                                        for (int lp = first_step_on_level(levelp - 1);
                                            lp < first_step_on_level(levelp); lp++) {
                                            for (int kp = 1; kp < lp; kp++) {
                                                for (int jp = 0; jp < kp; jp++) {
                                                    if (jp == p || kp == p || lp == p) {
                                                        const auto sel_varp = get_sel_var(spec, ip, svar_ctrp);
                                                        pLits[ctr++] = pabc::Abc_Var2Lit(sel_varp, 0);
                                                    }
                                                    svar_ctrp++;
                                                }
                                            }
                                        }
                                    }
                                    (void)solver->add_clause(pLits, pLits + ctr);
                                    svar_ctr++;
                                }
                            }
                        }
                    }
                }
            }
        }

        void reset_sim_tts(int nr_in)
        {
            for (int i = 0; i < NR_SIM_TTS; i++) {
                sim_tts[i] = kitty::dynamic_truth_table(nr_in);
                if (i < nr_in) {
                    kitty::create_nth_var(sim_tts[i], i);
                }
            }
        }

        bool encode(spec& spec)
        { 

            assert(spec.nr_in >= 3);
            
            spec.add_noreapply_clauses = false;
            spec.add_colex_clauses = false;
            
            create_variables(spec);
            create_main_clauses(spec);

            if (!create_fanin_clauses(spec)) {
                return false;
            }

            if (spec.add_alonce_clauses) {
                create_alonce_clauses(spec);
            }

            /* if (spec.add_colex_clauses) {
                create_colex_clauses(spec);
            }*/
            
            if (spec.add_lex_func_clauses) {
                create_lex_func_clauses(spec);
            }

            if (spec.add_symvar_clauses && !create_symvar_clauses(spec)) {
                return false;
            }

            return true;
        }

        void update_level_map(const spec& spec, const fence& f)
        {
            nr_levels = f.nr_levels();
            level_dist[0] = spec.nr_in + 1;
            for (int i = 1; i <= nr_levels; i++) {
                level_dist[i] = level_dist[i-1] + f.at(i-1);
            }
        }

        int get_level(const spec& spec, int step_idx) const
        {
            // PIs are considered to be on level zero.
            if (step_idx <= spec.nr_in) {
                return 0;
            } else if (step_idx == spec.nr_in + 1) { 
                // First step is always on level one
                return 1;
            }
            for (int i = 0; i <= nr_levels; i++) {
                if (level_dist[i] > step_idx) {
                    return i;
                }
            }
            return -1;
        }

        void fence_create_fanin_clauses(const spec& spec)
        {
            for (int i = 0; i < spec.nr_steps; i++) {
                const auto nr_svars_for_i = nr_svars_for_step(spec, i);
                for (int j = 0; j < nr_svars_for_i; j++) {
                    const auto sel_var = get_sel_var(spec, i, j);
                    pLits[j] = pabc::Abc_Var2Lit(sel_var, 0);
                }

                const auto res = solver->add_clause(pLits, pLits + nr_svars_for_i);
                assert(res);
            }
        }

        bool
        encode(const spec& spec, const fence& f)
        {
            assert(spec.nr_in >= 3);
            assert(spec.nr_steps == f.nr_nodes());

            bool success = true;

            update_level_map(spec, f);
            fence_create_variables(spec);
            if (!fence_create_main_clauses(spec)) {
                return false;
            }

            fence_create_fanin_clauses(spec);

            if (spec.add_alonce_clauses) {
                fence_create_alonce_clauses(spec);
            }
            
            /*if (spec.add_colex_clauses) {
                fence_create_colex_clauses(spec);
            }*/

            if (spec.add_lex_func_clauses) {
                fence_create_lex_func_clauses(spec);
            }

            if (spec.add_symvar_clauses) {
                fence_create_symvar_clauses(spec);
            }

            return true;
        }

        bool
        cegar_encode(const spec&, const partial_dag&)
        {
            // TODO: implement!
            assert(false);
            return false;
        }

        void extract_mig(const spec& spec, mig& chain)
        {
            int op_inputs[3] = { 0, 0, 0 };

            chain.reset(spec.nr_in, 1, spec.nr_steps);

            for (int i = 0; i < spec.nr_steps; i++) {
                int op = 0;
                for (int j = 0; j < MIG_OP_VARS_PER_STEP; j++) {
                    if (solver->var_value(get_op_var(spec, i, j))) {
                        op = j;
                        break;
                    }
                }

                if (spec.verbosity) {
                    printf("  step x_%d performs operation ",
                        i + spec.nr_in + 1);
                    switch (op) {
                    case 0:
                        printf("<abc>\n");
                        break;
                    case 1:
                        printf("<!abc>\n");
                        break;
                    case 2:
                        printf("<a!bc>\n");
                        break;
                    case 3:
                        printf("<ab!c>\n");
                        break;
                    default:
                        fprintf(stderr, "Error: unexpected MIG operator\n");
                        exit(1);
                        break;
                    }
                }

                for (int l = 2; l <= spec.nr_in + i; l++) {
                    for (int k = 1; k < l; k++) {
                        for (int j = 0; j < k; j++) {
                            const auto sel_var = svars[i][j][k][l];
                            if (solver->var_value(sel_var)) {
                                op_inputs[0] = j;
                                op_inputs[1] = k;
                                op_inputs[2] = l;
                                break;
                            }
                        }
                    }
                }
                chain.set_step(i, op_inputs[0], op_inputs[1], op_inputs[2], op);
            }

            // TODO: support multiple outputs
            chain.set_output(0,
                ((spec.nr_steps + spec.nr_in) << 1) +
                ((spec.out_inv) & 1));
        }

        void fence_extract_mig(const spec& spec, mig& chain)
        {
            int op_inputs[3] = { 0, 0, 0 };

            chain.reset(spec.nr_in, 1, spec.nr_steps);

            for (int i = 0; i < spec.nr_steps; i++) {
                int op = 0;
                for (int j = 0; j < MIG_OP_VARS_PER_STEP; j++) {
                    if (solver->var_value(get_op_var(spec, i, j))) {
                        op = j;
                        break;
                    }
                }

                if (spec.verbosity) {
                    printf("  step x_%d performs operation ",
                        i + spec.nr_in + 1);
                    switch (op) {
                    case 0:
                        printf("<abc>\n");
                        break;
                    case 1:
                        printf("<!abc>\n");
                        break;
                    case 2:
                        printf("<a!bc>\n");
                        break;
                    case 3:
                        printf("<ab!c>\n");
                        break;
                    default:
                        fprintf(stderr, "Error: unexpected MIG operator\n");
                        exit(1);
                        break;
                    }
                }

                int ctr = 0;
                const auto level = get_level(spec, spec.nr_in + i + 1);
                for (int l = first_step_on_level(level - 1); 
                    l < first_step_on_level(level); l++) {
                    for (int k = 1; k < l; k++) {
                        for (int j = 0; j < k; j++) {
                            const auto sel_var = get_sel_var(spec, i, ctr++);
                            if (solver->var_value(sel_var)) {
                                op_inputs[0] = j;
                                op_inputs[1] = k;
                                op_inputs[2] = l;
                                break;
                            }
                        }
                    }
                }
                chain.set_step(i, op_inputs[0], op_inputs[1], op_inputs[2], op);
            }

            // TODO: support multiple outputs
            chain.set_output(0,
                ((spec.nr_steps + spec.nr_in) << 1) +
                ((spec.out_inv) & 1));
        }

        void print_solver_state(spec& spec)
        {
            for (auto i = 0; i < spec.nr_steps; i++) {
                for (int l = 2; l <= spec.nr_in + i; l++) {
                    for (int k = 1; k < l; k++) {
                        for (int j = 0; j < k; j++) {
                            const auto sel_var = svars[i][j][k][l];
                            if (solver->var_value(sel_var)) {
                                printf("s[%d][%d][%d][%d]=1\n", i, j, k, l);
                            } else {
                                printf("s[%d][%d][%d][%d]=0\n", i, j, k, l);
                            }
                        }
                    }
                }
            }

            for (auto i = 0; i < spec.nr_steps; i++) {
                for (int j = 0; j < MIG_OP_VARS_PER_STEP; j++) {
                    if (solver->var_value(get_op_var(spec, i, j))) {
                        printf("op_%d_%d=1\n", i, j);
                    } else {
                        printf("op_%d_%d=0\n", i, j);
                    }
                }
            }

            for (auto i = 0; i < spec.nr_steps; i++) {
                printf("tt_%d_0=0\n", i);
                for (int t = 0; t < spec.tt_size; t++) {
                    const auto sim_var = get_sim_var(spec, i, t);
                    if (solver->var_value(sim_var)) {
                        printf("tt_%d_%d=1\n", i, t + 1);
                    } else {
                        printf("tt_%d_%d=0\n", i, t + 1);
                    }
                }
            }
        }

        bool is_dirty() 
        {
            return dirty;
        }

        void set_dirty(bool _dirty)
        {
            dirty = _dirty;
        }
        
        bool cegar_encode(const spec& spec)
        {
            // TODO: implement
            assert(false);
            return false;
        }
        
        bool block_solution(const spec& spec)
        {
            // TODO: implement
            assert(false);
            return false;
        }
        
        bool block_struct_solution(const spec& spec)
        {
            // TODO: implement
            assert(false);
            return false;
        }
        
    };
}
