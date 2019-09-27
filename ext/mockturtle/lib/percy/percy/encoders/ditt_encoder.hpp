#pragma once

#include "encoder.hpp"
#include "../misc.hpp"

namespace percy
{
    class ditt_encoder : public std_cegar_encoder, public enumerating_encoder
    {
        private:
			int nr_op_vars_per_step;
			int nr_op_vars;
			int nr_out_vars;
			int nr_tt_vars;
			int nr_input_tt_vars;
			int nr_lex_vars;
			int nr_sel_vars;
            int sel_offset;
            int ops_offset;
            int out_offset;
            int tt_offset;
            int input_tt_offset;
            int lex_offset;
            int total_nr_vars;
            
            pabc::Vec_Int_t* vLits; // Dynamic vector of literals

        public:
            ditt_encoder(solver_wrapper& solver)
            {
                vLits = pabc::Vec_IntAlloc(128);
                set_solver(solver);
            }

            ~ditt_encoder()
            {
                pabc::Vec_IntFree(vLits);
            }

            int
            get_op_var(const spec& spec, int step_idx, int var_idx)
            const 
            {
                assert(step_idx < spec.nr_steps);
                assert(var_idx > 0);
                assert(var_idx <= nr_op_vars_per_step);

                return ops_offset + step_idx * nr_op_vars_per_step + var_idx-1;
            }

            int
            get_sel_var(const spec& spec, int i, int j, int k) 
            const
            {
                assert(i < spec.nr_steps);
                assert(j < spec.get_nr_in() + i);
                assert(k < spec.fanin);

                auto prev_svars = 0;
                for (int ip = 0; ip < i; ip++) {
                    prev_svars += spec.fanin * (spec.get_nr_in() + ip);
                }

                return sel_offset + prev_svars + k * (spec.get_nr_in() + i) + j;
            }

            int 
            get_out_var(const spec& spec, int h, int i) 
            const
            {
                assert(h < spec.nr_nontriv);
                assert(i < spec.nr_steps);

                return out_offset + spec.nr_steps * h + i;
            }

            int
            get_tt_var(const spec& spec, int i, int t) const
            {
                assert(i < spec.nr_steps);
                assert(t < spec.get_tt_size());

                return tt_offset + spec.get_tt_size() * i + t;
            }

            int
            get_input_tt_var(const spec& spec, int i, int t, int k)
            const
            {
                assert(k < spec.fanin);
                assert(t < spec.get_tt_size());

                auto prev_itt_vars = 0;
                for (int ip = 0; ip < i; ip++) {
                    prev_itt_vars += spec.fanin * spec.get_tt_size();
                }

                return input_tt_offset + prev_itt_vars + 
                    k * spec.get_tt_size() + t;
            }

            int
            get_lex_var(const spec& spec, int step_idx, int op_idx) const
            {
                assert(step_idx < spec.nr_steps);
                assert(op_idx < nr_op_vars_per_step);

                return lex_offset + step_idx * (nr_op_vars_per_step - 1) + op_idx;
            }

            /// Ensures that each gate has spec.fanin operands AND that
            /// fanins are ordered tuples.
            bool 
            create_op_clauses(const spec& spec)
            {
                int pLits[2];
                auto status = true;

                if (spec.verbosity > 2) {
                    printf("Creating op clauses (DITT-%d)\n", spec.fanin);
                    printf("Nr. clauses = %d (PRE)\n", solver->nr_clauses());
                }

                for (int i = 0; i < spec.nr_steps; i++) {
                    for (int k = 0; k < spec.fanin; k++) {
                        int ctr = 0;
                        const auto max_fanin_idx = spec.get_nr_in() + i - spec.fanin + k;
                        for (int j = k; j <= max_fanin_idx; j++) {
                            const auto s_ij_k = get_sel_var(spec, i, j, k);
                            pabc::Vec_IntSetEntry(vLits, ctr++,
                                pabc::Abc_Var2Lit(s_ij_k, 0));
                        }
                        status &= solver->add_clause(
                            pabc::Vec_IntArray(vLits),
                            pabc::Vec_IntArray(vLits) + ctr
                        );
                    }
                }
                for (int i = 0; i < spec.nr_steps; i++) {
                    for (int k = 0; k < spec.fanin - 1; k++) {
                        const auto max_fanin_idx = spec.get_nr_in() + i - spec.fanin + k;
                        for (int j = k; j <= max_fanin_idx; j++) {
                            const auto s_ij_k = get_sel_var(spec, i, j, k);
                            pLits[0] = pabc::Abc_Var2Lit(s_ij_k, 1);
                            for (int kp = k + 1; kp < spec.fanin; kp++) {
                                for (int jp = 0; jp <= j; jp++) {
                                    const auto s_ijp_kp = get_sel_var(spec, i, jp, kp);
                                    pLits[1] = pabc::Abc_Var2Lit(s_ijp_kp, 1);
                                    status &= solver->add_clause(pLits, pLits + 2);
                                    assert(status);
                                }
                            }
                        }
                    }
                }
                if (spec.verbosity > 2) {
                    printf("Nr. clauses = %d (POST)\n", solver->nr_clauses());
                }

                return status;
            }

