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
  \brief Generator for partial DAGs with fanin size 3
  \author Winston Haaswijk
*/

#include <percy/partial_dag.hpp>

namespace percy
{

class partial_dag3_generator
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
    int _disabled_matrix[18][18][18][18];

    partial_gen_type _gen_type = GEN_NOREAPPLY;

    // The index at which backtracking should terminate.
    int _stop_level = -1;

    // Function to call when a solution is found.
    std::function<void(partial_dag3_generator*)> _callback;

public:
    partial_dag3_generator() : _initialized(false) { }

    partial_dag3_generator(int nr_vertices)
    {
        reset(nr_vertices);
    }

    int _js[18];
    int _ks[18];
    int _ls[18];

    // The level from which the search is assumed to have started
    int _start_level = 1;

    int nr_vertices() const { return _nr_vertices; }

    void verbosity(int verbosity) { _verbosity = verbosity; }
    int verbosity() { return _verbosity; }

    partial_gen_type gen_type() const { return _gen_type; }
    void gen_type(partial_gen_type gen_type) { _gen_type = gen_type; }

    void set_callback(std::function<void(partial_dag3_generator*)>& f)
    {
        _callback = f;
    }

    void set_callback(std::function<void(partial_dag3_generator*)>&& f)
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
            for (int l = 2; l < 18; l++) {
                for (int k = 1; k < l; k++) {
                    for (int j = 0; j < k; j++) {
                        _disabled_matrix[i][j][k][l] = 0;
                    }
                }
            }

            // The first vertex can only point to PIs
            _js[0] = 0;
            _ks[0] = 0;
            _ls[0] = 0;

            _nr_solutions = 0;
            _level = 0;
            _stop_level = -1;

