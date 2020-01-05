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
 *              Copyright (C) 2004-2020 Politecnico di Milano
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

/// Graph include
#include "edge_info.hpp"
#include "graph_info.hpp"
#include "node_info.hpp"

/// STD include
#include <fstream>
#include <ostream>

/// STL include
#include "custom_set.hpp"
#include <deque>
#include <list>

/// Utility include
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

typedef boost::adjacency_list<boost::listS, boost::listS, boost::bidirectionalS, boost::property<boost::vertex_index_t, std::size_t, NodeInfoRef>, EdgeInfoRef, GraphInfoRef> boost_raw_graph;

struct RawGraph : public boost_raw_graph
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

   /**
    * Destructor
    */
   ~RawGraph() = default;

   inline boost::graph_traits<boost_raw_graph>::vertex_descriptor AddVertex(const NodeInfoRef v_info)
   {
      size_t index = boost::num_vertices(*this);
      boost::graph_traits<boost_raw_graph>::vertex_descriptor new_v = boost::add_vertex(*this);
      boost::property_map<boost_raw_graph, boost::vertex_index_t>::type index_map = boost::get(boost::vertex_index_t(), *this);
      (*this)[new_v] = v_info;
      index_map[new_v] = index;
      return new_v;
   };

   inline void RemoveVertex(boost::graph_traits<boost_raw_graph>::vertex_descriptor v)
   {
      boost::remove_vertex(v, *this);
      boost::property_map<boost_raw_graph, boost::vertex_index_t>::type index_map = boost::get(boost::vertex_index_t(), *this);
      boost::graph_traits<boost_raw_graph>::vertex_iterator v_it, v_it_end;
      size_t index = 0;
      for(boost::tie(v_it, v_it_end) = boost::vertices(*this); v_it != v_it_end; v_it++, index++)
      {
         index_map[*v_it] = index;
      }
   }

   inline boost::graph_traits<boost_raw_graph>::edge_descriptor AddEdge(boost::graph_traits<boost_raw_graph>::vertex_descriptor src, boost::graph_traits<boost_raw_graph>::vertex_descriptor tgt, const EdgeInfoRef e_info)
   {
      boost::graph_traits<boost_raw_graph>::edge_descriptor e;
      bool found;
      boost::tie(e, found) = boost::edge(src, tgt, *this);
      THROW_ASSERT(not found, "Trying to insert an already existing edge");
      boost::tie(e, found) = boost::add_edge(src, tgt, e_info, *this);
      (*this)[e] = e_info;
      return e;
   }

   inline void RemoveEdge(boost::graph_traits<boost_raw_graph>::edge_descriptor e)
   {
      boost::remove_edge(boost::source(e, *this), boost::target(e, *this), *this);
   }

   inline void RemoveEdge(boost::graph_traits<boost_raw_graph>::vertex_descriptor src, boost::graph_traits<boost_raw_graph>::vertex_descriptor tgt)
   {
      boost::graph_traits<boost_raw_graph>::edge_descriptor e;
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
      info = EdgeInfoRef(new info_object);
   THROW_ASSERT(GetPointer<info_object>(info) != nullptr, "Function get_raw_edge_info: the edges associated with the graph used are not derived from info_object\n\tCheck the actual type of info_object and the type of the edge of Graph");
   return GetPointer<info_object>(info);
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
   THROW_ASSERT(!info || dynamic_cast<const info_object*>(info) != nullptr, "Function Cget_raw_edge_info: the edges associated with the graph used are not derived from info_object\n\tCheck the actual type of info_object and the type of the edge of Graph");
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
   EdgeProperty(const int _selector, EdgeInfoRef _info) : selector(_selector), info(std::move(_info))
   {
   }
};
#if BOOST_VERSION >= 104600
typedef boost::adjacency_list<boost::listS, boost::listS, boost::bidirectionalS, boost::property<boost::vertex_index_t, std::size_t, boost::property<boost::vertex_color_t, boost::default_color_type, NodeInfoRef>>, EdgeProperty, GraphInfoRef>
    boost_graphs_collection;

typedef boost::adjacency_list<boost::listS, boost::listS, boost::undirectedS, boost::property<boost::vertex_index_t, std::size_t, boost::property<boost::vertex_color_t, boost::default_color_type, NodeInfoRef>>, EdgeProperty, GraphInfoRef>
    undirected_boost_graphs_collection;

