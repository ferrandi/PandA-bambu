#pragma once

#include <vector>
#include <cassert>
#include <iostream>
#include <memory>

#include "dag.hpp"
#include "spec.hpp"
#include "misc.hpp"

/*******************************************************************************
    Definition of Boolean chain. A Boolean chain is a sequence of steps. Each
    step applies a Boolean operator to a fixed number of previous steps. The
    main objective of this package is to synthesize Boolean chains efficiently.
*******************************************************************************/
namespace percy
{
	using kitty::dynamic_truth_table;
	using kitty::create_nth_var;

    class chain
    {
        private:
            int nr_in;
            int fanin;
            int op_tt_size; // The truth table size of operands in the chain (depends on fanin)
            std::vector<dynamic_truth_table> compiled_functions;
            std::vector<std::vector<int>> steps;
            std::vector<dynamic_truth_table> operators;
            std::vector<int> outputs;

        public:
            chain() 
            { 
                reset(0, 0, 0, 0);
            }
            chain(const chain& c) 
            {
                copy(c);
            }

            /*
            chain(chain&& c)
            {
                vertices = std::move(c.vertices);
                operators = std::move(c.operators);
                outputs = std::move(c.outputs);
            }
            */

            void reset(int nr_in, int nr_out, int nr_steps, int fanin)
            {
                assert(nr_steps >= 0);
                assert(fanin <= MAX_FANIN);

                this->nr_in = nr_in;
                this->fanin = fanin;
                this->op_tt_size = (1 << fanin);
                steps.resize(nr_steps);
                for (auto& step : steps) {
                    step.resize(fanin);
                }
                operators.resize(nr_steps);
                outputs.resize(nr_out);
            }

            int get_fanin() const { return fanin; }
            int get_nr_steps() const { return steps.size(); }
            int get_nr_inputs() const { return nr_in; }
            int get_nr_outputs() const { return outputs.size(); }
            const std::vector<int>& get_step(int i) const { return steps[i]; }
            const std::vector<int>& get_outputs() const { return outputs; }

            const dynamic_truth_table& get_operator(int i) const
            {
              return operators.at(i);
            }

            std::vector<int>& get_outputs() { return outputs; }

            bool
            is_output_inverted(int out_idx)
            {
                return outputs[out_idx] & 1;
            }

            void set_compiled_functions( std::vector<dynamic_truth_table> const& fs )
            {
              compiled_functions = fs;
            }

            void
            set_step(int i, const int* const in, const dynamic_truth_table& op)
            {
                for (int j = 0; j < fanin; j++) {
                    steps[i][j] = in[j];
                }
                operators[i] = op;
            }

            void
            set_step(int i, int fanin1, int fanin2, const dynamic_truth_table& op)
            {
                steps[i][0] = fanin1;
                steps[i][1] = fanin2;
                operators[i] = op;
            }

            void
            set_step(int i, const std::vector<int>& in, const dynamic_truth_table& op)
            {
                for (int j = 0; j < fanin; j++) {
                    steps[i][j] = in[j];
                }
                operators[i] = op;
            }

            void
            add_step(const int* const in, const dynamic_truth_table& op)
            {
                std::vector<int> step(fanin);
                for (int j = 0; j < fanin; j++) {
                    step[j] = in[j];
                }
                steps.push_back(step);
                operators.push_back(op);
            }

            void
            add_step(const std::vector<int>& in, const dynamic_truth_table& op)
            {
                std::vector<int> step(fanin);
                for (int j = 0; j < fanin; j++) {
                    step[j] = in[j];
                }
                steps.push_back(step);
                operators.push_back(op);
            }

            void
            set_output(int out_idx, int lit)
            {
                outputs[out_idx] = lit;
            }

            void
            set_output(int out_idx, int step, bool invert)
            {
                outputs[out_idx] = (step << 1) | invert;
            }

            void 
            invert() 
            {
                for (auto i = 0u; i < outputs.size(); i++) {
                    outputs[i] = (outputs[i] ^ 1);
                }
            }
            
