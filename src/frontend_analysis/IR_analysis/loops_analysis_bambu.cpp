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
 * @file loops_analysis_bambu.cpp
 * @brief Analysis step performing loops analysis.
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "loops_analysis_bambu.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "ext_tree_node.hpp"
#include "function_behavior.hpp"
#include "loop.hpp"
#include "loops.hpp"
#include "string_manipulation.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"

#if HAVE_PRAGMA_BUILT
#include "pragma_manager.hpp"
#endif

#include "config_HAVE_PRAGMA_BUILT.hpp"

LoopsAnalysisBambu::LoopsAnalysisBambu(const ParameterConstRef _parameters, const application_managerRef _AppM,
                                       unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, LOOPS_ANALYSIS_BAMBU, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

LoopsAnalysisBambu::~LoopsAnalysisBambu() = default;

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
LoopsAnalysisBambu::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(BB_FEEDBACK_EDGES_IDENTIFICATION, SAME_FUNCTION));
         relationships.insert(std::make_pair(LOOPS_COMPUTATION, SAME_FUNCTION));
         relationships.insert(std::make_pair(EXTRACT_GIMPLE_COND_OP, SAME_FUNCTION));
         const auto is_simd = tree_helper::has_omp_simd(GetPointer<const statement_list>(
             GetPointer<const function_decl>(AppM->get_tree_manager()->GetTreeNode(function_id))->body));
         if(is_simd)
         {
            relationships.insert(std::make_pair(SERIALIZE_MUTUAL_EXCLUSIONS, SAME_FUNCTION));
         }
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(SIMPLE_CODE_MOTION, SAME_FUNCTION));
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}

