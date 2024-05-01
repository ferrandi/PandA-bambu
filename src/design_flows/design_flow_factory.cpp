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

DesignFlowFactory::DesignFlowFactory(const DesignFlowManagerConstRef _design_flow_manager,
                                     const ParameterConstRef _parameters)
    : DesignFlowStepFactory(DesignFlowStep::DESIGN_FLOW, _design_flow_manager, _parameters)
{
}

DesignFlowFactory::~DesignFlowFactory() = default;

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
         return DesignFlowStepRef(new NonDeterministicFlows(design_flow_manager.lock(), parameters));
      }

      default:
         THROW_UNREACHABLE("");
   }
   THROW_UNREACHABLE("");
   return DesignFlowStepRef();
}
