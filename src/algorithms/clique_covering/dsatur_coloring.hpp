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
 * @file dsatur_coloring.hpp
 * @brief Boost-based implementation of a heuristic sequential coloring algorithm based on the work of Daniel Brelaz (Version 1).
 *
 * Some ideas come from the Joseph Culberson's implementation of DSATUR heuristic of Daniel Brelaz.
 * For more details see
 *  - http://web.cs.ualberta.ca/~joe/Coloring/index.html
 *  - New Methods to Color the Vertices of a Graph, Daniel Brelaz, CACM 22, 251--256, 1979.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef DSATUR_COLORING_HPP
#define DSATUR_COLORING_HPP

#include <boost/config.hpp>
#include <boost/graph/properties.hpp>
#include <boost/version.hpp>
#if BOOST_VERSION >= 104000
#include <boost/property_map/property_map.hpp>
#else
#include <boost/property_map.hpp>
#endif
#include "custom_set.hpp"
#include <algorithm>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/visitors.hpp>
#include <boost/limits.hpp>
#include <boost/tuple/tuple.hpp>
#include <vector>

/**
 * This algorithm is to find coloring of a graph
 * Algorithm:
 * Let G = (V,E) be a graph with vertices v_1, v_2, ..., v_n.
 * It is like the sequential algorithm, except that the order in which vertices are
 * chosen is determined dynamically based on the number of colors that cannot be used
 * because of conflicts with previously colored vertices. Vertices with the fewest available
 * colors are colored first.
 * Instead of a circular lists data structure based on arrays I have preferred a vector of double linked lists.
 * Reference:
 * - http://web.cs.ualberta.ca/~joe/Coloring/index.html
 * - New Methods to Color the Vertices of a Graph, Daniel Brelaz, CACM 22, 251--256, 1979.
 *
 * The color of the vertex v will be stored in color[v].
 * i.e., vertex v belongs to coloring color[v]
 */

namespace boost
{
   template <typename size_type, typename VertexListGraph>
   struct dsatur_degree_compare_functor
   {
    public:
      bool operator()(size_type v1, size_type v2) const
      {
         return degree(vertex(v1, G), G) >= degree(vertex(v2, G), G);
      }
      explicit dsatur_degree_compare_functor(const VertexListGraph& _G) : G(_G)
      {
      }

    private:
      const VertexListGraph& G;
   };

   template <typename size_type, typename VertexListGraph>
   struct dsatur_degree_predicate_functor
   {
    public:
      bool operator()(size_type v1) const
      {
         return degree(vertex(v1, G), G) < v2_degree;
      }
      dsatur_degree_predicate_functor(const VertexListGraph& _G, size_type& _v2) : G(_G), v2_degree(degree(vertex(_v2, _G), _G))
      {
      }

    private:
      const VertexListGraph& G;
      size_type v2_degree;
   };

   template <typename size_type>
   struct dsatur_d_list
   {
      size_type data;
      dsatur_d_list* prev;
      dsatur_d_list* next;
      explicit dsatur_d_list(size_type _data) : data(_data), prev(0), next(0)
      {
      }
   };

   template <typename VertexListGraph, typename ColorMap, typename size_type>
   class dsatur_coloring_helper
   {
    private:
      typedef graph_traits<VertexListGraph> GraphTraits;
      typedef typename GraphTraits::vertex_descriptor Vertex;
      const size_type num_node;
      std::vector<dsatur_d_list<size_type>*> satur_to_list;
      std::vector<size_type> vertex_to_satur;
      typename std::vector<dsatur_d_list<size_type>*> vertex_to_iter;
      std::vector<CustomUnorderedSet<size_type>> color_set;
      const VertexListGraph& G;
      ColorMap& CM;

    public:
      dsatur_coloring_helper(const VertexListGraph& _G, ColorMap& _CM, const size_type _num_node) : num_node(_num_node), satur_to_list(1), vertex_to_satur(_num_node, 0), vertex_to_iter(_num_node), color_set(_num_node), G(_G), CM(_CM)
      {
         std::vector<size_type> tmp_vertex_degree_ordering(_num_node);
         for(size_type i = 0; i < _num_node; i++)
            tmp_vertex_degree_ordering[i] = i;
         std::stable_sort(tmp_vertex_degree_ordering.begin(), tmp_vertex_degree_ordering.end(), dsatur_degree_compare_functor<size_type, VertexListGraph>(_G));
         dsatur_d_list<size_type>* last = 0;
         for(size_type i = 0; i < _num_node; i++)
         {
            size_type curr_vert = tmp_vertex_degree_ordering[i];
            dsatur_d_list<size_type>* dobj = new dsatur_d_list<size_type>(curr_vert);
            if(!last)
               last = satur_to_list[0] = dobj;
            else
            {
               last->next = dobj;
               dobj->prev = last;
               last = dobj;
            }
            vertex_to_iter[curr_vert] = dobj;
         }
      }

