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
 * @file xml_script_command.hpp
 * @brief Classes for handling configuration files.
 *
 * This file contains the interfaces for the classes used by synthesis tools when handling
 * script generator configuration files.
 *
 * @author Andrea Zoppi <texzk@email.it>
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef _XML_SCRIPT_COMMAND_HPP_
#define _XML_SCRIPT_COMMAND_HPP_

#include <vector>

#include "refcount.hpp"

#include "polixml.hpp"
#include "xml_helper.hpp"

#include "DesignParameters.hpp"

REF_FORWARD_DECL(xml_script_node_t);
REF_FORWARD_DECL(xml_set_variable_t);
REF_FORWARD_DECL(xml_set_entry_t);
REF_FORWARD_DECL(xml_parameter_t);
REF_FORWARD_DECL(xml_command_t);
REF_FORWARD_DECL(xml_shell_t);
REF_FORWARD_DECL(xml_ite_block_t);

// Tags of XML nodes
#define TAG_VARIABLE "set"
#define TAG_ENTRY "entry"
#define TAG_PARAMETER "param"
#define TAG_COMMAND "cmd"
#define TAG_SHELL "sh"
#define TAG_ITE_BLOCK "if"
#define TAG_FOREACH "foreach"

/// Node types.
typedef enum
{
   NODE_UNKNOWN = 0,
   NODE_ENTRY = 1,
   NODE_VARIABLE = 2,
   NODE_PARAMETER = 3,
   NODE_COMMAND = 4,
   NODE_SHELL = 5,
   NODE_ITE_BLOCK = 6,
   NODE_FOREACH = 7
} xml_script_node_enum_t;

/** \class xml_script_node_t
 * This is the abstract class which describes a generic synthesis script node,
 * and some static helper methods.
 */
class xml_script_node_t
{
 public:
   xml_script_node_enum_t nodeType;

   /// Gets the XML element name of this node type.
   virtual std::string get_xml_name() const = 0;
   /// Creates an XML node for polixml data structures.
   virtual xml_nodeRef create_xml_node() const = 0;

   /// Cleans object attributes.
   virtual void clean() = 0;

   /** If the node has a compile-time condition, this method evaluates it.
    * @param dp Design parameters, used to check conditions at compile time.
    * @return The condition evaluation if any, otherwise is true by default.
    */
   virtual bool checkCondition(const DesignParametersRef& dp) const;

   /** Finds the type of an XML element.
    * @param element XML element to be parsed.
    * @return Node type (see xml_script_node_enum_t).
    */
   static xml_script_node_enum_t find_type(const xml_element* element);
   /** Creates a script node by parsing the XML element.
    * @param element XML element to be parsed.
    * @return Script node.
    */
   static xml_script_node_t* create(const xml_element* element);

   /** Evaluates a string condition.
    * @param condition Condition to be evaluated.
    * @return false <=> condition != NULL && (
    *    trim(*condition) == "" ||
    *    (float)*condition == "0.0" ||
    *    tolowercase(trim(*condition)) == "false"
    * )
    */
   static bool evaluate_condition(const std::string* condition);
   /** Evaluates a string condition. If the condition is a design parameter,
    * its string value is evaluated.
    * @param condition Condition to be evaluated.
    * @param dp Design parameters.
    * @return
    *    (trim(*condition) matches /${__.*__}/)
    *    ?  evaluate_condition(value of dp[variable of *condition])
    *    :  evaluate_condition(condition)
    * )
    */
   static bool evaluate_condition(const std::string* condition, const DesignParametersRef& dp);

   explicit xml_script_node_t(xml_script_node_enum_t _type) : nodeType(_type)
   {
   }
   virtual ~xml_script_node_t();
};
typedef refcount<xml_script_node_t> xml_script_node_tRef;

/** \class xml_set_entry_t
 * String entry of a multiple values variable (set).
 */
class xml_set_entry_t : public xml_script_node_t
{
 public:
   std::string value;
   std::string* condition;

   xml_set_entry_t(std::string _value, const std::string* _condition);
   explicit xml_set_entry_t(const xml_element* element);

   ~xml_set_entry_t() override;

   std::string get_xml_name() const override;
   xml_nodeRef create_xml_node() const override;

   void clean() override;

   bool checkCondition(const DesignParametersRef& dp) const override;
};
typedef refcount<xml_set_entry_t> xml_set_entry_tRef;

