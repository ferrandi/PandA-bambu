#pragma once

#ifndef DISABLE_NAUTY
#include <nauty.h>
#endif
#include <vector>
#include <ostream>
#include <set>
#include "tt_utils.hpp"

namespace percy
{
    /// Convention: the zero fanin keeps a node's fanin "free". This fanin will not
    /// be connected to any of the other nodes in the partial DAG but rather may
    /// be connected to any one of the PIs.
    const int FANIN_PI = 0;

    class partial_dag
    {
        private:
            int fanin; /// The in-degree of vertices in the DAG
            std::vector<std::vector<int>> vertices;

        public:
            partial_dag() : fanin(0) { }

            partial_dag(int fanin, int nr_vertices = 0)
            {
                reset(fanin, nr_vertices);
            }

            partial_dag(const partial_dag& dag)
            {
                fanin = dag.fanin;
                vertices = dag.vertices;
            }

            partial_dag& operator=(const partial_dag& dag)
            {
                fanin = dag.fanin;
                vertices = dag.vertices;
                return *this;
            }

            partial_dag(partial_dag&& dag)
            {
                fanin = dag.fanin;
                vertices = std::move(dag.vertices);
            }

            int nr_pi_fanins()
            {
                int count = 0;
                for (const auto& v : vertices) {
                    for (auto fanin : v) {
                        if (fanin == FANIN_PI) {
                            count++;
                        }
                    }
                }
                return count;
            }

            template<typename Fn>
            void 
            foreach_vertex(Fn&& fn) const
            {
                for (int i = 0; i < nr_vertices(); i++) {
                    fn(vertices[i], i);
                }
            }

            template<typename Fn>
            void 
            foreach_fanin(std::vector<int>& v, Fn&& fn) const
            {
                for (auto i = 0; i < fanin; i++) {
                    fn(v[i], i);
                }
            }

            void 
            reset(int fanin, int nr_vertices)
            {
                this->fanin = fanin;
                vertices.resize(nr_vertices);
                for (auto& vertex : vertices) {
                    vertex.resize(fanin);
                }
            }

            void 
            set_vertex(int v_idx, const std::vector<int>& fanins)
            {
                assert(v_idx < nr_vertices());
                assert(fanins.size() == static_cast<unsigned>(fanin));
                for (int i = 0; i < fanin; i++) {
                    vertices[v_idx][i] = fanins[i];
                }
            }

            void
            set_vertex(int v_idx, int fi1, int fi2)
            {
                assert(v_idx < nr_vertices());
                vertices[v_idx][0] = fi1;
                vertices[v_idx][1] = fi2;
            }
            
            void
            set_vertex(int v_idx, int fi1, int fi2, int fi3)
            {
                assert(v_idx < nr_vertices());
                vertices[v_idx][0] = fi1;
                vertices[v_idx][1] = fi2;
                vertices[v_idx][2] = fi3;
            }

            void 
            add_vertex(const std::vector<int>& fanins)
            {
                assert(fanins.size() == static_cast<unsigned>(fanin));
                vertices.push_back(fanins);
            }

            const std::vector<int>& 
            get_vertex(int v_idx) const
            {
                return vertices[v_idx];
            }

            const std::vector<std::vector<int>>& get_vertices() const 
            {
                return vertices;
            }

