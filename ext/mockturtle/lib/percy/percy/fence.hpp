#pragma once

#include <vector>
#include <cstdio>
#include <algorithm>
#include <cassert>
#include <memory>
#include <functional>
#include "concurrentqueue.h"
#include <cmath>
#include <thread>
#include <percy/spec.hpp>

/*******************************************************************************
    Definitions of Boolean fences and fence filters and generators.

    A fence generator creates a stream of Boolean fences based on some set of
    specifictions (e.g. the number of nodes and levels). A fence filter is a
    special type of generator that is given a fence stream and creates a new
    stream out of it (i.e. by altering the existing stream). By chaining
    different generators/filters together, we can create customized fence
    streams for our particular synthesis demands.
*******************************************************************************/
namespace percy
{
    class fence
    {
        private:
            std::vector<int> _fence;
            int _nr_nodes;
            int _nr_levels;

        public:
            fence() { reset(0, 0); }

            fence(const fence& f) 
            {
                reset(f._nr_nodes, f._nr_levels);
                for (int i = 0; i < _nr_levels; i++) {
                    _fence[i] = f._fence[i];
                }
            }

            fence& operator=(const fence& f)
            {
                reset(f._nr_nodes, f._nr_levels);
                for (int i = 0; i < _nr_levels; i++) {
                    _fence[i] = f._fence[i];
                }

                return *this;
            }

            fence(fence&& f) 
            {
                _nr_nodes = f._nr_nodes;
                _nr_levels = f._nr_levels;
                _fence = std::move(f._fence);
            }
            
            fence(int k, int l) { reset(k, l); }

            void reset(int nr_nodes, int nr_levels)
            { 
                _nr_nodes = nr_nodes;
                _nr_levels = nr_levels;
                _fence.resize(nr_levels);
            }
            
            int nr_nodes() const
            { 
                return _nr_nodes;
            }

            int nr_levels() const
            { 
                return _nr_levels;
            }

            int& operator[] (const int index)
            {
                return _fence[index];
            }

            int at(const int index) const
            {
                return _fence[index];
            }
    };

    class fence_generator
    {
        public:
            virtual bool next_fence(fence& f) = 0;
    };

    /***************************************************************************
        Generates Boolean fences based on an integer partition algorithm. This
        implementation  is based on a Python implementation at
        http://www.ics.uci.edu/~eppstein/PADS/IntegerPartitions.py, which
        itself was adapted from Knuth v4 fasc3 p38. Knuth credits it to
        Hindenburg, 1779.
    ***************************************************************************/
    class partition_generator : public fence_generator
    {
        private:
            int _nnodes, _nlevels;
            std::vector<int> _partition;
            std::vector<int> _fence;
            bool _partitions_finished;
            bool _fences_finished;

            bool next_partition(std::vector<int>& f)
            {
                if (_partitions_finished) {
                    return false;
                }
                if (_nlevels == 1) {
                    f[0] = _nnodes;
                    this->_partitions_finished = true;
                    return true;
                }
                for (int i = 0; i < _nlevels; i++) {
                    f[i] = this->_partition[i];
                }
                if (_partition[0] - 1 > _partition[1]) {
                    _partition[0] -= 1;
                    _partition[1] += 1;
                    return true;
                }
                int j = 2;
                int s = _partition[0] + _partition[1] - 1;
                while (j < _nlevels && _partition[j] >= _partition[0] - 1) {
                    s += _partition[j];
                    j += 1;
                }
                if (j >= _nlevels) {
                    _partitions_finished = true;
                    return true;
                }
                int x = _partition[j] = _partition[j] + 1;
                j -= 1;
                while (j > 0) {
                    _partition[j] = x;
                    s -= x;
                    j -= 1;
                    _partition[0] = s;
                }
                return true;
            }

        public:
            partition_generator()
            {
                _nnodes = 0;
                _nlevels = 0;
                _partitions_finished = false;
                _fences_finished = false;
            }

            partition_generator(partition_generator&& g)
            {
                _nnodes = g._nnodes;
                _nlevels = g._nlevels;
                _partitions_finished = g._partitions_finished;
                _fences_finished = g._fences_finished;
                _partition = std::move(g._partition);
                _fence = std::move(g._fence);
            }

