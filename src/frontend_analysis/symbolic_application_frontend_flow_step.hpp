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
 * @file symbolic_application_frontend_flow_step.hpp
 * @brief This class models the application of a analysis to all the functions of an application
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */

#ifndef SYMBOLIC_APPLICATION_FRONTEND_FLOW_STEP_HPP
#define SYMBOLIC_APPLICATION_FRONTEND_FLOW_STEP_HPP
#include "application_frontend_flow_step.hpp"

#include "custom_set.hpp"
#include "design_flow_step.hpp"
#include "frontend_flow_step.hpp"

#include <string>
#include <utility>

class SymbolicApplicationFrontendFlowStep : public ApplicationFrontendFlowStep
{
 private:
   /// The analysis represented by this step
   const FrontendFlowStepType represented_frontend_flow_step_type;

   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor
    * @param AppM is the application manager
    * @param _represented_frontend_flow_step is the type of the step
    * @param design_flow_manager is the design flow manager
    * @param parameters is the set of the parameters
    */
   SymbolicApplicationFrontendFlowStep(const application_managerRef AppM,
                                       const FrontendFlowStepType _represented_frontend_flow_step,
                                       const DesignFlowManager& design_flow_manager,
                                       const ParameterConstRef parameters);

   DesignFlowStep_Status Exec() override;

   std::string GetKindText() const override;

   bool HasToBeExecuted() const override;

   /**
    * Compute the signature of a symbolic application frontend flow step
    * @param frontend_flow_step_type is the type of frontend flow
    * @return the corresponding signature
    */
   static signature_t ComputeSignature(const FrontendFlowStepType frontend_flow_step_type);
};
#endif
