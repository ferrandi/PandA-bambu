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
 * @file write_technology.cpp
 * @brief Step to writes technology as xml file
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Header include
#include "write_technology.hpp"

///. include
#include "Parameter.hpp"

/// constants include
#include "technology_xml.hpp"

/// polixml include
#include "xml_document.hpp"

/// technology include
#include "technology_manager.hpp"

/// technology/physical_library include
#include "library_manager.hpp"

/// technology/target_device include
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "target_device.hpp"

WriteTechnology::WriteTechnology(const technology_managerRef _TM, const target_deviceRef _target, const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters)
    : TechnologyFlowStep(_TM, _target, _design_flow_manager, TechnologyFlowStep_Type::WRITE_TECHNOLOGY, _parameters)
{
}

WriteTechnology::~WriteTechnology() = default;

DesignFlowStep_Status WriteTechnology::Exec()
{
   try
   {
      const auto output_file = parameters->isOption(OPT_output_file) ? parameters->getOption<std::string>(OPT_output_file) : "technology_out.xml";
      const auto libraries = TM->get_library_list();
      xml_document document;
      xml_element* nodeRoot = document.create_root_node("target");

      target->xwrite(nodeRoot);
      xml_element* tmRoot = nodeRoot->add_child_element("technology");

      TM->xwrite(tmRoot, target->get_type());
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

const CustomUnorderedSet<TechnologyFlowStep_Type> WriteTechnology::ComputeTechnologyRelationships(const DesignFlowStep::RelationshipType relationship_type) const
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
         relationships.insert(TechnologyFlowStep_Type::FIX_CHARACTERIZATION);
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
