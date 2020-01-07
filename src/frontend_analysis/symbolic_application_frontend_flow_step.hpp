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
 * @file symbolic_application_frontend_flow_step.hpp
 * @brief This class models the application of a analysis to all the functions of an application
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef SYMBOLIC_APPLICATION_FRONTEND_FLOW_STEP_HPP
#define SYMBOLIC_APPLICATION_FRONTEND_FLOW_STEP_HPP

#include "application_frontend_flow_step.hpp" // for ApplicationFrontendFlo...
#include "custom_set.hpp"                     // for unordered_set
#include "design_flow_step.hpp"               // for DesignFlowManagerConstRef
#include "frontend_flow_step.hpp"             // for FrontendFlowStepType
#include <string>                             // for string
#include <utility>                            // for pair

class SymbolicApplicationFrontendFlowStep : public ApplicationFrontendFlowStep
{
 private:
   /// The analysis represented by this step
   const FrontendFlowStepType represented_frontend_flow_step_type;

   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   const CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor
    * @param AppM is the application manager
    * @param frontend_flow_step_type is the type of the step
    * @param design_flow_manager is the design flow manager
    * @param _Param is the set of the parameters
    */
   SymbolicApplicationFrontendFlowStep(const application_managerRef AppM, const FrontendFlowStepType frontend_flow_step_type, const DesignFlowManagerConstRef design_flow_manager, const ParameterConstRef parameters);

   /**
    * Destructor
    */
   ~SymbolicApplicationFrontendFlowStep() override;

   /**
    * Execute this step
    * @return the exit status of this step
    */
   DesignFlowStep_Status Exec() override;

   /**
    * Return the type of this step
    */
   const std::string GetKindText() const override;

   /**
    * Return the signature of this step
    */
   const std::string GetSignature() const override;

   /**
    * Compute the signature of a symbolic application frontend flow step
    * @param frontend_flow_step_type is the type of frontend flow
    * @return the corresponding signature
    */
   static const std::string ComputeSignature(const FrontendFlowStepType frontend_flow_step_type);

   /**
    * Check if this step has actually to be executed
    * @return true if the step has to be executed
    */
   bool HasToBeExecuted() const override;
};
#endif
