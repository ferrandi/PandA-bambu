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
 *              Copyright (C) 2004-2026 Politecnico di Milano
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
 * @file rehashed_heap.hpp
 * @brief This package provides the class used to represent the priority queues adopted
 * by the list based algorithm.
 *
 * @defgroup Rehashed_heap Priority queue of vertices with rehash Package
 * @ingroup HLS
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef REHASHED_HEAP_HPP
#define REHASHED_HEAP_HPP

#include "custom_map.hpp"
#include "graph.hpp"
#include "priority.hpp"
#include "refcount.hpp"

#include <algorithm>
#include <queue>
#include <vector>

/**
 * Class used to represent a priority queue of vertex with rehash.
 */
template <class _Type, typename vertex_descriptor = gc_vertex_descriptor>
struct rehashed_heap
    : public std::priority_queue<vertex_descriptor, std::vector<vertex_descriptor>, priority_compare_functor<_Type>>
{
   /**
    * Constructor
    * @param _comp is the priority functor.
    */
   rehashed_heap(const priority_compare_functor<_Type>& _comp)
       : std::priority_queue<vertex_descriptor, std::vector<vertex_descriptor>, priority_compare_functor<_Type>>(_comp)
   {
   }
   /**
    * Rehash the heap associated with the priority queue.
    */
   void rehash()
   {
      std::make_heap(
          std::priority_queue<vertex_descriptor, std::vector<vertex_descriptor>, priority_compare_functor<_Type>>::c
              .begin(),
          std::priority_queue<vertex_descriptor, std::vector<vertex_descriptor>, priority_compare_functor<_Type>>::c
              .end(),
          std::priority_queue<vertex_descriptor, std::vector<vertex_descriptor>,
                              priority_compare_functor<_Type>>::comp);
   }

   auto begin()
   {
      return rehashed_heap::c.begin();
   }

   auto end()
   {
      return rehashed_heap::c.end();
   }
};

/**
 * Class used to represent a tree of priority queues.
 */
template <class _Type, typename vertex_descriptor = gc_vertex_descriptor>
struct tree_rehashed_heap
    : public CustomUnorderedMap<vertex_descriptor, std::vector<rehashed_heap<_Type, vertex_descriptor>>>
{
   /**
    * Return the vertex with the highest priority. Precondition: empty() is false.
    * @param curren_black_list is the black list, that is the set of queues not usable.
    * @param priority_functor is the priority functor.
    * @param controlling_vertex filled with the controlling vertex in case found is true.
    * @param b_tag filled with the branch tag when found is true.
    * @param found is true when there exists a queue with candidate vertices.
    */
   auto top(const CustomUnorderedMap<vertex_descriptor, CustomOrderedSet<unsigned int>>& curren_black_list,
            const priority_data<_Type>& priority_functor, vertex_descriptor controlling_vertex, unsigned int& b_tag,
            bool& found)
   {
      found = false;
      auto it_end = this->end();
      typename std::vector<rehashed_heap<_Type, vertex_descriptor>>::iterator res;
      for(auto it = this->begin(); it != it_end; ++it)
      {
         auto vit_end = it->second.end();
         unsigned int i = 0;
         bool cv_not_in_bl = curren_black_list.find(it->first) == curren_black_list.end();
         for(auto vit = it->second.begin(); vit != vit_end; ++vit, ++i)
         {
            if(!vit->empty() && (cv_not_in_bl || curren_black_list.find(it->first)->second.find(i) ==
                                                     curren_black_list.find(it->first)->second.end()))
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
      auto it_end = this->end();
      for(auto it = this->begin(); it != it_end; ++it)
      {
         auto vit_end = it->second.end();
         for(auto vit = it->second.begin(); vit != vit_end; ++vit)
         {
            vit->rehash();
         }
      }
   }
};

#endif
