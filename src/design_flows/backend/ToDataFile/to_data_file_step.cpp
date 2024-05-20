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
 * @file to_data_file_step.cpp
 * @brief Base class for data backend
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#include "to_data_file_step.hpp"

#include "design_flow_manager.hpp"
#include "string_manipulation.hpp"

ToDataFileStep::ToDataFileStep(const DesignFlowManagerConstRef _design_flow_manager,
                               const ToDataFileStep_Type _to_data_file_step_type, const ParameterConstRef _parameters)
    : DesignFlowStep(ComputeSignature(_to_data_file_step_type), _design_flow_manager, _parameters),
      to_data_file_step_type(_to_data_file_step_type)
{
}

DesignFlowStep::signature_t ToDataFileStep::ComputeSignature(const ToDataFileStep_Type to_data_file_step_type)
{
   return DesignFlowStep::ComputeSignature(TO_DATA_FILE, static_cast<unsigned short>(to_data_file_step_type), 0);
}

std::string ToDataFileStep::EnumToName(const ToDataFileStep_Type to_data_file_step)
{
   switch(to_data_file_step)
   {
      case ToDataFileStep_Type::UNKNOWN:
      {
         THROW_UNREACHABLE("");
         return "";
      }
#if HAVE_CIRCUIT_BUILT
      case ToDataFileStep_Type::GENERATE_FU_LIST:
      {
         return "GenerateFuList";
      }
#endif
      default:
         THROW_UNREACHABLE("");
   }
   return "";
}

ToDataFileStep_Type ToDataFileStep::NameToEnum(const std::string&
#if HAVE_CIRCUIT_BUILT
                                                   to_data_file_step
#endif
)
{
#if HAVE_CIRCUIT_BUILT
   if(to_data_file_step == "GenerateFuList")
   {
      return ToDataFileStep_Type::GENERATE_FU_LIST;
   }
#endif
   THROW_UNREACHABLE("");
   return ToDataFileStep_Type::UNKNOWN;
}

bool ToDataFileStep::HasToBeExecuted() const
{
   return true;
}

std::string ToDataFileStep::GetName() const
{
   return "ToDataFile::" + EnumToName(to_data_file_step_type);
}

DesignFlowStepFactoryConstRef ToDataFileStep::CGetDesignFlowStepFactory() const
{
   return design_flow_manager.lock()->CGetDesignFlowStepFactory(DesignFlowStep::TO_DATA_FILE);
}
