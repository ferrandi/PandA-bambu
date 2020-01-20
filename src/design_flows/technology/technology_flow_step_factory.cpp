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
 *              Copyright (C) 2015-2020 Politecnico di Milano
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
 * @file technology_flow_step_factory.cpp
 * @brief Factory for technology flow step
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Header include
#include "technology_flow_step_factory.hpp"

/// technology includes
#include "fix_characterization.hpp"
#include "load_builtin_technology.hpp"
#include "load_default_technology.hpp"
#include "load_device_technology.hpp"
#include "load_file_technology.hpp"
#include "load_technology.hpp"
#include "write_technology.hpp"

TechnologyFlowStepFactory::TechnologyFlowStepFactory(const technology_managerRef _TM, const target_deviceRef _target, const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters)
    : DesignFlowStepFactory(_design_flow_manager, _parameters), TM(_TM), target(_target)
{
}

TechnologyFlowStepFactory::~TechnologyFlowStepFactory() = default;

const std::string TechnologyFlowStepFactory::GetPrefix() const
{
   return "Technology";
}

DesignFlowStepRef TechnologyFlowStepFactory::CreateTechnologyFlowStep(const TechnologyFlowStep_Type technology_flow_step_type) const
{
   switch(technology_flow_step_type)
   {
      case TechnologyFlowStep_Type::FIX_CHARACTERIZATION:
      {
         return DesignFlowStepRef(new FixCharacterization(TM, target, design_flow_manager.lock(), parameters));
      }
#if HAVE_CIRCUIT_BUILT
      case TechnologyFlowStep_Type::LOAD_BUILTIN_TECHNOLOGY:
      {
         return DesignFlowStepRef(new LoadBuiltinTechnology(TM, target, design_flow_manager.lock(), parameters));
      }
#endif
      case TechnologyFlowStep_Type::LOAD_DEFAULT_TECHNOLOGY:
      {
         return DesignFlowStepRef(new LoadDefaultTechnology(TM, target, design_flow_manager.lock(), parameters));
      }
      case TechnologyFlowStep_Type::LOAD_DEVICE_TECHNOLOGY:
      {
         return DesignFlowStepRef(new LoadDeviceTechnology(TM, target, design_flow_manager.lock(), parameters));
      }
      case TechnologyFlowStep_Type::LOAD_FILE_TECHNOLOGY:
      {
         return DesignFlowStepRef(new LoadFileTechnology(TM, target, design_flow_manager.lock(), parameters));
      }
      case TechnologyFlowStep_Type::LOAD_TECHNOLOGY:
      {
         return DesignFlowStepRef(new LoadTechnology(TM, target, design_flow_manager.lock(), parameters));
      }
      case TechnologyFlowStep_Type::WRITE_TECHNOLOGY:
      {
         return DesignFlowStepRef(new WriteTechnology(TM, target, design_flow_manager.lock(), parameters));
      }
      default:
         THROW_UNREACHABLE("");
   }
   THROW_UNREACHABLE("");
   return DesignFlowStepRef();
}
