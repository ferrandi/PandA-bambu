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

    /// Isomorphism check using set containinment (hashing)
    inline void pd_filter_isomorphic_sfast(
        const std::vector<partial_dag>& dags, 
        std::vector<partial_dag>& ni_dags,
        bool show_progress = false)
    {
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

  /*! \brief Writes a partial DAG in DOT format to output stream
   *
   *  Converts a partial DAG to the GraphViz DOT format and writes it
   *  to the specified output stream
   */
  inline void to_dot( partial_dag const& pdag, std::ostream& os )
  {
    os << "graph{\n";
    os << "rankdir = BT;\n";

    pdag.foreach_vertex( [&pdag, &os]( auto const& v, int index ){
        std::string label = "(";
        for ( auto i = 0; i < v.size(); ++i )
        {
          label += std::to_string( v.at( i ) );
          if ( i+1 < v.size() )
            label += ',';
        }
        label += ")";

        auto const dot_index = index + 1;
        os << "n" << dot_index << " [label=<<sub>" << dot_index << "</sub> " << label << ">];\n";

        for ( const auto& child : v )
        {
          if ( child != 0u )
            os << "n" << child << " -- n" << dot_index << ";\n";
        }
      });
    os << "}\n";
  }

  /*! \brief Writes partial DAG in DOT format into file
   *
   *  Converts a partial DAG to the GraphViz DOT format and writes it
   *  to the specified file
   */
  inline void to_dot( partial_dag const& pdag, std::string const& filename )
  {
    std::ofstream ofs( filename, std::ofstream::out );
    to_dot( pdag, ofs );
    ofs.close();
  }
}
