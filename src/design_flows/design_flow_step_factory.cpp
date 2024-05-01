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
 * @file design_flow_step_factory.cpp
 * @brief Pure virtual base class for all the design flow step factory
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "design_flow_step_factory.hpp"

#include "Parameter.hpp"
#include "exceptions.hpp"

DesignFlowStepFactory::DesignFlowStepFactory(DesignFlowStep::StepClass _step_class,
                                             const DesignFlowManagerConstRef& _design_flow_manager,
                                             const ParameterConstRef& _parameters)
    : design_flow_manager(_design_flow_manager),
      parameters(_parameters),
      debug_level(_parameters->getOption<int>(OPT_debug_level)),
      step_class(_step_class)
{
}

DesignFlowStepFactory::~DesignFlowStepFactory() = default;

DesignFlowStepRef DesignFlowStepFactory::CreateFlowStep(DesignFlowStep::signature_t) const
{
   THROW_UNREACHABLE("Not yet implemented");
   return DesignFlowStepRef();
}
