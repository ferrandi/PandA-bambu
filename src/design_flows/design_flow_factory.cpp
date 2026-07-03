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
 * @file design_flow_factory.cpp
 * @brief Factory for creating design flows
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#include "design_flow_factory.hpp"

#include "design_flow.hpp"
#include "design_flow_step.hpp"
#include "exceptions.hpp"
#include "non_deterministic_flows.hpp"

DesignFlowFactory::DesignFlowFactory(const DesignFlowManager& _design_flow_manager, const ParameterConstRef _parameters)
    : DesignFlowStepFactory(DesignFlowStep::DESIGN_FLOW, _design_flow_manager, _parameters)
{
}

DesignFlowStepRef DesignFlowFactory::CreateFlowStep(DesignFlowStep::signature_t signature) const
{
   THROW_ASSERT(DesignFlowStep::GetStepClass(signature) == GetClass(), "Wrong step class");
   const auto design_flow_type = static_cast<DesignFlow_Type>(DesignFlowStep::GetStepType(signature));
   return CreateDesignFlow(design_flow_type);
}

DesignFlowStepRef DesignFlowFactory::CreateDesignFlow(const DesignFlow_Type design_flow_type) const
{
   switch(design_flow_type)
   {
      case DesignFlow_Type::NON_DETERMINISTIC_FLOWS:
      {
         return DesignFlowStepRef(new NonDeterministicFlows(design_flow_manager, parameters));
      }

      default:
         THROW_UNREACHABLE("");
   }
   THROW_UNREACHABLE("");
   return DesignFlowStepRef();
}
