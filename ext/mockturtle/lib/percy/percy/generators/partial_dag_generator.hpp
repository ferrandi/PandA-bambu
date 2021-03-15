/* percy
 * Copyright (C) 2018-2019  EPFL
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/*!
  \file partial_dag_generator.hpp
  \brief Generator for partial DAGs with fanin size 2
  \author Winston Haaswijk
*/

#include <percy/partial_dag.hpp>

namespace percy
{

class partial_dag_generator
{
private:
    int _nr_vertices;
    int _verbosity = 0;
    bool _initialized;
    int _level;
    uint64_t _nr_solutions;

    // Array indicating which steps have been covered. (And how many
    // times.) 
    int _covered_steps[18];

    // Array indicating which steps are "disabled", meaning that
    // selecting them will not result in a valid DAG.
    int _disabled_matrix[18][18][18];

    partial_gen_type _gen_type = GEN_NOREAPPLY;

    // The index at which backtracking should terminate.
    int _stop_level = -1;

    // Function to call when a solution is found.
    std::function<void(partial_dag_generator*)> _callback;

public:
    partial_dag_generator() : _initialized(false) { }

    partial_dag_generator(int nr_vertices)
    {
        reset(nr_vertices);
    }

    // Two arrays that represent the "stack" of selected steps.
    int _js[18];
    int _ks[18];

    // The level from which the search is assumed to have started
    int _start_level = 1;

    int nr_vertices() const { return _nr_vertices; }

    void verbosity(int verbosity) { _verbosity = verbosity; }
    int verbosity() { return _verbosity; }

    partial_gen_type gen_type() const { return _gen_type; }
    void gen_type(partial_gen_type gen_type) { _gen_type = gen_type; }

    void set_callback(std::function<void(partial_dag_generator*)>& f)
    {
        _callback = f;
    }

    void set_callback(std::function<void(partial_dag_generator*)>&& f)
    {
        _callback = std::move(f);
    }

    void clear_callback()
    {
        _callback = 0;
    }

    void reset(int nr_vertices)
    {
        assert(nr_vertices > 0);

        if (_verbosity) {
            printf("setting nr. vertices=%d\n", nr_vertices);
        }

        _nr_vertices = nr_vertices;

        for (int i = 0; i < 18; i++) {
            _covered_steps[i] = 0;
        }

        for (int i = 0; i < 18; i++) {
            for (int j = 0; j < 18; j++) {
                for (int k = 0; k < 18; k++) {
                    _disabled_matrix[i][j][k] = 0;
                }
            }
        }

        // The first vertex can only point to PIs
        _js[0] = 0;
        _ks[0] = 0;

        _nr_solutions = 0;
        _level = 0;
        _stop_level = -1;

        _initialized = true;
    }

    auto count_tuples()
    {
        assert(_initialized);
        _level = _start_level;
        _nr_solutions = 0;

        search_tuples();

        return _nr_solutions;
    }

    void search_tuples()
    {
        if (_level == _nr_vertices) {
            ++_nr_solutions;
            if (_verbosity) {
                printf("Found solution: ");
                for (int i = 0; i < _nr_vertices; i++) {
                    const auto j = _js[i];
                    const auto k = _ks[i];
                    if (i > 0) {
                        printf(" - ");
                    }
                    printf("(%d, %d)", j, k);
                }
                printf("\n");
            }
            backtrack();
        } else {
            // It's always possible that this node is only connected to PIs
            _js[_level] = 0;
            _ks[_level] = 0;
            ++_level;
            search_tuples();
            for (int k = 1; k <= _level; k++) {
                for (int j = 0; j < k; j++) {
                    _js[_level] = j;
                    _ks[_level] = k;
                    ++_level;
                    search_tuples();
                }
            }
            backtrack();
        }
    }

    auto count_connected_dags()
    {
        assert(_initialized);
        _level = _start_level;
        _nr_solutions = 0;

        search_connected_dags();

        return _nr_solutions;
    }

    void search_connected_dags()
    {
        if (_level == _nr_vertices) {
            for (int i = 1; i <= _nr_vertices - 1; i++) {
                if (_covered_steps[i] == 0) {
                    // There is some uncovered internal step, so the
                    // graph cannot be connected.
                    backtrack();
                    return;
                }
            }
            ++_nr_solutions;
            if (_verbosity) {
                printf("Found solution: ");
                for (int i = 0; i < _nr_vertices; i++) {
                    const auto j = _js[i];
                    const auto k = _ks[i];
                    if (i > 0) {
                        printf(" - ");
                    }
                    printf("(%d, %d)", j, k);
                }
                printf("\n");
            }
            backtrack();
        } else {
            _js[_level] = 0;
            _ks[_level] = 0;
            ++_level;
            search_connected_dags();
            for (int k = 1; k <= _level; k++) {
                for (int j = 0; j < k; j++) {
                    _js[_level] = j;
                    _ks[_level] = k;
                    ++_covered_steps[j];
                    ++_covered_steps[k];
                    ++_level;
                    search_connected_dags();
                }
            }
            backtrack();
        }
    }

