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
 * @file phi_opt.cpp
 * @brief Analysis step that improves the IR w.r.t. phis
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "phi_opt.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "dbgPrintHelper.hpp"
#include "ext_tree_node.hpp"
#include "function_behavior.hpp"
#include "string_manipulation.hpp"
#include "token_interface.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "tree_node.hpp"

#if HAVE_ILP_BUILT
#include "allocation_information.hpp"
#include "hls.hpp"
#include "hls_constraints.hpp"
#include "hls_manager.hpp"
#include "hls_step.hpp"
#include "schedule.hpp"
#endif

#include <boost/range/adaptor/reversed.hpp>

#include <fstream>

PhiOpt::PhiOpt(const application_managerRef _AppM, unsigned int _function_id,
               const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters)
    : FunctionFrontendFlowStep(_AppM, _function_id, PHI_OPT, _design_flow_manager, _parameters), bb_modified(false)
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

PhiOpt::~PhiOpt() = default;

CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionFrontendFlowStep::FunctionRelationship>>
PhiOpt::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(BLOCK_FIX, SAME_FUNCTION));
         relationships.insert(std::make_pair(MULTI_WAY_IF, SAME_FUNCTION));
         relationships.insert(std::make_pair(SHORT_CIRCUIT_TAF, SAME_FUNCTION));
         relationships.insert(std::make_pair(SWITCH_FIX, SAME_FUNCTION));
         relationships.insert(std::make_pair(USE_COUNTING, SAME_FUNCTION));
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
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

void PhiOpt::Initialize()
{
   bb_modified = false;
   TM = AppM->get_tree_manager();
   tree_man = tree_manipulationConstRef(new tree_manipulation(TM, parameters, AppM));
   const auto fd = GetPointerS<const function_decl>(TM->GetTreeNode(function_id));
   sl = GetPointerS<statement_list>(fd->body);
#if HAVE_ILP_BUILT
   if(parameters->getOption<HLSFlowStep_Type>(OPT_scheduling_algorithm) == HLSFlowStep_Type::SDC_SCHEDULING &&
      GetPointer<const HLS_manager>(AppM) && GetPointer<const HLS_manager>(AppM)->get_HLS(function_id) and
      GetPointer<const HLS_manager>(AppM)->get_HLS(function_id)->Rsch)
   {
      schedule = GetPointerS<const HLS_manager>(AppM)->get_HLS(function_id)->Rsch;
   }
#endif
}

