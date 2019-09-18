#pragma once

#include <vector>
#include <kitty/kitty.hpp>
#include "encoder.hpp"
#include "../partial_dag.hpp"

namespace percy
{
    

    class partial_dag_encoder : public encoder
    {
    private:
        int nr_sel_vars;
        int nr_res_vars;
        int nr_op_vars;
        int nr_sim_vars;
        int total_nr_vars;
        int sel_offset;
        int res_offset;
        int ops_offset;
        int sim_offset;
        pabc::Vec_Int_t* vLits = NULL;

        // We only support fanin-2 gates for now,
        // so this is a constant.
        const int PD_OP_VARS_PER_STEP = 3;

        static const int NR_SIM_TTS = 32;
        std::vector<kitty::dynamic_truth_table> sim_tts { NR_SIM_TTS };

        int nr_svars_for_step(
            const spec& spec, 
            const partial_dag& dag, 
            int i) const
        {
            const auto& vertex = dag.get_vertex(i);
            auto nr_pi_fanins = 0;
            if (vertex[1] == FANIN_PI) {
                // If the second fanin is a PI, the first one 
                // certainly is.
                nr_pi_fanins = 2;
            } else if (vertex[0] == FANIN_PI) {
                nr_pi_fanins = 1;
            }
            switch (nr_pi_fanins) {
            case 1:
                return spec.nr_in;
            case 2:
                return (spec.nr_in * (spec.nr_in - 1)) / 2;
            default: // No fanin flexibility
                return 0;
            }
        }

        int nr_pi_fanins_for_step(const partial_dag& dag, int i) const
        {
            const auto& vertex = dag.get_vertex(i);
            auto nr_pi_fanins = 0;
            if (vertex[1] == FANIN_PI) {
                nr_pi_fanins = 2;
            } else if (vertex[0] == FANIN_PI) {
                nr_pi_fanins = 1;
            }

            return nr_pi_fanins;
        }

        int get_sel_var(
            const spec& spec,
            const partial_dag& dag,
            int step_idx,
            int var_idx) const
        {
            auto offset = 0;
            for (int i = 0; i < step_idx; i++) {
                offset += nr_svars_for_step(spec, dag, i);
            }
            return offset + var_idx;
        }

        int get_res_var(
            const spec& spec, 
            const partial_dag& dag, 
            int step_idx, 
            int res_var_idx) const
        {
            auto offset = 0;
            for (int i = 0; i < step_idx; i++) {
                offset += (nr_svars_for_step(spec, dag, i) + 1) * (1 + 2);
            }

            return res_offset + offset + res_var_idx;
        }

        int get_sim_var(const spec& spec, int step_idx, int t) const
        {
            return sim_offset + spec.get_tt_size() * step_idx + t;
        }

        int get_op_var(int step_idx, int var_idx) const
        {
            return ops_offset + step_idx * PD_OP_VARS_PER_STEP + var_idx;
        }

    public:
        partial_dag_encoder()
        {
            vLits = pabc::Vec_IntAlloc(128);
        }

        partial_dag_encoder(solver_wrapper& solver)
        {
            set_solver(solver);
            vLits = pabc::Vec_IntAlloc(128);
        }

        ~partial_dag_encoder()
        {
            pabc::Vec_IntFree(vLits);
        }

        void create_variables(const spec& spec, const partial_dag& dag)
        {
            nr_op_vars = spec.nr_steps * PD_OP_VARS_PER_STEP;
            nr_sim_vars = spec.nr_steps * spec.get_tt_size();

            nr_sel_vars = 0;
            for (int i = 0; i < spec.nr_steps; i++) {
                const auto nr_svars_for_i = nr_svars_for_step(spec, dag, i);
                nr_sel_vars += nr_svars_for_i;
            }

            sel_offset = 0;
            ops_offset = nr_sel_vars;
            sim_offset = nr_sel_vars + nr_op_vars;
            total_nr_vars = nr_sel_vars + nr_op_vars + nr_sim_vars;

            if (spec.verbosity > 1) {
                printf("Creating variables (PD-%d)\n", spec.fanin);
                printf("nr steps = %d\n", spec.nr_steps);
                printf("nr_sel_vars=%d\n", nr_sel_vars);
                printf("nr_op_vars = %d\n", nr_op_vars);
                printf("nr_sim_vars = %d\n", nr_sim_vars);
                printf("creating %d total variables\n", total_nr_vars);
            }

            solver->set_nr_vars(total_nr_vars);
        }

        void cegar_create_variables(const spec& spec, const partial_dag& dag)
        {
            nr_op_vars = spec.nr_steps * PD_OP_VARS_PER_STEP;
            nr_sim_vars = spec.nr_steps * spec.get_tt_size();

            nr_sel_vars = 0;
            nr_res_vars = 0;
            for (int i = 0; i < spec.nr_steps; i++) {
                const auto nr_svars_for_i = nr_svars_for_step(spec, dag, i);
                nr_sel_vars += nr_svars_for_i;
                nr_res_vars += (nr_svars_for_i + 1) * (1 + 2);
            }

            sel_offset = 0;
            res_offset = nr_sel_vars;
            ops_offset = nr_sel_vars + nr_res_vars;
            sim_offset = nr_sel_vars + nr_res_vars + nr_op_vars;
            total_nr_vars = nr_sel_vars + nr_res_vars + nr_op_vars + nr_sim_vars;

            if (spec.verbosity > 1) {
                printf("Creating variables (PD-%d)\n", spec.fanin);
                printf("nr steps = %d\n", spec.nr_steps);
                printf("nr_sel_vars=%d\n", nr_sel_vars);
                printf("nr_res_vars=%d\n", nr_res_vars);
                printf("nr_op_vars = %d\n", nr_op_vars);
                printf("nr_sim_vars = %d\n", nr_sim_vars);
                printf("creating %d total variables\n", total_nr_vars);
            }

            solver->set_nr_vars(total_nr_vars);
        }

