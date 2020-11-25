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
 *              Copyright (C) 2004-2020 Politecnico di Milano
 *
 *   This file is part of the PandA framework.
 *
 *   The PandA framework is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
/**
 * @file call_graph_computation.cpp
 * @brief Build call_graph data structure starting from the tree_manager.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Pietro Fezzardi <pietro.fezzardi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

/// Header include
#include "call_graph_computation.hpp"

/// Behavior include
#include "application_manager.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"

/// Parameter include
#include "Parameter.hpp"

/// Tree include
#include "behavioral_helper.hpp"
#include "ext_tree_node.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

/// Utility include
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

call_graph_computation::call_graph_computation(const ParameterConstRef _parameters, const application_managerRef _AppM, const DesignFlowManagerConstRef _design_flow_manager)
    : ApplicationFrontendFlowStep(_AppM, FUNCTION_ANALYSIS, _design_flow_manager, _parameters), CGM(_AppM->GetCallGraphManager()), current(0)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

call_graph_computation::~call_graph_computation() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> call_graph_computation::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(CREATE_TREE_MANAGER, WHOLE_APPLICATION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
#if HAVE_FROM_PRAGMA_BUILT
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(PRAGMA_ANALYSIS, WHOLE_APPLICATION));
#endif
#if HAVE_BAMBU_BUILT
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(HDL_FUNCTION_DECL_FIX, WHOLE_APPLICATION));
#endif
#if HAVE_TASTE
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(CREATE_ADDRESS_TRANSLATION, WHOLE_APPLICATION));
#endif
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}

DesignFlowStep_Status call_graph_computation::Exec()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Creating call graph data structure");
   const tree_managerRef TM = AppM->get_tree_manager();
   const auto functions = TM->GetAllFunctions();

   // iterate on functions and add them to the call graph
   for(const auto f_id : functions)
   {
      const std::string fu_name = tree_helper::name_function(TM, f_id);
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Adding function " + STR(f_id) + " " + fu_name + " to call graph");
      if(fu_name == "__start_pragma__" or fu_name == "__close_pragma__" or boost::algorithm::starts_with(fu_name, "__pragma__"))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Skipped...");
         continue;
      }
      // avoid nested functions
      const tree_nodeRef fun = TM->get_tree_node_const(f_id);
      const auto* fd = GetPointer<const function_decl>(fun);
      if(fd->scpe and GET_NODE(fd->scpe)->get_kind() == function_decl_K)
      {
         THROW_ERROR_CODE(NESTED_FUNCTIONS_EC, "Nested functions not yet supported " + STR(f_id));
      }

      // add the function to the call graph if necessary
      if(not CGM->IsVertex(f_id))
      {
         bool has_body = TM->get_implementation_node(f_id) != 0;
         BehavioralHelperRef helper = BehavioralHelperRef(new BehavioralHelper(AppM, f_id, has_body, parameters));
         FunctionBehaviorRef FB = FunctionBehaviorRef(new FunctionBehavior(AppM, helper, parameters));
         CGM->AddFunction(f_id, FB);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Added function " + STR(f_id) + " " + fu_name + " to call graph");
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Function " + STR(f_id) + " " + fu_name + " was already in call graph");
      }
   }

   // iterate on the bodies of the functions to find calls
   for(const auto f_id : functions)
   {
      current = f_id;
      const std::string fu_name = tree_helper::name_function(TM, f_id);
      bool has_body = TM->get_implementation_node(f_id) != 0;
      if(has_body)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Analyze body of " + fu_name);
         const tree_nodeRef fun = TM->get_tree_node_const(f_id);
         const auto* fd = GetPointer<const function_decl>(fun);
         const auto* sl = GetPointer<const statement_list>(GET_NODE(fd->body));
         if(sl->list_of_bloc.empty())
         {
            THROW_ERROR("We can only work on CFG provided by GCC/CLANG");
         }
         else
         {
            for(const auto& b : sl->list_of_bloc)
            {
               for(const auto& stmt : b.second->CGetStmtList())
               {
                  call_graph_computation_recursive(TM, stmt, stmt->index, FunctionEdgeInfo::CallType::function_address);
               }
            }
         }
      }
   }

   if(debug_level >= DEBUG_LEVEL_PEDANTIC or parameters->getOption<bool>(OPT_print_dot))
      CGM->CGetCallGraph()->WriteDot("call_graph.dot");
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Created call graph");
   return DesignFlowStep_Status::SUCCESS;
}

void call_graph_computation::call_graph_computation_recursive(const tree_managerRef& TM, const tree_nodeRef& tn, unsigned int node_stmt, enum FunctionEdgeInfo::CallType call_type)
{
   THROW_ASSERT(tn->get_kind() == tree_reindex_K, "Node is not a tree reindex");
   const tree_nodeRef& curr_tn = GET_NODE(tn);
   unsigned int ind = GET_INDEX_NODE(tn);
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Recursive analysis of " + STR(ind) + " of type " + curr_tn->get_kind_text() + "(statement is " + tn->ToString() + ")");

   switch(curr_tn->get_kind())
   {
      case function_decl_K:
      {
         unsigned int impl = TM->get_implementation_node(ind);
         if(impl)
            ind = impl;
         /// check for nested function
         auto* fd = GetPointer<function_decl>(curr_tn);
         if(fd->scpe && GET_NODE(fd->scpe)->get_kind() == function_decl_K)
         {
            THROW_ERROR_CODE(NESTED_FUNCTIONS_EC, "Nested functions not yet supported " + STR(ind));
            THROW_ERROR("Nested functions not yet supported " + STR(ind));
         }
         CGM->AddCallPoint(current, ind, node_stmt, call_type);
         break;
      }
      case gimple_return_K:
      {
         auto* re = GetPointer<gimple_return>(curr_tn);
         if(re->op)
         {
            call_graph_computation_recursive(TM, re->op, node_stmt, call_type);
         }
         break;
      }
      case gimple_assign_K:
      {
         auto* me = GetPointer<gimple_assign>(curr_tn);

         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Analyzing left part");
         call_graph_computation_recursive(TM, me->op0, node_stmt, call_type);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Analyzed left part - Analyzing right part");
         call_graph_computation_recursive(TM, me->op1, node_stmt, call_type);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Analyzed right part");
         if(me->predicate)
         {
            call_graph_computation_recursive(TM, me->predicate, node_stmt, call_type);
         }
         break;
      }
      case gimple_nop_K:
      {
         break;
      }
      case aggr_init_expr_K:
      case call_expr_K:
      {
         auto* ce = GetPointer<call_expr>(curr_tn);
         tree_nodeRef fun_node = GET_NODE(ce->fn);
         if(fun_node->get_kind() == addr_expr_K)
         {
            auto* ue = GetPointer<unary_expr>(fun_node);
            fun_node = ue->op;
         }
         else if(fun_node->get_kind() == obj_type_ref_K)
         {
            fun_node = tree_helper::find_obj_type_ref_function(ce->fn);
         }
         else
         {
            fun_node = ce->fn;
         }

         call_graph_computation_recursive(TM, fun_node, node_stmt, FunctionEdgeInfo::CallType::direct_call);
         for(auto& arg : ce->args)
         {
            call_graph_computation_recursive(TM, arg, node_stmt, call_type);
         }
         break;
      }
      case gimple_call_K:
      {
         auto* ce = GetPointer<gimple_call>(curr_tn);
         tree_nodeRef fun_node = GET_NODE(ce->fn);
         if(fun_node->get_kind() == addr_expr_K)
         {
            auto* ue = GetPointer<unary_expr>(fun_node);
            fun_node = ue->op;
         }
         else if(fun_node->get_kind() == obj_type_ref_K)
         {
            fun_node = tree_helper::find_obj_type_ref_function(ce->fn);
         }
         else
         {
            fun_node = ce->fn;
         }
         call_graph_computation_recursive(TM, fun_node, node_stmt, FunctionEdgeInfo::CallType::direct_call);
         for(auto& arg : ce->args)
         {
            call_graph_computation_recursive(TM, arg, node_stmt, call_type);
         }
         break;
      }
      case cond_expr_K:
      {
         auto* ce = GetPointer<cond_expr>(curr_tn);
         call_graph_computation_recursive(TM, ce->op0, node_stmt, call_type);
         call_graph_computation_recursive(TM, ce->op1, node_stmt, call_type);
         call_graph_computation_recursive(TM, ce->op2, node_stmt, call_type);
         break;
      }
      case gimple_cond_K:
      {
         auto* gc = GetPointer<gimple_cond>(curr_tn);
         call_graph_computation_recursive(TM, gc->op0, node_stmt, call_type);
         break;
      }
      /* Unary expressions.  */
      case CASE_UNARY_EXPRESSION:
      {
         auto* ue = GetPointer<unary_expr>(curr_tn);
         call_graph_computation_recursive(TM, ue->op, node_stmt, call_type);
         break;
      }
      case CASE_BINARY_EXPRESSION:
      {
         auto* be = GetPointer<binary_expr>(curr_tn);
         call_graph_computation_recursive(TM, be->op0, node_stmt, call_type);
         call_graph_computation_recursive(TM, be->op1, node_stmt, call_type);
         break;
      }
      /*ternary expressions*/
      case gimple_switch_K:
      {
         auto* se = GetPointer<gimple_switch>(curr_tn);
         call_graph_computation_recursive(TM, se->op0, node_stmt, call_type);
         break;
      }
      case gimple_multi_way_if_K:
      {
         auto* gmwi = GetPointer<gimple_multi_way_if>(curr_tn);
         for(const auto& cond : gmwi->list_of_cond)
            if(cond.first)
               call_graph_computation_recursive(TM, cond.first, node_stmt, call_type);
         break;
      }
      case obj_type_ref_K:
      {
         tree_nodeRef fun = tree_helper::find_obj_type_ref_function(tn);
         call_graph_computation_recursive(TM, fun, node_stmt, call_type);
         break;
      }
      case save_expr_K:
      case component_ref_K:
      case bit_field_ref_K:
      case vtable_ref_K:
      case with_cleanup_expr_K:
      case vec_cond_expr_K:
      case vec_perm_expr_K:
      case dot_prod_expr_K:
      case ternary_plus_expr_K:
      case ternary_pm_expr_K:
      case ternary_mp_expr_K:
      case ternary_mm_expr_K:
      case bit_ior_concat_expr_K:
      {
         auto* te = GetPointer<ternary_expr>(curr_tn);
         call_graph_computation_recursive(TM, te->op0, node_stmt, call_type);
         call_graph_computation_recursive(TM, te->op1, node_stmt, call_type);
         if(te->op2)
            call_graph_computation_recursive(TM, te->op2, node_stmt, call_type);
         break;
      }
      case CASE_QUATERNARY_EXPRESSION:
      {
         auto* qe = GetPointer<quaternary_expr>(curr_tn);
         call_graph_computation_recursive(TM, qe->op0, node_stmt, call_type);
         call_graph_computation_recursive(TM, qe->op1, node_stmt, call_type);
         if(qe->op2)
            call_graph_computation_recursive(TM, qe->op2, node_stmt, call_type);
         if(qe->op3)
            call_graph_computation_recursive(TM, qe->op3, node_stmt, call_type);
         break;
      }
      case lut_expr_K:
      {
         auto* le = GetPointer<lut_expr>(curr_tn);
         call_graph_computation_recursive(TM, le->op0, node_stmt, call_type);
         call_graph_computation_recursive(TM, le->op1, node_stmt, call_type);
         if(le->op2)
            call_graph_computation_recursive(TM, le->op2, node_stmt, call_type);
         if(le->op3)
            call_graph_computation_recursive(TM, le->op3, node_stmt, call_type);
         if(le->op4)
            call_graph_computation_recursive(TM, le->op4, node_stmt, call_type);
         if(le->op5)
            call_graph_computation_recursive(TM, le->op5, node_stmt, call_type);
         if(le->op6)
            call_graph_computation_recursive(TM, le->op6, node_stmt, call_type);
         if(le->op7)
            call_graph_computation_recursive(TM, le->op7, node_stmt, call_type);
         if(le->op8)
            call_graph_computation_recursive(TM, le->op8, node_stmt, call_type);
         break;
      }
      case constructor_K:
      {
         auto* c = GetPointer<constructor>(curr_tn);
         for(const auto& i : c->list_of_idx_valu)
         {
            call_graph_computation_recursive(TM, i.second, node_stmt, call_type);
         }
         break;
      }
      case var_decl_K:
      {
         /// var decl performs an assignment when init is not null
         auto* vd = GetPointer<var_decl>(curr_tn);
         if(vd->init)
            call_graph_computation_recursive(TM, vd->init, node_stmt, call_type);
      }
      case result_decl_K:
      case parm_decl_K:
      case ssa_name_K:
      case integer_cst_K:
      case real_cst_K:
      case string_cst_K:
      case vector_cst_K:
      case void_cst_K:
      case complex_cst_K:
      case field_decl_K:
      case label_decl_K:
      case template_decl_K:
      case gimple_label_K:
      case gimple_goto_K:
      case gimple_asm_K:
      case gimple_phi_K:
      case target_mem_ref_K:
      case target_mem_ref461_K:
      case CASE_PRAGMA_NODES:
      case gimple_pragma_K:
      {
         break;
      }
      case binfo_K:
      case block_K:
      case CASE_CPP_NODES:
      case case_label_expr_K:
      case CASE_FAKE_NODES:
      case const_decl_K:
      case gimple_bind_K:
      case gimple_for_K:
      case gimple_predict_K:
      case gimple_resx_K:
      case gimple_while_K:
      case identifier_node_K:
      case namespace_decl_K:
      case statement_list_K:
      case translation_unit_decl_K:
      case error_mark_K:
      case using_decl_K:
      case tree_list_K:
      case tree_vec_K:
      case type_decl_K:
      case target_expr_K:
      case CASE_TYPE_NODES:
      {
         THROW_ERROR(std::string("Node not supported (") + STR(ind) + std::string("): ") + curr_tn->get_kind_text());
         break;
      }
      default:
         THROW_UNREACHABLE("");
   };
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Completed the recursive analysis of node " + STR(ind));
}
