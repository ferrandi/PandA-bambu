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
 * @file xml_script_command.cpp
 * @brief Classes for handling configuration files.
 *
 * (for interface descriptions, see xml_script_command.hpp)
 *
 * @author Andrea Zoppi <texzk@email.it>
 * $Date$
 * Last modified by $Author$
 *
 */

#include "exceptions.hpp"

#include "boost/algorithm/string/case_conv.hpp"
#include "boost/algorithm/string/trim.hpp"

#include "xml_script_command.hpp"
#include <boost/algorithm/string/predicate.hpp>
#include <utility>

xml_script_node_t::~xml_script_node_t() = default;

bool xml_script_node_t::checkCondition(const DesignParametersRef&) const
{
   return true; // Condition ignored by default
}

xml_script_node_enum_t xml_script_node_t::find_type(const xml_element* element)
{
   std::string name = element->get_name();
   if(name == TAG_ENTRY)
   {
      return NODE_ENTRY;
   }
   else if(name == TAG_VARIABLE)
   {
      return NODE_VARIABLE;
   }
   else if(name == TAG_PARAMETER)
   {
      return NODE_PARAMETER;
   }
   else if(name == TAG_COMMAND)
   {
      return NODE_COMMAND;
   }
   else if(name == TAG_SHELL)
   {
      return NODE_SHELL;
   }
   else if(name == TAG_ITE_BLOCK)
   {
      return NODE_ITE_BLOCK;
   }
   else if(name == TAG_FOREACH)
   {
      return NODE_FOREACH;
   }
   else
   {
      return NODE_UNKNOWN;
   }
}

xml_script_node_t* xml_script_node_t::create(const xml_element* element)
{
   xml_script_node_t* node = nullptr;
   std::string name = element->get_name();
   if(name == TAG_ENTRY)
   {
      node = new xml_set_entry_t(element);
   }
   else if(name == TAG_VARIABLE)
   {
      node = new xml_set_variable_t(element);
   }
   else if(name == TAG_PARAMETER)
   {
      node = new xml_parameter_t(element);
   }
   else if(name == TAG_COMMAND)
   {
      node = new xml_command_t(element);
   }
   else if(name == TAG_SHELL)
   {
      node = new xml_shell_t(element);
   }
   else if(name == TAG_ITE_BLOCK)
   {
      node = new xml_ite_block_t(element);
   }
   else if(name == TAG_FOREACH)
   {
      node = new xml_foreach_t(element);
   }
   else
   {
      THROW_ERROR("Unhandled XML element named \"" + element->get_name() + "\"");
   }
   return node;
}

bool xml_script_node_t::evaluate_condition(const std::string* condition)
{
   if(!condition)
   {
      return false;
   }

   if(condition->length() == 0)
   {
      return false;
   }

   std::string trimmed = *condition;
   boost::algorithm::trim(trimmed);

   if(trimmed == "\"\"")
   {
      return false;
   }

   /// represent a variable that has not been substituted/defined
   if(boost::algorithm::starts_with(trimmed, "${__"))
   {
      return false;
   }

   auto length = static_cast<unsigned int>(trimmed.length());
   if(length == 0)
   {
      return false;
   }
   try
   {
      if(boost::lexical_cast<unsigned int>(trimmed) == 0)
      {
         return false;
      }
      else if(boost::lexical_cast<unsigned int>(trimmed) == 1)
      {
         return true;
      }
   }
   catch(const std::exception& ex)
   {
   } // trimmed contains no numbers

   std::string lowered = trimmed;
   boost::algorithm::to_lower(lowered);
   return lowered != "false";
}

bool xml_script_node_t::evaluate_condition(const std::string* condition, const DesignParametersRef& dp)
{
   if(!condition)
   {
      return true;
   }
   std::string trimmed = *condition;
   boost::algorithm::trim(trimmed);
   bool negate = false;
   if(trimmed[0] == '!')
   {
      trimmed = trimmed.substr(1, trimmed.length());
      negate = true;
   }
   if(trimmed[0] == '$')
   {
      std::string var;
      std::string::size_type length = trimmed.length();
      if(length > 7 && trimmed[1] == '{' && trimmed[length - 1] == '}')
      {
         var = trimmed.substr(4, length - 7);
      }
      else
      {
         var = trimmed.substr(3, length - 4);
      }
      if(var.length())
      {
         // Iterate trough the parameters map and compare the values
         const DesignParameters::map_t& map = dp->parameter_values;
         if(map.find(var) == map.end())
         {
            THROW_ERROR("variable " + var + " not configured");
         }
         for(const auto& it : map)
         {
            const std::string& name = it.first;
            const std::string& value = it.second;
            if(var == name)
            {
               return (negate ? !evaluate_condition(&value) : evaluate_condition(&value));
            }
         }
      }
   }

   // No parameters found, use basic evaluation
   return evaluate_condition(condition);
}

