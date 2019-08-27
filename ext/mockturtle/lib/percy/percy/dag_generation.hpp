#pragma once

#include "dag.hpp"
#include <thread>
#include <functional>
#include <mutex>
#include "tt_utils.hpp"
#include "concurrentqueue.h"
#include "solvers.hpp"
#include <vector>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <abc/vecInt.h>
#pragma GCC diagnostic pop

namespace percy
{
    /*
    template<typename TT, typename Solver>
    class old_dag_synthesizer
    {
        private:
            Solver _solver;
            int _nr_in;
            int _nr_steps;
            int _nr_op_vars;
            int _nr_sim_vars;
            int _tt_size;
            int _verbosity = 0;

        public:
            old_dag_synthesizer()
            {
                solver_alloc(&_solver);
            }

            ~old_dag_synthesizer()
            {
                solver_dealloc(&_solver);
            }

            void set_verbosity(int verbosity) { _verbosity = verbosity; }

            void reset(int nr_in, int nr_steps)
            {
                _nr_in = nr_in;
                _nr_steps = nr_steps;
                _tt_size = (1 << _nr_in) - 1;
            }

            void create_variables(const TT& spec)
            {
                solver_restart(&_solver);

                _nr_op_vars = _nr_steps * 3;
                _nr_sim_vars = _nr_steps * _tt_size;
                if (_verbosity > 1) {
                    printf("nr_op_vars=%d\n", _nr_op_vars);
                    printf("nr_sim_vars=%d\n", _nr_sim_vars);
                }
                
                solver_set_nr_vars(_solver, _nr_op_vars + _nr_sim_vars);
            }

            int
            dag_get_op_var(int i, int c, int b)
            {
                assert(i < _nr_steps);
                assert(b < 2);
                assert(c < 2);
                assert(b > 0 || c > 0);

                return i * 3 + ( c << 1 ) + b - 1;
            }

            int 
            dag_get_sim_var(int i, int t)
            {
                assert(i < _nr_steps);
                assert(t < _nr_sim_vars);

                return _nr_op_vars + _tt_size * i + t;
            }

            bool add_simulation_clause(
                    int i, int j, int k, 
                    int t, int a, int b, int c)
            {
                int pLits[4], ctr = 0;

                if (j < _nr_in) {
                    if (( ( ( t + 1 ) & ( 1 << j ) ) ? 1 : 0 ) != b) {
                        return true;
                    }
                } else {
                    pLits[ctr++] = pabc::Abc_Var2Lit(
                            dag_get_sim_var(j - _nr_in, t), b);
                }

                if (k < _nr_in) {
                    if (( ( ( t + 1 ) & ( 1 << k ) ) ? 1 : 0 ) != c)
                        return true;
                } else {
                    pLits[ctr++] = pabc::Abc_Var2Lit(
                            dag_get_sim_var(k - _nr_in, t), c);
                }

                pLits[ctr++] = pabc::Abc_Var2Lit(dag_get_sim_var(i, t), a);

                if (b | c) {
                    pLits[ctr++] = pabc::Abc_Var2Lit(dag_get_op_var(i, c, b), 1 - a);
                    //printf("%sf_%d_%d_%d\n", (1-a) ? "!" : "", i+1, c, b);
                }

                return solver_add_clause(_solver, pLits, pLits + ctr);
            }

            bool create_tt_clauses(const TT& spec, const dag<2>& dag, int t)
            {
                int pLits[2];

                auto ret = true;

                for (int i = 0; i < _nr_steps; i++) {
                    auto vertex = dag.get_vertex(i);
                    int j = vertex.first;
                    int k = vertex.second;

                    ret &= add_simulation_clause(i, j, k, t, 0, 0, 1);
                    ret &= add_simulation_clause(i, j, k, t, 0, 1, 0);
                    ret &= add_simulation_clause(i, j, k, t, 0, 1, 1);
                    ret &= add_simulation_clause(i, j, k, t, 1, 0, 0);
                    ret &= add_simulation_clause(i, j, k, t, 1, 0, 1);
                    ret &= add_simulation_clause(i, j, k, t, 1, 1, 0);
                    ret &= add_simulation_clause(i, j, k, t, 1, 1, 1);

                    // If an output has selected this particular operand, we
                    // need to ensure that this operand's truth table satisfies
                    // the specified output function.
                    if (i == _nr_steps-1) {
                        if (_verbosity > 1) {
                            printf("bit %d=%d", t+2, kitty::get_bit(spec, t+1));
                            printf("\tvar=%d\n", dag_get_sim_var(i, t));
                        }
                        auto outbit = kitty::get_bit(spec, t+1);
                        pLits[0] = pabc::Abc_Var2Lit(dag_get_sim_var(i, t), 
                                1 - outbit);
                        ret &= solver_add_clause(_solver,pLits,pLits+1);
                    }
                }

                return ret;
            }

            bool create_main_clauses(const TT& spec, const dag<2>& dag)
            {
                bool success = true;
                for (int t = 0; t < _tt_size; t++) {
                    success &= create_tt_clauses(spec, dag, t);
                }
                return success;
            }

            bool create_nontriv_clauses()
            {
                bool success = true;
                int pLits[3];

                for (int i = 0; i < _nr_steps; i++) {
                    pLits[0] = pabc::Abc_Var2Lit(dag_get_op_var(i, 0, 1), 0);
                    pLits[1] = pabc::Abc_Var2Lit(dag_get_op_var(i, 1, 0), 0);
                    pLits[2] = pabc::Abc_Var2Lit(dag_get_op_var(i, 1, 1), 0);
                    success &= solver_add_clause(_solver, pLits, pLits + 3);

                    pLits[0] = pabc::Abc_Var2Lit(dag_get_op_var(i, 0, 1), 0);
                    pLits[1] = pabc::Abc_Var2Lit(dag_get_op_var(i, 1, 0), 1);
                    pLits[2] = pabc::Abc_Var2Lit(dag_get_op_var(i, 1, 1), 1);
                    success &= solver_add_clause(_solver, pLits, pLits + 3);

                    pLits[0] = pabc::Abc_Var2Lit(dag_get_op_var(i, 0, 1), 1);
                    pLits[1] = pabc::Abc_Var2Lit(dag_get_op_var(i, 1, 0), 0);
                    pLits[2] = pabc::Abc_Var2Lit(dag_get_op_var(i, 1, 1), 1);
                    success &= solver_add_clause(_solver, pLits, pLits + 3);
                }
                return success;
            }

            bool add_clauses(const TT& spec, const dag<2>& dag)
            {
                bool success = true;
                create_variables(spec);
                success &= create_main_clauses(spec, dag);
            //    success &= create_nontriv_clauses();
                return success;
            }

            void 
            dag_chain_extract(const dag<2>& dag, chain<2>& chain, bool invert)
            {
                int op_inputs[2];

                chain.reset(_nr_in, 1, _nr_steps);

                for (int i = 0; i < _nr_steps; i++) {
                    kitty::static_truth_table<2> op;
                    if (solver_var_value(_solver, dag_get_op_var(i, 0, 1 )))
                        kitty::set_bit(op, 1); 
                    if (solver_var_value(_solver, dag_get_op_var(i, 1, 0)))
                        kitty::set_bit(op, 2); 
                    if (solver_var_value(_solver, dag_get_op_var(i, 1, 1)))
                        kitty::set_bit(op, 3); 

                    if (_verbosity) {
                        printf("  step x_%d performs operation\n  ",i+_nr_in+1);
                        kitty::print_binary(op, std::cout);
                        printf("\n");
                    }

                    auto vertex = dag.get_vertex(i);
                    if (_verbosity) {
                        printf("  with operands x_%d and x_%d", 
                                vertex.first+1, vertex.second+1);
                    }
                    op_inputs[0] = vertex.first;
                    op_inputs[1] = vertex.second;
                    chain.set_step(i, op_inputs, op);

                    if (_verbosity) {
                        printf("\n");
                    }
                }

                chain.set_output(0, ((_nr_in + _nr_steps) << 1));
                if (invert) {
                    chain.invert();
                }
            }

            synth_result 
            synthesize(const TT& spec, const dag<2>& dag, chain<2>& chain)
            {
                TT local_spec = spec;

                bool invert = false;
                if (!is_normal(spec)) {
                    local_spec = ~local_spec;
                    invert = true;
                }
                if (!add_clauses(local_spec, dag)) {
                    return failure;
                }
                auto status = solver_solve(_solver, 0, 0, 0);
                if (status == success) {
                    dag_chain_extract(dag, chain, invert);
                }
                return status;
            }
            
            *******************************************************************
                Synthesizes while allowing for different input permutations.
            *******************************************************************
            synth_result 
            perm_synthesize(
                    const TT& spec, 
                    dag<2>& dag, 
                    chain<2>& chain, 
                    std::vector<int>& perm)
            {
                const auto num_vars = spec.num_vars();
                assert(num_vars <= 6 && perm.size() == num_vars);

                if (num_vars <= 2) {
                    return synthesize(spec, dag, chain);
                }

                bool invert = false;
                TT local_spec = spec;
                if (!is_normal(spec)) {
                    local_spec = ~local_spec;
                    invert = true;
                }
                        
                std::iota( perm.begin(), perm.end(), 0u );
                if (add_clauses(local_spec, dag)) {
                    auto status = solver_solve(_solver, 0, 0, 0);
                    if (status == success) {
                        dag_chain_extract(dag, chain, invert);
                        return success;
                    }
                }

                const auto& swaps = kitty::detail::swaps[num_vars - 2u];
                for (std::size_t i = 0; i < swaps.size(); i++) {
                    const auto pos = swaps[i];

                    dag.swap_adjacent_inplace(pos);

                    if (!add_clauses(local_spec, dag)) {
                        continue;
                    }
                    auto status = solver_solve(_solver, 0, 0, 0);
                    if (status == success) {
                        dag_chain_extract(dag, chain, invert);
                        for (auto j = 0; j <= i; j++) {
                            const auto pos = swaps[i];
                            std::swap(perm[pos], perm[pos + 1]);
                        }
                        return success;
                    }
                }

                return failure;
            }

            void print_solver_state()
            {
                printf("\n");
                printf("========================================"
                        "========================================\n");
                printf("  SOLVER STATE\n\n");

                for (int i = 0; i < _nr_steps; i++) {
                    printf("  f_%d = ", _nr_in+i+1);
                    printf("%d", solver_var_value(_solver, 
                                dag_get_op_var(i, 1, 1)));
                    printf("%d", solver_var_value(_solver, 
                                dag_get_op_var(i, 1, 0)));
                    printf("%d", solver_var_value(_solver, 
                                dag_get_op_var(i, 0, 1)));
                    printf("0\n");
                    printf("  tt_%d = ", _nr_in+i+1);
                    for (int t = _tt_size - 1; t >= 0; t--) {
                        printf("%d", solver_var_value(_solver, 
                                    dag_get_sim_var(i, t)));
                    }
                    printf("0\n\n");
                }

                for (int i = 0; i < _nr_steps; i++) {
                    printf("\n");
                    printf("f_%d_0_0=0\n", _nr_in+i+1);
                    printf("f_%d_0_1=%d\n", _nr_in+i+1, solver_var_value(
                                _solver, dag_get_op_var(i, 0, 1)));
                    printf("f_%d_1_0=%d\n", _nr_in+i+1, solver_var_value(
                                _solver, dag_get_op_var(i, 1, 0)));
                    printf("f_%d_1_1=%d\n", _nr_in+i+1, solver_var_value(
                                _solver, dag_get_op_var(i, 1, 1)));
                    printf("\n");
                    for (int t = _tt_size - 1; t >= 0; t--) {
                        printf("x_%d_%d=%d\n", _nr_in+i+1, t+1, 
                                solver_var_value(
                                    _solver, dag_get_sim_var(i, t)));
                    }
                    printf("x_%d_0=0\n", _nr_in+i);
                    printf("\n");
                }
                printf("\n");

                printf("========================================"
                        "========================================\n");
            }
    };
*/


