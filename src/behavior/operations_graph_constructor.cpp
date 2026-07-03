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
 * @file operations_graph_constructor.cpp
 * @brief This class provides methods to build a operations graph.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#include "operations_graph_constructor.hpp"

#include "custom_map.hpp"
#include "custom_set.hpp"
#include "exceptions.hpp"
#include "function_behavior.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"
#include "op_graph.hpp"
#include "string_manipulation.hpp"
#include "typed_node_info.hpp"

#include <boost/tuple/tuple.hpp>

#include <list>
#include <utility>

operations_graph_constructor::operations_graph_constructor(OpGraphsCollection& _og) : og(_og)
{
}

void operations_graph_constructor::Clear()
{
   og.Clear();
   index_map.clear();
}

OpGraph::vertex_descriptor operations_graph_constructor::getIndex(const std::string& source)
{
   if(index_map.find(source) != index_map.end())
   {
      return index_map.find(source)->second;
   }
   OpNodeInfo node_info;
   node_info.vertex_name = source;
   const auto v_og = og.AddVertex(node_info);
   index_map[source] = v_og;
   return index_map[source];
}

OpGraph::vertex_descriptor operations_graph_constructor::CgetIndex(const std::string& source) const
{
   THROW_ASSERT(index_map.find(source) != index_map.end(), "Index with name " + source + " doesn't exist");
   return index_map.find(source)->second;
}

OpGraph::edge_descriptor operations_graph_constructor::AddEdge(OpGraph::vertex_descriptor source,
                                                               OpGraph::vertex_descriptor dest, int selector)
{
   return og.AddEdge(source, dest, selector);
}

void operations_graph_constructor::RemoveEdge(OpGraph::vertex_descriptor source, OpGraph::vertex_descriptor dest,
                                              int selector)
{
   og.RemoveSelector(source, dest, selector);
}

void operations_graph_constructor::RemoveSelector(const OpGraph::edge_descriptor& edge, const int selector)
{
   og.RemoveSelector(edge, selector);
}

void operations_graph_constructor::CompressEdges()
{
   og.CompressEdges();
}

void operations_graph_constructor::add_edge_info(OpGraph::vertex_descriptor src, OpGraph::vertex_descriptor tgt,
                                                 const int selector, unsigned int NodeID)
{
   const auto [e, inserted] = boost::edge(src, tgt, og);
   THROW_ASSERT(inserted, "Edge from " + og.CGetNodeInfo(src).vertex_name + " to " + og.CGetNodeInfo(tgt).vertex_name +
                              " doesn't exists");
   og.GetEdgeInfo(e).add_nodeID(NodeID, selector);
}

void operations_graph_constructor::AddOperation(const ir_managerRef TM, const std::string& src,
                                                const std::string& operation_t, unsigned int bb_index,
                                                const unsigned int node_id)
{
   THROW_ASSERT(src != "", "Vertex name empty");
   THROW_ASSERT(operation_t != "", "Operation empty");
   auto current = getIndex(src);
   auto& node_info = og.GetNodeInfo(current);
   THROW_ASSERT(!node_info.node || node_id == 0 || node_id == node_info.GetNodeId(),
                "Trying to set node_id " + STR(node_id) + " to vertex " + src + " that has already node_id " +
                    STR(node_info.GetNodeId()));
   if(node_id > 0 && node_id != ENTRY_ID && node_id != EXIT_ID)
   {
      og.GetNodeInfo(current).node = TM->GetIRNode(node_id);
   }
   const auto updated_node_id = node_info.GetNodeId();
   if(updated_node_id != 0 && updated_node_id != ENTRY_ID && updated_node_id != EXIT_ID)
   {
      GetPointerS<node_stmt>(TM->GetIRNode(updated_node_id))->operation = operation_t;
   }
   node_info.bb_index = bb_index;
   if(src == ENTRY)
   {
      og.GetGraphInfo().entry_vertex = current;
   }
   if(src == EXIT)
   {
      og.GetGraphInfo().exit_vertex = current;
   }
   og.GetGraphInfo().ir_node_to_operation[node_id] = current;
}

void operations_graph_constructor::add_type(const std::string& src, unsigned int type_t)
{
   THROW_ASSERT(src != "", "Vertex name empty");
   THROW_ASSERT(type_t != 0, "Type of vertex " + src + " is zero");
   auto src_index = getIndex(src);
   auto& node_info = og.GetNodeInfo(src_index);
   if(node_info.node_type != TYPE_GENERIC)
   {
      node_info.node_type |= type_t;
   }
   else
   {
      node_info.node_type = type_t;
   }
}

void operations_graph_constructor::AddVariable(OpGraph::vertex_descriptor op_vertex, const unsigned int variable,
                                               const VariableType variable_type, const VariableAccessType access_type)
{
   og.GetNodeInfo(op_vertex).AddVariable(variable_type, access_type, variable);
   if(access_type == VariableAccessType::DEFINITION)
   {
      THROW_ASSERT(og.GetGraphInfo().SSA2Def.find(variable) == og.GetGraphInfo().SSA2Def.end() ||
                       og.GetGraphInfo().SSA2Def.find(variable)->second == op_vertex,
                   "unexpected condition: multiple vertex defining the same variable: " + STR(variable));
      og.GetGraphInfo().SSA2Def.emplace(variable, op_vertex);
   }
}

void operations_graph_constructor::add_parameter(OpGraph::vertex_descriptor Ver, unsigned int Var)
{
   og.GetNodeInfo(Ver).actual_parameters.push_back(Var);
}

void operations_graph_constructor::add_called_function(const std::string& source, unsigned int called_function)
{
   og.GetNodeInfo(getIndex(source)).called.insert(called_function);
}

void operations_graph_constructor::AddSourceCodeVariable(OpGraph::vertex_descriptor Ver, unsigned int Vargc)
{
   og.GetNodeInfo(Ver).cited_variables.insert(Vargc);
}
