#pragma once

#include "encoder.hpp"

namespace percy
{

    class msv_encoder : public std_cegar_encoder, public enumerating_encoder
    {
        private:
			int nr_op_vars_per_step;
			int nr_op_vars;
			int nr_out_vars;
			int nr_sim_vars;
			int nr_sel_vars;
			int nr_res_vars;
			int nr_lex_vars;
            int sel_offset;
            int res_offset;
            int ops_offset;
            int out_offset;
            int sim_offset;
            int lex_offset;
            int total_nr_vars;
            
            pabc::Vec_Int_t* vLits; // Dynamic vector of literals

        public:
            msv_encoder(solver_wrapper& solver)
            {
                vLits = pabc::Vec_IntAlloc(128);
                set_solver(solver);
            }

            ~msv_encoder()
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
            get_sel_var(const spec& spec, int step_idx, int svar_idx) const
            {
                assert(step_idx < spec.nr_steps);
                assert(svar_idx < (spec.get_nr_in() + step_idx));

                auto offset = 0;
                for (int i = 0; i < step_idx; i++) {
                    offset += (spec.get_nr_in() + i);
                }

                return sel_offset + offset + svar_idx;
            }

            int
            get_res_var(const spec& spec, int step_idx, int res_var_idx) const
            {
                assert(step_idx < spec.nr_steps);
                assert(res_var_idx < (spec.fanin + 2) * (spec.get_nr_in() + step_idx + 1));

                auto offset = 0;
                for (int i = 0; i < step_idx; i++) {
                    offset += (spec.get_nr_in() + i + 1) * (spec.fanin + 2);
                }

                return res_offset + offset + res_var_idx;
            }

            int 
            get_out_var(const spec& spec, int h, int i) const
            {
                assert(h < spec.nr_nontriv);
                assert(i < spec.nr_steps);

                return out_offset + spec.nr_steps * h + i;
            }

            int
            get_sim_var(const spec& spec, int step_idx, int t) const
            {
                assert(step_idx < spec.nr_steps);
                assert(t < spec.get_tt_size());

                return sim_offset + spec.get_tt_size() * step_idx + t;
            }

            int
            get_lex_var(const spec& spec, int step_idx, int op_idx) const
            {
                assert(step_idx < spec.nr_steps);
                assert(op_idx < nr_op_vars_per_step);

                return lex_offset + step_idx * (nr_op_vars_per_step - 1) + op_idx;
            }

            /*******************************************************************
                Ensures that each gate has FI operands.
            *******************************************************************/
            void create_op_clauses(const spec& spec)
            {
                std::vector<int> svars;
                std::vector<int> res_vars;

                for (int i = 0; i < spec.nr_steps; i++) {
                    svars.clear();
                    res_vars.clear();

                    const auto nr_svars = spec.get_nr_in() + i;
                    for (int j = 0; j < nr_svars; j++) {
                        svars.push_back(get_sel_var(spec, i, j));
                    }

                    const auto nr_res_vars = (spec.fanin + 2) * (nr_svars + 1);
                    for (int j = 0; j < nr_res_vars; j++) {
                        res_vars.push_back(get_res_var(spec, i, j));
                    }

                    create_cardinality_circuit(solver, svars, res_vars, spec.fanin);

                    // Ensure that the fanin cardinality for each step i is
                    // exactly FI.
                    const auto fi_var = 
                        get_res_var(spec, i, (spec.get_nr_in() + i) * (spec.fanin + 2) + spec.fanin);
                    auto fi_lit = pabc::Abc_Var2Lit(fi_var, 0);
                    (void)solver->add_clause(&fi_lit, &fi_lit + 1);
                }
            }

            bool 
            create_output_clauses(const spec& spec)
            {
                auto status = true;

                if (spec.verbosity > 2) {
                    printf("Creating output clauses (MSV-%d)\n", spec.fanin);
                    printf("Nr. clauses = %d (PRE)\n",
                            solver->nr_clauses());
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
                    printf("Nr. clauses = %d (POST)\n",
                            solver->nr_clauses());
                }

                return status;
            }