xml_set_entry_t::xml_set_entry_t(std::string _value, const std::string* _condition) : xml_script_node_t(NODE_ENTRY), value(std::move(_value))
{
   condition = _condition ? new std::string(*_condition) : nullptr;
}

xml_set_entry_t::xml_set_entry_t(const xml_element* element) : xml_script_node_t(NODE_ENTRY)
{
   xml_attribute* a;

   condition = nullptr;

   a = element->get_attribute("value");
   if(a)
   {
      value = a->get_value();
   }
   else
   {
      THROW_ERROR("Attribute \"value\" required for entry");
   }

   a = element->get_attribute("condition");
   if(a)
   {
      condition = new std::string(a->get_value());
   }
}

xml_set_entry_t::~xml_set_entry_t()
{
   clean();
}

void xml_set_entry_t::clean()
{
   delete condition;

   condition = nullptr;
}

std::string xml_set_entry_t::get_xml_name() const
{
   return TAG_ENTRY;
}

xml_nodeRef xml_set_entry_t::create_xml_node() const
{
   xml_element* node = new xml_element(get_xml_name());
   node->set_attribute("value", value);
   if(condition)
   {
      node->set_attribute("condition", *condition);
   }
   return xml_nodeRef(node);
}

bool xml_set_entry_t::checkCondition(const DesignParametersRef& dp) const
{
   return evaluate_condition(condition, dp);
}

xml_set_variable_t::xml_set_variable_t(std::string _name, const std::string* _singleValue, const std::string* _condition) : xml_script_node_t(NODE_VARIABLE), name(std::move(_name))
{
   singleValue = _singleValue ? new std::string(*_singleValue) : nullptr;
   multiValues.clear();
   condition = _condition ? new std::string(*_condition) : nullptr;
}

xml_set_variable_t::xml_set_variable_t(const xml_element* element) : xml_script_node_t(NODE_VARIABLE)
{
   xml_attribute* a;

   singleValue = nullptr;
   multiValues.clear();
   condition = nullptr;

   a = element->get_attribute("name");
   if(a)
   {
      name = a->get_value();
   }
   else
   {
      THROW_ERROR("Attribute \"name\" must be defined");
   }

   a = element->get_attribute("value");
   if(a)
   {
      singleValue = new std::string(a->get_value());
   }

   const xml_node::node_list& list = element->get_children();
   for(const auto& l : list)
   {
      const xml_element* child = GetPointer<xml_element>(l);
      if(child && child->get_name() == TAG_ENTRY)
      {
         xml_set_entry_tRef entry = xml_set_entry_tRef(new xml_set_entry_t(child));
         multiValues.push_back(entry);
      }
   }
   if(singleValue && !multiValues.empty())
   {
      THROW_ERROR("Only one between \"value\" and <entry> children can be defined");
   }

   a = element->get_attribute("condition");
   if(a)
   {
      condition = new std::string(a->get_value());
   }
}

xml_set_variable_t::~xml_set_variable_t()
{
   clean();
}

void xml_set_variable_t::clean()
{
   delete singleValue;
   delete condition;

   singleValue = nullptr;
   multiValues.clear();
   condition = nullptr;
}

std::string xml_set_variable_t::get_xml_name() const
{
   return TAG_VARIABLE;
}

xml_nodeRef xml_set_variable_t::create_xml_node() const
{
   xml_element* node = new xml_element(get_xml_name());
   node->set_attribute("name", name);
   if(singleValue)
   {
      node->set_attribute("value", *singleValue);
   }
   if(condition)
   {
      node->set_attribute("condition", *condition);
   }
   for(const auto& child : multiValues)
   {
      node->add_child_element(child->create_xml_node());
   }
   return xml_nodeRef(node);
}

