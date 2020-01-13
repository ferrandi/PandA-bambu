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
 *              Copyright (C) 2017-2020 Politecnico di Milano
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
#include "design_flow_manager.hpp" // for DesignFlowStepFactoryConstRef
#include "exceptions.hpp"          // for THROW_UNREACHABLE
#include "string_manipulation.hpp" // for STR

DesignFlow::DesignFlow(const DesignFlowManagerConstRef _design_flow_manager, const DesignFlow_Type _design_flow_type, const ParameterConstRef _parameters) : DesignFlowStep(_design_flow_manager, _parameters), design_flow_type(_design_flow_type)
{
}

DesignFlow::~DesignFlow() = default;

void DesignFlow::ComputeRelationships(DesignFlowStepSet&, const DesignFlowStep::RelationshipType)
{
}

const std::string DesignFlow::GetSignature() const
{
   return ComputeSignature(design_flow_type);
}

const std::string DesignFlow::GetName() const
{
   return "DF::" + EnumToKindText(design_flow_type);
}

const std::string DesignFlow::EnumToKindText(const DesignFlow_Type design_flow_type)
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

const DesignFlowStepFactoryConstRef DesignFlow::CGetDesignFlowStepFactory() const
{
   return design_flow_manager.lock()->CGetDesignFlowStepFactory("DF");
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

bool DesignFlow::HasToBeExecuted() const
{
   return true;
}

std::string DesignFlow::ComputeSignature(const DesignFlow_Type design_flow_type)
{
   return "DF::" + STR(static_cast<int>(design_flow_type));
}
