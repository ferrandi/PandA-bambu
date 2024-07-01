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
 *              Copyright (C) 2015-2024 Politecnico di Milano
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
 * @file to_data_file_step_factory.cpp
 * @brief Factory for to data file step
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

#include "to_data_file_step_factory.hpp"

/// utility include
#include "exceptions.hpp"

/// design_flows/ToDataFile include
#include "to_data_file_step.hpp"
#if HAVE_CIRCUIT_BUILT
#include "generate_fu_list.hpp"
#endif

ToDataFileStepFactory::ToDataFileStepFactory(const generic_deviceRef _device,
                                             const DesignFlowManagerConstRef _design_flow_manager,
                                             const ParameterConstRef _parameters)
    : DesignFlowStepFactory(DesignFlowStep::TO_DATA_FILE, _design_flow_manager, _parameters), device(_device)
{
}

ToDataFileStepFactory::~ToDataFileStepFactory() = default;

DesignFlowStepRef ToDataFileStepFactory::CreateStep(DesignFlowStep::signature_t signature) const
{
   THROW_ASSERT(DesignFlowStep::GetStepClass(signature) == GetClass(), "Wrong step class");
   const auto to_data_file_step_type = static_cast<ToDataFileStep_Type>(DesignFlowStep::GetStepType(signature));
   switch(to_data_file_step_type)
   {
      case ToDataFileStep_Type::UNKNOWN:
      {
         THROW_UNREACHABLE("");
         break;
      }
#if HAVE_CIRCUIT_BUILT
      case ToDataFileStep_Type::GENERATE_FU_LIST:
      {
         return DesignFlowStepRef(new GenerateFuList(device, design_flow_manager.lock(), parameters));
      }
#endif
      default:
         THROW_UNREACHABLE("");
   }
   THROW_UNREACHABLE("");
   return DesignFlowStepRef();
}
