/*
 *
 *                   _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *                  _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *                 _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *                _/      _/    _/ _/    _/ _/   _/ _/    _/
 *               _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *             ***********************************************
 *                              PandA Project
 *                     URL: http://panda.dei.polimi.it
 *                       Politecnico di Milano - DEIB
 *                        System Architectures Group
 *             ***********************************************
 *              Copyright (C) 2004-2024 Politecnico di Milano
 *
 *   This file is part of the PandA framework.
 *
 *   The PandA framework is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
/**
 * @file clique_covering.hpp
 * @brief This header file includes four different algorithms heuristically solving the clique covering problem.
 *
 * This header has 4 classes solving clique covering problem. Three of them consider the weighted of the edges during
 * the clique selection.
 *
 * The first two heuristics are based on these two papers:
 *  - E. Tomita, A. Tanaka, H. Takahashi
 *    "The worst-case time complexity for generating all maximal cliques and computational experiments",
 *     Theoretical Computer Science, Volume 363, Issue 1, 2006.
 *  - K. C. Dukka Bahadur, Tatsuya Akutsu, Etsuji Tomita, and Tomokazu Seki.
 *    "Protein side-chain packing problem: a maximum edge-weight clique algorithmic approach",
 *    In Proceedings of the second conference on Asia-Pacific bioinformatics - Volume 29 (APBC '04),
 *    Yi-Ping Phoebe Chen (Ed.), Vol. 29. Australian Computer Society, Inc., Darlinghurst, Australia, Australia,
 * 191-200. The third one is a simple clique covering obtained by coloring the complement graph. The last one performs
 * iteratively the clique covering on a filtered graph induced by different edge weights.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef CLIQUE_COVERING_HPP
#define CLIQUE_COVERING_HPP

#include "clique_covering_graph.hpp"
#include "custom_map.hpp" // for map
#include "custom_set.hpp" // for set
#include "dsatur2_coloring.hpp"
#include "exceptions.hpp"
#include "ortools/graph/assignment.h"
#include "refcount.hpp"
#include "string_manipulation.hpp"

#include <algorithm> // for binary_search, sort
#include <boost/graph/adjacency_matrix.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/incremental_components.hpp>
#include <boost/graph/properties.hpp>
#include <boost/pending/disjoint_sets.hpp>
#include <boost/tuple/tuple.hpp> // for tie
#include <boost/version.hpp>
#include <cstddef>  // for size_t
#include <iterator> // for inserter, reverse_...
#include <limits>   // for numeric_limits
#include <ostream>  // for operator<<, ostream
#include <string>   // for string, operator+
#include <utility>  // for pair, swap
#include <vector>   // for vector, allocator
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#else
#pragma GCC diagnostic warning "-Wsign-compare"
#pragma GCC diagnostic warning "-Wsign-conversion"
#endif

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-compare"
#pragma clang diagnostic ignored "-Wsign-conversion"
#endif

template <typename vertex_type>
struct check_clique;
template <typename vertex_type>
struct filter_clique;

/// unordered_set_difference
template <typename IIter1, typename Container, typename OIter>
OIter unordered_set_difference(IIter1 first1, IIter1 last1, Container second, OIter result)
{
   while(first1 != last1)
   {
      if(second.find(*first1) == second.end())
      {
         *result = *first1;
         ++result;
      }
      ++first1;
   }
   return result;
}

/// unordered_set_intersection
template <typename IIter1, typename Container, typename OIter>
OIter unordered_set_intersection(IIter1 first1, IIter1 last1, Container second, OIter result)
{
   while(first1 != last1)
   {
      if(second.find(*first1) != second.end())
      {
         *result = *first1;
         ++result;
      }
      ++first1;
   }
   return result;
}

/// Defines all clique covering algorithm
enum class CliqueCovering_Algorithm
{
   COLORING = 0,
   WEIGHTED_COLORING,
   TTT_CLIQUE_COVERING,
   TTT_CLIQUE_COVERING2,
   TTT_CLIQUE_COVERING_FAST,
   TTT_CLIQUE_COVERING_FAST2,
   TS_CLIQUE_COVERING,
   TS_WEIGHTED_CLIQUE_COVERING,
   BIPARTITE_MATCHING
};

/**
 * Return the name corresponding to an clique covering algorithm enum
 * @param clique_covering_algorithm is the clique covering algorithm
 * @return the name
 */
const std::string CliqueCovering_AlgorithmToString(const CliqueCovering_Algorithm clique_covering_algorithm);

template <typename VertexType>
class clique_covering
{
 public:
   /**
    * Default constructor.
    */
   clique_covering() = default;

   /**
    * Destructor
    */
   virtual ~clique_covering() = default;

   /**
    * Creates a reference to desired solver
    * @param solver is the solver which you want to perform clique covering with
    * @param graph_type is the kind of graph you are going to create
    * @return a reference to the desired solver
    */
   static refcount<clique_covering<VertexType>> create_solver(CliqueCovering_Algorithm solver, unsigned int nvert);

   /**
    * Adds a vertex to graph. It checks if element is already into graph. If it is, an assertion fails, otherwise
    * the vertex is added and the new index is saved for future checks
    * @param VertexType& is the reference to element that deals with compatibility
    * @return the new vertex index
    */
   virtual C_vertex add_vertex(const VertexType& element, const std::string& name) = 0;

   /**
    * Adds an edge to graph. It checks if source and vertex are stored into graph. If one of them isn't into graph,
    * the related assertion fails. If both vertices are stored, a (weighted) edge is added
    * @param src is the index of first vertex
    * @param dest is the index of second vertex
    * @param weight is the weight associated with edge (optional, set to unitary value as default)
    */
   virtual void add_edge(const VertexType& src, const VertexType& dest, int _weight) = 0;

   /**
    * Returns number of cliques into graph after performing clique covering
    */
   virtual size_t num_vertices() = 0;

   /**
    * Abstract method that will execute clique covering algorithm. If you want to specialize the implementation
    * with your favorite algorithm, you have to implement this method.
    * @param fc is the filtering clique functor used to reduce the proposed clique
    */
   virtual void exec(const filter_clique<VertexType>& fc, check_clique<VertexType>& cq) = 0;

   /**
    * Returns a clique
    * @param i is the i-th clique into graph
    * @return set of elements into clique
    */
   virtual CustomOrderedSet<VertexType> get_clique(unsigned int i) = 0;

   /**
    * Writes a dotty representation of the actual graph
    * @param filename is the output filename
    */
   virtual void writeDot(const std::string& filename) const = 0;

   /**
    * add subpartitions over which bipartite matching can start on
    * @param id is the subpartition id
    * @param vertex is the vertex of the given subpartition
    */
   virtual void add_subpartitions(size_t id, VertexType v) = 0;

   /**
    * suggest that the problem have at least a given number of resources
    * @param n_resources is the number of resources available
    */
   virtual void suggest_min_resources(size_t n_resources) = 0;

   /**
    * suggest that the problem have at worst no more than the given number of resources
    * @param n_resources is the number of resources
    */
   virtual void suggest_max_resources(size_t n_resources) = 0;

   /**
    * specify the maximum number of resources
    * @param n_resources is the number of resources
    */
   virtual void max_resources(size_t n_resources) = 0;

   /**
    * specify the minimum number of resources
    * @param n_resources is the number of resources
    */
   virtual void min_resources(size_t n_resources) = 0;
};

/**
 * Functor used by boost::write_graphviz to write the edge info
 */
class compatibility_edge_writer
{
 private:
   /// reference to graph where the node is stored
   const boost_cc_compatibility_graph& g;

 public:
   /**
    * Constructor. It initializes reference to the graph provided as parameter
    */
   explicit compatibility_edge_writer(const boost_cc_compatibility_graph& _g) : g(_g)
   {
   }

   /**
    * Functor actually called by the boost library to perform the writing
    */
   void operator()(std::ostream& out, const boost::graph_traits<cc_compatibility_graph>::edge_descriptor& e) const
   {
      out << "[label=\"" << g[e].weight << "\"]";
   }
};

/**
 * Functor used by boost::write_graphviz to write the node info
 */
class compatibility_node_info_writer
{
 private:
   /// names of the vertices
   const CustomUnorderedMap<C_vertex, std::string>& names;

 public:
   /// Constructor
   explicit compatibility_node_info_writer(const CustomUnorderedMap<C_vertex, std::string>& _names) : names(_names)
   {
   }

   /// Functor used to print a node
   void operator()(std::ostream& out, const C_vertex& v) const
   {
      out << "[label=\"" << names.find(v)->second << "\"]";
   }
};

/**
 * Class computing the maximal weighted clique from a generic graph
 */
template <typename Graph>
class TTT_maximal_weighted_clique
{
   /// vertex iterator
   using vertex_iterator = typename boost::graph_traits<Graph>::vertex_iterator;
   /// vertex object
   using vertex = typename boost::graph_traits<Graph>::vertex_descriptor;
   /// adjacency iterator
   using adjacency_iterator = typename boost::graph_traits<Graph>::adjacency_iterator;
   /// out edge iterator
   using edge_iterator = typename boost::graph_traits<Graph>::out_edge_iterator;

