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
 * @file NP_functionality.cpp
 * @brief Not parsed functionality manager.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#include "NP_functionality.hpp"
#include "exceptions.hpp"                     // for THROW_AS...
#include "xml_attribute.hpp"                  // for attribut...
#include "xml_element.hpp"                    // for xml_element
#include "xml_helper.hpp"                     // for WRITE_XNVM2
#include <boost/iterator/iterator_facade.hpp> // for operator!=
#include <boost/lexical_cast.hpp>             // for lexical_...
#include <boost/token_functions.hpp>          // for char_sep...
#include <boost/tokenizer.hpp>                // for tokenize...
#include <list>                               // for _List_co...
#include <utility>                            // for pair

/// STD include
#include <iostream>

/// utility include
#include "string_manipulation.hpp"

NP_functionality::NP_functionaly_type NP_functionality::to_NP_functionaly_type(const std::string& val)
{
   unsigned int i;
   for(i = 0; i < UNKNOWN; i++)
   {
      if(val == NP_functionaly_typeNames[i])
      {
         break;
      }
   }
   THROW_ASSERT(i < UNKNOWN, "Wrong NP_functionaly_typeNames specified: |" + val + "|");
   return NP_functionaly_type(i);
}

const char* NP_functionality::NP_functionaly_typeNames[] = {"TABLE",
                                                            "EQUATION",
                                                            "PORT_LIST",
                                                            "LIBRARY",
                                                            "GRAPH",
                                                            "FSM",
                                                            "FSM_CS",
                                                            "SC_PROVIDED",
                                                            "VHDL_PROVIDED",
                                                            "VERILOG_PROVIDED",
                                                            "SYSTEM_VERILOG_PROVIDED",
                                                            "VERILOG_GENERATOR",
                                                            "VHDL_GENERATOR",
                                                            "FLOPOCO_PROVIDED",
                                                            "BAMBU_PROVIDED",
                                                            "BLIF_DESCRIPTION",
                                                            "AIGER_DESCRIPTION",
                                                            "IP_COMPONENT",
                                                            "IP_LIBRARY",
                                                            "VERILOG_FILE_PROVIDED",
                                                            "VHDL_FILE_PROVIDED",
                                                            "UNKNOWN"};

void NP_functionality::add_NP_functionality(NP_functionaly_type type, const std::string& functionality_description)
{
   descriptions[type] = functionality_description;
}

void NP_functionality::xload(const xml_element* Enode)
{
   // Recurse through attributes:
   const xml_element::attribute_list& list = Enode->get_attributes();
   for(auto iter : list)
   {
      descriptions[to_NP_functionaly_type(iter->get_name())] = iter->get_value();
   }
}
void NP_functionality::xwrite(xml_element* rootnode)
{
   xml_element* Enode = rootnode->add_child_element(get_kind_text());
   auto it_end = descriptions.end();
   for(auto it = descriptions.begin(); it != it_end; ++it)
   {
      WRITE_XNVM2(NP_functionaly_typeNames[it->first], it->second, Enode);
   }
}

void NP_functionality::print(std::ostream& os) const
{
   auto it_end = descriptions.end();
   for(auto it = descriptions.begin(); it != it_end; ++it)
   {
      os << NP_functionaly_typeNames[it->first] << " " << it->second << std::endl;
   }
}

std::string NP_functionality::get_NP_functionality(NP_functionaly_type type) const
{
   if(descriptions.find(type) == descriptions.end())
   {
      return "";
   }
   else
   {
      return descriptions.find(type)->second;
   }
}

bool NP_functionality::exist_NP_functionality(NP_functionaly_type type) const
{
   return (descriptions.find(type) != descriptions.end() && !descriptions.find(type)->second.empty());
}

std::string NP_functionality::get_library_name() const
{
   std::string library_description = get_NP_functionality(NP_functionality::LIBRARY);
   if(library_description.empty())
   {
      return library_description;
   }
   using tokenizer = boost::tokenizer<boost::char_separator<char>>;
   boost::char_separator<char> sep(" ", nullptr);
   tokenizer tokens(library_description, sep);
   return *tokens.begin();
}

void NP_functionality::get_library_parameters(std::vector<std::string>& parameters) const
{
   std::string library_description = get_NP_functionality(NP_functionality::LIBRARY);
   if(library_description.empty())
   {
      return;
   }

   using tokenizer = boost::tokenizer<boost::char_separator<char>>;
   boost::char_separator<char> sep(" ", nullptr);
   tokenizer tokens(library_description, sep);
   tokenizer::iterator tok_iter = tokens.begin();
   for(++tok_iter; tok_iter != tokens.end(); ++tok_iter)
   {
      parameters.push_back(*tok_iter);
   }
}

void NP_functionality::get_port_list(std::map<unsigned int, std::map<std::string, std::string>>& InPortMap, std::map<unsigned int, std::map<std::string, std::string>>& OutPortMap) const
{
   std::string port_list = get_NP_functionality(NP_functionality::PORT_LIST);
   if(port_list.empty())
   {
      return;
   }
   std::vector<std::string> splitted = SplitString(port_list, ",");
   for(auto port_description : splitted)
   {
      if(port_description.empty())
      {
         continue;
      }
      std::vector<std::string> ports = SplitString(port_description, ":");
      THROW_ASSERT(ports.size() == 4, "Wrong format for NP_functionality::PORT_LIST functionality");
      if(ports[0] == "I")
      {
         InPortMap[boost::lexical_cast<unsigned int>(ports[1])][ports[2]] = ports[3];
      }
      if(ports[0] == "O")
      {
         OutPortMap[boost::lexical_cast<unsigned int>(ports[1])][ports[2]] = ports[3];
      }
   }
}

NP_functionality::NP_functionality(const NP_functionalityRef& obj)
{
   for(unsigned int i = 0; i < UNKNOWN; i++)
   {
      std::string val = obj->get_NP_functionality(static_cast<NP_functionaly_type>(i));
      if(!val.empty())
      {
         descriptions[static_cast<NP_functionaly_type>(i)] = val;
      }
   }
}
