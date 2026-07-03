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
 * @file basic_blocks_graph_constructor.hpp
 * @brief This class provides methods to build a basic blocks graph.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#ifndef BASIC_BLOCKS_GRAPH_CONSTRUCTOR_HPP
#define BASIC_BLOCKS_GRAPH_CONSTRUCTOR_HPP

#include "basic_block.hpp"
#include "custom_map.hpp"
#include "refcount.hpp"

REF_FORWARD_DECL(bloc);

/**
 * class providing methods to manage a basic blocks graph.
 */
class BasicBlocksGraphConstructor
{
 private:
   /// reference to the bulk basic blocks graph
   BBGraphsCollection& bg;

   /// Map between basic block node index and vertices
   CustomUnorderedMap<unsigned int, BBGraph::vertex_descriptor>& bb_index_map;

 public:
   /**
    * Constructor.
    * @param _bg is the reference to the bulk operations graph.
    */
   explicit BasicBlocksGraphConstructor(BBGraphsCollection& _bg);

   /**
    * Add a new vertex to the basic blocks graphs
    * @param info is the bloc associated with basic block node
    * @return the vertex just added
    */
   BBGraph::vertex_descriptor add_vertex(const blocRef info);

   /**
    * Add an edge selector
    * @param source is the source vertex
    * @param target is the target vertex
    * @param selector is the type of the edge
    */
   BBGraph::edge_descriptor AddEdge(BBGraph::vertex_descriptor source, BBGraph::vertex_descriptor target,
                                    const int selector);

   /**
    * Remove an edge selector
    * @param source is the source vertex
    * @param target is the target vertex
    * @param selector is the type of the edge
    */
   void RemoveEdge(BBGraph::vertex_descriptor source, BBGraph::vertex_descriptor target, const int selector);

   /**
    * Remove an edge selector
    * @param edge is the edge to be removed
    * @param selector is the selector
    */
   void RemoveEdge(const BBGraph::edge_descriptor& edge, const int selector);

   /**
    * Remove all vertices and edges
    */
   void Clear();

   /**
    * add label to edge between vertex source and vertex target
    * @param source is the source vertex
    * @param target is the target vertexes
    * @param type is the type of the label
    * @param label is the label to be added
    */
   void add_bb_edge_info(BBGraph::vertex_descriptor source, BBGraph::vertex_descriptor target, int type,
                         const unsigned int label);

   /**
    * add edge between source and exit
    * @param source is the vertex to connect with exit
    */
   BBGraph::edge_descriptor connect_to_exit(BBGraph::vertex_descriptor source);

   /**
    * add edge between entry and target
    * @param target is the vertex to which connect entry
    */
   BBGraph::edge_descriptor connect_to_entry(BBGraph::vertex_descriptor target);

   /**
    * return true in case the vertex has been already created
    * @param block_index is the basic block identifier
    */
   bool check_vertex(unsigned int block_index) const;

   /**
    * return a vertex of the graph given the functionID.
    * if vertex does not exist throw error
    * @param block_index is the function identifier
    */
   BBGraph::vertex_descriptor Cget_vertex(unsigned int block_index) const;

   /**
    * Add an operation to its basic block
    * @param op is the name of the operation vertex
    * @param index is the index of the basic blocks
    */
   void add_operation_to_bb(BBGraph::vertex_descriptor op, unsigned int index);
};

#endif
