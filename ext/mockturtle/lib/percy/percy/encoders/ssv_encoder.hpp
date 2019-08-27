#pragma once

#include "encoder.hpp"
#include "../misc.hpp"

namespace percy
{
    class ssv_encoder : public std_cegar_encoder, public enumerating_encoder
    {
        private:
			int nr_op_vars_per_step;
			int nr_op_vars;
			int nr_out_vars;
			int nr_sim_vars;
			int nr_sel_vars;
			int nr_lex_vars;
            int sel_offset;
            int ops_offset;
            int out_offset;
            int sim_offset;
            int lex_offset;
            int total_nr_vars;
            
            pabc::Vec_Int_t* vLits; // Dynamic vector of literals
            std::vector<std::vector<int>> svar_map;
            std::vector<int> nr_svar_map;

        public:
            ssv_encoder(solver_wrapper& solver)
            {
                vLits = pabc::Vec_IntAlloc(128);
                set_solver(solver);
            }

            ~ssv_encoder()
            {
                pabc::Vec_IntFree(vLits);
            }

            int get_op_var(const spec& spec, int step_idx, int var_idx) const 
            {
                assert(step_idx < spec.nr_steps);
                assert(var_idx > 0);
                assert(var_idx <= nr_op_vars_per_step);

                return ops_offset + step_idx * nr_op_vars_per_step + var_idx-1;
            }

            int get_sel_var(int var_idx) const
            {
                assert(var_idx < nr_sel_vars);

                return sel_offset + var_idx;
            }

            int get_out_var(const spec& spec, int h, int i) const
            {
                assert(h < spec.nr_nontriv);
                assert(i < spec.nr_steps);

                return out_offset + spec.nr_steps * h + i;
            }

            int get_sim_var(const spec& spec, int step_idx, int t) const
            {
                assert(step_idx < spec.nr_steps);
                assert(t < spec.get_tt_size());

                return sim_offset + spec.get_tt_size() * step_idx + t;
            }

            int get_lex_var(const spec& spec, int step_idx, int op_idx) const
            {
                assert(step_idx < spec.nr_steps);
                assert(op_idx < nr_op_vars_per_step);

                return lex_offset + step_idx * (nr_op_vars_per_step - 1) + op_idx;
            }

            /*******************************************************************
                Ensures that each gate has FI operands.
            *******************************************************************/
            bool create_op_clauses(const spec& spec)
            {
                auto status = true;

                if (spec.verbosity > 2) {
                    printf("Creating op clauses (SSV-%d)\n", spec.fanin);
                    printf("Nr. clauses = %d (PRE)\n", solver->nr_clauses());
                }

                auto svar_offset = 0;
                for (int i = 0; i < spec.nr_steps; i++) {
                    const auto nr_svars_for_i = nr_svar_map[i];
                    
                    for (int j = 0u; j < nr_svars_for_i; j++) {
                        pabc::Vec_IntSetEntry(vLits, j, 
                                pabc::Abc_Var2Lit(get_sel_var(j + svar_offset), 0));
                    }

                    status &= solver->add_clause(
                            pabc::Vec_IntArray(vLits), 
                            pabc::Vec_IntArray(vLits) + nr_svars_for_i);

                    if (spec.verbosity > 2) {
                        printf("creating op clause: ( ");
                        for (int j = 0u; j < nr_svars_for_i; j++) {
                            printf("%ss_%d_%d ", j > 0 ? "\\/ " : "",
                                    spec.get_nr_in() + i + 1, j + 1);
                        }
                        printf(") (status=%d)\n", status);
                        for (int j = 0u; j < nr_svars_for_i; j++) {
                            printf("svar(%d) = %d\n", j + svar_offset,
                                    get_sel_var(j + svar_offset));
                        }
                    }

                    svar_offset += nr_svars_for_i;
                }
                if (spec.verbosity > 2) {
                    printf("Nr. clauses = %d (POST)\n", solver->nr_clauses());
                }

                return status;
            }

