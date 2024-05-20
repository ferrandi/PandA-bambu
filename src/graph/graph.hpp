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
#include "edge_info.hpp"
#include "exceptions.hpp"
#include "graph_info.hpp"
#include "node_info.hpp"
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

/**
 * @name forward declarations
 */
//@{
CONSTREF_FORWARD_DECL(EdgeInfo);
CONSTREF_FORWARD_DECL(EdgeWriter);
CONSTREF_FORWARD_DECL(GraphWriter);
CONSTREF_FORWARD_DECL(NodeInfo);
REF_FORWARD_DECL(NodeInfo);
CONSTREF_FORWARD_DECL(Parameter);
CONSTREF_FORWARD_DECL(UEdgeWriter);
CONSTREF_FORWARD_DECL(UVertexWriter);
CONSTREF_FORWARD_DECL(VertexWriter);
//@}

template <typename T>
struct is_shared_ptr : std::false_type
{
   typedef void ptd_type;
};

template <typename T>
struct is_shared_ptr<std::shared_ptr<T>> : std::true_type
{
   typedef T ptd_type;
};

template <typename T>
using get_shared_ptr_t = typename is_shared_ptr<T>::ptd_type;

template <typename T>
struct add_const_ref
{
   typedef std::add_lvalue_reference_t<std::add_const_t<T>> type;
};

template <typename T>
struct add_const_ref<std::shared_ptr<T>>
{
   typedef std::shared_ptr<std::add_const_t<T>> type;
};

template <typename T>
using add_const_ref_t = typename add_const_ref<T>::type;

template <class Graph>
struct graph_base : public Graph
{
   using vertex_descriptor = typename boost::graph_traits<Graph>::vertex_descriptor;
   using vertex_property = typename boost::vertex_property_type<Graph>::type;
   using edge_descriptor = typename boost::graph_traits<Graph>::edge_descriptor;
   using edge_property = typename boost::edge_property_type<Graph>::type;
   using graph_property = typename boost::graph_property_type<Graph>::type;

   template <typename... Args>
   explicit graph_base(const graph_property& g_info, Args&&... args) : Graph(std::forward<Args>(args)...)
   {
      (*this)[boost::graph_bundle] = g_info;
   }

   template <typename graph_property_t = graph_property,
             std::enable_if_t<std::is_default_constructible<graph_property_t>::value &&
                                  !is_shared_ptr<graph_property_t>::value,
                              bool> = true,
             typename... Args>
   explicit graph_base(Args&&... args) : Graph(std::forward<Args>(args)...)
   {
   }

   template <typename graph_property_t = graph_property,
             std::enable_if_t<is_shared_ptr<graph_property_t>::value &&
                                  std::is_default_constructible<get_shared_ptr_t<graph_property_t>>::value,
                              bool> = true,
             typename... Args>
   explicit graph_base(Args&&... args) : Graph(std::forward<Args>(args)...)
   {
      (*this)[boost::graph_bundle] = graph_property_t(new get_shared_ptr_t<graph_property_t>);
   }

   ~graph_base() = default;

   template <typename vertex_property_t = vertex_property>
   std::enable_if_t<!is_shared_ptr<vertex_property_t>::value, vertex_descriptor>
   AddVertex(const vertex_property_t& v_info)
   {
      return boost::add_vertex(v_info, *this);
   }

   template <typename vertex_property_t = vertex_property>
   std::enable_if_t<is_shared_ptr<vertex_property_t>::value, vertex_descriptor>
   AddVertex(const vertex_property_t& v_info)
   {
      THROW_ASSERT(v_info, "Vertex without associated info.");
      return boost::add_vertex(v_info, *this);
   }

   template <typename vertex_property_t = vertex_property>
   std::enable_if_t<!is_shared_ptr<vertex_property_t>::value && std::is_default_constructible<vertex_property_t>::value,
                    vertex_descriptor>
   AddVertex()
   {
      return AddVertex(vertex_property_t());
   }

   template <typename vertex_property_t = vertex_property>
   std::enable_if_t<is_shared_ptr<vertex_property_t>::value &&
                        std::is_default_constructible<get_shared_ptr_t<vertex_property_t>>::value,
                    vertex_descriptor>
   AddVertex()
   {
      return AddVertex(vertex_property(new get_shared_ptr_t<vertex_property_t>));
   }

   void RemoveVertex(vertex_descriptor v)
   {
      boost::remove_vertex(v, *this);
   }

   template <typename edge_property_t = edge_property>
   std::enable_if_t<!is_shared_ptr<edge_property_t>::value, edge_descriptor>
   AddEdge(vertex_descriptor src, vertex_descriptor tgt, const edge_property& e_info)
   {
      auto [e, inserted] = boost::add_edge(src, tgt, e_info, *this);
      THROW_ASSERT(inserted, "Trying to insert an already existing edge");
      return e;
   }

   template <typename edge_property_t = edge_property>
   std::enable_if_t<is_shared_ptr<edge_property_t>::value, edge_descriptor>
   AddEdge(vertex_descriptor src, vertex_descriptor tgt, const edge_property_t& e_info)
   {
      THROW_ASSERT(e_info, "Edge without associated info.");
      auto [e, inserted] = boost::add_edge(src, tgt, e_info, *this);
      THROW_ASSERT(inserted, "Trying to insert an already existing edge");
      return e;
   }

   template <typename edge_property_t = edge_property>
   std::enable_if_t<!is_shared_ptr<edge_property_t>::value && std::is_default_constructible<edge_property_t>::value,
                    edge_descriptor>
   AddEdge(vertex_descriptor src, vertex_descriptor tgt)
   {
      return AddEdge(src, tgt, edge_property_t());
   }

   template <typename edge_property_t = edge_property>
   std::enable_if_t<is_shared_ptr<edge_property_t>::value &&
                        std::is_default_constructible<get_shared_ptr_t<edge_property_t>>::value,
                    edge_descriptor>
   AddEdge(vertex_descriptor src, vertex_descriptor tgt)
   {
      return AddEdge(src, tgt, edge_property_t(new get_shared_ptr_t<edge_property_t>));
   }

   inline void RemoveEdge(edge_descriptor e)
   {
      boost::remove_edge(boost::source(e, *this), boost::target(e, *this), *this);
   }

   inline void RemoveEdge(vertex_descriptor src, vertex_descriptor tgt)
   {
      auto [e, found] = boost::edge(src, tgt, *this);
      THROW_ASSERT(found, "Edge not found");
      boost::remove_edge(boost::source(e, *this), boost::target(e, *this), *this);
   }

   inline bool ExistsEdge(const vertex_descriptor src, const vertex_descriptor tgt) const
   {
      return boost::edge(src, tgt, *this).second;
   }

   inline edge_descriptor CGetEdge(const vertex_descriptor src, const vertex_descriptor tgt) const
   {
      auto [e, found] = boost::edge(src, tgt, *this);
      THROW_ASSERT(found, "Edge does not exist in this graph");
      return e;
   }

   template <typename vertex_property_t = vertex_property>
   std::enable_if_t<!std::is_empty<vertex_property_t>::value, vertex_property_t>& GetNodeInfo(vertex_descriptor node)
   {
      return (*this)[node];
   }

   template <typename vertex_property_t = vertex_property>
   std::enable_if_t<!std::is_empty<vertex_property_t>::value, add_const_ref_t<vertex_property_t>>
   CGetNodeInfo(vertex_descriptor node) const
   {
      return (*this)[node];
   }