#define boost_CGetOpGraph_property(graph_arg) (graph_arg)[boost::graph_bundle]

#else

/**
 * Custom graph property: GraphInfo.
 * This property provide a refcount pointer to the GraphInfo object. This object store all the information associatet with the whole graph.
 */
struct graph_info_t
{
   /// typedef defining graph_info_t as graph object
   typedef boost::graph_property_tag kind;
};

typedef boost::property<graph_info_t, GraphInfoRef> GraphProperty;

typedef boost::adjacency_list<boost::listS, boost::listS, boost::bidirectionalS, boost::property<boost::vertex_index_t, std::size_t, boost::property<boost::vertex_color_t, boost::default_color_type, NodeInfoRef>>, EdgeProperty, GraphProperty>
    boost_graphs_collection;

typedef boost::adjacency_list<boost::listS, boost::listS, boost::undirectedS, boost::property<boost::vertex_index_t, std::size_t, boost::property<boost::vertex_color_t, boost::default_color_type, NodeInfoRef>>, EdgeProperty, GraphProperty>
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
   virtual ~graphs_collection()
   {
   }

   /**
    * Add a selector to an existing edge
    * @param edge is the edge to be considered
    * @param selector is the selector to be added
    */
   inline boost::graph_traits<graphs_collection>::edge_descriptor AddSelector(const boost::graph_traits<graphs_collection>::edge_descriptor edge, const int selector)
   {
      (*this)[edge].selector |= selector;
      return edge;
   }

   /**
    * Add a selector to an existing edge
    * @param source is the source of the edge
    * @param target is the target of the edge
    */
   inline boost::graph_traits<graphs_collection>::edge_descriptor AddSelector(const boost::graph_traits<graphs_collection>::vertex_descriptor source, const boost::graph_traits<graphs_collection>::vertex_descriptor target, const int selector)
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
   inline void RemoveSelector(boost::graph_traits<graphs_collection>::vertex_descriptor source, boost::graph_traits<graphs_collection>::vertex_descriptor target, const int selector)
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
   inline boost::graph_traits<graphs_collection>::edge_descriptor AddEdge(boost::graph_traits<graphs_collection>::vertex_descriptor, boost::graph_traits<graphs_collection>::vertex_descriptor, const int)
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
   inline boost::graph_traits<graphs_collection>::edge_descriptor InternalAddEdge(boost::graph_traits<graphs_collection>::vertex_descriptor source, boost::graph_traits<graphs_collection>::vertex_descriptor target, const int selector,
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
   inline bool ExistsEdge(const boost::graph_traits<graphs_collection>::vertex_descriptor source, const boost::graph_traits<graphs_collection>::vertex_descriptor target) const
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
         boost::remove_edge(e0, *this);
   }
};

