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
 * @file multi_way_if.cpp
 * @brief Analysis step rebuilding multi-way if.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

/// Header include
#include "multi_way_if.hpp"

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

/// design_flows includes
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"

/// hls includes
#include "hls.hpp"
#include "hls_manager.hpp"
#if HAVE_ILP_BUILT
#include "hls_step.hpp"
#endif

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

multi_way_if::multi_way_if(const ParameterConstRef _parameters, const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, MULTI_WAY_IF, _design_flow_manager, _parameters), sl(nullptr), bb_modified(false)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

multi_way_if::~multi_way_if() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> multi_way_if::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(SWITCH_FIX, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(BLOCK_FIX, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(USE_COUNTING, SAME_FUNCTION));
#if HAVE_BAMBU_BUILT && HAVE_ILP_BUILT
         if(parameters->getOption<HLSFlowStep_Type>(OPT_scheduling_algorithm) == HLSFlowStep_Type::SDC_SCHEDULING)
         {
            relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(UPDATE_SCHEDULE, SAME_FUNCTION));
         }
#endif
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
#if HAVE_BAMBU_BUILT && HAVE_ILP_BUILT
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(SDC_CODE_MOTION, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(UPDATE_SCHEDULE, SAME_FUNCTION));
#endif
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
   tree_man = tree_manipulationRef(new tree_manipulation(TM, parameters));
   const auto temp = TM->get_tree_node_const(function_id);
   auto fd = GetPointer<function_decl>(temp);
   sl = GetPointer<statement_list>(GET_NODE(fd->body));
#if HAVE_ILP_BUILT
   if(parameters->getOption<HLSFlowStep_Type>(OPT_scheduling_algorithm) == HLSFlowStep_Type::SDC_SCHEDULING and GetPointer<const HLS_manager>(AppM) and GetPointer<const HLS_manager>(AppM)->get_HLS(function_id) and
      GetPointer<const HLS_manager>(AppM)->get_HLS(function_id)->Rsch)
   {
      for(const auto& block : sl->list_of_bloc)
      {
         block.second->schedule = GetPointer<const HLS_manager>(AppM)->get_HLS(function_id)->Rsch;
      }
   }
#endif
}

