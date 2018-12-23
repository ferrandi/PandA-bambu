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
 *              Copyright (c) 2017 Politecnico di Milano
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
 *
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 *
 */

// include class header
#include "Discrepancy.hpp"

// include from circuit/
#include "structural_manager.hpp"
#include "utility.hpp"

void Discrepancy::add_discrepancy_parameter(const structural_managerRef& SM, const std::string& name, const std::string& value)
{
   const std::string discr_param = SM->get_circ()->is_parameter(DISCREPANCY_PARAMETER) ? (SM->get_circ()->get_parameter(DISCREPANCY_PARAMETER) + ";") : "";

   for(const auto& p : convert_string_to_vector<std::string>(discr_param, ";"))
   {
      const std::vector<std::string> current_parameter = convert_string_to_vector<std::string>(p, "=");
      THROW_ASSERT(current_parameter.size() == 2, "expected two elements");
      if(current_parameter.at(0) == name)
      {
         if(value == current_parameter.at(1))
            return;
         THROW_ERROR("The discrepancy parameter '" + name +
                     "' has been set with (at least)"
                     " two different values: " +
                     value + " != " + current_parameter.at(1));
      }
   }
   SM->get_circ()->set_parameter(DISCREPANCY_PARAMETER, discr_param + name + "=" + value);
}
