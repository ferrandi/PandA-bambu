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
 * @file op_feedback_edges_computation.cpp
 * @brief Analysis step computing Analysis step computing feedback edges for operation control flow graph
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#include "op_feedback_edges_computation.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "behavioral_helper.hpp"
#include "dbgPrintHelper.hpp"
#include "function_behavior.hpp"
#include "ir_basic_block.hpp"
#include "ir_manager.hpp"
#include "loop.hpp"
#include "loops.hpp"
#include "op_graph.hpp"
#include "string_manipulation.hpp"

op_feedback_edges_computation::op_feedback_edges_computation(const ParameterConstRef _parameters,
                                                             const application_managerRef _AppM,
                                                             unsigned int _function_id,
                                                             const DesignFlowManager& _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, OP_FEEDBACK_EDGES_IDENTIFICATION, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
op_feedback_edges_computation::ComputeFrontendRelationships(
    const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(LOOPS_COMPUTATION, SAME_FUNCTION));
         relationships.insert(std::make_pair(OPERATIONS_CFG_COMPUTATION, SAME_FUNCTION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
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

DesignFlowStep_Status op_feedback_edges_computation::InternalExec()
{
   const auto fbb = function_behavior->GetBBGraph(FunctionBehavior::FBB);
   /// then consider loops
   const auto& loops = function_behavior->getConstLoops()->getList();
   for(const auto& loop : loops)
   {
      if(loop->getLoopId() == 0)
      {
         continue;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing loop " + STR(loop->getLoopId()));
      for(auto sp_back_edge : loop->getBackEdges())
      {
         const auto& from_bb = sp_back_edge.first;
         const auto& to_bb = sp_back_edge.second;
         const auto& bb_node_info = fbb.CGetNodeInfo(to_bb);
         auto label_vertex = bb_node_info.statements_list.front();
         const auto bb_node_info_from = fbb.CGetNodeInfo(from_bb);
         THROW_ASSERT(bb_node_info_from.statements_list.size(),
                      "Empty block " + std::to_string(bb_node_info_from.block->number));
         auto goto_vertex = bb_node_info_from.statements_list.back();
         /// add the feedback control dependence and the feedback control flow graph edges
         function_behavior->ogc->RemoveEdge(goto_vertex, label_vertex, CFG_SELECTOR);
         function_behavior->ogc->AddEdge(goto_vertex, label_vertex, FB_CFG_SELECTOR);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Transforming " + STR(fbb.CGetNodeInfo(from_bb).block->number) + "->" +
                            STR(fbb.CGetNodeInfo(to_bb).block->number));
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed loop " + STR(loop->getLoopId()));
   }

   if(parameters->getOption<bool>(OPT_print_dot))
   {
      function_behavior->GetOpGraph(FunctionBehavior::FCFG).writeDot(function_behavior->GetDotPath() / "OP_FCFG.dot");
      function_behavior->GetOpGraph(FunctionBehavior::CFG).writeDot(function_behavior->GetDotPath() / "OP_CFG.dot");
   }
   return DesignFlowStep_Status::SUCCESS;
}
