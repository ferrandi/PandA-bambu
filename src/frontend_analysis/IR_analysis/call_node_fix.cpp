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
 * @file call_node_fix.cpp
 * @brief Analysis step which fix a non-void list of parameters to function with void as input parameter type
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "call_node_fix.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "dbgPrintHelper.hpp"
#include "function_behavior.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"
#include "string_manipulation.hpp"

#include <fstream>

call_node_fix::call_node_fix(const application_managerRef _AppM, unsigned int _function_id,
                             const DesignFlowManager& _design_flow_manager, const ParameterConstRef _parameters)
    : FunctionFrontendFlowStep(_AppM, _function_id, CALL_NODE_FIX, _design_flow_manager, _parameters)
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionFrontendFlowStep::FunctionRelationship>>
call_node_fix::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
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

DesignFlowStep_Status call_node_fix::InternalExec()
{
   const ir_managerRef TM = AppM->get_ir_manager();
   ir_nodeRef temp = TM->GetIRNode(function_id);
   auto* fdcur = GetPointer<function_val_node>(temp);
   auto* sl = GetPointer<statement_list_node>(fdcur->body);
   bool bb_modified = false;

   /// Checking if there are call_stmt or call_node for which the fix apply
   for(const auto& block : sl->list_of_bloc)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->analyzing BB" + std::to_string(block.first));
      for(const auto& statement : block.second->CGetStmtList())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing node " + statement->ToString());
         if(statement->get_kind() == call_stmt_K)
         {
            auto* ce = GetPointer<call_stmt>(statement);
            std::vector<ir_nodeRef>& args = ce->args;
            auto* ae = GetPointer<addr_node>(ce->fn);
            if(ae && args.size())
            {
               auto* fd = GetPointer<function_val_node>(ae->op);
               if(fd->body)
               {
                  auto* fun_type = GetPointer<function_ty_node>(fd->type);
                  bool is_var_args_p = fun_type->varargs_flag;
                  if(!is_var_args_p)
                  {
                     /// check if fun_type there is only one parameter and it is equal to void
                     unsigned int count_param = 0;
                     for(const auto& p : fun_type->list_of_args_type)
                     {
                        if(p->get_kind() != void_ty_node_K)
                        {
                           count_param++;
                        }
                     }
                     if(fd->list_of_args.size() == 0 && count_param == 0)
                     {
                        ce->args.clear();
                        bb_modified = true;
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---actuals cleared");
                     }
                  }
               }
               else if(ir_helper::GetFunctionName(ae->op) == "__builtin_dwarf_cfa")
               {
                  ce->args.clear();
                  bb_modified = true;
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---actuals cleared");
               }
            }
         }
         if(statement->get_kind() == assign_stmt_K)
         {
            auto* ga = GetPointer<assign_stmt>(statement);
            if(ga->op1->get_kind() == call_node_K)
            {
               auto* ce = GetPointer<call_node>(ga->op1);
               std::vector<ir_nodeRef>& args = ce->args;
               auto* ae = GetPointer<addr_node>(ce->fn);
               if(ae && args.size())
               {
                  auto* fd = GetPointer<function_val_node>(ae->op);
                  if(fd->body)
                  {
                     auto* fun_type = GetPointer<function_ty_node>(fd->type);
                     bool is_var_args_p = fun_type->varargs_flag;
                     if(!is_var_args_p)
                     {
                        /// check if fun_type there is only one parameter and it is equal to void
                        unsigned int count_param = 0;
                        for(const auto& p : fun_type->list_of_args_type)
                        {
                           if(p->get_kind() != void_ty_node_K)
                           {
                              count_param++;
                           }
                        }
                        if(fd->list_of_args.size() == 0 && count_param == 0)
                        {
                           ce->args.clear();
                           bb_modified = true;
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---actuals cleared");
                        }
                     }
                  }
                  else if(ir_helper::GetFunctionName(ae->op) == "__builtin_dwarf_cfa")
                  {
                     ce->args.clear();
                     bb_modified = true;
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---actuals cleared");
                  }
               }
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   }

   bb_modified ? function_behavior->UpdateBBVersion() : 0;
   return bb_modified ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}