    /***************************************************************************
        A DAG generator based on a SAT formulation.
    ***************************************************************************/
    class sat_dag_generator
    {
        private:
            bsat_wrapper _solver;
            pabc::Vec_Int_t* _vLits; // Dynamic vector of literals for clauses.
            int _nr_vars;
            int _nr_vertices;
            bool _reset;
            int _verbosity = 0;

            // If true, the generate will only create DAGs that correspond
            // to functions with full support.
            bool _gen_true_dags = false;

            // If true, generates only connected DAGs.
            bool _gen_connected_dags = true;
            
            // If true, generates colexicographically ordered DAGs.
            bool _gen_colex_dags = true;
            
            // If true, generates DAGs without potential reapplication of
            // operands.
            bool _gen_noreapply_dags = true;

            int dag_get_sel_var(int i, int j, int k)
            {
                int offset = 0;

                assert(i < _nr_vertices);
                assert(k < _nr_vars + i);
                assert(j < k);

                for (int a = _nr_vars; a < _nr_vars + i; a++)
                    offset += a * ( a - 1 ) / 2;

                return offset + ((-j * ( 1 + j - 2 * (_nr_vars + i))) / 2) +
                    (k - j - 1);
            }

            void create_variables()
            {
                int nr_sel_vars = 0;
                for (int i = _nr_vars; i < _nr_vars + _nr_vertices; i++) {
                    nr_sel_vars += ( i * ( i - 1 ) ) / 2;
                    /*if (_verbosity > 1) {
                        printf("adding %d svars\n", ( i * ( i - 1 ) ) / 2);
                    }*/
                }
                /*
                if (_verbosity > 1) {
                    printf("creating %d svars\n", nr_sel_vars);
                }
                */
                _solver.set_nr_vars(nr_sel_vars);
                pabc::Vec_IntGrowResize(_vLits, nr_sel_vars);
            }

