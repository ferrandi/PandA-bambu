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
 * @file soft_int_cg_ext.cpp
 * @brief Step that extends the call graph with software implementation of integer operators.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

/// Header include
#include "soft_int_cg_ext.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "behavioral_helper.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "dbgPrintHelper.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "design_flow_step.hpp"
#include "exceptions.hpp"
#include "function_behavior.hpp"
#include "function_frontend_flow_step.hpp"
#include "hls_device.hpp"
#include "hls_manager.hpp"
#include "math_function.hpp"
#include "string_manipulation.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

#include <string>

soft_int_cg_ext::soft_int_cg_ext(const application_managerRef AM, const DesignFlowManagerConstRef dfm,
                                 const ParameterConstRef par)
    : ApplicationFrontendFlowStep(AM, SOFT_INT_CG_EXT, dfm, par),
      TreeM(AM->get_tree_manager()),
      use64bitMul(false),
      use32bitMul(false),
      doSoftDiv(par->isOption(OPT_hls_div) && par->getOption<std::string>(OPT_hls_div) != "none")
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

soft_int_cg_ext::~soft_int_cg_ext() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
soft_int_cg_ext::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         relationships.insert(std::make_pair(COMPUTE_IMPLICIT_CALLS, ALL_FUNCTIONS));
         relationships.insert(std::make_pair(FIX_STRUCTS_PASSED_BY_VALUE, ALL_FUNCTIONS));
         relationships.insert(std::make_pair(FUNCTION_CALL_TYPE_CLEANUP, ALL_FUNCTIONS));
         relationships.insert(std::make_pair(IR_LOWERING, ALL_FUNCTIONS));
         relationships.insert(std::make_pair(USE_COUNTING, ALL_FUNCTIONS));
         relationships.insert(std::make_pair(CSE_STEP, ALL_FUNCTIONS));
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(DEAD_CODE_ELIMINATION_IPA, WHOLE_APPLICATION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
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

void soft_int_cg_ext::ComputeRelationships(DesignFlowStepSet& relationships,
                                           const DesignFlowStep::RelationshipType relationship_type)
{
   if(relationship_type == INVALIDATION_RELATIONSHIP)
   {
      for(const auto& i : fun_id_to_restart)
      {
         const auto step_signature =
             FunctionFrontendFlowStep::ComputeSignature(FrontendFlowStepType::FUNCTION_CALL_TYPE_CLEANUP, i);
         const auto frontend_step = design_flow_manager.lock()->GetDesignFlowStep(step_signature);
         THROW_ASSERT(frontend_step != NULL_VERTEX, "step " + step_signature + " is not present");
         const auto design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
         const auto design_flow_step = design_flow_graph->CGetDesignFlowStepInfo(frontend_step)->design_flow_step;
         relationships.insert(design_flow_step);
      }
      fun_id_to_restart.clear();
   }
   ApplicationFrontendFlowStep::ComputeRelationships(relationships, relationship_type);
}

DesignFlowStep_Status soft_int_cg_ext::Exec()
{
   const tree_manipulationRef tree_man(new tree_manipulation(TreeM, parameters, AppM));

   const auto CGMan = AppM->CGetCallGraphManager();
   const auto cg = CGMan->CGetCallGraph();
   const auto reached_body_fun_ids = CGMan->GetReachedBodyFunctions();

   for(const auto& function_id : reached_body_fun_ids)
   {
      const auto fu_name = AppM->CGetFunctionBehavior(function_id)->CGetBehavioralHelper()->get_function_name();
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                     "-->Analyzing function \"" + fu_name + "\": id = " + STR(function_id));
      const auto curr_tn = TreeM->GetTreeNode(function_id);
      const auto fname = tree_helper::GetFunctionName(TreeM, curr_tn);
      const auto fd = GetPointerS<function_decl>(curr_tn);
      const auto sl = GetPointerS<statement_list>(GET_NODE(fd->body));

      THROW_ASSERT(GetPointer<const HLS_manager>(AppM)->get_HLS_device(), "unexpected condition");
      const auto hls_d = GetPointer<const HLS_manager>(AppM)->get_HLS_device();
      if(fname != "__umul64" && fname != "__mul64")
      {
         if((hls_d->has_parameter("use_soft_64_mul") && hls_d->get_parameter<size_t>("use_soft_64_mul")) ||
            (parameters->isOption(OPT_DSP_fracturing) &&
             parameters->getOption<std::string>(OPT_DSP_fracturing) == "16") ||
            (parameters->isOption(OPT_DSP_fracturing) &&
             parameters->getOption<std::string>(OPT_DSP_fracturing) == "32"))
         {
            use64bitMul = true;
         }
      }
      if(fname != "__umul32" && fname != "__mul32")
      {
         if((hls_d->has_parameter("use_soft_32_mul") && hls_d->get_parameter<size_t>("use_soft_32_mul")) ||
            (parameters->isOption(OPT_DSP_fracturing) &&
             parameters->getOption<std::string>(OPT_DSP_fracturing) == "16"))
         {
            use32bitMul = true;
         }
      }

      bool modified = false;
      for(const auto& idx_bb : sl->list_of_bloc)
      {
         const auto& BB = idx_bb.second;
         for(const auto& stmt : BB->CGetStmtList())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "-->Examine " + STR(GET_INDEX_NODE(stmt)) + " " + GET_NODE(stmt)->ToString());
            modified |= recursive_transform(function_id, stmt, stmt, tree_man);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "<--Examined " + STR(GET_INDEX_NODE(stmt)) + " " + GET_NODE(stmt)->ToString());
         }
      }

      if(modified)
      {
         fun_id_to_restart.insert(function_id);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                     "<--Analyzed function \"" + fu_name + "\": id = " + STR(function_id));
   }
   for(const auto& i : fun_id_to_restart)
   {
      const auto FB = AppM->GetFunctionBehavior(i);
      FB->GetBBVersion();
   }
   return fun_id_to_restart.empty() ? DesignFlowStep_Status::UNCHANGED : DesignFlowStep_Status::SUCCESS;
}

