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
 * @file load_file_technology.cpp
 * @brief This class loads a technology library from a file specified at command line
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "load_file_technology.hpp"

#include "Parameter.hpp"
#include "dbgPrintHelper.hpp"
#include "parse_technology.hpp"
#include "string_manipulation.hpp"

LoadFileTechnology::LoadFileTechnology(const technology_managerRef _TM, const generic_deviceRef _target,
                                       const DesignFlowManager& _design_flow_manager,
                                       const ParameterConstRef _parameters)
    : TechnologyFlowStep(_TM, _target, _design_flow_manager, TechnologyFlowStep_Type::LOAD_FILE_TECHNOLOGY, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

DesignFlowStep_Status LoadFileTechnology::Exec()
{
   const auto tech_files = parameters->getOption<CustomSet<std::string>>(OPT_technology_file);
   for(const auto& tech_file : tech_files)
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_VERY_PEDANTIC, output_level, "---Reading file " + tech_file);
      read_technology_File(tech_file, TM, parameters);
   }
   return DesignFlowStep_Status::SUCCESS;
}

CustomUnorderedSet<TechnologyFlowStep_Type>
LoadFileTechnology::ComputeTechnologyRelationships(const DesignFlowStep::RelationshipType relationship_type) const
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
