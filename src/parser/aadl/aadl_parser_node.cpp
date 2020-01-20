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
 * @file aadl_parser_node.cpp
 * @brief Specification of the data structure associated with a node during aadl parsing
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Header include
#include "aadl_parser_node.hpp"
#include <ostream>

std::ostream& operator<<(std::ostream& os, const AadlParserNode& aadl_parser_node)
{
   if(!aadl_parser_node.strg.empty())
   {
      os << aadl_parser_node.strg;
   }
   if(!aadl_parser_node.property_associations.empty())
   {
      bool first = true;
      for(const auto& property_association : aadl_parser_node.property_associations)
      {
         if(not first)
         {
            os << ", ";
         }
         os << property_association.first << " => " << property_association.second;
         first = false;
      }
   }
   if(not aadl_parser_node.features.empty())
   {
      bool first_feature = true;
      for(const auto& feature : aadl_parser_node.features)
      {
         if(not first_feature)
         {
            os << " -- ";
         }
         os << feature.first << ": ";
         bool first_property = true;
         for(const auto& property : feature.second)
         {
            if(not first_property)
            {
               os << ",";
            }
            os << property.first << " => " << property.second;
            first_property = false;
         }
         first_feature = false;
      }
   }
   return os;
}
