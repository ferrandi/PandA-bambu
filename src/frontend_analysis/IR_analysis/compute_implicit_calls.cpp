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
 *              Copyright (C) 2004-2024 Politecnico di Milano
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
 * @file compute_implicit_calls.cpp
 * @brief Class to determine the variable to be stored in memory
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
/**
  This step do the lowering of two types of implicit memory operations: memset and memcpy implicit patterns.
  Currently, only the memset implicit pattern is managed.

  For the memset we look for this pattern:
    BB1:
      (xxxxa)*
      var = {};
      (xxxxb)*

  and we translate such code into this one:
    BB1:
      (xxxxa)*
      upperlimit=varsize+var;
    BBN1:
      i0=phi(var,i1)
      i1=i0+sizeof(item of var);
      i0* = 0;
      if(i1!= upperlimit)
        goto BBN1;
    BBN2:
      (xxxxb)*
  Note that implicit memory virtual definition has to be translated from "var = {};" into "var[i0] = 0;".
  */

/// Header include
#include "compute_implicit_calls.hpp"
#include "config_HAVE_PRAGMA_BUILT.hpp"

/// Behavior include
#include "application_manager.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"

/// Parameter include
#include "Parameter.hpp"
#include "module_interface.hpp"

/// Tree include
#include "behavioral_helper.hpp"
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "ext_tree_node.hpp"
#include "op_graph.hpp"
#include "string_manipulation.hpp" // for GET_CLASS
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "tree_node.hpp"

compute_implicit_calls::compute_implicit_calls(const ParameterConstRef _parameters, const application_managerRef _AppM,
                                               unsigned int _function_id,
                                               const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, COMPUTE_IMPLICIT_CALLS, _design_flow_manager, _parameters),
      TM(_AppM->get_tree_manager()),
      update_bb_ver(false)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

