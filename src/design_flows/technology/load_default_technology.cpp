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
 * @file load_default_technology.cpp
 * @brief This class loads default technology libraries
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "load_default_technology.hpp"

/// Autoheader include
#include "config_HAVE_KOALA_BUILT.hpp"
#include "config_PANDA_DATA_INSTALLDIR.hpp"

/// parser/polixml include
#include "xml_dom_parser.hpp"

/// polixml include
#include "xml_document.hpp"

/// STD include
#include <string>

/// STL include
#include "custom_set.hpp"

/// technology include
#include "technology_manager.hpp"

/// utility include
#include "fileIO.hpp"

LoadDefaultTechnology::LoadDefaultTechnology(const technology_managerRef _TM, const target_deviceRef _target, const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters)
    : TechnologyFlowStep(_TM, _target, _design_flow_manager, TechnologyFlowStep_Type::LOAD_DEFAULT_TECHNOLOGY, _parameters)
{
}

LoadDefaultTechnology::~LoadDefaultTechnology() = default;

const CustomUnorderedSet<TechnologyFlowStep_Type> LoadDefaultTechnology::ComputeTechnologyRelationships(const DesignFlowStep::RelationshipType) const
{
   return CustomUnorderedSet<TechnologyFlowStep_Type>();
}

DesignFlowStep_Status LoadDefaultTechnology::Exec()
{
   size_t i = 0;
   try
   {
      /// Load default resources
      const char* builtin_resources_data[] = {
         "C_COMPLEX_IPs.data",
         "C_FP_IPs.data",
         "C_HLS_IPs.data",
         "C_IO_IPs.data",
         "C_MEM_IPs.data",
#if HAVE_EXPERIMENTAL
         "C_PC_IPs.data",
#endif
         "CS_COMPONENT.data",
         "C_PROFILING_IPs.data",
         "C_STD_IPs.data",
         "C_VEC_IPs.data",
         "NC_HLS_IPs.data",
         "NC_MEM_IPs.data",
         "NC_PC_IPs.data",
         "NC_SF_IPs.data",
         "NC_STD_IPs.data",
         "NC_VEC_IPs.data",
         "NC_wishbone_IPs.data"
      };

      for(i = 0; i < sizeof(builtin_resources_data) / sizeof(char*); ++i)
      {
         XMLDomParser parser(relocate_compiler_path(PANDA_DATA_INSTALLDIR "/panda/design_flows/technology/") + builtin_resources_data[i]);
         parser.Exec();
         if(parser)
         {
            // Walk the tree:
            const xml_element* node = parser.get_document()->get_root_node(); // deleted by DomParser.
            TM->xload(node, target);
         }
      }
   }
   catch(const char* msg)
   {
      THROW_ERROR("Error during parsing of technology file: " + std::string(msg));
   }
   catch(const std::string& msg)
   {
      THROW_ERROR("Error during parsing of technology file number " + STR(i) + " : " + msg);
   }
   catch(const std::exception& ex)
   {
      THROW_ERROR("Error during parsing of technology file: " + std::string(ex.what()));
   }
   catch(...)
   {
      THROW_ERROR("Error during parsing of technology file - unknown exception");
   }

   return DesignFlowStep_Status::SUCCESS;
}
