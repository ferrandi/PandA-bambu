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
 * @file frontend_flow_step_factory.hpp
 * @brief This class contains the methods to create a frontend flow step
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */

#ifndef FRONTEND_FLOW_STEP_FACTORY_HPP
#define FRONTEND_FLOW_STEP_FACTORY_HPP

#include "custom_set.hpp"
#include "design_flow_step.hpp"
#include "design_flow_step_factory.hpp"
#include "frontend_flow_step.hpp"
#include "refcount.hpp"

#include <string>

/// Forward declaration
REF_FORWARD_DECL(application_manager);
REF_FORWARD_DECL(ArchManager);
REF_FORWARD_DECL(DesignFlowManager);
REF_FORWARD_DECL(DesignFlowStep);

class FrontendFlowStepFactory : public DesignFlowStepFactory
{
 protected:
   /// The application manager
   const application_managerRef AppM;

 public:
   /**
    * Constructor
    * @param AppM is the application manager
    * @param design_flow_manager is the design flow manager
    * @param parameters is the set of input parameters
    */
   FrontendFlowStepFactory(const application_managerRef AppM, const DesignFlowManager& design_flow_manager,
                           const ParameterConstRef parameters);

   /**
    * Create the frontend design flow steps
    * @param frontend_flow_step_types is the set of frontend flow transformation to be considered
    */
   DesignFlowStepSet
   GenerateFrontendSteps(const CustomUnorderedSet<FrontendFlowStepType>& frontend_flow_step_types) const;

   /**
    * Create the frontend design flow step
    * @param frontend_flow_step_type is the frontend flow to be considered
    */
   DesignFlowStepRef GenerateFrontendStep(FrontendFlowStepType frontend_flow_step_type) const;

   /**
    * Create an application frontend flow step
    * @param design_flow_step_type is the type of step to be created
    */
   DesignFlowStepRef CreateApplicationFrontendFlowStep(const FrontendFlowStepType design_flow_step_type) const;

   /**
    * Create a function frontend flow step
    * @param design_flow_step_type is the type of step to be created
    * @param function_id is the index of the function
    */
   DesignFlowStepRef CreateFunctionFrontendFlowStep(const FrontendFlowStepType design_flow_step_type,
                                                    const unsigned int function_id) const;
};
#endif
