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
 * @file bipartite_matching.hpp
 * @brief The class solve the bipartite weighted matching problem with the Hungarian method.
 *
 * The algorithm is based on the description available through http://community.topcoder.com/tc?module=Static&d1=tutorials&d2=hungarianAlgorithm
 * in particular it has been implemented the O(n^3) version of the algorithm.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef BIPARTITE_MATCHING_HPP
#define BIPARTITE_MATCHING_HPP

#include <cassert>
#include <limits>
#include <vector>
/**
 * Base class functor used bipartite_matching to manage the costs
 */
struct BM_cost_functor
{
   /// Destructor
   virtual ~BM_cost_functor()
   {
   }

   /**
    * This functor returns a cost given x and y
    * @param x is the index of the row
    * @param y is the index of the column
    */
   virtual int operator()(size_t x, size_t y) const = 0;

   /**
    * return the maximum cost for a given row
    * @param x is the index of the row
    */
   virtual int max_row(size_t x) const = 0;
};

template <class CostFunctor>
class bipartite_matching
{
 private:
   const size_t MINUS_ONE;
   const size_t MINUS_TWO;
   size_t num_rows;
   size_t num_cols;
   size_t nel;
   /// define labels for x and y
   std::vector<int> lx;
   std::vector<int> ly;

   /// initialize xy and yx
   std::vector<size_t> xy;
   std::vector<size_t> yx;

   std::vector<int> slack;
   std::vector<size_t> slackx;
   std::vector<bool> S;
   std::vector<bool> T;
   std::vector<size_t> prev;

   const CostFunctor& cost;

   /// this function returns w_{x,y} givent the cost vector and the actual number of columns
   int get_cost(size_t x, size_t y)
   {
      if(x < num_rows && y < num_cols)
         return cost(x, y);
      else
         return 0;
   }
   void add_to_tree(size_t x, size_t prevx)
   // x - current vertex,prevx - vertex from X before x in the alternating path,
   // so we add edges (prevx, xy[x]), (xy[x], x)
   {
      S[x] = true;                    // add x to S
      prev[x] = prevx;                // we need this when augmenting
      for(size_t y = 0; y < nel; y++) // update slacks, because we add new vertex to S
         if(lx[x] + ly[y] - get_cost(x, y) < slack[y])
         {
            slack[y] = lx[x] + ly[y] - get_cost(x, y);
            slackx[y] = x;
         }
   }
   void update_labels()
   {
      size_t x, y;
      int delta = std::numeric_limits<int>::max(); // init delta as infinity
      for(y = 0; y < nel; y++)                     // calculate delta using slack
         if(!T[y])
            delta = std::min(delta, slack[y]);
      for(x = 0; x < nel; x++) // update X labels
         if(S[x])
            lx[x] -= delta;
      for(y = 0; y < nel; y++) // update Y labels
         if(T[y])
            ly[y] += delta;
      for(y = 0; y < nel; y++) // update slack array
         if(!T[y])
            slack[y] -= delta;
   }

 public:
   /// constructor
   bipartite_matching(size_t _num_rows, size_t _num_cols, const CostFunctor& _cost)
       : MINUS_ONE(std::numeric_limits<size_t>::max()),
         MINUS_TWO(std::numeric_limits<size_t>::max() - 1),
         num_rows(_num_rows),
         num_cols(_num_cols),
         nel(std::max(_num_rows, _num_cols)),
         lx(nel),
         ly(nel, 0),
         xy(nel, MINUS_ONE),
         yx(nel, MINUS_ONE),
         slack(nel),
         slackx(nel),
         S(nel),
         T(nel),
         prev(nel),
         cost(_cost)
   {
      /// initialize lx
      /// given the cost matrix we have we just have to iterate over the row
      for(size_t i = 0; i < nel; ++i)
         lx[i] = cost.max_row(i);
   }
   std::vector<size_t>& get_solution()
   {
      return xy;
   }
   void solve_bipartite_matching()
   {
      for(size_t max_match = 0; max_match < nel; ++max_match)
      {
         size_t x, y, root = MINUS_ONE; // just counters and root vertex
         std::vector<size_t> q(nel);
         size_t wr = 0, rd = 0; // q - queue for bfs, wr,rd - write and read
         // pos in queue
         S.assign(nel, false);        // init set S
         T.assign(nel, false);        // init set T
         prev.assign(nel, MINUS_ONE); // init set prev - for the alternating tree
         for(x = 0; x < nel; x++)     // finding root of the tree
            if(xy[x] == MINUS_ONE)
            {
               q[wr++] = root = x;
               prev[x] = MINUS_TWO;
               S[x] = true;
               break;
            }

         for(y = 0; y < nel; y++) // initializing slack array
         {
            // std::cerr << "root " << root << " y " << y << std::endl;
            slack[y] = lx[root] + ly[y] - get_cost(root, y);
            slackx[y] = root;
         }
         // second part of augment() function
         bool found = false;
         while(!found) // main cycle
         {
            while(rd < wr && !found) // building tree with bfs cycle
            {
               x = q[rd++];                   // current vertex from X part
               for(y = 0; y < nel && !found;) // iterate through all edges in equality graph
               {
                  if(get_cost(x, y) == lx[x] + ly[y] && !T[y])
                  {
                     if(yx[y] == MINUS_ONE)
                     {
                        found = true; // an exposed vertex in Y found, so
                     }
                     // augmenting path exists!
                     else
                     {
                        T[y] = true;     // else just add y to T,
                        q[wr++] = yx[y]; // add vertex yx[y], which is matched
                        // with y, to the queue
                        add_to_tree(yx[y], x); // add edges (x,y) and (y,yx[y]) to the tree
                        ++y;
                     }
                  }
                  else
                     ++y;
               }
            }
            if(!found)
            {
               update_labels(); // augmenting path not found, so improve labeling
               wr = rd = 0;
               for(y = 0; y < nel && !found;)
               {
                  // in this cycle we add edges that were added to the equality graph as a
                  // result of improving the labeling, we add edge (slackx[y], y) to the tree if
                  // and only if !T[y] &&  slack[y] == 0, also with this edge we add another one
                  //(y, yx[y]) or augment the matching, if y was exposed
                  if(!T[y] && slack[y] == 0)
                  {
                     if(yx[y] == MINUS_ONE) // exposed vertex in Y found - augmenting path exists!
                     {
                        x = slackx[y];
                        found = true;
                     }
                     else
                     {
                        T[y] = true; // else just add y to T,
                        if(!S[yx[y]])
                        {
                           q[wr++] = yx[y]; // add vertex yx[y], which is matched with
                           // y, to the queue
                           add_to_tree(yx[y], slackx[y]); // and add edges (x,y) and (y,
                           // yx[y]) to the tree
                        }
                        ++y;
                     }
                  }
                  else
                     ++y;
               }
            }
         }

         assert(found);
         // in this cycle we inverse edges along augmenting path
         for(size_t cx = x, cy = y, ty; cx != MINUS_TWO; cx = prev[cx], cy = ty)
         {
            ty = xy[cx];
            yx[cy] = cx;
            xy[cx] = cy;
         }
      }
   }
};
#endif
