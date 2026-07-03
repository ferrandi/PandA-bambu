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
 *              Copyright (C) 2017-2026 Politecnico di Milano
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
 * @file design_flow_factory.hpp
 * @brief Factory for creating design flows
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#ifndef DESIGN_FLOW_FACTORY_HPP
#define DESIGN_FLOW_FACTORY_HPP
#include "design_flow_step_factory.hpp"

#include <string>

enum class DesignFlow_Type;

class DesignFlowFactory : public DesignFlowStepFactory
{
 public:
   /**
    * Constructor
    * @param design_flow_manager is the design flow manager
    * @param parameters is the set of input parameters
    */
   DesignFlowFactory(const DesignFlowManager& design_flow_manager, const ParameterConstRef parameters);

   DesignFlowStepRef CreateFlowStep(DesignFlowStep::signature_t signature) const override;

   /**
    * Create a design flow
    * @param design_flow_type is the type of design flow to be created
    * @return the step corresponding to the design flow
    */
   DesignFlowStepRef CreateDesignFlow(const DesignFlow_Type design_flow_type) const;
};
#endif
