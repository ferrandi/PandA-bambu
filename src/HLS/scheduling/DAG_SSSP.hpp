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
 *              Copyright (C) 2014-2022 Politecnico di Milano
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
 * @file DAG_SSSP.hpp
 * @brief Delay calculator based on single-source shortest path algorithm working on DAG.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef DAG_SSSP_HPP
#define DAG_SSSP_HPP

#include <limits>
#include <list>
#include <map>
#include <vector>

class DAG_SSSP
{
   /// graph adjacency list
   std::vector<std::list<unsigned int>> gal;
   /// visited vector
   std::vector<bool> visited;
   /// topological sorted vertices
   std::vector<unsigned int> ordered;
   /// weights
   std::map<std::pair<unsigned int, unsigned int>, double> edge_weight;

   void DFS_topological_sort(unsigned int src)
   {
      visited.at(src) = true;
      for(auto v : gal.at(src))
      {
         if(!visited.at(v))
         {
            DFS_topological_sort(v);
         }
      }
      ordered.push_back(src);
   }

 public:
   DAG_SSSP() = delete;

   DAG_SSSP(size_t nodes)
   {
      gal.resize(nodes);
      visited.resize(nodes, false);
   }

   void add_edge(unsigned int src, unsigned int tgt, double weight)
   {
      edge_weight[std::make_pair(src, tgt)] = weight;
      gal.at(src).push_back(tgt);
   }

   void init()
   {
      auto n_nodes = gal.size();
      for(unsigned int i = 0; i < n_nodes; i++)
      {
         if(!visited.at(i))
         {
            DFS_topological_sort(i);
         }
      }
   }

   void exec(unsigned int source_node, std::vector<double>& dist)
   {
      auto n_nodes = gal.size();
      auto max_value = std::numeric_limits<double>::max();

      dist.clear();
      dist.resize(n_nodes, max_value);
      dist[source_node] = 0;

      for(auto it = ordered.rbegin(); it != ordered.rend(); ++it)
      {
         auto u = *it;
         auto dist_curr = dist.at(u);
         if(dist_curr != max_value)
         {
            for(auto v : gal.at(u))
            {
               auto weight = edge_weight.find(std::make_pair(u, v))->second;
               if(dist.at(v) > dist_curr + weight)
               {
                  dist.at(v) = dist_curr + weight;
               }
            }
         }
      }
   }
};

#endif