void multi_way_if::UpdateCfg(unsigned int pred_bb, unsigned int curr_bb)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Updating control flow graph");
   /// Remove curr_bb from successor of pred_bb
   if(std::find(sl->list_of_bloc[pred_bb]->list_of_succ.begin(), sl->list_of_bloc[pred_bb]->list_of_succ.end(), curr_bb) != sl->list_of_bloc[pred_bb]->list_of_succ.end())
      sl->list_of_bloc[pred_bb]->list_of_succ.erase(std::find(sl->list_of_bloc[pred_bb]->list_of_succ.begin(), sl->list_of_bloc[pred_bb]->list_of_succ.end(), curr_bb));

   /// For each successor succ of curr_bb
   for(auto succ : sl->list_of_bloc[curr_bb]->list_of_succ)
   {
      /// Remove curr_bb from its predecessor
      if(sl->list_of_bloc[succ]->list_of_pred.begin() != sl->list_of_bloc[succ]->list_of_pred.end())
      {
         while(std::find(sl->list_of_bloc[succ]->list_of_pred.begin(), sl->list_of_bloc[succ]->list_of_pred.end(), curr_bb) != sl->list_of_bloc[succ]->list_of_pred.end())
            sl->list_of_bloc[succ]->list_of_pred.erase(std::find(sl->list_of_bloc[succ]->list_of_pred.begin(), sl->list_of_bloc[succ]->list_of_pred.end(), curr_bb));
      }

      /// Add pred_bb to its predecessor
      if(std::find(sl->list_of_bloc[succ]->list_of_pred.begin(), sl->list_of_bloc[succ]->list_of_pred.end(), pred_bb) == sl->list_of_bloc[succ]->list_of_pred.end())
         sl->list_of_bloc[succ]->list_of_pred.push_back(pred_bb);

      /// Add succ to successor of pred_bb
      if(std::find(sl->list_of_bloc[pred_bb]->list_of_succ.begin(), sl->list_of_bloc[pred_bb]->list_of_succ.end(), succ) == sl->list_of_bloc[pred_bb]->list_of_succ.end())
         sl->list_of_bloc[pred_bb]->list_of_succ.push_back(succ);

      /// Update phi information
      for(const auto& phi : sl->list_of_bloc[succ]->CGetPhiList())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Original phi " + phi->ToString());
         auto* current_phi = GetPointer<gimple_phi>(GET_NODE(phi));
         for(const auto& def_edge : current_phi->CGetDefEdgesList())
         {
            if(def_edge.second == curr_bb)
               current_phi->ReplaceDefEdge(TM, def_edge, gimple_phi::DefEdge(def_edge.first, pred_bb));
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Modified phi " + phi->ToString());
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Updated control flow graph");
}

DesignFlowStep_Status multi_way_if::InternalExec()
{
   CustomUnorderedMap<unsigned int, vertex> inverse_vertex_map;
   BBGraphsCollectionRef GCC_bb_graphs_collection(new BBGraphsCollection(BBGraphInfoRef(new BBGraphInfo(AppM, function_id)), parameters));
   BBGraphRef GCC_bb_graph(new BBGraph(GCC_bb_graphs_collection, CFG_SELECTOR));

   CustomOrderedSet<unsigned int> bb_to_be_removed;
   for(auto block : sl->list_of_bloc)
   {
      inverse_vertex_map[block.first] = GCC_bb_graphs_collection->AddVertex(BBNodeInfoRef(new BBNodeInfo(block.second)));
   }
   /// add edges
   for(auto bb : sl->list_of_bloc)
   {
      unsigned int curr_bb = bb.first;
      std::vector<unsigned int>::const_iterator lop_it_end = sl->list_of_bloc[curr_bb]->list_of_pred.end();
      for(std::vector<unsigned int>::const_iterator lop_it = sl->list_of_bloc[curr_bb]->list_of_pred.begin(); lop_it != lop_it_end; ++lop_it)
      {
         GCC_bb_graphs_collection->AddEdge(inverse_vertex_map[*lop_it], inverse_vertex_map[curr_bb], CFG_SELECTOR);
      }
      for(auto succ : sl->list_of_bloc[curr_bb]->list_of_succ)
      {
         if(succ == bloc::EXIT_BLOCK_ID)
            GCC_bb_graphs_collection->AddEdge(inverse_vertex_map[curr_bb], inverse_vertex_map[succ], CFG_SELECTOR);
      }
      if(sl->list_of_bloc[curr_bb]->list_of_succ.empty())
         GCC_bb_graphs_collection->AddEdge(inverse_vertex_map[curr_bb], inverse_vertex_map[bloc::EXIT_BLOCK_ID], CFG_SELECTOR);
   }
   /// add a connection between entry and exit thus avoiding problems with non terminating code
   GCC_bb_graphs_collection->AddEdge(inverse_vertex_map[bloc::ENTRY_BLOCK_ID], inverse_vertex_map[bloc::EXIT_BLOCK_ID], CFG_SELECTOR);
   /// sort basic block vertices from the entry till the exit
   std::list<vertex> bb_sorted_vertices;
   cyclic_topological_sort(*GCC_bb_graph, std::front_inserter(bb_sorted_vertices));
   for(auto bb : bb_sorted_vertices)
   {
      auto bb_node_info = GCC_bb_graph->GetBBNodeInfo(bb);
      unsigned int curr_bb = bb_node_info->block->number;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining BB" + STR(curr_bb));
      if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
      {
         WriteBBGraphDot("BB_Before_" + GetName() + "_" + STR(curr_bb) + ".dot");
      }
      if(curr_bb == bloc::ENTRY_BLOCK_ID || curr_bb == bloc::EXIT_BLOCK_ID)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped because entry or exit");
         continue;
      }
      if(bb_node_info->block->list_of_pred.size() > 1)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped because it has multiple predecessors");
         continue;
      }
      unsigned int pred = bb_node_info->block->list_of_pred.front();
      if(pred == bloc::ENTRY_BLOCK_ID)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped because predecessor is entry");
         continue;
      }
      if(sl->list_of_bloc[pred]->CGetStmtList().empty())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped because predecessor is entry");
         continue;
      }
      tree_nodeRef last_pred_stmt = GET_NODE(sl->list_of_bloc[pred]->CGetStmtList().back());
      if(last_pred_stmt->get_kind() != gimple_cond_K and last_pred_stmt->get_kind() != gimple_multi_way_if_K)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped because predecessor ends with " + last_pred_stmt->get_kind_text());
         continue;
      }
      if(sl->list_of_bloc[curr_bb]->CGetStmtList().size() != 1)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped because it is not a singleton");
         continue;
      }
      tree_nodeRef last_curr_stmt = GET_NODE(sl->list_of_bloc[curr_bb]->CGetStmtList().back());