            /// De-normalizes a chain, meaning that all outputs will be
            /// converted to non-complemented edges. This may mean that some
            /// shared steps have to be duplicated or replaced by NOT gates.
            /// NOTE: some outputs may point to constants or PIs. These will
            /// not be changed.
            void denormalize()
            {
                // Does nothing if there are no steps to push inverters into.
                if (steps.size() == 0) {
                    return;
                }
                
                if (outputs.size() == 1) {
                    if (outputs[0] & 1) {
                        operators[steps.size() - 1] = ~operators[steps.size() - 1];
                        outputs[0] = (outputs[0] ^ 1);
                    }
                    return;
                }

                std::vector<int> refcount(steps.size());
                std::vector<dynamic_truth_table> tmps(steps.size());
                std::vector<dynamic_truth_table> ins;
                std::vector<dynamic_truth_table> fs(outputs.size());

                for (auto i = 1u; i < steps.size(); i++) {
                    const auto& v = steps[i];
                    for (const auto fid : v) {
                        if (fid > nr_in) {
                            refcount[fid - nr_in - 1]++;
                        }
                    }
                }

                for (auto i = 0u; i < outputs.size(); i++) {
                    const auto step_idx = outputs[i] >> 1;
                    if (step_idx > nr_in) {
                        refcount[step_idx - nr_in - 1]++;
                    }
                }

                for (auto count : refcount) {
                    assert(count > 0);
                }

                for (auto i = 0; i < nr_in; ++i) {
                    auto in_tt = kitty::create<dynamic_truth_table>(nr_in);
                    ins.push_back(in_tt);
                }

                auto tt_step = kitty::create<dynamic_truth_table>(nr_in);
                auto tt_compute = kitty::create<dynamic_truth_table>(nr_in);
                
                for (auto i = 0u; i < steps.size(); i++) {
                    const auto& step = steps[i];

                    for (int j = 0; j < fanin; j++) {
                        const auto fanin = step[j];
                        if (fanin < nr_in) {
                            create_nth_var(ins[j], fanin);
                        } else {
                            ins[j] = tmps[fanin - nr_in];
                        }
                    }

                    kitty::clear(tt_step);
                    for (int j = 1; j < op_tt_size; j++) {
                        kitty::clear(tt_compute);
                        tt_compute = ~tt_compute;
                        if (get_bit(operators[i], j)) {
                            for (int k = 0; k < fanin; k++) {
                                if ((j >> k) & 1) {
                                    tt_compute &= ins[k];
                                } else {
                                    tt_compute &= ~ins[k];
                                }
                            }
                            tt_step |= tt_compute;
                        }
                    }
                    tmps[i] = tt_step;

                    for (auto h = 0u; h < outputs.size(); h++) {
                        const auto out = static_cast<unsigned>(outputs[h]);
                        const auto var = out >> 1u;
                        const auto inv = out & 1u;
                        if (var - nr_in - 1u == i) {
                            fs[h] = inv ? ~tt_step : tt_step;
                        }
                    }
                }
                
                for (auto i = 0u; i < outputs.size(); i++) {
                    auto step_idx = outputs[i] >> 1;
                    const auto invert = outputs[i] & 1;

                    if (!invert || step_idx <= nr_in) {
                        continue;
                    }

                    step_idx -= (nr_in + 1);
                    assert(refcount[step_idx] >= 1);
                    if (refcount[step_idx] == 1) {
                        operators[step_idx] = ~operators[step_idx];
                    } else {
                        // This output points to a shared step that needs to
                        // be inverted. If no inverted version of this step
                        // exists somewhere in the chain, we need to add a new
                        // step.
                        bool inv_step_found = false;
                        for (auto j = 0u; j < steps.size(); j++) {
                            if (tmps[j] == fs[i]) {
                                set_output(i, j + nr_in + 1, false);
                                inv_step_found = true;
                            }
                        }
                        if (inv_step_found) {
                            continue;
                        }
                        std::vector<int> fanins(fanin);
                        const auto& v = steps[step_idx];
                        for (int j = 0; j < fanin; j++) {
                            fanins[j] = v[j];
                        };

                        add_step(fanins, ~operators[step_idx]);
                        set_output(i, nr_in + steps.size(), false);
                        tmps.push_back(fs[i]);

                        refcount[step_idx]--;
                    }
                }
            }

