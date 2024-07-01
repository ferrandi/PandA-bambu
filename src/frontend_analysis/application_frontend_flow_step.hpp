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
 * @file application_frontend_flow_step.hpp
 * @brief This class contains the base representation for a generic frontend flow step which works on the whole function
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
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
                               const DesignFlowManagerConstRef design_flow_manager, const ParameterConstRef parameters);

 public:
   /**
    * Constructor
    * @param AppM is the application manager
    * @param frontend_flow_step_type is the type of the step
    * @param design_flow_manager is the design flow manager
    * @param _Param is the set of the parameters
    */
   ApplicationFrontendFlowStep(const application_managerRef AppM, const FrontendFlowStepType frontend_flow_step_type,
                               const DesignFlowManagerConstRef design_flow_manager, const ParameterConstRef parameters);

   ~ApplicationFrontendFlowStep() override;

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