   /// set of vertices of the current clique
   CustomUnorderedSet<vertex> Q;
   /// set of vertices of the maximum clique found so far
   CustomOrderedSet<vertex> Q_max;
   /// weight of Q
   int W_Q;
   /// weight of Q_max
   int W_Q_max;

   CustomUnorderedMap<C_vertex, std::string>& names;

   /// return the vertex of subg with the maximum intersection with cand
   vertex get_max_weighted_adiacent_intersection(const CustomUnorderedSet<vertex>& subg,
                                                 const CustomUnorderedSet<vertex>& cand, const Graph& g)
   {
      vertex result = boost::graph_traits<Graph>::null_vertex();
      THROW_ASSERT(!subg.empty(), "at least one element should belong to subg");
      int max_weighted_intersection = -1;
      const typename CustomUnorderedSet<vertex>::const_iterator it_end = subg.end();
      for(auto it = subg.begin(); it != it_end; ++it)
      {
         int weight_intersection = 0;
         edge_iterator ei, ei_end;
         boost::tie(ei, ei_end) = boost::out_edges(*it, g);
         for(; ei != ei_end; ++ei)
         {
            if(cand.find(boost::target(*ei, g)) != cand.end())
            {
               weight_intersection += g[*ei].weight;
            }
         }
         if(weight_intersection > max_weighted_intersection)
         {
            max_weighted_intersection = weight_intersection;
            result = *it;
         }
      }
      THROW_ASSERT(max_weighted_intersection >= 0, "something wrong happened");
      return result;
   }

   /// return the vertex of ext having the maximum edge weight with respect to the graph g
   vertex get_max_weight_vertex(CustomUnorderedSet<vertex>& ext, const Graph& g)
   {
      vertex result = boost::graph_traits<Graph>::null_vertex();
      int max_weight = -1;
      THROW_ASSERT(!ext.empty(), "at least one element should belong to ext");
      const typename CustomUnorderedSet<vertex>::const_iterator it_end = ext.end();
      for(typename CustomUnorderedSet<vertex>::const_iterator it = ext.begin(); it != it_end; ++it)
      {
         int cur_weight = 0;
         edge_iterator ei, ei_end;
         boost::tie(ei, ei_end) = boost::out_edges(*it, g);
         for(; ei != ei_end; ++ei)
         {
            cur_weight += g[*ei].weight;
         }
         if(cur_weight > max_weight)
         {
            result = *it;
            max_weight = cur_weight;
         }
      }
      THROW_ASSERT(max_weight >= 0, "something wrong happened");
      return result;
   }

   /// compute the delta of the weight by adding q_vertex to the clique
   int compute_delta_weight(vertex q_vertex, CustomUnorderedSet<vertex> Q_set, const Graph& g)
   {
      int result = 0;
      edge_iterator ei, ei_end;
      boost::tie(ei, ei_end) = boost::out_edges(q_vertex, g);
      for(; ei != ei_end; ++ei)
      {
         if(Q_set.find(boost::target(*ei, g)) != Q_set.end())
         {
            result += g[*ei].weight;
         }
      }
      return result;
   }

   /// recursive procedure expand defined in first cited paper
   void expand(CustomUnorderedSet<vertex>& subg, CustomUnorderedSet<vertex>& cand, const Graph& g, int upper_bound)
   {
      if(subg.empty() && Q.size() >= Q_max.size())
      {
         if(Q.size() > Q_max.size() || W_Q > W_Q_max)
         {
            Q_max.clear();
            Q_max.insert(Q.begin(), Q.end());
            W_Q_max = W_Q;
            // std::cerr << "clique W=" << W_Q_max << " size=" << Q_max.size() << std::endl;
         }
         return;
      }
      else if(cand.empty())
      {
         return;
      }

      /// get the vertex in subg with the maximum of adjacent vertices in cand
      vertex u = get_max_weighted_adiacent_intersection(subg, cand, g);
      // vertex u =get_max_adiacent_intersection(subg, cand, g);
      /// get adjacent vertices of u
      adjacency_iterator vi, vi_end;
      boost::tie(vi, vi_end) = boost::adjacent_vertices(u, g);
      /// set of vertices adjacent to u
      CustomUnorderedSet<vertex> gamma_u;
      gamma_u.insert(vi, vi_end);
      /// compute EXT_u = CAND - gamma_u
      CustomUnorderedSet<vertex> EXT_u;
      unordered_set_difference(cand.begin(), cand.end(), gamma_u, std::inserter(EXT_u, EXT_u.end()));
      while(!EXT_u.empty())
      {
         vertex q = get_max_weight_vertex(EXT_u, g);
         // vertex q = get_max_degree_vertex(EXT_u, g);
         // std::cerr << names[q] << "," << std::endl;
         Q.insert(q);
         int W_Q_pre = W_Q;
         /// compute delta_weight
         int delta = compute_delta_weight(q, Q, g);
         W_Q += delta;
         boost::tie(vi, vi_end) = boost::adjacent_vertices(q, g);
         CustomUnorderedSet<vertex> gamma_q;
         gamma_q.insert(vi, vi_end);
         CustomUnorderedSet<vertex> subg_q;
         unordered_set_intersection(subg.begin(), subg.end(), gamma_q, std::inserter(subg_q, subg_q.end()));
         CustomUnorderedSet<vertex> cand_q;
         unordered_set_intersection(cand.begin(), cand.end(), gamma_q, std::inserter(cand_q, cand_q.end()));
         expand(subg_q, cand_q, g, upper_bound);
         if(upper_bound <= W_Q_max)
         {
            return;
         }
         cand.erase(q);
         // std::cerr << "back," << std::endl;
         Q.erase(q);
         W_Q = W_Q_pre;
         EXT_u.erase(q);
      }
   }

 public:
   /// return the weighted maximal clique of a graph g
   const CustomOrderedSet<vertex> get_weighted_maximal_cliques(const Graph& g, int upper_bound)
   {
      Q.clear();
      Q_max.clear();
      W_Q = 0;
      W_Q_max = std::numeric_limits<int>::min();
      CustomUnorderedSet<vertex> subg;
      CustomUnorderedSet<vertex> cand;
      vertex_iterator vi, vi_end;
      boost::tie(vi, vi_end) = boost::vertices(g);
      subg.insert(vi, vi_end);
      cand.insert(vi, vi_end);
      expand(subg, cand, g, upper_bound);
      return Q_max;
   }

   /// return the last weight of the maximum clique
   int get_last_W_Q_max()
   {
      return W_Q_max;
   }
   explicit TTT_maximal_weighted_clique(CustomUnorderedMap<C_vertex, std::string>& _names)
       : W_Q(0), W_Q_max(std::numeric_limits<int>::min()), names(_names)
   {
   }
};

/**
 * fast version that just returns the first maximal clique found
 */
template <typename Graph>
class TTT_maximal_weighted_clique_fast
{
   /// vertex iterator
   using vertex_iterator = typename boost::graph_traits<Graph>::vertex_iterator;
   /// vertex object
   using vertex = typename boost::graph_traits<Graph>::vertex_descriptor;
   /// adjacency iterator
   using adjacency_iterator = typename boost::graph_traits<Graph>::adjacency_iterator;
   /// out edge iterator
   using edge_iterator = typename boost::graph_traits<Graph>::out_edge_iterator;

   /// set of vertices of the current clique
   CustomUnorderedSet<vertex> Q;
   /// set of vertices of the maximum clique found so far
   CustomOrderedSet<vertex> Q_max;
   /// weight of Q
   int W_Q;
   /// weight of Q_max
   int W_Q_max;

   CustomUnorderedMap<C_vertex, std::string>& names;

   /// return the vertex of subg with the maximum intersection with cand
   vertex get_max_weighted_adiacent_intersection(const CustomUnorderedSet<vertex>& subg,
                                                 const CustomUnorderedSet<vertex>& cand, const Graph& g)
   {
      vertex result = boost::graph_traits<Graph>::null_vertex();
      THROW_ASSERT(!subg.empty(), "at least one element should belong to subg");
      int max_weighted_intersection = -1;
      const typename CustomUnorderedSet<vertex>::const_iterator it_end = subg.end();
      for(auto it = subg.begin(); it != it_end; ++it)
      {
         int weight_intersection = 0;
         edge_iterator ei, ei_end;
         boost::tie(ei, ei_end) = boost::out_edges(*it, g);
         for(; ei != ei_end; ++ei)
         {
            if(cand.find(boost::target(*ei, g)) != cand.end())
            {
               weight_intersection += g[*ei].weight;
            }
         }
         if(weight_intersection > max_weighted_intersection)
         {
            max_weighted_intersection = weight_intersection;
            result = *it;
         }
      }
      THROW_ASSERT(max_weighted_intersection >= 0, "something wrong happened");
      return result;
   }