            bool 
            create_output_clauses(const spec& spec)
            {
                auto status = true;

                if (spec.verbosity > 2) {
                    printf("Creating output clauses (DITT-%d)\n", spec.fanin);
                    printf("Nr. clauses = %d (PRE)\n", solver->nr_clauses());
                }
                // Every output points to an operand.
                if (spec.nr_nontriv > 1) {
                    for (int h = 0; h < spec.nr_nontriv; h++) {
                        for (int i = 0; i < spec.nr_steps; i++) {
                            pabc::Vec_IntSetEntry(vLits, i, 
                                    pabc::Abc_Var2Lit(get_out_var(spec, h, i), 0));
                        }
                        status &= solver->add_clause(
                                pabc::Vec_IntArray(vLits),
                                pabc::Vec_IntArray(vLits) + spec.nr_steps);

                        if (spec.verbosity > 2) {
                            printf("creating output clause: ( ");
                            for (int i = 0; i < spec.nr_steps; i++) {
                                printf("%sg_%d_%d ", i > 0 ? "\\/ " : "",
                                        h + 1, spec.get_nr_in() + i + 1);
                            }
                            printf(") (status = %d)\n", status);
                        }
                    }
                }

                // At least one of the outputs has to refer to the final
                // operator, otherwise it may as well not be there.
                const auto last_op = spec.nr_steps - 1;
                for (int h = 0; h < spec.nr_nontriv; h++) {
                    pabc::Vec_IntSetEntry(vLits, h,
                            pabc::Abc_Var2Lit(get_out_var(spec, h, last_op),0));
                }
                status &= solver->add_clause(
                    pabc::Vec_IntArray(vLits),
                    pabc::Vec_IntArray(vLits) + spec.nr_nontriv);

                if (spec.verbosity > 2) {
                    printf("creating output clause: ( ");
                    for (int h = 0; h < spec.nr_nontriv; h++) {
                        printf("%sg_%d_%d ", h > 0 ? "\\/ " : "",
                                h + 1, spec.get_nr_in() + last_op + 1);
                    }
                    printf(") (status = %d)\n", status);
                    printf("Nr. clauses = %d (POST)\n", solver->nr_clauses());
                }

                return status;
            }

            void
            create_variables(const spec& spec)
            {
                nr_op_vars_per_step = ((1u << spec.fanin) - 1);
                nr_op_vars = spec.nr_steps * nr_op_vars_per_step;
                nr_out_vars = spec.nr_nontriv * spec.nr_steps;
                nr_tt_vars = spec.nr_steps * spec.get_tt_size();
                nr_input_tt_vars = spec.fanin * spec.nr_steps * spec.get_tt_size();
                nr_lex_vars = (spec.nr_steps - 1) * (nr_op_vars_per_step - 1);
                nr_sel_vars = 0;
                for (int i = 0; i < spec.nr_steps; i++) {
                    const auto nr_svars_for_i = spec.fanin * (spec.get_nr_in() + i);
                    nr_sel_vars += nr_svars_for_i;
                }
                sel_offset = 0;
                ops_offset = nr_sel_vars;
                out_offset = nr_sel_vars + nr_op_vars;
                tt_offset = nr_sel_vars + nr_op_vars + nr_out_vars;
                input_tt_offset = nr_sel_vars + nr_op_vars + nr_out_vars + nr_tt_vars;
                lex_offset = nr_sel_vars + nr_op_vars + nr_out_vars + nr_tt_vars + nr_input_tt_vars;
                
                total_nr_vars = nr_op_vars + nr_out_vars + nr_tt_vars +
                    nr_input_tt_vars + nr_sel_vars + nr_lex_vars;

                if (spec.verbosity > 2) {
                    printf("Creating variables (DITT-%d)\n", spec.fanin);
                    printf("nr steps = %d\n", spec.nr_steps);
                    printf("nr_sel_vars=%d\n", nr_sel_vars);
                    printf("nr_op_vars = %d\n", nr_op_vars);
                    printf("nr_out_vars = %d\n", nr_out_vars);
                    printf("nr_tt_vars = %d\n", nr_tt_vars);
                    printf("nr_input_tt_vars = %d\n", nr_input_tt_vars);
                    printf("nr_lex_vars = %d\n", nr_lex_vars);
                    printf("creating %d total variables\n", total_nr_vars);
                }

                solver->set_nr_vars(total_nr_vars);
            }

            bool
            add_simulation_clause(
                    const spec& spec, 
                    const int t, 
                    const int i, 
                    const int output, 
                    const int opvar_idx,
                    const std::vector<int>& fanin_asgn)
            {
                pabc::Vec_IntSetEntry(vLits, 0,
                    pabc::Abc_Var2Lit(get_tt_var(spec, i, t), output));
                
                int ctr = 1;
                if (spec.verbosity > 3) {
                    //printf("assignment: %s\n", fanin_asgn.to_string().c_str());
                }
                for (int k = 0; k < spec.fanin; k++) {
                    const auto x_it_k = get_input_tt_var(spec, i, t, k);
                    pabc::Vec_IntSetEntry(vLits, ctr++,
                        pabc::Abc_Var2Lit(x_it_k, fanin_asgn[k]));
                }

                //printf("opvar_idx=%d\n", opvar_idx);
                if (opvar_idx > 0) {
                    pabc::Vec_IntSetEntry(vLits, ctr++, pabc::Abc_Var2Lit(
                                get_op_var(spec, i, opvar_idx), 1 - output));
                }

                const auto status = solver->add_clause(
                        pabc::Vec_IntArray(vLits),
                        pabc::Vec_IntArray(vLits) + ctr); 

                return status;
            }

