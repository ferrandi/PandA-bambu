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
 *              Copyright (C) 2004-2023 Politecnico di Milano
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
 * $Revision$
 * $Date$
 *
 */

#include "parse_technology.hpp"

#include "library_manager.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"
#include "exceptions.hpp"
#include "fileIO.hpp"
#include "polixml.hpp"
#include "xml_dom_parser.hpp"

#include <iosfwd>
#include <string>

#include "simple_indent.hpp"
#include <boost/algorithm/string/trim.hpp>
#include <filesystem>

#include "Parameter.hpp"
#include "cpu_time.hpp"
#include "utility.hpp"

/// STL include
#include <vector>

/// utility include
#include "string_manipulation.hpp"

void read_technology_File(const std::string& fn, const technology_managerRef& TM, const ParameterConstRef& Param,
                          const target_deviceRef& device)
{
   try
   {
      XMLDomParser parser(fn);
      parser.Exec();
      if(parser)
      {
         // Walk the tree:
         const xml_element* node = parser.get_document()->get_root_node(); // deleted by DomParser.
         TM->xload(node, device);

         std::vector<std::string> input_libraries;
         if(Param->isOption(OPT_input_libraries))
         {
            auto input_libs = Param->getOption<std::string>(OPT_input_libraries);
            input_libraries = convert_string_to_vector<std::string>(input_libs, ";");
         }
         const std::vector<std::string>& libraries = TM->get_library_list();
         for(const auto& librarie : libraries)
         {
            if(WORK_LIBRARY == librarie or DESIGN == librarie or PROXY_LIBRARY == librarie)
            {
               continue;
            }
            if(std::find(input_libraries.begin(), input_libraries.end(), librarie) == input_libraries.end())
            {
               input_libraries.push_back(librarie);
            }
         }
         /// FIXME: setting parameters
         const_cast<Parameter*>(Param.get())
             ->setOption(OPT_input_libraries, convert_vector_to_string<std::string>(input_libraries, ";"));
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

void read_technology_library(const technology_managerRef& TM, const ParameterConstRef& Param,
                             const target_deviceRef& device)
{
#ifndef NDEBUG
   int debug_level = Param->get_class_debug_level("parse_technology");
#endif

   if(Param->isOption("input_xml_library_file"))
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "(koala) Reading the XML technology library");

      std::string LibraryName;
      auto XmlLibraryList = Param->getOption<std::string>("input_xml_library_file");
      std::vector<std::string> SplittedLibs = SplitString(XmlLibraryList, ";");
      for(unsigned int i = 0; i < SplittedLibs.size(); i++)
      {
         if(SplittedLibs.empty())
         {
            continue;
         }
         LibraryName = SplittedLibs[i];
         long xmlTime;
         START_TIME(xmlTime);
         read_technology_File(SplittedLibs[i], TM, Param, device);
         STOP_TIME(xmlTime);
         PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level,
                       "(koala) Read the XML technology library file \"" + LibraryName + "\" in " +
                           boost::lexical_cast<std::string>(print_cpu_time(xmlTime)) + " seconds;\n");
      }
   }
}


void write_technology_File(unsigned int type,
                           const std::string&
                           ,
                           const technology_managerRef&
                           ,
                           TargetDevice_Type,
                           const CustomOrderedSet<std::string>&
)
{
   if((type & technology_manager::XML) != 0)
   {
      THROW_UNREACHABLE("Unexpected case");
   }
}

#if HAVE_EXPERIMENTAL
void write_technology_File(unsigned int type, const std::string& f, library_manager* LM, TargetDevice_Type dv_type)
{
   if((type & technology_manager::XML) != 0)
   {
      write_xml_technology_File(f + ".xml", LM, dv_type);
   }
}
#endif

void write_xml_technology_File(const std::string& f, library_manager* LM, TargetDevice_Type dv_type)
{
   try
   {
      xml_document document;
      xml_element* nodeRoot = document.create_root_node("technology");
      LM->xwrite(nodeRoot, dv_type);
      document.write_to_file_formatted(f);
      LM->set_info(library_manager::XML, f);
   }
   catch(const char* msg)
   {
      std::cerr << msg << std::endl;
   }
   catch(const std::string& msg)
   {
      std::cerr << msg << std::endl;
   }
   catch(const std::exception& ex)
   {
      std::cout << "Exception caught: " << ex.what() << std::endl;
   }
   catch(...)
   {
      std::cerr << "unknown exception" << std::endl;
   }
}
