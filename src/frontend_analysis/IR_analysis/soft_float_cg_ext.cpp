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
 * @file soft_float_cg_ext.cpp
 * @brief Step that extends the call graph with the soft-float calls where appropriate.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

/// Header include
#include "soft_float_cg_ext.hpp"

/// design_flows include
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "design_flow_step.hpp"

/// frontend_analysis
#include "symbolic_application_frontend_flow_step.hpp"

/// Behavior include
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"

/// Graph include
#include "basic_block.hpp"
#include "basic_blocks_graph_constructor.hpp"

/// Parameter include
#include "Parameter.hpp"

/// STD include
#include <fstream>

/// STL include
#include "custom_map.hpp"
#include <string>

/// Tree include
#include "ext_tree_node.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

/// Utility include
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

soft_float_cg_ext::soft_float_cg_ext(const ParameterConstRef _parameters, const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, SOFT_FLOAT_CG_EXT, _design_flow_manager, _parameters), TreeM(_AppM->get_tree_manager()), tree_man(new tree_manipulation(TreeM, parameters)), modified(false)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

soft_float_cg_ext::~soft_float_cg_ext() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> soft_float_cg_ext::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(UN_COMPARISON_LOWERING, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(EXTRACT_GIMPLE_COND_OP, SAME_FUNCTION));
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

DesignFlowStep_Status soft_float_cg_ext::InternalExec()
{
   const tree_nodeRef curr_tn = TreeM->GetTreeNode(function_id);
   tree_nodeRef Scpe = TreeM->GetTreeReindex(function_id);
   auto* fd = GetPointer<function_decl>(curr_tn);
   auto* sl = GetPointer<statement_list>(GET_NODE(fd->body));
   modified = false;

   for(const auto& block : sl->list_of_bloc)
   {
      for(const auto& stmt : block.second->CGetStmtList())
      {
         RecursiveExaminate(stmt, stmt);
      }
   }
   modified ? function_behavior->UpdateBBVersion() : 0;
   return modified ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}

