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
 * @file multi_way_if.cpp
 * @brief Analysis step rebuilding multi-way if.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#include "multi_way_if.hpp"
#include "Parameter.hpp"
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "cyclic_topological_sort.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "function_behavior.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"
#if HAVE_ILP_BUILT
#include "hls_step.hpp"
#endif
#include "behavioral_helper.hpp"
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "ext_tree_node.hpp"
#include "schedule.hpp"
#include "string_manipulation.hpp" // for GET_CLASS
#include "token_interface.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include <cstdlib>
#include <fstream>

multi_way_if::multi_way_if(const ParameterConstRef _parameters, const application_managerRef _AppM,
                           unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, MULTI_WAY_IF, _design_flow_manager, _parameters),
      sl(nullptr),
      bb_modified(false)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

multi_way_if::~multi_way_if() = default;

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
multi_way_if::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(SWITCH_FIX, SAME_FUNCTION));
         relationships.insert(std::make_pair(BLOCK_FIX, SAME_FUNCTION));
         relationships.insert(std::make_pair(USE_COUNTING, SAME_FUNCTION));
#if HAVE_ILP_BUILT
         if(parameters->getOption<HLSFlowStep_Type>(OPT_scheduling_algorithm) == HLSFlowStep_Type::SDC_SCHEDULING)
         {
            relationships.insert(std::make_pair(UPDATE_SCHEDULE, SAME_FUNCTION));
         }
