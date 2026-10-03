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
 *   Copyright (C) 2004-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file design_flow_step_factory.cpp
 * @brief Pure virtual base class for all the design flow step factory
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#include "design_flow_step_factory.hpp"

#include "Parameter.hpp"
#include "exceptions.hpp"

DesignFlowStepFactory::DesignFlowStepFactory(DesignFlowStep::StepClass _step_class,
                                             const DesignFlowManager& _design_flow_manager,
                                             const ParameterConstRef& _parameters)
    : design_flow_manager(_design_flow_manager),
      parameters(_parameters),
      debug_level(_parameters->getOption<int>(OPT_debug_level)),
      step_class(_step_class)
{
}

DesignFlowStepRef DesignFlowStepFactory::CreateFlowStep(DesignFlowStep::signature_t) const
{
   THROW_UNREACHABLE("Not yet implemented");
   return DesignFlowStepRef();
}