        /// Ensures that each gate has the proper number of fanins.
        bool create_fanin_clauses(const spec& spec, const partial_dag& dag)
        {
            auto status = true;

            if (spec.verbosity > 2) {
                printf("Creating op clauses (KNUTH-%d)\n", spec.fanin);
                printf("Nr. clauses = %d (PRE)\n", solver->nr_clauses());
            }

            for (int i = 0; i < spec.nr_steps; i++) {
                const auto nr_svars_for_i = nr_svars_for_step(spec, dag, i);
                if (nr_svars_for_i == 0) {
                    continue;
                }

                for (int j = 0; j < nr_svars_for_i; j++) {
                    pabc::Vec_IntSetEntry(vLits, j,
                        pabc::Abc_Var2Lit(get_sel_var(spec, dag, i, j), 0));
                }

                status &= solver->add_clause(
                    pabc::Vec_IntArray(vLits),
                    pabc::Vec_IntArray(vLits) + nr_svars_for_i);
            }
            if (spec.verbosity > 2) {
                printf("Nr. clauses = %d (POST)\n", solver->nr_clauses());
            }

            return status;
        }

        /// The simulation variables of the final step must be equal to
        /// the function we're trying to synthesize.
        bool fix_output_sim_vars(const spec& spec)
        {
            bool ret = true;
            auto ilast_step = spec.nr_steps - 1;

            for (int t = 0; t < spec.get_tt_size(); t++) {
                auto outbit = kitty::get_bit(
                    spec[spec.synth_func(0)], t + 1);
                if ((spec.out_inv >> spec.synth_func(0)) & 1) {
                    outbit = 1 - outbit;
                }
                const auto sim_var = get_sim_var(spec, ilast_step, t);
                pabc::lit sim_lit = pabc::Abc_Var2Lit(sim_var, 1 - outbit);
                ret &= solver->add_clause(&sim_lit, &sim_lit + 1);
            }

            return ret;
        }

        void vfix_output_sim_vars(const spec& spec)
        {
            auto ilast_step = spec.nr_steps - 1;

            for (int t = 0; t < spec.get_tt_size(); t++) {
                auto outbit = kitty::get_bit(
                    spec[spec.synth_func(0)], t + 1);
                if ((spec.out_inv >> spec.synth_func(0)) & 1) {
                    outbit = 1 - outbit;
                }
                const auto sim_var = get_sim_var(spec, ilast_step, t);
                pabc::lit sim_lit = pabc::Abc_Var2Lit(sim_var, 1 - outbit);
                (void)solver->add_clause(&sim_lit, &sim_lit + 1);
            }
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
            (void)solver->add_clause(&sim_lit, &sim_lit + 1);
        }

        bool create_nontriv_clauses(const spec& spec)
        {
            int pLits[3];
            bool status = true;
            for (int i = 0; i < spec.nr_steps; i++) {
                // Dissallow the constant zero operator.
                pLits[0] = pabc::Abc_Var2Lit(get_op_var(i, 0), 0);
                pLits[1] = pabc::Abc_Var2Lit(get_op_var(i, 1), 0);
                pLits[2] = pabc::Abc_Var2Lit(get_op_var(i, 2), 0);
                status &= solver->add_clause(pLits, pLits + 3);

                // Dissallow variable projections.
                pLits[0] = pabc::Abc_Var2Lit(get_op_var(i, 0), 0);
                pLits[1] = pabc::Abc_Var2Lit(get_op_var(i, 1), 1);
                pLits[2] = pabc::Abc_Var2Lit(get_op_var(i, 2), 1);
                status &= solver->add_clause(pLits, pLits + 3);

                pLits[0] = pabc::Abc_Var2Lit(get_op_var(i, 0), 1);
                pLits[1] = pabc::Abc_Var2Lit(get_op_var(i, 1), 0);
                pLits[2] = pabc::Abc_Var2Lit(get_op_var(i, 2), 1);
                status &= solver->add_clause(pLits, pLits + 3);
            }

            return status;
        }

        bool add_simulation_clause(
            const spec& spec,
            const int t,
            const int i,
            const int j,
            const int k,
            const int a,
            const int b,
            const int c)
        {
            int pLits[4];
            int ctr = 0;

            if (j < spec.nr_in) {
                if ((((t + 1) & (1 << j)) ? 1 : 0) != b) {
                    return true;
                }
            } else {
                pLits[ctr++] = pabc::Abc_Var2Lit(
                    get_sim_var(spec, j - spec.nr_in, t), b);
            }

            if (k < spec.nr_in) {
                if ((((t + 1) & (1 << k)) ? 1 : 0) != c) {
                    return true;
                }
            } else {
                pLits[ctr++] = pabc::Abc_Var2Lit(
                    get_sim_var(spec, k - spec.nr_in, t), c);
            }

            pLits[ctr++] = pabc::Abc_Var2Lit(get_sim_var(spec, i, t), a);

            if (b | c) {
                pLits[ctr++] = pabc::Abc_Var2Lit(
                    get_op_var(i, ((c << 1) | b) - 1), 1 - a);
            }

            auto status = solver->add_clause(pLits, pLits + ctr);

            return status;
        }

