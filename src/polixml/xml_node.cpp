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
 * @file xml_node.cpp
 * @brief
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

#include "xml_node.hpp"
#include "xml_att_decl_node.hpp"
#include "xml_comment_node.hpp"
#include "xml_element.hpp"
#include "xml_text_node.hpp"

/// STL include
#include <vector>

/// utility include
#include "string_manipulation.hpp"

xml_element* xml_child::add_child_element(const std::string& _name)
{
   auto* new_el = new xml_element(_name);
   xml_nodeRef new_ref(new_el);
   child_list.push_back(new_ref);
   return new_el;
}

xml_element* xml_child::add_child_element(const xml_nodeRef& node)
{
   child_list.push_back(node);
   return GetPointer<xml_element>(node);
}

xml_text_node* xml_child::add_child_text(const std::string& content)
{
   auto* new_el = new xml_text_node(content);
   if(!first_text)
   {
      first_text = new_el;
   }
   xml_nodeRef new_ref(new_el);
   child_list.push_back(new_ref);
   return new_el;
}

xml_comment_node* xml_child::add_child_comment(const std::string& content)
{
   auto* new_el = new xml_comment_node(content);
   xml_nodeRef new_ref(new_el);
   child_list.push_back(new_ref);
   return new_el;
}

xml_att_decl_node* xml_child::add_child_attribute_declaration(const std::string& _name)
{
   auto* new_el = new xml_att_decl_node(_name);
   xml_nodeRef new_ref(new_el);
   child_list.push_back(new_ref);
   return new_el;
}

void xml_node::set_line(int _line)
{
   line = _line;
}

int xml_node::get_line() const
{
   return line;
}

const CustomSet<xml_nodeRef> xml_child::CGetDescendants(const std::string& path) const
{
   CustomSet<xml_nodeRef> ret;
   std::vector<std::string> splitted = SplitString(path, "/");
   CustomSet<xml_nodeRef> iteration_input_nodes, iteration_output_nodes;
   for(const auto& child : get_children())
   {
      const auto* child_xml = GetPointer<const xml_element>(child);
      if(not child_xml)
      {
         continue;
      }
      iteration_input_nodes.insert(child);
   }
   for(size_t level = 0; level < splitted.size(); level++)
   {
      const auto current_level_tag = splitted[level];
      for(const auto& iteration_input_node : iteration_input_nodes)
      {
         if(iteration_input_node->get_name() == current_level_tag)
         {
            if(level == splitted.size() - 1)
            {
               ret.insert(iteration_input_node);
            }
            else
            {
               for(const auto& child : GetPointer<xml_child>(iteration_input_node)->get_children())
               {
                  const auto* child_xml = GetPointer<const xml_element>(child);
                  if(not child_xml)
                  {
                     continue;
                  }
                  iteration_output_nodes.insert(child);
               }
            }
         }
      }
      iteration_input_nodes = iteration_output_nodes;
   }
   return ret;
}