typedef refcount<graphs_collection> graphs_collectionRef;

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
      boost::property_map<undirected_boost_graphs_collection, boost::vertex_index_t>::type index_map = boost::get(boost::vertex_index_t(), *this);
      index_map[v] = index;
      return v;
   }

   /**
    * Remove a vertex from this graph
    * @param v is the vertex to be removed
    */
   inline void RemoveVertex(boost::graph_traits<undirected_boost_graphs_collection>::vertex_descriptor v)
   {
      boost::remove_vertex(v, *this);
      boost::property_map<undirected_boost_graphs_collection, boost::vertex_index_t>::type index_map = boost::get(boost::vertex_index_t(), *this);
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

typedef refcount<undirected_graphs_collection> undirected_graphs_collectionRef;

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
      info = NodeInfoRef(new info_object);
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
      info = NodeInfoRef(new info_object);
   THROW_ASSERT(GetPointer<info_object>(info) != nullptr, "Function get_node_info: the vertices associated with the graph used are not derived from info_object\n\tCheck the actual type of info_object and the type of the node of Graph");
   return GetPointer<info_object>(info);
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
   THROW_ASSERT(!info || dynamic_cast<const info_object*>(info) != nullptr, "Function Cget_node_info: the nodes associated with the graph used are not derived from info_object\n\tCheck the actual type of info_object and the type of the node of Graph");
   return info ? dynamic_cast<const info_object*>(info) : nullptr;
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
      info = EdgeInfoRef(new info_object);
   THROW_ASSERT(GetPointer<info_object>(info) != nullptr, "Function get_edge_info: the edges associated with the graph used are not derived from info_object\n\tCheck the actual type of info_object and the type of the edge of Graph");
   return GetPointer<info_object>(info);
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
   THROW_ASSERT(!info || dynamic_cast<const info_object*>(info) != nullptr, "Function Cget_edge_info: the edges associated with the graph used are not derived from info_object\n\tCheck the actual type of info_object and the type of the edge of Graph");
   return info ? dynamic_cast<const info_object*>(info) : nullptr;
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
   explicit SelectVertex(CustomUnorderedSet<typename boost::graph_traits<Graph>::vertex_descriptor> _subset) : all(false), subset(std::move(_subset))
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
         return true;
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

 public:
   /**
    * Default constructor
    */
   SelectEdge() : selector(0), g(nullptr), subgraph_vertices()
   {
   }

   /**
    * Constructor for filtering only on selector
    * @param _selector is the selector of the filtered graph
    * @param _g is the graph
    */
   SelectEdge(const int _selector, Graph* _g) : selector(_selector), g(_g), subgraph_vertices()
   {
   }

   /**
    * Constructor for filtering also on vertices
    * @param _selector is the selector of the filtered graph
    * @param _g is the graph
    * @param _subgraph_vertices is the set of vertices of the filtered graph
    */
   SelectEdge(const int _selector, Graph* _g, CustomUnorderedSet<typename boost::graph_traits<Graph>::vertex_descriptor> _subgraph_vertices) : selector(_selector), g(_g), subgraph_vertices(std::move(_subgraph_vertices))
   {
   }

   template <typename Edge>
   bool operator()(const Edge& e) const
   {
      if(subgraph_vertices.empty())
         return selector & (*g)[e].selector;
      else
      {
         typename boost::graph_traits<Graph>::vertex_descriptor u, v;
         u = boost::source(e, *g);
         v = boost::target(e, *g);
         if(subgraph_vertices.find(v) != subgraph_vertices.end() && subgraph_vertices.find(u) != subgraph_vertices.end())
            return selector & (*g)[e].selector;
         else
            return false;
      }
   }
};

/**
 * General class used to describe a graph in PandA.
 */
struct graph : public boost::filtered_graph<boost_graphs_collection, SelectEdge<boost_graphs_collection>, SelectVertex<boost_graphs_collection>>
{
 protected:
   /**
    * Get the node property
    * @param node is the node whose property is asked
    * @return the associated property
    */
   inline const NodeInfoConstRef CGetNodeInfo(typename boost::graph_traits<graphs_collection>::vertex_descriptor node) const
   {
      const NodeInfoRef info = (*this)[node];
      THROW_ASSERT(info, "Node without associate info");
      return info;
   }