DesignFlowStep_Status PhiOpt::InternalExec()
{
   bool restart = true;

   /// remove dead PHIs
   while(restart)
   {
      restart = false;

      for(const auto& block : sl->list_of_bloc)
      {
         std::list<tree_nodeRef> phis_to_be_removed;
         for(const auto& phi : block.second->CGetPhiList())
         {
            const auto gp = GetPointer<const gimple_phi>(phi);
            const auto sn = GetPointer<const ssa_name>(gp->res);
            if(sn->CGetUseStmts().empty())
            {
               phis_to_be_removed.push_back(phi);
            }
         }
         for(const auto& phi : phis_to_be_removed)
         {
            if(AppM->ApplyNewTransformation())
            {
               AppM->RegisterTransformation(GetName(), tree_nodeConstRef());
               block.second->RemovePhi(phi);
               bb_modified = true;
               restart = true;
            }
         }
      }
   }

   auto removePhiOnly = [&]() {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Merging phis");
      /// Removed blocks composed only of phi
      CustomSet<unsigned int> blocks_to_be_removed;
      for(const auto& block : sl->list_of_bloc)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Checking BB" + STR(block.first));
         if(block.second->list_of_pred.size() >= 2 && block.second->CGetPhiList().size() &&
            block.second->CGetStmtList().empty())
         {
            const auto successor = block.second->list_of_succ.front();
            THROW_ASSERT(sl->list_of_bloc.count(successor), "");
            const auto succ_block = sl->list_of_bloc.at(successor);
            /// Check that two basic block do not have any common predecessor
            const bool common_predecessor = [&]() {
               for(auto predecessor : block.second->list_of_pred)
               {
                  if(std::find(succ_block->list_of_pred.begin(), succ_block->list_of_pred.end(), predecessor) !=
                     succ_block->list_of_pred.end())
                  {
                     return true;
                  }
               }
               return false;
            }();
            if(common_predecessor)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped because of common predecessor");
               continue;
            }

            // True if the variables defined in the first basic block are only used in the phi of the second basic block
            const bool only_phi_use = [&]() {
               /// Check that ssa defined by phi are used only once
               for(const auto& phi : block.second->CGetPhiList())
               {
                  const auto gp = GetPointer<const gimple_phi>(phi);
                  const auto sn = GetPointer<const ssa_name>(gp->res);
                  for(const auto& use : sn->CGetUseStmts())
                  {
                     const auto gn = GetPointer<const gimple_node>(use.first);
                     if(gn->get_kind() != gimple_phi_K)
                     {
                        return false;
                     }
                     if(gn->bb_index != successor)
                     {
                        return false;
                     }
                  }
               }
               return true;
            }();
            if(!only_phi_use)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "<--Skipped because not used only in the second phi");
               continue;
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Added to the basic block to be merged");
            blocks_to_be_removed.insert(block.first);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      }
      for(auto block_to_be_removed : blocks_to_be_removed)
      {
         if(AppM->ApplyNewTransformation())
         {
            AppM->RegisterTransformation(GetName(), tree_nodeConstRef());
            MergePhi(block_to_be_removed);
            if(debug_level >= DEBUG_LEVEL_PEDANTIC &&
               (!parameters->IsParameter("print-dot-FF") || parameters->GetParameter<unsigned int>("print-dot-FF")))
            {
               WriteBBGraphDot("BB_During_" + GetName() + "_AfterMerge_BB" + STR(block_to_be_removed) + ".dot");
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Written BB_During_" + GetName() + "_AfterMerge_BB" + STR(block_to_be_removed) +
                                  ".dot");
            }
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Merged phis");
   };
   removePhiOnly();

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Removing single input phis");
   /// Transform single input phi
   for(const auto& block : sl->list_of_bloc)
   {
      if(block.second->list_of_pred.size() == 1 && block.second->CGetPhiList().size())
      {
         if(AppM->ApplyNewTransformation())
         {
            AppM->RegisterTransformation(GetName(), tree_nodeConstRef());
            SinglePhiOptimization(block.first);
            bb_modified = true;
         }
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Removed single input phis");
   if(debug_level >= DEBUG_LEVEL_PEDANTIC)
   {
      WriteBBGraphDot("BB_Removed_Single_Input_Phis_" + GetName() + "_chain.dot");
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Removing chains of BBs");

   while(restart)
   {
      restart = false;
      /// Transform chain of basic blocks
      for(const auto& block : sl->list_of_bloc)
      {
         if(block.first == bloc::ENTRY_BLOCK_ID)
         {
            continue;
         }
         if(block.second->list_of_succ.size() == 1)
         {
            const auto succ_block = block.second->list_of_succ.front();
            THROW_ASSERT(sl->list_of_bloc.count(succ_block),
                         "Successor block BB" + STR(succ_block) + " does not exist");
            if(sl->list_of_bloc.at(succ_block)->list_of_pred.size() == 1)
            {
               if(AppM->ApplyNewTransformation())
               {
                  AppM->RegisterTransformation(GetName(), tree_nodeConstRef());
                  ChainOptimization(block.first);
                  bb_modified = true;
                  restart = true;
                  break;
               }
            }
         }
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Removed chains of BBs");

   if(debug_level >= DEBUG_LEVEL_PEDANTIC)
   {
      WriteBBGraphDot("BB_Removed_Chains_" + GetName() + "_chain.dot");
   }

   restart = true;
   while(restart)
   {
      bool removePhiOnlyP = false;
      restart = false;
      /// Workaround to avoid invalidation of pointer
      CustomSet<decltype(sl->list_of_bloc)::key_type> blocks_to_be_analyzed;
      for(const auto& block : sl->list_of_bloc)
      {
         blocks_to_be_analyzed.insert(block.first);
      }

      /// Remove empty basic block
      for(auto bloc_to_be_analyzed : blocks_to_be_analyzed)
      {
         THROW_ASSERT(sl->list_of_bloc.count(bloc_to_be_analyzed), "");
         const auto& block = sl->list_of_bloc.at(bloc_to_be_analyzed);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "-->Analyzing BB" + STR(block->number) + " Stmts= " + STR(block->CGetStmtList().size()) +
                            " Phis=" + STR(block->CGetPhiList().size()));

         /// Remove nop
         if(block->CGetStmtList().size() == 1 && block->CGetStmtList().front()->get_kind() == gimple_nop_K)
         {
            block->RemoveStmt(block->CGetStmtList().front(), AppM);
            bb_modified = true;
         }
         if(block->list_of_pred.size() >= 2 && block->CGetPhiList().size() && block->CGetStmtList().empty())
         {
            removePhiOnlyP = true;
         }

         if(block->CGetStmtList().size() || block->CGetPhiList().size())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Basic block is not empty");
            continue;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Basic block is empty");
         if(block->number == bloc::ENTRY_BLOCK_ID)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Basic block is Entry");
            continue;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Basic block is not Entry");
         if(block->number == bloc::EXIT_BLOCK_ID)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Basic block is Exit");
            continue;
         }
#if HAVE_PRAGMA_BUILT
         if(parameters->getOption<int>(OPT_gcc_openmp_simd))
         {
            if(block->list_of_pred.size() == 1)
            {
               const auto pred_block_id = block->list_of_pred.front();
               THROW_ASSERT(sl->list_of_bloc.count(pred_block_id), "");
               const auto pred_block = sl->list_of_bloc.at(pred_block_id);
               if(pred_block->loop_id != block->loop_id)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Basic block is Landing pad");
                  continue;
               }
            }
         }
#endif
         if(!AppM->ApplyNewTransformation())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Reached limit of cfg transformations");
            continue;
         }
         AppM->RegisterTransformation(GetName(), tree_nodeConstRef());
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Basic block is not Exit");
         if(debug_level >= DEBUG_LEVEL_PEDANTIC &&
            (!parameters->IsParameter("print-dot-FF") || parameters->GetParameter<unsigned int>("print-dot-FF")))
         {
            WriteBBGraphDot("BB_Before_" + GetName() + "_Before_BB" + STR(block->number) + ".dot");
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Written BB_Before_" + GetName() + "_Before_BB" + STR(block->number) + ".dot");
         }
         const auto pattern_type = IdentifyPattern(block->number);
         switch(pattern_type)
         {
            case PhiOpt_PatternType::GIMPLE_NOTHING:
            {
               ApplyGimpleNothing(block->number);
               bb_modified = true;
               break;
            }
            case PhiOpt_PatternType::DIFF_NOTHING:
            {
               ApplyDiffNothing(block->number);
               bb_modified = true;
               restart = true;
               break;
            }
            case PhiOpt_PatternType::IF_MERGE:
            {
               ApplyIfMerge(block->number);
               bb_modified = true;
               break;
            }
            case PhiOpt_PatternType::IF_NOTHING:
            {
               ApplyIfNothing(block->number);
               bb_modified = true;
               break;
            }
            case PhiOpt_PatternType::IF_REMOVE:
            {
               ApplyIfRemove(block->number);
               bb_modified = true;
               break;
            }
            case PhiOpt_PatternType::MULTI_MERGE:
            {
               ApplyMultiMerge(block->number);
               bb_modified = true;
               break;
            }
            case PhiOpt_PatternType::MULTI_NOTHING:
            {
               ApplyMultiNothing(block->number);
               bb_modified = true;
               break;
            }
            case PhiOpt_PatternType::MULTI_REMOVE:
            {
               ApplyMultiRemove(block->number);
               bb_modified = true;
               break;
            }
            case PhiOpt_PatternType::UNCHANGED:
            {
               break;
            }
            case PhiOpt_PatternType::UNKNOWN:
            {
               THROW_UNREACHABLE("Found an unknown pattern in CFG");
               break;
            }
            default:
            {
               THROW_UNREACHABLE("");
               break;
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Removed BB" + STR(block->number));
      }
      if(removePhiOnlyP)
      {
         removePhiOnly();
      }
   }

   TreeNodeSet ces_to_be_removed;

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Removing redundant cond_expr");
   for(const auto& block : sl->list_of_bloc)
   {
      for(const auto& statement : block.second->CGetStmtList())
      {
         const auto* ga = GetPointer<const gimple_assign>(statement);
         if(ga && ga->op1->get_kind() == cond_expr_K)
         {
            const auto* ce = GetPointer<const cond_expr>(ga->op1);
            if(ce && ce->op1->index == ce->op2->index)
            {
               ces_to_be_removed.insert(statement);
            }
         }
      }
   }
   if(!ces_to_be_removed.empty())
   {
      bb_modified = true;
   }
   for(const auto& ce_to_be_removed : ces_to_be_removed)
   {
      RemoveCondExpr(ce_to_be_removed);
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Removed redundant cond_expr");

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Removing chains of BBs");
   restart = true;

   while(restart)
   {
      restart = false;
      /// Transform chain of basic blocks
      for(const auto& block : sl->list_of_bloc)
      {
         if(block.first == bloc::ENTRY_BLOCK_ID)
         {
            continue;
         }
         if(block.second->list_of_succ.size() == 1)
         {
            const auto succ_block = block.second->list_of_succ.front();
            THROW_ASSERT(sl->list_of_bloc.count(succ_block),
                         "Successor block BB" + STR(succ_block) + " does not exist");
            if(sl->list_of_bloc.at(succ_block)->list_of_pred.size() == 1)
            {
               if(AppM->ApplyNewTransformation())
               {
                  AppM->RegisterTransformation(GetName(), tree_nodeConstRef());
                  ChainOptimization(block.first);
                  bb_modified = true;
                  restart = true;
                  break;
               }
            }
         }
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Removed chains of BB");

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Removing nop with virtual operands");
   for(const auto& block : sl->list_of_bloc)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing BB" + STR(block.first));
      TreeNodeSet to_be_removeds;
      for(const auto& stmt : block.second->CGetStmtList())
      {
         const auto gn = GetPointerS<gimple_node>(stmt);
         if(gn->get_kind() != gimple_nop_K || !gn->vdef ||
            (gn->vovers.find(gn->vdef) != gn->vovers.end() && gn->vovers.size() > 1) ||
            (gn->vovers.find(gn->vdef) == gn->vovers.end() && (!gn->vovers.empty())))
         {
            if(gn->get_kind() == gimple_nop_K)
            {
               INDENT_DBG_MEX(
                   DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                   "---skipped gimple nop " + STR(gn->index) + (!gn->vdef ? "NOVDEF " : "VDEF ") +
                       ((gn->vovers.find(gn->vdef) != gn->vovers.end() && gn->vovers.size() > 1) ? "cond1 " :
                                                                                                   "not cond1 ") +
                       ((gn->vovers.find(gn->vdef) == gn->vovers.end() && (!gn->vovers.empty())) ? "cond2 " :
                                                                                                   "not cond2 "));
            }
            continue;
         }
         if(AppM->ApplyNewTransformation())
         {
            const auto virtual_ssa = GetPointerS<ssa_name>(gn->vdef);
            THROW_ASSERT(virtual_ssa && virtual_ssa->virtual_flag, "unexpected condition");

            /// If there is only a single vuse replace vdef with vuse in all the uses of vdef
            if(gn->vuses.size() == 1)
            {
               const auto vuse = *(gn->vuses.begin());
               const auto uses = virtual_ssa->CGetUseStmts();
               for(const auto& use : uses)
               {
                  TM->ReplaceTreeNode(use.first, gn->vdef, vuse);
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Removing gimple nop " + STR(gn->index));
               to_be_removeds.insert(stmt);
               AppM->RegisterTransformation(GetName(), tree_nodeConstRef());
            }
            else
            {
               /// Check that all the uses are not in phi or not defining a self loop
               const auto canBeProp = [&]() -> bool {
                  for(const auto& use_stmt : virtual_ssa->CGetUseStmts())
                  {
                     if(use_stmt.first->get_kind() == gimple_phi_K)
                     {
                        return false;
                     }
                     if(use_stmt.first->index == stmt->index)
                     {
                        return false;
                     }
                  }
                  return true;
               }();
               if(canBeProp)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "-->Virtual ssa " + gn->vdef->ToString() + " not used in any phi");
                  ReplaceVirtualUses(gn->vdef, gn->vuses);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Removing gimple nop " + STR(gn->index));
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
                  to_be_removeds.insert(stmt);
                  AppM->RegisterTransformation(GetName(), tree_nodeConstRef());
               }
            }
         }
      }
      for(const auto& to_be_removed : to_be_removeds)
      {
         block.second->RemoveStmt(to_be_removed, AppM);
      }
      if(!to_be_removeds.empty())
      {
         bb_modified = true;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed BB" + STR(block.first));
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Removed nop with virtual operands");
   bb_modified ? function_behavior->UpdateBBVersion() : 0;
   return bb_modified ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}

void PhiOpt::ApplyDiffNothing(const unsigned int bb_index)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Duplicating empty basic block");
   const auto& curr_block = sl->list_of_bloc.at(bb_index);
   const auto& succ_block = sl->list_of_bloc.at(curr_block->list_of_succ.front());
   THROW_ASSERT(std::find(succ_block->list_of_pred.begin(), succ_block->list_of_pred.end(), bb_index) !=
                    succ_block->list_of_pred.end(),
                "bb_index not included in the list of pred: " + STR(bb_index) + " succ: " + STR(succ_block->number));
   succ_block->list_of_pred.erase(
       std::find(succ_block->list_of_pred.begin(), succ_block->list_of_pred.end(), bb_index));

   CustomSet<unsigned int> created_bbs;

   for(auto pred : curr_block->list_of_pred)
   {
      const auto& pred_block = sl->list_of_bloc.at(pred);

      /// Create empty basic block
      const auto new_basic_block_index = (sl->list_of_bloc.rbegin())->first + 1;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "-->Created BB" + STR(new_basic_block_index) + " as new successor of BB" + STR(pred));
      created_bbs.insert(new_basic_block_index);
      const auto new_block = blocRef(new bloc(new_basic_block_index));
      sl->list_of_bloc[new_basic_block_index] = new_block;

      new_block->loop_id = curr_block->loop_id;
      new_block->SetSSAUsesComputed();
      new_block->schedule = pred_block->schedule;

      /// Add predecessor as pred basic block
      new_block->list_of_pred.push_back(pred);

      /// Add successor as succ basic block
      new_block->list_of_succ.push_back(succ_block->number);

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

      /// Fix predecessor of successor
      succ_block->list_of_pred.push_back(new_basic_block_index);

      /// Fix gimple_multi_way_if
      if(pred_block->CGetStmtList().size())
      {
         auto pred_last_stmt = pred_block->CGetStmtList().back();
         if(pred_last_stmt->get_kind() == gimple_multi_way_if_K)
         {
            const auto gmwi = GetPointerS<gimple_multi_way_if>(pred_last_stmt);
            for(auto& cond : gmwi->list_of_cond)
            {
               if(cond.second == curr_block->number)
               {
                  cond.second = new_basic_block_index;
               }
            }
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Fixing phis");
   /// Fix phis
   for(const auto& phi : succ_block->CGetPhiList())
   {
      const auto gp = GetPointerS<gimple_phi>(phi);
      gimple_phi::DefEdgeList new_list_of_def_edge;
      auto curr_value = tree_nodeRef();

      for(auto& def : gp->CGetDefEdgesList())
      {
         if(def.second != curr_block->number)
         {
            new_list_of_def_edge.push_back(def);
         }
         else
         {
            curr_value = def.first;
         }
      }
      for(auto pred : created_bbs)
      {
         new_list_of_def_edge.push_back(decltype(new_list_of_def_edge)::value_type(curr_value, pred));
      }
      gp->SetDefEdgeList(TM, new_list_of_def_edge);
   }
   sl->list_of_bloc.erase(bb_index);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Duplicated BB" + STR(bb_index));
}

void PhiOpt::ApplyIfMerge(const unsigned int bb_index)
{
   const auto& curr_block = sl->list_of_bloc.at(bb_index);
   const auto& pred_block = sl->list_of_bloc.at(curr_block->list_of_pred.front());
   const auto& succ_block = sl->list_of_bloc.at(curr_block->list_of_succ.front());

   const auto pred_stmt_list = pred_block->CGetStmtList();

   /// True if bb_index is on the true edge
   const auto true_edge = pred_block->true_edge == bb_index;

   /// The condition
   const auto condition = tree_man->ExtractCondition(pred_stmt_list.back(), pred_block, function_id);

   /// Remove gimple_cond from list of statement
   pred_block->RemoveStmt(pred_stmt_list.back(), AppM);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Condition is " + condition->ToString());

   /// Update all the phis
   for(const auto& phi : succ_block->CGetPhiList())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Modifying " + phi->ToString());
      /// The value coming from true edge
      tree_nodeRef true_value = nullptr;

      /// The value coming from false edge
      tree_nodeRef false_value = nullptr;

      const auto gp = GetPointerS<gimple_phi>(phi);

      /// The type of the expression
      const auto type_node = tree_helper::CGetType(gp->res);

      for(const auto& def : gp->CGetDefEdgesList())
      {
         if((true_edge && bb_index == def.second) || (!true_edge && pred_block->number == def.second))
         {
            THROW_ASSERT(!true_value, "True value already found");
            true_value = def.first;
         }
         else if((!true_edge && bb_index == def.second) || (true_edge && pred_block->number == def.second))
         {
            THROW_ASSERT(!false_value, "True value already found");
            false_value = def.first;
         }
      }
      THROW_ASSERT(true_value, "True value not found");
      THROW_ASSERT(false_value, "False value not found");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---From true edge " + true_value->ToString());
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---From false edge " + false_value->ToString());

      tree_nodeRef gimple_node;

      /// Create the ssa with the new input of the phi
      tree_nodeRef var = nullptr;
      if(true_value->get_kind() == ssa_name_K && false_value->get_kind() == ssa_name_K)
      {
         const auto sn1 = GetPointerS<const ssa_name>(true_value);
         const auto sn2 = GetPointerS<const ssa_name>(false_value);
         if(sn1->var && sn2->var && sn1->var->index == sn2->var->index)
         {
            var = sn1->var;
         }
      }
      const auto gp_res = GetPointer<const ssa_name>(gp->res);
      const auto ssa_node =
          tree_man->create_ssa_name(var, type_node, gp_res->min, gp_res->max, false, gp->virtual_flag);
      GetPointerS<ssa_name>(ssa_node)->bit_values = gp_res->bit_values;
      if(gp->virtual_flag)
      {
         /// Create a nop with virtual operands
         std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> gimple_nop_schema;
         gimple_nop_schema[TOK(TOK_SRCP)] = BUILTIN_SRCP;
         gimple_nop_schema[TOK(TOK_SCPE)] = STR(function_id);
         const auto gimple_node_id = TM->new_tree_node_id();
         TM->create_tree_node(gimple_node_id, gimple_nop_K, gimple_nop_schema);
         gimple_node = TM->GetTreeNode(gimple_node_id);
         const auto gn = GetPointer<gimple_nop>(gimple_node);
         gn->SetVdef(ssa_node);
         gn->AddVuse(true_value);
         gn->AddVuse(false_value);
      }
      else
      {
         /// Create the cond expr
         auto condition_type = tree_helper::CGetType(condition);
         auto isAVectorType = tree_helper::is_a_vector(TM, condition_type->index);
         const auto cond_expr_node =
             tree_man->create_ternary_operation(type_node, condition, true_value, false_value, BUILTIN_SRCP,
                                                (isAVectorType ? vec_cond_expr_K : cond_expr_K));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created cond_expr " + cond_expr_node->ToString());

         /// Create the assign
         gimple_node = tree_man->create_gimple_modify_stmt(ssa_node, cond_expr_node, function_id, BUILTIN_SRCP);
      }
      pred_block->PushBack(gimple_node, AppM);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + gimple_node->ToString());

      /// Updating the phi
      gimple_phi::DefEdgeList new_list_of_def_edge;
      for(const auto& def : gp->CGetDefEdgesList())
      {
         if(def.second == pred_block->number)
         {
            new_list_of_def_edge.push_back(gimple_phi::DefEdge(ssa_node, pred_block->number));
         }
         else if(def.second == bb_index)
         {
            /// Do nothing - this edge will be removed
         }
         else
         {
            new_list_of_def_edge.push_back(def);
         }
      }
      gp->SetDefEdgeList(TM, new_list_of_def_edge);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Modified " + phi->ToString());
   }

   /// Refactoring of the cfg - updating the predecessor
   pred_block->list_of_succ.clear();
   pred_block->true_edge = 0;
   pred_block->false_edge = 0;
   pred_block->list_of_succ.push_back(succ_block->number);

   /// Refactoring of the cfg - updating the successor
   succ_block->list_of_pred.erase(
       std::find(succ_block->list_of_pred.begin(), succ_block->list_of_pred.end(), bb_index));

   /// Remove the current basic block
   sl->list_of_bloc.erase(bb_index);
}

void PhiOpt::ApplyIfNothing(const unsigned int bb_index)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Removing empty basic block");
   const auto& curr_block = sl->list_of_bloc.at(bb_index);
   const auto& pred_block = sl->list_of_bloc.at(curr_block->list_of_pred.front());
   const auto& succ_block = sl->list_of_bloc.at(curr_block->list_of_succ.front());
   for(const auto& phi : succ_block->CGetPhiList())
   {
      const auto gp = GetPointerS<gimple_phi>(phi);
      for(auto& def_edge : gp->CGetDefEdgesList())
      {
         if(def_edge.second == curr_block->number)
         {
            gp->ReplaceDefEdge(TM, def_edge, gimple_phi::DefEdge(def_edge.first, pred_block->number));
            break;
         }
      }
   }

   /// Refactoring of the cfg - updating the predecessor
   if(pred_block->true_edge == bb_index)
   {
      pred_block->true_edge = succ_block->number;
   }
   else if(pred_block->false_edge == bb_index)
   {
      pred_block->false_edge = succ_block->number;
   }
   else
   {
      THROW_UNREACHABLE("");
   }
   pred_block->list_of_succ.clear();
   pred_block->list_of_succ.push_back(pred_block->true_edge);
   pred_block->list_of_succ.push_back(pred_block->false_edge);

   /// Refactoring of the cfg - updating the successor
   succ_block->list_of_pred.erase(
       std::find(succ_block->list_of_pred.begin(), succ_block->list_of_pred.end(), bb_index));
   succ_block->list_of_pred.push_back(pred_block->number);

   /// Remove the current basic block
   sl->list_of_bloc.erase(bb_index);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Removed BB" + STR(bb_index));
}

void PhiOpt::ApplyGimpleNothing(const unsigned int bb_index)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Removing empty basic block");
   const auto& curr_block = sl->list_of_bloc.at(bb_index);
   const auto& pred_block = sl->list_of_bloc.at(curr_block->list_of_pred.front());
   const auto& succ_block = sl->list_of_bloc.at(curr_block->list_of_succ.front());
   for(const auto& phi : succ_block->CGetPhiList())
   {
      const auto gp = GetPointer<gimple_phi>(phi);
      for(auto& def_edge : gp->CGetDefEdgesList())
      {
         if(def_edge.second == curr_block->number)
         {
            gp->ReplaceDefEdge(TM, def_edge, gimple_phi::DefEdge(def_edge.first, pred_block->number));
            break;
         }
      }
   }

   /// Refactoring of the cfg - updating the predecessor
   pred_block->list_of_succ.erase(
       std::find(pred_block->list_of_succ.begin(), pred_block->list_of_succ.end(), bb_index));
   pred_block->list_of_succ.push_back(succ_block->number);

   /// Refactoring of the cfg - updating the successor
   succ_block->list_of_pred.erase(
       std::find(succ_block->list_of_pred.begin(), succ_block->list_of_pred.end(), bb_index));
   succ_block->list_of_pred.push_back(pred_block->number);

   /// Remove the current basic block
   sl->list_of_bloc.erase(bb_index);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Removed BB" + STR(bb_index));
}

void PhiOpt::ApplyIfRemove(const unsigned int bb_index)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Removing phis dominated by if");
   const auto& curr_block = sl->list_of_bloc.at(bb_index);
   const auto& pred_block = sl->list_of_bloc.at(curr_block->list_of_pred.front());
   const auto& succ_block = sl->list_of_bloc.at(curr_block->list_of_succ.front());

   /// True if bb_index is on the true edge
   const auto true_edge = pred_block->true_edge == bb_index;

   /// Remove gimple_cond from list of statement. New statements can be pushed at the end
   THROW_ASSERT(pred_block->CGetStmtList().size(), "Statement list of predecessor is empty");

   /// The condition
   const auto last_stmt = pred_block->CGetStmtList().back();
   const auto condition = tree_man->ExtractCondition(last_stmt, pred_block, function_id);
   pred_block->RemoveStmt(last_stmt, AppM);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Condition is " + condition->ToString());

   /// Remove all the phis
   for(const auto& phi : succ_block->CGetPhiList())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Removing " + phi->ToString());
      /// The value coming from true edge
      tree_nodeRef true_value = nullptr;

      /// The value coming from false edge
      tree_nodeRef false_value = nullptr;

      const auto gp = GetPointerS<gimple_phi>(phi);

      /// The type of the expression
      const auto type_node = tree_helper::CGetType(gp->res);

      for(const auto& def_edge : gp->CGetDefEdgesList())
      {
         if((true_edge && bb_index == def_edge.second) || (!true_edge && pred_block->number == def_edge.second))
         {
            THROW_ASSERT(!true_value, "True value already found");
            true_value = def_edge.first;
         }
         else if((!true_edge && bb_index == def_edge.second) || (true_edge && pred_block->number == def_edge.second))
         {
            THROW_ASSERT(!false_value, "False value already found");
            false_value = def_edge.first;
         }
         else
         {
            THROW_UNREACHABLE(std::string(true_edge ? "BB is on the true edge" : "BB is on the false edge") +
                              " - Condition comes from BB" + STR(def_edge.second) + " - If basic block is BB" +
                              STR((pred_block->number)));
         }
      }
      THROW_ASSERT(true_value, "True value not found");
      THROW_ASSERT(false_value, "False value not found");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---From true edge " + true_value->ToString());
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---From false edge " + false_value->ToString());

      if(gp->virtual_flag)
      {
         const auto virtual_ssa = GetPointerS<ssa_name>(gp->res);
         bool create_gimple_nop = false;
         for(const auto& use_stmt : virtual_ssa->CGetUseStmts())
         {
            if(use_stmt.first->get_kind() == gimple_phi_K)
            {
               create_gimple_nop = true;
            }
         }
         if(create_gimple_nop)
         {
            const auto gimple_node_id = TM->new_tree_node_id();
            { /// Create a nop with virtual operands
               std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> gimple_nop_schema;
               gimple_nop_schema[TOK(TOK_SRCP)] = BUILTIN_SRCP;
               gimple_nop_schema[TOK(TOK_SCPE)] = STR(function_id);
               TM->create_tree_node(gimple_node_id, gimple_nop_K, gimple_nop_schema);
            }
            const auto gn = GetPointerS<gimple_nop>(TM->GetTreeNode(gimple_node_id));
            gn->SetVdef(gp->res);
            gn->AddVuse(true_value);
            gn->AddVuse(false_value);
            succ_block->PushFront(TM->GetTreeNode(gimple_node_id), AppM);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Created " + gn->ToString());
         }
         else
         {
            TreeNodeSet new_ssas;
            for(const auto& def_edge : gp->CGetDefEdgesList())
            {
               new_ssas.insert(def_edge.first);
            }
            ReplaceVirtualUses(gp->res, new_ssas);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Substituted vuses ");
         }
      }
      else
      {
         /// Create the cond expr
         auto condition_type = tree_helper::CGetType(condition);
         auto isAVectorType = tree_helper::is_a_vector(TM, condition_type->index);
         const auto cond_expr_node =
             tree_man->create_ternary_operation(type_node, condition, true_value, false_value, BUILTIN_SRCP,
                                                (isAVectorType ? vec_cond_expr_K : cond_expr_K));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created cond_expr " + cond_expr_node->ToString());

         /// Create the assign
         const auto gimple_node =
             tree_man->create_gimple_modify_stmt(gp->res, cond_expr_node, function_id, BUILTIN_SRCP);
         succ_block->PushFront(gimple_node, AppM);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Created " + gimple_node->ToString());
      }
   }

   while(succ_block->CGetPhiList().size())
   {
      succ_block->RemovePhi(succ_block->CGetPhiList().front());
   }

   /// Refactoring of the cfg - updating the predecessor
   pred_block->list_of_succ.clear();
   pred_block->true_edge = 0;
   pred_block->false_edge = 0;
   pred_block->list_of_succ.push_back(succ_block->number);

   /// Refactoring of the cfg - updating the successor
   succ_block->list_of_pred.erase(
       std::find(succ_block->list_of_pred.begin(), succ_block->list_of_pred.end(), bb_index));

   /// Remove the current basic block
   sl->list_of_bloc.erase(bb_index);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Removed phis");
}