            partition_generator(const int nnodes, const int nlevels)
            {
                assert(nlevels <= nnodes && nlevels > 0);
                reset(nnodes, nlevels);
            }

            int get_nnodes() { return _nnodes; }
            int get_nlevels() { return _nlevels; }

            bool next_fence(fence& f)
            {
                if (_fences_finished) {
                    return false;
                }
                f.reset(_nnodes, _nlevels);
                // Reverse the order of the nodes in the fence. This ensures
                // that the sequence of fences will be in "bottom up" order.
                // In other words, we first generate fences with more nodes at
                // the bottom than at the top.
                for (int i = 1; i <= _nlevels; i++) {
                    f[_nlevels - i] = _fence[i-1];
                }
                if (!std::next_permutation(_fence.begin(), _fence.end())) {
                    if (next_partition(_fence)) {
                        std::sort(_fence.begin(), _fence.end());
                    } else {
                        _fences_finished = true;
                    }
                }
                return true;
            }

            void reset(const int nnodes, const int nlevels)
            {
                assert(nnodes >= nlevels);

                _nnodes = nnodes;
                _nlevels = nlevels;

                _partition.resize(nlevels);
                _partition[0] = nnodes - nlevels + 1;
                for (int i = 1; i < nlevels; i++) {
                    _partition[i] = 1;
                }
                _fences_finished = _partitions_finished = (nnodes < nlevels);

                _fence.resize(nlevels);
                if (next_partition(_fence)) {
                    std::sort(_fence.begin(), _fence.end());
                } else {
                    _fences_finished = true;
                }
            }

            

            partition_generator& operator=(const partition_generator&) = delete;
            partition_generator& operator=(const partition_generator&& g)
            {
                _nnodes = g._nnodes;
                _nlevels = g._nlevels;
                _partition = g._partition;
                _fence = g._fence;
                _partitions_finished = g._partitions_finished;
                _fences_finished = g._fences_finished;

                return *this;
            }
    };


    /***************************************************************************
        Generates all Boolean fences for the fence family of a specified
        size. Uses the partition generator to generate fences of varying
        levels.
    ***************************************************************************/
    class family_generator : public fence_generator
    {
        private:
            partition_generator _gen;

        public:
            family_generator() { }

            family_generator(family_generator&& gen)
            {
                _gen = std::move(gen._gen);
            }

            family_generator(int nnodes)
            {
                _gen.reset(nnodes, nnodes);
            }

            family_generator& operator=(const family_generator&) = delete;
            family_generator& operator=(const family_generator&& g)
            {
                _gen = std::move(g._gen);

                return *this;
            }

            void reset(int nnodes)
            {
                _gen.reset(nnodes, nnodes);
            }
            
            int get_nnodes() { return _gen.get_nnodes(); }
            int get_nlevels() { return _gen.get_nlevels(); }

            bool next_fence(fence& f)
            {
                if (_gen.next_fence(f)) {
                    return true;
                }
                if (_gen.get_nlevels() == 1) {
                    return false;
                }
                _gen.reset(_gen.get_nnodes(), _gen.get_nlevels()-1);
                return _gen.next_fence(f);
            }
    };


    /***************************************************************************
        Continually generates fences of increasing size.
    ***************************************************************************/
    class unbounded_generator : public fence_generator
    {
        private:
            family_generator _gen;

        public:
            unbounded_generator(const int initial_nr_nodes = 1) 
            {
                _gen.reset(initial_nr_nodes);
            }

            unbounded_generator(unbounded_generator&& gen)
            {
                _gen = std::move(gen._gen);
            }

            int get_nnodes() { return _gen.get_nnodes(); }
            int get_nlevels() { return _gen.get_nlevels(); }

            bool next_fence(fence& f)
            {
                if (_gen.next_fence(f)) {
                    return true;
                }
                _gen.reset(_gen.get_nnodes() + 1);
                return _gen.next_fence(f);
            }
    };
        

    template<class T> 
    class fence_filter : public fence_generator
    {
        protected:
            T _gen;