   /// return the vertex of ext having the maximum degree with respect to the graph g
   vertex get_max_weight_vertex(CustomUnorderedSet<vertex>& ext, const Graph& g)
   {
      vertex result = boost::graph_traits<Graph>::null_vertex();
      int max_weight = -1;
      THROW_ASSERT(!ext.empty(), "at least one element should belong to ext");
      const typename CustomUnorderedSet<vertex>::const_iterator it_end = ext.end();
      for(typename CustomUnorderedSet<vertex>::const_iterator it = ext.begin(); it != it_end; ++it)
      {
         int cur_weight = 0;
         edge_iterator ei, ei_end;
         boost::tie(ei, ei_end) = boost::out_edges(*it, g);
         for(; ei != ei_end; ++ei)
         {
            cur_weight += g[*ei].weight;
         }
         if(cur_weight > max_weight)
         {
            result = *it;
            max_weight = cur_weight;
         }
      }
      THROW_ASSERT(max_weight >= 0, "something wrong happened");
      return result;
   }

   /// compute the delta of the weight by adding q_vertex to the clique
   int compute_delta_weight(vertex q_vertex, CustomUnorderedSet<vertex> Q_set, const Graph& g)
   {
      int result = 0;
      edge_iterator ei, ei_end;
      boost::tie(ei, ei_end) = boost::out_edges(q_vertex, g);
      for(; ei != ei_end; ++ei)
      {
         if(Q_set.find(boost::target(*ei, g)) != Q_set.end())
         {
            result += g[*ei].weight;
         }
      }
      return result;
   }

   /// recursive procedure expand defined in first cited paper
   void expand(CustomUnorderedSet<vertex>& subg, CustomUnorderedSet<vertex>& cand, const Graph& g)
   {
      if(subg.empty() && Q.size() >= Q_max.size())
      {
         if(Q.size() > Q_max.size() || W_Q > W_Q_max)
         {
            Q_max.clear();
            Q_max.insert(Q.begin(), Q.end());
            W_Q_max = W_Q;
            // std::cerr << "clique W=" << W_Q_max << " size=" << Q_max.size() << std::endl;
         }
         return;
      }
      else if(cand.empty())
      {
         return;
      }

      /// get the vertex in subg with the maximum of adjacent vertices in cand
      vertex u = get_max_weighted_adiacent_intersection(subg, cand, g);
      // vertex u =get_max_adiacent_intersection(subg, cand, g);
      /// get adjacent vertices of u
      adjacency_iterator vi, vi_end;
      boost::tie(vi, vi_end) = boost::adjacent_vertices(u, g);
      /// set of vertices adjacent to u
      CustomUnorderedSet<vertex> gamma_u;
      gamma_u.insert(vi, vi_end);
      /// compute EXT_u = CAND - gamma_u
      CustomUnorderedSet<vertex> EXT_u;
      unordered_set_difference(cand.begin(), cand.end(), gamma_u, std::inserter(EXT_u, EXT_u.end()));
      while(!EXT_u.empty())
      {
         vertex q = get_max_weight_vertex(EXT_u, g);
         // vertex q = get_max_degree_vertex(EXT_u, g);
         // std::cerr << names[q] << "," << std::endl;
         Q.insert(q);
         int W_Q_pre = W_Q;
         /// compute delta_weight
         int delta = compute_delta_weight(q, Q, g);
         W_Q += delta;
         // std::cerr << "W_Q=" << W_Q << std::endl;
         boost::tie(vi, vi_end) = boost::adjacent_vertices(q, g);
         CustomUnorderedSet<vertex> gamma_q;
         gamma_q.insert(vi, vi_end);
         CustomUnorderedSet<vertex> subg_q;
         unordered_set_intersection(subg.begin(), subg.end(), gamma_q, std::inserter(subg_q, subg_q.end()));
         CustomUnorderedSet<vertex> cand_q;
         unordered_set_intersection(cand.begin(), cand.end(), gamma_q, std::inserter(cand_q, cand_q.end()));
         expand(subg_q, cand_q, g);
         // std::cerr << "W_Q_max=" << W_Q_max << std::endl;
         if(W_Q_max >= 0)
         {
            return;
         }
         cand.erase(q);
         // std::cerr << "back," << std::endl;
         Q.erase(q);
         W_Q = W_Q_pre;
         EXT_u.erase(q);
      }
   }

 public:
   /// return the weighted maximal clique of a graph g
   const CustomOrderedSet<vertex> get_weighted_maximal_cliques(const Graph& g)
   {
      Q.clear();
      Q_max.clear();
      W_Q = 0;
      W_Q_max = std::numeric_limits<int>::min();
      CustomUnorderedSet<vertex> subg;
      CustomUnorderedSet<vertex> cand;
      vertex_iterator vi, vi_end;
      boost::tie(vi, vi_end) = boost::vertices(g);
      subg.insert(vi, vi_end);
      cand.insert(vi, vi_end);
      expand(subg, cand, g);
      return Q_max;
   }

   explicit TTT_maximal_weighted_clique_fast(
       CustomUnorderedMap<typename boost::graph_traits<Graph>::vertex_descriptor, std::string>& _names)
       : W_Q(0), W_Q_max(std::numeric_limits<int>::min()), names(_names)
   {
   }
};

template <typename vertex_type>
class coloring_based_clique_covering : public clique_covering<vertex_type>
{
   /// bulk undirected graph
   boost_cc_compatibility_graph clique_covering_graph_bulk;
   /// set of maximal clique computed
   std::vector<CustomOrderedSet<C_vertex>> cliques;
   /// map between vertex_type and C_vertex
   CustomUnorderedMap<vertex_type, C_vertex> v2uv;
   /// edge selector to select all edges of the compatibility graph
   static const int COMPATIBILITY_ALL_EDGES = ~0;
   int max_level;
   bool all_edges;
   unsigned vindex;

 protected:
   /// map between C_vertex and vertex_type
   CustomUnorderedMap<C_vertex, vertex_type> uv2v;
   /// name map for the C_vertex vertices
   CustomUnorderedMap<C_vertex, std::string> names;

 public:
   /// constructor
   explicit coloring_based_clique_covering(bool _all_edges, unsigned int nvert)
       : clique_covering_graph_bulk(nvert), max_level(0), all_edges(_all_edges), vindex(0)
   {
   }

   /// destructor
   ~coloring_based_clique_covering() override = default;

   /// add a vertex
   C_vertex add_vertex(const vertex_type& element, const std::string& name) override
   {
      C_vertex result;
      THROW_ASSERT(v2uv.find(element) == v2uv.end(), "vertex already added");
      /// vertex weight not considered
      v2uv[element] = result = boost::vertex(vindex, clique_covering_graph_bulk);
      ++vindex;
      uv2v[result] = element;
      names[result] = name;
      return result;
   }

   /// add an edge
   void add_edge(const vertex_type& src, const vertex_type& dest, int _weight) override
   {
      THROW_ASSERT(src != dest, "autoloops are not allowed in a compatibility graph");
      THROW_ASSERT(v2uv.find(src) != v2uv.end(), "src not added");
      THROW_ASSERT(v2uv.find(dest) != v2uv.end(), "dest not added");
      THROW_ASSERT(_weight > 0 && _weight < 32, "weights from 1 to 31 are allowed " + STR(_weight));
      max_level = std::max(max_level, _weight);
      C_vertex SRC = v2uv.find(src)->second;
      C_vertex DEST = v2uv.find(dest)->second;
      boost::add_edge(SRC, DEST, edge_compatibility_selector(1 << _weight, _weight), clique_covering_graph_bulk);
   }

   /// return the number of vertices of the clique
   size_t num_vertices() override
   {
      return cliques.size();
   }

   CustomOrderedSet<vertex_type> get_clique(unsigned int i) override
   {
      CustomOrderedSet<vertex_type> result;
      CustomOrderedSet<C_vertex>& cur_clique = cliques[i];
      auto it_end = cur_clique.end();
      for(auto it = cur_clique.begin(); it != it_end; ++it)
      {
         THROW_ASSERT(uv2v.find(*it) != uv2v.end(), "vertex not added");
         result.insert(uv2v.find(*it)->second);
      }
      return result;
   }

