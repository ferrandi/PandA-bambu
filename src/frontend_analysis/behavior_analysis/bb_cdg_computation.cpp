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
 * @file bb_cdg_computation.cpp
 * @brief Analysis step performing basic block control dependence computation.
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#include "bb_cdg_computation.hpp"

#include "Parameter.hpp"
#include "SemiNCADominance.hpp"
#include "basic_block.hpp"
#include "basic_blocks_graph_constructor.hpp"
#include "function_behavior.hpp"
#include "hash_helper.hpp"
#include "op_graph.hpp"

BBCdgComputation::BBCdgComputation(const ParameterConstRef _Param, const application_managerRef _AppM,
                                   unsigned int _function_id, const DesignFlowManager& _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, BB_CONTROL_DEPENDENCE_COMPUTATION, _design_flow_manager, _Param)
{
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
BBCdgComputation::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(BB_FEEDBACK_EDGES_IDENTIFICATION, SAME_FUNCTION));
         relationships.insert(std::make_pair(DOM_POST_DOM_COMPUTATION, SAME_FUNCTION));
         relationships.insert(std::make_pair(BB_ORDER_COMPUTATION, SAME_FUNCTION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
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

void BBCdgComputation::Initialize()
{
   if(bb_version != 0 and bb_version != function_behavior->GetBBVersion())
   {
      const auto bb_cdg = function_behavior->GetBBGraph(FunctionBehavior::CDG_BB);
      if(bb_cdg.num_vertices() != 0)
      {
         for(const auto& e : bb_cdg.edges())
         {
            function_behavior->bbgc->RemoveEdge(e, CDG_SELECTOR);
         }
      }
   }
}

DesignFlowStep_Status BBCdgComputation::InternalExec()
{
   const auto bb = function_behavior->GetBBGraph(FunctionBehavior::BB);
   const auto& post_dominators = function_behavior->post_dominators;
   const auto helper = function_behavior->CGetBehavioralHelper();

   std::list<BBGraph::vertex_descriptor> bb_levels;
   bb.TopologicalSort(bb_levels);
   std::map<BBGraph::vertex_descriptor, unsigned int> bb_sorted;
   unsigned int counter = 0;
   for(auto& bb_level : bb_levels)
   {
      bb_sorted[bb_level] = ++counter;
   }
   // iterate over outgoing edges of the basic block CFG.
   for(const auto& ei : bb.edges())
   {
      const auto A = bb.source(ei);
      const auto B = bb.target(ei);
      auto current_node = B;
      while(current_node && current_node != A && current_node != post_dominators->getImmediateDominator(A))
      {
         if(bb_sorted[current_node] > bb_sorted[A])
         {
            function_behavior->bbgc->AddEdge(A, current_node, CDG_SELECTOR);
            const auto labels = bb.CGetEdgeInfo(ei).get_labels(CFG_SELECTOR);
            for(const auto& label : labels)
            {
               function_behavior->bbgc->add_bb_edge_info(A, current_node, CDG_SELECTOR, label);
            }
         }
         else
         {
            break;
         }
         current_node = post_dominators->getImmediateDominator(current_node);
      }
   }

   auto cdg_bb = function_behavior->GetBBGraph(FunctionBehavior::CDG_BB);

   // Counter used to enumerate different
   unsigned int cer_counter = 0;
   // Map control equivalent region codification to control equivalent index;
   // The codification is the set of pair predecessor-edge label in the cdg_computation
   std::map<CustomOrderedSet<std::pair<BBGraph::vertex_descriptor, CustomOrderedSet<unsigned int>>>, unsigned int>
       cdg_to_index;

   const auto topological_sorted_nodes = function_behavior->get_bb_levels();
   for(const auto& node : topological_sorted_nodes)
   {
      unsigned int cer_index = cer_counter;
      auto& bb_node_info = cdg_bb.GetNodeInfo(node);
      if(cdg_bb.in_degree(node) > 0)
      {
         // codification of this basic block
         CustomOrderedSet<std::pair<BBGraph::vertex_descriptor, CustomOrderedSet<unsigned int>>> this_cod;
         for(const auto& ei : cdg_bb.in_edges(node))
         {
            this_cod.emplace(std::make_pair<BBGraph::vertex_descriptor, CustomOrderedSet<unsigned int>>(
                cdg_bb.source(ei), cdg_bb.CGetEdgeInfo(ei).get_labels(CDG_SELECTOR)));
         }
         if(cdg_to_index.find(this_cod) == cdg_to_index.end())
         {
            cdg_to_index[this_cod] = cer_counter;
            cer_counter++;
         }
         else
         {
            cer_index = cdg_to_index[this_cod];
         }
         bb_node_info.cer = cer_index;
      }
   }

   if(parameters->getOption<bool>(OPT_print_dot))
   {
      cdg_bb.writeDot(function_behavior->GetDotPath() / "BB_CDG.dot");
   }
   return DesignFlowStep_Status::SUCCESS;
}
