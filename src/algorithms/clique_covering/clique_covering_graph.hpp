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
 * @file clique_covering_graph.hpp
 * @author Stefano Bodini, Federico Badini
 *
 */
#ifndef CLIQUE_COVERING_GRAPH_HPP
#define CLIQUE_COVERING_GRAPH_HPP

/// Graph includes
#include <boost/config.hpp>
#include <boost/graph/adjacency_matrix.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/incremental_components.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/topological_sort.hpp>
#include <boost/pending/disjoint_sets.hpp>
#include <boost/version.hpp>

#include "custom_map.hpp"
#include "custom_set.hpp"
#include "refcount.hpp"

/// Predicate functor object used to select the proper set of vertices
template <typename Graph>
struct cc_compatibility_graph_vertex_selector
{
 public:
   using vertex_descriptor = typename boost::graph_traits<Graph>::vertex_descriptor;
   using SET_container = CustomUnorderedSet<vertex_descriptor>;
   /// constructor
   cc_compatibility_graph_vertex_selector() : all(true), support(nullptr)
   {
   }
   /// constructor
   explicit cc_compatibility_graph_vertex_selector(SET_container* _support) : all(false), support(_support)
   {
   }
   /// selector operator
   bool operator()(const vertex_descriptor& v) const
   {
      if(all)
      {
         return true;
      }
      else
      {
         return support->find(v) != support->end();
      }
   }

 private:
   bool all;
   SET_container* support;
};

/// Predicate functor object used to select the proper set of edges
template <typename Graph>
struct cc_compatibility_graph_edge_selector
{
 private:
   /// The selector associated with the filtered graph
   int selector;

   /// bulk graph
   Graph* g;

   bool all;

 public:
   /**
    * Constructor for filtering only on selector
    * @param _selector is the selector of the filtered graph
    * @param _g is the graph
    */
   cc_compatibility_graph_edge_selector(const int _selector, Graph* _g) : selector(_selector), g(_g), all(false)
   {
   }

   /// all edges selector
   cc_compatibility_graph_edge_selector() : selector(0), g(nullptr), all(true)
   {
   }

   /// edge selector operator
   template <typename Edge>
   bool operator()(const Edge& e) const
   {
      if(all)
      {
         return true;
      }
      else
      {
         return selector & (*g)[e].selector;
      }
   }
};

struct edge_compatibility_selector
{
 public:
   /// The selector associated with the edge
   int selector;

   /// edge weight
   int weight;

   edge_compatibility_selector() : selector(0), weight(0)
   {
   }

   /**
    * Constructor with selector
    * @param _selector is the selector to be associated with the edge
    * @param _weight is the weight to be associated with the edge
    */
   edge_compatibility_selector(int _selector, int _weight) : selector(_selector), weight(_weight)
   {
   }
};

/// bulk compatibility graph
using boost_cc_compatibility_graph =
    boost::adjacency_matrix<boost::undirectedS, boost::property<boost::vertex_index_t, std::size_t>,
                            edge_compatibility_selector>;

/// compatibility graph
using cc_compatibility_graph =
    boost::filtered_graph<boost_cc_compatibility_graph,
                          cc_compatibility_graph_edge_selector<boost_cc_compatibility_graph>,
                          cc_compatibility_graph_vertex_selector<boost_cc_compatibility_graph>>;

/// refcount version of cc_compatibility_graph
using cc_compatibility_graphRef = refcount<cc_compatibility_graph>;

/// cc_compatibility_graph vertex
using C_vertex = boost::graph_traits<cc_compatibility_graph>::vertex_descriptor;
using C_outEdgeIterator = boost::graph_traits<cc_compatibility_graph>::out_edge_iterator;

/// vertices index type
using VertexIndex = boost::graph_traits<boost_cc_compatibility_graph>::vertices_size_type;
/// index map type
using vertex_index_pmap_t = boost::property_map<boost_cc_compatibility_graph, boost::vertex_index_t>::type;
/// rank property map definition
using rank_pmap_type = boost::iterator_property_map<std::vector<VertexIndex>::iterator, vertex_index_pmap_t>;
/// parent property map definition
using pred_pmap_type = boost::iterator_property_map<std::vector<C_vertex>::iterator, vertex_index_pmap_t>;

//*************** definitions used in randomized clique covering

using RVertexIndex = boost::graph_traits<boost_cc_compatibility_graph>::vertices_size_type;
using REdge = boost::graph_traits<boost_cc_compatibility_graph>::edge_descriptor;

/* VERSIONE PRECEDENTE
typedef RVertexIndex* RRank;
typedef C_vertex* RParent;
typedef boost::disjoint_sets<RRank, RParent, boost::find_with_full_path_compression> RDisjointSet;
*/

using RDisjointSet = boost::disjoint_sets<rank_pmap_type, pred_pmap_type>;

#endif