            int nr_vertices() const
            {
                return vertices.size();
            }

#ifndef DISABLE_NAUTY
            bool is_isomorphic(const partial_dag& g) const
            {
                const auto total_vertices = nr_vertices();
                assert(total_vertices == g.nr_vertices());

                void (*adjacencies)(graph*, int*, int*, int, 
                        int, int, int*, int, boolean, int, int) = NULL;

                DYNALLSTAT(int,lab1,lab1_sz);
                DYNALLSTAT(int,lab2,lab2_sz);
                DYNALLSTAT(int,ptn,ptn_sz);
                DYNALLSTAT(int,orbits,orbits_sz);
                DYNALLSTAT(int,map,map_sz);
                DYNALLSTAT(graph,g1,g1_sz);
                DYNALLSTAT(graph,g2,g2_sz);
                DYNALLSTAT(graph,cg1,cg1_sz);
                DYNALLSTAT(graph,cg2,cg2_sz);
                DEFAULTOPTIONS_DIGRAPH(options);
                statsblk stats;

                int m = SETWORDSNEEDED(total_vertices);;

                options.getcanon = TRUE;

                DYNALLOC1(int,lab1,lab1_sz,total_vertices,"malloc");
                DYNALLOC1(int,lab2,lab2_sz,total_vertices,"malloc");
                DYNALLOC1(int,ptn,ptn_sz,total_vertices,"malloc");
                DYNALLOC1(int,orbits,orbits_sz,total_vertices,"malloc");
                DYNALLOC1(int,map,map_sz,total_vertices,"malloc");

                // Make the first graph
                DYNALLOC2(graph,g1,g1_sz,total_vertices,m,"malloc");
                EMPTYGRAPH(g1,m,total_vertices);
                for (int i = 1; i < total_vertices; i++) {
                    const auto& vertex = get_vertex(i);
                    for (const auto fanin : vertex) {
                        if (fanin != FANIN_PI) {
                            ADDONEARC(g1, fanin - 1, i, m);
                        }
                    }
                }

                // Make the second graph
                DYNALLOC2(graph,g2,g2_sz,total_vertices,m,"malloc");
                EMPTYGRAPH(g2,m,total_vertices);
                for (int i = 0; i < total_vertices; i++) {
                    const auto& vertex = g.get_vertex(i);
                    for (const auto fanin : vertex) {
                        if (fanin != FANIN_PI) {
                            ADDONEARC(g2, fanin - 1, i, m);
                        }
                    }
                }

                // Create canonical graphs
                DYNALLOC2(graph,cg1,cg1_sz,total_vertices,m,"malloc");
                DYNALLOC2(graph,cg2,cg2_sz,total_vertices,m,"malloc");
                densenauty(g1,lab1,ptn,orbits,&options,&stats,m,total_vertices,cg1);
                densenauty(g2,lab2,ptn,orbits,&options,&stats,m,total_vertices,cg2);

                // Compare the canonical graphs to see if the two graphs are
                // isomorphic
                bool isomorphic = true;
                for (int k = 0; k < m*total_vertices; k++) {
                    if (cg1[k] != cg2[k]) {
                        isomorphic = false;
                        break;
                    }
                }
                if (false) {
                    // Print the mapping between graphs for debugging purposes
                    for (int i = 0; i < total_vertices; ++i) {
                        map[lab1[i]] = lab2[i];
                    }
                    for (int i = 0; i < total_vertices; ++i) {
                        printf(" %d-%d",i,map[i]);
                    }
                    printf("\n");
                }

                return isomorphic;
            }
#endif
            
    };

    enum partial_gen_type
    {
        GEN_TUPLES, /// No restrictions besides acyclicity
        GEN_CONNECTED, /// Generated graphs must be connected
        GEN_COLEX, /// Graph inputs must be co-lexicographically ordered
        GEN_NOREAPPLY, /// Graph inputs must not allow re-application of operators
    };

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

#ifndef DISABLE_NAUTY
        class pd_iso_checker
        {
        private:
            int total_vertices;
            int *lab1;
            size_t lab1_sz = 0;
            int *lab2;
            size_t lab2_sz = 0;
            int *ptn;
            size_t ptn_sz = 0;
            int *orbits;
            size_t orbits_sz = 0;
            int *map;
            size_t map_sz = 0;
            graph *g1 = NULL;
            size_t g1_sz = 0;
            graph *g2 = NULL;
            size_t g2_sz = 0;
            graph *cg1 = NULL;
            size_t cg1_sz = 0;
            graph *cg2 = NULL;
            size_t cg2_sz = 0;
            statsblk stats;
            int m;

            void initialize()
            {
                m = SETWORDSNEEDED(total_vertices);;

                DYNALLOC1(int, lab1, lab1_sz, total_vertices, "malloc");
                DYNALLOC1(int, lab2, lab2_sz, total_vertices, "malloc");
                DYNALLOC1(int, ptn, ptn_sz, total_vertices, "malloc");
                DYNALLOC1(int, orbits, orbits_sz, total_vertices, "malloc");
                DYNALLOC1(int, map, map_sz, total_vertices, "malloc");

                DYNALLOC2(graph, g1, g1_sz, total_vertices, m, "malloc");
                DYNALLOC2(graph, g2, g2_sz, total_vertices, m, "malloc");

                DYNALLOC2(graph, cg1, cg1_sz, total_vertices, m, "malloc");
                DYNALLOC2(graph, cg2, cg2_sz, total_vertices, m, "malloc");
            }

        public:
            pd_iso_checker(int _total_vertices)
            {
                total_vertices = _total_vertices;
                initialize();
            }

            ~pd_iso_checker()
            {
                DYNFREE(lab1, lab1_sz);
                DYNFREE(lab2, lab2_sz);
                DYNFREE(ptn, ptn_sz);
                DYNFREE(orbits, orbits_sz);
                DYNFREE(map, map_sz);

                DYNFREE(g1, g1_sz);
                DYNFREE(g2, g2_sz);

                DYNFREE(cg1, cg1_sz);
                DYNFREE(cg2, cg2_sz);
            }

