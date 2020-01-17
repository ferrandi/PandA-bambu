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
 * @file degree_coloring.hpp
 * @brief Boost-based implementation of a heuristic sequential coloring algorithm based on the descandant degree ordering of vertices.
 *
 * Simple sequential coloring heuristic based on descandant degree coloring.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef DEGREE_COLORING_HPP
#define DEGREE_COLORING_HPP

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
 * chosen is determined statically following the descendant degree coloring.
 *
 * The color of the vertex v will be stored in color[v].
 * i.e., vertex v belongs to coloring color[v]
 */

namespace boost
{
   /**
    * degree compare functor
    */
   template <typename VertexListGraph>
   struct degree_compare_functor
   {
    public:
      template <typename Vertex>
      bool operator()(Vertex v1, Vertex v2) const
      {
         return out_degree(v1, G) < out_degree(v2, G);
      }
      explicit degree_compare_functor(const VertexListGraph& _G) : G(_G)
      {
      }

    private:
      const VertexListGraph& G;
   };

   /**
    * coloring of a graph following the descandant degree of vertices
    */
   template <class VertexListGraph, class ColorMap>
   typename property_traits<ColorMap>::value_type degree_coloring(const VertexListGraph& G, ColorMap color)
   {
      typedef graph_traits<VertexListGraph> GraphTraits;
      typedef typename GraphTraits::vertex_descriptor Vertex;
      typedef typename property_traits<ColorMap>::value_type size_type;

      size_type max_color = 0;
      const size_type V = num_vertices(G);
      if(V == 0)
         return 0;

      // We need to keep track of which colors are used by
      // adjacent vertices. We do this by marking the colors
      // that are used. The mark array contains the mark
      // for each color. The length of mark is the
      // number of vertices since the maximum possible number of colors
      // is the number of vertices.
      std::vector<size_type> mark(V, std::numeric_limits<size_type>::max BOOST_PREVENT_MACRO_SUBSTITUTION());
      unsigned iheap = 0, heapsize;
      // Initialize colors
      typename GraphTraits::vertex_iterator v, vend;
      for(boost::tie(v, vend) = vertices(G); v != vend; ++v)
      {
         put(color, *v, V - 1);
         iheap++;
      }
      heapsize = iheap;
      Vertex* heap_container;
      heap_container = new size_type[iheap];
      for(boost::tie(v, vend) = vertices(G); v != vend; ++v)
      {
         --iheap;
         heap_container[iheap] = *v;
      }
      degree_compare_functor<VertexListGraph> HCF(G);
      std::make_heap(heap_container, heap_container + heapsize, HCF);

      // Determine the color for every vertex one by one
      for(size_type i = 0; i < heapsize; ++i)
      {
         Vertex current = heap_container[0];
         std::pop_heap(heap_container, heap_container + heapsize - i, HCF);
         typename GraphTraits::adjacency_iterator v1, v1end;

         // Mark the colors of vertices adjacent to current.
         // i can be the value for marking since i increases successively
         for(boost::tie(v1, v1end) = adjacent_vertices(current, G); v1 != v1end; ++v1)
            mark[get(color, *v1)] = i;

         // Next step is to assign the smallest un-marked color
         // to the current vertex.
         size_type j = 0;

         // Scan through all useable colors, find the smallest possible
         // color that is not used by neighbors.  Note that if mark[j]
         // is equal to i, color j is used by one of the current vertex's
         // neighbors.
         while(j < max_color && mark[j] == i)
            ++j;

         if(j == max_color) // All colors are used up. Add one more color
            ++max_color;

         // At this point, j is the smallest possible color
         put(color, current, j); // Save the color of vertex current
      }
      delete[] heap_container;
      return max_color;
   }

} // namespace boost

#endif
