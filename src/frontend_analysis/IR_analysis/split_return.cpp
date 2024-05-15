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
 * @file split_return.cpp
 * @brief Simple transformations that remove almost empty basic blocks having a single phi and a return statement.
 *        The transformation remove this BB by propagating back the return statement.
 *
 * @author Hamidreza Hanafi <hamidreza.hanafi@mail.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#include "split_return.hpp"
#include "Parameter.hpp"
#include "application_manager.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"
#include "phi_opt.hpp"
#if HAVE_ILP_BUILT
#include "hls_step.hpp"
#endif
#include "custom_set.hpp"
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "ext_tree_node.hpp"
#include "string_manipulation.hpp" // for GET_CLASS
#include "token_interface.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "tree_node.hpp"
#include <fstream>

SplitReturn::SplitReturn(const ParameterConstRef _parameters, const application_managerRef _AppM,
                         unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, SPLIT_RETURN, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

SplitReturn::~SplitReturn() = default;

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
SplitReturn::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(BLOCK_FIX, SAME_FUNCTION));
         relationships.insert(std::make_pair(SWITCH_FIX, SAME_FUNCTION));
         relationships.insert(std::make_pair(USE_COUNTING, SAME_FUNCTION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(CSE_STEP, SAME_FUNCTION));
         relationships.insert(std::make_pair(REMOVE_CLOBBER_GA, SAME_FUNCTION));
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}

bool SplitReturn::HasToBeExecuted() const
{
   if(parameters->IsParameter("disable-SplitReturn") && parameters->GetParameter<int>("disable-SplitReturn") == 1)
   {
      return false;
   }
#if HAVE_ILP_BUILT
   if(parameters->isOption(OPT_scheduling_algorithm) &&
      parameters->getOption<HLSFlowStep_Type>(OPT_scheduling_algorithm) == HLSFlowStep_Type::SDC_SCHEDULING)
   {
      return GetPointer<const HLS_manager>(AppM) && GetPointer<const HLS_manager>(AppM)->get_HLS(function_id) &&
             GetPointer<const HLS_manager>(AppM)->get_HLS(function_id)->Rsch &&
             FunctionFrontendFlowStep::HasToBeExecuted();
   }
   else
#endif
   {
      return FunctionFrontendFlowStep::HasToBeExecuted();
   }
}

