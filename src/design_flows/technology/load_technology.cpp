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
 * @file load_technology.cpp
 * @brief Pseudo step to force dependencies from all load_*_technology steps
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#include "load_technology.hpp"
#include "Parameter.hpp"
#include "custom_set.hpp"
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "generic_device.hpp"
#include "parse_technology.hpp"
#include "string_manipulation.hpp" // for GET_CLASS
#include "technology_manager.hpp"
#include <string>

LoadTechnology::LoadTechnology(const technology_managerRef _TM, const generic_deviceRef _target,
                               const DesignFlowManagerConstRef _design_flow_manager,
                               const ParameterConstRef _parameters)
    : TechnologyFlowStep(_TM, _target, _design_flow_manager, TechnologyFlowStep_Type::LOAD_TECHNOLOGY, _parameters)
{
   composed = true;
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

LoadTechnology::~LoadTechnology() = default;

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
