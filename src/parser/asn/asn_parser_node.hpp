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
 * @file asn_parser_node.hpp
 * @brief Specification of the data structure associated with a node during asn parsing
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#ifndef ASN_PARSER_NODE_HPP
#define ASN_PARSER_NODE_HPP

/// STD include
#include <iosfwd>
#include <string>

/// STL include
#include <list>

/// utility include
#include "custom_map.hpp"
#include "refcount.hpp"

REF_FORWARD_DECL(AsnType);

/**
 * Data associated with a node of the asn parser
 */
struct AsnParserNode
{
   /// A string associated with this node
   std::string strg;

   /// A asn type
   AsnTypeRef asn_type;

   /// List of pair name type
   std::list<std::pair<std::string, AsnTypeRef>> element_type_list;

   /// List of named number
   std::list<std::pair<std::string, unsigned int>> named_number_list;

   /**
    * Friend definition of the << operator.
    * @param os is the output stream
    * @param to_be_printed is the node to be printed
    */
   friend std::ostream& operator<<(std::ostream& os, const AsnParserNode& to_be_printed);
};
#endif