#if 0
      if(last_curr_stmt->get_kind() == gimple_multi_way_if_K)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped because it contains a multi way if");
         continue;
      }
#endif
      if(last_curr_stmt->get_kind() != gimple_cond_K and last_curr_stmt->get_kind() != gimple_multi_way_if_K)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped because it ends with a " + last_curr_stmt->get_kind_text());
         continue;
      }
      if(sl->list_of_bloc[curr_bb]->CGetPhiList().size() != 0)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped because it contains an if");
         continue;
      }
#ifndef NDEBUG
      if(not AppM->ApplyNewTransformation())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Reached limit of cfg transformations");
         continue;
      }
      AppM->RegisterTransformation(GetName(), tree_nodeConstRef());
#endif
      /// check for short circuit conditions: i.e., if they have at least a successor in common
      /// if so we add a basic block on the shortest path (e.g., predecessor --> common successor)
      /// In this way in the produced gimple_multi_way_if there cannot be multiple conditions with the same next bb
      /// phi_opt will remove the extra basic block
      bool restart = false;
      const auto& curr_list_of_succ = sl->list_of_bloc[curr_bb]->list_of_succ;
      do
      {
         restart = false;
         for(auto succ_bb : sl->list_of_bloc[pred]->list_of_succ)
         {
            if(std::find(curr_list_of_succ.begin(), curr_list_of_succ.end(), succ_bb) != curr_list_of_succ.end())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---BB" + STR(succ_bb) + " is a common sucessor");
               FixCfg(curr_bb, succ_bb);
               restart = true;
               if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
               {
                  WriteBBGraphDot("BB_After_" + GetName() + "_" + STR(curr_bb) + "_Fix.dot");
               }
               break;
            }
         }
      }
      /// Fixed point since list of successor is changed by FixCfg
      while(restart);

      /// now the merging starts
      if(last_pred_stmt->get_kind() == gimple_cond_K and last_curr_stmt->get_kind() == gimple_cond_K)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Merging gimple_cond with gimple_cond (BB" + STR(curr_bb) + " with BB" + STR(pred) + ")");
         MergeCondCond(pred, curr_bb);
      }
      else if(last_pred_stmt->get_kind() == gimple_cond_K and last_curr_stmt->get_kind() == gimple_multi_way_if_K)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Merging gimple_cond with gimple_multi_way_if (BB" + STR(curr_bb) + " with BB" + STR(pred) + ")");
         MergeCondMulti(pred, curr_bb);
      }
      else if(last_pred_stmt->get_kind() == gimple_multi_way_if_K and last_curr_stmt->get_kind() == gimple_multi_way_if_K)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Merging gimple_multi_way_if with gimple_multi_way_if (BB" + STR(curr_bb) + " with BB" + STR(pred) + ")");
         MergeMultiMulti(pred, curr_bb);
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Merging gimple_multi_way_if with gimple_cond (BB" + STR(curr_bb) + " with BB" + STR(pred) + ")");
         MergeMultiCond(pred, curr_bb);
      }
      UpdateCfg(pred, curr_bb);
      bb_modified = true;
      bb_to_be_removed.insert(curr_bb);
      if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
      {
         WriteBBGraphDot("BB_After_" + GetName() + "_" + STR(curr_bb) + ".dot");
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

void multi_way_if::MergeCondMulti(const unsigned int pred_bb, const unsigned int curr_bb)
{
   /// create the gimple_multi_way_if node
   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;
   unsigned int gimple_multi_way_if_id = TM->new_tree_node_id();
   IR_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
   TM->create_tree_node(gimple_multi_way_if_id, gimple_multi_way_if_K, IR_schema);
   auto new_gwi = GetPointer<gimple_multi_way_if>(TM->get_tree_node_const(gimple_multi_way_if_id));
   new_gwi->bb_index = pred_bb;

   auto old_gwi = GetPointer<gimple_multi_way_if>(GET_NODE(sl->list_of_bloc[curr_bb]->CGetStmtList().back()));

   /// Create ce_condition
   auto ce_cond = tree_man->ExtractCondition(sl->list_of_bloc[pred_bb]->CGetStmtList().back(), sl->list_of_bloc[pred_bb]);

   /// Remove old gimple_cond
   sl->list_of_bloc[pred_bb]->RemoveStmt(sl->list_of_bloc[pred_bb]->CGetStmtList().back());

   /// Remove old gimple multi way if
   while(sl->list_of_bloc[curr_bb]->CGetStmtList().size())
      sl->list_of_bloc[curr_bb]->RemoveStmt(sl->list_of_bloc[curr_bb]->CGetStmtList().front());

   /// First case: second bb is on the true edge
   if(sl->list_of_bloc[pred_bb]->true_edge == curr_bb)
   {
      auto default_bb = 0u;
      for(auto old_cond : old_gwi->list_of_cond)
      {
         if(old_cond.first)
         {
            const auto new_cond = tree_man->CreateAndExpr(ce_cond, old_cond.first, sl->list_of_bloc[pred_bb]);
            new_gwi->add_cond(new_cond, old_cond.second);
         }
         else
         {
            default_bb = old_cond.second;
            /// Skipped default condition
         }
      }
      new_gwi->add_cond(tree_man->CreateNotExpr(ce_cond, sl->list_of_bloc[pred_bb]), sl->list_of_bloc[pred_bb]->false_edge);
      new_gwi->add_cond(tree_nodeRef(), default_bb);
   }
   /// Second case: second bb is on the false edge
   else
   {
      new_gwi->add_cond(ce_cond, sl->list_of_bloc[pred_bb]->true_edge);
      for(auto old_cond : old_gwi->list_of_cond)
      {
         const auto not_second = tree_man->CreateNotExpr(ce_cond, sl->list_of_bloc[pred_bb]);
         const tree_nodeRef new_cond = old_cond.first ? tree_man->CreateAndExpr(old_cond.first, tree_man->CreateNotExpr(ce_cond, sl->list_of_bloc[pred_bb]), sl->list_of_bloc[pred_bb]) : tree_nodeRef();
         new_gwi->add_cond(new_cond, old_cond.second);
      }
   }
   sl->list_of_bloc[pred_bb]->PushBack(TM->GetTreeReindex(gimple_multi_way_if_id));
   sl->list_of_bloc[pred_bb]->false_edge = 0;
   sl->list_of_bloc[pred_bb]->true_edge = 0;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + new_gwi->ToString());
}

