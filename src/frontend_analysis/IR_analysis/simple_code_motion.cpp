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
 * @file simple_code_motion.cpp
 * @brief Analysis step that performs some simple code motions over the IR
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

/// Skipping warnings due to operator() redefinition
#pragma GCC diagnostic ignored "-Woverloaded-virtual"

/// header include
#include "simple_code_motion.hpp"

///. include
#include "Parameter.hpp"

/// algorithm/dominance include
#include "Dominance.hpp"

/// behavior includes
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "function_behavior.hpp"

/// design_flows include
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"

#if HAVE_ILP_BUILT
/// HLS includes
#include "hls.hpp"
#include "hls_manager.hpp"
#include "hls_step.hpp"

/// HLS/scheduling include
#include "schedule.hpp"
#endif

/// parser/compiler include
#include "token_interface.hpp"

/// STD include
#include <fstream>

/// tree includes
#include "ext_tree_node.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "tree_node.hpp"

/// utility include
#include "dbgPrintHelper.hpp"
#include "math_function.hpp"

#include "behavioral_helper.hpp"
#include "cyclic_topological_sort.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

simple_code_motion::simple_code_motion(const ParameterConstRef _parameters, const application_managerRef _AppM,
                                       unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, SIMPLE_CODE_MOTION, _design_flow_manager, _parameters),
      restart_ifmwi_opt(false),
      schedule(ScheduleRef()),
      conservative(
#if HAVE_ILP_BUILT
          (parameters->IsParameter("enable-conservative-sdc") &&
           parameters->GetParameter<bool>("enable-conservative-sdc") &&
           parameters->isOption(OPT_scheduling_algorithm) and
           parameters->getOption<HLSFlowStep_Type>(OPT_scheduling_algorithm) == HLSFlowStep_Type::SDC_SCHEDULING) ?
              true :
              false
#else
          false
#endif
      )
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

simple_code_motion::~simple_code_motion() = default;

CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionFrontendFlowStep::FunctionRelationship>>
simple_code_motion::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(PREDICATE_STATEMENTS, SAME_FUNCTION));
         relationships.insert(std::make_pair(USE_COUNTING, SAME_FUNCTION));
         relationships.insert(std::make_pair(BLOCK_FIX, SAME_FUNCTION));
         relationships.insert(std::make_pair(SWITCH_FIX, SAME_FUNCTION));
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
#if HAVE_ILP_BUILT
         relationships.insert(std::make_pair(SDC_CODE_MOTION, SAME_FUNCTION));
#endif
         relationships.insert(std::make_pair(DEAD_CODE_ELIMINATION_IPA, WHOLE_APPLICATION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         if(GetStatus() == DesignFlowStep_Status::SUCCESS)
         {
            if(restart_ifmwi_opt)
            {
               relationships.insert(std::make_pair(SHORT_CIRCUIT_TAF, SAME_FUNCTION));
               relationships.insert(std::make_pair(PHI_OPT, SAME_FUNCTION));
               relationships.insert(std::make_pair(MULTI_WAY_IF, SAME_FUNCTION));
            }
         }
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}

void simple_code_motion::Initialize()
{
#if HAVE_ILP_BUILT
   if(GetPointer<const HLS_manager>(AppM) && GetPointer<const HLS_manager>(AppM)->get_HLS(function_id) &&
      GetPointer<const HLS_manager>(AppM)->get_HLS(function_id)->Rsch)
   {
      if(parameters->isOption(OPT_scheduling_algorithm) &&
         parameters->getOption<HLSFlowStep_Type>(OPT_scheduling_algorithm) == HLSFlowStep_Type::SDC_SCHEDULING)
      {
         const auto TM = AppM->get_tree_manager();
         schedule = GetPointerS<const HLS_manager>(AppM)->get_HLS(function_id)->Rsch;
      }
   }
#endif
}

FunctionFrontendFlowStep_Movable simple_code_motion::CheckMovable(const unsigned int bb_index, tree_nodeRef tn,
                                                                  bool& zero_delay, const tree_managerRef TM)
{
   if(AppM->CGetFunctionBehavior(function_id)->is_simple_pipeline())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Yes because we aim to full pipelining");
      return FunctionFrontendFlowStep_Movable::MOVABLE;
   }
   if(tn->get_kind() == gimple_nop_K)
   {
      return FunctionFrontendFlowStep_Movable::MOVABLE;
   }

   auto* ga = GetPointer<gimple_assign>(tn);

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "-->Checking if " + STR(ga->index) + " - " + ga->ToString() + " can be moved in BB" + STR(bb_index));
   tree_nodeRef left = ga->op0;
   if(!GetPointer<ssa_name>(left))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "<--No because of " + left->get_kind_text() + " in left part");
      return FunctionFrontendFlowStep_Movable::UNMOVABLE;
   }
   /// FIXME: already added in master?
   if(ga->op1->get_kind() == call_expr_K || ga->op1->get_kind() == aggr_init_expr_K)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No because of call_expr in right part");
      return FunctionFrontendFlowStep_Movable::UNMOVABLE;
   }
   if(tree_helper::is_constant(TM, ga->op1->index))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes because right part is constant");
      return FunctionFrontendFlowStep_Movable::MOVABLE;
   }
   if(ga->op0->get_kind() == ssa_name_K && ga->op1->get_kind() == constructor_K)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes because it is an assignment with a constructor");
      return FunctionFrontendFlowStep_Movable::MOVABLE;
   }
   if(ga->op0->get_kind() == ssa_name_K && ga->op1->get_kind() == ssa_name_K)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes because it is an assignment");
      return FunctionFrontendFlowStep_Movable::MOVABLE;
   }
   CustomOrderedSet<const ssa_name*> rhs_ssa_uses;
   tree_helper::compute_ssa_uses_rec_ptr(ga->op1, rhs_ssa_uses);
   tree_nodeRef right = ga->op1;

   if(rhs_ssa_uses.empty() && right->get_kind() != call_expr_K && right->get_kind() != aggr_init_expr_K &&
      right->get_kind() != var_decl_K && right->get_kind() != mem_ref_K)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "<--Yes because there is not any use of ssa in right part");
      return FunctionFrontendFlowStep_Movable::MOVABLE;
   }

   /// If we have the ending time information use it
