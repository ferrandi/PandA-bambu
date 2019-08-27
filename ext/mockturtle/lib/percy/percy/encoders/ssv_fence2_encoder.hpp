#pragma once

#include "encoder.hpp"
#include "../fence.hpp"

namespace percy
{
    class ssv_fence2_encoder : public fence_encoder
    {
    private:
        int level_dist[32]; // How many steps are below a certain level
        int nr_levels; // The number of levels in the Boolean fence
        int nr_op_vars;
        int nr_sim_vars;
        int nr_sel_vars;
        int nr_res_vars;
        int total_nr_vars;
        int sel_offset;
        int res_offset;
        int ops_offset;
        int sim_offset;

        pabc::Vec_Int_t* vLits; // Dynamic vector of literals

        static const int NR_SIM_TTS = 32;
        std::vector<kitty::dynamic_truth_table> sim_tts { NR_SIM_TTS };

        bool fix_output_sim_vars(const spec& spec, int  t)
        {
            if (spec.is_dont_care(0, t + 1)) {
                return true;
            }
            auto ilast_step = spec.nr_steps - 1;
            auto outbit = kitty::get_bit(
                spec[spec.synth_func(0)], t + 1);
            if ((spec.out_inv >> spec.synth_func(0)) & 1) {
                outbit = 1 - outbit;
            }
            const auto sim_var = get_sim_var(spec, ilast_step, t);
            pabc::lit sim_lit = pabc::Abc_Var2Lit(sim_var, 1 - outbit);
            return solver->add_clause(&sim_lit, &sim_lit + 1);
        }

        void vfix_output_sim_vars(const spec& spec, int  t)
        {
            auto ilast_step = spec.nr_steps - 1;

            auto outbit = kitty::get_bit(
                spec[spec.synth_func(0)], t + 1);
            if ((spec.out_inv >> spec.synth_func(0)) & 1) {
                outbit = 1 - outbit;
            }
            const auto sim_var = get_sim_var(spec, ilast_step, t);
            pabc::lit sim_lit = pabc::Abc_Var2Lit(sim_var, 1 - outbit);
            (void)solver->add_clause(&sim_lit, &sim_lit + 1);
        }



    public:
        const int OP_VARS_PER_STEP = 3;

        ssv_fence2_encoder(solver_wrapper& solver)
        {
            // TODO: compute better upper bound on number of literals
            vLits = pabc::Vec_IntAlloc(128);
            set_solver(solver);
        }

        ~ssv_fence2_encoder()
        {
            pabc::Vec_IntFree(vLits);
        }

        int nr_svars_for_step(const spec& spec, int i) const
        {
            // Determine the level of this step.
            const auto level = get_level(spec, i + spec.nr_in);
            auto nr_svars_for_i = 0;
            assert(level > 0);
            for (auto k = first_step_on_level(level - 1);
                k < first_step_on_level(level); k++) {
                // We select k as fanin 2, so have k options 
                // (j={0,...,(k-1)}) left for fanin 1.
                nr_svars_for_i += k;
            }

            return nr_svars_for_i;
        }

        void 
        update_level_map(const spec& spec, const fence& f)
        {
            nr_levels = f.nr_levels();
            level_dist[0] = spec.nr_in;
            for (int i = 1; i <= nr_levels; i++) {
                level_dist[i] = level_dist[i-1] + f.at(i-1);
            }
        }

