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
 * @file xml_comment_node.hpp
 * @brief
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
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
      {
         (*pp)(os, "\n");
      }
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
    * @param content The text. This must be unescaped, meaning that the predefined entities will be created for you
    * where necessary. See get_content().
    */
   void set_content(const std::string& content)
   {
      set_name(content);
   }
};

#endif