   virtual void do_clique_covering(const cc_compatibility_graphRef filteredCG,
                                   boost::disjoint_sets<rank_pmap_type, pred_pmap_type>& ds,
                                   CustomUnorderedSet<C_vertex>&, const CustomUnorderedSet<C_vertex>&,
                                   const filter_clique<vertex_type>&)
   {
      using conflict_graph = boost::adjacency_matrix<boost::undirectedS>;
      using cg_vertices_size_type = boost::graph_traits<conflict_graph>::vertices_size_type;
      using cg_vertex_index_map = boost::property_map<conflict_graph, boost::vertex_index_t>::const_type;
      boost::iterator_property_map<cg_vertices_size_type*, cg_vertex_index_map, cg_vertices_size_type,
                                   cg_vertices_size_type&>
          color;

      std::vector<cg_vertices_size_type> color_vec;
      /// conflict graph

      using vertex_descriptor_cg = boost::graph_traits<conflict_graph>::vertex_descriptor;
      CustomUnorderedMap<C_vertex, vertex_descriptor_cg> vmap;
      std::vector<C_vertex> reverse_map;

      unsigned int num_vert = 0;
      cc_compatibility_graph::vertex_iterator vi, vi_end;
      for(boost::tie(vi, vi_end) = boost::vertices(*filteredCG); vi != vi_end; ++vi)
      {
         ++num_vert;
      }
      auto* cg = new conflict_graph(num_vert);

      unsigned int vertex_index = 0;
      BGL_FORALL_VERTICES(v, *filteredCG, cc_compatibility_graph)
      {
         vmap[v] = boost::vertex(vertex_index, *cg);
         reverse_map.push_back(v);
         ++vertex_index;
      }
      color_vec.resize(vertex_index);
      color = boost::iterator_property_map<cg_vertices_size_type*, cg_vertex_index_map, cg_vertices_size_type,
                                           cg_vertices_size_type&>(&color_vec.front(),
                                                                   boost::get(boost::vertex_index, *cg));

      BGL_FORALL_VERTICES(u, *filteredCG, cc_compatibility_graph)
      {
         std::vector<C_vertex> neighbors(boost::adjacent_vertices(u, *filteredCG).first,
                                         boost::adjacent_vertices(u, *filteredCG).second);
         std::sort(neighbors.begin(), neighbors.end());
         BGL_FORALL_VERTICES(v, *filteredCG, cc_compatibility_graph)
         {
            if(u > v)
            {
               continue; /// the graph is an undirected graph...
            }
            // Might want to check for self-loops
            if(!std::binary_search(neighbors.begin(), neighbors.end(), v))
            {
               boost::add_edge(vmap[u], vmap[v], *cg);
            }
         }
      }

      /// coloring based on DSATUR 2 heuristic
      cg_vertices_size_type num_colors = dsatur2_coloring(*cg, color);
      std::vector<unsigned int> colors(num_colors);

      for(unsigned int i = 0; i < num_colors; ++i)
      {
         colors[i] = std::numeric_limits<unsigned int>::max();
      }
      for(unsigned int i = 0; i < vertex_index; ++i)
      {
         cg_vertices_size_type c = color_vec[i];
         if(colors[c] == std::numeric_limits<unsigned int>::max())
         {
            colors[c] = i;
         }
         else
         {
            C_vertex ug_vertex_i = reverse_map[i];
            C_vertex ug_vertex_c = reverse_map[colors[c]];
            ds.union_set(ug_vertex_i, ug_vertex_c);
         }
      }
   }

   /// build partitions
   void build_partitions(CustomUnorderedSet<C_vertex>& support,
                         boost::disjoint_sets<rank_pmap_type, pred_pmap_type>& ds,
                         std::map<C_vertex, CustomOrderedSet<C_vertex>>& current_partitions)
   {
      auto it_end = support.end();
      for(auto it = support.begin(); it != it_end;)
      {
         auto current = it++;
         C_vertex rep = ds.find_set(*current);
         C_vertex cur = *current;
         if(rep != cur)
         {
            if(current_partitions.find(rep) == current_partitions.end())
            {
               CustomOrderedSet<C_vertex> singularity;
               singularity.insert(cur);
               current_partitions[rep] = singularity;
            }
            else
            {
               current_partitions.find(rep)->second.insert(cur);
            }
         }
         else
         {
            if(current_partitions.find(rep) == current_partitions.end())
            {
               CustomOrderedSet<C_vertex> null_set;
               current_partitions[rep] = null_set;
            }
         }
      }
   }

#define THRESHOLD_2_SIMPLIFY 200
   void exec(const filter_clique<vertex_type>& fc, check_clique<vertex_type>&) override
   {
      VertexIndex n = boost::num_vertices(clique_covering_graph_bulk);
      std::vector<VertexIndex> rank_map(n);
      std::vector<C_vertex> pred_map(n);
      vertex_index_pmap_t cindex_pmap = boost::get(boost::vertex_index_t(), clique_covering_graph_bulk);
      rank_pmap_type rank_pmap = boost::make_iterator_property_map(rank_map.begin(), cindex_pmap, rank_map[0]);
      pred_pmap_type pred_pmap = boost::make_iterator_property_map(pred_map.begin(), cindex_pmap, pred_map[0]);
      boost::disjoint_sets<rank_pmap_type, pred_pmap_type> ds(rank_pmap, pred_pmap);
      using u_vertex_iterator = boost::graph_traits<boost_cc_compatibility_graph>::vertex_iterator;
      CustomUnorderedSet<C_vertex> support;
      CustomUnorderedSet<C_vertex> all_vertices;

      boost::initialize_incremental_components(clique_covering_graph_bulk, ds);

      u_vertex_iterator ui, uiend;
      for(boost::tie(ui, uiend) = boost::vertices(clique_covering_graph_bulk); ui != uiend; ++ui)
      {
         support.insert(*ui);
         all_vertices.insert(*ui);
      }
      auto completeCG = cc_compatibility_graphRef(
          new cc_compatibility_graph(clique_covering_graph_bulk,
                                     cc_compatibility_graph_edge_selector<boost_cc_compatibility_graph>(
                                         COMPATIBILITY_ALL_EDGES, &clique_covering_graph_bulk),
                                     cc_compatibility_graph_vertex_selector<boost_cc_compatibility_graph>(&support)));

      std::map<C_vertex, CustomOrderedSet<C_vertex>> current_partitions;

      if(all_edges || support.size() > THRESHOLD_2_SIMPLIFY)
      {
         do_clique_covering(completeCG, ds, support, all_vertices, fc);
      }
      else
      {
         /// color the conflict graph projection
         int cur_level = max_level;
         int selector = 1 << cur_level;
         auto filteredCG = cc_compatibility_graphRef(new cc_compatibility_graph(
             clique_covering_graph_bulk,
             cc_compatibility_graph_edge_selector<boost_cc_compatibility_graph>(selector, &clique_covering_graph_bulk),
             cc_compatibility_graph_vertex_selector<boost_cc_compatibility_graph>(&support)));
         while(cur_level > 0)
         {
            do_clique_covering(filteredCG, ds, support, all_vertices, fc);
            /// build the current partitions
            build_partitions(support, ds, current_partitions);
            /// remove non conformant edges
            bool restart;
            do
            {
               restart = false;
               auto cp_it_end = current_partitions.end();
               for(auto cp_it = current_partitions.begin(); cp_it != cp_it_end; ++cp_it)
               {
                  C_vertex rep = cp_it->first;
                  CustomOrderedSet<C_vertex>& current_cliques = cp_it->second;
                  auto c_it_end = current_cliques.end();
                  for(auto c_it = current_cliques.begin(); c_it != c_it_end;)
                  {
                     auto current = c_it++;
                     C_vertex cur = *current;
                     C_outEdgeIterator ei, ei_end;
                     /// remove edges given the current set of clique
                     for(boost::tie(ei, ei_end) = boost::out_edges(rep, *completeCG); ei != ei_end; ++ei)
                     {
                        C_vertex rep_target = boost::target(*ei, *completeCG);
                        if(rep_target != cur)
                        {
                           std::vector<C_vertex> neighbors(boost::adjacent_vertices(cur, *completeCG).first,
                                                           boost::adjacent_vertices(cur, *completeCG).second);
                           std::sort(neighbors.begin(), neighbors.end());
                           if(!std::binary_search(neighbors.begin(), neighbors.end(), rep_target))
                           {
                              (*completeCG)[*ei].selector = 0;
                              // std::cerr << names[cur] << "|"<< names[rep] << " -0- " << names[rep_target] <<
                              // std::endl;
                              c_it = current_cliques.begin(); /// restart the pruning
                              restart = true;                 /// and then restart the whole partition analysis
                              // std::cerr << "restart\n";
                           }
                        }
                     }
                     for(boost::tie(ei, ei_end) = boost::out_edges(cur, *completeCG); ei != ei_end; ++ei)
                     {
                        C_vertex cur_target = boost::target(*ei, *completeCG);
                        if(cur_target != rep)
                        {
                           std::vector<C_vertex> neighbors(boost::adjacent_vertices(rep, *completeCG).first,
                                                           boost::adjacent_vertices(rep, *completeCG).second);
                           std::sort(neighbors.begin(), neighbors.end());
                           if(!std::binary_search(neighbors.begin(), neighbors.end(), cur_target))
                           {
                              (*completeCG)[*ei].selector = 0;
                              // std::cerr << names[rep] << "|"<< names[cur] << " -1- " << names[cur_target] <<
                              // std::endl;
                           }
                        }
                     }
                  }
               }
            } while(restart);
            current_partitions.clear();

            /// recompute the support
            /// by keeping only the representative of each partition
            auto it_end = support.end();
            for(auto it = support.begin(); it != it_end;)
            {
               auto current = it++;
               C_vertex rep = ds.find_set(*current);
               C_vertex cur = *current;
               if(rep != cur || boost::out_degree(rep, *completeCG) == 0)
               {
                  // std::cerr << "remove vertex " << names[cur] << std::endl;
                  support.erase(current);
               }
            }
            /// rebuild the graph starting from which the coloring has performed
            --cur_level;
            selector = selector | 1 << cur_level;
            // std::cerr << "new selector " << cur_level << std::endl;
            filteredCG = cc_compatibility_graphRef(new cc_compatibility_graph(
                clique_covering_graph_bulk,
                cc_compatibility_graph_edge_selector<boost_cc_compatibility_graph>(selector,
                                                                                   &clique_covering_graph_bulk),
                cc_compatibility_graph_vertex_selector<boost_cc_compatibility_graph>(&support)));
         }
         /// rebuild the partitions
         boost::tie(ui, uiend) = boost::vertices(clique_covering_graph_bulk);
         support.clear();
         support.insert(ui, uiend);
      }

      build_partitions(support, ds, current_partitions);
      /// and then fill the cliques
      auto cp_it_end = current_partitions.end();
      for(auto cp_it = current_partitions.begin(); cp_it != cp_it_end; ++cp_it)
      {
         cliques.push_back(cp_it->second);
         cliques.back().insert(cp_it->first);
      }
   }

