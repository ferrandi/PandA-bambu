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
 *              Copyright (C) 2022-2024 Politecnico di Milano
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
 * @file sdc_solver.cpp
 * @brief class interface for the Parallel solver of system of difference constraints.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef SDC_SOLVER_HPP
#define SDC_SOLVER_HPP

#include <map>

class sdc_solver
{
   /// internal data structure storing the constraint graph's edges
   std::map<std::pair<unsigned, unsigned>, int> constraints;
   /// internal procedure to solve the system of difference constraints
   bool solve_SDC_internal(std::map<unsigned int, int>& vals, bool negate_solution);

 public:
   /// add constraints to the SDC problem in the form V_j - V_i <= weight
   void add_constraint(unsigned int i, unsigned int j, int weight)
   {
      auto key = std::make_pair(i, j);
      if(constraints.find(key) == constraints.end())
      {
         constraints[key] = weight;
      }
      else if(constraints.at(key) > weight)
      {
         constraints.at(key) = weight;
      }
   }
   /// solve the SDC problem
   /// @return true in case the problem is feasible
   bool solve_SDC(std::map<unsigned int, int>& vals);
   /// same as solve_SDC but all vals are negated
   bool solve_SDCNeg(std::map<unsigned int, int>& vals);
};

#endif