#if HAVE_ILP_BUILT
   if(schedule)
   {
      auto movable = schedule->CanBeMoved(ga->index, bb_index);
      if(movable == FunctionFrontendFlowStep_Movable::UNMOVABLE or movable == FunctionFrontendFlowStep_Movable::TIMING)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No because of timing");
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes because of timing");
      }
      return movable;
   }
#endif
   switch(right->get_kind())
   {
      case assert_expr_K:
      case convert_expr_K:
      case view_convert_expr_K:
      case ssa_name_K:
      case constructor_K:
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes");
         return FunctionFrontendFlowStep_Movable::MOVABLE;
      }
      /// binary expressions
      case eq_expr_K:
      case lrotate_expr_K:
      case lshift_expr_K:
      case max_expr_K:
      case min_expr_K:
      case ne_expr_K:
      case rrotate_expr_K:
      case rshift_expr_K:
      {
         if(conservative)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No");
            return FunctionFrontendFlowStep_Movable::UNMOVABLE;
         }
         else
         {
            auto* be = GetPointer<binary_expr>(right);
            auto n_bit = std::max(tree_helper::Size(be->op0), tree_helper::Size(be->op1));
            bool is_constant =
                tree_helper::is_constant(TM, be->op0->index) || tree_helper::is_constant(TM, be->op1->index);
            if(n_bit > 9 && !is_constant)
            {
               zero_delay = false;
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes");
            return FunctionFrontendFlowStep_Movable::MOVABLE;
         }
      }
      case fshl_expr_K:
      case fshr_expr_K:
      {
         if(conservative)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No");
            return FunctionFrontendFlowStep_Movable::UNMOVABLE;
         }
         else
         {
            auto* te = GetPointer<ternary_expr>(right);
            auto n_bit = tree_helper::Size(te->op0);
            bool is_constant = tree_helper::is_constant(TM, te->op1->index);
            if(n_bit > 9 && !is_constant)
            {
               zero_delay = false;
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes");
            return FunctionFrontendFlowStep_Movable::MOVABLE;
         }
      }
      case mult_expr_K:
      case mult_highpart_expr_K:
      case widen_mult_expr_K:
      {
         if(tree_helper::is_real(TM, ga->op1->index))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No because floating point operations");
            return FunctionFrontendFlowStep_Movable::UNMOVABLE;
         }
         auto* be = GetPointer<binary_expr>(right);
         if(tree_helper::IsConstant(be->op1))
         {
            const auto ic = GetPointer<integer_cst>(be->op1);
            if(ic)
            {
               const auto v = tree_helper::GetConstValue(be->op1);
               if(!(v && !(v & (v - 1))))
               {
                  zero_delay = false;
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes");
               return FunctionFrontendFlowStep_Movable::MOVABLE;
            }
            else
            {
               zero_delay = false;
            }
         }
         else
         {
            zero_delay = false;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "<--No because is a multiplication with non constant args");
         return FunctionFrontendFlowStep_Movable::UNMOVABLE;
      }
      case nop_expr_K:
      {
         auto* ne = GetPointer<nop_expr>(right);
         const auto left_type = tree_helper::CGetType(ga->op0);
         const auto right_type = tree_helper::CGetType(ne->op);
         const auto is_realR = tree_helper::IsRealType(right_type);
         const auto is_realL = tree_helper::IsRealType(left_type);
         if(is_realR || is_realL)
         {
            zero_delay = false;
         }

         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes");
         return FunctionFrontendFlowStep_Movable::MOVABLE;
      }
      case addr_expr_K:
      {
         zero_delay = false;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes");
         return FunctionFrontendFlowStep_Movable::MOVABLE;
      }
      case bit_and_expr_K:
      case bit_ior_expr_K:
      case bit_xor_expr_K:
      case truth_and_expr_K:
      case truth_andif_expr_K:
      case truth_or_expr_K:
      case truth_orif_expr_K:
      case truth_xor_expr_K:
      case bit_not_expr_K:
      case truth_not_expr_K:
      case cond_expr_K:
      case lut_expr_K:
      {
         if(conservative)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No");
            return FunctionFrontendFlowStep_Movable::UNMOVABLE;
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes");
            return FunctionFrontendFlowStep_Movable::MOVABLE;
         }
      }
      case bit_ior_concat_expr_K:
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes");
         return FunctionFrontendFlowStep_Movable::MOVABLE;
      }
      case extractvalue_expr_K:
      case insertvalue_expr_K:
      case extract_bit_expr_K:
      case extractelement_expr_K:
      case insertelement_expr_K:
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes");
         return FunctionFrontendFlowStep_Movable::MOVABLE;
      }
      case ge_expr_K:
      case gt_expr_K:
      case le_expr_K:
      case lt_expr_K:
      case minus_expr_K:
      case plus_expr_K:
      case pointer_plus_expr_K:
      case postdecrement_expr_K:
      case postincrement_expr_K:
      case predecrement_expr_K:
      case preincrement_expr_K:
      case sat_plus_expr_K:
      case sat_minus_expr_K:
      {
         if(tree_helper::is_real(TM, ga->op1->index))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No because floating point operations");
            return FunctionFrontendFlowStep_Movable::UNMOVABLE;
         }
         if(conservative)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No");
            return FunctionFrontendFlowStep_Movable::UNMOVABLE;
         }
         else
         {
            auto* be = GetPointer<binary_expr>(right);
            auto n_bit = std::max(tree_helper::Size(be->op0), tree_helper::Size(be->op1));
            auto n_bit_min = std::min(tree_helper::Size(be->op0), tree_helper::Size(be->op1));
            bool is_constant =
                tree_helper::is_constant(TM, be->op0->index) || tree_helper::is_constant(TM, be->op1->index);
            if((n_bit > 9 && !is_constant && n_bit_min != 1) || n_bit > 16)
            {
               zero_delay = false;
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes");
            return FunctionFrontendFlowStep_Movable::MOVABLE;
         }
      }
      case ternary_plus_expr_K:
      case ternary_pm_expr_K:
      case ternary_mp_expr_K:
      case ternary_mm_expr_K:
      {
         if(conservative)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No");
            return FunctionFrontendFlowStep_Movable::UNMOVABLE;
         }
         else
         {
            auto* be = GetPointer<ternary_expr>(right);
            auto n_bit =
                std::max(std::max(tree_helper::Size(be->op0), tree_helper::Size(be->op1)), tree_helper::Size(be->op2));
            auto n_bit_min =
                std::min(std::min(tree_helper::Size(be->op0), tree_helper::Size(be->op1)), tree_helper::Size(be->op2));
            bool is_constant =
                tree_helper::is_constant(TM, be->op0->index) || tree_helper::is_constant(TM, be->op1->index);
            if((n_bit > 9 && !is_constant && n_bit_min != 1) || n_bit > 16)
            {
               zero_delay = false;
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes");
            return FunctionFrontendFlowStep_Movable::MOVABLE;
         }
      }
      case negate_expr_K:
      {
         auto* ne = GetPointer<negate_expr>(right);
         auto n_bit = tree_helper::Size(ne->op);
         bool is_constant = tree_helper::is_constant(TM, ne->op->index);
         if((n_bit > 9 && !is_constant) || n_bit > 16)
         {
            zero_delay = false;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes");
         return FunctionFrontendFlowStep_Movable::MOVABLE;
      }
      case float_expr_K:
      {
         zero_delay = false;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes");
         return FunctionFrontendFlowStep_Movable::MOVABLE;
      }
      case exact_div_expr_K:
      case trunc_div_expr_K:
      case trunc_mod_expr_K:
      {
         auto* be = GetPointer<binary_expr>(right);
         if(tree_helper::IsConstant(be->op1))
         {
            auto ic = GetPointer<integer_cst>(be->op1);
            if(ic)
            {
               const auto v = tree_helper::GetConstValue(be->op1);
               if(v)
               {
                  if(!(v && !(v & (v - 1))))
                  {
                     zero_delay = false;
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes");
                  return FunctionFrontendFlowStep_Movable::MOVABLE;
               }
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No because is a division by a non constant");
         return FunctionFrontendFlowStep_Movable::UNMOVABLE;
      }
      case abs_expr_K:
      {
         if(conservative)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No");
            return FunctionFrontendFlowStep_Movable::UNMOVABLE;
         }
         else
         {
            return FunctionFrontendFlowStep_Movable::MOVABLE;
         }
      }
      case catch_expr_K:
      case ceil_div_expr_K:
      case ceil_mod_expr_K:
      case complex_expr_K:
      case compound_expr_K:
      case eh_filter_expr_K:
      case fdesc_expr_K:
      case floor_div_expr_K:
      case floor_mod_expr_K:
      case goto_subroutine_K:
      case in_expr_K:
      case init_expr_K:
      case mem_ref_K:
      case modify_expr_K:
      case ordered_expr_K:
      case range_expr_K:
      case round_div_expr_K:
      case round_mod_expr_K:
      case set_le_expr_K:
      case try_catch_expr_K:
      case try_finally_K:
      case uneq_expr_K:
      case ltgt_expr_K:
      case unge_expr_K:
      case ungt_expr_K:
      case unle_expr_K:
      case unlt_expr_K:
      case unordered_expr_K:
      case widen_sum_expr_K:
      case with_size_expr_K:
      case vec_lshift_expr_K:
      case vec_rshift_expr_K:
      case widen_mult_hi_expr_K:
      case widen_mult_lo_expr_K:
      case vec_pack_trunc_expr_K:
      case vec_pack_sat_expr_K:
      case vec_pack_fix_trunc_expr_K:
      case vec_extracteven_expr_K:
      case vec_extractodd_expr_K:
      case vec_interleavehigh_expr_K:
      case vec_interleavelow_expr_K:
      case CASE_CPP_NODES:
      case CASE_CST_NODES:
      case CASE_DECL_NODES:
      case CASE_GIMPLE_NODES:
      case component_ref_K:
      case bit_field_ref_K:
      case vtable_ref_K:
      case with_cleanup_expr_K:
      case obj_type_ref_K:
      case save_expr_K:
      case vec_cond_expr_K:
      case vec_perm_expr_K:
      case dot_prod_expr_K:
      case call_expr_K:
      case aggr_init_expr_K:
      case CASE_FAKE_NODES:
      case rdiv_expr_K:
      case frem_expr_K:
      case case_label_expr_K:
      case target_mem_ref_K:
      case binfo_K:
      case block_K:
      case identifier_node_K:
      case CASE_PRAGMA_NODES:
      case statement_list_K:
      case tree_list_K:
      case tree_vec_K:
      case CASE_QUATERNARY_EXPRESSION:
      case CASE_TYPE_NODES:
      case alignof_expr_K:
      case arrow_expr_K:
      case buffer_ref_K:
      case card_expr_K:
      case cleanup_point_expr_K:
      case conj_expr_K:
      case exit_expr_K:
      case fix_ceil_expr_K:
      case fix_floor_expr_K:
      case fix_round_expr_K:
      case fix_trunc_expr_K:
      case imagpart_expr_K:
      case indirect_ref_K:
      case misaligned_indirect_ref_K:
      case loop_expr_K:
      case non_lvalue_expr_K:
      case realpart_expr_K:
      case reference_expr_K:
      case reinterpret_cast_expr_K:
      case sizeof_expr_K:
      case static_cast_expr_K:
      case throw_expr_K:
      case unsave_expr_K:
      case va_arg_expr_K:
      case reduc_max_expr_K:
      case reduc_min_expr_K:
      case reduc_plus_expr_K:
      case vec_unpack_hi_expr_K:
      case vec_unpack_lo_expr_K:
      case vec_unpack_float_hi_expr_K:
      case vec_unpack_float_lo_expr_K:
      case target_expr_K:
      case target_mem_ref461_K:
      case error_mark_K:
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "<--No because right part is " + right->get_kind_text());
         return FunctionFrontendFlowStep_Movable::UNMOVABLE;
      }
      case paren_expr_K:
      default:
      {
         THROW_UNREACHABLE("");
         return FunctionFrontendFlowStep_Movable::UNMOVABLE;
      }
   }
}