            bool 
            create_tt_clauses(const spec& spec, const int t)
            {
                auto ret = true;
                int pLits[3];
                std::vector<int> fanin_asign(spec.fanin);

                for (int i = 0; i < spec.nr_steps; i++) {
                    // Encode propagation of truth tables:
                    // if s_ij^k, then x_it^k = x_jt
                    for (int k = 0; k < spec.fanin; k++) {
                        for (int j = 0; j < spec.get_nr_in() + i; j++) {
                            const auto svar = get_sel_var(spec, i, j, k);
                            const auto x_it_k = get_input_tt_var(spec, i, t, k);
                            if (j < spec.get_nr_in()) {
                                // In this case, we know the value of
                                // x_jt a-priori
                                const auto val = ((t + 1) & (1 << j)) ? 1 : 0;
                                pLits[0] = pabc::Abc_Var2Lit(svar, 1);
                                pLits[1] = pabc::Abc_Var2Lit(x_it_k, 1 - val);
                                ret &= solver->add_clause(pLits, pLits + 2);
                            } else {
                                const auto x_jt = get_tt_var(spec, j - spec.get_nr_in(), t);

                                pLits[0] = pabc::Abc_Var2Lit(svar, 1);
                                pLits[1] = pabc::Abc_Var2Lit(x_it_k, 1);
                                pLits[2] = pabc::Abc_Var2Lit(x_jt, 0);
                                ret &= solver->add_clause(pLits, pLits + 3);

                                pLits[1] = pabc::Abc_Var2Lit(x_it_k, 0);
                                pLits[2] = pabc::Abc_Var2Lit(x_jt, 1);
                                ret &= solver->add_clause(pLits, pLits + 3);
                            }
                        }
                    }

                    // Fix the input truth tables if this step has a PI 
                    // as fanin
                    for (int k = 0; k < spec.fanin; k++) {
                        for (int j = 0; j < spec.get_nr_in(); j++) {
                            const auto s_ij_k = get_sel_var(spec, i, j, k);
                            const auto x_it_k = get_input_tt_var(spec, i, t, k);
                            const auto val = ((t + 1) & (1 << j)) ? 1 : 0;
                            pLits[0] = pabc::Abc_Var2Lit(s_ij_k, 1);
                            pLits[1] = pabc::Abc_Var2Lit(x_it_k, 1 - val);
                            ret &= solver->add_clause(pLits, pLits + 2);
                        }
                    }

                    // Encode operator/truth table constraints
                    // First add clauses for all cases where step i
                    // computes zero.
                    int opvar_idx = 0;
                    clear_assignment(fanin_asign);
                    while (true) {
                        next_assignment(fanin_asign);
                        if (is_zero(fanin_asign)) {
                            break;
                        }
                        opvar_idx++;
                        ret &= add_simulation_clause(spec, t, i, 0, 
                            opvar_idx, fanin_asign);
                    }

                    // Next, all the cases where it computes one.
                    opvar_idx = 0;
                    ret &= add_simulation_clause(spec, t, i, 1,
                                opvar_idx, fanin_asign);
                    while (true) {
                        next_assignment(fanin_asign);
                        if (is_zero(fanin_asign)) {
                            break;
                        }
                        opvar_idx++;
                        ret &= add_simulation_clause(spec, t, i, 1,
                            opvar_idx, fanin_asign);
                    }
                    
                    // If an output has selected this particular operand, we
                    // need to ensure that this operand's truth table satisfies
                    // the specified output function.
                    for (int h = 0; h < spec.nr_nontriv; h++) {
                        if (spec.is_dont_care(h, t + 1)) {
                            continue;
                        }
                        auto outbit = kitty::get_bit(
                                spec[spec.synth_func(h)], t+1);
                        if ((spec.out_inv >> spec.synth_func(h)) & 1) {
                            outbit = 1 - outbit;
                        }
                        pLits[0] = pabc::Abc_Var2Lit(get_out_var(spec, h, i), 1);
                        pLits[1] = pabc::Abc_Var2Lit(get_tt_var(spec, i, t), 
                                1 - outbit);
                        ret &= solver->add_clause(pLits, pLits+2);
                        if (spec.verbosity > 2) {
                            printf("creating oimp clause: ( ");
                            printf("!g_%d_%d \\/ %sx_%d_%d ) (status=%d)\n", 
                                    h + 1, 
                                    spec.get_nr_in() + i + 1, 
                                    (1 - outbit) ?  "!" : "",
                                    spec.get_nr_in() + i + 1, 
                                    t + 2,
                                    ret);
                        }
                    }
                }

                return ret;
            }

            bool 
            create_main_clauses(const spec& spec)
            {
                if (spec.verbosity > 2) {
                    printf("Creating main clauses (DITT-%d)\n", spec.fanin);
                    printf("Nr. clauses = %d (PRE)\n", solver->nr_clauses());
                }
                auto success = true;

                for (int t = 0; t < spec.get_tt_size(); t++) {
                    success &= create_tt_clauses(spec, t);
                }

                if (spec.verbosity > 2) {
                    printf("Nr. clauses = %d (POST)\n", solver->nr_clauses());
                    printf("success = %d\n", success);
                }

                return success;
            }