        bool add_simulation_clause(
            const spec& spec,
            const int t,
            const int i,
            const int j,
            const int k,
            const int a,
            const int b,
            const int c,
            int sel_var)
        {
            int pLits[5];
            int ctr = 0;

            if (j < spec.nr_in) {
                if ((((t + 1) & (1 << j)) ? 1 : 0) != b) {
                    return true;
                }
            } else {
                pLits[ctr++] = pabc::Abc_Var2Lit(
                    get_sim_var(spec, j - spec.nr_in, t), b);
            }

            if (k < spec.nr_in) {
                if ((((t + 1) & (1 << k)) ? 1 : 0) != c) {
                    return true;
                }
            } else {
                pLits[ctr++] = pabc::Abc_Var2Lit(
                    get_sim_var(spec, k - spec.nr_in, t), c);
            }

            pLits[ctr++] = pabc::Abc_Var2Lit(sel_var, 1);
            pLits[ctr++] = pabc::Abc_Var2Lit(get_sim_var(spec, i, t), a);

            if (b | c) {
                pLits[ctr++] = pabc::Abc_Var2Lit(
                    get_op_var(i, ((c << 1) | b) - 1), 1 - a);
            }

            return solver->add_clause(pLits, pLits + ctr);
        }

        bool create_tt_clauses(
            const spec& spec,
            const partial_dag& dag,
            const int t)
        {
            auto ret = true;

            for (int i = 0; i < spec.nr_steps; i++) {
                const auto& vertex = dag.get_vertex(i);
                auto nr_pi_fanins = 0;
                if (vertex[1] == FANIN_PI) {
                    // If the second fanin is a PI, the first one 
                    // certainly is.
                    nr_pi_fanins = 2;
                } else if (vertex[0] == FANIN_PI) {
                    nr_pi_fanins = 1;
                }
                if (nr_pi_fanins == 0) {
                    // The fanins for this step are fixed
                    const auto j = vertex[0] + spec.nr_in - 1;
                    const auto k = vertex[1] + spec.nr_in - 1;
                    ret &= add_simulation_clause(spec, t, i, j, k, 0, 0, 1);
                    ret &= add_simulation_clause(spec, t, i, j, k, 0, 1, 0);
                    ret &= add_simulation_clause(spec, t, i, j, k, 0, 1, 1);
                    ret &= add_simulation_clause(spec, t, i, j, k, 1, 0, 0);
                    ret &= add_simulation_clause(spec, t, i, j, k, 1, 0, 1);
                    ret &= add_simulation_clause(spec, t, i, j, k, 1, 1, 0);
                    ret &= add_simulation_clause(spec, t, i, j, k, 1, 1, 1);
                } else if (nr_pi_fanins == 1) {
                    // The first fanin is flexible
                    const auto k = vertex[1] + spec.nr_in - 1;
                    auto ctr = 0;
                    for (int j = 0; j < spec.nr_in; j++) {
                        const auto sel_var = get_sel_var(spec, dag, i, j);
                        ret &= add_simulation_clause(spec, t, i, j, k, 0, 0, 1, sel_var);
                        ret &= add_simulation_clause(spec, t, i, j, k, 0, 1, 0, sel_var);
                        ret &= add_simulation_clause(spec, t, i, j, k, 0, 1, 1, sel_var);
                        ret &= add_simulation_clause(spec, t, i, j, k, 1, 0, 0, sel_var);
                        ret &= add_simulation_clause(spec, t, i, j, k, 1, 0, 1, sel_var);
                        ret &= add_simulation_clause(spec, t, i, j, k, 1, 1, 0, sel_var);
                        ret &= add_simulation_clause(spec, t, i, j, k, 1, 1, 1, sel_var);
                        ctr++;
                    }
                } else {
                    // Both fanins are fully flexible
                    auto ctr = 0;
                    for (int k = 1; k < spec.nr_in; k++) {
                        for (int j = 0; j < k; j++) {
                            const auto sel_var = get_sel_var(spec, dag, i, ctr);
                            ret &= add_simulation_clause(spec, t, i, j, k, 0, 0, 1, sel_var);
                            ret &= add_simulation_clause(spec, t, i, j, k, 0, 1, 0, sel_var);
                            ret &= add_simulation_clause(spec, t, i, j, k, 0, 1, 1, sel_var);
                            ret &= add_simulation_clause(spec, t, i, j, k, 1, 0, 0, sel_var);
                            ret &= add_simulation_clause(spec, t, i, j, k, 1, 0, 1, sel_var);
                            ret &= add_simulation_clause(spec, t, i, j, k, 1, 1, 0, sel_var);
                            ret &= add_simulation_clause(spec, t, i, j, k, 1, 1, 1, sel_var);
                            ctr++;
                        }
                    }
                }
            }

            return ret;
        }

