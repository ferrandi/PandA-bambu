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
 * @file xml_attribute.hpp
 * @brief
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef XML_ATTRIBUTE_HPP
#define XML_ATTRIBUTE_HPP

#include "custom_map.hpp"
#include "refcount.hpp"
#include "xml_node.hpp"

#include <cassert>
#include <list>
#include <string>

REF_FORWARD_DECL(xml_attribute);

class xml_attribute : public xml_node
{
   std::string at_value;

 public:
   explicit xml_attribute(const std::string& _name) : xml_node(_name)
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
      std::string escaped(at_value);
      convert_unescaped(escaped);
      /// replace '\n' character with the escaped version "\\n"
      std::string::size_type lPos = 0;
      while((lPos = escaped.find('\n', lPos)) != std::string::npos)
      {
         escaped.replace(lPos++, 1, "\\n");
      }
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
    * @param value The attribute's value.
    */
   void set_value(const std::string& value)
   {
      at_value = value;
   }
};

struct attribute_sequence
{
   using attribute_list = std::list<xml_attribute*>;
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
      {
         return nullptr;
      }
      else
      {
         return a_map.find(name)->second.get();
      }
   }
   /* Remove the attribute with this name.
    * name is the attribute to be removed.
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

   /// Obtain the list of attributes for this element.
   attribute_list get_attributes()
   {
      return a_list;
   }

   /// Obtain the list of attributes for this element. Constant version.
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
