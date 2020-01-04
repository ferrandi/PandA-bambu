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
 * @file data_xml_parser.cpp
 * @brief Parse xml file containing generic data
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Date$
 *
 */

/// Header include
#include "data_xml_parser.hpp"

/// Parameter include
#include "Parameter.hpp"

/// Constants includes
#include "experimental_setup_xml.hpp"
#include "latex_table_xml.hpp"

/// Utility include
#include "exceptions.hpp"
#include "fileIO.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

/// XML include
#include "data_xml.hpp"
#include "polixml.hpp"
#include "xml_dom_parser.hpp"
#include "xml_helper.hpp"

DataXmlParser::DataXmlParser(const ParameterConstRef& Param) : debug_level(Param->get_class_debug_level(GET_CLASS(*this)))
{
}

DataXmlParser::~DataXmlParser() = default;

void DataXmlParser::Parse(const CustomSet<std::string>& file_names, std::map<std::string, CustomMap<std::string, std::string>>& output) const
{
   try
   {
      for(const auto& file_name : file_names)
      {
         XMLDomParser parser(file_name);
         parser.Exec();
         if(parser)
         {
            // Walk the tree:
            const xml_element* root = parser.get_document()->get_root_node();
            if(root->get_name() != STR_XML_latex_table_root and root->get_name() != STR_XML_experimental_setup_root)
            {
               std::string benchmark_name;
               if(!CE_XVM(benchmark_name, root))
               {
                  THROW_ERROR("Name of benchmark not found in file " + file_name);
               }
               benchmark_name = root->get_attribute(STR_XML_data_xml_benchmark_name)->get_value();

               // Recurse through child nodes:
               const auto& list = root->get_children();
               xml_node::node_list::const_iterator child, child_end = list.end();
               for(child = list.begin(); child != child_end; ++child)
               {
                  const auto* child_element = GetPointer<const xml_element>(*child);
                  if(!child_element)
                  {
                     continue;
                  }
                  const std::string child_name = child_element->get_name();
                  const std::string value = child_element->get_attribute(STR_XML_data_xml_value)->get_value();
                  output[benchmark_name][child_name] = value;
               }
            }
         }
      }
   }
   catch(const char* msg)
   {
      THROW_ERROR("Error during parsing of data xml file: " + std::string(msg));
   }
   catch(const std::string& msg)
   {
      THROW_ERROR("Error during parsing of data xml file: " + msg);
   }
   catch(const std::exception& ex)
   {
      THROW_ERROR("Error during parsing of data xml file: " + std::string(ex.what()));
   }
   catch(...)
   {
      THROW_ERROR("Error during parsing of data xml file");
   }
}
