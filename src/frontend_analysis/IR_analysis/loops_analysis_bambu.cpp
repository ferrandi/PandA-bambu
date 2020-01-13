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
 * @file loops_analysis_bambu.cpp
 * @brief Analysis step performing loops analysis.
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Autoheader include
#include "config_HAVE_PRAGMA_BUILT.hpp"

/// Header include
#include "loops_analysis_bambu.hpp"

///. include
#include "Parameter.hpp"

/// algorithms/loops_detection include
#include "loop.hpp"
#include "loops.hpp"

/// behavior include
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "function_behavior.hpp"

/// pragma include
#include "pragma_manager.hpp"

/// tree include
#include "ext_tree_node.hpp"
#include "string_manipulation.hpp" // for GET_CLASS
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_reindex.hpp"

LoopsAnalysisBambu::LoopsAnalysisBambu(const ParameterConstRef _parameters, const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, LOOPS_ANALYSIS_BAMBU, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

LoopsAnalysisBambu::~LoopsAnalysisBambu() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> LoopsAnalysisBambu::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(BB_FEEDBACK_EDGES_IDENTIFICATION, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(DOM_POST_DOM_COMPUTATION, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(LOOPS_COMPUTATION, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(EXTRACT_GIMPLE_COND_OP, SAME_FUNCTION));
         const auto is_simd = [&]() -> bool {
            const auto sl = GetPointer<const statement_list>(GET_NODE(GetPointer<const function_decl>(AppM->get_tree_manager()->CGetTreeNode(function_id))->body));
            THROW_ASSERT(sl, "");
            for(const auto& block : sl->list_of_bloc)
            {
               for(const auto& stmt : block.second->CGetStmtList())
               {
                  const auto gp = GetPointer<const gimple_pragma>(GET_NODE(stmt));
                  if(gp and gp->scope and GetPointer<const omp_pragma>(GET_NODE(gp->scope)))
                  {
                     const auto* sp = GetPointer<const omp_simd_pragma>(GET_NODE(gp->directive));
                     if(sp)
                     {
                        return true;
                     }
                  }
               }
            }
            return false;
         }();
         if(is_simd)
            relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(SERIALIZE_MUTUAL_EXCLUSIONS, SAME_FUNCTION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(SIMPLE_CODE_MOTION, SAME_FUNCTION));
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
   const tree_managerRef TM = AppM->get_tree_manager();
   DesignFlowStep_Status return_value = DesignFlowStep_Status::UNCHANGED;
   const BBGraphConstRef bb = function_behavior->CGetBBGraph(FunctionBehavior::BB);
   const BBGraphConstRef fbb = function_behavior->CGetBBGraph(FunctionBehavior::FBB);
   const BBGraphConstRef pdt = function_behavior->CGetBBGraph(FunctionBehavior::POST_DOM_TREE);
   for(auto loop : function_behavior->GetLoops()->GetModifiableList())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing loop " + STR(loop->GetId()));
      if(!loop->IsReducible())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Loop is irreducible");
         continue;
      }
      const vertex header = loop->GetHeader();
      const pragma_managerRef PM = AppM->get_pragma_manager();
      PM->CheckAddOmpFor(function_id, header);
      PM->CheckAddOmpSimd(function_id, header);
      bool do_while = false;
      size_t feedback_edges = 0;
      InEdgeIterator ei, ei_end;
      for(boost::tie(ei, ei_end) = boost::in_edges(header, *fbb); ei != ei_end; ei++)
      {
         if((FB_CFG_SELECTOR)&fbb->GetSelector(*ei))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Feedback edge");
            feedback_edges++;
            if(boost::source(*ei, *fbb) == boost::target(*ei, *fbb))
            {
               do_while = true;
               loop->loop_type &= ~UNKNOWN_LOOP;
               loop->loop_type |= DO_WHILE_LOOP;
               continue;
            }
            vertex current_dom = boost::target(*ei, *fbb);
            while(current_dom != boost::source(*ei, *fbb) and boost::in_degree(current_dom, *pdt) == 1)
            {
               InEdgeIterator pdt_ei, pdt_ei_end;
               boost::tie(pdt_ei, pdt_ei_end) = boost::in_edges(current_dom, *pdt);
               current_dom = boost::source(*pdt_ei, *pdt);
            }
            if(current_dom == boost::source(*ei, *fbb))
            {
               do_while = true;
               loop->loop_type &= ~UNKNOWN_LOOP;
               loop->loop_type |= DO_WHILE_LOOP;
            }
         }
      }
      if(feedback_edges != 1)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--More than a feedback loop");
         continue;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Single feedback loop");

      /// Get last basic block of the loop
      const vertex last_bb = [&]() -> vertex {
         for(boost::tie(ei, ei_end) = boost::in_edges(header, *fbb); ei != ei_end; ei++)
         {
            if((FB_CFG_SELECTOR)&fbb->GetSelector(*ei))
            {
               return boost::source(*ei, *fbb);
            }
         }
         return NULL_VERTEX;
      }();
      const unsigned int last_bb_index = fbb->CGetBBNodeInfo(last_bb)->block->number;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Last basic block is BB" + STR(last_bb_index));
      /// Get exit condition of the loop
      const tree_nodeRef last_stmt = GET_NODE(fbb->CGetBBNodeInfo(do_while ? last_bb : header)->block->CGetStmtList().back());
      if(last_stmt->get_kind() != gimple_cond_K)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Multi way if in the header");
         continue;
      }
      const tree_nodeRef op = GET_NODE(GetPointer<const gimple_cond>(last_stmt)->op0);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Condition operand (" + op->get_kind_text() + ") is " + op->ToString());
      const auto* cond_sn = GetPointer<const ssa_name>(op);
      if(not cond_sn)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Argument of cond expression is not a ssa");
         continue;
      }
      const auto cond_defs = cond_sn->CGetDefStmts();
      if(cond_defs.size() != 1)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Condition variable is not defined in a single statement");
         continue;
      }
      const tree_nodeRef cond_def = GET_NODE(*(cond_defs.begin()));
      if(cond_def->get_kind() != gimple_assign_K)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Condition variable not defined in a gimple_stmt");
         continue;
      }
      const tree_nodeRef cond = GET_NODE(GetPointer<const gimple_assign>(cond_def)->op1);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Condition variable is assigned in " + STR(cond));
      if(cond->get_kind() != eq_expr_K and cond->get_kind() != ge_expr_K and cond->get_kind() != gt_expr_K and cond->get_kind() != le_expr_K and cond->get_kind() != lt_expr_K and cond->get_kind() != ne_expr_K and cond->get_kind() != uneq_expr_K and
         cond->get_kind() != ungt_expr_K and cond->get_kind() != unge_expr_K and cond->get_kind() != unle_expr_K and cond->get_kind() != unlt_expr_K)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Condition is not a comparison");
         continue;
      }
      /// We assume that induction variable is the left one; if it is not, analysis will fail
      const auto* cond_be = GetPointer<const binary_expr>(cond);
      const tree_nodeRef first_operand = GET_NODE(cond_be->op0);
      /// Induction variable must be a ssa name
      if(first_operand->get_kind() != ssa_name_K)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--First operand of gimple cond is not ssa_name");
         continue;
      }
      const auto* sn = GetPointer<const ssa_name>(first_operand);
      /// Look for its definition
      const auto defs = sn->CGetDefStmts();
      if(defs.size() != 1)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Induction variable is not ssa");
         continue;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Induction variable is " + STR(first_operand));
      const tree_nodeRef def = [&]() -> tree_nodeRef {
         const auto temp_def = GET_NODE(*(defs.begin()));
         if(do_while)
            return temp_def;
         const auto gp = GetPointer<const gimple_phi>(temp_def);
         if(not gp)
            return tree_nodeRef();
         for(const auto& def_edge : gp->CGetDefEdgesList())
         {
            if(def_edge.second == last_bb_index)
            {
               const auto temp_sn = GetPointer<const ssa_name>(GET_NODE(def_edge.first));
               if(not temp_sn)
                  return tree_nodeRef();
               return GET_NODE(temp_sn->CGetDefStmt());
            }
         }
         return tree_nodeRef();
      }();
      if(not def)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Induction variable not assigned");
         continue;
      }
      if(def->get_kind() != gimple_assign_K)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Induction variable is assigned in " + def->ToString());
         continue;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Induction variable is assigned in " + def->ToString());
      const auto* ga = GetPointer<const gimple_assign>(def);
      if(GET_NODE(ga->op1)->get_kind() != plus_expr_K and GET_NODE(ga->op1)->get_kind() != minus_expr_K)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Induction variable is not incremented or decremented");
         continue;
      }
      const auto* be = GetPointer<const binary_expr>(GET_NODE(ga->op1));
      if(GET_NODE(be->op1)->get_kind() != integer_cst_K)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Induction variable is not incremnted or decremented of a constant");
         continue;
      }
      const tree_nodeRef second_induction_variable = GET_NODE(be->op0);
      if(second_induction_variable->get_kind() != ssa_name_K)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Temp induction variable is not a ssa");
         continue;
      }
      const auto defs2 = GetPointer<const ssa_name>(second_induction_variable)->CGetDefStmts();
      if(defs2.size() != 1)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Temp induction variable is not in ssa form");
         continue;
      }
      const tree_nodeRef def2 = GET_NODE(*(defs2.begin()));
      if(def2->get_kind() != gimple_phi_K)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Temp induction variable is not defined in a phi");
         continue;
      }
      const auto* gp = GetPointer<const gimple_phi>(def2);
      if(gp->CGetDefEdgesList().size() != 2)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Phi has not two incoming definitions");
         continue;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Init phi is " + gp->ToString());
      const tree_nodeRef init = [&]() -> tree_nodeRef {
         for(const auto& def_edge : gp->CGetDefEdgesList())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + def_edge.first->ToString() + " comes from " + STR(def_edge.second));
            if(def_edge.second != last_bb_index)
            {
               return def_edge.first;
            }
         }
         return tree_nodeRef();
      }();
      loop->main_iv = first_operand->index;
      loop->initialization_tree_node_id = init->index;
      loop->init_gimple_id = def2->index;
      loop->inc_id = ga->index;
      if(GET_NODE(be->op1)->get_kind() == integer_cst_K)
      {
         loop->increment = tree_helper::get_integer_cst_value(GetPointer<const integer_cst>(GET_NODE(be->op1)));
         if(be->get_kind() == minus_expr_K)
         {
            loop->increment = -loop->increment;
         }
      }
      loop->upper_bound_tn = cond_be->op1;
      loop->increment_tn = be->op1;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Comparison is " + STR(cond) + " (" + cond->get_kind_text() + ")");
      if(GET_NODE(init)->get_kind() == integer_cst_K and GET_NODE(cond_be->op1)->get_kind() == integer_cst_K)
      {
         loop->lower_bound = tree_helper::get_integer_cst_value(GetPointer<const integer_cst>(GET_NODE(init)));
         loop->upper_bound = tree_helper::get_integer_cst_value(GetPointer<const integer_cst>(GET_NODE(cond_be->op1)));
         if(cond_be->get_kind() == ge_expr_K or cond_be->get_kind() == le_expr_K or cond_be->get_kind() == unge_expr_K or cond_be->get_kind() == unle_expr_K)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Close interval");
            loop->close_interval = true;
         }
         loop->loop_type |= COUNTABLE_LOOP;
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Initialization " + init->ToString());
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Bound " + GET_NODE(be->op1)->ToString());
      }
      return_value = DesignFlowStep_Status::SUCCESS;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed loop " + STR(loop->GetId()));
   }
   if(parameters->getOption<bool>(OPT_print_dot))
   {
      function_behavior->CGetLoops()->WriteDot("LF.dot");
   }
   return return_value;
}