bool xml_set_variable_t::checkCondition(const DesignParametersRef& dp) const
{
   return evaluate_condition(condition, dp);
}

xml_parameter_t::xml_parameter_t(const std::string* _name, const std::string* _singleValue, const std::string* _condition, const std::string& _separator, bool _curlyBrackets) : xml_script_node_t(NODE_PARAMETER)
{
   name = name ? new std::string(*_name) : nullptr;
   singleValue = _singleValue ? new std::string(*_singleValue) : nullptr;
   multiValues.clear();
   condition = _condition ? new std::string(*_condition) : nullptr;
   separator = _separator;
   curlyBrackets = _curlyBrackets;
}

xml_parameter_t::xml_parameter_t(const xml_element* element) : xml_script_node_t(NODE_PARAMETER)
{
   xml_attribute* a;

   name = nullptr;
   singleValue = nullptr;
   multiValues.clear();
   condition = nullptr;
   separator = " ";
   curlyBrackets = false;

   a = element->get_attribute("name");
   if(a)
   {
      name = new std::string(a->get_value());
   }

   a = element->get_attribute("value");
   if(a)
   {
      singleValue = new std::string(a->get_value());
   }

   const xml_node::node_list& list = element->get_children();
   for(const auto& l : list)
   {
      const xml_element* child = GetPointer<xml_element>(l);
      if(child && child->get_name() == TAG_ENTRY)
      {
         xml_set_entry_tRef entry = xml_set_entry_tRef(new xml_set_entry_t(child));
         multiValues.push_back(entry);
      }
   }
   if(singleValue && !multiValues.empty())
   {
      THROW_ERROR("Only one between \"value\" and <entry> children can be defined");
   }
   if(!name && !singleValue && multiValues.empty())
   {
      THROW_ERROR("At least one among \"name\", \"value\" or <entry> children must be defined");
   }

   a = element->get_attribute("condition");
   if(a)
   {
      condition = new std::string(a->get_value());
   }

   a = element->get_attribute("separator");
   if(a)
   {
      separator = a->get_value();
   }

   a = element->get_attribute("curly");
   if(a)
   {
      std::string cond = a->get_value();
      curlyBrackets = xml_script_node_t::evaluate_condition(&cond);
   }
}

xml_parameter_t::~xml_parameter_t()
{
   clean();
}

void xml_parameter_t::clean()
{
   delete name;
   delete singleValue;

   name = nullptr;
   singleValue = nullptr;
   multiValues.clear();
}

std::string xml_parameter_t::get_xml_name() const
{
   return TAG_PARAMETER;
}

xml_nodeRef xml_parameter_t::create_xml_node() const
{
   xml_element* node = new xml_element(get_xml_name());
   if(name)
   {
      node->set_attribute("name", *name);
   }
   if(singleValue)
   {
      node->set_attribute("value", *singleValue);
   }
   if(condition)
   {
      node->set_attribute("condition", *condition);
   }
   if(curlyBrackets)
   {
      node->set_attribute("curly", "true");
   }
   for(const auto& child : multiValues)
   {
      node->add_child_element(child->create_xml_node());
   }
   return xml_nodeRef(node);
}

bool xml_parameter_t::checkCondition(const DesignParametersRef& dp) const
{
   return evaluate_condition(condition, dp);
}

xml_command_t::xml_command_t(const std::string* _name, const std::string* _value, const std::string* _condition, const std::string* _output) : xml_script_node_t(NODE_COMMAND)
{
   name = _name ? new std::string(*_name) : nullptr;
   value = _value ? new std::string(*_value) : nullptr;
   parameters.clear();
   condition = _condition ? new std::string(*_condition) : nullptr;
   output = _output ? new std::string(*_output) : nullptr;
}