void multi_way_if::MergeMultiMulti(const unsigned int pred_bb, const unsigned int curr_bb)
{
   /// create the gimple_multi_way_if node
   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;
   unsigned int gimple_multi_way_if_id = TM->new_tree_node_id();
   IR_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
   TM->create_tree_node(gimple_multi_way_if_id, gimple_multi_way_if_K, IR_schema);
   auto new_gwi = GetPointer<gimple_multi_way_if>(TM->get_tree_node_const(gimple_multi_way_if_id));
   new_gwi->bb_index = pred_bb;

   auto old_gwi1 = GetPointer<gimple_multi_way_if>(GET_NODE(sl->list_of_bloc[pred_bb]->CGetStmtList().back()));
   auto old_gwi2 = GetPointer<gimple_multi_way_if>(GET_NODE(sl->list_of_bloc[curr_bb]->CGetStmtList().back()));
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---First gimple multi way if is " + old_gwi1->ToString());
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Second gimple multi way if is " + old_gwi2->ToString());

   /// Remove old gimple_cond
   sl->list_of_bloc[pred_bb]->RemoveStmt(sl->list_of_bloc[pred_bb]->CGetStmtList().back());

   /// Remove old gimple multi way if
   while(sl->list_of_bloc[curr_bb]->CGetStmtList().size())
      sl->list_of_bloc[curr_bb]->RemoveStmt(sl->list_of_bloc[curr_bb]->CGetStmtList().front());

   for(auto old_cond1 : old_gwi1->list_of_cond)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering condition " + (old_cond1.first ? old_cond1.first->ToString() : " default"));
      /// Non default and succ is on this edge
      if(old_cond1.first and old_cond1.second == curr_bb)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---It is not default and nested gimple_multi_way_if is on this edge");
         for(auto old_cond2 : old_gwi2->list_of_cond)
         {
            if(old_cond2.first)
            {
               const auto new_cond = tree_man->CreateAndExpr(old_cond1.first, old_cond2.first, sl->list_of_bloc[pred_bb]);
               new_gwi->add_cond(new_cond, old_cond2.second);
            }
            else
            {
               tree_nodeRef new_cond = tree_nodeRef();
               for(auto other_old_cond2 : old_gwi2->list_of_cond)
               {
                  if(other_old_cond2.first)
                  {
                     if(new_cond)
                     {
                        new_cond = tree_man->CreateAndExpr(new_cond, tree_man->CreateNotExpr(other_old_cond2.first, sl->list_of_bloc[pred_bb]), sl->list_of_bloc[pred_bb]);
                     }
                     else
                     {
                        new_cond = tree_man->CreateNotExpr(other_old_cond2.first, sl->list_of_bloc[pred_bb]);
                     }
                  }
               }
               new_cond = tree_man->CreateAndExpr(new_cond, old_cond1.first, sl->list_of_bloc[pred_bb]);
               new_gwi->add_cond(new_cond, old_cond2.second);
            }
         }
      }
      /// Non default and succ is not on this edge
      else if(old_cond1.first)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---It is not default and nested gimple_multi_way_if is not on this edge");
         new_gwi->add_cond(old_cond1.first, old_cond1.second);
      }
      /// Default and succ is on this edge
      else if(old_cond1.second == curr_bb)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---It is default and nested gimple_multi_way_if is on this edge");
         /// Building the and of the not of other conditions of cond1
         tree_nodeRef not_cond = tree_nodeRef();
         for(auto other_old_cond1 : old_gwi1->list_of_cond)
         {
            if(other_old_cond1.first)
            {
               if(not_cond)
               {
                  not_cond = tree_man->CreateAndExpr(not_cond, tree_man->CreateNotExpr(other_old_cond1.first, sl->list_of_bloc[pred_bb]), sl->list_of_bloc[pred_bb]);
               }
               else
               {
                  not_cond = tree_man->CreateNotExpr(other_old_cond1.first, sl->list_of_bloc[pred_bb]);
               }
            }
         }
         for(auto old_cond2 : old_gwi2->list_of_cond)
         {
            if(old_cond2.first)
            {
               const auto new_cond = tree_man->CreateAndExpr(not_cond, old_cond2.first, sl->list_of_bloc[pred_bb]);
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
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---It is default and nested gimple_multi_way_if is not on this edge");
         new_gwi->add_cond(tree_nodeRef(), old_cond1.second);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered condition " + (old_cond1.first ? old_cond1.first->ToString() : " default"));
   }
   sl->list_of_bloc[pred_bb]->PushBack(TM->GetTreeReindex(gimple_multi_way_if_id));
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + new_gwi->ToString());
}

