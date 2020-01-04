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
 * @file typed_node_info.hpp
 * @brief Base class description of data information associated with each node of a graph.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef TYPED_NODE_INFO_HPP
#define TYPED_NODE_INFO_HPP

/// Superclass include
#include "node_info.hpp"

/// Graph include
#include "graph.hpp"

/// STD include
#include <ostream>

/// Utility include
#include "refcount.hpp"

/**
 * constant string identifying an operation node of type entry. Used during the behavioral_manager building starting from the tree(GCC) data structure.
 */
#define ENTRY "ENTRY"

/**
 * constant string identifying an operation node of type exit. Used during the behavioral_manager building starting from the tree(GCC) data structure.
 */
#define EXIT "EXIT"

/**
 * constant identifying the node type of an entry node.
 */
#define TYPE_ENTRY 2

/**
 * constant identifying the node type of an exit node.
 */
#define TYPE_EXIT 4

/**
 * constant identifying the node type of a GENERIC operation.
 */
#define TYPE_GENERIC 8

/**
 * Base class storing user data information.
 * This class is associated with the graph data structure.
 */

/// FIXME: change in function of graph
/**
 * Helper function checking if a node is an entry node.
 * @param g is the graph.
 * @param node is the examined node.
 * @return true when node is an entry node.
 */
template <class Graph>
bool is_entry_node(const Graph* g, typename boost::graph_traits<Graph>::vertex_descriptor node)
{
   return (GET_TYPE(g, node) == TYPE_ENTRY && boost::in_degree(node, *g) == 0);
}

/**
 * Helper function checking if a node is an exit node.
 * @param g is the graph.
 * @param node is the examined node.
 * @return true when node is an exit node.
 */
template <class Graph>
bool is_exit_node(const Graph* g, typename boost::graph_traits<Graph>::vertex_descriptor node)
{
   return (GET_TYPE(g, node) == TYPE_EXIT && boost::out_degree(node, *g) == 0);
}

struct TypedNodeInfo : public NodeInfo
{
   /**
    * Custom vertex property: node_operation.
    * This property defines which operation is performed: assignment, addition, comparison, etc.
    */
   std::string node_operation;

   /**
    * Definition of the node name property.
    */
   std::string vertex_name;

   /**
    * Custom vertex property: node_type.
    * This property defines which type of node is: read and write a port, read a constant, if, case, wait and notify or generic operation.
    */
   unsigned int node_type;

   /**
    * Constructor
    */
   TypedNodeInfo();

   /**
    * Destructor
    */
   ~TypedNodeInfo() override;

   /**
    * Print the information associated with the node of the graph.
    * @param os is the output stream.
    */
   void print(std::ostream&, int detail_level = 0) const override;

   /**
    * Friend definition of the << operator.
    * @param os is the output stream.
    * @param s is the node to print.
    */
   friend std::ostream& operator<<(std::ostream& os, const TypedNodeInfo& s)
   {
      s.print(os);
      return os;
   }
};

/**
 * Helper macro returning the operation associated with a node. This function should be carefully used. For example it can only be used by cdfg based graphs and not by basic_block based graphs.
 * @param data is the graph.
 * @param vertex_index is the index of the node.
 */
#define GET_OPERATION(data, vertex_index) Cget_node_info<TypedNodeInfo>(vertex_index, *(data))->node_operation

/**
 * Helper macro returning the name associated with a node. This function should be carefully used. For example it can only be used by cdfg based graphs and not by basic_block based graphs.
 * @param data is the graph.
 * @param vertex_index is the index of the node.
 */
#define GET_NAME(data, vertex_index) Cget_node_info<TypedNodeInfo>(vertex_index, *(data))->vertex_name

/**
 * Helper macro returning the type associated with a node. This function should be carefully used. For example it can only be used by cdfg based graphs and not by basic_block based graphs.
 * @param data is the graph.
 * @param vertex_index is the index of the node.
 */
#define GET_TYPE(data, vertex_index) Cget_node_info<TypedNodeInfo>(vertex_index, *(data))->node_type
#endif