            /*******************************************************************
                Ensures that each vertex has fanin 2.
            *******************************************************************/
            void 
            create_op_clauses()
            {
                for (int i = 0; i < _nr_vertices; i++)
                {
                    int ctr = 0;
                    for (int k = 1; k < _nr_vars + i; k++) {
                        for (int j = 0; j < k; j++) {
                            pabc::Vec_IntSetEntry(_vLits, ctr++, 
                                    pabc::Abc_Var2Lit(dag_get_sel_var(i, j, k ), 0 ));
                        }
                    }
                    _solver.add_clause(
                        pabc::Vec_IntArray(_vLits), 
                        pabc::Vec_IntArray(_vLits) + ctr);
                }
            }

            /*******************************************************************
              Add clauses which ensure that every vertex is used at least once.
             ******************************************************************/
            void 
            create_alonce_clauses()
            {
                // Make sure that all input variables are covered
                /*
                if (_gen_true_dags) {
                    for (int j = 0; j < _nr_vars; j++) {
                        auto ctr = 0;
                        for (int i = 0; i < _nr_vertices; i++) {
                            for (int k = j+1; k < _nr_vars + i; k++) {
                                Vec_IntSetEntry(_vLits, ctr++, pabc::Abc_Var2Lit(
                                            dag_get_sel_var(i, j, k), 0));
                            }
                            for (int k = 0; k < j; k++) {
                                pabc::Vec_IntSetEntry(_vLits, ctr++, pabc::Abc_Var2Lit(
                                            dag_get_sel_var(i, k, j), 0));
                            }
                        }
                        solver_add_clause(_solver, pabc::Vec_IntArray(_vLits),
                                pabc::Vec_IntArray(_vLits) + ctr);
                    }
                }
                */
                // Make sure that all internal vertices are covered
                for (int i = 0; i < _nr_vertices - 1; i++) {
                    auto ctr = 0;
                    for (int ip = i + 1; ip < _nr_vertices; ip++) {
                        for (int j = 0; j < _nr_vars+i; j++) {
                            pabc::Vec_IntSetEntry(_vLits, ctr++, pabc::Abc_Var2Lit(
                                        dag_get_sel_var(ip, j, _nr_vars+i), 0));
                        }
                        for (int j = _nr_vars+i+1; j < _nr_vars+ip; j++) {
                            pabc::Vec_IntSetEntry(_vLits, ctr++, pabc::Abc_Var2Lit(
                                        dag_get_sel_var(ip, _nr_vars+i, j), 0));
                        }
                    }
                    _solver.add_clause(pabc::Vec_IntArray(_vLits), pabc::Vec_IntArray(_vLits) + ctr);
                }
            }

