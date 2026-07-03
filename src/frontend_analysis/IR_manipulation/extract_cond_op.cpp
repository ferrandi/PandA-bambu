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
 * @file extract_cond_op.cpp
 * @brief Analysis step that extract condition from multi_way_if_stmt
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#include "extract_cond_op.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "dbgPrintHelper.hpp"
#include "function_behavior.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_manipulation.hpp"
#include "ir_node.hpp"
#include "string_manipulation.hpp"

ExtractCondOp::ExtractCondOp(const application_managerRef _AppM, const DesignFlowManager& _design_flow_manager,
                             const unsigned int _function_id, const ParameterConstRef _parameters)
    : FunctionFrontendFlowStep(_AppM, _function_id, EXTRACT_COND_OP, _design_flow_manager, _parameters),
      bb_modified(false)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

void ExtractCondOp::Initialize()
{
   bb_modified = false;
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionFrontendFlowStep::FunctionRelationship>>
ExtractCondOp::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      case(PRECEDENCE_RELATIONSHIP):
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

DesignFlowStep_Status ExtractCondOp::InternalExec()
{
   bb_modified = false;
   const auto TM = AppM->get_ir_manager();
   const auto ir_man = ir_manipulationConstRef(new ir_manipulation(TM, parameters, AppM));
   const auto fd = GetPointer<function_val_node>(TM->GetIRNode(function_id));
   const auto sl = GetPointer<statement_list_node>(fd->body);
   for(const auto& block : sl->list_of_bloc)
   {
      const auto& stmt_list = block.second->CGetStmtList();
      if(stmt_list.size())
      {
         const auto last_stmt = stmt_list.back();
         if(last_stmt->get_kind() == multi_way_if_stmt_K)
         {
            auto stmt = GetPointerS<multi_way_if_stmt>(last_stmt);
            for(auto& op_el : stmt->list_of_cond)
            {
               if(op_el.first && (!ir_helper::IsBooleanType(op_el.first) ||
                                  (op_el.first->get_kind() != ssa_node_K && !GetPointer<cst_node>(op_el.first))))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---fixing ifelseif: " + last_stmt->ToString());
                  op_el.first = ir_man->ExtractCondition(op_el.first, block.second, function_id, stmt->include_name,
                                                         stmt->line_number, stmt->column_number);
                  bb_modified = true;
               }
            }
         }
      }
   }
   bb_modified ? function_behavior->UpdateBBVersion() : 0;
   return bb_modified ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}
