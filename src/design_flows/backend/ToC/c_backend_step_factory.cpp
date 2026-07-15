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