void multi_way_if::MergeMultiCond(const unsigned int pred_bb, const unsigned int curr_bb)
{
   /// create the gimple_multi_way_if node
   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;
   unsigned int gimple_multi_way_if_id = TM->new_tree_node_id();
   IR_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
   TM->create_tree_node(gimple_multi_way_if_id, gimple_multi_way_if_K, IR_schema);
   auto new_gwi = GetPointer<gimple_multi_way_if>(TM->get_tree_node_const(gimple_multi_way_if_id));
   new_gwi->bb_index = pred_bb;

   auto old_gwi = GetPointer<gimple_multi_way_if>(GET_NODE(sl->list_of_bloc[pred_bb]->CGetStmtList().back()));
   auto old_ce = sl->list_of_bloc[curr_bb]->CGetStmtList().back();
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Gimple multi way if is " + old_gwi->ToString());
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Gimple cond " + old_ce->ToString());

   /// Remove old gimple_multi_way_if
   sl->list_of_bloc[pred_bb]->RemoveStmt(sl->list_of_bloc[pred_bb]->CGetStmtList().back());

   /// Remove old gimple_cond
   while(sl->list_of_bloc[curr_bb]->CGetStmtList().size())
      sl->list_of_bloc[curr_bb]->RemoveStmt(sl->list_of_bloc[curr_bb]->CGetStmtList().front());

   /// Create condition
   auto ce_cond = tree_man->ExtractCondition(old_ce, sl->list_of_bloc[pred_bb]);
   for(auto old_cond : old_gwi->list_of_cond)
   {
      /// Non default and succ is on this edge
      if(old_cond.first and old_cond.second == curr_bb)
      {
         const tree_nodeRef true_cond = tree_man->CreateAndExpr(old_cond.first, ce_cond, sl->list_of_bloc[pred_bb]);
         new_gwi->add_cond(true_cond, sl->list_of_bloc[curr_bb]->true_edge);
         const tree_nodeRef false_cond = tree_man->CreateAndExpr(old_cond.first, tree_man->CreateNotExpr(ce_cond, sl->list_of_bloc[pred_bb]), sl->list_of_bloc[pred_bb]);
         new_gwi->add_cond(false_cond, sl->list_of_bloc[curr_bb]->false_edge);
      }
      /// Non default and succ is not on this edge
      else if(old_cond.first)
      {
         new_gwi->add_cond(old_cond.first, old_cond.second);
      }
      /// Default and succ is is on this edge
      else if(old_cond.second == curr_bb)
      {
         /// Building the and of the not of other conditions of cond
         tree_nodeRef not_cond = tree_nodeRef();
         for(auto other_old_cond : old_gwi->list_of_cond)
         {
            if(other_old_cond.first)
            {
               if(not_cond)
               {
                  not_cond = tree_man->CreateAndExpr(not_cond, tree_man->CreateNotExpr(other_old_cond.first, sl->list_of_bloc[pred_bb]), sl->list_of_bloc[pred_bb]);
               }
               else
               {
                  not_cond = tree_man->CreateNotExpr(other_old_cond.first, sl->list_of_bloc[pred_bb]);
               }
            }
         }
         const auto true_cond = tree_man->CreateAndExpr(not_cond, ce_cond, sl->list_of_bloc[pred_bb]);
         new_gwi->add_cond(true_cond, sl->list_of_bloc[curr_bb]->true_edge);
         new_gwi->add_cond(tree_nodeRef(), sl->list_of_bloc[curr_bb]->false_edge);
      }
      /// Default and second is not on this edge
      else
      {
         new_gwi->add_cond(tree_nodeRef(), old_cond.second);
      }
   }
   sl->list_of_bloc[pred_bb]->PushBack(TM->GetTreeReindex(gimple_multi_way_if_id));
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + new_gwi->ToString());
}

