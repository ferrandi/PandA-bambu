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
 *              Copyright (C) 2004-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */
/**
 * @file graph.hpp
 * @brief Class specification of the graph structures.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#ifndef GRAPH_HPP
#define GRAPH_HPP
#include "custom_set.hpp"
#include "exceptions.hpp"
#include "refcount.hpp"

#include <boost/config.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/strong_components.hpp>
#include <boost/graph/topological_sort.hpp>
#include <boost/version.hpp>

#include <deque>
#include <filesystem>
#include <fstream>
#include <list>
#include <ostream>
#include <utility>

template <class Graph>
struct graph_base : public Graph
{
   typedef typename boost::graph_traits<Graph>::vertex_descriptor vertex_descriptor;
   typedef typename boost::graph_traits<Graph>::edge_descriptor edge_descriptor;

   typedef typename boost::graph_property_type<Graph>::type graph_property_type;
   typedef typename boost::lookup_one_property<graph_property_type, boost::graph_bundle_t>::type graph_bundled;

   typedef typename boost::edge_property_type<Graph>::type edge_property_type;
   typedef typename boost::lookup_one_property<edge_property_type, boost::edge_bundle_t>::type edge_bundled;

   typedef typename boost::vertex_property_type<Graph>::type vertex_property_type;
   typedef typename boost::lookup_one_property<vertex_property_type, boost::vertex_bundle_t>::type vertex_bundled;

   template <typename... Args>
   graph_base(const graph_bundled& g_info, Args&&... args) : Graph(std::forward<Args>(args)...)
   {
      this->operator[](boost::graph_bundle) = g_info;
   }

   template <typename graph_bundled_t = graph_bundled, typename... Args>
   graph_base(Args&&... args) : Graph(std::forward<Args>(args)...)
   {
   }

   auto vertices() const
   {
      return boost::make_iterator_range(boost::vertices(*this));
   }

   auto num_vertices() const
   {
      return boost::num_vertices(*this);
   }

   auto edges() const
   {
      return boost::make_iterator_range(boost::edges(*this));
   }

   auto num_edges() const
   {
      return boost::num_edges(*this);
   }

   vertex_descriptor source(edge_descriptor e) const
   {
      return boost::source(e, *this);
   }

   vertex_descriptor target(edge_descriptor e) const
   {
      return boost::target(e, *this);
   }

   auto in_degree(vertex_descriptor v) const
   {
      return boost::in_degree(v, *this);
   }

   auto in_edges(vertex_descriptor v) const
   {
      return boost::make_iterator_range(boost::in_edges(v, *this));
   }

   auto out_degree(vertex_descriptor v) const
   {
      return boost::out_degree(v, *this);
   }

   auto out_edges(vertex_descriptor v) const
   {
      return boost::make_iterator_range(boost::out_edges(v, *this));
   }

   auto reverse_graph() const
   {
      return graph_base<boost::reverse_graph<Graph>>(*this);
   }

   bool ExistsEdge(vertex_descriptor src, vertex_descriptor tgt) const
   {
      return boost::edge(src, tgt, *this).second;
   }

   edge_descriptor CGetEdge(vertex_descriptor src, vertex_descriptor tgt) const
   {
      auto [e, found] = boost::edge(src, tgt, *this);
      THROW_ASSERT(found, "Edge does not exist in this graph");
      return e;
   }

   template <typename T = vertex_bundled, std::enable_if_t<!std::is_empty<T>::value, bool> = true>
   T& GetNodeInfo(vertex_descriptor node)
   {
      return this->operator[](node);
   }

   template <typename T = vertex_bundled, std::enable_if_t<!std::is_empty<T>::value, bool> = true>
   const T& CGetNodeInfo(vertex_descriptor node) const
   {
      return this->operator[](node);
   }

   template <typename T = edge_bundled, std::enable_if_t<!std::is_empty<T>::value, bool> = true>
   T& GetEdgeInfo(edge_descriptor edge)
   {
      return this->operator[](edge);
   }

   template <typename T = edge_bundled, std::enable_if_t<!std::is_empty<T>::value, bool> = true>
   const T& CGetEdgeInfo(edge_descriptor edge) const
   {
      return this->operator[](edge);
   }

   template <typename T = graph_bundled, std::enable_if_t<!std::is_empty<T>::value, bool> = true>
   T& GetGraphInfo()
   {
      return this->operator[](boost::graph_bundle);
   }

   template <typename T = graph_bundled, std::enable_if_t<!std::is_empty<T>::value, bool> = true>
   const T& CGetGraphInfo() const
   {
      return this->operator[](boost::graph_bundle);
   }

   bool IsReachable(const vertex_descriptor x, const vertex_descriptor y) const
   {
      std::deque<vertex_descriptor> running_vertices;
      std::set<vertex_descriptor> encountered_vertices;
      running_vertices.push_back(x);
      encountered_vertices.insert(x);
      while(!running_vertices.empty())
      {
         const auto current = running_vertices.front();
         running_vertices.pop_front();
         for(const auto& oe : boost::make_iterator_range(boost::out_edges(current, *this)))
         {
            const auto tgt = boost::target(oe, *this);
            if(tgt == y)
            {
               return true;
            }
            if(encountered_vertices.insert(tgt).second)
            {
               running_vertices.push_back(tgt);
            }
         }
      }
      return false;
   }

   std::vector<std::list<vertex_descriptor>> GetStronglyConnectedComponents() const
   {
      std::vector<std::list<vertex_descriptor>> sccs;
      std::map<vertex_descriptor, size_t> _vtoc;
      boost::associative_property_map<std::map<vertex_descriptor, size_t>> vtoc(_vtoc);
      const auto sccs_count = boost::strong_components<const Graph>(*this, vtoc);
      sccs.resize(sccs_count);
      for(const auto v : boost::make_iterator_range(boost::vertices(*this)))
      {
         sccs.at(vtoc[v]).push_back(v);
      }
      return sccs;
   }

   /**
    * Compute the reverse topological order of the graph
    * @param sorted_vertices is where results will be store
    */
   template <class Container>
   void ReverseTopologicalSort(Container& sorted_vertices) const
   {
      boost::topological_sort<const Graph>(*this, std::back_inserter(sorted_vertices));
   }

   /**
    * Compute the topological order of the graph
    * @param sorted_vertices is where results will be store
    */
   template <class Container>
   void TopologicalSort(Container& sorted_vertices) const
   {
      boost::topological_sort<const Graph>(*this, std::front_inserter(sorted_vertices));
   }

   template <class DFSVisitor>
   void DepthFirstVisit(vertex_descriptor u, DFSVisitor vis) const
   {
      std::vector<boost::default_color_type> color_vec(num_vertices());
      boost::depth_first_visit(*this, u, vis,
                               boost::make_iterator_property_map(
                                   color_vec.begin(), boost::get(boost::vertex_index_t(), *this), boost::white_color));
   }

   /**
    * Compute depth for each vertex (each vertex may have different depths in unbalanced graphs)
    */
   std::vector<std::set<vertex_descriptor>> DepthMap() const
   {
      std::vector<std::set<vertex_descriptor>> depth_vertices;
      std::unordered_map<vertex_descriptor, std::set<unsigned int>> vertex_depths;
      std::list<vertex_descriptor> sorted_vertices;
      TopologicalSort(sorted_vertices);
      depth_vertices.push_back(std::set<vertex_descriptor>());
      depth_vertices[0].insert(sorted_vertices.front());
      vertex_depths[sorted_vertices.front()].insert(0);
      sorted_vertices.pop_front();
      for(const auto v : sorted_vertices)
      {
         for(const auto& e : in_edges(v))
         {
            const auto src = source(e);
            auto& depths = vertex_depths[v];
            THROW_ASSERT(vertex_depths.count(src), "");
            for(const auto depth : vertex_depths.at(src))
            {
               depths.insert(depth + 1U);
               if(depth_vertices.size() <= depth + 1U)
               {
                  depth_vertices.resize(depth + 2U);
               }
               depth_vertices[depth + 1U].insert(v);
            }
         }
      }
      return depth_vertices;
   }

   template <typename VertexPropertiesWriter, typename EdgePropertiesWriter, typename GraphPropertiesWriter>
   void writeDot(const std::filesystem::path& filename, VertexPropertiesWriter vpw, EdgePropertiesWriter epw,
                 GraphPropertiesWriter gpw) const
   {
      std::filesystem::create_directories(filename.parent_path());
      std::ofstream fs(filename);
      boost::write_graphviz(fs, *this, vpw, epw, gpw);
   }

   template <typename VertexPropertiesWriter, typename EdgePropertiesWriter>
   void writeDot(const std::filesystem::path& filename, VertexPropertiesWriter vpw, EdgePropertiesWriter epw) const
   {
      boost::default_writer gpw;
      writeDot(filename, vpw, epw, gpw);
   }

   template <typename VertexPropertiesWriter>
   void writeDot(const std::filesystem::path& filename, VertexPropertiesWriter vpw) const
   {
      boost::default_writer epw;
      boost::default_writer gpw;
      writeDot(filename, vpw, epw, gpw);
   }
};

