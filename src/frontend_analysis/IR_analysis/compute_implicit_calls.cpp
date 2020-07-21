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
#include "tree_reindex.hpp"

compute_implicit_calls::compute_implicit_calls(const ParameterConstRef _parameters, const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, COMPUTE_IMPLICIT_CALLS, _design_flow_manager, _parameters), TM(_AppM->get_tree_manager())
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

compute_implicit_calls::~compute_implicit_calls() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> compute_implicit_calls::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
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

DesignFlowStep_Status compute_implicit_calls::InternalExec()
{
   tree_nodeRef node = TM->get_tree_node_const(function_id);
   auto* fd = GetPointer<function_decl>(node);
   if(!fd || !fd->body)
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Node is not a function or it hasn't a body");
      return DesignFlowStep_Status::UNCHANGED;
   }
   bool changed = false;
   std::list<std::pair<tree_nodeRef, unsigned int>> to_be_lowered_memset;

   unsigned int max_loop_id = 0;

   auto* sl = GetPointer<statement_list>(GET_NODE(fd->body));
   THROW_ASSERT(sl, "Body is not a statement_list");
   std::map<unsigned int, blocRef>::iterator it_bb, it_bb_end = sl->list_of_bloc.end();
   for(it_bb = sl->list_of_bloc.begin(); it_bb != it_bb_end; ++it_bb)
   {
      if(it_bb->second->number == BB_ENTRY || it_bb->second->number == BB_EXIT)
         continue;
      max_loop_id = std::max(max_loop_id, it_bb->second->loop_id);
      for(const auto& stmt : it_bb->second->CGetStmtList())
      {
         tree_nodeRef tn = GET_NODE(stmt);
         if(tn->get_kind() == gimple_assign_K)
         {
            auto* gm = GetPointer<gimple_assign>(tn);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing node " + tn->ToString());

            /// check for implicit memset/memcpy calls
            tree_nodeRef op0 = GET_NODE(gm->op0);
            tree_nodeRef op1 = GET_NODE(gm->op1);
            unsigned int op0_type_index, op1_type_index;
            tree_nodeRef op0_type = tree_helper::get_type_node(op0, op0_type_index);
            tree_nodeRef op1_type = tree_helper::get_type_node(op1, op1_type_index);

            bool is_a_vector_bitfield = false;
            if(op1->get_kind() == bit_field_ref_K)
            {
               auto* bfr = GetPointer<bit_field_ref>(op1);
               if(tree_helper::is_a_vector(TM, GET_INDEX_NODE(bfr->op0)))
                  is_a_vector_bitfield = true;
            }

            bool load_candidate = (op1->get_kind() == bit_field_ref_K && !is_a_vector_bitfield) || op1->get_kind() == component_ref_K || op1->get_kind() == indirect_ref_K || op1->get_kind() == misaligned_indirect_ref_K || op1->get_kind() == mem_ref_K ||
                                  op1->get_kind() == array_ref_K || op1->get_kind() == target_mem_ref_K || op1->get_kind() == target_mem_ref461_K;
            if(op1->get_kind() == realpart_expr_K || op1->get_kind() == imagpart_expr_K)
            {
               enum kind code1 = GET_NODE(GetPointer<unary_expr>(op1)->op)->get_kind();
               if((code1 == bit_field_ref_K && !is_a_vector_bitfield) || code1 == component_ref_K || code1 == indirect_ref_K || code1 == bit_field_ref_K || code1 == misaligned_indirect_ref_K || code1 == mem_ref_K || code1 == array_ref_K ||
                  code1 == target_mem_ref_K || code1 == target_mem_ref461_K)
                  load_candidate = true;
               if(code1 == var_decl_K)
                  load_candidate = true;
            }
            bool store_candidate = op0->get_kind() == bit_field_ref_K || op0->get_kind() == component_ref_K || op0->get_kind() == indirect_ref_K || op0->get_kind() == misaligned_indirect_ref_K || op0->get_kind() == mem_ref_K ||
                                   op0->get_kind() == array_ref_K || op0->get_kind() == target_mem_ref_K || op0->get_kind() == target_mem_ref461_K;
            if(op0->get_kind() == realpart_expr_K || op0->get_kind() == imagpart_expr_K)
            {
               enum kind code0 = GET_NODE(GetPointer<unary_expr>(op0)->op)->get_kind();
               if(code0 == component_ref_K || code0 == indirect_ref_K || code0 == bit_field_ref_K || code0 == misaligned_indirect_ref_K || code0 == mem_ref_K || code0 == array_ref_K || code0 == target_mem_ref_K || code0 == target_mem_ref461_K)
                  store_candidate = true;
               if(code0 == var_decl_K)
                  store_candidate = true;
            }
            if(!gm->clobber && !gm->init_assignment && op0_type && op1_type &&
               ((op0_type->get_kind() == record_type_K && op1_type->get_kind() == record_type_K && op1->get_kind() != view_convert_expr_K) ||
                (op0_type->get_kind() == union_type_K && op1_type->get_kind() == union_type_K && op1->get_kind() != view_convert_expr_K) || (op0_type->get_kind() == array_type_K) || (store_candidate && load_candidate)))
            {
               changed = true;
               if(op1->get_kind() == constructor_K && GetPointer<constructor>(op1) && GetPointer<constructor>(op1)->list_of_idx_valu.size() == 0)
               {
                  auto* mr = GetPointer<mem_ref>(op0);
                  THROW_ASSERT(mr, "unexpected condition");
                  unsigned int var = tree_helper::get_base_index(TM, GET_INDEX_NODE(mr->op0));
                  bool do_lowering = var != 0;
                  if(do_lowering)
                  {
                     auto type_index = tree_helper::get_type_index(TM, var);
                     const auto type_node = TM->get_tree_node_const(type_index);
                     do_lowering = type_node->get_kind() == array_type_K;
                     if(do_lowering)
                     {
                        const auto element_type = tree_helper::CGetElements(type_node);
                        const auto element_type_kind = element_type->get_kind();
                        if(not(element_type_kind == boolean_type_K or element_type_kind == CharType_K or element_type_kind == enumeral_type_K or element_type_kind == integer_type_K or element_type_kind == pointer_type_K or
                               element_type_kind == record_type_K))
                        {
                           do_lowering = false;
                           THROW_ASSERT(element_type_kind == array_type_K or element_type_kind == nullptr_type_K or element_type_kind == type_pack_expansion_K or element_type_kind == real_type_K or element_type_kind == complex_type_K or
                                            element_type_kind == function_type_K or element_type_kind == lang_type_K or element_type_kind == method_type_K or element_type_kind == offset_type_K or element_type_kind == qual_union_type_K or
                                            element_type_kind == record_type_K or element_type_kind == reference_type_K or element_type_kind == set_type_K or element_type_kind == template_type_parm_K or element_type_kind == typename_type_K or
                                            element_type_kind == union_type_K or element_type_kind == vector_type_K or element_type_kind == void_type_K,
                                        element_type->get_kind_text());
                        }
                     }
                  }
                  if(do_lowering)
                     to_be_lowered_memset.push_front(std::make_pair(stmt, it_bb->second->number));
                  else
                  {
                     unsigned int memset_function_id = TM->function_index("__internal_bambu_memset");
                     THROW_ASSERT(AppM->GetFunctionBehavior(memset_function_id)->GetBehavioralHelper()->has_implementation(), "inconsistent behavioral helper");
                     AppM->GetCallGraphManager()->AddCallPoint(function_id, memset_function_id, GET_INDEX_NODE(stmt), FunctionEdgeInfo::CallType::direct_call);
                  }
               }
               else
               {
                  unsigned int memcpy_function_id = TM->function_index("__internal_bambu_memcpy");
                  THROW_ASSERT(AppM->GetFunctionBehavior(memcpy_function_id)->GetBehavioralHelper()->has_implementation(), "inconsistent behavioral helper");
                  AppM->GetCallGraphManager()->AddCallPoint(function_id, memcpy_function_id, GET_INDEX_NODE(stmt), FunctionEdgeInfo::CallType::direct_call);
               }
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed node " + tn->ToString());
         }
      }
   }

   /// do the memset transformations
   for(auto stmt_bb_pair : to_be_lowered_memset)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Transforming (" + STR(stmt_bb_pair.first->index) + ")" + STR(stmt_bb_pair.first));
      auto BB1_index = stmt_bb_pair.second;
      auto BB1_block = sl->list_of_bloc[BB1_index];

      /// create BBN1
      const auto BBN1_block_index = (sl->list_of_bloc.rbegin())->first + 1;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created BB" + STR(BBN1_block_index));
      auto BBN1_block = blocRef(new bloc(BBN1_block_index));
      sl->list_of_bloc[BBN1_block_index] = BBN1_block;

      max_loop_id++;
      BBN1_block->loop_id = max_loop_id;
      BBN1_block->schedule = BB1_block->schedule;

      /// Add BBN1 predecessor as BB1_index basic block
      BBN1_block->list_of_pred.push_back(BB1_index);
      BBN1_block->list_of_pred.push_back(BBN1_block_index);

      /// create BBN2
      const auto BBN2_block_index = (sl->list_of_bloc.rbegin())->first + 1;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created BB" + STR(BBN2_block_index));
      auto BBN2_block = blocRef(new bloc(BBN2_block_index));
      sl->list_of_bloc[BBN2_block_index] = BBN2_block;

      BBN2_block->loop_id = BB1_block->loop_id;
      BBN2_block->schedule = BB1_block->schedule;

      /// Add BBN2 predecessor as BBN1 basic block
      BBN2_block->list_of_pred.push_back(BBN1_block_index);
      /// Add BBN2 successor as BB1 basic block
      BBN2_block->list_of_succ = BB1_block->list_of_succ;
      /// fix true and false edges
      BBN2_block->true_edge = BB1_block->true_edge;
      BB1_block->true_edge = 0;
      BBN2_block->false_edge = BB1_block->false_edge;
      BB1_block->false_edge = 0;
      BB1_block->list_of_succ.clear();
      BB1_block->list_of_succ.push_back(BBN1_block_index);
      /// Add BBN1 successor as succ basic block
      BBN1_block->list_of_succ.push_back(BBN1_block_index);
      BBN1_block->true_edge = BBN1_block_index;
      BBN1_block->list_of_succ.push_back(BBN2_block_index);
      BBN1_block->false_edge = BBN2_block_index;

      /// Fix BBN2 successors
      for(auto succ : BBN2_block->list_of_succ)
      {
         auto succ_block = sl->list_of_bloc[succ];
         succ_block->list_of_pred.erase(std::find(succ_block->list_of_pred.begin(), succ_block->list_of_pred.end(), BB1_index));
         succ_block->list_of_pred.push_back(BBN2_block_index);
         /// Update all the phis
         for(auto& phi : succ_block->CGetPhiList())
         {
            auto gp = GetPointer<gimple_phi>(GET_NODE(phi));
            for(auto& def_edge : gp->CGetDefEdgesList())
            {
               if(def_edge.second == BB1_index)
               {
                  gp->ReplaceDefEdge(TM, def_edge, gimple_phi::DefEdge(def_edge.first, BBN2_block_index));
               }
            }
         }
      }
      /// The tree manipulation
      auto tree_man = tree_manipulationRef(new tree_manipulation(TM, parameters));

      /// retrieve the starting variable
      auto* ga = GetPointer<gimple_assign>(GET_NODE(stmt_bb_pair.first));
      auto* mr = GetPointer<mem_ref>(GET_NODE(ga->op0));
      unsigned int var = tree_helper::get_base_index(TM, GET_INDEX_NODE(mr->op0));
      tree_nodeRef init_var = mr->op0;
      const std::string srcp_default = ga->include_name + ":" + STR(ga->line_number) + ":" + STR(ga->column_number);
      auto type_index = tree_helper::get_type_index(TM, var);
      tree_nodeConstRef type_node1 = TM->CGetTreeNode(type_index);
      auto algn = GetPointer<const type_node>(type_node1)->algn;
      THROW_ASSERT(type_node1->get_kind() == array_type_K, "unexpected condition");
      while(type_node1->get_kind() == array_type_K)
         type_node1 = tree_helper::CGetElements(type_node1);
      tree_nodeRef offset_type = tree_man->create_size_type();
      tree_nodeRef pt = tree_man->create_pointer_type(type_node1, algn);

      /// add a cast
      tree_nodeRef nop_init_var = tree_man->create_unary_operation(pt, init_var, srcp_default, nop_expr_K);
      tree_nodeRef nop_init_var_ga = tree_man->CreateGimpleAssign(pt, tree_nodeRef(), tree_nodeRef(), nop_init_var, BB1_index, srcp_default);
      init_var = GetPointer<gimple_assign>(GET_NODE(nop_init_var_ga))->op0;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---adding cast statement " + GET_NODE(nop_init_var_ga)->ToString());

      /// Create phi for the induction variable of BBN1
      tree_nodeRef new_induction_var;
      /// The list of def edge which contains for the moment only the value coming from the forward edge
      std::vector<std::pair<tree_nodeRef, unsigned int>> list_of_def_edge;
      list_of_def_edge.push_back(std::pair<tree_nodeRef, unsigned int>(init_var, BB1_index));
      auto phi = tree_man->create_phi_node(new_induction_var, list_of_def_edge, TM->GetTreeReindex(function_id), BBN1_block_index);
      BBN1_block->AddPhi(phi);
      auto* gp = GetPointer<gimple_phi>(GET_NODE(phi));
      GetPointer<ssa_name>(GET_NODE(gp->res))->use_set = PointToSolutionRef(new PointToSolution());
      GetPointer<ssa_name>(GET_NODE(gp->res))->use_set->variables.push_back(TM->GetTreeReindex(var));

      /// compute the size of memory to be set with memset
      const auto dst_type = tree_helper::CGetType(GET_NODE(mr->op0));
      const auto* dst_ptr_t = GetPointer<const pointer_type>(dst_type);
      THROW_ASSERT(dst_ptr_t, "unexpected condition");
      const auto dst_size = tree_helper::Size(dst_ptr_t->ptd);
      THROW_ASSERT(dst_size % 8 == 0, "unexpected condition");
      const auto copy_byte_size = dst_size / 8;
      tree_nodeRef copy_byte_size_node = TM->CreateUniqueIntegerCst(static_cast<long long int>(copy_byte_size), GET_INDEX_NODE(offset_type));
      tree_nodeRef pp = tree_man->create_binary_operation(pt, init_var, copy_byte_size_node, srcp_default, pointer_plus_expr_K);
      tree_nodeRef pp_ga = tree_man->CreateGimpleAssign(pt, tree_nodeRef(), tree_nodeRef(), pp, BB1_index, srcp_default);
      GetPointer<gimple_assign>(GET_NODE(pp_ga))->temporary_address = true;
      tree_nodeRef vd_limit = GetPointer<gimple_assign>(GET_NODE(pp_ga))->op0;
      GetPointer<ssa_name>(GET_NODE(vd_limit))->use_set = PointToSolutionRef(new PointToSolution());
      GetPointer<ssa_name>(GET_NODE(vd_limit))->use_set->variables.push_back(TM->GetTreeReindex(var));

      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---adding statement " + GET_NODE(pp_ga)->ToString());
      tree_nodeRef size_node = TM->CreateUniqueIntegerCst(static_cast<long long int>(tree_helper::Size(type_node1) / 8), GET_INDEX_NODE(offset_type));
      tree_nodeRef pp_ind = tree_man->create_binary_operation(pt, gp->res, size_node, srcp_default, pointer_plus_expr_K);
      tree_nodeRef pp_ga_ind = tree_man->CreateGimpleAssign(pt, tree_nodeRef(), tree_nodeRef(), pp_ind, BBN1_block_index, srcp_default);
      GetPointer<gimple_assign>(GET_NODE(pp_ga_ind))->temporary_address = true;
      BBN1_block->PushBack(pp_ga_ind);
      tree_nodeRef vd_ind = GetPointer<gimple_assign>(GET_NODE(pp_ga_ind))->op0;
      GetPointer<ssa_name>(GET_NODE(vd_ind))->use_set = PointToSolutionRef(new PointToSolution());
      GetPointer<ssa_name>(GET_NODE(vd_ind))->use_set->variables.push_back(TM->GetTreeReindex(var));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---adding statement " + GET_NODE(pp_ga_ind)->ToString());
      GetPointer<gimple_phi>(GET_NODE(phi))->AddDefEdge(TM, gimple_phi::DefEdge(vd_ind, BBN1_block_index));

      /// the comparison
      const auto boolean_type = tree_man->create_boolean_type();
      const auto comparison = tree_man->create_binary_operation(boolean_type, vd_ind, vd_limit, srcp_default, ne_expr_K);
      tree_nodeRef comp_ga = tree_man->CreateGimpleAssign(boolean_type, TM->CreateUniqueIntegerCst(0, type_index), TM->CreateUniqueIntegerCst(1, type_index), comparison, BBN1_block_index, srcp_default);
      BBN1_block->PushBack(comp_ga);
      tree_nodeRef comp_res = GetPointer<gimple_assign>(GET_NODE(comp_ga))->op0;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---comparison assign " + STR(comp_ga));

      /// the gimple cond
      const auto gc = tree_man->create_gimple_cond(comp_res, srcp_default, BBN1_block_index);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---gimple cond " + STR(gc));

      /// restructure of the implicit memset statement
      tree_nodeRef zero_offset = TM->CreateUniqueIntegerCst(0, GET_INDEX_NODE(pt));
      tree_nodeRef new_mem_ref = tree_man->create_binary_operation(TM->GetTreeReindex(type_node1->index), gp->res, zero_offset, srcp_default, mem_ref_K);
      ga->op0 = new_mem_ref;
      tree_nodeRef zero_value = TM->CreateUniqueIntegerCst(0, type_node1->index);
      ga->op1 = zero_value;

      const auto list_of_stmt = BB1_block->CGetStmtList();
      bool found_memset_statement = false;
      /// We must use pointer since we are erasing elements in the list
      for(auto statement = list_of_stmt.begin(); statement != list_of_stmt.end(); statement++)
      {
         if(GET_INDEX_NODE(*statement) == GET_INDEX_CONST_NODE(stmt_bb_pair.first))
         {
            /// move var = {}; to BBN1
            found_memset_statement = true;
            const auto temp_statement = *statement;
            /// Going one step step forward to avoid invalidation of the pointer
            auto tmp_it = statement;
            ++tmp_it;
            /// Moving statement
            BB1_block->RemoveStmt(temp_statement);
            BBN1_block->PushBack(temp_statement);
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
            BB1_block->RemoveStmt(temp_statement);
            BBN2_block->PushBack(temp_statement);
            /// Going one step back since pointer is already increment in for loop
            --tmp_it;
            statement = tmp_it;
         }
      }
      THROW_ASSERT(found_memset_statement, "unexpected condition");
      BB1_block->PushBack(nop_init_var_ga);
      BB1_block->PushBack(pp_ga);
      BBN1_block->PushBack(gc);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Transformed " + STR(stmt_bb_pair.first));
   }
   if(debug_level >= DEBUG_LEVEL_PEDANTIC && parameters->getOption<bool>(OPT_print_dot))
      AppM->CGetCallGraphManager()->CGetCallGraph()->WriteDot("compute_implicit_calls" + GetSignature() + ".dot");
   return changed ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}