            /*******************************************************************
                Add clauses that prevent trivial variable projection and
                constant operators from being synthesized.
            *******************************************************************/
            void 
            create_nontriv_clauses(const spec& spec)
            {
                dynamic_truth_table triv_op(spec.fanin);

                for (int i = 0; i < spec.nr_steps; i++) {
                    kitty::clear(triv_op);
                    
                    // Dissallow the constant zero operator.
                    for (int j = 1; j <= nr_op_vars_per_step; j++) {
                        pabc::Vec_IntSetEntry(vLits, j-1,
                                pabc::Abc_Var2Lit(get_op_var(spec, i, j), 0));
                    }
                    auto status = solver->add_clause(
                            pabc::Vec_IntArray(vLits), 
                            pabc::Vec_IntArray(vLits) + nr_op_vars_per_step);
                    assert(status);
                    
                    // Dissallow all variable projection operators.
                    for (int n = 0; n < spec.fanin; n++) {
                        kitty::create_nth_var(triv_op, n);
                        for (int j = 1; j <= nr_op_vars_per_step; j++) {
                            pabc::Vec_IntSetEntry(vLits, j-1,
                                    pabc::Abc_Var2Lit(get_op_var(spec, i, j), 
                                        kitty::get_bit(triv_op, j)));
                        }
                        status = solver->add_clause(
                                pabc::Vec_IntArray(vLits),
                                pabc::Vec_IntArray(vLits) + nr_op_vars_per_step);
                        assert(status);
                    }
                }
            }

            /*******************************************************************
              Add clauses which ensure that every step is used at least once.
            *******************************************************************/
            bool 
            create_alonce_clauses(const spec& spec)
            {
                bool status = true;

                for (int i = 0; i < spec.nr_steps; i++) {
                    auto ctr = 0;

                    // Either one of the outputs points to this step.
                    for (int h = 0; h < spec.nr_nontriv; h++) {
                        pabc::Vec_IntSetEntry(vLits, ctr++, 
                                pabc::Abc_Var2Lit(get_out_var(spec, h, i), 0));
                    }

                    // Or one of the succeeding steps points to this step.
                    for (int ip = i + 1; ip < spec.nr_steps; ip++) {
                        for (int k = 0; k < spec.fanin; k++) {
                            const auto sel_var = get_sel_var(spec, ip, spec.get_nr_in() + i, k);
                            pabc::Vec_IntSetEntry(
                                vLits, 
                                ctr++,
                                pabc::Abc_Var2Lit(sel_var, 0));
                        }
                    }
                        
                    status &= solver->add_clause(
                            pabc::Vec_IntArray(vLits),
                            pabc::Vec_IntArray(vLits) + ctr);
                }

                return status;
            }

            /*******************************************************************
                Add clauses which ensure that operands are never re-applied. In
                other words, (Sijk --> ~Si'ji) & (Sijk --> ~Si'ki), 
                for all (i < i').
            *******************************************************************/
            void 
            create_noreapply_clauses(const spec& spec)
            {
                std::vector<int> remaining_fanins;
                std::vector<int> fanins(spec.fanin);
                for (int i = 0; i < spec.nr_steps - 1; i++) {
                    const auto max_fanin = spec.get_nr_in() + i - 1;
                    clear_assignment(fanins);
                    while (true) {
                        for (int k = 0; k < spec.fanin; k++) {
                            const auto s_ij_k = get_sel_var(spec, i, fanins[k], k);
                            pabc::Vec_IntSetEntry(vLits, k, pabc::Abc_Var2Lit(s_ij_k, 1));
                        }

                        for (int ip = i + 1; ip < spec.nr_steps; ip++) {
                            remaining_fanins.clear();
                            for (int l = 0; l <= spec.get_nr_in() + ip - 1; l++) {
                                if (l == spec.get_nr_in() + i) continue;
                                if (std::find(fanins.begin(), fanins.end(), l) == fanins.end()) {
                                    remaining_fanins.push_back(l);
                                }
                            }
                            // (s_i(j1)_1 /\ ... /\ (s_i(jm)_k) --> (s_(ip)i_1) --> (not one of the other fanins)
                            // the other fanins be subsumed by the current
                            // ifanin assignment to i.
                            for (int k = 0; k < spec.fanin; k++) {
                                auto ctr = spec.fanin;
                                const auto s_ip_i_k = get_sel_var(spec, ip,
                                    spec.get_nr_in() + i, k);
                                pabc::Vec_IntSetEntry(vLits, ctr++, 
                                    pabc::Abc_Var2Lit(s_ip_i_k, 1));

                                for (int kp = 0; kp < spec.fanin; kp++) {
                                    if (kp == k) continue;
                                    for (const auto fanin : remaining_fanins) {
                                        const auto s_ip_f_kp = get_sel_var(spec, ip, fanin, kp);
                                        pabc::Vec_IntSetEntry(vLits, ctr++,
                                            pabc::Abc_Var2Lit(s_ip_f_kp, 0));
                                    }
                                }
                                const auto status = solver->add_clause(
                                    pabc::Vec_IntArray(vLits),
                                    pabc::Vec_IntArray(vLits) + ctr
                                );
                                assert(status);
                            }

                            
                        }

                        inc_assignment(fanins, max_fanin);
                        if (is_zero(fanins)) {
                            break;
                        }
                    }
                }
            }

