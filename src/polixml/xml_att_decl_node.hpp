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