            void create_unique_fanin_clauses()             
            { 
                int pLits[2];

                for (int i = 0; i < _nr_vertices; i++) {
                    for (int k = 2; k < _nr_vars+i; k++) {
                        for (int j = 0; j < k; j++) {
                            for (int kp = 1; kp <= k; kp++) {
                                for (int jp = 0; jp < kp; jp++) {
                                    if (kp == k && jp >= j) {
                                        break;
                                    }
                                    pLits[0] = 
                                        pabc::Abc_Var2Lit(dag_get_sel_var(i,j,k),1);
                                    pLits[1] = pabc::Abc_Var2Lit(
                                            dag_get_sel_var(i,jp,kp),1);
                                    _solver.add_clause(pLits, pLits+2);
                                }
                            }
                        }
                    }
                }
            }

            /*******************************************************************
                Add clauses which ensure that dag vertices occur in
                co-lexicographic order. This ensures that we do not generate
                many symmetric dags.
            *******************************************************************/
            void create_colex_clauses()
            {
                int pLits[2];

                for (int i = 0; i < _nr_vertices-1; i++) {
                    for (int k = 2; k < _nr_vars+i; k++) {
                        for (int j = 1; j < k; j++) {
                            for (int jp = 0; jp < j; jp++) {
                                pLits[0] = pabc::Abc_Var2Lit(
                                        dag_get_sel_var(i, j, k), 1);
                                pLits[1] = pabc::Abc_Var2Lit(
                                        dag_get_sel_var(i+1, jp, k), 1);
                                _solver.add_clause(pLits, pLits+2);
                            }
                        }
                        for (int j = 0; j < k; j++) {
                            for (int kp = 1; kp < k; kp++) {
                                for (int jp = 0; jp < kp; jp++) {
                                    pLits[0] = pabc::Abc_Var2Lit(
                                            dag_get_sel_var(i, j, k), 1);
                                    pLits[1] = pabc::Abc_Var2Lit(
                                            dag_get_sel_var(i+1, jp, kp),1);
                                    _solver.add_clause(pLits, pLits + 2);
                                }
                            }
                        }
                    }
                }
            }

            /*******************************************************************
                Add clauses which ensure that operands are never re-applied. In
                other words, (Sijk --> ~Si'ji) & (Sijk --> ~Si'ki)
                for all (i < i').
            *******************************************************************/
            void create_noreapply_clauses()
            {
                int pLits[2];

                for (int i = 0; i < _nr_vertices; i++) {
                    for (int ip = i+1; ip < _nr_vertices; ip++) {
                        for (int k = 1; k < _nr_vars+i; k++) {
                            for (int j = 0; j < k; j++) {
                                pLits[0] = pabc::Abc_Var2Lit(
                                        dag_get_sel_var(i, j, k), 1);
                                pLits[1] = pabc::Abc_Var2Lit(
                                        dag_get_sel_var(ip, j, _nr_vars+i),1);
                                _solver.add_clause(pLits, pLits + 2);
                                pLits[1] = pabc::Abc_Var2Lit(
                                        dag_get_sel_var(ip, k, _nr_vars+i),1);
                                _solver.add_clause(pLits, pLits+2);
                            }
                        }
                    }
                }
            }

            void dag_extract(dag<2>& g)
            {
                g.reset(_nr_vars, _nr_vertices);

                for (int i = 0; i < _nr_vertices; i++) {
                    int found = 0;
                    for (int k = 1; (k < _nr_vars + i); k++) {
                        for (int j = 0; j < k; j++) {
                            if (_solver.var_value(dag_get_sel_var(i, j, k)))
                            {
                                g.set_vertex(i, j, k);
                                found++;
                            }

                        }
                    }
                    assert(found == 1);
                }
            }

            bool block_solution()
            {
                int ctr = 0;
                for (int i = 0; i < _nr_vertices; i++) {
                    int found = 0;
                    for (int k = 1; (k < (_nr_vars + i)); k++) {
                        for (int j = 0; j < k; j++) {
                            if (_solver.var_value(dag_get_sel_var(i, j, k)))
                            {
                                pabc::Vec_IntSetEntry(_vLits, ctr++,
                                        pabc::Abc_Var2Lit(dag_get_sel_var(i, j, k),
                                            1)); 
                                found++;
                            }
                        }
                    }
                    assert(found == 1);
                }
                assert(ctr == _nr_vertices);

                auto status = _solver.add_clause(
                    pabc::Vec_IntArray(_vLits),
                    pabc::Vec_IntArray(_vLits) + ctr);

                if (_verbosity > 1) {
                    printf("nr vars=%d, nr clauses=%d\n", _solver.nr_vars(), _solver.nr_clauses());
                }
                return status;
            }

        public:
            sat_dag_generator()
            {
                _vLits = pabc::Vec_IntAlloc(0);
            }

            ~sat_dag_generator()
            {
                pabc::Vec_IntFree(_vLits);
            }

            int nr_vars() const { return _nr_vars; }
            int nr_vertices() const { return _nr_vertices; }

            void gen_true_dags(bool gen) { _gen_true_dags = gen; }

            void verbosity(int verbosity) { _verbosity = verbosity; }
            int verbosity() { return _verbosity; }

            void gen_connected_dags(bool gen_connected_dags)
            {
                _gen_connected_dags = gen_connected_dags;
            }

            bool gen_connected_dags() { return _gen_connected_dags; }

            void gen_colex_dags(bool gen_colex_dags) 
            { 
                _gen_colex_dags = gen_colex_dags;
            }

            bool gen_colex_dags() { return _gen_colex_dags; }

            void gen_noreapply_dags(bool gen_noreapply_dags)
            {
                _gen_noreapply_dags = gen_noreapply_dags;
            }

            bool gen_noreapply_dags() { return _gen_noreapply_dags; }