            /*******************************************************************
                Add clauses which ensure that steps occur in co-lexicographical
                order. In other words, we require steps operands to be 
                co-lexicographically ordered tuples.
            *******************************************************************/
            bool 
            create_colex_clauses(const spec& spec)
            {
                bool status = true;
                std::vector<int> fanins(spec.fanin);
                std::vector<int> ip_fanins(spec.fanin);

                for (int i = 0; i < spec.nr_steps - 1; i++) {
                    clear_assignment(fanins);
                    const auto max_fanin = spec.get_nr_in() + i - 1;
                    while (true) {
                        clear_assignment(ip_fanins);
                        const auto max_ip_fanin = max_fanin + 1;
                        while (true) {
                            if (colex_compare(fanins, ip_fanins) < 1) {
                                inc_assignment(ip_fanins, max_ip_fanin);
                                if (is_zero(ip_fanins)) {
                                    break;
                                } else {
                                    continue;
                                }
                            }
                            auto ctr = 0;
                            for (int k = 0; k < spec.fanin; k++) {
                                pabc::Vec_IntSetEntry(vLits, ctr++,
                                    pabc::Abc_Var2Lit(get_sel_var(spec, i, fanins[k], k), 1));
                                pabc::Vec_IntSetEntry(vLits, ctr++,
                                    pabc::Abc_Var2Lit(get_sel_var(spec, i + 1, ip_fanins[k], k), 1));
                            }

                            status &= solver->add_clause(
                                pabc::Vec_IntArray(vLits),
                                pabc::Vec_IntArray(vLits) + ctr
                            );
                            
                            inc_assignment(ip_fanins, max_ip_fanin);
                            if (is_zero(ip_fanins)) {
                                break;
                            }
                        }

                        inc_assignment(fanins, max_fanin);
                        if (is_zero(fanins)) {
                            break;
                        }
                    }
                }
                return status;
            }

            /*******************************************************************
                Add clauses which ensure that steps occur in lexicographical
                order. In other words, we require steps operands to be
                lexicographically ordered tuples.
            *******************************************************************/
            bool
            create_lex_clauses(const spec& spec)
            {
                auto status = true;
                std::vector<int> fanins(spec.fanin);
                std::vector<int> ip_fanins(spec.fanin);

                for (int i = 0; i < spec.nr_steps - 1; i++) {
                    clear_assignment(fanins);
                    const auto max_fanin = spec.get_nr_in() + i - 1;
                    while (true) {
                        clear_assignment(ip_fanins);
                        const auto max_ip_fanin = max_fanin + 1;
                        while (true) {
                            if (lex_compare(fanins, ip_fanins) < 1) {
                                inc_assignment(ip_fanins, max_ip_fanin);
                                if (is_zero(ip_fanins)) {
                                    break;
                                } else {
                                    continue;
                                }
                            }
                            auto ctr = 0;
                            for (int k = 0; k < spec.fanin; k++) {
                                pabc::Vec_IntSetEntry(vLits, ctr++,
                                    pabc::Abc_Var2Lit(get_sel_var(spec, i, fanins[k], k), 1));
                                pabc::Vec_IntSetEntry(vLits, ctr++,
                                    pabc::Abc_Var2Lit(get_sel_var(spec, i + 1, ip_fanins[k], k), 1));
                            }

                            status &= solver->add_clause(
                                pabc::Vec_IntArray(vLits),
                                pabc::Vec_IntArray(vLits) + ctr
                            );
                            
                            inc_assignment(ip_fanins, max_ip_fanin);
                            if (is_zero(ip_fanins)) {
                                break;
                            }
                        }

                        inc_assignment(fanins, max_fanin);
                        if (is_zero(fanins)) {
                            break;
                        }
                    }
                }
                return status;
            }

