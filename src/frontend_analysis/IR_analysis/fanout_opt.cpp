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
 *              Copyright (C) 2017-2020 Politecnico di Milano
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
 * @file fanout_opt.cpp
 * @brief Fanout optimization step.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

/// Header include
#include "fanout_opt.hpp"

///. include
#include "Parameter.hpp"

/// boost include
#include <boost/graph/topological_sort.hpp>

/// behavior includes
#include "application_manager.hpp"
#include "function_behavior.hpp"

/// HLS include
#include "hls_manager.hpp"
#if HAVE_BAMBU_BUILT && HAVE_ILP_BUILT
/// HLS includes
#include "hls.hpp"

/// HLS/scheduling include
#include "schedule.hpp"
#endif

/// STD include
#include <cmath>
#include <fstream>
#include <string>

#include "dbgPrintHelper.hpp"      // for DEBUG_LEVEL_
#include "string_manipulation.hpp" // for GET_CLASS
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

fanout_opt::fanout_opt(const ParameterConstRef _parameters, const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, FANOUT_OPT, _design_flow_manager, _parameters), TM(_AppM->get_tree_manager())
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

fanout_opt::~fanout_opt() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> fanout_opt::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(INVALIDATION_RELATIONSHIP):
      {
         switch(GetStatus())
         {
            case DesignFlowStep_Status::SUCCESS:
            {
               if(not parameters->getOption<int>(OPT_gcc_openmp_simd))
                  relationships.insert(std::make_pair(BIT_VALUE, SAME_FUNCTION));
               break;
            }
            case DesignFlowStep_Status::SKIPPED:
            case DesignFlowStep_Status::UNCHANGED:
            case DesignFlowStep_Status::UNEXECUTED:
            case DesignFlowStep_Status::UNNECESSARY:
            {
               break;
            }
            case DesignFlowStep_Status::ABORTED:
            case DesignFlowStep_Status::EMPTY:
            case DesignFlowStep_Status::NONEXISTENT:
            default:
               THROW_UNREACHABLE("");
         }
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      case DEPENDENCE_RELATIONSHIP:
      {
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(CSE_STEP, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(EXTRACT_PATTERNS, SAME_FUNCTION));
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   return relationships;
}

bool fanout_opt::is_dest_relevant(tree_nodeRef t, bool)
{
   THROW_ASSERT(t->get_kind() == tree_reindex_K, "t is not a tree_reindex node");
   if(GET_NODE(t)->get_kind() == gimple_assign_K)
   {
      auto* temp_assign = GetPointer<gimple_assign>(GET_NODE(t));
      if(GET_NODE(temp_assign->op1)->get_kind() == mult_expr_K || GET_NODE(temp_assign->op1)->get_kind() == widen_mult_expr_K || GET_NODE(temp_assign->op1)->get_kind() == ternary_plus_expr_K || GET_NODE(temp_assign->op1)->get_kind() == ternary_mm_expr_K ||
         GET_NODE(temp_assign->op1)->get_kind() == ternary_pm_expr_K || GET_NODE(temp_assign->op1)->get_kind() == ternary_mp_expr_K)
         return true;
   }
   return false;
}

DesignFlowStep_Status fanout_opt::InternalExec()
{
   if(parameters->IsParameter("disable-fanout_opt"))
      return DesignFlowStep_Status::SKIPPED;
   bool IR_changed = false;

   tree_nodeRef temp = TM->get_tree_node_const(function_id);
   auto* fd = GetPointer<function_decl>(temp);
   auto* sl = GetPointer<statement_list>(GET_NODE(fd->body));
   const tree_manipulationRef tree_man = tree_manipulationRef(new tree_manipulation(TM, parameters));

   for(auto block : sl->list_of_bloc)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining BB" + STR(block.first));
      for(const auto& stmt : block.second->CGetStmtList())
      {
#ifndef NDEBUG
         if(not AppM->ApplyNewTransformation())
         {
            break;
         }
#endif
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining statement " + GET_NODE(stmt)->ToString());
         if(GET_NODE(stmt)->get_kind() == gimple_assign_K)
         {
            auto* ga = GetPointer<gimple_assign>(GET_NODE(stmt));
            const std::string srcp_default = ga->include_name + ":" + STR(ga->line_number) + ":" + STR(ga->column_number);
            if(GET_NODE(ga->op0)->get_kind() == ssa_name_K)
            {
               auto* ssa_defined = GetPointer<ssa_name>(GET_NODE(ga->op0));
               if(ssa_defined->CGetNumberUses() > 1)
               {
                  unsigned assigned_ssa_type_index;
                  const tree_nodeRef assigned_ssa_type_node = tree_helper::get_type_node(GET_NODE(ga->op0), assigned_ssa_type_index);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---the assigned ssa_name " + STR(GET_NODE(ga->op0)) + " has type " + assigned_ssa_type_node->ToString());
                  bool is_first_stmt = true;
                  std::list<tree_nodeRef> list_of_dest_statements;
                  for(auto dest_statement : ssa_defined->CGetUseStmts())
                  {
                     if(is_first_stmt)
                        is_first_stmt = false;
                     else if(is_dest_relevant(dest_statement.first, false))
                        list_of_dest_statements.push_back(dest_statement.first);
                  }
                  for(auto dest_statement : list_of_dest_statements)
                  {
                     tree_nodeRef temp_assign = tree_man->CreateGimpleAssign(assigned_ssa_type_node, ssa_defined->min, ssa_defined->max, ga->op0, block.first, srcp_default);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---create a temporary assignment " + temp_assign->ToString());
                     block.second->PushAfter(temp_assign, stmt);
                     tree_nodeRef temp_ssa_var = GetPointer<gimple_assign>(GET_NODE(temp_assign))->op0;
                     GetPointer<gimple_assign>(GET_NODE(temp_assign))->keep = true;
                     GetPointer<gimple_assign>(GET_NODE(temp_assign))->temporary_address = ga->temporary_address;
                     GetPointer<ssa_name>(GET_NODE(temp_ssa_var))->SetDefStmt(temp_assign);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---dest statement before replacement " + dest_statement->ToString());
                     TM->ReplaceTreeNode(dest_statement, ga->op0, temp_ssa_var);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---dest statement after replacement " + dest_statement->ToString());
                     IR_changed = true;
                  }
               }
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Statement examined " + GET_NODE(stmt)->ToString());
      }
#if 1
      /// fanout duplication may prevent openmp simd detection
      if(!parameters->getOption<int>(OPT_gcc_openmp_simd))
      {
         for(auto phi : block.second->CGetPhiList())
         {
            auto gp = GetPointer<gimple_phi>(GET_NODE(phi));
            auto* ssa_defined = GetPointer<ssa_name>(GET_NODE(gp->res));

            if(ssa_defined->CGetNumberUses() > 1)
            {
               bool is_first_stmt = true;
               std::list<tree_nodeRef> list_of_dest_statements;
               for(auto dest_statement : ssa_defined->CGetUseStmts())
               {
                  if(is_first_stmt)
                     is_first_stmt = false;
                  else if(is_dest_relevant(dest_statement.first, true))
                     list_of_dest_statements.push_back(dest_statement.first);
               }
               for(auto dest_statement : list_of_dest_statements)
               {
                  /// Copy the list of def edges
                  std::vector<std::pair<tree_nodeRef, unsigned int>> list_of_def_edge;
                  for(const auto& def_edge : gp->CGetDefEdgesList())
                  {
                     list_of_def_edge.push_back(std::pair<tree_nodeRef, unsigned int>(def_edge.first, def_edge.second));
                  }
                  tree_nodeRef new_res_var;
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---starting from phi " + phi->ToString());
                  auto new_phi = tree_man->create_phi_node(new_res_var, list_of_def_edge, gp->scpe, block.first);
                  auto new_res_var_ssa = GetPointer<ssa_name>(GET_NODE(new_res_var));
                  new_res_var_ssa->min = ssa_defined->min;
                  new_res_var_ssa->max = ssa_defined->max;
                  GetPointer<gimple_phi>(GET_NODE(new_phi))->SetSSAUsesComputed();
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---created a new phi " + new_phi->ToString());
                  block.second->AddPhi(new_phi);
                  GetPointer<gimple_phi>(GET_NODE(new_phi))->keep = true;
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---dest statement before replacement " + dest_statement->ToString());
                  TM->ReplaceTreeNode(dest_statement, gp->res, new_res_var);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---dest statement after replacement " + dest_statement->ToString());
                  IR_changed = true;
               }
            }
         }
      }
#endif
      if(IR_changed && schedule)
      {
         for(const auto& stmt : block.second->CGetStmtList())
            schedule->UpdateTime(GET_INDEX_NODE(stmt));
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered BB" + STR(block.first));
   }
   IR_changed ? function_behavior->UpdateBBVersion() : 0;
   return IR_changed ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}

void fanout_opt::Initialize()
{
#if HAVE_BAMBU_BUILT && HAVE_ILP_BUILT
   if(GetPointer<const HLS_manager>(AppM) and GetPointer<const HLS_manager>(AppM)->get_HLS(function_id) and GetPointer<const HLS_manager>(AppM)->get_HLS(function_id)->Rsch)
   {
      schedule = GetPointer<const HLS_manager>(AppM)->get_HLS(function_id)->Rsch;
   }
#endif
}
