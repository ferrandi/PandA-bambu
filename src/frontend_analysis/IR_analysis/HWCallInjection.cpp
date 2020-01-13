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
 *            Copyright (C) 2004-2020 Politecnico di Milano
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

/// parser/treegcc include
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

HWCallInjection::HWCallInjection(const ParameterConstRef Param, const application_managerRef _AppM, unsigned int funId, const DesignFlowManagerConstRef DFM) : FunctionFrontendFlowStep(_AppM, funId, HWCALL_INJECTION, DFM, Param), already_executed(false)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

HWCallInjection::~HWCallInjection() = default;

DesignFlowStep_Status HWCallInjection::InternalExec()
{
   tree_managerRef TM = AppM->get_tree_manager();
   tree_nodeRef tn = TM->get_tree_node_const(function_id);
   auto* fd = GetPointer<function_decl>(tn);
   THROW_ASSERT(fd && fd->body, "Node is not a function or it hasn't a body");
   auto* sl = GetPointer<statement_list>(GET_NODE(fd->body));
   THROW_ASSERT(sl, "Body is not a statement_list");

   for(const auto& block : sl->list_of_bloc)
   {
      const auto list_of_stmt = block.second->CGetStmtList();
      auto stmt = list_of_stmt.begin();
      while(stmt != list_of_stmt.end())
      {
         stmt++;
         if(isHardwareCall(GET_NODE(*(std::prev(stmt)))))
         {
            buildBuiltinCall(block.second, *(std::prev(stmt)));
         }
      }
   }
   already_executed = true;
   return DesignFlowStep_Status::SUCCESS;
}

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> HWCallInjection::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType RT) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(RT)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
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
         THROW_UNREACHABLE("HWCallInjection::ComputeFrontendRelationships");
      }
   }
   return relationships;
}

bool HWCallInjection::isHardwareCall(tree_nodeRef expr)
{
   tree_managerRef TM = AppM->get_tree_manager();
   const ParameterConstRef Param = AppM->get_parameter();

   tree_nodeRef FD;
   if(expr->get_kind() == gimple_call_K)
   {
      auto* GC = GetPointer<gimple_call>(expr);
      FD = GetPointer<addr_expr>(GET_NODE(GC->fn)) ? GetPointer<addr_expr>(GET_NODE(GC->fn))->op : GC->fn;
   }
   else if(expr->get_kind() == gimple_assign_K)
   {
      auto* GA = GetPointer<gimple_assign>(expr);
      if(GET_NODE(GA->op1)->get_kind() == call_expr_K || GET_NODE(GA->op1)->get_kind() == aggr_init_expr_K)
      {
         auto* CE = GetPointer<call_expr>(GET_NODE(GA->op1));
         FD = GetPointer<addr_expr>(GET_NODE(CE->fn)) ? GetPointer<addr_expr>(GET_NODE(CE->fn))->op : CE->fn;
      }
   }

   // When the instruction is not a function call return false.
   if(!FD)
      return false;

   bool result = false;
   if(GET_NODE(FD)->get_kind() == function_decl_K)
   {
      auto* FDPtr = GetPointer<function_decl>(GET_NODE(FD));
      result = FDPtr->hwcall_flag;

      if(!result)
      {
         std::string name = tree_helper::name_function(TM, GET_INDEX_NODE(FD));
         std::string cmdArg = Param->getOption<std::string>(OPT_additional_top);

         std::vector<std::string> additionalTops = SplitString(cmdArg, ",");

         result |= std::find(additionalTops.begin(), additionalTops.end(), name) != additionalTops.end();
      }
   }
   else if(GET_NODE(FD)->get_kind() == ssa_name_K)
   {
      // This is the case for function pointers call.
      result = true;
   }

   return result;
}