void multi_way_if::MergeCondCond(unsigned int pred, unsigned int curr_bb)
{
   /// identify the first gimple_cond
   const auto pred_block = sl->list_of_bloc[pred];
   const auto list_of_stmt_cond1 = pred_block->CGetStmtList();
   THROW_ASSERT(GET_NODE(list_of_stmt_cond1.back())->get_kind() == gimple_cond_K, "a gimple_cond is expected");
   tree_nodeRef cond1_statement = list_of_stmt_cond1.back();
   pred_block->RemoveStmt(cond1_statement);
   const auto ssa1_node = tree_man->ExtractCondition(cond1_statement, pred_block);

   /// identify the second gimple_cond
   const auto list_of_stmt_cond2 = sl->list_of_bloc[curr_bb]->CGetStmtList();
   THROW_ASSERT(GET_NODE(list_of_stmt_cond2.back())->get_kind() == gimple_cond_K, "a gimple_cond is expected");
   tree_nodeRef cond2_statement = list_of_stmt_cond2.back();
   sl->list_of_bloc[curr_bb]->RemoveStmt(cond2_statement);
   const auto ssa2_node = tree_man->ExtractCondition(cond2_statement, pred_block);

   /// create the gimple_multi_way_if node
   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;
   unsigned int gimple_multi_way_if_id = TM->new_tree_node_id();
   IR_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
   TM->create_tree_node(gimple_multi_way_if_id, gimple_multi_way_if_K, IR_schema);
   IR_schema.clear();
   tree_nodeRef gimple_multi_way_if_stmt = TM->GetTreeReindex(gimple_multi_way_if_id);
   GetPointer<gimple_node>(GET_NODE(gimple_multi_way_if_stmt))->bb_index = pred;
   auto* gmwi = GetPointer<gimple_multi_way_if>(GET_NODE(gimple_multi_way_if_stmt));
   gmwi->bb_index = pred;
   if(pred_block->false_edge == curr_bb)
   {
      std::pair<tree_nodeRef, unsigned int> cond1(ssa1_node, pred_block->true_edge);
      gmwi->list_of_cond.push_back(cond1);
      const auto res_and = tree_man->CreateAndExpr(ssa2_node, tree_man->CreateNotExpr(ssa1_node, pred_block), pred_block);
      std::pair<tree_nodeRef, unsigned int> cond2(res_and, sl->list_of_bloc[curr_bb]->true_edge);
      gmwi->list_of_cond.push_back(cond2);
      std::pair<tree_nodeRef, unsigned int> cond3(tree_nodeRef(), sl->list_of_bloc[curr_bb]->false_edge);
      gmwi->list_of_cond.push_back(cond3);
   }
   else
   {
      const auto res_not = tree_man->CreateNotExpr(ssa1_node, pred_block);
      std::pair<tree_nodeRef, unsigned int> cond1(res_not, sl->list_of_bloc[pred]->false_edge);
      gmwi->list_of_cond.push_back(cond1);
      const auto res_and2 = tree_man->CreateAndExpr(ssa1_node, ssa2_node, sl->list_of_bloc[pred]);
      std::pair<tree_nodeRef, unsigned int> cond2(res_and2, sl->list_of_bloc[curr_bb]->true_edge);
      gmwi->list_of_cond.push_back(cond2);

      std::pair<tree_nodeRef, unsigned int> cond3(tree_nodeRef(), sl->list_of_bloc[curr_bb]->false_edge);
      gmwi->list_of_cond.push_back(cond3);
   }
   pred_block->PushBack(gimple_multi_way_if_stmt);
   sl->list_of_bloc[pred]->false_edge = 0;
   sl->list_of_bloc[pred]->true_edge = 0;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + gmwi->ToString());
}