        /*******************************************************************
            Returns the level (in the Boolean chain) of the specified step.
        *******************************************************************/
        int 
        get_level(const spec& spec, int step_idx) const
        {
            // PIs are considered to be on level zero.
            if (step_idx < spec.nr_in) {
                return 0;
            } else if (step_idx == spec.nr_in) { 
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

        /*******************************************************************
            Returns the index of the first step on the specified level.
        *******************************************************************/
        int 
        first_step_on_level(int level) const
        {
            if (level == 0) { return 0; }
            return level_dist[level-1];
        }

        int 
        last_step_on_level(int level) const
        {
            return first_step_on_level(level + 1) - 1;
        }

        int
        get_op_var(const spec& spec, int step_idx, int var_idx) const
        {
            assert(step_idx < spec.nr_steps);
            assert(var_idx < OP_VARS_PER_STEP);

            return ops_offset + step_idx * OP_VARS_PER_STEP + var_idx;
        }

        int get_sel_var(const spec& spec, int idx, int var_idx) const
        {
            assert(idx < spec.nr_steps);
            assert(var_idx < nr_svars_for_step(spec, idx));
            auto offset = 0;
            for (int i = 0; i < idx; i++) {
                offset += nr_svars_for_step(spec, i);
            }
            return sel_offset + offset + var_idx;
        }

        int get_res_var(const spec& spec, int step_idx, int res_var_idx) const
        {
            auto offset = 0;
            for (int i = 0; i < step_idx; i++) {
                offset += (nr_svars_for_step(spec, i) + 1) * (1 + 2);
            }

            return res_offset + offset + res_var_idx;
        }

        int get_sim_var(const spec& spec, int step_idx, int t) const
        {
            assert(step_idx < spec.nr_steps);
            assert(t < spec.get_tt_size());

            return sim_offset + spec.get_tt_size() * step_idx + t;
        }

        void create_variables(const spec& spec)
        {
            nr_op_vars = spec.nr_steps * OP_VARS_PER_STEP;
            nr_sim_vars = spec.nr_steps * spec.get_tt_size();

            // Ensure that steps are constrained to the proper level.
            nr_sel_vars = 0;
            for (int i = 0; i < spec.nr_steps; i++) {
                nr_sel_vars += nr_svars_for_step(spec, i);
            }

            sel_offset = 0;
            ops_offset = nr_sel_vars;
            sim_offset = nr_sel_vars + nr_op_vars;
            total_nr_vars = nr_sel_vars + nr_op_vars + nr_sim_vars;

            if (spec.verbosity) {
                printf("creating %d sel_vars\n", nr_sel_vars);
                printf("creating %d op_vars\n", nr_op_vars);
                printf("creating %d sim_vars\n", nr_sim_vars);
            }

            solver->set_nr_vars(total_nr_vars);
        }

        void cegar_create_variables(const spec& spec)
        {
            nr_op_vars = spec.nr_steps * OP_VARS_PER_STEP;
            nr_sim_vars = spec.nr_steps * spec.get_tt_size();

            // Ensure that steps are constrained to the proper level.
            nr_sel_vars = 0;
            nr_res_vars = 0;
            for (int i = 0; i < spec.nr_steps; i++) {
                const auto nr_svars_for_i = nr_svars_for_step(spec, i);
                nr_sel_vars += nr_svars_for_i;
                nr_res_vars += (nr_svars_for_i + 1) * (1 + 2);
            }

            sel_offset = 0;
            res_offset = nr_sel_vars;
            ops_offset = nr_sel_vars + nr_res_vars;
            sim_offset = nr_sel_vars + nr_res_vars + nr_op_vars;
            total_nr_vars = nr_sel_vars + nr_res_vars + nr_op_vars + nr_sim_vars;

            if (spec.verbosity) {
                printf("creating %d sel_vars\n", nr_sel_vars);
                printf("creating %d res_vars\n", nr_res_vars);
                printf("creating %d op_vars\n", nr_op_vars);
                printf("creating %d sim_vars\n", nr_sim_vars);
            }

            solver->set_nr_vars(total_nr_vars);
        }

        bool create_main_clauses(const spec& spec)
        {
            for (int t = 0; t < spec.get_tt_size(); t++) {
                if (!create_tt_clauses(spec, t)) {
                    return false;
                }
            }
            return true;
        }

        

        /*******************************************************************
            Add clauses that prevent trivial variable projection and
            constant operators from being synthesized.
        *******************************************************************/

        bool create_nontriv_clauses(const spec& spec)
        {
            int pLits[3];
            bool status = true;
            for (int i = 0; i < spec.nr_steps; i++) {
                // Dissallow the constant zero operator.
                pLits[0] = pabc::Abc_Var2Lit(get_op_var(spec, i, 0), 0);
                pLits[1] = pabc::Abc_Var2Lit(get_op_var(spec, i, 1), 0);
                pLits[2] = pabc::Abc_Var2Lit(get_op_var(spec, i, 2), 0);
                status &= solver->add_clause(pLits, pLits + 3);

                // Dissallow variable projections.
                pLits[0] = pabc::Abc_Var2Lit(get_op_var(spec, i, 0), 0);
                pLits[1] = pabc::Abc_Var2Lit(get_op_var(spec, i, 1), 1);
                pLits[2] = pabc::Abc_Var2Lit(get_op_var(spec, i, 2), 1);
                status &= solver->add_clause(pLits, pLits + 3);

                pLits[0] = pabc::Abc_Var2Lit(get_op_var(spec, i, 0), 1);
                pLits[1] = pabc::Abc_Var2Lit(get_op_var(spec, i, 1), 0);
                pLits[2] = pabc::Abc_Var2Lit(get_op_var(spec, i, 2), 1);
                status &= solver->add_clause(pLits, pLits + 3);
            }

            return status;
        }

        bool create_tt_clauses(const spec& spec, int t) override
        {
            auto ret = true;

            for (int i = 0; i < spec.nr_steps; i++) {
                auto level = get_level(spec, i + spec.nr_in);
                assert(level > 0);
                auto ctr = 0;
                for (int k = first_step_on_level(level - 1);
                    k < first_step_on_level(level); k++) {
                    for (int j = 0; j < k; j++) {
                        const auto sel_var = get_sel_var(spec, i, ctr++);
                        ret &= add_simulation_clause(spec, t, i, j, k, 0, 0, 1, sel_var);
                        ret &= add_simulation_clause(spec, t, i, j, k, 0, 1, 0, sel_var);
                        ret &= add_simulation_clause(spec, t, i, j, k, 0, 1, 1, sel_var);
                        ret &= add_simulation_clause(spec, t, i, j, k, 1, 0, 0, sel_var);
                        ret &= add_simulation_clause(spec, t, i, j, k, 1, 0, 1, sel_var);
                        ret &= add_simulation_clause(spec, t, i, j, k, 1, 1, 0, sel_var);
                        ret &= add_simulation_clause(spec, t, i, j, k, 1, 1, 1, sel_var);
                    }
                }
            }

            ret &= fix_output_sim_vars(spec, t);

            return ret;
        }

        void vcreate_tt_clauses(const spec& spec, int t)
        {
            for (int i = 0; i < spec.nr_steps; i++) {
                auto level = get_level(spec, i + spec.nr_in);
                assert(level > 0);
                auto ctr = 0;
                for (int k = first_step_on_level(level - 1);
                    k < first_step_on_level(level); k++) {
                    for (int j = 0; j < k; j++) {
                        const auto sel_var = get_sel_var(spec, i, ctr++);
                        add_simulation_clause(spec, t, i, j, k, 0, 0, 1, sel_var);
                        add_simulation_clause(spec, t, i, j, k, 0, 1, 0, sel_var);
                        add_simulation_clause(spec, t, i, j, k, 0, 1, 1, sel_var);
                        add_simulation_clause(spec, t, i, j, k, 1, 0, 0, sel_var);
                        add_simulation_clause(spec, t, i, j, k, 1, 0, 1, sel_var);
                        add_simulation_clause(spec, t, i, j, k, 1, 1, 0, sel_var);
                        add_simulation_clause(spec, t, i, j, k, 1, 1, 1, sel_var);
                    }
                }
            }
            fix_output_sim_vars(spec, t);
        }

        void create_cardinality_constraints(const spec& spec)
        {
            std::vector<int> svars;
            std::vector<int> rvars;

            for (int i = 0; i < spec.nr_steps; i++) {
                svars.clear();
                rvars.clear();
                const auto level = get_level(spec, spec.nr_in + i);
                auto svar_ctr = 0;
                for (int k = first_step_on_level(level - 1);
                    k < first_step_on_level(level); k++) {
                    for (int j = 0; j < k; j++) {
                        const auto sel_var = get_sel_var(spec, i, svar_ctr++);
                        svars.push_back(sel_var);
                    }
                }
                const int nr_res_vars = (1 + 2) * (svars.size() + 1);
                for (int j = 0; j < nr_res_vars; j++) {
                    rvars.push_back(get_res_var(spec, i, j));
                }
                create_cardinality_circuit(solver, svars, rvars, 1);

                // Ensure that the fanin cardinality for each step i 
                // is exactly FI.
                const auto fi_var =
                    get_res_var(spec, i, svars.size() * (1 + 2) + 1);
                auto fi_lit = pabc::Abc_Var2Lit(fi_var, 0);
                (void)solver->add_clause(&fi_lit, &fi_lit + 1);
            }
        }

        /// Ensure that each gate has 2 operands.
        bool create_fanin_clauses(const spec& spec)
        {
            bool res = true;
            for (int i = 0; i < spec.nr_steps; i++) {
                const auto nr_svars_for_i = nr_svars_for_step(spec, i);
                for (int j = 0; j < nr_svars_for_i; j++) {
                    const auto sel_var = get_sel_var(spec, i, j);
                    pabc::Vec_IntSetEntry(vLits, j, pabc::Abc_Var2Lit(sel_var, 0));
                }
                res &= solver->add_clause(pabc::Vec_IntArray(vLits), 
                        pabc::Vec_IntArray(vLits) + nr_svars_for_i);
            }
            return res;
        }

        /// Add clauses which ensure that every step is used at least once.
        void create_alonce_clauses(const spec& spec)
        {
            for (int i = 0; i < spec.nr_steps - 1; i++) {
                auto ctr = 0;
                const auto level = get_level(spec, i + spec.nr_in);
                const auto idx = spec.nr_in + i;
                for (int ip = i + 1; ip < spec.nr_steps; ip++) {
                    auto levelp = get_level(spec, ip + spec.nr_in);
                    assert(levelp >= level);
                    if (levelp == level) {
                        continue;
                    }
                    auto svctr = 0;
                    for (int k = first_step_on_level(levelp - 1);
                        k < first_step_on_level(levelp); k++) {
                        for (int j = 0; j < k; j++) {
                            if (j != idx && k != idx) {
                                svctr++;
                                continue;
                            }
                            const auto sel_var = get_sel_var(spec, ip, svctr++);
                            pabc::Vec_IntSetEntry(
                                vLits,
                                ctr++,
                                pabc::Abc_Var2Lit(sel_var, 0));
                        }
                    }
                }
                solver->add_clause(pabc::Vec_IntArray(vLits), pabc::Vec_IntArray(vLits) + ctr);
            }
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
                    get_op_var(spec, i, ((c << 1) | b) - 1), 1 - a);
            }

            return solver->add_clause(pLits, pLits + ctr);
        }

