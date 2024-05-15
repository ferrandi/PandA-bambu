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
 * @file CSE.cpp
 * @brief common subexpression elimination step
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

/// Header include
#include "CSE.hpp"

///. include
#include "Parameter.hpp"

/// boost include
#include <boost/graph/topological_sort.hpp>

/// behavior includes
#include "application_manager.hpp"
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"

/// algorithm/dominance include
#include "Dominance.hpp"

/// design_flows include
#include "design_flow_manager.hpp"

/// frontend_analysis
#include "application_frontend_flow_step.hpp"

/// HLS include
#include "hls_manager.hpp"
#if HAVE_ILP_BUILT
/// HLS includes
#include "hls.hpp"

/// HLS/scheduling include
#include "schedule.hpp"
#endif

/// STD include
#include <cmath>
#include <fstream>
#include <string>

/// tree include
#include "behavioral_helper.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"

#include "basic_block.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

CSE::CSE(const ParameterConstRef _parameters, const application_managerRef _AppM, unsigned int _function_id,
         const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, CSE_STEP, _design_flow_manager, _parameters),
      TM(_AppM->get_tree_manager()),
      restart_phi_opt(false)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CSE::~CSE() = default;

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
CSE::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         if(!parameters->getOption<int>(OPT_gcc_openmp_simd))
         {
            relationships.insert(std::make_pair(BITVALUE_RANGE, SAME_FUNCTION));
         }
         relationships.insert(std::make_pair(DEAD_CODE_ELIMINATION, SAME_FUNCTION));
         relationships.insert(std::make_pair(DEAD_CODE_ELIMINATION_IPA, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(DETERMINE_MEMORY_ACCESSES, SAME_FUNCTION));
         relationships.insert(std::make_pair(SIMPLE_CODE_MOTION, SAME_FUNCTION));
         relationships.insert(std::make_pair(USE_COUNTING, SAME_FUNCTION));
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(IR_LOWERING, SAME_FUNCTION));
         relationships.insert(std::make_pair(PHI_OPT, SAME_FUNCTION));
         relationships.insert(std::make_pair(SDC_CODE_MOTION, SAME_FUNCTION));
         relationships.insert(std::make_pair(UN_COMPARISON_LOWERING, SAME_FUNCTION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         if(GetStatus() == DesignFlowStep_Status::SUCCESS)
         {
            relationships.insert(std::make_pair(DEAD_CODE_ELIMINATION, SAME_FUNCTION));
            if(restart_phi_opt)
            {
               relationships.insert(std::make_pair(PHI_OPT, SAME_FUNCTION));
            }
         }
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   return relationships;
}

void CSE::Initialize()
{
#if HAVE_ILP_BUILT
   if(GetPointer<const HLS_manager>(AppM) && GetPointerS<const HLS_manager>(AppM)->get_HLS(function_id) &&
      GetPointerS<const HLS_manager>(AppM)->get_HLS(function_id)->Rsch)
   {
      schedule = GetPointerS<const HLS_manager>(AppM)->get_HLS(function_id)->Rsch;
   }
#endif
}

DesignFlowStep_Status CSE::InternalExec()
{
   if(parameters->IsParameter("disable-cse") && parameters->GetParameter<unsigned int>("disable-cse") == 1)
   {
      return DesignFlowStep_Status::UNCHANGED;
   }
   bool IR_changed = false;
   restart_phi_opt = false;
#ifndef NDEBUG
   size_t n_equiv_stmt = 0;
#endif
   const auto IRman = tree_manipulationRef(new tree_manipulation(TM, parameters, AppM));
   /// define a map relating variables and columns
   std::map<vertex, CustomUnorderedMapStable<CSE_tuple_key_type, tree_nodeRef>> unique_table;

   const auto temp = TM->GetTreeNode(function_id);
   const auto fd = GetPointerS<const function_decl>(temp);
   const auto sl = GetPointerS<const statement_list>(fd->body);

   /// store the GCC BB graph ala boost::graph
   BBGraphsCollectionRef GCC_bb_graphs_collection(
       new BBGraphsCollection(BBGraphInfoRef(new BBGraphInfo(AppM, function_id)), parameters));
   BBGraphRef GCC_bb_graph(new BBGraph(GCC_bb_graphs_collection, CFG_SELECTOR));
   CustomUnorderedMap<unsigned int, vertex> inverse_vertex_map;
   /// add vertices
   for(const auto& block : sl->list_of_bloc)
   {
      inverse_vertex_map[block.first] =
          GCC_bb_graphs_collection->AddVertex(BBNodeInfoRef(new BBNodeInfo(block.second)));
   }
   /// add edges
   for(const auto& idx_bb : sl->list_of_bloc)
   {
      for(const auto& pred : idx_bb.second->list_of_pred)
      {
         THROW_ASSERT(inverse_vertex_map.find(pred) != inverse_vertex_map.end(),
                      "BB" + STR(pred) + " (successor of BB" + STR(idx_bb.first) + ") does not exist");
         GCC_bb_graphs_collection->AddEdge(inverse_vertex_map[pred], inverse_vertex_map[idx_bb.first], CFG_SELECTOR);
      }
      for(const auto& succ : idx_bb.second->list_of_succ)
      {
         if(succ == bloc::EXIT_BLOCK_ID)
         {
            GCC_bb_graphs_collection->AddEdge(inverse_vertex_map[idx_bb.first], inverse_vertex_map[succ], CFG_SELECTOR);
         }
      }
      if(idx_bb.second->list_of_succ.empty())
      {
         GCC_bb_graphs_collection->AddEdge(inverse_vertex_map[idx_bb.first], inverse_vertex_map[bloc::EXIT_BLOCK_ID],
                                           CFG_SELECTOR);
      }
   }
   /// add a connection between entry and exit thus avoiding problems with non terminating code
   GCC_bb_graphs_collection->AddEdge(inverse_vertex_map[bloc::ENTRY_BLOCK_ID], inverse_vertex_map[bloc::EXIT_BLOCK_ID],
                                     CFG_SELECTOR);

   refcount<dominance<BBGraph>> bb_dominators;
   bb_dominators = refcount<dominance<BBGraph>>(new dominance<BBGraph>(
       *GCC_bb_graph, inverse_vertex_map[bloc::ENTRY_BLOCK_ID], inverse_vertex_map[bloc::EXIT_BLOCK_ID], parameters));
   bb_dominators->calculate_dominance_info(dominance<BBGraph>::CDI_DOMINATORS);
   const auto& bb_dominator_map = bb_dominators->get_dominator_map();

   BBGraphRef bb_domGraph(new BBGraph(GCC_bb_graphs_collection, D_SELECTOR));
   for(auto it : bb_dominator_map)
   {
      if(it.first != inverse_vertex_map[bloc::ENTRY_BLOCK_ID])
      {
         GCC_bb_graphs_collection->AddEdge(it.second, it.first, D_SELECTOR);
      }
   }

   std::deque<vertex> sort_list;
   boost::topological_sort(*bb_domGraph, std::front_inserter(sort_list));

   for(const auto& bb : sort_list)
   {
      const auto B = bb_domGraph->CGetBBNodeInfo(bb)->block;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering BB " + STR(B->number));
      /// CSE on basic blocks
      unique_table[bb].clear();
      if(bb_dominator_map.find(bb) != bb_dominator_map.end())
      {
         THROW_ASSERT(unique_table.find(bb_dominator_map.at(bb)) != unique_table.end(), "unexpected condition");
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Adding dominator equiv: " +
                            STR(bb_domGraph->CGetBBNodeInfo(bb_dominator_map.at(bb))->block->number));

         for(const auto& key_value_pair : unique_table.at(bb_dominator_map.at(bb)))
         {
            unique_table.at(bb)[key_value_pair.first] = key_value_pair.second;
         }
      }
      TreeNodeSet to_be_removed;
      for(const auto& stmt : B->CGetStmtList())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Analyzing " + stmt->ToString());
         if(!AppM->ApplyNewTransformation())
         {
            break;
         }
         const auto eq_tn = hash_check(stmt, bb, sl, unique_table);
         if(eq_tn)
         {
            const auto ref_ga = GetPointerS<gimple_assign>(eq_tn);
            const auto dead_ga = GetPointerS<const gimple_assign>(stmt);
            const auto ref_ssa = GetPointerS<ssa_name>(ref_ga->op0);
            const auto dead_ssa = GetPointerS<const ssa_name>(dead_ga->op0);

            if(ref_ssa->min)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---ref_min=" + STR(ref_ssa->min));
            }
            if(ref_ssa->max)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---ref_max=" + STR(ref_ssa->max));
            }
            if(dead_ssa->min)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---dead_min=" + STR(dead_ssa->min));
            }
            if(dead_ssa->max)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---dead_max=" + STR(dead_ssa->max));
            }

            if(!ref_ssa->bit_values.empty())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---ref_bit_values=" + ref_ssa->bit_values);
            }
            if(!dead_ssa->bit_values.empty())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---dead_bit_values=" + dead_ssa->bit_values);
            }

            bool same_range = false;
            if(!parameters->getOption<int>(OPT_gcc_openmp_simd))
            {
               same_range = dead_ssa->bit_values.empty() || ref_ssa->bit_values == dead_ssa->bit_values;
            }
            else
            {
               const auto ga_op_type = tree_helper::CGetType(ref_ga->op0);
               if(ga_op_type->get_kind() == integer_type_K && ref_ssa->min && ref_ssa->max && dead_ssa->min &&
                  dead_ssa->max)
               {
                  const auto ref_min = tree_helper::GetConstValue(ref_ssa->min);
                  const auto dead_min = tree_helper::GetConstValue(dead_ssa->min);
                  const auto ref_max = tree_helper::GetConstValue(ref_ssa->max);
                  const auto dead_max = tree_helper::GetConstValue(dead_ssa->max);
                  if(dead_min == ref_min && dead_max == ref_max)
                  {
                     same_range = true;
                  }
                  else
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---replace equivalent statement before: ref_min=" + STR(ref_min) + " dead_min=" +
                                        STR(dead_min) + " ref_max=" + STR(ref_max) + " dead_max=" + STR(dead_max));
                  }
               }
            }
            if(same_range)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Updating/Removing " + STR(dead_ga->op0));
               ref_ga->temporary_address = ref_ga->temporary_address && dead_ga->temporary_address;
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---ref_ga->temporary_address" +
                                  (ref_ga->temporary_address ? std::string("T") : std::string("F")));
               if(dead_ssa->use_set && !ref_ssa->use_set)
               {
                  ref_ssa->use_set = dead_ssa->use_set;
               }

               const auto StmtUses = dead_ssa->CGetUseStmts();
               for(const auto& use : StmtUses)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---replace equivalent statement before: " + use.first->ToString());
                  TM->ReplaceTreeNode(use.first, dead_ga->op0, ref_ga->op0);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---replace equivalent statement after: " + use.first->ToString());
               }
               to_be_removed.insert(stmt);

               AppM->RegisterTransformation(GetName(), stmt);
               IR_changed = true;