void PhiOpt::ApplyMultiMerge(const unsigned int bb_index)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Removing BB" + STR(bb_index));
   const auto& curr_block = sl->list_of_bloc.at(bb_index);
   const auto& pred_block = sl->list_of_bloc.at(curr_block->list_of_pred.front());
   const auto& succ_block = sl->list_of_bloc.at(curr_block->list_of_succ.front());

   auto& pred_stmt_list = pred_block->CGetStmtList();

   /// True if bb_index is on the first edge
   auto first_edge = false;

   /// The gimple multi way if
   THROW_ASSERT(pred_stmt_list.back()->get_kind() == gimple_multi_way_if_K, "");
   const auto gmwi_node = pred_stmt_list.back();
   const auto gmwi = GetPointerS<gimple_multi_way_if>(gmwi_node);

   /// Temporary remove gimple multi way if
   pred_block->RemoveStmt(pred_stmt_list.back(), AppM);

   /// The first condition
   auto first_condition = std::pair<tree_nodeRef, unsigned int>(tree_nodeRef(), 0);

   /// The second condition
   auto second_condition = std::pair<tree_nodeRef, unsigned int>(tree_nodeRef(), 0);

   for(const auto& cond : gmwi->list_of_cond)
   {
      if(cond.second == curr_block->number)
      {
         if(!first_condition.first)
         {
            first_condition = cond;
            first_edge = true;
         }
         else
         {
            second_condition = cond;
            break;
         }
      }
      else if(cond.second == succ_block->number)
      {
         if(!first_condition.first)
         {
            first_condition = cond;
            first_edge = false;
         }
         else
         {
            second_condition = cond;
            break;
         }
      }
   }

   const auto new_cond = [&]() -> unsigned int {
      if(second_condition.first)
      {
         const auto new_node =
             tree_man->CreateOrExpr(first_condition.first, second_condition.first, pred_block, function_id);
         return new_node->index;
      }
      else
      {
         return 0;
      }
   }();

   decltype(gmwi->list_of_cond) new_list_of_cond;
   for(const auto& cond : gmwi->list_of_cond)
   {
      if(cond == first_condition)
      {
         if(second_condition.first)
         {
            new_list_of_cond.push_front(gimple_phi::DefEdge(TM->GetTreeNode(new_cond), succ_block->number));
         }
         else
         {
            new_list_of_cond.push_back(gimple_phi::DefEdge(tree_nodeRef(), succ_block->number));
         }
      }
      else if(cond != second_condition)
      {
         if(cond.first)
         {
            new_list_of_cond.push_front(cond);
         }
         else
         {
            new_list_of_cond.push_back(cond);
         }
      }
   }
   gmwi->list_of_cond = new_list_of_cond;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---New gimple multi way if " + gmwi->ToString());

   /// Update all the phis
   for(const auto& phi : succ_block->CGetPhiList())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Modifying phi " + phi->ToString());
      /// The value coming from the first edge
      tree_nodeRef first_value = nullptr;

      /// The value coming from the second edge
      tree_nodeRef second_value = nullptr;

      auto gp = GetPointer<gimple_phi>(phi);

      /// The type of the expression
      const auto type_node = tree_helper::CGetType(gp->res);

      for(const auto& def_edge : gp->CGetDefEdgesList())
      {
         if((first_edge && bb_index == def_edge.second) || (!first_edge && pred_block->number == def_edge.second))
         {
            first_value = def_edge.first;
         }
         else if((!first_edge && bb_index == def_edge.second) || (first_edge && pred_block->number == def_edge.second))
         {
            second_value = def_edge.first;
         }
      }

      /// Create the ssa with the new input of the phi
      tree_nodeRef var;
      if(first_value->get_kind() == ssa_name_K && second_value->get_kind() == ssa_name_K)
      {
         const auto sn1 = GetPointer<const ssa_name>(first_value);
         const auto sn2 = GetPointer<const ssa_name>(second_value);
         if(sn1->var && sn2->var && sn1->var->index == sn2->var->index)
         {
            var = sn1->var;
         }
      }
      const auto gp_res = GetPointer<const ssa_name>(gp->res);
      const auto ssa_node =
          tree_man->create_ssa_name(var, type_node, gp_res->min, gp_res->max, false, gp->virtual_flag);
      GetPointer<ssa_name>(ssa_node)->bit_values = gp_res->bit_values;
      tree_nodeRef gimple_node;
      if(gp->virtual_flag)
      {
         /// Create a nop with virtual operands
         std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> gimple_nop_schema;
         gimple_nop_schema[TOK(TOK_SRCP)] = BUILTIN_SRCP;
         gimple_nop_schema[TOK(TOK_SCPE)] = STR(function_id);
         const auto gimple_node_id = TM->new_tree_node_id();
         TM->create_tree_node(gimple_node_id, gimple_nop_K, gimple_nop_schema);
         gimple_node = TM->GetTreeNode(gimple_node_id);
         auto gn = GetPointer<gimple_nop>(gimple_node);
         gn->SetVdef(ssa_node);
         gn->AddVuse(first_value);
         gn->AddVuse(second_value);
      }
      else
      {
         /// Create the cond expr
         auto condition_type = tree_helper::CGetType(first_condition.first);
         auto isAVectorType = tree_helper::is_a_vector(TM, condition_type->index);
         const auto cond_expr_node =
             tree_man->create_ternary_operation(type_node, first_condition.first, first_value, second_value,
                                                BUILTIN_SRCP, (isAVectorType ? vec_cond_expr_K : cond_expr_K));

         /// Create the assign
         gimple_node = tree_man->create_gimple_modify_stmt(ssa_node, cond_expr_node, function_id, BUILTIN_SRCP);
      }
      pred_block->PushBack(gimple_node, AppM);

      /// Updating the phi
      gimple_phi::DefEdgeList new_list_of_def_edge;
      for(const auto& def_edge : gp->CGetDefEdgesList())
      {
         if(def_edge.second == pred_block->number)
         {
            new_list_of_def_edge.push_back(gimple_phi::DefEdge(ssa_node, pred_block->number));
         }
         else if(def_edge.second == bb_index)
         {
            /// Do nothing - this edge will be removed
         }
         else
         {
            new_list_of_def_edge.push_back(def_edge);
         }
      }
      gp->SetDefEdgeList(TM, new_list_of_def_edge);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Modified phi " + phi->ToString());
   }

   /// Readding gimple multi way if it has more than two exits
   if(gmwi->list_of_cond.size() >= 2)
   {
      pred_block->PushBack(gmwi_node, AppM);
   }

   /// Refactoring of the cfg - updating the predecessor
   pred_block->list_of_succ.erase(
       std::find(pred_block->list_of_succ.begin(), pred_block->list_of_succ.end(), bb_index));

   /// Refactoring of the cfg - updating the successor
   succ_block->list_of_pred.erase(
       std::find(succ_block->list_of_pred.begin(), succ_block->list_of_pred.end(), bb_index));

   /// Remove the current basic block
   sl->list_of_bloc.erase(bb_index);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Removed BB" + STR(bb_index));
}