#endif
         if(!parameters->getOption<int>(OPT_gcc_openmp_simd))
         {
            relationships.insert(std::make_pair(BITVALUE_RANGE, SAME_FUNCTION));
         }
         relationships.insert(std::make_pair(SHORT_CIRCUIT_TAF, SAME_FUNCTION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         /// Not executed
         if(GetStatus() != DesignFlowStep_Status::SUCCESS)
         {
            if(parameters->getOption<HLSFlowStep_Type>(OPT_scheduling_algorithm) == HLSFlowStep_Type::SDC_SCHEDULING and
               GetPointer<const HLS_manager>(AppM) and GetPointer<const HLS_manager>(AppM)->get_HLS(function_id) and
               GetPointer<const HLS_manager>(AppM)->get_HLS(function_id)->Rsch)
            {
               /// If schedule is not up to date, do not execute this step and invalidate UpdateSchedule
               const auto update_schedule = design_flow_manager.lock()->GetDesignFlowStep(
                   FunctionFrontendFlowStep::ComputeSignature(FrontendFlowStepType::UPDATE_SCHEDULE, function_id));
               if(update_schedule != DesignFlowGraph::null_vertex())
               {
                  const DesignFlowGraphConstRef design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
                  const DesignFlowStepRef design_flow_step =
                      design_flow_graph->CGetNodeInfo(update_schedule)->design_flow_step;
                  if(GetPointer<const FunctionFrontendFlowStep>(design_flow_step)->CGetBBVersion() !=
                     function_behavior->GetBBVersion())
                  {
                     relationships.insert(std::make_pair(UPDATE_SCHEDULE, SAME_FUNCTION));
                     break;
                  }
               }
            }
         }
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(DETERMINE_MEMORY_ACCESSES, SAME_FUNCTION));
         relationships.insert(std::make_pair(INTERFACE_INFER, WHOLE_APPLICATION));
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

void multi_way_if::Initialize()
{
   bb_modified = false;
   TM = AppM->get_tree_manager();
   tree_man = tree_manipulationRef(new tree_manipulation(TM, parameters, AppM));
   const auto temp = TM->GetTreeNode(function_id);
   const auto fd = GetPointerS<const function_decl>(temp);
   sl = GetPointerS<statement_list>(fd->body);
#if HAVE_ILP_BUILT
   if(parameters->getOption<HLSFlowStep_Type>(OPT_scheduling_algorithm) == HLSFlowStep_Type::SDC_SCHEDULING and
      GetPointer<const HLS_manager>(AppM) and GetPointer<const HLS_manager>(AppM)->get_HLS(function_id) and
      GetPointer<const HLS_manager>(AppM)->get_HLS(function_id)->Rsch)
   {
      for(const auto& block : sl->list_of_bloc)
      {
         block.second->schedule = GetPointer<const HLS_manager>(AppM)->get_HLS(function_id)->Rsch;
      }
   }
#endif
}

void multi_way_if::UpdateCfg(const blocRef& pred_bb, const blocRef& curr_bb)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Updating control flow graph");
   /// Remove curr_bb from successor of pred_bb
   pred_bb->list_of_succ.erase(std::remove(pred_bb->list_of_succ.begin(), pred_bb->list_of_succ.end(), curr_bb->number),
                               pred_bb->list_of_succ.end());

   /// For each successor succ of curr_bb
   for(const auto succ : curr_bb->list_of_succ)
   {
      THROW_ASSERT(sl->list_of_bloc.count(succ), "");
      const auto& succ_bb = sl->list_of_bloc.at(succ);
      /// Remove curr_bb from its predecessor
      succ_bb->list_of_pred.erase(
          std::remove(succ_bb->list_of_pred.begin(), succ_bb->list_of_pred.end(), curr_bb->number),
          succ_bb->list_of_pred.end());

      /// Add pred_bb to its predecessor
      if(std::find(succ_bb->list_of_pred.begin(), succ_bb->list_of_pred.end(), pred_bb->number) ==
         succ_bb->list_of_pred.end())
      {
         succ_bb->list_of_pred.push_back(pred_bb->number);
      }

      /// Add succ to successor of pred_bb
      if(std::find(pred_bb->list_of_succ.begin(), pred_bb->list_of_succ.end(), succ) == pred_bb->list_of_succ.end())
      {
         pred_bb->list_of_succ.push_back(succ);
      }

      /// Update phi information
      for(const auto& phi : succ_bb->CGetPhiList())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Original phi " + phi->ToString());
         auto* current_phi = GetPointerS<gimple_phi>(phi);
         for(const auto& def_edge : current_phi->CGetDefEdgesList())
         {
            if(def_edge.second == curr_bb->number)
            {
               current_phi->ReplaceDefEdge(TM, def_edge, gimple_phi::DefEdge(def_edge.first, pred_bb->number));
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Modified phi " + phi->ToString());
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Updated control flow graph");
}

DesignFlowStep_Status multi_way_if::InternalExec()
{
   CustomUnorderedMap<unsigned int, vertex> inverse_vertex_map;
   BBGraphsCollectionRef GCC_bb_graphs_collection(
       new BBGraphsCollection(BBGraphInfoRef(new BBGraphInfo(AppM, function_id)), parameters));
   BBGraphRef GCC_bb_graph(new BBGraph(GCC_bb_graphs_collection, CFG_SELECTOR));

   CustomOrderedSet<unsigned int> bb_to_be_removed;
   for(const auto& block : sl->list_of_bloc)
   {
      inverse_vertex_map[block.first] =
          GCC_bb_graphs_collection->AddVertex(BBNodeInfoRef(new BBNodeInfo(block.second)));
   }
   /// add edges
   for(const auto& bb : sl->list_of_bloc)
   {
      const auto& curr_bbi = bb.first;
      const auto& curr_bb = bb.second;
      for(const auto pred : curr_bb->list_of_pred)
      {
         GCC_bb_graphs_collection->AddEdge(inverse_vertex_map[pred], inverse_vertex_map[curr_bbi], CFG_SELECTOR);
      }
      for(const auto succ : curr_bb->list_of_succ)
      {
         if(succ == bloc::EXIT_BLOCK_ID)
         {
            GCC_bb_graphs_collection->AddEdge(inverse_vertex_map[curr_bbi], inverse_vertex_map[succ], CFG_SELECTOR);
         }
      }
      if(curr_bb->list_of_succ.empty())
      {
         GCC_bb_graphs_collection->AddEdge(inverse_vertex_map[curr_bbi], inverse_vertex_map[bloc::EXIT_BLOCK_ID],
                                           CFG_SELECTOR);
      }
   }
   /// add a connection between entry and exit thus avoiding problems with non terminating code
   GCC_bb_graphs_collection->AddEdge(inverse_vertex_map[bloc::ENTRY_BLOCK_ID], inverse_vertex_map[bloc::EXIT_BLOCK_ID],
                                     CFG_SELECTOR);
   /// sort basic block vertices from the entry till the exit
   std::list<vertex> bb_sorted_vertices;
   cyclic_topological_sort(*GCC_bb_graph, std::front_inserter(bb_sorted_vertices));
   for(auto bb : bb_sorted_vertices)
   {
      const auto bb_node_info = GCC_bb_graph->CGetBBNodeInfo(bb);
      const auto& curr_bbi = bb_node_info->block->number;
      const auto& curr_bb = bb_node_info->block;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining BB" + STR(curr_bbi));
      if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC &&
         (!parameters->IsParameter("print-dot-FF") || parameters->GetParameter<unsigned int>("print-dot-FF")))
      {
         WriteBBGraphDot("BB_Before_" + GetName() + "_" + STR(curr_bbi) + ".dot");
      }
      if(curr_bbi == bloc::ENTRY_BLOCK_ID || curr_bbi == bloc::EXIT_BLOCK_ID)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped because entry or exit");
         continue;
      }
      if(bb_node_info->block->list_of_pred.size() > 1)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped because it has multiple predecessors");
         continue;
      }
      const auto pred = bb_node_info->block->list_of_pred.front();
      if(pred == bloc::ENTRY_BLOCK_ID)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped because predecessor is entry");
         continue;
      }
      THROW_ASSERT(sl->list_of_bloc.count(pred), "");
      const auto pred_bb = sl->list_of_bloc.at(pred);
      if(pred_bb->CGetStmtList().empty())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped because predecessor is empty");
         continue;
      }
      const auto last_pred_stmt = pred_bb->CGetStmtList().back();
      if(last_pred_stmt->get_kind() != gimple_cond_K && last_pred_stmt->get_kind() != gimple_multi_way_if_K)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "<--Skipped because predecessor ends with " + last_pred_stmt->get_kind_text());
         continue;
      }
      if(curr_bb->CGetStmtList().size() != 1)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped because it is not a singleton");
         continue;
      }
      const auto last_curr_stmt = curr_bb->CGetStmtList().back();
      if(last_curr_stmt->get_kind() != gimple_cond_K && last_curr_stmt->get_kind() != gimple_multi_way_if_K)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "<--Skipped because it ends with a " + last_curr_stmt->get_kind_text());
         continue;
      }
      if(curr_bb->CGetPhiList().size() != 0)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped because it contains a phi");
         continue;
      }
      if(!AppM->ApplyNewTransformation())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Reached limit of cfg transformations");
         continue;
      }
      AppM->RegisterTransformation(GetName(), tree_nodeConstRef());
      /// check for short circuit conditions: i.e., if they have at least a successor in common
      /// if so we add a basic block on the shortest path (e.g., predecessor --> common successor)
      /// In this way in the produced gimple_multi_way_if there cannot be multiple conditions with the same next bb
      /// phi_opt will remove the extra basic block
      bool restart = false;
      const auto& curr_list_of_succ = curr_bb->list_of_succ;
      do
      {
         restart = false;
         for(const auto succ_bbi : pred_bb->list_of_succ)
         {
            if(std::find(curr_list_of_succ.begin(), curr_list_of_succ.end(), succ_bbi) != curr_list_of_succ.end())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---BB" + STR(succ_bbi) + " is a common sucessor");
               THROW_ASSERT(sl->list_of_bloc.count(succ_bbi), "");
               FixCfg(curr_bb, sl->list_of_bloc.at(succ_bbi));
               restart = true;
               if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC &&
                  (!parameters->IsParameter("print-dot-FF") || parameters->GetParameter<unsigned int>("print-dot-FF")))
               {
                  WriteBBGraphDot("BB_After_" + GetName() + "_" + STR(curr_bbi) + "_Fix.dot");
               }
               break;
            }
         }
      }
      /// Fixed point since list of successor is changed by FixCfg
      while(restart);

      /// now the merging starts
      if(last_pred_stmt->get_kind() == gimple_cond_K && last_curr_stmt->get_kind() == gimple_cond_K)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Merging gimple_cond with gimple_cond (BB" + STR(curr_bbi) + " with BB" + STR(pred) + ")");
         MergeCondCond(pred_bb, curr_bb);
      }
      else if(last_pred_stmt->get_kind() == gimple_cond_K && last_curr_stmt->get_kind() == gimple_multi_way_if_K)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Merging gimple_cond with gimple_multi_way_if (BB" + STR(curr_bbi) + " with BB" + STR(pred) +
                            ")");
         MergeCondMulti(pred_bb, curr_bb);
      }
      else if(last_pred_stmt->get_kind() == gimple_multi_way_if_K &&
              last_curr_stmt->get_kind() == gimple_multi_way_if_K)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Merging gimple_multi_way_if with gimple_multi_way_if (BB" + STR(curr_bbi) + " with BB" +
                            STR(pred) + ")");
         MergeMultiMulti(pred_bb, curr_bb);
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Merging gimple_multi_way_if with gimple_cond (BB" + STR(curr_bbi) + " with BB" + STR(pred) +
                            ")");
         MergeMultiCond(pred_bb, curr_bb);
      }
      UpdateCfg(pred_bb, curr_bb);
      bb_modified = true;
      bb_to_be_removed.insert(curr_bbi);
      if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC &&
         (!parameters->IsParameter("print-dot-FF") || parameters->GetParameter<unsigned int>("print-dot-FF")))
      {
         WriteBBGraphDot("BB_After_" + GetName() + "_" + STR(curr_bbi) + ".dot");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Transformed");
   }
   CustomOrderedSet<unsigned int>::iterator it_tbr, it_tbr_end = bb_to_be_removed.end();
   for(it_tbr = bb_to_be_removed.begin(); it_tbr != it_tbr_end; ++it_tbr)
   {
      sl->list_of_bloc.erase(*it_tbr);
   }
   bb_to_be_removed.clear();

   bb_modified ? function_behavior->UpdateBBVersion() : 0;
   return bb_modified ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}