using gc_vertex_list_t = boost::listS;

using gc_directed = boost::bidirectionalS;

using gc_vertex_descriptor = typename boost::mpl::if_<typename boost::detail::is_random_access<gc_vertex_list_t>::type,
                                                      std::size_t, void*>::type;

inline constexpr gc_vertex_descriptor gc_null_vertex()
{
   return static_cast<gc_vertex_descriptor>(0);
}

using gc_edge_descriptor = boost::detail::edge_desc_impl<boost::bidirectional_tag, gc_vertex_descriptor>;

/**
 * Definition of hash function for gc_edge_descriptor
 */
namespace std
{
   template <>
   struct hash<gc_edge_descriptor>
   {
      size_t operator()(gc_edge_descriptor edge) const
      {
         size_t hash_value = 0;
         boost::hash_combine(hash_value, edge.m_source);
         boost::hash_combine(hash_value, edge.m_target);
         return hash_value;
      }
   };
} // namespace std

template <typename node_info_t = boost::no_property, typename edge_info_t = boost::no_property,
          typename graph_info_t = boost::no_property>
using RawGraphBase =
    boost::adjacency_list<gc_vertex_list_t, gc_vertex_list_t, gc_directed,
                          boost::property<boost::vertex_index_t, std::size_t, node_info_t>, edge_info_t, graph_info_t>;

