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
 * @file liberty_model.hpp
 * @brief Class implementation
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "liberty_model.hpp"

#include "polixml.hpp"
#include "timing_group.hpp"

liberty_model::liberty_model(const ParameterConstRef _Param) : time_model(_Param), drive_strength(1), skew(1)
{
}

liberty_model::~liberty_model() = default;

void liberty_model::xwrite(xml_element* pin_node, const std::string& output_pin)
{
   if(timing_groups.find(output_pin) != timing_groups.end())
   {
      for(auto& g : timing_groups[output_pin])
      {
         xml_element* group = pin_node->add_child_element("timing");
         g.second->xwrite(group, g.first);
      }
   }
}

void liberty_model::add_timing_group(const std::string& output, const CustomOrderedSet<std::string>& inputs, const timing_groupRef& tg)
{
   timing_groups[output][inputs] = tg;
}

void liberty_model::set_drive_strength(double value)
{
   drive_strength = value;
}

double liberty_model::get_drive_strength() const
{
   return drive_strength;
}

void liberty_model::set_skew_value(double value)
{
   skew = value;
}

double liberty_model::get_skew_value() const
{
   return skew;
}

void liberty_model::set_timing_groups(const std::map<std::string, std::map<CustomOrderedSet<std::string>, timing_groupRef>>& timing_groups_)
{
   timing_groups = timing_groups_;
}
