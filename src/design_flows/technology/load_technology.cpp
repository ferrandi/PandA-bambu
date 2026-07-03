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
 * @file load_technology.cpp
 * @brief Pseudo step to force dependencies from all load_*_technology steps
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#include "load_technology.hpp"

#include "Parameter.hpp"
#include "custom_set.hpp"
#include "dbgPrintHelper.hpp"
#include "generic_device.hpp"
#include "parse_technology.hpp"
#include "string_manipulation.hpp"
#include "technology_manager.hpp"

#include <fstream>
#include <string>

LoadTechnology::LoadTechnology(const technology_managerRef _TM, const generic_deviceRef _target,
                               const DesignFlowManager& _design_flow_manager, const ParameterConstRef _parameters)
    : TechnologyFlowStep(_TM, _target, _design_flow_manager, TechnologyFlowStep_Type::LOAD_TECHNOLOGY, _parameters)
{
   composed = true;
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

CustomUnorderedSet<TechnologyFlowStep_Type>
LoadTechnology::ComputeTechnologyRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<TechnologyFlowStep_Type> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(TechnologyFlowStep_Type::LOAD_DEFAULT_TECHNOLOGY);
#if HAVE_CIRCUIT_BUILT
         relationships.insert(TechnologyFlowStep_Type::LOAD_BUILTIN_TECHNOLOGY);
#endif
         relationships.insert(TechnologyFlowStep_Type::LOAD_DEVICE_TECHNOLOGY);
         if(parameters->isOption(OPT_technology_file))
         {
            relationships.insert(TechnologyFlowStep_Type::LOAD_FILE_TECHNOLOGY);
         }
#ifndef NDEBUG
         if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
         {
            relationships.insert(TechnologyFlowStep_Type::WRITE_TECHNOLOGY);
         }
#endif
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}

DesignFlowStep_Status LoadTechnology::Exec()
{
   return DesignFlowStep_Status::EMPTY;
}

void LoadTechnology::PrintFinalIR() const
{
   const auto file_name =
       parameters->getOption<std::filesystem::path>(OPT_output_temporary_directory) / ("after_" + GetName() + ".tm");
   std::ofstream raw_file(file_name);
   TM->print(raw_file);
   raw_file.close();
}