        // A version that does not compute a return value.
        void vcreate_tt_clauses(
            const spec& spec,
            const partial_dag& dag,
            const int t)
        {
            for (int i = 0; i < spec.nr_steps; i++) {
                const auto& vertex = dag.get_vertex(i);
                auto nr_pi_fanins = 0;
                if (vertex[1] == FANIN_PI) {
                    // If the second fanin is a PI, the first one 
                    // certainly is.
                    nr_pi_fanins = 2;
                } else if (vertex[0] == FANIN_PI) {
                    nr_pi_fanins = 1;
                }
                if (nr_pi_fanins == 0) {
                    // The fanins for this step are fixed
                    const auto j = vertex[0] + spec.nr_in - 1;
                    const auto k = vertex[1] + spec.nr_in - 1;
                    (void)add_simulation_clause(spec, t, i, j, k, 0, 0, 1);
                    (void)add_simulation_clause(spec, t, i, j, k, 0, 1, 0);
                    (void)add_simulation_clause(spec, t, i, j, k, 0, 1, 1);
                    (void)add_simulation_clause(spec, t, i, j, k, 1, 0, 0);
                    (void)add_simulation_clause(spec, t, i, j, k, 1, 0, 1);
                    (void)add_simulation_clause(spec, t, i, j, k, 1, 1, 0);
                    (void)add_simulation_clause(spec, t, i, j, k, 1, 1, 1);
                } else if (nr_pi_fanins == 1) {
                    // The first fanin is flexible
                    const auto k = vertex[1] + spec.nr_in - 1;
                    auto ctr = 0;
                    for (int j = 0; j < spec.nr_in; j++) {
                        const auto sel_var = get_sel_var(spec, dag, i, j);
                        (void)add_simulation_clause(spec, t, i, j, k, 0, 0, 1, sel_var);
                        (void)add_simulation_clause(spec, t, i, j, k, 0, 1, 0, sel_var);
                        (void)add_simulation_clause(spec, t, i, j, k, 0, 1, 1, sel_var);
                        (void)add_simulation_clause(spec, t, i, j, k, 1, 0, 0, sel_var);
                        (void)add_simulation_clause(spec, t, i, j, k, 1, 0, 1, sel_var);
                        (void)add_simulation_clause(spec, t, i, j, k, 1, 1, 0, sel_var);
                        (void)add_simulation_clause(spec, t, i, j, k, 1, 1, 1, sel_var);
                        ctr++;
                    }
                } else {
                    // Both fanins are fully flexible
                    auto ctr = 0;
                    for (int k = 1; k < spec.nr_in; k++) {
                        for (int j = 0; j < k; j++) {
                            const auto sel_var = get_sel_var(spec, dag, i, ctr);
                            (void)add_simulation_clause(spec, t, i, j, k, 0, 0, 1, sel_var);
                            (void)add_simulation_clause(spec, t, i, j, k, 0, 1, 0, sel_var);
                            (void)add_simulation_clause(spec, t, i, j, k, 0, 1, 1, sel_var);
                            (void)add_simulation_clause(spec, t, i, j, k, 1, 0, 0, sel_var);
                            (void)add_simulation_clause(spec, t, i, j, k, 1, 0, 1, sel_var);
                            (void)add_simulation_clause(spec, t, i, j, k, 1, 1, 0, sel_var);
                            (void)add_simulation_clause(spec, t, i, j, k, 1, 1, 1, sel_var);
                            ctr++;
                        }
                    }
                }
            }
        }

        void create_main_clauses(const spec& spec, const partial_dag& dag)
        {
            for (int t = 0; t < spec.get_tt_size(); t++) {
                vcreate_tt_clauses(spec, dag, t);
            }
        }

