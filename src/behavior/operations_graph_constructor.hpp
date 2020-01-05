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
 * @file operations_graph_constructor.hpp
 * @brief This class provides methods to build an operations graph.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef OPERATIONS_GRAPH_CONSTRUCTOR_HPP
#define OPERATIONS_GRAPH_CONSTRUCTOR_HPP

/// Autoheader include
#include "config_HAVE_EXPERIMENTAL.hpp"
#include "custom_map.hpp" // for map
#include <string>         // for string

#include "graph.hpp"
#include "refcount.hpp"
#include "strong_typedef.hpp"

/**
 * @name Forward declarations
 */
//@{
enum class FunctionBehavior_VariableAccessType;
enum class FunctionBehavior_VariableType;
STRONG_TYPEDEF_FORWARD_DECL(unsigned int, MemoryAddress);
REF_FORWARD_DECL(operations_graph_constructor);
REF_FORWARD_DECL(OpGraph);
REF_FORWARD_DECL(OpGraphsCollection);
REF_FORWARD_DECL(tree_manager);
//@}

/**
 * class providing methods to manage an operations graph.
 */
class operations_graph_constructor
{
 private:
   /// reference to the bulk operations graph
   const OpGraphsCollectionRef og;

   /// The graph with all the edges
   const OpGraphRef op_graph;

   /// Mapping between id to index
   std::map<std::string, vertex> index_map;

 public:
   /**
    * Return the vertex index given the id of the vertex node.
    * @param source is the name of the vertex.
    * @return the index associated with the source.
    */
   vertex getIndex(const std::string& source);

   /**
    * Return the vertex index given the id of the vertex node.
    * @param source is the name of the vertex.
    * @return the index associated with the source.
    */
   vertex CgetIndex(const std::string& source) const;

   /**
    * add an edge between vertex source and vertex dest
    * @param source is the source vertex
    * @param dest is the dest vertexes
    * @param selector is the type of the edge
    */
   EdgeDescriptor AddEdge(const vertex source, const vertex dest, int selector);

   /**
    * remove a selector between two vertices
    * @param source is the source vertex
    * @param dest is the dest vertexes
    * @param selector is the type of the edge
    */
   void RemoveEdge(const vertex source, const vertex dest, int selector);

   /**
    * set the selector of an edge between vertex source and vertex dest
    * @param edge is the edge descriptor from which the selector has to be removed
    * @param selector is the selector to be removed
    */
   void RemoveSelector(const EdgeDescriptor edge, const int selector);

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
    * @param ef is the family of the edge. See cdfg_edge_info class for details.
    * @param NodeID is the NodeID of the variable carrying the data through the edge.
    */
   void add_edge_info(const vertex src, const vertex tgt, const int selector, unsigned int NodeID);

   /**
    * Add the operation associated with a vertex.
    * @param TM is the tree manager
    * @param source is the vertex name at which the operation is associated.
    * @param oper is a string representing the operation associated with source.
    * @param bb_index is the basic block index associated with the operation.
    * @param node_id is the index of the tree node
    */
   void AddOperation(const tree_managerRef TM, const std::string& source, const std::string& oper, unsigned int bb_index, const unsigned int node_id);

   /**
    * Add the type associated with a vertex.
    * @param source is the vertex name at which the type is associated.
    * @param type is an unsigned int representing the type associated with source.
    */
   void add_type(const std::string& source, unsigned int type);

   /**
    * Constructor.
    * @param og is the collection of operations graph
    */
   explicit operations_graph_constructor(OpGraphsCollectionRef _og);

   /**
    * Destructor.
    */
   ~operations_graph_constructor();

   /**
    * Adds an access to a variable to an operation vertex
    * @param op_vertex is the operation to be considered
    * @param variable is the index of the variable
    * @param type is the type of the variable
    * @param access_type is the type of the access
    */
   void AddVariable(const vertex op_vertex, const unsigned int variable, const FunctionBehavior_VariableType variable_type, const FunctionBehavior_VariableAccessType access_type);

#if HAVE_EXPERIMENTAL
   /**
    * Adds an access to a memory location to an operation vertex
    * @param op_vertex is the operation to be considered
    * @param variable is the index of the variable
    * @param access_type is the type of the access
    */
   void AddMemoryLocation(const vertex op_vertex, const MemoryAddress variable, const FunctionBehavior_VariableAccessType access_type);
#endif

   /**
    * Adds a (ssa-)variable to the set of variables referred by the operation vertex
    * @param Ver is the operation vertex
    * @param Var is the node id associated to the variable referred by the operation vertex
    */
   void AddSourceCodeVariable(const vertex& Ver, unsigned int Vargc);

   /**
    * Adds a parameter to the vertex
    * @param Ver is the operation vertex
    * @param Var is the node id associated to the variable referred by the operation vertex
    */
   void add_parameter(const vertex& Ver, unsigned int Vargc);

   /**
    * Adds a call to the vertex
    * @param source is the vertex name at which the type is associated.
    * @param called is the called function
    */
   void add_called_function(const std::string& source, unsigned int called_function);
};

#endif