template <typename node_info_t = boost::no_property, typename edge_info_t = boost::no_property,
          typename graph_info_t = boost::no_property>
struct RawGraph : public graph_base<RawGraphBase<node_info_t, edge_info_t, graph_info_t>>
{
   typedef RawGraphBase<node_info_t, edge_info_t, graph_info_t> Base;

   typedef typename boost::graph_traits<Base>::vertex_descriptor vertex_descriptor;
   typedef typename boost::graph_traits<Base>::edge_descriptor edge_descriptor;

   typedef typename boost::graph_property_type<Base>::type graph_property_type;
   typedef typename boost::lookup_one_property<graph_property_type, boost::graph_bundle_t>::type graph_bundled;

   typedef typename boost::edge_property_type<Base>::type edge_property_type;
   typedef typename boost::lookup_one_property<edge_property_type, boost::edge_bundle_t>::type edge_bundled;

   typedef typename boost::vertex_property_type<Base>::type vertex_property_type;
   typedef typename boost::lookup_one_property<vertex_property_type, boost::vertex_bundle_t>::type vertex_bundled;

   template <typename... Args>
   RawGraph(const graph_bundled& g_info, Args&&... args) : graph_base<Base>(g_info, std::forward<Args>(args)...)
   {
   }

   template <typename graph_bundled_t = graph_bundled, typename... Args>
   RawGraph(Args&&... args) : graph_base<Base>(std::forward<Args>(args)...)
   {
   }

   vertex_descriptor AddVertex(const vertex_bundled& v_info)
   {
      const auto index = boost::num_vertices(*this);
      return boost::add_vertex(vertex_property_type(index, v_info), *this);
   }

   template <typename T = vertex_bundled, std::enable_if_t<std::is_default_constructible<T>::value, bool> = true>
   vertex_descriptor AddVertex()
   {
      return AddVertex(T());
   }

   void RemoveVertex(vertex_descriptor _v)
   {
      boost::remove_vertex(_v, *this);
      auto index_map = boost::get(boost::vertex_index_t(), *this);
      size_t index = 0;
      for(const auto v : boost::make_iterator_range(boost::vertices(*this)))
      {
         index_map[v] = index++;
      }
   }

