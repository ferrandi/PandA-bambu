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
 * @file xml_att_decl_node.hpp
 * @brief
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
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
   /// constructor
   explicit xml_att_decl_node(const std::string& _name) : xml_node(_name)
   {
   }

   /**
    * Print the class.
    * @param os is the stream.
    * @param formatted when true the xml is formatted in human readable way.
    * @param pp is the pretty print helper.
    */
   void print(std::ostream& os, bool, simple_indent*) const override
   {
      os << "<!ATTLIST" << get_name();
      if(has_attributes())
      {
         print_attributes(os);
      }
      os << ">";
   }
};

#endif
