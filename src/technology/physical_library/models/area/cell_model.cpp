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
 * @file cell_model.hpp
 * @brief Class specification for cell_model
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "cell_model.hpp"

#include "library_manager.hpp"
#include "technology_node.hpp"

#include "polixml.hpp"

#include "Parameter.hpp"
#include "utility.hpp"

#include <cmath>

cell_model::cell_model(const ParameterConstRef& _Param) : area_model(_Param), area(area_model::area_DEFAULT)
{
}

cell_model::cell_model(const ParameterConstRef& _Param, const double& _area_) : area_model(_Param), area(_area_)
{
}

cell_model::~cell_model() = default;

void cell_model::print(std::ostream& os) const
{
   os << "Area value: " << area << std::endl;
}

double cell_model::get_area_value() const
{
   return area;
}

void cell_model::set_area_value(const double& _area_)
{
   area = _area_;
}

bool cell_model::is_characterization(unsigned int characterization_type) const
{
   return characterization.find(characterization_type) != characterization.end();
}

bool cell_model::has_element_characterization(unsigned int characterization_type, unsigned int element_type) const
{
   return is_characterization(characterization_type) and characterization.find(characterization_type)->second.find(element_type) != characterization.find(characterization_type)->second.end();
}

double cell_model::get_characterization(unsigned int characterization_type, unsigned int element_type) const
{
   THROW_ASSERT(is_characterization(characterization_type), "Missing characterization");
   THROW_ASSERT(has_element_characterization(characterization_type, element_type), "Missing characterization");
   return characterization.find(characterization_type)->second.find(element_type)->second;
}

void cell_model::set_characterization(unsigned int characterization_type, unsigned int element_type, double value)
{
   characterization[characterization_type][element_type] = value;
}