        public:
            fence_filter(T gen) : _gen(std::move(gen)) { }
    };

    
    /***************************************************************************
        Filters out fences based on the number of POs and maximum operator
        fanin. This is useful in a concrete synthesis context, as not all
        Boolean fences are relevant when trying to synthesize optimum Boolean
        chains.
    ***************************************************************************/
    template <class T> 
        class po_filter : protected fence_filter<T>
    {
        private:
            int _nout;
            int _max_fanin;

        public:
            po_filter(po_filter&& f)
            {
                this->_gen = std::move(f._gen);
                _nout = f._nout;
                _max_fanin = f._max_fanin;
            }

            po_filter(T gen, int nout, int max_fanin) : fence_filter<T>(std::move(gen)) 
            {
                _nout = nout;
                _max_fanin = max_fanin;
            }

            int get_nlevels() { return this->_gen.get_nlevels(); }
            int get_nnodes() { return this->_gen.get_nnodes(); }

            bool next_fence(fence& f)
            {
                while (true) {
                    if (this->_gen.next_fence(f)) {
                        bool is_valid = true;
                        const int nlevels = f.nr_levels();
						auto max_in_use = std::min(_nout, f[nlevels-1]);
                        auto nnodes = f[nlevels-1];
                        if (max_in_use < nnodes) {
                            continue;
                        }
                        max_in_use *= _max_fanin;
                        for (int i = 1; i < nlevels; i++) {
                            auto nnodes = f[nlevels-1-i];
                            if (max_in_use < nnodes) 
                            {
                                is_valid = false;
                                break;
                            }
                            max_in_use *= _max_fanin;
                        }
                        if (is_valid) {
                            return true;
                        } else {
                            continue;
                        }
                    } else {
                        return false;
                    }
                }
            }
    };

    void print_fence(const fence& f);


    static inline int nr_fences(int nr_steps) {
        assert(nr_steps > 0);
        return 1 << (nr_steps-1);
    }

    /***************************************************************************
        A fence generator based on a recursive backtracking algorithm.
    ***************************************************************************/
    class rec_fence_generator
    {
        private:
            int _nr_levels;
            int _nr_nodes;
            int _nr_outputs;
            // The budget is the number of nodes we can distribute over any
            // of the levels.
            int _budget;
            bool _initialized = false;

            // If true, this filters out all fences that are guaranteed to be
            // suboptimal with the given number of outputs.
            bool _po_filter = true;

            int _max_fanin;

            int _level;

            uint64_t _nr_solutions;

            // Keeps track of how many nodes we spent at each level.
            int *_nodes_spent = nullptr;

            // Function to call when a solution is found.
            std::function<void(rec_fence_generator*)> _callback;

        public:
            rec_fence_generator() { }

            rec_fence_generator(bool po_filter) : _po_filter(po_filter) { }

            ~rec_fence_generator()
            {
                if (_nodes_spent != nullptr) {
                    delete[] _nodes_spent;
                }
            }

            uint64_t nr_solutions() const { return _nr_solutions; }

            int nr_levels() const { return _nr_levels; }
            int nr_nodes() const { return _nr_nodes; }
            int nodes_on_level(int i) const { return _nodes_spent[i]+1; }
            bool po_filter() const { return _po_filter; }
            void set_po_filter(bool po_filter) { _po_filter = po_filter; }

            void 
            reset(
                    int nr_nodes, 
                    int nr_levels, 
                    int nr_outputs=1, 
                    int max_fanin=2)
            {
                assert(nr_levels >= 1);
                assert(nr_nodes >= nr_levels);
            
                if (_nodes_spent != nullptr) {
                    delete[] _nodes_spent;
                }
                _nodes_spent = new int[nr_levels];
                for (int i = 0; i < nr_levels; i++) {
                    _nodes_spent[i] = 0;
                }

                _nr_nodes = nr_nodes;
                _nr_levels = nr_levels;
                _nr_outputs = nr_outputs;
                _max_fanin = max_fanin;

                _budget = _nr_nodes - _nr_levels;
                _nr_solutions = 0;
                _level = 0;

                _initialized = true;
            }

            int max_nodes_on_level(int level) 
            {
                assert(level >= 0);
                if (level == 0) {
                    return _nr_outputs;
                }
                const auto nr_out_nodes = _nodes_spent[0] + 1;
                int nr_allowed = nr_out_nodes;
                for (int i = 0; i < level; i++) {
                    nr_allowed *= _max_fanin;
                }
                return nr_allowed;
            }

