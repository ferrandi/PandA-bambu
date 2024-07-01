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
 * @file operations_graph_constructor.cpp
 * @brief This class provides methods to build a operations graph.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

/// Header include
#include "operations_graph_constructor.hpp"
#include "custom_map.hpp"        // for CustomMap
#include "custom_set.hpp"        // for CustomSet
#include "exceptions.hpp"        // for THROW_ASSERT
#include "function_behavior.hpp" // for tree_nodeRef, Funct...
#include "op_graph.hpp"
#include "string_manipulation.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "typed_node_info.hpp"   // for GET_NAME, ENTRY, EXIT
#include <boost/tuple/tuple.hpp> // for tie
#include <list>                  // for list
#include <utility>               // for pair

operations_graph_constructor::operations_graph_constructor(OpGraphsCollectionRef _og)
    : og(std::move(_og)), op_graph(new OpGraph(og, -1))
{
}

operations_graph_constructor::~operations_graph_constructor() = default;

void operations_graph_constructor::Clear()
{
   og->Clear();
   index_map.clear();
}

vertex operations_graph_constructor::getIndex(const std::string& source)
{
   if(index_map.find(source) != index_map.end())
   {
      return index_map.find(source)->second;
   }
   NodeInfoRef node_info(new OpNodeInfo());
   GetPointerS<OpNodeInfo>(node_info)->vertex_name = source;
   const vertex v_og = og->AddVertex(node_info);
   index_map[source] = v_og;
   return index_map[source];
}

vertex operations_graph_constructor::CgetIndex(const std::string& source) const
{
   THROW_ASSERT(index_map.find(source) != index_map.end(), "Index with name " + source + " doesn't exist");
   return index_map.find(source)->second;
}

EdgeDescriptor operations_graph_constructor::AddEdge(const vertex source, const vertex dest, int selector)
{
   return og->AddEdge(source, dest, selector);
}

void operations_graph_constructor::RemoveEdge(const vertex source, const vertex dest, int selector)
{
   og->RemoveSelector(source, dest, selector);
}

void operations_graph_constructor::RemoveSelector(const EdgeDescriptor edge, const int selector)
{
   og->RemoveSelector(edge, selector);
}

void operations_graph_constructor::CompressEdges()
{
   og->CompressEdges();
}

void operations_graph_constructor::add_edge_info(const vertex src, const vertex tgt, const int selector,
                                                 unsigned int NodeID)
{
   EdgeDescriptor e;
   bool inserted;
   boost::tie(e, inserted) = boost::edge(src, tgt, *og);
   THROW_ASSERT(inserted, "Edge from " + GET_NAME(og, src) + " to " + GET_NAME(og, tgt) + " doesn't exists");
   get_edge_info<OpEdgeInfo>(e, *(og))->add_nodeID(NodeID, selector);
}

void operations_graph_constructor::AddOperation(const tree_managerRef TM, const std::string& src,
                                                const std::string& operation_t, unsigned int bb_index,
                                                const unsigned int node_id)
{
   THROW_ASSERT(src != "", "Vertex name empty");
   THROW_ASSERT(operation_t != "", "Operation empty");
   vertex current = getIndex(src);
   THROW_ASSERT(!op_graph->CGetOpNodeInfo(current)->node || node_id == 0 ||
                    node_id == op_graph->CGetOpNodeInfo(current)->GetNodeId(),
                "Trying to set node_id " + STR(node_id) + " to vertex " + src + " that has already node_id " +
                    STR(op_graph->CGetOpNodeInfo(current)->GetNodeId()));
   if(node_id > 0 && node_id != ENTRY_ID && node_id != EXIT_ID)
   {
      op_graph->GetOpNodeInfo(current)->node = TM->GetTreeNode(node_id);
   }
   const unsigned int updated_node_id = op_graph->GetOpNodeInfo(current)->GetNodeId();
   if(updated_node_id != 0 && updated_node_id != ENTRY_ID && updated_node_id != EXIT_ID)
   {
      GetPointerS<gimple_node>(TM->GetTreeNode(updated_node_id))->operation = operation_t;
   }
   GET_NODE_INFO(og, OpNodeInfo, current)->bb_index = bb_index;
   if(src == ENTRY)
   {
      op_graph->GetOpGraphInfo()->entry_vertex = current;
   }
   if(src == EXIT)
   {
      op_graph->GetOpGraphInfo()->exit_vertex = current;
   }
   op_graph->GetOpGraphInfo()->tree_node_to_operation[node_id] = current;
}

void operations_graph_constructor::add_type(const std::string& src, unsigned int type_t)
{
   THROW_ASSERT(src != "", "Vertex name empty");
   THROW_ASSERT(type_t != 0, "Type of vertex " + src + " is zero");
   vertex src_index = getIndex(src);
   if(GET_NODE_INFO(og, OpNodeInfo, src_index)->node_type != TYPE_GENERIC)
   {
      GET_NODE_INFO(og, OpNodeInfo, src_index)->node_type |= type_t;
   }
   else
   {
      GET_NODE_INFO(og, OpNodeInfo, src_index)->node_type = type_t;
   }
}

void operations_graph_constructor::AddVariable(const vertex op_vertex, const unsigned int variable,
                                               const FunctionBehavior_VariableType variable_type,
                                               const FunctionBehavior_VariableAccessType access_type)
{
   op_graph->GetOpNodeInfo(op_vertex)->variables[variable_type][access_type].insert(variable);
}

void operations_graph_constructor::add_parameter(const vertex& Ver, unsigned int Var)
{
   op_graph->GetOpNodeInfo(Ver)->actual_parameters.push_back(Var);
}

void operations_graph_constructor::add_called_function(const std::string& source, unsigned int called_function)
{
   op_graph->GetOpNodeInfo(getIndex(source))->called.insert(called_function);
}

void operations_graph_constructor::AddSourceCodeVariable(const vertex& Ver, unsigned int Vargc)
{
   op_graph->GetOpNodeInfo(Ver)->cited_variables.insert(Vargc);
}
