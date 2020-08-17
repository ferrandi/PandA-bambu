#pragma once

#include "encoder.hpp"
#include "../io.hpp"

namespace percy
{
    template<int FI>
    class ssv_dag_encoder : public dag_encoder<FI>
    {
        protected:
            int nr_op_vars;
            int nr_sim_vars;
            int nr_op_vars_per_step;

            pabc::Vec_Int_t* vLits; // Dynamic vector of literals

        public:
            ssv_dag_encoder()
            {
                vLits = pabc::Vec_IntAlloc(128);
            }

            ~ssv_dag_encoder()
            {
                pabc::Vec_IntFree(vLits);
            }

            int
            get_op_var(const spec& spec, int step_idx, int var_idx)
            {
                assert(step_idx < spec.nr_steps);
                assert(var_idx > 0);
                assert(var_idx <= nr_op_vars_per_step);

                return step_idx * nr_op_vars_per_step + var_idx - 1;
            }

            int
            get_sim_var(const spec& spec, const dag<FI>& dag, int step_idx, int t)
            {
                assert(step_idx < dag.get_nr_vertices());

                return nr_op_vars + spec.get_tt_size() * step_idx + t;
            }

            void
            create_variables(const spec& spec, const dag<FI>& dag)
            {
                nr_op_vars_per_step = ((1u << FI) - 1);
                nr_op_vars = dag.get_nr_vertices() * nr_op_vars_per_step;
                nr_sim_vars = dag.get_nr_vertices() * spec.get_tt_size();
                if (spec.verbosity > 1) {
                    printf("nr_op_vars_per_step=%d\n", nr_op_vars_per_step);
                    printf("nr_op_vars=%d\n", nr_op_vars);
                    printf("nr_sim_vars=%d\n", nr_sim_vars);
                }

                this->solver->set_nr_vars(nr_op_vars + nr_sim_vars);
            }