            bool create_output_clauses(const spec& spec)
            {
                auto status = true;

                if (spec.verbosity > 2) {
                    printf("Creating output clauses (SSV-%d)\n", spec.fanin);
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

            void create_variables(const spec& spec)
            {
                std::vector<int> fanins(spec.fanin);

                nr_op_vars_per_step = ((1u << spec.fanin) - 1);
                nr_op_vars = spec.nr_steps * nr_op_vars_per_step;
                nr_out_vars = spec.nr_nontriv * spec.nr_steps;
                nr_sim_vars = spec.nr_steps * spec.get_tt_size();
                nr_lex_vars = (spec.nr_steps - 1) * (nr_op_vars_per_step - 1);
                nr_sel_vars = 0;
                svar_map.clear();
                nr_svar_map.resize(spec.nr_steps);
                for (int i = spec.get_nr_in(); 
						i < spec.get_nr_in() + spec.nr_steps; i++) {
                    if (spec.verbosity > 2) {
                        printf("adding sel vars for step %d\n",
                                i + 1);
                    }
                    //spec.nr_sel_vars += binomial_coeff(i, FI); 
					//( i * ( i - 1 ) ) / 2;
                    auto nr_svars_for_i = 0u;
                    fanin_init(fanins, spec.fanin - 1);
                    do  {
                        if (spec.verbosity > 4) {
                            print_fanin(fanins);
                        }
                        svar_map.push_back(fanins);
                        nr_svars_for_i++;
                    } while (fanin_inc(fanins, i-1));
                    
                    if (spec.verbosity > 2) {
                        printf("added %u sel vars\n", nr_svars_for_i);
                    }

                    nr_sel_vars += nr_svars_for_i;
                    nr_svar_map[i - spec.get_nr_in()] = nr_svars_for_i;
                    assert(nr_svars_for_i == binomial_coeff(i, spec.fanin));
                }
                sel_offset = 0;
                ops_offset = nr_sel_vars;
                out_offset = nr_sel_vars + nr_op_vars;
                sim_offset = nr_sel_vars + nr_op_vars + nr_out_vars;
                lex_offset = nr_sel_vars + nr_op_vars + nr_out_vars + nr_sim_vars;
                
                total_nr_vars = nr_op_vars + nr_out_vars + nr_sim_vars +
                                nr_sel_vars + nr_lex_vars;

                if (spec.verbosity > 1) {
                    printf("Creating variables (SSV-%d)\n", spec.fanin);
                    printf("nr steps = %d\n", spec.nr_steps);
                    printf("nr_sel_vars=%d\n", nr_sel_vars);
                    printf("nr_op_vars = %d\n", nr_op_vars);
                    printf("nr_out_vars = %d\n", nr_out_vars);
                    printf("nr_sim_vars = %d\n", nr_sim_vars);
                    printf("nr_lex_vars = %d\n", nr_lex_vars);
                    printf("creating %d total variables\n", total_nr_vars);
                }

                solver->set_nr_vars(total_nr_vars);
            }

            bool create_tt_clauses(const spec& spec, const int t)
            {
                auto ret = true;
                std::vector<int> fanin_asgn(spec.fanin);
                int pLits[2];

                int svar_offset = 0;
                for (int i = 0; i < spec.nr_steps; i++) {
                    const auto nr_svars_for_i = nr_svar_map[i];

                    for (int j = 0; j < nr_svars_for_i; j++) {
                        const auto svar = j + svar_offset;
                        const auto& fanins = svar_map[svar];

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
                            ret &= add_simulation_clause(spec, t, i, svar, 0,
                                    opvar_idx, fanins, fanin_asgn);
                        }

                        // Next, all cases where operator i computes one.
                        opvar_idx = 0;
                        ret &= add_simulation_clause(spec, t, i, svar, 1,
                                opvar_idx, fanins, fanin_asgn);
                        while (true) {
                            next_assignment(fanin_asgn);
                            if (is_zero(fanin_asgn)) {
                                break;
                            }
                            opvar_idx++;
                            ret &= add_simulation_clause(spec, t, i, svar, 1,
                                    opvar_idx, fanins, fanin_asgn);
                        }
                    }
                    svar_offset += nr_svars_for_i;

                    // If an output has selected this particular operand, we
                    // need to ensure that this operand's truth table satisfies
                    // the specified output function.
                    for (int h = 0; h < spec.nr_nontriv; h++) {
                        if (spec.is_dont_care(h, t + 1)) {
                            continue;
                        }
                        auto outbit = kitty::get_bit(
                                spec[spec.synth_func(h)], t + 1);
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

            bool create_main_clauses(const spec& spec)
            {
                if (spec.verbosity > 2) {
                    printf("Creating main clauses (SSV-%d)\n", spec.fanin);
                    printf("Nr. clauses = %d (PRE)\n", solver->nr_clauses());
                }
                auto success = true;

                for (int t = 0; t < spec.get_tt_size(); t++) {
                    success &= create_tt_clauses(spec, t);
                }

                if (spec.verbosity > 2) {
                    printf("Nr. clauses = %d (POST)\n", solver->nr_clauses());
                }

                return success;
            }

            bool add_simulation_clause(
                    const spec& spec, 
                    const int t, 
                    const int i, 
                    const int svar, 
                    const int output, 
                    const int opvar_idx,
                    const std::vector<int>& fanins,
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

                pabc::Vec_IntSetEntry(vLits, ctr++,
                        pabc::Abc_Var2Lit(get_sel_var(svar), 1));
                pabc::Vec_IntSetEntry(vLits, ctr++,
                        pabc::Abc_Var2Lit(get_sim_var(spec, i, t), output));

                //printf("sel_var=%d, sim_var=%d\n", svar, get_sim_var(spec, i, t));

                //printf("opvar_idx=%d\n", opvar_idx);
                if (opvar_idx > 0) {
                    pabc::Vec_IntSetEntry(vLits, ctr++, pabc::Abc_Var2Lit(
                                get_op_var(spec, i, opvar_idx), 1 - output));
                }

                auto status = solver->add_clause(
                        pabc::Vec_IntArray(vLits),
                        pabc::Vec_IntArray(vLits) + ctr); 

                if (spec.verbosity > 3) {
                    printf("creating sim. clause: (");
                    printf(" !s_%d_%d ", spec.get_nr_in() + i + 1, svar + 1);
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
            void create_nontriv_clauses(const spec& spec)
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

            bool create_primitive_clauses(const spec& spec)
            {
                const auto primitives = spec.get_compiled_primitives();

                if (primitives.size() == 1) {
                    const auto op = primitives[0];
                    for (int i = 0; i < spec.nr_steps; i++) {
                        for (int j = 1; j <= nr_op_vars_per_step; j++) {
                            const auto op_var = get_op_var(spec, i, j);
                            auto op_lit = pabc::Abc_Var2Lit(op_var, 1 - kitty::get_bit(op, j));
                            const auto status = solver->add_clause(&op_lit, &op_lit + 1);
                            if (!status) {
                                return false;
                            }
                        }
                    }
                } else {
                    kitty::dynamic_truth_table tt(spec.fanin);
                    kitty::clear(tt);
                    do {
                        if (!is_normal(tt)) {
                            kitty::next_inplace(tt);
                            continue;
                        }
                        bool is_primitive_operator = false;
                        for (const auto& primitive : primitives) {
                            if (primitive == tt) {
                                is_primitive_operator = true;
                            }
                        }
                        if (!is_primitive_operator) {
                            for (int i = 0; i < spec.nr_steps; i++) {
                                for (int j = 1; j <= nr_op_vars_per_step; j++) {
                                    pabc::Vec_IntSetEntry(vLits, j - 1,
                                        pabc::Abc_Var2Lit(get_op_var(spec, i, j),
                                            kitty::get_bit(tt, j)));
                                }
                                const auto status = solver->add_clause(
                                    pabc::Vec_IntArray(vLits),
                                    pabc::Vec_IntArray(vLits) + nr_op_vars_per_step);
                                if (!status) {
                                    return false;
                                }
                            }
                        }
                        kitty::next_inplace(tt);
                    } while (!kitty::is_const0(tt));
                }

                return true;
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

                    auto svar_offset = 0;
                    for (int j = 0; j < i + 1; j++) {
                        svar_offset += nr_svar_map[j];
                    }

                    // Or one of the succeeding steps points to this step.
                    for (int ip = i + 1; ip < spec.nr_steps; ip++) {
                        const auto nr_svars_for_ip = nr_svar_map[ip];
                        for (int j = 0; j < nr_svars_for_ip; j++) {
                            const auto sel_var = get_sel_var(svar_offset + j);
                            const auto& fanins = svar_map[svar_offset + j];
                            for (auto fanin : fanins) {
                                if (fanin == spec.get_nr_in() + i) {
                                    pabc::Vec_IntSetEntry(
                                            vLits, 
                                            ctr++,
                                            pabc::Abc_Var2Lit(
                                                get_sel_var(sel_var), 0)
                                    );
                                }
                            }
                        }
                        svar_offset += nr_svars_for_ip;
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
                int pLits[3];
                auto svar_offset = 0;

                for (int i = 0; i < spec.nr_steps - 1; i++) {
                    const auto nr_svars_for_i = nr_svar_map[i];
                    for (int j = 0; j < nr_svars_for_i; j++) {
                        const auto sel_var = get_sel_var(svar_offset + j);
                        const auto& fanins = svar_map[svar_offset + j];
                        
                        auto svar_offsetp = 0;
                        for (int k = 0; k < i + 1; k++) {
                            svar_offsetp += nr_svar_map[k];
                        }

                        for (int ip = i + 1; ip < spec.nr_steps; ip++) {
                            const auto nr_svars_for_ip = nr_svar_map[ip];
                            for (int jp = 0; jp < nr_svars_for_ip; jp++) {
                                const auto sel_varp = 
                                    get_sel_var(svar_offsetp + jp);
                                const auto& faninsp = 
                                    svar_map[svar_offsetp + jp];

                                auto subsumed = true;
                                auto has_fanin_i = false;
                                for (auto faninp : faninsp) {
                                    if (faninp == i + spec.get_nr_in()) {
                                        has_fanin_i = true;
                                    } else {
                                        auto is_included = false;
                                        for (auto fanin : fanins) {
                                            if (fanin == faninp) {
                                                is_included = true;
                                            }
                                        }
                                        if (!is_included) {
                                            subsumed = false;
                                        }
                                    }
                                }
                                if (has_fanin_i && subsumed) {
                                    pLits[0] = pabc::Abc_Var2Lit(sel_var, 1);
                                    pLits[1] = pabc::Abc_Var2Lit(sel_varp, 1);
                                    auto status = solver->add_clause(pLits, pLits + 2);
                                    assert(status);
                                }

                                // Disallow:
                                // x_i  : (j, k)
                                // x_ip : (j, k)
                                // x_ipp: (i,ip)
                                // TODO: implement general case!
                                if (spec.fanin == 2 && fanins == faninsp) {
                                    auto svar_offsetpp = svar_offsetp + nr_svar_map[ip];
                                    for (int ipp = ip + 1; ipp < spec.nr_steps; ipp++) {
                                        const auto nr_svars_for_ipp = nr_svar_map[ipp];
                                        for (int jpp = 0; jpp < nr_svars_for_ipp; jpp++) {
                                            const auto sel_varpp =
                                                get_sel_var(svar_offsetpp + jpp);
                                            const auto& faninspp =
                                                svar_map[svar_offsetpp + jpp];
                                            if ((faninspp[0] == spec.nr_in + i) && (faninspp[1] == spec.nr_in + ip)) {
                                                pLits[0] = pabc::Abc_Var2Lit(sel_var, 1);
                                                pLits[1] = pabc::Abc_Var2Lit(sel_varp, 1);
                                                pLits[2] = pabc::Abc_Var2Lit(sel_varpp, 1);
                                                auto status = solver->add_clause(pLits, pLits + 3);
                                                assert(status);
                                                break;
                                            }
                                        }
                                        svar_offsetpp += nr_svars_for_ipp;
                                    }
                                }
                            }

                            svar_offsetp += nr_svars_for_ip;
                        }
                    }
                    svar_offset += nr_svars_for_i;
                }
            }

            /*******************************************************************
                Add clauses which ensure that steps occur in co-lexicographical
                order. In other words, we require steps operands to be 
                co-lexicographically ordered tuples.
            *******************************************************************/
            void 
            create_colex_clauses(const spec& spec)
            {
                int pLits[2];
                auto svar_offset = 0;

                for (int i = 0; i < spec.nr_steps - 1; i++) {
                    const auto nr_svars_for_i = nr_svar_map[i];
                    for (int j = 0; j < nr_svars_for_i; j++) {
                        const auto sel_var = get_sel_var(svar_offset + j);
                        const auto& fanins1 = svar_map[svar_offset + j];
                        pLits[0] = pabc::Abc_Var2Lit(sel_var, 1);

                        auto svar_offsetp = svar_offset + nr_svars_for_i;
                        const auto nr_svars_for_ip = nr_svar_map[i + 1];
                        for (int jp = 0; jp < nr_svars_for_ip; jp++) {
                            const auto sel_varp = get_sel_var(svar_offsetp+jp);
                            const auto& fanins2 = svar_map[svar_offsetp + jp];

                            if (colex_compare(fanins1, fanins2) == 1) {
                                pLits[1] = pabc::Abc_Var2Lit(sel_varp, 1);
                                auto status = solver->add_clause(pLits, pLits+2);
                                assert(status);
                            }
                        }
                    }

                    svar_offset += nr_svars_for_i;
                }
            }

            /*******************************************************************
                Add clauses which ensure that steps occur in lexicographical
                order. In other words, we require steps operands to be
                lexicographically ordered tuples.
            *******************************************************************/
            void 
            create_lex_clauses(const spec& spec)
            {
                int pLits[2];
                auto svar_offset = 0;

                for (int i = 0; i < spec.nr_steps - 1; i++) {
                    const auto nr_svars_for_i = nr_svar_map[i];
                    for (int j = 0; j < nr_svars_for_i; j++) {
                        const auto sel_var = get_sel_var(svar_offset + j);
                        const auto& fanins1 = svar_map[svar_offset + j];
                        pLits[0] = pabc::Abc_Var2Lit(sel_var, 1);

                        auto svar_offsetp = svar_offset + nr_svars_for_i;
                        const auto nr_svars_for_ip = nr_svar_map[i + 1];
                        for (int jp = 0; jp < nr_svars_for_ip; jp++) {
                            const auto sel_varp = get_sel_var(svar_offsetp+jp);
                            const auto& fanins2 = svar_map[svar_offsetp + jp];

                            if (lex_compare(fanins1, fanins2) == 1) {
                                pLits[1] = pabc::Abc_Var2Lit(sel_varp, 1);
                                auto status = solver->add_clause(pLits, pLits+2);
                                assert(status);
                            }
                        }
                    }

                    svar_offset += nr_svars_for_i;
                }
            }

            /*******************************************************************
                Ensure that Boolean operators are lexicographically ordered:
                (S_ijk /\ S_(i+1)jk) ==> f_i <= f_(i+1)
            *******************************************************************/
            void 
            create_lex_func_clauses(const spec& spec)
            {
                std::vector<int> fvar_asgns(spec.fanin);
                int lits[3];

                auto svar_offset = 0;
                for (int i = 0; i < spec.nr_steps - 1; i++) {
                    const auto nr_svars_for_i = nr_svar_map[i];
                    for (int j = 0; j < nr_svars_for_i; j++) {
                        const auto sel_var = get_sel_var(svar_offset + j);
                        const auto& fanins1 = svar_map[svar_offset + j];
                        pabc::Vec_IntSetEntry(vLits, 0, pabc::Abc_Var2Lit(sel_var, 1));
                        
                        auto svar_offsetp = svar_offset + nr_svars_for_i;
                        const auto nr_svars_for_ip = nr_svar_map[i + 1];
                        for (int jp = 0; jp < nr_svars_for_ip; jp++) {
                            const auto sel_varp = get_sel_var(svar_offsetp + jp);
                            const auto& fanins2 = svar_map[svar_offsetp + jp];

                            bool equal_fanin = true;
                            for (int k = 0; k < spec.fanin; k++) {
                                if (fanins1[k] != fanins2[k]) {
                                    equal_fanin = false;
                                    break;
                                }
                            }
                            if (!equal_fanin) {
                                continue;
                            }

                            pabc::Vec_IntSetEntry(vLits, 1, pabc::Abc_Var2Lit(sel_varp, 1));
                            
                            // The steps have the same fanin, so enforce lexicographical order.
                            // We do this by constraining the operator variables of both steps.
                            // Note: the operator variable with the highest index is used 
                            // first in the ordering.
                            for (int op_idx = 0; op_idx < nr_op_vars_per_step; op_idx++) {
                                // Inequality only has to hold if all previous operator variables
                                // are equal.
                                auto ctr = 2;
                                for (int prev_idx = 0; prev_idx < op_idx; prev_idx++) {
                                    const auto prev_alpha_i = get_lex_var(spec, i, prev_idx);
                                    pabc::Vec_IntSetEntry(vLits, ctr++, pabc::Abc_Var2Lit(prev_alpha_i, 1));
                                }

                                // Ensure that f_i_n <= f_{i+1}_n.
                                const auto iop_var = get_op_var(spec, i, nr_op_vars_per_step - op_idx);
                                const auto ipop_var = get_op_var(spec, i + 1, nr_op_vars_per_step - op_idx);
                                pabc::Vec_IntSetEntry(vLits, ctr++, pabc::Abc_Var2Lit(iop_var, 1));
                                pabc::Vec_IntSetEntry(vLits, ctr++, pabc::Abc_Var2Lit(ipop_var, 0));
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
                        }
                    }
                    svar_offset += nr_svars_for_i;
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
                        if (spec.verbosity > 3) {
                            printf("  variables x_%d and x_%d are symmetric\n",
                                    p+1, q+1);
                        }

                        auto svar_offset = 0;
                        for (int i = 0; i < spec.nr_steps; i++) {
                            const auto nr_svars_for_i = nr_svar_map[i];
                            for (int j = 0; j < nr_svars_for_i; j++) {
                                const auto sel_var = get_sel_var(svar_offset+j);
                                const auto& fanins1 = svar_map[svar_offset+j];
                                
                                auto has_fanin_p = false;
                                auto has_fanin_q = false;
                                for (auto fanin : fanins1) {
                                    if (fanin == q) {
                                        has_fanin_q = true;
                                        break;
                                    } else if (fanin == p) {
                                        has_fanin_p = true;
                                    }
                                }
                                if (!has_fanin_q || has_fanin_p) {
                                    continue;
                                }

                                pabc::Vec_IntSetEntry(vLits, 0, 
                                        pabc::Abc_Var2Lit(sel_var, 1));

                                auto ctr = 1;
                                auto svar_offsetp = 0;
                                for (int ip = 0; ip < i; ip++) {
                                    const auto nr_svars_for_ip = nr_svar_map[ip];
                                    for (int jp = 0; jp < nr_svars_for_ip; jp++) {
                                        const auto sel_varp = 
                                            get_sel_var(svar_offsetp + jp);
                                        const auto& fanins2 = 
                                            svar_map[svar_offsetp + jp];

                                        has_fanin_p = false;
                                        for (auto fanin : fanins2) {
                                            if (fanin == p) {
                                                has_fanin_p = true;
                                            }
                                        }
                                        if (!has_fanin_p) {
                                            continue;
                                        }
                                        pabc::Vec_IntSetEntry(vLits, ctr++, 
                                                pabc::Abc_Var2Lit(sel_varp, 0));
                                    }
                                    svar_offsetp += nr_svars_for_ip;
                                }
                                if (!solver->add_clause(Vec_IntArray(vLits), Vec_IntArray(vLits) + ctr)) {
                                    return false;
                                }
                            }
                            svar_offset += nr_svars_for_i;
                        }
                    }
                }

                return true;
            }

            /*******************************************************************
                Ensure that every step has exactly 2 inputs. This may not
                happen e.g. when we synthesize with more than the minimum
                number of steps. (Example: synthesizing n-input OR function,
                with more than the minimum number of steps.)
            *******************************************************************/
            void
            create_cardinality_constraints(const spec& spec)
            {
                int pLits[2];

                auto svar_offset = 0;
                for (int i = 0; i < spec.nr_steps; i++) {
                    const auto nr_svars_for_i = nr_svar_map[i];
                    for (int j = 0; j < nr_svars_for_i - 1; j++) {
                        for (int jp = j + 1; jp < nr_svars_for_i; jp++) {
                            const auto svar1 = get_sel_var(svar_offset + j);
                            const auto svar2 = get_sel_var(svar_offset + jp);

                            pLits[0] = pabc::Abc_Var2Lit(svar1, 1);
                            pLits[1] = pabc::Abc_Var2Lit(svar2, 1);

                            auto status = solver->add_clause(pLits, pLits + 2);
                            assert(status);
                        }
                    }
                    svar_offset += nr_svars_for_i;
                }
                
                for (int h = 0; h < spec.nr_nontriv; h++) {
                    for (int i = 0; i < spec.nr_steps - 1; i++) {
                        for (int ip = i + 1; ip < spec.nr_steps; ip++) {
                            pLits[0] = pabc::Abc_Var2Lit(get_out_var(spec, h, i), 1);
                            pLits[1] = pabc::Abc_Var2Lit(get_out_var(spec, h, ip), 1);
                            auto status = solver->add_clause(pLits, pLits + 2);
                            assert(status);
                        }
                    }
                }
            }

            /// Extracts chain from encoded CNF solution.
            void extract_chain(const spec& spec, chain& chain)
            {
                chain.reset(spec.get_nr_in(), spec.get_nr_out(), spec.nr_steps, spec.fanin);

                auto svar_offset = 0;
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

                    const auto nr_svars_for_i = nr_svar_map[i];
                    for (int j = 0; j < nr_svars_for_i; j++) {
                        const auto sel_var = get_sel_var(svar_offset + j);
                        if (solver->var_value(sel_var)) {
                            const auto& fanins = svar_map[svar_offset + j];
                            if (spec.verbosity) {
                                printf("  with operands ");
                                for (int k = 0; k < spec.fanin; k++) {
                                    printf("x_%d ", fanins[k] + 1);
                                }
                            }
                            chain.set_step(i, fanins, op);
                        }
                    }

                    svar_offset += nr_svars_for_i;

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
                chain.reset(spec.get_nr_in(), spec.get_nr_out(), spec.nr_steps, spec.fanin);

                auto svar_offset = 0;
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

                    const auto nr_svars_for_i = nr_svar_map[i];
                    for (int j = 0; j < nr_svars_for_i; j++) {
                        const auto sel_var = get_sel_var(svar_offset + j);
                        if (solver->var_value(sel_var)) {
                            const auto& fanins = svar_map[svar_offset + j];
                            if (spec.verbosity) {
                                printf("  with operands ");
                                for (int k = 0; k < spec.fanin; k++) {
                                    printf("x_%d ", fanins[k] + 1);
                                }
                            }
                            chain.set_step(i, fanins, op);
                        }
                    }

                    svar_offset += nr_svars_for_i;

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
                auto svar_offset = 0;
                for (int j = 0; j < i; j++) {
                    svar_offset += nr_svar_map[j];
                }
                const auto nr_svars_for_i = nr_svar_map[i];
                for (int j = 0; j < nr_svars_for_i; j++) {
                    const auto sel_var = get_sel_var(svar_offset + j);
                    if (solver->var_value(sel_var)) {
                        const auto& sim_fanins = svar_map[svar_offset + j];
                        for (int k = 0; k < spec.fanin; k++) {
                            fanins[k] = sim_fanins[k];
                        }
                        return;
                    }
                }
                assert(false);
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

            /*******************************************************************
                Extracts only the underlying DAG structure from a solution.
            *******************************************************************/
            template<int FI>
            void 
            extract_dag(const spec& spec, dag<FI>& dag)
            {
                assert(FI == spec.fanin);
                dag.reset(spec.get_nr_in(), spec.nr_steps);

                auto svar_offset = 0;
                for (int i = 0; i < spec.nr_steps; i++) {
                    const auto nr_svars_for_i = nr_svar_map[i];
                    for (int j = 0; j < nr_svars_for_i; j++) {
                        const auto sel_var = get_sel_var(svar_offset + j);
                        if (solver->var_value(sel_var)) {
                            const auto& fanins = svar_map[svar_offset + j];
                            if (spec.verbosity) {
                                printf("  with operands ");
                                for (int k = 0; k < spec.fanin; k++) {
                                    printf("x_%d ", fanins[k] + 1);
                                }
                            }
                            dag.set_vertex(i, fanins);
                        }
                    }
                    svar_offset += nr_svars_for_i;
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

                auto svar_offset = 0;
                for (int i = 0; i < spec.nr_steps; i++) {
                    bool step_has_fanins = false;
                    const auto nr_svars_for_i = nr_svar_map[i];
                    for (int j = 0; j < nr_svars_for_i; j++) {
                        const auto sel_var = get_sel_var(svar_offset + j);
                        if (solver->var_value(sel_var)) {
                            const auto& fanins = svar_map[svar_offset + j];
                            printf("  x_%d has inputs ", 
                                    spec.get_nr_in() + i + 1);
                            for (int k = spec.fanin-1; k >= 0; k--) {
                                printf("x_%d ", fanins[k] + 1);
                            }
                            step_has_fanins = true;
                        }
                    }
                    svar_offset += nr_svars_for_i;
                    if (!step_has_fanins) {
                        printf("  no fanins found for x_%d\n",
                                spec.get_nr_in() + i + 1);
                    }

                    printf("  f_%d = ", spec.get_nr_in()+i+1);
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

                svar_offset = 0;
                for (int i = 0; i < spec.nr_steps; i++) {
                    const auto nr_svars_for_i = nr_svar_map[i];
                    for (int j = 0; j < nr_svars_for_i; j++) {
                        printf("  s_%d", spec.get_nr_in() + i + 1);
                        const auto& fanins = svar_map[j + svar_offset];
                        for (auto fi : fanins) {
                            printf("_%d", fi + 1);
                        }
                        printf("=%d\n", solver->var_value(get_sel_var(j + svar_offset)));
                    }
                    printf("\n");

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
                                solver->var_value(get_sim_var(spec, i, t)));
                    }
                    printf("  x_%d_0=0\n", spec.get_nr_in() + i + 1);
                    printf("\n");

                    svar_offset += nr_svars_for_i;
                }
                printf("\n");

                printf("========================================"
                        "========================================\n");
            }

			/// Encodes specifciation for use in standard synthesis flow.
            bool encode(const spec& spec)
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
                
                create_cardinality_constraints(spec);

                if (spec.is_primitive_set()) {
                    if (!create_primitive_clauses(spec))
                        return false;
                } else if (spec.add_nontriv_clauses) {
                    create_nontriv_clauses(spec);
                }

                if (spec.add_alonce_clauses) {
                    create_alonce_clauses(spec);
                }

                if (!spec.is_primitive_set() &&
                    spec.add_noreapply_clauses) {
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
            bool cegar_encode(const spec& spec)
            {
                assert(spec.nr_steps <= MAX_STEPS);

                create_variables(spec);
                
                if (!create_output_clauses(spec)) {
                    return false;
                }
                
                if (!create_op_clauses(spec)) {
                    return false;
                }
                
                create_cardinality_constraints(spec);

                if (spec.is_primitive_set()) {
                    if (!create_primitive_clauses(spec))
                        return false;
                } else if (spec.add_nontriv_clauses) {
                    create_nontriv_clauses(spec);
                }

                if (spec.add_alonce_clauses) {
                    create_alonce_clauses(spec);
                }
                
                if (!spec.is_primitive_set() && spec.add_noreapply_clauses) {
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
                int svar_offset = 0;

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

                    const auto nr_svars_for_i = nr_svar_map[i];
                    for (int j = 0; j < nr_svars_for_i; j++) {
                        const auto sel_var = get_sel_var(svar_offset + j);
                        if (solver->var_value(sel_var)) {
                            pabc::Vec_IntSetEntry(vLits, ctr++,
                                    pabc::Abc_Var2Lit(sel_var, 1));
                            break;
                        }
                    }

                    svar_offset += nr_svars_for_i;
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
                int svar_offset = 0;

                for (int i = 0; i < spec.nr_steps; i++) {
                    const auto nr_svars_for_i = nr_svar_map[i];
                    for (int j = 0; j < nr_svars_for_i; j++) {
                        const auto sel_var = get_sel_var(svar_offset + j);
                        if (solver->var_value(sel_var)) {
                            pabc::Vec_IntSetEntry(vLits, ctr++,
                                    pabc::Abc_Var2Lit(sel_var, 1));
                            break;
                        }
                    }

                    svar_offset += nr_svars_for_i;
                }

                return solver->add_clause(
                            pabc::Vec_IntArray(vLits), 
                            pabc::Vec_IntArray(vLits) + ctr);
            }
    };
}

