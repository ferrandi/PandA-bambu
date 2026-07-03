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
 * @file write_technology.cpp
 * @brief Step to writes technology as xml file
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#include "write_technology.hpp"

#include "Parameter.hpp"
#include "dbgPrintHelper.hpp"
#include "fileIO.hpp"
#include "generic_device.hpp"
#include "library_manager.hpp"
#include "technology_manager.hpp"
#include "technology_xml.hpp"
#include "xml_document.hpp"

WriteTechnology::WriteTechnology(const technology_managerRef _TM, const generic_deviceRef _target,
                                 const DesignFlowManager& _design_flow_manager, const ParameterConstRef _parameters)
    : TechnologyFlowStep(_TM, _target, _design_flow_manager, TechnologyFlowStep_Type::WRITE_TECHNOLOGY, _parameters)
{
}

DesignFlowStep_Status WriteTechnology::Exec()
{
   try
   {
      const auto output_file = parameters->isOption(OPT_output_file) ?
                                   parameters->getOption<std::string>(OPT_output_file) :
                                   "technology_out.xml";
      const auto libraries = TM->get_library_list();
      xml_document document;
      xml_element* nodeRoot = document.create_root_node("target");

      target->xwrite(nodeRoot);
      xml_element* tmRoot = nodeRoot->add_child_element("technology");

      TM->xwrite(tmRoot);
      document.write_to_file_formatted(output_file);
      INDENT_OUT_MEX(OUTPUT_LEVEL_VERY_PEDANTIC, output_level, "---Writing " + output_file);
      for(const auto& library : libraries)
      {
         TM->get_library_manager(library)->set_info(library_manager::XML, output_file);
      }
   }
   catch(const char* msg)
   {
      PRINT_OUT_MEX(0, 0, msg);
   }
   catch(const std::string& msg)
   {
      PRINT_OUT_MEX(0, 0, msg);
   }
   catch(const std::exception& ex)
   {
      PRINT_OUT_MEX(0, 0, std::string("Exception caught: ") + ex.what());
   }
   catch(...)
   {
      PRINT_OUT_MEX(0, 0, std::string("Unknown excetpion"));
   }
   return DesignFlowStep_Status::SUCCESS;
}

CustomUnorderedSet<TechnologyFlowStep_Type>
WriteTechnology::ComputeTechnologyRelationships(const DesignFlowStep::RelationshipType relationship_type) const
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
         relationships.insert(TechnologyFlowStep_Type::LOAD_DEVICE_TECHNOLOGY);
         relationships.insert(TechnologyFlowStep_Type::LOAD_FILE_TECHNOLOGY);
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}
