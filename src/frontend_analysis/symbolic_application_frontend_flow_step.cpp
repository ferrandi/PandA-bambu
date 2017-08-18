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
 *              Copyright (c) 2004-2017 Politecnico di Milano
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

///Autoheader include
#include "config_HAVE_BAMBU_BUILT.hpp"
#include "config_HAVE_ZEBU_BUILT.hpp"

///Header include
#include "symbolic_application_frontend_flow_step.hpp"

///Behavior include
#include "application_manager.hpp"
#include "call_graph_manager.hpp"

///Design flow include
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"

///Frontend flow include
#include "function_frontend_flow_step.hpp"

///Parameter include
#include "Parameter.hpp"

SymbolicApplicationFrontendFlowStep::SymbolicApplicationFrontendFlowStep(const application_managerRef _AppM, const FrontendFlowStepType _represented_frontend_flow_step,  const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters) :
   ApplicationFrontendFlowStep(_AppM, SYMBOLIC_APPLICATION_FRONTEND_FLOW_STEP, _design_flow_manager, _parameters),
   represented_frontend_flow_step_type(_represented_frontend_flow_step)
{
   composed = true;
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

SymbolicApplicationFrontendFlowStep::~SymbolicApplicationFrontendFlowStep()
{}

const std::unordered_set<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship> > SymbolicApplicationFrontendFlowStep::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   std::unordered_set<std::pair<FrontendFlowStepType, FunctionRelationship> > relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP) :
      {
         relationships.insert(std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>(represented_frontend_flow_step_type, ALL_FUNCTIONS));
         relationships.insert(std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>(FUNCTION_ANALYSIS, WHOLE_APPLICATION));
         relationships.insert(std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>(COMPLETE_CALL_GRAPH, WHOLE_APPLICATION));
#if HAVE_ZEBU_BUILT
         relationships.insert(std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>(FUNCTION_POINTER_CALLGRAPH_COMPUTATION, WHOLE_APPLICATION));
#endif
         break;
      }
      case(INVALIDATION_RELATIONSHIP) :
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

const std::string SymbolicApplicationFrontendFlowStep::GetKindText() const
{
   return "SymbolicApplicationFrontendFlowStep(" + EnumToKindText(represented_frontend_flow_step_type) + ")";
}

const std::string SymbolicApplicationFrontendFlowStep::ComputeSignature(const FrontendFlowStepType represented_frontend_flow_step_type)
{
   return "Frontend::" + boost::lexical_cast<std::string>(SYMBOLIC_APPLICATION_FRONTEND_FLOW_STEP) + "(" + boost::lexical_cast<std::string>(represented_frontend_flow_step_type)  + ")";
}

const std::string SymbolicApplicationFrontendFlowStep::GetSignature() const
{
   return ComputeSignature(represented_frontend_flow_step_type);
}

bool SymbolicApplicationFrontendFlowStep::HasToBeExecuted() const
{
   return true;
}