        void create_noreapply_clauses(const spec& spec)
        {
            int pLits[3];

            // Disallow:
            // x_i : (j, k)
            // x_ip: (j, i) OR (k, i)
            for (int i = 0; i < spec.nr_steps; i++) {
                const auto idx = spec.nr_in + i;
                const auto level = get_level(spec, idx);

                for (int ip = i+1; ip < spec.nr_steps; ip++) {
                    const auto levelp = get_level(spec, ip +
                            spec.nr_in);
                    if (levelp == level) { 
                        // A node cannot have a node on the same level in
                        // its fanin.
                        continue;
                    }

                    auto svar_ctr = 0;
                    for (int k = first_step_on_level(level-1); 
                        k < first_step_on_level(level); k++) {
                        for (int j = 0; j < k; j++) {
                            const auto sel_var = get_sel_var(spec, i, svar_ctr++);
                            pLits[0] = pabc::Abc_Var2Lit(sel_var, 1);

                            // Note that it's possible for node ip to never have
                            // i as its second fanin.
                            auto svar_ctrp = 0;
                            for (int kp = first_step_on_level(levelp-1); 
                                    kp < first_step_on_level(levelp); kp++) {
                                for (int jp = 0; jp < kp; jp++) {
                                    if (kp == idx && (jp == j || jp == k)) {
                                        const auto sel_varp = get_sel_var(spec, ip, svar_ctrp);
                                        pLits[1] = pabc::Abc_Var2Lit(sel_varp, 1);
                                        auto status = solver->add_clause(pLits, pLits + 2);
                                        assert(status);
                                    }
                                    svar_ctrp++;
                                }
                            }
                        }
                    }
                }
            }

            // Disallow:
            // x_i  : (j, k)
            // x_ip : (j, k)
            // x_ipp: (i,ip)
            for (int i = 0; i < spec.nr_steps; i++) {
                const auto level = get_level(spec, spec.nr_in + i);

                for (int ip = i + 1; ip < spec.nr_steps; ip++) {
                    const auto levelp = get_level(spec, ip + spec.nr_in);
                    if (levelp != level) {
                        // If they are not on the same level they 
                        // cannot have the same fanin
                        continue;
                    }

                    for (int ipp = ip + 1; ipp < spec.nr_steps; ipp++) {
                        const auto levelpp = get_level(spec, ipp + spec.nr_in);
                        if (levelpp == level) {
                            continue;
                        }

                        auto svar_ctr = 0;
                        for (int k = first_step_on_level(level - 1);
                            k < first_step_on_level(level); k++) {
                            for (int j = 0; j < k; j++) {
                                auto svar_ctrpp = 0;
                                for (int kpp = first_step_on_level(levelpp - 1);
                                    kpp < first_step_on_level(levelpp); kpp++) {
                                    for (int jpp = 0; jpp < kpp; jpp++) {
                                        if ((jpp == spec.nr_in + i) && (kpp == spec.nr_in + ip)) {
                                            const auto sel_var = get_sel_var(spec, i, svar_ctr);
                                            const auto sel_varp = get_sel_var(spec, ip, svar_ctr);
                                            const auto sel_varpp = get_sel_var(spec, ipp, svar_ctrpp);
                                            pLits[0] = pabc::Abc_Var2Lit(sel_var, 1);
                                            pLits[1] = pabc::Abc_Var2Lit(sel_varp, 1);
                                            pLits[2] = pabc::Abc_Var2Lit(sel_varpp, 1);
                                            (void)solver->add_clause(pLits, pLits + 3);
                                        }
                                        svar_ctrpp++;
                                    }
                                }
                                svar_ctr++;
                            }
                        }
                    }
                }
            }
        }

