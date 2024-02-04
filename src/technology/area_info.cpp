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
 *              Copyright (C) 2023-2024 Politecnico di Milano
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
 * @file area_info.cpp
 * @brief Collect information about resource area
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "area_info.hpp"
#include "exceptions.hpp"
#include "generic_device.hpp"

const double area_info::area_DEFAULT = 1.0;

area_info::area_info(const ParameterConstRef& _Param) : Param(_Param)
{
}

area_info::~area_info() = default;

area_infoRef area_info::factory(const ParameterConstRef& Param)
{
   return area_infoRef(new area_info(Param));
}

double area_info::get_area_value() const
{
   return static_cast<double>(area);
}

void area_info::set_area_value(const double& _area)
{
   area = _area;
}

void area_info::set_resource_value(value_t val, double num)
{
   used_resources[val] = num;
}

bool area_info::is_used_resource(value_t val) const
{
   return used_resources.find(val) != used_resources.end();
}

double area_info::get_resource_value(value_t val) const
{
   if(!is_used_resource(val))
   {
      return 0;
   }
   return used_resources.find(val)->second;
}

void area_info::print(std::ostream& os) const
{
   if(used_resources.find(LUT_FF_PAIRS) != used_resources.end())
   {
      os << "LUT/FF pairs: " << used_resources.find(LUT_FF_PAIRS)->second << std::endl;
   }
}
