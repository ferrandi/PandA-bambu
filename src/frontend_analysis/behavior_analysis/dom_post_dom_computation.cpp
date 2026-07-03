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
 * @file dom_post_dom_computation.cpp
 * @brief Analysis step performing dominators and post dominators computation.
 *
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#include "dom_post_dom_computation.hpp"

#include "Parameter.hpp"
#include "SemiNCADominance.hpp"
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "basic_blocks_graph_constructor.hpp"
#include "behavioral_helper.hpp"
#include "custom_map.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "function_behavior.hpp"
#include "graph.hpp"
#include "hash_helper.hpp"
#include "string_manipulation.hpp"

dom_post_dom_computation::dom_post_dom_computation(const ParameterConstRef _parameters,
                                                   const application_managerRef _AppM, unsigned int _function_id,
                                                   const DesignFlowManager& _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, DOM_POST_DOM_COMPUTATION, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
dom_post_dom_computation::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(BASIC_BLOCKS_CFG_COMPUTATION, SAME_FUNCTION));
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

void dom_post_dom_computation::Initialize()
{
   function_behavior->dominators = nullptr;
   function_behavior->post_dominators = nullptr;
}

DesignFlowStep_Status dom_post_dom_computation::InternalExec()
{
   const auto fbb = function_behavior->GetBBGraph(FunctionBehavior::FBB);

   const BehavioralHelperConstRef helper = function_behavior->CGetBehavioralHelper();
   /// dominators computation
   THROW_ASSERT(!function_behavior->dominators, "Dominators already built");
   const auto bbentry = fbb.CGetGraphInfo().entry_vertex;
   const auto bbexit = fbb.CGetGraphInfo().exit_vertex;
   function_behavior->dominators = std::make_unique<dominance<BBGraph>>(fbb, bbentry, bbexit);
   function_behavior->dominators->forEachDominanceRelation(
       [&](const BBGraph::vertex_descriptor child, const BBGraph::vertex_descriptor dom_vertex) {
          if(child != bbentry)
          {
             function_behavior->bbgc->AddEdge(dom_vertex, child, D_SELECTOR);
          }
       });
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Built dominators tree of " + helper->GetFunctionName());
   if(parameters->getOption<bool>(OPT_print_dot))
   {
      function_behavior->GetBBGraph(FunctionBehavior::DOM_TREE)
          .writeDot(function_behavior->GetDotPath() / "BB_dom_tree.dot");
   }
   /// post-dominators computation
   THROW_ASSERT(!function_behavior->post_dominators, "Post dominators yet built");
   function_behavior->post_dominators = std::make_unique<dominance<BBGraph, true>>(fbb, bbentry, bbexit);
   function_behavior->post_dominators->forEachDominanceRelation(
       [&](const BBGraph::vertex_descriptor child, const BBGraph::vertex_descriptor dom_vertex) {
          if(child != bbexit)
          {
             function_behavior->bbgc->AddEdge(dom_vertex, child, PD_SELECTOR);
          }
       });
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Built post-dominators tree of " + helper->GetFunctionName());

   if(parameters->getOption<bool>(OPT_print_dot))
   {
      function_behavior->GetBBGraph(FunctionBehavior::POST_DOM_TREE)
          .writeDot(function_behavior->GetDotPath() / "BB_post_dom_tree.dot");
   }
   return DesignFlowStep_Status::SUCCESS;
}