            void search_fences()
            {
                if (_budget == 0) {
                    ++_nr_solutions;
                    if (_callback) {
                        _callback(this);
                    }
                    backtrack();
                } else {
                    // At the current level, we can spend at most "budget"
                    // number of nodes. However, at the final level we have
                    // to spend the remaining budget, or we may end up in
                    // an infinite search branch.
                    auto start_budget = 0;
                    if (_level == _nr_levels-1) {
                        start_budget = _budget;
                    }
                    int max_budget;
                    if (_po_filter) {
                        // At the highest level, we need at most as many nodes
                        // as we have outputs. For lower levels we have similar
                        // bounds.
                        max_budget = std::min((max_nodes_on_level(_level)-1), _budget);
                    } else {
                        max_budget = _budget;
                    }

                    for (int i = start_budget; i <= max_budget; i++) {
                        _budget -= i;
                        _nodes_spent[_level] = i;
                        ++_level;
                        search_fences();
                    }
                    backtrack();
                }
            }

            void backtrack()
            {
                if (--_level > -1) {
                    const auto nodes_spent = _nodes_spent[_level];
                    _budget += nodes_spent;
                    _nodes_spent[_level] = 0;
                }
            }

            uint64_t count_fences()
            {
                assert(_initialized);
                search_fences();
                _initialized = false;
                return _nr_solutions;
            }

            void generate_fences(std::vector<fence>& fences)
            {
                assert(_initialized);
                _callback = [&fences](rec_fence_generator* gen) {
                    const auto nr_levels = gen->nr_levels();
                    fence f(gen->nr_nodes(), nr_levels);
                    for (int i = 0; i < nr_levels; i++) {
                        f[i] = gen->nodes_on_level(nr_levels-1-i);
                    }
                    fences.push_back(f);
                };
                search_fences();
                _callback = 0;
                _initialized = false;
            }

            void 
            generate_fences(moodycamel::ConcurrentQueue<fence>& q)
            {
                assert(_initialized);
                _callback = [&q](rec_fence_generator* gen) {
                    const auto nr_levels = gen->nr_levels();
                    fence f(gen->nr_nodes(), nr_levels);
                    for (int i = 0; i < nr_levels; i++) {
                        f[i] = gen->nodes_on_level(nr_levels-1-i);
                    }
                    while (!q.try_enqueue(f)) {
                        std::this_thread::yield();
                    }
                };
                search_fences();
                _callback = 0;
                _initialized = false;
            }

    };

    /***************************************************************************
        Generates all fences of k nodes.
    ***************************************************************************/
    inline std::vector<fence>
    generate_fences(int k, bool po_filter=true, int nr_outputs=1)
    {
        std::vector<fence> fences;
        rec_fence_generator gen(po_filter);

        for (int l = 1; l <= k; l++) {
            gen.reset(k, l, nr_outputs);
            gen.generate_fences(fences);
        }

        return fences;
    }

    /***************************************************************************
        Overloaded version that appends generated fences to existing vector.
    ***************************************************************************/
    inline void
    generate_fences(
            std::vector<fence>& fences, 
            int k, 
            bool po_filter=true, 
            int nr_outputs=1)
    {
        rec_fence_generator gen(po_filter);

        for (int l = 1; l <= k; l++) {
            gen.reset(k, l, nr_outputs);
            gen.generate_fences(fences);
        }
    }
    
    /***************************************************************************
        Generates all fences of k nodes and puts them into a concurrent queue
        to be consumed by other threads.
    ***************************************************************************/
    inline void
    generate_fences(spec& spec, moodycamel::ConcurrentQueue<fence>& q)
    {
        rec_fence_generator gen;

        for (int l = 1; l <= spec.nr_steps; l++) {
            gen.reset(spec.nr_steps, l, spec.get_nr_out(), spec.fanin);
            gen.generate_fences(q);
        }
    }
    
    inline void print_fence(const fence& f)
    {
        for (int i = f.nr_levels()-1; i >= 0; i--) {
            printf("  ");
            for (int j = 0; j < f.at(i); j++) {
                printf("\u25CB ");
            }
            printf("\n");
        }
    }


}

