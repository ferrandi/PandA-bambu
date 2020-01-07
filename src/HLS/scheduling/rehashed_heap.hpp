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
 * @file rehashed_heap.hpp
 * @brief This package provides the class used to represent the priority queues adopted
 * by the list based algorithm.
 *
 * @defgroup Rehashed_heap Priority queue of vertices with rehash Package
 * @ingroup HLS
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef REHASHED_HEAP_HPP
#define REHASHED_HEAP_HPP

#include <algorithm>
#include <queue>
#include <vector>

#include "custom_map.hpp"
#include "graph.hpp"
#include "priority.hpp"
#include "refcount.hpp"

/**
 * @name Forward declarations.
 */
//@{
//@}

/**
 * Class used to represent a priority queue of vertex with rehash.
 */
template <class _Type>
struct rehashed_heap : public std::priority_queue<vertex, std::vector<vertex>, priority_compare_functor<_Type>>
{
   /**
    * Constructor
    * @param pri is the priority functor.
    */
   rehashed_heap(const priority_compare_functor<_Type>& _comp) : std::priority_queue<vertex, std::vector<vertex>, priority_compare_functor<_Type>>(_comp)
   {
   }
   /**
    * Rehash the heap associated with the priority queue.
    */
   void rehash()
   {
      std::make_heap(std::priority_queue<vertex, std::vector<vertex>, priority_compare_functor<_Type>>::c.begin(), std::priority_queue<vertex, std::vector<vertex>, priority_compare_functor<_Type>>::c.end(),
                     std::priority_queue<vertex, std::vector<vertex>, priority_compare_functor<_Type>>::comp);
   }

   std::vector<vertex>::const_iterator begin()
   {
      return rehashed_heap::c.begin();
   }
   std::vector<vertex>::const_iterator end()
   {
      return rehashed_heap::c.end();
   }
};

/**
 * Class used to represent a tree of priority queues.
 */
template <class _Type>
struct tree_rehashed_heap : public CustomUnorderedMap<vertex, std::vector<rehashed_heap<_Type>>>
{
   /**
    * Return the vertex with the highest priority. Precondition: empty() is false.
    * @param curren_black_list is the black list, that is the set of queues not usable.
    * @param priority_functor is the priority functor.
    * @param controlling_vertex filled with the controlling vertex in case found is true.
    * @param b_tag filled with the branch tag when found is true.
    * @param found is true when there exists a queue with candidate vertices.
    */
   typename std::vector<rehashed_heap<_Type>>::iterator top(const CustomUnorderedMap<vertex, CustomOrderedSet<unsigned int>>& curren_black_list, const priority_data<_Type>& priority_functor, vertex& controlling_vertex, unsigned int& b_tag, bool& found)
   {
      found = false;
      typename std::vector<rehashed_heap<_Type>>::iterator res;
      typename tree_rehashed_heap::const_iterator it_end = this->end();
      for(typename tree_rehashed_heap::iterator it = this->begin(); it != it_end; ++it)
      {
         typename std::vector<rehashed_heap<_Type>>::const_iterator vit_end = it->second.end();
         unsigned int i = 0;
         bool cv_not_in_bl = curren_black_list.find(it->first) == curren_black_list.end();
         for(typename std::vector<rehashed_heap<_Type>>::iterator vit = it->second.begin(); vit != vit_end; ++vit, ++i)
         {
            if(!vit->empty() && (cv_not_in_bl || curren_black_list.find(it->first)->second.find(i) == curren_black_list.find(it->first)->second.end()))
            {
               if(!found)
               {
                  found = true;
                  res = vit;
                  controlling_vertex = it->first;
                  b_tag = i;
               }
               else if(priority_functor(vit->top()) > priority_functor(res->top()))
               {
                  res = vit;
                  controlling_vertex = it->first;
                  b_tag = i;
               }
            }
         }
      }
      return res;
   }

   /**
    * Rehash all the heaps in the map.
    */
   void rehash()
   {
      typename tree_rehashed_heap::iterator it_end = this->end();
      for(typename tree_rehashed_heap::iterator it = this->begin(); it != it_end; ++it)
      {
         typename std::vector<rehashed_heap<_Type>>::iterator vit_end = it->second.end();
         for(typename std::vector<rehashed_heap<_Type>>::iterator vit = it->second.begin(); vit != vit_end; ++vit)
         {
            vit->rehash();
         }
      }
   }
};

#endif