            /*******************************************************************
                Derive truth tables from the chain, one for each output.
            *******************************************************************/
            std::vector<dynamic_truth_table> simulate() const
            {
                std::vector<dynamic_truth_table> fs(outputs.size());
                std::vector<dynamic_truth_table> tmps(steps.size());
                std::vector<dynamic_truth_table> ins;

                for ( auto i = 0; i < nr_in; ++i ) {
                    ins.push_back( kitty::create<dynamic_truth_table>( nr_in ) );
                }

                auto tt_step = kitty::create<dynamic_truth_table>(nr_in);
                auto tt_compute = kitty::create<dynamic_truth_table>(nr_in);

                // Some outputs may be simple constants or projections.
                for (auto h = 0u; h < outputs.size(); h++)
                {
                    const auto out = outputs[h];
                    const auto var = out >> 1;
                    const auto inv = out & 1;
                    if (var == 0) {
                        clear(tt_step);
                        fs[h] = inv ? ~tt_step : tt_step;
                    } else if (var <= nr_in) {
                        create_nth_var(tt_step, var-1, inv);
                        fs[h] = tt_step;
                    }
                }

                for (auto i = 0u; i < steps.size(); i++) {
                  const auto& step = steps[i];

                  for ( int j = 0; j < fanin; ++j )
                  {
                    const auto fanin = step[j];
                    // std::cout << fanin << ' ';
                    if ( fanin < nr_in )
                    {
                      create_nth_var(ins[j], fanin);
                    }
                    else if ( fanin < nr_in + compiled_functions.size() )
                    {
                      ins[j] = compiled_functions.at( fanin - nr_in );
                    }
                    else
                    {
                      ins[j] = tmps[fanin - nr_in - compiled_functions.size()];
                    }
                    // kitty::print_binary( ins[j] );
                    // std::cout << std::endl;
                  }

                  kitty::clear(tt_step);
                  for ( int j = 0; j < op_tt_size; ++j )
                  {
                    kitty::clear(tt_compute);
                    tt_compute = ~tt_compute;
                    if ( get_bit( operators[i], j ) )
                    {
                      for ( int k = 0; k < fanin; ++k)
                      {
                        if ((j >> k) & 1)
                        {
                          tt_compute &= ins[k];
                        }
                        else
                        {
                          tt_compute &= ~ins[k];
                        }
                      }
                      tt_step |= tt_compute;
                    }
                  }
                  // std::cout << "step = ";
                  // kitty::print_binary( tt_step );
                  // std::cout << std::endl;
                  tmps[i] = tt_step;

                  for ( auto h = 0u; h < outputs.size(); ++h )
                  {
                    const auto out = outputs[h];
                    const auto var = out >> 1;
                    const auto inv = out & 1;
                    if (var - nr_in - compiled_functions.size() - 1 == static_cast<int>(i)) {
                      fs[h] = inv ? ~tt_step : tt_step;
                    }
                  }
                }

                return fs;
            }


