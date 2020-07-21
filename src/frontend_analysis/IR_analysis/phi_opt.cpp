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
 * @file phi_opt.cpp
 * @brief Analysis step that improves the GCC IR w.r.t. phis
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

/// Autoheader include
#include "config_HAVE_STDCXX_11.hpp"

/// header include
#include "phi_opt.hpp"

///. include
#include "Parameter.hpp"

/// behavior includes
#include "application_manager.hpp"
#include "function_behavior.hpp"

/// design_flow_manager includes
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"

#if HAVE_ILP_BUILT && HAVE_BAMBU_BUILT
/// HLS includes
#include "hls.hpp"
#include "hls_constraints.hpp"
#include "hls_manager.hpp"
#include "hls_step.hpp"

/// HLS/module_allocation include
#include "allocation_information.hpp"

/// HLS/scheduling include
#include "schedule.hpp"
#endif

/// parser/treegcc include
#include "token_interface.hpp"

/// STD include
#include <fstream>

/// design_flows/technology includes
#include "technology_flow_step.hpp"
#include "technology_flow_step_factory.hpp"

/// tree includes
#include "ext_tree_node.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

/// utility include
#include "dbgPrintHelper.hpp"
#include "string_manipulation.hpp" // for GET_CLASS
#include <boost/range/adaptor/reversed.hpp>

PhiOpt::PhiOpt(const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters)
    : FunctionFrontendFlowStep(_AppM, _function_id, PHI_OPT, _design_flow_manager, _parameters), bb_modified(false)
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

PhiOpt::~PhiOpt() = default;