#ifndef NDEBUG
               ++n_equiv_stmt;
#endif
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "<--Updated/Removed duplicated statement " + STR(dead_ga->op0));
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---not the same range");
            }
         }
      }
      for(const auto& stmt : to_be_removed)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Removing " + stmt->ToString());
         B->RemoveStmt(stmt, AppM);
      }
      if(B->CGetStmtList().empty() && B->CGetPhiList().empty() && !to_be_removed.empty())
      {
         restart_phi_opt = true;
      }
      if(!to_be_removed.empty() && schedule)
      {
         for(const auto& stmt : B->CGetStmtList())
         {
            schedule->UpdateTime(stmt->index);
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered BB" + STR(B->number));
   }
   if(!IR_changed)
   {
      restart_phi_opt = false;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "---CSE: number of equivalent statement = " + STR(n_equiv_stmt));
   IR_changed ? function_behavior->UpdateBBVersion() : 0;
   return IR_changed ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}

bool CSE::has_memory_access(const gimple_assign* ga) const
{
   const auto& fun_mem_data = function_behavior->get_function_mem();
   const auto rhs_kind = ga->op1->get_kind();
   const auto op0_type = tree_helper::CGetType(ga->op0);
   const auto op1_type = tree_helper::CGetType(ga->op1);
   /// check for bit field ref of vector type
   const auto is_a_vector_bitfield =
       rhs_kind == bit_field_ref_K && tree_helper::IsVectorType(GetPointerS<const bit_field_ref>(ga->op1)->op0);

   bool skip_check = rhs_kind == var_decl_K || rhs_kind == string_cst_K || rhs_kind == constructor_K ||
                     (rhs_kind == bit_field_ref_K && !is_a_vector_bitfield) || rhs_kind == component_ref_K ||
                     rhs_kind == indirect_ref_K || rhs_kind == misaligned_indirect_ref_K || rhs_kind == array_ref_K ||
                     rhs_kind == target_mem_ref_K || rhs_kind == target_mem_ref461_K or rhs_kind == mem_ref_K;
   if(rhs_kind == realpart_expr_K || rhs_kind == imagpart_expr_K)
   {
      const auto code1 = GetPointerS<const unary_expr>(ga->op1)->op->get_kind();
      if((code1 == bit_field_ref_K && !is_a_vector_bitfield) || code1 == component_ref_K || code1 == indirect_ref_K ||
         code1 == bit_field_ref_K || code1 == misaligned_indirect_ref_K || code1 == mem_ref_K || code1 == array_ref_K ||
         code1 == target_mem_ref_K || code1 == target_mem_ref461_K)
      {
         skip_check = true;
      }
      if(code1 == var_decl_K &&
         fun_mem_data.find(GetPointerS<const unary_expr>(ga->op1)->op->index) != fun_mem_data.end())
      {
         skip_check = true;
      }
   }
   else if(rhs_kind == view_convert_expr_K)
   {
      const auto vc = GetPointerS<const view_convert_expr>(ga->op1);
      const auto vc_op_type = tree_helper::CGetType(vc->op);
      if(op0_type->get_kind() == record_type_K || op0_type->get_kind() == union_type_K)
      {
         skip_check = true;
      }
      if(vc_op_type->get_kind() == record_type_K || vc_op_type->get_kind() == union_type_K)
      {
         skip_check = true;
      }

      if(vc_op_type->get_kind() == array_type_K && op0_type->get_kind() == vector_type_K)
      {
         skip_check = true;
      }
      if(vc_op_type->get_kind() == vector_type_K && op0_type->get_kind() == array_type_K)
      {
         skip_check = true;
      }
   }
   if(!tree_helper::IsVectorType(ga->op0) && tree_helper::IsArrayEquivType(ga->op0) &&
      !tree_helper::IsPointerType(ga->op0))
   {
      skip_check = true;
   }
   if(fun_mem_data.find(ga->op0->index) != fun_mem_data.end() ||
      fun_mem_data.find(ga->op1->index) != fun_mem_data.end())
   {
      skip_check = true;
   }
   if(op0_type && op1_type &&
      ((op0_type->get_kind() == record_type_K && op1_type->get_kind() == record_type_K &&
        rhs_kind != view_convert_expr_K) ||
       (op0_type->get_kind() == union_type_K && op1_type->get_kind() == union_type_K &&
        rhs_kind != view_convert_expr_K) ||
       (op0_type->get_kind() == array_type_K)))
   {
      skip_check = true;
   }

   return skip_check;
}