            bool isomorphic(const partial_dag& dag1, const partial_dag& dag2)
            {
                void(*adjacencies)(graph*, int*, int*, int,
                    int, int, int*, int, boolean, int, int) = NULL;

                DEFAULTOPTIONS_DIGRAPH(options);
                options.getcanon = TRUE;

                const auto nr_vertices = dag1.nr_vertices();

                EMPTYGRAPH(g1, m, nr_vertices);
                EMPTYGRAPH(g2, m, nr_vertices);

                for (int i = 1; i < nr_vertices; i++) {
                    const auto& vertex = dag1.get_vertex(i);
                    for (const auto fanin : vertex) {
                        if (fanin != FANIN_PI) {
                            ADDONEARC(g1, fanin - 1, i, m);
                        }
                    }
                }

                for (int i = 0; i < nr_vertices; i++) {
                    const auto& vertex = dag2.get_vertex(i);
                    for (const auto fanin : vertex) {
                        if (fanin != FANIN_PI) {
                            ADDONEARC(g2, fanin - 1, i, m);
                        }
                    }
                }

                densenauty(g1, lab1, ptn, orbits, &options, &stats, m, nr_vertices, cg1);
                densenauty(g2, lab2, ptn, orbits, &options, &stats, m, nr_vertices, cg2);

                bool isomorphic = true;
                for (int k = 0; k < m*nr_vertices; k++) {
                    if (cg1[k] != cg2[k]) {
                        isomorphic = false;
                        break;
                    }
                }
                return isomorphic;
            }

            /// Computes the canonical representation of the given DAG 
            /// and returns it as a vector of numbers.
            std::vector<graph> crepr(const partial_dag& dag)
            {
                void(*adjacencies)(graph*, int*, int*, int,
                    int, int, int*, int, boolean, int, int) = NULL;

                DEFAULTOPTIONS_DIGRAPH(options);
                options.getcanon = TRUE;

                const auto nr_vertices = dag.nr_vertices();
                std::vector<graph> repr(m*dag.nr_vertices());

                EMPTYGRAPH(g1, m, nr_vertices);

                for (int i = 1; i < nr_vertices; i++) {
                    const auto& vertex = dag.get_vertex(i);
                    for (const auto fanin : vertex) {
                        if (fanin != FANIN_PI)
                            ADDONEARC(g1, fanin - 1, i, m);
                    }
                }

                densenauty(g1, lab1, ptn, orbits, &options, &stats, m, nr_vertices, cg1);

                for (int k = 0; k < m * nr_vertices; k++) {
                    repr[k] = cg1[k];
                }

                return repr;
            }

        };
#endif

        inline void write_partial_dag(const partial_dag& dag, FILE* fhandle)
        {
            int buf = dag.nr_vertices();
            fwrite(&buf, sizeof(int), 1, fhandle);
            for (int i = 0; i < dag.nr_vertices(); i++) {
                auto& v = dag.get_vertex(i);
                for (const auto fanin : v) {
                    buf = fanin;
                    auto stat = fwrite(&buf, sizeof(int), 1, fhandle);
                    assert(stat == 1);
                }
            }
        }

        /// Writes a collection of partial DAGs to the specified filename
        inline void write_partial_dags(const std::vector<partial_dag>& dags, const char* const filename)
        {
            auto fhandle = fopen(filename, "wb");
            if (fhandle == NULL) {
                fprintf(stderr, "Error: unable to open output file\n");
                exit(1);
            }

            for (auto& dag : dags) {
                write_partial_dag(dag, fhandle);
            }

            fclose(fhandle);
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

        inline void pd3_write_nonisomorphic(int nr_vertices, const char* const filename, int nr_in = -1)
        {
            (void)nr_in;
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
                printf("%llu\r", ++ctr);
                write_partial_dag(g, fhandle);
            });

#endif
            g.reset(3, nr_vertices);
            gen.reset(nr_vertices);
            gen.count_dags();
            printf("\n");

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

