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
 * @file memory_data_flow_analysis.hpp
 * @brief Data flow analysis based on sequential chain of virtual memory accesses computed by gcc
 * It is based on the memuse and memdef of the gimple
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */

/// Header include
#include "memory_data_flow_analysis.hpp"

///. include
#include "Parameter.hpp"

/// behavior include
#include "function_behavior.hpp"
#include "op_graph.hpp"
#include "operations_graph_constructor.hpp"

/// utility include
#include "hash_helper.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

MemoryDataFlowAnalysis::MemoryDataFlowAnalysis(const application_managerRef _AppM, const unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters)
    : DataDependenceComputation(_AppM, _function_id, MEMORY_DATA_FLOW_ANALYSIS, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

MemoryDataFlowAnalysis::~MemoryDataFlowAnalysis() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> MemoryDataFlowAnalysis::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(OP_REACHABILITY_COMPUTATION, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(VAR_ANALYSIS, SAME_FUNCTION));
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

void MemoryDataFlowAnalysis::Initialize()
{
   if(bb_version != 0 and bb_version != function_behavior->GetBBVersion())
   {
      const OpGraphConstRef fsaodg = function_behavior->CGetOpGraph(FunctionBehavior::FSAODG);
      if(boost::num_vertices(*fsaodg) != 0)
      {
         EdgeIterator edge, edge_end;
         for(boost::tie(edge, edge_end) = boost::edges(*fsaodg); edge != edge_end; edge++)
         {
            function_behavior->ogc->RemoveSelector(*edge, DFG_AGG_SELECTOR | FB_DFG_AGG_SELECTOR | ADG_AGG_SELECTOR | FB_ADG_AGG_SELECTOR | ODG_AGG_SELECTOR | FB_ODG_AGG_SELECTOR);
         }
      }
   }
}
