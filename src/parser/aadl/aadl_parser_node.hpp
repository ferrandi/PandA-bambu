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
 *              Copyright (C) 2015-2020 Politecnico di Milano
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
 * @file aadl_parser_node.hpp
 * @brief Specification of the data structure associated with a node during aadl parsing
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#ifndef AADL_PARSER_NODE_HPP
#define AADL_PARSER_NODE_HPP

/// STD include
#include <iosfwd>
#include <string>

/// STL include
#include <list>

/// utility include
#include "custom_map.hpp"

/**
 * Data associated with a node of the aadl parser
 */
struct AadlParserNode
{
   /// A string associated with this node
   std::string strg;

   /// A set of property associations
   CustomMap<std::string, std::string> property_associations;

   /// A list of feature provided by something
   std::list<std::pair<std::string, CustomMap<std::string, std::string>>> features;

   /**
    * Friend definition of the << operator.
    * @param os is the output stream
    * @param to_be_printed is the node to be printed
    */
   friend std::ostream& operator<<(std::ostream& os, const AadlParserNode& to_be_printed);
};
#endif