            /*******************************************************************
                Ensure that Boolean operators are lexicographically ordered:
                (S_ijk /\ S_(i+1)jk) ==> f_i <= f_(i+1)
            *******************************************************************/
            void 
            create_lex_func_clauses(const spec& spec)
            {
                std::vector<int> fanins(spec.fanin);
                std::vector<int> fvar_asgns(spec.fanin);
                int lits[3];

                for (int i = 0; i < spec.nr_steps - 1; i++) {
                    clear_assignment(fanins);
                    const auto max_fanin = spec.get_nr_in() + i - 1;
                    while (true) {
                        auto base_ctr = 0;
                        for (int k = 0; k < spec.fanin; k++) {
                            auto sel_var = get_sel_var(spec, i, fanins[k], k);
                            Vec_IntSetEntry(vLits, base_ctr++, pabc::Abc_Var2Lit(sel_var, 1));
                            sel_var = get_sel_var(spec, i + 1, fanins[k], k);
                            Vec_IntSetEntry(vLits, base_ctr++, pabc::Abc_Var2Lit(sel_var, 1));
                        }

                        // The steps have the same fanin, so enforce lexicographical order.
                        // We do this by constraining the operator variables of both steps.
                        // Note: the operator variable with the highest index is used 
                        // first in the ordering.
                        for (int op_idx = 0; op_idx < nr_op_vars_per_step; op_idx++) {
                            // Inequality only has to hold if all previous operator variables
                            // are equal.
                            auto ctr = base_ctr;
                            for (int prev_idx = 0; prev_idx < op_idx; prev_idx++) {
                                const auto prev_alpha_i = get_lex_var(spec, i, prev_idx);
                                Vec_IntSetEntry(vLits, ctr++, pabc::Abc_Var2Lit(prev_alpha_i, 1));
                            }

                            // Ensure that f_i_n <= f_{i+1}_n.
                            const auto iop_var = get_op_var(spec, i, nr_op_vars_per_step - op_idx);
                            const auto ipop_var = get_op_var(spec, i + 1, nr_op_vars_per_step - op_idx);
                            Vec_IntSetEntry(vLits, ctr++, pabc::Abc_Var2Lit(iop_var, 1));
                            Vec_IntSetEntry(vLits, ctr++, pabc::Abc_Var2Lit(ipop_var, 0));
                            auto status = solver->add_clause(
                                pabc::Vec_IntArray(vLits),
                                pabc::Vec_IntArray(vLits) + ctr);
                            assert(status);
                            if (op_idx == (nr_op_vars_per_step - 1)) {
                                continue;
                            }
                            // alpha_i is 1 iff f_j_i == f_{j+1}_i.
                            auto alpha_i = get_lex_var(spec, i, op_idx);
                            lits[0] = pabc::Abc_Var2Lit(alpha_i, 1);
                            lits[1] = pabc::Abc_Var2Lit(iop_var, 0);
                            lits[2] = pabc::Abc_Var2Lit(ipop_var, 1);
                            solver->add_clause(lits, lits + 3);
                            lits[0] = pabc::Abc_Var2Lit(alpha_i, 1);
                            lits[1] = pabc::Abc_Var2Lit(iop_var, 1);
                            lits[2] = pabc::Abc_Var2Lit(ipop_var, 0);
                            solver->add_clause(lits, lits + 3);
                            lits[0] = pabc::Abc_Var2Lit(alpha_i, 0);
                            lits[1] = pabc::Abc_Var2Lit(iop_var, 1);
                            lits[2] = pabc::Abc_Var2Lit(ipop_var, 1);
                            solver->add_clause(lits, lits + 3);
                            lits[0] = pabc::Abc_Var2Lit(alpha_i, 0);
                            lits[1] = pabc::Abc_Var2Lit(iop_var, 0);
                            lits[2] = pabc::Abc_Var2Lit(ipop_var, 0);
                            solver->add_clause(lits, lits + 3);
                        }

                        inc_assignment(fanins, max_fanin);
                        if (is_zero(fanins)) {
                            break;
                        }
                    }
                }
            }

            /*******************************************************************
                Ensure that symmetric variables occur in order.
            *******************************************************************/
            bool
            create_symvar_clauses(const spec& spec)
            {
                for (int q = 1; q < spec.get_nr_in(); q++) {
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
                        if (spec.verbosity) {
                            printf("  variables x_%d and x_%d are symmetric\n",
                                    p+1, q+1);
                        }

                        for (int i = 0; i < spec.nr_steps; i++) {
                            // If (has_input(i, q) and !has_input(i, p)) --> (has_input(i-1, p) \/ ... \/ has_input(0, p))
                            for (int k = 0; k < spec.fanin; k++) {
                                int ctr = 0;
                                auto sel_var = get_sel_var(spec, i, q, k);
                                pabc::Vec_IntSetEntry(vLits, ctr++,
                                    pabc::Abc_Var2Lit(sel_var, 1));

                                for (int kp = 0; kp < spec.fanin; kp++) {
                                    if (kp == k) continue;
                                    sel_var = get_sel_var(spec, i, p, kp);
                                    pabc::Vec_IntSetEntry(vLits, ctr++,
                                        pabc::Abc_Var2Lit(sel_var, 0));
                                }

                                for (int ip = 0; ip < i; ip++) {
                                    for (int k = 0; k < spec.fanin; k++) {
                                        sel_var = get_sel_var(spec, ip, p, k);
                                        pabc::Vec_IntSetEntry(vLits, ctr++,
                                            pabc::Abc_Var2Lit(sel_var, 0));
                                    }
                                }

                                const auto status = solver->add_clause(Vec_IntArray(vLits), Vec_IntArray(vLits) + ctr);
                                if (!status) {
                                    return false;
                                }
                            }
                        }
                    }
                }

                return true;
            }

            /// Extracts chain from encoded CNF solution.
            void extract_chain(const spec& spec, chain& chain)
            {
                std::vector<int> fanins(spec.fanin);

                chain.reset(spec.get_nr_in(), spec.get_nr_out(), spec.nr_steps, spec.fanin);

                for (int i = 0; i < spec.nr_steps; i++) {
                    dynamic_truth_table op(spec.fanin);
                    for (int j = 1; j <= nr_op_vars_per_step; j++) {
                        if (solver->var_value(get_op_var(spec, i, j))) {
                            kitty::set_bit(op, j); 
                        }
                    }

                    if (spec.verbosity) {
                        printf("  step x_%d performs operation\n  ", 
                                i+spec.get_nr_in()+1);
                        kitty::print_binary(op, std::cout);
                        printf("\n");
                    }

                    for (int k = 0; k < spec.fanin; k++) {
                        for (int j = 0; j < spec.get_nr_in() + i; j++) {
                            const auto s_ij_k = get_sel_var(spec, i, j, k);
                            if (solver->var_value(s_ij_k)) {
                                fanins[k] = j;
                            }
                        }
                    }
                    if (spec.verbosity) {
                        printf("  with operands ");
                        for (int k = 0; k < spec.fanin; k++) {
                            printf("x_%d ", fanins[k] + 1);
                        }
                    }
                        
                    chain.set_step(i, fanins, op);

                    if (spec.verbosity) {
                        printf("\n");
                    }
                }

                auto triv_count = 0, nontriv_count = 0;
                for (int h = 0; h < spec.get_nr_out(); h++) {
                    if ((spec.triv_flag >> h) & 1) {
                        chain.set_output(h, 
                                (spec.triv_func(triv_count++) << 1) +
                                ((spec.out_inv >> h) & 1));
                        continue;
                    }
                    for (int i = 0; i < spec.nr_steps; i++) {
                        if (solver->var_value(get_out_var(spec, nontriv_count, i))) {
                            chain.set_output(h, 
                                    ((i + spec.get_nr_in() + 1) << 1) +
                                    ((spec.out_inv >> h) & 1));
                            nontriv_count++;
                            break;
                        }
                    }
                }
            }

