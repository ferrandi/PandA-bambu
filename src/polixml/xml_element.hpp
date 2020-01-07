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
 * @file xml_element.hpp
 * @brief
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef XML_ELEMENT_HPP
#define XML_ELEMENT_HPP

#include "refcount.hpp"

#include "exceptions.hpp"
#include "xml_attribute.hpp"
#include "xml_node.hpp"
#include <string>

/**
 * @name forward declarations
 */
//@{
REF_FORWARD_DECL(xml_element);
//@}

class xml_element : public xml_child, public attribute_sequence
{
 public:
   /**
    * constructor
    */
   explicit xml_element(const std::string& _name) : xml_child(_name)
   {
   }

   /**
    * Print the class.
    */
   void print(std::ostream& os, bool formatted, simple_indent* pp) const override
   {
      THROW_ASSERT(pp, "Null indenter object: unexpected condition");
      if(formatted)
      {
         (*pp)(os, "\n");
      }
      (*pp)(os, "<" + get_name());
      if(has_attributes())
         print_attributes(os);
      if(has_child())
      {
         const char soc[2] = {STD_OPENING_CHAR, '\0'};
         const char scc[2] = {STD_CLOSING_CHAR, '\0'};
         (*pp)(os, ">");
         if(formatted)
         {
            (*pp)(os, soc);
         }
         xml_child::print(os, formatted, pp);
         if(formatted)
         {
            (*pp)(os, scc);
            if(get_children().size() > 1 || !get_child_text())
               (*pp)(os, "\n");
         }
         (*pp)(os, "</" + get_name() + ">");
      }
      else
      {
         (*pp)(os, "/>");
      }
   }
};

#endif