            /*******************************************************************
                Checks if a chain satisfies the given specification. This
                checks not just if the chain computes the correct function, but
                also other requirements such as co-lexicographic order (if
                specified).
            *******************************************************************/
            bool
            satisfies_spec(const spec& spec)
            {
                if (spec.nr_triv == spec.get_nr_out()) {
                    return true;
                }
                auto tts = simulate();
                dynamic_truth_table op_tt(fanin);

                for (auto& step : steps) {
                    if (step.size() != static_cast<unsigned>(spec.fanin)) {
                        assert(false);
                        return false;
                    }
                }

                auto nr_nontriv = 0;
                for (int i = 0; i < spec.nr_nontriv; i++) {
                    if ((spec.triv_flag >> i) & 1) {
                        continue;
                    }
                    if (tts[nr_nontriv++] != spec[i]) {
                        assert(false);
                        return false;
                    }
                }

                if (spec.add_nontriv_clauses) {
                    // Ensure that there are no trivial operators.
                    for (auto& op : operators) {
                        clear(op_tt);
                        if (op == op_tt) {
                            assert(false);
                            return false;
                        }
                        for (int i = 0; i < fanin; i++) {
                            create_nth_var(op_tt, i);
                            if (op == op_tt) {
                                assert(false);
                                return false;
                            }
                        }
                    }
                }

                if ( spec.add_alonce_clauses )
                {
                  /* Ensure that each step is used at least once. */
                  std::vector<int32_t> nr_uses( steps.size() );

                  for ( auto i = 0u; i < steps.size(); ++i )
                  {
                    auto const& step = steps[i];
                    for ( const auto& fid : step )
                    {
                      if ( fid >= nr_in + compiled_functions.size() )
                      {
                        nr_uses[fid - nr_in - compiled_functions.size()]++;
                      }
                    }
                  }

                  for ( auto const& output : outputs )
                  {
                    auto const step_idx = output >> 1;
                    if ( step_idx > nr_in + compiled_functions.size() )
                    {
                      nr_uses[step_idx - nr_in - compiled_functions.size() - 1]++;
                    }
                  }

                  for ( auto const& nr : nr_uses )
                  {
                    if ( nr == 0 )
                    {
                      assert( false );
                      return false;
                    }
                  }
                }

                if (spec.add_noreapply_clauses) {
                    // Ensure there is no re-application of operands.
                    for (auto i = 0u; i < steps.size() - 1; i++) {
                        const auto & fanins1 = steps[i];
                        for (auto ip = i + 1; ip < steps.size(); ip++) {
                            const auto& fanins2 = steps[ip];

                            auto is_subsumed = true;
                            auto has_fanin_i = false;
                            for (auto j : fanins2) {
                                if (static_cast<unsigned>(j) == i + nr_in) {
                                    has_fanin_i = true;
                                    continue;
                                }
                                auto is_included = false;
                                for (auto jp : fanins1) {
                                    if (j == jp) {
                                        is_included = true;
                                    }
                                }
                                if (!is_included) {
                                    is_subsumed = false;
                                }
                            }
                            if (is_subsumed && has_fanin_i) {
                                assert(false);
                                return false;
                            }
                        }
                    }
                }

                if (spec.add_colex_clauses) {
                    // Ensure that steps are in co-lexicographical order.
                    for (int i = 0; i < spec.nr_steps - 1; i++) {
                        const auto& v1 = steps[i];
                        const auto& v2 = steps[i + 1];
                        
                        if (colex_compare(v1, v2) == 1) {
                            assert(false);
                            return false;
                        }
                    }
                }

                if (spec.add_lex_clauses) {
                    // Ensure that steps are in lexicographical order.
                    for (int i = 0; i < spec.nr_steps - 1; i++) {
                        const auto& v1 = steps[i];
                        const auto& v2 = steps[i + 1];
                        
                        if (lex_compare(v1, v2) == 1) {
                            assert(false);
                            return false;
                        }
                    }
                }

                if (spec.add_lex_func_clauses) {
                    // Ensure that step operators are in lexicographical order.
                    for (int i = 0; i < spec.nr_steps - 1; i++) {
                        const auto& v1 = steps[i];
                        const auto& v2 = steps[i + 1];

                        if (colex_compare(v1, v2) == 0) {
                            // The operator of step i must be lexicographically
                            // less than that of i + 1.
                            const auto& op1 = operators[i];
                            const auto& op2 = operators[i + 1];
                            if (op2 < op1) {
                                assert(false);
                                return false;
                            }
                        }
                    }
                }

                if (spec.add_symvar_clauses) {
                    // Ensure that symmetric variables are ordered.
                    for (int q = 1; q < spec.get_nr_in(); q++) {
                        for (int p = 0; p < q; p++) {
                            auto symm = true;
                            for (int i = 0; i < spec.get_nr_out(); i++) {
                                auto outfunc = spec[i];
                                if (!(swap(outfunc, p, q) == outfunc)) {
                                    symm = false;
                                    break;
                                }
                            }
                            if (!symm) {
                                continue;
                            }
                            for (int i = 1; i < spec.nr_steps; i++) {
                                const auto& v1 = steps[i];
                                auto has_fanin_p = false;
                                auto has_fanin_q = false;

                                for (const auto fid : v1) {
                                    if (fid == p) {
                                        has_fanin_p = true;
                                    } else if (fid == q) {
                                        has_fanin_q = true;
                                    }
                                }

                                if (!has_fanin_q || has_fanin_p) {
                                    continue;
                                }
                                auto p_in_prev_step = false;
                                for (int ip = 0; ip < i; ip++) {
                                    const auto& v2 = steps[ip];
                                    has_fanin_p = false;

                                    for (const auto fid : v2) {
                                        if (fid == p) {
                                            has_fanin_p = true;
                                        }
                                    }
                                    if (has_fanin_p) {
                                        p_in_prev_step = true;
                                    }   
                                }
                                if (!p_in_prev_step) {
                                    assert(false);
                                    return false;
                                }
                            }
                        }
                    }
                }

                return true;
            }