            void
            create_variables(const spec& spec)
            {
                nr_op_vars_per_step = ((1u << spec.fanin) - 1);
                nr_op_vars = spec.nr_steps * nr_op_vars_per_step;
                nr_out_vars = spec.nr_nontriv * spec.nr_steps;
                nr_sim_vars = spec.nr_steps * spec.get_tt_size();
                nr_lex_vars = (spec.nr_steps - 1) * (nr_op_vars_per_step - 1);
                nr_sel_vars = 0;
                nr_res_vars = 0;
                for (int i = 0; i < spec.nr_steps; i++) {
                    nr_sel_vars += (spec.get_nr_in() + i);
                    nr_res_vars += (spec.get_nr_in() + i + 1) * (spec.fanin + 2);
                }
                sel_offset = 0;
                res_offset = nr_sel_vars;
                ops_offset = nr_res_vars + nr_sel_vars;
                out_offset = nr_res_vars + nr_sel_vars + nr_op_vars;
                sim_offset = nr_res_vars + nr_sel_vars + nr_op_vars + nr_out_vars;
                lex_offset = nr_res_vars + nr_sel_vars + nr_op_vars + nr_out_vars + nr_sim_vars;
                
                total_nr_vars = nr_op_vars + nr_out_vars + nr_sim_vars +
                                nr_sel_vars + nr_res_vars + nr_lex_vars;

                if (spec.verbosity > 2) {
                    printf("Creating variables (MSV-%d)\n", spec.fanin);
                    printf("nr steps = %d\n", spec.nr_steps);
                    printf("nr_sel_vars=%d\n", nr_sel_vars);
                    printf("nr_res_vars=%d\n", nr_res_vars);
                    printf("nr_op_vars = %d\n", nr_op_vars);
                    printf("nr_out_vars = %d\n", nr_out_vars);
                    printf("nr_sim_vars = %d\n", nr_sim_vars);
                    printf("nr_lex_vars = %d\n", nr_lex_vars);
                    printf("creating %d total variables\n", total_nr_vars);
                }

                solver->set_nr_vars(total_nr_vars);
            }