   template <typename edge_property_t = edge_property>
   std::enable_if_t<!std::is_empty<edge_property_t>::value, edge_property_t>& GetEdgeInfo(edge_descriptor edge)
   {
      return (*this)[edge];
   }

   template <typename edge_property_t = edge_property>
   std::enable_if_t<!std::is_empty<edge_property_t>::value, add_const_ref_t<edge_property_t>>
   CGetEdgeInfo(edge_descriptor edge) const
   {
      return (*this)[edge];
   }

   template <typename graph_property_t = graph_property>
   std::enable_if_t<!std::is_empty<graph_property_t>::value, graph_property_t>& GetGraphInfo()
   {
      return (*this)[boost::graph_bundle];
   }

   template <typename graph_property_t = graph_property>
   std::enable_if_t<!std::is_empty<graph_property_t>::value, add_const_ref_t<graph_property_t>> CGetGraphInfo() const
   {
      return (*this)[boost::graph_bundle];
   }

   bool IsReachable(const vertex_descriptor x, const vertex_descriptor y) const
   {
      std::list<vertex_descriptor> running_vertices;
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
      const auto sccs_count = boost::strong_components(*this, vtoc);
      sccs.resize(sccs_count);
      for(const auto v : boost::make_iterator_range(boost::vertices(*this)))
      {
         sccs.at(vtoc[v]).push_back(v);
      }
      return sccs;
   }

   template <typename VertexPropertiesWriter, typename EdgePropertiesWriter, typename GraphPropertiesWriter>
   inline void WriteDot(const std::filesystem::path& filename, VertexPropertiesWriter vpw, EdgePropertiesWriter epw,
                        GraphPropertiesWriter gpw) const
   {
      std::ofstream fs(filename);
      boost::write_graphviz(fs, *this, vpw, epw, gpw);
   }

   template <typename VertexPropertiesWriter, typename EdgePropertiesWriter>
   inline void WriteDot(const std::filesystem::path& filename, VertexPropertiesWriter vpw,
                        EdgePropertiesWriter epw) const
   {
      boost::default_writer gpw;
      WriteDot(filename, vpw, epw, gpw);
   }

   template <typename VertexPropertiesWriter>
   inline void WriteDot(const std::filesystem::path& filename, VertexPropertiesWriter vpw) const
   {
      boost::default_writer epw;
      boost::default_writer gpw;
      WriteDot(filename, vpw, epw, gpw);
   }
};

struct RawGraph : public boost::adjacency_list<boost::listS, boost::listS, boost::bidirectionalS,
                                               boost::property<boost::vertex_index_t, std::size_t, NodeInfoRef>,
                                               EdgeInfoRef, GraphInfoRef>
{
 public:
   /**
    * Constructor
    * @param g_info is the property associated with the graph
    */
   explicit RawGraph(GraphInfoRef g_info)
   {
      (*this)[boost::graph_bundle] = g_info;
   }

   inline boost::graph_traits<RawGraph>::vertex_descriptor AddVertex(const NodeInfoRef v_info)
   {
      size_t index = boost::num_vertices(*this);
      auto new_v = boost::add_vertex(*this);
      (*this)[new_v] = v_info;
      boost::get(boost::vertex_index_t(), *this)[new_v] = index;
      return new_v;
   };

   inline void RemoveVertex(boost::graph_traits<RawGraph>::vertex_descriptor v)
   {
      boost::remove_vertex(v, *this);
      auto index_map = boost::get(boost::vertex_index_t(), *this);
      boost::graph_traits<RawGraph>::vertex_iterator v_it, v_it_end;
      size_t index = 0;
      for(boost::tie(v_it, v_it_end) = boost::vertices(*this); v_it != v_it_end; v_it++, index++)
      {
         index_map[*v_it] = index;
      }
   }

   inline boost::graph_traits<RawGraph>::edge_descriptor AddEdge(boost::graph_traits<RawGraph>::vertex_descriptor src,
                                                                 boost::graph_traits<RawGraph>::vertex_descriptor tgt,
                                                                 const EdgeInfoRef e_info)
   {
      boost::graph_traits<RawGraph>::edge_descriptor e;
      bool found;
      boost::tie(e, found) = boost::edge(src, tgt, *this);
      THROW_ASSERT(not found, "Trying to insert an already existing edge");
      boost::tie(e, found) = boost::add_edge(src, tgt, e_info, *this);
      (*this)[e] = e_info;
      return e;
   }

   inline void RemoveEdge(boost::graph_traits<RawGraph>::edge_descriptor e)
   {
      boost::remove_edge(boost::source(e, *this), boost::target(e, *this), *this);
   }

   inline void RemoveEdge(boost::graph_traits<RawGraph>::vertex_descriptor src,
                          boost::graph_traits<RawGraph>::vertex_descriptor tgt)
   {
      boost::graph_traits<RawGraph>::edge_descriptor e;
      bool found;
      boost::tie(e, found) = boost::edge(src, tgt, *this);
      THROW_ASSERT(found, "Edge not found");
      boost::remove_edge(boost::source(e, *this), boost::target(e, *this), *this);
   }
};

/**
 * Function returning the edge information associated with the specified edge.
 * This function modifies the graph if EdgeInfo is not yet associated with the edge.
 * @param e is the considered edge.
 * @param g is the graph.
 * @return the EdgeInfo object associated with e.
 */
template <class info_object, class Graph>
info_object* get_raw_edge_info(typename boost::graph_traits<Graph>::edge_descriptor e, Graph& g)
{
   /// Note: reference has to be used since we want to modify the info associated with the edge
   EdgeInfoRef& info = g[e];
   if(!info)
   {
      info = EdgeInfoRef(new info_object);
   }
   THROW_ASSERT(GetPointer<info_object>(info) != nullptr,
                "Function get_raw_edge_info: the edges associated with the graph used are not derived from "
                "info_object\n\tCheck the actual type of info_object and the type of the edge of Graph");
   return GetPointerS<info_object>(info);
}
/**
 * Function returning the edge information associated with the specified edge.
 * This function does not modify the graph.
 * @param e is the considered edge.
 * @param g is the graph.
 * @return the edge_info object associated with e.
 */
template <class info_object, class Graph>
const info_object* Cget_raw_edge_info(typename boost::graph_traits<Graph>::edge_descriptor e, const Graph& g)
{
   const EdgeInfo* info = g[e].get();
   THROW_ASSERT(!info || dynamic_cast<const info_object*>(info) != nullptr,
                "Function Cget_raw_edge_info: the edges associated with the graph used are not derived from "
                "info_object\n\tCheck the actual type of info_object and the type of the edge of Graph");
   return info ? dynamic_cast<const info_object*>(info) : nullptr;
}

#define GET_RAW_EDGE_INFO(data, edge_info, edge_index) get_edge_info<edge_info>(edge_index, *(data))

#define CGET_RAW_EDGE_INFO(data, edge_info, edge_index) Cget_edge_info<edge_info>(edge_index, *(data))

/**
 * The property associated with edge
 */
struct EdgeProperty
{
 public:
   /// The selector associated with the edge
   int selector;

   /// The info associated with an edge
   EdgeInfoRef info;

   /**
    * Default constructor
    */
   EdgeProperty() : selector(0)
   {
   }

