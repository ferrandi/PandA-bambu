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
 * @file complete_bb_graph.cpp
 * @brief This class models the ending of execution of all steps which can modify control flow graph of basic blocks
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#include "complete_bb_graph.hpp"

#include "Parameter.hpp"
#include "exceptions.hpp"
#include "hash_helper.hpp"
#include "string_manipulation.hpp"

CompleteBBGraph::CompleteBBGraph(const application_managerRef _AppM, const unsigned int _function_index,
                                 const DesignFlowManager& _design_flow_manager, const ParameterConstRef _parameters)
    : FunctionFrontendFlowStep(_AppM, _function_index, COMPLETE_BB_GRAPH, _design_flow_manager, _parameters)
{
   composed = true;
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

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
         relationships.insert(std::make_pair(BITVALUE_RANGE, SAME_FUNCTION));
         relationships.insert(std::make_pair(BITVALUE_RANGE, CALLED_FUNCTIONS));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(SDC_CODE_MOTION, SAME_FUNCTION));
         relationships.insert(std::make_pair(DCE_PASS, SAME_FUNCTION));
         relationships.insert(std::make_pair(DCE_PASS, CALLED_FUNCTIONS));
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
