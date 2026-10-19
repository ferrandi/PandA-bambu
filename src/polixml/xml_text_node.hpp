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
 * @file xml_text_node.hpp
 * @brief
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef XML_TEXT_NODE_HPP
#define XML_TEXT_NODE_HPP

#include "refcount.hpp"

#include "xml_node.hpp"

#include <string>

class xml_text_node : public xml_node
{
 public:
   explicit xml_text_node(const std::string& content) : xml_node(content)
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
      static_cast<void>(pretty_printer);
      std::string escaped(get_name());
      convert_unescaped(escaped);
      if(formatted)
      {
         // remove \t \n \r and spaces at the beginning
         size_t len_index = escaped.length();
         while(len_index > 0 && (escaped[len_index - 1] == ' ' || escaped[len_index - 1] == '\t' ||
                                 escaped[len_index - 1] == '\r' || escaped[len_index - 1] == '\n'))
         {
            len_index--;
         }
         if(len_index > 0)
         {
            unsigned int index = 0;
            while(escaped[index] == ' ' || escaped[index] == '\t' || escaped[index] == '\r' || escaped[index] == '\n')
            {
               index++;
            }
            os << escaped.substr(index, len_index - index);
         }
      }
      else
      {
         os << escaped;
      }
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