   /**
    * Constructor with selector
    * @param _selector is the selector to be associated with the edge
    */
   explicit EdgeProperty(int _selector) : selector(_selector)
   {
   }

   /**
    * Constructor with selector and property
    * @param _info is the property to be associated with the edge
    */
   EdgeProperty(const int _selector, EdgeInfoRef _info) : selector(_selector), info(_info)
   {
   }
};
#if BOOST_VERSION >= 104600
using boost_graphs_collection = boost::adjacency_list<
    boost::listS, boost::listS, boost::bidirectionalS,
    boost::property<boost::vertex_index_t, std::size_t,
                    boost::property<boost::vertex_color_t, boost::default_color_type, NodeInfoRef>>,
    EdgeProperty, GraphInfoRef>;

using undirected_boost_graphs_collection = boost::adjacency_list<
    boost::listS, boost::listS, boost::undirectedS,
    boost::property<boost::vertex_index_t, std::size_t,
                    boost::property<boost::vertex_color_t, boost::default_color_type, NodeInfoRef>>,
    EdgeProperty, GraphInfoRef>;

#define boost_CGetOpGraph_property(graph_arg) (graph_arg)[boost::graph_bundle]

#else

/**
 * Custom graph property: GraphInfo.
 * This property provide a refcount pointer to the GraphInfo object. This object store all the information associatet
 * with the whole graph.
 */
struct graph_info_t
{
   /// typedef defining graph_info_t as graph object
   typedef boost::graph_property_tag kind;
};

typedef boost::property<graph_info_t, GraphInfoRef> GraphProperty;

typedef boost::adjacency_list<
    boost::listS, boost::listS, boost::bidirectionalS,
    boost::property<boost::vertex_index_t, std::size_t,
                    boost::property<boost::vertex_color_t, boost::default_color_type, NodeInfoRef>>,
    EdgeProperty, GraphProperty>
    boost_graphs_collection;

typedef boost::adjacency_list<
    boost::listS, boost::listS, boost::undirectedS,
    boost::property<boost::vertex_index_t, std::size_t,
                    boost::property<boost::vertex_color_t, boost::default_color_type, NodeInfoRef>>,
    EdgeProperty, GraphProperty>
    undirected_boost_graphs_collection;

#define boost_CGetOpGraph_property(graph_arg) boost::get_property(graph_arg, graph_info_t())

#endif

/**
 * bulk graph. All the edge of a graph are store in this object
 */
struct graphs_collection : public boost_graphs_collection
{
 public:
   /// Set of input parameters
   const ParameterConstRef parameters;

   /**
    * Constructor of graph.
    * @param info is the property associated with the graph
    */
   graphs_collection(GraphInfoRef info, const ParameterConstRef _parameters) : parameters(_parameters)
   {
      boost_CGetOpGraph_property(*this) = info;
   }

   /// Destructor
   virtual ~graphs_collection() = default;

   /**
    * Add a selector to an existing edge
    * @param edge is the edge to be considered
    * @param selector is the selector to be added
    */
   inline boost::graph_traits<graphs_collection>::edge_descriptor
   AddSelector(const boost::graph_traits<graphs_collection>::edge_descriptor edge, const int selector)
   {
      (*this)[edge].selector |= selector;
      return edge;
   }

   /**
    * Add a selector to an existing edge
    * @param source is the source of the edge
    * @param target is the target of the edge
    */
   inline boost::graph_traits<graphs_collection>::edge_descriptor
   AddSelector(const boost::graph_traits<graphs_collection>::vertex_descriptor source,
               const boost::graph_traits<graphs_collection>::vertex_descriptor target, const int selector)
   {
      boost::graph_traits<graphs_collection>::edge_descriptor edge;
      bool inserted;
      boost::tie(edge, inserted) = boost::edge(source, target, *this);
      THROW_ASSERT(inserted, "Edge not found");
      AddSelector(edge, selector);
      return edge;
   }

   /**
    * Remove all the selectors of an edge from this graph
    * @param edge is the edge to be considered
    */
   inline void RemoveSelector(boost::graph_traits<graphs_collection>::edge_descriptor edge)
   {
      (*this)[edge].selector = 0;
   }

   /**
    * Remove an edge from this graph
    * @param edge is the edge to be considered
    * @param selector is the selector to remove
    */
   inline void RemoveSelector(boost::graph_traits<graphs_collection>::edge_descriptor edge, const int selector)
   {
      (*this)[edge].selector = (*this)[edge].selector & ~selector;
   }

   /**
    * Remove an edge from this graph
    */
   inline void RemoveSelector(boost::graph_traits<graphs_collection>::vertex_descriptor source,
                              boost::graph_traits<graphs_collection>::vertex_descriptor target, const int selector)
   {
      boost::graph_traits<graphs_collection>::edge_descriptor edge;
      bool inserted;
      boost::tie(edge, inserted) = boost::edge(source, target, *this);
      THROW_ASSERT(inserted, "Edge not found");
      RemoveSelector(edge, selector);
   }

   /**
    * Return the selectors associated with an edge
    * @param e is the edge
    * @return the associated selector
    */
   inline int GetSelector(const edge_descriptor e) const
   {
      return (*this)[e].selector;
   }

   /**
    * Add a vertex to this graph with a property
    * @param info is the property to be associated with the new vertex
    * @return the added vertex
    */
   virtual boost::graph_traits<boost_graphs_collection>::vertex_descriptor AddVertex(const NodeInfoRef info);

   /**
    * Add an edge to this graph
    * FIXME: this should be pure virtual
    * @param source is the source of the edge to be added
    * @param target is the target of the edge to be added
    * @param selector is the selector to be set on the edge
    * @param info is the info to be associated with the edge
    */
   inline boost::graph_traits<graphs_collection>::edge_descriptor
   AddEdge(boost::graph_traits<graphs_collection>::vertex_descriptor,
           boost::graph_traits<graphs_collection>::vertex_descriptor, const int)
   {
      THROW_UNREACHABLE("This function should be overriden by derived classes");
      return boost::graph_traits<graphs_collection>::edge_descriptor();
   }

   /**
    * Add an edge to this graph
    * FIXME: this should be protected
    * @param souce is the source of the edge to be added
    * @param target is the target of the edge to be added
    * @param selector is the selector to be set on the edge
    * @param info is the info to be associated with the edge
    */
   inline boost::graph_traits<graphs_collection>::edge_descriptor
   InternalAddEdge(boost::graph_traits<graphs_collection>::vertex_descriptor source,
                   boost::graph_traits<graphs_collection>::vertex_descriptor target, const int selector,
                   const EdgeInfoRef info)
   {
      boost::graph_traits<graphs_collection>::edge_descriptor edge;
      bool inserted;
      boost::tie(edge, inserted) = boost::edge(source, target, *this);
      THROW_ASSERT(not inserted, "Trying to add an already existing edge");
      boost::add_edge(source, target, EdgeProperty(selector), *this);
      boost::tie(edge, inserted) = boost::edge(source, target, *this);
      AddSelector(source, target, selector);
      (*this)[edge].info = info;
      return edge;
   }

   /**
    * Remove a vertex from this graph
    * @param v is the vertex to be removed
    */
   virtual void RemoveVertex(boost::graph_traits<boost_graphs_collection>::vertex_descriptor v);