        /// Extracts chain from encoded CNF solution.
        void extract_chain(const spec& spec, chain& chain) override
        {
            chain.reset(spec.nr_in, 1, spec.nr_steps, 2);

            kitty::dynamic_truth_table op(2);
            for (int i = 0; i < spec.nr_steps; i++) {
                for (int j = 0; j < OP_VARS_PER_STEP; j++) {
                    if (solver->var_value(get_op_var(spec, i, j)))
                        kitty::set_bit(op, j + 1); 
                    else
                        kitty::clear_bit(op, j + 1); 
                }

                if (spec.verbosity) {
                    printf("  step x_%d performs operation\n  ", 
                            i+spec.nr_in+1);
                    kitty::print_binary(op, std::cout);
                    printf("\n");
                }

                auto ctr = 0;
                const auto level = get_level(spec, spec.nr_in + i);
                for (int k = first_step_on_level(level - 1);
                    k < first_step_on_level(level); k++) {
                    for (int j = 0; j < k; j++) {
                        const auto sel_var = get_sel_var(spec, i, ctr++);
                        if (solver->var_value(sel_var)) {
                            if (spec.verbosity) {
                                printf("  with operands ");
                                printf("x_%d ", j + 1);
                                printf("x_%d ", k + 1);
                            }
                            chain.set_step(i, j, k, op);
                            break;
                        }
                    }
                }

                if (spec.verbosity) {
                    printf("\n");
                }
            }

            // TODO: support multiple outputs
            chain.set_output(0,
                ((spec.nr_steps + spec.nr_in) << 1) +
                ((spec.out_inv) & 1));
        }