   edge_descriptor AddEdge(vertex_descriptor src, vertex_descriptor tgt, const edge_bundled& e_info)
   {
      auto [e, inserted] = boost::add_edge(src, tgt, e_info, *this);
      THROW_ASSERT(inserted, "Trying to insert an already existing edge");
      return e;
   }

   template <typename T = edge_bundled, std::enable_if_t<std::is_default_constructible<T>::value, bool> = true>
   edge_descriptor AddEdge(vertex_descriptor src, vertex_descriptor tgt)
   {
      return AddEdge(src, tgt, T());
   }

   void RemoveEdge(edge_descriptor e)
   {
      boost::remove_edge(boost::source(e, *this), boost::target(e, *this), *this);
   }

   void RemoveEdge(vertex_descriptor src, vertex_descriptor tgt)
   {
      auto [e, found] = boost::edge(src, tgt, *this);
      THROW_ASSERT(found, "Edge not found");
      boost::remove_edge(boost::source(e, *this), boost::target(e, *this), *this);
   }
};

/**
 * The property associated with edge
 */
template <class T>
struct EdgeProperty : public T
{
   /// The selector associated with the edge
   int selector;

   /**
    * Constructor with selector and property
    * @param info is the property to be associated with the edge
    * @param _selector is the selector to be associated with the edge
    */
   EdgeProperty(const T& info, const int _selector) : T(info), selector(_selector)
   {
   }

   EdgeProperty(const T& info) : EdgeProperty(info, 0)
   {
   }

   /**
    * Constructor with selector
    * @param _selector is the selector to be associated with the edge
    */
   template <typename Base = T, std::enable_if_t<std::is_default_constructible<Base>::value, bool> = true>
   EdgeProperty(int _selector) : EdgeProperty(Base(), _selector)
   {
   }

   template <typename Base = T, std::enable_if_t<std::is_default_constructible<Base>::value, bool> = true>
   EdgeProperty() : EdgeProperty(Base(), 0)
   {
   }
};

template <typename node_info_t = boost::no_property, typename edge_info_t = boost::no_property,
          typename graph_info_t = boost::no_property>
using graphs_collection_base = boost::adjacency_list<
    gc_vertex_list_t, gc_vertex_list_t, gc_directed,
    boost::property<boost::vertex_index_t, std::size_t,
                    boost::property<boost::vertex_color_t, boost::default_color_type, node_info_t>>,
    EdgeProperty<edge_info_t>, graph_info_t>;

/**
 * bulk graph. All the edge of a graph are store in this object
 */
template <typename node_info_t = boost::no_property, typename edge_info_t = boost::no_property,
          typename graph_info_t = boost::no_property>
struct graphs_collection : public graph_base<graphs_collection_base<node_info_t, edge_info_t, graph_info_t>>

{
   typedef graphs_collection_base<node_info_t, edge_info_t, graph_info_t> Base;

   typedef typename boost::graph_traits<Base>::vertex_descriptor vertex_descriptor;
   typedef typename boost::graph_traits<Base>::edge_descriptor edge_descriptor;

   typedef typename boost::graph_property_type<Base>::type graph_property_type;
   typedef typename boost::lookup_one_property<graph_property_type, boost::graph_bundle_t>::type graph_bundled;

   typedef typename boost::edge_property_type<Base>::type edge_property_type;
   typedef typename boost::lookup_one_property<edge_property_type, boost::edge_bundle_t>::type edge_bundled;

   typedef typename boost::vertex_property_type<Base>::type vertex_property_type;
   typedef typename boost::lookup_one_property<vertex_property_type, boost::vertex_bundle_t>::type vertex_bundled;

   template <typename... Args>
   graphs_collection(Args&&... args) : graph_base<Base>(std::forward<Args>(args)...)
   {
   }

   virtual ~graphs_collection() = default;

   /**
    * Add a selector to an existing edge
    * @param edge is the edge to be considered
    * @param selector is the selector to be added
    */
   edge_descriptor AddSelector(const edge_descriptor& edge, const int selector)
   {
      this->operator[](edge).selector |= selector;
      return edge;
   }