DesignFlowStep_Status simple_code_motion::InternalExec()
{
   const auto TM = AppM->get_tree_manager();
   bool modified = false;
   restart_ifmwi_opt = false;

   const auto fd = GetPointerS<const function_decl>(TM->GetTreeNode(function_id));
   const auto sl = GetPointerS<const statement_list>(fd->body);

   const auto isFunctionPipelined = AppM->CGetFunctionBehavior(function_id)->is_simple_pipeline();

   /// store the GCC BB graph ala boost::graph
   const auto bb_graph_info = BBGraphInfoRef(new BBGraphInfo(AppM, function_id));
   BBGraphsCollectionRef GCC_bb_graphs_collection(new BBGraphsCollection(bb_graph_info, parameters));
   BBGraphRef GCC_bb_graph(new BBGraph(GCC_bb_graphs_collection, CFG_SELECTOR));
   CustomUnorderedMap<vertex, unsigned int> direct_vertex_map;
   CustomUnorderedMap<unsigned int, vertex> inverse_vertex_map;
   /// add vertices
   const auto& list_of_bloc = sl->list_of_bloc;
   for(const auto& block : list_of_bloc)
   {
      inverse_vertex_map[block.first] =
          GCC_bb_graphs_collection->AddVertex(BBNodeInfoRef(new BBNodeInfo(block.second)));
      direct_vertex_map[inverse_vertex_map[block.first]] = block.first;
   }
   /// add edges
   for(const auto& bbi_bb : list_of_bloc)
   {
      const auto& bbi = bbi_bb.first;
      const auto& bb = bbi_bb.second;
      for(const auto& pred : bb->list_of_pred)
      {
         THROW_ASSERT(inverse_vertex_map.count(pred),
                      "BB" + STR(pred) + " (predecessor of " + STR(bbi) + ") does not exist");
         THROW_ASSERT(inverse_vertex_map.count(bbi), STR(bbi));
         GCC_bb_graphs_collection->AddEdge(inverse_vertex_map.at(pred), inverse_vertex_map.at(bbi), CFG_SELECTOR);
      }
      for(const auto& succ : bb->list_of_succ)
      {
         if(succ == bloc::EXIT_BLOCK_ID)
         {
            GCC_bb_graphs_collection->AddEdge(inverse_vertex_map.at(bbi), inverse_vertex_map.at(succ), CFG_SELECTOR);
         }
      }
      if(bb->list_of_succ.empty())
      {
         GCC_bb_graphs_collection->AddEdge(inverse_vertex_map.at(bbi), inverse_vertex_map.at(bloc::EXIT_BLOCK_ID),
                                           CFG_SELECTOR);
      }
   }
   bb_graph_info->entry_vertex = inverse_vertex_map.at(bloc::ENTRY_BLOCK_ID);
   bb_graph_info->exit_vertex = inverse_vertex_map.at(bloc::EXIT_BLOCK_ID);
   /// add a connection between entry and exit thus avoiding problems with non terminating code
   GCC_bb_graphs_collection->AddEdge(inverse_vertex_map.at(bloc::ENTRY_BLOCK_ID),
                                     inverse_vertex_map.at(bloc::EXIT_BLOCK_ID), CFG_SELECTOR);
   /// sort basic block vertices from the entry till the exit
   std::list<vertex> bb_sorted_vertices;
   cyclic_topological_sort(*GCC_bb_graph, std::front_inserter(bb_sorted_vertices));
   static size_t counter = 0;
   if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC &&
      (!parameters->IsParameter("print-dot-FF") || parameters->GetParameter<unsigned int>("print-dot-FF")))
   {
      GCC_bb_graph->WriteDot("BB_simple_code_motion_" + STR(counter) + ".dot");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---Written BB_simple_code_motion_" + STR(counter) + ".dot");
      counter++;
   }

   std::unique_ptr<dominance<BBGraph>> bb_dominators(
       new dominance<BBGraph>(*GCC_bb_graph, inverse_vertex_map.at(bloc::ENTRY_BLOCK_ID),
                              inverse_vertex_map.at(bloc::EXIT_BLOCK_ID), parameters));
   bb_dominators->calculate_dominance_info(dominance<BBGraph>::CDI_DOMINATORS);
   const auto& bb_dominator_map = bb_dominators->get_dominator_map();

   /// If we are performing simd transformation, look for simd pragma
   // cppcheck-suppress uninitvar
   const CustomSet<vertex> simd_loop_headers =
       parameters->getOption<int>(OPT_gcc_openmp_simd) == 0 ? CustomSet<vertex>() : [&]() -> CustomSet<vertex> const {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Looking for openmp simd pragma");
      CustomSet<vertex> return_value;
      for(const auto& block : list_of_bloc)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing BB" + STR(block.first));
         for(const auto& statement : block.second->CGetStmtList())
         {
            const auto* gp = GetPointer<const gimple_pragma>(statement);
            if(gp && GetPointer<const omp_pragma>(gp->scope) && GetPointer<const omp_simd_pragma>(gp->directive))
            {
               THROW_ASSERT(boost::out_degree(inverse_vertex_map[block.first], *GCC_bb_graph) == 1,
                            "OpenMP simd pragma block has more than one successor");
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Found openmp pragma");
               OutEdgeIterator oe, oe_end;
               boost::tie(oe, oe_end) = boost::out_edges(inverse_vertex_map[block.first], *GCC_bb_graph);
               return_value.insert(boost::target(*oe, *GCC_bb_graph));
               break;
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Found " + STR(return_value.size()) + " simd pragmas");
      return return_value;
   }();

   const tree_manipulationConstRef tree_man(new tree_manipulation(TM, parameters, AppM));

   for(const auto bb_vertex : bb_sorted_vertices)
   {
      const auto curr_bb = direct_vertex_map.at(bb_vertex);
      if(curr_bb == bloc::ENTRY_BLOCK_ID)
      {
         continue;
      }
      if(curr_bb == bloc::EXIT_BLOCK_ID)
      {
         continue;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing BB" + STR(curr_bb));
      bool restart_bb_code_motion = false;
      do
      {
         if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC &&
            (!parameters->IsParameter("print-dot-FF") || parameters->GetParameter<unsigned int>("print-dot-FF")))
         {
            GCC_bb_graph->WriteDot("BB_simple_code_motion_" + STR(counter) + ".dot");
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Written BB_simple_code_motion_" + STR(counter) + ".dot");
            counter++;
         }
         const auto& list_of_stmt = list_of_bloc.at(curr_bb)->CGetStmtList();
         std::list<tree_nodeRef> to_be_removed;
         CustomOrderedSet<unsigned int> zero_delay_stmts;
         std::list<tree_nodeRef> to_be_added_back;
         std::list<tree_nodeRef> to_be_added_front;
         /// We must use pointer since we are erasing elements in the list
         for(auto statement = list_of_stmt.begin(); statement != list_of_stmt.end(); statement++)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing " + (*statement)->ToString());
            /// skip gimple statements defining or using virtual operands
            tree_nodeRef tn = *statement;
            auto* gn = GetPointer<gimple_node>(tn);

            THROW_ASSERT(gn, "unexpected condition");
            if(!isFunctionPipelined && gn->vdef) /// load can be loop pipelined/predicated
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped because of memory store");
               continue;
            }

            /// only gimple_assign are considered for code motion
            if(!GetPointer<gimple_assign>(tn) && !GetPointer<gimple_nop>(tn))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped because " + tn->get_kind_text());
               continue;
            }

            /// compute the SSA variables used by stmt
            CustomOrderedSet<const ssa_name*> stmt_ssa_uses;
            tree_helper::compute_ssa_uses_rec_ptr(*statement, stmt_ssa_uses);
            for(const auto& vo : gn->vovers)
            {
               tree_helper::compute_ssa_uses_rec_ptr(vo, stmt_ssa_uses);
            }

            /// compute BB where the SSA variables are defined
            CustomOrderedSet<unsigned int> BB_def;
            /// check for anti-dependencies
            for(auto stmt0 = list_of_stmt.begin(); stmt0 != list_of_stmt.end() && *stmt0 != *statement && gn->vdef;
                stmt0++)
            {
               tree_nodeRef tn0 = *stmt0;
               const auto gn0 = GetPointerS<gimple_node>(tn0);
               if(gn0->vuses.find(gn->vdef) != gn0->vuses.end())
               {
                  BB_def.insert(curr_bb);
               }
            }

            const auto ssu_it_end = stmt_ssa_uses.cend();
            for(auto ssu_it = stmt_ssa_uses.cbegin(); ssu_it != ssu_it_end; ++ssu_it)
            {
               const auto sn = *ssu_it;
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---for variable " + sn->ToString());
               for(auto const& def_stmt : sn->CGetDefStmts())
               {
                  auto* def_gn = GetPointer<gimple_node>(def_stmt);
                  THROW_ASSERT(def_gn->get_kind() == gimple_nop_K or def_gn->index,
                               sn->ToString() + " is defined in entry");
                  THROW_ASSERT(def_gn->get_kind() == gimple_nop_K or
                                   (def_gn->get_kind() == gimple_assign_K &&
                                    GetPointer<const gimple_assign>(def_stmt)->clobber) or
                                   def_gn->bb_index or sn->virtual_flag,
                               "Definition " + def_gn->ToString() + " of " + sn->ToString() + " is in BB" +
                                   STR(def_gn->bb_index));
                  if(statement == list_of_stmt.begin() && list_of_bloc.at(curr_bb)->list_of_pred.size() == 1 &&
                     def_gn->bb_index == curr_bb && def_gn->get_kind() != gimple_phi_K)
                  {
                     /// allow to move first statements later overwritten in the same BB
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---  no constraint because is the first one");
                  }
                  else
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---  Adding BB" + STR(def_gn->bb_index) + " because of " + def_gn->ToString());
                     BB_def.insert(def_gn->bb_index);
                  }
               }
            }
            /// skip the statement in case it uses ssa variables defined in the current BB
            if(BB_def.find(curr_bb) != BB_def.end())
            {
               /// check if list of pred has a loop_id greater than the loop_id of curr_bb
               bool can_be_pipelined = list_of_bloc.at(curr_bb)->loop_id != 0;
               const auto Lop_it_end = list_of_bloc.at(curr_bb)->list_of_pred.end();
               for(auto Lop_it = list_of_bloc.at(curr_bb)->list_of_pred.begin();
                   Lop_it != Lop_it_end && can_be_pipelined; ++Lop_it)
               {
                  can_be_pipelined = list_of_bloc.at(curr_bb)->loop_id >= list_of_bloc.at(*Lop_it)->loop_id;
               }
               /// check if current statement can be loop pipelined
               bool zero_delay = true;
               if(can_be_pipelined && (gn->vuses.size() || CheckMovable(curr_bb, tn, zero_delay, TM) ==
                                                               FunctionFrontendFlowStep_Movable::MOVABLE))
               {
                  THROW_ASSERT(bb_dominator_map.find(bb_vertex) != bb_dominator_map.end(), "unexpected condition");
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "<--Skipped because uses ssa defined in the same block");
               continue;
            }
            if((gn->vuses.size() ||
                (GetPointer<gimple_assign>(tn) && (GetPointer<gimple_assign>(tn)->op1->get_kind() == call_expr_K ||
                                                   GetPointer<gimple_assign>(tn)->op1->get_kind() == mem_ref_K))) &&
               !isFunctionPipelined)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped because of vuses");
               continue; /// load cannot be code moved
            }
            /// find in which BB can be moved
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Checking where it can be moved");
            auto dest_bb_index = curr_bb;
            auto prev_dest_bb_index = curr_bb;
            if(gn->vdef || gn->vuses.size() ||
               (tn->get_kind() == gimple_assign_K && GetPointer<gimple_assign>(tn)->op1->get_kind() == mem_ref_K))
            {
               if(list_of_bloc.at(curr_bb)->list_of_pred.size() == 1 &&
                  list_of_bloc.at(curr_bb)->list_of_pred.front() != bloc::ENTRY_BLOCK_ID &&
                  ((tn->get_kind() == gimple_assign_K &&
                    (GetPointer<gimple_assign>(tn)->op0->get_kind() == mem_ref_K ||
                     GetPointer<gimple_assign>(tn)->op1->get_kind() == mem_ref_K)) ||
                   tn->get_kind() == gimple_nop_K) &&
                  list_of_bloc.at(list_of_bloc.at(curr_bb)->list_of_pred.front())->loop_id ==
                      list_of_bloc.at(curr_bb)->loop_id)
               {
                  dest_bb_index = list_of_bloc.at(curr_bb)->list_of_pred.front();
               }
               else
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "<--Skipped because is not a store/load or because we do not know the condition under "
                                 "which the store/load is done");
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
                  continue;
               }
            }
            else
            {
               vertex dom_bb = bb_vertex;
               if(bb_dominator_map.find(dom_bb) != bb_dominator_map.end())
               {
                  dom_bb = bb_dominator_map.find(dom_bb)->second;
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Considering its dominator BB" + STR(direct_vertex_map[dom_bb]));
                  auto dom_bb_index = direct_vertex_map[dom_bb];
                  while(dom_bb_index != bloc::ENTRY_BLOCK_ID)
                  {
                     unsigned loop_idU = list_of_bloc.at(dom_bb_index)->loop_id;
                     unsigned loop_idC = list_of_bloc.at(curr_bb)->loop_id;
                     if(loop_idC >= loop_idU)
                     {
                        prev_dest_bb_index = dest_bb_index;
                        dest_bb_index = dom_bb_index;
                        bool internLoopDep = false;
                        for(auto BBdef : BB_def)
                        {
                           if(list_of_bloc.at(BBdef)->loop_id > loop_idU && list_of_bloc.at(BBdef)->loop_id <= loop_idC)
                           {
                              internLoopDep = true;
                           }
                        }
                        if(internLoopDep)
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "---the statement depends on values defined at inner level than dest BB" +
                                              STR(dest_bb_index));
                           dest_bb_index = curr_bb;
                           break;
                        }
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---Going up one level. Considering now BB" + STR(dest_bb_index));
                     }
                     if(BB_def.find(dom_bb_index) != BB_def.end())
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---It contains the definition of one SSA used by the statement to be moved");
                        break;
                     }
                     if(bb_dominator_map.find(dom_bb) == bb_dominator_map.end())
                     {
                        break;
                     }
                     dom_bb = bb_dominator_map.find(dom_bb)->second;
                     dom_bb_index = direct_vertex_map[dom_bb];
                  }
               }
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");

            /// check the result of the dominator tree analysis
            if(dest_bb_index == curr_bb)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "<--Skipped (1) because destination would be the same bb");
               continue;
            }
            /// check if the controlling condition of curr_bb is constant
            bool is_controlling_condition_constant = true;
            const auto Lop_it_end = list_of_bloc.at(curr_bb)->list_of_pred.end();
            for(auto Lop_it = list_of_bloc.at(curr_bb)->list_of_pred.begin();
                Lop_it != Lop_it_end && is_controlling_condition_constant; ++Lop_it)
            {
               if(sl->list_of_bloc.at(*Lop_it)->CGetStmtList().empty())
               {
                  is_controlling_condition_constant = false;
               }
               else
               {
                  auto last_statement = sl->list_of_bloc.at(*Lop_it)->CGetStmtList().back();
                  if(last_statement->get_kind() == gimple_cond_K)
                  {
                     auto* gc = GetPointer<gimple_cond>(last_statement);
                     if(gc->op0->get_kind() != integer_cst_K)
                     {
                        is_controlling_condition_constant = false;
                     }
                  }
                  else if(last_statement->get_kind() == gimple_multi_way_if_K)
                  {
                     is_controlling_condition_constant = false;
                  }
                  else
                  {
                     is_controlling_condition_constant = false;
                  }
               }
            }
            bool zero_delay = true;
            auto check_movable = CheckMovable(dest_bb_index, tn, zero_delay, TM);
            if((check_movable == FunctionFrontendFlowStep_Movable::TIMING) && is_controlling_condition_constant)
            {
               check_movable = FunctionFrontendFlowStep_Movable::MOVABLE;
            }
            if(!isFunctionPipelined && check_movable == FunctionFrontendFlowStep_Movable::UNMOVABLE)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped because cannot be moved");
               continue;
            }
            if(!AppM->ApplyNewTransformation())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "<--Skipped because reached limit of CFG transformations");
               continue;
            }

            /// finally we found something of meaningful

            /// check if the current uses in dest_bb_index are due only to phis
            bool only_phis = true;
            for(const auto sn : stmt_ssa_uses)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Checking definition of " + sn->ToString());
               for(auto const& def_stmt : sn->CGetDefStmts())
               {
                  auto* def_gn = GetPointer<gimple_node>(def_stmt);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Checked definition " + def_gn->ToString());
                  if(def_gn->bb_index == dest_bb_index && def_gn->get_kind() != gimple_phi_K &&
                     zero_delay_stmts.find(def_stmt->index) == zero_delay_stmts.end())
                  {
                     only_phis = false;
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Not a phi");
                  }
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
            }
            if(only_phis && zero_delay)
            {
               zero_delay_stmts.insert((*statement)->index);
            }
            if(check_movable == FunctionFrontendFlowStep_Movable::TIMING or (!only_phis && !zero_delay))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Going down of one level because of non-zero delay");
               dest_bb_index = prev_dest_bb_index;
            }

            /// check if the statement can be really moved
            if(dest_bb_index == curr_bb)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "<--Skipped (2) because destination would be the same bb");
               continue;
            }
            modified = true;
            AppM->RegisterTransformation(GetName(), *statement);

            /// add predication in case is required
            if(tn->get_kind() == gimple_assign_K && (GetPointer<gimple_assign>(tn)->op0->get_kind() == mem_ref_K ||
                                                     GetPointer<gimple_assign>(tn)->op1->get_kind() == mem_ref_K))
            {
               if(list_of_bloc.at(dest_bb_index)->CGetStmtList().empty())
               {
                  /// it may happen: two consecutive BBs without conditional jump
               }
               else
               {
                  const auto& lastStmt = *(list_of_bloc.at(dest_bb_index)->CGetStmtList().rbegin());
                  auto lastStmtNode = lastStmt;
                  auto ga = GetPointer<gimple_assign>(tn);
                  if(lastStmtNode->get_kind() == gimple_cond_K)
                  {
                     if(ga->predicate && ga->predicate->get_kind() == integer_cst_K)
                     {
                        auto cond = tree_helper::GetConstValue(ga->predicate);
                        if(cond != 0)
                        {
                           if(list_of_bloc.at(dest_bb_index)->true_edge == curr_bb)
                           {
                              TM->ReplaceTreeNode(*statement, ga->predicate,
                                                  GetPointer<gimple_cond>(lastStmtNode)->op0);
                           }
                           else
                           {
                              /// create a negated condition
                              auto not_cond = tree_man->CreateNotExpr(GetPointer<gimple_cond>(lastStmtNode)->op0,
                                                                      list_of_bloc.at(dest_bb_index), function_id);
                              TM->ReplaceTreeNode(*statement, ga->predicate, not_cond);
                           }
                        }
                     }
                     else
                     {
                        if(list_of_bloc.at(dest_bb_index)->true_edge == curr_bb)
                        {
                           auto and_cond =
                               tree_man->CreateAndExpr(GetPointer<gimple_cond>(lastStmtNode)->op0, ga->predicate,
                                                       list_of_bloc.at(dest_bb_index), function_id);
                           TM->ReplaceTreeNode(*statement, ga->predicate, and_cond);
                        }
                        else
                        {
                           auto not_cond = tree_man->CreateNotExpr(GetPointer<gimple_cond>(lastStmtNode)->op0,
                                                                   list_of_bloc.at(dest_bb_index), function_id);
                           auto and_cond = tree_man->CreateAndExpr(not_cond, ga->predicate,
                                                                   list_of_bloc.at(dest_bb_index), function_id);
                           TM->ReplaceTreeNode(*statement, ga->predicate, and_cond);
                        }
                     }
                  }
                  else if(lastStmtNode->get_kind() == gimple_multi_way_if_K)
                  {
                     auto gmwi = GetPointer<gimple_multi_way_if>(lastStmtNode);
                     bool found_condition = false;
                     for(const auto& gmwicond : gmwi->list_of_cond)
                     {
                        if(gmwicond.second == curr_bb)
                        {
                           found_condition = true;
                           if(!gmwicond.first)
                           {
                              /// compute default condition
                              auto firstCond = true;
                              tree_nodeRef Cur;
                              for(const auto& gmwicond0 : gmwi->list_of_cond)
                              {
                                 if(gmwicond0.first)
                                 {
                                    if(firstCond)
                                    {
                                       Cur = gmwicond0.first;
                                       firstCond = false;
                                    }
                                    else
                                    {
                                       Cur = tree_man->CreateOrExpr(Cur, gmwicond0.first,
                                                                    list_of_bloc.at(dest_bb_index), function_id);
                                    }
                                 }
                              }
                              Cur = tree_man->CreateNotExpr(Cur, list_of_bloc.at(dest_bb_index), function_id);
                              if(ga->predicate && ga->predicate->get_kind() == integer_cst_K)
                              {
                                 const auto cond = tree_helper::GetConstValue(ga->predicate);
                                 if(cond != 0)
                                 {
                                    TM->ReplaceTreeNode(*statement, ga->predicate, Cur);
                                 }
                              }
                              else
                              {
                                 auto and_cond = tree_man->CreateAndExpr(Cur, ga->predicate,
                                                                         list_of_bloc.at(dest_bb_index), function_id);
                                 TM->ReplaceTreeNode(*statement, ga->predicate, and_cond);
                              }
                           }
                           else
                           {
                              if(ga->predicate && ga->predicate->get_kind() == integer_cst_K)
                              {
                                 const auto cond = tree_helper::GetConstValue(ga->predicate);
                                 if(cond != 0)
                                 {
                                    TM->ReplaceTreeNode(*statement, ga->predicate, gmwicond.first);
                                 }
                              }
                              else
                              {
                                 auto and_cond = tree_man->CreateAndExpr(gmwicond.first, ga->predicate,
                                                                         list_of_bloc.at(dest_bb_index), function_id);
                                 TM->ReplaceTreeNode(*statement, ga->predicate, and_cond);
                              }
                           }
                           break;
                        }
                     }
                     if(!found_condition)
                     {
                        THROW_ERROR("node not supported: " + lastStmtNode->get_kind_text() + " " +
                                    lastStmtNode->ToString());
                     }
                  }
                  else if(list_of_bloc.at(dest_bb_index)->list_of_succ.size() == 1 &&
                          list_of_bloc.at(dest_bb_index)->loop_id == list_of_bloc.at(curr_bb)->loop_id)
                  {
                     /// it may happen: two consecutive BBs without conditional jump
                  }
                  else
                  {
                     THROW_ERROR("node not supported: " + lastStmtNode->get_kind_text() + " " +
                                 lastStmtNode->ToString());
                  }
               }
            }
            const auto temp_statement = *statement;
            /// Going one step step forward to avoid invalidation of the pointer
            auto tmp_it = statement;
            ++tmp_it;
            /// Moving statement
            list_of_bloc.at(curr_bb)->RemoveStmt(temp_statement, AppM);
            if(list_of_bloc.at(curr_bb)->CGetStmtList().empty() && list_of_bloc.at(curr_bb)->CGetPhiList().empty())
            {
               restart_ifmwi_opt = true;
            }
            list_of_bloc.at(dest_bb_index)->PushBack(temp_statement, AppM);
            /// Going one step back since pointer is already increment in for loop
            --tmp_it;
            statement = tmp_it;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Moved in BB" + STR(dest_bb_index));
         }

         for(const auto& removing : to_be_removed)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Removing " + removing->ToString() + " from BB" + STR(curr_bb));
            list_of_bloc.at(curr_bb)->RemoveStmt(removing, AppM);
         }
         if(!to_be_removed.empty() && list_of_bloc.at(curr_bb)->CGetStmtList().empty() &&
            list_of_bloc.at(curr_bb)->CGetPhiList().empty())
         {
            restart_ifmwi_opt = true;
         }
         for(const auto& adding_back : to_be_added_back)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Adding back " + adding_back->ToString() + " from BB" + STR(curr_bb));
            list_of_bloc.at(curr_bb)->PushBack(adding_back, AppM);
         }
         for(const auto& adding_front : to_be_added_front)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Adding front " + adding_front->ToString() + " from BB" + STR(curr_bb));
            list_of_bloc.at(curr_bb)->PushFront(adding_front, AppM);
         }
         restart_bb_code_motion = (!to_be_added_back.empty()) or (!to_be_added_front.empty());
         if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC &&
            (!parameters->IsParameter("print-dot-FF") || parameters->GetParameter<unsigned int>("print-dot-FF")))
         {
            GCC_bb_graph->WriteDot("BB_simple_code_motion_" + STR(counter) + ".dot");
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Written BB_simple_code_motion_" + STR(counter) + ".dot");
            counter++;
         }
         if(restart_bb_code_motion)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Restart Analyzing BB" + STR(curr_bb));
         }
      } while(restart_bb_code_motion);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed BB" + STR(curr_bb));
   }

   modified ? function_behavior->UpdateBBVersion() : 0;
   return modified ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}

bool simple_code_motion::IsScheduleBased() const
{
   if(schedule)
   {
      return true;
   }
   else
   {
      return false;
   }
}