        void create_colex_clauses(const spec& spec)
        {
            int pLits[2];

            for (int i = 0; i < spec.nr_steps - 1; i++) {
                const auto level = get_level(spec, i + spec.nr_in);
                const auto levelp = get_level(spec, i + 1 + spec.nr_in);
                int svar_ctr = 0;
                for (int k = first_step_on_level(level-1); 
                        k < first_step_on_level(level); k++) {
                    for (int j = 0; j < k; j++) {
                        if (k < 2) {
                            svar_ctr++;
                            continue;
                        }
                        const auto sel_var = get_sel_var(spec, i, svar_ctr);
                        pLits[0] = pabc::Abc_Var2Lit(sel_var, 1);
                        int svar_ctrp = 0;
                        for (int kp = first_step_on_level(levelp - 1);
                            kp < first_step_on_level(levelp); kp++) {
                            for (int jp = 0; jp < kp; jp++) {
                                if ((kp == k && jp < j) || (kp < k)) {
                                    const auto sel_varp = get_sel_var(spec, i + 1, svar_ctrp);
                                    pLits[1] = pabc::Abc_Var2Lit(sel_varp, 1);
                                    (void)solver->add_clause(pLits, pLits + 2);
                                }
                                svar_ctrp++;
                            }
                        }
                        svar_ctr++;
                    }
                }
            }
        }

/*
        void 
        create_colex_func_clauses(const spec& spec)
        {
            for (int i = 0; i < spec.nr_steps-1; i++) {
                for (int k = 1; k < spec.nr_in+i; k++) {
                    for (int j = 0; j < k; j++) {
                        pLits[0] = pabc::Abc_Var2Lit(spec.selection_vars[i][j][k], 1);
                        pLits[1] = pabc::Abc_Var2Lit(spec.selection_vars[i+1][j][k], 1);

                        pLits[2] = 
                            pabc::Abc_Var2Lit(get_op_var(spec, i, 1, 1), 1);
                        pLits[3] = 
                            pabc::Abc_Var2Lit(get_op_var(spec, i+1, 1, 1), 0);
                        solver_add_clause(this->solver, pLits, pLits+4);

                        pLits[3] = 
                            pabc::Abc_Var2Lit(get_op_var(spec, i, 1, 0), 1);
                        pLits[4] = 
                            pabc::Abc_Var2Lit(get_op_var(spec, i+1, 1, 1), 0);
                        solver_add_clause(this->solver, pLits, pLits+5);
                        pLits[4] = 
                            pabc::Abc_Var2Lit(get_op_var(spec, i+1, 1, 0), 0);
                        solver_add_clause(this->solver, pLits, pLits+5);

                        pLits[4] = 
                            pabc::Abc_Var2Lit(get_op_var(spec, i, 0, 1), 1);
                        pLits[5] = 
                            pabc::Abc_Var2Lit(get_op_var(spec, i+1, 1, 1), 0);
                        solver_add_clause(this->solver, pLits, pLits+6);
                        pLits[5] = 
                            pabc::Abc_Var2Lit(get_op_var(spec, i+1, 1, 0), 0);
                        solver_add_clause(this->solver, pLits, pLits+6);
                        pLits[5] = 
                            pabc::Abc_Var2Lit(get_op_var(spec, i+1, 0, 1), 0);
                        solver_add_clause(this->solver, pLits, pLits+6);
                    }
                }
            }
        }
        */

