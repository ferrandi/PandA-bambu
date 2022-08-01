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
 *              Copyright (C) 2022 Politecnico di Milano
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
 * @file sssp_solver.cpp
 * @brief class interface for the Parallel solver of single source shortest path problem.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef SSSP_SOLVER_HPP
#define SSSP_SOLVER_HPP

#include <map>

class sssp_solver
{
   /// internal data structure storing the constraint graph's edges
   std::map<std::pair<unsigned, unsigned>, double> g_edges;
   /// internal procedure to solve the single source shortest path problem
   bool solve_SSSP_internal(unsigned int src_sssp, std::map<unsigned int, double>& vals, bool negate_solution);

 public:
   /// add edges to the graph problem
   void add_edge(unsigned int src, unsigned int tgt, double weight)
   {
      auto key = std::make_pair(src, tgt);
      if(g_edges.find(key) == g_edges.end())
      {
         g_edges[key] = weight;
      }
      else if(g_edges.at(key) > weight)
      {
         g_edges.at(key) = weight;
      }
   }
   /// solve the SSSP problem
   /// @return true in case the problem is feasible
   bool solve_SSSP(unsigned int src_sssp, std::map<unsigned int, double>& vals);
   /// same as solve_SSSP but all vals are negated
   bool solve_SSSPNeg(unsigned int src_sssp, std::map<unsigned int, double>& vals);
};

#endif