            bool
            create_tt_clauses(const spec& spec, const int t)
            {
                auto ret = true;
                std::vector<int> fanin_asgn(spec.fanin);
                std::vector<int> fanins(spec.fanin);
                std::vector<int> fanin_svars(spec.fanin);
                int pLits[2];

                for (int i = 0; i < spec.nr_steps; i++) {
                    // Generate the appropriate constraints for all fanin combinations.
                    const auto nr_svars_for_i = spec.get_nr_in() + i;
                    std::string bitmask(spec.fanin, 1);
                    bitmask.resize(nr_svars_for_i, 0);
                    do {
                        auto selected_idx = 0;
                        for (int svar_idx = 0; svar_idx < nr_svars_for_i; svar_idx++) {
                            if (bitmask[svar_idx]) {
                                fanins[selected_idx] = svar_idx;
                                fanin_svars[selected_idx] = get_sel_var(spec, i, svar_idx);
                                selected_idx++;
                            }
                        }
                        assert(selected_idx == spec.fanin);

                        // First add clauses for all cases where the
                        // operator i computes zero.
                        int opvar_idx = 0;
                        clear_assignment(fanin_asgn);
                        while (true) {
                            next_assignment(fanin_asgn);
                            if (is_zero(fanin_asgn)) {
                                break;
                            }
                            opvar_idx++;
                            ret &= add_simulation_clause(spec, t, i, 0,
                                    opvar_idx, fanins, fanin_svars, fanin_asgn);
                        }

                        // Next, all cases where operator i computes one.
                        opvar_idx = 0;
                        ret &= add_simulation_clause(spec, t, i, 1,
                                opvar_idx, fanins, fanin_svars, fanin_asgn);
                        while (true) {
                            next_assignment(fanin_asgn);
                            if (is_zero(fanin_asgn)) {
                                break;
                            }
                            opvar_idx++;
                            ret &= add_simulation_clause(spec, t, i, 1,
                                    opvar_idx, fanins, fanin_svars, fanin_asgn);
                        }
                    } while (std::prev_permutation(bitmask.begin(), bitmask.end()));

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
                        pLits[1] = pabc::Abc_Var2Lit(get_sim_var(spec, i, t), 
                                1 - outbit);
                        ret &= solver->add_clause(pLits, pLits+2);
                        if (spec.verbosity > 2) {
                            printf("creating oimp clause: ( ");
                            printf("!g_%d_%d \\/ %sx_%d_%d ) (status=%d)\n", 
                                    h+1, 
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
                    printf("Creating main clauses (MSV-%d)\n", spec.fanin);
                    printf("Nr. clauses = %d (PRE)\n",
                            solver->nr_clauses());
                }
                auto success = true;

                for (int t = 0; t < spec.get_tt_size(); t++) {
                    success &= create_tt_clauses(spec, t);
                }

                if (spec.verbosity > 2) {
                    printf("Nr. clauses = %d (POST)\n",
                            solver->nr_clauses());
                }

                return success;
            }

            bool 
            add_simulation_clause(
                    const spec& spec, 
                    const int t, 
                    const int i, 
                    const int output, 
                    const int opvar_idx,
                    const std::vector<int>& fanins,
                    const std::vector<int>& fanin_svars,
                    const std::vector<int>& fanin_asgn)
            {
                int ctr = 0;

                if (spec.verbosity > 3) {
                    //printf("assignment: %s\n", fanin_asgn.to_string().c_str());
                }

                for (int j = 0; j < spec.fanin; j++) {
                    auto child = fanins[j];
                    auto assign = fanin_asgn[j];
                    if (child < spec.get_nr_in()) {
                        if ((((t + 1) & (1 << child) ) ? 1 : 0 ) != assign) {
                            return true;
                        }
                    } else {
                        pabc::Vec_IntSetEntry(vLits, ctr++, pabc::Abc_Var2Lit(
                                    get_sim_var(spec, child - spec.get_nr_in(),
                                        t), assign));
                    }
                }

                for (const auto svar : fanin_svars) {
                    pabc::Vec_IntSetEntry(vLits, ctr++, pabc::Abc_Var2Lit(svar, 1));
                }

                pabc::Vec_IntSetEntry(vLits, ctr++,
                        pabc::Abc_Var2Lit(get_sim_var(spec, i, t), output));

                //printf("opvar_idx=%d\n", opvar_idx);
                if (opvar_idx > 0) {
                    pabc::Vec_IntSetEntry(vLits, ctr++, pabc::Abc_Var2Lit(
                                get_op_var(spec, i, opvar_idx), 1 - output));
                }

                auto status = solver->add_clause(
                        pabc::Vec_IntArray(vLits),
                        pabc::Vec_IntArray(vLits) + ctr); 

                if (spec.verbosity > 2) {
                    printf("creating sim. clause: (");
                    for (const auto fanin : fanins) {
                        printf(" !s_%d_%d ", spec.get_nr_in() + i + 1, fanin);
                    }
                    printf(" \\/ %sx_%d_%d ", output ? "!" : "", 
                            spec.get_nr_in() + i + 1, t + 2);

                    for (int j = 0; j < spec.fanin; j++) {
                        auto child = fanins[j];
                        auto assign = fanin_asgn[j];
                        if (child < spec.get_nr_in()) {
                            continue;
                        }
                        printf(" \\/ %sx_%d_%d ",
                                    assign ? "!" : "", child + 1, t + 2);
                    }
                    if (opvar_idx > 0) {
                        printf(" \\/ %sf_%d_%d ", 
                                (1-output) ? "!" : "", 
                                spec.get_nr_in() + i + 1, opvar_idx + 1);
                    }
                    printf(") (status=%d)\n", status);
                }

                return status;
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
            void 
            create_alonce_clauses(const spec& spec)
            {
                for (int i = 0; i < spec.nr_steps; i++) {
                    auto ctr = 0;

                    // Either one of the outputs points to this step.
                    for (int h = 0; h < spec.nr_nontriv; h++) {
                        pabc::Vec_IntSetEntry(vLits, ctr++, 
                                pabc::Abc_Var2Lit(get_out_var(spec, h, i), 0));
                    }

                    // Or one of the succeeding steps points to this step.
                    for (int ip = i + 1; ip < spec.nr_steps; ip++) {
                        const auto sel_var = get_sel_var(spec, ip, spec.get_nr_in() + i);
                        pabc::Vec_IntSetEntry(vLits, ctr++, pabc::Abc_Var2Lit(sel_var, 0));
                    }
                    auto status = solver->add_clause(
                            pabc::Vec_IntArray(vLits),
                            pabc::Vec_IntArray(vLits) + ctr);
                    assert(status);
                }
            }

            /*******************************************************************
                Add clauses which ensure that operands are never re-applied. In
                other words, (Sijk --> ~Si'ji) & (Sijk --> ~Si'ki), 
                for all (i < i').
            *******************************************************************/
            void 
            create_noreapply_clauses(const spec& spec)
            {
                std::vector<int> fanins(spec.fanin);
                std::vector<int> fanin_svars(spec.fanin);
                std::vector<int> pfanin_svars(spec.fanin);

                for (int i = 0; i < spec.nr_steps - 1; i++) {
                    // Generate the appropriate constraints for all fanin combinations.
                    const auto nr_svars_for_i = spec.get_nr_in() + i;
                    std::string bitmask(spec.fanin, 1);
                    bitmask.resize(nr_svars_for_i, 0);
                    do {
                        auto selected_idx = 0;
                        for (int svar_idx = 0; svar_idx < nr_svars_for_i; svar_idx++) {
                            if (bitmask[svar_idx]) {
                                fanins[selected_idx] = svar_idx;
                                fanin_svars[selected_idx] = get_sel_var(spec, i, svar_idx);
                                selected_idx++;
                            }
                        }
                        assert(selected_idx == spec.fanin);

                        // Step i' cannot have both fanin i and the remaining FI - 1 fanins from
                        // the same collection as the fanin of step i.
                        for (int ip = i + 1; ip < spec.nr_steps; ip++) {
                            pfanin_svars[spec.fanin - 1] = get_sel_var(spec, ip, spec.get_nr_in() + i);
                            for (int j = 0; j < spec.fanin; j++) {
                                std::string bitmaskp(spec.fanin - 1, 1);
                                bitmaskp.resize(spec.fanin, 0);
                                do {
                                    selected_idx = 0;
                                    for (int fi_idx = 0; fi_idx < spec.fanin; fi_idx++) {
                                        if (bitmaskp[fi_idx]) {
                                            pfanin_svars[selected_idx++] = get_sel_var(spec, ip, fanins[fi_idx]);
                                        }
                                    }
                                    assert(selected_idx == (spec.fanin - 1));

                                    auto ctr = 0;
                                    for (const auto svar : fanin_svars) {
                                        pabc::Vec_IntSetEntry(vLits, ctr++, pabc::Abc_Var2Lit(svar, 1));
                                    }
                                    for (const auto svar : pfanin_svars) {
                                        pabc::Vec_IntSetEntry(vLits, ctr++, pabc::Abc_Var2Lit(svar, 1));
                                    }
                                    auto status = solver->add_clause(
                                        pabc::Vec_IntArray(vLits),
                                        pabc::Vec_IntArray(vLits) + ctr);
                                    assert(status);
                                } while (std::prev_permutation(bitmaskp.begin(), bitmaskp.end()));
                            }
                        }
                    } while (std::prev_permutation(bitmask.begin(), bitmask.end()));


                }
            }

            /*******************************************************************
                Add clauses which ensure that steps occur in co-lexicographical
                order.
            *******************************************************************/
            void 
            create_colex_clauses(const spec& spec)
            {
                auto pLits = new int[2 * spec.fanin];
                std::vector<int> fanins_i(spec.fanin);
                std::vector<int> fanins_ip(spec.fanin);

                for (int i = 0; i < spec.nr_steps - 1; i++) {
                    const auto nr_svars_for_i = spec.get_nr_in() + i;
                    std::string bitmask(spec.fanin, 1);
                    bitmask.resize(nr_svars_for_i, 0);
                    do {
                        auto fanin_ctr = 0;
                        for (int k = 0; k < nr_svars_for_i; k++) {
                            if (bitmask[k]) {
                                pLits[fanin_ctr] = pabc::Abc_Var2Lit(get_sel_var(spec, i, k), 1);
                                fanins_i[fanin_ctr] = k;
                                fanin_ctr++;
                            }
                        }
                        assert(fanin_ctr == spec.fanin);

                        const auto nr_svars_for_ip = spec.get_nr_in() + i + 1;
                        std::string bitmask_ip(spec.fanin, 1);
                        bitmask_ip.resize(nr_svars_for_ip, 0);
                        do {
                            fanin_ctr = 0;
                            for (int k = 0; k < nr_svars_for_ip; k++) {
                                if (bitmask_ip[k]) {
                                    pLits[spec.fanin + fanin_ctr] = pabc::Abc_Var2Lit(get_sel_var(spec, i + 1, k), 1);
                                    fanins_ip[fanin_ctr] = k;
                                    fanin_ctr++;
                                }
                            }
                            assert(fanin_ctr == spec.fanin);
                            if (colex_compare(fanins_i, fanins_ip) == 1) {
                                auto status = solver->add_clause(pLits, pLits + (spec.fanin * 2));
                                assert(status);
                            }
                        } while (std::prev_permutation(bitmask_ip.begin(), bitmask_ip.end()));
                    } while (std::prev_permutation(bitmask.begin(), bitmask.end()));
                }

                delete[] pLits;
            }

            /*******************************************************************
                Add clauses which ensure that steps occur in lexicographical
                order. In other words, we require steps operands to be
                lexicographically ordered tuples.
            *******************************************************************/
            void 
            create_lex_clauses(const spec& spec)
            {
                auto pLits = new int[2 * spec.fanin];
                std::vector<int> fanins_i(spec.fanin);
                std::vector<int> fanins_ip(spec.fanin);

                for (int i = 0; i < spec.nr_steps - 1; i++) {
                    const auto nr_svars_for_i = spec.get_nr_in() + i;
                    std::string bitmask(spec.fanin, 1);
                    bitmask.resize(nr_svars_for_i, 0);
                    do {
                        auto fanin_ctr = 0;
                        for (int k = 0; k < nr_svars_for_i; k++) {
                            if (bitmask[k]) {
                                pLits[fanin_ctr] = pabc::Abc_Var2Lit(get_sel_var(spec, i, k), 1);
                                fanins_i[fanin_ctr] = k;
                                fanin_ctr++;
                            }
                        }
                        assert(fanin_ctr == spec.fanin);

                        const auto nr_svars_for_ip = spec.get_nr_in() + i + 1;
                        std::string bitmask_ip(spec.fanin, 1);
                        bitmask_ip.resize(nr_svars_for_ip, 0);
                        do {
                            fanin_ctr = 0;
                            for (int k = 0; k < nr_svars_for_ip; k++) {
                                if (bitmask_ip[k]) {
                                    pLits[spec.fanin + fanin_ctr] = pabc::Abc_Var2Lit(get_sel_var(spec, i + 1, k), 1);
                                    fanins_ip[fanin_ctr] = k;
                                    fanin_ctr++;
                                }
                            }
                            assert(fanin_ctr == spec.fanin);
                            if (lex_compare(fanins_i, fanins_ip) == 1) {
                                auto status = solver->add_clause(pLits, pLits + (spec.fanin * 2));
                                assert(status);
                            }
                        } while (std::prev_permutation(bitmask_ip.begin(), bitmask_ip.end()));
                    } while (std::prev_permutation(bitmask.begin(), bitmask.end()));
                }
            }

            /*******************************************************************
                Ensure that Boolean operators are lexicographically ordered.
            *******************************************************************/
            void 
            create_lex_func_clauses(const spec& spec)
            {
                int lits[3];
                std::vector<int> fanin(spec.fanin);
                std::vector<int> fvar_asgns;

                for (int i = 0; i < spec.nr_steps - 1; i++) {
                    const auto nr_svars_for_i = spec.get_nr_in() + i;
                    std::string bitmask(spec.fanin, 1);
                    bitmask.resize(nr_svars_for_i, 0);
                    do {
                        auto fanin_ctr = 0;
                        for (int j = 0; j < nr_svars_for_i; j++) {
                            if (bitmask[j]) {
                                fanin[fanin_ctr++] = j;
                            }
                        }
                        assert(fanin_ctr == spec.fanin);

                        auto ctr = 0;
                        for (int k = 0; k < spec.fanin; k++) {
                            Vec_IntSetEntry(vLits, ctr++, pabc::Abc_Var2Lit(get_sel_var(spec, i, fanin[k]), 1));
                            Vec_IntSetEntry(vLits, ctr++, pabc::Abc_Var2Lit(get_sel_var(spec, i + 1, fanin[k]), 1));
                        }
                        assert(ctr == spec.fanin * 2);
                        // The steps have the same fanin, so enforce lexicographical order.
                        // We do this by constraining the operator variables of both steps.
                        // Note: the operator variable with the highest index is used 
                        // first in the ordering.
                        for (int op_idx = 0; op_idx < nr_op_vars_per_step; op_idx++) {
                            // Inequality only has to hold if all previous operator variables
                            // are equal.
                            ctr = spec.fanin * 2;
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
                    } while (std::prev_permutation(bitmask.begin(), bitmask.end()));
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
                        if (spec.verbosity > 1) {
                            printf("  variables x_%d and x_%d are symmetric\n",
                                    p+1, q+1);
                        }

                        for (int i = 0; i < spec.nr_steps; i++) {
                            const auto svar_p = get_sel_var(spec, i, p);
                            const auto svar_q = get_sel_var(spec, i, q);

                            pabc::Vec_IntSetEntry(vLits, 0, pabc::Abc_Var2Lit(svar_p, 0));
                            pabc::Vec_IntSetEntry(vLits, 1, pabc::Abc_Var2Lit(svar_q, 1));

                            auto ctr = 2;
                            for (int ip = 0; ip < i; ip++) {
                                const auto ip_svar_p = get_sel_var(spec, ip, p);
                                pabc::Vec_IntSetEntry(vLits, ctr++, pabc::Abc_Var2Lit(ip_svar_p, 0));
                            }
                            if (!solver->add_clause(Vec_IntArray(vLits), Vec_IntArray(vLits) + ctr)) {
                                return false;
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

                if (spec.verbosity > 2) {
                    print_solver_state(spec);
                }

                for (int i = 0; i < spec.nr_steps; i++) {
                    kitty::dynamic_truth_table op(spec.fanin);
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

                    auto svar_idx = 0;
                    for (int j = 0; j < spec.get_nr_in() + i; j++) {
                        const auto sel_var = get_sel_var(spec, i, j);
                        if (solver->var_value(sel_var)) {
                            fanins[svar_idx++] = j;
                        }
                    }
                    assert(svar_idx == spec.fanin);
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

                if (spec.verbosity > 2) {
                    print_solver_state(spec);
                }

                for (int i = 0; i < spec.nr_steps; i++) {
                    kitty::dynamic_truth_table op(spec.fanin);
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

                    auto svar_idx = 0;
                    for (int j = 0; j < spec.get_nr_in() + i; j++) {
                        const auto sel_var = get_sel_var(spec, i, j);
                        if (solver->var_value(sel_var)) {
                            fanins[svar_idx++] = j;
                        }
                    }
                    assert(svar_idx == spec.fanin);
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
                auto svar_idx = 0;
                for (int j = 0; j < spec.nr_in + i; j++) {
                    const auto sel_var = get_sel_var(spec, i, j);
                    if (solver->var_value(sel_var)) {
                        fanins[svar_idx++] = j;
                    }
                }
            }

            /*******************************************************************
                Extracts only the underlying DAG structure from a solution.
            *******************************************************************/
            template<int FI>
            void 
            extract_dag(spec& spec, dag<FI>& dag)
            {
                assert(FI == spec.fanin);
                dag.reset(spec.get_nr_in(), spec.nr_steps);
                std::vector<int> fanins(spec.fanin);

                for (int i = 0; i < spec.nr_steps; i++) {
                    const auto nr_svars_for_i = spec.get_nr_in() + i;
                    auto svar_idx = 0;
                    for (int j = 0; j < nr_svars_for_i; j++) {
                        const auto sel_var = get_sel_var(spec, i, j);
                        if (solver->var_value(sel_var)) {
                            fanins[svar_idx++] = j;
                        }
                    }
                    assert(svar_idx == spec.fanin);
                    dag.set_vertex(i, fanins);
                }
            }

            void
            print_solver_state(const spec& spec)
            {
                std::vector<int> fanins(spec.fanin);

                printf("\n");
                printf("========================================"
                        "========================================\n");
                printf("  SOLVER STATE\n\n");

                printf("  Nr. variables = %d\n", solver->nr_vars());
                printf("  Nr. clauses = %d\n\n", solver->nr_clauses());

                for (int i = 0; i < spec.nr_steps; i++) {
                    bool step_has_fanins = false;
                    auto svar_idx = 0;
                    for (int j = 0; j < spec.get_nr_in() + i; j++) {
                        const auto sel_var = get_sel_var(spec, i, j);
                        if (solver->var_value(sel_var)) {
                            fanins[svar_idx++] = j;
                        }
                    }
                    if (svar_idx == spec.fanin) {
                        printf("  x_%d has inputs ",
                            spec.get_nr_in() + i + 1);
                        for (int k = spec.fanin - 1; k >= 0; k--) {
                            printf("x_%d ", fanins[k] + 1);
                        }
                        step_has_fanins = true;
                    }
                    if (!step_has_fanins) {
                        printf("  only %d fanins found for x_%d\n",
                                svar_idx, spec.get_nr_in() + i + 1);
                    }

                    printf("  f_%d = ", spec.get_nr_in() + i + 1);
                    for (int oidx = nr_op_vars_per_step; oidx > 0; oidx--) {
                        printf("%d", solver->var_value(get_op_var(spec, i, oidx)));
                    }
                    printf("0\n");

                    printf("  tt_%d = ", spec.get_nr_in() + i + 1);
                    for (int t = spec.get_tt_size() - 1; t >= 0; t--) {
                        printf("%d", solver->var_value(get_sim_var(spec, i, t)));
                    }
                    printf("0\n\n");
                }

                for (int h = 0; h < spec.nr_nontriv; h++) {
                    for (int i = 0; i < spec.nr_steps; i++) {
                        printf("  g_%d_%d=%d\n", h + 1, 
                                spec.get_nr_in() + i + 1,
                                solver->var_value(get_out_var(spec, h, i)));
                    }
                }
                printf("\n");

                for (int i = 0; i < spec.nr_steps; i++) {
                    for (int j = 0; j < spec.get_nr_in() + i; j++) {
                        printf("  s_%d_%d = %d\n", spec.get_nr_in() + i + 1, j,
                            solver->var_value(get_sel_var(spec, i, j)));
                    }
                    printf("\n");

                    for (int k = 0; k < spec.get_nr_in() + i + 1; k++) {
                        for (int c = 0; c < spec.fanin + 2; c++) {
                            printf("res[%d] ", c);
                        }
                        printf("\n");
                        for (int c = 0; c < spec.fanin + 2; c++) {
                            printf("    %d   ", solver->var_value(
                                get_res_var(spec, i, k * (spec.fanin + 2) + c)));
                        }
                        printf("\n");
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
                                solver->var_value( get_sim_var(spec, i, t)));
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

                create_op_clauses(spec);
                
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

                if (spec.add_lex_clauses) {
                    create_lex_clauses(spec);
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
                
                if (!create_output_clauses(spec)) {
                    return false;
                }
                
                create_op_clauses(spec);
                
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
                
                if (spec.add_lex_clauses) {
                    create_lex_clauses(spec);
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

                    for (int j = 0; j < spec.get_nr_in() + i; j++) {
                        const auto sel_var = get_sel_var(spec, i , j);
                        if (solver->var_value(sel_var)) {
                            pabc::Vec_IntSetEntry(vLits, ctr++,
                                    pabc::Abc_Var2Lit(sel_var, 1));
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
                    for (int j = 0; j < spec.get_nr_in() + i; j++) {
                        const auto sel_var = get_sel_var(spec, i, j);
                        if (solver->var_value(sel_var)) {
                            pabc::Vec_IntSetEntry(vLits, ctr++,
                                    pabc::Abc_Var2Lit(sel_var, 1));
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
                            for (int k = 0; k < spec.fanin; k++) {
                                if ((j >> k) & 1) {
                                    tt_comp &= sim_tts[fanins[k]];
                                } else {
                                    tt_comp &= ~sim_tts[fanins[k]];
                                }
                            }
                            sim_tts[spec.nr_in + i] |= tt_comp;
                        }
                    }
                }

                const auto iMint = kitty::find_first_bit_difference(sim_tts[spec.nr_in + spec.nr_steps - 1], spec.out_inv ? ~spec[0] : spec[0]);
                assert(iMint > 0 || iMint == -1);
                return iMint;
            }
            
    };
}