            void reset(int nr_vars, int nr_vertices)
            {
                assert(nr_vars > 0);
                assert(nr_vertices > 0);

                if (_verbosity) {
                    printf("setting nr. vars=%d\n", nr_vars);
                    printf("setting nr. vertices=%d\n", nr_vertices);
                }

                _reset = true;
                _nr_vars = nr_vars;
                _nr_vertices = nr_vertices;
                _solver.restart();
                create_variables();
                create_op_clauses();
                create_unique_fanin_clauses();
                if (_gen_connected_dags) {
                    create_alonce_clauses();
                }
                if (_gen_colex_dags) {
                    create_colex_clauses();
                }
                if (_gen_noreapply_dags) {
                    create_noreapply_clauses();
                }


                if (_verbosity) {
                    printf("starting with %d vars, %d clauses\n", 
                            _solver.nr_vars(), 
                            _solver.nr_clauses());
                }
            }

            bool next_dag(dag<2>& g)
            {
                if (!_reset) {
                    // Block current solution
                    if (!block_solution()) {
                        return false;
                    }
                }
                _reset = false;
                auto status = _solver.solve(0, 0, 0);
                if (status == success) {
                    dag_extract(g);

                    /*
                    printf("Found solution: ");
                    for (auto i = 0; i < g.nr_vertices(); i++) {
                        const auto& step = g.get_vertex(i);
                        if (i > 0) {
                            printf(" - ");
                        }
                        printf("(%d, %d)", step.first+1, step.second+1);
                    }
                    printf("\n");
                    */
                    return true;
                } else {
                    return false;
                }
            }

            int count_dags() {
                dag<2> g;
                int counter = 0;
                while (next_dag(g)) {
                    counter++;
                }
                return counter;
            }
    };

    class unbounded_dag_generator
    {
        private:
            int nr_inputs;
            sat_dag_generator gen;

        public:
            unbounded_dag_generator()
            {
                nr_inputs = -1;
            }

            void reset(int n) {
                assert(n > 0);

                nr_inputs = n;
                gen.reset(nr_inputs, 1);
            }

            void gen_true_dags(bool true_dags) { gen.gen_true_dags(true_dags); }

            bool next_dag(dag<2>& g)
            {
                if (!gen.next_dag(g)) {
                    gen.reset(nr_inputs, gen.nr_vertices()+1);
                    return next_dag(g);
                }
                return true;
            }

    };

#if !defined(DISABLE_NAUTY)
    class nonisomorphic_dag_generator
    {
        private:
            int _nr_vars;
            unbounded_dag_generator _gen;
            std::vector<dag<2>> _dags;

        public:
            nonisomorphic_dag_generator()
            {
                _nr_vars = -1;
            }

            void reset(int nr_vars) {
                assert(nr_vars > 0);

                _nr_vars = nr_vars;
                _gen.reset(nr_vars);
                _dags.clear();
            }

            void gen_true_dags(bool gen) { _gen.gen_true_dags(gen); }

            int nr_dags() { return _dags.size(); }

            bool next_dag(dag<2>& g)
            {
                while (true) {
                    bool found_isomorphism = false;
                    _gen.next_dag(g);
                    const auto num_vertices = g.get_nr_vertices();
                    for (int i = (_dags.size()-1); i >= 0; i--) {
                        const auto& g2 = _dags[i];
                        if (g2.get_nr_vertices() != num_vertices) {
                            // We haven't found an isomorphism.
                            _dags.push_back(g);
                            return true;
                        }
                        if (g.is_isomorphic(g2)) {
                            // We've found an isomorphism, generate next DAG.
                            found_isomorphism = true;
                            break;
                        }
                    }
                    if (!found_isomorphism) {
                        _dags.push_back(g);
                        return true;
                    }
                }
            }

    };
#endif
    
    /***************************************************************************
        Generates DAGs using a recursive backtrack search.
    ***************************************************************************/
    class rec_dag_generator
    {
        private:
            int _nr_vars;
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

            // If true, generates only connected DAGs.
            bool _gen_connected_dags = true;

            // If true, generates colexicographically ordered DAGs.
            bool _gen_colex_dags = true;
            
            // If true, generates DAGs without potential reapplication of
            // operands.
            bool _gen_noreapply_dags = true;

            // The index at which backtracking should terminate.
            int _stop_level = -1;

            // Function to call when a solution is found.
            std::function<void(rec_dag_generator*)> _callback;


        public:
            rec_dag_generator() : _initialized(false)
            {
            }

            // Two arrays that represent the "stack" of selected steps.
            int _js[18];
            int _ks[18];

            // The level from which the search is assumed to have started
            int _start_level = 0;

            int nr_vars() const { return _nr_vars; }
            int nr_vertices() const { return _nr_vertices; }

            void verbosity(int verbosity) { _verbosity = verbosity; }
            int verbosity() { return _verbosity; }
            
            void gen_connected_dags(bool gen_connected_dags)
            {
                _gen_connected_dags = gen_connected_dags;
            }

            bool gen_connected_dags() { return _gen_connected_dags; }

            void gen_colex_dags(bool gen_colex_dags) 
            { 
                _gen_colex_dags = gen_colex_dags;
            }

            bool gen_colex_dags() { return _gen_colex_dags; }

            void gen_noreapply_dags(bool gen_noreapply_dags)
            {
                _gen_noreapply_dags = gen_noreapply_dags;
            }

            bool gen_noreapply_dags() { return _gen_noreapply_dags; }

            void set_callback(std::function<void(rec_dag_generator*)>& f)
            {
                _callback = f;
            }

            void set_callback(std::function<void(rec_dag_generator*)>&& f)
            {
                _callback = std::move(f);
            }

            void clear_callback()
            {
                _callback = 0;
            }

