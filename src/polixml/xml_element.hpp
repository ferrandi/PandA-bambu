/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************            
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2004-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file xml_element.hpp
 * @brief
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef XML_ELEMENT_HPP
#define XML_ELEMENT_HPP

#include "refcount.hpp"

#include "exceptions.hpp"
#include "xml_attribute.hpp"
#include "xml_node.hpp"
#include <string>

REF_FORWARD_DECL(xml_element);

class xml_element : public xml_child, public attribute_sequence
{
 public:
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
      {
         print_attributes(os);
      }
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
            {
               (*pp)(os, "\n");
            }
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
