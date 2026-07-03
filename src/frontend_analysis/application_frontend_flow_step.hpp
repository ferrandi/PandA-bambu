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
 * @file application_frontend_flow_step.hpp
 * @brief This class contains the base representation for a generic frontend flow step which works on the whole function
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#ifndef APPLICATION_FRONTEND_FLOW_STEP_HPP
#define APPLICATION_FRONTEND_FLOW_STEP_HPP
#include "custom_set.hpp"
#include "design_flow_step.hpp"
#include "frontend_flow_step.hpp"

#include <string>
#include <utility>

class ApplicationFrontendFlowStep : public FrontendFlowStep
{
 protected:
   ApplicationFrontendFlowStep(signature_t signature, const application_managerRef AppM,
                               const FrontendFlowStepType frontend_flow_step_type,
                               const DesignFlowManager& design_flow_manager, const ParameterConstRef parameters);

 public:
   /**
    * Constructor
    * @param AppM is the application manager
    * @param frontend_flow_step_type is the type of the step
    * @param design_flow_manager is the design flow manager
    * @param parameters is the set of the parameters
    */
   ApplicationFrontendFlowStep(const application_managerRef AppM, const FrontendFlowStepType frontend_flow_step_type,
                               const DesignFlowManager& design_flow_manager, const ParameterConstRef parameters);

   virtual ~ApplicationFrontendFlowStep() override = default;

   DesignFlowStep_Status Exec() override = 0;

   virtual std::string GetName() const override;

   bool HasToBeExecuted() const override;

   /**
    * Compute the signature of a function frontend flow step
    * @param frontend_flow_step_type is the type of frontend flow
    * @return the corresponding signature
    */
   static signature_t ComputeSignature(const FrontendFlowStepType frontend_flow_step_type);
};
#endif
