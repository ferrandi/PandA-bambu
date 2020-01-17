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
 * @file load_device_technology.cpp
 * @brief This class loads device dependent technology information
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

/// Header include
#include "load_device_technology.hpp"

///. include
#include "Parameter.hpp"

/// parser/polixml include
#include "xml_dom_parser.hpp"

/// polixml include
#include "xml_document.hpp"

/// technology/target_device include
#include "target_device.hpp"

/// utility include
#include "fileIO.hpp"

LoadDeviceTechnology::LoadDeviceTechnology(const technology_managerRef _TM, const target_deviceRef _target, const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters)
    : TechnologyFlowStep(_TM, _target, _design_flow_manager, TechnologyFlowStep_Type::LOAD_DEVICE_TECHNOLOGY, _parameters)
{
}

LoadDeviceTechnology::~LoadDeviceTechnology() = default;

const CustomUnorderedSet<TechnologyFlowStep_Type> LoadDeviceTechnology::ComputeTechnologyRelationships(const DesignFlowStep::RelationshipType relationship_type) const
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
#if HAVE_CIRCUIT_BUILT
         relationships.insert(TechnologyFlowStep_Type::LOAD_BUILTIN_TECHNOLOGY);
#endif
         relationships.insert(TechnologyFlowStep_Type::LOAD_DEFAULT_TECHNOLOGY);
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

DesignFlowStep_Status LoadDeviceTechnology::Exec()
{
   // if configuration file is given, it is parsed to check for technology information
   if(parameters->isOption(OPT_xml_input_configuration))
   {
      std::string fn = parameters->getOption<std::string>(OPT_xml_input_configuration);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "checking for technology information in the configuration file...");
      try
      {
         XMLDomParser parser(fn);
         parser.Exec();
         if(parser)
         {
            target->xload(target, parser.get_document()->get_root_node());
         }
      }
      catch(const char* msg)
      {
         THROW_ERROR("Error during technology file parsing: " + std::string(msg));
      }
      catch(const std::string& msg)
      {
         THROW_ERROR("Error during technology file parsing: " + msg);
      }
      catch(const std::exception& ex)
      {
         THROW_ERROR("Error during technology file parsing: " + std::string(ex.what()));
      }
      catch(...)
      {
         THROW_ERROR("Error during technology file parsing");
      }
      PRINT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, " ==== XML configuration file parsed for technology information ====");
   }

   /// load specific device information
   target->load_devices(target);
   return DesignFlowStep_Status::SUCCESS;
}