xml_command_t::xml_command_t(const xml_element* element) : xml_script_node_t(NODE_COMMAND)
{
   xml_attribute* a;

   name = nullptr;
   value = nullptr;
   parameters.clear();
   condition = nullptr;
   output = nullptr;

   a = element->get_attribute("name");
   if(a)
   {
      name = new std::string(a->get_value());
   }

   a = element->get_attribute("value");
   if(a)
   {
      value = new std::string(a->get_value());
   }

   const xml_node::node_list& list = element->get_children();
   for(const auto& l : list)
   {
      const xml_element* child = GetPointer<xml_element>(l);
      if(child && child->get_name() == TAG_PARAMETER)
      {
         xml_parameter_tRef param = xml_parameter_tRef(new xml_parameter_t(child));
         parameters.push_back(param);
      }
   }

   a = element->get_attribute("condition");
   if(a)
   {
      condition = new std::string(a->get_value());
   }

   a = element->get_attribute("output");
   if(a)
   {
      output = new std::string(a->get_value());
   }
}

xml_command_t::~xml_command_t()
{
   clean();
}

void xml_command_t::clean()
{
   delete name;
   delete value;
   delete condition;
   delete output;

   name = nullptr;
   value = nullptr;
   parameters.clear();
   condition = nullptr;
   output = nullptr;
}

std::string xml_command_t::get_xml_name() const
{
   return TAG_COMMAND;
}

xml_nodeRef xml_command_t::create_xml_node() const
{
   xml_element* node = new xml_element(get_xml_name());
   if(name)
   {
      node->set_attribute("name", *name);
   }
   if(value)
   {
      node->set_attribute("value", *value);
   }
   if(condition)
   {
      node->set_attribute("condition", *condition);
   }
   if(output)
   {
      node->set_attribute("output", *output);
   }
   for(const auto& child : parameters)
   {
      node->add_child_element(child->create_xml_node());
   }
   return xml_nodeRef(node);
}

bool xml_command_t::checkCondition(const DesignParametersRef& dp) const
{
   return evaluate_condition(condition, dp);
}

xml_shell_t::xml_shell_t(const std::string* _name, const std::string* _value, const std::string* _condition, const std::string* _output) : xml_script_node_t(NODE_SHELL)
{
   name = _name ? new std::string(*_name) : nullptr;
   value = _value ? new std::string(*_value) : nullptr;
   parameters.clear();
   condition = _condition ? new std::string(*_condition) : nullptr;
   output = _output ? new std::string(*_output) : nullptr;
}

xml_shell_t::xml_shell_t(const xml_element* element) : xml_script_node_t(NODE_SHELL)
{
   xml_attribute* a;

   name = nullptr;
   value = nullptr;
   parameters.clear();
   condition = nullptr;
   output = nullptr;

   a = element->get_attribute("name");
   if(a)
   {
      name = new std::string(a->get_value());
   }

   a = element->get_attribute("value");
   if(a)
   {
      value = new std::string(a->get_value());
   }

   const xml_node::node_list& list = element->get_children();
   for(const auto& l : list)
   {
      const xml_element* child = GetPointer<xml_element>(l);
      if(child && child->get_name() == TAG_PARAMETER)
      {
         xml_parameter_tRef param = xml_parameter_tRef(new xml_parameter_t(child));
         parameters.push_back(param);
      }
   }

   a = element->get_attribute("condition");
   if(a)
   {
      condition = new std::string(a->get_value());
   }

   a = element->get_attribute("output");
   if(a)
   {
      output = new std::string(a->get_value());
   }
}

xml_shell_t::~xml_shell_t()
{
   clean();
}

void xml_shell_t::clean()
{
   delete name;
   delete value;
   delete condition;
   delete output;

   name = nullptr;
   value = nullptr;
   parameters.clear();
   condition = nullptr;
   output = nullptr;
}

std::string xml_shell_t::get_xml_name() const
{
   return TAG_SHELL;
}

xml_nodeRef xml_shell_t::create_xml_node() const
{
   xml_element* node = new xml_element(get_xml_name());
   if(name)
   {
      node->set_attribute("name", *name);
   }
   if(value)
   {
      node->set_attribute("value", *value);
   }
   if(condition)
   {
      node->set_attribute("condition", *condition);
   }
   if(output)
   {
      node->set_attribute("output", *output);
   }
   for(const auto& child : parameters)
   {
      node->add_child_element(child->create_xml_node());
   }
   return xml_nodeRef(node);
}

bool xml_shell_t::checkCondition(const DesignParametersRef& dp) const
{
   return evaluate_condition(condition, dp);
}