            void reset(int nr_vars, int nr_vertices)
            {
                assert(nr_vars > 0);
                assert(nr_vertices > 0);
                
                if (_verbosity) {
                    printf("setting nr. vars=%d\n", nr_vars);
                    printf("setting nr. vertices=%d\n", nr_vertices);
                }
                
                _nr_vars = nr_vars;
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

                _js[0] = 0;
                _ks[0] = 1;

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
                            printf("(%d, %d)", j+1, k+1);
                        }
                        printf("\n");
                    }
                    backtrack();
                } else {
                    for (int k = 1; k < _nr_vars + _level; k++) {
                        for (int j = 0; j < k; j++) {
                            _js[_level] = j;
                            _ks[_level] = j;
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
                    for (int i = _nr_vars; i < _nr_vars+_nr_vertices-1; i++) {
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
                            printf("(%d, %d)", j+1, k+1);
                        }
                        printf("\n");
                    }
                    backtrack();
                } else {
                    for (int k = 1; k < _nr_vars + _level; k++) {
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
                _level = _start_level;
                _nr_solutions = 0;

                search_colex_dags();

                return _nr_solutions;
            }

            void search_colex_dags()
            {
                if (_level == _nr_vertices) {
                    for (int i = _nr_vars; i < _nr_vars+_nr_vertices-1; i++) {
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
                        for (int i = 1; i <= _nr_vertices; i++) {
                            const auto j = _js[i];
                            const auto k = _ks[i];
                            if (i > 0) {
                                printf(" - ");
                            }
                            printf("(%d, %d)", j+1, k+1);
                        }
                        printf("\n");
                    }
                    backtrack();
                } else {
                    // We are only interested in DAGs that are in
                    // co-lexicographical order. Look at the previous step
                    // on the stack, the current step should be greater or
                    // equal to it.
                    const auto start_j = _js[_level];
                    const auto start_k = _ks[_level];

                    _ks[_level] = start_k;
                    for (int j = start_j; j < start_k; j++) {
                        ++_covered_steps[j];
                        ++_covered_steps[start_k];
                        ++_level;
                        _js[_level] = j;
                        search_colex_dags();
                    }
                    for (int k = start_k+1; k < _nr_vars + _level; k++) {
                        for (int j = 0; j < k; j++) {
                            ++_covered_steps[j];
                            ++_covered_steps[k];
                            ++_level;
                            _js[_level] = j;
                            _ks[_level] = k;
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

                search_noreapply_dags();

                return _nr_solutions;
            }

            void search_noreapply_dags()
            {
                if (_level == _nr_vertices) {
                    ++_nr_solutions;
                    if (_callback) {
                        _callback(this);
                    }
                    if (_verbosity) {
                        printf("Found solution: ");
                        for (int i = 1; i <= _nr_vertices; i++) {
                            const auto j = _js[i];
                            const auto k = _ks[i];
                            if (i > 0) {
                                printf(" - ");
                            }
                            printf("(%d, %d)", j+1, k+1);
                        }
                        printf("\n");
                    }
                    noreapply_backtrack();
                } else {
                    // We are only interested in DAGs that are in
                    // co-lexicographical order. Look at the previous step
                    // on the stack, the current step should be greater or
                    // equal to it.
                    const auto start_j = _js[_level];
                    const auto start_k = _ks[_level];

                    _ks[_level+1] = start_k;
                    for (int j = start_j; j < start_k; j++) {
                        if (_disabled_matrix[_level][j][start_k]) {
                            continue;
                        }

                        // If we choose fanin (j, k), record that j and k
                        // are covered.
                        ++_covered_steps[j];
                        ++_covered_steps[start_k];

                        int uncovered = 0;
                        for (int i = _nr_vars; i <= _nr_vars+_level; i++) {
                            if (_covered_steps[i] == 0) {
                                ++uncovered;
                            }
                        }
                        if (uncovered > _nr_vertices-_level) {
                            --_covered_steps[j];
                            --_covered_steps[start_k];
                            continue;
                        }

                        // We are adding step (i, j, k). This means that we
                        // don't have to consider steps (i',j,i) or (i',k,i)
                        // for i < i' <= n+r. This avoiding reapplying an
                        // operand.
                        for (int ip = _level+1; ip < _nr_vertices; ip++) {
                            ++_disabled_matrix[ip][j][_nr_vars+_level];
                            ++_disabled_matrix[ip][start_k][_nr_vars+_level];
                        }

                        ++_level;
                        _js[_level] = j;
                        search_noreapply_dags();
                    }
                    for (int k = start_k+1; k < _nr_vars + _level; k++) {
                        for (int j = 0; j < k; j++) {
                            if (_disabled_matrix[_level][j][k]) {
                                continue;
                            }
                            ++_covered_steps[j];
                            ++_covered_steps[k];

                            int uncovered = 0;
                            for (int i = _nr_vars; i <= _nr_vars+_level; i++) {
                                if (_covered_steps[i] == 0) {
                                    ++uncovered;
                                }
                            }
                            if (uncovered > _nr_vertices-_level) {
                                --_covered_steps[j];
                                --_covered_steps[k];
                                continue;
                            }

                            for (int ip = _level+1; ip < _nr_vertices; ip++) {
                                ++_disabled_matrix[ip][j][_nr_vars+_level];
                                ++_disabled_matrix[ip][k][_nr_vars+_level];
                            }
                            ++_level;
                            _js[_level] = j;
                            _ks[_level] = k;

                            search_noreapply_dags();
                        }
                    }

                    noreapply_backtrack();
                }
            }

            // TODO: fix this function to work with new synthesis API
            void search_dags(
                    spec& spec, 
                    dag<2>& g, 
                    chain& chain)
            {
                // We can terminate early if a solution has already been found.
                if (_nr_solutions > 0) {
                    return;
                } else if (_level == _nr_vertices) {
                    for (int i = 1; i <= _nr_vertices; i++) {
                        const auto j = _js[i];
                        const auto k = _ks[i];
                        g.set_vertex(i-1, j, k);
                    }
                    const auto result = success; // TODO: fix! dag_synthesize(spec, g, chain);
                    if (result == success) {
                        ++_nr_solutions;
                        if (_verbosity) {
                            printf("Found solution: ");
                            for (int i = 1; i <= _nr_vertices; i++) {
                                const auto j = _js[i];
                                const auto k = _ks[i];
                                if (i > 0) {
                                    printf(" - ");
                                }
                                printf("(%d, %d)", j+1, k+1);
                            }
                            printf("\n");
                        }
                    }
                    noreapply_backtrack();
                } else {
                    // We are only interested in DAGs that are in
                    // co-lexicographical order. Look at the previous step
                    // on the stack, the current step should be greater or
                    // equal to it.
                    const auto start_j = _js[_level];
                    const auto start_k = _ks[_level];

                    _ks[_level+1] = start_k;
                    for (int j = start_j; j < start_k && !_nr_solutions; j++) {
                        if (_disabled_matrix[_level][j][start_k]) {
                            continue;
                        }

                        // If we choose fanin (j, k), record that j and k
                        // are covered.
                        ++_covered_steps[j];
                        ++_covered_steps[start_k];

                        int uncovered = 0;
                        for (int i = _nr_vars; i <= _nr_vars+_level; i++) {
                            if (_covered_steps[i] == 0) {
                                ++uncovered;
                            }
                        }
                        if (uncovered > _nr_vertices-_level) {
                            --_covered_steps[j];
                            --_covered_steps[start_k];
                            continue;
                        }

                        // We are adding step (i, j, k). This means that we
                        // don't have to consider steps (i',j,i) or (i',k,i)
                        // for i < i' <= n+r. This avoiding reapplying an
                        // operand.
                        for (int ip = _level+1; ip < _nr_vertices; ip++) {
                            ++_disabled_matrix[ip][j][_nr_vars+_level];
                            ++_disabled_matrix[ip][start_k][_nr_vars+_level];
                        }

                        ++_level;
                        _js[_level] = j;
                        search_dags(spec, g, chain);
                    }
                    for (int k = start_k+1; (k < _nr_vars+_level && 
                            !_nr_solutions); k++) {
                        for (int j = 0; j < k && !_nr_solutions; j++) {
                            if (_disabled_matrix[_level][j][k]) {
                                continue;
                            }
                            ++_covered_steps[j];
                            ++_covered_steps[k];

                            int uncovered = 0;
                            for (int i = _nr_vars; i <= _nr_vars+_level; i++) {
                                if (_covered_steps[i] == 0) {
                                    ++uncovered;
                                }
                            }
                            if (uncovered > _nr_vertices-_level) {
                                --_covered_steps[j];
                                --_covered_steps[k];
                                continue;
                            }

                            for (int ip = _level+1; ip < _nr_vertices; ip++) {
                                ++_disabled_matrix[ip][j][_nr_vars+_level];
                                ++_disabled_matrix[ip][k][_nr_vars+_level];
                            }
                            ++_level;
                            _js[_level] = j;
                            _ks[_level] = k;

                            search_dags(spec, g, chain);
                        }
                    }

                    noreapply_backtrack();
                }
            }

            void 
            qpsearch_dags(
                    dag<2>& g, 
                    moodycamel::ConcurrentQueue<dag<2>>& q, 
                    bool* found)
            {
                if (*found) {
                    return;
                } else if (_level == _nr_vertices) {
                    g.reset(_nr_vars, _nr_vertices);
                    for (int i = 1; i <= _nr_vertices; i++) {
                        const auto j = _js[i];
                        const auto k = _ks[i];
                        g.set_vertex(i-1, j, k);
                    }
                    while (!q.try_enqueue(g)) {
                        std::this_thread::yield();
                        // It's possible that a consumer thread finds a
                        // solution while the queue is full.
                        if (*found) {
                            break;
                        }
                    }
                    noreapply_backtrack();
                } else {
                    // We are only interested in DAGs that are in
                    // co-lexicographical order. Look at the previous step
                    // on the stack, the current step should be greater or
                    // equal to it.
                    const auto start_j = _js[_level];
                    const auto start_k = _ks[_level];

                    _ks[_level+1] = start_k;
                    for (int j = start_j; j < start_k && !(*found); j++) {
                        if (_disabled_matrix[_level][j][start_k]) {
                            continue;
                        }

                        // If we choose fanin (j, k), record that j and k
                        // are covered.
                        ++_covered_steps[j];
                        ++_covered_steps[start_k];

                        int uncovered = 0;
                        for (int i = _nr_vars; i <= _nr_vars+_level; i++) {
                            if (_covered_steps[i] == 0) {
                                ++uncovered;
                            }
                        }
                        if (uncovered > _nr_vertices-_level) {
                            --_covered_steps[j];
                            --_covered_steps[start_k];
                            continue;
                        }

                        // We are adding step (i, j, k). This means that we
                        // don't have to consider steps (i',j,i) or (i',k,i)
                        // for i < i' <= n+r. This avoiding reapplying an
                        // operand.
                        for (int ip = _level+1; ip < _nr_vertices; ip++) {
                            ++_disabled_matrix[ip][j][_nr_vars+_level];
                            ++_disabled_matrix[ip][start_k][_nr_vars+_level];
                        }

                        ++_level;
                        _js[_level] = j;
                        qpsearch_dags(g, q, found);
                    }
                    for (int k = start_k+1; k < _nr_vars+_level && 
                            !(*found); k++) {
                        for (int j = 0; j < k && !(*found); j++) {
                            if (_disabled_matrix[_level][j][k]) {
                                continue;
                            }
                            ++_covered_steps[j];
                            ++_covered_steps[k];

                            int uncovered = 0;
                            for (int i = _nr_vars; i <= _nr_vars+_level; i++) {
                                if (_covered_steps[i] == 0) {
                                    ++uncovered;
                                }
                            }
                            if (uncovered > _nr_vertices-_level) {
                                --_covered_steps[j];
                                --_covered_steps[k];
                                continue;
                            }

                            for (int ip = _level+1; ip < _nr_vertices; ip++) {
                                ++_disabled_matrix[ip][j][_nr_vars+_level];
                                ++_disabled_matrix[ip][k][_nr_vars+_level];
                            }
                            ++_level;
                            _js[_level] = j;
                            _ks[_level] = k;

                            qpsearch_dags(g, q, found);
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
                const auto j = _js[_level];
                const auto k = _ks[_level];
                --_level;
                if (_level > _stop_level) {
                    --_covered_steps[j];
                    --_covered_steps[k];
                    for (int ip = _level+1; ip < _nr_vertices; ip++) {
                        --_disabled_matrix[ip][j][_nr_vars+_level];
                        --_disabled_matrix[ip][k][_nr_vars+_level];
                    }
                }
            }
            
            auto count_dags()
            {
                if (!_gen_connected_dags) {
                    return count_tuples();
                } else if (!_gen_colex_dags) {
                    return count_connected_dags();
                } else if (!_gen_noreapply_dags) {
                    return count_colex_dags();
                } else {
                    return count_noreapply_dags();
                }
            }

            void add_selection(int j, int k)
            {
                assert(_level < _nr_vertices);
                for (int ip = _level+1; ip < _nr_vertices; ip++) {
                    ++_disabled_matrix[ip][j][_nr_vars+_level];
                    ++_disabled_matrix[ip][k][_nr_vars+_level];
                }
                ++_level;
                ++_stop_level;
                _js[_level] = j;
                _ks[_level] = k;
                ++_covered_steps[j];
                ++_covered_steps[k];
            }

            std::vector<dag<2>> 
            gen_dags()
            {
                assert(_initialized);
                dag<2> g;
                std::vector<dag<2>> dags;
                
                const auto nr_vars = _nr_vars;
                const auto nr_vertices = _nr_vertices;

                g.reset(nr_vars, nr_vertices);
                set_callback([&dags, &g, nr_vertices]
                        (rec_dag_generator* gen) {
                            for (int i = 1; i <= nr_vertices; i++) {
                                g.set_vertex(i-1, gen->_js[i], gen->_ks[i]);
                            }
                            dags.push_back(g);
                        }
                );
                count_dags();
                clear_callback();

                return dags;
            }

            /*******************************************************************
                Enumerates DAGs until one is found that can implement the given
                function. Fails if no DAG of the current size can implement
                the function.
            *******************************************************************/
            synth_result 
            find_dag(
                    spec& spec, 
                    dag<2>& g, 
                    chain& chain)
            {
                assert(_initialized);

                search_dags(spec, g, chain);

                if (_nr_solutions > 0) {
                    return success;
                } else {
                    return failure;
                }
            }

            /*******************************************************************
                Parallel version of find_dag based on putting enumerated DAGs
                into a concurrent queue.
            *******************************************************************/
            void 
            qpfind_dag(moodycamel::ConcurrentQueue<dag<2>>& q, bool* found)
            {
                dag<2> g;
                assert(_initialized);

                assert(_nr_solutions == 0);
                qpsearch_dags(g, q, found);
            }

#ifndef DISABLE_NAUTY
            uint64_t 
            count_non_isomorphic_dags()
            {
                assert(_initialized);
                dag<2> g;
                std::vector<dag<2>> dags;
                const auto nr_vars = _nr_vars;
                const auto nr_vertices = _nr_vertices;

                g.reset(nr_vars, nr_vertices);
                set_callback([&dags, &g, nr_vars, nr_vertices]
                        (rec_dag_generator* gen) {
                            for (int i = 1; i <= nr_vertices; i++) {
                                g.set_vertex(i-1, gen->_js[i], gen->_ks[i]);
                            }
                            bool is_isomorphic = false;
                            for (const auto& dag : dags) {
                                if (dag.is_isomorphic(g)) {
                                    is_isomorphic = true;
                                    break;
                                }
                            }
                            if (!is_isomorphic) {
                                dags.push_back(g);
                            }
                        }
                );
                count_dags();
                clear_callback();

                return uint64_t(dags.size());
            }

            std::vector<dag<2>> 
            gen_non_isomorphic_dags()
            {
                assert(_initialized);
                dag<2> g;
                std::vector<dag<2>> dags;
                const auto nr_vars = _nr_vars;
                const auto nr_vertices = _nr_vertices;

                g.reset(nr_vars, nr_vertices);
                set_callback([&dags, &g, nr_vars, nr_vertices]
                        (rec_dag_generator* gen) {
                            for (int i = 1; i <= nr_vertices; i++) {
                                g.set_vertex(i-1, gen->_js[i], gen->_ks[i]);
                            }
                            bool is_isomorphic = false;
                            for (const auto& dag : dags) {
                                if (dag.is_isomorphic(g)) {
                                    is_isomorphic = true;
                                    break;
                                }
                            }
                            if (!is_isomorphic) {
                                dags.push_back(g);
                            }
                        }
                );
                count_dags();
                clear_callback();

                return dags;
            }
#endif
    };
    
}