    /// Isomorphism check using set containinment (hashing)
    inline void pd_filter_isomorphic_sfast(
        const std::vector<partial_dag>& dags, 
        std::vector<partial_dag>& ni_dags,
        bool show_progress = false)
    {
        (void)show_progress;
#ifndef DISABLE_NAUTY
        if (dags.size() == 0) {
            return;
        }

        const auto nr_vertices = dags[0].nr_vertices();
        pd_iso_checker checker(nr_vertices);
        std::vector<std::vector<graph>> reprs(dags.size());
        auto ctr = 0u;
        if (show_progress)
            printf("computing canonical representations\n");
        for (auto i = 0u; i < dags.size(); i++) {
            const auto& dag = dags[i];
            reprs[i] = checker.crepr(dag);
            if (show_progress)
                printf("(%u,%zu)\r", ++ctr, dags.size());
        }
        if (show_progress)
            printf("\n");

        std::vector<int> is_iso(dags.size());
        for (auto i = 0u; i < dags.size(); i++) {
            is_iso[i] = 0;
        }

        std::set<std::vector<graph>> can_reps;
        
        ctr = 0u;
        if (show_progress)
            printf("checking isomorphisms\n");
        for (auto i = 0u; i < dags.size(); i++) {
            auto repr = reprs[i];
            const auto res = can_reps.insert(repr);
            if (!res.second) { // Already have an isomorphic representative
                is_iso[i] = 1;
            }
            if (show_progress)
                printf("(%u,%zu)\r", ++ctr, dags.size());
        }
        if (show_progress)
            printf("\n");

        for (auto i = 0u; i < dags.size(); i++) {
            if (!is_iso[i]) {
                ni_dags.push_back(dags[i]);
            }
        }
#else
        for (auto& dag : dags) {
            ni_dags.push_back(dag);
        }
#endif
    }


#ifndef DISABLE_NAUTY
    inline std::vector<partial_dag> pd_filter_isomorphic(
        const std::vector<partial_dag>& dags, 
        std::vector<partial_dag>& ni_dags,
        bool show_progress = false)
    {
        size_t ctr = 0;
        for (const auto& g1 : dags) {
            bool iso_found = false;
            for (const auto& g2 : ni_dags) {
                if (g2.nr_vertices() == g1.nr_vertices()) {
                    if (g1.is_isomorphic(g2)) {
                        iso_found = true;
                        break;
                    }
                }
            }
            if (!iso_found) {
                ni_dags.push_back(g1);
            }
            if (show_progress)
                printf("(%zu, %zu)\r", ++ctr, dags.size());
        }
        if (show_progress)
            printf("\n");

        return ni_dags;
    }

    inline std::vector<partial_dag> pd_filter_isomorphic(
        const std::vector<partial_dag>& dags,
        bool show_progress = false)
    {
        std::vector<partial_dag> ni_dags;
        pd_filter_isomorphic(dags, ni_dags, show_progress);
        return ni_dags;
    }

    /// Filters out isomorphic DAGs. NOTE: assumes that
    /// all gven DAGs have the same number of vertices.
    inline void pd_filter_isomorphic_fast(
        const std::vector<partial_dag>& dags, 
        std::vector<partial_dag>& ni_dags,
        bool show_progress = false)
    {
        if (dags.size() == 0) {
            return;
        }

        const auto nr_vertices = dags[0].nr_vertices();
        pd_iso_checker checker(nr_vertices);
        std::vector<std::vector<graph>> reprs(dags.size());
        auto ctr = 0u;
        if (show_progress)
            printf("computing canonical representations\n");
        for (auto i = 0u; i < dags.size(); i++) {
            const auto& dag = dags[i];
            reprs[i] = checker.crepr(dag);
            if (show_progress)
                printf("(%u,%zu)\r", ++ctr, dags.size());
        }
        if (show_progress)
            printf("\n");

        std::vector<int> is_iso(dags.size());
        for (auto i = 0u; i < dags.size(); i++) {
            is_iso[i] = 0;
        }
        
        ctr = 0u;
        if (show_progress)
            printf("checking isomorphisms\n");
        for (auto i = 1u; i < dags.size(); i++) {
            for (auto j = 0u; j < i; j++) {
                if (reprs[i] == reprs[j]) {
                    is_iso[i] = 1;
                    break;
                }
            }
            if (show_progress)
                printf("(%u,%zu)\r", ++ctr, dags.size());
        }
        if (show_progress)
            printf("\n");

        for (auto i = 0u; i < dags.size(); i++) {
            if (!is_iso[i]) {
                ni_dags.push_back(dags[i]);
            }
        }
    }

    
    inline void pd_filter_isomorphic(
        const std::vector<partial_dag>& dags, 
        int max_size, 
        std::vector<partial_dag>& ni_dags,
        bool show_progress = false)
    {
        size_t ctr = 0;
        pd_iso_checker checker(max_size);
        for (const auto& g1 : dags) {
            bool iso_found = false;
            for (const auto& g2 : ni_dags) {
                if (g2.nr_vertices() == g1.nr_vertices()) {
                    if (checker.isomorphic(g1, g2)) {
                        iso_found = true;
                        break;
                    }
                }
            }
            if (!iso_found) {
                ni_dags.push_back(g1);
            }
            if (show_progress)
                printf("(%zu,%zu)\r", ++ctr, dags.size());
        }
        if (show_progress)
            printf("\n");
        ni_dags = dags;
    }

    inline std::vector<partial_dag> pd_filter_isomorphic(
        const std::vector<partial_dag>& dags, 
        int max_size,
        bool show_progress = false)
    {
        std::vector<partial_dag> ni_dags;
        pd_filter_isomorphic(dags, max_size, ni_dags, show_progress);
        return ni_dags;
    }
#endif
}