compute_implicit_calls::~compute_implicit_calls() = default;

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
compute_implicit_calls::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(CHECK_SYSTEM_TYPE, SAME_FUNCTION));
         relationships.insert(std::make_pair(IR_LOWERING, SAME_FUNCTION));
         relationships.insert(std::make_pair(FUNCTION_ANALYSIS, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(FIX_STRUCTS_PASSED_BY_VALUE, SAME_FUNCTION));
         relationships.insert(std::make_pair(USE_COUNTING, SAME_FUNCTION));
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

DesignFlowStep_Status compute_implicit_calls::InternalExec()
{
   tree_nodeRef node = TM->GetTreeNode(function_id);
   const auto fd = GetPointer<function_decl>(node);
   if(!fd || !fd->body)
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Node is not a function or it hasn't a body");
      return DesignFlowStep_Status::UNCHANGED;
   }
   bool changed = false;
   std::list<std::pair<tree_nodeRef, unsigned int>> to_be_lowered_memset;

   /// The tree manipulation
   const auto tree_man = tree_manipulationRef(new tree_manipulation(TM, parameters, AppM));

   unsigned int max_loop_id = 0;

   const auto sl = GetPointer<statement_list>(fd->body);
   THROW_ASSERT(sl, "Body is not a statement_list");
   for(const auto& bb : sl->list_of_bloc)
   {
      if(bb.second->number == BB_ENTRY || bb.second->number == BB_EXIT)
      {
         continue;
      }
      max_loop_id = std::max(max_loop_id, bb.second->loop_id);
      // Statement list may be modified during the scan, thus it is necessary to iterate over a constant copy of it
      const std::list<tree_nodeRef> const_sl = bb.second->CGetStmtList();
      for(const auto& stmt : const_sl)
      {
         if(stmt->get_kind() == gimple_assign_K)
         {
            const auto gm = GetPointer<gimple_assign>(stmt);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing node " + stmt->ToString());

            /// check for implicit memset/memcpy calls
            const auto op0 = gm->op0;
            const auto op1 = gm->op1;
            const auto op0_type = tree_helper::CGetType(gm->op0);
            const auto op1_type = tree_helper::CGetType(gm->op1);

            bool is_a_vector_bitfield = false;
            if(op1->get_kind() == bit_field_ref_K)
            {
               const auto bfr = GetPointer<bit_field_ref>(op1);
               if(tree_helper::IsVectorType(bfr->op0))
               {
                  is_a_vector_bitfield = true;
               }
            }

            bool load_candidate = (op1->get_kind() == bit_field_ref_K && !is_a_vector_bitfield) ||
                                  op1->get_kind() == component_ref_K || op1->get_kind() == indirect_ref_K ||
                                  op1->get_kind() == misaligned_indirect_ref_K || op1->get_kind() == mem_ref_K ||
                                  op1->get_kind() == array_ref_K || op1->get_kind() == target_mem_ref_K ||
                                  op1->get_kind() == target_mem_ref461_K;
            if(op1->get_kind() == realpart_expr_K || op1->get_kind() == imagpart_expr_K)
            {
               const auto code1 = GetPointer<unary_expr>(op1)->op->get_kind();
               if((code1 == bit_field_ref_K && !is_a_vector_bitfield) || code1 == component_ref_K ||
                  code1 == indirect_ref_K || code1 == bit_field_ref_K || code1 == misaligned_indirect_ref_K ||
                  code1 == mem_ref_K || code1 == array_ref_K || code1 == target_mem_ref_K ||
                  code1 == target_mem_ref461_K)
               {
                  load_candidate = true;
               }
               if(code1 == var_decl_K)
               {
                  load_candidate = true;
               }
            }
            bool store_candidate = op0->get_kind() == bit_field_ref_K || op0->get_kind() == component_ref_K ||
                                   op0->get_kind() == indirect_ref_K || op0->get_kind() == misaligned_indirect_ref_K ||
                                   op0->get_kind() == mem_ref_K || op0->get_kind() == array_ref_K ||
                                   op0->get_kind() == target_mem_ref_K || op0->get_kind() == target_mem_ref461_K;
            if(op0->get_kind() == realpart_expr_K || op0->get_kind() == imagpart_expr_K)
            {
               const auto code0 = GetPointer<unary_expr>(op0)->op->get_kind();
               if(code0 == component_ref_K || code0 == indirect_ref_K || code0 == bit_field_ref_K ||
                  code0 == misaligned_indirect_ref_K || code0 == mem_ref_K || code0 == array_ref_K ||
                  code0 == target_mem_ref_K || code0 == target_mem_ref461_K)
               {
                  store_candidate = true;
               }
               if(code0 == var_decl_K)
               {
                  store_candidate = true;
               }
            }
            if(!gm->clobber && !gm->init_assignment && op0_type && op1_type && op1->get_kind() != insertvalue_expr_K &&
               op1->get_kind() != extractvalue_expr_K &&
               ((op0_type->get_kind() == record_type_K && op1_type->get_kind() == record_type_K &&
                 op1->get_kind() != view_convert_expr_K) ||
                (op0_type->get_kind() == union_type_K && op1_type->get_kind() == union_type_K &&
                 op1->get_kind() != view_convert_expr_K) ||
                (op0_type->get_kind() == array_type_K) || (store_candidate && load_candidate)))
            {
               changed = true;
               const auto mr = GetPointer<mem_ref>(op0);
               THROW_ASSERT(mr, "unexpected condition " + gm->ToString());
               /// compute the size of memory to be set with memset
               const auto dst_type = tree_helper::CGetType(mr->op0);
               const auto dst_ptr_t = GetPointer<const pointer_type>(dst_type);
               THROW_ASSERT(dst_ptr_t, "unexpected condition");
               const auto dst_size = tree_helper::SizeAlloc(dst_ptr_t->ptd);
               if(dst_size)
               {
                  if(op1->get_kind() == constructor_K && GetPointer<constructor>(op1) &&
                     GetPointer<constructor>(op1)->list_of_idx_valu.size() == 0)
                  {
                     const auto var = tree_helper::GetBaseVariable(mr->op0);
                     bool do_lowering = var != nullptr;
                     if(do_lowering)
                     {
                        const auto type_node = tree_helper::CGetType(var);
                        do_lowering = type_node->get_kind() == array_type_K;
                        if(do_lowering)
                        {
                           const auto element_type = tree_helper::CGetElements(type_node);
                           const auto element_type_kind = element_type->get_kind();
                           if(!(element_type_kind == boolean_type_K || element_type_kind == CharType_K ||
                                element_type_kind == enumeral_type_K || element_type_kind == integer_type_K ||
                                element_type_kind == pointer_type_K or element_type_kind == record_type_K))
                           {
                              do_lowering = false;
                              THROW_ASSERT(
                                  element_type_kind == array_type_K || element_type_kind == nullptr_type_K ||
                                      element_type_kind == type_pack_expansion_K || element_type_kind == real_type_K ||
                                      element_type_kind == complex_type_K or element_type_kind == function_type_K ||
                                      element_type_kind == lang_type_K || element_type_kind == method_type_K ||
                                      element_type_kind == offset_type_K || element_type_kind == qual_union_type_K or
                                      element_type_kind == record_type_K || element_type_kind == reference_type_K ||
                                      element_type_kind == set_type_K || element_type_kind == template_type_parm_K ||
                                      element_type_kind == typename_type_K or element_type_kind == union_type_K ||
                                      element_type_kind == vector_type_K || element_type_kind == void_type_K,
                                  tree_node::GetString(element_type_kind));
                           }
                        }
                     }
                     if(do_lowering)
                     {
                        to_be_lowered_memset.push_front(std::make_pair(stmt, bb.second->number));
                     }
                     else
                     {
                        THROW_ASSERT(GetPointerS<const function_decl>(TM->GetFunction(MEMSET))->body,
                                     "inconsistent behavioral helper");
                        replace_with_memset(stmt, sl, tree_man);
                        update_bb_ver = true;
                     }
                  }
                  else
                  {
                     THROW_ASSERT(GetPointerS<const function_decl>(TM->GetFunction(MEMCPY))->body,
                                  "inconsistent behavioral helper");
                     replace_with_memcpy(stmt, sl, tree_man);
                     update_bb_ver = true;
                  }
               }
               else
               {
                  bb.second->RemoveStmt(stmt, AppM);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Empty struct assignement removed");
               }
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed node " + stmt->ToString());
         }
      }
   }

   /// do the memset transformations
   for(const auto& stmt_bb_pair : to_be_lowered_memset)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "-->Transforming (" + STR(stmt_bb_pair.first->index) + ") " + STR(stmt_bb_pair.first));
      const auto BB1_block = sl->list_of_bloc.at(stmt_bb_pair.second);

      /// create BBN1
      const auto BBN1_block = blocRef(new bloc((sl->list_of_bloc.rbegin())->first + 1));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created BB" + STR(BBN1_block->number));
      sl->add_bloc(BBN1_block);

      ++max_loop_id;
      BBN1_block->loop_id = max_loop_id;
      BBN1_block->schedule = BB1_block->schedule;
      BBN1_block->SetSSAUsesComputed();

      /// Add BBN1 predecessor as BB1
      BBN1_block->add_pred(BB1_block->number);
      BBN1_block->add_pred(BBN1_block->number);
      BBN1_block->add_succ(BBN1_block->number);
      BBN1_block->true_edge = BBN1_block->number;

      /// create BBN2
      const auto BBN2_block = blocRef(new bloc((sl->list_of_bloc.rbegin())->first + 1));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created BB" + STR(BBN2_block->number));
      sl->add_bloc(BBN2_block);

      BBN2_block->loop_id = BB1_block->loop_id;
      BBN2_block->schedule = BB1_block->schedule;
      BBN2_block->SetSSAUsesComputed();

      /// Add BBN2 predecessor as BBN1 basic block
      BBN2_block->add_pred(BBN1_block->number);
      /// Add BBN2 successor as BB1 basic block
      BBN2_block->list_of_succ = BB1_block->list_of_succ;
      /// fix true and false edges
      BBN2_block->true_edge = BB1_block->true_edge;
      BBN2_block->false_edge = BB1_block->false_edge;
      BB1_block->true_edge = 0;
      BB1_block->false_edge = 0;
      BB1_block->list_of_succ.clear();
      BB1_block->add_succ(BBN1_block->number);

      /// Add BBN1 successor as succ basic block
      BBN1_block->add_succ(BBN2_block->number);
      BBN1_block->false_edge = BBN2_block->number;

      /// Fix BBN2 successors
      for(const auto& succ : BBN2_block->list_of_succ)
      {
         const auto succ_block = sl->list_of_bloc.at(succ);
         succ_block->list_of_pred.erase(
             std::find(succ_block->list_of_pred.begin(), succ_block->list_of_pred.end(), BB1_block->number));
         succ_block->add_pred(BBN2_block->number);
         /// Update all the phis
         for(const auto& phi : succ_block->CGetPhiList())
         {
            auto gp = GetPointerS<gimple_phi>(phi);
            for(const auto& def_edge : gp->CGetDefEdgesList())
            {
               if(def_edge.second == BB1_block->number)
               {
                  gp->ReplaceDefEdge(TM, def_edge, gimple_phi::DefEdge(def_edge.first, BBN2_block->number));
               }
            }
         }
      }

      /// retrieve the starting variable
      const auto ga = GetPointerS<gimple_assign>(stmt_bb_pair.first);
      const auto mr = GetPointerS<mem_ref>(ga->op0);
      const auto var = tree_helper::GetBaseVariable(mr->op0);
      auto init_var = mr->op0;
      const auto srcp_default = ga->include_name + ":" + STR(ga->line_number) + ":" + STR(ga->column_number);
      auto type_node1 = tree_helper::CGetType(var);
      const auto algn = GetPointerS<const type_node>(type_node1)->algn;
      THROW_ASSERT(type_node1->get_kind() == array_type_K, "unexpected condition");
      while(type_node1->get_kind() == array_type_K)
      {
         type_node1 = tree_helper::CGetElements(type_node1);
      }
      const auto offset_type = tree_man->GetSizeType();
      const auto pt = tree_man->GetPointerType(type_node1, algn);

      /// add a cast
      const auto nop_init_var = tree_man->create_unary_operation(pt, init_var, srcp_default, nop_expr_K);
      const auto nop_init_var_ga =
          tree_man->CreateGimpleAssign(pt, tree_nodeRef(), tree_nodeRef(), nop_init_var, function_id, srcp_default);
      init_var = GetPointerS<gimple_assign>(nop_init_var_ga)->op0;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---Created cast statement " + nop_init_var_ga->ToString());

      /// Create phi for the induction variable of BBN1
      tree_nodeRef new_induction_var;
      /// The list of def edge which contains for the moment only the value coming from the forward edge
      std::vector<std::pair<tree_nodeRef, unsigned int>> list_of_def_edge;
      list_of_def_edge.push_back(std::make_pair(init_var, BB1_block->number));
      const auto phi = tree_man->create_phi_node(new_induction_var, list_of_def_edge, function_id);
      const auto gp = GetPointerS<gimple_phi>(phi);
      const auto phi_res_use_set = PointToSolutionRef(new PointToSolution());
      phi_res_use_set->Add(var);
      GetPointerS<ssa_name>(gp->res)->use_set = phi_res_use_set;
      BBN1_block->AddPhi(phi);

      /// compute the size of memory to be set with memset
      const auto dst_type = tree_helper::CGetType(mr->op0);
      const auto dst_ptr_t = GetPointer<const pointer_type>(dst_type);
      THROW_ASSERT(dst_ptr_t, "unexpected condition");
      const auto dst_size = tree_helper::SizeAlloc(dst_ptr_t->ptd);
      THROW_ASSERT(dst_size % 8 == 0, "unexpected condition");
      const auto copy_byte_size = dst_size / 8;
      const auto copy_byte_size_node =
          TM->CreateUniqueIntegerCst(static_cast<long long int>(copy_byte_size), offset_type);
      const auto pp =
          tree_man->create_binary_operation(pt, init_var, copy_byte_size_node, srcp_default, pointer_plus_expr_K);
      const auto pp_ga =
          tree_man->CreateGimpleAssign(pt, tree_nodeRef(), tree_nodeRef(), pp, function_id, srcp_default);
      GetPointerS<gimple_assign>(pp_ga)->temporary_address = true;
      const auto vd_limit = GetPointerS<gimple_assign>(pp_ga)->op0;
      const auto vd_limit_use_set = PointToSolutionRef(new PointToSolution());
      vd_limit_use_set->Add(var);
      GetPointerS<ssa_name>(vd_limit)->use_set = vd_limit_use_set;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Create statement " + pp_ga->ToString());

      const auto size_node =
          TM->CreateUniqueIntegerCst(static_cast<long long int>(tree_helper::SizeAlloc(type_node1) / 8), offset_type);
      const auto pp_ind = tree_man->create_binary_operation(pt, gp->res, size_node, srcp_default, pointer_plus_expr_K);
      const auto pp_ga_ind =
          tree_man->CreateGimpleAssign(pt, tree_nodeRef(), tree_nodeRef(), pp_ind, function_id, srcp_default);
      GetPointerS<gimple_assign>(pp_ga_ind)->temporary_address = true;
      const auto vd_ind = GetPointerS<gimple_assign>(pp_ga_ind)->op0;
      const auto vd_ind_use_set = PointToSolutionRef(new PointToSolution());
      vd_ind_use_set->Add(var);
      GetPointerS<ssa_name>(vd_ind)->use_set = vd_ind_use_set;
      gp->AddDefEdge(TM, gimple_phi::DefEdge(vd_ind, BBN1_block->number));
      BBN1_block->PushBack(pp_ga_ind, AppM);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Create statement " + pp_ga_ind->ToString());

      /// the comparison
      const auto boolean_type = tree_man->GetBooleanType();
      const auto comparison =
          tree_man->create_binary_operation(boolean_type, vd_ind, vd_limit, srcp_default, ne_expr_K);
      const auto comp_ga = tree_man->CreateGimpleAssign(boolean_type, TM->CreateUniqueIntegerCst(0, type_node1),
                                                        TM->CreateUniqueIntegerCst(1, type_node1), comparison,
                                                        function_id, srcp_default);
      BBN1_block->PushBack(comp_ga, AppM);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Create comparison " + STR(comp_ga));

      /// the gimple cond
      const auto comp_res = GetPointerS<gimple_assign>(comp_ga)->op0;
      const auto gc = tree_man->create_gimple_cond(comp_res, function_id, srcp_default);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Create branch condition " + STR(gc));

      /// restructure of the implicit memset statement
      const auto zero_offset = TM->CreateUniqueIntegerCst(0, pt);
      const auto new_mem_ref =
          tree_man->create_binary_operation(type_node1, gp->res, zero_offset, srcp_default, mem_ref_K);
      const auto zero_value = TM->CreateUniqueIntegerCst(0, type_node1);
      TM->ReplaceTreeNode(stmt_bb_pair.first, ga->op0, new_mem_ref);
      TM->ReplaceTreeNode(stmt_bb_pair.first, ga->op1, zero_value);

      const auto list_of_stmt = BB1_block->CGetStmtList();
      bool found_memset_statement = false;
      /// We must use pointer since we are erasing elements in the list
      for(auto statement = list_of_stmt.begin(); statement != list_of_stmt.end(); statement++)
      {
         if((*statement)->index == stmt_bb_pair.first->index)
         {
            /// move var = {}; to BBN1
            found_memset_statement = true;
            const auto temp_statement = *statement;
            /// Going one step step forward to avoid invalidation of the pointer
            auto tmp_it = statement;
            ++tmp_it;
            /// Moving statement
            BB1_block->RemoveStmt(temp_statement, AppM);
            BBN1_block->PushBack(temp_statement, AppM);
            /// Going one step back since pointer is already increment in for loop
            --tmp_it;
            statement = tmp_it;
         }
         else if(found_memset_statement)
         {
            /// move (xxxxb)* to BBN2
            const auto temp_statement = *statement;
            /// Going one step step forward to avoid invalidation of the pointer
            auto tmp_it = statement;
            ++tmp_it;
            /// Moving statement
            BB1_block->RemoveStmt(temp_statement, AppM);
            BBN2_block->PushBack(temp_statement, AppM);
            /// Going one step back since pointer is already increment in for loop
            --tmp_it;
            statement = tmp_it;
         }
      }
      THROW_ASSERT(found_memset_statement, "unexpected condition");
      BB1_block->PushBack(nop_init_var_ga, AppM);
      BB1_block->PushBack(pp_ga, AppM);
      BBN1_block->PushBack(gc, AppM);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Transformed " + STR(stmt_bb_pair.first));
   }
   if(debug_level >= DEBUG_LEVEL_PEDANTIC && parameters->getOption<bool>(OPT_print_dot) &&
      (!parameters->IsParameter("print-dot-FF") || parameters->GetParameter<unsigned int>("print-dot-FF")))
   {
      AppM->CGetCallGraphManager()->CGetCallGraph()->WriteDot("compute_implicit_calls." + GetName() + ".dot");
   }
   if(update_bb_ver)
   {
      AppM->GetFunctionBehavior(function_id)->UpdateBBVersion();
   }
   return (changed || update_bb_ver) ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}

