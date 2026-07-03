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
 * @file extract_patterns.cpp
 * @brief
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "extract_patterns.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "dbgPrintHelper.hpp"
#include "function_behavior.hpp"
#include "hls_device.hpp"
#include "hls_manager.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_manipulation.hpp"
#include "string_manipulation.hpp"

#include <cmath>
#include <fstream>
#include <string>

extract_patterns::extract_patterns(const ParameterConstRef _parameters, const application_managerRef _AppM,
                                   unsigned int _function_id, const DesignFlowManager& _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, EXTRACT_PATTERNS, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
extract_patterns::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(USE_COUNTING, SAME_FUNCTION));
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(SDC_CODE_MOTION, SAME_FUNCTION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   return relationships;
}

static kind ternary_operation_type0(kind operation_kind1, kind operation_kind2)
{
   if(operation_kind1 == add_node_K && operation_kind2 == add_node_K)
   {
      return ternary_add_node_K;
   }
   else if(operation_kind1 == add_node_K && operation_kind2 == sub_node_K)
   {
      return ternary_as_node_K;
   }
   else if(operation_kind1 == sub_node_K && operation_kind2 == add_node_K)
   {
      return ternary_sa_node_K;
   }
   else
   { // if(operation_kind1 == sub_node_K && operation_kind2 == sub_node_K)
      return ternary_ss_node_K;
   }
}

static kind ternary_operation_type1(kind operation_kind1, kind operation_kind2)
{
   if(operation_kind1 == add_node_K && operation_kind2 == add_node_K)
   {
      return ternary_add_node_K;
   }
   else if(operation_kind1 == add_node_K && operation_kind2 == sub_node_K)
   {
      return ternary_ss_node_K;
   }
   else if(operation_kind1 == sub_node_K && operation_kind2 == add_node_K)
   {
      return ternary_as_node_K;
   }
   else
   { // if(operation_kind1 == sub_node_K && operation_kind2 == sub_node_K)
      return ternary_sa_node_K;
   }
}

DesignFlowStep_Status extract_patterns::InternalExec()
{
   if(parameters->IsParameter("disable-extract-patterns") &&
      parameters->GetParameter<unsigned int>("disable-extract-patterns") == 1)
   {
      return DesignFlowStep_Status::UNCHANGED;
   }
   const auto hls_d = GetPointer<const HLS_manager>(AppM)->get_HLS_device();
   if(AppM->GetParameterFromParameterOrDeviceOrDefault<unsigned>("disable_extract_ternary_patterns", hls_d, 0U) != 0U)
   {
      /// Now, the only patterns extracted are ternary.
      /// So, this part needs to be changed in case other patterns will be added.
      return DesignFlowStep_Status::UNCHANGED;
   }
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, " --------- EXTRACT_PATTERNS ---------- ");
   const auto TM = AppM->get_ir_manager();
   const auto tn = TM->GetIRNode(function_id);
   const auto fd = GetPointer<const function_val_node>(tn);
   THROW_ASSERT(fd && fd->body, "Node is not a function or it hasn't a body");
   auto sl = GetPointer<statement_list_node>(fd->body);
   THROW_ASSERT(sl, "Body is not a statement_list_node");

   /// for each basic block B in CFG do > Consider all blocks successively
   bool modified = false;
   for(const auto& bb_pair : sl->list_of_bloc)
   {
      const auto& B = bb_pair.second;
      const auto& B_id = B->number;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining BB" + STR(B_id));
      const auto list_of_stmt = B->CGetStmtList();
      /// manage capacity
      auto it_los = list_of_stmt.begin();
      auto it_los_end = list_of_stmt.end();
      while(it_los != it_los_end)
      {
         if(!AppM->ApplyNewTransformation())
         {
            break;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining statement " + (*it_los)->ToString());
         if((*it_los)->get_kind() == assign_stmt_K)
         {
            const auto ga = GetPointerS<const assign_stmt>(*it_los);
            const auto code0 = ga->op0->get_kind();
            const auto code1 = ga->op1->get_kind();
            if(code0 == ssa_node_K && (code1 == add_node_K || code1 == sub_node_K))
            {
               if(!(ir_helper::IsRealType(ga->op0) || ir_helper::IsVectorType(ga->op0)))
               {
                  const auto ssa_defined = GetPointerS<const ssa_node>(ga->op0);
                  const auto ssa_defined_size = ir_helper::Size(ir_helper::CGetType(ga->op0));
                  const auto binop0 = GetPointerS<const binary_node>(ga->op1);
                  if((ssa_defined->CGetNumberUses() == 1) &&
                     (ssa_defined_size == ir_helper::Size(ir_helper::CGetType(binop0->op0))) &&
                     (ssa_defined_size == ir_helper::Size(ir_helper::CGetType(binop0->op1))))
                  {
                     const auto statement_node = ssa_defined->CGetUseStmts().begin()->first;
                     if(statement_node->get_kind() == assign_stmt_K)
                     {
                        auto ga_dest = GetPointerS<assign_stmt>(statement_node);
                        const auto code_dest0 = ga_dest->op0->get_kind();
                        const auto code_dest1 = ga_dest->op1->get_kind();
                        const auto ssa_dest0_size = ir_helper::Size(ir_helper::CGetType(ga_dest->op0));
                        if(code_dest0 == ssa_node_K && (code_dest1 == add_node_K || code_dest1 == sub_node_K) &&
                           ga_dest->bb_index == B_id && ssa_dest0_size == ssa_defined_size)
                        {
                           ir_manipulationRef IRman(new ir_manipulation(TM, parameters, AppM));
                           /// matched
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "---Ternary plus expr statement found ");
                           const auto loc_info_default = ga_dest->include_name + ":" + STR(ga_dest->line_number) + ":" +
                                                         STR(ga_dest->column_number);
                           const auto binop_dest = GetPointerS<const binary_node>(ga_dest->op1);
                           if(ga->op0->index == binop_dest->op0->index)
                           {
                              const auto ternary_op = IRman->create_ternary_operation(
                                  binop_dest->type, binop0->op0, binop0->op1, binop_dest->op1, loc_info_default,
                                  ternary_operation_type0(code1, code_dest1));
                              TM->ReplaceIRNode(statement_node, ga_dest->op1, ternary_op);
                           }
                           else
                           {
                              const auto ternary_op = IRman->create_ternary_operation(
                                  binop_dest->type, binop_dest->op0, binop0->op0, binop0->op1, loc_info_default,
                                  ternary_operation_type1(code1, code_dest1));
                              TM->ReplaceIRNode(statement_node, ga_dest->op1, ternary_op);
                           }
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "<--Statement removed " + (*it_los)->ToString());
                           B->RemoveStmt(*it_los, AppM);
                           it_los = list_of_stmt.begin();
                           it_los_end = list_of_stmt.end();
                           AppM->RegisterTransformation(GetName(), statement_node);
                           modified = true;
                           continue;
                        }
                     }
                  }
               }
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Statement analyzed " + (*it_los)->ToString());
         ++it_los;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined BB" + STR(B_id));
   }

   if(modified)
   {
      function_behavior->UpdateBBVersion();
   }
   return modified ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}
