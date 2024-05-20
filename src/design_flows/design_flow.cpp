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
 *              Copyright (C) 2017-2024 Politecnico di Milano
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

DesignFlow::DesignFlow(const DesignFlowManagerConstRef _design_flow_manager, DesignFlow_Type _design_flow_type,
                       const ParameterConstRef _parameters)
    : DesignFlowStep(ComputeSignature(_design_flow_type), _design_flow_manager, _parameters),
      design_flow_type(_design_flow_type)
{
}

DesignFlow::~DesignFlow() = default;

void DesignFlow::ComputeRelationships(DesignFlowStepSet&, const DesignFlowStep::RelationshipType)
{
}

std::string DesignFlow::GetName() const
{
   return "DF::" + EnumToKindText(design_flow_type);
}

DesignFlowStepFactoryConstRef DesignFlow::CGetDesignFlowStepFactory() const
{
   return design_flow_manager.lock()->CGetDesignFlowStepFactory(DesignFlowStep::DESIGN_FLOW);
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