   /**
    * Check if an edge exists
    * @param source is the source vertex
    * @param target is the target vertex
    * @return true if source-target exists
    */
   inline bool ExistsEdge(const boost::graph_traits<graphs_collection>::vertex_descriptor source,
                          const boost::graph_traits<graphs_collection>::vertex_descriptor target) const
   {
      boost::graph_traits<graphs_collection>::edge_descriptor edge;
      bool inserted;
      boost::tie(edge, inserted) = boost::edge(source, target, *this);
      return inserted;
   }

   inline void CompressEdges()
   {
      std::deque<boost::graph_traits<graphs_collection>::edge_descriptor> toBeRemoved;
      boost::graph_traits<graphs_collection>::edge_iterator ei0, ei0_end;
      for(boost::tie(ei0, ei0_end) = boost::edges(*this); ei0 != ei0_end; ++ei0)
      {
         if((*this)[*ei0].selector == 0)
         {
            toBeRemoved.push_back(*ei0);
         }
      }
      for(auto e0 : toBeRemoved)
      {
         boost::remove_edge(e0, *this);
      }
   }
};

using graphs_collectionRef = refcount<graphs_collection>;

/**
 * bulk graph. All the edge of a graph are store in this object
 */
struct undirected_graphs_collection : public undirected_boost_graphs_collection
{
   /// Constructor of graph.
   undirected_graphs_collection() = default;

   /// Destructor
   ~undirected_graphs_collection() = default;

   /**
    * Add a vertex to this graph
    * @return the added vertex
    */
   inline boost::graph_traits<undirected_boost_graphs_collection>::vertex_descriptor AddVertex()
   {
      size_t index = boost::num_vertices(*this);
      boost::graph_traits<undirected_boost_graphs_collection>::vertex_descriptor v = boost::add_vertex(*this);
      boost::get(boost::vertex_index_t(), *this)[v] = index;
      return v;
   }

   /**
    * Remove a vertex from this graph
    * @param v is the vertex to be removed
    */
   inline void RemoveVertex(boost::graph_traits<undirected_boost_graphs_collection>::vertex_descriptor v)
   {
      boost::remove_vertex(v, *this);
      boost::property_map<undirected_boost_graphs_collection, boost::vertex_index_t>::type index_map =
          boost::get(boost::vertex_index_t(), *this);
      boost::graph_traits<undirected_boost_graphs_collection>::vertex_iterator v_it, v_it_end;
      size_t index = 0;
      for(boost::tie(v_it, v_it_end) = boost::vertices(*this); v_it != v_it_end; v_it++, index++)
      {
         index_map[*v_it] = index;
      }
   }

   /**
    * Return the selectors associated with an edge
    * @param e is the edge
    * @return the associated selector
    */
   inline int GetSelector(const edge_descriptor e) const
   {
      return (*this)[e].selector;
   }

   /**
    * Set the selector associated with an edge
    * @param e is the edge
    * #param selector is the new selector
    */
   inline void SetSelector(edge_descriptor e, const int selector)
   {
      (*this)[e].selector = selector;
   }
};

using undirected_graphs_collectionRef = refcount<undirected_graphs_collection>;

/**
 * Function returning the reference to the edge information associated with the specified edge.
 * This function modifies the graph if EdgeInfo is not yet associated with the edge.
 * @param e is the considered edge.
 * @param g is the graph.
 * @return the reference to the EdgeInfo object associated with e.
 */
template <class info_object, class Graph>
NodeInfoRef& Cget_node_infoRef(typename boost::graph_traits<Graph>::vertex_descriptor v, Graph& g)
{
   NodeInfoRef& info = g[v];
   if(!info)
   {
      info = NodeInfoRef(new info_object);
   }
   return info;
}

/**
 * Function returning the node information associated with the specified node.
 * This function modifies the graph if NodeInfo is not yet associated with the node.
 * @param v is the considered node.
 * @param g is the graph.
 * @return the NodeInfo object associated with v.
 */
template <class info_object, class Graph>
void set_node_info(typename boost::graph_traits<Graph>::vertex_descriptor v, const refcount<info_object>& obj, Graph& g)
{
   NodeInfoRef& info = g[v];
   info = obj;
}

#define SET_NODE_INFO_REF(data, NodeInfo, obj, vertex_index) set_node_info<NodeInfo>(vertex_index, obj, *(data))

#define GET_NODE_INFO_REF(data, NodeInfo, vertex_index) Cget_node_infoRef<NodeInfo>(vertex_index, *(data))

/**
 * Function returning the node information associated with the specified node.
 * This function modifies the graph if NodeInfo is not yet associated with the node.
 * @param v is the considered node.
 * @param g is the graph.
 * @return the NodeInfo object associated with v.
 */
template <class info_object, class Graph>
info_object* get_node_info(typename boost::graph_traits<Graph>::vertex_descriptor v, Graph& g)
{
   NodeInfoRef& info = g[v];
   if(!info)
   {
      info = NodeInfoRef(new info_object);
   }
   THROW_ASSERT(GetPointer<info_object>(info) != nullptr,
                "Function get_node_info: the vertices associated with the graph used are not derived from "
                "info_object\n\tCheck the actual type of info_object and the type of the node of Graph");
   return GetPointerS<info_object>(info);
}

/**
 * Function returning the node information associated with the specified node.
 * This function does not modify the graph.
 * @param v is the considered node.
 * @param g is the graph.
 * @return the node_info object associated with v.
 */
template <class info_object, class Graph>
const info_object* Cget_node_info(typename boost::graph_traits<Graph>::vertex_descriptor v, const Graph& g)
{
   const NodeInfo* info = g[v].get();
   THROW_ASSERT(!info || dynamic_cast<const info_object*>(info) != nullptr,
                "Function Cget_node_info: the nodes associated with the graph used are not derived from "
                "info_object\n\tCheck the actual type of info_object and the type of the node of Graph");
   return info ? static_cast<const info_object*>(info) : nullptr;
}

#define GET_NODE_INFO(data, NodeInfo, vertex_index) get_node_info<NodeInfo>(vertex_index, *(data))

#define CGET_NODE_INFO(data, NodeInfo, vertex_index) Cget_node_info<NodeInfo>(vertex_index, *(data))

/**
 * Function returning the edge information associated with the specified edge.
 * This function modifies the graph if EdgeInfo is not yet associated with the edge.
 * @param e is the considered edge.
 * @param g is the graph.
 * @return the EdgeInfo object associated with e.
 */
template <class info_object, class Graph>
info_object* get_edge_info(typename boost::graph_traits<Graph>::edge_descriptor e, Graph& g)
{
   /// Note: reference has to be used since we want to modify the info associated with the edge
   EdgeInfoRef& info = g[e].info;
   if(!info)
   {
      info = EdgeInfoRef(new info_object);
   }
   THROW_ASSERT(GetPointer<info_object>(info) != nullptr,
                "Function get_edge_info: the edges associated with the graph used are not derived from "
                "info_object\n\tCheck the actual type of info_object and the type of the edge of Graph");
   return GetPointerS<info_object>(info);
}
/**
 * Function returning the edge information associated with the specified edge.
 * This function does not modify the graph.
 * @param v is the considered node.
 * @param g is the graph.
 * @return the edge_info object associated with v.
 */
