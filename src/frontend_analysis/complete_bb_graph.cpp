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
 * @file complete_bb_graph.cpp
 * @brief This class models the ending of execution of all steps which can modify control flow graph of basic blocks
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#include "complete_bb_graph.hpp"

#include "Parameter.hpp"           // for Parameter, ParameterConstRef
#include "exceptions.hpp"          // for THROW_UNREACHABLE
#include "hash_helper.hpp"         // for hash
#include "string_manipulation.hpp" // for GET_CLASS

CompleteBBGraph::CompleteBBGraph(const application_managerRef _AppM, const unsigned int _function_index,
                                 const DesignFlowManagerConstRef _design_flow_manager,
                                 const ParameterConstRef _parameters)
    : FunctionFrontendFlowStep(_AppM, _function_index, COMPLETE_BB_GRAPH, _design_flow_manager, _parameters)
{
   composed = true;
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

CompleteBBGraph::~CompleteBBGraph() = default;

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
CompleteBBGraph::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(BLOCK_FIX, SAME_FUNCTION));
         relationships.insert(std::make_pair(MULTI_WAY_IF, SAME_FUNCTION));
         relationships.insert(std::make_pair(PHI_OPT, SAME_FUNCTION));
         relationships.insert(std::make_pair(REMOVE_ENDING_IF, SAME_FUNCTION));
         relationships.insert(std::make_pair(SHORT_CIRCUIT_TAF, SAME_FUNCTION));
         relationships.insert(std::make_pair(SWITCH_FIX, SAME_FUNCTION));
         relationships.insert(std::make_pair(SPLIT_RETURN, SAME_FUNCTION));
         if(!parameters->getOption<int>(OPT_gcc_openmp_simd))
         {
            relationships.insert(std::make_pair(BITVALUE_RANGE, SAME_FUNCTION));
            relationships.insert(std::make_pair(BITVALUE_RANGE, CALLED_FUNCTIONS));
         }
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(SDC_CODE_MOTION, SAME_FUNCTION));
         relationships.insert(std::make_pair(DEAD_CODE_ELIMINATION, SAME_FUNCTION));
         relationships.insert(std::make_pair(DEAD_CODE_ELIMINATION, CALLED_FUNCTIONS));
         relationships.insert(std::make_pair(LUT_TRANSFORMATION, SAME_FUNCTION));
         relationships.insert(std::make_pair(CSE_STEP, SAME_FUNCTION));
         relationships.insert(std::make_pair(DETERMINE_MEMORY_ACCESSES, SAME_FUNCTION));
         break;
      }
      default:
         THROW_UNREACHABLE("Relationship type does not exist");
   }
   return relationships;
}

DesignFlowStep_Status CompleteBBGraph::InternalExec()
{
   return DesignFlowStep_Status::EMPTY;
}
