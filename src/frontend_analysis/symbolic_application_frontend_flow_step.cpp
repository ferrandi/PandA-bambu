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
 * @file symbolic_application_frontend_flow_step.cpp
 * @brief This class models the application of a analysis to all the functions of an application
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "symbolic_application_frontend_flow_step.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "exceptions.hpp"
#include "function_frontend_flow_step.hpp"
#include "hash_helper.hpp"
#include "string_manipulation.hpp"

#include <iostream>

SymbolicApplicationFrontendFlowStep::SymbolicApplicationFrontendFlowStep(
    const application_managerRef _AppM, const FrontendFlowStepType _represented_frontend_flow_step,
    const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters)
    : ApplicationFrontendFlowStep(ComputeSignature(_represented_frontend_flow_step), _AppM,
                                  SYMBOLIC_APPLICATION_FRONTEND_FLOW_STEP, _design_flow_manager, _parameters),
      represented_frontend_flow_step_type(_represented_frontend_flow_step)
{
   composed = true;
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

SymbolicApplicationFrontendFlowStep::~SymbolicApplicationFrontendFlowStep() = default;

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
SymbolicApplicationFrontendFlowStep::ComputeFrontendRelationships(
    const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(represented_frontend_flow_step_type, ALL_FUNCTIONS));
         relationships.insert(std::make_pair(FUNCTION_ANALYSIS, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(COMPLETE_CALL_GRAPH, WHOLE_APPLICATION));
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

DesignFlowStep_Status SymbolicApplicationFrontendFlowStep::Exec()
{
   return DesignFlowStep_Status::EMPTY;
}

std::string SymbolicApplicationFrontendFlowStep::GetKindText() const
{
   return "SymbolicApplicationFrontendFlowStep(" + EnumToKindText(represented_frontend_flow_step_type) + ")";
}

DesignFlowStep::signature_t
SymbolicApplicationFrontendFlowStep::ComputeSignature(const FrontendFlowStepType represented_frontend_flow_step_type)
{
   return DesignFlowStep::ComputeSignature(SYMBOLIC_APPLICATION_FRONTEND, SYMBOLIC_APPLICATION_FRONTEND_FLOW_STEP,
                                           represented_frontend_flow_step_type);
}

bool SymbolicApplicationFrontendFlowStep::HasToBeExecuted() const
{
   return true;
}
