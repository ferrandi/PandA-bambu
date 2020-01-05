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
 * @file xml_attribute.hpp
 * @brief
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef XML_ATTRIBUTE_HPP
#define XML_ATTRIBUTE_HPP

/// STD include
#include <string>

/// STL include
#include "custom_map.hpp"
#include <list>

/// Utility include
#include "refcount.hpp"
#include <cassert>

/// XML include
#include "xml_node.hpp"

/**
 * @name forward declarations
 */
//@{
REF_FORWARD_DECL(xml_attribute);
//@}

class xml_attribute : public xml_node
{
   std::string at_value;

 public:
   /// constructor
   explicit xml_attribute(const std::string& _name) : xml_node(_name)
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
      std::string escaped(at_value);
      convert_unescaped(escaped);
      /// replace '\n' character with the escaped version "\\n"
      std::string::size_type lPos = 0;
      while((lPos = escaped.find("\n", lPos)) != std::string::npos)
         escaped.replace(lPos++, 1, "\\n");
      os << get_name() << "=\"" << escaped << "\"";
   }

   /**
    * Get the value of this attribute.
    * @returns The node's value.
    */
   std::string get_value() const
   {
      return at_value;
   }
   /**
    * Set the value of this attribute.
    * @param The attribute's value.
    */
   void set_value(const std::string& value)
   {
      at_value = value;
   }
};

struct attribute_sequence
{
   typedef std::list<xml_attribute*> attribute_list;
   /**
    * Print the class.
    * @param os is the stream.
    */
   void print_attributes(std::ostream& os) const
   {
      auto it_end = a_list.end();
      for(auto it = a_list.begin(); it != it_end; ++it)
      {
         os << " ";
         (*it)->print(os, false, nullptr);
      }
   }

   /** Set the value of the attribute with this name, and optionally with this namespace.
    * A matching attribute will be added if no matching attribute already exists.
    * @param name The name of the attribute whose value will change.
    * @param value The new value for the attribute
    * @return The attribute that was changed or created.
    */
   xml_attribute* set_attribute(const std::string& name, const std::string& value)
   {
      assert("" != name);
      if(a_map.find(name) != a_map.end())
      {
         xml_attribute* at = a_map.find(name)->second.get();
         at->set_value(value);
         return at;
      }
      else
      {
         auto* at = new xml_attribute(name);
         xml_attributeRef at_ref(at);
         a_list.push_back(at);
         a_map[name] = at_ref;
         at->set_value(value);
         return at;
      }
   }

   /** Obtain the attribute with this name
    * @param name The name of the attribute searched.
    * @return the searched attribute, or 0 if no suitable attribute was found.
    */
   xml_attribute* get_attribute(const std::string& name) const
   {
      if(a_map.find(name) == a_map.end())
         return nullptr;
      else
         return a_map.find(name)->second.get();
   }
   /** Remove the attribute with this name.
    * @param name The name of the attribute to be removed
    */
   /*void remove_attribute(const std::string& name)
   {
      if(a_map.find(name) != a_map.end())
      {
         xml_attribute * at = a_map.find(name)->second.get();
         a_map.erase(name);
         a_list.remove(at);
      }
   }*/

   /** Obtain the list of attributes for this element.
    * @returns The list of attributes.
    */
   attribute_list get_attributes()
   {
      return a_list;
   }

   /** Obtain the list of attributes for this element. Constant version.
    * @returns The list of attributes.
    */
   const attribute_list& get_attributes() const
   {
      return a_list;
   }

   /**
    * @returns Whether this node has attributes, or is empty.
    */
   bool has_attributes() const
   {
      return !a_list.empty();
   }

 private:
   attribute_list a_list;
   std::map<std::string, xml_attributeRef> a_map;
};

#endif