void multi_way_if::MergeCondMulti(const blocRef& pred_bb, const blocRef& curr_bb)
{
   /// create the gimple_multi_way_if node
   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;
   unsigned int gimple_multi_way_if_id = TM->new_tree_node_id();
   IR_schema[TOK(TOK_SRCP)] = BUILTIN_SRCP;
   IR_schema[TOK(TOK_SCPE)] = STR(function_id);
   TM->create_tree_node(gimple_multi_way_if_id, gimple_multi_way_if_K, IR_schema);
   auto new_gwi = GetPointerS<gimple_multi_way_if>(TM->GetTreeNode(gimple_multi_way_if_id));
   new_gwi->bb_index = pred_bb->number;

   const auto old_gwi = GetPointerS<const gimple_multi_way_if>(curr_bb->CGetStmtList().back());

   /// Create ce_condition
   const auto ce_cond = tree_man->ExtractCondition(pred_bb->CGetStmtList().back(), pred_bb, function_id);

   /// Remove old gimple_cond
   pred_bb->RemoveStmt(pred_bb->CGetStmtList().back(), AppM);

   /// Remove old gimple multi way if
   while(curr_bb->CGetStmtList().size())
   {
      curr_bb->RemoveStmt(curr_bb->CGetStmtList().front(), AppM);
   }

   /// First case: second bb is on the true edge
   if(pred_bb->true_edge == curr_bb->number)
   {
      auto default_bb = 0u;
      for(const auto& old_cond : old_gwi->list_of_cond)
      {
         if(old_cond.first)
         {
            const auto new_cond = tree_man->CreateAndExpr(ce_cond, old_cond.first, pred_bb, function_id);
            new_gwi->add_cond(new_cond, old_cond.second);
         }
         else
         {
            default_bb = old_cond.second;
            /// Skipped default condition
         }
      }
      new_gwi->add_cond(tree_man->CreateNotExpr(ce_cond, pred_bb, function_id), pred_bb->false_edge);
      new_gwi->add_cond(tree_nodeRef(), default_bb);
   }
   /// Second case: second bb is on the false edge
   else
   {
      new_gwi->add_cond(ce_cond, pred_bb->true_edge);
      for(const auto& old_cond : old_gwi->list_of_cond)
      {
         const auto not_second = tree_man->CreateNotExpr(ce_cond, pred_bb, function_id);
         const tree_nodeRef new_cond =
             old_cond.first ?
                 tree_man->CreateAndExpr(old_cond.first, tree_man->CreateNotExpr(ce_cond, pred_bb, function_id),
                                         pred_bb, function_id) :
                 tree_nodeRef();
         new_gwi->add_cond(new_cond, old_cond.second);
      }
   }
   pred_bb->PushBack(TM->GetTreeNode(gimple_multi_way_if_id), AppM);
   pred_bb->false_edge = 0;
   pred_bb->true_edge = 0;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + new_gwi->ToString());
}