    auto count_colex_dags()
    {
        assert(_initialized);
        _level = 1;
        _nr_solutions = 0;

        search_colex_dags();

        return _nr_solutions;
    }

    void search_colex_dags()
    {
        if (_level == _nr_vertices) {
            for (int i = 1; i <= _nr_vertices - 1; i++) {
                if (_covered_steps[i] == 0) {
                    // There is some uncovered internal step, so the
                    // graph cannot be connected.
                    backtrack();
                    return;
                }
            }
            ++_nr_solutions;
            if (_verbosity) {
                printf("Found solution: ");
                for (int i = 0; i < _nr_vertices; i++) {
                    const auto j = _js[i];
                    const auto k = _ks[i];
                    if (i > 0) {
                        printf(" - ");
                    }
                    printf("(%d, %d)", j, k);
                }
                printf("\n");
            }
            backtrack();
        } else {
            // Check if this step can have pure PI fanins
            _js[_level] = 0;
            _ks[_level] = 0;
            ++_level;
            search_colex_dags();

            // We are only interested in DAGs that are in
            // co-lexicographical order. Look at the previous step
            // on the stack, the current step should be greater or
            // equal to it.
            const auto start_j = _js[_level - 1];
            auto start_k = _ks[_level - 1];

            if (start_j == start_k) { // Previous input has two PI inputs
                ++start_k;
            }

            _ks[_level] = start_k;
            for (int j = start_j; j < start_k; j++) {
                ++_covered_steps[j];
                ++_covered_steps[start_k];
                _js[_level] = j;
                ++_level;
                search_colex_dags();
            }
            for (int k = start_k + 1; k <= _level; k++) {
                for (int j = 0; j < k; j++) {
                    ++_covered_steps[j];
                    ++_covered_steps[k];
                    _js[_level] = j;
                    _ks[_level] = k;
                    ++_level;
                    search_colex_dags();
                }
            }

            backtrack();
        }
    }

    auto count_noreapply_dags()
    {
        assert(_initialized);
        _nr_solutions = 0;
        _level = 1;

        search_noreapply_dags();

        return _nr_solutions;
    }

    void search_noreapply_dags()
    {
        if (_level == _nr_vertices) {
            for (int i = 1; i <= _nr_vertices - 1; i++) {
                if (_covered_steps[i] == 0) {
                    // There is some uncovered internal step, so the
                    // graph cannot be connected.
                    noreapply_backtrack();
                    return;
                }
            }
            ++_nr_solutions;
            if (_verbosity) {
                printf("Found solution: ");
                for (int i = 0; i < _nr_vertices; i++) {
                    const auto j = _js[i];
                    const auto k = _ks[i];
                    if (i > 0) {
                        printf(" - ");
                    }
                    printf("(%d, %d)", j, k);
                }
                printf("\n");
            }
            if (_callback) {
                _callback(this);
            }
            noreapply_backtrack();
        } else {
            // Check if this step can have pure PI fanins
            _js[_level] = 0;
            _ks[_level] = 0;
            ++_level;
            search_noreapply_dags();

            // We are only interested in DAGs that are in
            // co-lexicographical order. Look at the previous step
            // on the stack, the current step should be greater or
            // equal to it.
            const auto start_j = _js[_level - 1];
            auto start_k = _ks[_level - 1];

            if (start_j == start_k) { // Previous input has two PI inputs
                ++start_k;
            }

            _ks[_level] = start_k;
            for (int j = start_j; j < start_k; j++) {
                if (_disabled_matrix[_level][j][start_k]) {
                    continue;
                }

                // If we choose fanin (j, k), record that j and k
                // are covered.
                ++_covered_steps[j];
                ++_covered_steps[start_k];

                // We are adding step (i, j, k). This means that we
                // don't have to consider steps (i',j,i) or (i',k,i)
                // for i < i' <= n+r. This avoiding reapplying an
                // operand.
                for (int ip = _level + 1; ip < _nr_vertices; ip++) {
                    ++_disabled_matrix[ip][j][_level];
                    ++_disabled_matrix[ip][start_k][_level];
                }

                _js[_level] = j;
                ++_level;
                search_noreapply_dags();
            }
            for (int k = start_k + 1; k <= _level; k++) {
                for (int j = 0; j < k; j++) {
                    if (_disabled_matrix[_level][j][k]) {
                        continue;
                    }
                    ++_covered_steps[j];
                    ++_covered_steps[k];

                    for (int ip = _level + 1; ip < _nr_vertices; ip++) {
                        ++_disabled_matrix[ip][j][_level];
                        ++_disabled_matrix[ip][k][_level];
                    }
                    _js[_level] = j;
                    _ks[_level] = k;
                    ++_level;
                    search_noreapply_dags();
                }
            }

            noreapply_backtrack();
        }
    }