void HWCallInjection::buildBuiltinCall(const blocRef block, const tree_nodeRef stmt)
{
   tree_nodeRef expr = GET_NODE(stmt);
   tree_managerRef TM = AppM->get_tree_manager();
   tree_manipulationRef IRman = tree_manipulationRef(new tree_manipulation(TM, parameters));

   unsigned int retVar = 0;

   if(!builtinWaitCallDeclIdx)
   {
      unsigned int varArgParamList = TM->new_tree_node_id();
      std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> varArgParamMap;
      varArgParamMap[TOK(TOK_VALU)] = STR(GET_INDEX_NODE(IRman->create_default_integer_type()));
      TM->create_tree_node(varArgParamList, tree_list_K, varArgParamMap);

      unsigned int builtinIdString = GET_INDEX_NODE(IRman->create_identifier_node(BUILTIN_WAIT_CALL));

      unsigned int funTypeSizeIdx = TM->new_tree_node_id();
      std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> funTypeSize;
      funTypeSize[TOK(TOK_TYPE)] = STR(GET_INDEX_NODE(IRman->create_default_integer_type()));
      funTypeSize[TOK(TOK_VALUE)] = STR(8);
      TM->create_tree_node(funTypeSizeIdx, integer_cst_K, funTypeSize);

      std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> builtinFunTypeMap;
      builtinFunTypeMap[TOK(TOK_RETN)] = STR(GET_INDEX_NODE(IRman->create_void_type()));
      builtinFunTypeMap[TOK(TOK_VARARGS)] = STR(1);
      builtinFunTypeMap[TOK(TOK_PRMS)] = STR(varArgParamList);
      builtinFunTypeMap[TOK(TOK_SIZE)] = STR(funTypeSizeIdx);
      builtinFunTypeMap[TOK(TOK_ALIGNED)] = STR(8);

      unsigned int builtinFunctionTypeIdx = TM->new_tree_node_id();
      TM->create_tree_node(builtinFunctionTypeIdx, function_type_K, builtinFunTypeMap);

      std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> builtinFunctionDeclMap;
      builtinFunctionDeclMap[TOK(TOK_TYPE)] = STR(builtinFunctionTypeIdx);
      builtinFunctionDeclMap[TOK(TOK_NAME)] = STR(builtinIdString);
      builtinFunctionDeclMap[TOK(TOK_SRCP)] = "<builtin>:0:0";
      builtinWaitCallDeclIdx = TM->new_tree_node_id();
      TM->create_tree_node(builtinWaitCallDeclIdx, function_decl_K, builtinFunctionDeclMap);
   }

   auto* srcPtr = GetPointer<srcp>(expr);

   tree_nodeRef functionDeclTN = TM->GetTreeNode(builtinWaitCallDeclIdx);
   auto* functionDecl = GetPointer<function_decl>(functionDeclTN);

   unsigned int addrExprBuiltinCall = TM->new_tree_node_id();
   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> addrExprBuiltinCallMap;
   addrExprBuiltinCallMap[TOK(TOK_TYPE)] = STR(GET_INDEX_NODE(IRman->create_pointer_type(functionDecl->type, 8)));
   addrExprBuiltinCallMap[TOK(TOK_OP)] = STR(builtinWaitCallDeclIdx);
   addrExprBuiltinCallMap[TOK(TOK_SRCP)] = srcPtr->include_name + ":" + STR(srcPtr->line_number) + ":" + STR(srcPtr->column_number);
   TM->create_tree_node(addrExprBuiltinCall, addr_expr_K, addrExprBuiltinCallMap);

   unsigned int gimpleCallIdx = TM->new_tree_node_id();
   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> gimpleCallMap;
   gimpleCallMap[TOK(TOK_FN)] = STR(addrExprBuiltinCall);
   gimpleCallMap[TOK(TOK_SRCP)] = srcPtr->include_name + ":" + STR(srcPtr->line_number) + ":" + STR(srcPtr->column_number);
   TM->create_tree_node(gimpleCallIdx, gimple_call_K, gimpleCallMap);

   tree_nodeRef builtinGimpleCallTN = TM->GetTreeNode(gimpleCallIdx);
   tree_nodeRef builtinCallTN = TM->GetTreeReindex(gimpleCallIdx);
   auto* builtinGimpleCall = GetPointer<gimple_call>(builtinGimpleCallTN);
   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> HasReturnMap;
   HasReturnMap[TOK(TOK_TYPE)] = STR(GET_INDEX_NODE(IRman->create_default_integer_type()));
   if(GetPointer<gimple_call>(expr))
   {
      auto* GC = GetPointer<gimple_call>(expr);
      builtinGimpleCall->AddArg(GC->fn);

      unsigned int HasReturnIdx = TM->new_tree_node_id();
      HasReturnMap[TOK(TOK_VALUE)] = STR(0);
      TM->create_tree_node(HasReturnIdx, integer_cst_K, HasReturnMap);
      builtinGimpleCall->AddArg(TM->GetTreeReindex(HasReturnIdx));

      for(std::vector<tree_nodeRef>::const_iterator argItr = GC->args.begin(), argEnd = GC->args.end(); argItr != argEnd; ++argItr)
      {
         builtinGimpleCall->AddArg(*argItr);
      }

      builtinGimpleCall->memuse = GC->memuse;
      builtinGimpleCall->memdef = GC->memdef;
      ssa_name* ssamemdef = builtinGimpleCall->memdef ? GetPointer<ssa_name>(GET_NODE(builtinGimpleCall->memdef)) : nullptr;
      if(ssamemdef)
      {
         ssamemdef->SetDefStmt(builtinCallTN);
      }

      builtinGimpleCall->vuses = GC->vuses;
      builtinGimpleCall->vovers = GC->vovers;
      builtinGimpleCall->vdef = GC->vdef;
      ssa_name* ssavdef = builtinGimpleCall->vdef ? GetPointer<ssa_name>(GET_NODE(builtinGimpleCall->vdef)) : nullptr;
      if(ssavdef)
      {
         ssavdef->SetDefStmt(builtinCallTN);
      }

      builtinGimpleCall->pragmas = GC->pragmas;
      builtinGimpleCall->use_set = GC->use_set;
      builtinGimpleCall->clobbered_set = GC->clobbered_set;
      builtinGimpleCall->scpe = GC->scpe;
      builtinGimpleCall->bb_index = GC->bb_index;
      builtinGimpleCall->include_name = GC->include_name;
      builtinGimpleCall->line_number = GC->line_number;
      builtinGimpleCall->column_number = GC->column_number;

      GC->memuse = tree_nodeRef();
      GC->memdef = tree_nodeRef();

      GC->vdef = tree_nodeRef();
      GC->vovers = TreeNodeSet();
      GC->vuses = TreeNodeSet();

      GC->pragmas = std::vector<tree_nodeRef>();
      GC->use_set = PointToSolutionRef(new PointToSolution());
      GC->clobbered_set = PointToSolutionRef(new PointToSolution());
   }
   else if(GetPointer<gimple_assign>(expr))
   {
      auto* GA = GetPointer<gimple_assign>(expr);
      if(GET_NODE(GA->op1)->get_kind() == call_expr_K || GET_NODE(GA->op1)->get_kind() == aggr_init_expr_K)
      {
         auto* CE = GetPointer<call_expr>(GET_NODE(GA->op1));
         builtinGimpleCall->AddArg(CE->fn);

         unsigned int HasReturnIdx = TM->new_tree_node_id();
         HasReturnMap[TOK(TOK_VALUE)] = STR(1);
         TM->create_tree_node(HasReturnIdx, integer_cst_K, HasReturnMap);
         builtinGimpleCall->AddArg(TM->GetTreeReindex(HasReturnIdx));

         for(std::vector<tree_nodeRef>::const_iterator argItr = CE->args.begin(), argEnd = CE->args.end(); argItr != argEnd; ++argItr)
         {
            builtinGimpleCall->AddArg(*argItr);
         }

         if(auto* ssaRet = GetPointer<ssa_name>(GET_NODE(GA->op0)))
         {
            retVar = TM->new_tree_node_id();
            std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> retVarMap;

            retVarMap[TOK(TOK_SRCP)] = GA->include_name + ":" + STR(GA->line_number) + ":" + STR(GA->column_number);
            if(ssaRet->type)
            {
               retVarMap[TOK(TOK_TYPE)] = STR(GET_INDEX_NODE(ssaRet->type));
               retVarMap[TOK(TOK_SIZE)] = STR(GET_INDEX_NODE(GetPointer<type_node>(GET_NODE(ssaRet->type))->size));
               retVarMap[TOK(TOK_ALGN)] = STR(GetPointer<type_node>(GET_NODE(ssaRet->type))->algn);
            }
            else
            {
               auto* vd = GetPointer<var_decl>(GET_NODE(ssaRet->var));
               retVarMap[TOK(TOK_TYPE)] = STR(GET_INDEX_NODE(vd->type));
               retVarMap[TOK(TOK_SIZE)] = STR(GET_INDEX_NODE(GetPointer<type_node>(GET_NODE(vd->type))->size));
               retVarMap[TOK(TOK_ALGN)] = STR(GetPointer<type_node>(GET_NODE(vd->type))->algn);
            }
            std::string var_name = std::string("__return_value_") + STR(retVar);
            retVarMap[TOK(TOK_NAME)] = STR(GET_INDEX_NODE(IRman->create_identifier_node(var_name)));
            retVarMap[TOK(TOK_SCPE)] = STR(GET_INDEX_NODE(GA->scpe));
            retVarMap[TOK(TOK_USED)] = STR(1);
            retVarMap[TOK(TOK_USE_TMPL)] = STR(-1);
            retVarMap[TOK(TOK_STATIC_STATIC)] = STR(false);
            retVarMap[TOK(TOK_EXTERN)] = STR(false);
            retVarMap[TOK(TOK_STATIC)] = STR(false);
            retVarMap[TOK(TOK_REGISTER)] = STR(false);
            retVarMap[TOK(TOK_ARTIFICIAL)] = STR(true);
            TM->create_tree_node(retVar, var_decl_K, retVarMap);

            GA->op1 = TM->GetTreeReindex(retVar);
         }

         if(!retVar)
            retVar = GET_INDEX_NODE(GA->op0);

         unsigned int addrExprReturnValue = TM->new_tree_node_id();
         std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> addrExprReturnValueMap;
         auto typeRetVar = TM->GetTreeReindex(tree_helper::get_type_index(TM, retVar));
         addrExprReturnValueMap[TOK(TOK_TYPE)] = STR(GET_INDEX_NODE(IRman->create_pointer_type(typeRetVar, ALGN_POINTER)));
         addrExprReturnValueMap[TOK(TOK_OP)] = STR(retVar);
         addrExprReturnValueMap[TOK(TOK_SRCP)] = GA->include_name + ":" + STR(GA->line_number) + ":" + STR(GA->column_number);
         TM->create_tree_node(addrExprReturnValue, addr_expr_K, addrExprReturnValueMap);
         builtinGimpleCall->AddArg(TM->GetTreeReindex(addrExprReturnValue));

         builtinGimpleCall->memuse = GA->memuse;
         builtinGimpleCall->memdef = GA->memdef;
         ssa_name* ssamemdef = builtinGimpleCall->memdef ? GetPointer<ssa_name>(GET_NODE(builtinGimpleCall->memdef)) : nullptr;
         if(ssamemdef)
         {
            ssamemdef->SetDefStmt(builtinCallTN);
         }

         builtinGimpleCall->vuses = GA->vuses;
         builtinGimpleCall->vovers = GA->vovers;
         builtinGimpleCall->vdef = GA->vdef;
         ssa_name* ssavdef = builtinGimpleCall->vdef ? GetPointer<ssa_name>(GET_NODE(builtinGimpleCall->vdef)) : nullptr;
         if(ssavdef)
         {
            ssavdef->SetDefStmt(builtinCallTN);
         }

         builtinGimpleCall->pragmas = GA->pragmas;
         builtinGimpleCall->use_set = GA->use_set;
         builtinGimpleCall->clobbered_set = GA->clobbered_set;
         builtinGimpleCall->scpe = GA->scpe;
         builtinGimpleCall->bb_index = GA->bb_index;
         builtinGimpleCall->include_name = GA->include_name;
         builtinGimpleCall->line_number = GA->line_number;
         builtinGimpleCall->column_number = GA->column_number;

         GA->memuse = builtinGimpleCall->memdef;
         GA->memdef = tree_nodeRef();

         GA->vdef = tree_nodeRef();
         GA->vovers = TreeNodeSet();
         GA->vuses = TreeNodeSet();
         THROW_ASSERT(builtinGimpleCall->vdef, "");
         GA->vuses.insert(builtinGimpleCall->vdef);
         GetPointer<ssa_name>(GET_NODE(builtinGimpleCall->vdef))->AddUseStmt(stmt);

         GA->pragmas = std::vector<tree_nodeRef>();
         GA->use_set = PointToSolutionRef(new PointToSolution());
         GA->clobbered_set = PointToSolutionRef(new PointToSolution());
      }
   }
   else
   {
      THROW_UNREACHABLE("Error not a gimple call or assign statement!");
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---adding to BB" + STR(block->number) + " stmt: " + builtinCallTN->ToString());
   block->PushBefore(builtinCallTN, stmt);
   if(not retVar)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---removing from BB" + STR(block->number) + " stmt: " + stmt->ToString());
      block->RemoveStmt(stmt);
   }
}

bool HWCallInjection::HasToBeExecuted() const
{
   return not already_executed;
}
