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
 * @file multi_way_if.cpp
 * @brief Analysis step rebuilding multi-way if.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "multi_way_if.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "behavioral_helper.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "dbgPrintHelper.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "function_behavior.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"
#include "hls_step.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_manipulation.hpp"
#include "schedule.hpp"
#include "string_manipulation.hpp"
#include "token_interface.hpp"
#include <cstdlib>
#include <fstream>

multi_way_if::multi_way_if(const ParameterConstRef _parameters, const application_managerRef _AppM,
                           unsigned int _function_id, const DesignFlowManager& _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, MULTI_WAY_IF, _design_flow_manager, _parameters),
      sl(nullptr),
      bb_modified(false)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
multi_way_if::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(USE_COUNTING, SAME_FUNCTION));
         if(parameters->getOption<HLSFlowStep_Type>(OPT_scheduling_algorithm) == HLSFlowStep_Type::SDC_SCHEDULING)
         {
            relationships.insert(std::make_pair(UPDATE_SCHEDULE, SAME_FUNCTION));
         }
         relationships.insert(std::make_pair(BITVALUE_RANGE, SAME_FUNCTION));
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
               const auto update_schedule = design_flow_manager.GetDesignFlowStep(
                   FunctionFrontendFlowStep::ComputeSignature(FrontendFlowStepType::UPDATE_SCHEDULE, function_id));
               if(update_schedule != DesignFlowGraph::null_vertex())
               {
                  const DesignFlowGraphConstRef design_flow_graph = design_flow_manager.CGetDesignFlowGraph();
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
         relationships.insert(std::make_pair(BLOCK_FIX, SAME_FUNCTION));
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
   TM = AppM->get_ir_manager();
   ir_man = ir_manipulationRef(new ir_manipulation(TM, parameters, AppM));
   const auto temp = TM->GetIRNode(function_id);
   const auto fd = GetPointerS<const function_val_node>(temp);
   sl = GetPointerS<statement_list_node>(fd->body);
   if(parameters->getOption<HLSFlowStep_Type>(OPT_scheduling_algorithm) == HLSFlowStep_Type::SDC_SCHEDULING and
      GetPointer<const HLS_manager>(AppM) and GetPointer<const HLS_manager>(AppM)->get_HLS(function_id) and
      GetPointer<const HLS_manager>(AppM)->get_HLS(function_id)->Rsch)
   {
      for(const auto& block : sl->list_of_bloc)
      {
         block.second->schedule = GetPointer<const HLS_manager>(AppM)->get_HLS(function_id)->Rsch;
      }
   }
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
         auto* current_phi = GetPointerS<phi_stmt>(phi);
         for(const auto& def_edge : current_phi->CGetDefEdgesList())
         {
            if(def_edge.second == curr_bb->number)
            {
               current_phi->ReplaceDefEdge(TM, def_edge, phi_stmt::DefEdge(def_edge.first, pred_bb->number));
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Modified phi " + phi->ToString());
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Updated control flow graph");
}

DesignFlowStep_Status multi_way_if::InternalExec()
{
   CustomUnorderedMap<unsigned int, BBGraph::vertex_descriptor> inverse_vertex_map;
   BBGraphsCollection bb_graphs_collection(BBGraphInfo(AppM, function_id));
   BBGraph bb_graph(bb_graphs_collection, CFG_SELECTOR);

   CustomOrderedSet<unsigned int> bb_to_be_removed;
   for(const auto& block : sl->list_of_bloc)
   {
      inverse_vertex_map[block.first] = bb_graphs_collection.AddVertex(BBNodeInfo(block.second));
   }
   /// add edges
   for(const auto& [bbi, bb] : sl->list_of_bloc)
   {
      for(const auto pred : bb->list_of_pred)
      {
         bb_graphs_collection.AddEdge(inverse_vertex_map[pred], inverse_vertex_map[bbi], CFG_SELECTOR);
      }
      for(const auto succ : bb->list_of_succ)
      {
         if(succ == bloc::EXIT_BLOCK_ID)
         {
            bb_graphs_collection.AddEdge(inverse_vertex_map[bbi], inverse_vertex_map[succ], CFG_SELECTOR);
         }
      }
      if(bb->list_of_succ.empty())
      {
         bb_graphs_collection.AddEdge(inverse_vertex_map[bbi], inverse_vertex_map[bloc::EXIT_BLOCK_ID], CFG_SELECTOR);
      }
   }
   /// add a connection between entry and exit thus avoiding problems with non terminating code
   bb_graphs_collection.AddEdge(inverse_vertex_map[bloc::ENTRY_BLOCK_ID], inverse_vertex_map[bloc::EXIT_BLOCK_ID],
                                CFG_SELECTOR);
   /// sort basic block vertices from the entry till the exit
   std::list<BBGraph::vertex_descriptor> bb_sorted_vertices;
   struct LocalDFSVisitor : public boost::dfs_visitor<>
   {
      explicit LocalDFSVisitor(std::list<BBGraph::vertex_descriptor>& Out) : Lref(Out)
      {
      }
      void finish_vertex(const BBGraph::vertex_descriptor& u, const BBGraph&)
      {
         Lref.push_front(u);
      }
      std::list<BBGraph::vertex_descriptor>& Lref;
   };
   {
      LocalDFSVisitor vis(bb_sorted_vertices);
      std::vector<boost::default_color_type> color_storage(boost::num_vertices(bb_graph));
      const auto idmap = boost::get(boost::vertex_index_t(), bb_graph);
      auto color_map = boost::make_iterator_property_map(color_storage.begin(), idmap, color_storage[0]);
      boost::depth_first_search(bb_graph, boost::visitor(vis).color_map(color_map).vertex_index_map(idmap));
   }
   for(auto bb : bb_sorted_vertices)
   {
      const auto& bb_node_info = bb_graph.CGetNodeInfo(bb);
      const auto& curr_bbi = bb_node_info.block->number;
      const auto& curr_bb = bb_node_info.block;
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
      if(bb_node_info.block->list_of_pred.size() > 1)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped because it has multiple predecessors");
         continue;
      }
      const auto pred = bb_node_info.block->list_of_pred.front();
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
      if(last_pred_stmt->get_kind() != multi_way_if_stmt_K)
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
      if(last_curr_stmt->get_kind() != multi_way_if_stmt_K)
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
      AppM->RegisterTransformation(GetName(), ir_nodeConstRef());
      /// check for short circuit conditions: i.e., if they have at least a successor in common
      /// if so we add a basic block on the shortest path (e.g., predecessor --> common successor)
      /// In this way in the produced multi_way_if_stmt there cannot be multiple conditions with the same next bb
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

      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---Merging multi_way_if_stmt with multi_way_if_stmt(BB" + STR(curr_bbi) + " with BB" + STR(pred) +
                         ")");
      MergeMultiMulti(pred_bb, curr_bb);

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

void multi_way_if::MergeMultiMulti(const blocRef& pred_bb, const blocRef& curr_bb)
{
   /// create the multi_way_if_stmt node
   ir_manager::IRSchema IR_schema;
   IR_schema[TOK(TOK_IR_LOCINFO)] = BUILTIN_LOCINFO;
   IR_schema[TOK(TOK_PARENT)] = STR(function_id);
   auto gwi_node = TM->create_ir_node(multi_way_if_stmt_K, IR_schema);
   auto new_gwi = GetPointerS<multi_way_if_stmt>(gwi_node);
   new_gwi->bb_index = pred_bb->number;

   const auto old_gwi1 = GetPointerS<const multi_way_if_stmt>(pred_bb->CGetStmtList().back());
   const auto old_gwi2 = GetPointerS<const multi_way_if_stmt>(curr_bb->CGetStmtList().back());
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---First multi way if is " + old_gwi1->ToString());
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Second multi way if is " + old_gwi2->ToString());

   /// Remove old multi way if
   pred_bb->RemoveStmt(pred_bb->CGetStmtList().back(), AppM);

   /// Remove old multi way if
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
                        "---It is not default and nested multi_way_if_stmt is on this edge");
         for(const auto& old_cond2 : old_gwi2->list_of_cond)
         {
            if(old_cond2.first)
            {
               const auto new_cond = ir_man->CreateAndExpr(old_cond1.first, old_cond2.first, pred_bb, function_id);
               new_gwi->add_cond(new_cond, old_cond2.second);
            }
            else
            {
               ir_nodeRef new_cond = ir_nodeRef();
               for(const auto& other_old_cond2 : old_gwi2->list_of_cond)
               {
                  if(other_old_cond2.first)
                  {
                     if(new_cond)
                     {
                        new_cond = ir_man->CreateAndExpr(
                            new_cond, ir_man->CreateNotExpr(other_old_cond2.first, pred_bb, function_id), pred_bb,
                            function_id);
                     }
                     else
                     {
                        new_cond = ir_man->CreateNotExpr(other_old_cond2.first, pred_bb, function_id);
                     }
                  }
               }
               new_cond = ir_man->CreateAndExpr(new_cond, old_cond1.first, pred_bb, function_id);
               new_gwi->add_cond(new_cond, old_cond2.second);
            }
         }
      }
      /// Non default and succ is not on this edge
      else if(old_cond1.first)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---It is not default and nested multi_way_if_stmt is not on this edge");
         new_gwi->add_cond(old_cond1.first, old_cond1.second);
      }
      /// Default and succ is on this edge
      else if(old_cond1.second == curr_bb->number)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---It is default and nested multi_way_if_stmt is on this edge");
         /// Building the and of the not of other conditions of cond1
         ir_nodeRef not_cond = ir_nodeRef();
         for(const auto& other_old_cond1 : old_gwi1->list_of_cond)
         {
            if(other_old_cond1.first)
            {
               if(not_cond)
               {
                  not_cond = ir_man->CreateAndExpr(not_cond,
                                                   ir_man->CreateNotExpr(other_old_cond1.first, pred_bb, function_id),
                                                   pred_bb, function_id);
               }
               else
               {
                  not_cond = ir_man->CreateNotExpr(other_old_cond1.first, pred_bb, function_id);
               }
            }
         }
         for(const auto& old_cond2 : old_gwi2->list_of_cond)
         {
            if(old_cond2.first)
            {
               const auto new_cond = ir_man->CreateAndExpr(not_cond, old_cond2.first, pred_bb, function_id);
               new_gwi->add_cond(new_cond, old_cond2.second);
            }
            else
            {
               new_gwi->add_cond(ir_nodeRef(), old_cond2.second);
            }
         }
      }
      /// Default and second is not on this edge
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---It is default and nested multi_way_if_stmt is not on this edge");
         new_gwi->add_cond(ir_nodeRef(), old_cond1.second);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "<--Considered condition " + (old_cond1.first ? old_cond1.first->ToString() : " default"));
   }
   pred_bb->PushBack(gwi_node, AppM);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + new_gwi->ToString());
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

   /// Fix the last statement of the predecessor
   const auto& pred_list_of_stmt = pred_bb->CGetStmtList();
   THROW_ASSERT(pred_list_of_stmt.size(), "Unexpexted condition");
   if(pred_list_of_stmt.back()->get_kind() == multi_way_if_stmt_K)
   {
      auto gmwi = GetPointerS<multi_way_if_stmt>(pred_list_of_stmt.back());
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
      auto gp = GetPointer<phi_stmt>(phi);
      for(auto& def_edge : gp->CGetDefEdgesList())
      {
         if(def_edge.second == pred_bb->number)
         {
            gp->ReplaceDefEdge(TM, def_edge, phi_stmt::DefEdge(def_edge.first, new_basic_block_index));
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

   if(parameters->getOption<HLSFlowStep_Type>(OPT_scheduling_algorithm) == HLSFlowStep_Type::SDC_SCHEDULING)
   {
      if(GetPointer<const HLS_manager>(AppM) and GetPointer<const HLS_manager>(AppM)->get_HLS(function_id) and
         GetPointer<const HLS_manager>(AppM)->get_HLS(function_id)->Rsch)
      {
         /// If schedule is not up to date, do not execute this step and invalidate UpdateSchedule
         const auto update_schedule = design_flow_manager.GetDesignFlowStep(
             FunctionFrontendFlowStep::ComputeSignature(FrontendFlowStepType::UPDATE_SCHEDULE, function_id));
         if(update_schedule != DesignFlowGraph::null_vertex())
         {
            const DesignFlowGraphConstRef design_flow_graph = design_flow_manager.CGetDesignFlowGraph();
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

   return true;
}
