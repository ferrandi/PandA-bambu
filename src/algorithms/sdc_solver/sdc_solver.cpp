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
 * @file sdc_solver.cpp
 * @brief Parallel solver of system of difference constraints exploiting a parallel General Weight SSSP (Bellman-Ford)
 * adapted from GBBS/ParlayLib libraries (see https://paralg.github.io/gbbs/ and https://github.com/ParAlg/parlaylib)
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#include "sdc_solver.hpp"

#include "gbbs/edge_map_data.h"
#include "gbbs/graph.h"
#include "gbbs/vertex_subset.h"
#include <set>

namespace gbbs
{
   template <class W, class Distance>
   struct BF_F
   {
      Distance* SP;
      intE* Visited;
      BF_F(Distance* _SP, intE* _Visited) : SP(_SP), Visited(_Visited)
      {
      }
      inline bool update(const uintE& s, const uintE& d, const W& edgeLen)
      {
         Distance newDist;
         if constexpr(std::is_same<W, gbbs::empty>())
         {
            newDist = SP[s] + 1;
         }
         else
         {
            newDist = SP[s] + edgeLen;
         }
         if(SP[d] > newDist)
         {
            SP[d] = newDist;
            if(Visited[d] == 0)
            {
               Visited[d] = 1;
               return 1;
            }
         }
         return 0;
      }
      inline bool updateAtomic(const uintE& s, const uintE& d, const W& edgeLen)
      {
         Distance newDist;
         if constexpr(std::is_same<W, gbbs::empty>())
         {
            newDist = SP[s] + 1;
         }
         else
         {
            newDist = SP[s] + edgeLen;
         }
         return (gbbs::write_min(&SP[d], newDist) && gbbs::atomic_compare_and_swap(&Visited[d], 0, 1));
      }
      inline bool cond(uintE d)
      {
         return cond_true(d);
      }
   };

   // reset visited vertices
   struct BF_Vertex_F
   {
      intE* Visited;
      BF_Vertex_F(intE* _Visited) : Visited(_Visited)
      {
      }
      inline bool operator()(uintE i)
      {
         Visited[i] = 0;
         return 1;
      }
   };

   template <class Graph>
   auto BellmanFord(Graph& G, uintE start, bool& res)
   {
      res = true;
      using W = typename Graph::weight_type;
      using Distance = typename std::conditional<std::is_same<W, gbbs::empty>::value, uintE, W>::type;

      size_t n = G.n;
      auto Visited = sequence<int>(n, 0);
      auto SP = sequence<Distance>(n, std::numeric_limits<Distance>::max());
      SP[start] = 0;

      vertexSubset Frontier(n, start);
      size_t round = 0;
      while(!Frontier.isEmpty())
      {
         // Check for a negative weight cycle
         if(round == n)
         {
            std::cout << " Found negative weight cycle." << std::endl;
            res = false;
            return SP;
         }
         auto em_f = BF_F<W, Distance>(SP.begin(), Visited.begin());
         auto output = edgeMap(G, Frontier, em_f, G.m / 10, sparse_blocked | dense_forward);
         vertexMap(output, BF_Vertex_F(Visited.begin()));
         std::cout << output.size() << "\n";
         Frontier = std::move(output);
         round++;
      }
      auto dist_im_f = [&](size_t i) { return (SP[i] == (std::numeric_limits<Distance>::max())) ? 0 : SP[i]; };
      auto dist_im = parlay::delayed_seq<Distance>(n, dist_im_f);
      std::cout << "max dist = " << parlay::reduce_max(dist_im) << "\n";
      std::cout << "n rounds = " << round << "\n";
      return SP;
   }

} // namespace gbbs

bool sdc_solver::solve_SDC(std::map<unsigned int, int>& vals)
{
   /// prepare the data structure
   using edge = std::tuple<gbbs::uintE, gbbs::uintE, gbbs::intE>;
   gbbs::sequence<edge> edges;
   std::set<unsigned> visited;
   unsigned max_vertex_id = 0;
   for(auto t : constraints)
   {
      auto src = std::get<0>(t);
      max_vertex_id = std::max(src, max_vertex_id);
      auto tgt = std::get<1>(t);
      max_vertex_id = std::max(tgt, max_vertex_id);
      auto w = std::get<2>(t);
      if(visited.find(src) == visited.end())
      {
         visited.insert(src);
         edges.push_back(std::make_tuple(0, src + 1, 0));
      }
      if(visited.find(tgt) == visited.end())
      {
         visited.insert(tgt);
         edges.push_back(std::make_tuple(0, tgt + 1, 0));
      }
      edges.push_back(std::make_tuple(src + 1, tgt + 1, w));
   }
   assert(visited.size() == max_vertex_id + 1);
   auto G = gbbs::asym_graph_from_edges(edges, visited.size(), /* is_sorted = */ false);
   bool res = true;
   auto distances = gbbs::BellmanFord(G, 0, res);
   if(!res)
   {
      return res;
   }
   unsigned i = 0;
   vals.clear();
   for(auto dist : distances)
   {
      std::cout << "V" << i << "=" << dist << "\n";
      if(i != 0)
      {
         vals[i - 1] = -dist;
      }
      ++i;
   }
   return true;
}