void multi_way_if::FixCfg(const unsigned int pred_bb, const unsigned int succ_bb)
{
   /// The index of the basic block to be created
   const unsigned int new_basic_block_index = (sl->list_of_bloc.rbegin())->first + 1;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Adding BB" + STR(new_basic_block_index));

   const auto pred_block = sl->list_of_bloc[pred_bb];
   const auto succ_block = sl->list_of_bloc[succ_bb];

   /// Create the new basic block and set all the fields
   const auto new_block = blocRef(new bloc(new_basic_block_index));
   sl->list_of_bloc[new_basic_block_index] = new_block;

   new_block->list_of_pred.push_back(pred_bb);
   new_block->list_of_succ.push_back(succ_bb);
   new_block->loop_id = succ_block->loop_id;
   new_block->SetSSAUsesComputed();
   new_block->schedule = pred_block->schedule;

   /// Fix the predecessor
   THROW_ASSERT(std::find(pred_block->list_of_succ.begin(), pred_block->list_of_succ.end(), succ_bb) != pred_block->list_of_succ.end(), "");
   pred_block->list_of_succ.erase(std::find(pred_block->list_of_succ.begin(), pred_block->list_of_succ.end(), succ_bb));
   pred_block->list_of_succ.push_back(new_basic_block_index);
   if(pred_block->true_edge == succ_bb)
      pred_block->true_edge = new_basic_block_index;
   if(pred_block->false_edge == succ_bb)
      pred_block->false_edge = new_basic_block_index;

   /// Fix the last statement of the predecessor
   auto& pred_list_of_stmt = pred_block->CGetStmtList();
   THROW_ASSERT(pred_list_of_stmt.size(), "Unexpexted condition");
   auto pred_last_stmt = GET_NODE(pred_list_of_stmt.back());
   if(pred_last_stmt->get_kind() == gimple_multi_way_if_K)
   {
      auto gmwi = GetPointer<gimple_multi_way_if>(pred_last_stmt);
      for(auto& cond : gmwi->list_of_cond)
      {
         if(cond.second == succ_bb)
         {
            cond.second = new_basic_block_index;
         }
      }
   }

   /// Fix the successor
   succ_block->list_of_pred.erase(std::find(succ_block->list_of_pred.begin(), succ_block->list_of_pred.end(), pred_bb));
   succ_block->list_of_pred.push_back(new_basic_block_index);

   /// Fix the phi
   for(auto phi : succ_block->CGetPhiList())
   {
      auto gp = GetPointer<gimple_phi>(GET_NODE(phi));
      for(auto& def_edge : gp->CGetDefEdgesList())
      {
         if(def_edge.second == pred_bb)
            gp->ReplaceDefEdge(TM, def_edge, gimple_phi::DefEdge(def_edge.first, new_basic_block_index));
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Added BB" + STR(new_basic_block_index));
}

bool multi_way_if::HasToBeExecuted() const
{
#if HAVE_ILP_BUILT
   if((parameters->getOption<HLSFlowStep_Type>(OPT_scheduling_algorithm) == HLSFlowStep_Type::SDC_SCHEDULING) and GetPointer<const HLS_manager>(AppM) and not GetPointer<const HLS_manager>(AppM)->get_HLS(function_id))
      return false;
#endif

   /// Multi way if can be executed only after vectorization
   if(parameters->getOption<int>(OPT_gcc_openmp_simd))
   {
      const auto vectorize_vertex = design_flow_manager.lock()->GetDesignFlowStep(ComputeSignature(FrontendFlowStepType::VECTORIZE, function_id));
      if(vectorize_vertex == NULL_VERTEX)
      {
         return false;
      }
      const auto vectorize_step = design_flow_manager.lock()->CGetDesignFlowGraph()->CGetDesignFlowStepInfo(vectorize_vertex)->design_flow_step;
      if(GetPointer<const FunctionFrontendFlowStep>(vectorize_step)->CGetBBVersion() == 0)
      {
         return false;
      }
   }

   return FunctionFrontendFlowStep::HasToBeExecuted();
}
