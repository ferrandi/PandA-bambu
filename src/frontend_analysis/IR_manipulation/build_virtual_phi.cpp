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

/// parser/treegcc include
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
#include "tree_reindex.hpp"

#include <set>

void BuildVirtualPhi::ComputeRelationships(DesignFlowStepSet& relationship, const DesignFlowStep::RelationshipType relationship_type)
{
   switch(relationship_type)
   {
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      case DEPENDENCE_RELATIONSHIP:
      {
         break;
      }
      case INVALIDATION_RELATIONSHIP:
      {
         if(design_flow_manager.lock()->GetStatus(GetSignature()) == DesignFlowStep_Status::SUCCESS && AppM->CGetFunctionBehavior(function_id)->is_pipelining_enabled())
         {
            const std::string step_signature = FunctionFrontendFlowStep::ComputeSignature(FrontendFlowStepType::SIMPLE_CODE_MOTION, function_id);
            vertex frontend_step = design_flow_manager.lock()->GetDesignFlowStep(step_signature);
            THROW_ASSERT(frontend_step != NULL_VERTEX, "step " + step_signature + " is not present");
            const DesignFlowGraphConstRef design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
            const DesignFlowStepRef design_flow_step = design_flow_graph->CGetDesignFlowStepInfo(frontend_step)->design_flow_step;
            relationship.insert(design_flow_step);
         }
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   FunctionFrontendFlowStep::ComputeRelationships(relationship, relationship_type);
}

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> BuildVirtualPhi::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(BB_FEEDBACK_EDGES_IDENTIFICATION, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(DOM_POST_DOM_COMPUTATION, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(BB_REACHABILITY_COMPUTATION, SAME_FUNCTION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
#if HAVE_FROM_PRAGMA_BUILT && HAVE_BAMBU_BUILT
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(EXTRACT_OMP_ATOMIC, SAME_FUNCTION));
#endif
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(VECTORIZE, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(BASIC_BLOCKS_CFG_COMPUTATION, SAME_FUNCTION));
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}

BuildVirtualPhi::BuildVirtualPhi(const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters)
    : FunctionFrontendFlowStep(_AppM, _function_id, BUILD_VIRTUAL_PHI, _design_flow_manager, _parameters), TM(_AppM->get_tree_manager())
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

BuildVirtualPhi::~BuildVirtualPhi() = default;

DesignFlowStep_Status BuildVirtualPhi::InternalExec()
{
   const auto loops = function_behavior->CGetLoops();
   const auto bb_index_map = basic_block_graph->CGetBBGraphInfo()->bb_index_map;

   /// For each virtual operand its definition
   TreeNodeMap<tree_nodeConstRef> virtual_ssa_definitions;

   /// The set of nodes which overwrite a vop
   TreeNodeMap<TreeNodeSet> vovers;

   /// Computing definitions and overwriting
   VertexIterator basic_block, basic_block_end;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Computing definitions");
   for(boost::tie(basic_block, basic_block_end) = boost::vertices(*basic_block_graph); basic_block != basic_block_end; basic_block++)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing BB" + STR(basic_block_graph->CGetBBNodeInfo(*basic_block)->block->number));
      const auto block_info = basic_block_graph->CGetBBNodeInfo(*basic_block)->block;
      for(const auto& stmt : block_info->CGetStmtList())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing stmt " + STR(stmt));
         auto gn = GetPointer<gimple_node>(GET_NODE(stmt));
         if(gn->vdef)
         {
            THROW_ASSERT(virtual_ssa_definitions.find(gn->vdef) == virtual_ssa_definitions.end(), gn->vdef->ToString() + " is defined also in " + virtual_ssa_definitions.find(gn->vdef)->second->ToString());
            virtual_ssa_definitions[gn->vdef] = stmt;
         }
         gn->memdef = tree_nodeRef();
         if(gn->memuse)
         {
            GetPointer<ssa_name>(GET_NODE(gn->memuse))->RemoveUse(stmt);
            gn->memuse = tree_nodeRef();
         }
         const auto cur_bb_index = gn->bb_index;
         const auto cur_bb = bb_index_map.find(cur_bb_index)->second;
         if(gn->vdef && gn->vovers.find(gn->vdef) != gn->vovers.end() && !function_behavior->CheckBBReachability(cur_bb, cur_bb))
         {
            gn->vovers.erase(gn->vdef);
         }
         for(const auto& vover : gn->vovers)
         {
            vovers[vover].insert(stmt);
         }
         /// clean not reachable vuses
         std::list<tree_nodeRef> to_be_removed;
         for(const auto& vu : gn->vuses)
         {
            auto sn = GetPointer<ssa_name>(GET_NODE(vu));
            auto def_stmt = sn->CGetDefStmt();
            const auto use_bb_index = GetPointer<const gimple_node>(GET_NODE(def_stmt))->bb_index;
            const auto use_bb = bb_index_map.find(use_bb_index)->second;
            if(use_bb_index == cur_bb_index)
            {
               /// here we may have a Use-Def or a Def-Use. They are both perfectly fine.
            }
            else if(!function_behavior->CheckBBReachability(use_bb, cur_bb) && !function_behavior->CheckBBReachability(cur_bb, use_bb))
            {
               sn->RemoveUse(stmt);
               to_be_removed.push_back(vu);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Removing " + STR(vu) + " from vuses of unreachable stmt: " + STR(sn));
            }
         }
         for(auto vu : to_be_removed)
            gn->vuses.erase(gn->vuses.find(vu));

         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed stmt " + STR(stmt));
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed BB" + STR(basic_block_graph->CGetBBNodeInfo(*basic_block)->block->number));
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Computed definitions");

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Checking uses");
   /// Check uses
   for(const auto& virtual_ssa_definition : virtual_ssa_definitions)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering ssa " + virtual_ssa_definition.first->ToString());
      auto sn = GetPointer<ssa_name>(GET_NODE(virtual_ssa_definition.first));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Defined in " + virtual_ssa_definition.second->ToString());
      const auto definition = GetPointer<const gimple_node>(GET_CONST_NODE(virtual_ssa_definition.second));
      THROW_ASSERT(definition, GET_CONST_NODE(sn->CGetDefStmt())->ToString());
      const auto definition_bb_index = definition->bb_index;
      const auto definition_bb = bb_index_map.find(definition_bb_index)->second;
      THROW_ASSERT(sn, virtual_ssa_definition.first->ToString());

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
         const auto use_bb_index = GetPointer<const gimple_node>(GET_NODE(use_stmt.first))->bb_index;
         const auto use_bb = bb_index_map.find(use_bb_index)->second;

         const auto gn = GetPointer<const gimple_node>(GET_NODE(use_stmt.first));

         /// Check if this use can be ignored because of transitive reduction
         bool skip = [&]() -> bool {
            if(definition_bb_index == use_bb_index)
               return false;
            if(vovers.find(virtual_ssa_definition.first) != vovers.end())
            {
               for(const auto& vover_stmt : vovers.find(virtual_ssa_definition.first)->second)
               {
                  const auto vover_bb_index = GetPointer<const gimple_node>(GET_NODE(vover_stmt))->bb_index;
                  const auto vover_bb = bb_index_map.find(vover_bb_index)->second;
                  if(function_behavior->CheckBBReachability(use_bb, vover_bb) or use_bb == vover_bb)
                  {
                     return false;
                  }
               }
            }
            for(const auto& other_use_stmt : sn->CGetUseStmts())
            {
               const auto other_use_bb_index = GetPointer<const gimple_node>(GET_NODE(other_use_stmt.first))->bb_index;
               const auto other_use_bb = bb_index_map.find(other_use_bb_index)->second;
               if(other_use_stmt.first->index != use_stmt.first->index and function_behavior->CheckBBReachability(other_use_bb, use_bb) and other_use_stmt.first->index != virtual_ssa_definition.second->index)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Considered other use: " + STR(other_use_stmt.first->index) + " " + STR(other_use_stmt.first));
                  const auto other_gn = GetPointer<const gimple_node>(GET_NODE(other_use_stmt.first));
                  if(other_gn->vdef and gn->vuses.find(other_gn->vdef) != gn->vuses.end())
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
      for(const auto& transitive_use : transitive_uses)
      {
         for(size_t number_use = 0; number_use < transitive_use.second; number_use++)
         {
            sn->RemoveUse(transitive_use.first);
            GetPointer<gimple_node>(GET_NODE(transitive_use.first))->vuses.erase(virtual_ssa_definition.first);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Removing " + STR(virtual_ssa_definition.first) + " from vuses of " + STR(transitive_use.first));
         }
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
         for(auto cur_pair : current_loop->get_sp_back_edges())
            phi_headers.insert(cur_pair.second);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Phi has to be added in header of loop " + STR(current_loop->GetId()));
         current_loop = current_loop->Parent();
      }
      for(auto cur_pair : current_loop->get_sp_back_edges())
         phi_headers.insert(cur_pair.second);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Phi has to be added in header of loop " + STR(current_loop->GetId()));

      /// Build the gimple nop where volatile ssa is defined
      std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> nop_IR_schema;
      const auto nop_id = TM->new_tree_node_id();
      nop_IR_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
      TM->create_tree_node(nop_id, gimple_nop_K, nop_IR_schema);

      unsigned int ssa_vers = TM->get_next_vers();
      const auto volatile_id = TM->new_tree_node_id();
      std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> ssa_IR_schema;
      ssa_IR_schema[TOK(TOK_TYPE)] = STR(tree_helper::get_type_index(TM, sn->index));
      ssa_IR_schema[TOK(TOK_VERS)] = STR(ssa_vers);
      if(sn->var)
         ssa_IR_schema[TOK(TOK_VAR)] = STR(sn->var->index);
      ssa_IR_schema[TOK(TOK_VOLATILE)] = STR(true);
      ssa_IR_schema[TOK(TOK_VIRTUAL)] = STR(true);
      TM->create_tree_node(volatile_id, ssa_name_K, ssa_IR_schema);
      const auto volatile_sn = TM->GetTreeReindex(volatile_id);
      GetPointer<ssa_name>(GET_NODE(volatile_sn))->SetDefStmt(TM->GetTreeReindex(nop_id));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created volatile ssa _" + STR(ssa_vers));

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
         for(boost::tie(oe, oe_end) = boost::out_edges(basic_block_graph->CGetBBGraphInfo()->entry_vertex, *basic_block_graph); oe != oe_end; oe++)
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
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---BB" + STR(basic_block_graph->CGetBBNodeInfo(use_bb)->block->number) + " can be reached from BB" + STR(basic_block_graph->CGetBBNodeInfo(current)->block->number));
                  reachable = true;
                  break;
               }
               else if(current == use_bb)
               {
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
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Removing BB" + STR(basic_block_graph->CGetBBNodeInfo(removable)->block->number));
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
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Checking BB" + STR(basic_block_graph->CGetBBNodeInfo(current)->block->number));

         InEdgeIterator ie, ie_end;
         if(boost::in_degree(current, *basic_block_graph) == 1)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Single entry BB");
            boost::tie(ie, ie_end) = boost::in_edges(current, *basic_block_graph);
            const auto source = boost::source(*ie, *basic_block_graph);
            THROW_ASSERT(reaching_defs.at(virtual_ssa_definition.first).find(source) != reaching_defs.at(virtual_ssa_definition.first).end(), "unexpected condition");
            reaching_defs[virtual_ssa_definition.first][current] = reaching_defs[virtual_ssa_definition.first][source];
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
                     if(not loops->CGetLoop(current_loop_id)->IsReducible())
                     {
                        /// If loop is irreducible, than we have to consider the definition coming from sp_back_edge since it can be different: different definitions can enter in the loop in the different enter points
                        build_phi = true;
                     }
                  }
               }
               else
               {
                  THROW_ASSERT(reaching_defs.find(virtual_ssa_definition.first) != reaching_defs.end() and reaching_defs.find(virtual_ssa_definition.first)->second.find(source) != reaching_defs.find(virtual_ssa_definition.first)->second.end(),
                               "Definition coming from BB" + STR(basic_block_graph->CGetBBNodeInfo(source)->block->number));
                  local_reaching_defs.insert(reaching_defs[virtual_ssa_definition.first][source]);
               }
            }
            if(local_reaching_defs.size() > 1)
            {
               build_phi = true;
            }

            if(build_phi)
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
               IR_schema[TOK(TOK_VIRTUAL)] = STR(true);
               TM->create_tree_node(phi_def_ssa_node_nid, ssa_name_K, IR_schema);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created ssa _" + STR(phi_ssa_vers));
               IR_schema.clear();
               tree_nodeRef phi_def_ssa_node = TM->GetTreeReindex(phi_def_ssa_node_nid);

               unsigned int phi_gimple_stmt_id = TM->new_tree_node_id();
               IR_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
               IR_schema[TOK(TOK_RES)] = boost::lexical_cast<std::string>(phi_def_ssa_node_nid);
               IR_schema[TOK(TOK_VIRTUAL)] = STR(true);
               TM->create_tree_node(phi_gimple_stmt_id, gimple_phi_K, IR_schema);
               auto new_gp = GetPointer<gimple_phi>(TM->get_tree_node_const(phi_gimple_stmt_id));
               new_gp->SetSSAUsesComputed();
               reaching_defs[virtual_ssa_definition.first][current] = TM->GetTreeReindex(phi_def_ssa_node_nid);

               for(boost::tie(ie, ie_end) = boost::in_edges(current, *basic_block_graph); ie != ie_end; ie++)
               {
                  if((basic_block_graph->GetSelector(*ie) & CFG_SELECTOR) != 0)
                  {
                     const auto source = boost::source(*ie, *basic_block_graph);
                     new_gp->AddDefEdge(TM, gimple_phi::DefEdge(reaching_defs[virtual_ssa_definition.first][source], basic_block_graph->CGetBBNodeInfo(source)->block->number));
                  }
               }
               basic_block_graph->GetBBNodeInfo(current)->block->AddPhi(TM->GetTreeReindex(phi_gimple_stmt_id));
               reaching_defs[virtual_ssa_definition.first][current] = TM->GetTreeReindex(phi_def_ssa_node_nid);
               added_phis[virtual_ssa_definition.first][current] = TM->GetTreeReindex(phi_gimple_stmt_id);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created phi " + new_gp->ToString());
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---PHI has not to be built");
               THROW_ASSERT(local_reaching_defs.size() == 1, "");
               reaching_defs[virtual_ssa_definition.first][current] = *(local_reaching_defs.begin());
            }
         }

         /// If this is the basic block definition or contains a use
         if(definition_bb == current or use_bbs.find(current) != use_bbs.end())
         {
            bool before_definition = definition_bb == current or function_behavior->CheckBBReachability(current, definition_bb);
            for(const auto& stmt : basic_block_graph->CGetBBNodeInfo(current)->block->CGetStmtList())
            {
               if(use_stmts.find(stmt) != use_stmts.end() and stmt->index != virtual_ssa_definition.second->index)
               {
                  if(before_definition)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding for anti dependence " + STR(reaching_defs[virtual_ssa_definition.first][current]) + " in " + STR(stmt));
                     THROW_ASSERT(not GetPointer<ssa_name>(GET_NODE(reaching_defs[virtual_ssa_definition.first][current]))->volatile_flag or not function_behavior->CheckBBFeedbackReachability(definition_bb, current), "");
                     GetPointer<gimple_node>(GET_NODE(stmt))->vuses.insert(reaching_defs[virtual_ssa_definition.first][current]);
                     GetPointer<ssa_name>(GET_NODE(reaching_defs[virtual_ssa_definition.first][current]))->AddUseStmt(stmt);
                  }
                  else
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Replacing " + STR(virtual_ssa_definition.first) + " with " + STR(reaching_defs[virtual_ssa_definition.first][current]) + " in " + STR(stmt));
                     THROW_ASSERT(not GetPointer<ssa_name>(GET_NODE(reaching_defs[virtual_ssa_definition.first][current]))->volatile_flag or not function_behavior->CheckBBFeedbackReachability(definition_bb, current), "");
                     TM->ReplaceTreeNode(stmt, virtual_ssa_definition.first, reaching_defs[virtual_ssa_definition.first][current]);
                  }
               }
               if(stmt->index == virtual_ssa_definition.second->index)
               {
                  before_definition = false;
                  auto gn = GetPointer<gimple_node>(GET_NODE(stmt));
                  if(gn->vovers.find(virtual_ssa_definition.first) != gn->vovers.end())
                  {
                     GetPointer<gimple_node>(GET_NODE(stmt))->vovers.erase(virtual_ssa_definition.first);
                     GetPointer<gimple_node>(GET_NODE(stmt))->AddVover(reaching_defs[virtual_ssa_definition.first][current]);
                  }
                  reaching_defs[virtual_ssa_definition.first][current] = virtual_ssa_definition.first;
               }
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Reaching definition at the exit is " + STR(reaching_defs[virtual_ssa_definition.first][current]));

         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Checking successors");
         OutEdgeIterator oe, oe_end;
         for(boost::tie(oe, oe_end) = boost::out_edges(current, *basic_block_graph); oe != oe_end; oe++)
         {
            const auto target = boost::target(*oe, *basic_block_graph);
            if(loop_basic_blocks.find(target) != loop_basic_blocks.end())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Considering BB" + STR(basic_block_graph->CGetBBNodeInfo(target)->block->number));
               if((basic_block_graph->GetSelector(*oe) & FB_CFG_SELECTOR) != 0)
               {
                  if(phi_headers.find(target) != phi_headers.end())
                  {
                     THROW_ASSERT(added_phis.find(virtual_ssa_definition.first) != added_phis.end() and added_phis.find(virtual_ssa_definition.first)->second.find(target) != added_phis.find(virtual_ssa_definition.first)->second.end(),
                                  "Phi for " + STR(virtual_ssa_definition.first) + " was not created in BB" + STR(basic_block_graph->CGetBBNodeInfo(target)->block->number));
                     GetPointer<gimple_phi>(GET_NODE(added_phis.find(virtual_ssa_definition.first)->second.find(target)->second))
                         ->AddDefEdge(TM, gimple_phi::DefEdge(reaching_defs[virtual_ssa_definition.first][current], basic_block_graph->CGetBBNodeInfo(current)->block->number));
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Updated phi " + STR(added_phis.find(virtual_ssa_definition.first)->second.find(target)->second));
                  }
                  else if(not loops->CGetLoop(basic_block_graph->CGetBBNodeInfo(target)->loop_id)->IsReducible())
                  {
                     THROW_ASSERT(added_phis.find(virtual_ssa_definition.first) != added_phis.end() and added_phis.find(virtual_ssa_definition.first)->second.find(target) != added_phis.find(virtual_ssa_definition.first)->second.end(),
                                  "Phi for " + STR(virtual_ssa_definition.first) + " was not created in BB" + STR(basic_block_graph->CGetBBNodeInfo(target)->block->number));
                     GetPointer<gimple_phi>(GET_NODE(added_phis.find(virtual_ssa_definition.first)->second.find(target)->second))
                         ->AddDefEdge(TM, gimple_phi::DefEdge(reaching_defs[virtual_ssa_definition.first][current], basic_block_graph->CGetBBNodeInfo(current)->block->number));
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Updated phi " + STR(added_phis.find(virtual_ssa_definition.first)->second.find(target)->second));
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
                           THROW_ASSERT(reaching_defs.find(virtual_ssa_definition.first) != reaching_defs.end() and reaching_defs.find(virtual_ssa_definition.first)->second.find(source) != reaching_defs.find(virtual_ssa_definition.first)->second.end(),
                                        "Definition coming from BB" + STR(basic_block_graph->CGetBBNodeInfo(source)->block->number));
                           local_reaching_defs.insert(reaching_defs[virtual_ssa_definition.first][source]);
                        }
                     }
                     if(local_reaching_defs.size() > 1)
                     {
                        THROW_ASSERT(added_phis.find(virtual_ssa_definition.first) != added_phis.end() and added_phis.find(virtual_ssa_definition.first)->second.find(target) != added_phis.find(virtual_ssa_definition.first)->second.end(),
                                     "Phi for " + STR(virtual_ssa_definition.first) + " was not created in BB" + STR(basic_block_graph->CGetBBNodeInfo(target)->block->number));
                        GetPointer<gimple_phi>(GET_NODE(added_phis.find(virtual_ssa_definition.first)->second.find(target)->second))
                            ->AddDefEdge(TM, gimple_phi::DefEdge(reaching_defs[virtual_ssa_definition.first][current], basic_block_graph->CGetBBNodeInfo(current)->block->number));
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Updated phi " + STR(added_phis.find(virtual_ssa_definition.first)->second.find(target)->second));
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
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Checked BB" + STR(basic_block_graph->CGetBBNodeInfo(current)->block->number));
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered ssa " + virtual_ssa_definition.first->ToString());
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Checked uses");
   function_behavior->UpdateBBVersion();
   if(parameters->getOption<bool>(OPT_print_dot))
   {
      function_behavior->GetBBGraph(FunctionBehavior::FBB)->WriteDot("BB_FCFG.dot");
   }
#ifndef NDEBUG
   if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
   {
      for(boost::tie(basic_block, basic_block_end) = boost::vertices(*basic_block_graph); basic_block != basic_block_end; basic_block++)
      {
         auto block = basic_block_graph->CGetBBNodeInfo(*basic_block)->block;
         auto& phi_list = block->CGetPhiList();
         for(const auto& phi : phi_list)
         {
            const auto gp = GetPointer<const gimple_phi>(GET_NODE(phi));
            if(gp->virtual_flag)
            {
               THROW_ASSERT(gp->CGetDefEdgesList().size() == boost::in_degree(*basic_block, *basic_block_graph),
                            STR(phi) + " of BB" + STR(block->number) + " has wrong number of inputs: " + STR(gp->CGetDefEdgesList().size()) + " vs " + STR(boost::in_degree(*basic_block, *basic_block_graph)));
            }
         }
      }
   }
#endif
   return DesignFlowStep_Status::SUCCESS;
}

void BuildVirtualPhi::Initialize()
{
   FunctionFrontendFlowStep::Initialize();
   basic_block_graph = function_behavior->GetBBGraph(FunctionBehavior::FBB);
   tree_man = tree_manipulationRef(new tree_manipulation(TM, parameters));
}

bool BuildVirtualPhi::HasToBeExecuted() const
{
   return bb_version == 0;
}