void PhiOpt::ApplyMultiNothing(const unsigned int bb_index)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Removing empty basic block");
   const auto& curr_block = sl->list_of_bloc.at(bb_index);
   const auto& pred_block = sl->list_of_bloc.at(curr_block->list_of_pred.front());
   const auto& succ_block = sl->list_of_bloc.at(curr_block->list_of_succ.front());

   THROW_ASSERT(pred_block->CGetStmtList().back()->get_kind() == gimple_multi_way_if_K, "");
   const auto gmwi = GetPointerS<gimple_multi_way_if>(pred_block->CGetStmtList().back());
   for(auto& cond : gmwi->list_of_cond)
   {
      if(cond.second == bb_index)
      {
         cond.second = succ_block->number;
         break;
      }
   }

   for(const auto& phi : succ_block->CGetPhiList())
   {
      const auto gp = GetPointerS<gimple_phi>(phi);
      for(auto& def_edge : gp->CGetDefEdgesList())
      {
         if(def_edge.second == curr_block->number)
         {
            gp->ReplaceDefEdge(TM, def_edge, gimple_phi::DefEdge(def_edge.first, pred_block->number));
            break;
         }
      }
   }

   /// Refactoring of the cfg - updating the predecessor
   pred_block->list_of_succ.erase(
       std::find(pred_block->list_of_succ.begin(), pred_block->list_of_succ.end(), bb_index));
   pred_block->list_of_succ.push_back(succ_block->number);

   /// Refactoring of the cfg - updating the successor
   succ_block->list_of_pred.erase(
       std::find(succ_block->list_of_pred.begin(), succ_block->list_of_pred.end(), bb_index));
   succ_block->list_of_pred.push_back(pred_block->number);

   /// Remove the current basic block
   sl->list_of_bloc.erase(bb_index);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Removed BB" + STR(bb_index));
}

