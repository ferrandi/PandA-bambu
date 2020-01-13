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
 * @file design_flow_factory.cpp
 * @brief Factory for creating design flows
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#include "design_flow_factory.hpp"
#include "design_flow.hpp"             // for DesignFlow_Type, DesignFlow
#include "design_flow_step.hpp"        // for DesignFlowStepRef, DesignFlo...
#include "exceptions.hpp"              // for THROW_UNREACHABLE, THROW_ASSERT
#include "non_deterministic_flows.hpp" // for NonDeterministicFlows

DesignFlowFactory::DesignFlowFactory(const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters) : DesignFlowStepFactory(_design_flow_manager, _parameters)
{
}

DesignFlowFactory::~DesignFlowFactory() = default;

const std::string DesignFlowFactory::GetPrefix() const
{
   return "DF";
}

DesignFlowStepRef DesignFlowFactory::CreateFlowStep(const std::string& signature) const
{
   THROW_ASSERT(signature.substr(0, GetPrefix().size() + 2) == GetPrefix() + "::", signature);
   const auto design_flow_type = DesignFlow::KindTextToEnum(signature.substr(4));
   return CreateDesignFlow(design_flow_type);
}

const DesignFlowStepRef DesignFlowFactory::CreateDesignFlow(const DesignFlow_Type design_flow_type) const
{
   switch(design_flow_type)
   {
      case DesignFlow_Type::NON_DETERMINISTIC_FLOWS:
      {
         return DesignFlowStepRef(new NonDeterministicFlows(design_flow_manager.lock(), parameters));
      }

      default:
         THROW_UNREACHABLE("");
   }
   THROW_UNREACHABLE("");
   return DesignFlowStepRef();
}