   /**
    * Add a selector to an existing edge
    * @param source is the source of the edge
    * @param target is the target of the edge
    * @param selector is the selector to be added
    */
   edge_descriptor AddSelector(vertex_descriptor source, vertex_descriptor target, int selector)
   {
      const auto [edge, found] = boost::edge(source, target, *this);
      THROW_ASSERT(found, "Edge not found");
      return AddSelector(edge, selector);
   }

   /**
    * Remove an edge from this graph
    * @param edge is the edge to be considered
    * @param selector is the selector to remove
    */
   void RemoveSelector(const edge_descriptor& edge, int selector = -1)
   {
      auto& eselector = this->operator[](edge).selector;
      eselector &= ~selector;
      if(!eselector)
      {
      }
   }

   /**
    * Remove an edge from this graph
    */
   void RemoveSelector(vertex_descriptor source, vertex_descriptor target, int selector)
   {
      const auto [edge, found] = boost::edge(source, target, *this);
      THROW_ASSERT(found, "Edge not found" + std::to_string(selector));
      RemoveSelector(edge, selector);
   }

   /**
    * Return the selectors associated with an edge
    * @param e is the edge
    * @return the associated selector
    */
   int GetSelector(const edge_descriptor& e) const
   {
      return this->operator[](e).selector;
   }

   /**
    * Add a vertex to this graph with a property
    * @param info is the property to be associated with the new vertex
    * @return the added vertex
    */
   virtual vertex_descriptor AddVertex(const vertex_bundled& info)
   {
      const auto index = boost::num_vertices(*this);
      const auto new_v = boost::add_vertex(*this);
      boost::get(boost::vertex_index_t(), *this)[new_v] = index;
      boost::get(boost::vertex_bundle_t(), *this)[new_v] = info;
      return new_v;
   }

   template <typename T = vertex_bundled, std::enable_if_t<std::is_default_constructible<T>::value, bool> = true>
   vertex_descriptor AddVertex()
   {
      return AddVertex(T());
   }

   /**
    * Remove a vertex from this graph
    * @param _v is the vertex to be removed
    */
   virtual void RemoveVertex(vertex_descriptor _v)
   {
      boost::remove_vertex(_v, *this);
      auto index_map = boost::get(boost::vertex_index_t(), *this);
      size_t index = 0;
      for(const auto v : boost::make_iterator_range(boost::vertices(*this)))
      {
         index_map[v] = index++;
      }
   }

   /**
    * Add an edge to this graph
    * @param source is the source of the edge to be added
    * @param target is the target of the edge to be added
    * @param selector is the selector to be set on the edge
    */
   template <typename T = edge_bundled, std::enable_if_t<std::is_default_constructible<T>::value, bool> = true>
   edge_descriptor AddEdge(vertex_descriptor source, vertex_descriptor target, int selector)
   {
      if(!ExistsEdge(source, target))
      {
         boost::add_edge(source, target, EdgeProperty<T>(selector), *this);
      }
      return AddSelector(source, target, selector);
   }

   /**
    * Add an edge to this graph
    * @param source is the source of the edge to be added
    * @param target is the target of the edge to be added
    * @param selector is the selector to be set on the edge
    * @param info is the info to be associated with the edge
    */
   edge_descriptor AddEdge(vertex_descriptor source, vertex_descriptor target, int selector, const edge_bundled& info)
   {
      THROW_ASSERT(!ExistsEdge(source, target), "Trying to add an already existing edge");
      boost::add_edge(source, target, EdgeProperty<edge_bundled>(info, selector), *this);
      AddSelector(source, target, selector);
      const auto [edge, found] = boost::edge(source, target, *this);
      return edge;
   }

   /**
    * Check if an edge exists
    * @param source is the source vertex
    * @param target is the target vertex
    * @return true if source-target exists
    */
   bool ExistsEdge(vertex_descriptor source, vertex_descriptor target) const
   {
      const auto [edge, found] = boost::edge(source, target, *this);
      return found;
   }

   void CompressEdges()
   {
      std::deque<edge_descriptor> toBeRemoved;
      for(const auto& e : boost::make_iterator_range(boost::edges(*this)))
      {
         if(this->operator[](e).selector == 0)
         {
            toBeRemoved.push_back(e);
         }
      }
      for(const auto& e : toBeRemoved)
      {
         boost::remove_edge(e, *this);
      }
   }
};