        void create_symvar_clauses(const spec& spec)
        {
            for (int q = 1; q < spec.nr_in; q++) {
                for (int p = 0; p < q; p++) {
                    auto symm = true;
                    for (int i = 0; i < spec.nr_nontriv; i++) {
                        auto& f = spec[spec.synth_func(i)];
                        if (!(swap(f, p, q) == f)) {
                            symm = false;
                            break;
                        }
                    }
                    if (!symm) {
                        continue;
                    }
                    for (int i = 1; i < spec.nr_steps; i++) {
                        const auto level = get_level(spec, i + spec.nr_in);
                        int svar_ctr = 0;
                        for (int k = first_step_on_level(level - 1);
                            k < first_step_on_level(level); k++) {
                            for (int j = 0; j < k; j++) {
                                if (!(j == q || k == q) || j == p) {
                                    svar_ctr++;
                                    continue;
                                }
                                const auto sel_var = get_sel_var(spec, i, svar_ctr);
                                pabc::Vec_IntSetEntry(vLits, 0, pabc::Abc_Var2Lit(sel_var, 1));
                                auto ctr = 1;
                                for (int ip = 0; ip < i; ip++) {
                                    const auto levelp = get_level(spec, spec.nr_in + ip);
                                    auto svar_ctrp = 0;
                                    for (int kp = first_step_on_level(levelp - 1);
                                        kp < first_step_on_level(levelp); kp++) {
                                        for (int jp = 0; jp < kp; jp++) {
                                            if (jp == p || kp == p) {
                                                const auto sel_varp = get_sel_var(spec, ip, svar_ctrp);
                                                pabc::Vec_IntSetEntry(vLits, ctr++,
                                                    pabc::Abc_Var2Lit(sel_varp, 0));
                                            }
                                            svar_ctrp++;
                                        }
                                    }
                                }
                                (void)solver->add_clause(Vec_IntArray(vLits), Vec_IntArray(vLits) + ctr);
                                svar_ctr++;
                            }
                        }
                    }
                }
            }
        }