void multi_way_if::MergeMultiMulti(const blocRef& pred_bb, const blocRef& curr_bb)
{
   /// create the gimple_multi_way_if node
   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;
   unsigned int gimple_multi_way_if_id = TM->new_tree_node_id();
   IR_schema[TOK(TOK_SRCP)] = BUILTIN_SRCP;
   IR_schema[TOK(TOK_SCPE)] = STR(function_id);
   TM->create_tree_node(gimple_multi_way_if_id, gimple_multi_way_if_K, IR_schema);
   auto new_gwi = GetPointerS<gimple_multi_way_if>(TM->GetTreeNode(gimple_multi_way_if_id));
   new_gwi->bb_index = pred_bb->number;

   const auto old_gwi1 = GetPointerS<const gimple_multi_way_if>(pred_bb->CGetStmtList().back());
   const auto old_gwi2 = GetPointerS<const gimple_multi_way_if>(curr_bb->CGetStmtList().back());
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---First gimple multi way if is " + old_gwi1->ToString());
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Second gimple multi way if is " + old_gwi2->ToString());

   /// Remove old gimple_cond
   pred_bb->RemoveStmt(pred_bb->CGetStmtList().back(), AppM);

   /// Remove old gimple multi way if
   while(curr_bb->CGetStmtList().size())
   {
      curr_bb->RemoveStmt(curr_bb->CGetStmtList().front(), AppM);
   }

   for(const auto& old_cond1 : old_gwi1->list_of_cond)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "-->Considering condition " + (old_cond1.first ? old_cond1.first->ToString() : " default"));
      /// Non default and succ is on this edge
      if(old_cond1.first && old_cond1.second == curr_bb->number)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---It is not default and nested gimple_multi_way_if is on this edge");
         for(const auto& old_cond2 : old_gwi2->list_of_cond)
         {
            if(old_cond2.first)
            {
               const auto new_cond = tree_man->CreateAndExpr(old_cond1.first, old_cond2.first, pred_bb, function_id);
               new_gwi->add_cond(new_cond, old_cond2.second);
            }
            else
            {
               tree_nodeRef new_cond = tree_nodeRef();
               for(const auto& other_old_cond2 : old_gwi2->list_of_cond)
               {
                  if(other_old_cond2.first)
                  {
                     if(new_cond)
                     {
                        new_cond = tree_man->CreateAndExpr(
                            new_cond, tree_man->CreateNotExpr(other_old_cond2.first, pred_bb, function_id), pred_bb,
                            function_id);
                     }
                     else
                     {
                        new_cond = tree_man->CreateNotExpr(other_old_cond2.first, pred_bb, function_id);
                     }
                  }
               }
               new_cond = tree_man->CreateAndExpr(new_cond, old_cond1.first, pred_bb, function_id);
               new_gwi->add_cond(new_cond, old_cond2.second);
            }
         }
      }
      /// Non default and succ is not on this edge
      else if(old_cond1.first)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---It is not default and nested gimple_multi_way_if is not on this edge");
         new_gwi->add_cond(old_cond1.first, old_cond1.second);
      }
      /// Default and succ is on this edge
      else if(old_cond1.second == curr_bb->number)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---It is default and nested gimple_multi_way_if is on this edge");
         /// Building the and of the not of other conditions of cond1
         tree_nodeRef not_cond = tree_nodeRef();
         for(const auto& other_old_cond1 : old_gwi1->list_of_cond)
         {
            if(other_old_cond1.first)
            {
               if(not_cond)
               {
                  not_cond = tree_man->CreateAndExpr(
                      not_cond, tree_man->CreateNotExpr(other_old_cond1.first, pred_bb, function_id), pred_bb,
                      function_id);
               }
               else
               {
                  not_cond = tree_man->CreateNotExpr(other_old_cond1.first, pred_bb, function_id);
               }
            }
         }
         for(const auto& old_cond2 : old_gwi2->list_of_cond)
         {
            if(old_cond2.first)
            {
               const auto new_cond = tree_man->CreateAndExpr(not_cond, old_cond2.first, pred_bb, function_id);
               new_gwi->add_cond(new_cond, old_cond2.second);
            }
            else
            {
               new_gwi->add_cond(tree_nodeRef(), old_cond2.second);
            }
         }
      }
      /// Default and second is not on this edge
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---It is default and nested gimple_multi_way_if is not on this edge");
         new_gwi->add_cond(tree_nodeRef(), old_cond1.second);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "<--Considered condition " + (old_cond1.first ? old_cond1.first->ToString() : " default"));
   }
   pred_bb->PushBack(TM->GetTreeNode(gimple_multi_way_if_id), AppM);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + new_gwi->ToString());
}

