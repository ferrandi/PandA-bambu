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
#include "tree_reindex.hpp"

static tree_nodeRef getFunctionPointerType(tree_nodeRef fptr);
void CallGraphBuiltinCall::lookForBuiltinCall(const tree_nodeRef TN)
{
   THROW_ASSERT(TN->get_kind() == tree_reindex_K, "Node is not a tree reindex");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Update recursively node: " + STR(GET_NODE(TN)) + " id: " + STR(GET_INDEX_NODE(TN)));

   const tree_managerRef TM = AppM->get_tree_manager();
   const tree_nodeRef currentTreeNode = GET_NODE(TN);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + currentTreeNode->get_kind_text());
   switch(currentTreeNode->get_kind())
   {
      case call_expr_K:
      case aggr_init_expr_K:
      {
         const call_expr* CE = GetPointer<call_expr>(currentTreeNode);
         const std::vector<tree_nodeRef>& args = CE->args;

         tree_nodeRef FN = GetPointer<addr_expr>(GET_NODE(CE->fn)) ? GetPointer<addr_expr>(GET_NODE(CE->fn))->op : CE->fn;
         THROW_ASSERT(FN, "Address expression with null op");
         if(GET_NODE(FN)->get_kind() == function_decl_K)
         {
            unsigned int functionDeclIdx = FN->index;
            std::string funName = tree_helper::name_function(TM, functionDeclIdx);
            if(funName == BUILTIN_WAIT_CALL)
            {
               tree_nodeRef builtinArgZero = GetPointer<addr_expr>(GET_NODE(args[0])) ? GetPointer<addr_expr>(GET_NODE(args[0]))->op : args[0];
               if(GET_NODE(builtinArgZero)->get_kind() == function_decl_K)
               {
                  unsigned int calledFunctionId = builtinArgZero->index;
                  AppM->GetCallGraphManager()->AddCallPoint(function_id, calledFunctionId, GET_INDEX_NODE(TN), FunctionEdgeInfo::CallType::indirect_call);
                  modified = true;
               }
               else if(GET_NODE(builtinArgZero)->get_kind() == ssa_name_K)
               {
                  tree_nodeRef funPtrType = getFunctionPointerType(GET_NODE(builtinArgZero));
                  ExtendCallGraph(function_id, funPtrType, CE->index);
               }
            }
         }
         break;
      }
      case gimple_call_K:
      {
         const gimple_call* CE = GetPointer<gimple_call>(currentTreeNode);
         const std::vector<tree_nodeRef>& args = CE->args;

         tree_nodeRef FN = GetPointer<addr_expr>(GET_NODE(CE->fn)) ? GetPointer<addr_expr>(GET_NODE(CE->fn))->op : CE->fn;
         THROW_ASSERT(FN, "Address expression with null op");
         if(GET_NODE(FN)->get_kind() == function_decl_K)
         {
            unsigned int functionDeclIdx = GET_NODE(FN)->index;
            std::string funName = tree_helper::name_function(TM, functionDeclIdx);
            if(funName == BUILTIN_WAIT_CALL)
            {
               tree_nodeRef builtinArgZero = GetPointer<addr_expr>(GET_NODE(args[0])) ? GetPointer<addr_expr>(GET_NODE(args[0]))->op : args[0];
               if(GET_NODE(builtinArgZero)->get_kind() == function_decl_K)
               {
                  unsigned int calledFunctionId = builtinArgZero->index;
                  AppM->GetCallGraphManager()->AddCallPoint(function_id, calledFunctionId, GET_INDEX_NODE(TN), FunctionEdgeInfo::CallType::indirect_call);
                  modified = true;
               }
               else if(GET_NODE(builtinArgZero)->get_kind() == ssa_name_K)
               {
                  // Function pointers case.
                  tree_nodeRef funPtrType = getFunctionPointerType(GET_NODE(builtinArgZero));
                  ExtendCallGraph(function_id, funPtrType, CE->index);
               }
            }
         }
         break;
      }
      case gimple_assign_K:
      {
         auto* gm = GetPointer<gimple_assign>(currentTreeNode);
         if(GetPointer<call_expr>(GET_NODE(gm->op1)))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---call_expr " + STR(GET_NODE(gm->op1)));
            lookForBuiltinCall(gm->op1);
         }
         else if(GetPointer<addr_expr>(GET_NODE(gm->op1)))
         {
            auto* ae = GetPointer<addr_expr>(GET_NODE(gm->op1));
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---is addr_expr " + GET_NODE(ae->op)->get_kind_text());
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---takes address " + STR(GET_NODE(gm->op1)));
            if(GetPointer<function_decl>(GET_NODE(ae->op)))
            {
               AppM->GetCallGraphManager()->AddCallPoint(function_id, GET_INDEX_NODE(ae->op), currentTreeNode->index, FunctionEdgeInfo::CallType::function_address);
            }
         }
         else if(GetPointer<nop_expr>(GET_NODE(gm->op1)))
         {
            auto* nop = GetPointer<nop_expr>(GET_NODE(gm->op1));
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---nop_expr(" + STR(GET_NODE(nop->op)) +
                               ")"
                               " op is " +
                               GET_NODE(nop->op)->get_kind_text());
            if(GetPointer<addr_expr>(GET_NODE(nop->op)))
            {
               auto* ae = GetPointer<addr_expr>(GET_NODE(nop->op));
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---is addr_expr of " + GET_NODE(ae->op)->get_kind_text());
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---takes address " + STR(GET_NODE(nop->op)));
               if(GetPointer<function_decl>(GET_NODE(ae->op)))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---takes address of function_decl " + STR(GET_INDEX_NODE(ae->op)));
                  AppM->GetCallGraphManager()->AddCallPoint(function_id, GET_INDEX_NODE(ae->op), currentTreeNode->index, FunctionEdgeInfo::CallType::function_address);
               }
            }
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "--- not addr_expr " + GET_NODE(gm->op1)->get_kind_text());
         }
         break;
      }
      case gimple_nop_K:
      case var_decl_K:
      case parm_decl_K:
      case ssa_name_K:
      case tree_list_K:
      case CASE_UNARY_EXPRESSION:
      case CASE_BINARY_EXPRESSION:
      case CASE_TERNARY_EXPRESSION:
      case CASE_QUATERNARY_EXPRESSION:
      case lut_expr_K:
      case constructor_K:
      case gimple_cond_K:
      case gimple_switch_K:
      case gimple_multi_way_if_K:
      case gimple_return_K:
      case gimple_for_K:
      case gimple_while_K:
      case CASE_TYPE_NODES:
      case type_decl_K:
      case target_expr_K:
      case target_mem_ref_K:
      case target_mem_ref461_K:
      case real_cst_K:
      case complex_cst_K:
      case string_cst_K:
      case integer_cst_K:
      case field_decl_K:
      case function_decl_K:
      case label_decl_K:
      case result_decl_K:
      case vector_cst_K:
      case void_cst_K:
      case tree_vec_K:
      case case_label_expr_K:
      case gimple_label_K:
      case gimple_asm_K:
      case gimple_goto_K:
      case CASE_PRAGMA_NODES:
      case binfo_K:
      case block_K:
      case const_decl_K:
      case CASE_CPP_NODES:
      case gimple_bind_K:
      case gimple_phi_K:
      case gimple_pragma_K:
      case gimple_predict_K:
      case gimple_resx_K:
      case identifier_node_K:
      case last_tree_K:
      case namespace_decl_K:
      case none_K:
      case placeholder_expr_K:
      case statement_list_K:
      case translation_unit_decl_K:
      case error_mark_K:
      case using_decl_K:
      case template_decl_K:
      case tree_reindex_K:
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Updated recursively " + STR(GET_INDEX_NODE(TN)));
}

