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
 *            Copyright (C) 2004-2024 Politecnico di Milano
 *
 * This file is part of the PandA framework.
 *
 * The PandA framework is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "HWCallInjection.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "op_graph.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "tree_node.hpp"

/// parser/compiler include
#include "dbgPrintHelper.hpp"      // for DEBUG_LEVEL_
#include "string_manipulation.hpp" // for GET_CLASS
#include "token_interface.hpp"

/// STD include
#include <string>

/// STL includes
#include "custom_set.hpp"
#include <utility>
#include <vector>

unsigned int HWCallInjection::builtinWaitCallDeclIdx = 0;

HWCallInjection::HWCallInjection(const ParameterConstRef Param, const application_managerRef _AppM, unsigned int funId,
                                 const DesignFlowManagerConstRef DFM)
    : FunctionFrontendFlowStep(_AppM, funId, HWCALL_INJECTION, DFM, Param)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

HWCallInjection::~HWCallInjection() = default;

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
   return bb_version == 0;
}

DesignFlowStep_Status HWCallInjection::InternalExec()
{
   const auto TM = AppM->get_tree_manager();
   const auto fd = GetPointer<const function_decl>(TM->GetTreeNode(function_id));
   THROW_ASSERT(fd && fd->body, "Node is not a function or it hasn't a body");
   const auto sl = GetPointerS<statement_list>(fd->body);
   THROW_ASSERT(sl, "Body is not a statement_list");

   const auto isHardwareCall = [&](const tree_nodeRef& expr) -> bool {
      tree_nodeRef FD;
      if(expr->get_kind() == gimple_call_K)
      {
         const auto GC = GetPointerS<gimple_call>(expr);
         FD = GetPointer<const addr_expr>(GC->fn) ? GetPointerS<const addr_expr>(GC->fn)->op : GC->fn;
      }
      else if(expr->get_kind() == gimple_assign_K)
      {
         const auto GA = GetPointerS<const gimple_assign>(expr);
         if(GA->op1->get_kind() == call_expr_K || GA->op1->get_kind() == aggr_init_expr_K)
         {
            const auto CE = GetPointerS<const call_expr>(GA->op1);
            FD = GetPointer<const addr_expr>(CE->fn) ? GetPointerS<const addr_expr>(CE->fn)->op : CE->fn;
         }
      }

      // When the instruction is not a function call return false.
      bool result = false;
      if(FD)
      {
         if(FD->get_kind() == function_decl_K)
         {
            const auto FDPtr = GetPointerS<const function_decl>(FD);
            result = FDPtr->hwcall_flag;
         }
         else if(FD->get_kind() == ssa_name_K)
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

void HWCallInjection::buildBuiltinCall(const blocRef& block, const tree_nodeRef& stmt)
{
   const auto stmt_kind = stmt->get_kind();
   const auto TM = AppM->get_tree_manager();
   const auto IRman = tree_manipulationRef(new tree_manipulation(TM, parameters, AppM));

   if(!builtinWaitCallDeclIdx)
   {
      const auto vararg_list_idx = TM->new_tree_node_id();
      std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> attr_map;
      attr_map[TOK(TOK_VALU)] = STR(IRman->GetSignedIntegerType()->index);
      TM->create_tree_node(vararg_list_idx, tree_list_K, attr_map);
      attr_map.clear();

      const auto ftype_idx = TM->new_tree_node_id();
      attr_map[TOK(TOK_RETN)] = STR(IRman->GetVoidType()->index);
      attr_map[TOK(TOK_VARARGS)] = STR(1);
      attr_map[TOK(TOK_PRMS)] = STR(vararg_list_idx);
      const auto funTypeSize = TM->CreateUniqueIntegerCst(8, IRman->GetSignedIntegerType());
      attr_map[TOK(TOK_SIZE)] = STR(funTypeSize->index);
      attr_map[TOK(TOK_ALIGNED)] = STR(8);
      TM->create_tree_node(ftype_idx, function_type_K, attr_map);
      attr_map.clear();

      builtinWaitCallDeclIdx = TM->new_tree_node_id();
      attr_map[TOK(TOK_TYPE)] = STR(ftype_idx);
      const auto builtinIdString = IRman->create_identifier_node(BUILTIN_WAIT_CALL);
      attr_map[TOK(TOK_NAME)] = STR(builtinIdString->index);
      attr_map[TOK(TOK_SRCP)] = BUILTIN_SRCP;
      TM->create_tree_node(builtinWaitCallDeclIdx, function_decl_K, attr_map);
      attr_map.clear();
   }

   const auto gn = GetPointerS<gimple_node>(stmt);
   const auto srcp_str = gn->include_name + ":" + STR(gn->line_number) + ":" + STR(gn->column_number);

   const auto builtin_stmt_idx = TM->new_tree_node_id();
   {
      const auto addr_expr_idx = TM->new_tree_node_id();
      std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> attr_map;
      const auto functionDecl = GetPointerS<const function_decl>(TM->GetTreeNode(builtinWaitCallDeclIdx));
      attr_map[TOK(TOK_TYPE)] = STR(IRman->GetPointerType(functionDecl->type, 8)->index);
      attr_map[TOK(TOK_OP)] = STR(builtinWaitCallDeclIdx);
      attr_map[TOK(TOK_SRCP)] = srcp_str;
      TM->create_tree_node(addr_expr_idx, addr_expr_K, attr_map);
      attr_map.clear();

      attr_map[TOK(TOK_FN)] = STR(addr_expr_idx);
      if(stmt_kind == gimple_call_K)
      {
         const auto GC = GetPointerS<const gimple_call>(stmt);
         attr_map[TOK(TOK_SCPE)] = STR(GC->scpe->index);
      }
      else if(stmt_kind == gimple_assign_K)
      {
         const auto GA = GetPointerS<const gimple_assign>(stmt);
         attr_map[TOK(TOK_SCPE)] = STR(GA->scpe->index);
      }
      attr_map[TOK(TOK_SRCP)] = srcp_str;
      TM->create_tree_node(builtin_stmt_idx, gimple_call_K, attr_map);
   }

   const auto builtin_stmt = TM->GetTreeNode(builtin_stmt_idx);
   const auto builtin_call = GetPointerS<gimple_call>(builtin_stmt);
   tree_nodeRef retVar = nullptr;
   if(stmt_kind == gimple_call_K)
   {
      const auto GC = GetPointerS<gimple_call>(stmt);
      builtin_call->AddArg(GC->fn);

      const auto has_return = TM->CreateUniqueIntegerCst(0, IRman->GetSignedIntegerType());
      builtin_call->AddArg(has_return);

      for(const auto& arg : GC->args)
      {
         builtin_call->AddArg(arg);
      }

      builtin_call->memuse = GC->memuse;
      builtin_call->memdef = GC->memdef;
      builtin_call->vdef = GC->vdef;
      builtin_call->vuses = GC->vuses;
      builtin_call->vovers = GC->vovers;

      builtin_call->pragmas = GC->pragmas;
      builtin_call->use_set = GC->use_set;
      builtin_call->clobbered_set = GC->clobbered_set;
      builtin_call->scpe = GC->scpe;
      builtin_call->bb_index = GC->bb_index;
      builtin_call->include_name = GC->include_name;
      builtin_call->line_number = GC->line_number;
      builtin_call->column_number = GC->column_number;
   }
   else if(stmt_kind == gimple_assign_K)
   {
      const auto GA = GetPointerS<gimple_assign>(stmt);
      if(GA->op1->get_kind() == call_expr_K || GA->op1->get_kind() == aggr_init_expr_K)
      {
         const auto CE = GetPointerS<const call_expr>(GA->op1);
         builtin_call->AddArg(CE->fn);

         const auto has_return = TM->CreateUniqueIntegerCst(1, IRman->GetSignedIntegerType());
         builtin_call->AddArg(has_return);

         for(const auto& arg : CE->args)
         {
            builtin_call->AddArg(arg);
         }

         if(const auto ssaRet = GetPointer<const ssa_name>(GA->op0))
         {
            tree_nodeRef ret_var_type, ret_var_size;
            unsigned int ret_var_algn;
            if(ssaRet->type)
            {
               ret_var_type = ssaRet->type;
               ret_var_size = GetPointerS<const type_node>(ssaRet->type)->size;
               ret_var_algn = GetPointerS<const type_node>(ssaRet->type)->algn;
            }
            else
            {
               const auto vd = GetPointerS<const var_decl>(ssaRet->var);
               ret_var_type = vd->type;
               ret_var_size = GetPointerS<const type_node>(vd->type)->size;
               ret_var_algn = GetPointerS<const type_node>(vd->type)->algn;
            }
            retVar = IRman->create_var_decl(
                IRman->create_identifier_node("__return_value"), ret_var_type, GA->scpe, ret_var_size, nullptr, nullptr,
                STR(GA->include_name + ":" + STR(GA->line_number) + ":" + STR(GA->column_number)), ret_var_algn, 1,
                true);

            GA->op1 = retVar;
         }

         if(!retVar)
         {
            retVar = GA->op0;
         }

         const auto addrExprReturnValue = TM->new_tree_node_id();
         {
            std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> addrExprReturnValueMap;
            const auto typeRetVar = tree_helper::CGetType(retVar);
            addrExprReturnValueMap[TOK(TOK_TYPE)] = STR(IRman->GetPointerType(typeRetVar)->index);
            addrExprReturnValueMap[TOK(TOK_OP)] = STR(retVar->index);
            addrExprReturnValueMap[TOK(TOK_SRCP)] = srcp_str;
            TM->create_tree_node(addrExprReturnValue, addr_expr_K, addrExprReturnValueMap);
         }
         builtin_call->AddArg(TM->GetTreeNode(addrExprReturnValue));

         builtin_call->memdef = GA->memdef;
         builtin_call->memuse = GA->memuse;
         builtin_call->vdef = GA->vdef;
         builtin_call->vuses = GA->vuses;
         builtin_call->vovers = GA->vovers;

         builtin_call->pragmas = GA->pragmas;
         builtin_call->use_set = GA->use_set;
         builtin_call->clobbered_set = GA->clobbered_set;
         builtin_call->scpe = GA->scpe;
         builtin_call->bb_index = GA->bb_index;
         builtin_call->include_name = GA->include_name;
         builtin_call->line_number = GA->line_number;
         builtin_call->column_number = GA->column_number;

         GA->memdef = nullptr;
         GA->memuse = builtin_call->memdef;
         GA->vdef = nullptr;
         GA->vuses.clear();
         GA->vovers.clear();
         THROW_ASSERT(builtin_call->vdef, "Unexpected condition");
         GA->AddVuse(builtin_call->vdef);

         GA->pragmas.clear();
         GA->use_set = PointToSolutionRef(new PointToSolution());
         GA->clobbered_set = PointToSolutionRef(new PointToSolution());
      }
   }
   else
   {
      THROW_UNREACHABLE("Error not a gimple call or assign statement!");
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