void compute_implicit_calls::replace_with_memcpy(tree_nodeRef stmt, const statement_list* sl,
                                                 tree_manipulationRef tree_man) const
{
   const auto ga = GetPointer<const gimple_assign>(stmt);
   const auto lhs_node = ga->op0;
   THROW_ASSERT(
       lhs_node->get_kind() == mem_ref_K,
       "unexpected condition: " + AppM->CGetFunctionBehavior(function_id)->CGetBehavioralHelper()->get_function_name() +
           " calls function " + MEMCPY + " in operation " + ga->ToString() + " but lhs " + lhs_node->ToString() +
           " is not a mem_ref: it's a " + lhs_node->get_kind_text());
   const auto mr_lhs = GetPointer<const mem_ref>(lhs_node);
   THROW_ASSERT(GetPointer<const ssa_name>(mr_lhs->op0), "");
   THROW_ASSERT(mr_lhs->op1->get_kind() == integer_cst_K && tree_helper::GetConstValue(mr_lhs->op1) == 0, "");
   const auto rhs_node = ga->op1;
   const auto rhs_kind = rhs_node->get_kind();

   const auto s = GetPointer<const srcp>(stmt);
   const std::string current_srcp =
       s ? (s->include_name + ":" + STR(s->line_number) + ":" + STR(s->column_number)) : "";

   unsigned long int copy_byte_size = 0;
   // args to be filled before the creation of the gimple call
   // dst is always the ssa on the rhs
   std::vector<tree_nodeRef> args = {mr_lhs->op0};

   THROW_ASSERT(
       rhs_kind == mem_ref_K || rhs_kind == parm_decl_K || rhs_kind == string_cst_K,
       "unexpected condition: " + AppM->CGetFunctionBehavior(function_id)->CGetBehavioralHelper()->get_function_name() +
           " calls function " + MEMCPY + " in operation " + ga->ToString() + " but rhs " + rhs_node->ToString() +
           " is not a mem_ref: it's a " + rhs_node->get_kind_text());
   if(rhs_kind == mem_ref_K)
   {
      const auto mr_rhs = GetPointer<const mem_ref>(rhs_node);
      THROW_ASSERT(GetPointer<const ssa_name>(mr_rhs->op0), "");
      THROW_ASSERT(mr_rhs->op1->get_kind() == integer_cst_K && tree_helper::GetConstValue(mr_rhs->op1) == 0, "");

      // src
      args.push_back(mr_rhs->op0);

      // compute the size in bytes of the copied memory
      const auto dst_type = tree_helper::CGetType(mr_lhs->op0);
      const auto src_type = tree_helper::CGetType(mr_rhs->op0);
      const auto dst_ptr_t = GetPointer<const pointer_type>(dst_type);
      const auto src_ptr_t = GetPointer<const pointer_type>(src_type);
      unsigned long long dst_size;
      if(dst_ptr_t)
      {
         dst_size = tree_helper::SizeAlloc(dst_ptr_t->ptd);
      }
      else
      {
         const auto dst_rptr_t = GetPointer<const reference_type>(dst_type);
         dst_size = tree_helper::SizeAlloc(dst_rptr_t->refd);
      }
      unsigned long long src_size;
      if(src_ptr_t)
      {
         src_size = tree_helper::SizeAlloc(src_ptr_t->ptd);
      }
      else
      {
         const auto src_rptr_t = GetPointer<const reference_type>(src_type);
         src_size = tree_helper::SizeAlloc(src_rptr_t->refd);
      }
      if(src_size != dst_size)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---WARNING: src_size = " + STR(src_size) + "; dst_size = " + STR(dst_size));
      }
      THROW_ASSERT(src_size % 8U == 0, "");
      copy_byte_size = src_size / 8U;
   }
   else if(rhs_kind == string_cst_K)
   {
      // compute src param
      const auto memcpy_src_ga = tree_man->CreateGimpleAssignAddrExpr(rhs_node, function_id, current_srcp);
      // push the new gimple_assign with lhs = addr_expr(param_decl) before the call
      THROW_ASSERT(not sl->list_of_bloc.empty(), "");
      THROW_ASSERT(sl->list_of_bloc.find(ga->bb_index) != sl->list_of_bloc.end(), "");
      const auto block = sl->list_of_bloc.at(ga->bb_index);
      block->PushBefore(memcpy_src_ga, stmt, AppM);
      // push back src param
      const auto new_ga = GetPointer<const gimple_assign>(memcpy_src_ga);
      args.push_back(new_ga->op0);

      // compute the size in bytes of the copied memory
      const auto dst_type = tree_helper::CGetType(mr_lhs->op0);
      const auto dst_ptr_t = GetPointer<const pointer_type>(dst_type);
      THROW_ASSERT(dst_ptr_t, "");
      const auto dst_bitsize = tree_helper::SizeAlloc(dst_ptr_t->ptd);
      THROW_ASSERT(dst_bitsize % 8U == 0, "");
      const auto dst_size = dst_bitsize / 8U;
      const auto sc = GetPointer<const string_cst>(rhs_node);
      const auto src_strlen = sc->strg.length();
      if((src_strlen + 1) != dst_size)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---WARNING: src_strlen = " + STR(src_strlen) + "; dst_size = " + STR(dst_size));
      }
      copy_byte_size = src_strlen;
   }
   else // rhs_kind == parm_decl_K
   {
      // compute src param
      const auto memcpy_src_ga = tree_man->CreateGimpleAssignAddrExpr(rhs_node, function_id, current_srcp);
      // push the new gimple_assign with lhs = addr_expr(param_decl) before the call
      THROW_ASSERT(!sl->list_of_bloc.empty(), "");
      THROW_ASSERT(sl->list_of_bloc.find(ga->bb_index) != sl->list_of_bloc.end(), "");
      const auto block = sl->list_of_bloc.at(ga->bb_index);
      block->PushBefore(memcpy_src_ga, stmt, AppM);
      // push back src param
      const auto new_ga = GetPointer<const gimple_assign>(memcpy_src_ga);
      args.push_back(new_ga->op0);

      // compute the size in bytes of the copied memory
      const auto dst_type = tree_helper::CGetType(mr_lhs->op0);
      const auto src_type = tree_man->GetPointerType(tree_helper::CGetType(rhs_node), 8);
      const auto dst_ptr_t = GetPointer<const pointer_type>(dst_type);
      const auto src_ptr_t = GetPointer<const pointer_type>(src_type);
      THROW_ASSERT(dst_ptr_t, "");
      THROW_ASSERT(src_ptr_t, "");
      const auto dst_size = tree_helper::SizeAlloc(dst_ptr_t->ptd);
      const auto src_size = tree_helper::SizeAlloc(src_ptr_t->ptd);
      if(src_size != dst_size)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---WARNING: src_size = " + STR(src_size) + "; dst_size = " + STR(dst_size));
      }
      THROW_ASSERT(src_size % 8 == 0, "");
      copy_byte_size = src_size / 8;
   }
   THROW_ASSERT(copy_byte_size <= std::numeric_limits<unsigned long int>::max(), "");

   const auto memcpy_fu_node = TM->GetFunction(MEMCPY);
   // add size to arguments
   const auto formal_type_node = tree_helper::GetFormalIth(memcpy_fu_node, 2);
   args.push_back(TM->CreateUniqueIntegerCst(static_cast<long long>(copy_byte_size), formal_type_node));
   // create the new gimple call
   const auto new_gimple_call = tree_man->create_gimple_call(memcpy_fu_node, args, function_id, current_srcp);
   // replace the gimple_assign with the new gimple_call
   THROW_ASSERT(!sl->list_of_bloc.empty(), "");
   THROW_ASSERT(sl->list_of_bloc.find(ga->bb_index) != sl->list_of_bloc.end(), "");
   const auto block = sl->list_of_bloc.at(ga->bb_index);
   block->Replace(stmt, new_gimple_call, true, AppM);

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "---Replaced hidden call " + STR(ga) + " with call " + STR(new_gimple_call) +
                      " id: " + STR(new_gimple_call->index));
}

