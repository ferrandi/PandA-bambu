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
 *              Copyright (C) 2021 Politecnico di Milano
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
 * @file FunctionCallInline.cpp
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#include "FunctionCallInline.hpp"

#include "application_manager.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

#include "Parameter.hpp"
#include "dbgPrintHelper.hpp"      // for DEBUG_LEVEL_
#include "string_manipulation.hpp" // for GET_CLASS

CustomSet<unsigned int> FunctionCallInline::always_inline;
CustomMap<unsigned int, CustomSet<unsigned int>> FunctionCallInline::inline_call;

FunctionCallInline::FunctionCallInline(const ParameterConstRef Param, const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, FUNCTION_CALL_INLINE, _design_flow_manager, Param)
{
   debug_level = Param->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);

   // TODO: add parameter to define functions to be inlined
}

FunctionCallInline::~FunctionCallInline() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> FunctionCallInline::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(FUNCTION_CALL_INLINE, CALLED_FUNCTIONS));
         relationships.insert(std::make_pair(COMPLETE_CALL_GRAPH, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(FIX_STRUCTS_PASSED_BY_VALUE, CALLED_FUNCTIONS));
         relationships.insert(std::make_pair(FUNCTION_CALL_TYPE_CLEANUP, SAME_FUNCTION));
         relationships.insert(std::make_pair(IR_LOWERING, SAME_FUNCTION));
         relationships.insert(std::make_pair(USE_COUNTING, SAME_FUNCTION));
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(REMOVE_CLOBBER_GA, SAME_FUNCTION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         if(GetStatus() == DesignFlowStep_Status::SUCCESS)
         {
            relationships.insert(std::make_pair(COMPLETE_CALL_GRAPH, WHOLE_APPLICATION));
            // relationships.insert(std::make_pair(DEAD_CODE_ELIMINATION, SAME_FUNCTION));
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

bool FunctionCallInline::HasToBeExecuted() const
{
   return !parameters->IsParameter("no-inline") && FunctionFrontendFlowStep::HasToBeExecuted0() && FunctionFrontendFlowStep::HasToBeExecuted();
}

DesignFlowStep_Status FunctionCallInline::InternalExec()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
   bool need_execution = static_cast<bool>(inline_call.count(function_id));
   if(!need_execution)
   {
      for(const auto& called_id : AppM->CGetCallGraphManager()->get_called_by(function_id))
      {
         if(static_cast<bool>(always_inline.count(called_id)))
         {
            need_execution = true;
            break;
         }
      }
   }
   if(!need_execution)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Nothing to inline");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      return DesignFlowStep_Status::UNCHANGED;
   }

   const auto TM = AppM->get_tree_manager();
   const auto fd = GetPointerS<function_decl>(TM->GetTreeNode(function_id));
   const auto sl = GetPointerS<statement_list>(GET_NODE(fd->body));

   CustomSet<unsigned int> inline_stmts;
   for(const auto& ibb : sl->list_of_bloc)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Analysing BB" + STR(ibb.first));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
      const auto& bb = ibb.second;
      for(const auto& stmt : bb->CGetStmtList())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Analysing statement " + GET_CONST_NODE(stmt)->ToString());
         const auto stmt_kind = GET_CONST_NODE(stmt)->get_kind();
         tree_nodeRef fn;
         if(stmt_kind == gimple_call_K)
         {
            const auto gc = GetPointerS<const gimple_call>(GET_CONST_NODE(stmt));
            fn = gc->fn;
         }
         else if(stmt_kind == gimple_assign_K)
         {
            const auto ga = GetPointerS<const gimple_assign>(GET_CONST_NODE(stmt));
            const auto ce = GetPointer<const call_expr>(GET_CONST_NODE(ga->op1));
            if(ce)
            {
               fn = ce->fn;
            }
         }
         if(fn)
         {
            if(GET_CONST_NODE(fn)->get_kind() == addr_expr_K)
            {
               fn = GetPointerS<const addr_expr>(GET_CONST_NODE(fn))->op;
            }
            if(static_cast<bool>(always_inline.count(GET_INDEX_CONST_NODE(fn))))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Inlining required for this call statement");
               inline_stmts.insert(GET_INDEX_CONST_NODE(stmt));
            }
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Analysed BB" + STR(ibb.first));
   }
   const auto call_stmts = inline_call.find(function_id);
   if(call_stmts != inline_call.end())
   {
      inline_stmts.insert(call_stmts->second.cbegin(), call_stmts->second.cend());
   }
   if(inline_stmts.empty())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      return DesignFlowStep_Status::UNCHANGED;
   }
   tree_manipulationRef tree_man(new tree_manipulation(TM, parameters));
   const auto CGM = AppM->GetCallGraphManager();
   for(const auto& stmt_id : inline_stmts)
   {
      const auto stmt = TM->CGetTreeReindex(stmt_id);
      if(stmt)
      {
         const auto gn = GetPointerS<const gimple_node>(GET_CONST_NODE(stmt));
         const auto bb_it = sl->list_of_bloc.find(gn->bb_index);
         if(gn->bb_index != 0 && bb_it != sl->list_of_bloc.end())
         {
            const auto& bb = bb_it->second;
            if(std::find_if(bb->CGetStmtList().cbegin(), bb->CGetStmtList().cend(), [&](const tree_nodeRef& tn) { return GET_INDEX_CONST_NODE(tn) == stmt_id; }) != bb->CGetStmtList().cend())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Inlining required for call statement " + GET_CONST_NODE(stmt)->ToString());
               tree_man->InlineFunctionCall(stmt, bb, fd, CGM);
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Statement " + STR(stmt_id) + " was not present in BB" + STR(bb->number));
            }
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "BB" + STR(gn->bb_index) + " was not found in current function");
         }
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Statement " + STR(stmt_id) + " was not in tree manager");
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   inline_call.erase(function_id);
   return DesignFlowStep_Status::SUCCESS;
}