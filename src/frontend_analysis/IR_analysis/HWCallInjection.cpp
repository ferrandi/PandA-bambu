/*
 *                 _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *                _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *               _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *              _/      _/    _/ _/    _/ _/   _/ _/    _/
 *             _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *           ***********************************************
 *                            PandA Project
 *                   URL: http://panda.dei.polimi.it
 *                     Politecnico di Milano - DEIB
 *                      System Architectures Group
 *           ***********************************************
 *            Copyright (C) 2004-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 * This file is part of the PandA framework.
 *
 * Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "HWCallInjection.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "custom_set.hpp"
#include "dbgPrintHelper.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_manipulation.hpp"
#include "ir_node.hpp"
#include "op_graph.hpp"
#include "string_manipulation.hpp"
#include "token_interface.hpp"

#include <string>
#include <utility>
#include <vector>

ir_nodeRef HWCallInjection::builtinWaitCallDecl = nullptr;

HWCallInjection::HWCallInjection(const ParameterConstRef Param, const application_managerRef _AppM, unsigned int funId,
                                 const DesignFlowManager& DFM)
    : FunctionFrontendFlowStep(_AppM, funId, HWCALL_INJECTION, DFM, Param)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
HWCallInjection::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType RT) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(RT)
   {
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      case(DEPENDENCE_RELATIONSHIP):
      {
         break;
      }
      default:
      {
         THROW_UNREACHABLE("HWCallInjection::ComputeFrontendRelationships");
      }
   }
   return relationships;
}

bool HWCallInjection::HasToBeExecuted() const
{
   return bb_version == 0 && FunctionFrontendFlowStep::HasToBeExecuted();
}

DesignFlowStep_Status HWCallInjection::InternalExec()
{
   const auto TM = AppM->get_ir_manager();
   const auto fd = GetPointerS<const function_val_node>(TM->GetIRNode(function_id));
   THROW_ASSERT(fd->body, "Node is not a function or it hasn't a body");
   const auto sl = GetPointerS<statement_list_node>(fd->body);

   const auto isHardwareCall = [&](const ir_nodeRef& expr) -> bool {
      ir_nodeRef FD;
      if(expr->get_kind() == call_stmt_K)
      {
         const auto gc = GetPointerS<call_stmt>(expr);
         FD = GetPointer<const addr_node>(gc->fn) ? GetPointerS<const addr_node>(gc->fn)->op : gc->fn;
      }
      else if(expr->get_kind() == assign_stmt_K)
      {
         const auto ga = GetPointerS<const assign_stmt>(expr);
         if(ga->op1->get_kind() == call_node_K)
         {
            const auto CE = GetPointerS<const call_node>(ga->op1);
            FD = GetPointer<const addr_node>(CE->fn) ? GetPointerS<const addr_node>(CE->fn)->op : CE->fn;
         }
      }

      // When the instruction is not a function call return false.
      bool result = false;
      if(FD)
      {
         if(FD->get_kind() == ssa_node_K)
         {
            // This is the case for function pointers call.
            result = true;
         }
      }
      return result;
   };

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Searching for hardware implemented calls");
   for(const auto& block : sl->list_of_bloc)
   {
      const auto list_of_stmt = block.second->CGetStmtList();
      auto stmt = list_of_stmt.begin();
      while(stmt != list_of_stmt.end())
      {
         stmt++;
         const auto& cur_stmt = *std::prev(stmt);
         if(isHardwareCall(cur_stmt))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Transforming call " + STR(cur_stmt));
            buildBuiltinCall(block.second, cur_stmt);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
         }
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   return DesignFlowStep_Status::SUCCESS;
}

void HWCallInjection::buildBuiltinCall(const blocRef& block, const ir_nodeRef& stmt)
{
   const auto stmt_kind = stmt->get_kind();
   const auto TM = AppM->get_ir_manager();
   ir_manipulation ir_man(TM, parameters, AppM);

   if(!builtinWaitCallDecl)
   {
      ir_manager::IRSchema attr_map;

      attr_map[TOK(TOK_RETN)] = STR(ir_man.GetVoidType()->index);
      attr_map[TOK(TOK_VARARGS)] = STR(1);
      attr_map[TOK(TOK_ARG)] = STR(ir_man.GetSignedIntegerType()->index);
      attr_map[TOK(TOK_BITSIZEALLOC)] = STR(8);
      attr_map[TOK(TOK_ALIGNED)] = STR(8);
      auto ftype_node = TM->create_ir_node(function_ty_node_K, attr_map);
      attr_map.clear();

      attr_map[TOK(TOK_TYPE)] = STR(ftype_node->index);
      const auto builtinIdString = ir_man.create_identifier_node(BUILTIN_WAIT_CALL);
      attr_map[TOK(TOK_NAME)] = STR(builtinIdString->index);
      attr_map[TOK(TOK_IR_LOCINFO)] = BUILTIN_LOCINFO;
      builtinWaitCallDecl = TM->create_ir_node(function_val_node_K, attr_map);
   }

   const auto gn = GetPointerS<node_stmt>(stmt);
   const auto loc_info_str = gn->include_name + ":" + STR(gn->line_number) + ":" + STR(gn->column_number);

   const auto builtin_stmt = [&]() {
      ir_manager::IRSchema attr_map;
      const auto functionDecl = GetPointerS<const function_val_node>(builtinWaitCallDecl);
      attr_map[TOK(TOK_TYPE)] = STR(ir_man.GetPointerType(functionDecl->type, 8)->index);
      attr_map[TOK(TOK_OP)] = STR(builtinWaitCallDecl->index);
      attr_map[TOK(TOK_IR_LOCINFO)] = loc_info_str;
      auto ae_node = TM->create_ir_node(addr_node_K, attr_map);
      attr_map.clear();

      attr_map[TOK(TOK_FN)] = STR(ae_node->index);
      if(stmt_kind == call_stmt_K)
      {
         const auto gc = GetPointerS<const call_stmt>(stmt);
         attr_map[TOK(TOK_PARENT)] = STR(gc->parent->index);
      }
      else if(stmt_kind == assign_stmt_K)
      {
         const auto ga = GetPointerS<const assign_stmt>(stmt);
         attr_map[TOK(TOK_PARENT)] = STR(ga->parent->index);
      }
      attr_map[TOK(TOK_IR_LOCINFO)] = loc_info_str;
      return TM->create_ir_node(call_stmt_K, attr_map);
   }();

   const auto builtin_call = GetPointerS<call_stmt>(builtin_stmt);
   ir_nodeRef retVar = nullptr;
   if(stmt_kind == call_stmt_K)
   {
      const auto gc = GetPointerS<call_stmt>(stmt);
      builtin_call->AddArg(gc->fn);

      const auto has_return = TM->CreateUniqueIntegerCst(0, ir_man.GetSignedIntegerType());
      builtin_call->AddArg(has_return);

      for(const auto& arg : gc->args)
      {
         builtin_call->AddArg(arg);
      }

      builtin_call->memuse = gc->memuse;
      builtin_call->memdef = gc->memdef;
      builtin_call->vdef = gc->vdef;
      builtin_call->vuses = gc->vuses;
      builtin_call->vovers = gc->vovers;

      builtin_call->parent = gc->parent;
      builtin_call->bb_index = gc->bb_index;
      builtin_call->include_name = gc->include_name;
      builtin_call->line_number = gc->line_number;
      builtin_call->column_number = gc->column_number;
   }
   else if(stmt_kind == assign_stmt_K)
   {
      const auto ga = GetPointerS<assign_stmt>(stmt);
      if(ga->op1->get_kind() == call_node_K)
      {
         const auto CE = GetPointerS<const call_node>(ga->op1);
         builtin_call->AddArg(CE->fn);

         const auto has_return = TM->CreateUniqueIntegerCst(1, ir_man.GetSignedIntegerType());
         builtin_call->AddArg(has_return);

         for(const auto& arg : CE->args)
         {
            builtin_call->AddArg(arg);
         }

         if(const auto ssaRet = GetPointer<const ssa_node>(ga->op0))
         {
            ir_nodeRef ret_var_type;
            unsigned ret_var_size;
            unsigned int ret_var_algn;
            if(ssaRet->type)
            {
               ret_var_type = ssaRet->type;
               ret_var_size = GetPointerS<const type_node>(ssaRet->type)->bitsizealloc;
               ret_var_algn = GetPointerS<const type_node>(ssaRet->type)->algn;
            }
            else
            {
               const auto vd = GetPointerS<const variable_val_node>(ssaRet->var);
               ret_var_type = vd->type;
               ret_var_size = GetPointerS<const type_node>(vd->type)->bitsizealloc;
               ret_var_algn = GetPointerS<const type_node>(vd->type)->algn;
            }
            retVar = ir_man.create_var_decl(
                ir_man.create_identifier_node("__return_value"), ret_var_type, ga->parent, ret_var_size, nullptr,
                STR(ga->include_name + ":" + STR(ga->line_number) + ":" + STR(ga->column_number)), ret_var_algn, 1,
                true);

            ga->op1 = retVar;
         }

         if(!retVar)
         {
            retVar = ga->op0;
         }

         const auto addrExprReturnValue = ir_man.create_unary_operation(
             ir_man.GetPointerType(ir_helper::CGetType(retVar)), retVar, loc_info_str, addr_node_K);

         builtin_call->AddArg(addrExprReturnValue);

         builtin_call->memdef = ga->memdef;
         builtin_call->memuse = ga->memuse;
         builtin_call->vdef = ga->vdef;
         builtin_call->vuses = ga->vuses;
         builtin_call->vovers = ga->vovers;

         builtin_call->parent = ga->parent;
         builtin_call->bb_index = ga->bb_index;
         builtin_call->include_name = ga->include_name;
         builtin_call->line_number = ga->line_number;
         builtin_call->column_number = ga->column_number;

         ga->memdef = nullptr;
         ga->memuse = builtin_call->memdef;
         ga->vdef = nullptr;
         ga->vuses.clear();
         ga->vovers.clear();
         THROW_ASSERT(builtin_call->vdef, "Unexpected condition");
         ga->AddVuse(builtin_call->vdef);
      }
   }
   else
   {
      THROW_UNREACHABLE("Error not a call_stmt or assign_stmt!");
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "---Added to BB" + STR(block->number) + " " + STR(builtin_stmt));
   block->PushBefore(builtin_stmt, stmt, AppM);
   if(!retVar)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---Removed from BB" + STR(block->number) + " " + STR(stmt));
      block->RemoveStmt(stmt, AppM);
   }
   else
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Modified " + STR(stmt));
   }
}
