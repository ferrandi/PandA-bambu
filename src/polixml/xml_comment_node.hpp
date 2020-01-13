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
 * @file xml_comment_node.hpp
 * @brief
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef XML_COMMENT_NODE_HPP
#define XML_COMMENT_NODE_HPP

#include "refcount.hpp"

#include "xml_node.hpp"

#include <string>

class xml_comment_node : public xml_node
{
 public:
   /// constructor
   explicit xml_comment_node(const std::string& comment) : xml_node(comment)
   {
   }

   /**
    * Print the class.
    * @param os is the stream.
    * @param formatted when true the xml is formatted in human readable way.
    * @param pp is the pretty print helper.
    */
   void print(std::ostream& os, bool formatted, simple_indent* pp) const override
   {
      std::string escaped(get_name());
      convert_unescaped(escaped);
      if(formatted && pp)
         (*pp)(os, "\n");
      os << "<!--" << escaped << "-->";
   }

   /** Get the text of this content node.
    * @returns The text. Note that the 5 predefined entities (&amp;, &quot;, &lt;, &qt, &apos;)
    * are always resolved, so this content will show their human-readable equivalents.
    */
   std::string get_content() const
   {
      return get_name();
   }

   /** Set the text of this content node
    * @param content The text. This must be unescaped, meaning that the predefined entities will be created for you where necessary.
    * See get_content().
    */
   void set_content(const std::string& content)
   {
      set_name(content);
   }
};

#endif