   /**
    * Get the edge property
    * @param source is the source vertex of the edge
    * @param target is the target vertex of the edge
    * @return the associated property
    */
   inline EdgeInfoRef GetEdgeInfo(typename boost::graph_traits<graphs_collection>::vertex_descriptor source, typename boost::graph_traits<graphs_collection>::vertex_descriptor target)
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
   inline const EdgeInfoConstRef CGetEdgeInfo(typename boost::graph_traits<graphs_collection>::vertex_descriptor source, typename boost::graph_traits<graphs_collection>::vertex_descriptor target) const
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
   inline EdgeInfoRef GetEdgeInfo(typename boost::graph_traits<graphs_collection>::edge_descriptor edge) const
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
   void InternalWriteDot(const std::string& file_name, const VertexWriterConstRef vertex_writer, const EdgeWriterConstRef edge_writer, const GraphWriterConstRef graph_writer) const
   {
      std::ofstream file_stream(file_name.c_str());
      boost::write_graphviz(file_stream, *this, *(GetPointer<VertexWriterTemplate>(vertex_writer)), *(GetPointer<EdgeWriterTemplate>(edge_writer)), *(GetPointer<GraphWriterTemplate>(graph_writer)));
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
       : boost::filtered_graph<boost_graphs_collection, SelectEdge<boost_graphs_collection>, SelectVertex<boost_graphs_collection>>(*g, SelectEdge<boost_graphs_collection>(_selector, g), SelectVertex<boost_graphs_collection>()),
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
   graph(graphs_collection* g, const int _selector, const CustomUnorderedSet<boost::graph_traits<graphs_collection>::vertex_descriptor>& vertices)
       : boost::filtered_graph<boost_graphs_collection, SelectEdge<boost_graphs_collection>, SelectVertex<boost_graphs_collection>>(*g, SelectEdge<boost_graphs_collection>(_selector, g, vertices), SelectVertex<boost_graphs_collection>(vertices)),
         collection(g),
         selector(_selector)
   {
   }
   //@}

   /// this function can access the bulk graph
   friend boost::graph_traits<graph>::vertex_descriptor VERTEX(const boost::graph_traits<graph>::vertices_size_type, const graph&);

   /// Destructor
   virtual ~graph()
   {
   }

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
   inline int GetSelector(const boost::graph_traits<graphs_collection>::vertex_descriptor source, const boost::graph_traits<graphs_collection>::vertex_descriptor target) const
   {
      bool found;
      typename boost::graph_traits<graphs_collection>::edge_descriptor edge;
      boost::tie(edge, found) = boost::edge(source, target, *this);
      return collection->GetSelector(edge);
   }

   /**
    * Compute the strongly connected components of the graph
    * @param strongly_connected_components is where the set of vertices which compose the different strongly connected components will be stored;
    * key is the index of the strongly connected component
    */
   void GetStronglyConnectedComponents(std::map<size_t, UnorderedSetStdStable<boost::graph_traits<graphs_collection>::vertex_descriptor>>& strongly_connected_components) const
   {
      std::map<boost::graph_traits<graphs_collection>::vertex_descriptor, size_t> temp_vertex_to_component;
      boost::associative_property_map<std::map<boost::graph_traits<graphs_collection>::vertex_descriptor, size_t>> vertex_to_component(temp_vertex_to_component);
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
   void BreadthFirstSearch(const boost::graph_traits<graphs_collection>::vertex_descriptor node, boost::bfs_visitor<>* vis) const
   {
      boost::breadth_first_search(*this, node, visitor(*vis));
   }

   /**
    * Compute the reverse topological order of the graph
    * @param sorted_vertices is where results will be store
    */
   void ReverseTopologicalSort(std::deque<boost::graph_traits<graphs_collection>::vertex_descriptor>& sorted_vertices) const
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
   bool IsReachable(const boost::graph_traits<graphs_collection>::vertex_descriptor x, const boost::graph_traits<graphs_collection>::vertex_descriptor y) const
   {
      std::list<boost::graph_traits<graphs_collection>::vertex_descriptor> running_vertices;
      CustomUnorderedSet<boost::graph_traits<graphs_collection>::vertex_descriptor> encountered_vertices;
      running_vertices.push_back(x);
      encountered_vertices.insert(x);
      while(not running_vertices.empty())
      {
         const boost::graph_traits<graphs_collection>::vertex_descriptor current = running_vertices.front();
         running_vertices.pop_front();
         boost::graph_traits<graph>::out_edge_iterator oe, oe_end;
         for(boost::tie(oe, oe_end) = boost::out_edges(current, *this); oe != oe_end; oe++)
         {
            const boost::graph_traits<graphs_collection>::vertex_descriptor target = boost::target(*oe, *this);
            if(target == y)
               return true;
            if(encountered_vertices.find(target) == encountered_vertices.end())
            {
               encountered_vertices.insert(target);
               running_vertices.push_back(target);
            }
         }
      }
      return false;
   }

   /**
    * FIXME: this method should become protected and called by equivalent method in subclasses
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
   inline const GraphInfoConstRef CGetGraphInfo() const
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
   void InternalWriteDot(const std::string& file_name, const VertexWriterConstRef vertex_writer, const EdgeWriterConstRef edge_writer) const
   {
      std::ofstream file_stream(file_name.c_str());
      boost::write_graphviz(file_stream, *this, *(GetPointer<VertexWriterTemplate>(vertex_writer)), *(GetPointer<EdgeWriterTemplate>(edge_writer)));
   }

   /**
    * Check if an edge exists
    * @param source is the source vertex
    * @param target is the target vertex
    * @return true if source-target exists
    */
   inline bool ExistsEdge(const boost::graph_traits<graphs_collection>::vertex_descriptor source, const boost::graph_traits<graphs_collection>::vertex_descriptor target) const
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
   inline boost::graph_traits<graphs_collection>::edge_descriptor CGetEdge(const boost::graph_traits<graphs_collection>::vertex_descriptor source, const boost::graph_traits<graphs_collection>::vertex_descriptor target) const
   {
      boost::graph_traits<graphs_collection>::edge_descriptor edge;
      bool inserted;
      boost::tie(edge, inserted) = boost::edge(source, target, *this);
      THROW_ASSERT(inserted, "Edge does not exist in this graph");
      return edge;
   }

#if BOOST_VERSION >= 104200
   typedef boost::edge_property_type<graphs_collection>::type edge_property_type;
   typedef boost::vertex_property_type<graphs_collection>::type vertex_property_type;
   typedef boost::graph_property_type<graphs_collection>::type graph_property_type;
#endif
};
typedef refcount<graph> graphRef;
typedef refcount<const graph> graphConstRef;

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
            typedef typename FilteredGraph::graph_type Graph;
            typedef property_map<Graph, Tag> Map;
            typedef typename Map::type type;
            typedef typename Map::const_type const_type;
         };
      };
   } // namespace detail
   template <>
   struct vertex_property_selector<filtered_graph_tag>
   {
      typedef detail::filtered_graph_property_selector type;
   };
   template <>
   struct edge_property_selector<filtered_graph_tag>
   {
      typedef detail::filtered_graph_property_selector type;
   };

} // namespace boost
#endif