            void cegar_extract_chain(const spec& spec, chain& chain)
            {
                std::vector<int> fanins(spec.fanin);

                chain.reset(spec.get_nr_in(), spec.get_nr_out(), spec.nr_steps, spec.fanin);

                for (int i = 0; i < spec.nr_steps; i++) {
                    dynamic_truth_table op(spec.fanin);
                    for (int j = 1; j <= nr_op_vars_per_step; j++) {
                        if (solver->var_value(get_op_var(spec, i, j))) {
                            kitty::set_bit(op, j); 
                        }
                    }

                    if (spec.verbosity) {
                        printf("  step x_%d performs operation\n  ", 
                                i+spec.get_nr_in()+1);
                        kitty::print_binary(op, std::cout);
                        printf("\n");
                    }

                    for (int k = 0; k < spec.fanin; k++) {
                        for (int j = 0; j < spec.get_nr_in() + i; j++) {
                            const auto s_ij_k = get_sel_var(spec, i, j, k);
                            if (solver->var_value(s_ij_k)) {
                                fanins[k] = j;
                            }
                        }
                    }
                    if (spec.verbosity) {
                        printf("  with operands ");
                        for (int k = 0; k < spec.fanin; k++) {
                            printf("x_%d ", fanins[k] + 1);
                        }
                    }
                        
                    chain.set_step(i, fanins, op);

                    if (spec.verbosity) {
                        printf("\n");
                    }
                }

                chain.set_output(0,
                    ((spec.nr_steps + spec.get_nr_in()) << 1) +
                    (spec.out_inv & 1));
            }

            void find_fanin(int i, std::vector<int>& fanins, const spec& spec) const
            {
                for (int k = 0; k < spec.fanin; k++) {
                    for (int j = 0; j < spec.get_nr_in() + i; j++) {
                        const auto s_ij_k = get_sel_var(spec, i, j, k);
                        if (solver->var_value(s_ij_k)) {
                            fanins[k] = j;
                        }
                    }
                }
            }

            /*******************************************************************
                Extracts only the underlying DAG structure from a solution.
            *******************************************************************/
            template<int FI>
            void 
            extract_dag(const spec& spec, dag<FI>& dag)
            {
                std::vector<int> fanins;
                assert(FI == spec.fanin);
                dag.reset(spec.get_nr_in(), spec.nr_steps);

                for (int i = 0; i < spec.nr_steps; i++) {
                    for (int k = 0; k < spec.fanin; k++) {
                        for (int j = 0; j < spec.get_nr_in() + i; j++) {
                            const auto s_ij_k = get_sel_var(spec, i, j, k);
                            if (solver->var_value(s_ij_k)) {
                                fanins[k] = j;
                            }
                        }
                    }
                    dag.set_vertex(i, fanins);
                }
            }

            void
            print_solver_state(const spec& spec)
            {
                printf("\n");
                printf("========================================"
                        "========================================\n");
                printf("  SOLVER STATE\n\n");

                printf("  Nr. variables = %d\n", solver->nr_vars());
                printf("  Nr. clauses = %d\n\n", solver->nr_clauses());

                
                for (int i = 0; i < spec.nr_steps; i++) {
                    for (int k = 0; k < spec.fanin; k++) {
                        for (int j = 0; j < spec.get_nr_in() + i; j++) {
                            const auto s_ij_k_var = get_sel_var(spec, i, j, k);
                            printf("  s_%d_%d_%d = %d\n", i, j, k,
                                solver->var_value(s_ij_k_var));
                        }
                    }

                    for (int oidx = nr_op_vars_per_step; oidx > 0; oidx--) {
                        printf("  f_%d_%d=%d\n", 
                                spec.get_nr_in() + i + 1, 
                                oidx + 1,
                                solver->var_value(get_op_var(spec, i, oidx))
                              );
                    }
                    printf("  f_%d_1=0\n", spec.get_nr_in() + i + 1);
                    printf("\n");

                    for (int t = spec.get_tt_size() - 1; t >= 0; t--) {
                        printf("  x_%d_%d=%d\n", spec.get_nr_in() + i+1, t + 2, 
                                solver->var_value(get_tt_var(spec, i, t)));
                    }
                    printf("  x_%d_0=0\n", spec.get_nr_in() + i + 1);
                    printf("\n");
                }
                printf("\n");

                printf("========================================"
                        "========================================\n");
            }

