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
 * @file call_graph.hpp
 * @brief Call graph hierarchy.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 *
 */
#ifndef CALL_GRAPH_HPP
#define CALL_GRAPH_HPP
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "edge_info.hpp"
#include "graph.hpp"
#include "graph_info.hpp"
#include "node_info.hpp"
#include "refcount.hpp"

#include <iostream>
#include <string>

REF_FORWARD_DECL(FunctionBehavior);

#define STD_SELECTOR (1 << 0)
#define FEEDBACK_SELECTOR (1 << 1)

/**
 * Information associated with a call_graph node.
 */
struct FunctionNodeInfo : public NodeInfo
{
   /// this is the nodeID of the function associated with the vertex
   unsigned int nodeID{0};
};

/**
 * Information associated with a call_graph edge.
 */
struct FunctionEdgeInfo : public EdgeInfo
{
   enum class CallType
   {
      direct_call,
      indirect_call,
      function_address,
      call_any
   };

   /// the index of the statements of the caller function where the target is called;
   CustomOrderedSet<unsigned int> direct_call_points;
   CustomOrderedSet<unsigned int> indirect_call_points;
   CustomOrderedSet<unsigned int> function_addresses;
};

/**
 * The info associated with the call graph
 */
struct CallGraphInfo : public GraphInfo
{
   /// reference to the behaviors
   std::map<unsigned int, FunctionBehaviorRef> behaviors;
};

using CallGraphsCollection = graphs_collection<FunctionNodeInfo, FunctionEdgeInfo, CallGraphInfo>;

/**
 * This class is the view of a call graph
 */
class CallGraph : public graph<CallGraphsCollection>
{
 public:
   /**
    * Constructor
    * @param call_graphs_collection is the starting call graphs collection
    * @param selector is the selector of the view
    */
   CallGraph(const CallGraphsCollection& call_graphs_collection, const int selector);

   /**
    * Constructor
    * @param call_graphs_collection is the starting call graphs collection
    * @param selector is the selector of the view
    * @param vertices is the set of vertices to be considered
    */
   CallGraph(const CallGraphsCollection& call_graphs_collection, const int selector,
             const CustomUnorderedSet<vertex_descriptor>& vertices);

   /**
    * Write the call graph in dot format
    * @param file_name is the name of the file to create
    */
   void writeDot(const std::filesystem::path& file_name) const;
};

/**
 * Functor used by write_graphviz to write the label of the vertices of a function graph
 */
class FunctionVertexWriter : public VertexWriter<CallGraph>
{
   /// reference to the behaviors
   const std::map<unsigned int, FunctionBehaviorRef>& behaviors;

 public:
   /**
    * constructor
    * @param call_graph is the graph to be printed
    */
   explicit FunctionVertexWriter(const CallGraph& call_graph);

   /**
    * operator function returning the label of the vertex
    * @param out is the output stream
    * @param v is the vertex
    */
   void operator()(std::ostream& out, CallGraph::vertex_descriptor v) const override;
};

/**
 * Functor used by write_graphviz to write the edges of a function graph
 */
class FunctionEdgeWriter : public EdgeWriter<CallGraph>
{
   /// reference to the behaviors
   const std::map<unsigned int, FunctionBehaviorRef>& behaviors;

 public:
   /**
    * constructor
    * @param call_graph is the graph to be printed
    */
   explicit FunctionEdgeWriter(const CallGraph& call_graph);

   /**
    * operator function returning the edge description
    * @param out is the output stream
    * @param e is the edge
    */
   void operator()(std::ostream& out, const CallGraph::edge_descriptor& e) const override;
};
#endif