   void writeDot(const std::string& filename) const override
   {
      std::ofstream f(filename.c_str());
      boost::write_graphviz(f, clique_covering_graph_bulk, compatibility_node_info_writer(names),
                            compatibility_edge_writer(clique_covering_graph_bulk));
   }

   void add_subpartitions(size_t, vertex_type) override
   {
   }

   void suggest_min_resources(size_t) override
   {
   }

   void suggest_max_resources(size_t) override
   {
   }

   void min_resources(size_t) override
   {
   }

   void max_resources(size_t) override
   {
   }
};

/// second fast version of the TTT_based_clique_covering. The first maximal clique found is used by the greedy clique
/// covering.
template <typename vertex_type>
class TTT_based_clique_covering_fast : public coloring_based_clique_covering<vertex_type>
{
 public:
   /// constructor
   explicit TTT_based_clique_covering_fast(bool _all_edges, unsigned int nvert)
       : coloring_based_clique_covering<vertex_type>(_all_edges, nvert)
   {
   }

   void do_clique_covering(const cc_compatibility_graphRef CG,
                           typename boost::disjoint_sets<rank_pmap_type, pred_pmap_type>& ds,
                           CustomUnorderedSet<C_vertex>& support, const CustomUnorderedSet<C_vertex>& all_vertices,
                           const filter_clique<vertex_type>& fc) override
   {
      TTT_maximal_weighted_clique_fast<cc_compatibility_graph> MWC(coloring_based_clique_covering<vertex_type>::names);
      // std::cerr << "Looking for a maximum weighted clique in a set of " << support.size() << std::endl;
      CustomUnorderedSet<C_vertex> support_copy(support);
      while(!support.empty())
      {
         CustomOrderedSet<C_vertex> curr_clique = MWC.get_weighted_maximal_cliques(*CG);
         // std::cerr << "Found one of size " << curr_clique.size() << std::endl;

         /// do some filtering
         bool removed_vertex = false;
         C_vertex vertex_to_be_removed;
         do
         {
            if(curr_clique.size() > 1)
            {
               CustomOrderedSet<C_vertex> curr_expandend_clique;
               const CustomUnorderedSet<C_vertex>::const_iterator av_it_end = all_vertices.end();
               for(auto av_it = all_vertices.begin(); av_it != av_it_end; ++av_it)
               {
                  C_vertex rep = ds.find_set(*av_it);
                  if(curr_clique.find(rep) != curr_clique.end())
                  {
                     curr_expandend_clique.insert(*av_it);
                  }
               }

               removed_vertex = fc.select_candidate_to_remove(curr_expandend_clique, vertex_to_be_removed,
                                                              coloring_based_clique_covering<vertex_type>::uv2v, *CG);
               if(removed_vertex)
               {
                  // std::cerr << "Vertex removed " << curr_clique.size() << std::endl;
                  curr_clique.erase(ds.find_set(vertex_to_be_removed));
               }
            }
            else
            {
               removed_vertex = false;
            }
         } while(removed_vertex);

         // std::cerr << "Found one of size " << curr_clique.size() << std::endl;
         C_vertex first = *curr_clique.begin();
         const CustomOrderedSet<C_vertex>::const_iterator cc_it_end = curr_clique.end();
         const CustomOrderedSet<C_vertex>::const_iterator first_cc_it = curr_clique.begin();
         CustomOrderedSet<C_vertex>::const_iterator cc_it = first_cc_it;
         do
         {
            C_vertex curr_vertex = *cc_it;
            auto current = support.find(curr_vertex);
            THROW_ASSERT(current != support.end(), "unexpected condition");
            if(cc_it != first_cc_it)
            {
               ds.union_set(first, curr_vertex);
            }
            support.erase(current);
            ++cc_it;
         } while(cc_it != cc_it_end);

         /// check trivial cases
         auto s_it_end = support.end();
         for(auto s_it = support.begin(); s_it != s_it_end;)
         {
            auto current = s_it++;
            if(boost::out_degree(*current, *CG) == 0)
            {
               support.erase(current);
            }
         }
      }
      support.insert(support_copy.begin(), support_copy.end());
   }
};

template <typename vertex_type>
class TTT_based_clique_covering : public coloring_based_clique_covering<vertex_type>
{
 public:
   /// constructor
   explicit TTT_based_clique_covering(bool _all_edges, unsigned int nvert)
       : coloring_based_clique_covering<vertex_type>(_all_edges, nvert)
   {
   }

   void do_clique_covering(const cc_compatibility_graphRef CG,
                           typename boost::disjoint_sets<rank_pmap_type, pred_pmap_type>& ds,
                           CustomUnorderedSet<C_vertex>& support, const CustomUnorderedSet<C_vertex>& all_vertices,
                           const filter_clique<vertex_type>& fc) override
   {
      TTT_maximal_weighted_clique<cc_compatibility_graph> MWC(coloring_based_clique_covering<vertex_type>::names);
      CustomUnorderedSet<C_vertex> support_copy(support);
      int upper_bound = std::numeric_limits<int>::max();
      while(!support.empty())
      {
         // std::cerr << "Looking for a maximum weighted clique on a graph with " << support.size() << " vertices" <<
         // std::endl;
         CustomOrderedSet<C_vertex> curr_clique = MWC.get_weighted_maximal_cliques(*CG, upper_bound);
         /// update the upper bound with the last weight
         upper_bound = MWC.get_last_W_Q_max();

         /// do some filtering
         bool removed_vertex = false;
         C_vertex vertex_to_be_removed;
         do
         {
            if(curr_clique.size() > 1)
            {
               CustomOrderedSet<C_vertex> curr_expandend_clique;
               const CustomUnorderedSet<C_vertex>::const_iterator av_it_end = all_vertices.end();
               for(auto av_it = all_vertices.begin(); av_it != av_it_end; ++av_it)
               {
                  C_vertex rep = ds.find_set(*av_it);
                  if(curr_clique.find(rep) != curr_clique.end())
                  {
                     curr_expandend_clique.insert(*av_it);
                  }
               }
               removed_vertex = fc.select_candidate_to_remove(curr_expandend_clique, vertex_to_be_removed,
                                                              coloring_based_clique_covering<vertex_type>::uv2v, *CG);
               if(removed_vertex)
               {
                  // std::cerr << "Vertex removed " << curr_clique.size() << std::endl;
                  curr_clique.erase(ds.find_set(vertex_to_be_removed));
               }
            }
            else
            {
               removed_vertex = false;
            }
         } while(removed_vertex);

         // std::cerr << "Found one of size " << curr_clique.size() << std::endl;
         C_vertex first = *curr_clique.begin();
         const CustomOrderedSet<C_vertex>::const_iterator cc_it_end = curr_clique.end();
         const CustomOrderedSet<C_vertex>::const_iterator first_cc_it = curr_clique.begin();
         CustomOrderedSet<C_vertex>::const_iterator cc_it = first_cc_it;
         do
         {
            C_vertex curr_vertex = *cc_it;
            auto current = support.find(curr_vertex);
            THROW_ASSERT(current != support.end(), "unexpected condition");
            if(cc_it != first_cc_it)
            {
               ds.union_set(first, curr_vertex);
            }
            support.erase(current);
            ++cc_it;
         } while(cc_it != cc_it_end);

         /// check trivial cases
         auto s_it_end = support.end();
         for(auto s_it = support.begin(); s_it != s_it_end;)
         {
            auto current = s_it++;
            if(boost::out_degree(*current, *CG) == 0)
            {
               support.erase(current);
            }
         }
      }
      support.insert(support_copy.begin(), support_copy.end());
   }
};

template <typename vertex_type>
class TS_based_clique_covering : public coloring_based_clique_covering<vertex_type>
{
   using edge_descriptor = boost::graph_traits<cc_compatibility_graph>::edge_descriptor;

