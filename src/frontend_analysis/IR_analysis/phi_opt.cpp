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
 *              Copyright (C) 2004-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */
/**
 * @file phi_opt.cpp
 * @brief Analysis step that improves the IR w.r.t. phis
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "phi_opt.hpp"

#include "Parameter.hpp"
#include "allocation_information.hpp"
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "dbgPrintHelper.hpp"
#include "function_behavior.hpp"
#include "hls.hpp"
#include "hls_constraints.hpp"
#include "hls_manager.hpp"
#include "hls_step.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_manipulation.hpp"
#include "ir_node.hpp"
#include "schedule.hpp"
#include "string_manipulation.hpp"
#include "token_interface.hpp"
#include <boost/range/adaptor/reversed.hpp>
#include <fstream>

PhiOpt::PhiOpt(const application_managerRef _AppM, unsigned int _function_id,
               const DesignFlowManager& _design_flow_manager, const ParameterConstRef _parameters)
    : FunctionFrontendFlowStep(_AppM, _function_id, PHI_OPT, _design_flow_manager, _parameters), bb_modified(false)
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionFrontendFlowStep::FunctionRelationship>>
PhiOpt::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(MULTI_WAY_IF, SAME_FUNCTION));
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
   TM = AppM->get_ir_manager();
   ir_man = ir_manipulationConstRef(new ir_manipulation(TM, parameters, AppM));
   const auto fd = GetPointerS<const function_val_node>(TM->GetIRNode(function_id));
   sl = GetPointerS<statement_list_node>(fd->body);
   if(parameters->getOption<HLSFlowStep_Type>(OPT_scheduling_algorithm) == HLSFlowStep_Type::SDC_SCHEDULING &&
      GetPointer<const HLS_manager>(AppM) && GetPointer<const HLS_manager>(AppM)->get_HLS(function_id) and
      GetPointer<const HLS_manager>(AppM)->get_HLS(function_id)->Rsch)
   {
      schedule = GetPointerS<const HLS_manager>(AppM)->get_HLS(function_id)->Rsch;
   }
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
         std::list<ir_nodeRef> phis_to_be_removed;
         for(const auto& phi : block.second->CGetPhiList())
         {
            const auto gp = GetPointer<const phi_stmt>(phi);
            const auto sn = GetPointer<const ssa_node>(gp->res);
            if(sn->CGetUseStmts().empty())
            {
               phis_to_be_removed.push_back(phi);
            }
         }
         for(const auto& phi : phis_to_be_removed)
         {
            if(AppM->ApplyNewTransformation())
            {
               AppM->RegisterTransformation(GetName(), ir_nodeConstRef());
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
                  const auto gp = GetPointer<const phi_stmt>(phi);
                  const auto sn = GetPointer<const ssa_node>(gp->res);
                  for(const auto& use : sn->CGetUseStmts())
                  {
                     const auto gn = GetPointer<const node_stmt>(use.first);
                     if(gn->get_kind() != phi_stmt_K)
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
            AppM->RegisterTransformation(GetName(), ir_nodeConstRef());
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
            AppM->RegisterTransformation(GetName(), ir_nodeConstRef());
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
                  AppM->RegisterTransformation(GetName(), ir_nodeConstRef());
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
         if(block->CGetStmtList().size() == 1 && block->CGetStmtList().front()->get_kind() == nop_stmt_K)
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
         if(!AppM->ApplyNewTransformation())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Reached limit of cfg transformations");
            continue;
         }
         AppM->RegisterTransformation(GetName(), ir_nodeConstRef());
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
            case PhiOpt_PatternType::NO_STMT:
            {
               ApplyNoStmt(block->number);
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

   IRNodeSet ces_to_be_removed;

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Removing redundant select_node");
   for(const auto& block : sl->list_of_bloc)
   {
      for(const auto& statement : block.second->CGetStmtList())
      {
         const auto* ga = GetPointer<const assign_stmt>(statement);
         if(ga && ga->op1->get_kind() == select_node_K)
         {
            const auto* ce = GetPointer<const select_node>(ga->op1);
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
      RemoveSelectNode(ce_to_be_removed);
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Removed redundant select_node");

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
                  AppM->RegisterTransformation(GetName(), ir_nodeConstRef());
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
      IRNodeSet to_be_removeds;
      for(const auto& stmt : block.second->CGetStmtList())
      {
         const auto gn = GetPointerS<node_stmt>(stmt);
         if(gn->get_kind() != nop_stmt_K || !gn->vdef ||
            (gn->vovers.find(gn->vdef) != gn->vovers.end() && gn->vovers.size() > 1) ||
            (gn->vovers.find(gn->vdef) == gn->vovers.end() && (!gn->vovers.empty())))
         {
            if(gn->get_kind() == nop_stmt_K)
            {
               INDENT_DBG_MEX(
                   DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                   "---skipped nop statement" + STR(gn->index) + (!gn->vdef ? "NOVDEF " : "VDEF ") +
                       ((gn->vovers.find(gn->vdef) != gn->vovers.end() && gn->vovers.size() > 1) ? "cond1 " :
                                                                                                   "not cond1 ") +
                       ((gn->vovers.find(gn->vdef) == gn->vovers.end() && (!gn->vovers.empty())) ? "cond2 " :
                                                                                                   "not cond2 "));
            }
            continue;
         }
         if(AppM->ApplyNewTransformation())
         {
            const auto virtual_ssa = GetPointerS<ssa_node>(gn->vdef);
            THROW_ASSERT(virtual_ssa && virtual_ssa->virtual_flag, "unexpected condition");

            /// If there is only a single vuse replace vdef with vuse in all the uses of vdef
            if(gn->vuses.size() == 1)
            {
               const auto vuse = *(gn->vuses.begin());
               const auto uses = virtual_ssa->CGetUseStmts();
               for(const auto& use : uses)
               {
                  TM->ReplaceIRNode(use.first, gn->vdef, vuse);
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Removing nop statement" + STR(gn->index));
               to_be_removeds.insert(stmt);
               AppM->RegisterTransformation(GetName(), ir_nodeConstRef());
            }
            else
            {
               /// Check that all the uses are not in phi or not defining a self loop
               const auto canBeProp = [&]() -> bool {
                  for(const auto& use_stmt : virtual_ssa->CGetUseStmts())
                  {
                     if(use_stmt.first->get_kind() == phi_stmt_K)
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
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Removing nop statement " + STR(gn->index));
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
                  to_be_removeds.insert(stmt);
                  AppM->RegisterTransformation(GetName(), ir_nodeConstRef());
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

      /// Fix predecessor of successor
      succ_block->list_of_pred.push_back(new_basic_block_index);

      /// Fix multi_way_if_stmt
      if(pred_block->CGetStmtList().size())
      {
         auto pred_last_stmt = pred_block->CGetStmtList().back();
         if(pred_last_stmt->get_kind() == multi_way_if_stmt_K)
         {
            const auto gmwi = GetPointerS<multi_way_if_stmt>(pred_last_stmt);
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
      const auto gp = GetPointerS<phi_stmt>(phi);
      phi_stmt::DefEdgeList new_list_of_def_edge;
      auto curr_value = ir_nodeRef();

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

void PhiOpt::ApplyNoStmt(const unsigned int bb_index)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Removing empty basic block");
   const auto& curr_block = sl->list_of_bloc.at(bb_index);
   const auto& pred_block = sl->list_of_bloc.at(curr_block->list_of_pred.front());
   const auto& succ_block = sl->list_of_bloc.at(curr_block->list_of_succ.front());
   for(const auto& phi : succ_block->CGetPhiList())
   {
      const auto gp = GetPointer<phi_stmt>(phi);
      for(auto& def_edge : gp->CGetDefEdgesList())
      {
         if(def_edge.second == curr_block->number)
         {
            gp->ReplaceDefEdge(TM, def_edge, phi_stmt::DefEdge(def_edge.first, pred_block->number));
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

void PhiOpt::ApplyMultiMerge(const unsigned int bb_index)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Removing BB" + STR(bb_index));
   const auto& curr_block = sl->list_of_bloc.at(bb_index);
   const auto& pred_block = sl->list_of_bloc.at(curr_block->list_of_pred.front());
   const auto& succ_block = sl->list_of_bloc.at(curr_block->list_of_succ.front());

   auto& pred_stmt_list = pred_block->CGetStmtList();

   /// True if bb_index is on the first edge
   auto first_edge = false;

   /// The multi way if statement
   THROW_ASSERT(pred_stmt_list.back()->get_kind() == multi_way_if_stmt_K, "");
   const auto gmwi_node = pred_stmt_list.back();
   const auto gmwi = GetPointerS<multi_way_if_stmt>(gmwi_node);

   /// Temporary remove multi way if statement
   pred_block->RemoveStmt(pred_stmt_list.back(), AppM);

   /// The first condition
   auto first_condition = std::pair<ir_nodeRef, unsigned int>(ir_nodeRef(), 0);

   /// The second condition
   auto second_condition = std::pair<ir_nodeRef, unsigned int>(ir_nodeRef(), 0);

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
             ir_man->CreateOrExpr(first_condition.first, second_condition.first, pred_block, function_id);
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
            new_list_of_cond.push_front(phi_stmt::DefEdge(TM->GetIRNode(new_cond), succ_block->number));
         }
         else
         {
            new_list_of_cond.push_back(phi_stmt::DefEdge(ir_nodeRef(), succ_block->number));
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
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---New multi way if statement " + gmwi->ToString());

   /// Update all the phis
   for(const auto& phi : succ_block->CGetPhiList())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Modifying phi " + phi->ToString());
      /// The value coming from the first edge
      ir_nodeRef first_value = nullptr;

      /// The value coming from the second edge
      ir_nodeRef second_value = nullptr;

      auto gp = GetPointer<phi_stmt>(phi);

      /// The type of the expression
      const auto type_node = ir_helper::CGetType(gp->res);

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
      ir_nodeRef var;
      if(first_value->get_kind() == ssa_node_K && second_value->get_kind() == ssa_node_K)
      {
         const auto sn1 = GetPointer<const ssa_node>(first_value);
         const auto sn2 = GetPointer<const ssa_node>(second_value);
         if(sn1->var && sn2->var && sn1->var->index == sn2->var->index)
         {
            var = sn1->var;
         }
      }
      const auto gp_res = GetPointer<const ssa_node>(gp->res);
      const auto result_ssa_ref = ir_man->create_ssa_name(var, type_node, gp_res->min, gp_res->max, gp->virtual_flag);
      GetPointer<ssa_node>(result_ssa_ref)->bit_values = gp_res->bit_values;
      ir_nodeRef node_stmt;
      if(gp->virtual_flag)
      {
         /// Create a nop with virtual operands
         ir_manager::IRSchema nop_stmt_schema;
         nop_stmt_schema[TOK(TOK_IR_LOCINFO)] = BUILTIN_LOCINFO;
         nop_stmt_schema[TOK(TOK_PARENT)] = STR(function_id);
         node_stmt = TM->create_ir_node(nop_stmt_K, nop_stmt_schema);
         auto gn = GetPointerS<nop_stmt>(node_stmt);
         gn->SetVdef(result_ssa_ref);
         gn->AddVuse(first_value);
         gn->AddVuse(second_value);
      }
      else
      {
         /// Create the select_node
         auto condition_type = ir_helper::CGetType(first_condition.first);
         const auto select_node_node = ir_man->create_ternary_operation(type_node, first_condition.first, first_value,
                                                                        second_value, BUILTIN_LOCINFO, select_node_K);

         /// Create the assign
         node_stmt = ir_man->create_assign_stmt(result_ssa_ref, select_node_node, function_id, BUILTIN_LOCINFO);
      }
      pred_block->PushBack(node_stmt, AppM);

      /// Updating the phi
      phi_stmt::DefEdgeList new_list_of_def_edge;
      for(const auto& def_edge : gp->CGetDefEdgesList())
      {
         if(def_edge.second == pred_block->number)
         {
            new_list_of_def_edge.push_back(phi_stmt::DefEdge(result_ssa_ref, pred_block->number));
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

   /// Readding multi way if statement it has more than two exits
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

   THROW_ASSERT(pred_block->CGetStmtList().back()->get_kind() == multi_way_if_stmt_K, "");
   const auto gmwi = GetPointerS<multi_way_if_stmt>(pred_block->CGetStmtList().back());
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
      const auto gp = GetPointerS<phi_stmt>(phi);
      for(auto& def_edge : gp->CGetDefEdgesList())
      {
         if(def_edge.second == curr_block->number)
         {
            gp->ReplaceDefEdge(TM, def_edge, phi_stmt::DefEdge(def_edge.first, pred_block->number));
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

   /// The multi way if statement
   THROW_ASSERT(pred_stmt_list.back()->get_kind() == multi_way_if_stmt_K, "");
   const auto gmwi_node = pred_stmt_list.back();
   const auto gmwi = GetPointerS<multi_way_if_stmt>(gmwi_node);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Multi way if is " + gmwi->ToString());

   /// Temporary remove multi way if statement
   pred_block->RemoveStmt(pred_stmt_list.back(), AppM);

   /// The first condition
   auto first_condition = std::pair<ir_nodeRef, unsigned int>(ir_nodeRef(), 0);

   /// The second condition
   auto second_condition = std::pair<ir_nodeRef, unsigned int>(ir_nodeRef(), 0);

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
   const auto new_cond = [&]() -> ir_nodeRef {
      if(second_condition.first)
      {
         const auto new_node =
             ir_man->CreateOrExpr(first_condition.first, second_condition.first, pred_block, function_id);
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
            new_list_of_cond.push_front(phi_stmt::DefEdge(new_cond, succ_block->number));
         }
         else
         {
            new_list_of_cond.push_back(phi_stmt::DefEdge(nullptr, succ_block->number));
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
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Rewritten multi way if statement as " + gmwi->ToString());

   /// Remove all the phis
   for(const auto& phi : succ_block->CGetPhiList())
   {
      /// The value coming from the first edge
      ir_nodeRef first_value = nullptr;

      /// The value coming from the second edge
      ir_nodeRef second_value = nullptr;

      const auto gp = GetPointerS<phi_stmt>(phi);

      /// The type of the expression
      const auto type_node = ir_helper::CGetType(gp->res);

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
      ir_nodeRef new_node_stmt = nullptr;
      bool create_nop_stmt = false;
      if(gp->virtual_flag)
      {
         const auto virtual_ssa = GetPointerS<ssa_node>(gp->res);
         for(const auto& use_stmt : virtual_ssa->CGetUseStmts())
         {
            if(use_stmt.first->get_kind() == phi_stmt_K)
            {
               create_nop_stmt = true;
            }
         }
         if(create_nop_stmt)
         {
            /// Create a nop with virtual operands
            ir_manager::IRSchema nop_stmt_schema;
            nop_stmt_schema[TOK(TOK_IR_LOCINFO)] = BUILTIN_LOCINFO;
            nop_stmt_schema[TOK(TOK_PARENT)] = STR(function_id);
            new_node_stmt = TM->create_ir_node(nop_stmt_K, nop_stmt_schema);
            const auto gn = GetPointerS<nop_stmt>(new_node_stmt);
            gn->SetVdef(gp->res);
            gn->AddVuse(first_value);
            gn->AddVuse(second_value);
         }
         else
         {
            OrderedIRNodeSet new_ssas;
            new_ssas.insert(first_value);
            new_ssas.insert(second_value);
            ReplaceVirtualUses(gp->res, new_ssas);
         }
      }
      else
      {
         /// Create the select_node
         auto condition_type = ir_helper::CGetType(first_condition.first);
         const auto select_node_node = ir_man->create_ternary_operation(type_node, first_condition.first, first_value,
                                                                        second_value, BUILTIN_LOCINFO, select_node_K);

         /// Create the assign
         new_node_stmt = ir_man->create_assign_stmt(gp->res, select_node_node, function_id, BUILTIN_LOCINFO);
      }
      if(!gp->virtual_flag || create_nop_stmt)
      {
         succ_block->PushFront(new_node_stmt, AppM);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Added assign_stmt " + new_node_stmt->ToString());
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
                            function_behavior->CGetBehavioralHelper()->GetFunctionName());
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
      const auto gp = GetPointerS<const phi_stmt>(phi);
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
         return PhiOpt_PatternType::NO_STMT;
      }
      if(pred_block->CGetStmtList().size())
      {
         const auto pred_last_stmt = pred_block->CGetStmtList().back();
         if(pred_last_stmt->get_kind() == multi_way_if_stmt_K)
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
      if(pred_last_stmt->get_kind() == multi_way_if_stmt_K)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Controlled by multi way if");
         if(schedule)
         {
            /// The multi way if statement
            const auto gmwi = GetPointer<const multi_way_if_stmt>(pred_last_stmt);

            /// The conditions
            auto first_condition = ir_nodeRef();
            auto second_condition = ir_nodeRef();

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
                              "<--New multi way if statement would increase basic block latency");
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
   if(pred_last_stmt->get_kind() == multi_way_if_stmt_K)
   {
      /// Successor is ending if of the function
      if(succ_block->CGetStmtList().size() == 1 && GetPointer<const return_stmt>(succ_block->CGetStmtList().front()))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Empty path to phi to be merged");
         return PhiOpt_PatternType::MULTI_MERGE;
      }
      if(schedule)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "-->Potentially is a path to phi to be merged. Checking timing");
         /// Simulate adding the select_node and check if there are problems with timing

         /// The multi way if statement
         THROW_ASSERT(pred_block->CGetStmtList().back()->get_kind() == multi_way_if_stmt_K, "");
         const auto gmwi = GetPointerS<const multi_way_if_stmt>(pred_block->CGetStmtList().back());

         /// The first condition
         auto condition = ir_nodeRef();

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
            ir_nodeRef first_value = nullptr;

            /// The value coming from the second edge
            ir_nodeRef second_value = nullptr;

            /// True if bb_index is on the first edge
            auto first_edge = false;

            const auto gp = GetPointerS<const phi_stmt>(phi);

            /// The type of the expression
            const auto type_node = ir_helper::CGetType(gp->res);

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

            /// Create the select_node
            auto condition_type = ir_helper::CGetType(condition);
            const auto select_node_node = ir_man->create_ternary_operation(
                type_node, condition, first_value, second_value, BUILTIN_LOCINFO, select_node_K);

            /// Create the assign
            /// Workaround: we need to consider the overhead due to multiplexers associated with the phi; for this
            /// reason definition is one of the operands; this is not fully consistent, but it is a temporary assignment
            const auto assign_stmt_node =
                ir_man->create_assign_stmt(first_value, select_node_node, function_id, BUILTIN_LOCINFO);

            /// Created statement is not added to the predecessor
            if(schedule && schedule->CanBeMoved(assign_stmt_node->index, pred_block->number) !=
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
      const auto gp = GetPointerS<const phi_stmt>(phi);
      const auto& left_part = gp->res;
      THROW_ASSERT(gp->CGetDefEdgesList().size() == 1, "");
      const auto right_part = gp->CGetDefEdgesList().front().first;
      const auto left_ssa = GetPointerS<const ssa_node>(gp->res);
      /// Building temp set of use stmts (to avoid invalidation during loop execution and to skip phi)
      IRNodeSet use_stmts;
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
         TM->ReplaceIRNode(use_stmt, left_part, right_part);
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
            const auto gp = GetPointerS<phi_stmt>(phi);
            for(auto& def_edge : gp->CGetDefEdgesList())
            {
               if(def_edge.second == succ_block->number)
               {
                  gp->ReplaceDefEdge(TM, def_edge, phi_stmt::DefEdge(def_edge.first, curr_block->number));
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
      auto gp = GetPointerS<phi_stmt>(phi);
      phi_stmt::DefEdgeList new_list_of_def_edge;
      for(auto def_edge : gp->CGetDefEdgesList())
      {
         if(def_edge.second == bb_index)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Found " + def_edge.first->ToString() + " coming from BB" + STR(def_edge.second));
            const auto def_to_be_removed = def_edge.first;
            if(def_to_be_removed->get_kind() == ssa_node_K)
            {
               const auto def_stmt = GetPointerS<const ssa_node>(def_to_be_removed)->GetDefStmt();
               const auto phi_to_be_removed = GetPointer<const phi_stmt>(def_stmt);
               if(phi_to_be_removed && phi_to_be_removed->bb_index == bb_index)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---" + def_edge.first->ToString() + " comes from a phi to be removed");
                  /// Removing phi only if number of uses is 1
                  if(GetPointerS<const ssa_node>(def_to_be_removed)->CGetNumberUses() == 1)
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
                                     " in BB" + STR(GetPointer<const node_stmt>(def_stmt)->bb_index));
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
      const auto gp = GetPointerS<phi_stmt>(phi);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Adding " + gp->ToString());
      for(auto predecessor : succ_block->list_of_pred)
      {
         if(predecessor != bb_index)
         {
            gp->AddDefEdge(TM, phi_stmt::DefEdge(gp->res, predecessor));
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
      /// Fixing phi of predecessor
      if(pred_block->CGetStmtList().size())
      {
         const auto& last_stmt = pred_block->CGetStmtList().back();
         if(last_stmt->get_kind() == multi_way_if_stmt_K)
         {
            const auto gmw = GetPointerS<multi_way_if_stmt>(last_stmt);
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

void PhiOpt::RemoveSelectNode(const ir_nodeRef statement)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Removing " + statement->ToString());
   THROW_ASSERT(statement->get_kind() == assign_stmt_K, "");
   const auto ga = GetPointerS<const assign_stmt>(statement);
   const auto sn = GetPointer<const ssa_node>(ga->op0);
   THROW_ASSERT(sn, "select_node defines " + ga->op0->ToString());
   THROW_ASSERT(ga->op1->get_kind() == select_node_K, "");
   const auto new_sn = GetPointerS<const select_node>(ga->op1)->op1;
   const auto uses = sn->CGetUseStmts();
   for(const auto& use : uses)
   {
      TM->ReplaceIRNode(use.first, ga->op0, new_sn);
   }
   THROW_ASSERT(sl->list_of_bloc.count(ga->bb_index), "");
   sl->list_of_bloc.at(ga->bb_index)->RemoveStmt(statement, AppM);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Removed " + statement->ToString());
}

void PhiOpt::ReplaceVirtualUses(const ir_nodeRef& old_vssa, const OrderedIRNodeSet& new_vssa) const
{
   const auto virtual_ssa = GetPointerS<ssa_node>(old_vssa);
   while(virtual_ssa->CGetUseStmts().size())
   {
      const auto use_stmt = virtual_ssa->CGetUseStmts().begin()->first;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " use stmt " + use_stmt->ToString());
      const auto gn = GetPointerS<node_stmt>(use_stmt);
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
               GetPointerS<ssa_node>(vssa)->AddUseStmt(use_stmt);
            }
         }
         if(has_vovers)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---add vover " + vssa->ToString());
            if(gn->AddVover(vssa))
            {
               GetPointerS<ssa_node>(vssa)->AddUseStmt(use_stmt);
            }
         }
      }
   }
}
