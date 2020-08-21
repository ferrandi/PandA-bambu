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

#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_

#include "tree_manager.hpp"
#include "tree_node.hpp"

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
      if(source_name.length() > trailer.length())
      {
         source_name.erase(source_name.length() - trailer.length(), trailer.length());
         source_name.append(".pipeline.xml");
         auto XMLfilename = source_name;
         if((boost::filesystem::exists(boost::filesystem::path(XMLfilename))))
         {
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
                     std::string simple_pipeline = "null";
                     std::string initiation_time = "null";
                     for(auto attr : Enode->get_attributes())
                     {
                        std::string key = attr->get_name();
                        std::string value = attr->get_value();
                        if(key == "id")
                           fname = value;
                        if(key == "is_pipelined")
                           is_pipelined = value;
                        if(key == "is_simple")
                           simple_pipeline = value;
                        if(key == "initiation_time")
                           initiation_time = value;
                     }
                     if(fname == "")
                        THROW_ERROR("malformed pipeline infer file");
                     if(is_pipelined.compare("yes") && is_pipelined.compare("no"))
                        THROW_ERROR("malformed pipeline infer file");
                     if(simple_pipeline.compare("yes") && simple_pipeline.compare("no"))
                        THROW_ERROR("malformed pipeline infer file");
                     if(is_pipelined.compare("yes") && !simple_pipeline.compare("yes"))
                        THROW_ERROR("simple pipeline but not enabled");
                     if(!simple_pipeline.compare("yes") && std::stoi(initiation_time) != 1)
                        THROW_ERROR("malformed pipeline infer file");
                     for(const auto& iterArg : Enode->get_children())
                     {
                        const auto* EnodeArg = GetPointer<const xml_element>(iterArg);
                        if(EnodeArg)
                           THROW_ERROR("malformed pipeline infer file");
                     }
                     auto findex = TM->function_index(fname);
                     if(!findex)
                        // the function is not present in the tree manager
                        continue;
                     auto my_node = GetPointer<function_decl>(TM->get_tree_node_const(findex));
                     THROW_ASSERT(my_node->get_kind() == function_decl_K, "Not a function_decl");
                     my_node->set_pipelining(!is_pipelined.compare("yes"));
                     my_node->set_simple_pipeline(!simple_pipeline.compare(("yes")));
                     my_node->set_initiation_time(std::stoi(initiation_time));
                  }
               }
            }
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