template <class info_object, class Graph>
const info_object* Cget_edge_info(typename boost::graph_traits<Graph>::edge_descriptor e, const Graph& g)
{
   const EdgeInfo* info = g[e].info.get();
   THROW_ASSERT(!info || dynamic_cast<const info_object*>(info) != nullptr,
                "Function Cget_edge_info: the edges associated with the graph used are not derived from "
                "info_object\n\tCheck the actual type of info_object and the type of the edge of Graph");
   return info ? static_cast<const info_object*>(info) : nullptr;
}

#define GET_EDGE_INFO(data, edge_info, edge_index) get_edge_info<edge_info>(edge_index, *(data))

#define CGET_EDGE_INFO(data, edge_info, edge_index) Cget_edge_info<edge_info>(edge_index, *(data))

/**
 * Predicate functor object used to select the proper set of vertexes
 */
template <typename Graph>
struct SelectVertex
{
 private:
   /// True if all the vertices have to be selected
   bool all;

   /// The set of vertices to be considered
   CustomUnorderedSet<typename boost::graph_traits<Graph>::vertex_descriptor> subset;

 public:
   /**
    * Constructor to select all the vertices
    */
   SelectVertex() : all(true), subset()
   {
   }

   /**
    * Constructor
    * @param _subset is the set of vertices to be considered
    */
   explicit SelectVertex(const CustomUnorderedSet<typename boost::graph_traits<Graph>::vertex_descriptor>& _subset)
       : all(false), subset(_subset)
   {
   }

   /**
    * Operator to check if a vertex has to be selected
    * @param v is the vertex to be analyzed
    * @return true if the vertex has to be selected
    */
   bool operator()(const typename boost::graph_traits<Graph>::vertex_descriptor& v) const
   {
      if(all)
      {
         return true;
      }
      else
      {
         return (subset.find(v) != subset.end());
      }
   }
};

/**
 * Predicate functor object used to select the proper set of edges
 */
template <typename Graph>
struct SelectEdge
{
 private:
   /// The selector associated with the filtered graph
   int selector;

   /// The bulk graph
   Graph* g;

   /// The vertices of subgraph
   CustomUnorderedSet<typename boost::graph_traits<Graph>::vertex_descriptor> subgraph_vertices;

   /// true when the subvertices set is empty
   bool empty;

 public:
   /**
    * Default constructor
    */
   SelectEdge() : selector(0), g(nullptr), subgraph_vertices(), empty(true)
   {
   }

   /**
    * Constructor for filtering only on selector
    * @param _selector is the selector of the filtered graph
    * @param _g is the graph
    */
   SelectEdge(const int _selector, Graph* _g) : selector(_selector), g(_g), subgraph_vertices(), empty(true)
   {
   }

   /**
    * Constructor for filtering also on vertices
    * @param _selector is the selector of the filtered graph
    * @param _g is the graph
    * @param _subgraph_vertices is the set of vertices of the filtered graph
    */
   SelectEdge(const int _selector, Graph* _g,
              const CustomUnorderedSet<typename boost::graph_traits<Graph>::vertex_descriptor>& _subgraph_vertices)
       : selector(_selector), g(_g), subgraph_vertices(_subgraph_vertices), empty(false)
   {
   }

   template <typename Edge>
   bool operator()(const Edge& e) const
   {
      if(empty)
      {
         return selector & (*g)[e].selector;
      }
      else
      {
         typename boost::graph_traits<Graph>::vertex_descriptor u, v;
         u = boost::source(e, *g);
         v = boost::target(e, *g);
         if((selector & (*g)[e].selector) && subgraph_vertices.find(v) != subgraph_vertices.end() &&
            subgraph_vertices.find(u) != subgraph_vertices.end())
         {
            return true;
         }
         else
         {
            return false;
         }
      }
   }
};

/**
 * General class used to describe a graph in PandA.
 */