DesignFlowStep_Status SplitReturn::InternalExec()
{
   const auto TM = AppM->get_tree_manager();
   const auto tree_man = tree_manipulationRef(new tree_manipulation(TM, parameters, AppM));
   const auto f_node = TM->GetTreeNode(function_id);
   const auto ret_type = tree_helper::GetFunctionReturnType(f_node);
   const auto fd = GetPointerS<const function_decl>(f_node);
   const auto sl = GetPointerS<statement_list>(fd->body);

   const auto create_return_and_fix_cfg = [&](const tree_nodeRef& new_gr, const blocRef& pred_block,
                                              const blocRef& curr_bb) -> void {
      const auto bb_index = curr_bb->number;
      if(pred_block->list_of_succ.size() == 1)
      {
         pred_block->PushBack(new_gr, AppM);
         pred_block->list_of_succ.erase(
             std::find(pred_block->list_of_succ.begin(), pred_block->list_of_succ.end(), bb_index));
         pred_block->list_of_succ.push_back(curr_bb->list_of_succ.front());
      }
      else
      {
         const auto new_basic_block_index = (sl->list_of_bloc.rbegin())->first + 1;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Created BB" + STR(new_basic_block_index) + " as new successor of BB" +
                            STR(pred_block->number));
         const auto new_block = blocRef(new bloc(new_basic_block_index));
         sl->list_of_bloc[new_basic_block_index] = new_block;
         new_block->loop_id = curr_bb->loop_id;
         new_block->SetSSAUsesComputed();
         new_block->schedule = curr_bb->schedule;
         new_block->PushBack(new_gr, AppM);
         /// Add predecessor as pred basic block
         new_block->list_of_pred.push_back(pred_block->number);
         /// Add successor as succ basic block
         new_block->list_of_succ.push_back(curr_bb->list_of_succ.front());
         /// Fix successor of predecessor
         pred_block->list_of_succ.erase(
             std::find(pred_block->list_of_succ.begin(), pred_block->list_of_succ.end(), bb_index));
         pred_block->list_of_succ.push_back(new_basic_block_index);
         if(pred_block->true_edge == bb_index)
         {
            pred_block->true_edge = new_basic_block_index;
         }
         if(pred_block->false_edge == bb_index)
         {
            pred_block->false_edge = new_basic_block_index;
         }
         /// Fix gimple_multi_way_if
         if(pred_block->CGetStmtList().size())
         {
            const auto pred_last_stmt = pred_block->CGetStmtList().back();
            if(pred_last_stmt->get_kind() == gimple_multi_way_if_K)
            {
               const auto gmwi = GetPointerS<gimple_multi_way_if>(pred_last_stmt);
               for(auto& cond : gmwi->list_of_cond)
               {
                  if(cond.second == bb_index)
                  {
                     cond.second = new_basic_block_index;
                  }
               }
            }
         }
      }
   };

   const auto list_of_bloc = sl->list_of_bloc;
   bool modified = false;
   for(const auto& bbi_bb : list_of_bloc)
   {
      auto& bb = bbi_bb.second;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "--- Considering BB" + STR(bb->number) + " " + STR(bb->CGetPhiList().size()) + " " +
                         STR(bb->CGetStmtList().size()));
      if(bb->list_of_pred.size() > 1 && bb->CGetPhiList().size() == 1 && bb->CGetStmtList().size() == 1)
      {
         const auto stmt = bb->CGetStmtList().front();
         if(stmt->get_kind() == gimple_return_K)
         {
            const auto bb_index = bb->number;
            const auto gp = GetPointerS<const gimple_phi>(bb->CGetPhiList().front());
            const auto gr = GetPointerS<const gimple_return>(stmt);
            if(gr->op && gp->res->index == gr->op->index)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "--- There is a split return possible at BB" + STR(bb_index));
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "--- Create return statement based of def edges");
               for(const auto& def_edge : gp->CGetDefEdgesList())
               {
                  const auto new_gr =
                      tree_man->create_gimple_return(ret_type, def_edge.first, function_id, BUILTIN_SRCP);
                  auto& pred_block = sl->list_of_bloc.at(def_edge.second);
                  create_return_and_fix_cfg(new_gr, pred_block, bb);
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Removing BB" + STR(bb_index));
               bb->RemovePhi(bb->CGetPhiList().front());
               bb->RemoveStmt(bb->CGetStmtList().front(), AppM);
               sl->list_of_bloc.erase(bb_index);
               modified = true;
            }
            else if(gp->virtual_flag)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "--- There is a split return possible at BB" + STR(bb_index));
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "--- Create return statement based of def edges");
               for(const auto& def_edge : gp->CGetDefEdgesList())
               {
                  const auto new_gr = tree_man->create_gimple_return(ret_type, gr->op, function_id, BUILTIN_SRCP);
                  GetPointerS<gimple_return>(new_gr)->AddVuse(def_edge.first);
                  const auto& pred_block = sl->list_of_bloc.at(def_edge.second);
                  create_return_and_fix_cfg(new_gr, pred_block, bb);
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Removing BB" + STR(bb_index));
               bb->RemovePhi(bb->CGetPhiList().front());
               bb->RemoveStmt(bb->CGetStmtList().front(), AppM);
               sl->list_of_bloc.erase(bb_index);
               modified = true;
            }
         }
      }
      else if(bb->list_of_pred.size() > 1 && bb->CGetPhiList().size() == 0 && bb->CGetStmtList().size() == 1)
      {
         const auto bb_index = bb->number;
         const auto stmt = bb->CGetStmtList().front();
         if(stmt->get_kind() == gimple_return_K)
         {
            const auto gr = GetPointerS<const gimple_return>(stmt);
            for(const auto& pred_block_index : bb->list_of_pred)
            {
               const auto& pred_block = sl->list_of_bloc.at(pred_block_index);
               const auto new_gr = tree_man->create_gimple_return(ret_type, gr->op, function_id, BUILTIN_SRCP);
               create_return_and_fix_cfg(new_gr, pred_block, bb);
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Removing BB" + STR(bb_index));
            bb->RemoveStmt(bb->CGetStmtList().front(), AppM);
            sl->list_of_bloc.erase(bb_index);
            modified = true;
         }
      }
   }

   if(modified)
   {
      function_behavior->UpdateBBVersion();
      return DesignFlowStep_Status::SUCCESS;
   }
   return DesignFlowStep_Status::UNCHANGED;
}
