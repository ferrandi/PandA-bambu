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
 * @file LUT_model.hpp
 * @brief Class implementation
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "LUT_model.hpp"

#include "polixml.hpp"

LUT_model::LUT_model(const ParameterConstRef _Param) : time_model(_Param)
{
}

LUT_model::~LUT_model() = default;

void LUT_model::xwrite(xml_element*, const std::string&)
{
}

void LUT_model::set_timing_value(value_t val, double num)
{
   timing_results[val] = num;
   execution_time = num;
}

bool LUT_model::is_timing_value(value_t val) const
{
   return timing_results.find(val) != timing_results.end();
}

double LUT_model::get_timing_value(value_t val) const
{
   if(!is_timing_value(val))
      return 0;
   return timing_results.find(val)->second;
}