struct graph : public boost::filtered_graph<boost_graphs_collection, SelectEdge<boost_graphs_collection>,
                                            SelectVertex<boost_graphs_collection>>
{
 protected:
   /**
    * Get the node property
    * @param node is the vertex whose property is asked
    * @return the associated property
    */
   inline NodeInfoRef GetNodeInfo(typename boost::graph_traits<graphs_collection>::vertex_descriptor node)
   {
      NodeInfoRef info = (*this)[node];
      THROW_ASSERT(info, "Node without associate info");
      return info;
   }

   /**
    * Get the node property
    * @param node is the node whose property is asked
    * @return the associated property
    */
   inline NodeInfoConstRef CGetNodeInfo(typename boost::graph_traits<graphs_collection>::vertex_descriptor node) const
   {
      const NodeInfoConstRef info = (*this)[node];
      THROW_ASSERT(info, "Node without associate info");
      return info;
   }

   /**
    * Get the edge property
    * @param source is the source vertex of the edge
    * @param target is the target vertex of the edge
    * @return the associated property
    */
   inline EdgeInfoRef GetEdgeInfo(typename boost::graph_traits<graphs_collection>::vertex_descriptor source,
                                  typename boost::graph_traits<graphs_collection>::vertex_descriptor target)
   {
      bool found;
      typename boost::graph_traits<graphs_collection>::edge_descriptor edge;
      boost::tie(edge, found) = boost::edge(source, target, *this);
      THROW_ASSERT(found, "Edge not present in the graph");
      return GetEdgeInfo(edge);
   }

   /**
    * Get the edge property
    * @param source is the source vertex of the edge
    * @param target is the target vertex of the edge
    * @return the associated property
    */
   inline EdgeInfoConstRef CGetEdgeInfo(typename boost::graph_traits<graphs_collection>::vertex_descriptor source,
                                        typename boost::graph_traits<graphs_collection>::vertex_descriptor target) const
   {
      bool found;
      typename boost::graph_traits<graphs_collection>::edge_descriptor edge;
      boost::tie(edge, found) = boost::edge(source, target, *this);
      THROW_ASSERT(found, "Edge not present in the graph");
      return CGetEdgeInfo(edge);
   }

   /**
    * Get the edge property
    * @param edge is the edge whose property is asked
    * @return the associated property
    */
   inline EdgeInfoRef GetEdgeInfo(typename boost::graph_traits<graphs_collection>::edge_descriptor edge)
   {
      const EdgeInfoRef info = (*this)[edge].info;
      THROW_ASSERT(info, "Info not associated with the edge");
      return info;
   }

   /**
    * Get the edge property
    * @param edge is the edge whose property is asked
    * @return the associated property
    */
   inline EdgeInfoConstRef CGetEdgeInfo(typename boost::graph_traits<graphs_collection>::edge_descriptor edge) const
   {
      const EdgeInfoConstRef info = (*this)[edge].info;
      THROW_ASSERT(info, "Info not associated with the edge");
      return info;
   }

   /// The graph collection
   graphs_collection* collection;

   /// selector
   const int selector;

   /**
    * Print the graph in dot format
    * @param file_name is the name of the file to be created
    * @param node_writer is the functor used to print the node labels
    * @param edge_writer is the functor used to print the edge labels
    * @param graph_writer is the functor used to print the graph properties
    */
   template <typename VertexWriterTemplate, typename EdgeWriterTemplate, typename GraphWriterTemplate>
   void InternalWriteDot(const std::string& file_name, const VertexWriterConstRef vertex_writer,
                         const EdgeWriterConstRef edge_writer, const GraphWriterConstRef graph_writer) const
   {
      std::ofstream file_stream(file_name.c_str());
      boost::write_graphviz(file_stream, *this, *(GetPointer<VertexWriterTemplate>(vertex_writer)),
                            *(GetPointer<EdgeWriterTemplate>(edge_writer)),
                            *(GetPointer<GraphWriterTemplate>(graph_writer)));
   }

 public:
   /// @name graph constructors
   //@{
   /**
    * Standard constructor.
    * @param g is the bulk graph.
    * @param _selector is the selector used to filter the bulk graph.
    */
   graph(graphs_collection* g, const int _selector)
       : boost::filtered_graph<boost_graphs_collection, SelectEdge<boost_graphs_collection>,
                               SelectVertex<boost_graphs_collection>>(
             *g, SelectEdge<boost_graphs_collection>(_selector, g), SelectVertex<boost_graphs_collection>()),
         collection(g),
         selector(_selector)
   {
   }

   /**
    * Sub-graph constructor.
    * @param g is the bulk graph.
    * @param _selector is the selector used to filter the bulk graph.
    * @param vertices is the set of vertexes on which the graph is filtered.
    */
   graph(graphs_collection* g, const int _selector,
         const CustomUnorderedSet<boost::graph_traits<graphs_collection>::vertex_descriptor>& vertices)
       : boost::filtered_graph<boost_graphs_collection, SelectEdge<boost_graphs_collection>,
                               SelectVertex<boost_graphs_collection>>(
             *g, SelectEdge<boost_graphs_collection>(_selector, g, vertices),
             SelectVertex<boost_graphs_collection>(vertices)),
         collection(g),
         selector(_selector)
   {
   }
   //@}

   /// this function can access the bulk graph
   friend boost::graph_traits<graph>::vertex_descriptor VERTEX(const boost::graph_traits<graph>::vertices_size_type,
                                                               const graph&);

   /// Destructor
   virtual ~graph() = default;

   /**
    * return true in case the vertex is a vertex of the subgraph.
    */
   bool is_in_subset(const boost::graph_traits<graph>::vertex_descriptor v) const
   {
      return m_vertex_pred(v);
   }

   /**
    * Return the selector of this graph
    * @return the selector of the graph
    */
   inline int GetSelector() const
   {
      return selector;
   }

   /**
    * Return the selectors associated with an edge
    * @param e is the edge
    * @return the associated selector
    */
   inline int GetSelector(const edge_descriptor e) const
   {
      return collection->GetSelector(e) & GetSelector();
   }

   /**
    * Return the selectors associated with an edge
    * @param source is the source of an edge
    * @param target is the target of an edge
    * @return the associated selector
    */
   inline int GetSelector(const boost::graph_traits<graphs_collection>::vertex_descriptor source,
                          const boost::graph_traits<graphs_collection>::vertex_descriptor target) const
   {
      bool found;
      typename boost::graph_traits<graphs_collection>::edge_descriptor edge;
      boost::tie(edge, found) = boost::edge(source, target, *this);
      return collection->GetSelector(edge);
   }

   /**
    * Compute the strongly connected components of the graph
    * @param strongly_connected_components is where the set of vertices which compose the different strongly connected
    * components will be stored; key is the index of the strongly connected component
    */
   void GetStronglyConnectedComponents(
       std::map<size_t, UnorderedSetStdStable<boost::graph_traits<graphs_collection>::vertex_descriptor>>&
           strongly_connected_components) const
   {
      std::map<boost::graph_traits<graphs_collection>::vertex_descriptor, size_t> temp_vertex_to_component;
      boost::associative_property_map<std::map<boost::graph_traits<graphs_collection>::vertex_descriptor, size_t>>
          vertex_to_component(temp_vertex_to_component);
      boost::strong_components(*this, vertex_to_component);
      boost::graph_traits<graph>::vertex_iterator vertex_it, vertex_end;
      for(boost::tie(vertex_it, vertex_end) = boost::vertices(*this); vertex_it != vertex_end; vertex_it++)
      {
         const size_t strongly_connected_component = vertex_to_component[*vertex_it];
         strongly_connected_components[strongly_connected_component].insert(*vertex_it);
      }
   }

   /**
    * Compute the breadth first search
    */
   void BreadthFirstSearch(const boost::graph_traits<graphs_collection>::vertex_descriptor node,
                           boost::bfs_visitor<>* vis) const
   {
      boost::breadth_first_search(*this, node, visitor(*vis));
   }

   /**
    * Compute the reverse topological order of the graph
    * @param sorted_vertices is where results will be store
    */
   void
   ReverseTopologicalSort(std::deque<boost::graph_traits<graphs_collection>::vertex_descriptor>& sorted_vertices) const
   {
      boost::topological_sort(*this, std::back_inserter(sorted_vertices));
   }

   /**
    * Compute the topological order of the graph
    * @param sorted_vertices is where results will be store
    */
   void TopologicalSort(std::list<boost::graph_traits<graphs_collection>::vertex_descriptor>& sorted_vertices) const
   {
      boost::topological_sort(*this, std::front_inserter(sorted_vertices));
   }

   /**
    * Compute if vertex y is reachable from x
    */
   bool IsReachable(const boost::graph_traits<graphs_collection>::vertex_descriptor x,
                    const boost::graph_traits<graphs_collection>::vertex_descriptor y) const
   {
      std::list<boost::graph_traits<graphs_collection>::vertex_descriptor> running_vertices;
      CustomUnorderedSet<boost::graph_traits<graphs_collection>::vertex_descriptor> encountered_vertices;
      running_vertices.push_back(x);
      encountered_vertices.insert(x);
      while(!running_vertices.empty())
      {
         const auto current = running_vertices.front();
         running_vertices.pop_front();
         BOOST_FOREACH(typename boost::graph_traits<graphs_collection>::edge_descriptor oe,
                       boost::out_edges(current, *this))
         {
            const auto target = boost::target(oe, *this);
            if(target == y)
            {
               return true;
            }
            if(encountered_vertices.insert(target).second)
            {
               running_vertices.push_back(target);
            }
         }
      }
      return false;
   }

   /**
    * FIXME: this method should become protected and called by equivalent method in subclasses
    * Get the graph property
    * @return the property associated with the graph
    */
   inline GraphInfoRef GetGraphInfo()
   {
      GraphInfoRef info = boost_CGetOpGraph_property(*this);
      return info;
   }

   /**
    * FIXME: this method should become protected and called by equivalent method in subclasses
    * Get the graph property
    * @return the property associated with the graph
    */
   inline GraphInfoConstRef CGetGraphInfo() const
   {
      GraphInfoRef info = boost_CGetOpGraph_property(*this);
      return info;
   }

   /**
    * Print the graph in dot format
    * FIXME: this method should become protected and called by WriteDot
    * @param file_name is the name of the file to be created
    * @param node_writer is the functor used to print the node labels
    * @param edge_writer is the functor used to print the edge labels
    */
   template <typename VertexWriterTemplate, typename EdgeWriterTemplate>
   void InternalWriteDot(const std::filesystem::path& file_name, const VertexWriterConstRef vertex_writer,
                         const EdgeWriterConstRef edge_writer) const
   {
      std::ofstream file_stream(file_name);
      boost::write_graphviz(file_stream, *this, *(GetPointer<VertexWriterTemplate>(vertex_writer)),
                            *(GetPointer<EdgeWriterTemplate>(edge_writer)));
   }

   /**
    * Check if an edge exists
    * @param source is the source vertex
    * @param target is the target vertex
    * @return true if source-target exists
    */
   inline bool ExistsEdge(const boost::graph_traits<graphs_collection>::vertex_descriptor source,
                          const boost::graph_traits<graphs_collection>::vertex_descriptor target) const
   {
      boost::graph_traits<graphs_collection>::edge_descriptor edge;
      bool inserted;
      boost::tie(edge, inserted) = boost::edge(source, target, *this);
      return inserted;
   }

   /**
    * Returns the edge connecting two vertices; throw error if it does not exist
    * @param source is the source of the edge to be returned
    * @param target is the target of the edge to be returned
    * @return the edge connecting source with target
    */
   inline boost::graph_traits<graphs_collection>::edge_descriptor
   CGetEdge(const boost::graph_traits<graphs_collection>::vertex_descriptor source,
            const boost::graph_traits<graphs_collection>::vertex_descriptor target) const
   {
      boost::graph_traits<graphs_collection>::edge_descriptor edge;
      bool inserted;
      boost::tie(edge, inserted) = boost::edge(source, target, *this);
      THROW_ASSERT(inserted, "Edge does not exist in this graph");
      return edge;
   }

#if BOOST_VERSION >= 104200
   using edge_property_type = boost::edge_property_type<graphs_collection>::type;
   using vertex_property_type = boost::vertex_property_type<graphs_collection>::type;
   using graph_property_type = boost::graph_property_type<graphs_collection>::type;
#endif
};
using graphRef = refcount<graph>;
using graphConstRef = refcount<const graph>;