    void backtrack()
    {
        --_level;
        if (_level > _stop_level) {
            const auto j = _js[_level];
            const auto k = _ks[_level];
            --_covered_steps[j];
            --_covered_steps[k];
        }
    }

    void noreapply_backtrack()
    {
        --_level;
        const auto j = _js[_level];
        const auto k = _ks[_level];
        if (_level > _stop_level) {
            --_covered_steps[j];
            --_covered_steps[k];
            for (int ip = _level + 1; ip < _nr_vertices; ip++) {
                --_disabled_matrix[ip][j][_level];
                --_disabled_matrix[ip][k][_level];
            }
        }
    }

    auto count_dags()
    {
        switch (_gen_type) {
        case GEN_TUPLES:
            return count_tuples();
        case GEN_CONNECTED:
            return count_connected_dags();
        case GEN_COLEX:
            return count_colex_dags();
        default:
            return count_noreapply_dags();
        }
    }

    bool search_operator(
        spec& spec,
        chain& chain,
        const partial_dag& dag,
        int idx) {
        kitty::dynamic_truth_table tt(2);
        do {
            if (is_normal(tt) && !is_trivial(tt)) {
                chain.set_step(idx, _js[idx], _ks[idx], tt);
                const auto found = search_sol(spec, chain, dag, idx + 1);
                if (found) {
                    return true;
                }
            }
            kitty::next_inplace(tt);
        } while (!kitty::is_const0(tt));

        return false;
    }

