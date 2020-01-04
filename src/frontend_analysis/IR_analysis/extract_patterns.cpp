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
 * @file extract_patterns.cpp
 * @brief
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

// Header include
#include "extract_patterns.hpp"

// Behavior include
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "function_behavior.hpp"

// Parameter include
#include "Parameter.hpp"

// STD include
#include <cmath>
#include <fstream>
#include <string>

// Tree include
#include "dbgPrintHelper.hpp"      // for DEBUG_LEVEL_
#include "string_manipulation.hpp" // for GET_CLASS
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "tree_reindex.hpp"

extract_patterns::extract_patterns(const ParameterConstRef _parameters, const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, EXTRACT_PATTERNS, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

extract_patterns::~extract_patterns() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> extract_patterns::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(PRECEDENCE_RELATIONSHIP):
      {
#if HAVE_BAMBU_BUILT && HAVE_ILP_BUILT
         relationships.insert(std::make_pair(SDC_CODE_MOTION, SAME_FUNCTION));
#endif
         break;
      }
      case DEPENDENCE_RELATIONSHIP:
      {
         relationships.insert(std::make_pair(USE_COUNTING, SAME_FUNCTION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   return relationships;
}

static kind ternary_operation_type0(kind operation_kind1, kind operation_kind2)
{
   if(operation_kind1 == plus_expr_K && operation_kind2 == plus_expr_K)
      return ternary_plus_expr_K;
   else if(operation_kind1 == plus_expr_K && operation_kind2 == minus_expr_K)
      return ternary_pm_expr_K;
   else if(operation_kind1 == minus_expr_K && operation_kind2 == plus_expr_K)
      return ternary_mp_expr_K;
   else // if(operation_kind1 == minus_expr_K && operation_kind2 == minus_expr_K)
      return ternary_mm_expr_K;
}

static kind ternary_operation_type1(kind operation_kind1, kind operation_kind2)
{
   if(operation_kind1 == plus_expr_K && operation_kind2 == plus_expr_K)
      return ternary_plus_expr_K;
   else if(operation_kind1 == plus_expr_K && operation_kind2 == minus_expr_K)
      return ternary_mm_expr_K;
   else if(operation_kind1 == minus_expr_K && operation_kind2 == plus_expr_K)
      return ternary_pm_expr_K;
   else // if(operation_kind1 == minus_expr_K && operation_kind2 == minus_expr_K)
      return ternary_mp_expr_K;
}

void extract_patterns::ternary_plus_expr_extraction(statement_list* sl, tree_managerRef TM)
{
   for(auto bb_pair : sl->list_of_bloc)
   {
      blocRef B = bb_pair.second;
      unsigned int B_id = B->number;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining BB" + STR(B_id));
      const auto list_of_stmt = B->CGetStmtList();
      /// manage capacity
      auto it_los_end = list_of_stmt.end();
      auto it_los = list_of_stmt.begin();
      while(it_los != it_los_end)
      {
#ifndef NDEBUG
         if(not AppM->ApplyNewTransformation())
            break;
#endif
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining statement " + GET_NODE(*it_los)->ToString());
         if(GET_NODE(*it_los)->get_kind() == gimple_assign_K)
         {
            auto* ga = GetPointer<gimple_assign>(GET_NODE(*it_los));
            enum kind code0 = GET_NODE(ga->op0)->get_kind();
            enum kind code1 = GET_NODE(ga->op1)->get_kind();
            if(code0 == ssa_name_K && (code1 == plus_expr_K || code1 == minus_expr_K))
            {
               unsigned int ssa_node_id = GET_INDEX_NODE(ga->op0);
               if(!(tree_helper::is_real(TM, ssa_node_id) || tree_helper::is_a_complex(TM, ssa_node_id) || tree_helper::is_a_vector(TM, ssa_node_id)))
               {
                  auto* ssa_defined = GetPointer<ssa_name>(GET_NODE(ga->op0));
                  unsigned int ssa_defined_size = tree_helper::Size(tree_helper::get_type_node(GET_NODE(ga->op0)));
                  auto* binop0 = GetPointer<binary_expr>(GET_NODE(ga->op1));
                  if(ssa_defined->CGetNumberUses() == 1 and ssa_defined_size == tree_helper::Size(tree_helper::get_type_node(GET_NODE(binop0->op0))) and ssa_defined_size == tree_helper::Size(tree_helper::get_type_node(GET_NODE(binop0->op1))))
                  {
                     auto statement_node = ssa_defined->CGetUseStmts().begin()->first;
                     if(GET_NODE(statement_node)->get_kind() == gimple_assign_K)
                     {
                        auto* ga_dest = GetPointer<gimple_assign>(GET_NODE(statement_node));
                        enum kind code_dest0 = GET_NODE(ga_dest->op0)->get_kind();
                        enum kind code_dest1 = GET_NODE(ga_dest->op1)->get_kind();
                        unsigned int ssa_dest0_size = tree_helper::Size(tree_helper::get_type_node(GET_NODE(ga_dest->op0)));
                        if(code_dest0 == ssa_name_K && (code_dest1 == plus_expr_K || code_dest1 == minus_expr_K) && ga_dest->bb_index == B_id && ssa_dest0_size == ssa_defined_size)
                        {
                           tree_manipulationRef IRman(new tree_manipulation(TM, parameters));
                           /// matched
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Ternary plus expr statement found ");
                           const std::string srcp_default = ga_dest->include_name + ":" + STR(ga_dest->line_number) + ":" + STR(ga_dest->column_number);
                           auto* binop_dest = GetPointer<binary_expr>(GET_NODE(ga_dest->op1));
                           if(GET_INDEX_NODE(ga->op0) == GET_INDEX_NODE(binop_dest->op0))
                           {
                              ga_dest->op1 = IRman->create_ternary_operation(binop_dest->type, binop0->op0, binop0->op1, binop_dest->op1, srcp_default, ternary_operation_type0(code1, code_dest1));
                              GetPointer<ssa_name>(GET_NODE(binop_dest->op0))->RemoveUse(statement_node);
                           }
                           else
                           {
                              ga_dest->op1 = IRman->create_ternary_operation(binop_dest->type, binop_dest->op0, binop0->op0, binop0->op1, srcp_default, ternary_operation_type1(code1, code_dest1));
                              GetPointer<ssa_name>(GET_NODE(binop_dest->op1))->RemoveUse(statement_node);
                           }
                           auto sn0 = GetPointer<ssa_name>(GET_NODE(binop0->op0));
                           if(sn0)
                              sn0->AddUseStmt(statement_node);
                           auto sn1 = GetPointer<ssa_name>(GET_NODE(binop0->op1));
                           if(sn1)
                              sn1->AddUseStmt(statement_node);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Statement removed " + GET_NODE(*it_los)->ToString());
                           B->RemoveStmt(*it_los);
                           it_los = list_of_stmt.begin();
                           it_los_end = list_of_stmt.end();
#ifndef NDEBUG
                           AppM->RegisterTransformation(GetName(), statement_node);
#endif
                           continue;
                        }
                     }
                  }
               }
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Statement analyzed " + GET_NODE(*it_los)->ToString());
         ++it_los;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined BB" + STR(B_id));
   }
}

DesignFlowStep_Status extract_patterns::InternalExec()
{
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, " --------- EXTRACT_PATTERNS ---------- ");
   tree_managerRef TM = AppM->get_tree_manager();
   tree_nodeRef tn = TM->get_tree_node_const(function_id);
   // tree_nodeRef Scpe = TM->GetTreeReindex(function_id);
   auto* fd = GetPointer<function_decl>(tn);
   THROW_ASSERT(fd && fd->body, "Node is not a function or it hasn't a body");
   auto* sl = GetPointer<statement_list>(GET_NODE(fd->body));
   THROW_ASSERT(sl, "Body is not a statement_list");
   /// for each basic block B in CFG do > Consider all blocks successively
   ternary_plus_expr_extraction(sl, TM);

   return DesignFlowStep_Status::SUCCESS;
}
