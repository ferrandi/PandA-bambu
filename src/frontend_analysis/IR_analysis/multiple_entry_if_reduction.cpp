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
 * @file multiple_entry_if_reduction.cpp
 * @brief This class implements the reduction of blocks with n input and m output
 * These operations can be done if the BB contains only conditional statements and phi
 * First duplicate the block on all the n input and add the respective connections to predecessor and successors (bidirectional)
 * Each duplicate starts as an empty one with only connections
 * Then the new BB is populated by conditional functions derivated from the original statement list
 *    - if cond is defined outside the BB keep them
 *    - if cond is defined in the BB then modify it leaving only the component of the condition defined in the BB predecessor
 * After that the phi statement defined in the original BB are added to the successors blocks changing this ssa names
 * To keep the tree coherent each time that an old ssa name of a phi is used substitute it with the new one
 * Then remove the old BB connections from predecessor and successors and eventually eliminate it
 *
 * for more details look at the comments in the code
 *
 * @author Andrea Caielli <andrea.caielli@mail.polimi.it>
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Header include
#include "multiple_entry_if_reduction.hpp"

/// algorithms/loops_detection includes
#include "loop.hpp"
#include "loops.hpp"

/// Behavior include
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "behavioral_helper.hpp"
#include "function_behavior.hpp"

/// design_flows includes
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"

/// frontend_analysis/IR_analysis
#include "simple_code_motion.hpp"

/// HLS include
#include "hls.hpp"
#include "hls_manager.hpp"

/// HLS/allocation include
#include "allocation_information.hpp"

/// Parameter include
#include "Parameter.hpp"

/// parser/treegcc include
#include "token_interface.hpp"

/// STD include
#include <boost/range/adaptor/reversed.hpp>
#include <cmath>
#include <fstream>
#include <limits>
#include <string>

/// STL includes
#include "custom_map.hpp"
#include "custom_set.hpp"
#include <list>
#include <set>
#include <utility>

/// Tree include
#include "ext_tree_node.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "tree_node.hpp"
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#include "dbgPrintHelper.hpp"      // for DEBUG_LEVEL_
#include "string_manipulation.hpp" // for GET_CLASS
#include "tree_node_dup.hpp"
#include "tree_reindex.hpp"

REF_FORWARD_DECL(tree_node_dup);

#define MAX_DOUBLE std::numeric_limits<double>::max();

MultipleEntryIfReduction::MultipleEntryIfReduction(const ParameterConstRef _parameters, const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, MULTIPLE_ENTRY_IF_REDUCTION, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

void MultipleEntryIfReduction::Initialize()
{
   bb_modified = false;
   TM = AppM->get_tree_manager();
   tree_man = tree_manipulationRef(new tree_manipulation(TM, parameters));
   const auto temp = TM->get_tree_node_const(function_id);
   auto fd = GetPointer<function_decl>(temp);
   sl = GetPointer<statement_list>(GET_NODE(fd->body));
   if(GetPointer<const HLS_manager>(AppM) and GetPointer<const HLS_manager>(AppM)->get_HLS(function_id) and GetPointer<const HLS_manager>(AppM)->get_HLS(function_id)->allocation_information)
   {
      allocation_information = GetPointer<const HLS_manager>(AppM)->get_HLS(function_id)->allocation_information;
   }
}

MultipleEntryIfReduction::~MultipleEntryIfReduction() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> MultipleEntryIfReduction::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(PRECEDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(BUILD_VIRTUAL_PHI, SAME_FUNCTION));
         break;
      }
      case DEPENDENCE_RELATIONSHIP:
      {
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(BB_FEEDBACK_EDGES_IDENTIFICATION, SAME_FUNCTION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         switch(GetStatus())
         {
            case DesignFlowStep_Status::SUCCESS:
            {
               if(not parameters->getOption<int>(OPT_gcc_openmp_simd))
               {
                  relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(BIT_VALUE, SAME_FUNCTION));
               }
               relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(PHI_OPT, SAME_FUNCTION));
               relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(SIMPLE_CODE_MOTION, SAME_FUNCTION));
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
               break;
            default:
               THROW_UNREACHABLE("");
         }
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   return relationships;
}

