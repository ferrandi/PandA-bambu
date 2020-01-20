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
 * @file xml_helper.hpp
 * @brief Some macro used to interface with the XML library.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef XML_HELPER_HPP
#define XML_HELPER_HPP

#include <boost/lexical_cast.hpp>
#include <boost/typeof/typeof.hpp> // for BOOST_TYPEOF_TPL

/// WRITE XML Value Macro. Insert a value in an XML tree.
#define WRITE_XVM(variable, node) (node)->set_attribute(#variable, boost::lexical_cast<std::string>(variable))

/// WRITE XML Name Value Macro. Insert a value in an XML tree given the name of the attribute. The name is converted in a string.
#define WRITE_XNVM(variable, value, node) (node)->set_attribute(#variable, value)

/// WRITE XML Name Value Macro second version. Insert a value in an XML tree given the name of the attribute.
#define WRITE_XNVM2(name, value, node) (node)->set_attribute(name, value)

/// WRITE XML Name Value Macro third version. Insert a value in an XML tree given the name of the attribute; it sets the
/// attribute with "value" field. It adds a child to node with variable name
#define WRITE_VALUE(variable, node) WRITE_XNVM(value, boost::lexical_cast<std::string>(variable), (node)->add_child(#variable))

/// LOAD XML Value Macro. Set a variable starting from an XML value. Conversion is performed if needed.
#define LOAD_XVM(variable, node) variable = boost::lexical_cast<BOOST_TYPEOF_TPL(variable)>((node)->get_attribute(#variable)->get_value())

/// LOAD XML Value for field Macro. Set a variable starting from an XML value. Conversion is performed if needed.
#define LOAD_XVFM(variable, node, field) variable = boost::lexical_cast<BOOST_TYPEOF_TPL(variable)>((node)->get_attribute(#field)->get_value())

/// under windows long double numbers are not correctly managed. This hack solves the problem
#define LOAD_XVM_LD(variable, node) variable = strtold(((node)->get_attribute(#variable)->get_value()).c_str(), nullptr)

/// LOAD XML Value Macro. Set a variable starting from an XML attribute composed of name and value.
/// Conversion is performed if needed.
#define LOAD_VALUE(variable, node)     \
   if((node)->get_name() == #variable) \
   variable = boost::lexical_cast<BOOST_TYPEOF_TPL(variable)>((node)->get_attribute("value")->get_value())

/// LOAD XML Value Macro. Set a variable starting from an XML attribute composed of name and value.
/// Conversion is performed if needed.
#define GET_STRING_VALUE(node) boost::lexical_cast<std::string>((node)->get_attribute("value")->get_value())

#define GET_NODE_NAME(node) ((node)->get_name())

/// Check existence XML Value Macro. Check if an XML attribute is present in the XML tree.
#define CE_XVM(variable, node) (node)->get_attribute(#variable)

#endif