        bool reapply_helper(
            const spec& spec,
            const partial_dag& dag,
            int *svars,
            int depth,
            int i, 
            int j,
            int k,
            int ip,
            int jp,
            int kp,
            int jpp,
            int kpp) {
            int pLits[3];
            if (depth == 3) {
                if (j == jp && k == kp &&
                    (spec.nr_in + i) == jpp
                    && (spec.nr_in + ip) == kpp) {
                    const auto sel_var = svars[0];
                    const auto sel_varp = svars[1];
                    const auto sel_varpp = svars[2];
                    auto ctr = 0;
                    if (sel_var != -1) {
                        pLits[ctr++] = pabc::Abc_Var2Lit(sel_var, 1);
                    }
                    if (sel_varp != -1) {
                        pLits[ctr++] = pabc::Abc_Var2Lit(sel_varp, 1);
                    }
                    if (sel_varpp != -1) {
                        pLits[ctr++] = pabc::Abc_Var2Lit(sel_varpp, 1);
                    }
                    if (ctr > 1) {
                        return solver->add_clause(pLits, pLits + ctr);
                    }
                }
            } else if (depth == 2) {
                if (kp == (spec.nr_in + i) &&
                    (jp == j || jp == k)) {
                    const auto sel_var = svars[0];
                    const auto sel_varp = svars[1];
                    auto ctr = 0;
                    if (sel_var != -1) {
                        pLits[ctr++] = pabc::Abc_Var2Lit(sel_var, 1);
                    }
                    if (sel_varp != -1) {
                        pLits[ctr++] = pabc::Abc_Var2Lit(sel_varp, 1);
                    }
                    if (ctr > 1) {
                        return solver->add_clause(pLits, pLits + ctr);
                    }
                }
                for (int ipp = ip + 1; ipp < spec.nr_steps; ipp++) {
                    const auto& vertex = dag.get_vertex(ipp);
                    auto nr_pi_fanins = 0;
                    if (vertex[1] == FANIN_PI) {
                        nr_pi_fanins = 2;
                    } else if (vertex[0] == FANIN_PI) {
                        nr_pi_fanins = 1;
                    }
                    if (nr_pi_fanins == 0) {
                        svars[2] = -1;
                        jpp = spec.nr_in + vertex[0] - 1;
                        kpp = spec.nr_in + vertex[1] - 1;
                        reapply_helper(spec, dag, svars, 3, i, j, k, ip, jp, kp, jpp, kpp);
                    } else if (nr_pi_fanins == 1) {
                        kpp = spec.nr_in + vertex[1] - 1;
                        for (jpp = 0; jpp < spec.nr_in; jpp++) {
                            svars[2] = get_sel_var(spec, dag, ipp, jpp);
                            reapply_helper(spec, dag, svars, 3, i, j, k, ip, jp, kp, jpp, kpp);
                        }
                    } else {
                        auto svar_ctr = 0;
                        for (kpp = 1; kpp < spec.nr_in; kpp++) {
                            for (jpp = 0; jpp < kpp; jpp++) {
                                const auto sel_var = get_sel_var(spec, dag, ipp, svar_ctr++);
                                svars[2] = sel_var;
                                reapply_helper(spec, dag, svars, 3, i, j, k, ip, jp, kp, jpp, kpp);
                            }
                        }
                    }
                }
            } else if (depth == 1) {
                for (ip = i + 1; ip < spec.nr_steps; ip++) {
                    const auto& vertex = dag.get_vertex(ip);
                    auto nr_pi_fanins = 0;
                    if (vertex[1] == FANIN_PI) {
                        nr_pi_fanins = 2;
                    } else if (vertex[0] == FANIN_PI) {
                        nr_pi_fanins = 1;
                    }
                    if (nr_pi_fanins == 0) {
                        svars[1] = -1;
                        jp = spec.nr_in + vertex[0] - 1;
                        kp = spec.nr_in + vertex[1] - 1;
                        reapply_helper(spec, dag, svars, 2, i, j, k, ip, jp, kp, 0, 0);
                    } else if (nr_pi_fanins == 1) {
                        kp = spec.nr_in + vertex[1] - 1;
                        for (jp = 0; jp < spec.nr_in; jp++) {
                            svars[1] = get_sel_var(spec, dag, ip, jp);
                            reapply_helper(spec, dag, svars, 2, i, j, k, ip, jp, kp, 0, 0);
                        }
                    } else {
                        auto svar_ctr = 0;
                        for (kp = 1; kp < spec.nr_in; kp++) {
                            for (jp = 0; jp < kp; jp++) {
                                const auto sel_var = get_sel_var(spec, dag, ip, svar_ctr++);
                                svars[1] = sel_var;
                                reapply_helper(spec, dag, svars, 2, i, j, k, ip, jp, kp, 0, 0);
                            }
                        }
                    }
                }
            } else {
                for (i = 0; i < spec.nr_steps; i++) {
                    const auto& vertex = dag.get_vertex(i);
                    auto nr_pi_fanins = 0;
                    if (vertex[1] == FANIN_PI) {
                        nr_pi_fanins = 2;
                    } else if (vertex[0] == FANIN_PI) {
                        nr_pi_fanins = 1;
                    }
                    if (nr_pi_fanins == 0) {
                        svars[0] = -1;
                        j = spec.nr_in + vertex[0] - 1;
                        k = spec.nr_in + vertex[1] - 1;
                        reapply_helper(spec, dag, svars, 1, i, j, k, 0, 0, 0, 0, 0);
                    } else if (nr_pi_fanins == 1) {
                        k = spec.nr_in + vertex[1] - 1;
                        for (j = 0; j < spec.nr_in; j++) {
                            svars[0] = get_sel_var(spec, dag, i, j);
                            reapply_helper(spec, dag, svars, 1, i, j, k, 0, 0, 0, 0, 0);
                        }
                    } else {
                        auto svar_ctr = 0;
                        for (k = 1; k < spec.nr_in; k++) {
                            for (j = 0; j < k; j++) {
                                const auto sel_var = get_sel_var(spec, dag, i, svar_ctr++);
                                svars[0] = sel_var;
                                reapply_helper(spec, dag, svars, 1, i, j, k, 0, 0, 0, 0, 0);
                            }
                        }
                    }
                }
            }

            return true;
        }

