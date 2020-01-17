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
 *              Copyright (C) 2016-2020 Politecnico di Milano
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
 * @file serialize_mutual_exclusions.cpp
 * @brief This class contains the methods for remove mutual exclusions from simd loops
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Header include
#include "serialize_mutual_exclusions.hpp"

///. include
#include "Parameter.hpp"

/// behavior includes
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "function_behavior.hpp"

/// design_flows include
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"

/// parser/treegcc include
#include "token_interface.hpp"

/// tree includes
#include "ext_tree_node.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

/// utility include
#include "dbgPrintHelper.hpp"      // for DEBUG_LEVEL_
#include "string_manipulation.hpp" // for GET_CLASS

SerializeMutualExclusions::SerializeMutualExclusions(const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters)
    : FunctionFrontendFlowStep(_AppM, _function_id, FrontendFlowStepType::SERIALIZE_MUTUAL_EXCLUSIONS, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

SerializeMutualExclusions::~SerializeMutualExclusions() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> SerializeMutualExclusions::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(BB_CONTROL_DEPENDENCE_COMPUTATION, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(BB_REACHABILITY_COMPUTATION, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(BB_FEEDBACK_EDGES_IDENTIFICATION, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(DOM_POST_DOM_COMPUTATION, SAME_FUNCTION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         if(design_flow_manager.lock()->GetStatus(GetSignature()) == DesignFlowStep_Status::SUCCESS)
         {
            relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(BASIC_BLOCKS_CFG_COMPUTATION, SAME_FUNCTION));
         }
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
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

bool SerializeMutualExclusions::HasToBeExecuted() const
{
   return true;
}

DesignFlowStep_Status SerializeMutualExclusions::InternalExec()
{
   const tree_managerRef TM = AppM->get_tree_manager();
   const tree_manipulationRef tree_man(new tree_manipulation(TM, parameters));
   bool bb_modified = false;
   const auto cdg_bb_graph = function_behavior->CGetBBGraph(FunctionBehavior::CDG_BB);
   const auto cfg_bb_graph = function_behavior->CGetBBGraph(FunctionBehavior::BB);
   const auto post_dom_bb_graph = function_behavior->CGetBBGraph(FunctionBehavior::POST_DOM_TREE);
   const auto bb_index_map = cfg_bb_graph->CGetBBGraphInfo()->bb_index_map;

   std::deque<vertex> basic_blocks;
   cdg_bb_graph->ReverseTopologicalSort(basic_blocks);
   /// Check if the control flow graph is structured
   for(const auto basic_block : basic_blocks)
   {
      if(boost::in_degree(basic_block, *cdg_bb_graph) > 1)
      {
         THROW_ERROR("Basic block structure not supported: BB" + STR(cdg_bb_graph->CGetBBNodeInfo(basic_block)->block->number));
      }
   }

   /// Analyzing basic blocks starting from the leaves
   for(const auto& basic_block : basic_blocks)
   {
      if(bb_modified)
         break;
      if(basic_block == cfg_bb_graph->CGetBBGraphInfo()->entry_vertex or basic_block == cfg_bb_graph->CGetBBGraphInfo()->exit_vertex)
         continue;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing BB" + STR(cfg_bb_graph->CGetBBNodeInfo(basic_block)->block->number));
      const auto bb_node_info = cfg_bb_graph->CGetBBNodeInfo(basic_block)->block;
      if(not bb_node_info->loop_id)
      {
         // For the moment this pass is exploited only by vectorize; if we are outside loops, this is not necessary and does not work after split_return
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skip because in loop 0");
         continue;
      }
      /// NOTE: here cfg_bb_graph is correct
      if(boost::out_degree(basic_block, *cfg_bb_graph) == 2 and GET_CONST_NODE(cfg_bb_graph->CGetBBNodeInfo(basic_block)->block->CGetStmtList().back())->get_kind() == gimple_cond_K)
      {
         OutEdgeIterator oe, oe_end;
         vertex true_vertex = NULL_VERTEX, false_vertex = NULL_VERTEX;
         for(boost::tie(oe, oe_end) = boost::out_edges(basic_block, *cfg_bb_graph); oe != oe_end; oe++)
         {
            if(cfg_bb_graph->CGetBBEdgeInfo(*oe)->cfg_edge_T())
            {
               true_vertex = boost::target(*oe, *cfg_bb_graph);
            }
            else if(cfg_bb_graph->CGetBBEdgeInfo(*oe)->cfg_edge_F())
            {
               false_vertex = boost::target(*oe, *cfg_bb_graph);
            }
            else
            {
               THROW_UNREACHABLE();
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "True-->BB" + STR(cfg_bb_graph->CGetBBNodeInfo(true_vertex)->block->number) + " - False-->BB" + STR(cfg_bb_graph->CGetBBNodeInfo(false_vertex)->block->number));
         if(function_behavior->CheckBBReachability(true_vertex, false_vertex))
         {
         }
         else if(function_behavior->CheckBBReachability(false_vertex, true_vertex))
         {
            std::swap(bb_node_info->true_edge, bb_node_info->false_edge);
            auto last_stmt = bb_node_info->CGetStmtList().back();
            bb_node_info->RemoveStmt(last_stmt);
            auto gc = GetPointer<gimple_cond>(GET_NODE(last_stmt));
            THROW_ASSERT(gc, "");
            auto new_cond = tree_man->CreateNotExpr(gc->op0, bb_node_info);
            gc->op0 = new_cond;
            bb_node_info->PushBack(last_stmt);
            bb_modified = true;
         }
         else
         {
            const auto basic_block_id = bb_node_info->number;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Subgraph dominated by BB" + STR(basic_block_id) + " has to be restructured");
            auto fd = GetPointer<function_decl>(TM->get_tree_node_const(function_id));
            auto sl = GetPointer<statement_list>(GET_NODE(fd->body));
            auto new_block = blocRef(new bloc(sl->list_of_bloc.rbegin()->first + 1));
            new_block->SetSSAUsesComputed();
            sl->list_of_bloc[new_block->number] = new_block;

            InEdgeIterator ie, ie_end;
            THROW_ASSERT(boost::in_degree(basic_block, *post_dom_bb_graph) == 1, "");
            boost::tie(ie, ie_end) = boost::in_edges(basic_block, *post_dom_bb_graph);
            const auto end_if = boost::source(*ie, *post_dom_bb_graph);
            const auto end_if_block = post_dom_bb_graph->CGetBBNodeInfo(end_if)->block;
            const auto end_if_id = end_if_block->number;

            const auto true_bb_id = bb_node_info->true_edge;
            const auto true_bb = bb_index_map.find(true_bb_id)->second;
            const auto false_bb_id = bb_node_info->false_edge;
            const auto false_bb = bb_index_map.find(false_bb_id)->second;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---True BB is " + STR(true_bb_id));
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---False BB is " + STR(false_bb_id));

            CustomSet<vertex> true_bb_ends, false_bb_ends;
            for(boost::tie(ie, ie_end) = boost::in_edges(end_if, *cfg_bb_graph); ie != ie_end; ie++)
            {
               const auto source = boost::source(*ie, *cfg_bb_graph);
               if(!function_behavior->CheckBBReachability(basic_block, source))
                  continue;
               if(source == true_bb or function_behavior->CheckBBReachability(true_bb, source))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Inserting BB" + STR(cfg_bb_graph->CGetBBNodeInfo(source)->block->number) + " into then part");
                  true_bb_ends.insert(source);
                  continue;
               }
               if(source == false_bb or function_behavior->CheckBBReachability(false_bb, source))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Inserting BB" + STR(cfg_bb_graph->CGetBBNodeInfo(source)->block->number) + " into else part");
                  false_bb_ends.insert(source);
                  continue;
               }
               THROW_UNREACHABLE("");
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Classified BBs");
            new_block->list_of_pred.push_back(basic_block_id);
            bb_node_info->list_of_succ.erase(std::find(bb_node_info->list_of_succ.begin(), bb_node_info->list_of_succ.end(), false_bb_id));
            bb_node_info->list_of_succ.push_back(new_block->number);
            bb_node_info->false_edge = new_block->number;
            for(const auto& true_bb_end : true_bb_ends)
            {
               auto true_bb_block = cfg_bb_graph->CGetBBNodeInfo(true_bb_end)->block;
               true_bb_block->list_of_succ.erase(std::find(true_bb_block->list_of_succ.begin(), true_bb_block->list_of_succ.end(), end_if_id));
               true_bb_block->list_of_succ.push_back(new_block->number);
               if(true_bb_block->true_edge == end_if_id)
                  true_bb_block->true_edge = new_block->number;
               else if(true_bb_block->false_edge == end_if_id)
                  true_bb_block->false_edge = new_block->number;
               new_block->list_of_pred.push_back(true_bb_block->number);
               end_if_block->list_of_pred.erase(std::find(end_if_block->list_of_pred.begin(), end_if_block->list_of_pred.end(), true_bb_block->number));
            }

            auto false_bb_block = cfg_bb_graph->CGetBBNodeInfo(false_bb)->block;
            new_block->true_edge = false_bb_id;
            new_block->list_of_succ.push_back(false_bb_id);
            false_bb_block->list_of_pred.erase(std::find(false_bb_block->list_of_pred.begin(), false_bb_block->list_of_pred.end(), basic_block_id));
            false_bb_block->list_of_pred.push_back(new_block->number);

            new_block->false_edge = end_if_id;
            new_block->list_of_succ.push_back(end_if_id);
            end_if_block->list_of_pred.push_back(new_block->number);

            THROW_ASSERT(bb_node_info->CGetStmtList().size(), "");
            const auto gc = GetPointer<const gimple_cond>(GET_NODE(bb_node_info->CGetStmtList().back()));
            THROW_ASSERT(gc, "");
            const auto cond = gc->op0;
            const auto not_cond = tree_man->CreateNotExpr(cond, new_block);

            std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> tree_node_schema;
            tree_node_schema[TOK(TOK_SRCP)] = gc->include_name + ":" + STR(gc->line_number) + ":" + STR(gc->column_number);
            tree_node_schema[TOK(TOK_OP0)] = STR(not_cond->index);
            unsigned int new_tree_node_index = TM->new_tree_node_id();
            TM->create_tree_node(new_tree_node_index, gimple_cond_K, tree_node_schema);
            new_block->PushBack(TM->GetTreeReindex(new_tree_node_index));
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Fixed basic blocks connections");

            /// Fix the phi in end if and create the phi in new block
            for(const auto& phi : end_if_block->CGetPhiList())
            {
               auto gp = GetPointer<gimple_phi>(GET_NODE(phi));
               gimple_phi::DefEdgeList end_if_new_def_edge_list;

               const auto type = tree_helper::CGetType(GET_NODE(gp->res));

               std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> ssa_schema;
               auto ssa_vers = TM->get_next_vers();
               auto ssa_node_nid = TM->new_tree_node_id();
               ssa_schema[TOK(TOK_TYPE)] = STR(type->index);
               ssa_schema[TOK(TOK_VERS)] = STR(ssa_vers);
               ssa_schema[TOK(TOK_VOLATILE)] = STR(false);
               ssa_schema[TOK(TOK_VIRTUAL)] = STR(gp->virtual_flag);
               if(GetPointer<ssa_name>(GET_NODE(gp->res))->var)
                  ssa_schema[TOK(TOK_VAR)] = STR(GetPointer<ssa_name>(GET_NODE(gp->res))->var->index);
               TM->create_tree_node(ssa_node_nid, ssa_name_K, ssa_schema);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(TM->CGetTreeNode(ssa_node_nid)));

               std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> gimple_phi_schema;
               const auto gimple_phi_id = TM->new_tree_node_id();
               gimple_phi_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
               gimple_phi_schema[TOK(TOK_TYPE)] = STR(type);
               gimple_phi_schema[TOK(TOK_RES)] = STR(ssa_node_nid);
               gimple_phi_schema[TOK(TOK_SCPE)] = STR(function_id);
               TM->create_tree_node(gimple_phi_id, gimple_phi_K, gimple_phi_schema);
               auto new_gp = GetPointer<gimple_phi>(TM->get_tree_node_const(gimple_phi_id));
               new_gp->SetSSAUsesComputed();

               const auto zero = [&]() -> tree_nodeRef {
                  if(type->get_kind() == integer_type_K)
                  {
                     return tree_man->CreateIntegerCst(TM->GetTreeReindex(type->index), 0, TM->new_tree_node_id());
                  }
                  THROW_UNREACHABLE("");
                  return tree_nodeRef();
               }();
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Fixing phis");
               new_gp->AddDefEdge(TM, gimple_phi::DefEdge(zero, basic_block_id));
               end_if_new_def_edge_list.push_back(gimple_phi::DefEdge(new_gp->res, new_block->number));
               for(const auto& def_edge : gp->CGetDefEdgesList())
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Considering source BB" + STR(def_edge.second));
                  const auto source_bb = bb_index_map.find(def_edge.second)->second;
                  if(true_bb_ends.find(source_bb) != true_bb_ends.end())
                  {
                     new_gp->AddDefEdge(TM, def_edge);
                  }
                  else if(false_bb_ends.find(source_bb) != false_bb_ends.end())
                  {
                     end_if_new_def_edge_list.push_back(def_edge);
                  }
                  else if(function_behavior->CheckBBReachability(basic_block, source_bb))
                  {
                     THROW_UNREACHABLE("BB" + STR(cfg_bb_graph->CGetBBNodeInfo(source_bb)->block->number) + " not classified");
                  }
                  else
                  {
                     end_if_new_def_edge_list.push_back(def_edge);
                  }
               }
               new_block->AddPhi(TM->GetTreeReindex(gimple_phi_id));
               gp->SetDefEdgeList(TM, end_if_new_def_edge_list);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Added phi " + STR(TM->CGetTreeNode(gimple_phi_id)) + " - Fixed phi " + gp->ToString());
            }
            bb_modified = true;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
         }
      }
      else if(boost::out_degree(basic_block, *cfg_bb_graph) >= 2)
      {
#if HAVE_ASSERTS
         const auto last_stmt = GET_CONST_NODE(cfg_bb_graph->CGetBBNodeInfo(basic_block)->block->CGetStmtList().back());
         const auto gmwi = GetPointer<const gimple_multi_way_if>(last_stmt);
         THROW_ASSERT(gmwi, last_stmt->get_kind_text());
         vertex previous_basic_block = NULL_VERTEX;
         for(const auto& cond : gmwi->list_of_cond)
         {
            const auto current_basic_block_index = cond.second;
            const auto current_basic_block = bb_index_map.at(current_basic_block_index);
            THROW_ASSERT(previous_basic_block == NULL_VERTEX or function_behavior->CheckBBReachability(previous_basic_block, current_basic_block), "");
         }
#endif
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed BB" + STR(cfg_bb_graph->CGetBBNodeInfo(basic_block)->block->number));
   }

   bb_modified ? function_behavior->UpdateBBVersion() : 0;
   return bb_modified ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}