/**
 * General class used to describe a graph in PandA.
 */
struct ugraph : public boost::filtered_graph<undirected_boost_graphs_collection, SelectEdge<undirected_boost_graphs_collection>, SelectVertex<undirected_boost_graphs_collection>>
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
   void InternalWriteDot(const std::string& file_name, const UVertexWriterConstRef uvertex_writer, const UEdgeWriterConstRef uedge_writer) const
   {
      std::ofstream file_stream(file_name.c_str());
      boost::write_graphviz(file_stream, *this, *(GetPointer<UVertexWriterTemplate>(uvertex_writer)), *(GetPointer<UEdgeWriterTemplate>(uedge_writer)));
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
       : boost::filtered_graph<undirected_boost_graphs_collection, SelectEdge<undirected_boost_graphs_collection>, SelectVertex<undirected_boost_graphs_collection>>(*g, SelectEdge<undirected_boost_graphs_collection>(_selector, g),
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
   ugraph(undirected_graphs_collection* g, const int _selector, const CustomUnorderedSet<boost::graph_traits<undirected_boost_graphs_collection>::vertex_descriptor>& vertices)
       : boost::filtered_graph<undirected_boost_graphs_collection, SelectEdge<undirected_boost_graphs_collection>, SelectVertex<undirected_boost_graphs_collection>>(*g, SelectEdge<undirected_boost_graphs_collection>(_selector, g, vertices),
                                                                                                                                                                     SelectVertex<undirected_boost_graphs_collection>(vertices)),
         collection(g),
         selector(_selector)

   {
   }
   //@}

   /// this function can access the bulk graph
   friend boost::graph_traits<ugraph>::vertex_descriptor VERTEX(const boost::graph_traits<ugraph>::vertices_size_type, const ugraph&);

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
   typedef boost::edge_property_type<undirected_graphs_collection>::type edge_property_type;
   typedef boost::vertex_property_type<undirected_graphs_collection>::type vertex_property_type;
   typedef boost::graph_property_type<undirected_graphs_collection>::type graph_property_type;
#endif
};

typedef refcount<ugraph> ugraphRef;

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
   bool operator()(const typename boost::graph_traits<Graph>::edge_descriptor first, const typename boost::graph_traits<Graph>::edge_descriptor second) const
   {
      if(boost::source(first, *g) < boost::source(second, *g))
         return true;
      if(boost::source(first, *g) > boost::source(second, *g))
         return false;
      return boost::target(first, *g) < boost::target(second, *g);
   }
};

/// vertex definition.
typedef boost::graph_traits<graph>::vertex_descriptor vertex;
/// null vertex definition
#define NULL_VERTEX boost::graph_traits<graph>::null_vertex()
/// vertex_iterator definition.
typedef boost::graph_traits<graph>::vertex_iterator VertexIterator;