            bool
            add_simulation_clause(
                    const spec& spec,
                    const dag<FI>& dag,
                    const int t, 
                    const int i, 
                    const int output, 
                    const int opvar_idx,
                    const int* const fanins,
                    const std::vector<int>& fanin_asgn)
            {
                int ctr = 0;

                for (int j = 0; j < FI; j++) {
                    auto child = fanins[j];
                    auto assign = fanin_asgn[j];
                    if (child < spec.get_nr_in()) {
                        if (( ( (t + 1) & (1 << child) ) ? 1 : 0 ) != assign) {
                            return true;
                        }
                    } else {
                        pabc::Vec_IntSetEntry(vLits, ctr++, pabc::Abc_Var2Lit(
                                    get_sim_var(spec, dag, child -
                                        spec.get_nr_in(), t), assign));
                    }
                }

                pabc::Vec_IntSetEntry(vLits, ctr++,
                        pabc::Abc_Var2Lit(get_sim_var(spec, dag, i, t), output));

                if (opvar_idx > 0) {
                    pabc::Vec_IntSetEntry(vLits, ctr++, pabc::Abc_Var2Lit(
                                get_op_var(spec, i, opvar_idx), 1 - output));
                }

                auto status =  this->solver->add_clause(
                        pabc::Vec_IntArray(vLits), 
                        pabc::Vec_IntArray(vLits) + ctr); 

                if (spec.verbosity > 2) {
                    printf("creating sim. clause: (");
                    printf(" %sx_%d_%d ", output ? "!" : "", 
                            spec.get_nr_in() + i + 1, t + 2);

                    for (int j = 0; j < FI; j++) {
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

            bool
            create_tt_clauses(const spec& spec, const dag<FI>& dag, int t)
            {
                int fanins[FI];
                std::vector<int> fanin_asgn(FI);

                for (int i = 0; i < dag.get_nr_vertices(); i++) {
                    auto v = dag.get_vertex(i);
                    dag.foreach_fanin(v, [&fanins] (auto f_id, int j) {
                        fanins[j] = f_id;
                    });

                    // First add clauses for all cases where the operator i
                    // computes zero.
                    int opvar_idx = 0;
                    clear_assignment(fanin_asgn);
                    while (true) {
                        next_assignment(fanin_asgn);
                        if (is_zero(fanin_asgn)) {
                            break;
                        }
                        opvar_idx++;
                        if (!add_simulation_clause(spec, dag, t, i, 0,
                                    opvar_idx, fanins, fanin_asgn)) {
                            return false;
                        }
                    }

                    // Next, all cases where operator i computes one.
                    opvar_idx = 0;
                    if (!add_simulation_clause(spec, dag, t, i, 1, opvar_idx,
                                fanins, fanin_asgn)) {
                        return false;
                    }
                    while (true) {
                        next_assignment(fanin_asgn);
                        if (is_zero(fanin_asgn)) {
                            break;
                        }
                        opvar_idx++;
                        if (!add_simulation_clause(spec, dag, t, i, 1, opvar_idx,
                                fanins, fanin_asgn)) {
                            return false;
                        }
                    }

                    // If an output has selected this particular operand, we
                    // need to ensure that this operand's truth table satisfies
                    // the specified output function.
                    if (i == dag.get_nr_vertices()-1) {
                        int pLits[1];
                        auto outbit = kitty::get_bit(spec[0], t+1);
                        if (spec.out_inv & 1) {
                            outbit = 1 - outbit;
                        }
                        pLits[0] = pabc::Abc_Var2Lit(get_sim_var(spec, dag, i,
                                    t), 1 - outbit);
                        if (!this->solver->add_clause(pLits, pLits + 1)) {
                            return false;
                        }

                        if (spec.verbosity > 1) {
                            printf("bit %d=%lu", t+2, outbit);
                            printf("\tvar=%d\n", get_sim_var(spec, dag, i, t));
                        }
                    }
                }

                return true;
            }

            bool
            create_main_clauses(const spec& spec, const dag<FI>& dag)
            {
                for (int t = 0; t < spec.get_tt_size(); t++) {
                    if (!create_tt_clauses(spec, dag, t)) {
                        return false;
                    }
                }
                return true;
            }

			/// Encodes specifciation for use in standard synthesis flow.
            bool 
            encode(const spec& spec, const dag<FI>& dag)
            {
                bool success = true;
                create_variables(spec, dag);
                success &= create_main_clauses(spec, dag);
                // success &= create_nontriv_clauses();
                return success;
            }

			/// Encodes specifciation for use in CEGAR based synthesis flow.
            bool 
            cegar_encode(const spec& spec, const dag<FI>& dag)
            {
                create_variables(spec, dag);
                return true;
            }

            /// Extracts chain from encoded CNF solution.
            void 
            extract_chain(
                    const spec& spec, 
                    const dag<2>& dag, 
                    chain& chain)
            {
                std::vector<int> op_inputs(spec.fanin);

                chain.reset(spec.get_nr_in(), 1, spec.nr_steps, 2);

                for (int i = 0; i < spec.nr_steps; i++) {
                    kitty::dynamic_truth_table op(2);
                    for (int j = 1; j <= nr_op_vars_per_step; j++) {
                        if (this->solver->var_value(get_op_var(spec, i, j))) {
                            kitty::set_bit(op, j); 
                        }
                    }

                    if (spec.verbosity) {
                        printf("  step x_%d performs operation\n  ", 
                                i+spec.get_nr_in()+1);
                        kitty::print_binary(op, std::cout);
                        printf("\n");
                    }

                    const auto& v = dag.get_vertex(i);
                    dag.foreach_fanin(v, [&op_inputs](auto f_id, int j) {
                        op_inputs[j] = f_id;
                    });
                    chain.set_step(i, op_inputs, op);
                        
                    if (spec.verbosity) {
                        printf("\n");
                    }
                }


                // TODO: support multiple outputs
                chain.set_output(0, 
                        ((spec.nr_steps + spec.get_nr_in()) << 1) +
                        ((spec.out_inv) & 1));

                /*
                auto triv_count = 0, nontriv_count = 0;
                for (int h = 0; h < spec.get_nr_out(); h++) {
                    if ((spec.triv_flag >> h) & 1) {
                        chain.set_output(h, (spec.triv_functions[triv_count++] << 1) +
                                ((spec.out_inv >> h) & 1));
                        continue;
                    }
                    for (int i = 0; i < spec.nr_steps; i++) {
                        if (solver_var_value(this->solver, 
                                    get_out_var(spec, nontriv_count, i))) {
                            chain.set_output(h, ((i + spec.get_nr_in() + 1) << 1) +
                                    ((spec.out_inv >> h) & 1));
                            nontriv_count++;
                            break;
                        }
                    }
                }
                */
            }

            /// Assumes that a solution was found by the synthesizer. In that
            /// case, we can block the current solution by blocking the current
            /// assignment to the operator variables.
            void
            block_solution()
            {
                // TODO: implement
            }
    };
}