            bool is_aig()
            {
                if (fanin != 2) {
                    return false;
                }
                kitty::dynamic_truth_table in1(2), in2(2);
                kitty::create_nth_var(in1, 0);
                kitty::create_nth_var(in2, 1);
                const auto tt1 = in1 & in2;
                const auto tt2 = ~in1 & in2;
                const auto tt3 = in1 & ~in2;
                const auto tt4 = in1 | in2;
                for (const auto& op : operators) {
                    if (op != tt1 && op != tt2 && op != tt3 && op != tt4) {
                        return false;
                    }
                }
                return true;
            }

            bool is_mag()
            {
                if (fanin != 3) {
                    return false;
                }
                kitty::dynamic_truth_table maj_tt(3);
                kitty::create_majority(maj_tt);

                for (const auto& op : operators) {
                    if (op != maj_tt) {
                        return false;
                    }
                }
                return true;
            }

            void
            copy(const chain& c)
            {
                nr_in = c.nr_in;
                fanin = c.fanin;
                op_tt_size = c.op_tt_size;
                steps = c.steps;
                operators = c.operators;
                outputs = c.outputs;
            }

            /*******************************************************************
                Creates a DAG from the Boolean chain in .dot format, so that
                it may be rendered using various DAG packages (e.g. graphviz).
            *******************************************************************/
            void
            to_dot(std::ostream& s) /* NOTE: deprecated impl. */
            {
                s << "graph {\n";
                s << "rankdir = BT\n";
                s << "x0 [shape=none,label=<\u22A5>];\n";
                for (int i = 0; i < nr_in; i++) {
                    const auto idx = i + 1;
                    s << "x" << idx << " [shape=none,label=<x<sub>" << idx
                        << "</sub>>];\n";
                }

                s << "node [shape=circle];\n";
                for (size_t i = 0; i < steps.size(); i++) {
                    const auto& step = steps[i];
                    const auto idx = nr_in + i + 1;
                    s << "x" << idx << " [label=<";
                    kitty::print_binary(operators[i], s);
                    s << ">];\n";
                    for (int j = 0; j < fanin; j++) {
                        s << "x" << step[j]+1 << " -- x" << idx << ";\n";
                    }
                }

                for (size_t h = 0u; h < outputs.size(); h++) {
                    const auto out = outputs[h];
                    const auto inv = out & 1;
                    const auto var = out >> 1;
                    s << "f" << h << " [shape=none,label=<f<sub>" << h+1
                        << "</sub>>];\n";
                    s << "x" << var << " -- f" << h << " [style=" <<
                        (inv ? "dashed" : "solid") << "];\n";
                }

                // Group inputs on same level.
                s << "{rank = same; x0; ";
                for (int i = 0; i < nr_in; i++) {
                    s << "x" << i+1 << "; ";
                }
                s << "}\n";

                // Group outputs on same level.
                s << "{rank = same; ";
                for (size_t h = 0u; h < outputs.size(); h++) {
                    s << "f" << h << "; ";
                }
                s << "}\n";

                // Add invisible edges between PIs and POs to enforce order.
                s << "edge[style=invisible];\n";
                for (int i = nr_in; i > 0; i--) {
                    s << "x" << i-1 << " -- x" << i << ";\n";
                }
                for (size_t h = outputs.size(); h > 1; h--) {
                    s << "f" << h-2 << " -- f" << h-1 << ";\n";
                }

                s << "}\n";

            }

