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
 * @file build_virtual_phi.cpp
 * @brief Analysis step building phi of vops
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#include "build_virtual_phi.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "cdfg_edge_info.hpp"
#include "dbgPrintHelper.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "function_behavior.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_manipulation.hpp"
#include "ir_node.hpp"
#include "loop.hpp"
#include "loops.hpp"
#include "string_manipulation.hpp"
#include "token_interface.hpp"

#include <set>
#include <utility>

BuildVirtualPhi::BuildVirtualPhi(const application_managerRef _AppM, unsigned int _function_id,
                                 const DesignFlowManager& _design_flow_manager, const ParameterConstRef _parameters)
    : FunctionFrontendFlowStep(_AppM, _function_id, BUILD_VIRTUAL_PHI, _design_flow_manager, _parameters),
      TM(_AppM->get_ir_manager())
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
BuildVirtualPhi::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(BB_FEEDBACK_EDGES_IDENTIFICATION, SAME_FUNCTION));
         relationships.insert(std::make_pair(DOM_POST_DOM_COMPUTATION, SAME_FUNCTION));
         relationships.insert(std::make_pair(USE_COUNTING, SAME_FUNCTION));
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(BASIC_BLOCKS_CFG_COMPUTATION, SAME_FUNCTION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         if(GetStatus() == DesignFlowStep_Status::SUCCESS && function_behavior->is_function_pipelined())
         {
            relationships.insert(std::make_pair(SIMPLE_CODE_MOTION, SAME_FUNCTION));
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

DesignFlowStep_Status BuildVirtualPhi::InternalExec()
{
   const auto ir_man = ir_manipulationRef(new ir_manipulation(TM, parameters, AppM));
   auto basic_block_graph = function_behavior->GetBBGraph(FunctionBehavior::FBB);
   const auto loops = function_behavior->getConstLoops();
   const auto bb_index_map = basic_block_graph.CGetGraphInfo().bb_index_map;
   using ReachabilityKey = std::pair<BBGraph::vertex_descriptor, BBGraph::vertex_descriptor>;
   CustomUnorderedMapUnstable<ReachabilityKey, bool> bb_reachability_cache;
   const auto check_bb_reachability_cached = [&](const BBGraph::vertex_descriptor from,
                                                 const BBGraph::vertex_descriptor to) -> bool {
      const ReachabilityKey key(from, to);
      const auto cached = bb_reachability_cache.find(key);
      if(cached != bb_reachability_cache.end())
      {
         return cached->second;
      }
      const bool result = function_behavior->CheckBBReachability(from, to);
      bb_reachability_cache.emplace(key, result);
      return result;
   };
   CustomUnorderedMapUnstable<ReachabilityKey, bool> bb_feedback_reachability_cache;
   const auto check_bb_feedback_reachability_cached = [&](const BBGraph::vertex_descriptor from,
                                                          const BBGraph::vertex_descriptor to) -> bool {
      const ReachabilityKey key(from, to);
      const auto cached = bb_feedback_reachability_cache.find(key);
      if(cached != bb_feedback_reachability_cache.end())
      {
         return cached->second;
      }
      const bool result = function_behavior->CheckBBFeedbackReachability(from, to);
      bb_feedback_reachability_cache.emplace(key, result);
      return result;
   };
   /// Cache of created phi - first key is the used ssa - second key is the basic block where is created
   IRNodeMap<CustomUnorderedMapStable<BBGraph::vertex_descriptor, ir_nodeRef>> added_phis;
   /// Cache of reaching defs - first key is the used ssa - second key is the basic block to be considered
   IRNodeMap<CustomUnorderedMapStable<BBGraph::vertex_descriptor, ir_nodeRef>> reaching_defs;
   /// For each virtual operand its definition
   IRNodeMap<ir_nodeConstRef> virtual_ssa_definitions;
   /// The set of nodes which overwrite a vop
   IRNodeMap<IRNodeSet> vovers;

   std::list<BBGraph::vertex_descriptor> objs(basic_block_graph.vertices().begin(), basic_block_graph.vertices().end());
   /// Computing definitions and overwriting
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Computing definitions: " + STR(objs.size()));
   for(const auto basic_block : basic_block_graph.vertices())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "-->Analyzing BB" + STR(basic_block_graph.CGetNodeInfo(basic_block).block->number));
      const auto& block_info = basic_block_graph.CGetNodeInfo(basic_block).block;
      for(const auto& stmt : block_info->CGetStmtList())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing stmt " + STR(stmt));
         const auto gn = GetPointerS<node_stmt>(stmt);
         if(gn->vdef)
         {
            THROW_ASSERT(virtual_ssa_definitions.count(gn->vdef) == 0,
                         gn->vdef->ToString() + " is defined also in " + STR(virtual_ssa_definitions.at(gn->vdef)));
            virtual_ssa_definitions[gn->vdef] = stmt;
         }
         const auto& cur_bb = bb_index_map.at(gn->bb_index);
         const auto vo_it = gn->vdef ? gn->vovers.find(gn->vdef) : gn->vovers.end();
         if(vo_it != gn->vovers.end() && !check_bb_reachability_cached(cur_bb, cur_bb))
         {
            gn->vovers.erase(vo_it);
            GetPointerS<ssa_node>(gn->vdef)->RemoveUse(stmt);
         }
         for(const auto& vover : gn->vovers)
         {
            vovers[vover].insert(stmt);
         }
         /// clean not reachable vuses
         auto vu_it = gn->vuses.begin();
         while(vu_it != gn->vuses.end())
         {
            const auto sn = GetPointerS<ssa_node>(*vu_it);
            const auto def_stmt = sn->GetDefStmt();
            const auto use_bb_index = GetPointerS<const node_stmt>(def_stmt)->bb_index;
            const auto& use_bb = bb_index_map.at(use_bb_index);
            if(use_bb_index == gn->bb_index)
            {
               /// here we may have a Use-Def or a Def-Use. They are both perfectly fine.
               ++vu_it;
            }
            else if(!check_bb_reachability_cached(use_bb, cur_bb) && !check_bb_reachability_cached(cur_bb, use_bb))
            {
               sn->RemoveUse(stmt);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Removing " + STR(*vu_it) + " from vuses of unreachable stmt: " + STR(sn));
               vu_it = gn->vuses.erase(vu_it);
            }
            else
            {
               ++vu_it;
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed stmt " + STR(stmt));
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "<--Analyzed BB" + STR(basic_block_graph.CGetNodeInfo(basic_block).block->number));
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Computed definitions");

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Checking uses");
   /// Check uses
   for(const auto& virtual_ssa_definition : virtual_ssa_definitions)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "-->Considering ssa " + virtual_ssa_definition.first->ToString());
      const auto sn = GetPointerS<ssa_node>(virtual_ssa_definition.first);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---Defined in " + virtual_ssa_definition.second->ToString());
      const auto definition = GetPointerS<const node_stmt>(virtual_ssa_definition.second);
      THROW_ASSERT(definition, STR(sn->GetDefStmt()));
      const auto definition_bb_index = definition->bb_index;
      const auto& definition_bb = bb_index_map.at(definition_bb_index);
      THROW_ASSERT(sn, STR(virtual_ssa_definition.first));

      /// The index of the loop to be considered (the most internal loops which contains the definition and all the uses
      auto loop_id = basic_block_graph.CGetNodeInfo(definition_bb).loop_id;

      /// The depth of the loop to be considered
      auto depth = loops->getConstLoop(loop_id)->getLoopDepth();

      /// The set of basic block uses
      CustomSet<BBGraph::vertex_descriptor> use_bbs;

      /// The set of statement uses
      IRNodeSet use_stmts;

      /// The set of false uses
      IRNodeMap<size_t> transitive_uses;

      for(const auto& use_stmt : sn->CGetUseStmts())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering use in " + STR(use_stmt.first));
         const auto use_bb_index = GetPointerS<const node_stmt>(use_stmt.first)->bb_index;
         const auto& use_bb = bb_index_map.at(use_bb_index);

         const auto gn = GetPointerS<const node_stmt>(use_stmt.first);

         /// Check if this use can be ignored because of transitive reduction
         bool skip = [&]() -> bool {
            if(definition_bb_index == use_bb_index)
            {
               return false;
            }
            if(vovers.find(virtual_ssa_definition.first) != vovers.end())
            {
               for(const auto& vover_stmt : vovers.find(virtual_ssa_definition.first)->second)
               {
                  const auto vover_bb_index = GetPointerS<const node_stmt>(vover_stmt)->bb_index;
                  const auto vover_bb = bb_index_map.at(vover_bb_index);
                  if(check_bb_reachability_cached(use_bb, vover_bb) || use_bb == vover_bb)
                  {
                     return false;
                  }
               }
            }
            for(const auto& other_use_stmt : sn->CGetUseStmts())
            {
               const auto other_use_bb_index = GetPointerS<const node_stmt>(other_use_stmt.first)->bb_index;
               const auto other_use_bb = bb_index_map.at(other_use_bb_index);
               if(other_use_stmt.first->index != use_stmt.first->index &&
                  check_bb_reachability_cached(other_use_bb, use_bb) &&
                  other_use_stmt.first->index != virtual_ssa_definition.second->index)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Considered other use: " + STR(other_use_stmt.first->index) + " " +
                                     STR(other_use_stmt.first));
                  const auto other_gn = GetPointerS<const node_stmt>(other_use_stmt.first);
                  if(other_gn->vdef && gn->vuses.find(other_gn->vdef) != gn->vuses.end())
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Defines " + STR(other_gn->vdef));
                     return true;
                  }
               }
            }
            return false;
         }();
         if(skip)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped because of transitivity");
            transitive_uses.insert(use_stmt);
            continue;
         }
         use_stmts.insert(use_stmt.first);
         use_bbs.insert(use_bb);

         auto use_loop_id = basic_block_graph.CGetNodeInfo(use_bb).loop_id;
         auto use_depth = loops->getConstLoop(use_loop_id)->getLoopDepth();

         /// Use is in the considered loop
         if(use_loop_id == loop_id)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Use is in the same loop");
            continue;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Definition and use are in different loops");
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Current loop is " + STR(loop_id) + " (depth is " + STR(depth) + ") - Use loop is " +
                            STR(use_loop_id) + " (depth " + STR(use_depth) + ")");
         /// Use is in a more nested loop
         while(use_depth > depth)
         {
            use_loop_id = loops->getConstLoop(use_loop_id)->getParent()->getLoopId();
            use_depth = loops->getConstLoop(use_loop_id)->getLoopDepth();
         }
         /// Use is in a less nested loop
         while(use_depth < depth)
         {
            loop_id = loops->getConstLoop(loop_id)->getParent()->getLoopId();
            depth = loops->getConstLoop(loop_id)->getLoopDepth();
         }
         while(use_loop_id != loop_id)
         {
            use_loop_id = loops->getConstLoop(use_loop_id)->getParent()->getLoopId();
            loop_id = loops->getConstLoop(loop_id)->getParent()->getLoopId();
            depth = loops->getConstLoop(loop_id)->getLoopDepth();
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Loop to be considered updated to " + STR(loop_id));
      }
      for(const auto& transitive_use : transitive_uses)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Removing " + STR(virtual_ssa_definition.first) + " from vuses of " +
                            STR(transitive_use.first));
         const auto gn = GetPointerS<node_stmt>(transitive_use.first);
         gn->vuses.erase(virtual_ssa_definition.first);
         sn->RemoveUse(transitive_use.first);
      }

      /// It is possible that the use is not reachable; this is due to the frontend compiler alias oracle
      if(use_bbs.empty())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--There is not any reachable use");
         continue;
      }

      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Loop to be considered is " + STR(loop_id));

      // The set of header basic blocks where a phi has to be inserted
      CustomSet<BBGraph::vertex_descriptor> phi_headers;
      auto current_loop = loops->getConstLoop(basic_block_graph.CGetNodeInfo(definition_bb).loop_id);
      while(current_loop->getLoopId() != loop_id)
      {
         for(const auto& cur_pair : current_loop->getBackEdges())
         {
            phi_headers.insert(cur_pair.second);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Phi has to be added in header of loop " + STR(current_loop->getLoopId()));
         current_loop = current_loop->getParent();
      }
      for(const auto& cur_pair : current_loop->getBackEdges())
      {
         phi_headers.insert(cur_pair.second);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---Phi has to be added in header of loop " + STR(current_loop->getLoopId()));

      /// Build the nop statement where volatile ssa is defined
      ir_manager::IRSchema nop_IR_schema;
      nop_IR_schema[TOK(TOK_IR_LOCINFO)] = BUILTIN_LOCINFO;
      nop_IR_schema[TOK(TOK_PARENT)] = STR(function_id);
      auto nop_node = TM->create_ir_node(nop_stmt_K, nop_IR_schema);

      const auto volatile_sn =
          ir_man->create_ssa_name(sn->var, ir_helper::CGetType(virtual_ssa_definition.first), nullptr, nullptr, true);
      GetPointerS<ssa_node>(volatile_sn)->SetDefStmt(nop_node);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created volatile ssa " + STR(volatile_sn));

      /// Set of basic blocks belonging to the loop
      std::set<BBGraph::vertex_descriptor> loop_basic_blocks;
      loops->getConstLoop(loop_id)->collectBlocksRecursively(loop_basic_blocks);

      /// Set of basic blocks to be analyzed
      const bb_vertex_order_by_map comp_i(function_behavior->get_bb_map_levels());
      std::set<BBGraph::vertex_descriptor, bb_vertex_order_by_map> to_be_processed(comp_i);

      /// Loop 0 must be managed in a different way
      if(loop_id == 0)
      {
         reaching_defs[virtual_ssa_definition.first][basic_block_graph.CGetGraphInfo().entry_vertex] = volatile_sn;
         for(const auto& oe : basic_block_graph.out_edges(basic_block_graph.CGetGraphInfo().entry_vertex))
         {
            const auto target = basic_block_graph.target(oe);
            if(basic_block_graph.CGetGraphInfo().exit_vertex != target)
            {
               to_be_processed.insert(target);
            }
         }

         /// Remove all basic blocks from which no use can be reached
         CustomSet<BBGraph::vertex_descriptor> to_be_removed;
         for(const auto current : loop_basic_blocks)
         {
            bool reachable = false;
            // TODO: iteration of vertex set may cause non-determinism
            for(const auto use_bb : use_bbs)
            {
               if(check_bb_feedback_reachability_cached(current, use_bb))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---BB" + STR(basic_block_graph.CGetNodeInfo(use_bb).block->number) +
                                     " can be reached from BB" +
                                     STR(basic_block_graph.CGetNodeInfo(current).block->number));
                  reachable = true;
                  break;
               }
               else if(current == use_bb)
               {
                  reachable = true;
                  break;
               }
            }
            if(!reachable)
            {
               to_be_removed.insert(current);
            }
         }
         for(const auto removable : to_be_removed)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Removing BB" + STR(basic_block_graph.CGetNodeInfo(removable).block->number));
            loop_basic_blocks.erase(removable);
         }
      }
      else
      {
         std::set<BBGraph::vertex_descriptor> loop_bbs;
         loops->getConstLoop(loop_id)->collectBlocksRecursively(loop_bbs);
         for(const auto& loop_bb : loop_bbs)
         {
            for(const auto ei : basic_block_graph.in_edges(loop_bb))
            {
               const auto source = basic_block_graph.source(ei);
               if(loop_bbs.find(source) == loop_bbs.end())
               {
                  reaching_defs[virtual_ssa_definition.first][source] = volatile_sn;
               }
            }
         }
         for(const auto& feedback_edge : loops->getConstLoop(loop_id)->getBackEdges())
         {
            to_be_processed.insert(feedback_edge.second);
         }
      }

      while(to_be_processed.size())
      {
         const auto current = *(to_be_processed.begin());
         to_be_processed.erase(current);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "-->Checking BB" + STR(basic_block_graph.CGetNodeInfo(current).block->number));

         if(basic_block_graph.in_degree(current) == 1)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Single entry BB");
            const auto source = basic_block_graph.source(basic_block_graph.in_edges(current).front());
            THROW_ASSERT(reaching_defs.at(virtual_ssa_definition.first).count(source), "unexpected condition");
            reaching_defs[virtual_ssa_definition.first][current] =
                reaching_defs.at(virtual_ssa_definition.first).at(source);
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Multiple entries BB");
            /// The phi is necessary only if there are different reaching definition
            bool build_phi = false;
            IRNodeSet local_reaching_defs;
            for(const auto& ie : basic_block_graph.in_edges(current))
            {
               const auto source = basic_block_graph.source(ie);
               if((basic_block_graph.GetSelector(ie) & FB_CFG_SELECTOR))
               {
                  if(phi_headers.find(current) != phi_headers.end())
                  {
                     /// Phi must always be built in header loop if definition is inside the loop
                     build_phi = true;
                     break;
                  }
                  else
                  {
                     /// Check if this is a irreducible loop
                     const auto current_loop_id = basic_block_graph.CGetNodeInfo(current).loop_id;
                     if(!loops->getConstLoop(current_loop_id)->isReducible())
                     {
                        /// If loop is irreducible, than we have to consider the definition coming from sp_back_edge
                        /// since it can be different: different definitions can enter in the loop in the different
                        /// enter points
                        build_phi = true;
                     }
                  }
               }
               else
               {
                  THROW_ASSERT(reaching_defs.find(virtual_ssa_definition.first) != reaching_defs.end() &&
                                   reaching_defs.find(virtual_ssa_definition.first)->second.count(source),
                               "Definition coming from BB" + STR(basic_block_graph.CGetNodeInfo(source).block->number));
                  local_reaching_defs.insert(reaching_defs.at(virtual_ssa_definition.first).at(source));
               }
            }
            if(local_reaching_defs.size() > 1)
            {
               build_phi = true;
            }

            if(build_phi)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---PHI has to be built");

               std::vector<phi_stmt::DefEdge> def_edges;
               for(const auto& ie : basic_block_graph.in_edges(current))
               {
                  if((basic_block_graph.GetSelector(ie) & CFG_SELECTOR) != 0)
                  {
                     const auto source = basic_block_graph.source(ie);
                     const auto& vssa = reaching_defs.at(virtual_ssa_definition.first).at(source);
                     def_edges.push_back(phi_stmt::DefEdge(vssa, basic_block_graph.CGetNodeInfo(source).block->number));
                  }
               }
               ir_nodeRef phi_res;
               const auto phi_tn = ir_man->create_phi_node(phi_res, def_edges, function_id, true);
               THROW_ASSERT(
                   ir_helper::CGetType(phi_res)->index == ir_helper::CGetType(virtual_ssa_definition.first)->index, "");
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created ssa " + phi_res->ToString());
               GetPointerS<phi_stmt>(phi_tn)->SetSSAUsesComputed();
               basic_block_graph.GetNodeInfo(current).block->AddPhi(phi_tn);
               reaching_defs[virtual_ssa_definition.first][current] = phi_res;
               added_phis[virtual_ssa_definition.first][current] = phi_tn;
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created phi " + phi_tn->ToString());
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---PHI has not to be built");
               THROW_ASSERT(local_reaching_defs.size() == 1, "");
               reaching_defs[virtual_ssa_definition.first][current] = *(local_reaching_defs.begin());
            }
         }

         /// If this is the basic block definition or contains a use
         if(definition_bb == current || use_bbs.count(current))
         {
            bool before_definition = definition_bb == current || check_bb_reachability_cached(current, definition_bb);
            for(const auto& stmt : basic_block_graph.CGetNodeInfo(current).block->CGetStmtList())
            {
               const auto& reaching_def = reaching_defs.at(virtual_ssa_definition.first).at(current);
               if(use_stmts.count(stmt) && stmt->index != virtual_ssa_definition.second->index)
               {
                  if(before_definition)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Adding for anti dependence " + STR(reaching_def) + " in " + STR(stmt));
                     if(GetPointerS<node_stmt>(stmt)->AddVuse(reaching_def))
                     {
                        GetPointerS<ssa_node>(reaching_def)->AddUseStmt(stmt);
                     }
                  }
                  else
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Replacing " + STR(virtual_ssa_definition.first) + " with " + STR(reaching_def) +
                                        " in " + STR(stmt));
                     TM->ReplaceIRNode(stmt, virtual_ssa_definition.first, reaching_def);
                  }
               }
               if(stmt->index == virtual_ssa_definition.second->index)
               {
                  before_definition = false;
                  const auto gn = GetPointerS<node_stmt>(stmt);
                  if(gn->vovers.erase(virtual_ssa_definition.first))
                  {
                     const auto old_vssa = GetPointerS<ssa_node>(virtual_ssa_definition.first);
                     old_vssa->RemoveUse(stmt);
                  }
                  if(gn->AddVover(reaching_def))
                  {
                     const auto reaching_vssa = GetPointerS<ssa_node>(reaching_def);
                     reaching_vssa->AddUseStmt(stmt);
                  }
                  reaching_defs[virtual_ssa_definition.first][current] = virtual_ssa_definition.first;
               }
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Reaching definition at the exit is " +
                            STR(reaching_defs.at(virtual_ssa_definition.first).at(current)));

         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Checking successors");
         for(const auto& oe : basic_block_graph.out_edges(current))
         {
            const auto target = basic_block_graph.target(oe);
            if(loop_basic_blocks.count(target))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Considering BB" + STR(basic_block_graph.CGetNodeInfo(target).block->number));
               if((basic_block_graph.GetSelector(oe) & FB_CFG_SELECTOR) != 0)
               {
                  if(phi_headers.count(target))
                  {
                     THROW_ASSERT(added_phis.find(virtual_ssa_definition.first) != added_phis.end() &&
                                      added_phis.find(virtual_ssa_definition.first)->second.count(target),
                                  "Phi for " + STR(virtual_ssa_definition.first) + " was not created in BB" +
                                      STR(basic_block_graph.CGetNodeInfo(target).block->number));
                     GetPointerS<phi_stmt>(added_phis.at(virtual_ssa_definition.first).at(target))
                         ->AddDefEdge(TM, phi_stmt::DefEdge(reaching_defs.at(virtual_ssa_definition.first).at(current),
                                                            basic_block_graph.CGetNodeInfo(current).block->number));
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Updated phi " + STR(added_phis.at(virtual_ssa_definition.first).at(target)));
                  }
                  else if(!loops->getConstLoop(basic_block_graph.CGetNodeInfo(target).loop_id)->isReducible())
                  {
                     THROW_ASSERT(added_phis.find(virtual_ssa_definition.first) != added_phis.end() &&
                                      added_phis.find(virtual_ssa_definition.first)->second.count(target),
                                  "Phi for " + STR(virtual_ssa_definition.first) + " was not created in BB" +
                                      STR(basic_block_graph.CGetNodeInfo(target).block->number));
                     GetPointerS<phi_stmt>(added_phis.at(virtual_ssa_definition.first).at(target))
                         ->AddDefEdge(TM, phi_stmt::DefEdge(reaching_defs.at(virtual_ssa_definition.first).at(current),
                                                            basic_block_graph.CGetNodeInfo(current).block->number));
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Updated phi " + STR(added_phis.at(virtual_ssa_definition.first).at(target)));
                  }
                  else
                  {
                     IRNodeSet local_reaching_defs;
                     /// Check if phi has to be created because of different definitions coming from outside the loop
                     for(const auto& ie : basic_block_graph.in_edges(target))
                     {
                        if((basic_block_graph.GetSelector(ie) & FB_CFG_SELECTOR) == 0)
                        {
                           const auto source = basic_block_graph.source(ie);
                           THROW_ASSERT(reaching_defs.find(virtual_ssa_definition.first) != reaching_defs.end() &&
                                            reaching_defs.find(virtual_ssa_definition.first)->second.count(source),
                                        "Definition coming from BB" +
                                            STR(basic_block_graph.CGetNodeInfo(source).block->number));
                           local_reaching_defs.insert(reaching_defs.at(virtual_ssa_definition.first).at(source));
                        }
                     }
                     if(local_reaching_defs.size() > 1)
                     {
                        THROW_ASSERT(added_phis.find(virtual_ssa_definition.first) != added_phis.end() &&
                                         added_phis.find(virtual_ssa_definition.first)->second.count(target),
                                     "Phi for " + STR(virtual_ssa_definition.first) + " was not created in BB" +
                                         STR(basic_block_graph.CGetNodeInfo(target).block->number));
                        GetPointerS<phi_stmt>(added_phis.at(virtual_ssa_definition.first).at(target))
                            ->AddDefEdge(TM,
                                         phi_stmt::DefEdge(reaching_defs.at(virtual_ssa_definition.first).at(current),
                                                           basic_block_graph.CGetNodeInfo(current).block->number));
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---Updated phi " + STR(added_phis.at(virtual_ssa_definition.first).at(target)));
                     }
                  }
               }
               else
               {
                  to_be_processed.insert(target);
               }
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Checked successors");
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "<--Checked BB" + STR(basic_block_graph.CGetNodeInfo(current).block->number));
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered ssa " + STR(virtual_ssa_definition.first));
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Checked uses");
   // function_behavior->UpdateBBVersion();
   if(parameters->getOption<bool>(OPT_print_dot))
   {
      function_behavior->GetBBGraph(FunctionBehavior::FBB).writeDot(function_behavior->GetDotPath() / "BB_FCFG.dot");
   }
   bool restart;
   std::set<ir_nodeRef> removedPhis;
   do
   {
      restart = false;
      for(const auto& ssa_bbv : added_phis)
      {
         for(const auto& bbv_phi : ssa_bbv.second)
         {
            if(removedPhis.find(bbv_phi.second) == removedPhis.end())
            {
               const auto& bb = basic_block_graph.GetNodeInfo(bbv_phi.first).block;
               const auto phi_tn = GetPointerS<phi_stmt>(bbv_phi.second);
               const auto vssa = GetPointerS<ssa_node>(phi_tn->res);
               if(vssa->CGetNumberUses() == 0 ||
                  (vssa->CGetNumberUses() == 1 && vssa->CGetUseStmts().begin()->first->index == phi_tn->index))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Removing just created dead phi from BB" + STR(bb->number) + " - (" +
                                     GetPointerS<ssa_node>(ssa_bbv.first)->ToString() + ") " + phi_tn->ToString());
                  bb->RemovePhi(bbv_phi.second);
                  restart = true;
                  removedPhis.insert(bbv_phi.second);
               }
            }
         }
      }
      if(restart)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Phi dead removal restarted");
      }
   } while(restart);
#ifndef NDEBUG
   if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
   {
      for(const auto basic_block : basic_block_graph.vertices())
      {
         const auto& block = basic_block_graph.CGetNodeInfo(basic_block).block;
         for(const auto& phi : block->CGetPhiList())
         {
            const auto gp = GetPointerS<const phi_stmt>(phi);
            if(gp->virtual_flag)
            {
               THROW_ASSERT(gp->CGetDefEdgesList().size() == basic_block_graph.in_degree(basic_block),
                            STR(phi) + " of BB" + STR(block->number) +
                                " has wrong number of inputs: " + STR(gp->CGetDefEdgesList().size()) + " vs " +
                                STR(basic_block_graph.in_degree(basic_block)));
            }
         }
      }
   }
#endif
   return DesignFlowStep_Status::SUCCESS;
}