DesignFlowStep_Status LoopsAnalysisBambu::InternalExec()
{
   const auto TM = AppM->get_tree_manager();
   DesignFlowStep_Status return_value = DesignFlowStep_Status::UNCHANGED;
   const auto fbb = function_behavior->CGetBBGraph(FunctionBehavior::FBB);
   for(const auto& loop : function_behavior->GetLoops()->GetModifiableList())
   {
      loop->loop_type = UNKNOWN_LOOP;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing loop " + STR(loop->GetId()));
      if(!loop->IsReducible())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Loop is irreducible");
         continue;
      }
      const vertex header = loop->GetHeader();
#if HAVE_PRAGMA_BUILT
      const auto PM = AppM->get_pragma_manager();
      if(PM)
      {
         PM->CheckAddOmpFor(function_id, header, AppM);
         PM->CheckAddOmpSimd(function_id, header, AppM);
      }
#endif
      const auto nexit = loop->num_exits();
      if(nexit == 0)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No loop");
         continue;
      }
      if(nexit == 1)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Single exit loop considered");
         loop->loop_type |= SINGLE_EXIT_LOOP;
      }
      const auto exit_vertex = *loop->exit_block_iter_begin();
      bool do_while = false;
      if(exit_vertex == header && loop->num_blocks() != 1)
      {
         /// while loop
         loop->loop_type &= ~UNKNOWN_LOOP;
         loop->loop_type |= WHILE_LOOP;
      }
      else
      {
         /// do while loop
         loop->loop_type &= ~UNKNOWN_LOOP;
         loop->loop_type |= DO_WHILE_LOOP;
         do_while = true;
      }
      /// very simple condition
      if(do_while && loop->is_innermost() && loop->num_blocks() == 1)
      {
         loop->loop_type |= PIPELINABLE_LOOP;
      }

      /// Get exit condition of the loop
      const tree_nodeRef last_stmt = fbb->CGetBBNodeInfo(exit_vertex)->block->CGetStmtList().back();
      if(last_stmt->get_kind() != gimple_cond_K)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Multi way if in the header");
         continue;
      }
      const auto SPBE = loop->get_sp_back_edges();
      if(SPBE.size() > 1)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--More than one feedback edge");
         continue;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Single feedback loop");
      const auto sourceSPBE1 = SPBE.begin()->first;
      const unsigned int sourceSPBE1_index = fbb->CGetBBNodeInfo(sourceSPBE1)->block->number;
      EdgeDescriptor e;
      bool found;
      boost::tie(e, found) = boost::edge(sourceSPBE1, SPBE.begin()->second, *fbb);

      const auto op = GetPointerS<const gimple_cond>(last_stmt)->op0;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---Condition operand (" + op->get_kind_text() + ") is " + op->ToString());
      const auto cond_sn = GetPointer<const ssa_name>(op);
      if(!cond_sn)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Argument of cond expression is not a ssa");
         continue;
      }
      const auto cond_defs = cond_sn->CGetDefStmts();
      if(cond_defs.size() != 1)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "<--Condition variable is not defined in a single statement");
         continue;
      }
      const auto cond_def = *(cond_defs.begin());
      if(cond_def->get_kind() != gimple_assign_K)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Condition not defined by gimple_assign");
         continue;
      }
      const auto cond = GetPointer<const gimple_assign>(cond_def)->op1;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Condition variable is assigned in " + STR(cond));
      if(cond->get_kind() != eq_expr_K && cond->get_kind() != ge_expr_K && cond->get_kind() != gt_expr_K &&
         cond->get_kind() != le_expr_K && cond->get_kind() != lt_expr_K && cond->get_kind() != ne_expr_K &&
         cond->get_kind() != uneq_expr_K and cond->get_kind() != ungt_expr_K && cond->get_kind() != unge_expr_K &&
         cond->get_kind() != unle_expr_K && cond->get_kind() != unlt_expr_K)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Condition is not a simple comparison");
         continue;
      }
      /// We assume that induction variable is the left one; if it is not, analysis will fail
      const auto cond_be = GetPointerS<const binary_expr>(cond);
      const auto first_operand = cond_be->op0;
      /// Induction variable must be a ssa name
      if(first_operand->get_kind() != ssa_name_K)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--First operand of gimple cond is not ssa_name");
         continue;
      }
      const auto sn = GetPointerS<const ssa_name>(first_operand);
      /// Look for its definition
      const auto defs = sn->CGetDefStmts();
      if(defs.size() != 1)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Induction variable is not ssa");
         continue;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Induction variable is " + STR(first_operand));
      const auto def = [&]() -> tree_nodeRef {
         const auto temp_def = *(defs.begin());
         if(do_while)
         {
            return temp_def;
         }
         const auto gp = GetPointer<const gimple_phi>(temp_def);
         if(!gp)
         {
            return tree_nodeRef();
         }
         for(const auto& def_edge : gp->CGetDefEdgesList())
         {
            if(def_edge.second == sourceSPBE1_index)
            {
               const auto temp_sn = GetPointer<const ssa_name>(def_edge.first);
               if(!temp_sn)
               {
                  return tree_nodeRef();
               }
               return temp_sn->CGetDefStmt();
            }
         }
         return tree_nodeRef();
      }();
      if(!def)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Induction variable not assigned");
         continue;
      }
      if(def->get_kind() != gimple_assign_K)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "<--Induction variable is assigned in " + def->ToString());
         continue;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Induction variable is assigned in " + def->ToString());
      const auto ga = GetPointerS<const gimple_assign>(def);
      if(ga->op1->get_kind() != plus_expr_K && ga->op1->get_kind() != minus_expr_K)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "<--Induction variable is not incremented or decremented");
         continue;
      }
      const auto be = GetPointerS<const binary_expr>(ga->op1);
      if(be->op1->get_kind() != integer_cst_K)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "<--Induction variable is not incremented or decremented by a constant");
         continue;
      }
      const auto second_induction_variable = be->op0;
      if(second_induction_variable->get_kind() != ssa_name_K)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Temp induction variable is not a ssa");
         continue;
      }
      const auto defs2 = GetPointerS<const ssa_name>(second_induction_variable)->CGetDefStmts();
      if(defs2.size() != 1)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Temp induction variable is not in ssa form");
         continue;
      }
      const auto def2 = *(defs2.begin());
      if(def2->get_kind() != gimple_phi_K)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Temp induction variable is not defined in a phi");
         continue;
      }
      const auto gp = GetPointerS<const gimple_phi>(def2);
      if(gp->CGetDefEdgesList().size() != 2)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Phi has not two incoming definitions");
         continue;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Init phi is " + gp->ToString());
      const auto init = [&]() -> tree_nodeRef {
         for(const auto& def_edge : gp->CGetDefEdgesList())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---" + def_edge.first->ToString() + " comes from " + STR(def_edge.second));
            if(def_edge.second != sourceSPBE1_index)
            {
               return def_edge.first;
            }
         }
         return tree_nodeRef();
      }();
      loop->main_iv = first_operand->index;
      loop->init = init;
      loop->init_gimple_id = def2->index;
      loop->inc_id = ga->index;
      if(be->op1->get_kind() == integer_cst_K)
      {
         loop->increment = tree_helper::GetConstValue(be->op1);
         if(be->get_kind() == minus_expr_K)
         {
            loop->increment = -loop->increment;
         }
      }
      loop->upper_bound_tn = cond_be->op1;
      loop->increment_tn = be->op1;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---Comparison is " + STR(cond) + " (" + cond->get_kind_text() + ")");
      if(init->get_kind() == integer_cst_K && cond_be->op1->get_kind() == integer_cst_K)
      {
         loop->lower_bound = tree_helper::GetConstValue(init);
         loop->upper_bound = tree_helper::GetConstValue(cond_be->op1);
         const auto cond_type = cond_be->get_kind();
         if(cond_type == ge_expr_K || cond_type == le_expr_K || cond_type == unge_expr_K || cond_type == unle_expr_K)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Close interval");
            loop->close_interval = true;
         }
         loop->loop_type |= COUNTABLE_LOOP;
      }
      if(init)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Initialization " + init->ToString());
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Bound " + loop->upper_bound_tn->ToString());
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Increment " + loop->increment_tn->ToString());

      return_value = DesignFlowStep_Status::SUCCESS;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed loop " + STR(loop->GetId()));
   }
   if(parameters->getOption<bool>(OPT_print_dot))
   {
      function_behavior->CGetLoops()->WriteDot("LF.dot");
   }
   return return_value;
}