void soft_float_cg_ext::RecursiveExaminate(const tree_nodeRef current_statement, const tree_nodeRef current_tree_node)
{
   THROW_ASSERT(current_tree_node->get_kind() == tree_reindex_K, "Node is not a tree reindex");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Update recursively (" + STR(current_tree_node->index) + ") " + STR(current_tree_node));
   const tree_nodeRef curr_tn = GET_NODE(current_tree_node);
   const std::string current_srcp = [curr_tn]() -> std::string {
      const auto srcp_tn = GetPointer<const srcp>(curr_tn);
      if(srcp_tn)
      {
         return srcp_tn->include_name + ":" + STR(srcp_tn->line_number) + ":" + STR(srcp_tn->column_number);
      }
      return "";
   }();
   switch(curr_tn->get_kind())
   {
      case call_expr_K:
      case aggr_init_expr_K:
      {
         const call_expr* ce = GetPointer<call_expr>(curr_tn);
         const std::vector<tree_nodeRef>& args = ce->args;
         std::vector<tree_nodeRef>::const_iterator arg, arg_end = args.end();
         unsigned int parm_index = 0;
         for(arg = args.begin(); arg != arg_end; ++arg)
         {
            RecursiveExaminate(current_statement, *arg);
            ++parm_index;
         }
         break;
      }
      case gimple_call_K:
      {
         const gimple_call* ce = GetPointer<gimple_call>(curr_tn);
         const std::vector<tree_nodeRef>& args = ce->args;
         std::vector<tree_nodeRef>::const_iterator arg, arg_end = args.end();
         unsigned int parm_index = 0;
         for(arg = args.begin(); arg != arg_end; ++arg)
         {
            RecursiveExaminate(current_statement, *arg);
            ++parm_index;
         }
         break;
      }
      case gimple_assign_K:
      {
         auto* gm = GetPointer<gimple_assign>(curr_tn);
         RecursiveExaminate(current_statement, gm->op0);
         RecursiveExaminate(current_statement, gm->op1);
         if(gm->predicate)
         {
            RecursiveExaminate(current_statement, gm->predicate);
         }
         break;
      }
      case gimple_nop_K:
      {
         break;
      }
      case var_decl_K:
      case parm_decl_K:
      case ssa_name_K:
      {
         break;
      }
      case tree_list_K:
      {
         tree_nodeRef current = current_tree_node;
         while(current)
         {
            RecursiveExaminate(current_statement, GetPointer<tree_list>(GET_NODE(current))->valu);
            current = GetPointer<tree_list>(GET_NODE(current))->chan;
         }
         break;
      }
      case CASE_UNARY_EXPRESSION:
      {
         const unary_expr* ue = GetPointer<unary_expr>(curr_tn);
         RecursiveExaminate(current_statement, ue->op);
         tree_nodeRef expr_type = GET_NODE(ue->type);
         unsigned int op_expr_type_index;
         tree_nodeRef op_expr_type = tree_helper::get_type_node(GET_NODE(ue->op), op_expr_type_index);
         if(expr_type->get_kind() == real_type_K)
         {
            switch(curr_tn->get_kind())
            {
               case float_expr_K:
               {
                  unsigned int bitsize_in = tree_helper::size(TreeM, op_expr_type_index);
                  if(bitsize_in < 32)
                     bitsize_in = 32;
                  else if(bitsize_in > 32 && bitsize_in < 64)
                     bitsize_in = 64;
                  unsigned int bitsize_out = tree_helper::size(TreeM, GET_INDEX_NODE(ue->type));
                  if(bitsize_in < 32)
                     bitsize_in = 32;
                  else if(bitsize_in > 32 && bitsize_in < 64)
                     bitsize_in = 64;
                  std::string bitsize_str_in = bitsize_in == 96 ? "x80" : STR(bitsize_in);
                  std::string bitsize_str_out = bitsize_out == 96 ? "x80" : STR(bitsize_out);
                  std::string fu_name;
                  if(op_expr_type->get_kind() != real_type_K)
                  {
                     if(tree_helper::is_unsigned(TreeM, op_expr_type_index))
                        fu_name = "__uint" + bitsize_str_in + "_to_float" + bitsize_str_out + "if";
                     else
                        fu_name = "__int" + bitsize_str_in + "_to_float" + bitsize_str_out + "if";
                     unsigned int called_function_id = TreeM->function_index(fu_name);
                     std::vector<tree_nodeRef> args;
                     args.push_back(ue->op);
                     modified = true;
                     TreeM->ReplaceTreeNode(current_statement, current_tree_node, tree_man->CreateCallExpr(TreeM->GetTreeReindex(called_function_id), args, current_srcp));
                     THROW_ASSERT(called_function_id, "The library miss this function " + fu_name);
                     THROW_ASSERT(AppM->GetFunctionBehavior(called_function_id)->GetBehavioralHelper()->has_implementation(), "inconsistent behavioral helper");
                     AppM->GetCallGraphManager()->AddCallPoint(function_id, called_function_id, current_statement->index, FunctionEdgeInfo::CallType::direct_call);
                  }
                  break;
               }
               case nop_expr_K:
               case abs_expr_K:
               case negate_expr_K:
               case view_convert_expr_K:
               case indirect_ref_K:
               case imagpart_expr_K:
               case realpart_expr_K:
               case paren_expr_K:
                  break;
               case addr_expr_K:
               case arrow_expr_K:
               case bit_not_expr_K:
               case buffer_ref_K:
               case card_expr_K:
               case cleanup_point_expr_K:
               case conj_expr_K:
               case convert_expr_K:
               case exit_expr_K:
               case fix_ceil_expr_K:
               case fix_floor_expr_K:
               case fix_round_expr_K:
               case fix_trunc_expr_K:
               case misaligned_indirect_ref_K:
               case loop_expr_K:
               case non_lvalue_expr_K:
               case reference_expr_K:
               case reinterpret_cast_expr_K:
               case sizeof_expr_K:
               case static_cast_expr_K:
               case throw_expr_K:
               case truth_not_expr_K:
               case unsave_expr_K:
               case va_arg_expr_K:
               case reduc_max_expr_K:
               case reduc_min_expr_K:
               case reduc_plus_expr_K:
               case vec_unpack_hi_expr_K:
               case vec_unpack_lo_expr_K:
               case vec_unpack_float_hi_expr_K:
               case vec_unpack_float_lo_expr_K:
               case binfo_K:
               case block_K:
               case call_expr_K:
               case aggr_init_expr_K:
               case case_label_expr_K:
               case constructor_K:
               case identifier_node_K:
               case ssa_name_K:
               case statement_list_K:
               case target_expr_K:
               case target_mem_ref_K:
               case target_mem_ref461_K:
               case tree_list_K:
               case tree_vec_K:
               case error_mark_K:
               case lut_expr_K:
               case CASE_BINARY_EXPRESSION:
               case CASE_CPP_NODES:
               case CASE_CST_NODES:
               case CASE_DECL_NODES:
               case CASE_FAKE_NODES:
               case CASE_GIMPLE_NODES:
               case CASE_PRAGMA_NODES:
               case CASE_QUATERNARY_EXPRESSION:
               case CASE_TERNARY_EXPRESSION:
               case CASE_TYPE_NODES:
               {
                  THROW_ERROR("not yet supported soft float function: " + curr_tn->get_kind_text());
                  break;
               }
               default:
               {
                  THROW_UNREACHABLE("");
               }
            }
         }
         if(op_expr_type->get_kind() == real_type_K)
         {
            switch(curr_tn->get_kind())
            {
               case fix_trunc_expr_K:
               {
                  unsigned int bitsize_in = tree_helper::size(TreeM, op_expr_type_index);
                  unsigned int bitsize_out = tree_helper::size(TreeM, GET_INDEX_NODE(ue->type));
                  if(bitsize_out < 32)
                     bitsize_out = 32;
                  else if(bitsize_out > 32 && bitsize_out < 64)
                     bitsize_out = 64;
                  bool is_unsigned = tree_helper::is_unsigned(TreeM, GET_INDEX_NODE(ue->type));
                  std::string bitsize_str_in = bitsize_in == 96 ? "x80" : STR(bitsize_in);
                  std::string bitsize_str_out = bitsize_out == 96 ? "x80" : STR(bitsize_out);
                  std::string fu_name = "__float" + bitsize_str_in + "_to_" + (is_unsigned ? "u" : "") + "int" + bitsize_str_out + "_round_to_zeroif";
                  unsigned int called_function_id = TreeM->function_index(fu_name);
                  THROW_ASSERT(called_function_id, "The library miss this function " + fu_name);
                  THROW_ASSERT(AppM->GetFunctionBehavior(called_function_id)->GetBehavioralHelper()->has_implementation(), "inconsistent behavioral helper");
                  std::vector<tree_nodeRef> args;
                  args.push_back(ue->op);
                  modified = true;
                  TreeM->ReplaceTreeNode(current_statement, current_tree_node, tree_man->CreateCallExpr(TreeM->GetTreeReindex(called_function_id), args, current_srcp));
                  AppM->GetCallGraphManager()->AddCallPoint(function_id, called_function_id, current_statement->index, FunctionEdgeInfo::CallType::direct_call);
                  break;
               }
               case binfo_K:
               case block_K:
               case call_expr_K:
               case aggr_init_expr_K:
               case case_label_expr_K:
               case constructor_K:
               case identifier_node_K:
               case ssa_name_K:
               case statement_list_K:
               case target_expr_K:
               case target_mem_ref_K:
               case target_mem_ref461_K:
               case tree_list_K:
               case tree_vec_K:
               case lut_expr_K:
               case CASE_BINARY_EXPRESSION:
               case CASE_CPP_NODES:
               case CASE_CST_NODES:
               case CASE_DECL_NODES:
               case CASE_FAKE_NODES:
               case CASE_GIMPLE_NODES:
               case CASE_PRAGMA_NODES:
               case CASE_QUATERNARY_EXPRESSION:
               case CASE_TERNARY_EXPRESSION:
               case CASE_TYPE_NODES:
               case abs_expr_K:
               case addr_expr_K:
               case arrow_expr_K:
               case bit_not_expr_K:
               case buffer_ref_K:
               case card_expr_K:
               case cleanup_point_expr_K:
               case conj_expr_K:
               case convert_expr_K:
               case exit_expr_K:
               case fix_ceil_expr_K:
               case fix_floor_expr_K:
               case fix_round_expr_K:
               case float_expr_K:
               case imagpart_expr_K:
               case indirect_ref_K:
               case misaligned_indirect_ref_K:
               case loop_expr_K:
               case negate_expr_K:
               case non_lvalue_expr_K:
               case nop_expr_K:
               case paren_expr_K:
               case realpart_expr_K:
               case reference_expr_K:
               case reinterpret_cast_expr_K:
               case sizeof_expr_K:
               case static_cast_expr_K:
               case throw_expr_K:
               case truth_not_expr_K:
               case unsave_expr_K:
               case va_arg_expr_K:
               case view_convert_expr_K:
               case reduc_max_expr_K:
               case reduc_min_expr_K:
               case reduc_plus_expr_K:
               case vec_unpack_hi_expr_K:
               case vec_unpack_lo_expr_K:
               case vec_unpack_float_hi_expr_K:
               case vec_unpack_float_lo_expr_K:
               case error_mark_K:
                  break;
               default:
               {
                  THROW_UNREACHABLE("");
               }
            }
         }
         break;
      }
      case CASE_BINARY_EXPRESSION:
      {
         const binary_expr* be = GetPointer<binary_expr>(curr_tn);
         RecursiveExaminate(current_statement, be->op0);
         RecursiveExaminate(current_statement, be->op1);
         unsigned int expr_type_index;
         tree_nodeRef expr_type = tree_helper::get_type_node(GET_NODE(be->op0), expr_type_index);
         if(expr_type->get_kind() == real_type_K)
         {
            bool add_call = true;
            std::string fu_suffix;
            switch(curr_tn->get_kind())
            {
               case mult_expr_K:
               {
                  fu_suffix = "mul";
                  break;
               }
               case plus_expr_K:
               {
                  fu_suffix = "add";
                  break;
               }
               case minus_expr_K:
               {
                  fu_suffix = "sub";
                  break;
               }
               case rdiv_expr_K:
               {
                  fu_suffix = "div";
                  unsigned int bitsize = tree_helper::size(TreeM, expr_type_index);
                  if(bitsize == 32 || bitsize == 64)
                  {
                     THROW_ASSERT(parameters->isOption(OPT_hls_fpdiv), "a default is expected");
                     if(parameters->getOption<std::string>(OPT_hls_fpdiv) == "SRT4")
                        fu_suffix = fu_suffix + "SRT4";
                     else if(parameters->getOption<std::string>(OPT_hls_fpdiv) == "G")
                        fu_suffix = fu_suffix + "G";
                     else if(parameters->getOption<std::string>(OPT_hls_fpdiv) == "SF")
                        ; // do nothing
                     else
                        THROW_ERROR("FP-Division algorithm not supported:" + parameters->getOption<std::string>(OPT_hls_fpdiv));
                  }
                  break;
               }
               case gt_expr_K:
               {
                  fu_suffix = "gt";
                  break;
               }
               case ge_expr_K:
               {
                  fu_suffix = "ge";
                  break;
               }
               case lt_expr_K:
               {
                  fu_suffix = "lt";
                  break;
               }
               case le_expr_K:
               {
                  fu_suffix = "le";
                  break;
               }
               case ltgt_expr_K:
               {
                  fu_suffix = "ltgt_quiet";
                  break;
               }
               case uneq_expr_K:
               case unge_expr_K:
               case ungt_expr_K:
               case unle_expr_K:
               case unlt_expr_K:
               {
                  THROW_ERROR("Unsupported tree node " + curr_tn->get_kind_text());
                  break;
               }
               case assert_expr_K:
               case bit_and_expr_K:
               case bit_ior_expr_K:
               case bit_xor_expr_K:
               case catch_expr_K:
               case ceil_div_expr_K:
               case ceil_mod_expr_K:
               case complex_expr_K:
               case compound_expr_K:
               case eh_filter_expr_K:
               case eq_expr_K:
               case exact_div_expr_K:
               case fdesc_expr_K:
               case floor_div_expr_K:
               case floor_mod_expr_K:
               case goto_subroutine_K:
               case in_expr_K:
               case init_expr_K:
               case lrotate_expr_K:
               case lshift_expr_K:
               case lut_expr_K:
               case max_expr_K:
               case mem_ref_K:
               case min_expr_K:
               case modify_expr_K:
               case mult_highpart_expr_K:
               case ne_expr_K:
               case ordered_expr_K:
               case pointer_plus_expr_K:
               case postdecrement_expr_K:
               case postincrement_expr_K:
               case predecrement_expr_K:
               case preincrement_expr_K:
               case range_expr_K:
               case round_div_expr_K:
               case round_mod_expr_K:
               case rrotate_expr_K:
               case rshift_expr_K:
               case set_le_expr_K:
               case trunc_div_expr_K:
               case trunc_mod_expr_K:
               case truth_and_expr_K:
               case truth_andif_expr_K:
               case truth_or_expr_K:
               case truth_orif_expr_K:
               case truth_xor_expr_K:
               case try_catch_expr_K:
               case try_finally_K:
               case unordered_expr_K:
               case widen_sum_expr_K:
               case widen_mult_expr_K:
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
               case extract_bit_expr_K:
               case sat_plus_expr_K:
               case sat_minus_expr_K:
               {
                  add_call = false;
                  break;
               }
               case binfo_K:
               case block_K:
               case call_expr_K:
               case aggr_init_expr_K:
               case case_label_expr_K:
               case constructor_K:
               case identifier_node_K:
               case ssa_name_K:
               case statement_list_K:
               case target_expr_K:
               case target_mem_ref_K:
               case target_mem_ref461_K:
               case tree_list_K:
               case tree_vec_K:
               case error_mark_K:
               case CASE_CPP_NODES:
               case CASE_CST_NODES:
               case CASE_DECL_NODES:
               case CASE_FAKE_NODES:
               case CASE_GIMPLE_NODES:
               case CASE_PRAGMA_NODES:
               case CASE_QUATERNARY_EXPRESSION:
               case CASE_TERNARY_EXPRESSION:
               case CASE_TYPE_NODES:
               case CASE_UNARY_EXPRESSION:
               {
                  break;
               }
               default:
               {
                  THROW_UNREACHABLE("");
               }
            }
            if(add_call)
            {
               unsigned int bitsize = tree_helper::size(TreeM, expr_type_index);
               std::string bitsize_str = bitsize == 96 ? "x80" : STR(bitsize);
               std::string fu_name = "__float" + bitsize_str + "_" + fu_suffix + "if";
               unsigned int called_function_id = TreeM->function_index(fu_name);
               THROW_ASSERT(called_function_id, "The library miss this function " + fu_name);
               THROW_ASSERT(AppM->GetFunctionBehavior(called_function_id)->GetBehavioralHelper()->has_implementation(), "inconsistent behavioral helper");
               std::vector<tree_nodeRef> args;
               args.push_back(be->op0);
               args.push_back(be->op1);
               modified = true;
               TreeM->ReplaceTreeNode(current_statement, current_tree_node, tree_man->CreateCallExpr(TreeM->GetTreeReindex(called_function_id), args, current_srcp));
               AppM->GetCallGraphManager()->AddCallPoint(function_id, called_function_id, current_statement->index, FunctionEdgeInfo::CallType::direct_call);
            }
         }
         break;
      }
      case CASE_TERNARY_EXPRESSION:
      {
         const ternary_expr* te = GetPointer<ternary_expr>(curr_tn);
         RecursiveExaminate(current_statement, te->op0);
         if(te->op1)
            RecursiveExaminate(current_statement, te->op1);
         if(te->op2)
            RecursiveExaminate(current_statement, te->op2);
         break;
      }
      case CASE_QUATERNARY_EXPRESSION:
      {
         const quaternary_expr* qe = GetPointer<quaternary_expr>(curr_tn);
         RecursiveExaminate(current_statement, qe->op0);
         if(qe->op1)
            RecursiveExaminate(current_statement, qe->op1);
         if(qe->op2)
            RecursiveExaminate(current_statement, qe->op2);
         if(qe->op3)
            RecursiveExaminate(current_statement, qe->op3);
         break;
      }
      case lut_expr_K:
      {
         auto* le = GetPointer<lut_expr>(curr_tn);
         RecursiveExaminate(current_statement, le->op0);
         RecursiveExaminate(current_statement, le->op1);
         if(le->op2)
            RecursiveExaminate(current_statement, le->op2);
         if(le->op3)
            RecursiveExaminate(current_statement, le->op3);
         if(le->op4)
            RecursiveExaminate(current_statement, le->op4);
         if(le->op5)
            RecursiveExaminate(current_statement, le->op5);
         if(le->op6)
            RecursiveExaminate(current_statement, le->op6);
         if(le->op7)
            RecursiveExaminate(current_statement, le->op7);
         if(le->op8)
            RecursiveExaminate(current_statement, le->op8);
         break;
      }
      case constructor_K:
      {
         const constructor* co = GetPointer<constructor>(curr_tn);
         const std::vector<std::pair<tree_nodeRef, tree_nodeRef>>& list_of_idx_valu = co->list_of_idx_valu;
         std::vector<std::pair<tree_nodeRef, tree_nodeRef>>::const_iterator it, it_end = list_of_idx_valu.end();
         for(it = list_of_idx_valu.begin(); it != it_end; ++it)
         {
            RecursiveExaminate(current_statement, it->second);
         }
         break;
      }
      case gimple_cond_K:
      {
         const gimple_cond* gc = GetPointer<gimple_cond>(curr_tn);
         RecursiveExaminate(current_statement, gc->op0);
         break;
      }
      case gimple_switch_K:
      {
         const gimple_switch* se = GetPointer<gimple_switch>(curr_tn);
         RecursiveExaminate(current_statement, se->op0);
         break;
      }
      case gimple_multi_way_if_K:
      {
         auto* gmwi = GetPointer<gimple_multi_way_if>(curr_tn);
         for(const auto& cond : gmwi->list_of_cond)
            if(cond.first)
               RecursiveExaminate(current_statement, cond.first);
         break;
      }
      case gimple_return_K:
      {
         const gimple_return* re = GetPointer<gimple_return>(curr_tn);
         if(re->op)
            RecursiveExaminate(current_statement, re->op);
         break;
      }
      case gimple_for_K:
      {
         const auto* gf = GetPointer<const gimple_for>(curr_tn);
         RecursiveExaminate(current_statement, gf->op0);
         RecursiveExaminate(current_statement, gf->op1);
         RecursiveExaminate(current_statement, gf->op2);
         break;
      }
      case gimple_while_K:
      {
         const gimple_while* gw = GetPointer<gimple_while>(curr_tn);
         RecursiveExaminate(current_statement, gw->op0);
         break;
      }
      case CASE_TYPE_NODES:
      case type_decl_K:
      {
         break;
      }
      case target_mem_ref_K:
      {
         const target_mem_ref* tmr = GetPointer<target_mem_ref>(curr_tn);
         if(tmr->symbol)
            RecursiveExaminate(current_statement, tmr->symbol);
         if(tmr->base)
            RecursiveExaminate(current_statement, tmr->base);
         if(tmr->idx)
            RecursiveExaminate(current_statement, tmr->idx);
         break;
      }
      case target_mem_ref461_K:
      {
         const target_mem_ref461* tmr = GetPointer<target_mem_ref461>(curr_tn);
         if(tmr->base)
            RecursiveExaminate(current_statement, tmr->base);
         if(tmr->idx)
            RecursiveExaminate(current_statement, tmr->idx);
         if(tmr->idx2)
            RecursiveExaminate(current_statement, tmr->idx2);
         break;
      }
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
      case gimple_pragma_K:
      case CASE_PRAGMA_NODES:
         break;
      case binfo_K:
      case block_K:
      case const_decl_K:
      case CASE_CPP_NODES:
      case gimple_bind_K:
      case gimple_phi_K:
      case gimple_predict_K:
      case gimple_resx_K:
      case identifier_node_K:
      case last_tree_K:
      case namespace_decl_K:
      case none_K:
      case placeholder_expr_K:
      case statement_list_K:
      case translation_unit_decl_K:
      case using_decl_K:
      case template_decl_K:
      case tree_reindex_K:
      case target_expr_K:
      case error_mark_K:
      {
         THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC, "Not supported node: " + std::string(curr_tn->get_kind_text()));
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Updated recursively (" + STR(current_tree_node->index) + ") " + STR(current_tree_node));
   return;
}