			/// Encodes specifciation for use in standard synthesis flow.
            bool 
            encode(const spec& spec)
            {
                assert(spec.nr_steps <= MAX_STEPS);

                create_variables(spec);
                if (!create_main_clauses(spec)) {
                    return false;
                }

                if (!create_output_clauses(spec)) {
                    return false;
                }

                if (!create_op_clauses(spec)) {
                    return false;
                }
                
                //create_cardinality_constraints(spec);

                if (spec.add_nontriv_clauses) {
                    create_nontriv_clauses(spec);
                }

                if (spec.add_alonce_clauses) {
                    create_alonce_clauses(spec);
                }

                if (spec.add_noreapply_clauses) {
                    create_noreapply_clauses(spec);
                }

                if (spec.add_colex_clauses && !create_colex_clauses(spec)) {
                    return false;
                }

                if (spec.add_lex_clauses && !create_lex_clauses(spec)) {
                    return false;
                }
                
                if (spec.add_lex_func_clauses) {
                    create_lex_func_clauses(spec);
                }
                
                if (spec.add_symvar_clauses && !create_symvar_clauses(spec)) {
                    return false;
                }

                return true;
            }

			/// Encodes specifciation for use in CEGAR based synthesis flow.
            bool 
            cegar_encode(const spec& spec)
            {
                assert(spec.nr_steps <= MAX_STEPS);

                create_variables(spec);
                
                if (!create_op_clauses(spec)) {
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
                
                if (spec.add_colex_clauses && !create_colex_clauses(spec)) {
                    return false;
                }

                if (spec.add_lex_clauses && !create_lex_clauses(spec)) {
                    return false;
                }

                if (spec.add_lex_func_clauses) {
                    create_lex_func_clauses(spec);
                }

                if (spec.add_symvar_clauses && !create_symvar_clauses(spec)) {
                    return false;
                }

                return true;
            }

            /// Assumes that a solution has been found by the current encoding.
            /// Blocks the current solution such that the solver is forced to
            /// find different ones (if they exist).
            bool
            block_solution(const spec& spec)
            {
                int ctr = 0;

                for (int i = 0; i < spec.nr_steps; i++) {
                    for (int j = 1; j <= nr_op_vars_per_step; j++) {
                        int invert = 0;
                        const auto op_var = get_op_var(spec, i, j);
                        if (solver->var_value(op_var)) {
                            invert = 1;
                        }
                        pabc::Vec_IntSetEntry(vLits, ctr++,
                                pabc::Abc_Var2Lit(get_op_var(spec, i, j),
                                    invert));
                    }

                    for (int k = 0; k < spec.fanin; k++) {
                        for (int j = 0; j < spec.get_nr_in() + i; j++) {
                            const auto s_ij_k = get_sel_var(spec, i, j, k);
                            if (solver->var_value(s_ij_k)) {
                                pabc::Vec_IntSetEntry(vLits, ctr++,
                                    pabc::Abc_Var2Lit(s_ij_k, 1));
                                break;
                            }
                        }
                    }
                }
                
                return solver->add_clause(
                            pabc::Vec_IntArray(vLits), 
                            pabc::Vec_IntArray(vLits) + ctr);
            }


            /// Similar to block_solution, but blocks all solutions with the
            /// same structure. This is more restrictive, since the other
            /// method allows for the same structure but different operators.
            bool
            block_struct_solution(const spec& spec)
            {
                int ctr = 0;

                for (int i = 0; i < spec.nr_steps; i++) {
                    for (int k = 0; k < spec.fanin; k++) {
                        for (int j = 0; j < spec.get_nr_in() + i; j++) {
                            const auto s_ij_k = get_sel_var(spec, i, j, k);
                            if (solver->var_value(s_ij_k)) {
                                pabc::Vec_IntSetEntry(vLits, ctr++,
                                    pabc::Abc_Var2Lit(s_ij_k, 1));
                                break;
                            }
                        }
                    }
                }

                return solver->add_clause(
                            pabc::Vec_IntArray(vLits), 
                            pabc::Vec_IntArray(vLits) + ctr);
            }

            int simulate(const spec& spec)
            {
                std::vector<int> fanins(spec.fanin);
                auto tt_comp = kitty::create<dynamic_truth_table>(spec.nr_in);

                for (int i = 0; i < spec.nr_steps; i++) {
                    find_fanin(i, fanins, spec);

                    kitty::clear(sim_tts[spec.nr_in + i]);
                    for (int j = 1; j <= nr_op_vars_per_step; j++) {
                        kitty::clear(tt_comp);
                        tt_comp = ~tt_comp;
                        if (solver->var_value(get_op_var(spec, i, j))) {
                            //printf("op[%d][%d]=1\n", i, j);
                            for (int k = 0; k < spec.fanin; k++) {
                                if ((j >> k) & 1) {
                                    tt_comp &= sim_tts[fanins[k]];
                                } else {
                                    tt_comp &= ~sim_tts[fanins[k]];
                                }
                            }
                            sim_tts[spec.nr_in + i] |= tt_comp;
                        /*} else {
                            printf("op[%d][%d]=0\n", i, j);*/
                        }
                    }

                    /*
                    printf("tt[%d]: ", i);
                    kitty::print_binary(sim_tts[spec.nr_in + i]);
                    std::cout << std::endl;
    */
                }

                /*
                std::cout << "sim tt: ";
                kitty::print_binary(sim_tts[spec.nr_in + spec.nr_steps - 1]);
                std::cout << std::endl;
                std::cout << "spc tt: ";
                kitty::print_binary(spec[0]);
                std::cout << std::endl;
                */

                const auto iMint = kitty::find_first_bit_difference(sim_tts[spec.nr_in + spec.nr_steps - 1], spec.out_inv ? ~spec[0] : spec[0]);
                assert(iMint > 0 || iMint == -1);
                return iMint;
            }
    };
}