void PhiOpt::ApplyMultiRemove(const unsigned int bb_index)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Removing BB" + STR(bb_index));
   const auto& curr_block = sl->list_of_bloc.at(bb_index);
   const auto& pred_block = sl->list_of_bloc.at(curr_block->list_of_pred.front());
   const auto& succ_block = sl->list_of_bloc.at(curr_block->list_of_succ.front());

   const auto& pred_stmt_list = pred_block->CGetStmtList();

   /// True if bb_index is on the first edge
   auto first_edge = false;

   /// The gimple multi way if
   THROW_ASSERT(pred_stmt_list.back()->get_kind() == gimple_multi_way_if_K, "");
   const auto gmwi_node = pred_stmt_list.back();
   const auto gmwi = GetPointerS<gimple_multi_way_if>(gmwi_node);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Multi way if is " + gmwi->ToString());

   /// Temporary remove gimple multi way if
   pred_block->RemoveStmt(pred_stmt_list.back(), AppM);

   /// The first condition
   auto first_condition = std::pair<tree_nodeRef, unsigned int>(tree_nodeRef(), 0);

   /// The second condition
   auto second_condition = std::pair<tree_nodeRef, unsigned int>(tree_nodeRef(), 0);

   for(const auto& cond : gmwi->list_of_cond)
   {
      if(cond.second == curr_block->number)
      {
         if(!first_condition.first)
         {
            first_condition = cond;
            first_edge = true;
         }
         else
         {
            second_condition = cond;
            break;
         }
      }
      else if(cond.second == succ_block->number)
      {
         if(!first_condition.first)
         {
            first_condition = cond;
            first_edge = false;
         }
         else
         {
            second_condition = cond;
            break;
         }
      }
   }
   THROW_ASSERT(first_condition.first, "First condition is empty");
   const auto new_cond = [&]() -> tree_nodeRef {
      if(second_condition.first)
      {
         const auto new_node =
             tree_man->CreateOrExpr(first_condition.first, second_condition.first, pred_block, function_id);
         return new_node;
      }
      return nullptr;
   }();

   decltype(gmwi->list_of_cond) new_list_of_cond;
   for(const auto& cond : gmwi->list_of_cond)
   {
      if(cond == first_condition)
      {
         if(second_condition.first)
         {
            new_list_of_cond.push_front(gimple_phi::DefEdge(new_cond, succ_block->number));
         }
         else
         {
            new_list_of_cond.push_back(gimple_phi::DefEdge(nullptr, succ_block->number));
         }
      }
      else if(cond != second_condition)
      {
         if(cond.first)
         {
            new_list_of_cond.push_front(cond);
         }
         else
         {
            new_list_of_cond.push_back(cond);
         }
      }
   }
   gmwi->list_of_cond = new_list_of_cond;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Rewritten gimple multi way if as " + gmwi->ToString());

   /// Remove all the phis
   for(const auto& phi : succ_block->CGetPhiList())
   {
      /// The value coming from the first edge
      tree_nodeRef first_value = nullptr;

      /// The value coming from the second edge
      tree_nodeRef second_value = nullptr;

      const auto gp = GetPointerS<gimple_phi>(phi);

      /// The type of the expression
      const auto type_node = tree_helper::CGetType(gp->res);

      for(const auto& def_edge : gp->CGetDefEdgesList())
      {
         if((first_edge && bb_index == def_edge.second) || (!first_edge && pred_block->number == def_edge.second))
         {
            first_value = def_edge.first;
         }
         else if((!first_edge && bb_index == def_edge.second) || (first_edge && pred_block->number == def_edge.second))
         {
            second_value = def_edge.first;
         }
         else
         {
            THROW_UNREACHABLE("");
         }
      }
      tree_nodeRef new_gimple_node = nullptr;
      bool create_gimple_nop = false;
      if(gp->virtual_flag)
      {
         const auto virtual_ssa = GetPointerS<ssa_name>(gp->res);
         for(const auto& use_stmt : virtual_ssa->CGetUseStmts())
         {
            if(use_stmt.first->get_kind() == gimple_phi_K)
            {
               create_gimple_nop = true;
            }
         }
         if(create_gimple_nop)
         {
            /// Create a nop with virtual operands
            const auto gimple_node_id = TM->new_tree_node_id();
            std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> gimple_nop_schema;
            gimple_nop_schema[TOK(TOK_SRCP)] = BUILTIN_SRCP;
            gimple_nop_schema[TOK(TOK_SCPE)] = STR(function_id);
            TM->create_tree_node(gimple_node_id, gimple_nop_K, gimple_nop_schema);
            new_gimple_node = TM->GetTreeNode(gimple_node_id);
            const auto gn = GetPointerS<gimple_nop>(new_gimple_node);
            gn->SetVdef(gp->res);
            gn->AddVuse(first_value);
            gn->AddVuse(second_value);
         }
         else
         {
            TreeNodeSet new_ssas;
            new_ssas.insert(first_value);
            new_ssas.insert(second_value);
            ReplaceVirtualUses(gp->res, new_ssas);
         }
      }
      else
      {
         /// Create the cond expr
         auto condition_type = tree_helper::CGetType(first_condition.first);
         auto isAVectorType = tree_helper::is_a_vector(TM, condition_type->index);
         const auto cond_expr_node =
             tree_man->create_ternary_operation(type_node, first_condition.first, first_value, second_value,
                                                BUILTIN_SRCP, (isAVectorType ? vec_cond_expr_K : cond_expr_K));

         /// Create the assign
         new_gimple_node = tree_man->create_gimple_modify_stmt(gp->res, cond_expr_node, function_id, BUILTIN_SRCP);
      }
      if(!gp->virtual_flag || create_gimple_nop)
      {
         succ_block->PushFront(new_gimple_node, AppM);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Added gimple assignment " + new_gimple_node->ToString());
      }
   }

   /// Readd multi way if
   if(gmwi->list_of_cond.size() >= 2)
   {
      pred_block->PushBack(gmwi_node, AppM);
   }

   while(succ_block->CGetPhiList().size())
   {
      succ_block->RemovePhi(succ_block->CGetPhiList().front());
   }

   /// Refactoring of the cfg - updating the predecessor
   pred_block->list_of_succ.erase(
       std::find(pred_block->list_of_succ.begin(), pred_block->list_of_succ.end(), bb_index));

   /// Refactoring of the cfg - updating the successor
   succ_block->list_of_pred.clear();
   succ_block->list_of_pred.push_back(pred_block->number);

   /// Remove the current basic block
   sl->list_of_bloc.erase(bb_index);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Removed BB" + STR(bb_index));
}