#if BOOST_VERSION >= 104600
namespace boost
{
   namespace detail
   {
      struct filtered_graph_property_selector
      {
         template <class FilteredGraph, class Property, class Tag>
         struct bind_
         {
            using Graph = typename FilteredGraph::graph_type;
            using Map = property_map<Graph, Tag>;
            using type = typename Map::type;
            using const_type = typename Map::const_type;
         };
      };
   } // namespace detail
   template <>
   struct vertex_property_selector<filtered_graph_tag>
   {
      using type = detail::filtered_graph_property_selector;
   };
   template <>
   struct edge_property_selector<filtered_graph_tag>
   {
      using type = detail::filtered_graph_property_selector;
   };

} // namespace boost
#endif

/**
 * General class used to describe a graph in PandA.
 */
struct ugraph
    : public boost::filtered_graph<undirected_boost_graphs_collection, SelectEdge<undirected_boost_graphs_collection>,
                                   SelectVertex<undirected_boost_graphs_collection>>
{
 protected:
   /// The graph collection
   undirected_graphs_collection* collection;

   /// selector
   const int selector;

   /**
    * Print the graph in dot format
    * @param file_name is the name of the file to be created
    * @param node_writer is the functor used to print the node labels
    * @param edge_writer is the functor used to print the edge labels
    */
   template <typename UVertexWriterTemplate, typename UEdgeWriterTemplate>
   void InternalWriteDot(const std::string& file_name, const UVertexWriterConstRef uvertex_writer,
                         const UEdgeWriterConstRef uedge_writer) const
   {
      std::ofstream file_stream(file_name.c_str());
      boost::write_graphviz(file_stream, *this, *(GetPointer<UVertexWriterTemplate>(uvertex_writer)),
                            *(GetPointer<UEdgeWriterTemplate>(uedge_writer)));
   }

 public:
   /// @name graph constructors
   //@{
   /**
    * Standard constructor.
    * @param g is the bulk graph.
    * @param _selector is the selector used to filter the bulk graph.
    */
   ugraph(undirected_graphs_collection* g, const int _selector)
       : boost::filtered_graph<undirected_boost_graphs_collection, SelectEdge<undirected_boost_graphs_collection>,
                               SelectVertex<undirected_boost_graphs_collection>>(
             *g, SelectEdge<undirected_boost_graphs_collection>(_selector, g),
             SelectVertex<undirected_boost_graphs_collection>()),
         collection(g),
         selector(_selector)
   {
   }

   /**
    * Sub-graph constructor.
    * @param g is the bulk graph.
    * @param _selector is the selector used to filter the bulk graph.
    * @param vertices is the set of vertexes on which the graph is filtered.
    */
   ugraph(
       undirected_graphs_collection* g, const int _selector,
       const CustomUnorderedSet<boost::graph_traits<undirected_boost_graphs_collection>::vertex_descriptor>& vertices)
       : boost::filtered_graph<undirected_boost_graphs_collection, SelectEdge<undirected_boost_graphs_collection>,
                               SelectVertex<undirected_boost_graphs_collection>>(
             *g, SelectEdge<undirected_boost_graphs_collection>(_selector, g, vertices),
             SelectVertex<undirected_boost_graphs_collection>(vertices)),
         collection(g),
         selector(_selector)

   {
   }
   //@}

   /// this function can access the bulk graph
   friend boost::graph_traits<ugraph>::vertex_descriptor VERTEX(const boost::graph_traits<ugraph>::vertices_size_type,
                                                                const ugraph&);

   /// Destructor
   ~ugraph() = default;

   /**
    * return true in case the vertex is a vertex of the subgraph.
    */
   bool is_in_subset(const boost::graph_traits<ugraph>::vertex_descriptor v) const
   {
      return m_vertex_pred(v);
   }

   /**
    * Return the selector of this graph
    * @return the selector of the graph
    */
   inline int GetSelector() const
   {
      return selector;
   }

   /**
    * Return the selectors associated with an edge
    * @param e is the edge
    * @return the associated selector
    */
   inline int GetSelector(const edge_descriptor e) const
   {
      return collection->GetSelector(e);
   }

#if BOOST_VERSION >= 104200
   using edge_property_type = boost::edge_property_type<undirected_graphs_collection>::type;
   using vertex_property_type = boost::vertex_property_type<undirected_graphs_collection>::type;
   using graph_property_type = boost::graph_property_type<undirected_graphs_collection>::type;
#endif
};

using ugraphRef = refcount<ugraph>;

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
   /**
    * The constructors
    */
   explicit ltedge(const Graph* _g) : g(_g)
   {
   }

   /**
    * Redefinition of binary operator as less than
    */
   bool operator()(const typename boost::graph_traits<Graph>::edge_descriptor first,
                   const typename boost::graph_traits<Graph>::edge_descriptor second) const
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

/// vertex definition.
using vertex = boost::graph_traits<graph>::vertex_descriptor;
/// null vertex definition
#define NULL_VERTEX boost::graph_traits<graph>::null_vertex()
/// vertex_iterator definition.
using VertexIterator = boost::graph_traits<graph>::vertex_iterator;

/// in_edge_iterator definition.
using InEdgeIterator = boost::graph_traits<graph>::in_edge_iterator;
/// out_edge_iterator definition.
using OutEdgeIterator = boost::graph_traits<graph>::out_edge_iterator;
/// edge_iterator definition.
using EdgeIterator = boost::graph_traits<graph>::edge_iterator;
/// edge definition.
using EdgeDescriptor = boost::graph_traits<graph>::edge_descriptor;

/**
 * Definition of hash function for EdgeDescriptor
 */
