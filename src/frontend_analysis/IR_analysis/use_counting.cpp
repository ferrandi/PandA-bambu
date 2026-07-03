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
 * @file use_counting.cpp
 * @brief Analysis step counting how many times a ssa_node is used
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "use_counting.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "dbgPrintHelper.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"
#include "string_manipulation.hpp"

use_counting::use_counting(const ParameterConstRef _parameters, const application_managerRef _AppM,
                           unsigned int _function_id, const DesignFlowManager& _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, USE_COUNTING, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
use_counting::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(CALL_GRAPH_BUILTIN_CALL, SAME_FUNCTION));
         relationships.insert(std::make_pair(CHECK_SYSTEM_TYPE, SAME_FUNCTION));
         relationships.insert(std::make_pair(IR_LOWERING, SAME_FUNCTION));
         relationships.insert(std::make_pair(PARM_DECL_TAKEN_ADDRESS, SAME_FUNCTION));
         relationships.insert(std::make_pair(REBUILD_INITIALIZATION2, SAME_FUNCTION));
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
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

bool use_counting::HasToBeExecuted() const
{
   return bb_version == 0 && FunctionFrontendFlowStep::HasToBeExecuted();
}

DesignFlowStep_Status use_counting::InternalExec()
{
   const auto TM = AppM->get_ir_manager();
   const auto* fd = GetPointerS<const function_val_node>(TM->GetIRNode(function_id));
   const auto* sl = GetPointerS<const statement_list_node>(fd->body);
   const auto th_debug = ir_helper::debug_level;
   ir_helper::debug_level = debug_level;
   for(const auto& [idx, bb] : sl->list_of_bloc)
   {
      for(const auto& statement_node : bb->CGetStmtList())
      {
         const auto ssa_uses = ir_helper::ComputeSsaUses(statement_node);
         for(const auto& [node, uses] : ssa_uses)
         {
            auto* sn = GetPointerS<ssa_node>(node);
            for(auto counter = uses; counter; --counter)
            {
               sn->AddUseStmt(statement_node);
            }
         }
      }
      for(const auto& phi_node : bb->CGetPhiList())
      {
         const auto ssa_uses = ir_helper::ComputeSsaUses(phi_node);
         for(const auto& [node, uses] : ssa_uses)
         {
            auto* sn = GetPointerS<ssa_node>(node);
            for(auto counter = uses; counter; --counter)
            {
               sn->AddUseStmt(phi_node);
            }
         }
         GetPointerS<phi_stmt>(phi_node)->SetSSAUsesComputed();
      }
      bb->SetSSAUsesComputed();
   }

   ir_helper::debug_level = th_debug;
   return DesignFlowStep_Status::SUCCESS;
}