PhiOpt_PatternType PhiOpt::IdentifyPattern(const unsigned int bb_index) const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Identifying pattern starting from BB" + STR(bb_index));
   const auto& curr_block = sl->list_of_bloc.at(bb_index);
   if(curr_block->list_of_pred.size() == 1 && curr_block->list_of_pred.front() == bloc::ENTRY_BLOCK_ID)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Empty basic block connected to Entry");
      return PhiOpt_PatternType::UNCHANGED;
   }
   if(std::find(curr_block->list_of_succ.begin(), curr_block->list_of_succ.end(), bb_index) !=
      curr_block->list_of_succ.end())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Sink BB");
      return PhiOpt_PatternType::UNCHANGED;
   }
   if(curr_block->CGetStmtList().empty() && curr_block->list_of_pred.size() == 2 &&
      curr_block->list_of_succ.size() == 1)
   {
      const auto succ_bbi = curr_block->list_of_succ.front();
      const auto loop_bb = std::find(curr_block->list_of_pred.begin(), curr_block->list_of_pred.end(), succ_bbi);
      const auto infinite_empty_loop =
          sl->list_of_bloc.at(succ_bbi)->CGetStmtList().empty() && loop_bb != curr_block->list_of_pred.end();
      if(infinite_empty_loop)
      {
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                        "Infinite empty loop is present in the code in function " +
                            function_behavior->CGetBehavioralHelper()->GetMangledFunctionName());
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Infinite empty loop");
         return PhiOpt_PatternType::UNCHANGED;
      }
   }
   if(curr_block->list_of_pred.size() != 1)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Multiple empty paths");
      return PhiOpt_PatternType::DIFF_NOTHING;
   }
   const auto& succ_block = sl->list_of_bloc.at(curr_block->list_of_succ.front());
   THROW_ASSERT(succ_block->number != bloc::EXIT_BLOCK_ID, "");
   const auto& pred_block = sl->list_of_bloc.at(curr_block->list_of_pred.front());
   const auto phi_size = succ_block->list_of_pred.size();
   for(const auto& phi : succ_block->CGetPhiList())
   {
      const auto gp = GetPointerS<const gimple_phi>(phi);
      if(phi_size != gp->CGetDefEdgesList().size())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "<--Unknown because successor contain phi " + STR(phi) +
                            " with different size: " + STR(phi_size) + " vs. " + STR(gp->CGetDefEdgesList().size()));
         return PhiOpt_PatternType::UNKNOWN;
      }
   }
   if(std::find(pred_block->list_of_succ.begin(), pred_block->list_of_succ.end(), succ_block->number) ==
      pred_block->list_of_succ.end())
   {
      if(pred_block->list_of_succ.size() == 1)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "<--Empty basic block with predecessor with single Exit");
         return PhiOpt_PatternType::GIMPLE_NOTHING;
      }
      if(pred_block->CGetStmtList().size())
      {
         const auto pred_last_stmt = pred_block->CGetStmtList().back();
         if(pred_last_stmt->get_kind() == gimple_cond_K)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "<--Basic block dominated by if which can be removed");
            return PhiOpt_PatternType::IF_NOTHING;
         }
         if(pred_last_stmt->get_kind() == gimple_multi_way_if_K)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "<--Basic block dominated by multi way if which can be removed");
            return PhiOpt_PatternType::MULTI_NOTHING;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "<--Unknown because it can be removed but it is controlled by " +
                            pred_last_stmt->get_kind_text());
         return PhiOpt_PatternType::UNKNOWN;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "<--Unknown because it can be removed but predecessor is empty");
      return PhiOpt_PatternType::UNKNOWN;
   }
   if(phi_size == 0)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Unknown because it does not contain phi");
      return PhiOpt_PatternType::UNKNOWN;
   }
   if(phi_size == 1)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Unknown because of single phi");
      return PhiOpt_PatternType::UNKNOWN;
   }
   if(phi_size == 2)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Phis with size 2");
      if(pred_block->CGetStmtList().empty())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Unknown because predecessor is empty");
         return PhiOpt_PatternType::UNKNOWN;
      }
      const auto pred_last_stmt = pred_block->CGetStmtList().back();
      if(pred_last_stmt->get_kind() == gimple_cond_K)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Empty then or empty else");
         return PhiOpt_PatternType::IF_REMOVE;
      }
      if(pred_last_stmt->get_kind() == gimple_multi_way_if_K)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Controlled by multi way if");
         if(schedule)
         {
            /// The gimple multi way if
            const auto gmwi = GetPointer<const gimple_multi_way_if>(pred_last_stmt);

            /// The conditions
            auto first_condition = tree_nodeRef();
            auto second_condition = tree_nodeRef();

            for(const auto& cond : gmwi->list_of_cond)
            {
               if(cond.second == curr_block->number || cond.second == succ_block->number)
               {
                  if(!first_condition)
                  {
                     first_condition = cond.first;
                  }
                  else
                  {
                     second_condition = cond.first;
                  }
               }
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---First condition is " + (first_condition ? first_condition->ToString() : "default"));
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Second condition is " + (second_condition ? second_condition->ToString() : "default"));
            if(!first_condition || !second_condition ||
               schedule->EvaluateCondsMerging(pred_last_stmt->index, first_condition->index, second_condition->index,
                                              function_id))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Empty path to phi to be removed");
               return PhiOpt_PatternType::MULTI_REMOVE;
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "<--New gimple multi way if would increase basic block latency");
               return PhiOpt_PatternType::UNCHANGED;
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Empty path to phi to be removed");
         return PhiOpt_PatternType::MULTI_REMOVE;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "<--Unknown because dominator of phi is " + pred_last_stmt->ToString());
      return PhiOpt_PatternType::UNKNOWN;
   }
   if(pred_block->CGetStmtList().empty())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Unknown because predecessor is empty");
      return PhiOpt_PatternType::UNKNOWN;
   }
   const auto pred_last_stmt = pred_block->CGetStmtList().back();
   if(pred_last_stmt->get_kind() == gimple_cond_K)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Empty nested then || empty else");
      return PhiOpt_PatternType::IF_MERGE;
   }
   if(pred_last_stmt->get_kind() == gimple_multi_way_if_K)
   {
      /// Successor is ending if of the function
      if(succ_block->CGetStmtList().size() == 1 && GetPointer<const gimple_return>(succ_block->CGetStmtList().front()))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Empty path to phi to be merged");
         return PhiOpt_PatternType::MULTI_MERGE;
      }
      if(schedule)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "-->Potentially is a path to phi to be merged. Checking timing");
         /// Simulate to add the cond expr and check if there are problems with timing

         /// The gimple multi way if
         THROW_ASSERT(pred_block->CGetStmtList().back()->get_kind() == gimple_multi_way_if_K, "");
         const auto gmwi = GetPointerS<const gimple_multi_way_if>(pred_block->CGetStmtList().back());

         /// The first condition
         auto condition = tree_nodeRef();

         for(const auto& cond : gmwi->list_of_cond)
         {
            if(cond.second == curr_block->number || cond.second == succ_block->number)
            {
               condition = cond.first;
               break;
            }
         }
         THROW_ASSERT(condition, "Condition not found");
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Condition is " + condition->ToString());

         const auto& list_of_phi = succ_block->CGetPhiList();
         for(const auto& phi : list_of_phi)
         {
            /// The value coming from the first edge
            tree_nodeRef first_value = nullptr;

            /// The value coming from the second edge
            tree_nodeRef second_value = nullptr;

            /// True if bb_index is on the first edge
            auto first_edge = false;

            const auto gp = GetPointerS<const gimple_phi>(phi);

            /// The type of the expression
            const auto type_node = tree_helper::CGetType(gp->res);

            for(const auto& def_edge : gp->CGetDefEdgesList())
            {
               if((first_edge && bb_index == def_edge.second) || (!first_edge && pred_block->number == def_edge.second))
               {
                  first_value = def_edge.first;
               }
               else if((!first_edge && bb_index == def_edge.second) ||
                       (first_edge && pred_block->number == def_edge.second))
               {
                  second_value = def_edge.first;
               }
            }

            /// Create the cond expr
            auto condition_type = tree_helper::CGetType(condition);
            auto isAVectorType = tree_helper::is_a_vector(TM, condition_type->index);
            const auto cond_expr_node =
                tree_man->create_ternary_operation(type_node, condition, first_value, second_value, BUILTIN_SRCP,
                                                   (isAVectorType ? vec_cond_expr_K : cond_expr_K));

            /// Create the assign
            /// Workaround: we need to consider the overhead due to multiplexers associated with the phi; for this
            /// reason definition is one of the operands; this is not fully consistent, but it is a temporary assignment
            const auto gimple_assign_node =
                tree_man->create_gimple_modify_stmt(first_value, cond_expr_node, function_id, BUILTIN_SRCP);

            /// Created statement is not added to the predecessor
            if(schedule && schedule->CanBeMoved(gimple_assign_node->index, pred_block->number) !=
                               FunctionFrontendFlowStep_Movable::MOVABLE)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
               INDENT_DBG_MEX(
                   DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                   "<--Empty path to phi to be merged, but modifying would increase the latency of predecessor");
               return PhiOpt_PatternType::UNCHANGED;
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Empty path to phi to be merged");
      return PhiOpt_PatternType::MULTI_MERGE;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "<--Unknown because dominator of phi is " + pred_last_stmt->ToString());
   return PhiOpt_PatternType::UNKNOWN;
}