void multi_way_if::MergeMultiCond(const blocRef& pred_bb, const blocRef& curr_bb)
{
   /// create the gimple_multi_way_if node
   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;
   unsigned int gimple_multi_way_if_id = TM->new_tree_node_id();
   IR_schema[TOK(TOK_SRCP)] = BUILTIN_SRCP;
   IR_schema[TOK(TOK_SCPE)] = STR(function_id);
   TM->create_tree_node(gimple_multi_way_if_id, gimple_multi_way_if_K, IR_schema);
   auto new_gwi = GetPointerS<gimple_multi_way_if>(TM->GetTreeNode(gimple_multi_way_if_id));
   new_gwi->bb_index = pred_bb->number;

   const auto old_gwi = GetPointerS<const gimple_multi_way_if>(pred_bb->CGetStmtList().back());
   const auto old_ce = curr_bb->CGetStmtList().back();
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Gimple multi way if is " + old_gwi->ToString());
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Gimple cond " + old_ce->ToString());

   /// Remove old gimple_multi_way_if
   pred_bb->RemoveStmt(pred_bb->CGetStmtList().back(), AppM);

   /// Remove old gimple_cond
   while(curr_bb->CGetStmtList().size())
   {
      curr_bb->RemoveStmt(curr_bb->CGetStmtList().front(), AppM);
   }

   /// Create condition
   const auto ce_cond = tree_man->ExtractCondition(old_ce, pred_bb, function_id);
   for(const auto& old_cond : old_gwi->list_of_cond)
   {
      /// Non default and succ is on this edge
      if(old_cond.first && old_cond.second == curr_bb->number)
      {
         const auto true_cond = tree_man->CreateAndExpr(old_cond.first, ce_cond, pred_bb, function_id);
         new_gwi->add_cond(true_cond, curr_bb->true_edge);
         const auto false_cond = tree_man->CreateAndExpr(
             old_cond.first, tree_man->CreateNotExpr(ce_cond, pred_bb, function_id), pred_bb, function_id);
         new_gwi->add_cond(false_cond, curr_bb->false_edge);
      }
      /// Non default and succ is not on this edge
      else if(old_cond.first)
      {
         new_gwi->add_cond(old_cond.first, old_cond.second);
      }
      /// Default and succ is is on this edge
      else if(old_cond.second == curr_bb->number)
      {
         /// Building the and of the not of other conditions of cond
         tree_nodeRef not_cond = tree_nodeRef();
         for(const auto& other_old_cond : old_gwi->list_of_cond)
         {
            if(other_old_cond.first)
            {
               if(not_cond)
               {
                  not_cond = tree_man->CreateAndExpr(
                      not_cond, tree_man->CreateNotExpr(other_old_cond.first, pred_bb, function_id), pred_bb,
                      function_id);
               }
               else
               {
                  not_cond = tree_man->CreateNotExpr(other_old_cond.first, pred_bb, function_id);
               }
            }
         }
         const auto true_cond = tree_man->CreateAndExpr(not_cond, ce_cond, pred_bb, function_id);
         new_gwi->add_cond(true_cond, curr_bb->true_edge);
         new_gwi->add_cond(tree_nodeRef(), curr_bb->false_edge);
      }
      /// Default and second is not on this edge
      else
      {
         new_gwi->add_cond(tree_nodeRef(), old_cond.second);
      }
   }
   pred_bb->PushBack(TM->GetTreeNode(gimple_multi_way_if_id), AppM);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + new_gwi->ToString());
}

