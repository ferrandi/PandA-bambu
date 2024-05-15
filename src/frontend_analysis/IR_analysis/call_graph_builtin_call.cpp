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

#include "call_graph_builtin_call.hpp"

#include "custom_map.hpp"
#include <string>

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "design_flow_manager.hpp"
#include "ext_tree_node.hpp"
#include "function_behavior.hpp"
#include "function_decl_refs.hpp"
#include "op_graph.hpp"
#include "string_manipulation.hpp" // for GET_CLASS
#include "symbolic_application_frontend_flow_step.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"

static tree_nodeRef getFunctionPointerType(tree_nodeRef fptr);
void CallGraphBuiltinCall::lookForBuiltinCall(const tree_nodeRef TN)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "-->Update recursively node: " + STR(TN) + " id: " + STR(TN->index));

   const tree_managerRef TM = AppM->get_tree_manager();
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + TN->get_kind_text());
   switch(TN->get_kind())
   {
      case call_expr_K:
      case aggr_init_expr_K:
      {
         const call_expr* CE = GetPointer<call_expr>(TN);

         tree_nodeRef FN = GetPointer<addr_expr>(CE->fn) ? GetPointer<addr_expr>(CE->fn)->op : CE->fn;
         THROW_ASSERT(FN, "Address expression with null op");
         if(FN->get_kind() == function_decl_K)
         {
            unsigned int functionDeclIdx = FN->index;
            std::string funName = tree_helper::name_function(TM, functionDeclIdx);
            if(funName == BUILTIN_WAIT_CALL)
            {
               const std::vector<tree_nodeRef>& args = CE->args;
               tree_nodeRef builtinArgZero =
                   GetPointer<addr_expr>(args[0]) ? GetPointer<addr_expr>(args[0])->op : args[0];
               if(builtinArgZero->get_kind() == function_decl_K)
               {
                  unsigned int calledFunctionId = builtinArgZero->index;
                  CallGraphManager::addCallPointAndExpand(already_visited, AppM, function_id, calledFunctionId,
                                                          TN->index, FunctionEdgeInfo::CallType::indirect_call,
                                                          debug_level);
                  modified = true;
               }
               else if(builtinArgZero->get_kind() == ssa_name_K)
               {
                  tree_nodeRef funPtrType = getFunctionPointerType(builtinArgZero);
                  ExtendCallGraph(function_id, funPtrType, CE->index);
               }
            }
         }
         break;
      }
      case gimple_call_K:
      {
         const gimple_call* CE = GetPointer<gimple_call>(TN);

         tree_nodeRef FN = GetPointer<addr_expr>(CE->fn) ? GetPointer<addr_expr>(CE->fn)->op : CE->fn;
         THROW_ASSERT(FN, "Address expression with null op");
         if(FN->get_kind() == function_decl_K)
         {
            unsigned int functionDeclIdx = FN->index;
            std::string funName = tree_helper::name_function(TM, functionDeclIdx);
            if(funName == BUILTIN_WAIT_CALL)
            {
               const std::vector<tree_nodeRef>& args = CE->args;
               tree_nodeRef builtinArgZero =
                   GetPointer<addr_expr>(args[0]) ? GetPointer<addr_expr>(args[0])->op : args[0];
               if(builtinArgZero->get_kind() == function_decl_K)
               {
                  unsigned int calledFunctionId = builtinArgZero->index;
                  CallGraphManager::addCallPointAndExpand(already_visited, AppM, function_id, calledFunctionId,
                                                          TN->index, FunctionEdgeInfo::CallType::indirect_call,
                                                          debug_level);
                  modified = true;
               }
               else if(builtinArgZero->get_kind() == ssa_name_K)
               {
                  // Function pointers case.
                  tree_nodeRef funPtrType = getFunctionPointerType(builtinArgZero);
                  ExtendCallGraph(function_id, funPtrType, CE->index);
               }
            }
         }
         break;
      }
      case gimple_assign_K:
      {
         auto* gm = GetPointer<gimple_assign>(TN);
         if(GetPointer<call_expr>(gm->op1))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---call_expr " + STR(gm->op1));
            lookForBuiltinCall(gm->op1);
         }
         break;
      }
      case CASE_BINARY_EXPRESSION:
      case CASE_CPP_NODES:
      case CASE_FAKE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_QUATERNARY_EXPRESSION:
      case CASE_TERNARY_EXPRESSION:
      case CASE_TYPE_NODES:
      case CASE_UNARY_EXPRESSION:
      case binfo_K:
      case block_K:
      case case_label_expr_K:
      case complex_cst_K:
      case const_decl_K:
      case constructor_K:
      case error_mark_K:
      case field_decl_K:
      case function_decl_K:
      case gimple_asm_K:
      case gimple_bind_K:
      case gimple_cond_K:
      case gimple_for_K:
      case gimple_goto_K:
      case gimple_label_K:
      case gimple_multi_way_if_K:
      case gimple_nop_K:
      case gimple_phi_K:
      case gimple_pragma_K:
      case gimple_predict_K:
      case gimple_resx_K:
      case gimple_return_K:
      case gimple_switch_K:
      case gimple_while_K:
      case identifier_node_K:
      case integer_cst_K:
      case label_decl_K:
      case lut_expr_K:
      case namespace_decl_K:
      case parm_decl_K:
      case real_cst_K:
      case result_decl_K:
      case ssa_name_K:
      case statement_list_K:
      case string_cst_K:
      case target_expr_K:
      case target_mem_ref461_K:
      case target_mem_ref_K:
      case template_decl_K:
      case translation_unit_decl_K:
      case tree_list_K:
      case tree_vec_K:
      case type_decl_K:
      case using_decl_K:
      case var_decl_K:
      case vector_cst_K:
      case void_cst_K:
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Updated recursively " + STR(TN->index));
}

