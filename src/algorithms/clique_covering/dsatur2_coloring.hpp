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
 * @file dsatur2_coloring.hpp
 * @brief Boost-based implementation of a heuristic sequential coloring algorithm based on the work of Daniel Brelaz (Version 2).
 *
 * Some ideas come from the Michael A. Trick's implementation of DSATUR heuristic of Daniel Brelaz.
 * For more details see
 * - http://mat.gsia.cmu.edu/COLOR/color.html
 * - New Methods to Color the Vertices of a Graph, Daniel Brelaz, CACM 22, 251--256, 1979.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef DSATUR2_COLORING_HPP
#define DSATUR2_COLORING_HPP

#include <boost/config.hpp>
#include <boost/graph/properties.hpp>
#include <boost/version.hpp>
#if BOOST_VERSION >= 104000
#include <boost/property_map/property_map.hpp>
#else
#include <boost/property_map.hpp>
#endif
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/visitors.hpp>
#include <boost/limits.hpp>
#include <boost/tuple/tuple.hpp>
#if BOOST_VERSION >= 106400
#include <boost/serialization/array_wrapper.hpp>
#endif
#include <algorithm>
#include <boost/numeric/ublas/matrix.hpp>
#include <cassert>
#include <vector>

/**
 * This algorithm is to find coloring of a graph
 * Algorithm:
 * Let G = (V,E) be a graph with vertices v_1, v_2, ..., v_n.
 * It is like the sequential algorithm, except that the order in which vertices are
 * chosen is determined dynamically based on the number of colors that cannot be used
 * because of conflicts with previously colored vertices. Vertices with the fewest available
 * colors are colored first.
 * The degree of vertices is dynamically modified during the coloring (as in the Michael A. Trick's
 * implementation) while the order of vertices is based on a heap data structure.
 * This implementation corrects an error in the first loop: j start has to start from 0 not from 1.
 * Reference:
 * - http://mat.gsia.cmu.edu/COLOR/color.html
 * - New Methods to Color the Vertices of a Graph, Daniel Brelaz, CACM 22, 251--256, 1979.
 *
 * The color of the vertex v will be stored in color[v].
 * i.e., vertex v belongs to coloring color[v]
 */

namespace boost
{
   /**
    * heap compare functor
    */
   template <typename size_type>
   struct dsatur2_heap_compare_functor
   {
    public:
      bool operator()(size_type i1, size_type i2) const
      {
         size_type c1 = ColorCount[i1], c2 = ColorCount[i2];
         return !((c1 > c2) || ((c1 == c2) && (DegreeCount[i1] > DegreeCount[i2])));
      }
      dsatur2_heap_compare_functor(const std::vector<size_type>& _ColorCount, const std::vector<size_type>& _DegreeCount) : ColorCount(_ColorCount), DegreeCount(_DegreeCount)
      {
      }

    private:
      const std::vector<size_type>& ColorCount;
      const std::vector<size_type>& DegreeCount;
   };
   /**
    * helper used to color the graph.
    */
   template <typename VertexListGraph, typename ColorMap, typename size_type>
   class dsatur2_coloring_helper
   {
    private:
      typedef graph_traits<VertexListGraph> GraphTraits;
      typedef typename GraphTraits::vertex_descriptor Vertex;
      const size_type num_node;
      std::vector<bool> valid;
      typename boost::numeric::ublas::matrix<bool> ColorAdj;
      std::vector<size_type> ColorCount;
      std::vector<size_type> DegreeCount;
      const VertexListGraph& G;
      ColorMap& CM;
      size_type* heap_container;
      dsatur2_heap_compare_functor<size_type> HCF;

    public:
      dsatur2_coloring_helper(const VertexListGraph& _G, ColorMap& _CM, const size_type _num_node)
          : num_node(_num_node), valid(_num_node, true), ColorAdj(_num_node, _num_node), ColorCount(_num_node, 0), DegreeCount(_num_node), G(_G), CM(_CM), HCF(ColorCount, DegreeCount)
      {
         unsigned int i, iheap = 0;
         heap_container = new size_type[_num_node];
         for(i = 0; i < num_node; i++)
         {
            Vertex v = boost::vertex(i, G);
            DegreeCount[i] = degree(v, _G);
            heap_container[iheap] = i;
            iheap++;
         }
         boost::numeric::ublas::noalias(ColorAdj) = boost::numeric::ublas::zero_matrix<bool>(_num_node, _num_node);
      }
      ~dsatur2_coloring_helper()
      {
         delete[] heap_container;
      }
      // no copy constructor
      dsatur2_coloring_helper(const dsatur2_coloring_helper& inst) = delete;

      void AssignColor(size_type node, size_type color)
      {
         size_type node1;
         put(CM, boost::vertex(node, G), color);
         typename GraphTraits::adjacency_iterator v, vend;
         for(boost::tie(v, vend) = adjacent_vertices(boost::vertex(node, G), G); v != vend; ++v)
         {
            node1 = get(vertex_index, G, *v);
            if(!ColorAdj(node1, color))
               ColorCount[node1]++;
            ColorAdj(node1, color) = true;
            DegreeCount[node1]--;
            // assert(DegreeCount[node1] >= 0);
         }
      }

      size_type SeqColor()
      {
         size_type j, v;
         size_type max_color = 0;

         for(size_type i = 0; i < num_node; i++) /// color each vertex
         {
            /// Find node with maximum color_adj
            std::make_heap(heap_container + i, heap_container + num_node, HCF);
            v = heap_container[i];

            /// look for a color for vertex v
            for(j = 0; j <= max_color; j++)
            {
               if(!ColorAdj(v, j))
               {
                  AssignColor(v, j);
                  break;
               }
            }
            if(j <= max_color)
               continue;
            /// not able to color so we have to increase the maximum color available
            max_color++;
            put(CM, boost::vertex(v, G), max_color);
            AssignColor(v, max_color);
         }
         return max_color + 1;
      }
   };

   /**
    * coloring of a graph following the DSATUR heuristic (version2)
    */
   template <typename VertexListGraph, typename ColorMap>
   typename property_traits<ColorMap>::value_type dsatur2_coloring(const VertexListGraph& G, ColorMap color)
   {
      typedef typename property_traits<ColorMap>::value_type size_type;

      const size_type num_node = num_vertices(G);
      if(num_node == 0)
         return 0;
      dsatur2_coloring_helper<VertexListGraph, ColorMap, size_type> MDCH(G, color, num_node);

      return MDCH.SeqColor();
   }
} // namespace boost

#endif
