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
 *              Copyright (C) 2004-2020 Politecnico di Milano
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
 * @file load_file_technology.cpp
 * @brief This class loads a technology library from a file specified at command line
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

/// Header include
#include "load_file_technology.hpp"

///. include
#include "Parameter.hpp"

/// technology include
#include "parse_technology.hpp"

/// utility includes
#include "dbgPrintHelper.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

LoadFileTechnology::LoadFileTechnology(const technology_managerRef _TM, const target_deviceRef _target, const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters)
    : TechnologyFlowStep(_TM, _target, _design_flow_manager, TechnologyFlowStep_Type::LOAD_FILE_TECHNOLOGY, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

LoadFileTechnology::~LoadFileTechnology() = default;

DesignFlowStep_Status LoadFileTechnology::Exec()
{
   const auto tech_files = parameters->getOption<const CustomSet<std::string>>(OPT_technology_file);
   for(const auto& tech_file : tech_files)
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_VERY_PEDANTIC, output_level, "---Reading file " + tech_file);
      read_technology_File(tech_file, TM, parameters, target);
   }
   return DesignFlowStep_Status::SUCCESS;
}

const CustomUnorderedSet<TechnologyFlowStep_Type> LoadFileTechnology::ComputeTechnologyRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<TechnologyFlowStep_Type> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         relationships.insert(TechnologyFlowStep_Type::LOAD_DEFAULT_TECHNOLOGY);
#if HAVE_CIRCUIT_BUILT
         relationships.insert(TechnologyFlowStep_Type::LOAD_BUILTIN_TECHNOLOGY);
#endif
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}
