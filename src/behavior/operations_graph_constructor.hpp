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
 * @file operations_graph_constructor.hpp
 * @brief This class provides methods to build an operations graph.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#ifndef OPERATIONS_GRAPH_CONSTRUCTOR_HPP
#define OPERATIONS_GRAPH_CONSTRUCTOR_HPP

#include "custom_map.hpp"
#include "op_graph.hpp"
#include "refcount.hpp"

#include <string>

enum class VariableAccessType;
enum class VariableType;
REF_FORWARD_DECL(ir_manager);

/**
 * class providing methods to manage an operations graph.
 */
class operations_graph_constructor
{
   /// reference to the bulk operations graph
   OpGraphsCollection& og;

   /// Mapping between id to index
   std::map<std::string, OpGraph::vertex_descriptor> index_map;

 public:
   /**
    * Return the vertex index given the id of the vertex node.
    * @param source is the name of the vertex.
    * @return the index associated with the source.
    */
   OpGraph::vertex_descriptor getIndex(const std::string& source);

   /**
    * Return the vertex index given the id of the vertex node.
    * @param source is the name of the vertex.
    * @return the index associated with the source.
    */
   OpGraph::vertex_descriptor CgetIndex(const std::string& source) const;

   /**
    * add an edge between vertex source and vertex dest
    * @param source is the source vertex
    * @param dest is the dest vertexes
    * @param selector is the type of the edge
    */
   OpGraph::edge_descriptor AddEdge(OpGraph::vertex_descriptor source, OpGraph::vertex_descriptor dest, int selector);

   /**
    * remove a selector between two vertices
    * @param source is the source vertex
    * @param dest is the dest vertexes
    * @param selector is the type of the edge
    */
   void RemoveEdge(OpGraph::vertex_descriptor source, OpGraph::vertex_descriptor dest, int selector);

   /**
    * set the selector of an edge between vertex source and vertex dest
    * @param edge is the edge descriptor from which the selector has to be removed
    * @param selector is the selector to be removed
    */
   void RemoveSelector(const OpGraph::edge_descriptor& edge, const int selector);

   /**
    * Remove all redundant edges
    */
   void CompressEdges();

   /**
    * Remove all vertices and edges
    */
   void Clear();

   /**
    * Add edge info to the graph.
    * @param src is an unique id representing the source node.
    * @param tgt is an unique id representing the target node.
    * @param selector is the family of the edge. See cdfg_edge_info class for details.
    * @param NodeID is the NodeID of the variable carrying the data through the edge.
    */
   void add_edge_info(OpGraph::vertex_descriptor src, OpGraph::vertex_descriptor tgt, const int selector,
                      unsigned int NodeID);

   /**
    * Add the operation associated with a vertex.
    * @param TM is the IR manager
    * @param src is the vertex name at which the operation is associated.
    * @param oper is a string representing the operation associated with `src`.
    * @param bb_index is the basic block index associated with the operation.
    * @param node_id is the index of the IR node
    */
   void AddOperation(const ir_managerRef TM, const std::string& src, const std::string& oper, unsigned int bb_index,
                     const unsigned int node_id);

   /**
    * Add the type associated with a vertex.
    * @param src is the vertex name at which the type is associated.
    * @param type is an unsigned int representing the type associated with `src`.
    */
   void add_type(const std::string& src, unsigned int type);

   /**
    * Constructor.
    * @param _og is the collection of operations graphs managed by this helper
    */
   explicit operations_graph_constructor(OpGraphsCollection& _og);

   /**
    * Adds an access to a variable to an operation vertex
    * @param op_vertex is the operation to be considered
    * @param variable is the index of the variable
    * @param variable_type is the type of the variable
    * @param access_type is the type of the access
    */
   void AddVariable(OpGraph::vertex_descriptor op_vertex, const unsigned int variable, const VariableType variable_type,
                    const VariableAccessType access_type);

   /**
    * Adds a (ssa-)variable to the set of variables referred by the operation vertex
    * @param Ver is the operation vertex
    * @param Vargc is the node id associated with the variable referred by the operation vertex
    */
   void AddSourceCodeVariable(OpGraph::vertex_descriptor Ver, unsigned int Vargc);

   /**
    * Adds a parameter to the vertex
    * @param Ver is the operation vertex
    * @param Vargc is the node id associated with the variable referred by the operation vertex
    */
   void add_parameter(OpGraph::vertex_descriptor Ver, unsigned int Vargc);

   /**
    * Adds a call to the vertex
    * @param source is the vertex name at which the type is associated.
    * @param called_function is the index of the called function
    */
   void add_called_function(const std::string& source, unsigned int called_function);
};

#endif
