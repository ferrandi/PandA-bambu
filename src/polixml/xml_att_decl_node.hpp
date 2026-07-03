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
 * @file xml_att_decl_node.hpp
 * @brief
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef XML_ATT_DECL_NODE_HPP
#define XML_ATT_DECL_NODE_HPP

#include "refcount.hpp"

#include "xml_attribute.hpp"
#include "xml_node.hpp"

#include <string>

struct xml_att_decl_node : public xml_node, attribute_sequence
{
   explicit xml_att_decl_node(const std::string& _name) : xml_node(_name)
   {
   }

   /**
    * Print the class.
    * @param os is the stream.
    * @param formatted when true the xml is formatted in human readable way.
    * @param pretty_printer is the pretty print helper.
    */
   void print(std::ostream& os, bool formatted, simple_indent* pretty_printer) const override
   {
      static_cast<void>(formatted);
      static_cast<void>(pretty_printer);
      os << "<!ATTLIST" << get_name();
      if(has_attributes())
      {
         print_attributes(os);
      }
      os << ">";
   }
};

#endif
