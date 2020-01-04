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
 * @file complete_bb_graph.cpp
 * @brief This class models the ending of execution of all steps which can modify control flow graph of basic blocks
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#include "complete_bb_graph.hpp"

#include "config_HAVE_BAMBU_BUILT.hpp"  // for HAVE_BAMBU_BUILT
#include "config_HAVE_EXPERIMENTAL.hpp" // for HAVE_EXPERIMENTAL
#include "config_HAVE_ZEBU_BUILT.hpp"   // for HAVE_ZEBU_BUILT

#include "Parameter.hpp"           // for Parameter, ParameterConstRef
#include "exceptions.hpp"          // for THROW_UNREACHABLE
#include "hash_helper.hpp"         // for hash
#include "string_manipulation.hpp" // for GET_CLASS

CompleteBBGraph::CompleteBBGraph(const application_managerRef _AppM, const unsigned int _function_index, const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters)
    : FunctionFrontendFlowStep(_AppM, _function_index, COMPLETE_BB_GRAPH, _design_flow_manager, _parameters)
{
   composed = true;
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

CompleteBBGraph::~CompleteBBGraph() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> CompleteBBGraph::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>(BLOCK_FIX, SAME_FUNCTION));
#if HAVE_ZEBU_BUILT
         relationships.insert(std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>(HEADER_STRUCTURING, SAME_FUNCTION));
#endif
#if HAVE_ZEBU_BUILT && HAVE_EXPERIMENTAL
         relationships.insert(std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>(PARALLEL_LOOP_SWAP, SAME_FUNCTION));
#endif
#if HAVE_BAMBU_BUILT
         relationships.insert(std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>(PHI_OPT, SAME_FUNCTION));
#endif
#if HAVE_ZEBU_BUILT
         relationships.insert(std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>(SHORT_CIRCUIT_STRUCTURING, SAME_FUNCTION));
#endif
#if HAVE_BAMBU_BUILT
         relationships.insert(std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>(SHORT_CIRCUIT_TAF, SAME_FUNCTION));
#endif
         relationships.insert(std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>(SWITCH_FIX, SAME_FUNCTION));
#if HAVE_BAMBU_BUILT
         relationships.insert(std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>(SPLIT_RETURN, SAME_FUNCTION));
#endif
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
#if HAVE_BAMBU_BUILT
         relationships.insert(std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>(SDC_CODE_MOTION, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>(MULTI_WAY_IF, SAME_FUNCTION));
#endif
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