void CallGraphBuiltinCall::ExtendCallGraph(unsigned int callerIdx, tree_nodeRef funType, unsigned int stmtIdx)
{
   std::string type = tree_helper::print_type(AppM->get_tree_manager(), funType->index);
   buildTypeToDeclaration();
   for(unsigned int Itr : typeToDeclaration[type])
   {
      CallGraphManager::addCallPointAndExpand(already_visited, AppM, callerIdx, Itr, stmtIdx,
                                              FunctionEdgeInfo::CallType::indirect_call, debug_level);
      modified = true;
   }
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, CallGraphBuiltinCall::FunctionRelationship>>
CallGraphBuiltinCall::ComputeFrontendRelationships(DesignFlowStep::RelationshipType RT) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(RT)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(HWCALL_INJECTION, SAME_FUNCTION));
         relationships.insert(std::make_pair(FUNCTION_ANALYSIS, WHOLE_APPLICATION));
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      default:
         THROW_UNREACHABLE("HWCallInjection::ComputeFrontendRelationships");
   }
   return relationships;
}

CallGraphBuiltinCall::CallGraphBuiltinCall(const application_managerRef AM, unsigned int functionId,
                                           const DesignFlowManagerConstRef DFM, const ParameterConstRef P)
    : FunctionFrontendFlowStep(AM, functionId, CALL_GRAPH_BUILTIN_CALL, DFM, P),
      modified(false),
      typeToDeclarationBuilt(false)
{
   debug_level = P->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CallGraphBuiltinCall::~CallGraphBuiltinCall() = default;

void CallGraphBuiltinCall::Initialize()
{
}

void CallGraphBuiltinCall::buildTypeToDeclaration()
{
   if(!typeToDeclarationBuilt)
   {
      const auto TM = AppM->get_tree_manager();
      CustomUnorderedSet<unsigned int> allFunctions;
      function_decl_refs fdr_visitor(allFunctions);
      for(const auto root_function : AppM->GetCallGraphManager()->GetRootFunctions())
      {
         TM->GetTreeNode(root_function)->visit(&fdr_visitor);
      }
      for(unsigned int allFunction : allFunctions)
      {
         std::string functionName = tree_helper::name_function(TM, allFunction);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Analyzing function " + STR(allFunction) + " " + functionName);
         auto* funDecl = GetPointer<function_decl>(TM->GetTreeNode(allFunction));
         std::string type = tree_helper::print_type(TM, funDecl->type->index);
         if(funDecl->body && functionName != "__start_pragma__" && functionName != "__close_pragma__" &&
            !starts_with(functionName, "__pragma__"))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---FunctionTypeString " + type);
         }
         typeToDeclaration[type].insert(allFunction);
      }
      typeToDeclarationBuilt = true;
   }
}

DesignFlowStep_Status CallGraphBuiltinCall::InternalExec()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->BuiltinWaitCall Analysis");
   tree_managerRef TM = AppM->get_tree_manager();
   auto* functionDecl = GetPointer<function_decl>(TM->GetTreeNode(function_id));
   auto* stmtList = GetPointer<statement_list>(functionDecl->body);

   if(parameters->getOption<bool>(OPT_print_dot) && DEBUG_LEVEL_PEDANTIC <= debug_level &&
      (!parameters->IsParameter("print-dot-FF") || parameters->GetParameter<unsigned int>("print-dot-FF")))
   {
      AppM->CGetCallGraphManager()->CGetCallGraph()->WriteDot("builtin-graph-pre" + STR(function_id) + ".dot");
   }
   already_visited.clear();
   modified = false;
   typeToDeclaration.clear();
   typeToDeclarationBuilt = false;

   for(const auto& block : stmtList->list_of_bloc)
   {
      for(const auto& stmt : block.second->CGetStmtList())
      {
         if(stmt->get_kind() == gimple_call_K or stmt->get_kind() == gimple_assign_K)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Analyzing stmt " + stmt->ToString());
            lookForBuiltinCall(stmt);
         }
      }
   }

   if(parameters->getOption<bool>(OPT_print_dot) && DEBUG_LEVEL_PEDANTIC <= debug_level &&
      (!parameters->IsParameter("print-dot-FF") || parameters->GetParameter<unsigned int>("print-dot-FF")))
   {
      AppM->CGetCallGraphManager()->CGetCallGraph()->WriteDot("builtin-graph-post" + STR(function_id) + ".dot");
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--BuiltinWaitCall Analysis completed");
   modified ? function_behavior->UpdateBBVersion() : 0;
   return modified ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}

static tree_nodeRef getFunctionPointerType(tree_nodeRef fptr)
{
   auto* sa = GetPointer<ssa_name>(fptr);
   THROW_ASSERT(sa, "Function pointer not in SSA-form");
   pointer_type* pt;
   if(sa->var)
   {
      auto* var = GetPointer<decl_node>(sa->var);
      THROW_ASSERT(var, "Call expression does not point to a declaration node");
      pt = GetPointer<pointer_type>(var->type);
   }
   else
   {
      pt = GetPointer<pointer_type>(sa->type);
   }

   THROW_ASSERT(pt, "Declaration node has not information about pointer_type");
   THROW_ASSERT(GetPointer<function_type>(pt->ptd), "Pointer type has not information about pointed function_type");

   return pt->ptd;
}
