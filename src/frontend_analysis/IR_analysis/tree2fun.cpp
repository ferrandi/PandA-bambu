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
 * @file tree2fun.cpp
 * @brief Step that replace some tree node expression with function call
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "tree2fun.hpp"

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
#include "hls_manager.hpp"
#include "math_function.hpp"
#include "string_manipulation.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "tree_node.hpp"

tree2fun::tree2fun(const ParameterConstRef _parameters, const application_managerRef _AppM, unsigned int _function_id,
                   const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, TREE2FUN, _design_flow_manager, _parameters),
      TreeM(_AppM->get_tree_manager())
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

tree2fun::~tree2fun() = default;

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
tree2fun::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         relationships.insert(std::make_pair(COMPUTE_IMPLICIT_CALLS, SAME_FUNCTION));
         relationships.insert(std::make_pair(FIX_STRUCTS_PASSED_BY_VALUE, SAME_FUNCTION));
         relationships.insert(std::make_pair(FUNCTION_CALL_TYPE_CLEANUP, SAME_FUNCTION));
         relationships.insert(std::make_pair(IR_LOWERING, SAME_FUNCTION));
         relationships.insert(std::make_pair(USE_COUNTING, SAME_FUNCTION));
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         if(GetStatus() == DesignFlowStep_Status::SUCCESS)
         {
            relationships.insert(std::make_pair(FUNCTION_CALL_TYPE_CLEANUP, SAME_FUNCTION));
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

DesignFlowStep_Status tree2fun::InternalExec()
{
   const tree_manipulationRef tree_man(new tree_manipulation(TreeM, parameters, AppM));

   const auto curr_tn = TreeM->GetTreeNode(function_id);
   const auto fname = tree_helper::GetFunctionName(TreeM, curr_tn);
   const auto fd = GetPointerS<function_decl>(curr_tn);
   const auto sl = GetPointerS<statement_list>(fd->body);

   bool modified = false;
   for(const auto& idx_bb : sl->list_of_bloc)
   {
      const auto& BB = idx_bb.second;
      for(const auto& stmt : BB->CGetStmtList())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "-->Examine " + STR(stmt->index) + " " + stmt->ToString());
         modified |= recursive_transform(stmt, stmt, tree_man);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "<--Examined " + STR(stmt->index) + " " + stmt->ToString());
      }
   }

   if(modified)
   {
      function_behavior->UpdateBBVersion();
      return DesignFlowStep_Status::SUCCESS;
   }
   return DesignFlowStep_Status::UNCHANGED;
}

bool tree2fun::recursive_transform(const tree_nodeRef& curr_tn, const tree_nodeRef& current_statement,
                                   const tree_manipulationRef tree_man)
{
   bool modified = false;
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
         modified |= recursive_transform(gm->op0, current_statement, tree_man);
         modified |= recursive_transform(gm->op1, current_statement, tree_man);
         if(gm->predicate)
         {
            modified |= recursive_transform(gm->predicate, current_statement, tree_man);
         }
         break;
      }
      case tree_list_K:
      {
         tree_nodeRef current = curr_tn;
         while(current)
         {
            modified |= recursive_transform(GetPointerS<tree_list>(current)->valu, current_statement, tree_man);
            current = GetPointerS<tree_list>(current)->chan;
         }
         break;
      }
      case CASE_UNARY_EXPRESSION:
      {
         const auto ue = GetPointerS<unary_expr>(curr_tn);
         modified |= recursive_transform(ue->op, current_statement, tree_man);
         break;
      }
      case CASE_BINARY_EXPRESSION:
      {
         const auto be = GetPointerS<binary_expr>(curr_tn);
         const auto be_type = be->get_kind();
         modified |= recursive_transform(be->op0, current_statement, tree_man);
         modified |= recursive_transform(be->op1, current_statement, tree_man);
         if(be_type == frem_expr_K)
         {
            const auto expr_type = tree_helper::CGetType(be->op0);
            THROW_ASSERT(expr_type->get_kind() == real_type_K, "unexpected case");
            const auto bitsize = tree_helper::Size(expr_type);
            const std::string fu_name = bitsize == 32 ? "fmodf" : "fmod";
            const auto called_function = TreeM->GetFunction(fu_name);
            THROW_ASSERT(called_function, "Add option -lm to the command line for frem/fmod");
            THROW_ASSERT(TreeM->get_implementation_node(called_function->index) != 0, "inconsistent behavioral helper");
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding call to " + fu_name);
            const std::vector<tree_nodeRef> args = {be->op0, be->op1};
            const auto ce = tree_man->CreateCallExpr(called_function, args, get_current_srcp());
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Replaced " + STR(current_statement));
            TreeM->ReplaceTreeNode(current_statement, curr_tn, ce);
            CallGraphManager::addCallPointAndExpand(already_visited, AppM, function_id, called_function->index,
                                                    current_statement->index, FunctionEdgeInfo::CallType::direct_call,
                                                    DEBUG_LEVEL_NONE);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---      -> " + STR(current_statement));
            modified = true;
         }
         break;
      }
      case CASE_TERNARY_EXPRESSION:
      {
         const auto te = GetPointerS<ternary_expr>(curr_tn);
         modified |= recursive_transform(te->op0, current_statement, tree_man);
         if(te->op1)
         {
            modified |= recursive_transform(te->op1, current_statement, tree_man);
         }
         if(te->op2)
         {
            modified |= recursive_transform(te->op2, current_statement, tree_man);
         }
         break;
      }
      case CASE_QUATERNARY_EXPRESSION:
      {
         const auto qe = GetPointerS<quaternary_expr>(curr_tn);
         modified |= recursive_transform(qe->op0, current_statement, tree_man);
         if(qe->op1)
         {
            modified |= recursive_transform(qe->op1, current_statement, tree_man);
         }
         if(qe->op2)
         {
            modified |= recursive_transform(qe->op2, current_statement, tree_man);
         }
         if(qe->op3)
         {
            modified |= recursive_transform(qe->op3, current_statement, tree_man);
         }
         break;
      }
      case constructor_K:
      {
         const auto co = GetPointerS<constructor>(curr_tn);
         for(const auto& iv : co->list_of_idx_valu)
         {
            modified |= recursive_transform(iv.second, current_statement, tree_man);
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