        bool create_noreapply_clauses(const spec& spec, const partial_dag& dag)
        {
            int svars[3];
            
            return reapply_helper(spec, dag, svars, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        }

        bool create_symvar_clauses(const spec& spec, const partial_dag& dag)
        {
            for (int q = 1; q < spec.nr_in; q++) {
                for (int p = 0; p < q; p++) {
                    auto symm = true;
                    for (int i = 0; i < spec.nr_nontriv; i++) {
                        auto f = spec[spec.synth_func(i)];
                        if (!(swap(f, p, q) == f)) {
                            symm = false;
                            break;
                        }
                    }
                    if (!symm) {
                        continue;
                    }

                    for (int i = 1; i < spec.nr_steps; i++) {
                        const auto vertex = dag.get_vertex(i);
                        auto nr_pi_fanins = 0;
                        if (vertex[1] == FANIN_PI) {
                            // If the second fanin is a PI, the first one 
                            // certainly is.
                            nr_pi_fanins = 2;
                        } else if (vertex[0] == FANIN_PI) {
                            nr_pi_fanins = 1;
                        }
                        if (nr_pi_fanins == 0) {
                            continue;
                        }
                        if (nr_pi_fanins == 1) {
                            const auto sel_var = get_sel_var(spec, dag, i, q);
                            pabc::Vec_IntSetEntry(vLits, 0,
                                pabc::Abc_Var2Lit(sel_var, 1));
                            auto ctr = 1;
                            for (int ip = 0; ip < i; ip++) {
                                const auto vertex2 = dag.get_vertex(ip);
                                auto nr_pi_fanins2 = 0;
                                if (vertex2[1] == FANIN_PI) {
                                    // If the second fanin is a PI, the first one 
                                    // certainly is.
                                    nr_pi_fanins2 = 2;
                                } else if (vertex2[0] == FANIN_PI) {
                                    nr_pi_fanins2 = 1;
                                }
                                if (nr_pi_fanins2 == 0) {
                                    continue;
                                } else if (nr_pi_fanins2 == 1) {
                                    const auto sel_varp = get_sel_var(spec, dag, ip, p);
                                    pabc::Vec_IntSetEntry(vLits, ctr++,
                                        pabc::Abc_Var2Lit(sel_varp, 0));
                                } else {
                                    auto svar_ctr = 0;
                                    for (int k = 1; k < spec.nr_in; k++) {
                                        for (int j = 0; j < k; j++) {
                                            if (j == p || k == p) {
                                                const auto sel_varp = get_sel_var(spec, dag, ip, svar_ctr);
                                                pabc::Vec_IntSetEntry(vLits, ctr++,
                                                    pabc::Abc_Var2Lit(sel_varp, 0));
                                            }
                                            svar_ctr++;
                                        }
                                    }
                                }
                            }
                            if (!solver->add_clause(Vec_IntArray(vLits), Vec_IntArray(vLits) + ctr)) {
                                return false;
                            }
                        } else {
                            auto svar_ctr = 0;
                            for (int k = 1; k < spec.nr_in; k++) {
                                for (int j = 0; j < k; j++) {
                                    if (!(j == q || k == q) || j == p) {
                                        svar_ctr++;
                                        continue;
                                    }
                                    const auto sel_var = get_sel_var(spec, dag, i, svar_ctr);
                                    pabc::Vec_IntSetEntry(vLits, 0, pabc::Abc_Var2Lit(sel_var, 1));
                                    auto ctr = 1;
                                    for (int ip = 0; ip < i; ip++) {
                                        const auto vertex2 = dag.get_vertex(ip);
                                        auto nr_pi_fanins2 = 0;
                                        if (vertex2[1] == FANIN_PI) {
                                            // If the second fanin is a PI, the first one 
                                            // certainly is.
                                            nr_pi_fanins2 = 2;
                                        } else if (vertex2[0] == FANIN_PI) {
                                            nr_pi_fanins2 = 1;
                                        }
                                        if (nr_pi_fanins2 == 0) {
                                            continue;
                                        } else if (nr_pi_fanins2 == 1) {
                                            const auto sel_varp = get_sel_var(spec, dag, ip, p);
                                            pabc::Vec_IntSetEntry(vLits, ctr++,
                                                pabc::Abc_Var2Lit(sel_varp, 0));
                                        } else {
                                            auto svar_ctrp = 0;
                                            for (int kp = 1; kp < spec.nr_in; kp++) {
                                                for (int jp = 0; jp < kp; jp++) {
                                                    if (jp == p || kp == p) {
                                                        const auto sel_varp = get_sel_var(spec, dag, ip, svar_ctrp);
                                                        pabc::Vec_IntSetEntry(vLits, ctr++,
                                                            pabc::Abc_Var2Lit(sel_varp, 0));
                                                    }
                                                    svar_ctrp++;
                                                }
                                            }
                                        }
                                    }
                                    if (!solver->add_clause(Vec_IntArray(vLits), Vec_IntArray(vLits) + ctr)) {
                                        return false;
                                    }
                                    svar_ctr++;
                                }
                            }
                        }
                    }
                }
            }
            return true;
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

        bool
        encode(const spec& spec, const partial_dag& dag)
        {
            create_variables(spec, dag);
            create_main_clauses(spec, dag);
            vfix_output_sim_vars(spec);

            if (!create_fanin_clauses(spec, dag)) {
                return false;
            }

            if (spec.add_nontriv_clauses && !create_nontriv_clauses(spec)) {
                return false;
            }

            if (spec.add_noreapply_clauses && !create_noreapply_clauses(spec, dag)) {
                return false;
            }

            if (spec.add_symvar_clauses && !create_symvar_clauses(spec, dag)) {
                return false;
            }

            return true;
        }

        /// Allowing multiple selection variables to be true can lead
        /// to infinite CEGAR loops. Multiple different fanin assignments
        /// may be consistent with a partial truth table, but it is
        /// possible that only one of them leads to a complete valid
        /// chain. If this assignment is never selected when a chain
        /// is extracted in the CEGAR loop, this leads to trouble.
        /// For an example, try synthesizing the 4-input function with
        /// decimal truth table 127.
        void create_cardinality_constraints(
            const spec& spec, 
            const partial_dag& dag)
        {
            std::vector<int> svars;
            std::vector<int> rvars;

            for (int i = 0; i < spec.nr_steps; i++) {
                auto nr_pi_fanins = 0;
                const auto& vertex = dag.get_vertex(i);
                if (vertex[1] == FANIN_PI) {
                    // If the second fanin is a PI, the first one 
                    // certainly is.
                    nr_pi_fanins = 2;
                } else if (vertex[0] == FANIN_PI) {
                    nr_pi_fanins = 1;
                }
                if (nr_pi_fanins == 0) {
                    continue;
                } else {
                    svars.clear();
                    rvars.clear();
                    if (nr_pi_fanins == 1) {
                        for (int j = 0; j < spec.nr_in; j++) {
                            const auto sel_var = get_sel_var(spec, dag, i, j);
                            svars.push_back(sel_var);
                        }
                    } else {
                        auto ctr = 0;
                        for (int k = 1; k < spec.nr_in; k++) {
                            for (int j = 0; j < k; j++) {
                                const auto sel_var = get_sel_var(spec, dag, i, ctr++);
                                svars.push_back(sel_var);
                            }
                        }
                    }
                    const auto nr_res_vars = (1 + 2) * (svars.size() + 1);
                    for (auto j = 0u; j < nr_res_vars; j++) {
                        rvars.push_back(get_res_var(spec, dag, i, j));
                    }
                    create_cardinality_circuit(solver, svars, rvars, 1);

                    // Ensure that the fanin cardinality for each step i 
                    // is exactly FI.
                    const auto fi_var = 
                        get_res_var(spec, dag, i, svars.size() * (1 + 2) + 1);
                    auto fi_lit = pabc::Abc_Var2Lit(fi_var, 0);
                    (void)solver->add_clause(&fi_lit, &fi_lit + 1);
                }
            }
        }

        bool
        cegar_encode(const spec& spec, const partial_dag& dag)
        {
            cegar_create_variables(spec, dag);
            /*
            for (int i = 0; i < spec.nr_rand_tt_assigns; i++) {
                const auto t = rand() % spec.get_tt_size();
                //printf("creating tt/IO clause at idx %d\n", t+1);
                vcreate_tt_clauses(spec, dag, t);
                vfix_output_sim_vars(spec, t);
            }
            */

            create_cardinality_constraints(spec, dag);

            if (!create_fanin_clauses(spec, dag)) {
                return false;
            }

            if (!create_nontriv_clauses(spec)) {
                return false;
            }
            
            if (spec.add_noreapply_clauses && !create_noreapply_clauses(spec, dag)) {
                return false;
            }
            
            if (spec.add_symvar_clauses && !create_symvar_clauses(spec, dag)) {
                return false;
            }

            return true;
        }

        void extract_chain(
            const spec& spec,
            const partial_dag& dag,
            chain& chain)
        {
            int op_inputs[2];

            chain.reset(spec.nr_in, 1, spec.nr_steps, 2);

            for (int i = 0; i < spec.nr_steps; i++) {
                kitty::dynamic_truth_table op(2);
                for (int j = 0; j < PD_OP_VARS_PER_STEP; j++) {
                    if (solver->var_value(get_op_var(i, j))) {
                        kitty::set_bit(op, j + 1);
                    }
                }

                if (spec.verbosity) {
                    printf("  step x_%d performs operation\n  ",
                        i + spec.nr_in + 1);
                    kitty::print_binary(op, std::cout);
                    printf("\n");
                }

                auto nr_pi_fanins = 0;
                const auto& vertex = dag.get_vertex(i);
                if (vertex[1] == FANIN_PI) {
                    // If the second fanin is a PI, the first one 
                    // certainly is.
                    nr_pi_fanins = 2;
                } else if (vertex[0] == FANIN_PI) {
                    nr_pi_fanins = 1;
                }
                if (nr_pi_fanins == 1) {
                    for (int j = 0; j < spec.nr_in; j++) {
                        const auto sel_var = get_sel_var(spec, dag, i, j);
                        if (solver->var_value(sel_var)) {
                            op_inputs[0] = j;
                            break;
                        }
                    }
                    op_inputs[1] = spec.nr_in + vertex[1] - 1;
                } else if (nr_pi_fanins == 2) {
                    auto ctr = 0;
                    for (int k = 1; k < spec.nr_in; k++) {
                        for (int j = 0; j < k; j++) {
                            const auto sel_var = get_sel_var(spec, dag, i, ctr++);
                            if (solver->var_value(sel_var)) {
                                op_inputs[0] = j;
                                op_inputs[1] = k;
                                break;
                            }
                        }
                    }
                } else {
                    op_inputs[0] = vertex[0] + spec.nr_in - 1;
                    op_inputs[1] = vertex[1] + spec.nr_in - 1;
                }

                chain.set_step(i, op_inputs, op);

                if (spec.verbosity) {
                    printf("\n");
                }
            }

            // TODO: support multiple outputs
            chain.set_output(0,
                ((spec.nr_steps + spec.nr_in) << 1) +
                ((spec.out_inv) & 1));
        }

        void print_solver_state(spec& spec, const partial_dag& dag)
        {
            for (auto i = 0; i < spec.nr_steps; i++) {
                auto nr_pi_fanins = 0;
                const auto& vertex = dag.get_vertex(i);
                if (vertex[1] == FANIN_PI) {
                    // If the second fanin is a PI, the first one 
                    // certainly is.
                    nr_pi_fanins = 2;
                } else if (vertex[0] == FANIN_PI) {
                    nr_pi_fanins = 1;
                }
                if (nr_pi_fanins == 1) {
                    for (int j = 0; j < spec.nr_in; j++) {
                        const auto sel_var = get_sel_var(spec, dag, i, j);
                        if (solver->var_value(sel_var)) {
                            printf("s_%d_%d=1\n", i, j);
                        } else {
                            printf("s_%d_%d=0\n", i, j);
                        }
                    }
                } else if (nr_pi_fanins == 2) {
                    auto ctr = 0;
                    for (int k = 1; k < spec.nr_in; k++) {
                        for (int j = 0; j < k; j++) {
                            const auto sel_var = get_sel_var(spec, dag, i, ctr);
                            if (solver->var_value(sel_var)) {
                                printf("s_%d_%d=1\n", i, ctr);
                            } else {
                                printf("s_%d_%d=0\n", i, ctr);
                            }
                            ctr++;
                        }
                    }
                }

                auto res_var_idx = 0;
                for (int k = 0; k < nr_svars_for_step(spec, dag, i) + 1; k++) {
                    std::string comma_str;
                    for (int i = 0; i < k; i++) {
                        comma_str += "'";
                    }
                    for (int i = 0; i < 1 + 2; i++) {
                        const auto res_var = get_res_var(spec, dag, i, res_var_idx++);
                        if (solver->var_value(res_var)) {
                            printf("res%s[%d] = 1\n", comma_str.c_str(), i);
                        } else {
                            printf("res%s[%d] = 0\n", comma_str.c_str(), i);
                        }
                    }
                }
            }
            for (auto i = 0; i < spec.nr_steps; i++) {
                printf("tt_%d_0=0\n", i);
                for (int t = 0; t < spec.get_tt_size(); t++) {
                    const auto sim_var = get_sim_var(spec, i, t);
                    if (solver->var_value(sim_var)) {
                        printf("tt_%d_%d=1\n", i, t + 1);
                    } else {
                        printf("tt_%d_%d=0\n", i, t + 1);
                    }
                }
            }
        }

        kitty::dynamic_truth_table& simulate(const spec& spec, const partial_dag& dag)
        {
            int op_inputs[2] = { 0, 0 };

            for (int i = 0; i < spec.nr_steps; i++) {
                char op = 0;
                for (int j = 0; j < PD_OP_VARS_PER_STEP; j++) {
                    if (solver->var_value(get_op_var(i, j))) {
                        op |= (1 << (j + 1));
                    }
                }

                auto nr_pi_fanins = 0;
                const auto& vertex = dag.get_vertex(i);
                if (vertex[1] == FANIN_PI) {
                    // If the second fanin is a PI, the first one 
                    // certainly is.
                    nr_pi_fanins = 2;
                } else if (vertex[0] == FANIN_PI) {
                    nr_pi_fanins = 1;
                }
                if (nr_pi_fanins == 1) {
                    for (int j = 0; j < spec.nr_in; j++) {
                        const auto sel_var = get_sel_var(spec, dag, i, j);
                        if (solver->var_value(sel_var)) {
                            op_inputs[0] = j;
                            break;
                        }
                    }
                    op_inputs[1] = spec.nr_in + vertex[1] - 1;
                } else if (nr_pi_fanins == 2) {
                    auto ctr = 0;
                    auto brk = false;
                    for (int k = 1; k < spec.nr_in && !brk; k++) {
                        for (int j = 0; j < k && !brk; j++) {
                            const auto sel_var = get_sel_var(spec, dag, i, ctr++);
                            if (solver->var_value(sel_var)) {
                                op_inputs[0] = j;
                                op_inputs[1] = k;
                                brk = true;
                            }
                        }
                    }
                } else {
                    op_inputs[0] = vertex[0] + spec.nr_in - 1;
                    op_inputs[1] = vertex[1] + spec.nr_in - 1;
                }

                auto& tt_step = sim_tts[spec.nr_in + i];
                switch (op) {
                case 2: // x1^(~x2)
                    tt_step = sim_tts[op_inputs[0]] & (~sim_tts[op_inputs[1]]);
                    break;
                case 4: // (~x1)^x2
                    tt_step = (~sim_tts[op_inputs[0]]) & sim_tts[op_inputs[1]];
                    break;
                case 6: // XOR
                    tt_step = sim_tts[op_inputs[0]] ^ sim_tts[op_inputs[1]];
                    break;
                case 8: // AND
                    tt_step = sim_tts[op_inputs[0]] & sim_tts[op_inputs[1]];
                    break;
                case 14: // OR
                    tt_step = sim_tts[op_inputs[0]] | sim_tts[op_inputs[1]];
                    break;
                default:
                    fprintf(stderr, "Error: unknown operator\n");
                    assert(false);
                    exit(1);
                }
            }

            return sim_tts[spec.nr_in + spec.nr_steps - 1];
        }
    };
}
