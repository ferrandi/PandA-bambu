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
 * @file xml_helper.hpp
 * @brief Some macro used to interface with the XML library.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef XML_HELPER_HPP
#define XML_HELPER_HPP

#include "string_manipulation.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/typeof/typeof.hpp>

/// WRITE XML Value Macro. Insert a value in an XML tree.
#define WRITE_XVM(variable, node) (node)->set_attribute(#variable, STR(variable))

/// WRITE XML Name Value Macro. Insert a value in an XML tree given the name of the attribute. The name is converted in
/// a string.
#define WRITE_XNVM(variable, value, node) (node)->set_attribute(#variable, value)

/// WRITE XML Name Value Macro second version. Insert a value in an XML tree given the name of the attribute.
#define WRITE_XNVM2(name, value, node) (node)->set_attribute(name, value)

/// WRITE XML Name Value Macro third version. Insert a value in an XML tree given the name of the attribute; it sets the
/// attribute with "value" field. It adds a child to node with variable name
#define WRITE_VALUE(variable, node) WRITE_XNVM(value, STR(variable), (node)->add_child(#variable))

/// LOAD XML Value Macro. Set a variable starting from an XML value. Conversion is performed if needed.
#define LOAD_XVM(variable, node) \
   variable = boost::lexical_cast<BOOST_TYPEOF_TPL(variable)>((node)->get_attribute(#variable)->get_value())

/// LOAD XML Value for field Macro. Set a variable starting from an XML value. Conversion is performed if needed.
#define LOAD_XVFM(variable, node, field) \
   variable = boost::lexical_cast<BOOST_TYPEOF_TPL(variable)>((node)->get_attribute(#field)->get_value())

/// under windows long double numbers are not correctly managed. This hack solves the problem
#define LOAD_XVM_LD(variable, node) variable = strtold(((node)->get_attribute(#variable)->get_value()).c_str(), nullptr)

/// LOAD XML Value Macro. Set a variable starting from an XML attribute composed of name and value.
/// Conversion is performed if needed.
#define LOAD_VALUE(variable, node)     \
   if((node)->get_name() == #variable) \
   (variable) = boost::lexical_cast<BOOST_TYPEOF_TPL(variable)>((node)->get_attribute("value")->get_value())

/// LOAD XML Value Macro. Set a variable starting from an XML attribute composed of name and value.
/// Conversion is performed if needed.
#define GET_STRING_VALUE(node) STR((node)->get_attribute("value")->get_value())

#define GET_NODE_NAME(node) ((node)->get_name())

/// Check existence XML Value Macro. Check if an XML attribute is present in the XML tree.
#define CE_XVM(variable, node) (node)->get_attribute(#variable)

#endif
