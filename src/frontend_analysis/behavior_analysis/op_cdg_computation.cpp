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
 * @file op_cdg_computation.cpp
 * @brief Analysis step performing operation control dependence computation.
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#include "op_cdg_computation.hpp"

#include "Parameter.hpp"
#include "basic_block.hpp"
#include "dbgPrintHelper.hpp"
#include "function_behavior.hpp"
#include "hash_helper.hpp"
#include "op_graph.hpp"
#include "string_manipulation.hpp"

OpCdgComputation::OpCdgComputation(const ParameterConstRef _Param, const application_managerRef _AppM,
                                   unsigned int _function_id, const DesignFlowManager& _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, OP_CONTROL_DEPENDENCE_COMPUTATION, _design_flow_manager, _Param)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
OpCdgComputation::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(BB_CONTROL_DEPENDENCE_COMPUTATION, SAME_FUNCTION));
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

void OpCdgComputation::Initialize()
{
   if(bb_version != 0 and bb_version != function_behavior->GetBBVersion())
   {
      const auto cdg = function_behavior->GetOpGraph(FunctionBehavior::CDG);
      if(cdg.num_vertices() != 0)
      {
         for(const auto& edge : cdg.edges())
         {
            function_behavior->ogc->RemoveSelector(edge, CDG_SELECTOR);
         }
      }
   }
}

DesignFlowStep_Status OpCdgComputation::InternalExec()
{
   auto fcfg = function_behavior->GetOpGraph(FunctionBehavior::FCFG);
   const auto bb_cdg = function_behavior->GetBBGraph(FunctionBehavior::CDG_BB);
   for(const auto& edge : bb_cdg.edges())
   {
      const auto source = bb_cdg.source(edge);
      const auto target = bb_cdg.target(edge);
      const auto source_operations = bb_cdg.CGetNodeInfo(source).statements_list;
      const auto target_operations = bb_cdg.CGetNodeInfo(target).statements_list;
      if(source_operations.size() && target_operations.size())
      {
         const auto labels = bb_cdg.CGetEdgeInfo(edge).get_labels(CFG_SELECTOR);
         const auto source_operation = source_operations.back();
         for(const auto target_operation : target_operations)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Adding Control Dependence " + fcfg.CGetNodeInfo(source_operation).vertex_name + "-->" +
                               fcfg.CGetNodeInfo(target_operation).vertex_name);
            function_behavior->ogc->AddEdge(source_operation, target_operation, CDG_SELECTOR);
            for(const auto label : labels)
            {
               function_behavior->ogc->add_edge_info(source_operation, target_operation, CDG_SELECTOR, label);
            }
         }
      }
   }

   for(const auto& basic_block : bb_cdg.vertices())
   {
      const auto& bb_node_info = bb_cdg.CGetNodeInfo(basic_block);
      const auto cer_index = bb_node_info.cer;
      for(const auto statement : bb_node_info.statements_list)
      {
         fcfg.GetNodeInfo(statement).cer = cer_index;
      }
   }

   if(parameters->getOption<bool>(OPT_print_dot))
   {
      function_behavior->GetOpGraph(FunctionBehavior::CDG).writeDot(function_behavior->GetDotPath() / "OP_CDG.dot");
   }
   return DesignFlowStep_Status::SUCCESS;
}
