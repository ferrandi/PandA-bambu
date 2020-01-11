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
 *              Copyright (C) 2004-2019 Politecnico di Milano
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
 * @file parse_tree.cpp
 * @brief Implementation of the tree parsing interface function.
 *
 * Implementation of the function that parse a tree from a file.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * @warning This file is still in a work in progress state
 * @warning Last modified by $Author$
 *
 */

/// Header include
#include "parse_tree.hpp"

/// Parameter include
#include "Parameter.hpp"

/// STD include
#include <fstream>
#include <iostream>
#include <string>

/// tree include
#include "tree_manager.hpp"

/// Utility include
#include "refcount.hpp"

// XML includes used for writing and reading the configuration file
#include "polixml.hpp"
#include "xml_dom_parser.hpp"

#include "dbgPrintHelper.hpp"      // for DEBUG_LEVEL_

#include "tree_manager.hpp"

//#include <iostream>

// exit_code is stored in zebu.cpp
extern int exit_code;

tree_managerRef ParseTreeFile(const ParameterConstRef& Param, const std::string& f)
{
   try
   {
      extern tree_managerRef tree_parseY(const ParameterConstRef Param, std::string fn);
      auto TM = tree_parseY(Param, f);

      std::string source_name = f;
      std::string trailer = ".gimplePSSA";
      if(!source_name.compare(source_name.length() - trailer.length(), trailer.length(), trailer))
      {
         source_name.erase(source_name.length()-trailer.length(), trailer.length());
         source_name.append(".pipeline.xml");
         std::cout << "read from " << source_name << "\n";

         //const std::string output_temporary_directory = Param->getOption<std::string>(OPT_output_temporary_directory);
         //std::string leaf_name = source_file.second == "-" ? "stdin-" : GetLeafFileName(source_file.second);
         auto XMLfilename = source_name; //output_temporary_directory + "/" + leaf_name + ".pipeline.xml";
         if((boost::filesystem::exists(boost::filesystem::path(XMLfilename))))
         {
            //INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->parsing " + XMLfilename);
            XMLDomParser parser(XMLfilename);
            parser.Exec();
            if(parser)
            {
               const xml_element* node = parser.get_document()->get_root_node();
               for(const auto& iter : node->get_children())
               {
                  const auto* Enode = GetPointer<const xml_element>(iter);
                  if(!Enode)
                     continue;
                  if(Enode->get_name() == "function")
                  {
                     std::string fname;
                     std::string is_pipelined = "null";
                     for(auto attr : Enode->get_attributes())
                     {
                        std::string key = attr->get_name();
                        std::string value = attr->get_value();
                        if(key == "id")
                           fname = value;
                        if(key == "is_pipelined")
                           is_pipelined = value;
                     }
                     if(fname == "")
                        THROW_ERROR("malformed pipeline infer file");
                     if(is_pipelined.compare("yes") && is_pipelined.compare("no"))
                        THROW_ERROR("malformed pipeline infer file");
                     for(const auto& iterArg : Enode->get_children())
                     {
                        const auto* EnodeArg = GetPointer<const xml_element>(iterArg);
                        if(EnodeArg)
                           THROW_ERROR("malformed pipeline infer file");
                     }
                     auto findex = TM->function_index(fname);
                     std::cout << "The function " << fname << " has parameter is_pipelined=\"" << is_pipelined << "\"\n";
                     std::cout << "Tree retrieved index for the function is " << std::to_string(findex) << "\n\n";
                  }
               }
            }
            //INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--parsed file " + XMLfilename);
         }
         else
         {
            std::string error_string = "The file " << XMLfilename << " does not exhist";
            THROW_ERROR(error_string);
         }
      }
      return TM;
   }
   catch(const char* msg)
   {
      std::cerr << msg << std::endl;
   }
   catch(const std::string& msg)
   {
      std::cerr << msg << std::endl;
   }
   catch(...)
   {
      std::cerr << "unknown exception" << std::endl;
   }
   THROW_ERROR_CODE(exit_code, "Error in tree parsing");
   return tree_managerRef();
}
