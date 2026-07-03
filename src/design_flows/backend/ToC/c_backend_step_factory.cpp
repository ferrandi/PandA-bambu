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
 * @file c_backend_step_factory.cpp
 * @brief Factory class to create c backend
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#include "c_backend_step_factory.hpp"

#include "Parameter.hpp"
#include "c_backend.hpp"
#include "string_manipulation.hpp"

CBackendStepFactory::CBackendStepFactory(const DesignFlowManager& _design_flow_manager,
                                         const application_managerConstRef _application_manager,
                                         const ParameterConstRef _parameters)
    : DesignFlowStepFactory(DesignFlowStep::C_BACKEND, _design_flow_manager, _parameters),
      application_manager(_application_manager)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

DesignFlowStepRef CBackendStepFactory::CreateCBackendStep(const CBackendInformationConstRef c_backend_information) const
{
   return DesignFlowStepRef(new CBackend(c_backend_information, design_flow_manager, application_manager, parameters));
}