            _initialized = true;
        }
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
                    const auto l = _ls[i];
                    if (i > 0) {
                        printf(" - ");
                    }
                    printf("(%d, %d, %d)", j, k, l);
                }
                printf("\n");
            }
            backtrack();
        } else {
            // It's always possible that this node is only connected to PIs
            _js[_level] = 0;
            _ks[_level] = 0;
            _ls[_level] = 0;
            ++_level;
            search_tuples();

            // It may also just have one internal connection
            for (int l = 1; l <= _level; l++) {
                _ls[_level] = l;
                ++_level;
                search_tuples();
            }

            // Or at least two internal connections
            for (int l = 2; l <= _level; l++) {
                for (int k = 1; k < l; k++) {
                    for (int j = 0; j < k; j++) {
                        _js[_level] = j;
                        _ks[_level] = k;
                        _ls[_level] = l;
                        ++_level;
                        search_tuples();
                    }
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
                    const auto l = _ls[i];
                    if (i > 0) {
                        printf(" - ");
                    }
                    printf("(%d, %d, %d)", j, k, l);
                }
                printf("\n");
            }
            backtrack();
        } else {
            // It's always possible that this node is only connected to PIs
            _js[_level] = 0;
            _ks[_level] = 0;
            _ls[_level] = 0;
            ++_level;
            search_connected_dags();

            // It may also just have one internal connection
            for (int l = 1; l <= _level; l++) {
                _ls[_level] = l;
                ++_covered_steps[l];
                ++_level;
                search_connected_dags();
            }

            // Or at least two internal connections
            for (int l = 2; l <= _level; l++) {
                for (int k = 1; k < l; k++) {
                    for (int j = 0; j < k; j++) {
                        _js[_level] = j;
                        _ks[_level] = k;
                        _ls[_level] = l;
                        ++_covered_steps[j];
                        ++_covered_steps[k];
                        ++_covered_steps[l];
                        ++_level;
                        search_connected_dags();
                    }
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
                    const auto l = _ls[i];
                    if (i > 0) {
                        printf(" - ");
                    }
                    printf("(%d, %d, %d)", j, k, l);
                }
                printf("\n");
            }
            backtrack();
        } else {
            // We are only interested in DAGs that are in
            // co-lexicographical order. Look at the previous step
            // on the stack, the current step should be greater or
            // equal to it.
            const auto start_j = _js[_level - 1];
            const auto start_k = _ks[_level - 1];
            const auto start_l = _ls[_level - 1];

            _js[_level] = start_j;
            _ks[_level] = start_k;
            _ls[_level] = start_l;
            ++_covered_steps[start_j];
            ++_covered_steps[start_k];
            ++_covered_steps[start_l];
            ++_level;
            search_colex_dags();
            
            // One internal connection
            if (start_j == 0 && start_k == 0) {
                for (int l = start_l + 1; l <= _level; l++) {
                    _ls[_level] = l;
                    ++_covered_steps[l];
                    ++_level;
                    search_colex_dags();
                }
            }

            for (int l = start_l; l <= _level; l++) {
                for (int k = start_k; k < l; k++) {
                    for (int j = start_j; j < k; j++) {
                        ++_covered_steps[j];
                        ++_covered_steps[k];
                        ++_covered_steps[l];
                        _js[_level] = j;
                        _ks[_level] = k;
                        _ls[_level] = l;
                        ++_level;
                        search_colex_dags();
                    }
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
                    const auto l = _ls[i];
                    if (i > 0) {
                        printf(" - ");
                    }
                    printf("(%d, %d, %d)", j, k, l);
                }
                printf("\n");
            }
            if (_callback) {
                _callback(this);
            }
            noreapply_backtrack();
        } else {
            const auto start_j = _js[_level - 1];
            const auto start_k = _ks[_level - 1];
            const auto start_l = _ls[_level - 1];

            _js[_level] = start_j;
            _ks[_level] = start_k;
            _ls[_level] = start_l;
            ++_covered_steps[start_j];
            ++_covered_steps[start_k];
            ++_covered_steps[start_l];
            ++_level;
            search_noreapply_dags();

            // One internal connection
            if (start_j == 0 && start_k == 0) {
                for (int l = start_l + 1; l <= _level; l++) {
                    _ls[_level] = l;
                    ++_covered_steps[l];
                    ++_level;
                    search_noreapply_dags();
                }
            }
            
            for (int l = start_l; l <= _level; l++) {
                for (int k = start_k; k < l; k++) {
                    for (int j = start_j; j < k; j++) {
                        if (_disabled_matrix[_level][j][k][l]) {
                            continue;
                        }
                        ++_covered_steps[j];
                        ++_covered_steps[k];
                        ++_covered_steps[l];

                        if (k > 0) {
                            for (int ip = _level + 1; ip < _nr_vertices; ip++) {
                                if (j > 0) {
                                    ++_disabled_matrix[ip][j][k][_level + 1];
                                    ++_disabled_matrix[ip][j][l][_level + 1];
                                }
                                ++_disabled_matrix[ip][k][l][_level + 1];
                            }
                        }
                        _js[_level] = j;
                        _ks[_level] = k;
                        _ls[_level] = l;
                        ++_level;
                        search_noreapply_dags();
                    }
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
            const auto l = _ls[_level];
            --_covered_steps[j];
            --_covered_steps[k];
            --_covered_steps[l];
        }
    }

    void noreapply_backtrack()
    {
        --_level;
        if (_level > _stop_level) {
            const auto j = _js[_level];
            const auto k = _ks[_level];
            const auto l = _ls[_level];
            --_covered_steps[j];
            --_covered_steps[k];
            --_covered_steps[l];
            if (k > 0) {
                for (int ip = _level + 1; ip < _nr_vertices; ip++) {
                    if (j > 0) {
                        --_disabled_matrix[ip][j][k][_level + 1];
                        --_disabled_matrix[ip][j][l][_level + 1];
                    }
                    --_disabled_matrix[ip][k][l][_level + 1];
                }
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
};

inline void pd3_write_nonisomorphic(int nr_vertices, const char* const filename, int nr_in = -1)
{
    partial_dag g;
    partial_dag3_generator gen;
    auto fhandle = fopen(filename, "wb");
    if (fhandle == NULL) {
        fprintf(stderr, "Error: unable to open PD file\n");
        exit(1);
    }
    uint64_t ctr = 0;
#ifndef DISABLE_NAUTY
    std::set<std::vector<graph>> can_reprs;
    pd_iso_checker checker(nr_vertices);
 
    gen.set_callback([&g, fhandle, &can_reprs, &checker, nr_in, &ctr]
    (partial_dag3_generator* gen) {
        for (int i = 0; i < gen->nr_vertices(); i++) {
            g.set_vertex(i, gen->_js[i], gen->_ks[i], gen->_ls[i]);
        }
        const auto can_repr = checker.crepr(g);
        const auto res = can_reprs.insert(can_repr);
        printf("%zu\r", ++ctr);
        if (res.second) {
            if (nr_in != -1 && g.nr_pi_fanins() >= nr_in)
                write_partial_dag(g, fhandle);
            else if (nr_in == -1)
                write_partial_dag(g, fhandle);
        }
    });
#else
    gen.set_callback([&g, fhandle, &ctr]
    (partial_dag3_generator* gen) {
        for (int i = 0; i < gen->nr_vertices(); i++) {
            g.set_vertex(i, gen->_js[i], gen->_ks[i], gen->_ls[i]);
        }
        printf("%zu\r", ++ctr);
        write_partial_dag(g, fhandle);
    });
#endif
    g.reset(3, nr_vertices);
    gen.reset(nr_vertices);
    gen.count_dags();
    printf("\n");

    fclose(fhandle);
}

inline std::vector<partial_dag> pd3_generate_max(int max_vertices, int nr_in)
{
    partial_dag g;
    partial_dag3_generator gen;
    std::vector<partial_dag> dags;

    gen.set_callback([&g, &dags, nr_in]
    (partial_dag3_generator* gen) {
        for (int i = 0; i < gen->nr_vertices(); i++) {
            g.set_vertex(i, gen->_js[i], gen->_ks[i], gen->_ls[i]);
        }
        if (g.nr_pi_fanins() >= nr_in) {
            dags.push_back(g);
        }
    });

    for (int i = 1; i <= max_vertices; i++) {
        g.reset(3, i);
        gen.reset(i);
        gen.count_dags();
    }

    return dags;
}

inline std::vector<partial_dag> pd3_generate_filtered(int max_vertices, int nr_in)
{
    partial_dag g;
    partial_dag3_generator gen;
    std::vector<partial_dag> dags;

    gen.set_callback([&g, &dags, nr_in]
    (partial_dag3_generator* gen) {
        for (int i = 0; i < gen->nr_vertices(); i++) {
            g.set_vertex(i, gen->_js[i], gen->_ks[i], gen->_ls[i]);
        }
        if (g.nr_pi_fanins() >= nr_in) {
            dags.push_back(g);
        }
    });

    for (int i = 1; i <= max_vertices; i++) {
        g.reset(3, i);
        gen.reset(i);
        gen.count_dags();
    }

    return dags;
}

// Same as pd_generate_filtered, but generates only those PDs with the
// exact number of specified vertices.
inline std::vector<partial_dag> pd3_exact_generate_filtered(int nr_vertices, int nr_in)
{
    partial_dag g;
    partial_dag3_generator gen;
    std::vector<partial_dag> dags;

    gen.set_callback([&g, &dags, nr_in]
    (partial_dag3_generator* gen) {
        for (int i = 0; i < gen->nr_vertices(); i++) {
            g.set_vertex(i, gen->_js[i], gen->_ks[i], gen->_ls[i]);
        }
        if (g.nr_pi_fanins() >= nr_in) {
            dags.push_back(g);
        }
    });

    g.reset(3, nr_vertices);
    gen.reset(nr_vertices);
    gen.count_dags();

    return dags;
}

inline size_t count_partial_dag3s(FILE* fhandle)
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
            stat = fread(&buf, sizeof(int), 1, fhandle);
            assert(stat == 1);
        }
        nr_dags++;
    }

    return nr_dags;
}

inline std::vector<partial_dag> read_partial_dag3s(const char* const filename)
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
        g.reset(3, nr_vertices);
        for (int i = 0; i < nr_vertices; i++) {
            auto stat = fread(&buf, sizeof(int), 1, fhandle);
            assert(stat == 1);
            auto fanin1 = buf;
            stat = fread(&buf, sizeof(int), 1, fhandle);
            assert(stat == 1);
            auto fanin2 = buf;
            stat = fread(&buf, sizeof(int), 1, fhandle);
            assert(stat == 1);
            auto fanin3 = buf;
            g.set_vertex(i, fanin1, fanin2, fanin3);
        }
        dags.push_back(g);
    }

    fclose(fhandle);

    return dags;
}

} /* namespace percy */
