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
 * @file parm_decl_taken_address_fix.cpp
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "parm_decl_taken_address_fix.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "dbgPrintHelper.hpp"
#include "function_behavior.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_manipulation.hpp"
#include "ir_node.hpp"
#include "string_manipulation.hpp"

parm_decl_taken_address_fix::parm_decl_taken_address_fix(const ParameterConstRef params,
                                                         const application_managerRef AM, unsigned int fun_id,
                                                         const DesignFlowManager& dfm)
    : FunctionFrontendFlowStep(AM, fun_id, PARM_DECL_TAKEN_ADDRESS, dfm, params)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
parm_decl_taken_address_fix::ComputeFrontendRelationships(
    const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(IR_LOWERING, SAME_FUNCTION));
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

bool parm_decl_taken_address_fix::HasToBeExecuted() const
{
   return bb_version == 0 && FunctionFrontendFlowStep::HasToBeExecuted();
}

DesignFlowStep_Status parm_decl_taken_address_fix::InternalExec()
{
   const auto TM = AppM->get_ir_manager();
   const auto fnode = TM->GetIRNode(function_id);
   const auto* fd = GetPointerS<function_val_node>(fnode);
   THROW_ASSERT(fd->body, "unexpected condition");
   const auto* sl = GetPointerS<const statement_list_node>(fd->body);
   THROW_ASSERT(!GetPointerS<const function_ty_node>(ir_helper::CGetType(fnode))->varargs_flag,
                "Function " + ir_helper::GetFunctionName(fnode) + " is varargs");

   // compute the set of argument_val_node for which an address is taken
   CustomOrderedSet<unsigned int> parm_decl_addr;
   for(const auto& [bbi, bb] : sl->list_of_bloc)
   {
      for(const auto& stmt : bb->CGetStmtList())
      {
         if(stmt->get_kind() == assign_stmt_K)
         {
            const auto* ga = GetPointerS<const assign_stmt>(stmt);
            if(ga->op1->get_kind() == addr_node_K)
            {
               const auto* ae = GetPointerS<const addr_node>(ga->op1);
               if(ae->op->get_kind() == argument_val_node_K)
               {
                  parm_decl_addr.insert(ae->op->index);
               }
            }
         }
      }
   }

   if(parm_decl_addr.empty())
   {
      return DesignFlowStep_Status::UNCHANGED;
   }

   const ir_manipulation IRman(TM, parameters, AppM);
   const auto entry_blocks = sl->list_of_bloc.at(BB_ENTRY)->list_of_succ;
   THROW_ASSERT(entry_blocks.size() == 1,
                "Multiple entry basic blocks in function " + ir_helper::GetFunctionName(fnode));
   const auto first_bb = sl->list_of_bloc.at(entry_blocks.front());

   for(auto par_index : parm_decl_addr)
   {
      auto par = TM->GetIRNode(par_index);
      THROW_ASSERT(par->get_kind() == argument_val_node_K, "unexpected condition");
      const auto* pd = GetPointerS<const argument_val_node>(par);
      const auto original_param_name = pd->name ? GetPointerS<const identifier_node>(pd->name)->strg : STR(par_index);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Fix declaration for parameter " + par->ToString());
      const auto local_var_name = "__bambu_local_parm_" + original_param_name;
      const auto param_locinfo = pd->include_name + ":" + STR(pd->line_number) + ":" + STR(pd->column_number);
      const auto local_decl =
          IRman.create_var_decl(IRman.create_identifier_node(local_var_name), pd->type, pd->parent, pd->bitsizealloc,
                                ir_nodeRef(), param_locinfo, GetPointerS<const type_node>(pd->type)->algn);

      for(const auto& [bbi, bb] : sl->list_of_bloc)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining BB" + STR(bbi));
         for(const auto& stmt : bb->CGetStmtList())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining statement " + stmt->ToString());
            TM->ReplaceIRNode(stmt, par, local_decl, false);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement " + stmt->ToString());
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined BB" + STR(bbi));
      }

      auto ga_addr_node = IRman.CreateAssignStmtAddrExpr(local_decl, function_id, param_locinfo);
      auto* ga_addr = GetPointerS<assign_stmt>(ga_addr_node);
      ga_addr->temporary_address = true;
      first_bb->PushFront(ga_addr_node, AppM);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---New statement " + ga_addr_node->ToString());

      const auto local_ref = IRman.create_unary_operation(pd->type, ga_addr->op0, param_locinfo, mem_access_node_K);
      const auto pvalue = IRman.create_ssa_name(par, pd->type, nullptr, nullptr);
      const auto ga_node = IRman.create_assign_stmt(local_ref, pvalue, function_id, param_locinfo);
      GetPointerS<node_stmt>(ga_node)->artificial = true;
      first_bb->PushAfter(ga_node, ga_addr_node, AppM);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---New statement " + ga_node->ToString());
   }

   function_behavior->UpdateBBVersion();
   return DesignFlowStep_Status::SUCCESS;
}