      size_type SeqColor()
      {
         size_type maxsatur = 0;
         size_type maxclr = 0;
         for(size_type i = 0; i < num_node; i++) /// color each vertex
         {
            /// scan for maximum saturation list
            while(!satur_to_list[maxsatur])
               maxsatur--;
            /// v is vertex to color, remove from list
            size_type v = satur_to_list[maxsatur]->data;
            dsatur_d_list<size_type>* to_be_deleted = satur_to_list[maxsatur];
            satur_to_list[maxsatur] = satur_to_list[maxsatur]->next;
            delete to_be_deleted;
            if(satur_to_list[maxsatur])
               satur_to_list[maxsatur]->prev = 0;
            vertex_to_iter[v] = 0;
            /// assign minimum color not adjacent to v
            size_type color = 0;
            while(color_set[v].find(color) != color_set[v].end())
               color++;
            color_set[v].insert(color);
            if(maxclr < color)
               maxclr = color;
            put(CM, vertex(v, G), color);
            /// update neighbors saturation
            typename GraphTraits::adjacency_iterator vit, vend;
            for(boost::tie(vit, vend) = adjacent_vertices(vertex(v, G), G); vit != vend; ++vit)
            {
               size_type w = get(vertex_index, G, *vit);
               if(vertex_to_iter[w])
               {
                  /// color not previously adjacent to w
                  if(color_set[w].find(color) == color_set[w].end())
                  {
                     /// mark color in colorset
                     color_set[w].insert(color);

                     /// remove vertex from satur list
                     size_type w_satur = vertex_to_satur[w];
                     dsatur_d_list<size_type>* vertex_to_be_moved = vertex_to_iter[w];
                     if(vertex_to_be_moved->prev == 0)
                     {
                        satur_to_list[w_satur] = vertex_to_be_moved->next;
                        if(vertex_to_be_moved->next)
                           satur_to_list[w_satur]->prev = 0;
                     }
                     else if(vertex_to_be_moved->next == 0)
                     {
                        vertex_to_be_moved->prev->next = 0;
                     }
                     else
                     {
                        vertex_to_be_moved->prev->next = vertex_to_be_moved->next;
                        vertex_to_be_moved->next->prev = vertex_to_be_moved->prev;
                     }
                     /// increase saturation and check max
                     w_satur++;
                     vertex_to_satur[w] = w_satur;
                     if(maxsatur < w_satur)
                     {
                        maxsatur = w_satur;
                        satur_to_list.resize(maxsatur + 1); /// add a new  empty list
                        satur_to_list[maxsatur] = 0;
                     }
                     /// insert w into new list
                     dsatur_degree_predicate_functor<size_type, VertexListGraph> DPF(G, w);
                     if(satur_to_list[w_satur] == 0)
                     {
                        satur_to_list[w_satur] = vertex_to_be_moved;
                        vertex_to_be_moved->prev = 0;
                        vertex_to_be_moved->next = 0;
                     }
                     else if(DPF(satur_to_list[w_satur]->data))
                     {
                        vertex_to_be_moved->prev = 0;
                        vertex_to_be_moved->next = satur_to_list[w_satur];
                        satur_to_list[w_satur]->prev = vertex_to_be_moved;
                        satur_to_list[w_satur] = vertex_to_be_moved;
                     }
                     else
                     {
                        dsatur_d_list<size_type>* iter = satur_to_list[w_satur];
                        while(iter->next && !DPF(iter->next->data))
                           iter = iter->next;

                        vertex_to_be_moved->next = iter->next;
                        if(iter->next)
                           iter->next->prev = vertex_to_be_moved;
                        iter->next = vertex_to_be_moved;
                        vertex_to_be_moved->prev = iter;
                     }
                  }
               }
            }
         }
         return maxclr + 1;
      }
   };
   /**
    * coloring of a graph following the DSATUR heuristic (version1)
    */
   template <typename VertexListGraph, typename ColorMap>
   typename property_traits<ColorMap>::value_type dsatur_coloring(const VertexListGraph& G, ColorMap color)
   {
      typedef graph_traits<VertexListGraph> GraphTraits;
      typedef typename GraphTraits::vertex_descriptor Vertex;
      typedef typename property_traits<ColorMap>::value_type size_type;

      const size_type num_node = num_vertices(G);
      if(num_node == 0)
         return 0;
      dsatur_coloring_helper<VertexListGraph, ColorMap, size_type> DCH(G, color, num_node);

      return DCH.SeqColor();
   }
} // namespace boost

#endif