void CallGraphBuiltinCall::ExtendCallGraph(unsigned int callerIdx, tree_nodeRef funType, unsigned int stmtIdx)
{
   std::string type = tree_helper::print_type(AppM->get_tree_manager(), funType->index);
   for(unsigned int Itr : typeToDeclaration[type])
   {
      AppM->GetCallGraphManager()->AddCallPoint(callerIdx, Itr, stmtIdx, FunctionEdgeInfo::CallType::indirect_call);
      modified = true;
   }
}

const CustomUnorderedSet<std::pair<FrontendFlowStepType, CallGraphBuiltinCall::FunctionRelationship>> CallGraphBuiltinCall::ComputeFrontendRelationships(DesignFlowStep::RelationshipType RT) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(RT)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>(HWCALL_INJECTION, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>(FUNCTION_ANALYSIS, WHOLE_APPLICATION));
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
         THROW_UNREACHABLE("HWCallInjection::ComputeFrontendRelationships");
   }
   return relationships;
}

CallGraphBuiltinCall::CallGraphBuiltinCall(const application_managerRef AM, unsigned int functionId, const DesignFlowManagerConstRef DFM, const ParameterConstRef P)
    : FunctionFrontendFlowStep(AM, functionId, CALL_GRAPH_BUILTIN_CALL, DFM, P), modified(false)
{
   debug_level = P->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CallGraphBuiltinCall::~CallGraphBuiltinCall() = default;

void CallGraphBuiltinCall::Initialize()
{
   modified = false;
   typeToDeclaration.clear();
}

DesignFlowStep_Status CallGraphBuiltinCall::InternalExec()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->BuiltinWaitCall Analysis");
   tree_managerRef TM = AppM->get_tree_manager();
   const tree_nodeRef currentTreeNode = TM->GetTreeNode(function_id);
   auto* functionDecl = GetPointer<function_decl>(currentTreeNode);
   auto* stmtList = GetPointer<statement_list>(GET_NODE(functionDecl->body));

   if(parameters->getOption<bool>(OPT_print_dot) && DEBUG_LEVEL_PEDANTIC <= debug_level)
   {
      AppM->CGetCallGraphManager()->CGetCallGraph()->WriteDot("builtin-graph-pre" + STR(function_id) + ".dot");
   }

   // Build the typeToDeclarationMap
   const CallGraphManagerRef CGM = AppM->GetCallGraphManager();
   const auto root_functions = CGM->GetRootFunctions();
   CustomUnorderedSet<unsigned int> allFunctions;
   function_decl_refs fdr_visitor(allFunctions);
   for(const auto root_function : root_functions)
   {
      tree_nodeRef rf = TM->get_tree_node_const(root_function);
      rf->visit(&fdr_visitor);
   }
   for(CustomUnorderedSet<unsigned int>::const_iterator Itr = allFunctions.begin(), End = allFunctions.end(); Itr != End; ++Itr)
   {
      std::string functionName = tree_helper::name_function(TM, *Itr);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Analyzing function " + STR(*Itr) + " " + functionName);
      tree_nodeRef function = TM->get_tree_node_const(*Itr);
      auto* funDecl = GetPointer<function_decl>(function);
      std::string type = tree_helper::print_type(TM, GET_INDEX_NODE(funDecl->type));
      if(funDecl->body && functionName != "__start_pragma__" && functionName != "__close_pragma__" && !boost::algorithm::starts_with(functionName, "__pragma__"))
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---FunctionTypeString " + type);
      typeToDeclaration[type].insert(*Itr);
   }
   for(const auto& block : stmtList->list_of_bloc)
   {
      for(const auto& stmt : block.second->CGetStmtList())
      {
         if(GET_NODE(stmt)->get_kind() == gimple_call_K or GET_NODE(stmt)->get_kind() == gimple_assign_K)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Analyzing stmt " + stmt->ToString());
            lookForBuiltinCall(stmt);
         }
      }
   }

   if(parameters->getOption<bool>(OPT_print_dot) && DEBUG_LEVEL_PEDANTIC <= debug_level)
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
      auto* var = GetPointer<decl_node>(GET_NODE(sa->var));
      THROW_ASSERT(var, "Call expression does not point to a declaration node");
      pt = GetPointer<pointer_type>(GET_NODE(var->type));
   }
   else
      pt = GetPointer<pointer_type>(GET_NODE(sa->type));

   THROW_ASSERT(pt, "Declaration node has not information about pointer_type");
   THROW_ASSERT(GetPointer<function_type>(GET_NODE(pt->ptd)), "Pointer type has not information about pointed function_type");

   return GET_NODE(pt->ptd);
}