/**
 * Predicate functor object used to select the proper set of vertexes
 */
template <typename Graph>
struct SelectVertex
{
 private:
   /// The set of vertices to be considered
   CustomUnorderedSet<typename Graph::vertex_descriptor> subset;

 public:
   SelectVertex()
   {
   }

   /**
    * Constructor
    * @param _subset is the set of vertices to be considered
    */
   SelectVertex(const CustomUnorderedSet<typename Graph::vertex_descriptor>& _subset) : subset(_subset)
   {
   }

   /**
    * Operator to check if a vertex has to be selected
    * @param v is the vertex to be analyzed
    * @return true if the vertex has to be selected
    */
   bool operator()(typename Graph::vertex_descriptor v) const
   {
      return subset.empty() || (subset.find(v) != subset.end());
   }
};

/**
 * Predicate functor object used to select the proper set of edges
 */
template <typename Graph>
struct SelectEdge
{
 private:
   /// The bulk graph
   const Graph* g;

   /// The selector associated with the filtered graph
   int selector;

   /// The vertices of subgraph
   CustomUnorderedSet<typename Graph::vertex_descriptor> subset;

 public:
   SelectEdge() : g(nullptr), selector(0)
   {
   }

   /**
    * Constructor for filtering also on vertices
    * @param _g is the graph
    * @param _selector is the selector of the filtered graph
    * @param _subset is the set of vertices of the filtered graph
    */
   SelectEdge(const Graph* _g, int _selector = 0,
              const CustomUnorderedSet<typename Graph::vertex_descriptor>& _subset = {})
       : g(_g), selector(_selector), subset(_subset)
   {
   }

   bool operator()(const typename Graph::edge_descriptor& e) const
   {
      if(subset.empty())
      {
         return selector & g->operator[](e).selector;
      }

      const auto u = boost::source(e, *g);
      const auto v = boost::target(e, *g);
      if((selector & g->GetSelector(e)) && subset.find(v) != subset.end() && subset.find(u) != subset.end())
      {
         return true;
      }
      return false;
   }
};

/**
 * General class used to describe a graph in PandA.
 */
template <class GraphsCollection>
struct graph : public graph_base<boost::filtered_graph<typename GraphsCollection::Base, SelectEdge<GraphsCollection>,
                                                       SelectVertex<GraphsCollection>>>
{
   typedef boost::filtered_graph<typename GraphsCollection::Base, SelectEdge<GraphsCollection>,
                                 SelectVertex<GraphsCollection>>
       Base;

   typedef typename boost::graph_traits<Base>::vertex_descriptor vertex_descriptor;
   typedef typename boost::graph_traits<Base>::edge_descriptor edge_descriptor;

   typedef typename boost::graph_property_type<Base>::type graph_property_type;
   typedef typename boost::lookup_one_property<graph_property_type, boost::graph_bundle_t>::type graph_bundled;

   typedef typename boost::edge_property_type<Base>::type edge_property_type;
   typedef typename boost::lookup_one_property<edge_property_type, boost::edge_bundle_t>::type edge_bundled;

   typedef typename boost::vertex_property_type<Base>::type vertex_property_type;
   typedef typename boost::lookup_one_property<vertex_property_type, boost::vertex_bundle_t>::type vertex_bundled;

   /**
    * Sub-graph constructor.
    * @param g is the bulk graph.
    * @param selector is the selector used to filter the bulk graph.
    * @param vertices is the set of vertexes on which the graph is filtered.
    */
   graph(const GraphsCollection& g, const int selector, const CustomUnorderedSet<vertex_descriptor>& vertices = {})
       : graph_base<Base>(g, SelectEdge<GraphsCollection>(&g, selector, vertices),
                          SelectVertex<GraphsCollection>(vertices)),
         m_collection(g),
         m_selector(selector)
   {
   }

   const GraphsCollection& GetGraphsCollection() const
   {
      return m_collection;
   }

   /**
    * return true in case the vertex is a vertex of the subgraph.
    */
   bool is_in_subset(vertex_descriptor v) const
   {
      return m_vertex_pred(v);
   }

   /**
    * Return the selector of this graph
    * @return the selector of the graph
    */
   int GetSelector() const
   {
      return m_selector;
   }

   /**
    * Return the selectors associated with an edge
    * @param e is the edge
    * @return the associated selector
    */
   int GetSelector(const edge_descriptor& e) const
   {
      return m_collection.GetSelector(e) & GetSelector();
   }

   /**
    * Return the selectors associated with an edge
    * @param source is the source of an edge
    * @param target is the target of an edge
    * @return the associated selector
    */
   int GetSelector(vertex_descriptor source, vertex_descriptor target) const
   {
      const auto [edge, found] = boost::edge(source, target, *this);
      return GetSelector(edge);
   }

 protected:
   const GraphsCollection& m_collection;
   const int m_selector;
};

