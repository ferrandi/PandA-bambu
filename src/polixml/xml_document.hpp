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
 * @file xml_document.hpp
 * @brief
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef XML_DOCUMENT_HPP
#define XML_DOCUMENT_HPP

#include <fstream>
#include <ostream>
#include <string>
#include <utility>

#include "refcount.hpp"

#include "xml_element.hpp"
#include "xml_node.hpp"

#include "exceptions.hpp"

class xml_document : public xml_child
{
   xml_element* root_node;
   std::string version;
   std::string encoding;

 public:
   /** constructor
    */
   explicit xml_document(std::string _version = "1.0") : xml_child(std::string("")), root_node(nullptr), version(std::move(_version))
   {
   }

   /**
    * Print the class.
    */
   void print(std::ostream& os, bool formatted, simple_indent* pp) const override
   {
      os << "<?xml version=\"" << version << "\"";
      if(encoding != "")
         os << " encoding=\"" << encoding << "\"";
      os << "?>";
      if(!formatted || !pp)
         os << "\n";
      xml_child::print(os, formatted, pp);
      os << "\n";
   }

   /** Return the root node.
    * This function does _not_ create a default root node if it doesn't exist.
    * @return A pointer to the root node if it exists, 0 otherwise.
    */
   xml_element* get_root_node() const
   {
      return root_node;
   }

   /** Creates the root node.
    * @param name The node's name.
    * @return A pointer to the new root node
    */
   xml_element* create_root_node(const std::string& _name)
   {
      root_node = add_child_element(_name);
      return root_node;
   }

   /** Write the document to a file.
    * @param filename
    */
   void write_to_file(const std::string& filename)
   {
      std::ofstream xml_file(filename.c_str());
      THROW_UNREACHABLE("Bug: 0-pointer will be used in the next");
      print(xml_file, false, nullptr);
   }

   /** Write the document to a file.
    * The output is formatted by inserting whitespaces, which is easier to read for a human,
    * but may insert unwanted significant whitespaces. Use with care !
    * @param filename
    */
   void write_to_file_formatted(const std::string& filename)
   {
      std::ofstream xml_file(filename.c_str());
      simple_indent PP(STD_OPENING_CHAR, STD_CLOSING_CHAR, XML_TAB_SIZE);
      print(xml_file, true, &PP);
   }

   /** @return The encoding used in the source from which the document has been loaded.
    */
   std::string get_encoding() const
   {
      return encoding;
   }

   /** @return The encoding used in the source from which the document has been loaded.
    */
   void set_encoding(const std::string& _encoding)
   {
      encoding = _encoding;
   }
};

#endif /*DOCUMENT_HPP_*/
