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
 *              Copyright (C) 2004-2021 Politecnico di Milano
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
 * @file aggregate_data_flow_analysis.cpp
 * @brief Meta analysis step performing aggregate variable; dependence of this step is the actual step performing aggregate data flow analysis
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

/// Header include
#include "aggregate_data_flow_analysis.hpp"

/// Autoheader include
#include "config_HAVE_TECHNOLOGY_BUILT.hpp"

/// design_flows include
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"

#if HAVE_TECHNOLOGY_BUILT
/// design_flows/technology includes
#include "technology_flow_step.hpp"
#include "technology_flow_step_factory.hpp"
#endif

/// frontend_flow includes
#include "frontend_flow_step_factory.hpp"
#include "function_frontend_flow_step.hpp"

#include "hls_manager.hpp"
#include "hls_target.hpp"

#include "Parameter.hpp"
#include "hash_helper.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

AggregateDataFlowAnalysis::AggregateDataFlowAnalysis(const application_managerRef _AppM, const unsigned int _function_index, const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters)
    : FunctionFrontendFlowStep(_AppM, _function_index, AGGREGATE_DATA_FLOW_ANALYSIS, _design_flow_manager, _parameters)
{
   composed = true;
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

AggregateDataFlowAnalysis::~AggregateDataFlowAnalysis() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> AggregateDataFlowAnalysis::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
#if HAVE_ZEBU_BUILT && HAVE_EXPERIMENTAL
         if(parameters->getOption<bool>(OPT_memory_profiling))
         {
            relationships.insert(std::make_pair(DYNAMIC_AGGREGATE_DATA_FLOW_ANALYSIS, SAME_FUNCTION));
         }
         else
#endif
         {
#if HAVE_BAMBU_BUILT
            const auto technology_flow_signature = TechnologyFlowStep::ComputeSignature(TechnologyFlowStep_Type::LOAD_TECHNOLOGY);
            if(design_flow_manager.lock()->GetStatus(technology_flow_signature) == DesignFlowStep_Status::EMPTY)
            {
               if(GetPointer<const HLS_manager>(AppM) && GetPointer<const HLS_manager>(AppM)->IsSingleWriteMemory())
               {
                  relationships.insert(std::make_pair(MEMORY_DATA_FLOW_ANALYSIS, SAME_FUNCTION));
               }
               else
#endif
               {
                  relationships.insert(std::make_pair(VIRTUAL_AGGREGATE_DATA_FLOW_ANALYSIS, SAME_FUNCTION));
               }
#if HAVE_BAMBU_BUILT
            }
#endif
         }
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

void AggregateDataFlowAnalysis::ComputeRelationships(DesignFlowStepSet& relationship, const DesignFlowStep::RelationshipType relationship_type)
{
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
#if HAVE_TECHNOLOGY_BUILT
         const auto design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
         const auto technology_flow_step_factory = GetPointer<const TechnologyFlowStepFactory>(design_flow_manager.lock()->CGetDesignFlowStepFactory("Technology"));
         const auto technology_flow_signature = TechnologyFlowStep::ComputeSignature(TechnologyFlowStep_Type::LOAD_TECHNOLOGY);
         const auto technology_flow_step = design_flow_manager.lock()->GetDesignFlowStep(technology_flow_signature);
         const auto technology_design_flow_step = technology_flow_step ? design_flow_graph->CGetDesignFlowStepInfo(technology_flow_step)->design_flow_step : technology_flow_step_factory->CreateTechnologyFlowStep(TechnologyFlowStep_Type::LOAD_TECHNOLOGY);
         relationship.insert(technology_design_flow_step);
#endif
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      case INVALIDATION_RELATIONSHIP:
      {
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   FunctionFrontendFlowStep::ComputeRelationships(relationship, relationship_type);
}

DesignFlowStep_Status AggregateDataFlowAnalysis::InternalExec()
{
   return DesignFlowStep_Status::EMPTY;
}
