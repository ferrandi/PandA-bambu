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
 *              Copyright (C) 2022-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
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