   bool is_non_compliant(C_vertex src, C_vertex tgt, const cc_compatibility_graph& subgraph,
                         const CustomUnorderedSet<C_vertex>& all_vertices,
                         typename boost::disjoint_sets<rank_pmap_type, pred_pmap_type>& ds,
                         const filter_clique<vertex_type>& fc)
   {
      if(!fc.is_filtering())
      {
         return false;
      }
      C_vertex vertex_to_be_removed;
      CustomOrderedSet<C_vertex> curr_expandend_clique;
      const CustomUnorderedSet<C_vertex>::const_iterator av_it_end = all_vertices.end();
      for(auto av_it = all_vertices.begin(); av_it != av_it_end; ++av_it)
      {
         C_vertex rep = ds.find_set(*av_it);
         if(rep == src || rep == tgt)
         {
            curr_expandend_clique.insert(*av_it);
         }
      }
      return fc.select_candidate_to_remove(curr_expandend_clique, vertex_to_be_removed,
                                           coloring_based_clique_covering<vertex_type>::uv2v, subgraph);
   }

#define MAX_EDGE_CONSIDERED 50
   bool select_edge_start(C_vertex source, C_vertex& tgt, const cc_compatibility_graph& subgraph,
                          const CustomUnorderedSet<C_vertex>& all_vertices,
                          typename boost::disjoint_sets<rank_pmap_type, pred_pmap_type>& ds,
                          const filter_clique<vertex_type>& fc)
   {
      size_t h_max_neighbors = 0;
      size_t h_min_del_edges = std::numeric_limits<size_t>::max();
      bool first_iter = true;
      C_outEdgeIterator sei0, sei0_end;
      auto counter = 0u;
      for(boost::tie(sei0, sei0_end) = boost::out_edges(source, subgraph); sei0 != sei0_end; ++sei0)
      {
         C_vertex target = boost::target(*sei0, subgraph);
         if(is_non_compliant(source, target, subgraph, all_vertices, ds, fc))
         {
            continue;
         }

         size_t h_neighbors = 0, h_del_edges = 0;
         C_outEdgeIterator sei, sei_end, tei, tei_end;
         size_t src_out_degree = 0, tgt_out_degree = 0;
         CustomUnorderedSet<C_vertex> src_neighbors, tgt_neighbors, common_neighbors;

         for(boost::tie(sei, sei_end) = boost::out_edges(source, subgraph); sei != sei_end; ++sei)
         {
            ++src_out_degree;
            src_neighbors.insert(boost::target(*sei, subgraph));
         }
         for(boost::tie(tei, tei_end) = boost::out_edges(target, subgraph); tei != tei_end; ++tei)
         {
            ++tgt_out_degree;
            tgt_neighbors.insert(boost::target(*tei, subgraph));
         }
         unordered_set_intersection(src_neighbors.begin(), src_neighbors.end(), tgt_neighbors,
                                    std::inserter(common_neighbors, common_neighbors.end()));
         h_neighbors = common_neighbors.size();
         h_del_edges = src_out_degree + tgt_out_degree - h_neighbors - 1;
         if(first_iter || (h_neighbors > h_max_neighbors) ||
            (h_neighbors == h_max_neighbors && h_del_edges < h_min_del_edges))
         {
            first_iter = false;
            h_max_neighbors = h_neighbors;
            h_min_del_edges = h_del_edges;
            tgt = target;
         }
         if(counter > MAX_EDGE_CONSIDERED)
         {
            break;
         }
         else
         {
            ++counter;
         }
      }
      return !first_iter;
   }

   /// Compute the heuristics and return the best matching edge
   bool select_edge(C_vertex& src, C_vertex& tgt, const cc_compatibility_graph& subgraph,
                    const CustomUnorderedSet<C_vertex>& all_vertices,
                    typename boost::disjoint_sets<rank_pmap_type, pred_pmap_type>& ds,
                    const filter_clique<vertex_type>& fc)
   {
      size_t h_max_neighbors = 0;
      size_t h_min_del_edges = std::numeric_limits<size_t>::max();
      boost::graph_traits<cc_compatibility_graph>::edge_iterator ei, ei_end;
      bool first_iter = true;
      auto counter = 0u;
      for(boost::tie(ei, ei_end) = boost::edges(subgraph); ei != ei_end; ++ei)
      {
         C_vertex source, target;
         source = boost::source(*ei, subgraph);
         target = boost::target(*ei, subgraph);
         if(is_non_compliant(source, target, subgraph, all_vertices, ds, fc))
         {
            continue;
         }

         size_t h_neighbors = 0, h_del_edges = 0;
         C_outEdgeIterator sei, sei_end, tei, tei_end;
         size_t src_out_degree = 0, tgt_out_degree = 0;
         CustomUnorderedSet<C_vertex> src_neighbors, tgt_neighbors, common_neighbors;

         for(boost::tie(sei, sei_end) = boost::out_edges(source, subgraph); sei != sei_end; ++sei)
         {
            ++src_out_degree;
            src_neighbors.insert(boost::target(*sei, subgraph));
         }
         for(boost::tie(tei, tei_end) = boost::out_edges(target, subgraph); tei != tei_end; ++tei)
         {
            ++tgt_out_degree;
            tgt_neighbors.insert(boost::target(*tei, subgraph));
         }
         unordered_set_intersection(src_neighbors.begin(), src_neighbors.end(), tgt_neighbors,
                                    std::inserter(common_neighbors, common_neighbors.end()));
         h_neighbors = common_neighbors.size();
         h_del_edges = src_out_degree + tgt_out_degree - h_neighbors - 1;
         if(first_iter || (h_neighbors > h_max_neighbors) ||
            (h_neighbors == h_max_neighbors && h_del_edges < h_min_del_edges))
         {
            first_iter = false;
            h_max_neighbors = h_neighbors;
            h_min_del_edges = h_del_edges;
            src = source;
            tgt = target;
         }
         if(counter > MAX_EDGE_CONSIDERED)
         {
            break;
         }
         else
         {
            ++counter;
         }
      }
      return !first_iter;
   }

 public:
   /// constructor
   explicit TS_based_clique_covering(bool _all_edges, unsigned int nvert)
       : coloring_based_clique_covering<vertex_type>(_all_edges, nvert)
   {
   }

   void do_clique_covering(const cc_compatibility_graphRef CG,
                           typename boost::disjoint_sets<rank_pmap_type, pred_pmap_type>& ds,
                           CustomUnorderedSet<C_vertex>& support, const CustomUnorderedSet<C_vertex>& all_vertices,
                           const filter_clique<vertex_type>& fc) override
   {
      CustomUnorderedSet<C_vertex> support_copy(support);

      while(support.size() > 1)
      {
         // std::cerr << "Looking for a maximum weighted clique on a graph with " << support.size() << " vertices"
         //           << std::endl;
         /// build the clique seed
         C_vertex src, tgt;
         bool res_edge = select_edge(src, tgt, *CG, all_vertices, ds, fc);
         if(!res_edge)
         {
            break;
         }

         std::map<edge_descriptor, int> removed_edges;
         CustomUnorderedSet<C_vertex>::iterator current;

         do
         {
            /// remove non conformant edges
            CustomOrderedSet<C_vertex> neighbors_src;
            neighbors_src.insert(boost::adjacent_vertices(src, *CG).first, adjacent_vertices(src, *CG).second);
            CustomOrderedSet<C_vertex> neighbors_tgt;
            neighbors_tgt.insert(boost::adjacent_vertices(tgt, *CG).first, adjacent_vertices(tgt, *CG).second);
            C_outEdgeIterator sei, sei_end;
            for(boost::tie(sei, sei_end) = boost::out_edges(src, *CG); sei != sei_end; ++sei)
            {
               C_vertex target = boost::target(*sei, *CG);
               if(tgt == target)
               {
                  continue;
               }
               if(neighbors_tgt.find(target) == neighbors_tgt.end())
               {
                  removed_edges[*sei] = (*CG)[*sei].selector;
                  (*CG)[*sei].selector = 0;
               }
            }
            for(boost::tie(sei, sei_end) = boost::out_edges(tgt, *CG); sei != sei_end; ++sei)
            {
               C_vertex target = boost::target(*sei, *CG);
               if(src == target)
               {
                  continue;
               }
               if(neighbors_src.find(target) == neighbors_src.end())
               {
                  removed_edges[*sei] = (*CG)[*sei].selector;
                  (*CG)[*sei].selector = 0;
               }
            }
            ds.union_set(src, tgt);
            C_vertex rep = ds.find_set(src);
            THROW_ASSERT(rep == src || rep == tgt, "unexpected condition");
            if(rep != src)
            {
               std::swap(src, tgt);
            }
            current = support.find(tgt);
            THROW_ASSERT(current != support.end(), "unexpected condition");
            support.erase(current);
         } while(select_edge_start(src, tgt, *CG, all_vertices, ds, fc));

         current = support.find(src);
         THROW_ASSERT(current != support.end(), "unexpected condition");
         support.erase(current);
         const auto re_it_end = removed_edges.end();
         for(auto re_it = removed_edges.begin(); re_it != re_it_end; ++re_it)
         {
            (*CG)[re_it->first].selector = re_it->second;
         }

         // std::cerr << "Found one of size " << cluster_size << std::endl;

         /// check trivial cases
         auto s_it_end = support.end();
         for(auto s_it = support.begin(); s_it != s_it_end;)
         {
            current = s_it++;
            if(boost::out_degree(*current, *CG) == 0)
            {
               support.erase(current);
            }
         }
      }

      support.insert(support_copy.begin(), support_copy.end());
   }
};

template <typename vertex_type>
class RTS_based_clique_covering : public TS_based_clique_covering<vertex_type>
{
   using edge_descriptor = boost::graph_traits<cc_compatibility_graph>::edge_descriptor;

