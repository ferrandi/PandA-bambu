/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************            
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2004-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file symbolic_application_frontend_flow_step.cpp
 * @brief This class models the application of a analysis to all the functions of an application
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
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
    const DesignFlowManager& _design_flow_manager, const ParameterConstRef _parameters)
    : ApplicationFrontendFlowStep(ComputeSignature(_represented_frontend_flow_step), _AppM,
                                  SYMBOLIC_APPLICATION_FRONTEND_FLOW_STEP, _design_flow_manager, _parameters),
      represented_frontend_flow_step_type(_represented_frontend_flow_step)
{
   composed = true;
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

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
