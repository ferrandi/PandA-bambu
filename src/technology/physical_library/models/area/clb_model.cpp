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
 * @file clb_model.hpp
 * @brief Implementation of the class clb_model, with all the methods to return information
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
/// Header include
#include "clb_model.hpp"
#include "utility.hpp"

clb_model::clb_model(const ParameterConstRef& _Param) : area_model(_Param), area(area_model::area_DEFAULT)

{
}

clb_model::clb_model(const ParameterConstRef& _Param, const double& _area_) : area_model(_Param), area(_area_)
{
}

clb_model::~clb_model() = default;

double clb_model::get_area_value() const
{
   return static_cast<double>(area);
}

void clb_model::set_area_value(const double& _area_)
{
   area = ceil(_area_);
}

bool clb_model::is_characterization(unsigned int) const
{
   return false;
}

void clb_model::set_resource_value(value_t val, double num)
{
   used_resources[val] = num;
}

bool clb_model::is_used_resource(value_t val) const
{
   return used_resources.find(val) != used_resources.end();
}

double clb_model::get_resource_value(value_t val) const
{
   if(!is_used_resource(val))
   {
      return 0;
   }
   return used_resources.find(val)->second;
}

void clb_model::print(std::ostream& os) const
{
   if(used_resources.find(LUT_FF_PAIRS) != used_resources.end())
   {
      os << "LUT/FF pairs: " << used_resources.find(LUT_FF_PAIRS)->second << std::endl;
   }
}
