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
 * @file dom_post_dom_computation.cpp
 * @brief Analysis step performing dominators and post dominators computation.
 *
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
/// Header include
#include "dom_post_dom_computation.hpp"

///. include
#include "Parameter.hpp"

/// algorithms/dominance include
#include "Dominance.hpp"

/// behavior includes
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "basic_blocks_graph_constructor.hpp"
#include "function_behavior.hpp"

/// design_flows includes
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"

/// graph include
#include "graph.hpp"

/// tree include
#include "behavioral_helper.hpp"

/// utility include
#include "custom_map.hpp"
#include "hash_helper.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

dom_post_dom_computation::dom_post_dom_computation(const ParameterConstRef _parameters,
                                                   const application_managerRef _AppM, unsigned int _function_id,
                                                   const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, DOM_POST_DOM_COMPUTATION, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

dom_post_dom_computation::~dom_post_dom_computation() = default;

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
   const BBGraphConstRef fbb = function_behavior->CGetBBGraph(FunctionBehavior::FBB);

   const BehavioralHelperConstRef helper = function_behavior->CGetBehavioralHelper();
   /// dominators computation
   THROW_ASSERT(!function_behavior->dominators, "Dominators already built");
   const vertex bbentry = fbb->CGetBBGraphInfo()->entry_vertex;
   const vertex bbexit = fbb->CGetBBGraphInfo()->exit_vertex;
   function_behavior->dominators = new dominance<BBGraph>(*fbb, bbentry, bbexit, parameters);
   function_behavior->dominators->calculate_dominance_info(dominance<BBGraph>::CDI_DOMINATORS);
   const auto& dominator_map = function_behavior->dominators->get_dominator_map();
   for(auto& it : dominator_map)
   {
      if(it.first != bbentry)
      {
         function_behavior->bbgc->AddEdge(it.second, it.first, D_SELECTOR);
      }
   }
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Built dominators tree of " + helper->get_function_name());
   if(parameters->getOption<bool>(OPT_print_dot))
   {
      function_behavior->GetBBGraph(FunctionBehavior::DOM_TREE)->WriteDot("BB_dom_tree.dot");
   }
   /// post-dominators computation
   THROW_ASSERT(!function_behavior->post_dominators, "Post dominators yet built");
   function_behavior->post_dominators = new dominance<BBGraph>(*fbb, bbentry, bbexit, parameters);
   function_behavior->post_dominators->calculate_dominance_info(dominance<BBGraph>::CDI_POST_DOMINATORS);
   const auto& post_dominator_map = function_behavior->post_dominators->get_dominator_map();
   for(auto& it : post_dominator_map)
   {
      if(it.first != bbexit)
      {
         function_behavior->bbgc->AddEdge(it.second, it.first, PD_SELECTOR);
      }
   }
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                 "Built post-dominators tree of " + helper->get_function_name());

   if(parameters->getOption<bool>(OPT_print_dot))
   {
      function_behavior->GetBBGraph(FunctionBehavior::POST_DOM_TREE)->WriteDot("BB_post_dom_tree.dot");
   }
   return DesignFlowStep_Status::SUCCESS;
}
