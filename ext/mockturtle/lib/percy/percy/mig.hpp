#pragma once

#include <array>

namespace percy
{
    class mig
    {
    private:
        int nr_in;
        std::vector<int> outputs;
        using step = std::array<int, 3>;

        void rec_to_expression(std::ostream& o, const int i)
        {
            if (i < nr_in) {
                o << static_cast<char>('a' + i);
            } else {
                const auto& step = steps[i - nr_in];
                o << "<";
                rec_to_expression(o, step[0]);
                rec_to_expression(o, step[1]);
                rec_to_expression(o, step[2]);
                o << ">";
            }
        }

    public:
        std::vector<std::array<int, 3>> steps;
        std::vector<int> operators;

        mig()
        {
            reset(0, 0, 0);
        }

        void reset(int _nr_in, int _nr_out, int _nr_steps)
        {
            assert(_nr_steps >= 0 && _nr_out >= 0);
            nr_in = _nr_in;
            steps.resize(_nr_steps);
            operators.resize(_nr_steps);
            outputs.resize(_nr_out);
        }

        int get_nr_steps() const { return steps.size(); }

        std::vector<dynamic_truth_table> simulate() const
        {
            std::vector<dynamic_truth_table> fs(outputs.size());
            std::vector<dynamic_truth_table> tmps(steps.size());

            kitty::dynamic_truth_table tt_in1(nr_in);
            kitty::dynamic_truth_table tt_in2(nr_in);
            kitty::dynamic_truth_table tt_in3(nr_in);


            auto tt_step = kitty::create<dynamic_truth_table>(nr_in);
            auto tt_inute = kitty::create<dynamic_truth_table>(nr_in);

            // Some outputs may be simple constants or projections.
            for (auto h = 0u; h < outputs.size(); h++) {
                const auto out = outputs[h];
                const auto var = out >> 1;
                const auto inv = out & 1;
                if (var == 0) {
                    clear(tt_step);
                    fs[h] = inv ? ~tt_step : tt_step;
                } else if (var <= nr_in) {
                    create_nth_var(tt_step, var - 1, inv);
                    fs[h] = tt_step;
                }
            }

            for (auto i = 0u; i < steps.size(); i++) {
                const auto& step = steps[i];

                if (step[0] < nr_in) {
                    create_nth_var(tt_in1, step[0]);
                } else {
                    tt_in1 = tmps[step[0] - nr_in];
                }
                if (step[1] < nr_in) {
                    create_nth_var(tt_in2, step[1]);
                } else {
                    tt_in2 = tmps[step[1] - nr_in];
                }
                if (step[2] < nr_in) {
                    create_nth_var(tt_in3, step[2]);
                } else {
                    tt_in3 = tmps[step[2] - nr_in];
                }

                kitty::clear(tt_step);
                switch (operators[i]) {
                case 0:
                    tt_step = (tt_in1 & tt_in2) | (tt_in1 & tt_in3) | (tt_in2 & tt_in3);
                    break;
                case 1:
                    tt_step = (~tt_in1 & tt_in2) | (~tt_in1 & tt_in3) | (tt_in2 & tt_in3);
                    break;
                case 2:
                    tt_step = (tt_in1 & ~tt_in2) | (tt_in1 & tt_in3) | (~tt_in2 & tt_in3);
                    break;
                case 3:
                    tt_step = (tt_in1 & tt_in2) | (tt_in1 & ~tt_in3) | (tt_in2 & ~tt_in3);
                    break;
                }
                tmps[i] = tt_step;

                for (auto h = 0u; h < outputs.size(); h++) {
                    const auto out = outputs[h];
                    const auto var = out >> 1;
                    const auto inv = out & 1;
                    if (var - nr_in - 1 == static_cast<int>(i)) {
                        fs[h] = inv ? ~tt_step : tt_step;
                    }
                }
            }

            return fs;
        }

        void set_step(
            int i,
            int fanin1,
            int fanin2,
            const int fanin3,
            int op)
        {
            steps[i][0] = fanin1;
            steps[i][1] = fanin2;
            steps[i][2] = fanin3;
            operators[i] = op;
        }


        void set_output(int out_idx, int lit) 
        {
            outputs[out_idx] = lit;
        }

        bool satisfies_spec(const spec& spec)
        {
            if (spec.nr_triv == spec.get_nr_out()) {
                return true;
            }
            auto tts = simulate();

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

            if (spec.add_alonce_clauses) {
                // Ensure that each step is used at least once.
                std::vector<int> nr_uses(steps.size());

                for (auto i = 1u; i < steps.size(); i++) {
                    const auto& step = steps[i];
                    for (const auto fid : step) {
                        if (fid >= nr_in) {
                            nr_uses[fid - nr_in]++;
                        }
                    }
                }
                for (auto output : outputs) {
                    const auto step_idx = output >> 1;
                    if (step_idx > nr_in) {
                        nr_uses[step_idx - nr_in - 1]++;
                    }
                }

                for (auto nr : nr_uses) {
                    if (nr == 0) {
                        assert(false);
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
                // Ensure that steps are in STRICT co-lexicographical order.
                for (int i = 0; i < spec.nr_steps - 1; i++) {
                    const auto& v1 = steps[i];
                    const auto& v2 = steps[i + 1];

                    if (colex_compare(v1, v2) != -1) {
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
                        if (op2 > op1) {
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


        void to_expression(std::ostream& o, bool write_newline = false)
        {
            rec_to_expression(o, nr_in + steps.size() - 1);
            if (write_newline) {
                o << std::endl;
            }
        }
    };
}