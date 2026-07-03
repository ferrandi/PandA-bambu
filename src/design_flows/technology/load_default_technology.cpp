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
 * @file load_default_technology.cpp
 * @brief This class loads default technology libraries
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "load_default_technology.hpp"

#include "Parameter.hpp"
#include "custom_set.hpp"
#include "fileIO.hpp"
#include "string_manipulation.hpp"
#include "technology_manager.hpp"
#include "xml_document.hpp"
#include "xml_dom_parser.hpp"

#include <string>

#include "config_PANDA_LIB_INSTALLDIR.hpp"

LoadDefaultTechnology::LoadDefaultTechnology(const technology_managerRef _TM, const generic_deviceRef _target,
                                             const DesignFlowManager& _design_flow_manager,
                                             const ParameterConstRef _parameters)
    : TechnologyFlowStep(_TM, _target, _design_flow_manager, TechnologyFlowStep_Type::LOAD_DEFAULT_TECHNOLOGY,
                         _parameters)
{
}

CustomUnorderedSet<TechnologyFlowStep_Type>
LoadDefaultTechnology::ComputeTechnologyRelationships(const DesignFlowStep::RelationshipType) const
{
   return CustomUnorderedSet<TechnologyFlowStep_Type>();
}

DesignFlowStep_Status LoadDefaultTechnology::Exec()
{
   const auto libtech_dir = relocate_install_path(PANDA_LIB_INSTALLDIR "/libtech");
   try
   {
      for(const auto& ip_library : std::filesystem::directory_iterator(libtech_dir))
      {
         if(std::filesystem::is_directory(ip_library))
         {
            continue;
         }
         XMLDomParser parser(ip_library.path().string());
         parser.Exec();
         if(parser)
         {
            TM->xload(parser.get_document()->get_root_node());
         }
      }
   }
   catch(const char* msg)
   {
      THROW_ERROR("Error during parsing of technology file: " + std::string(msg));
   }
   catch(const std::string& msg)
   {
      THROW_ERROR("Error during parsing of technology file: " + msg);
   }
   catch(const std::exception& ex)
   {
      THROW_ERROR("Error during parsing of technology file: " + std::string(ex.what()));
   }
   catch(...)
   {
      THROW_ERROR("Error during parsing of technology file: unknown exception");
   }

   return DesignFlowStep_Status::SUCCESS;
}
