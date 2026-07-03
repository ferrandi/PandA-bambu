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
 * @file virtual_aggregate_data_flow_analysis.cpp
 * @brief Analysis step performing aggregate variable computation on the basis of IR virtual operands
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#include "virtual_aggregate_data_flow_analysis.hpp"

#include "Parameter.hpp"
#include "behavioral_helper.hpp"
#include "function_behavior.hpp"
#include "hash_helper.hpp"
#include "op_graph.hpp"
#include "string_manipulation.hpp"

VirtualAggregateDataFlowAnalysis::VirtualAggregateDataFlowAnalysis(const application_managerRef _AppM,
                                                                   const DesignFlowManager& _design_flow_manager,
                                                                   const unsigned int _function_index,
                                                                   const ParameterConstRef _parameters)
    : DataDependenceComputation(_AppM, _function_index, VIRTUAL_AGGREGATE_DATA_FLOW_ANALYSIS, _design_flow_manager,
                                _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
VirtualAggregateDataFlowAnalysis::ComputeFrontendRelationships(
    const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(BUILD_VIRTUAL_PHI, SAME_FUNCTION));
         relationships.insert(std::make_pair(OP_ORDER_COMPUTATION, SAME_FUNCTION));
         relationships.insert(std::make_pair(VAR_ANALYSIS, SAME_FUNCTION));
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
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

void VirtualAggregateDataFlowAnalysis::Initialize()
{
   if(bb_version != 0 && bb_version != function_behavior->GetBBVersion())
   {
      const auto fsaodg = function_behavior->GetOpGraph(FunctionBehavior::FSAODG);
      if(fsaodg.num_vertices() != 0)
      {
         for(const auto& edge : fsaodg.edges())
         {
            function_behavior->ogc->RemoveSelector(edge, DFG_AGG_SELECTOR | FB_DFG_AGG_SELECTOR | ADG_AGG_SELECTOR |
                                                             FB_ADG_AGG_SELECTOR | ODG_AGG_SELECTOR |
                                                             FB_ODG_AGG_SELECTOR);
         }
      }
   }
}
