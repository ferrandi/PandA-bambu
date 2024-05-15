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
 * @file extract_patterns.cpp
 * @brief
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#include "extract_patterns.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "function_behavior.hpp"
#include "hls_device.hpp"
#include "hls_manager.hpp"
#include "string_manipulation.hpp" // for GET_CLASS
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include <cmath>
#include <fstream>
#include <string>

extract_patterns::extract_patterns(const ParameterConstRef _parameters, const application_managerRef _AppM,
                                   unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, EXTRACT_PATTERNS, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

extract_patterns::~extract_patterns() = default;

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
extract_patterns::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         relationships.insert(std::make_pair(USE_COUNTING, SAME_FUNCTION));
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
#if HAVE_ILP_BUILT
         relationships.insert(std::make_pair(SDC_CODE_MOTION, SAME_FUNCTION));
#endif
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
   {
      return ternary_plus_expr_K;
   }
   else if(operation_kind1 == plus_expr_K && operation_kind2 == minus_expr_K)
   {
      return ternary_pm_expr_K;
   }
   else if(operation_kind1 == minus_expr_K && operation_kind2 == plus_expr_K)
   {
      return ternary_mp_expr_K;
   }
   else
   { // if(operation_kind1 == minus_expr_K && operation_kind2 == minus_expr_K)
      return ternary_mm_expr_K;
   }
}

static kind ternary_operation_type1(kind operation_kind1, kind operation_kind2)
{
   if(operation_kind1 == plus_expr_K && operation_kind2 == plus_expr_K)
   {
      return ternary_plus_expr_K;
   }
   else if(operation_kind1 == plus_expr_K && operation_kind2 == minus_expr_K)
   {
      return ternary_mm_expr_K;
   }
   else if(operation_kind1 == minus_expr_K && operation_kind2 == plus_expr_K)
   {
      return ternary_pm_expr_K;
   }
   else
   { // if(operation_kind1 == minus_expr_K && operation_kind2 == minus_expr_K)
      return ternary_mp_expr_K;
   }
}

DesignFlowStep_Status extract_patterns::InternalExec()
{
   if(parameters->IsParameter("disable-extract-patterns") &&
      parameters->GetParameter<unsigned int>("disable-extract-patterns") == 1)
   {
      return DesignFlowStep_Status::UNCHANGED;
   }
   const auto hls_d = GetPointer<const HLS_manager>(AppM)->get_HLS_device();
   if(hls_d->has_parameter("disable_extract_ternary_patterns") &&
      hls_d->get_parameter<unsigned>("disable_extract_ternary_patterns"))
   {
      /// Now, the only patterns extracted are ternary.
      /// So, this part needs to be changed in case other patterns will be added.
      return DesignFlowStep_Status::UNCHANGED;
   }
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, " --------- EXTRACT_PATTERNS ---------- ");
   const auto TM = AppM->get_tree_manager();
   const auto tn = TM->GetTreeNode(function_id);
   // tree_nodeRef Scpe = TM->GetTreeNode(function_id);
   const auto fd = GetPointer<const function_decl>(tn);
   THROW_ASSERT(fd && fd->body, "Node is not a function or it hasn't a body");
   auto sl = GetPointer<statement_list>(fd->body);
   THROW_ASSERT(sl, "Body is not a statement_list");

   /// for each basic block B in CFG do > Consider all blocks successively
   bool modified = false;
   for(const auto& bb_pair : sl->list_of_bloc)
   {
      const auto& B = bb_pair.second;
      const auto& B_id = B->number;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining BB" + STR(B_id));
      const auto list_of_stmt = B->CGetStmtList();
      /// manage capacity
      auto it_los = list_of_stmt.begin();
      auto it_los_end = list_of_stmt.end();
      while(it_los != it_los_end)
      {
         if(!AppM->ApplyNewTransformation())
         {
            break;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining statement " + (*it_los)->ToString());
         if((*it_los)->get_kind() == gimple_assign_K)
         {
            const auto ga = GetPointerS<const gimple_assign>(*it_los);
            const auto code0 = ga->op0->get_kind();
            const auto code1 = ga->op1->get_kind();
            if(code0 == ssa_name_K && (code1 == plus_expr_K || code1 == minus_expr_K))
            {
               if(!(tree_helper::IsRealType(ga->op0) || tree_helper::IsComplexType(ga->op0) ||
                    tree_helper::IsVectorType(ga->op0)))
               {
                  const auto ssa_defined = GetPointerS<const ssa_name>(ga->op0);
                  const auto ssa_defined_size = tree_helper::Size(tree_helper::CGetType(ga->op0));
                  const auto binop0 = GetPointerS<const binary_expr>(ga->op1);
                  if((ssa_defined->CGetNumberUses() == 1) &&
                     (ssa_defined_size == tree_helper::Size(tree_helper::CGetType(binop0->op0))) &&
                     (ssa_defined_size == tree_helper::Size(tree_helper::CGetType(binop0->op1))))
                  {
                     const auto statement_node = ssa_defined->CGetUseStmts().begin()->first;
                     if(statement_node->get_kind() == gimple_assign_K)
                     {
                        auto ga_dest = GetPointerS<gimple_assign>(statement_node);
                        const auto code_dest0 = ga_dest->op0->get_kind();
                        const auto code_dest1 = ga_dest->op1->get_kind();
                        const auto ssa_dest0_size = tree_helper::Size(tree_helper::CGetType(ga_dest->op0));
                        if(code_dest0 == ssa_name_K && (code_dest1 == plus_expr_K || code_dest1 == minus_expr_K) &&
                           ga_dest->bb_index == B_id && ssa_dest0_size == ssa_defined_size)
                        {
                           tree_manipulationRef IRman(new tree_manipulation(TM, parameters, AppM));
                           /// matched
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "---Ternary plus expr statement found ");
                           const auto srcp_default = ga_dest->include_name + ":" + STR(ga_dest->line_number) + ":" +
                                                     STR(ga_dest->column_number);
                           const auto binop_dest = GetPointerS<const binary_expr>(ga_dest->op1);
                           if(ga->op0->index == binop_dest->op0->index)
                           {
                              const auto ternary_op = IRman->create_ternary_operation(
                                  binop_dest->type, binop0->op0, binop0->op1, binop_dest->op1, srcp_default,
                                  ternary_operation_type0(code1, code_dest1));
                              TM->ReplaceTreeNode(statement_node, ga_dest->op1, ternary_op);
                           }
                           else
                           {
                              const auto ternary_op = IRman->create_ternary_operation(
                                  binop_dest->type, binop_dest->op0, binop0->op0, binop0->op1, srcp_default,
                                  ternary_operation_type1(code1, code_dest1));
                              TM->ReplaceTreeNode(statement_node, ga_dest->op1, ternary_op);
                           }
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "<--Statement removed " + (*it_los)->ToString());
                           B->RemoveStmt(*it_los, AppM);
                           it_los = list_of_stmt.begin();
                           it_los_end = list_of_stmt.end();
                           AppM->RegisterTransformation(GetName(), statement_node);
                           modified = true;
                           continue;
                        }
                     }
                  }
               }
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Statement analyzed " + (*it_los)->ToString());
         ++it_los;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined BB" + STR(B_id));
   }

   if(modified)
   {
      function_behavior->UpdateBBVersion();
   }
   return modified ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}