   size_t compute_cost(const std::vector<CustomOrderedSet<C_vertex>>& curr_cliques,
                       const filter_clique<vertex_type>& fc)
   {
      size_t total_cost = 0;
      for(const auto& clique_val : curr_cliques)
      {
         total_cost += fc.clique_cost(clique_val, coloring_based_clique_covering<vertex_type>::uv2v);
      }
      return total_cost;
   }

 public:
   /// constructor
   explicit RTS_based_clique_covering(bool _all_edges) : TS_based_clique_covering<vertex_type>(_all_edges)
   {
   }

   void exec(const filter_clique<vertex_type>& fc)
   {
      std::vector<CustomOrderedSet<C_vertex>> best_cliques;
      size_t best_cost = std::numeric_limits<size_t>::max();
      size_t iterations =
          std::min(boost::num_vertices(coloring_based_clique_covering<vertex_type>::clique_covering_graph_bulk), 10ul);
      // std::cerr << "N iterations " << iterations << std::endl;
      for(unsigned int i = 0; i < iterations; ++i)
      {
         coloring_based_clique_covering<vertex_type>::cliques.clear();
         coloring_based_clique_covering<vertex_type>::exec(fc);
         size_t new_cost = compute_cost(coloring_based_clique_covering<vertex_type>::cliques, fc);
         if(new_cost < best_cost)
         {
            // std::cerr << "New cost " << new_cost << "-" << i << std::endl;
            best_cliques = coloring_based_clique_covering<vertex_type>::cliques;
            best_cost = new_cost;
         }
      }
      coloring_based_clique_covering<vertex_type>::cliques = best_cliques;
   }
};

/// clique covering based on bipartite matching procedure
template <typename vertex_type>
class bipartite_matching_clique_covering : public clique_covering<vertex_type>
{
 private:
   /// bulk undirected graph
   boost_cc_compatibility_graph clique_covering_graph_bulk;
   /// set of maximal clique computed
   std::vector<CustomUnorderedSet<C_vertex>> cliques;
   /// map between vertex_type and C_vertex
   CustomUnorderedMap<vertex_type, C_vertex> v2uv;
   /// map between C_vertex and vertex_type
   CustomUnorderedMap<C_vertex, vertex_type> uv2v; // converter
   /// name map for the C_vertex vertices
   CustomUnorderedMap<C_vertex, std::string> names;
   /// maximum weight
   int max_weight;
   unsigned int vindex;
   /// number of columns
   size_t num_cols;
   /// maximum number of columns
   size_t max_num_cols;
   std::map<size_t, CustomOrderedSet<boost::graph_traits<boost_cc_compatibility_graph>::vertex_descriptor>> partitions;
   /// edge selector
   static const int COMPATIBILITY_EDGE = 1;
   static const int COMPATIBILITY_ALL_EDGES = ~0;

   /// partition the set of vertex in set where each element is in conflict with the others
   /// the maximum size of these sets is the number of the cliques used to cover the graph
   size_t color_the_cc_compatibility_graph(const boost_cc_compatibility_graph& CG)
   {
      using cg_vertices_size_type = boost::graph_traits<boost_cc_compatibility_graph>::vertices_size_type;
      using cg_vertex_index_map = boost::property_map<boost_cc_compatibility_graph, boost::vertex_index_t>::const_type;
      boost::iterator_property_map<cg_vertices_size_type*, cg_vertex_index_map, cg_vertices_size_type,
                                   cg_vertices_size_type&>
          color;

      std::vector<cg_vertices_size_type> color_vec;

      size_t n_vertices = boost::num_vertices(CG);
      color_vec.resize(n_vertices);
      color =
          boost::iterator_property_map<cg_vertices_size_type*, cg_vertex_index_map, cg_vertices_size_type,
                                       cg_vertices_size_type&>(&color_vec.front(), boost::get(boost::vertex_index, CG));

      /// coloring based on DSATUR 2 heuristic
      dsatur2_coloring(CG, color);
      size_t max_size = 1;
      boost::graph_traits<boost_cc_compatibility_graph>::vertex_iterator ui, uiend;
      for(boost::tie(ui, uiend) = boost::vertices(clique_covering_graph_bulk); ui != uiend; ++ui)
      {
         cg_vertices_size_type c = color[*ui];
         auto p_it = partitions.find(c);
         if(p_it == partitions.end())
         {
            CustomOrderedSet<boost::graph_traits<boost_cc_compatibility_graph>::vertex_descriptor> singularity;
            singularity.insert(*ui);
            partitions[c] = singularity;
         }
         else
         {
            p_it->second.insert(*ui);
            max_size = std::max(max_size, p_it->second.size());
         }
      }
      return max_size;
   }

 public:
   /// constructor
   bipartite_matching_clique_covering(unsigned int nvert)
       : clique_covering_graph_bulk(nvert),
         max_weight(std::numeric_limits<int>::min()),
         vindex(0),
         num_cols(0),
         max_num_cols(0)
   {
   }

   /// add a vertex
   C_vertex add_vertex(const vertex_type& element, const std::string& name) override
   {
      C_vertex result;
      THROW_ASSERT(v2uv.find(element) == v2uv.end(), "vertex already added");
      /// vertex weight not considered
      v2uv[element] = result = boost::vertex(vindex, clique_covering_graph_bulk);
      ++vindex;
      uv2v[result] = element;
      names[result] = name;
      return result;
   }

   /// add an edge
   void add_edge(const vertex_type& src, const vertex_type& dest, int _weight) override
   {
      THROW_ASSERT(src != dest, "autoloops are not allowed in a compatibility graph");
      THROW_ASSERT(v2uv.find(src) != v2uv.end(), "src not added");
      THROW_ASSERT(v2uv.find(dest) != v2uv.end(), "dest not added");
      THROW_ASSERT(_weight > 0, "weights greater than 0 are allowed");
      C_vertex SRC = v2uv.find(src)->second;
      C_vertex DEST = v2uv.find(dest)->second;
      max_weight = std::max(max_weight, _weight);
      boost::add_edge(SRC, DEST, edge_compatibility_selector(COMPATIBILITY_EDGE, _weight), clique_covering_graph_bulk);
   }

   /// return the number of vertices of the clique
   size_t num_vertices() override
   {
      return cliques.size();
   }

   CustomOrderedSet<vertex_type> get_clique(unsigned int i) override
   {
      CustomOrderedSet<vertex_type> result;
      CustomUnorderedSet<C_vertex>& cur_clique = cliques[i];
      CustomUnorderedSet<C_vertex>::const_iterator it_end = cur_clique.end();
      for(CustomUnorderedSet<C_vertex>::const_iterator it = cur_clique.begin(); it != it_end; ++it)
      {
         THROW_ASSERT(uv2v.find(*it) != uv2v.end(), "vertex not added");
         result.insert(uv2v.find(*it)->second);
      }
      return result;
   }

