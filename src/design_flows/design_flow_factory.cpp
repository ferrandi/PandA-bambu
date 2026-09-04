/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2017-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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