DesignFlowStep_Status MultipleEntryIfReduction::InternalExec()
{
   THROW_ASSERT(parameters->IsParameter("meif_threshold"), "");
   const auto threshold = parameters->GetParameter<double>("meif_threshold");

   const size_t max_iteration_number = parameters->IsParameter("meif_max_iterations_number") ? parameters->GetParameter<size_t>("meif_max_iterations_number") : std::numeric_limits<size_t>::max();

   static size_t executed_iteration = 0;
   if(executed_iteration >= max_iteration_number)
   {
      return DesignFlowStep_Status::UNCHANGED;
   }

   /// if the operation had been done
   bool modified = false;
   /// list to save the block to eliminate after the reduction
   std::list<unsigned int> block_to_erase;

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Multiple Entry If Reduction");

   /// The basic block graphs
   const BBGraphRef bb_cfg = function_behavior->GetBBGraph(FunctionBehavior::BB);
   const BBGraphConstRef bb_fcfg = function_behavior->CGetBBGraph(FunctionBehavior::FBB);
   const auto loops = function_behavior->CGetLoops();
   const auto bb_index_map = bb_cfg->CGetBBGraphInfo()->bb_index_map;

   VertexIterator basic_block, basic_block_end;
   for(boost::tie(basic_block, basic_block_end) = boost::vertices(*bb_fcfg); basic_block != basic_block_end; basic_block++)
   {
      const auto block = bb_cfg->CGetBBNodeInfo(*basic_block)->block;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining BB" + STR(block->number));
#ifndef NDEBUG
      if(not AppM->ApplyNewTransformation())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped because reached max cfg transformations");
         continue;
      }
#endif
      /// Analyze if a basic block is a possible candidate
      if(block->CGetStmtList().empty())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped because empty");
         continue;
      }
      const auto last_stmt = GET_NODE(block->CGetStmtList().back());
      if((not parameters->IsParameter("MEIR_y") or !parameters->GetParameter<bool>("MEIR_y")) and last_stmt->get_kind() != gimple_cond_K and last_stmt->get_kind() != gimple_multi_way_if_K)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped because does not end with a condition");
         continue;
      }
      if(block->list_of_pred.size() < 2)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped because it has a single predecessor");
         continue;
      }
      const bool vdef_op = [&]() -> bool {
         for(const auto& stmt : block->CGetStmtList())
         {
            const auto ga = GetPointer<const gimple_assign>(GET_NODE(stmt));
            if(ga and ga->temporary_address)
               return true;
            /// We skip basic block containing gimple_call since it would require modification of the call graph
            if(GET_NODE(stmt)->get_kind() == gimple_call_K)
               return true;
         }
         return false;
      }();

      if(vdef_op)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped because there is at least a vdef or a computation of an address");
         continue;
      }

      const auto area_weight = GetAreaCost(block->CGetStmtList());
      const auto multiplier = (parameters->IsParameter("MEIR_single") and parameters->GetParameter<bool>("MEIR_single")) ? 1 : static_cast<double>(boost::in_degree(*basic_block, *bb_fcfg) - 1);
      if(area_weight * multiplier > threshold)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped because too large " + STR(area_weight) + " * " + STR((static_cast<double>(boost::in_degree(*basic_block, *bb_fcfg)) - 1)) + " to be duplicated");
         continue;
      }

      /// Skip headers
      if(boost::in_degree(*basic_block, *bb_cfg) != boost::in_degree(*basic_block, *bb_fcfg))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped because header loop");
         continue;
      }
      const bool source_feedback = boost::out_degree(*basic_block, *bb_cfg) != boost::out_degree(*basic_block, *bb_fcfg);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Duplicating BB");
      modified = true;

      /// The index of the loop to be considered (the most internal loops which contains the definition and all the uses
      auto loop_id = bb_cfg->CGetBBNodeInfo(*basic_block)->loop_id;

      /// The depth of the loop to be considered
      auto depth = loops->CGetLoop(loop_id)->depth;

      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Modifying basic block graph");
      /// Create the copies
      CustomMap<unsigned int, unsigned int> copy_ids;
      InEdgeIterator to_be_copied_ie, to_be_copied_ie_end;
      for(boost::tie(to_be_copied_ie, to_be_copied_ie_end) = boost::in_edges(*basic_block, *bb_cfg); to_be_copied_ie != to_be_copied_ie_end; to_be_copied_ie++)
      {
         auto source = boost::source(*to_be_copied_ie, *bb_cfg);
         auto source_block = bb_cfg->CGetBBNodeInfo(source)->block;
         auto source_id = source_block->number;

         auto new_bb_index = sl->list_of_bloc.rbegin()->first + 1;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Creating BB" + STR(new_bb_index));
         copy_ids[source_id] = new_bb_index;

         auto new_block = blocRef(new bloc(new_bb_index));
         new_block->loop_id = block->loop_id;
         if(GetPointer<HLS_manager>(AppM) and GetPointer<HLS_manager>(AppM)->get_HLS(function_id))
            new_block->schedule = GetPointer<HLS_manager>(AppM)->get_HLS(function_id)->Rsch;
         new_block->SetSSAUsesComputed();
         sl->list_of_bloc[new_bb_index] = new_block;

         std::replace(source_block->list_of_succ.begin(), source_block->list_of_succ.end(), block->number, new_bb_index);
         if(source_block->true_edge == block->number)
         {
            source_block->true_edge = new_bb_index;
         }
         else if(source_block->false_edge == block->number)
         {
            source_block->false_edge = new_bb_index;
         }
         new_block->list_of_pred.push_back(source_id);
         for(const auto succ : block->list_of_succ)
         {
            auto target_block = sl->list_of_bloc[succ];
            auto target_id = target_block->number;
            if(target_block->number != BB_EXIT)
               target_block->list_of_pred.push_back(new_bb_index);
            new_block->list_of_succ.push_back(target_id);
            if(block->true_edge == target_id)
            {
               new_block->true_edge = target_id;
            }
            else if(block->false_edge == target_id)
            {
               new_block->false_edge = target_id;
            }
         }
         if(source_block->CGetStmtList().size() and GetPointer<gimple_multi_way_if>(GET_NODE(source_block->CGetStmtList().back())))
         {
            auto gmwi = GetPointer<gimple_multi_way_if>(GET_NODE(source_block->CGetStmtList().back()));
            for(auto& cond : gmwi->list_of_cond)
            {
               if(cond.second == block->number)
                  cond.second = new_bb_index;
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Created BB" + STR(new_bb_index));
      }
      OutEdgeIterator oe, oe_end;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Fixing other phis");
      for(boost::tie(oe, oe_end) = boost::out_edges(*basic_block, *bb_fcfg); oe != oe_end; oe++)
      {
         const auto target = boost::target(*oe, *bb_cfg);
         auto target_block = bb_cfg->CGetBBNodeInfo(target)->block;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Fixing phis of BB" + STR(target_block->number));
         /// Fixing phi: duplicating def_edge of ssa name which are not defined in the current bb
         for(const auto& phi : target_block->CGetPhiList())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Initial phi " + STR(phi));
            auto gp = GetPointer<gimple_phi>(GET_NODE(phi));
            gimple_phi::DefEdgeList def_edge_list;
            for(const auto& def_edge : gp->CGetDefEdgesList())
            {
               if(def_edge.second == block->number)
               {
                  const auto sn = GetPointer<const ssa_name>(GET_NODE(def_edge.first));
                  if(not sn or GetPointer<const gimple_node>(GET_NODE(sn->CGetDefStmt()))->bb_index != block->number)
                  {
                     for(const auto& copy_id : copy_ids)
                     {
                        def_edge_list.push_back(gimple_phi::DefEdge(def_edge.first, copy_id.second));
                     }
                  }
                  else
                  {
                     def_edge_list.push_back(def_edge);
                  }
               }
               else
               {
                  def_edge_list.push_back(def_edge);
               }
            }
            gp->SetDefEdgeList(TM, def_edge_list);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Fixed phi " + STR(phi));
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Fixed phis of BB" + STR(target_block->number));
         if(std::find(target_block->list_of_pred.begin(), target_block->list_of_pred.end(), block->number) != target_block->list_of_pred.end())
         {
            target_block->list_of_pred.erase(std::find(target_block->list_of_pred.begin(), target_block->list_of_pred.end(), block->number));
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Fixed phis");
      CustomMap<unsigned int, CustomUnorderedMapStable<unsigned int, unsigned int>> remaps;
      CustomMap<unsigned int, tree_node_dupRef> tree_node_dups;
      for(const auto& copy : copy_ids)
      {
         tree_node_dups[copy.second] = tree_node_dupRef(new tree_node_dup(remaps[copy.second], TM));
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Modified basic block graph");

      /// Build list of phis + gimple_nodes
      std::list<tree_nodeRef> gimples;
      for(const auto& phi : block->CGetPhiList())
      {
         gimples.push_back(phi);
      }
      for(const auto& stmt : block->CGetStmtList())
      {
         gimples.push_back(stmt);
      }
      for(const auto& gimple : gimples)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering gimple " + STR(gimple));
         /// The ssa defined by the statement (left part of gimple assignment and vdef)
         TreeNodeSet defined_sns;

         /// The set of basic block uses
         CustomSet<vertex> use_bbs;

         /// The set of statement uses
         TreeNodeSet use_stmts;

         /// The set of reaching definition
         CustomMap<unsigned int, tree_nodeRef> reaching_defs;

         /// Cache of created phi - first key is the used ssa - second key is the basic block where is created
         CustomMap<vertex, tree_nodeRef> added_phis;

         const auto vdef = GetPointer<const gimple_node>(GET_NODE(gimple))->vdef;
         if(vdef)
         {
            defined_sns.insert(vdef);
         }

         if(GET_NODE(gimple)->get_kind() == gimple_phi_K)
         {
            const auto old_gp = GetPointer<const gimple_phi>(GET_NODE(gimple));
            defined_sns.insert(old_gp->res);
            for(const auto& def_edge : old_gp->CGetDefEdgesList())
            {
               THROW_ASSERT(copy_ids.find(def_edge.second) != copy_ids.end(), "Copy BB connected to BB" + STR(def_edge.second) + " not found");
               reaching_defs[copy_ids[def_edge.second]] = def_edge.first;
               remaps[copy_ids[def_edge.second]][old_gp->res->index] = def_edge.first->index;
            }
         }
         else if(GET_NODE(gimple)->get_kind() == gimple_assign_K)
         {
            const auto old_ga = GetPointer<const gimple_assign>(GET_NODE(gimple));
            if(GET_NODE(old_ga->op0)->get_kind() == ssa_name_K)
            {
               defined_sns.insert(old_ga->op0);
            }
         }
         else if(GET_NODE(gimple)->get_kind() == gimple_multi_way_if_K or GET_NODE(gimple)->get_kind() == gimple_cond_K or GET_NODE(gimple)->get_kind() == gimple_return_K)
         {
         }
         else
         {
            THROW_UNREACHABLE(STR(gimple));
         }
         /// Duplicating statement
         if(GET_NODE(gimple)->get_kind() == gimple_phi_K)
         {
         }
         else if(GET_NODE(gimple)->get_kind() == gimple_assign_K)
         {
            const auto ga = GetPointer<const gimple_assign>(GET_NODE(gimple));
            const auto ssa_uses = tree_helper::ComputeSsaUses(ga->op1);
            for(const auto& ssa_use : ssa_uses)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering use " + STR(ssa_use.first));
               if(GetPointer<gimple_node>(GET_NODE(GetPointer<ssa_name>(GET_NODE(ssa_use.first))->CGetDefStmt()))->bb_index == block->number)
               {
#if HAVE_ASSERTS
                  for(auto copy : copy_ids)
                  {
                     THROW_ASSERT(remaps.find(copy.second)->second.find(ssa_use.first->index) != remaps.find(copy.second)->second.end(), STR(ssa_use.first));
                  }
#endif
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered use " + STR(ssa_use.first));
            }
            for(auto copy : copy_ids)
            {
               if(GET_NODE(ga->op0)->get_kind() == ssa_name_K)
               {
                  auto ssa0 = GetPointer<ssa_name>(GET_NODE(ga->op0));
                  const auto new_ssa = tree_man->create_ssa_name(tree_nodeRef(), GetPointer<ssa_name>(GET_NODE(ga->op0))->type, ssa0->min, ssa0->max);
                  remaps[copy.second][ga->op0->index] = new_ssa->index;
                  reaching_defs[copy.second] = new_ssa;
               }
               if(ga->vdef)
               {
                  const auto new_ssa = tree_man->create_ssa_name(tree_nodeRef(), GetPointer<ssa_name>(GET_NODE(ga->vdef))->type, tree_nodeRef(), tree_nodeRef());
                  GetPointer<ssa_name>(GET_NODE(new_ssa))->virtual_flag = true;
                  remaps[copy.second][ga->vdef->index] = new_ssa->index;
                  reaching_defs[copy.second] = new_ssa;
               }
               const auto new_stmt = tree_node_dups[copy.second]->create_tree_node(GET_NODE(gimple));
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---New statement (" + STR(new_stmt) + ") is " + STR(TM->CGetTreeNode(new_stmt)));
               sl->list_of_bloc[copy.second]->PushBack(TM->GetTreeReindex(new_stmt));
            }
         }
         else if(GET_NODE(gimple)->get_kind() == gimple_multi_way_if_K or GET_NODE(gimple)->get_kind() == gimple_cond_K or GET_NODE(gimple)->get_kind() == gimple_return_K)
         {
            const auto ssa_uses = tree_helper::ComputeSsaUses(gimple);
            for(const auto& ssa_use : ssa_uses)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering use " + STR(ssa_use.first));
               for(auto copy : copy_ids)
               {
                  if((GetPointer<const gimple_node>(GET_NODE(GetPointer<const ssa_name>(GET_CONST_NODE(ssa_use.first))->CGetDefStmt()))->bb_index != block->number))
                  {
                     /// Defined in a different basic block
                     remaps[copy.second][ssa_use.first->index] = ssa_use.first->index;
                  }
                  else
                  {
                     THROW_ASSERT(remaps.find(copy.second)->second.find(ssa_use.first->index) != remaps.find(copy.second)->second.end(), STR(ssa_use.first));
                  }
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered use " + STR(ssa_use.first));
            }
            for(auto copy : copy_ids)
            {
               const auto new_stmt = tree_node_dups[copy.second]->create_tree_node(GET_NODE(gimple));
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---New statement is " + STR(TM->CGetTreeNode(new_stmt)));
               sl->list_of_bloc[copy.second]->PushBack(TM->GetTreeReindex(new_stmt));
            }
         }
         else
         {
            THROW_UNREACHABLE(STR(gimple));
         }
         for(const auto& defined_sn : defined_sns)
         {
            auto sn = GetPointer<ssa_name>(GET_NODE(defined_sn));
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Updating uses of " + sn->ToString());
            /// The old ssa
            TreeNodeSet uses_to_be_removed;
            for(const auto& use_stmt : sn->CGetUseStmts())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering use in " + STR(use_stmt.first->index) + " " + STR(use_stmt.first));
               const auto use_bb_index = GetPointer<const gimple_node>(GET_NODE(use_stmt.first))->bb_index;
               if(bb_index_map.find(use_bb_index) == bb_index_map.end())
               {
                  THROW_ASSERT(sn->virtual_flag, "");
                  auto gn = GetPointer<gimple_node>(GET_NODE(use_stmt.first));
                  gn->vuses.erase(defined_sn);
                  uses_to_be_removed.insert(use_stmt.first);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Removing use from just created statement");
                  continue;
               }
               const auto use_bb = bb_index_map.find(use_bb_index)->second;

               use_stmts.insert(use_stmt.first);
               use_bbs.insert(use_bb);

               auto use_loop_id = bb_cfg->CGetBBNodeInfo(use_bb)->loop_id;
               auto use_depth = loops->CGetLoop(use_loop_id)->depth;

               /// Management of vuses for overwrite
               if(sn->virtual_flag and GET_NODE(use_stmt.first)->get_kind() != gimple_phi_K and not function_behavior->CheckBBReachability(*basic_block, use_bb))
               {
                  auto gn = GetPointer<gimple_node>(GET_NODE(use_stmt.first));
                  gn->vuses.erase(defined_sn);
                  uses_to_be_removed.insert(use_stmt.first);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Removing " + STR(defined_sn) + " from vuses of " + gn->ToString());
                  for(const auto& copy : copy_ids)
                  {
                     gn->vuses.insert(TM->GetTreeReindex(remaps[copy.second][sn->index]));
                     GetPointer<ssa_name>(TM->get_tree_node_const(remaps[copy.second][sn->index]))->AddUseStmt(use_stmt.first);
                  }
               }
               /// Use is in the considered loop
               if(use_loop_id == loop_id)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Use is in the same loop");
                  continue;
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Definition and use are in different loops");
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Current loop is " + STR(loop_id) + " (depth is " + STR(depth) + ") - Use loop is " + STR(use_loop_id) + " (depth " + STR(use_depth) + ")");
               /// Use is in a more nested loop
               while(use_depth > depth)
               {
                  use_loop_id = loops->CGetLoop(use_loop_id)->Parent()->GetId();
                  use_depth = loops->CGetLoop(use_loop_id)->depth;
               }
               /// Use is in a less nested loop
               while(use_depth < depth)
               {
                  loop_id = loops->CGetLoop(loop_id)->Parent()->GetId();
                  depth = loops->CGetLoop(loop_id)->depth;
               }
               while(use_loop_id != loop_id)
               {
                  use_loop_id = loops->CGetLoop(use_loop_id)->Parent()->GetId();
                  loop_id = loops->CGetLoop(loop_id)->Parent()->GetId();
                  depth = loops->CGetLoop(loop_id)->depth;
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Loop to be considered updated to " + STR(loop_id));
            }
            for(const auto& use_to_be_removed : uses_to_be_removed)
            {
               sn->RemoveUse(use_to_be_removed);
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Loop to be considered is " + STR(loop_id));

            // The set of header basic blocks where a phi has to be inserted
            CustomSet<vertex> phi_headers;
            auto current_loop = loops->CGetLoop(bb_cfg->CGetBBNodeInfo(*basic_block)->loop_id);
            while(current_loop->GetId() != loop_id)
            {
               phi_headers.insert(current_loop->GetHeader());
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Phi has to be added in header of loop " + STR(current_loop->GetId()));
               current_loop = current_loop->Parent();
            }
            phi_headers.insert(current_loop->GetHeader());
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Phi has to be added in header of loop " + STR(current_loop->GetId()));

            /// Set of basic blocks belonging to the loop
            CustomUnorderedSet<vertex> loop_basic_blocks;
            loops->CGetLoop(loop_id)->get_recursively_bb(loop_basic_blocks);
            /// Check if there a use of the ssa in the header; if not basic blocks after uses can be removed
            if(use_bbs.find(loops->CGetLoop(loop_id)->GetHeader()) == use_bbs.end())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---SSA not used in the header of the loop");
               /// Remove all basic blocks from which no use can be reached
               CustomSet<vertex> to_be_removed;
               for(const auto current : loop_basic_blocks)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Considering BB" + STR(bb_cfg->CGetBBNodeInfo(current)->block->number));
                  bool reachable = false;
                  for(const auto use_bb : use_bbs)
                  {
                     if(function_behavior->CheckBBReachability(current, use_bb))
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---BB" + STR(bb_cfg->CGetBBNodeInfo(use_bb)->block->number) + " can be reached from BB" + STR(bb_cfg->CGetBBNodeInfo(current)->block->number));
                        reachable = true;
                        break;
                     }
                     else if(current == use_bb)
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---BB" + STR(bb_cfg->CGetBBNodeInfo(use_bb)->block->number) + " contains a use");
                        reachable = true;
                        break;
                     }
                  }
                  if(not reachable)
                  {
                     to_be_removed.insert(current);
                  }
               }
               for(const auto removable : to_be_removed)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Removing BB" + STR(bb_cfg->CGetBBNodeInfo(removable)->block->number));
                  loop_basic_blocks.erase(removable);
               }
            }

            /// Set of basic blocks to be analyzed
            std::set<vertex, bb_vertex_order_by_map> to_be_processed(bb_vertex_order_by_map(function_behavior->get_bb_map_levels()));
            OutEdgeIterator oe_basic_block, oe_basic_block_end;
            for(boost::tie(oe_basic_block, oe_basic_block_end) = boost::out_edges(*basic_block, *bb_cfg); oe_basic_block != oe_basic_block_end; oe_basic_block++)
            {
               const auto target = boost::target(*oe_basic_block, *bb_cfg);
               if(loop_basic_blocks.find(target) != loop_basic_blocks.end())
               {
                  to_be_processed.insert(target);
               }
            }

            while(to_be_processed.size())
            {
               const auto current = *(to_be_processed.begin());
               const auto current_id = bb_cfg->CGetBBNodeInfo(current)->block->number;
               const auto current_block = sl->list_of_bloc[current_id];
               to_be_processed.erase(current);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Checking BB" + STR(current_id));

               /// True if different definitions reach this bb from all the bbs

               if(current_block->list_of_pred.size() == 1)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Single entry BB");
                  const auto source_id = current_block->list_of_pred.front();
                  if(reaching_defs.find(current_id) == reaching_defs.end() and reaching_defs.find(source_id) != reaching_defs.end())
                  {
                     reaching_defs[current_id] = reaching_defs[source_id];
                  }
               }
               else
               {
                  bool build_phi = true;
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Multiple entries BB");
                  TreeNodeSet local_reaching_defs;
                  InEdgeIterator ie, ie_end;
                  for(boost::tie(ie, ie_end) = boost::in_edges(current, *bb_fcfg); ie != ie_end; ie++)
                  {
                     const auto source = boost::source(*ie, *bb_fcfg);
                     const auto source_id = bb_fcfg->CGetBBNodeInfo(source)->block->number;
                     if(bb_fcfg->GetSelector(*ie) & FB_CFG_SELECTOR)
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Skipping feedback edge coming from BB" + STR(source_id));
                     }
                     else if(source_id == block->number)
                     {
                        for(const auto& copy_id : copy_ids)
                        {
                           THROW_ASSERT(reaching_defs.find(copy_id.second) != reaching_defs.end(), "Definition coming from BB" + STR(copy_id.second) + " not found");
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + STR(reaching_defs.find(copy_id.second)->second) + " from BB" + STR(copy_id.second));
                           local_reaching_defs.insert(reaching_defs.find(copy_id.second)->second);
                        }
                     }
                     else if(reaching_defs.find(source_id) != reaching_defs.end())
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + STR(reaching_defs.find(source_id)->second) + " from BB" + STR(source_id));
                        local_reaching_defs.insert(reaching_defs[source_id]);
                     }
                     else
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Nothing comes from BB" + STR(source_id));
                        build_phi = false;
                     }
                  }
                  if(build_phi)
                  {
                     if(local_reaching_defs.size() == 1)
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---PHI has not to be built");
                        reaching_defs[current_id] = *(local_reaching_defs.begin());
                     }
                     else
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---PHI has to be built");
                        unsigned int phi_ssa_vers = TM->get_next_vers();
                        unsigned int phi_def_ssa_node_nid = TM->new_tree_node_id();
                        std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;
                        IR_schema[TOK(TOK_TYPE)] = STR(tree_helper::get_type_index(TM, sn->index));
                        IR_schema[TOK(TOK_VERS)] = STR(phi_ssa_vers);
                        if(sn->var)
                           IR_schema[TOK(TOK_VAR)] = STR(sn->var->index);
                        IR_schema[TOK(TOK_VOLATILE)] = STR(false);
                        IR_schema[TOK(TOK_VIRTUAL)] = STR(sn->virtual_flag);
                        TM->create_tree_node(phi_def_ssa_node_nid, ssa_name_K, IR_schema);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created ssa _" + STR(phi_ssa_vers));
                        IR_schema.clear();
                        tree_nodeRef phi_def_ssa_node = TM->GetTreeReindex(phi_def_ssa_node_nid);

                        unsigned int phi_gimple_stmt_id = TM->new_tree_node_id();
                        IR_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
                        IR_schema[TOK(TOK_RES)] = boost::lexical_cast<std::string>(phi_def_ssa_node_nid);
                        IR_schema[TOK(TOK_VIRTUAL)] = STR(sn->virtual_flag);
                        TM->create_tree_node(phi_gimple_stmt_id, gimple_phi_K, IR_schema);
                        auto new_gp = GetPointer<gimple_phi>(TM->get_tree_node_const(phi_gimple_stmt_id));
                        new_gp->SetSSAUsesComputed();
                        reaching_defs[current_id] = TM->GetTreeReindex(phi_def_ssa_node_nid);

                        for(boost::tie(ie, ie_end) = boost::in_edges(current, *bb_fcfg); ie != ie_end; ie++)
                        {
                           const auto source = boost::source(*ie, *bb_fcfg);
                           const auto source_id = bb_fcfg->CGetBBNodeInfo(source)->block->number;
                           if(source_id == block->number)
                           {
                              for(const auto& copy_id : copy_ids)
                              {
                                 THROW_ASSERT(reaching_defs.find(copy_id.second) != reaching_defs.end(), "");
                                 THROW_ASSERT(reaching_defs.find(copy_id.second)->second, "");
                                 new_gp->AddDefEdge(TM, gimple_phi::DefEdge(reaching_defs[copy_id.second], copy_id.second));
                              }
                           }
                           else
                           {
                              if((bb_fcfg->GetSelector(*ie) & FB_CFG_SELECTOR) == 0)
                              {
                                 THROW_ASSERT(reaching_defs.find(source_id) != reaching_defs.end(), "");
                                 THROW_ASSERT(reaching_defs.find(source_id)->second, "");
                                 new_gp->AddDefEdge(TM, gimple_phi::DefEdge(reaching_defs[source_id], source_id));
                              }
                              else
                              {
                                 THROW_ASSERT(bb_fcfg->CGetBBNodeInfo(current)->loop_id == bb_fcfg->CGetBBNodeInfo(current)->block->number, "");
                                 new_gp->AddDefEdge(TM, gimple_phi::DefEdge(reaching_defs[current_id], source_id));
                              }
                           }
                        }
                        TM->GetTreeReindex(phi_gimple_stmt_id);
                        bb_cfg->GetBBNodeInfo(current)->block->AddPhi(TM->GetTreeReindex(phi_gimple_stmt_id));
                        reaching_defs[current_id] = TM->GetTreeReindex(phi_def_ssa_node_nid);
                        added_phis[current] = TM->GetTreeReindex(phi_gimple_stmt_id);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created phi " + new_gp->ToString());
                     }
                  }
                  for(auto local_phi : bb_cfg->CGetBBNodeInfo(current)->block->CGetPhiList())
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Analyzing " + STR(local_phi));
                     auto uses = tree_helper::ComputeSsaUses(local_phi);
                     if(uses.find(TM->GetTreeReindex(sn->index)) != uses.end())
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Uses the ssa");
                        gimple_phi::DefEdgeList new_def_edge_list;
                        auto local_gp = GetPointer<gimple_phi>(GET_NODE(local_phi));
                        for(const auto& def_edge : local_gp->CGetDefEdgesList())
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + STR(def_edge.first) + " comes from BB" + STR(def_edge.second));
                           if(def_edge.first->index == sn->index)
                           {
                              if(def_edge.second == block->number)
                              {
                                 for(const auto& copy_id : copy_ids)
                                 {
                                    const auto new_source_id = copy_id.second;
                                    THROW_ASSERT(reaching_defs.find(new_source_id) != reaching_defs.end(), "Defintion coming from " + STR(new_source_id) + " not found");
                                    new_def_edge_list.push_back(gimple_phi::DefEdge(reaching_defs.find(new_source_id)->second, new_source_id));
                                 }
                              }
                              else
                              {
                                 THROW_ASSERT(reaching_defs.find(def_edge.second) != reaching_defs.end(), "Defintion coming from " + STR(def_edge.second) + " not found");
                                 new_def_edge_list.push_back(gimple_phi::DefEdge(reaching_defs.find(def_edge.second)->second, def_edge.second));
                              }
                           }
                           else
                           {
                              new_def_edge_list.push_back(def_edge);
                           }
                        }
                        local_gp->SetDefEdgeList(TM, new_def_edge_list);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Transformed in " + local_gp->ToString());
                     }
                  }
               }
               /// If this is the basic block contains a use
               if(use_bbs.find(current) != use_bbs.end())
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->There is a use in BB" + STR(current_id));
                  for(const auto& stmt : bb_cfg->CGetBBNodeInfo(current)->block->CGetStmtList())
                  {
                     if(use_stmts.find(stmt) != use_stmts.end())
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Replacing " + STR(sn) + " with " + STR(reaching_defs[current_id]) + " in " + STR(stmt));
                        TM->ReplaceTreeNode(stmt, TM->GetTreeReindex(sn->index), reaching_defs[current_id]);
                     }
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
               }

               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Checking successors");
               for(boost::tie(oe, oe_end) = boost::out_edges(current, *bb_fcfg); oe != oe_end; oe++)
               {
                  const auto target = boost::target(*oe, *bb_fcfg);
                  if(loop_basic_blocks.find(target) != loop_basic_blocks.end())
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Considering BB" + STR(bb_fcfg->CGetBBNodeInfo(target)->block->number));
                     if((bb_fcfg->GetSelector(*oe) & FB_CFG_SELECTOR) != 0)
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Connected by feedback edge");
                        for(const auto& target_phi : bb_fcfg->CGetBBNodeInfo(target)->block->CGetPhiList())
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Analyzing " + STR(target_phi));
                           auto gp = GetPointer<gimple_phi>(GET_NODE(target_phi));
                           for(const auto& def_edge : gp->CGetDefEdgesList())
                           {
                              if(def_edge.first->index == sn->index and def_edge.second == current_id)
                              {
                                 THROW_ASSERT(reaching_defs.find(current_id) != reaching_defs.end(), "");
                                 gp->ReplaceDefEdge(TM, def_edge, gimple_phi::DefEdge(reaching_defs.find(current_id)->second, def_edge.second));
                              }
                           }
                        }
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
                     }
                     else
                     {
                        to_be_processed.insert(target);
                     }
                  }
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Checked successors");
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Checked BB" + STR(bb_cfg->CGetBBNodeInfo(current)->block->number));
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Updated uses of " + sn->ToString());
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered " + STR(gimple));
      }
      /// If the basic block is the source of a feedback edge, fix the phi in the header
      if(source_feedback)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Duplicating BB is source of feeback edges - Fixing header phis");
         for(boost::tie(oe, oe_end) = boost::out_edges(*basic_block, *bb_fcfg); oe != oe_end; oe++)
         {
            if(bb_fcfg->GetSelector(*oe) & FB_CFG_SELECTOR)
            {
               const vertex header = boost::target(*oe, *bb_fcfg);
               auto header_bb_node_info = bb_fcfg->CGetBBNodeInfo(header)->block;
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "BB" + STR(header_bb_node_info->number) + " is the header");
               for(auto phi : header_bb_node_info->CGetPhiList())
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Fixing " + STR(phi));
                  auto gp = GetPointer<gimple_phi>(GET_NODE(phi));
                  const auto old_def_edge_list = gp->CGetDefEdgesList();
                  gimple_phi::DefEdgeList new_def_edge_list;
                  for(const auto& old_def_edge : old_def_edge_list)
                  {
                     if(old_def_edge.second != block->number)
                     {
                        new_def_edge_list.push_back(old_def_edge);
                     }
                     else
                     {
                        for(auto copy : copy_ids)
                        {
                           THROW_ASSERT(remaps.find(copy.second) != remaps.end(), STR(copy.second));
                           THROW_ASSERT(remaps.find(copy.second)->second.find(old_def_edge.first->index) != remaps.find(copy.second)->second.end(), STR(old_def_edge.first->index) + " " + STR(TM->CGetTreeNode(old_def_edge.first->index)));
                           new_def_edge_list.push_back(gimple_phi::DefEdge(TM->GetTreeReindex(remaps[copy.second][old_def_edge.first->index]), copy.second));
                        }
                     }
                  }
                  gp->SetDefEdgeList(TM, new_def_edge_list);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Fixed " + STR(phi));
               }
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Duplicated BB is source of feeback edges - Fixing header phis");
      }
      while(block->CGetStmtList().size())
      {
         block->RemoveStmt(block->CGetStmtList().front());
      }
      while(block->CGetPhiList().size())
      {
         block->RemovePhi(block->CGetPhiList().front());
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Ended duplicated " + STR(block->number));
      sl->list_of_bloc.erase(block->number);
      executed_iteration++;
#ifndef NDEBUG
      AppM->RegisterTransformation(GetName(), tree_nodeConstRef());
#endif
      break;
   } /// endfor-list_of_bloc
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   if(modified)
   {
      function_behavior->UpdateBBVersion();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->starting erasing blocks " + STR(block_to_erase.size()));
      /// eliminate the  blocks (remove them from the tree)

      while(!block_to_erase.empty())
      {
         sl->list_of_bloc.erase(block_to_erase.back());
         block_to_erase.pop_back();
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--ending erasing blocks");

      return DesignFlowStep_Status::SUCCESS;
   }
   return DesignFlowStep_Status::UNCHANGED;
}

double MultipleEntryIfReduction::GetAreaCost(const std::list<tree_nodeRef>& list_of_stmt) const
{
   double ret_value = 0.0;
   for(const auto& temp_stmt : list_of_stmt)
   {
      const auto area = allocation_information->GetStatementArea(temp_stmt->index);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Area of " + STR(temp_stmt) + " is " + STR(area));
      ret_value += area;
   }
   return ret_value;
}

bool MultipleEntryIfReduction::HasToBeExecuted() const
{
   if(not parameters->IsParameter("meif_threshold"))
   {
      return false;
   }
   const vertex frontend_step = design_flow_manager.lock()->GetDesignFlowStep(FunctionFrontendFlowStep::ComputeSignature(FrontendFlowStepType::SIMPLE_CODE_MOTION, function_id));
   if(not frontend_step)
      return false;
   const DesignFlowGraphConstRef design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
   const DesignFlowStepRef design_flow_step = design_flow_graph->CGetDesignFlowStepInfo(frontend_step)->design_flow_step;
   return GetPointer<const simple_code_motion>(design_flow_step)->IsScheduleBased();
}