xml_ite_block_t::xml_ite_block_t(const std::string* _condition) : xml_script_node_t(NODE_ITE_BLOCK), condition(_condition ? *_condition : "")
{
   this->thenNodes.clear();
   this->elseNodes.clear();
}

xml_ite_block_t::xml_ite_block_t(const xml_element* element) : xml_script_node_t(NODE_ITE_BLOCK), condition(element->get_attribute("condition") ? element->get_attribute("condition")->get_value() : "")
{
   thenNodes.clear();
   elseNodes.clear();

   bool thenFound = false, elseFound = false;

   const xml_node::node_list& list = element->get_children();
   for(const auto& l : list)
   {
      const xml_element* child = GetPointer<xml_element>(l);
      if(!child)
      {
         continue;
      }
      if(child->get_name() == "then")
      {
         if(!thenFound && !elseFound)
         {
            for(const auto& s : child->get_children())
            {
               const xml_element* el = GetPointer<xml_element>(s);
               if(!el)
               {
                  continue;
               }
               xml_script_node_tRef node = xml_script_node_tRef(xml_script_node_t::create(el));
               thenNodes.push_back(node);
            }
         }
         else
         {
            THROW_ERROR("<then> already defined, or found after <else>");
         }
      }
      else if(child->get_name() == "else")
      {
         if(!elseFound)
         {
            for(const auto& s : child->get_children())
            {
               const xml_element* el = GetPointer<xml_element>(s);
               if(!el)
               {
                  continue;
               }
               xml_script_node_tRef node = xml_script_node_tRef(xml_script_node_t::create(el));
               elseNodes.push_back(node);
            }
         }
         else
         {
            THROW_ERROR("<else> already defined");
         }
      }
   }
}

xml_ite_block_t::~xml_ite_block_t()
{
   clean();
}

void xml_ite_block_t::clean()
{
   thenNodes.clear();
   elseNodes.clear();
}

std::string xml_ite_block_t::get_xml_name() const
{
   return TAG_ITE_BLOCK;
}

xml_nodeRef xml_ite_block_t::create_xml_node() const
{
   xml_element* node = new xml_element(get_xml_name());
   node->set_attribute("condition", condition);
   xml_element* thenElement = node->add_child_element("then");
   for(const auto& child : thenNodes)
   {
      thenElement->add_child_element(child->create_xml_node());
   }
   xml_element* elseElement = node->add_child_element("else");
   for(const auto& child : elseNodes)
   {
      elseElement->add_child_element(child->create_xml_node());
   }
   return xml_nodeRef(node);
}

bool xml_ite_block_t::checkCondition(const DesignParametersRef& dp) const
{
   return evaluate_condition(&condition, dp);
}

xml_foreach_t::xml_foreach_t(std::string _variable) : xml_script_node_t(NODE_FOREACH), variable(std::move(_variable))
{
}

xml_foreach_t::xml_foreach_t(const xml_element* element) : xml_script_node_t(NODE_FOREACH)
{
   Nodes.clear();

   xml_attribute* a = element->get_attribute("variable");
   THROW_ASSERT(a, "Error: the \"foreach\" block requires the definition of the variable");
   variable = a->get_value();
   const xml_node::node_list& list = element->get_children();
   for(const auto& l : list)
   {
      const xml_element* child = GetPointer<xml_element>(l);
      if(!child)
      {
         continue;
      }

      const xml_node::node_list subscript = child->get_children();
      for(const auto& s : subscript)
      {
         const xml_element* el = GetPointer<xml_element>(s);
         if(el == nullptr)
         {
            continue;
         }
         auto nodeptr = xml_script_node_t::create(el);
         THROW_ASSERT(nodeptr, "unexpected condition");
         xml_script_node_tRef node = xml_script_node_tRef(nodeptr);
         Nodes.push_back(node);
      }
   }
}

xml_foreach_t::~xml_foreach_t()
{
   clean();
}

void xml_foreach_t::clean()
{
   Nodes.clear();
}

std::string xml_foreach_t::get_xml_name() const
{
   return TAG_FOREACH;
}

xml_nodeRef xml_foreach_t::create_xml_node() const
{
   xml_element* node = new xml_element(get_xml_name());
   NOT_YET_IMPLEMENTED();
   return xml_nodeRef(node);
}
