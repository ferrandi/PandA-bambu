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
 * @file remove_ending_if.cpp
 * @brief Collapse and if and its "then" branch only if it is shorter than one cycle and the else is a BB composed only by return and PHI statements.
 *
 * Complete description:
 * This step collapses an if block and its "then" branch only if the branch lasts one cycle and doesn't contain store and call. Furthermore,
 * the BB following the if should be the last BB (preceding the EXTI) and has to contain only a return statement ad a PHI statement, also the "then" BB
 * has to be linked to this block.
 *                                    |
 *                             _ _ _ _| _ _ _                                             _ _ _ _ _ _ _ _
 *                            |              |                                           |               |
 *                            |    IF BLOC   |                                           |IF + THEN STMTs|
 *                            |_ _ _ _ _ _ _ |                                           |_ _ _ _ _ _ _ _|
 *                                /       \                                                      |
 *                               /         \                                                     |
 *                              /           \                    TRANSFORMATION                  |
 *                     _ _ _ _ /_ _ _        \                                            _ _ _ _|_ _ _ _
 *                    |              |       /                    ------->               |               |
 *                    | THEN BRANCH  |      /                                            |     RETURN    |
 *                    |_ _ _ _ _ _ _ |     /                                             |_ _ _ _ _ _ _ _|
 *                            \           /
 *                             \         /
 *                              \       /
 *                            _ _\_ _ _/_ _
 *                           |             |
 *                           | RETURN & PHI|
 *                           |_ _ _ _ _ _ _|
 *
 * Macro idea:
 *    1. Find the pattern
 *    2. Manipulation
 *
 *Find the pattern = chain of if condition
 *    1. Looking for a bloc that is connected to the EXIT bloc and that has exactly 2 incoming arcs
 *    2. If 1 -> Check if one of the 2 blocks contains an IF stmt and identify it.
 *    3. If 2 -> Check if the identified IF has exactly 2 succ and one is the bloc found in 1 and the other has only one pred and one succ and the succ is the bloc found in 1
 *    4. If 3 -> Check if the "then" branch doesn't contain call or store statement
 *    5. If 4 -> pattern found and the bloc of the "then" branch is identified, also the "if" bloc is identified
 *    6. Compute how much lasts the "then" branch
 *    7. If it lasts less then one cycle then the manipulation phase is enabled
 *
 *Manipulation
 *    For each stmt in the bloc identified as "then" branch:
 *       1. Delete the current stmt in the bloc
 *       2. Move the current stmt in the "if" bloc
 *       3. Update the scheduling for the moved stmt
 *    The void bloc will be removed by other steps.
 *
 * @author Marco Arnaboldi <marco1.arnaboldi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

/// Header include
#include "remove_ending_if.hpp"

///. include
#include "Parameter.hpp"

/// src/algorithms/graph_helpers include
#include "cyclic_topological_sort.hpp"

/// behavior includes
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"

/// design_flows include
#include "design_flow_manager.hpp"

/// hls includes
#include "hls.hpp"
#include "hls_constraints.hpp"
#include "hls_manager.hpp"
#include "hls_step.hpp"

/// HLS/module_allocation include
#include "allocation_information.hpp"

#if HAVE_BAMBU_BUILT
/// hls/scheduling includes
#include "schedule.hpp"
#endif

/// parser/treegcc include
#include "token_interface.hpp"

/// STD include
#include <cstdlib>
#include <fstream>

/// STL include
#include "custom_set.hpp"
#include <cstdlib>

/// tree includes
#include "behavioral_helper.hpp"
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "ext_tree_node.hpp"
#include "string_manipulation.hpp" // for GET_CLASS
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "tree_reindex.hpp"

/// Constructor implementation
RemoveEndingIf::RemoveEndingIf(const ParameterConstRef _parameters, const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, REMOVE_ENDING_IF, _design_flow_manager, _parameters), sl(nullptr), schedule(ScheduleRef())
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

RemoveEndingIf::~RemoveEndingIf() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> RemoveEndingIf::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(SWITCH_FIX, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(BLOCK_FIX, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(USE_COUNTING, SAME_FUNCTION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         if(design_flow_manager.lock()->GetStatus(GetSignature()) == DesignFlowStep_Status::SUCCESS)
         {
            relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(PHI_OPT, SAME_FUNCTION));
         }
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(PHI_OPT, SAME_FUNCTION));
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}

void RemoveEndingIf::Initialize()
{
   TM = AppM->get_tree_manager();
   tree_man = tree_manipulationRef(new tree_manipulation(TM, parameters));
   const auto temp = TM->get_tree_node_const(function_id);
   auto fd = GetPointer<function_decl>(temp);
   sl = GetPointer<statement_list>(GET_NODE(fd->body));
#if HAVE_ILP_BUILT
   if(parameters->getOption<HLSFlowStep_Type>(OPT_scheduling_algorithm) == HLSFlowStep_Type::SDC_SCHEDULING and GetPointer<const HLS_manager>(AppM) and GetPointer<const HLS_manager>(AppM)->get_HLS(function_id) and
      GetPointer<const HLS_manager>(AppM)->get_HLS(function_id)->Rsch)
   {
      schedule = GetPointer<const HLS_manager>(AppM)->get_HLS(function_id)->Rsch;
   }
#endif
}

bool RemoveEndingIf::HasToBeExecuted() const
{
   /// If no schedule exists, this step has NOT to be executed
#if HAVE_ILP_BUILT
   if(parameters->getOption<HLSFlowStep_Type>(OPT_scheduling_algorithm) == HLSFlowStep_Type::SDC_SCHEDULING and GetPointer<const HLS_manager>(AppM) and GetPointer<const HLS_manager>(AppM)->get_HLS(function_id) and
      GetPointer<const HLS_manager>(AppM)->get_HLS(function_id)->Rsch)
   {
      return FunctionFrontendFlowStep::HasToBeExecuted();
   }
#endif
   return false;
}