namespace std
{
   template <>
   struct hash<EdgeDescriptor> : public unary_function<EdgeDescriptor, size_t>
   {
      size_t operator()(EdgeDescriptor edge) const
      {
         size_t hash_value = 0;
         boost::hash_combine(hash_value, edge.m_source);
         boost::hash_combine(hash_value, edge.m_target);
         return hash_value;
      }
   };
} // namespace std

template <typename H>
H AbslHashValue(const H& h, const EdgeDescriptor& m)
{
   return H::combine(h, m.m_source, m.m_target);
}
/// vertex definition.
using uvertex = boost::graph_traits<ugraph>::vertex_descriptor;
/// null vertex definition
#define NULL_UVERTEX boost::graph_traits<ugraph>::null_vertex()

/// vertex definition for undirected_graphs_collection.
using UGCvertex = boost::graph_traits<undirected_graphs_collection>::vertex_descriptor;
#define NULL_UGCVERTEX boost::graph_traits<undirected_graphs_collection>::null_vertex()

/// vertex_iterator definition.
using UVertexIterator = boost::graph_traits<ugraph>::vertex_iterator;

/// in_edge_iterator definition.
using UInEdgeIterator = boost::graph_traits<ugraph>::in_edge_iterator;
/// out_edge_iterator definition.
using UOutEdgeIterator = boost::graph_traits<ugraph>::out_edge_iterator;
/// edge_iterator definition.
using UEdgeIterator = boost::graph_traits<ugraph>::edge_iterator;
/// edge definition.
using UEdgeDescriptor = boost::graph_traits<ugraph>::edge_descriptor;

/**
 * Given a filtered graph and an index returns the vertex exploiting the boost::vertex applied on the original graph.
 * @param i is the index of the vertex.
 * @param g is the graph for which the vertex is asked.
 * @return the vertex with the index i of the graph g.
 */
inline boost::graph_traits<graph>::vertex_descriptor VERTEX(const boost::graph_traits<graph>::vertices_size_type i,
                                                            const graph& g)
{
   return boost::vertex(i, (g.m_g));
}

/**
 * Given a filtered ugraph and an index returns the vertex exploiting the boost::vertex applied on the original graph.
 * @param i is the index of the vertex.
 * @param g is the ugraph for which the vertex is asked.
 * @return the vertex with the index i of the graph g.
 */
inline boost::graph_traits<ugraph>::vertex_descriptor VERTEX(const boost::graph_traits<ugraph>::vertices_size_type i,
                                                             const ugraph& g)
{
   return boost::vertex(i, (g.m_g));
}

template <class Graph>
void ADD_UEDGE(typename boost::graph_traits<Graph>::vertex_descriptor A,
               typename boost::graph_traits<Graph>::vertex_descriptor B, int selector, Graph& g)
{
   UEdgeDescriptor e;
   bool inserted;
   boost::tie(e, inserted) = boost::edge(A, B, g);
   if(inserted)
   {
      g[e].selector = g[e].selector | selector;
   }
   else
   {
      boost::add_edge(A, B, selector, g);
   }
}

/**
 * Functor used to write the content of a vertex to dotty file
 */
class VertexWriter
{
 protected:
   /// The graph to be printed
   const graph* printing_graph;

   /// The detail level
   const int detail_level;

 public:
   /**
    * Constructor
    * @param _graph is the graph to be printed
    * @param _detail_level is the level of details in printing
    */
   VertexWriter(const graph* _graph, const int _detail_level) : printing_graph(_graph), detail_level(_detail_level)
   {
   }

   /**
    * Destructor
    */
   virtual ~VertexWriter() = default;

   /**
    * Functor actually called by the boost library to perform the writing
    * @param out is the stream where the nodes have to be printed
    * @param v is the vertex to be printed
    */
   virtual void operator()(std::ostream& out, const vertex& v) const = 0;
};

/**
 * Functor used to write the content of the edges to a dotty file
 */
class EdgeWriter
{
 protected:
   /// The graph to be printed
   const graph* printing_graph;

   /// The selector of the graph to be printed
   const int selector;

   /// The detail level
   const int detail_level;

 public:
   /**
    * Constructor
    * @param sdf_graph is the graph to be printed
    * @param detail_level if 1, state of the sdf graph is printed
    */
   EdgeWriter(const graph* _printing_graph, const int _detail_level)
       : printing_graph(_printing_graph), selector(printing_graph->GetSelector()), detail_level(_detail_level)
   {
   }

   /**
    * Destructor
    */
   virtual ~EdgeWriter() = default;

   /**
    * Functor actually called by the boost library to perform the writing
    * @param out is the stream where the edges have to be printed
    * @param edge is the edge to be printed
    */
   virtual void operator()(std::ostream& out, const EdgeDescriptor& edge) const = 0;
};

/**
 * Functor used to write the content of the property of a graph to a dotty file
 */
class GraphWriter
{
 protected:
   /// The graph to be printed
   const graph* printing_graph;

   /// The detail level (i.e., how much information has to be included)
   const int detail_level;

 public:
   /**
    * Constructor
    * @param _printing_graph is the graph to be printed
    * @param _detail_level is the detail level of the printing
    */
   GraphWriter(const graph* _printing_graph, const int _detail_level)
       : printing_graph(_printing_graph), detail_level(_detail_level)
   {
   }

   /**
    * Destructor
    */
   virtual ~GraphWriter() = default;

   /**
    * Functor acturally called by the boost library to perform the writing
    * @param out is the stream where the edges have to be printed
    */
   virtual void operator()(std::ostream& out) const = 0;
};

/**
 * Functor used to write the content of a vertex to dotty file
 */
class UVertexWriter
{
 protected:
   /// The graph to be printed
   const ugraph* printing_ugraph;

   /// The detail level
   const int detail_level;

 public:
   /**
    * Constructor
    * @param _graph is the graph to be printed
    * @param _detail_level is the level of details in printing
    */
   UVertexWriter(const ugraph* _printing_ugraph, const int _detail_level)
       : printing_ugraph(_printing_ugraph), detail_level(_detail_level)
   {
   }

   /**
    * Destructor
    */
   virtual ~UVertexWriter() = default;

   /**
    * Functor actually called by the boost library to perform the writing
    * @param out is the stream where the nodes have to be printed
    * @param v is the vertex to be printed
    */
   virtual void operator()(std::ostream& out, const uvertex& v) const = 0;
};

/**
 * Functor used to write the content of the edges to a dotty file
 */
class UEdgeWriter
{
 protected:
   /// The graph to be printed
   const ugraph* printing_ugraph;

   /// The selector of the graph to be printed
   const int selector;

   /// The deail level; if one, tate of the graph is printed
   const int detail_level;

 public:
   /**
    * Constructor
    * @param sdf_graph is the graph to be printed
    * @param detail_level if 1, state of the sdf graph is printed
    */
   UEdgeWriter(const ugraph* _printing_ugraph, const int _detail_level)
       : printing_ugraph(_printing_ugraph), selector(printing_ugraph->GetSelector()), detail_level(_detail_level)
   {
   }

   /**
    * Destructor
    */
   virtual ~UEdgeWriter() = default;

   /**
    * Functor actually called by the boost library to perform the writing
    * @param out is the stream where the edges have to be printed
    * @param edge is the edge to be printed
    */
   virtual void operator()(std::ostream& out, const UEdgeDescriptor& edge) const = 0;
};
#endif