void multi_way_if::MergeCondCond(const blocRef& pred_bb, const blocRef& curr_bb)
{
   /// identify the first gimple_cond
   const auto list_of_stmt_cond1 = pred_bb->CGetStmtList();
   THROW_ASSERT(list_of_stmt_cond1.back()->get_kind() == gimple_cond_K, "a gimple_cond is expected");
   const auto cond1_statement = list_of_stmt_cond1.back();
   pred_bb->RemoveStmt(cond1_statement, AppM);
   const auto ssa1_node = tree_man->ExtractCondition(cond1_statement, pred_bb, function_id);

   /// identify the second gimple_cond
   const auto list_of_stmt_cond2 = curr_bb->CGetStmtList();
   THROW_ASSERT(list_of_stmt_cond2.back()->get_kind() == gimple_cond_K, "a gimple_cond is expected");
   const auto cond2_statement = list_of_stmt_cond2.back();
   curr_bb->RemoveStmt(cond2_statement, AppM);
   const auto ssa2_node = tree_man->ExtractCondition(cond2_statement, pred_bb, function_id);

   /// create the gimple_multi_way_if node
   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;
   const auto gimple_multi_way_if_id = TM->new_tree_node_id();
   IR_schema[TOK(TOK_SRCP)] = BUILTIN_SRCP;
   IR_schema[TOK(TOK_SCPE)] = STR(function_id);
   TM->create_tree_node(gimple_multi_way_if_id, gimple_multi_way_if_K, IR_schema);
   IR_schema.clear();
   const auto gimple_multi_way_if_stmt = TM->GetTreeNode(gimple_multi_way_if_id);
   auto* gmwi = GetPointerS<gimple_multi_way_if>(gimple_multi_way_if_stmt);
   gmwi->bb_index = pred_bb->number;
   if(pred_bb->false_edge == curr_bb->number)
   {
      gmwi->add_cond(ssa1_node, pred_bb->true_edge);
      const auto res_and = tree_man->CreateAndExpr(ssa2_node, tree_man->CreateNotExpr(ssa1_node, pred_bb, function_id),
                                                   pred_bb, function_id);
      gmwi->add_cond(res_and, curr_bb->true_edge);
      gmwi->add_cond(tree_nodeRef(), curr_bb->false_edge);
   }
   else
   {
      const auto res_not = tree_man->CreateNotExpr(ssa1_node, pred_bb, function_id);
      gmwi->add_cond(res_not, pred_bb->false_edge);
      const auto res_and2 = tree_man->CreateAndExpr(ssa1_node, ssa2_node, pred_bb, function_id);
      gmwi->add_cond(res_and2, curr_bb->true_edge);
      gmwi->add_cond(tree_nodeRef(), curr_bb->false_edge);
   }
   pred_bb->PushBack(gimple_multi_way_if_stmt, AppM);
   pred_bb->false_edge = 0;
   pred_bb->true_edge = 0;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + gmwi->ToString());
}