/** \class xml_set_variable_t
 * Variable assignment, either single value or multiple entries set.
 */
class xml_set_variable_t : public xml_script_node_t
{
 public:
   std::string name;
   std::string* singleValue;
   std::vector<xml_set_entry_tRef> multiValues;
   std::string* condition;

   xml_set_variable_t(std::string _name, const std::string* _singleValue, const std::string* _condition);
   explicit xml_set_variable_t(const xml_element* element);

   ~xml_set_variable_t() override;

   std::string get_xml_name() const override;
   xml_nodeRef create_xml_node() const override;

   void clean() override;

   bool checkCondition(const DesignParametersRef& dp) const override;
};
typedef refcount<xml_set_variable_t> xml_set_variable_tRef;

/** \class xml_parameter_t
 * Command line parameter. Just like a variable, it can be either a single
 * value or a set of entries.
 */
class xml_parameter_t : public xml_script_node_t
{
 public:
   std::string* name;
   std::string* singleValue;
   std::vector<xml_set_entry_tRef> multiValues;
   std::string* condition;
   std::string separator;
   bool curlyBrackets;

   xml_parameter_t(const std::string* _name, const std::string* _singleValue, const std::string* _condition, const std::string& _separator, bool _curlyBrackets);
   explicit xml_parameter_t(const xml_element* element);

   ~xml_parameter_t() override;

   std::string get_xml_name() const override;
   xml_nodeRef create_xml_node() const override;

   void clean() override;

   bool checkCondition(const DesignParametersRef& dp) const override;
};
typedef refcount<xml_parameter_t> xml_parameter_tRef;

/** \class xml_command_t
 * Command line of the synthesis tool.
 */
class xml_command_t : public xml_script_node_t
{
 public:
   std::string* name;
   std::string* value;
   std::vector<xml_parameter_tRef> parameters;
   std::string* condition;
   std::string* output;

   xml_command_t(const std::string* _name, const std::string* _value, const std::string* _condition, const std::string* _output);
   explicit xml_command_t(const xml_element* element);

   ~xml_command_t() override;

   std::string get_xml_name() const override;
   xml_nodeRef create_xml_node() const override;

   void clean() override;

   bool checkCondition(const DesignParametersRef& dp) const override;
};
typedef refcount<xml_command_t> xml_command_tRef;

/** \class xml_shell_t
 * Command line of the native shell.
 */
class xml_shell_t : public xml_script_node_t
{
 public:
   std::string* name;
   std::string* value;
   std::vector<xml_parameter_tRef> parameters;
   std::string* condition;
   std::string* output;

   xml_shell_t(const std::string* _name, const std::string* _value, const std::string* _condition, const std::string* _output);
   explicit xml_shell_t(const xml_element* element);

   ~xml_shell_t() override;

   std::string get_xml_name() const override;
   xml_nodeRef create_xml_node() const override;

   void clean() override;

   bool checkCondition(const DesignParametersRef& dp) const override;
};
typedef refcount<xml_shell_t> xml_shell_tRef;

/** \class xml_ite_block_t
 * If/Then/Else block, evaluated at compile-time.
 */
class xml_ite_block_t : public xml_script_node_t
{
 public:
   std::string condition;
   std::vector<xml_script_node_tRef> thenNodes;
   std::vector<xml_script_node_tRef> elseNodes;

   explicit xml_ite_block_t(const std::string* _condition);
   explicit xml_ite_block_t(const xml_element* element);

   std::string get_xml_name() const override;
   xml_nodeRef create_xml_node() const override;

   ~xml_ite_block_t() override;

   void clean() override;

   bool checkCondition(const DesignParametersRef& dp) const override;
};
typedef refcount<xml_ite_block_t> xml_ite_block_tRef;

/** \class xml_foreach_t
 * Foreach block, where the set of script nodes is applied to each parameter
 */
struct xml_foreach_t : public xml_script_node_t
{
 public:
   std::string variable;
   std::vector<xml_script_node_tRef> Nodes;

   explicit xml_foreach_t(std::string _variable);
   explicit xml_foreach_t(const xml_element* element);

   std::string get_xml_name() const override;
   xml_nodeRef create_xml_node() const override;

   ~xml_foreach_t() override;

   void clean() override;
};
typedef refcount<xml_foreach_t> xml_foreach_tRef;

#endif