        /// Encodes specifciation for use in fence-based synthesis flow.
        bool encode(const spec& spec, const fence& f) override
        {
            assert(spec.nr_steps == f.nr_nodes());

            bool success = true;

            update_level_map(spec, f);
            create_variables(spec);
            success = create_main_clauses(spec);
            if (!success) {
                return false;
            }

            if (!create_fanin_clauses(spec)) {
                return false;
            }

            if (spec.add_nontriv_clauses) {
                create_nontriv_clauses(spec);
            }

            if (spec.add_alonce_clauses) {
                create_alonce_clauses(spec);
            }
            if (spec.add_noreapply_clauses) {
                create_noreapply_clauses(spec);
            }
            if (spec.add_colex_clauses) {
                create_colex_clauses(spec);
            }
            /*
            if (spec.add_colex_func_clauses) {
                create_colex_func_clauses(spec);
            }
*/
            if (spec.add_symvar_clauses) {
                create_symvar_clauses(spec);
            }

            return true;
        }

        /// Encodes specifciation for use in CEGAR based synthesis flow.
        bool cegar_encode(const spec& spec, const fence& f) override
        {
            assert(spec.nr_steps == f.nr_nodes());

            update_level_map(spec, f);
            cegar_create_variables(spec);
            
            if (!create_fanin_clauses(spec)) {
                return false;
            }
            create_cardinality_constraints(spec);

            if (spec.add_nontriv_clauses) {
                create_nontriv_clauses(spec);
            }
            if (spec.add_alonce_clauses) {
                create_alonce_clauses(spec);
            }
            if (spec.add_noreapply_clauses) {
                create_noreapply_clauses(spec);
            }
            if (spec.add_colex_clauses) {
                create_colex_clauses(spec);
            }
            /*
            if (spec.add_colex_func_clauses) {
                create_colex_func_clauses(spec);
            }
            */
            if (spec.add_symvar_clauses) {
                create_symvar_clauses(spec);
            }

            return true;
        }

        /// Assumes that a solution has been found by the current encoding.
        /// Blocks the current solution such that the solver is forced to
        /// find different ones (if they exist).
        /*
        bool block_solution(const spec& spec)
        {
            // TODO: implement!
            return false;
        }


        /// Similar to block_solution, but blocks all solutions with the
        /// same structure. This is more restrictive, since the other
        /// method allows for the same structure but different operators.
        bool block_struct_solution(const spec& spec)
        {
            // TODO: implement!
            return false;
        }
        */

        kitty::dynamic_truth_table& simulate(const spec& spec) override
        {
            int op_inputs[2] = { 0, 0 };

            for (int i = 0; i < spec.nr_steps; i++) {
                char op = 0;
                if (solver->var_value(get_op_var(spec, i, 0))) {
                    op |= (1 << 1);
                }
                if (solver->var_value(get_op_var(spec, i, 1))) {
                    op |= (1 << 2);
                }
                if (solver->var_value(get_op_var(spec, i, 2))) {
                    op |= (1 << 3);
                }

                auto ctr = 0;
                auto brk = false;
                const auto level = get_level(spec, spec.nr_in + i);
                for (int k = first_step_on_level(level - 1); 
                    k < first_step_on_level(level) && !brk; k++) {
                    for (int j = 0; j < k && !brk; j++) {
                        const auto sel_var = get_sel_var(spec, i, ctr++);
                        if (solver->var_value(sel_var)) {
                            op_inputs[0] = j;
                            op_inputs[1] = k;
                            brk = true;
                        }
                    }
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

        void reset_sim_tts(int nr_in) override 
        {
            for (int i = 0; i < NR_SIM_TTS; i++) {
                sim_tts[i] = kitty::dynamic_truth_table(nr_in);
                if (i < nr_in) {
                    kitty::create_nth_var(sim_tts[i], i);
                }
            }
        }
    };
}

