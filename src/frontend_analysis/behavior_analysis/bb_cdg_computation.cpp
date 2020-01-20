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
 * @file bb_cdg_computation.cpp
 * @brief Analysis step performing basic block control dependence computation.
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
/// Header include
#include "bb_cdg_computation.hpp"

///. include
#include "Parameter.hpp"

/// algorithms/dominance include
#include "Dominance.hpp"

/// behavior include
#include "basic_block.hpp"
#include "basic_blocks_graph_constructor.hpp"
#include "function_behavior.hpp"
#include "hash_helper.hpp"
#include "op_graph.hpp"

BBCdgComputation::BBCdgComputation(const ParameterConstRef _Param, const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, BB_CONTROL_DEPENDENCE_COMPUTATION, _design_flow_manager, _Param)
{
}

BBCdgComputation::~BBCdgComputation() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> BBCdgComputation::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(BB_FEEDBACK_EDGES_IDENTIFICATION, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(DOM_POST_DOM_COMPUTATION, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(BB_ORDER_COMPUTATION, SAME_FUNCTION));
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
      const BBGraphConstRef bb_cdg = function_behavior->CGetBBGraph(FunctionBehavior::CDG_BB);
      if(boost::num_vertices(*bb_cdg) != 0)
      {
         EdgeIterator edge, edge_end;
         for(boost::tie(edge, edge_end) = boost::edges(*bb_cdg); edge != edge_end; edge++)
         {
            function_behavior->bbgc->RemoveEdge(*edge, CDG_SELECTOR);
         }
      }
   }
}

DesignFlowStep_Status BBCdgComputation::InternalExec()
{
   const BBGraphRef bb = function_behavior->GetBBGraph(FunctionBehavior::BB);
   const dominance<BBGraph>* post_dominators = function_behavior->post_dominators;
   const BehavioralHelperConstRef helper = function_behavior->CGetBehavioralHelper();

   EdgeIterator ei, ei_end;
   std::list<vertex> bb_levels;
   boost::topological_sort(*bb, std::front_inserter(bb_levels));
   std::map<vertex, unsigned int> bb_sorted;
   unsigned int counter = 0;
   for(auto& bb_level : bb_levels)
      bb_sorted[bb_level] = ++counter;
   // iterate over outgoing edges of the basic block CFG.
   for(boost::tie(ei, ei_end) = boost::edges(*bb); ei != ei_end; ++ei)
   {
      vertex A = boost::source(*ei, *bb);
      vertex B = boost::target(*ei, *bb);
      InEdgeIterator pd_ei, pd_ei_end;
      vertex current_node = B;
      while(current_node && current_node != A && current_node != post_dominators->get_immediate_dominator(A))
      {
         if(bb_sorted[current_node] > bb_sorted[A])
         {
            function_behavior->bbgc->AddEdge(A, current_node, CDG_SELECTOR);
            const auto& labels = bb->CGetBBEdgeInfo(*ei)->get_labels(CFG_SELECTOR);
            if(labels.size())
            {
               auto it_end = labels.end();
               for(auto it = labels.begin(); it != it_end; ++it)
               {
                  function_behavior->bbgc->add_bb_edge_info(A, current_node, CDG_SELECTOR, *it);
               }
            }
         }
         else
         {
            break;
         }
         current_node = post_dominators->get_immediate_dominator(current_node);
      }
   }

   BBGraphRef cdg_bb = function_behavior->cdg_bb;

   // Counter used to enumerate different
   unsigned int cer_counter = 0;
   // Map control equivalent region codification to control equivalent index;
   // The codification is the set of pair predecessor-edge label in the cdg_computation
   std::map<CustomOrderedSet<std::pair<vertex, CustomOrderedSet<unsigned int>>>, unsigned int> cdg_to_index;

   const std::deque<vertex>& topological_sorted_nodes = function_behavior->get_bb_levels();
   std::deque<vertex>::const_iterator it, it_end;
   it_end = topological_sorted_nodes.end();
   for(it = topological_sorted_nodes.begin(); it != it_end; ++it)
   {
      unsigned int cer_index = cer_counter;
      const BBNodeInfoRef bb_node_info = cdg_bb->GetBBNodeInfo(*it);
      if(boost::in_degree(*it, *cdg_bb) > 0)
      {
         // codification of this basic block
         CustomOrderedSet<std::pair<vertex, CustomOrderedSet<unsigned int>>> this_cod;
         InEdgeIterator eii, eii_end;
         for(boost::tie(eii, eii_end) = boost::in_edges(*it, *cdg_bb); eii != eii_end; eii++)
         {
            this_cod.emplace(std::pair<vertex, CustomOrderedSet<unsigned int>>(boost::source(*eii, *cdg_bb), cdg_bb->CGetBBEdgeInfo(*eii)->get_labels(CDG_SELECTOR)));
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
         bb_node_info->cer = cer_index;
      }
   }

   if(parameters->getOption<bool>(OPT_print_dot))
   {
      function_behavior->CGetBBGraph(FunctionBehavior::CDG_BB)->WriteDot("BB_CDG.dot");
   }
   return DesignFlowStep_Status::SUCCESS;
}
