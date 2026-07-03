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
 * @file AddArtificialCallFlowEdges.cpp
 * @brief Analysis step which adds flow edges to builtin bambu time functions
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#include "AddArtificialCallFlowEdges.hpp"

#include "Parameter.hpp"
#include "basic_block.hpp"
#include "behavioral_helper.hpp"
#include "dbgPrintHelper.hpp"
#include "function_behavior.hpp"
#include "hash_helper.hpp"
#include "ir_basic_block.hpp"
#include "op_graph.hpp"
#include "string_manipulation.hpp"

#include <boost/range/adaptor/reversed.hpp>

AddArtificialCallFlowEdges::AddArtificialCallFlowEdges(const application_managerRef _AppM, unsigned int _function_id,
                                                       const DesignFlowManager& _design_flow_manager,
                                                       const ParameterConstRef _parameters)
    : FunctionFrontendFlowStep(_AppM, _function_id, ADD_ARTIFICIAL_CALL_FLOW_EDGES, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
AddArtificialCallFlowEdges::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
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

DesignFlowStep_Status AddArtificialCallFlowEdges::InternalExec()
{
   /// The control flow graph of basic blocks
   const auto bb_graph = function_behavior->GetBBGraph(FunctionBehavior::BB);

   /// The control flow graph of operation
   const auto op_graph = function_behavior->GetOpGraph(FunctionBehavior::CFG);

   const auto BH = function_behavior->CGetBehavioralHelper();
   /// Adding operation to empty return
   for(const auto& v : bb_graph.vertices())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "-->Analyzing BB" + STR(bb_graph.CGetNodeInfo(v).block->number));
      const auto& statements_list = bb_graph.CGetNodeInfo(v).statements_list;
      for(const auto stmt : statements_list)
      {
         const auto& node_info = op_graph.CGetNodeInfo(stmt);
         const unsigned int st_tn_id = node_info.GetNodeId();
         if(!BH->CanBeMoved(st_tn_id))
         {
            bool previous = true;
            for(const auto other_stmt : statements_list)
            {
               if(other_stmt == stmt)
               {
                  previous = false;
                  continue;
               }
               if(previous)
               {
                  function_behavior->ogc->AddEdge(other_stmt, stmt, FLG_SELECTOR);
               }
               else
               {
                  function_behavior->ogc->AddEdge(stmt, other_stmt, FLG_SELECTOR);
               }
            }
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "<--Analyzed BB" + STR(bb_graph.CGetNodeInfo(v).block->number));
   }
   if(parameters->getOption<bool>(OPT_print_dot))
   {
      const auto dot_path = function_behavior->GetDotPath();
      function_behavior->GetOpGraph(FunctionBehavior::FLG).writeDot(dot_path / "OP_FL.dot");
      function_behavior->GetOpGraph(FunctionBehavior::FFLSAODG).writeDot(dot_path / "OP_FFLSAODG.dot");
   }
   return DesignFlowStep_Status::SUCCESS;
}