            void
            print_dot()
            {
                to_dot(std::cout);
            }

            /*******************************************************************
                Functions to convert the chain to a parseable expression.
                Currently only supported for single-output normal chains with
                2-input operators.
            *******************************************************************/
            void
            step_to_expression(std::ostream& s, int step_idx)
            {
                assert(fanin == 2);

                if (step_idx < nr_in) {
                    s << char(('a' + step_idx));
                    return;
                }
                const auto& step = steps[step_idx - nr_in];
                auto word = 0;
                for (int i = 0; i < 4; i++) {
                    if (kitty::get_bit(operators[step_idx - nr_in], i)) {
                        word |= (1 << i);
                    }
                }
                assert(word <= 15);
                switch (word) {
                    case 2:
                        s << "(";
                        step_to_expression(s, step[0]);
                        s << "!";
                        step_to_expression(s, step[1]);
                        s << ")";
                        break;
                    case 4:
                        s << "(";
                        s << "!";
                        step_to_expression(s, step[0]);
                        step_to_expression(s, step[1]);
                        s << ")";
                        break;
                    case 6:
                        s << "[";
                        step_to_expression(s, step[0]);
                        step_to_expression(s, step[1]);
                        s << "]";
                        break;
                    case 8:
                        s << "(";
                        step_to_expression(s, step[0]);
                        step_to_expression(s, step[1]);
                        s << ")";
                        break;
                    case 14:
                        s << "{";
                        step_to_expression(s, step[0]);
                        step_to_expression(s, step[1]);
                        s << "}";
                        break;
                    default:
                        // Invalid operator detected.
                        printf("Invalid operator %d\n", word);
                        assert(0);
                        break;
                }
            }

            void
            to_expression(std::ostream& s)
            {
                assert(outputs.size() == 1 && fanin == 2);
                const auto outlit = outputs[0];
                if (outlit & 1) {
                    s << "!";
                }
                const auto outvar = outlit >> 1;
                if (outvar == 0) { // Special case of constant 0
                    s << "0";
                } else {
                    step_to_expression(s, outvar-1);
                }
            }

            void
            print_expression()
            {
                to_expression(std::cout);
            }

            void step_to_mag_expression(std::ostream& s, int step_idx)
            {
                assert(fanin == 3);

                if (step_idx < nr_in) {
                    s << char(('a' + step_idx));
                    return;
                }
                const auto& step = get_step(step_idx - nr_in);
                s << "<";
                step_to_mag_expression(s, step[0]);
                step_to_mag_expression(s, step[1]);
                step_to_mag_expression(s, step[2]);
                s << ">";
            }

            void to_mag_expression(std::ostream& s)
            {
                assert(outputs.size() == 1 && fanin == 3);
                const auto outlit = outputs[0];
                const auto outvar = outlit >> 1;
                if (outvar == 0) { // Special case of constant 0
                    s << "0";
                } else {
                    step_to_mag_expression(s, outvar-1);
                }
            }

            void print_mag()
            {
                std::cout << steps.size() << "-step MAJ chain\n";
                to_mag_expression(std::cout);
                std::cout << "\n";
            }

            template<int FI>
            void
            extract_dag(dag<FI>& g) const
            {
                g.copy_dag(*this);
            }

    };

}

#include "printer.hpp"
