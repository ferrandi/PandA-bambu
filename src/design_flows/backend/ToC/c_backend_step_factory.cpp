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
 *              Copyright (C) 2004-2024 Politecnico di Milano
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
 * @file c_backend_step_factory.cpp
 * @brief Factory class to create c backend
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "c_backend_step_factory.hpp"

#include "Parameter.hpp"
#include "c_backend.hpp"
#include "string_manipulation.hpp"

CBackendStepFactory::CBackendStepFactory(const DesignFlowManagerConstRef _design_flow_manager,
                                         const application_managerConstRef _application_manager,
                                         const ParameterConstRef _parameters)
    : DesignFlowStepFactory(DesignFlowStep::C_BACKEND, _design_flow_manager, _parameters),
      application_manager(_application_manager)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

DesignFlowStepRef CBackendStepFactory::CreateCBackendStep(const CBackendInformationConstRef c_backend_information) const
{
   return DesignFlowStepRef(
       new CBackend(c_backend_information, design_flow_manager.lock(), application_manager, parameters));
}