   void exec(const filter_clique<vertex_type>& fc, check_clique<vertex_type>&) override
   {
      /// now color the graph and then do the bipartite matching on the vertex having the same color
      /// usually this reduces the number of the partitions and simplifies the problem
      partitions.clear();
      num_cols = color_the_cc_compatibility_graph(clique_covering_graph_bulk);
      // std::cerr << "initial max_num_cols " << max_num_cols << std::endl;
      auto completeCG = cc_compatibility_graphRef(
          new cc_compatibility_graph(clique_covering_graph_bulk,
                                     cc_compatibility_graph_edge_selector<boost_cc_compatibility_graph>(
                                         COMPATIBILITY_ALL_EDGES, &clique_covering_graph_bulk),
                                     cc_compatibility_graph_vertex_selector<boost_cc_compatibility_graph>()));

      // std::cerr << "cliques.size " << cliques.size() << "\n";
      /// initializes the cliques with empty sets
      for(unsigned int i = 0; i < num_cols; ++i)
      {
         CustomUnorderedSet<C_vertex> empty;
         cliques.push_back(empty);
      }
      // std::cerr << "num_cols " << num_cols << "\n";
      // std::cerr << "partitions.size " << partitions.size() << "\n";
      std::vector<CustomOrderedSet<boost::graph_traits<boost_cc_compatibility_graph>::vertex_descriptor>>
          partitionsSorted;
      for(const auto& p : partitions)
      {
         partitionsSorted.push_back(p.second);
      }
      std::sort(
          partitionsSorted.begin(), partitionsSorted.end(),
          [](const CustomOrderedSet<boost::graph_traits<boost_cc_compatibility_graph>::vertex_descriptor>& a,
             const CustomOrderedSet<boost::graph_traits<boost_cc_compatibility_graph>::vertex_descriptor>& b) -> bool {
             return a.size() > b.size();
          });
      size_t num_rows = num_cols;
      bool restart_bipartite;
      do
      {
         restart_bipartite = false;
         /// compute the assignment for each element of a partition
         // int pindex = 0;
         for(const auto& p : partitionsSorted)
         {
            // std::cerr << "partition" << pindex++ << "\n";
            // std::cerr << "partition size=" << p.size() << "\n";
            operations_research::SimpleLinearSumAssignment assignment;
            std::set<unsigned> column_already_assigned;
            auto v_it_end = p.end();
            auto v_it = p.begin();
            // auto min_to_be_removed = std::numeric_limits<unsigned>::max();
            for(unsigned i = 0; i < num_rows && !restart_bipartite; ++i)
            {
               if(v_it == v_it_end)
               {
                  // std::cerr << "over\n";
                  for(unsigned int y = 0; y < num_cols; ++y)
                  {
                     assignment.AddArcWithCost(static_cast<operations_research::NodeIndex>(i),
                                               static_cast<operations_research::NodeIndex>(y), 1);
                     // std::cerr << "i" << i << "-> y" << y << " (1)\n";
                  }
               }
               else
               {
                  /// check if already assigned
                  bool compatible_exist = false;
                  unsigned compatible_column = 0;
                  for(unsigned int y = 0; y < num_cols; ++y)
                  {
                     if(cliques[y].find(*v_it) != cliques[y].end())
                     {
                        /// already assigned to a clique
                        // std::cerr << "already assigned to a clique\n";
                        compatible_exist = true;
                        compatible_column = y;
                        THROW_ASSERT(column_already_assigned.count(y) == 0, "unexpected condition");
                        column_already_assigned.insert(y);
                        break;
                     }
                  }
                  if(compatible_exist)
                  {
                     assignment.AddArcWithCost(static_cast<operations_research::NodeIndex>(i),
                                               static_cast<operations_research::NodeIndex>(compatible_column), 1);
                     // std::cerr << "i" << i << "-> y" << compatible_column << " (1)\n";
                     // std::cerr << "compatible_exist\n";
                  }
                  else
                  {
                     // std::cerr << "compatible does not exist " << max_num_cols << " " << num_cols << "\n";
                     bool added_an_element = false;
                     // unsigned to_removed_number = 0;
                     for(unsigned int y = 0; y < num_cols; ++y)
                     {
                        if(column_already_assigned.find(y) == column_already_assigned.end())
                        {
                           CustomOrderedSet<C_vertex> curr_expandend_clique;
                           const auto& current_clique = cliques.at(y);
                           // std::cerr << "current_clique.size=" << current_clique.size() << "y=" << y << "\n";
                           // std::cerr << "y=" << y << "\n";
                           bool to_be_removed = false;
                           for(const auto cv : current_clique)
                           {
                              // std::cerr << "*v_it=" << *v_it << " cv=" << cv << "\n";
                              auto edge_check = boost::edge(*v_it, cv, *completeCG);
                              if(!edge_check.second)
                              {
                                 to_be_removed = true;
                                 break;
                              }
                           }
                           if(!to_be_removed && current_clique.size())
                           {
                              curr_expandend_clique.insert(current_clique.begin(), current_clique.end());
                              curr_expandend_clique.insert(*v_it);
                              // std::cerr << "curr_expandend_clique.size=" << curr_expandend_clique.size() << "\n";
                              C_vertex vertex_to_be_removed;
                              to_be_removed = fc.select_candidate_to_remove(curr_expandend_clique, vertex_to_be_removed,
                                                                            uv2v, *completeCG);
                           }
                           if(to_be_removed)
                           {
                              // std::cerr << "to be removed\n";
                              //++to_removed_number;
                           }
                           if(!to_be_removed)
                           {
                              auto clique_cost = 1 + fc.clique_cost(curr_expandend_clique, uv2v);
                              assignment.AddArcWithCost(static_cast<operations_research::NodeIndex>(i),
                                                        static_cast<operations_research::NodeIndex>(y), clique_cost);
                              added_an_element = true;
                              // std::cerr << "i" << i << "-> y" << y << " (" << clique_cost << ")\n";
                           }
                        }
                     }
                     // min_to_be_removed = std::min(min_to_be_removed, to_removed_number);
                     // std::cerr << "to_removed_number=" << to_removed_number << " p.size() " << p.size()
                     //           << " num_columns " << num_cols << "\n";
                     if(!added_an_element)
                     {
                        restart_bipartite = true;
                        //++skip_infeasibles;
                        // std::cerr << "restart the problem (" + STR(num_cols) + ")\n";
                     }
                  }
                  ++v_it;
               }
            }
            if(!restart_bipartite && assignment.Solve() == operations_research::SimpleLinearSumAssignment::OPTIMAL)
            {
               v_it = p.begin();
               for(unsigned int i = 0; i < num_rows; ++i)
               {
                  if(v_it != v_it_end)
                  {
                     auto s = assignment.RightMate(i);
                     // std::cerr << "assign " << names[*v_it] << " to clique" << s << "\n";
                     cliques[s].insert(*v_it);
                     ++v_it;
                  }
               }
            }
            else
            {
               // std::cerr << "restart assignment problem " << num_cols << "\n";
               THROW_ASSERT(max_num_cols == 0 || max_num_cols > num_cols,
                            "Module binding does not have a solution\n  Current number of resources=" + STR(num_cols) +
                                " Maximum number of resources " + STR(max_num_cols));
               for(unsigned sindex = 0; sindex < 1; ++sindex)
               {
                  ++num_cols;
                  ++num_rows;
                  CustomUnorderedSet<C_vertex> empty;
                  cliques.push_back(empty);
                  if(max_num_cols != 0 && num_cols == max_num_cols)
                  {
                     break;
                  }
               }
               for(auto& cl : cliques)
               {
                  cl.clear();
               }
               THROW_ASSERT(max_num_cols == 0 || max_num_cols >= num_cols,
                            "Module binding does not have a solution\n  Current number of resources=" + STR(num_cols) +
                                " Maximum number of resources " + STR(max_num_cols));
               restart_bipartite = true;
               break;
            }
         }
      } while(restart_bipartite);

      /// clean the results from empty cliques
      for(auto itC = cliques.begin(); itC != cliques.end();)
      {
         if(itC->empty())
         {
            itC = cliques.erase(itC);
         }
         else
         {
            ++itC;
         }
      }
   }

   void writeDot(const std::string& filename) const override
   {
      std::ofstream f(filename.c_str());
      boost::write_graphviz(f, clique_covering_graph_bulk, compatibility_node_info_writer(names),
                            compatibility_edge_writer(clique_covering_graph_bulk));
   }

   void add_subpartitions(size_t id, vertex_type v) override
   {
      THROW_ASSERT(v2uv.find(v) != v2uv.end(), "vertex not added");
      C_vertex C_v = v2uv.find(v)->second;
      partitions[id].insert(C_v);
      num_cols = std::max(partitions[id].size(), num_cols);
   }

   void suggest_min_resources(size_t n_resources) override
   {
      num_cols = std::max(num_cols, n_resources);
   }

   void min_resources(size_t n_resources) override
   {
      num_cols = std::max(num_cols, n_resources);
   }

   void suggest_max_resources(size_t) override
   {
   }

   void max_resources(size_t n_resources) override
   {
      max_num_cols = std::max(max_num_cols, n_resources);
   }
};

//******************************************************************************************************************

template <typename VertexType>
refcount<clique_covering<VertexType>> clique_covering<VertexType>::create_solver(CliqueCovering_Algorithm solver,
                                                                                 unsigned int nvert)
{
   switch(solver)
   {
      case CliqueCovering_Algorithm::COLORING:
         return refcount<clique_covering<VertexType>>(new coloring_based_clique_covering<VertexType>(true, nvert));
      case CliqueCovering_Algorithm::WEIGHTED_COLORING:
         return refcount<clique_covering<VertexType>>(new coloring_based_clique_covering<VertexType>(false, nvert));
      case CliqueCovering_Algorithm::TTT_CLIQUE_COVERING:
         return refcount<clique_covering<VertexType>>(new TTT_based_clique_covering<VertexType>(true, nvert));
      case CliqueCovering_Algorithm::TTT_CLIQUE_COVERING2:
         return refcount<clique_covering<VertexType>>(new TTT_based_clique_covering<VertexType>(false, nvert));
      case CliqueCovering_Algorithm::TTT_CLIQUE_COVERING_FAST:
         return refcount<clique_covering<VertexType>>(new TTT_based_clique_covering_fast<VertexType>(true, nvert));
      case CliqueCovering_Algorithm::TTT_CLIQUE_COVERING_FAST2:
         return refcount<clique_covering<VertexType>>(new TTT_based_clique_covering_fast<VertexType>(false, nvert));
      case CliqueCovering_Algorithm::TS_CLIQUE_COVERING:
         return refcount<clique_covering<VertexType>>(new TS_based_clique_covering<VertexType>(true, nvert));
      case CliqueCovering_Algorithm::TS_WEIGHTED_CLIQUE_COVERING:
         return refcount<clique_covering<VertexType>>(new TS_based_clique_covering<VertexType>(false, nvert));
      case CliqueCovering_Algorithm::BIPARTITE_MATCHING:
         return refcount<clique_covering<VertexType>>(new bipartite_matching_clique_covering<VertexType>(nvert));
      default:
         THROW_UNREACHABLE("This clique covering algorithm has not been implemented");
   }
   return refcount<clique_covering<VertexType>>();
}

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#endif // TTT_BASED_CLIQUE_COVERING_HPP
