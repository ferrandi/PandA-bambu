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
 * @file call_graph.hpp
 * @brief Call graph hierarchy.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef CALL_GRAPH_HPP
#define CALL_GRAPH_HPP
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "edge_info.hpp"  // for EdgeInfo, EdgeIn...
#include "graph.hpp"      // for graph, vertex
#include "graph_info.hpp" // for GraphInfo
#include "node_info.hpp"  // for NodeInfo
#include "refcount.hpp"   // for refcount, Refcou...

#include <iosfwd> // for ostream
#include <string> // for string

REF_FORWARD_DECL(FunctionBehavior);

/**
 * Information associated with a call_graph node.
 */
struct FunctionInfo : public NodeInfo
{
   /// this is the nodeID of the function associated with the vertex
   unsigned int nodeID;

   /**
    * Constructor
    */
   FunctionInfo();
};

/**
 * Information associated with a call_graph edge.
 */
struct FunctionEdgeInfo : public EdgeInfo
{
   /// the index of the statements of the caller function where the target is called;
   CustomOrderedSet<unsigned int> direct_call_points;
   CustomOrderedSet<unsigned int> indirect_call_points;
   CustomOrderedSet<unsigned int> function_addresses;

   enum class CallType
   {
      direct_call,
      indirect_call,
      function_address,
      call_any
   };

   /**
    * Constructor
    */
   FunctionEdgeInfo();
};
typedef refcount<FunctionEdgeInfo> FunctionEdgeInfoRef;
typedef refcount<const FunctionEdgeInfo> FunctionEdgeInfoConstRef;

/**
 * The info associated with the call graph
 */
struct CallGraphInfo : public GraphInfo
{
 public:
   /// reference to the behaviors
   std::map<unsigned int, FunctionBehaviorRef> behaviors;
};
/// The refcount definition for CallGraphInfo
typedef refcount<CallGraphInfo> CallGraphInfoRef;
typedef refcount<const CallGraphInfo> CallGraphInfoConstRef;

/**
 * This class collects information concerning the set of functions that will be analyzed by the PandA framework
 */
class CallGraphsCollection : public graphs_collection
{
 public:
   /**
    * Constructor
    * @param call_graph_info is the info to be associated with the call graph
    * @param _parameters is the set of input parameters
    */
   CallGraphsCollection(const CallGraphInfoRef call_graph_info, const ParameterConstRef _parameters);

   /**
    * Destructor
    */
   ~CallGraphsCollection() override;

   /**
    * Add an edge with empty information associated
    * @param source is the source of the edge
    * @param target is the target of the edge
    * @param selector is the selector to be added
    * @return the created edge
    */
   inline EdgeDescriptor AddEdge(const vertex source, const vertex target, const int selector)
   {
      if(ExistsEdge(source, target))
         return AddSelector(source, target, selector);
      else
         return InternalAddEdge(source, target, selector, EdgeInfoRef(new FunctionEdgeInfo()));
   }
};
/// The refcount definition for CallGraphInfo
typedef refcount<CallGraphsCollection> CallGraphsCollectionRef;
typedef refcount<const CallGraphsCollection> CallGraphsCollectionConstRef;

/**
 * This class is the view of a call graph
 */
class CallGraph : public graph
{
 public:
   /**
    * Constructor
    * @param call_graphs_collection is the starting call graphs collection
    * @param selector is the selector of the view
    */
   CallGraph(const CallGraphsCollectionRef call_graphs_collection, const int selector);

   /**
    * Constructor
    * @param call_graphs_collection is the starting call graphs collection
    * @param selector is the selector of the view
    * @param vertices is the set of vertices to be considered
    */
   CallGraph(const CallGraphsCollectionRef call_graphs_collection, const int selector, const CustomUnorderedSet<vertex>& vertices);

   /**
    * Destructor
    */
   ~CallGraph() override;

   /**
    * Return the info associated with an edge
    * @param edge is the edge to be considered
    */
   inline const FunctionEdgeInfoConstRef CGetFunctionEdgeInfo(const EdgeDescriptor edge) const
   {
      return RefcountCast<const FunctionEdgeInfo>(graph::CGetEdgeInfo(edge));
   }

   /**
    * Return the info associated with the call graph
    * @return the info associated with the call graph
    */
   inline const CallGraphInfoConstRef CGetCallGraphInfo() const
   {
      return RefcountCast<const CallGraphInfo>(graph::CGetGraphInfo());
   }

   /**
    * Return the info associated with the call graph
    * @return the info associated with the call graph
    */
   inline CallGraphInfoRef GetCallGraphInfo()
   {
      return RefcountCast<CallGraphInfo>(graph::GetGraphInfo());
   }

   /**
    * Write the call graph in dot format
    * @param file_name is the name of the file to create
    */
   void WriteDot(const std::string& file_name) const;
};
/// The refcount definition for CallGraph
typedef refcount<CallGraph> CallGraphRef;
typedef refcount<const CallGraph> CallGraphConstRef;

/**
 * Functor used by write_graphviz to write the label of the vertices of a function graph
 */
class FunctionWriter : public VertexWriter
{
 private:
   /// reference to the behaviors
   const std::map<unsigned int, FunctionBehaviorRef>& behaviors;

 public:
   /**
    * constructor
    * @param call_graph is the graph to be printed
    */
   explicit FunctionWriter(const CallGraph* call_graph);

   /**
    * operator function returning the label of the vertex
    * @param out is the output stream
    * @param v is the vertex
    */
   void operator()(std::ostream& out, const vertex& v) const override;
};

/**
 * Functor used by write_graphviz to write the edges of a function graph
 */
class FunctionEdgeWriter : public EdgeWriter
{
 private:
   /// reference to the behaviors
   const std::map<unsigned int, FunctionBehaviorRef>& behaviors;

 public:
   /**
    * constructor
    * @param g is the graph to be printed
    */
   explicit FunctionEdgeWriter(const CallGraph* call_graph);

   /**
    * Destructor
    */
   ~FunctionEdgeWriter() override;

   /**
    * operator function returning the edge description
    * @param out is the output stream
    * @param e is the edge
    */
   void operator()(std::ostream& out, const EdgeDescriptor& e) const override;
};
#endif