void PhiOpt::SinglePhiOptimization(const unsigned int bb_index)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Removing phis of BB" + STR(bb_index));
   const auto curr_block = sl->list_of_bloc.at(bb_index);
   THROW_ASSERT(curr_block->list_of_pred.size() == 1, "Basic block with single phis but not a single predecessor");
   const auto pred_block = sl->list_of_bloc.at(curr_block->list_of_pred.front());
   for(const auto& phi : boost::adaptors::reverse(curr_block->CGetPhiList()))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Fixing using of ssa defined in " + phi->ToString());
      const auto gp = GetPointerS<const gimple_phi>(phi);
      const auto& left_part = gp->res;
      THROW_ASSERT(gp->CGetDefEdgesList().size() == 1, "");
      const auto right_part = gp->CGetDefEdgesList().front().first;
      const auto left_ssa = GetPointerS<const ssa_name>(gp->res);
      /// Building temp set of use stmts (to avoid invalidation during loop execution and to skip phi)
      TreeNodeSet use_stmts;
      for(const auto& use_stmt : left_ssa->CGetUseStmts())
      {
         if(use_stmt.first->index != gp->index)
         {
            use_stmts.insert(use_stmt.first);
         }
      }

      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "-->Replacing " + left_part->ToString() + " with " + right_part->ToString());
      for(const auto& use_stmt : use_stmts)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Before ssa replacement " + use_stmt->ToString());
         TM->ReplaceTreeNode(use_stmt, left_part, right_part);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---After ssa replacement " + use_stmt->ToString());
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Fixed using of ssa defined in " + phi->ToString());
   }
   while(curr_block->CGetPhiList().size())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---Removing " + curr_block->CGetPhiList().front()->ToString());
      curr_block->RemovePhi(curr_block->CGetPhiList().front());
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Removed phis of BB" + STR(bb_index));
}

void PhiOpt::ChainOptimization(const unsigned int bb_index)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "-->Applying chaining optimization starting from BB" + STR(bb_index));
   const auto& curr_block = sl->list_of_bloc.at(bb_index);
   const auto& succ_block = sl->list_of_bloc.at(curr_block->list_of_succ.front());

   /// The phis are taken from the first block
   THROW_ASSERT(succ_block->CGetPhiList().empty(), "Second element of the chain has phi");

   /// Move statement of second block in first one
   while(succ_block->CGetStmtList().size())
   {
      const auto statement = succ_block->CGetStmtList().front();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Moving " + STR(statement));
      succ_block->RemoveStmt(statement, AppM);
      curr_block->PushBack(statement, AppM);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Moved statement");

   /// Update successor of curr_bb
   curr_block->true_edge = succ_block->true_edge;
   curr_block->false_edge = succ_block->false_edge;
   curr_block->list_of_succ = succ_block->list_of_succ;

   /// Fix successor of succ
   for(auto succ_succ : succ_block->list_of_succ)
   {
      if(succ_succ != bloc::EXIT_BLOCK_ID)
      {
         const auto& succ_succ_block = sl->list_of_bloc.at(succ_succ);
         succ_succ_block->list_of_pred.erase(
             std::find(succ_succ_block->list_of_pred.begin(), succ_succ_block->list_of_pred.end(), succ_block->number));
         succ_succ_block->list_of_pred.push_back(curr_block->number);
         for(const auto& phi : succ_succ_block->CGetPhiList())
         {
            const auto gp = GetPointerS<gimple_phi>(phi);
            for(auto& def_edge : gp->CGetDefEdgesList())
            {
               if(def_edge.second == succ_block->number)
               {
                  gp->ReplaceDefEdge(TM, def_edge, gimple_phi::DefEdge(def_edge.first, curr_block->number));
                  break;
               }
            }
         }
      }
   }

   /// Remove bb
   sl->list_of_bloc.erase(succ_block->number);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Removed BB" + STR(succ_block->number));
}

