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
 *            Copyright (C) 2004-2021 Politecnico di Milano
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
#include "tree_reindex.hpp"

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

HWCallInjection::HWCallInjection(const ParameterConstRef Param, const application_managerRef _AppM, unsigned int funId, const DesignFlowManagerConstRef DFM) : FunctionFrontendFlowStep(_AppM, funId, HWCALL_INJECTION, DFM, Param)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

HWCallInjection::~HWCallInjection() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> HWCallInjection::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType RT) const
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
   const auto fd = GetPointer<const function_decl>(TM->CGetTreeNode(function_id));
   THROW_ASSERT(fd && fd->body, "Node is not a function or it hasn't a body");
   const auto sl = GetPointerS<statement_list>(GET_NODE(fd->body));
   THROW_ASSERT(sl, "Body is not a statement_list");

   const auto isHardwareCall = [&](const tree_nodeRef& expr) -> bool {
      tree_nodeRef FD;
      if(expr->get_kind() == gimple_call_K)
      {
         const auto GC = GetPointerS<gimple_call>(expr);
         FD = GetPointer<const addr_expr>(GET_CONST_NODE(GC->fn)) ? GetPointerS<const addr_expr>(GET_CONST_NODE(GC->fn))->op : GC->fn;
      }
      else if(expr->get_kind() == gimple_assign_K)
      {
         const auto GA = GetPointerS<const gimple_assign>(expr);
         if(GET_CONST_NODE(GA->op1)->get_kind() == call_expr_K || GET_CONST_NODE(GA->op1)->get_kind() == aggr_init_expr_K)
         {
            const auto CE = GetPointerS<const call_expr>(GET_CONST_NODE(GA->op1));
            FD = GetPointer<const addr_expr>(GET_CONST_NODE(CE->fn)) ? GetPointerS<const addr_expr>(GET_CONST_NODE(CE->fn))->op : CE->fn;
         }
      }

      // When the instruction is not a function call return false.
      bool result = false;
      if(FD)
      {
         if(GET_CONST_NODE(FD)->get_kind() == function_decl_K)
         {
            const auto FDPtr = GetPointerS<const function_decl>(GET_CONST_NODE(FD));
            result = FDPtr->hwcall_flag;
            if(!result)
            {
               const auto cmdArg = parameters->getOption<std::string>(OPT_additional_top);
               const auto additionalTops = SplitString(cmdArg, ",");
               const auto name = tree_helper::print_function_name(TM, FDPtr);
               result |= std::find(additionalTops.begin(), additionalTops.end(), name) != additionalTops.end();
            }
         }
         else if(GET_CONST_NODE(FD)->get_kind() == ssa_name_K)
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
         if(isHardwareCall(GET_NODE(cur_stmt)))
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
   const auto expr = GET_NODE(stmt);
   const auto TM = AppM->get_tree_manager();
   const auto IRman = tree_manipulationRef(new tree_manipulation(TM, parameters, AppM));

   tree_nodeRef retVar = nullptr;
   if(!builtinWaitCallDeclIdx)
   {
      const auto varArgParamList = TM->new_tree_node_id();
      {
         std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> varArgParamMap;
         varArgParamMap[TOK(TOK_VALU)] = STR(GET_INDEX_NODE(IRman->GetSignedIntegerType()));
         TM->create_tree_node(varArgParamList, tree_list_K, varArgParamMap);
      }

      const auto builtinFunctionTypeIdx = TM->new_tree_node_id();
      {
         std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> builtinFunTypeMap;
         builtinFunTypeMap[TOK(TOK_RETN)] = STR(GET_INDEX_NODE(IRman->GetVoidType()));
         builtinFunTypeMap[TOK(TOK_VARARGS)] = STR(1);
         builtinFunTypeMap[TOK(TOK_PRMS)] = STR(varArgParamList);
         const auto funTypeSize = TM->CreateUniqueIntegerCst(8, IRman->GetSignedIntegerType());
         builtinFunTypeMap[TOK(TOK_SIZE)] = STR(GET_INDEX_CONST_NODE(funTypeSize));
         builtinFunTypeMap[TOK(TOK_ALIGNED)] = STR(8);
         TM->create_tree_node(builtinFunctionTypeIdx, function_type_K, builtinFunTypeMap);
      }

      builtinWaitCallDeclIdx = TM->new_tree_node_id();
      {
         std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> builtinFunctionDeclMap;
         builtinFunctionDeclMap[TOK(TOK_TYPE)] = STR(builtinFunctionTypeIdx);
         const auto builtinIdString = IRman->create_identifier_node(BUILTIN_WAIT_CALL);
         builtinFunctionDeclMap[TOK(TOK_NAME)] = STR(GET_INDEX_CONST_NODE(builtinIdString));
         builtinFunctionDeclMap[TOK(TOK_SRCP)] = BUILTIN_SRCP;
         TM->create_tree_node(builtinWaitCallDeclIdx, function_decl_K, builtinFunctionDeclMap);
      }
   }

   const auto srcPtr = GetPointer<srcp>(expr);
   const auto srcp_str = srcPtr->include_name + ":" + STR(srcPtr->line_number) + ":" + STR(srcPtr->column_number);
   const auto functionDecl = GetPointerS<const function_decl>(TM->CGetTreeNode(builtinWaitCallDeclIdx));

   const auto addrExprBuiltinCall = TM->new_tree_node_id();
   {
      std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> addrExprBuiltinCallMap;
      addrExprBuiltinCallMap[TOK(TOK_TYPE)] = STR(GET_INDEX_NODE(IRman->GetPointerType(functionDecl->type, 8)));
      addrExprBuiltinCallMap[TOK(TOK_OP)] = STR(builtinWaitCallDeclIdx);
      addrExprBuiltinCallMap[TOK(TOK_SRCP)] = srcp_str;
      TM->create_tree_node(addrExprBuiltinCall, addr_expr_K, addrExprBuiltinCallMap);
   }

   const auto gimpleCallIdx = TM->new_tree_node_id();
   {
      std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> gimpleCallMap;
      gimpleCallMap[TOK(TOK_FN)] = STR(addrExprBuiltinCall);
      if(expr->get_kind() == gimple_call_K)
      {
         const auto GC = GetPointerS<const gimple_call>(expr);
         gimpleCallMap[TOK(TOK_SCPE)] = STR(GET_INDEX_NODE(GC->scpe));
      }
      else if(expr->get_kind() == gimple_assign_K)
      {
         const auto GA = GetPointerS<const gimple_assign>(expr);
         gimpleCallMap[TOK(TOK_SCPE)] = STR(GET_INDEX_NODE(GA->scpe));
      }
      gimpleCallMap[TOK(TOK_SRCP)] = srcp_str;
      TM->create_tree_node(gimpleCallIdx, gimple_call_K, gimpleCallMap);
   }

   const auto builtinGimpleCallTN = TM->GetTreeNode(gimpleCallIdx);
   const auto builtinCallTN = TM->GetTreeReindex(gimpleCallIdx);
   const auto builtinGimpleCall = GetPointerS<gimple_call>(builtinGimpleCallTN);
   if(expr->get_kind() == gimple_call_K)
   {
      const auto GC = GetPointerS<gimple_call>(expr);
      builtinGimpleCall->AddArg(GC->fn);

      const auto has_return = TM->CreateUniqueIntegerCst(0, IRman->GetSignedIntegerType());
      builtinGimpleCall->AddArg(has_return);

      for(const auto& arg : GC->args)
      {
         builtinGimpleCall->AddArg(arg);
      }

      builtinGimpleCall->memuse = GC->memuse;
      builtinGimpleCall->memdef = GC->memdef;

      builtinGimpleCall->vuses = GC->vuses;
      builtinGimpleCall->vovers = GC->vovers;
      builtinGimpleCall->vdef = GC->vdef;

      builtinGimpleCall->pragmas = GC->pragmas;
      builtinGimpleCall->use_set = GC->use_set;
      builtinGimpleCall->clobbered_set = GC->clobbered_set;
      builtinGimpleCall->scpe = GC->scpe;
      builtinGimpleCall->bb_index = GC->bb_index;
      builtinGimpleCall->include_name = GC->include_name;
      builtinGimpleCall->line_number = GC->line_number;
      builtinGimpleCall->column_number = GC->column_number;

      GC->memuse = nullptr;
      GC->memdef = nullptr;
      GC->vdef = nullptr;
      GC->vuses.clear();
      GC->vovers.clear();

      GC->pragmas.clear();
      GC->use_set = PointToSolutionRef(new PointToSolution());
      GC->clobbered_set = PointToSolutionRef(new PointToSolution());
   }
   else if(expr->get_kind() == gimple_assign_K)
   {
      const auto GA = GetPointerS<gimple_assign>(expr);
      if(GET_NODE(GA->op1)->get_kind() == call_expr_K || GET_NODE(GA->op1)->get_kind() == aggr_init_expr_K)
      {
         const auto CE = GetPointerS<const call_expr>(GET_CONST_NODE(GA->op1));
         builtinGimpleCall->AddArg(CE->fn);

         const auto has_return = TM->CreateUniqueIntegerCst(1, IRman->GetSignedIntegerType());
         builtinGimpleCall->AddArg(has_return);

         for(const auto& arg : CE->args)
         {
            builtinGimpleCall->AddArg(arg);
         }

         if(const auto ssaRet = GetPointer<const ssa_name>(GET_CONST_NODE(GA->op0)))
         {
            tree_nodeRef ret_var_type, ret_var_size;
            unsigned int ret_var_algn;
            if(ssaRet->type)
            {
               ret_var_type = ssaRet->type;
               ret_var_size = GetPointerS<const type_node>(GET_CONST_NODE(ssaRet->type))->size;
               ret_var_algn = GetPointerS<const type_node>(GET_CONST_NODE(ssaRet->type))->algn;
            }
            else
            {
               const auto vd = GetPointerS<const var_decl>(GET_CONST_NODE(ssaRet->var));
               ret_var_type = vd->type;
               ret_var_size = GetPointerS<const type_node>(GET_CONST_NODE(vd->type))->size;
               ret_var_algn = GetPointerS<const type_node>(GET_CONST_NODE(vd->type))->algn;
            }
            retVar = IRman->create_var_decl(IRman->create_identifier_node("__return_value"), ret_var_type, GA->scpe, ret_var_size, nullptr, nullptr, STR(GA->include_name + ":" + STR(GA->line_number) + ":" + STR(GA->column_number)), ret_var_algn, 1, true);

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
            addrExprReturnValueMap[TOK(TOK_TYPE)] = STR(GET_INDEX_NODE(IRman->GetPointerType(typeRetVar, ALGN_POINTER)));
            addrExprReturnValueMap[TOK(TOK_OP)] = STR(GET_INDEX_CONST_NODE(retVar));
            addrExprReturnValueMap[TOK(TOK_SRCP)] = srcp_str;
            TM->create_tree_node(addrExprReturnValue, addr_expr_K, addrExprReturnValueMap);
         }
         builtinGimpleCall->AddArg(TM->GetTreeReindex(addrExprReturnValue));

         builtinGimpleCall->memuse = GA->memuse;
         builtinGimpleCall->memdef = GA->memdef;
         builtinGimpleCall->vuses = GA->vuses;
         builtinGimpleCall->vovers = GA->vovers;
         builtinGimpleCall->vdef = GA->vdef;

         builtinGimpleCall->pragmas = GA->pragmas;
         builtinGimpleCall->use_set = GA->use_set;
         builtinGimpleCall->clobbered_set = GA->clobbered_set;
         builtinGimpleCall->scpe = GA->scpe;
         builtinGimpleCall->bb_index = GA->bb_index;
         builtinGimpleCall->include_name = GA->include_name;
         builtinGimpleCall->line_number = GA->line_number;
         builtinGimpleCall->column_number = GA->column_number;

         GA->memdef = nullptr;
         GA->memuse = builtinGimpleCall->memdef;

         GA->vdef = nullptr;
         GA->vuses.clear();
         GA->vovers.clear();
         THROW_ASSERT(builtinGimpleCall->vdef, "");
         GA->vuses.push_back(builtinGimpleCall->vdef);

         GA->pragmas.clear();
         GA->use_set = PointToSolutionRef(new PointToSolution());
         GA->clobbered_set = PointToSolutionRef(new PointToSolution());
      }
   }
   else
   {
      THROW_UNREACHABLE("Error not a gimple call or assign statement!");
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Added to BB" + STR(block->number) + " " + STR(builtinCallTN));
   block->PushBefore(builtinCallTN, stmt, AppM);
   if(!retVar)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Removed from BB" + STR(block->number) + " " + STR(stmt));
      block->RemoveStmt(stmt, AppM);
   }
   else
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Modified " + STR(stmt));
   }
}