bool soft_int_cg_ext::recursive_transform(unsigned int function_id, const tree_nodeRef& current_tree_node,
                                          const tree_nodeRef& current_statement, const tree_manipulationRef tree_man)
{
   THROW_ASSERT(current_tree_node->get_kind() == tree_reindex_K, "Node is not a tree reindex");
   bool modified = false;
   const tree_nodeRef curr_tn = GET_NODE(current_tree_node);
   const auto get_current_srcp = [curr_tn]() -> std::string {
      const auto srcp_tn = GetPointer<const srcp>(curr_tn);
      if(srcp_tn)
      {
         return srcp_tn->include_name + ":" + STR(srcp_tn->line_number) + ":" + STR(srcp_tn->column_number);
      }
      return "";
   };
   switch(curr_tn->get_kind())
   {
      case call_expr_K:
      case aggr_init_expr_K:
      {
         break;
      }
      case gimple_assign_K:
      {
         const auto gm = GetPointerS<gimple_assign>(curr_tn);
         modified |= recursive_transform(function_id, gm->op0, current_statement, tree_man);
         modified |= recursive_transform(function_id, gm->op1, current_statement, tree_man);
         if(gm->predicate)
         {
            modified |= recursive_transform(function_id, gm->predicate, current_statement, tree_man);
         }
         break;
      }
      case tree_list_K:
      {
         tree_nodeRef current = current_tree_node;
         while(current)
         {
            modified |= recursive_transform(function_id, GetPointer<tree_list>(GET_NODE(current))->valu,
                                            current_statement, tree_man);
            current = GetPointer<tree_list>(GET_NODE(current))->chan;
         }
         break;
      }
      case CASE_UNARY_EXPRESSION:
      {
         const auto ue = GetPointerS<unary_expr>(curr_tn);
         modified |= recursive_transform(function_id, ue->op, current_statement, tree_man);
         break;
      }
      case CASE_BINARY_EXPRESSION:
      {
         const auto be = GetPointerS<binary_expr>(curr_tn);
         const auto be_type = be->get_kind();
         modified |= recursive_transform(function_id, be->op0, current_statement, tree_man);
         modified |= recursive_transform(function_id, be->op1, current_statement, tree_man);

         if(doSoftDiv && (be_type == exact_div_expr_K || be_type == trunc_div_expr_K || be_type == trunc_mod_expr_K))
         {
            const auto expr_type = tree_helper::CGetType(be->op0);
            const auto bitsize0 = ceil_pow2(tree_helper::Size(be->op0));
            const auto bitsize1 = ceil_pow2(tree_helper::Size(be->op1));
            const auto bitsize = std::max(bitsize0, bitsize1);

            const auto div_by_constant = [&]() {
               if(GetPointer<const integer_cst>(GET_CONST_NODE(be->op1)))
               {
                  const auto cst_val = tree_helper::GetConstValue(be->op1);
                  if((cst_val & (cst_val - 1)) == 0)
                  {
                     return true;
                  }
               }
               return false;
            }();

            if(!div_by_constant && GET_CONST_NODE(expr_type)->get_kind() == integer_type_K &&
               (bitsize == 32 || bitsize == 64))
            {
               const auto fu_suffix = be_type == trunc_mod_expr_K ? "mod" : "div";
               const auto bitsize_str = bitsize == 32 ? "s" : "d";
               const auto unsignedp = tree_helper::IsUnsignedIntegerType(expr_type);
               const std::string fu_name = "__" + STR(unsignedp ? "u" : "") + fu_suffix + bitsize_str + "i3" +
                                           ((bitsize0 == 64 && bitsize1 == 32) ? "6432" : "");
               const auto called_function = TreeM->GetFunction(fu_name);
               THROW_ASSERT(called_function, "The library miss this function " + fu_name);
               THROW_ASSERT(TreeM->get_implementation_node(called_function->index) != 0,
                            "inconsistent behavioral helper");
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding call to " + fu_name);
               const std::vector<tree_nodeRef> args = {be->op0, be->op1};
               const auto ce = tree_man->CreateCallExpr(called_function, args, get_current_srcp());
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Replaced " + STR(current_statement));
               TreeM->ReplaceTreeNode(current_statement, current_tree_node, ce);
               CallGraphManager::addCallPointAndExpand(already_visited, AppM, function_id, called_function->index,
                                                       GET_INDEX_CONST_NODE(current_statement),
                                                       FunctionEdgeInfo::CallType::direct_call, DEBUG_LEVEL_NONE);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---      -> " + STR(current_statement));
               modified = true;
            }
         }
         else if(be_type == mult_expr_K)
         {
            const auto expr_type = tree_helper::CGetType(be->op0);
            const auto bitsize0 = ceil_pow2(tree_helper::Size(be->op0));
            const auto bitsize1 = ceil_pow2(tree_helper::Size(be->op1));
            const auto bitsize2 = ceil_pow2(tree_helper::Size(be->type));
            const auto bitsizeIN = std::max(bitsize0, bitsize1);
            const auto bitsize = std::max(bitsizeIN, bitsize2);

            auto doTransf = false;
            std::string fname;
            if(use64bitMul && GET_CONST_NODE(expr_type)->get_kind() == integer_type_K && bitsize == 64)
            {
               const auto unsignedp = tree_helper::IsUnsignedIntegerType(expr_type);
               fname = unsignedp ? "__umul64" : "__mul64";
               doTransf = true;
            }
            if(use32bitMul && GET_CONST_NODE(expr_type)->get_kind() == integer_type_K && bitsize == 32 &&
               bitsizeIN == 32)
            {
               const auto unsignedp = tree_helper::IsUnsignedIntegerType(expr_type);
               fname = unsignedp ? "__umul32" : "__mul32";
               doTransf = true;
            }
            if(doTransf)
            {
               const auto called_function = TreeM->GetFunction(fname);
               THROW_ASSERT(called_function, "The library miss this function " + fname);
               THROW_ASSERT(AppM->get_tree_manager()->get_implementation_node(called_function->index) != 0,
                            "inconsistent behavioral helper");
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding call to " + fname);
               const std::vector<tree_nodeRef> args = {be->op0, be->op1};
               const auto ce = tree_man->CreateCallExpr(called_function, args, get_current_srcp());
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Replaced " + STR(current_statement));
               TreeM->ReplaceTreeNode(current_statement, current_tree_node, ce);
               CallGraphManager::addCallPointAndExpand(already_visited, AppM, function_id, called_function->index,
                                                       GET_INDEX_CONST_NODE(current_statement),
                                                       FunctionEdgeInfo::CallType::direct_call, DEBUG_LEVEL_NONE);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---      -> " + STR(current_statement));
               modified = true;
            }
         }
         break;
      }
      case CASE_TERNARY_EXPRESSION:
      {
         const ternary_expr* te = GetPointer<ternary_expr>(curr_tn);
         modified |= recursive_transform(function_id, te->op0, current_statement, tree_man);
         if(te->op1)
         {
            modified |= recursive_transform(function_id, te->op1, current_statement, tree_man);
         }
         if(te->op2)
         {
            modified |= recursive_transform(function_id, te->op2, current_statement, tree_man);
         }
         break;
      }
      case CASE_QUATERNARY_EXPRESSION:
      {
         const quaternary_expr* qe = GetPointer<quaternary_expr>(curr_tn);
         modified |= recursive_transform(function_id, qe->op0, current_statement, tree_man);
         if(qe->op1)
         {
            modified |= recursive_transform(function_id, qe->op1, current_statement, tree_man);
         }
         if(qe->op2)
         {
            modified |= recursive_transform(function_id, qe->op2, current_statement, tree_man);
         }
         if(qe->op3)
         {
            modified |= recursive_transform(function_id, qe->op3, current_statement, tree_man);
         }
         break;
      }
      case constructor_K:
      {
         const constructor* co = GetPointer<constructor>(curr_tn);
         for(const auto& iv : co->list_of_idx_valu)
         {
            modified |= recursive_transform(function_id, iv.second, current_statement, tree_man);
         }
         break;
      }
      case gimple_call_K:
      case gimple_nop_K:
      case var_decl_K:
      case parm_decl_K:
      case ssa_name_K:
      case lut_expr_K:
      case gimple_cond_K:
      case gimple_switch_K:
      case gimple_multi_way_if_K:
      case gimple_return_K:
      case gimple_for_K:
      case gimple_while_K:
      case CASE_TYPE_NODES:
      case type_decl_K:
      case template_decl_K:
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
      case gimple_pragma_K:
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
      case error_mark_K:
      case using_decl_K:
      case tree_reindex_K:
      case target_expr_K:
      {
         THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC, "Not supported node: " + curr_tn->get_kind_text());
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return modified;
}
