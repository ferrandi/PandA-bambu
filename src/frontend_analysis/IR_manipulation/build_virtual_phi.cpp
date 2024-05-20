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
 * @file build_virtual_phi.cpp
 * @brief Analysis step building phi of vops
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Header include
#include "build_virtual_phi.hpp"

///. include
#include "Parameter.hpp"

/// algorithms/loops_detection include
#include "loop.hpp"
#include "loops.hpp"

/// behavior include
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "cdfg_edge_info.hpp"
#include "function_behavior.hpp"

/// parser/compiler include
#include "token_interface.hpp"

/// tree includes
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "string_manipulation.hpp" // for GET_CLASS
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "tree_node.hpp"

#include <set>

BuildVirtualPhi::BuildVirtualPhi(const application_managerRef _AppM, unsigned int _function_id,
                                 const DesignFlowManagerConstRef _design_flow_manager,
                                 const ParameterConstRef _parameters)
    : FunctionFrontendFlowStep(_AppM, _function_id, BUILD_VIRTUAL_PHI, _design_flow_manager, _parameters),
      TM(_AppM->get_tree_manager())
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

BuildVirtualPhi::~BuildVirtualPhi() = default;

void BuildVirtualPhi::ComputeRelationships(DesignFlowStepSet& relationship,
                                           const DesignFlowStep::RelationshipType relationship_type)
{
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      case INVALIDATION_RELATIONSHIP:
      {
         if(GetStatus() == DesignFlowStep_Status::SUCCESS &&
            AppM->CGetFunctionBehavior(function_id)->is_simple_pipeline())
         {
            const auto step_signature =
                FunctionFrontendFlowStep::ComputeSignature(FrontendFlowStepType::SIMPLE_CODE_MOTION, function_id);
            const auto frontend_step = design_flow_manager.lock()->GetDesignFlowStep(step_signature);
            THROW_ASSERT(frontend_step != DesignFlowGraph::null_vertex(), "step is not present");
            const auto design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
            const auto design_flow_step = design_flow_graph->CGetNodeInfo(frontend_step)->design_flow_step;
            relationship.insert(design_flow_step);
         }
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   FunctionFrontendFlowStep::ComputeRelationships(relationship, relationship_type);
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
         relationships.insert(std::make_pair(BB_REACHABILITY_COMPUTATION, SAME_FUNCTION));
         relationships.insert(std::make_pair(DOM_POST_DOM_COMPUTATION, SAME_FUNCTION));
         relationships.insert(std::make_pair(USE_COUNTING, SAME_FUNCTION));
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(BASIC_BLOCKS_CFG_COMPUTATION, SAME_FUNCTION));
#if HAVE_FROM_PRAGMA_BUILT
         relationships.insert(std::make_pair(EXTRACT_OMP_ATOMIC, SAME_FUNCTION));
#endif
         relationships.insert(std::make_pair(VECTORIZE, SAME_FUNCTION));
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

DesignFlowStep_Status BuildVirtualPhi::InternalExec()
{
   const auto tree_man = tree_manipulationRef(new tree_manipulation(TM, parameters, AppM));
   const auto basic_block_graph = function_behavior->GetBBGraph(FunctionBehavior::FBB);
   const auto loops = function_behavior->CGetLoops();
   const auto bb_index_map = basic_block_graph->CGetBBGraphInfo()->bb_index_map;
   /// Cache of created phi - first key is the used ssa - second key is the basic block where is created
   TreeNodeMap<CustomUnorderedMapStable<vertex, tree_nodeRef>> added_phis;
   /// Cache of reaching defs - first key is the used ssa - second key is the basic block to be considered
   TreeNodeMap<CustomUnorderedMapStable<vertex, tree_nodeRef>> reaching_defs;
   /// For each virtual operand its definition
   TreeNodeMap<tree_nodeConstRef> virtual_ssa_definitions;
   /// The set of nodes which overwrite a vop
   TreeNodeMap<TreeNodeSet> vovers;

   /// Computing definitions and overwriting
   VertexIterator basic_block, basic_block_end;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Computing definitions");
   for(boost::tie(basic_block, basic_block_end) = boost::vertices(*basic_block_graph); basic_block != basic_block_end;
       basic_block++)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "-->Analyzing BB" + STR(basic_block_graph->CGetBBNodeInfo(*basic_block)->block->number));
      const auto block_info = basic_block_graph->CGetBBNodeInfo(*basic_block)->block;
      for(const auto& stmt : block_info->CGetStmtList())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing stmt " + STR(stmt));
         const auto gn = GetPointerS<gimple_node>(stmt);
         if(gn->vdef)
         {
            THROW_ASSERT(virtual_ssa_definitions.count(gn->vdef) == 0,
                         gn->vdef->ToString() + " is defined also in " + STR(virtual_ssa_definitions.at(gn->vdef)));
            virtual_ssa_definitions[gn->vdef] = stmt;
         }
         const auto& cur_bb = bb_index_map.at(gn->bb_index);
         const auto vo_it = gn->vdef ? gn->vovers.find(gn->vdef) : gn->vovers.end();
         if(vo_it != gn->vovers.end() && !function_behavior->CheckBBReachability(cur_bb, cur_bb))
         {
            gn->vovers.erase(vo_it);
            GetPointerS<ssa_name>(gn->vdef)->RemoveUse(stmt);
         }
         for(const auto& vover : gn->vovers)
         {
            vovers[vover].insert(stmt);
         }
         /// clean not reachable vuses
         auto vu_it = gn->vuses.begin();
         while(vu_it != gn->vuses.end())
         {
            const auto sn = GetPointerS<ssa_name>(*vu_it);
            const auto def_stmt = sn->CGetDefStmt();
            const auto use_bb_index = GetPointerS<const gimple_node>(def_stmt)->bb_index;
            const auto& use_bb = bb_index_map.at(use_bb_index);
            if(use_bb_index == gn->bb_index)
            {
               /// here we may have a Use-Def or a Def-Use. They are both perfectly fine.
               ++vu_it;
            }
            else if(!function_behavior->CheckBBReachability(use_bb, cur_bb) &&
                    !function_behavior->CheckBBReachability(cur_bb, use_bb))
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
                     "<--Analyzed BB" + STR(basic_block_graph->CGetBBNodeInfo(*basic_block)->block->number));
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Computed definitions");

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Checking uses");
   /// Check uses
   for(const auto& virtual_ssa_definition : virtual_ssa_definitions)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "-->Considering ssa " + virtual_ssa_definition.first->ToString());
      const auto sn = GetPointerS<ssa_name>(virtual_ssa_definition.first);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---Defined in " + virtual_ssa_definition.second->ToString());
      const auto definition = GetPointerS<const gimple_node>(virtual_ssa_definition.second);
      THROW_ASSERT(definition, STR(sn->CGetDefStmt()));
      const auto definition_bb_index = definition->bb_index;
      const auto& definition_bb = bb_index_map.at(definition_bb_index);
      THROW_ASSERT(sn, STR(virtual_ssa_definition.first));

      /// The index of the loop to be considered (the most internal loops which contains the definition and all the uses
      auto loop_id = basic_block_graph->CGetBBNodeInfo(definition_bb)->loop_id;

      /// The depth of the loop to be considered
      auto depth = loops->CGetLoop(loop_id)->depth;

      /// The set of basic block uses
      CustomSet<vertex> use_bbs;

      /// The set of statement uses
      TreeNodeSet use_stmts;

      /// The set of false uses
      TreeNodeMap<size_t> transitive_uses;

      for(const auto& use_stmt : sn->CGetUseStmts())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering use in " + STR(use_stmt.first));
         const auto use_bb_index = GetPointerS<const gimple_node>(use_stmt.first)->bb_index;
         const auto& use_bb = bb_index_map.at(use_bb_index);

         const auto gn = GetPointerS<const gimple_node>(use_stmt.first);

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
                  const auto vover_bb_index = GetPointerS<const gimple_node>(vover_stmt)->bb_index;
                  const auto vover_bb = bb_index_map.at(vover_bb_index);
                  if(function_behavior->CheckBBReachability(use_bb, vover_bb) || use_bb == vover_bb)
                  {
                     return false;
                  }
               }
            }
            for(const auto& other_use_stmt : sn->CGetUseStmts())
            {
               const auto other_use_bb_index = GetPointerS<const gimple_node>(other_use_stmt.first)->bb_index;
               const auto other_use_bb = bb_index_map.at(other_use_bb_index);
               if(other_use_stmt.first->index != use_stmt.first->index &&
                  function_behavior->CheckBBReachability(other_use_bb, use_bb) &&
                  other_use_stmt.first->index != virtual_ssa_definition.second->index)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Considered other use: " + STR(other_use_stmt.first->index) + " " +
                                     STR(other_use_stmt.first));
                  const auto other_gn = GetPointerS<const gimple_node>(other_use_stmt.first);
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

         auto use_loop_id = basic_block_graph->CGetBBNodeInfo(use_bb)->loop_id;
         auto use_depth = loops->CGetLoop(use_loop_id)->depth;

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
      for(const auto& transitive_use : transitive_uses)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Removing " + STR(virtual_ssa_definition.first) + " from vuses of " +
                            STR(transitive_use.first));
         const auto gn = GetPointerS<gimple_node>(transitive_use.first);
         gn->vuses.erase(virtual_ssa_definition.first);
         sn->RemoveUse(transitive_use.first);
      }

      /// It is possible that the use is not reachable; this is due to the gcc alias oracle
      if(use_bbs.empty())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--There is not any reachable use");
         continue;
      }

      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Loop to be considered is " + STR(loop_id));

      // The set of header basic blocks where a phi has to be inserted
      CustomSet<vertex> phi_headers;
      auto current_loop = loops->CGetLoop(basic_block_graph->CGetBBNodeInfo(definition_bb)->loop_id);
      while(current_loop->GetId() != loop_id)
      {
         for(const auto& cur_pair : current_loop->get_sp_back_edges())
         {
            phi_headers.insert(cur_pair.second);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Phi has to be added in header of loop " + STR(current_loop->GetId()));
         current_loop = current_loop->Parent();
      }
      for(const auto& cur_pair : current_loop->get_sp_back_edges())
      {
         phi_headers.insert(cur_pair.second);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---Phi has to be added in header of loop " + STR(current_loop->GetId()));

      /// Build the gimple nop where volatile ssa is defined
      std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> nop_IR_schema;
      const auto nop_id = TM->new_tree_node_id();
      nop_IR_schema[TOK(TOK_SRCP)] = BUILTIN_SRCP;
      nop_IR_schema[TOK(TOK_SCPE)] = STR(function_id);
      TM->create_tree_node(nop_id, gimple_nop_K, nop_IR_schema);

      const auto volatile_sn = tree_man->create_ssa_name(sn->var, tree_helper::CGetType(virtual_ssa_definition.first),
                                                         nullptr, nullptr, true, true);
      GetPointerS<ssa_name>(volatile_sn)->SetDefStmt(TM->GetTreeNode(nop_id));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created volatile ssa " + STR(volatile_sn));

      /// Set of basic blocks belonging to the loop
      CustomUnorderedSet<vertex> loop_basic_blocks;
      loops->CGetLoop(loop_id)->get_recursively_bb(loop_basic_blocks);

      /// Set of basic blocks to be analyzed
      const bb_vertex_order_by_map comp_i(function_behavior->get_bb_map_levels());
      std::set<vertex, bb_vertex_order_by_map> to_be_processed(comp_i);

      /// Loop 0 must be managed in a different way
      if(loop_id == 0)
      {
         reaching_defs[virtual_ssa_definition.first][basic_block_graph->CGetBBGraphInfo()->entry_vertex] = volatile_sn;
         OutEdgeIterator oe, oe_end;
         for(boost::tie(oe, oe_end) =
                 boost::out_edges(basic_block_graph->CGetBBGraphInfo()->entry_vertex, *basic_block_graph);
             oe != oe_end; oe++)
         {
            const auto target = boost::target(*oe, *basic_block_graph);
            if(basic_block_graph->CGetBBGraphInfo()->exit_vertex != target)
            {
               to_be_processed.insert(target);
            }
         }

         /// Remove all basic blocks from which no use can be reached
         CustomSet<vertex> to_be_removed;
         for(const auto current : loop_basic_blocks)
         {
            bool reachable = false;
            for(const auto use_bb : use_bbs)
            {
               if(function_behavior->CheckBBFeedbackReachability(current, use_bb))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---BB" + STR(basic_block_graph->CGetBBNodeInfo(use_bb)->block->number) +
                                     " can be reached from BB" +
                                     STR(basic_block_graph->CGetBBNodeInfo(current)->block->number));
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
                           "---Removing BB" + STR(basic_block_graph->CGetBBNodeInfo(removable)->block->number));
            loop_basic_blocks.erase(removable);
         }
      }
      else
      {
         CustomUnorderedSet<vertex> loop_bbs;
         loops->CGetLoop(loop_id)->get_recursively_bb(loop_bbs);
         for(const auto& loop_bb : loop_bbs)
         {
            InEdgeIterator ei, ei_end;
            for(boost::tie(ei, ei_end) = boost::in_edges(loop_bb, *basic_block_graph); ei != ei_end; ei++)
            {
               const auto source = boost::source(*ei, *basic_block_graph);
               if(loop_bbs.find(source) == loop_bbs.end())
               {
                  reaching_defs[virtual_ssa_definition.first][source] = volatile_sn;
               }
            }
         }
         for(const auto& feedback_edge : loops->CGetLoop(loop_id)->get_sp_back_edges())
         {
            to_be_processed.insert(feedback_edge.second);
         }
      }

      while(to_be_processed.size())
      {
         const auto current = *(to_be_processed.begin());
         to_be_processed.erase(current);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "-->Checking BB" + STR(basic_block_graph->CGetBBNodeInfo(current)->block->number));

         InEdgeIterator ie, ie_end;
         if(boost::in_degree(current, *basic_block_graph) == 1)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Single entry BB");
            boost::tie(ie, ie_end) = boost::in_edges(current, *basic_block_graph);
            const auto source = boost::source(*ie, *basic_block_graph);
            THROW_ASSERT(reaching_defs.at(virtual_ssa_definition.first).count(source), "unexpected condition");
            reaching_defs[virtual_ssa_definition.first][current] =
                reaching_defs.at(virtual_ssa_definition.first).at(source);
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Multiple entries BB");
            /// The phi is necessary only if there are different reaching definition
            bool build_phi = false;
            TreeNodeSet local_reaching_defs;
            for(boost::tie(ie, ie_end) = boost::in_edges(current, *basic_block_graph); ie != ie_end; ie++)
            {
               const auto source = boost::source(*ie, *basic_block_graph);
               if((basic_block_graph->GetSelector(*ie) & FB_CFG_SELECTOR))
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
                     const auto current_loop_id = basic_block_graph->CGetBBNodeInfo(current)->loop_id;
                     if(!loops->CGetLoop(current_loop_id)->IsReducible())
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
                               "Definition coming from BB" +
                                   STR(basic_block_graph->CGetBBNodeInfo(source)->block->number));
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

               std::vector<gimple_phi::DefEdge> def_edges;
               for(boost::tie(ie, ie_end) = boost::in_edges(current, *basic_block_graph); ie != ie_end; ie++)
               {
                  if((basic_block_graph->GetSelector(*ie) & CFG_SELECTOR) != 0)
                  {
                     const auto source = boost::source(*ie, *basic_block_graph);
                     const auto& vssa = reaching_defs.at(virtual_ssa_definition.first).at(source);
                     def_edges.push_back(
                         gimple_phi::DefEdge(vssa, basic_block_graph->CGetBBNodeInfo(source)->block->number));
                  }
               }
               tree_nodeRef phi_res;
               const auto phi_stmt = tree_man->create_phi_node(phi_res, def_edges, function_id, true);
               THROW_ASSERT(tree_helper::CGetType(phi_res)->index ==
                                tree_helper::CGetType(virtual_ssa_definition.first)->index,
                            "");
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created ssa " + phi_res->ToString());
               GetPointerS<gimple_phi>(phi_stmt)->SetSSAUsesComputed();
               basic_block_graph->GetBBNodeInfo(current)->block->AddPhi(phi_stmt);
               reaching_defs[virtual_ssa_definition.first][current] = phi_res;
               added_phis[virtual_ssa_definition.first][current] = phi_stmt;
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created phi " + phi_stmt->ToString());
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
            bool before_definition =
                definition_bb == current || function_behavior->CheckBBReachability(current, definition_bb);
            for(const auto& stmt : basic_block_graph->CGetBBNodeInfo(current)->block->CGetStmtList())
            {
               const auto& reaching_def = reaching_defs.at(virtual_ssa_definition.first).at(current);
               if(use_stmts.count(stmt) && stmt->index != virtual_ssa_definition.second->index)
               {
                  if(before_definition)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Adding for anti dependence " + STR(reaching_def) + " in " + STR(stmt));
                     THROW_ASSERT(!GetPointerS<ssa_name>(reaching_def)->volatile_flag ||
                                      !function_behavior->CheckBBFeedbackReachability(definition_bb, current),
                                  "");
                     if(GetPointerS<gimple_node>(stmt)->AddVuse(reaching_def))
                     {
                        GetPointerS<ssa_name>(reaching_def)->AddUseStmt(stmt);
                     }
                  }
                  else
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Replacing " + STR(virtual_ssa_definition.first) + " with " + STR(reaching_def) +
                                        " in " + STR(stmt));
                     THROW_ASSERT(!GetPointerS<ssa_name>(reaching_def)->volatile_flag ||
                                      !function_behavior->CheckBBFeedbackReachability(definition_bb, current),
                                  "");
                     TM->ReplaceTreeNode(stmt, virtual_ssa_definition.first, reaching_def);
                  }
               }
               if(stmt->index == virtual_ssa_definition.second->index)
               {
                  before_definition = false;
                  const auto gn = GetPointerS<gimple_node>(stmt);
                  if(gn->vovers.erase(virtual_ssa_definition.first))
                  {
                     const auto old_vssa = GetPointerS<ssa_name>(virtual_ssa_definition.first);
                     old_vssa->RemoveUse(stmt);
                  }
                  if(gn->AddVover(reaching_def))
                  {
                     const auto reaching_vssa = GetPointerS<ssa_name>(reaching_def);
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
         OutEdgeIterator oe, oe_end;
         for(boost::tie(oe, oe_end) = boost::out_edges(current, *basic_block_graph); oe != oe_end; oe++)
         {
            const auto target = boost::target(*oe, *basic_block_graph);
            if(loop_basic_blocks.count(target))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Considering BB" + STR(basic_block_graph->CGetBBNodeInfo(target)->block->number));
               if((basic_block_graph->GetSelector(*oe) & FB_CFG_SELECTOR) != 0)
               {
                  if(phi_headers.count(target))
                  {
                     THROW_ASSERT(added_phis.find(virtual_ssa_definition.first) != added_phis.end() &&
                                      added_phis.find(virtual_ssa_definition.first)->second.count(target),
                                  "Phi for " + STR(virtual_ssa_definition.first) + " was not created in BB" +
                                      STR(basic_block_graph->CGetBBNodeInfo(target)->block->number));
                     GetPointerS<gimple_phi>(added_phis.at(virtual_ssa_definition.first).at(target))
                         ->AddDefEdge(TM,
                                      gimple_phi::DefEdge(reaching_defs.at(virtual_ssa_definition.first).at(current),
                                                          basic_block_graph->CGetBBNodeInfo(current)->block->number));
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Updated phi " + STR(added_phis.at(virtual_ssa_definition.first).at(target)));
                  }
                  else if(!loops->CGetLoop(basic_block_graph->CGetBBNodeInfo(target)->loop_id)->IsReducible())
                  {
                     THROW_ASSERT(added_phis.find(virtual_ssa_definition.first) != added_phis.end() &&
                                      added_phis.find(virtual_ssa_definition.first)->second.count(target),
                                  "Phi for " + STR(virtual_ssa_definition.first) + " was not created in BB" +
                                      STR(basic_block_graph->CGetBBNodeInfo(target)->block->number));
                     GetPointerS<gimple_phi>(added_phis.at(virtual_ssa_definition.first).at(target))
                         ->AddDefEdge(TM,
                                      gimple_phi::DefEdge(reaching_defs.at(virtual_ssa_definition.first).at(current),
                                                          basic_block_graph->CGetBBNodeInfo(current)->block->number));
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Updated phi " + STR(added_phis.at(virtual_ssa_definition.first).at(target)));
                  }
                  else
                  {
                     TreeNodeSet local_reaching_defs;
                     /// Check if phi has to be created because of different definitions coming from outside the loop
                     for(boost::tie(ie, ie_end) = boost::in_edges(target, *basic_block_graph); ie != ie_end; ie++)
                     {
                        if((basic_block_graph->GetSelector(*ie) & FB_CFG_SELECTOR) == 0)
                        {
                           const auto source = boost::source(*ie, *basic_block_graph);
                           THROW_ASSERT(reaching_defs.find(virtual_ssa_definition.first) != reaching_defs.end() &&
                                            reaching_defs.find(virtual_ssa_definition.first)->second.count(source),
                                        "Definition coming from BB" +
                                            STR(basic_block_graph->CGetBBNodeInfo(source)->block->number));
                           local_reaching_defs.insert(reaching_defs.at(virtual_ssa_definition.first).at(source));
                        }
                     }
                     if(local_reaching_defs.size() > 1)
                     {
                        THROW_ASSERT(added_phis.find(virtual_ssa_definition.first) != added_phis.end() &&
                                         added_phis.find(virtual_ssa_definition.first)->second.count(target),
                                     "Phi for " + STR(virtual_ssa_definition.first) + " was not created in BB" +
                                         STR(basic_block_graph->CGetBBNodeInfo(target)->block->number));
                        GetPointerS<gimple_phi>(added_phis.at(virtual_ssa_definition.first).at(target))
                            ->AddDefEdge(
                                TM, gimple_phi::DefEdge(reaching_defs.at(virtual_ssa_definition.first).at(current),
                                                        basic_block_graph->CGetBBNodeInfo(current)->block->number));
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
                        "<--Checked BB" + STR(basic_block_graph->CGetBBNodeInfo(current)->block->number));
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered ssa " + STR(virtual_ssa_definition.first));
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Checked uses");
   // function_behavior->UpdateBBVersion();
   if(parameters->getOption<bool>(OPT_print_dot))
   {
      function_behavior->GetBBGraph(FunctionBehavior::FBB)->WriteDot("BB_FCFG.dot");
   }
   bool restart;
   std::set<tree_nodeRef> removedPhis;
   do
   {
      restart = false;
      for(const auto& ssa_bbv : added_phis)
      {
         for(const auto& bbv_phi : ssa_bbv.second)
         {
            if(removedPhis.find(bbv_phi.second) == removedPhis.end())
            {
               const auto& bb = basic_block_graph->GetBBNodeInfo(bbv_phi.first)->block;
               const auto phi_stmt = GetPointerS<gimple_phi>(bbv_phi.second);
               const auto vssa = GetPointerS<ssa_name>(phi_stmt->res);
               if(vssa->CGetNumberUses() == 0 ||
                  (vssa->CGetNumberUses() == 1 && vssa->CGetUseStmts().begin()->first->index == phi_stmt->index))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Removing just created dead phi from BB" + STR(bb->number) + " - (" +
                                     GetPointerS<ssa_name>(ssa_bbv.first)->ToString() + ") " + phi_stmt->ToString());
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
      for(boost::tie(basic_block, basic_block_end) = boost::vertices(*basic_block_graph);
          basic_block != basic_block_end; basic_block++)
      {
         const auto& block = basic_block_graph->CGetBBNodeInfo(*basic_block)->block;
         for(const auto& phi : block->CGetPhiList())
         {
            const auto gp = GetPointerS<const gimple_phi>(phi);
            if(gp->virtual_flag)
            {
               THROW_ASSERT(gp->CGetDefEdgesList().size() == boost::in_degree(*basic_block, *basic_block_graph),
                            STR(phi) + " of BB" + STR(block->number) +
                                " has wrong number of inputs: " + STR(gp->CGetDefEdgesList().size()) + " vs " +
                                STR(boost::in_degree(*basic_block, *basic_block_graph)));
            }
         }
      }
   }
#endif
   return DesignFlowStep_Status::SUCCESS;
}
