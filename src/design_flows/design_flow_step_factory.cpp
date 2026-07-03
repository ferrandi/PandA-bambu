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
 *              Copyright (C) 2004-2026 Politecnico di Milano
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