DesignFlowStep_Status RemoveEndingIf::InternalExec()
{
   bool bb_modified = false;
   const auto HLS = GetPointer<const HLS_manager>(AppM)->get_HLS(function_id);
   const auto clock_period = HLS->HLS_C->get_clock_period();
   const auto clock_period_margin = HLS->allocation_information->GetClockPeriodMargin();
   const auto net_clock_period = clock_period - clock_period_margin;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Remove ending if is starting");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Looking for feasible blocs");
   for(auto block : sl->list_of_bloc)
   {
#ifndef NDEBUG
      if(not AppM->ApplyNewTransformation())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Skipping remaining transformations because of reached limit of cfg transformations");
         break;
      }
#endif
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Checking if a bloc has just 2 incoming and 1 outcoming");
      // The block should have 2 inc arcs from if and then BB and one outgoing to EXIT
      if(block.second->list_of_succ.size() == 1 and block.second->list_of_pred.size() == 2)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Checking if the successor is the EXIT bloc");
         const auto successor_id = block.second->list_of_succ.front();
         // Control on the successor
         if(successor_id == bloc::EXIT_BLOCK_ID)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---It is the EXIT");
            // Control if the BB has just the return stmt
            if(block.second->CGetStmtList().size() == 1)
            {
               auto last_stmt = GET_NODE(block.second->CGetStmtList().back());
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---The block has just 1 statement");
               if(last_stmt->get_kind() == gimple_return_K)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Feasible block found");
                  auto block_pred1 = *(sl->list_of_bloc.find(block.second->list_of_pred.front()));
                  auto block_pred2 = *(sl->list_of_bloc.find(block.second->list_of_pred.back()));
                  // Check if the two pred of block are the BB of the searched pattern
                  // The BB containing IF should has: 2 outgoing arcs, one into "block"
                  // and one into bloc_pred2
                  const std::pair<blocRef, blocRef> if_succ = [&]() -> std::pair<blocRef, blocRef> {
                     const blocRef if_block = [&]() -> blocRef {
                        /// The basic block containing the if must have two successors
                        if(block_pred1.second->list_of_succ.size() == 2 and block_pred1.second->loop_id == 0)
                        {
                           return block_pred1.second;
                        }
                        if(block_pred2.second->list_of_succ.size() == 2 and block_pred2.second->loop_id == 0)
                        {
                           return block_pred2.second;
                        }
                        return blocRef();
                     }();
                     if(not if_block)
                     {
                        return std::pair<blocRef, blocRef>(blocRef(), blocRef());
                     }
                     const auto dep_block = block_pred1.first == if_block->number ? block_pred2.second : block_pred1.second;
                     if(dep_block->list_of_pred.size() != 1 or dep_block->list_of_succ.size() != 1 or dep_block->list_of_pred.front() != if_block->number)
                     {
                        return std::pair<blocRef, blocRef>(blocRef(), blocRef());
                     }
                     return std::pair<blocRef, blocRef>(if_block, dep_block);
                  }();
                  if(if_succ.first and if_succ.second)
                  {
                     const auto if_block = if_succ.first;
                     const auto dep_block = if_succ.second;
                     const bool to_be_removed = [&]() -> bool {
                        if(dep_block->CGetStmtList().empty())
                        {
                           return false;
                        }
                        double min = std::numeric_limits<double>::max();
                        double max = 0.0;
                        for(const auto& stmt : dep_block->CGetStmtList())
                        {
                           if(schedule->GetStartingTime(stmt->index) < min)
                           {
                              min = schedule->GetStartingTime(stmt->index);
                           }
                           if(schedule->GetEndingTime(stmt->index) > max)
                           {
                              max = schedule->GetEndingTime(stmt->index);
                           }
                           if(GET_NODE(stmt)->get_kind() == gimple_call_K or (GetPointer<const gimple_assign>(GET_NODE(stmt)) and (GET_NODE(GetPointer<const gimple_assign>(GET_NODE(stmt))->op1)->get_kind() == call_expr_K ||
                                                                                                                                   GET_NODE(GetPointer<const gimple_assign>(GET_NODE(stmt))->op1)->get_kind() == aggr_init_expr_K)))
                           {
                              return false;
                           }
                           if(GetPointer<const gimple_node>(GET_NODE(stmt))->vdef)
                           {
                              return false;
                           }
                        }
                        if((max - min) > net_clock_period)
                        {
                           return false;
                        }
                        return true;
                     }();
                     // Remove from the "then" BB and add to the "if" BB
                     if(to_be_removed)
                     {
                        while(not dep_block->CGetStmtList().empty())
                        {
                           auto current_stmt = dep_block->CGetStmtList().front();
                           dep_block->RemoveStmt(current_stmt);
                           if_block->PushBack(current_stmt);
                        }

                        bb_modified = true;
#ifndef NDEBUG
                        AppM->RegisterTransformation(GetName(), last_stmt);
#endif
                     }
                  }
                  else
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---pattern NOT found");
                  }
               }
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---The block has more than 1 statement");
            }
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---It is not the EXIT");
         }
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Skipped this bloc, because has more or less incoming/outcoming arcs than expected");
         continue;
      }
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Remove ending if has finished");

   bb_modified ? function_behavior->UpdateBBVersion() : 0;
   return bb_modified ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}