    bool search_sol(
        spec& spec,
        chain& chain,
        const partial_dag& dag,
        int idx) {
        if (idx == dag.nr_vertices()) {
            const auto tts = chain.simulate();
            if (tts[0] == (spec.out_inv ? ~spec[0] : spec[0])) {
                if (spec.out_inv) {
                    chain.invert();
                }
                return true;
            } else {
                return false;
            }
        }
        const auto& vertex = dag.get_vertex(idx);
        auto nr_pi_fanins = 0;
        if (vertex[1] == FANIN_PI) {
            nr_pi_fanins = 2;
        } else if (vertex[0] == FANIN_PI) {
            nr_pi_fanins = 1;
        }
        if (nr_pi_fanins == 0) {
            _js[idx] = vertex[0] + spec.get_nr_in() - 1;
            _ks[idx] = vertex[1] + spec.get_nr_in() - 1;
            const auto found = search_operator(spec, chain, dag, idx);
            if (found) {
                return true;
            }
        } else if (nr_pi_fanins == 1) {
            for (auto j = 0; j < spec.get_nr_in(); j++) {
                _js[idx] = j;
                _ks[idx] = vertex[1] + spec.get_nr_in() - 1;
                const auto found = search_operator(spec, chain, dag, idx);
                if (found) {
                    return true;
                }
            }
        } else {
            for (auto k = 1; k < spec.get_nr_in(); k++) {
                for (auto j = 0; j < k; j++) {
                    _js[idx] = j;
                    _ks[idx] = k;
                    const auto found = search_operator(spec, chain, dag, idx);
                    if (found) {
                        return true;
                    }
                }
            }
        }

        return false;
    }
};

/// Generate all partial DAGs of the specified number
/// of vertices.
inline std::vector<partial_dag> pd_generate(int nr_vertices)
{
    partial_dag g;
    partial_dag_generator gen;
    std::vector<partial_dag> dags;

    gen.set_callback([&g, &dags]
    (partial_dag_generator* gen) {
        for (int i = 0; i < gen->nr_vertices(); i++) {
            g.set_vertex(i, gen->_js[i], gen->_ks[i]);
        }
        dags.push_back(g);
    });

    g.reset(2, nr_vertices);
    gen.reset(nr_vertices);
    gen.count_dags();

    return dags;
}

#ifndef DISABLE_NAUTY
inline std::vector<partial_dag> pd_generate_nonisomorphic(int nr_vertices)
{
    partial_dag g;
    partial_dag_generator gen;
    std::vector<partial_dag> dags;
    std::set<std::vector<graph>> can_reprs;
    pd_iso_checker checker(nr_vertices);

    gen.set_callback([&g, &dags, &can_reprs, &checker]
    (partial_dag_generator* gen) {
        for (int i = 0; i < gen->nr_vertices(); i++) {
            g.set_vertex(i, gen->_js[i], gen->_ks[i]);
        }
        const auto can_repr = checker.crepr(g);
        const auto res = can_reprs.insert(can_repr);
        if (res.second)
            dags.push_back(g);
    });

    g.reset(2, nr_vertices);
    gen.reset(nr_vertices);
    gen.count_dags();

    return dags;
}
#endif

inline void pd_write_nonisomorphic(int nr_vertices, const char* const filename)
{
    partial_dag g;
    partial_dag_generator gen;
    auto fhandle = fopen(filename, "wb");
#ifndef DISABLE_NAUTY
    std::set<std::vector<graph>> can_reprs;
    pd_iso_checker checker(nr_vertices);
 
    gen.set_callback([&g, fhandle, &can_reprs, &checker]
    (partial_dag_generator* gen) {
        for (int i = 0; i < gen->nr_vertices(); i++) {
            g.set_vertex(i, gen->_js[i], gen->_ks[i]);
        }
        const auto can_repr = checker.crepr(g);
        const auto res = can_reprs.insert(can_repr);
        if (res.second)
            write_partial_dag(g, fhandle);
    });
#else
    gen.set_callback([&g, fhandle]
    (partial_dag_generator* gen) {
        for (int i = 0; i < gen->nr_vertices(); i++) {
            g.set_vertex(i, gen->_js[i], gen->_ks[i]);
        }
        write_partial_dag(g, fhandle);
    });
#endif
    g.reset(2, nr_vertices);
    gen.reset(nr_vertices);
    gen.count_dags();

    fclose(fhandle);
}

/// Generate all partial DAGs up to the specified number
/// of vertices.
inline std::vector<partial_dag> pd_generate_max(int max_vertices)
{
    partial_dag g;
    partial_dag_generator gen;
    std::vector<partial_dag> dags;

    gen.set_callback([&g, &dags]
    (partial_dag_generator* gen) {
        for (int i = 0; i < gen->nr_vertices(); i++) {
            g.set_vertex(i, gen->_js[i], gen->_ks[i]);
        }
        dags.push_back(g);
    });

    for (int i = 1; i <= max_vertices; i++) {
        g.reset(2, i);
        gen.reset(i);
        gen.count_dags();
    }

    return dags;
}

inline std::vector<partial_dag> pd_generate_filtered(int max_vertices, int nr_in)
{
    partial_dag g;
    partial_dag_generator gen;
    std::vector<partial_dag> dags;

    gen.set_callback([&g, &dags, nr_in]
    (partial_dag_generator* gen) {
        for (int i = 0; i < gen->nr_vertices(); i++) {
            g.set_vertex(i, gen->_js[i], gen->_ks[i]);
        }
        if (g.nr_pi_fanins() >= nr_in) {
            dags.push_back(g);
        }
    });

    for (int i = 1; i <= max_vertices; i++) {
        g.reset(2, i);
        gen.reset(i);
        gen.count_dags();
    }

    return dags;
}

inline size_t count_partial_dags(FILE* fhandle)
{
    size_t nr_dags = 0;
    int buf;
    while (fread(&buf, sizeof(int), 1, fhandle) != 0) {
        auto nr_vertices = buf;
        for (int i = 0; i < nr_vertices; i++) {
            auto stat = fread(&buf, sizeof(int), 1, fhandle);
            assert(stat == 1);
            stat = fread(&buf, sizeof(int), 1, fhandle);
            assert(stat == 1);
        }
        nr_dags++;
    }

    return nr_dags;
}

/// Reads serialized partial DAGs from file
inline std::vector<partial_dag> read_partial_dags(const char* const filename)
{
    std::vector<partial_dag> dags;

    auto fhandle = fopen(filename, "rb");
    if (fhandle == NULL) {
        fprintf(stderr, "Error: unable to open output file\n");
        exit(1);
    }

    partial_dag g;
    int buf;
    while (fread(&buf, sizeof(int), 1, fhandle) != 0) {
        auto nr_vertices = buf;
        g.reset(2, nr_vertices);
        for (int i = 0; i < nr_vertices; i++) {
            auto stat = fread(&buf, sizeof(int), 1, fhandle);
            assert(stat == 1);
            auto fanin1 = buf;
            stat = fread(&buf, sizeof(int), 1, fhandle);
            assert(stat == 1);
            auto fanin2 = buf;
            g.set_vertex(i, fanin1, fanin2);
        }
        dags.push_back(g);
    }

    fclose(fhandle);

    return dags;
}

} /* namespace percy */