/// in_edge_iterator definition.
typedef boost::graph_traits<graph>::in_edge_iterator InEdgeIterator;
/// out_edge_iterator definition.
typedef boost::graph_traits<graph>::out_edge_iterator OutEdgeIterator;
/// edge_iterator definition.
typedef boost::graph_traits<graph>::edge_iterator EdgeIterator;
/// edge definition.
typedef boost::graph_traits<graph>::edge_descriptor EdgeDescriptor;

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
H AbslHashValue(H h, const EdgeDescriptor& m)
{
   return H::combine(std::move(h), m.m_source, m.m_target);
}
/// vertex definition.
typedef boost::graph_traits<ugraph>::vertex_descriptor uvertex;
/// null vertex definition
#define NULL_UVERTEX boost::graph_traits<ugraph>::null_vertex()

/// vertex definition for undirected_graphs_collection.
typedef boost::graph_traits<undirected_graphs_collection>::vertex_descriptor UGCvertex;
#define NULL_UGCVERTEX boost::graph_traits<undirected_graphs_collection>::null_vertex()

/// vertex_iterator definition.
typedef boost::graph_traits<ugraph>::vertex_iterator UVertexIterator;

/// in_edge_iterator definition.
typedef boost::graph_traits<ugraph>::in_edge_iterator UInEdgeIterator;
/// out_edge_iterator definition.
typedef boost::graph_traits<ugraph>::out_edge_iterator UOutEdgeIterator;
/// edge_iterator definition.
typedef boost::graph_traits<ugraph>::edge_iterator UEdgeIterator;
/// edge definition.
typedef boost::graph_traits<ugraph>::edge_descriptor UEdgeDescriptor;

/**
 * Given a filtered graph and an index returns the vertex exploiting the boost::vertex applied on the original graph.
 * @param i is the index of the vertex.
 * @param g is the graph for which the vertex is asked.
 * @return the vertex with the index i of the graph g.
 */
inline boost::graph_traits<graph>::vertex_descriptor VERTEX(const boost::graph_traits<graph>::vertices_size_type i, const graph& g)
{
   return boost::vertex(i, (g.m_g));
}

/**
 * Given a filtered ugraph and an index returns the vertex exploiting the boost::vertex applied on the original graph.
 * @param i is the index of the vertex.
 * @param g is the ugraph for which the vertex is asked.
 * @return the vertex with the index i of the graph g.
 */
inline boost::graph_traits<ugraph>::vertex_descriptor VERTEX(const boost::graph_traits<ugraph>::vertices_size_type i, const ugraph& g)
{
   return boost::vertex(i, (g.m_g));
}

template <class Graph>
void ADD_UEDGE(typename boost::graph_traits<Graph>::vertex_descriptor A, typename boost::graph_traits<Graph>::vertex_descriptor B, int selector, Graph& g)
{
   UEdgeDescriptor e;
   bool inserted;
   boost::tie(e, inserted) = boost::edge(A, B, g);
   if(inserted)
      g[e].selector = g[e].selector | selector;
   else
      boost::add_edge(A, B, selector, g);
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
   virtual ~VertexWriter()
   {
   }

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
   EdgeWriter(const graph* _printing_graph, const int _detail_level) : printing_graph(_printing_graph), selector(printing_graph->GetSelector()), detail_level(_detail_level)
   {
   }

   /**
    * Destructor
    */
   virtual ~EdgeWriter()
   {
   }

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
   GraphWriter(const graph* _printing_graph, const int _detail_level) : printing_graph(_printing_graph), detail_level(_detail_level)
   {
   }

   /**
    * Destructor
    */
   virtual ~GraphWriter()
   {
   }

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
   UVertexWriter(const ugraph* _printing_ugraph, const int _detail_level) : printing_ugraph(_printing_ugraph), detail_level(_detail_level)
   {
   }

   /**
    * Destructor
    */
   virtual ~UVertexWriter()
   {
   }

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
   UEdgeWriter(const ugraph* _printing_ugraph, const int _detail_level) : printing_ugraph(_printing_ugraph), selector(printing_ugraph->GetSelector()), detail_level(_detail_level)
   {
   }

   /**
    * Destructor
    */
   virtual ~UEdgeWriter()
   {
   }

   /**
    * Functor actually called by the boost library to perform the writing
    * @param out is the stream where the edges have to be printed
    * @param edge is the edge to be printed
    */
   virtual void operator()(std::ostream& out, const UEdgeDescriptor& edge) const = 0;
};
#endif
