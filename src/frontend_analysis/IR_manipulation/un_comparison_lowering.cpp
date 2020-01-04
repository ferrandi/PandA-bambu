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
 *              Copyright (C) 2016-2020 Politecnico di Milano
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
 * @file un_comparison_lowering.cpp
 * @brief Step that replace uneq_expr, ltgt_expr, unge_expr, ungt_expr, unle_expr and unlt_expr
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Header include
#include "un_comparison_lowering.hpp"

///. include
#include "Parameter.hpp"

/// behavior include
#include "application_manager.hpp"
#include "function_behavior.hpp"

/// tree includes
#include "dbgPrintHelper.hpp"      // for DEBUG_LEVEL_
#include "string_manipulation.hpp" // for GET_CLASS
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

UnComparisonLowering::UnComparisonLowering(const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters)
    : FunctionFrontendFlowStep(_AppM, _function_id, UN_COMPARISON_LOWERING, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}
const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> UnComparisonLowering::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(EXTRACT_GIMPLE_COND_OP, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(USE_COUNTING, SAME_FUNCTION));
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

UnComparisonLowering::~UnComparisonLowering() = default;

DesignFlowStep_Status UnComparisonLowering::InternalExec()
{
   bool modified = false;
   const tree_managerRef TreeM = AppM->get_tree_manager();
   const tree_manipulationRef tree_man = tree_manipulationRef(new tree_manipulation(TreeM, parameters));
   const tree_nodeRef curr_tn = TreeM->GetTreeNode(function_id);
   tree_nodeRef Scpe = TreeM->GetTreeReindex(function_id);
   auto* fd = GetPointer<function_decl>(curr_tn);
   auto* sl = GetPointer<statement_list>(GET_NODE(fd->body));
   for(const auto& block : sl->list_of_bloc)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing BB" + STR(block.first));
      for(const auto& stmt : block.second->CGetStmtList())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing " + stmt->ToString());
         auto ga = GetPointer<gimple_assign>(GET_NODE(stmt));
         if(not ga)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped" + STR(stmt));
            continue;
         }
         auto be = GetPointer<binary_expr>(GET_NODE(ga->op1));
         if(not be)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped" + STR(stmt));
            continue;
         }
         if(be->get_kind() == unlt_expr_K or be->get_kind() == unge_expr_K or be->get_kind() == ungt_expr_K or be->get_kind() == unle_expr_K or be->get_kind() == ltgt_expr_K)
         {
            const std::string srcp_string = be->include_name + ":" + STR(be->line_number) + ":" + STR(be->column_number);
            enum kind new_kind = last_tree_K;
            if(be->get_kind() == unlt_expr_K)
               new_kind = ge_expr_K;
            else if(be->get_kind() == unge_expr_K)
               new_kind = lt_expr_K;
            else if(be->get_kind() == ungt_expr_K)
               new_kind = le_expr_K;
            else if(be->get_kind() == unle_expr_K)
               new_kind = gt_expr_K;
            else if(be->get_kind() == ltgt_expr_K)
               new_kind = eq_expr_K;
            else
               THROW_UNREACHABLE("");
            auto booleanType = tree_man->create_boolean_type();
            auto new_be = tree_man->create_binary_operation(booleanType, be->op0, be->op1, srcp_string, new_kind);
            auto new_ga = tree_man->CreateGimpleAssign(booleanType, TreeM->CreateUniqueIntegerCst(0, booleanType->index), TreeM->CreateUniqueIntegerCst(1, booleanType->index), new_be, 0, srcp_string);
            block.second->PushBefore(new_ga, stmt);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(new_ga));
            auto new_not = tree_man->create_unary_operation(booleanType, GetPointer<gimple_assign>(GET_NODE(new_ga))->op0, srcp_string, truth_not_expr_K);
            if(GET_INDEX_NODE(be->type) != GET_INDEX_NODE(booleanType))
            {
               auto new_ga_not = tree_man->CreateGimpleAssign(booleanType, TreeM->CreateUniqueIntegerCst(0, booleanType->index), TreeM->CreateUniqueIntegerCst(1, booleanType->index), new_not, 0, srcp_string);
               block.second->PushBefore(new_ga_not, stmt);
               auto new_nop = tree_man->create_unary_operation(be->type, GetPointer<gimple_assign>(GET_NODE(new_ga_not))->op0, srcp_string, nop_expr_K);
               TreeM->ReplaceTreeNode(stmt, ga->op1, new_nop);
            }
            else
               TreeM->ReplaceTreeNode(stmt, ga->op1, new_not);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Transformed into " + STR(stmt));
            modified = true;
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped" + STR(stmt));
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed BB" + STR(block.first));
   }
   if(modified)
   {
      function_behavior->UpdateBBVersion();
   }
   return modified ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}