void multi_way_if::FixCfg(const blocRef& pred_bb, const blocRef& succ_bb)
{
   /// The index of the basic block to be created
   const unsigned int new_basic_block_index = (sl->list_of_bloc.rbegin())->first + 1;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Adding BB" + STR(new_basic_block_index));

   /// Create the new basic block and set all the fields
   const auto new_block = blocRef(new bloc(new_basic_block_index));
   sl->list_of_bloc[new_basic_block_index] = new_block;

   new_block->list_of_pred.push_back(pred_bb->number);
   new_block->list_of_succ.push_back(succ_bb->number);
   new_block->loop_id = succ_bb->loop_id;
   new_block->SetSSAUsesComputed();
   new_block->schedule = pred_bb->schedule;

   /// Fix the predecessor
   THROW_ASSERT(std::find(pred_bb->list_of_succ.begin(), pred_bb->list_of_succ.end(), succ_bb->number) !=
                    pred_bb->list_of_succ.end(),
                "");
   pred_bb->list_of_succ.erase(std::remove(pred_bb->list_of_succ.begin(), pred_bb->list_of_succ.end(), succ_bb->number),
                               pred_bb->list_of_succ.end());
   pred_bb->list_of_succ.push_back(new_basic_block_index);
   if(pred_bb->true_edge == succ_bb->number)
   {
      pred_bb->true_edge = new_basic_block_index;
   }
   if(pred_bb->false_edge == succ_bb->number)
   {
      pred_bb->false_edge = new_basic_block_index;
   }

   /// Fix the last statement of the predecessor
   const auto& pred_list_of_stmt = pred_bb->CGetStmtList();
   THROW_ASSERT(pred_list_of_stmt.size(), "Unexpexted condition");
   if(pred_list_of_stmt.back()->get_kind() == gimple_multi_way_if_K)
   {
      auto gmwi = GetPointerS<gimple_multi_way_if>(pred_list_of_stmt.back());
      for(auto& cond : gmwi->list_of_cond)
      {
         if(cond.second == succ_bb->number)
         {
            cond.second = new_basic_block_index;
         }
      }
   }

   /// Fix the successor
   succ_bb->list_of_pred.erase(std::remove(succ_bb->list_of_pred.begin(), succ_bb->list_of_pred.end(), pred_bb->number),
                               succ_bb->list_of_pred.end());
   succ_bb->list_of_pred.push_back(new_basic_block_index);

   /// Fix the phi
   for(const auto& phi : succ_bb->CGetPhiList())
   {
      auto gp = GetPointer<gimple_phi>(phi);
      for(auto& def_edge : gp->CGetDefEdgesList())
      {
         if(def_edge.second == pred_bb->number)
         {
            gp->ReplaceDefEdge(TM, def_edge, gimple_phi::DefEdge(def_edge.first, new_basic_block_index));
         }
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Added BB" + STR(new_basic_block_index));
}

bool multi_way_if::HasToBeExecuted() const
{
   if(!FunctionFrontendFlowStep::HasToBeExecuted())
   {
      return false;
   }

#if HAVE_ILP_BUILT
   if(parameters->getOption<HLSFlowStep_Type>(OPT_scheduling_algorithm) == HLSFlowStep_Type::SDC_SCHEDULING)
   {
      if(GetPointer<const HLS_manager>(AppM) and GetPointer<const HLS_manager>(AppM)->get_HLS(function_id) and
         GetPointer<const HLS_manager>(AppM)->get_HLS(function_id)->Rsch)
      {
         /// If schedule is not up to date, do not execute this step and invalidate UpdateSchedule
         const auto update_schedule = design_flow_manager.lock()->GetDesignFlowStep(
             FunctionFrontendFlowStep::ComputeSignature(FrontendFlowStepType::UPDATE_SCHEDULE, function_id));
         if(update_schedule != DesignFlowGraph::null_vertex())
         {
            const DesignFlowGraphConstRef design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
            const DesignFlowStepRef design_flow_step =
                design_flow_graph->CGetNodeInfo(update_schedule)->design_flow_step;
            if(GetPointer<const FunctionFrontendFlowStep>(design_flow_step)->CGetBBVersion() !=
               function_behavior->GetBBVersion())
            {
               return false;
            }
            else
            {
               return true;
            }
         }
         else
         {
            return false;
         }
      }
      else
      {
         return true;
      }
   }
#endif

   /// Multi way if can be executed only after vectorization
   if(parameters->getOption<int>(OPT_gcc_openmp_simd))
   {
      const auto vectorize_vertex =
          design_flow_manager.lock()->GetDesignFlowStep(ComputeSignature(FrontendFlowStepType::VECTORIZE, function_id));
      if(vectorize_vertex == DesignFlowGraph::null_vertex())
      {
         return false;
      }
      const auto vectorize_step =
          design_flow_manager.lock()->CGetDesignFlowGraph()->CGetNodeInfo(vectorize_vertex)->design_flow_step;
      if(GetPointer<const FunctionFrontendFlowStep>(vectorize_step)->CGetBBVersion() == 0)
      {
         return false;
      }
   }

   return true;
}
