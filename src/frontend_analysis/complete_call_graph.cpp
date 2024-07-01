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
 * @file complete_call_graph.cpp
 * @brief This class models the ending of execution of all functions which can add a function to call graph
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#include "complete_call_graph.hpp"

#include "config_HAVE_FROM_PRAGMA_BUILT.hpp" // for HAVE_FROM_PRAGMA_BUILT

#include "Parameter.hpp"           // for Parameter, OPT_hls_div
#include "exceptions.hpp"          // for THROW_UNREACHABLE
#include "hash_helper.hpp"         // for hash
#include "string_manipulation.hpp" // for GET_CLASS
#include <string>                  // for string, operator!=, bas...

CompleteCallGraph::CompleteCallGraph(const application_managerRef _AppM,
                                     const DesignFlowManagerConstRef _design_flow_manager,
                                     const ParameterConstRef _parameters)
    : ApplicationFrontendFlowStep(_AppM, COMPLETE_CALL_GRAPH, _design_flow_manager, _parameters)
{
   composed = true;
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

CompleteCallGraph::~CompleteCallGraph() = default;

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
CompleteCallGraph::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(CALL_GRAPH_BUILTIN_CALL, ALL_FUNCTIONS));
         relationships.insert(std::make_pair(COMPUTE_IMPLICIT_CALLS, ALL_FUNCTIONS));
         relationships.insert(std::make_pair(DATAFLOW_CG_EXT, ALL_FUNCTIONS));
         relationships.insert(std::make_pair(FUNCTION_ANALYSIS, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(SOFT_INT_CG_EXT, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(MULT_EXPR_FRACTURING, WHOLE_APPLICATION));
         if(parameters->isOption(OPT_soft_float) && parameters->getOption<bool>(OPT_soft_float))
         {
            relationships.insert(std::make_pair(SOFT_FLOAT_CG_EXT, ALL_FUNCTIONS));
         }
         relationships.insert(std::make_pair(TREE2FUN, ALL_FUNCTIONS));
         relationships.insert(std::make_pair(UN_COMPARISON_LOWERING, ALL_FUNCTIONS));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      default:
         THROW_UNREACHABLE("Relationship type does not exist");
   }
   return relationships;
}

DesignFlowStep_Status CompleteCallGraph::Exec()
{
   return DesignFlowStep_Status::EMPTY;
}

bool CompleteCallGraph::HasToBeExecuted() const
{
   return true;
}