tree_nodeRef
CSE::hash_check(const tree_nodeRef& tn, vertex bb_vertex, const statement_list* sl,
                std::map<vertex, CustomUnorderedMapStable<CSE_tuple_key_type, tree_nodeRef>>& unique_table) const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Checking: " + tn->ToString());
   if(GetPointer<const gimple_node>(tn)->keep)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Checked: null keep");
      return nullptr;
   }
   const auto ga = GetPointer<const gimple_assign>(tn);
   if(ga && (ga->clobber || ga->init_assignment))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Checked: null clobber/init_assignment");
      return nullptr;
   }
   if(ga && ga->op0->get_kind() == ssa_name_K)
   {
      auto bitwidth_values = tree_helper::Size(ga->op0);
      const auto rhs = ga->op1;
      const auto rhs_kind = rhs->get_kind();
      if(GetPointer<const binary_expr>(rhs))
      {
         bitwidth_values = std::max(bitwidth_values, tree_helper::Size(GetPointerS<const binary_expr>(rhs)->op0));
      }
      if(rhs_kind != extract_bit_expr_K && rhs_kind != lut_expr_K && parameters->IsParameter("CSE_size") &&
         bitwidth_values < parameters->GetParameter<unsigned int>("CSE_size"))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Checked: too small");
         return nullptr;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Right part type: " + rhs->get_kind_text());

      /// check for LOADs, STOREs, MEMSET, MEMCPY, etc. etc.
      if(has_memory_access(ga))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Checked: null memory");
         return nullptr;
      }
      std::vector<unsigned int> ins;
      /// We add type of right part; load from same address with different types must be considered different
      ins.push_back(tree_helper::CGetType(ga->op1)->index);

      if(ga->vuses.size())
      {
         /// If there are virtual uses, not only they must be the same, but also the basic block must be the same
         ins.push_back(ga->bb_index);
         for(const auto& vuse : ga->vuses)
         {
            ins.push_back(vuse->index);

            /// Check if the virtual is defined in the same basic block
            const auto virtual_sn = GetPointerS<const ssa_name>(vuse);
            const auto virtual_sn_def = virtual_sn->CGetDefStmt();
            const auto virtual_sn_gn = GetPointerS<const gimple_node>(virtual_sn_def);

            if(virtual_sn_gn->bb_index == ga->bb_index)
            {
               THROW_ASSERT(sl->list_of_bloc.count(ga->bb_index), "");
               const auto& bb = sl->list_of_bloc.at(ga->bb_index);
               const auto vdef_it =
                   virtual_sn_def->get_kind() == gimple_phi_K ?
                       bb->CGetStmtList().end() :
                       std::find_if(bb->CGetStmtList().begin(), bb->CGetStmtList().end(),
                                    [&](const tree_nodeRef& stmt) { return stmt->index == virtual_sn_gn->index; });
               const auto ga_it = std::find_if(vdef_it, bb->CGetStmtList().end(),
                                               [&](const tree_nodeRef& stmt) { return stmt->index == ga->index; });
               if(ga_it != bb->CGetStmtList().end())
               {
                  ins.push_back(virtual_sn_def->index);
               }
            }
         }
      }
      if(rhs_kind == ssa_name_K)
      {
         ins.push_back(rhs->index);
      }
      else if(GetPointer<const cst_node>(rhs))
      {
         ins.push_back(rhs->index);
      }
      else if(rhs_kind == nop_expr_K || rhs_kind == view_convert_expr_K || rhs_kind == convert_expr_K ||
              rhs_kind == float_expr_K || rhs_kind == fix_trunc_expr_K)
      {
         const auto ue = GetPointerS<const unary_expr>(rhs);
         ins.push_back(ue->op->index);
         const auto type_index = tree_helper::CGetType(ga->op0)->index;
         ins.push_back(type_index);
      }
      else if(GetPointer<const unary_expr>(rhs))
      {
         const auto ue = GetPointerS<const unary_expr>(rhs);
         ins.push_back(ue->op->index);
      }
      else if(GetPointer<const binary_expr>(rhs))
      {
         const auto be = GetPointerS<const binary_expr>(rhs);
         ins.push_back(be->op0->index);
         ins.push_back(be->op1->index);
      }
      else if(GetPointer<const ternary_expr>(rhs))
      {
         const auto te = GetPointerS<const ternary_expr>(rhs);
         ins.push_back(te->op0->index);
         ins.push_back(te->op1->index);
         if(te->op2)
         {
            ins.push_back(te->op2->index);
         }
      }
      else if(GetPointer<const call_expr>(rhs))
      {
         const auto ce = GetPointerS<const call_expr>(rhs);
         if(ce->fn->get_kind() == addr_expr_K)
         {
            const auto addr_node = ce->fn;
            const auto ae = GetPointerS<const addr_expr>(addr_node);
            ins.push_back(ae->op->index);
            const auto fd = GetPointerS<const function_decl>(ae->op);
            // TODO: may be optimized
            if(fd->undefined_flag || fd->writing_memory || fd->reading_memory || ga->vuses.size())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Checked: null");
               return nullptr;
            }
            for(const auto& arg : ce->args)
            {
               ins.push_back(arg->index);
            }
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Checked: null");
            return nullptr;
         }
      }
      else if(GetPointer<const lut_expr>(rhs))
      {
         const auto le = GetPointerS<const lut_expr>(rhs);
         ins.push_back(le->op0->index);
         ins.push_back(le->op1->index);
         if(le->op2)
         {
            ins.push_back(le->op2->index);
         }
         if(le->op3)
         {
            ins.push_back(le->op3->index);
         }
         if(le->op4)
         {
            ins.push_back(le->op4->index);
         }
         if(le->op5)
         {
            ins.push_back(le->op5->index);
         }
         if(le->op6)
         {
            ins.push_back(le->op6->index);
         }
         if(le->op7)
         {
            ins.push_back(le->op7->index);
         }
         if(le->op8)
         {
            ins.push_back(le->op8->index);
         }
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Checked: null");
         return nullptr;
      }

#ifndef NDEBUG
      auto signature_message = "Signature of " + STR(tn->index) + " is ";
      for(const auto& temp_sign : ins)
      {
         signature_message += STR(temp_sign) + "-";
      }
      signature_message.pop_back();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, signature_message);
#endif
      CSE_tuple_key_type t(rhs_kind, ins);
      if(unique_table.at(bb_vertex).find(t) != unique_table.at(bb_vertex).end())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "--- statement = " + tn->ToString());
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "--- equivalent with = " + unique_table.at(bb_vertex).at(t)->ToString());
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
         THROW_ASSERT(!ga->memdef, "Unexpected memdef " + ga->memdef->ToString() + " in " + tn->ToString());
         THROW_ASSERT(!ga->vdef, "Unexpected vdef " + ga->vdef->ToString() + " in " + tn->ToString());
         THROW_ASSERT(ga->vovers.empty(), "Unexpected vovers in " + tn->ToString());
         return unique_table.at(bb_vertex).at(t);
      }
      else
      {
         unique_table.at(bb_vertex)[t] = tn;
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Checked: null");
   return nullptr;
}