void PhiOpt::Initialize()
{
   bb_modified = false;
   TM = AppM->get_tree_manager();
   tree_man = tree_manipulationConstRef(new tree_manipulation(TM, parameters));
   auto fd = GetPointer<function_decl>(TM->get_tree_node_const(function_id));
   sl = GetPointer<statement_list>(GET_NODE(fd->body));
#if HAVE_BAMBU_BUILT && HAVE_ILP_BUILT
   if(parameters->getOption<HLSFlowStep_Type>(OPT_scheduling_algorithm) == HLSFlowStep_Type::SDC_SCHEDULING and GetPointer<const HLS_manager>(AppM) and GetPointer<const HLS_manager>(AppM)->get_HLS(function_id) and
      GetPointer<const HLS_manager>(AppM)->get_HLS(function_id)->Rsch)
   {
      schedule = GetPointer<const HLS_manager>(AppM)->get_HLS(function_id)->Rsch;
   }
#endif
}

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionFrontendFlowStep::FunctionRelationship>> PhiOpt::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(BLOCK_FIX, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(SWITCH_FIX, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(USE_COUNTING, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(DEAD_CODE_ELIMINATION, SAME_FUNCTION));
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(MULTI_WAY_IF, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(SHORT_CIRCUIT_TAF, SAME_FUNCTION));
#if HAVE_BAMBU_BUILT && HAVE_ILP_BUILT
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(SDC_CODE_MOTION, SAME_FUNCTION));
#endif
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         switch(GetStatus())
         {
            case DesignFlowStep_Status::SUCCESS:
            {
               if(tree_helper::is_a_nop_function_decl(GetPointer<function_decl>(TM->get_tree_node_const(function_id))))
                  relationships.insert(std::make_pair(DEAD_CODE_ELIMINATION, SAME_FUNCTION));
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
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}

DesignFlowStep_Status PhiOpt::InternalExec()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Merging phis");
   /// Removed blocks composed only of phi
   CustomSet<unsigned int> blocks_to_be_removed;
   for(auto block : sl->list_of_bloc)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Checking BB" + STR(block.first));
      if(block.second->list_of_pred.size() >= 2 and block.second->CGetPhiList().size() and block.second->CGetStmtList().empty())
      {
         const auto successor = block.second->list_of_succ.front();
         const auto succ_block = sl->list_of_bloc.find(successor)->second;
         /// Check that two basic block do not have any common predecessor
         const bool common_predecessor = [&]() {
            for(auto predecessor : block.second->list_of_pred)
            {
               if(std::find(succ_block->list_of_pred.begin(), succ_block->list_of_pred.end(), predecessor) != succ_block->list_of_pred.end())
                  return true;
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
               const auto gp = GetPointer<const gimple_phi>(GET_NODE(phi));
               const auto sn = GetPointer<const ssa_name>(GET_NODE(gp->res));
               for(const auto& use : sn->CGetUseStmts())
               {
                  const auto gn = GetPointer<const gimple_node>(GET_NODE(use.first));
                  if(gn->get_kind() != gimple_phi_K)
                     return false;
                  if(gn->bb_index != successor)
                     return false;
               }
            }
            return true;
         }();
         if(not only_phi_use)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped because not used only in the second phi");
            continue;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Added to the basic block to be merged");
         blocks_to_be_removed.insert(block.first);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   }
   for(auto block_to_be_removed : blocks_to_be_removed)
   {
#ifndef NDEBUG
      if(AppM->ApplyNewTransformation())
#endif
      {
#ifndef NDEBUG
         AppM->RegisterTransformation(GetName(), tree_nodeConstRef());
#endif
         MergePhi(block_to_be_removed);
         if(debug_level >= DEBUG_LEVEL_PEDANTIC)
         {
            WriteBBGraphDot("BB_During_" + GetName() + "_AfterMerge_BB" + STR(block_to_be_removed) + ".dot");
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Written BB_During_" + GetName() + "_AfterMerge_BB" + STR(block_to_be_removed) + ".dot");
         }
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Merged phis");

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Removing single input phis");
   /// Transform single input phi
   for(auto block : sl->list_of_bloc)
   {
      if(block.second->list_of_pred.size() == 1 and block.second->CGetPhiList().size())
      {
#ifndef NDEBUG
         if(AppM->ApplyNewTransformation())
#endif
         {
#ifndef NDEBUG
            AppM->RegisterTransformation(GetName(), tree_nodeConstRef());
#endif
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
   bool restart = true;

   while(restart)
   {
      restart = false;
      /// Transform chain of basic blocks
      for(auto block : sl->list_of_bloc)
      {
         if(block.first == bloc::ENTRY_BLOCK_ID)
            continue;
         if(block.second->list_of_succ.size() == 1)
         {
            auto succ_block = block.second->list_of_succ.front();
            if(sl->list_of_bloc[succ_block]->list_of_pred.size() == 1 and sl->list_of_bloc[succ_block]->list_of_pred.front() != bloc::ENTRY_BLOCK_ID)
            {
#ifndef NDEBUG
               if(AppM->ApplyNewTransformation())
#endif
               {
#ifndef NDEBUG
                  AppM->RegisterTransformation(GetName(), tree_nodeConstRef());
#endif
                  ChainOptimization(block.first);
                  bb_modified = true;
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
      restart = false;
      /// Workaround to avoid invalidation of pointer
#if HAVE_STDCXX_11
      CustomSet<decltype(sl->list_of_bloc)::key_type> blocks_to_be_analyzed;
#else
      CustomSet<unsigned int> blocks_to_be_analyzed;
#endif
      for(auto block : sl->list_of_bloc)
         blocks_to_be_analyzed.insert(block.first);

      /// Remove empty basic block
      for(auto bloc_to_be_analyzed : blocks_to_be_analyzed)
      {
         auto block = *(sl->list_of_bloc.find(bloc_to_be_analyzed));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing BB" + STR(block.first));

         /// Remove nop
         if(block.second->CGetStmtList().size() == 1 and GET_NODE(block.second->CGetStmtList().front())->get_kind() == gimple_nop_K)
         {
            block.second->RemoveStmt(block.second->CGetStmtList().front());
            bb_modified = true;
         }

         if(block.second->CGetStmtList().size() or block.second->CGetPhiList().size())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Basic block is not empty");
            continue;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Basic block is empty");
         if(block.first == bloc::ENTRY_BLOCK_ID)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Basic block is Entry");
            continue;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Basic block is not entry");
         if(block.first == bloc::EXIT_BLOCK_ID)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Basic block is Exit");
            continue;
         }
#if HAVE_PRAGMA_BUILT
         if(parameters->getOption<int>(OPT_gcc_openmp_simd))
         {
            if(block.second->list_of_pred.size() == 1)
            {
               const auto pred_block_id = block.second->list_of_pred.front();
               const auto pred_block = sl->list_of_bloc.find(pred_block_id)->second;
               if(pred_block->loop_id != block.second->loop_id)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Basic block is Landing pad");
                  continue;
               }
            }
         }
#endif
#ifndef NDEBUG
         if(not(AppM->ApplyNewTransformation()))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Reached limit of cfg transformations");
            continue;
         }
         AppM->RegisterTransformation(GetName(), tree_nodeConstRef());
#endif
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Basic block is not Exit");
         if(debug_level >= DEBUG_LEVEL_PEDANTIC)
         {
            WriteBBGraphDot("BB_Before_" + GetName() + "_Before_BB" + STR(block.first) + ".dot");
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Written BB_Before_" + GetName() + "_Before_BB" + STR(block.first) + ".dot");
         }
         const auto pattern_type = IdentifyPattern(block.first);
         switch(pattern_type)
         {
            case PhiOpt_PatternType::GIMPLE_NOTHING:
            {
               ApplyGimpleNothing(block.first);
               bb_modified = true;
               break;
            }
            case PhiOpt_PatternType::DIFF_NOTHING:
            {
               ApplyDiffNothing(block.first);
               bb_modified = true;
               restart = true;
               break;
            }
            case PhiOpt_PatternType::IF_MERGE:
            {
               ApplyIfMerge(block.first);
               bb_modified = true;
               break;
            }
            case PhiOpt_PatternType::IF_NOTHING:
            {
               ApplyIfNothing(block.first);
               bb_modified = true;
               break;
            }
            case PhiOpt_PatternType::IF_REMOVE:
            {
               ApplyIfRemove(block.first);
               bb_modified = true;
               break;
            }
            case PhiOpt_PatternType::MULTI_MERGE:
            {
               ApplyMultiMerge(block.first);
               bb_modified = true;
               break;
            }
            case PhiOpt_PatternType::MULTI_NOTHING:
            {
               ApplyMultiNothing(block.first);
               bb_modified = true;
               break;
            }
            case PhiOpt_PatternType::MULTI_REMOVE:
            {
               ApplyMultiRemove(block.first);
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
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Removed BB" + STR(block.first));
      }
   }

   TreeNodeSet ces_to_be_removed;

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Removing redundant cond_expr");
   for(const auto& block : sl->list_of_bloc)
   {
      for(const auto& statement : block.second->CGetStmtList())
      {
         const auto* ga = GetPointer<const gimple_assign>(GET_NODE(statement));
         if(ga and GET_NODE(ga->op1)->get_kind() == cond_expr_K)
         {
            const auto* ce = GetPointer<const cond_expr>(GET_NODE(ga->op1));
            if(ce and ce->op1->index == ce->op2->index)
            {
               ces_to_be_removed.insert(statement);
            }
         }
      }
   }
   if(!ces_to_be_removed.empty())
      bb_modified = true;
   for(const auto& ce_to_be_removed : ces_to_be_removed)
   {
      RemoveCondExpr(ce_to_be_removed);
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Removed redundant cond_expr");

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Removing chains of BB");
   restart = true;

   while(restart)
   {
      restart = false;
      /// Transform chain of basic blocks
      for(auto block : sl->list_of_bloc)
      {
         if(block.first == bloc::ENTRY_BLOCK_ID)
            continue;
         if(block.second->list_of_succ.size() == 1)
         {
            auto succ_block = block.second->list_of_succ.front();
            THROW_ASSERT(sl->list_of_bloc.find(succ_block) != sl->list_of_bloc.end(), "Successor block BB" + STR(succ_block) + " does not exist");
            if(sl->list_of_bloc[succ_block]->list_of_pred.size() == 1)
            {
#ifndef NDEBUG
               if(AppM->ApplyNewTransformation())
#endif
               {
#ifndef NDEBUG
                  AppM->RegisterTransformation(GetName(), tree_nodeConstRef());
#endif
                  ChainOptimization(block.first);
                  bb_modified = true;
               }
            }
         }
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Removed chains of BB");

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Removing nop with virtual operands");
   for(auto block : sl->list_of_bloc)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing BB" + STR(block.first));
      TreeNodeSet to_be_removeds;
      for(auto stmt : block.second->CGetStmtList())
      {
         auto gn = GetPointer<gimple_node>(GET_NODE(stmt));
         if(gn->get_kind() != gimple_nop_K or not gn->vdef or (gn->vovers.find(gn->vdef) != gn->vovers.end() and gn->vovers.size() > 1) or (gn->vovers.find(gn->vdef) == gn->vovers.end() and (not gn->vovers.empty())))
         {
            continue;
         }
#ifndef NDEBUG
         if(AppM->ApplyNewTransformation())
#endif
         {
            auto virtual_ssa = GetPointer<ssa_name>(GET_NODE(gn->vdef));
            THROW_ASSERT(virtual_ssa, "unexpected condition");
            THROW_ASSERT(virtual_ssa->virtual_flag, "unexpected condition");

            /// If there is only a single vuse replace vdef with vuse in all the uses of vdef
            if(gn->vuses.size() == 1)
            {
               auto vuse = *(gn->vuses.begin());
               while(virtual_ssa->CGetUseStmts().size())
               {
                  auto use_stmt = virtual_ssa->CGetUseStmts().begin()->first;
                  TM->ReplaceTreeNode(use_stmt, gn->vdef, vuse);
                  while(virtual_ssa->CGetUseStmts().find(use_stmt) != virtual_ssa->CGetUseStmts().end())
                     virtual_ssa->RemoveUse(use_stmt);
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Removing gimple nop " + STR(gn->index));
               to_be_removeds.insert(stmt);
#ifndef NDEBUG
               AppM->RegisterTransformation(GetName(), tree_nodeConstRef());
#endif
            }
            else
            {
               /// Check that all the uses are not in phi or not defining a self loop
               bool cannotBeProp = [&]() -> bool {
                  for(const auto& use_stmt : virtual_ssa->CGetUseStmts())
                  {
                     if(GET_NODE(use_stmt.first)->get_kind() == gimple_phi_K)
                     {
                        return true;
                     }
                     if(GET_INDEX_NODE(use_stmt.first) == GET_INDEX_NODE(stmt))
                     {
                        return true;
                     }
                  }
                  return false;
               }();
               if(not cannotBeProp)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Virtual op not used in any phi");
                  while(virtual_ssa->CGetUseStmts().size())
                  {
                     auto use_stmt = virtual_ssa->CGetUseStmts().begin()->first;
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "--- use stmt " + GET_NODE(use_stmt)->ToString());
                     auto us = GetPointer<gimple_node>(GET_NODE(use_stmt));
                     THROW_ASSERT(us->vuses.find(gn->vdef) != us->vuses.end(), "unexpected condition");
                     us->vuses.erase(us->vuses.find(gn->vdef));
                     while(virtual_ssa->CGetUseStmts().find(use_stmt) != virtual_ssa->CGetUseStmts().end())
                        virtual_ssa->RemoveUse(use_stmt);
                     for(const auto& vuse : gn->vuses)
                     {
                        if(us->vuses.find(vuse) == us->vuses.end())
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---   add vuse " + GET_NODE(vuse)->ToString());
                           us->AddVuse(vuse);
                           auto defSSA = GetPointer<ssa_name>(GET_NODE(vuse));
                           const auto& defssaStmt = defSSA->CGetUseStmts();
                           if(defssaStmt.find(use_stmt) == defssaStmt.end())
                              defSSA->AddUseStmt(use_stmt);
                        }
                     }
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Removing gimple nop " + STR(gn->index));
                  to_be_removeds.insert(stmt);
#ifndef NDEBUG
                  AppM->RegisterTransformation(GetName(), tree_nodeConstRef());
#endif
               }
            }
         }
      }
      for(const auto& to_be_removed : to_be_removeds)
      {
         block.second->RemoveStmt(to_be_removed);
      }
      if(!to_be_removeds.empty())
         bb_modified = true;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed BB" + STR(block.first));
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Removed nop with virtual operands");
   bb_modified ? function_behavior->UpdateBBVersion() : 0;
   return bb_modified ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}

void PhiOpt::ApplyDiffNothing(const unsigned int bb_index)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Duplicating empty basic block");
   auto curr_block = sl->list_of_bloc[bb_index];
   auto succ_block = sl->list_of_bloc[curr_block->list_of_succ.front()];
   THROW_ASSERT(std::find(succ_block->list_of_pred.begin(), succ_block->list_of_pred.end(), bb_index) != succ_block->list_of_pred.end(), "bb_index not included in the list of pred: " + STR(bb_index) + " succ: " + STR(succ_block->number));
   succ_block->list_of_pred.erase(std::find(succ_block->list_of_pred.begin(), succ_block->list_of_pred.end(), bb_index));

   CustomSet<unsigned int> created_bbs;

   for(auto pred : curr_block->list_of_pred)
   {
      auto pred_block = sl->list_of_bloc.find(pred)->second;

      /// Create empty basic block
      const auto new_basic_block_index = (sl->list_of_bloc.rbegin())->first + 1;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Created BB" + STR(new_basic_block_index) + " as new successor of BB" + STR(pred));
      created_bbs.insert(new_basic_block_index);
      auto new_block = blocRef(new bloc(new_basic_block_index));
      sl->list_of_bloc[new_basic_block_index] = new_block;

      new_block->loop_id = curr_block->loop_id;
      new_block->SetSSAUsesComputed();
      new_block->schedule = pred_block->schedule;

      /// Add predecessor as pred basic block
      new_block->list_of_pred.push_back(pred);

      /// Add successor as succ basic block
      new_block->list_of_succ.push_back(succ_block->number);

      /// Fix successor of predecessor
      pred_block->list_of_succ.erase(std::find(pred_block->list_of_succ.begin(), pred_block->list_of_succ.end(), bb_index));
      pred_block->list_of_succ.push_back(new_basic_block_index);
      if(pred_block->true_edge == bb_index)
         pred_block->true_edge = new_basic_block_index;
      if(pred_block->false_edge == bb_index)
         pred_block->false_edge = new_basic_block_index;

      /// Fix predecessor of successor
      succ_block->list_of_pred.push_back(new_basic_block_index);

      /// Fix gimple_multi_way_if
      if(pred_block->CGetStmtList().size())
      {
         auto pred_last_stmt = GET_NODE(pred_block->CGetStmtList().back());
         if(pred_last_stmt->get_kind() == gimple_multi_way_if_K)
         {
            auto gmwi = GetPointer<gimple_multi_way_if>(pred_last_stmt);
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
   for(auto phi : succ_block->CGetPhiList())
   {
      auto gp = GetPointer<gimple_phi>(GET_NODE(phi));
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
#if HAVE_STDCXX_11
         new_list_of_def_edge.push_back(decltype(new_list_of_def_edge)::value_type(curr_value, pred));
#else
         new_list_of_def_edge.push_back(gimple_phi::DefEdge(curr_value, pred));
#endif
      }
      gp->SetDefEdgeList(TM, new_list_of_def_edge);
   }
   sl->list_of_bloc.erase(bb_index);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Duplicated BB" + STR(bb_index));
}

void PhiOpt::ApplyIfMerge(const unsigned int bb_index)
{
   auto curr_block = sl->list_of_bloc[bb_index];
   auto pred_block = sl->list_of_bloc[curr_block->list_of_pred.front()];
   auto succ_block = sl->list_of_bloc[curr_block->list_of_succ.front()];

   const auto pred_stmt_list = pred_block->CGetStmtList();

   /// True if bb_index is on the true edge
   const auto true_edge = pred_block->true_edge == bb_index;

   /// The condition
   const auto condition = tree_man->ExtractCondition(pred_stmt_list.back(), pred_block);

   /// Remove gimple_cond from list of statement
   pred_block->RemoveStmt(pred_stmt_list.back());
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Condition is " + condition->ToString());

   /// Update all the phis
   for(const auto& phi : succ_block->CGetPhiList())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Modifying " + phi->ToString());
      /// The value coming from true edge
      auto true_value = 0u;

      /// The value coming from false edge
      auto false_value = 0u;

      auto gp = GetPointer<gimple_phi>(GET_NODE(phi));

      /// The type of the expression
      const auto type_index = tree_helper::get_type_index(TM, gp->res->index);

      for(auto def : gp->CGetDefEdgesList())
      {
         if((true_edge and bb_index == def.second) or (not true_edge and pred_block->number == def.second))
         {
            THROW_ASSERT(true_value == 0, "True value already found");
            true_value = def.first->index;
         }
         else if((not true_edge and bb_index == def.second) or (true_edge and pred_block->number == def.second))
         {
            THROW_ASSERT(false_value == 0, "True value already found");
            false_value = def.first->index;
         }
      }
      THROW_ASSERT(true_value, "True value not found");
      THROW_ASSERT(false_value, "False value not found");
      std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> cond_expr_schema, gimple_assign_schema, ssa_schema;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---From true edge " + TM->get_tree_node_const(true_value)->ToString());
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---From false edge " + TM->get_tree_node_const(false_value)->ToString());

      const auto gimple_node_id = TM->new_tree_node_id();

      /// Create the ssa with the new input of the phi
      auto ssa_vers = TM->get_next_vers();
      auto ssa_node_nid = TM->new_tree_node_id();
      ssa_schema[TOK(TOK_TYPE)] = STR(type_index);
      ssa_schema[TOK(TOK_VERS)] = STR(ssa_vers);
      ssa_schema[TOK(TOK_VOLATILE)] = STR(false);
      ssa_schema[TOK(TOK_VIRTUAL)] = STR(gp->virtual_flag);
      ssa_schema[TOK(TOK_BIT_VALUES)] = GetPointer<const ssa_name>(GET_NODE(gp->res))->bit_values;
      if(TM->get_tree_node_const(true_value)->get_kind() == ssa_name_K and TM->get_tree_node_const(false_value)->get_kind() == ssa_name_K)
      {
         const auto sn1 = GetPointer<const ssa_name>(TM->get_tree_node_const(true_value));
         const auto sn2 = GetPointer<const ssa_name>(TM->get_tree_node_const(false_value));
         if(sn1->var and sn2->var and sn1->var->index == sn2->var->index)
         {
            ssa_schema[TOK(TOK_VAR)] = STR(sn1->var->index);
         }
      }
      TM->create_tree_node(ssa_node_nid, ssa_name_K, ssa_schema);
      if(gp->virtual_flag)
      {
         /// Create a nop with virtual operands
         std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> gimple_nop_schema;
         gimple_nop_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
         TM->create_tree_node(gimple_node_id, gimple_nop_K, gimple_nop_schema);
         auto gn = GetPointer<gimple_nop>(TM->get_tree_node_const(gimple_node_id));
         gn->AddVdef(TM->GetTreeReindex(ssa_node_nid));
         gn->AddVuse(TM->GetTreeReindex(true_value));
         gn->AddVuse(TM->GetTreeReindex(false_value));
      }
      else
      {
         /// Create the cond expr
         const auto cond_expr_id = TM->new_tree_node_id();
         cond_expr_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
         cond_expr_schema[TOK(TOK_TYPE)] = boost::lexical_cast<std::string>(type_index);
         cond_expr_schema[TOK(TOK_OP0)] = boost::lexical_cast<std::string>(condition->index);
         cond_expr_schema[TOK(TOK_OP1)] = boost::lexical_cast<std::string>(true_value);
         cond_expr_schema[TOK(TOK_OP2)] = boost::lexical_cast<std::string>(false_value);
         TM->create_tree_node(cond_expr_id, (tree_helper::is_a_vector(TM, type_index) ? vec_cond_expr_K : cond_expr_K), cond_expr_schema);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created cond_expr " + TM->get_tree_node_const(cond_expr_id)->ToString());

         /// Create the assign
         gimple_assign_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
         gimple_assign_schema[TOK(TOK_TYPE)] = STR(type_index);
         gimple_assign_schema[TOK(TOK_OP0)] = STR(ssa_node_nid);
         gimple_assign_schema[TOK(TOK_OP1)] = STR(cond_expr_id);
         TM->create_tree_node(gimple_node_id, gimple_assign_K, gimple_assign_schema);
      }
      pred_block->PushBack(TM->GetTreeReindex(gimple_node_id));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + TM->get_tree_node_const(gimple_node_id)->ToString());

      /// Updating the phi
      gimple_phi::DefEdgeList new_list_of_def_edge;
      for(const auto& def : gp->CGetDefEdgesList())
      {
         if(def.second == pred_block->number)
         {
            new_list_of_def_edge.push_back(gimple_phi::DefEdge(TM->GetTreeReindex(ssa_node_nid), pred_block->number));
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
   succ_block->list_of_pred.erase(std::find(succ_block->list_of_pred.begin(), succ_block->list_of_pred.end(), bb_index));

   /// Remove the current basic block
   sl->list_of_bloc.erase(bb_index);
}

void PhiOpt::ApplyIfNothing(const unsigned int bb_index)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Removing empty basic block");
   auto curr_block = sl->list_of_bloc[bb_index];
   auto pred_block = sl->list_of_bloc[curr_block->list_of_pred.front()];
   auto succ_block = sl->list_of_bloc[curr_block->list_of_succ.front()];
   for(auto phi : succ_block->CGetPhiList())
   {
      auto gp = GetPointer<gimple_phi>(GET_NODE(phi));
      for(auto& def_edge : gp->CGetDefEdgesList())
      {
         if(def_edge.second == curr_block->number)
         {
            gp->ReplaceDefEdge(TM, def_edge, gimple_phi::DefEdge(def_edge.first, pred_block->number));
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
   succ_block->list_of_pred.erase(std::find(succ_block->list_of_pred.begin(), succ_block->list_of_pred.end(), bb_index));
   succ_block->list_of_pred.push_back(pred_block->number);

   /// Remove the current basic block
   sl->list_of_bloc.erase(bb_index);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Removed BB" + STR(bb_index));
}

void PhiOpt::ApplyGimpleNothing(const unsigned int bb_index)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Removing empty basic block");
   auto curr_block = sl->list_of_bloc[bb_index];
   auto pred_block = sl->list_of_bloc[curr_block->list_of_pred.front()];
   auto succ_block = sl->list_of_bloc[curr_block->list_of_succ.front()];
   for(auto phi : succ_block->CGetPhiList())
   {
      auto gp = GetPointer<gimple_phi>(GET_NODE(phi));
      for(auto& def_edge : gp->CGetDefEdgesList())
      {
         if(def_edge.second == curr_block->number)
         {
            gp->ReplaceDefEdge(TM, def_edge, gimple_phi::DefEdge(def_edge.first, pred_block->number));
         }
      }
   }

   /// Refactoring of the cfg - updating the predecessor
   pred_block->list_of_succ.erase(std::find(pred_block->list_of_succ.begin(), pred_block->list_of_succ.end(), bb_index));
   pred_block->list_of_succ.push_back(succ_block->number);

   /// Refactoring of the cfg - updating the successor
   succ_block->list_of_pred.erase(std::find(succ_block->list_of_pred.begin(), succ_block->list_of_pred.end(), bb_index));
   succ_block->list_of_pred.push_back(pred_block->number);

   /// Remove the current basic block
   sl->list_of_bloc.erase(bb_index);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Removed BB" + STR(bb_index));
}

void PhiOpt::ApplyIfRemove(const unsigned int bb_index)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Removing phis dominated by if");
   auto curr_block = sl->list_of_bloc[bb_index];
   auto pred_block = sl->list_of_bloc[curr_block->list_of_pred.front()];
   auto succ_block = sl->list_of_bloc[curr_block->list_of_succ.front()];

   auto& pred_stmt_list = pred_block->CGetStmtList();

   /// True if bb_index is on the true edge
   const auto true_edge = pred_block->true_edge == bb_index;

   /// Remove gimple_cond from list of statement. New statements can be pushed at the end
   THROW_ASSERT(pred_stmt_list.size(), "Statement list of predecessor is empty");

   /// The condition
   const auto last_stmt = pred_stmt_list.back();
   const auto condition = tree_man->ExtractCondition(last_stmt, pred_block);
   pred_block->RemoveStmt(last_stmt);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Condition is " + condition->ToString());

   /// Remove all the phis
   auto& list_of_phi = succ_block->CGetPhiList();
   for(auto& phi : list_of_phi)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Removing " + phi->ToString());
      /// The value coming from true edge
      auto true_value = 0u;

      /// The value coming from false edge
      auto false_value = 0u;

      auto gp = GetPointer<gimple_phi>(GET_NODE(phi));

      /// The type of the expression
      const auto type_index = tree_helper::get_type_index(TM, gp->res->index);

      for(auto def_edge : gp->CGetDefEdgesList())
      {
         if((true_edge and bb_index == def_edge.second) or (not true_edge and pred_block->number == def_edge.second))
         {
            THROW_ASSERT(true_value == 0, "True value already found");
            true_value = def_edge.first->index;
         }
         else if((not true_edge and bb_index == def_edge.second) or (true_edge and pred_block->number == def_edge.second))
         {
            THROW_ASSERT(false_value == 0, "False value already found");
            false_value = def_edge.first->index;
         }
         else
         {
            THROW_UNREACHABLE(std::string(true_edge ? "BB is on the true edge" : "BB is on the false edge") + " - Condition comes from BB" + STR(def_edge.second) + " - If basic block is BB" + STR((pred_block->number)));
         }
      }
      THROW_ASSERT(true_value, "True value not found");
      THROW_ASSERT(false_value, "False value not found");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---From true edge " + TM->get_tree_node_const(true_value)->ToString());
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---From false edge " + TM->get_tree_node_const(false_value)->ToString());
      std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> cond_expr_schema, gimple_assign_schema;

      if(gp->virtual_flag)
      {
         auto virtual_ssa = GetPointer<ssa_name>(GET_NODE(gp->res));
         bool create_gimple_nop = false;
         for(const auto& use_stmt : virtual_ssa->CGetUseStmts())
         {
            auto gn = GetPointer<gimple_node>(GET_NODE(use_stmt.first));
            if(gn->get_kind() == gimple_phi_K)
            {
               create_gimple_nop = true;
            }
         }
         if(create_gimple_nop)
         {
            const auto gimple_node_id = TM->new_tree_node_id();
            /// Create a nop with virtual operands
            std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> gimple_nop_schema;
            gimple_nop_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
            TM->create_tree_node(gimple_node_id, gimple_nop_K, gimple_nop_schema);
            auto gn = GetPointer<gimple_nop>(TM->get_tree_node_const(gimple_node_id));
            gn->AddVdef(TM->GetTreeReindex(gp->res->index));
            gn->AddVuse(TM->GetTreeReindex(true_value));
            gn->AddVuse(TM->GetTreeReindex(false_value));
            succ_block->PushFront(TM->GetTreeReindex(gimple_node_id));
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Created " + TM->get_tree_node_const(gimple_node_id)->ToString());
         }
         else
         {
            /// Replace all use of virtual defined in phi with res of phi
            while(virtual_ssa->CGetUseStmts().size())
            {
               auto use_stmt = (virtual_ssa->CGetUseStmts()).begin()->first;
               auto gn = GetPointer<gimple_node>(GET_NODE(use_stmt));
               THROW_ASSERT(gn->vuses.find(gp->res) != gn->vuses.end(), STR(gp->res) + " is not in the vuses of " + STR(use_stmt));
               gn->vuses.erase(gn->vuses.find(gp->res));
               virtual_ssa->RemoveUse(use_stmt);
               for(const auto& def : gp->CGetDefEdgesList())
               {
                  if(gn->vuses.find(def.first) == gn->vuses.end())
                  {
                     gn->AddVuse(def.first);
                     auto defSSA = GetPointer<ssa_name>(GET_NODE(def.first));
                     auto& defssaStmt = defSSA->CGetUseStmts();
                     if(defssaStmt.find(use_stmt) == defssaStmt.end())
                        defSSA->AddUseStmt(use_stmt);
                  }
               }
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Substituted vuses ");
         }
      }
      else
      {
         const auto gimple_node_id = TM->new_tree_node_id();
         /// Create the cond expr
         const auto cond_expr_id = TM->new_tree_node_id();
         cond_expr_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
         cond_expr_schema[TOK(TOK_TYPE)] = boost::lexical_cast<std::string>(type_index);
         cond_expr_schema[TOK(TOK_OP0)] = boost::lexical_cast<std::string>(condition->index);
         cond_expr_schema[TOK(TOK_OP1)] = boost::lexical_cast<std::string>(true_value);
         cond_expr_schema[TOK(TOK_OP2)] = boost::lexical_cast<std::string>(false_value);
         TM->create_tree_node(cond_expr_id, (tree_helper::is_a_vector(TM, type_index) ? vec_cond_expr_K : cond_expr_K), cond_expr_schema);

         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created cond_expr " + TM->get_tree_node_const(cond_expr_id)->ToString());

         /// Create the assign
         gimple_assign_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
         gimple_assign_schema[TOK(TOK_TYPE)] = STR(type_index);
         gimple_assign_schema[TOK(TOK_OP0)] = STR(gp->res->index);
         gimple_assign_schema[TOK(TOK_OP1)] = STR(cond_expr_id);
         TM->create_tree_node(gimple_node_id, gimple_assign_K, gimple_assign_schema);
         succ_block->PushFront(TM->GetTreeReindex(gimple_node_id));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Created " + TM->get_tree_node_const(gimple_node_id)->ToString());
      }
   }

   while(succ_block->CGetPhiList().size())
      succ_block->RemovePhi(succ_block->CGetPhiList().front());

   /// Refactoring of the cfg - updating the predecessor
   pred_block->list_of_succ.clear();
   pred_block->true_edge = 0;
   pred_block->false_edge = 0;
   pred_block->list_of_succ.push_back(succ_block->number);

   /// Refactoring of the cfg - updating the successor
   succ_block->list_of_pred.erase(std::find(succ_block->list_of_pred.begin(), succ_block->list_of_pred.end(), bb_index));

   /// Remove the current basic block
   sl->list_of_bloc.erase(bb_index);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Removed phis");
}

void PhiOpt::ApplyMultiMerge(const unsigned int bb_index)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Removing BB" + STR(bb_index));
   auto curr_block = sl->list_of_bloc[bb_index];
   auto pred_block = sl->list_of_bloc[curr_block->list_of_pred.front()];
   auto succ_block = sl->list_of_bloc[curr_block->list_of_succ.front()];

   auto& pred_stmt_list = pred_block->CGetStmtList();

   /// True if bb_index is on the first edge
   auto first_edge = false;

   /// The gimple multi way if
   auto gmwi = GetPointer<gimple_multi_way_if>(GET_NODE(pred_stmt_list.back()));

   /// Temporary remove gimple multi way if
   pred_block->RemoveStmt(pred_stmt_list.back());

   /// The first condition
   auto first_condition = std::pair<tree_nodeRef, unsigned int>(tree_nodeRef(), 0);

   /// The second condition
   auto second_condition = std::pair<tree_nodeRef, unsigned int>(tree_nodeRef(), 0);

   for(auto cond : gmwi->list_of_cond)
   {
      if(cond.second == curr_block->number)
      {
         if(not first_condition.first)
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
         if(not first_condition.first)
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
         const auto new_node = tree_man->CreateOrExpr(first_condition.first, second_condition.first, pred_block);
         return new_node->index;
      }
      else
      {
         return 0;
      }
   }();

   decltype(gmwi->list_of_cond) new_list_of_cond;
   for(auto cond : gmwi->list_of_cond)
   {
      if(cond == first_condition)
      {
         if(second_condition.first)
         {
            new_list_of_cond.push_front(gimple_phi::DefEdge(TM->GetTreeReindex(new_cond), succ_block->number));
         }
         else
         {
            new_list_of_cond.push_back(gimple_phi::DefEdge(tree_nodeRef(), succ_block->number));
         }
      }
      else if(cond != second_condition)
      {
         if(cond.first)
            new_list_of_cond.push_front(cond);
         else
            new_list_of_cond.push_back(cond);
      }
   }
   gmwi->list_of_cond = new_list_of_cond;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---New gimple multi way if " + gmwi->ToString());

   /// Update all the phis
   auto& list_of_phi = succ_block->CGetPhiList();
   for(auto& phi : list_of_phi)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Modifying phi " + phi->ToString());
      /// The value coming from the first edge
      auto first_value = 0u;

      /// The value coming from the second edge
      auto second_value = 0u;

      auto gp = GetPointer<gimple_phi>(GET_NODE(phi));

      /// The type of the expression
      const auto type_index = tree_helper::get_type_index(TM, gp->res->index);

      for(auto def_edge : gp->CGetDefEdgesList())
      {
         if((first_edge and bb_index == def_edge.second) or (not first_edge and pred_block->number == def_edge.second))
         {
            first_value = def_edge.first->index;
         }
         else if((not first_edge and bb_index == def_edge.second) or (first_edge and pred_block->number == def_edge.second))
         {
            second_value = def_edge.first->index;
         }
      }
      std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> cond_expr_schema, gimple_assign_schema, ssa_schema;

      const auto gimple_node_id = TM->new_tree_node_id();

      /// Create the ssa with the new input of the phi
      auto ssa_vers = TM->get_next_vers();
      auto ssa_node_nid = TM->new_tree_node_id();
      ssa_schema[TOK(TOK_TYPE)] = STR(type_index);
      ssa_schema[TOK(TOK_VERS)] = STR(ssa_vers);
      ssa_schema[TOK(TOK_VOLATILE)] = STR(false);
      ssa_schema[TOK(TOK_VIRTUAL)] = STR(gp->virtual_flag);
      ssa_schema[TOK(TOK_BIT_VALUES)] = GetPointer<const ssa_name>(GET_NODE(gp->res))->bit_values;
      if(TM->get_tree_node_const(first_value)->get_kind() == ssa_name_K and TM->get_tree_node_const(second_value)->get_kind() == ssa_name_K)
      {
         const auto sn1 = GetPointer<const ssa_name>(TM->get_tree_node_const(first_value));
         const auto sn2 = GetPointer<const ssa_name>(TM->get_tree_node_const(second_value));
         if(sn1->var and sn2->var and sn1->var->index == sn2->var->index)
         {
            ssa_schema[TOK(TOK_VAR)] = STR(sn1->var->index);
         }
      }

      TM->create_tree_node(ssa_node_nid, ssa_name_K, ssa_schema);
      if(gp->virtual_flag)
      {
         /// Create a nop with virtual operands
         std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> gimple_nop_schema;
         gimple_nop_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
         TM->create_tree_node(gimple_node_id, gimple_nop_K, gimple_nop_schema);
         auto gn = GetPointer<gimple_nop>(TM->get_tree_node_const(gimple_node_id));
         gn->AddVdef(TM->GetTreeReindex(ssa_node_nid));
         gn->AddVuse(TM->GetTreeReindex(first_value));
         gn->AddVuse(TM->GetTreeReindex(second_value));
      }
      else
      {
         /// Create the cond expr
         const auto cond_expr_id = TM->new_tree_node_id();
         cond_expr_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
         cond_expr_schema[TOK(TOK_TYPE)] = boost::lexical_cast<std::string>(type_index);
         cond_expr_schema[TOK(TOK_OP0)] = boost::lexical_cast<std::string>(first_condition.first->index);
         cond_expr_schema[TOK(TOK_OP1)] = boost::lexical_cast<std::string>(first_value);
         cond_expr_schema[TOK(TOK_OP2)] = boost::lexical_cast<std::string>(second_value);
         TM->create_tree_node(cond_expr_id, (tree_helper::is_a_vector(TM, type_index) ? vec_cond_expr_K : cond_expr_K), cond_expr_schema);

         /// Create the assign
         gimple_assign_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
         gimple_assign_schema[TOK(TOK_TYPE)] = STR(type_index);
         gimple_assign_schema[TOK(TOK_OP0)] = STR(ssa_node_nid);
         gimple_assign_schema[TOK(TOK_OP1)] = STR(cond_expr_id);
         TM->create_tree_node(gimple_node_id, gimple_assign_K, gimple_assign_schema);
      }
      pred_block->PushBack(TM->GetTreeReindex(gimple_node_id));

      /// Updating the phi
      gimple_phi::DefEdgeList new_list_of_def_edge;
      for(const auto& def_edge : gp->CGetDefEdgesList())
      {
         if(def_edge.second == pred_block->number)
         {
            new_list_of_def_edge.push_back(gimple_phi::DefEdge(TM->GetTreeReindex(ssa_node_nid), pred_block->number));
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
      pred_block->PushBack(TM->GetTreeReindex(gmwi->index));

   /// Refactoring of the cfg - updating the predecessor
   pred_block->list_of_succ.erase(std::find(pred_block->list_of_succ.begin(), pred_block->list_of_succ.end(), bb_index));

   /// Refactoring of the cfg - updating the successor
   succ_block->list_of_pred.erase(std::find(succ_block->list_of_pred.begin(), succ_block->list_of_pred.end(), bb_index));

   /// Remove the current basic block
   sl->list_of_bloc.erase(bb_index);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Removed BB" + STR(bb_index));
}

void PhiOpt::ApplyMultiNothing(const unsigned int bb_index)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Removing empty basic block");
   auto curr_block = sl->list_of_bloc[bb_index];
   auto pred_block = sl->list_of_bloc[curr_block->list_of_pred.front()];
   auto succ_block = sl->list_of_bloc[curr_block->list_of_succ.front()];

   auto gmwi = GetPointer<gimple_multi_way_if>(GET_NODE(pred_block->CGetStmtList().back()));

   for(auto& cond : gmwi->list_of_cond)
   {
      if(cond.second == bb_index)
      {
         cond.second = succ_block->number;
         break;
      }
   }

   for(auto phi : succ_block->CGetPhiList())
   {
      auto gp = GetPointer<gimple_phi>(GET_NODE(phi));
      for(auto& def_edge : gp->CGetDefEdgesList())
      {
         if(def_edge.second == curr_block->number)
         {
            gp->ReplaceDefEdge(TM, def_edge, gimple_phi::DefEdge(def_edge.first, pred_block->number));
         }
      }
   }

   /// Refactoring of the cfg - updating the predecessor
   pred_block->list_of_succ.erase(std::find(pred_block->list_of_succ.begin(), pred_block->list_of_succ.end(), bb_index));
   pred_block->list_of_succ.push_back(succ_block->number);

   /// Refactoring of the cfg - updating the successor
   succ_block->list_of_pred.erase(std::find(succ_block->list_of_pred.begin(), succ_block->list_of_pred.end(), bb_index));
   succ_block->list_of_pred.push_back(pred_block->number);

   /// Remove the current basic block
   sl->list_of_bloc.erase(bb_index);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Removed BB" + STR(bb_index));
}

void PhiOpt::ApplyMultiRemove(const unsigned int bb_index)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Removing BB" + STR(bb_index));
   auto curr_block = sl->list_of_bloc[bb_index];
   auto pred_block = sl->list_of_bloc[curr_block->list_of_pred.front()];
   auto succ_block = sl->list_of_bloc[curr_block->list_of_succ.front()];

   auto& pred_stmt_list = pred_block->CGetStmtList();

   /// True if bb_index is on the first edge
   auto first_edge = false;

   /// The gimple multi way if
   auto gmwi = GetPointer<gimple_multi_way_if>(GET_NODE(pred_stmt_list.back()));
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Multi way if is " + gmwi->ToString());

   /// Temporary remove gimple multi way if
   pred_block->RemoveStmt(pred_stmt_list.back());

   /// The first condition
   auto first_condition = std::pair<tree_nodeRef, unsigned int>(tree_nodeRef(), 0);

   /// The second condition
   auto second_condition = std::pair<tree_nodeRef, unsigned int>(tree_nodeRef(), 0);

   for(auto cond : gmwi->list_of_cond)
   {
      if(cond.second == curr_block->number)
      {
         if(not first_condition.first)
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
         if(not first_condition.first)
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
   const auto new_cond = [&]() -> unsigned int {
      if(second_condition.first)
      {
         const auto new_node = tree_man->CreateOrExpr(first_condition.first, second_condition.first, pred_block);
         return new_node->index;
      }
      else
      {
         return 0;
      }
   }();

   decltype(gmwi->list_of_cond) new_list_of_cond;
   for(auto cond : gmwi->list_of_cond)
   {
      if(cond == first_condition)
      {
         if(second_condition.first)
         {
            new_list_of_cond.push_front(gimple_phi::DefEdge(TM->GetTreeReindex(new_cond), succ_block->number));
         }
         else
         {
            new_list_of_cond.push_back(gimple_phi::DefEdge(tree_nodeRef(), succ_block->number));
         }
      }
      else if(cond != second_condition)
      {
         if(cond.first)
            new_list_of_cond.push_front(cond);
         else
            new_list_of_cond.push_back(cond);
      }
   }
   gmwi->list_of_cond = new_list_of_cond;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Rewritten gimple multi way if as " + gmwi->ToString());

   /// Remove all the phis
   auto& list_of_phi = succ_block->CGetPhiList();
   for(auto& phi : list_of_phi)
   {
      /// The value coming from the first edge
      auto first_value = 0u;

      /// The value coming from the second edge
      auto second_value = 0u;

      auto gp = GetPointer<gimple_phi>(GET_NODE(phi));

      /// The type of the expression
      const auto type_index = tree_helper::get_type_index(TM, gp->res->index);

      for(auto def_edge : gp->CGetDefEdgesList())
      {
         if((first_edge and bb_index == def_edge.second) or (not first_edge and pred_block->number == def_edge.second))
         {
            first_value = def_edge.first->index;
         }
         else if((not first_edge and bb_index == def_edge.second) or (first_edge and pred_block->number == def_edge.second))
         {
            second_value = def_edge.first->index;
         }
         else
         {
            THROW_UNREACHABLE("");
         }
      }
      std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> cond_expr_schema, gimple_assign_schema;
      unsigned int gimple_node_id = 0;
      bool create_gimple_nop = false;
      if(gp->virtual_flag)
      {
         auto virtual_ssa = GetPointer<ssa_name>(GET_NODE(gp->res));
         for(const auto& use_stmt : virtual_ssa->CGetUseStmts())
         {
            auto gn = GetPointer<gimple_node>(GET_NODE(use_stmt.first));
            if(gn->get_kind() == gimple_phi_K)
            {
               create_gimple_nop = true;
            }
         }
         if(create_gimple_nop)
         {
            /// Create a nop with virtual operands
            gimple_node_id = TM->new_tree_node_id();
            std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> gimple_nop_schema;
            gimple_nop_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
            TM->create_tree_node(gimple_node_id, gimple_nop_K, gimple_nop_schema);
            auto gn = GetPointer<gimple_nop>(TM->get_tree_node_const(gimple_node_id));
            gn->AddVdef(TM->GetTreeReindex(gp->res->index));
            gn->AddVuse(TM->GetTreeReindex(first_value));
            gn->AddVuse(TM->GetTreeReindex(second_value));
         }
         else
         {
            /// Replace all use of virtual defined in phi with res of phi
            while(virtual_ssa->CGetUseStmts().size())
            {
               auto use_stmt = (GetPointer<const ssa_name>(GET_NODE(gp->res))->CGetUseStmts()).begin()->first;
               auto gn = GetPointer<gimple_node>(GET_NODE(use_stmt));
               THROW_ASSERT(gn->vuses.find(gp->res) != gn->vuses.end(), STR(gp) + " is not in the vuses of " + STR(use_stmt));
               gn->vuses.erase(gn->vuses.find(gp->res));
               virtual_ssa->RemoveUse(use_stmt);
               auto FV = TM->GetTreeReindex(first_value);
               if(gn->vuses.find(FV) == gn->vuses.end())
               {
                  gn->AddVuse(FV);
                  gn->AddVuse(FV);
                  auto defSSA = GetPointer<ssa_name>(GET_NODE(FV));
                  auto& defssaStmt = defSSA->CGetUseStmts();
                  if(defssaStmt.find(use_stmt) == defssaStmt.end())
                     defSSA->AddUseStmt(use_stmt);
               }
               auto SV = TM->GetTreeReindex(second_value);
               if(gn->vuses.find(SV) == gn->vuses.end())
               {
                  gn->AddVuse(SV);
                  gn->AddVuse(SV);
                  auto defSSA = GetPointer<ssa_name>(GET_NODE(SV));
                  auto& defssaStmt = defSSA->CGetUseStmts();
                  if(defssaStmt.find(use_stmt) == defssaStmt.end())
                     defSSA->AddUseStmt(use_stmt);
               }
            }
         }
      }
      else
      {
         /// Create the cond expr
         const auto cond_expr_id = TM->new_tree_node_id();
         cond_expr_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
         cond_expr_schema[TOK(TOK_TYPE)] = boost::lexical_cast<std::string>(type_index);
         cond_expr_schema[TOK(TOK_OP0)] = boost::lexical_cast<std::string>(first_condition.first->index);
         cond_expr_schema[TOK(TOK_OP1)] = boost::lexical_cast<std::string>(first_value);
         cond_expr_schema[TOK(TOK_OP2)] = boost::lexical_cast<std::string>(second_value);
         TM->create_tree_node(cond_expr_id, (tree_helper::is_a_vector(TM, type_index) ? vec_cond_expr_K : cond_expr_K), cond_expr_schema);

         /// Create the assign
         gimple_node_id = TM->new_tree_node_id();
         gimple_assign_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
         gimple_assign_schema[TOK(TOK_TYPE)] = STR(type_index);
         gimple_assign_schema[TOK(TOK_OP0)] = STR(gp->res->index);
         gimple_assign_schema[TOK(TOK_OP1)] = STR(cond_expr_id);
         TM->create_tree_node(gimple_node_id, gimple_assign_K, gimple_assign_schema);
      }
      if(not gp->virtual_flag or create_gimple_nop)
      {
         succ_block->PushFront(TM->GetTreeReindex(gimple_node_id));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Added gimple assignment " + TM->get_tree_node_const(gimple_node_id)->ToString());
      }
   }

   /// Readd multi way if
   if(gmwi->list_of_cond.size() >= 2)
      pred_block->PushBack(TM->GetTreeReindex(gmwi->index));

   while(succ_block->CGetPhiList().size())
      succ_block->RemovePhi(succ_block->CGetPhiList().front());

   /// Refactoring of the cfg - updating the predecessor
   pred_block->list_of_succ.erase(std::find(pred_block->list_of_succ.begin(), pred_block->list_of_succ.end(), bb_index));

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
   auto curr_block = sl->list_of_bloc.find(bb_index)->second;
   if(curr_block->list_of_pred.size() == 1 and curr_block->list_of_pred.front() == bloc::ENTRY_BLOCK_ID)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Empty basic block connected to entry");
      return PhiOpt_PatternType::UNCHANGED;
   }
   if(std::find(curr_block->list_of_succ.begin(), curr_block->list_of_succ.end(), bb_index) != curr_block->list_of_succ.end())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Sink BB");
      return PhiOpt_PatternType::UNCHANGED;
   }
   if(curr_block->list_of_pred.size() != 1)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Multiple empty paths");
      return PhiOpt_PatternType::DIFF_NOTHING;
   }
   const auto succ_block = sl->list_of_bloc.find(curr_block->list_of_succ.front())->second;
   THROW_ASSERT(succ_block->number != bloc::EXIT_BLOCK_ID, "");
   const auto pred_block = sl->list_of_bloc.find(curr_block->list_of_pred.front())->second;
   auto phi_size = succ_block->list_of_pred.size();
   for(const auto& phi : succ_block->CGetPhiList())
   {
      auto gp = GetPointer<const gimple_phi>(GET_NODE(phi));
      if(phi_size != gp->CGetDefEdgesList().size())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Unknown because successor contain phi " + STR(phi) + " with different size: " + STR(phi_size) + " vs. " + STR(gp->CGetDefEdgesList().size()));
         return PhiOpt_PatternType::UNKNOWN;
      }
   }
   if(std::find(pred_block->list_of_succ.begin(), pred_block->list_of_succ.end(), succ_block->number) == pred_block->list_of_succ.end())
   {
      if(pred_block->list_of_succ.size() == 1)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Empty basic block with predecessor with single exit");
         return PhiOpt_PatternType::GIMPLE_NOTHING;
      }
      if(pred_block->CGetStmtList().size())
      {
         auto pred_last_stmt = GET_NODE(pred_block->CGetStmtList().back());
         if(pred_last_stmt->get_kind() == gimple_cond_K)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Basic block dominated by if which can be removed");
            return PhiOpt_PatternType::IF_NOTHING;
         }
         if(pred_last_stmt->get_kind() == gimple_multi_way_if_K)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Basic block dominated by multi way if which can be removed");
            return PhiOpt_PatternType::MULTI_NOTHING;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Unknown because it can be removed but it is controlled by " + pred_last_stmt->get_kind_text());
         return PhiOpt_PatternType::UNKNOWN;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Unknown because it can be removed but predecessor is empty");
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
      auto pred_last_stmt = GET_NODE(pred_block->CGetStmtList().back());
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
            const auto gmwi = GetPointer<gimple_multi_way_if>(pred_last_stmt);

            /// The conditions
            auto first_condition = tree_nodeRef();
            auto second_condition = tree_nodeRef();

            for(auto cond : gmwi->list_of_cond)
            {
               if(cond.second == curr_block->number or cond.second == succ_block->number)
               {
                  if(not first_condition)
                     first_condition = cond.first;
                  else
                     second_condition = cond.first;
               }
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---First condition is " + (first_condition ? first_condition->ToString() : "default"));
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Second condition is " + (second_condition ? second_condition->ToString() : "default"));
            if(not first_condition or not second_condition
#if HAVE_BAMBU_BUILT
               or schedule->EvaluateCondsMerging(pred_last_stmt->index, first_condition->index, second_condition->index)
#endif
            )
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Empty path to phi to be removed");
               return PhiOpt_PatternType::MULTI_REMOVE;
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--New gimple multi way if would increase basic block latency");
               return PhiOpt_PatternType::UNCHANGED;
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Empty path to phi to be removed");
         return PhiOpt_PatternType::MULTI_REMOVE;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Unknown because dominator of phi is " + pred_last_stmt->ToString());
      return PhiOpt_PatternType::UNKNOWN;
   }
   if(pred_block->CGetStmtList().empty())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Unknown because predecessor is empty");
      return PhiOpt_PatternType::UNKNOWN;
   }
   auto pred_last_stmt = GET_NODE(pred_block->CGetStmtList().back());
   if(pred_last_stmt->get_kind() == gimple_cond_K)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Empty nested then or empty else");
      return PhiOpt_PatternType::IF_MERGE;
   }
   if(pred_last_stmt->get_kind() == gimple_multi_way_if_K)
   {
      /// Successor is ending if of the function
      if(succ_block->CGetStmtList().size() == 1 and GetPointer<gimple_return>(GET_CONST_NODE(succ_block->CGetStmtList().front())))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Empty path to phi to be merged");
         return PhiOpt_PatternType::MULTI_MERGE;
      }
      if(schedule)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Potentially is a path to phi to be merged. Checking timing");
         /// Simulate to add the cond expr and check if there are problems with timing
         const auto pred_stmt_list = pred_block->CGetStmtList();

         /// The gimple multi way if
         const auto gmwi = GetPointer<gimple_multi_way_if>(GET_NODE(pred_stmt_list.back()));

         /// The first condition
         auto condition = tree_nodeRef();

         for(auto cond : gmwi->list_of_cond)
         {
            if(cond.second == curr_block->number or cond.second == succ_block->number)
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
            auto first_value = 0u;

            /// The value coming from the second edge
            auto second_value = 0u;

            /// True if bb_index is on the first edge
            auto first_edge = false;

            auto gp = GetPointer<gimple_phi>(GET_NODE(phi));

            /// The type of the expression
            const auto type_index = tree_helper::get_type_index(TM, gp->res->index);

            for(auto def_edge : gp->CGetDefEdgesList())
            {
               if((first_edge and bb_index == def_edge.second) or (not first_edge and pred_block->number == def_edge.second))
               {
                  first_value = def_edge.first->index;
               }
               else if((not first_edge and bb_index == def_edge.second) or (first_edge and pred_block->number == def_edge.second))
               {
                  second_value = def_edge.first->index;
               }
            }
            std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> cond_expr_schema, gimple_assign_schema;

            /// Create the cond expr
            const auto cond_expr_id = TM->new_tree_node_id();
            cond_expr_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
            cond_expr_schema[TOK(TOK_TYPE)] = boost::lexical_cast<std::string>(type_index);
            cond_expr_schema[TOK(TOK_OP0)] = boost::lexical_cast<std::string>(condition->index);
            cond_expr_schema[TOK(TOK_OP1)] = boost::lexical_cast<std::string>(first_value);
            cond_expr_schema[TOK(TOK_OP2)] = boost::lexical_cast<std::string>(second_value);
            TM->create_tree_node(cond_expr_id, (tree_helper::is_a_vector(TM, type_index) ? vec_cond_expr_K : cond_expr_K), cond_expr_schema);

            /// Create the assign
            const auto gimple_assign_id = TM->new_tree_node_id();
            gimple_assign_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
            gimple_assign_schema[TOK(TOK_TYPE)] = STR(type_index);
            /// Workaround: we need to consider the overhead due to multiplexers associated with the phi; for this reason definition is one of the operands; this is not fully consistent, but it is a temporary assignment
            gimple_assign_schema[TOK(TOK_OP0)] = STR(first_value);
            gimple_assign_schema[TOK(TOK_OP1)] = STR(cond_expr_id);
            TM->create_tree_node(gimple_assign_id, gimple_assign_K, gimple_assign_schema);

            /// Created statement is not added to the predecessor
#if HAVE_BAMBU_BUILT
            auto ga = GetPointer<gimple_assign>(TM->get_tree_node_const(gimple_assign_id));
            if(schedule and schedule->CanBeMoved(ga->index, pred_block->number) != FunctionFrontendFlowStep_Movable::MOVABLE)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Empty path to phi to be merged, but modifying would increase the latency of predecessor");
               return PhiOpt_PatternType::UNCHANGED;
            }
#endif
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Empty path to phi to be merged");
      return PhiOpt_PatternType::MULTI_MERGE;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Unknown because dominator of phi is " + pred_last_stmt->ToString());
   return PhiOpt_PatternType::UNKNOWN;
}

void PhiOpt::SinglePhiOptimization(const unsigned int bb_index)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Removing phis of BB" + STR(bb_index));
   auto curr_block = sl->list_of_bloc[bb_index];
   THROW_ASSERT(curr_block->list_of_pred.size() == 1, "Basic block with single phis but not a single predecessor");
   auto pred_block = sl->list_of_bloc[curr_block->list_of_pred.front()];
   for(auto phi : boost::adaptors::reverse(curr_block->CGetPhiList()))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Fixing using of ssa defined in " + phi->ToString());
      auto gp = GetPointer<gimple_phi>(GET_NODE(phi));
      if(gp->virtual_flag)
      {
         const auto def_edge_list = gp->CGetDefEdgesList();
         THROW_ASSERT(def_edge_list.size() == 1, gp->ToString());
         const auto new_def = def_edge_list.begin()->first;
         auto old_virtual_ssa = GetPointer<ssa_name>(GET_NODE(gp->res));
         while(old_virtual_ssa->CGetUseStmts().size())
         {
            auto use_stmt = old_virtual_ssa->CGetUseStmts().begin()->first;
            TM->ReplaceTreeNode(use_stmt, gp->res, new_def);
         }
      }
      else
      {
         const auto left_part = gp->res;
         const auto right_part = gp->CGetDefEdgesList().front().first;
         const auto left_ssa = GetPointer<const ssa_name>(GET_NODE(gp->res));
         /// Building temp set of use stmts (to avoid invalidation during loop execution and to skip phi)
         TreeNodeSet use_stmts;
         for(const auto& use_stmt : left_ssa->CGetUseStmts())
         {
            if(use_stmt.first->index != gp->index)
               use_stmts.insert(use_stmt.first);
         }

         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Replacing " + left_part->ToString() + " with " + right_part->ToString());
         for(const auto& use_stmt : use_stmts)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Before ssa replacement " + use_stmt->ToString());
            TM->ReplaceTreeNode(use_stmt, left_part, right_part);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---After ssa replacement " + use_stmt->ToString());
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Fixed using of ssa defined in " + phi->ToString());
   }
   while(curr_block->CGetPhiList().size())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Removing " + curr_block->CGetPhiList().front()->ToString());
      curr_block->RemovePhi(curr_block->CGetPhiList().front());
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Removed phis of BB" + STR(bb_index));
}

void PhiOpt::ChainOptimization(const unsigned int bb_index)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Applying chaining optimization starting from BB" + STR(bb_index));
   auto curr_block = sl->list_of_bloc.find(bb_index)->second;
   auto succ_block = sl->list_of_bloc.find(curr_block->list_of_succ.front())->second;

   /// The phis are taken from the first block
   THROW_ASSERT(succ_block->CGetPhiList().empty(), "Second element of the chain has phi");

   /// Move statement of second block in first one
   while(succ_block->CGetStmtList().size())
   {
      const auto statement = succ_block->CGetStmtList().front();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Moving " + STR(statement));
      succ_block->RemoveStmt(statement);
      curr_block->PushBack(statement);
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
         auto succ_succ_block = sl->list_of_bloc[succ_succ];
         succ_succ_block->list_of_pred.erase(std::find(succ_succ_block->list_of_pred.begin(), succ_succ_block->list_of_pred.end(), succ_block->number));
         succ_succ_block->list_of_pred.push_back(curr_block->number);
         for(auto phi : succ_succ_block->CGetPhiList())
         {
            auto gp = GetPointer<gimple_phi>(GET_NODE(phi));
            for(auto& def_edge : gp->CGetDefEdgesList())
            {
               if(def_edge.second == succ_block->number)
               {
                  gp->ReplaceDefEdge(TM, def_edge, gimple_phi::DefEdge(def_edge.first, curr_block->number));
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
   const auto curr_block = sl->list_of_bloc[bb_index];
   THROW_ASSERT(curr_block->list_of_succ.size() == 1, "BB" + STR(bb_index) + " has " + STR(curr_block->list_of_succ.size()));
   const auto succ_block = sl->list_of_bloc[curr_block->list_of_succ.front()];
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
   for(auto phi : succ_block->CGetPhiList())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Fixing " + phi->ToString());
      auto gp = GetPointer<gimple_phi>(GET_NODE(phi));
      gimple_phi::DefEdgeList new_list_of_def_edge;
      for(const auto def_edge : gp->CGetDefEdgesList())
      {
         if(def_edge.second == bb_index)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Found " + def_edge.first->ToString() + " coming from BB" + STR(def_edge.second));
            const auto def_to_be_removed = GET_NODE(def_edge.first);
            if(def_to_be_removed->get_kind() == ssa_name_K)
            {
               const auto def_stmt = GetPointer<const ssa_name>(def_to_be_removed)->CGetDefStmt();
               const auto phi_to_be_removed = GetPointer<const gimple_phi>(GET_NODE(def_stmt));
               if(phi_to_be_removed and phi_to_be_removed->bb_index == bb_index)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + def_edge.first->ToString() + " comes from a phi to be removed");
                  /// Removing phi only if number of uses is 1
                  if(GetPointer<const ssa_name>(def_to_be_removed)->CGetNumberUses() == 1)
                  {
                     curr_block->RemovePhi(def_stmt);
                  }
                  for(const auto& curr_def_edge : phi_to_be_removed->CGetDefEdgesList())
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding from phi of predecessor <" + curr_def_edge.first->ToString() + ", BB" + STR(curr_def_edge.second) + ">");
                     new_list_of_def_edge.push_back(curr_def_edge);
                  }
               }
               else
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + def_edge.first->ToString() + " is defined in a " + GET_NODE(def_stmt)->get_kind_text() + " in BB" + STR(GetPointer<const gimple_node>(GET_NODE(def_stmt))->bb_index));
                  for(auto predecessor : curr_block->list_of_pred)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding from predecessor <" + def_edge.first->ToString() + ", BB" + STR(predecessor) + ">");
                     new_list_of_def_edge.push_back(decltype(def_edge)(def_edge.first, predecessor));
                  }
               }
            }
            else
            {
               for(auto predecessor : curr_block->list_of_pred)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding from predecessor <" + def_edge.first->ToString() + ", BB" + STR(predecessor) + ">");
                  new_list_of_def_edge.push_back(decltype(def_edge)(def_edge.first, predecessor));
               }
            }
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Readding <" + def_edge.first->ToString() + ", BB" + STR(def_edge.second) + ">");
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
      auto gp = GetPointer<gimple_phi>(GET_NODE(phi));
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
      auto pred_block = sl->list_of_bloc[predecessor];
      pred_block->list_of_succ.erase(std::find(pred_block->list_of_succ.begin(), pred_block->list_of_succ.end(), bb_index));
      pred_block->list_of_succ.push_back(succ_block->number);
      if(pred_block->true_edge == bb_index)
         pred_block->true_edge = succ_block->number;
      if(pred_block->false_edge == bb_index)
         pred_block->false_edge = succ_block->number;
      /// Fixing gimple phi of predecessor
      if(pred_block->CGetStmtList().size())
      {
         auto last_stmt = pred_block->CGetStmtList().back();
         auto gmw = GetPointer<gimple_multi_way_if>(GET_NODE(last_stmt));
         if(gmw)
         {
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
   succ_block->list_of_pred.erase(std::find(succ_block->list_of_pred.begin(), succ_block->list_of_pred.end(), bb_index));
   for(auto predecessor : curr_block->list_of_pred)
   {
      succ_block->list_of_pred.push_back(predecessor);
   }

   /// Erasing basic block
   sl->list_of_bloc.erase(bb_index);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Merged phi of BB" + STR(bb_index));
}

void PhiOpt::RemoveCondExpr(const tree_nodeRef statement)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Removing " + statement->ToString());
   const auto ga = GetPointer<const gimple_assign>(GET_NODE(statement));
   const auto sn = GetPointer<const ssa_name>(GET_NODE(ga->op0));
   THROW_ASSERT(sn, "cond expression defines " + ga->op0->ToString());
   const auto new_sn = GetPointer<const cond_expr>(GET_NODE(ga->op1))->op1;
   const auto uses = sn->CGetUseStmts();
   for(const auto& use : uses)
   {
      TM->ReplaceTreeNode(use.first, ga->op0, new_sn);
   }
   sl->list_of_bloc[ga->bb_index]->RemoveStmt(statement);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Removed " + statement->ToString());
}

void PhiOpt::ComputeRelationships(DesignFlowStepSet& relationship, const DesignFlowStep::RelationshipType relationship_type)
{
   switch(relationship_type)
   {
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      case DEPENDENCE_RELATIONSHIP:
      {
         const DesignFlowGraphConstRef design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
         const auto* technology_flow_step_factory = GetPointer<const TechnologyFlowStepFactory>(design_flow_manager.lock()->CGetDesignFlowStepFactory("Technology"));
         const std::string technology_flow_signature = TechnologyFlowStep::ComputeSignature(TechnologyFlowStep_Type::LOAD_TECHNOLOGY);
         const vertex technology_flow_step = design_flow_manager.lock()->GetDesignFlowStep(technology_flow_signature);
         const DesignFlowStepRef technology_design_flow_step =
             technology_flow_step ? design_flow_graph->CGetDesignFlowStepInfo(technology_flow_step)->design_flow_step : technology_flow_step_factory->CreateTechnologyFlowStep(TechnologyFlowStep_Type::LOAD_TECHNOLOGY);
         relationship.insert(technology_design_flow_step);
         break;
      }
      case INVALIDATION_RELATIONSHIP:
      {
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   FunctionFrontendFlowStep::ComputeRelationships(relationship, relationship_type);
}