/**
 * Functor used to sort edges
 */
template <typename Graph>
struct ltedge
{
 private:
   /// the graph
   const Graph* g;

 public:
   ltedge() : g(nullptr)
   {
   }

   ltedge(const Graph* _g) : g(_g)
   {
   }

   /**
    * Redefinition of binary operator as less than
    */
   bool operator()(const typename boost::graph_traits<Graph>::edge_descriptor& first,
                   const typename boost::graph_traits<Graph>::edge_descriptor& second) const
   {
      if(boost::source(first, *g) < boost::source(second, *g))
      {
         return true;
      }
      if(boost::source(first, *g) > boost::source(second, *g))
      {
         return false;
      }
      return boost::target(first, *g) < boost::target(second, *g);
   }
};

/**
 * Functor used to write the content of a vertex to dotty file
 */
template <typename Graph>
class VertexWriter
{
 protected:
   /// The graph to be printed
   const Graph& printing_graph;

   /// The detail level
   const int detail_level;

 public:
   /**
    * Constructor
    * @param _graph is the graph to be printed
    * @param _detail_level is the level of details in printing
    */
   VertexWriter(const Graph& _graph, const int _detail_level = 0) : printing_graph(_graph), detail_level(_detail_level)
   {
   }

   virtual ~VertexWriter() = default;

   /**
    * Functor actually called by the boost library to perform the writing
    * @param out is the stream where the nodes have to be printed
    * @param v is the vertex to be printed
    */
   virtual void operator()(std::ostream& out, typename boost::graph_traits<Graph>::vertex_descriptor v) const = 0;
};

/**
 * Functor used to write the content of the edges to a dotty file
 */
template <typename Graph>
class EdgeWriter
{
 protected:
   /// The graph to be printed
   const Graph& printing_graph;

   /// The selector of the graph to be printed
   const int selector;

   /// The detail level
   const int detail_level;

 public:
   /**
    * Constructor
    * @param _printing_graph is the graph to be printed
    * @param _detail_level if 1, the state of the graph is printed
    */
   EdgeWriter(const Graph& _printing_graph, const int _detail_level = 0)
       : printing_graph(_printing_graph), selector(printing_graph.GetSelector()), detail_level(_detail_level)
   {
   }

   virtual ~EdgeWriter() = default;

   /**
    * Functor actually called by the boost library to perform the writing
    * @param out is the stream where the edges have to be printed
    * @param edge is the edge to be printed
    */
   virtual void operator()(std::ostream& out,
                           const typename boost::graph_traits<Graph>::edge_descriptor& edge) const = 0;
};

/**
 * Functor used to write the content of the property of a graph to a dotty file
 */
template <typename Graph>
class GraphWriter
{
 protected:
   /// The graph to be printed
   const Graph& printing_graph;

   /// The detail level (i.e., how much information has to be included)
   const int detail_level;

 public:
   /**
    * Constructor
    * @param _printing_graph is the graph to be printed
    * @param _detail_level is the detail level of the printing
    */
   GraphWriter(const Graph& _printing_graph, const int _detail_level = 0)
       : printing_graph(_printing_graph), detail_level(_detail_level)
   {
   }

   virtual ~GraphWriter() = default;

   /**
    * Functor acturally called by the boost library to perform the writing
    * @param out is the stream where the edges have to be printed
    */
   virtual void operator()(std::ostream& out) const = 0;
};

#endif