void PhiOpt::MergePhi(const unsigned int bb_index)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Merging phi of BB" + STR(bb_index));
   const auto& curr_block = sl->list_of_bloc.at(bb_index);
   THROW_ASSERT(curr_block->list_of_succ.size() == 1,
                "BB" + STR(bb_index) + " has " + STR(curr_block->list_of_succ.size()));
   const auto& succ_block = sl->list_of_bloc.at(curr_block->list_of_succ.front());
   const auto& pred_succ_block = succ_block->list_of_pred;
   // This check has to be performed here since the structure of the basic block may be changed in the meanwhile
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Checking common predecessor");
   for(const auto& predecessor : curr_block->list_of_pred)
   {
      if(std::find(pred_succ_block.begin(), pred_succ_block.end(), predecessor) != pred_succ_block.end())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Found common predecessor BB" + STR(predecessor));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped merging phi of BB" + STR(bb_index));
         return;
      }
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Checked common predecessors");
   bb_modified = true;
   /// Fixing phis
   for(const auto& phi : succ_block->CGetPhiList())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Fixing " + phi->ToString());
      auto gp = GetPointerS<gimple_phi>(phi);
      gimple_phi::DefEdgeList new_list_of_def_edge;
      for(auto def_edge : gp->CGetDefEdgesList())
      {
         if(def_edge.second == bb_index)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Found " + def_edge.first->ToString() + " coming from BB" + STR(def_edge.second));
            const auto def_to_be_removed = def_edge.first;
            if(def_to_be_removed->get_kind() == ssa_name_K)
            {
               const auto def_stmt = GetPointerS<const ssa_name>(def_to_be_removed)->CGetDefStmt();
               const auto phi_to_be_removed = GetPointer<const gimple_phi>(def_stmt);
               if(phi_to_be_removed && phi_to_be_removed->bb_index == bb_index)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---" + def_edge.first->ToString() + " comes from a phi to be removed");
                  /// Removing phi only if number of uses is 1
                  if(GetPointerS<const ssa_name>(def_to_be_removed)->CGetNumberUses() == 1)
                  {
                     curr_block->RemovePhi(def_stmt);
                  }
                  for(const auto& curr_def_edge : phi_to_be_removed->CGetDefEdgesList())
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Adding from phi of predecessor <" + curr_def_edge.first->ToString() + ", BB" +
                                        STR(curr_def_edge.second) + ">");
                     new_list_of_def_edge.push_back(curr_def_edge);
                  }
               }
               else
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---" + def_edge.first->ToString() + " is defined in a " + def_stmt->get_kind_text() +
                                     " in BB" + STR(GetPointer<const gimple_node>(def_stmt)->bb_index));
                  for(auto predecessor : curr_block->list_of_pred)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Adding from predecessor <" + def_edge.first->ToString() + ", BB" +
                                        STR(predecessor) + ">");
                     new_list_of_def_edge.push_back(decltype(def_edge)(def_edge.first, predecessor));
                  }
               }
            }
            else
            {
               for(auto predecessor : curr_block->list_of_pred)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Adding from predecessor <" + def_edge.first->ToString() + ", BB" +
                                     STR(predecessor) + ">");
                  new_list_of_def_edge.push_back(decltype(def_edge)(def_edge.first, predecessor));
               }
            }
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Readding <" + def_edge.first->ToString() + ", BB" + STR(def_edge.second) + ">");
            new_list_of_def_edge.push_back(def_edge);
         }
      }
      gp->SetDefEdgeList(TM, new_list_of_def_edge);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Fixed phi is " + gp->ToString());
   }

   /// These are phis which were present only in the predecessor
   const auto curr_phis = curr_block->CGetPhiList();
   for(const auto& phi : curr_phis)
   {
      const auto gp = GetPointerS<gimple_phi>(phi);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Adding " + gp->ToString());
      for(auto predecessor : succ_block->list_of_pred)
      {
         if(predecessor != bb_index)
         {
            gp->AddDefEdge(TM, gimple_phi::DefEdge(gp->res, predecessor));
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Added modified " + gp->ToString());
      gp->bb_index = succ_block->number;
      curr_block->RemovePhi(phi);
      succ_block->AddPhi(phi);
   }

   /// Fixing predecessor
   for(auto predecessor : curr_block->list_of_pred)
   {
      const auto& pred_block = sl->list_of_bloc.at(predecessor);
      pred_block->list_of_succ.erase(
          std::find(pred_block->list_of_succ.begin(), pred_block->list_of_succ.end(), bb_index));
      pred_block->list_of_succ.push_back(succ_block->number);
      if(pred_block->true_edge == bb_index)
      {
         pred_block->true_edge = succ_block->number;
      }
      if(pred_block->false_edge == bb_index)
      {
         pred_block->false_edge = succ_block->number;
      }
      /// Fixing gimple phi of predecessor
      if(pred_block->CGetStmtList().size())
      {
         const auto& last_stmt = pred_block->CGetStmtList().back();
         if(last_stmt->get_kind() == gimple_multi_way_if_K)
         {
            const auto gmw = GetPointerS<gimple_multi_way_if>(last_stmt);
            for(auto& cond : gmw->list_of_cond)
            {
               if(cond.second == bb_index)
               {
                  cond.second = succ_block->number;
               }
            }
         }
      }
   }

   /// Fixing successor
   succ_block->list_of_pred.erase(
       std::find(succ_block->list_of_pred.begin(), succ_block->list_of_pred.end(), bb_index));
   std::copy(curr_block->list_of_pred.begin(), curr_block->list_of_pred.end(),
             std::back_inserter(succ_block->list_of_pred));

   /// Erasing basic block
   sl->list_of_bloc.erase(bb_index);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Merged phi of BB" + STR(bb_index));
}

void PhiOpt::RemoveCondExpr(const tree_nodeRef statement)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Removing " + statement->ToString());
   THROW_ASSERT(statement->get_kind() == gimple_assign_K, "");
   const auto ga = GetPointerS<const gimple_assign>(statement);
   const auto sn = GetPointer<const ssa_name>(ga->op0);
   THROW_ASSERT(sn, "cond expression defines " + ga->op0->ToString());
   THROW_ASSERT(ga->op1->get_kind() == cond_expr_K, "");
   const auto new_sn = GetPointerS<const cond_expr>(ga->op1)->op1;
   const auto uses = sn->CGetUseStmts();
   for(const auto& use : uses)
   {
      TM->ReplaceTreeNode(use.first, ga->op0, new_sn);
   }
   THROW_ASSERT(sl->list_of_bloc.count(ga->bb_index), "");
   sl->list_of_bloc.at(ga->bb_index)->RemoveStmt(statement, AppM);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Removed " + statement->ToString());
}

void PhiOpt::ReplaceVirtualUses(const tree_nodeRef& old_vssa, const TreeNodeSet& new_vssa) const
{
   const auto virtual_ssa = GetPointerS<ssa_name>(old_vssa);
   while(virtual_ssa->CGetUseStmts().size())
   {
      const auto use_stmt = virtual_ssa->CGetUseStmts().begin()->first;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " use stmt " + use_stmt->ToString());
      const auto gn = GetPointerS<gimple_node>(use_stmt);
      const auto has_vuses = gn->vuses.find(old_vssa) != gn->vuses.end();
      const auto has_vovers = gn->vovers.find(old_vssa) != gn->vovers.end();
      THROW_ASSERT(has_vuses || has_vovers,
                   old_vssa->ToString() + " is not in the vuses/vovers of " + use_stmt->ToString());

      if(has_vuses)
      {
         gn->vuses.erase(old_vssa);
      }
      if(has_vovers)
      {
         gn->vovers.erase(old_vssa);
      }
      for(auto uses = virtual_ssa->CGetUseStmts().begin()->second; uses > 0; --uses)
      {
         virtual_ssa->RemoveUse(use_stmt);
      }
      for(const auto& vssa : new_vssa)
      {
         if(has_vuses)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---add vuse " + vssa->ToString());
            if(gn->AddVuse(vssa))
            {
               GetPointerS<ssa_name>(vssa)->AddUseStmt(use_stmt);
            }
         }
         if(has_vovers)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---add vover " + vssa->ToString());
            if(gn->AddVover(vssa))
            {
               GetPointerS<ssa_name>(vssa)->AddUseStmt(use_stmt);
            }
         }
      }
   }
}
