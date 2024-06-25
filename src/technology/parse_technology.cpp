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
 * @file parse_technology.cpp
 * @brief Implementation of the technology parsing interface function.
 *
 * Implementation of the functions that parse the technology information from files.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#include "parse_technology.hpp"
#include "Parameter.hpp"
#include "exceptions.hpp"
#include "polixml.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"
#include "utility.hpp"
#include "xml_dom_parser.hpp"
#include <string>
#include <vector>

void read_technology_File(const std::string& fn, const technology_managerRef& TM, const ParameterConstRef& Param)
{
   try
   {
      XMLDomParser parser(fn);
      parser.Exec();
      if(parser)
      {
         // Walk the tree:
         const xml_element* node = parser.get_document()->get_root_node(); // deleted by DomParser.
         TM->xload(node);

         std::set<std::string> input_libraries;
         if(Param->isOption(OPT_input_libraries))
         {
            auto input_libs = Param->getOption<std::string>(OPT_input_libraries);
            string_to_container(std::inserter(input_libraries, input_libraries.end()), input_libs, ";");
         }
         for(const auto& library : TM->get_library_list())
         {
            if(WORK_LIBRARY == library || PROXY_LIBRARY == library)
            {
               continue;
            }
            input_libraries.insert(library);
         }
         /// FIXME: setting parameters
         const_cast<Parameter*>(Param.get())->setOption(OPT_input_libraries, container_to_string(input_libraries, ";"));
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
}
