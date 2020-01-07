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
 * @file timing_group.hpp
 * @brief Class implementation for base timing group
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "timing_group.hpp"

#include "library_manager.hpp"

#include "polixml.hpp"

timing_group::timing_group() : intrinsic_rise(0.0), intrinsic_fall(0.0)
{
}

void timing_group::xwrite(xml_element* pin_node, const CustomOrderedSet<std::string>& input_set)
{
   attributeRef att;

   att = attributeRef(new attribute(attribute::FLOAT64, boost::lexical_cast<std::string>(intrinsic_rise)));
   att->xwrite(pin_node, "intrinsic_rise");

   att = attributeRef(new attribute(attribute::FLOAT64, boost::lexical_cast<std::string>(intrinsic_fall)));
   att->xwrite(pin_node, "intrinsic_fall");

   std::string pins;
   for(const auto& i : input_set)
   {
      if(!pins.empty())
      {
         pins += " ";
      }
      pins += i;
   }
   att = attributeRef(new attribute(attribute::STRING, pins));
   att->xwrite(pin_node, "related_pin");
}
