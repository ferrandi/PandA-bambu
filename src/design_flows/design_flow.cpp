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
 * @file design_flow.hpp
 * @brief This class contains the base representation for design flow
 *
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#include "design_flow.hpp"

#include "design_flow_manager.hpp"
#include "exceptions.hpp"
#include "string_manipulation.hpp"

DesignFlow::DesignFlow(const DesignFlowManager& _design_flow_manager, DesignFlow_Type _design_flow_type,
                       const ParameterConstRef _parameters)
    : DesignFlowStep(ComputeSignature(_design_flow_type), _design_flow_manager, _parameters),
      design_flow_type(_design_flow_type)
{
}

void DesignFlow::ComputeRelationships(DesignFlowStepSet&, const DesignFlowStep::RelationshipType)
{
}

std::string DesignFlow::GetName() const
{
   return "DF::" + EnumToKindText(design_flow_type);
}

DesignFlowStepFactoryConstRef DesignFlow::CGetDesignFlowStepFactory() const
{
   return design_flow_manager.CGetDesignFlowStepFactory(DesignFlowStep::DESIGN_FLOW);
}

bool DesignFlow::HasToBeExecuted() const
{
   return true;
}

std::string DesignFlow::EnumToKindText(const DesignFlow_Type design_flow_type)
{
   switch(design_flow_type)
   {
      case DesignFlow_Type::NON_DETERMINISTIC_FLOWS:
         return "NonDeterministicFlows";
      default:
         THROW_UNREACHABLE("");
   }
   THROW_UNREACHABLE("");
   return "";
}

DesignFlow_Type DesignFlow::KindTextToEnum(const std::string& name)
{
   if(name == "NonDeterministicFlows")
   {
      return DesignFlow_Type::NON_DETERMINISTIC_FLOWS;
   }
   else
   {
      THROW_UNREACHABLE("Unknown design flow: " + name);
      return DesignFlow_Type::NON_DETERMINISTIC_FLOWS;
   }
}

DesignFlowStep::signature_t DesignFlow::ComputeSignature(DesignFlow_Type design_flow_type)
{
   return DesignFlowStep::ComputeSignature(DESIGN_FLOW, static_cast<unsigned short>(design_flow_type), 0);
}
