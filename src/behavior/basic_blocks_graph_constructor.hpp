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
 * @file basic_blocks_graph_constructor.hpp
 * @brief This class provides methods to build a basic blocks graph.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef BASIC_BLOCKS_GRAPH_CONSTRUCTOR_HPP
#define BASIC_BLOCKS_GRAPH_CONSTRUCTOR_HPP

#include "custom_map.hpp"
#include "graph.hpp"
#include "refcount.hpp"

/**
 * @name Forward declarations
 */
//@{
REF_FORWARD_DECL(BasicBlocksGraphConstructor);
REF_FORWARD_DECL(BBGraph);
REF_FORWARD_DECL(BBGraphsCollection);
REF_FORWARD_DECL(bloc);
//@}

/**
 * class providing methods to manage a basic blocks graph.
 */
class BasicBlocksGraphConstructor
{
 private:
   /// reference to the bulk basic blocks graph
   const BBGraphsCollectionRef bg;

   /// Reference to graph with all the edges
   const BBGraphRef bb_graph;

   /// Map between basic block node index and vertices
   CustomUnorderedMap<unsigned int, vertex>& bb_index_map;

 public:
   /**
    * Add a new vertex to the basic blocks graphs
    * @param info is the bloc associated with basic block node
    * @return the vertex just added
    */
   vertex add_vertex(const blocRef info);

   /**
    * Add an edge selector
    * @param source is the source vertex
    * @param target is the target vertexes
    * @param selector is the type of the edge
    */
   EdgeDescriptor AddEdge(const vertex source, const vertex target, const int selector);

   /**
    * Remove an edge selector
    * @param source is the source vertex
    * @param target is the target vertexes
    * @param selector is the type of the edge
    */
   void RemoveEdge(const vertex source, const vertex target, const int selector);

   /**
    * Remove an edge selector
    * @param edge is the edge to be removed
    * @param sel is the selector
    */
   void RemoveEdge(const EdgeDescriptor edge, const int selector);

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
   void add_bb_edge_info(const vertex source, const vertex target, int type, const unsigned int label);

   /**
    * add edge between source and exit
    * @param source is the vertex to connect with exit
    */
   EdgeDescriptor connect_to_exit(const vertex source);

   /**
    * add edge between entry and target
    * @param target is the vertex to which connect entry
    */
   EdgeDescriptor connect_to_entry(const vertex target);

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
   vertex Cget_vertex(unsigned int block_index) const;

   /**
    * Add an operation to its basic block
    * @param op is the name of the operation vertex
    * @param index is the index of the basic blocks
    */
   void add_operation_to_bb(vertex op, unsigned int index);

   /**
    * Constructor.
    * @param _og is the reference to the bulk operations graph.
    */
   explicit BasicBlocksGraphConstructor(BBGraphsCollectionRef _bg);

   /**
    * Destructor.
    */
   ~BasicBlocksGraphConstructor();
};

#endif
