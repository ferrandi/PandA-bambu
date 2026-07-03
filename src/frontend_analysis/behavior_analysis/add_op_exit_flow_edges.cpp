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
 * @file add_op_exit_flow_edges.hpp
 * @brief Analysis step which adds flow edges for scheduling to operation graphs
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#include "add_op_exit_flow_edges.hpp"

#include "Parameter.hpp"
#include "basic_block.hpp"
#include "behavioral_helper.hpp"
#include "dbgPrintHelper.hpp"
#include "function_behavior.hpp"
#include "hash_helper.hpp"
#include "op_graph.hpp"
#include "string_manipulation.hpp"

AddOpExitFlowEdges::AddOpExitFlowEdges(const ParameterConstRef _parameters, const application_managerRef _AppM,
                                       unsigned int _function_id, const DesignFlowManager& _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, ADD_OP_EXIT_FLOW_EDGES, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
AddOpExitFlowEdges::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(OPERATIONS_CFG_COMPUTATION, SAME_FUNCTION));
         relationships.insert(std::make_pair(OP_ORDER_COMPUTATION, SAME_FUNCTION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}

void AddOpExitFlowEdges::Initialize()
{
   if(bb_version != 0 and bb_version != function_behavior->GetBBVersion())
   {
      const auto flg = function_behavior->GetOpGraph(FunctionBehavior::FLG);
      if(flg.num_vertices() != 0)
      {
         for(const auto& edge : flg.edges())
         {
            if((flg.CGetNodeInfo(flg.target(edge)).node_type & TYPE_LAST_OP) != 0)
            {
               function_behavior->ogc->RemoveSelector(edge, FLG_SELECTOR);
            }
         }
      }
   }
}

DesignFlowStep_Status AddOpExitFlowEdges::InternalExec()
{
   /// The control flow graph of operation
   const auto fcfg = function_behavior->GetOpGraph(FunctionBehavior::FCFG);

   /// The control flow graph of basic block
   const auto basic_block_graph = function_behavior->GetBBGraph(FunctionBehavior::BB);

   /// Adding operation to empty return
   for(const auto& v : fcfg.vertices())
   {
      const auto& v_info = fcfg.CGetNodeInfo(v);
      if((v_info.node_type & TYPE_LAST_OP) != 0)
      {
         for(const auto operation :
             basic_block_graph.CGetNodeInfo(basic_block_graph.CGetGraphInfo().bb_index_map.at(v_info.bb_index))
                 .statements_list)
         {
            const auto reachability = function_behavior->CheckReachability(operation, v);
            if(reachability && ((fcfg.CGetNodeInfo(operation).node_type & TYPE_LAST_OP) == 0))
            {
               function_behavior->ogc->AddEdge(operation, v, FLG_SELECTOR);
            }
         }
      }
   }
   if(parameters->getOption<bool>(OPT_print_dot))
   {
      const auto dot_path = function_behavior->GetDotPath();
      function_behavior->GetOpGraph(FunctionBehavior::FLG).writeDot(dot_path / "OP_FL.dot");
      function_behavior->GetOpGraph(FunctionBehavior::FFLSAODG).writeDot(dot_path / "OP_FFLSAODG.dot");
   }
   return DesignFlowStep_Status::SUCCESS;
}