void compute_implicit_calls::replace_with_memset(tree_nodeRef stmt, const statement_list* sl,
                                                 tree_manipulationRef tree_man) const
{
   const auto ga = GetPointer<const gimple_assign>(stmt);
   const auto lhs_node = ga->op0;
   THROW_ASSERT(
       lhs_node->get_kind() == mem_ref_K,
       "unexpected condition: " + AppM->CGetFunctionBehavior(function_id)->CGetBehavioralHelper()->get_function_name() +
           " calls function " + MEMSET + " in operation " + ga->ToString() + " but lhs " + lhs_node->ToString() +
           " is not a mem_ref: it's a " + lhs_node->get_kind_text());
   const auto mr_lhs = GetPointer<const mem_ref>(lhs_node);
   THROW_ASSERT(GetPointer<const ssa_name>(mr_lhs->op0), "");
   THROW_ASSERT(mr_lhs->op1->get_kind() == integer_cst_K && tree_helper::GetConstValue(mr_lhs->op1) == 0, "");

   const auto s = GetPointer<const srcp>(stmt);
   const auto current_srcp = s ? (s->include_name + ":" + STR(s->line_number) + ":" + STR(s->column_number)) : "";

   unsigned long int copy_byte_size = 0U;
   // args to be filled before the creation of the gimple call
   // dst is always the ssa on the rhs
   std::vector<tree_nodeRef> args = {mr_lhs->op0};

   THROW_ASSERT(GetPointer<const constructor>(ga->op1)->list_of_idx_valu.empty(), "");
   const auto memset_fu_node = TM->GetFunction(MEMSET);

   // create the second argument of memset
   const auto memset_val_formal_type = tree_helper::GetFormalIth(memset_fu_node, 1);
   args.push_back(TM->CreateUniqueIntegerCst(0, memset_val_formal_type));

   // compute the size of memory to be set with memset
   const auto dst_type = tree_helper::CGetType(mr_lhs->op0);
   const auto dst_ptr_t = GetPointer<const pointer_type>(dst_type);
   THROW_ASSERT(dst_ptr_t, "");
   const auto dst_size = tree_helper::SizeAlloc(dst_ptr_t->ptd);
   THROW_ASSERT(dst_size % 8U == 0, "");
   copy_byte_size = dst_size / 8U;
   THROW_ASSERT(copy_byte_size <= std::numeric_limits<unsigned long int>::max(), "");

   // add size to arguments
   const auto size_formal_type = tree_helper::GetFormalIth(memset_fu_node, 2);
   args.push_back(TM->CreateUniqueIntegerCst(static_cast<long long>(copy_byte_size), size_formal_type));
   // create the new gimple call
   const auto new_gimple_call = tree_man->create_gimple_call(memset_fu_node, args, function_id, current_srcp);
   // replace the gimple_assign with the new gimple_call
   THROW_ASSERT(!sl->list_of_bloc.empty(), "");
   THROW_ASSERT(sl->list_of_bloc.find(ga->bb_index) != sl->list_of_bloc.end(), "");
   const auto block = sl->list_of_bloc.at(ga->bb_index);
   block->Replace(stmt, new_gimple_call, true, AppM);

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "<--Replaced hidden call " + STR(ga) + " with call " + STR(new_gimple_call) +
                      " id: " + STR(new_gimple_call->index));
}
