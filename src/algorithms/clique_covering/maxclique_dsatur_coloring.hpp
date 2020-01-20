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
 * @file maxclique_dsatur_coloring.hpp
 * @brief Boost-based implementation of an exact sequential coloring algorithm based on the work of Olivier Coudert and Daniel Brelaz.
 *
 * As a starting point it has been used the exact sequential vertex coloring implementation of Michael A. Trick.
 * For more details see
 * - New Methods to Color the Vertices of a Graph, Daniel Brelaz, CACM 22, 251--256, 1979.
 * - Olivier Coudert, "Exact Coloring of Real-Life Graphs is Easy". DAC 1997.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef MAXCLIQUE_DSATUR_COLORING_HPP
#define MAXCLIQUE_DSATUR_COLORING_HPP

#include <boost/config.hpp>
#include <boost/graph/properties.hpp>
#include <boost/version.hpp>
#if BOOST_VERSION >= 104000
#include <boost/property_map/property_map.hpp>
#else
#include <boost/property_map.hpp>
#endif
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/visitors.hpp>
#include <boost/limits.hpp>
#if BOOST_VERSION >= 106400
#include <boost/serialization/array_wrapper.hpp>
#endif
#include <algorithm>
#include <boost/numeric/ublas/matrix.hpp>
#include <cassert>
#include <vector>

#include "custom_set.hpp"
#include <boost/graph/filtered_graph.hpp>

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
 * - Olivier Coudert, "Exact Coloring of Real-Life Graphs is Easy". DAC 1997.
 *
 * The color of the vertex v will be stored in color[v].
 * i.e., vertex v belongs to coloring color[v]
 */

namespace boost
{
   template <typename size_type>
   struct heap_compare_functor
   {
    public:
      bool operator()(size_type i1, size_type i2) const
      {
         size_type c1 = ColorCount[i1], c2 = ColorCount[i2];
         return !((c1 > c2) || ((c1 == c2) && (DegreeCount[i1] > DegreeCount[i2])));
      }
      heap_compare_functor(const std::vector<size_type>& _ColorCount, const std::vector<size_type>& _DegreeCount) : ColorCount(_ColorCount), DegreeCount(_DegreeCount)
      {
      }

    private:
      const std::vector<size_type>& ColorCount;
      const std::vector<size_type>& DegreeCount;
   };
   template <class VertexListGraph, class ColorMap>
   typename property_traits<ColorMap>::value_type unsorted_coloring(const VertexListGraph& G, ColorMap color)
   {
      typedef graph_traits<VertexListGraph> GraphTraits;
      typedef typename GraphTraits::vertex_descriptor Vertex;
      typedef typename property_traits<ColorMap>::value_type size_type;

      size_type max_color = 0;
      const size_type V = num_vertices(G);

      // We need to keep track of which colors are used by
      // adjacent vertices. We do this by marking the colors
      // that are used. The mark array contains the mark
      // for each color. The length of mark is the
      // number of vertices since the maximum possible number of colors
      // is the number of vertices.
      std::vector<size_type> mark(V, std::numeric_limits<size_type>::max BOOST_PREVENT_MACRO_SUBSTITUTION());

      // Initialize colors
      typename GraphTraits::vertex_iterator v, vend;
      for(boost::tie(v, vend) = vertices(G); v != vend; ++v)
         put(color, *v, V - 1);

      // Determine the color for every vertex one by one
      size_type i = 0;
      for(boost::tie(v, vend) = vertices(G); v != vend; ++v)
      {
         Vertex current = *v;
         typename GraphTraits::adjacency_iterator v1, vend1;

         // Mark the colors of vertices adjacent to current.
         // i can be the value for marking since i increases successively
         for(boost::tie(v1, vend1) = adjacent_vertices(current, G); v1 != vend1; ++v1)
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
         ++i;
      }
      return max_color;
   }

   /**
    * Predicate functor object used to select the proper set of vertices
    */
   template <typename SET_container>
   struct select_vertex
   {
      select_vertex() : support(0)
      {
      }
      explicit select_vertex(SET_container* _support) : support(_support)
      {
      }
      select_vertex(const select_vertex& sv)
      {
         support = sv.support;
      }
      select_vertex& operator=(const select_vertex&) = delete;
      void init(SET_container* _support)
      {
         support = _support;
      }
      template <typename Vertex>
      bool operator()(const Vertex& v) const
      {
         // printf("bool = %d\n", support->find(v) != support->end() ? 1 : 0);
         return support->find(v) != support->end();
      }
      SET_container* support;
   };

   template <typename VertexListGraph, typename ColorMap, typename size_type, typename SET_container>
   class maxclique_dsatur_coloring_helper
   {
    private:
      typedef graph_traits<VertexListGraph> GraphTraits;
      typedef typename GraphTraits::vertex_descriptor Vertex;
      const size_type num_node;
      size_type BestColoring;
      std::vector<size_type> ColorClass;
      std::vector<bool> valid;
      //, boost::numeric::ublas::row_major, std::vector<size_type>
      typename boost::numeric::ublas::matrix<size_type> ColorAdj;
      std::vector<size_type> ColorCount;
      std::vector<size_type> DegreeCount;
      size_type& lb;
      const VertexListGraph& G;
      ColorMap& CM;
      size_type* heap_container;
      heap_compare_functor<size_type> HCF;
      const unsigned int clique_size;
      select_vertex<SET_container> filter;
      SET_container support;
      SET_container C;
      SET_container BestClique;
      typedef filtered_graph<VertexListGraph, keep_all, select_vertex<SET_container>> Induced_Graph;
      Induced_Graph FG;
      typedef graph_traits<Induced_Graph> IG_GraphTraits;
      typedef typename IG_GraphTraits::vertex_descriptor IG_Vertex;

    public:
      maxclique_dsatur_coloring_helper(const VertexListGraph& _G, ColorMap& _CM, const size_type _num_node, size_type& _lb)
          : num_node(_num_node),
            BestColoring(_num_node + 1),
            ColorClass(_num_node, _num_node - 1),
            valid(_num_node, true),
            ColorAdj(_num_node, _num_node),
            ColorCount(_num_node, 0),
            DegreeCount(_num_node),
            lb(_lb),
            G(_G),
            CM(_CM),
            HCF(ColorCount, DegreeCount),
            clique_size(0),
            filter(&support),
            FG(G, keep_all(), filter)
      {
         unsigned int i, iheap = 0;
         heap_container = new size_type[_num_node - clique_size];
         for(i = 0; i < num_node; i++)
         {
            Vertex v = vertex(i, G);
            support.insert(v);
            DegreeCount[i] = degree(v, G);
            heap_container[iheap] = i;
            iheap++;
         }
         boost::numeric::ublas::noalias(ColorAdj) = boost::numeric::ublas::zero_matrix<size_type>(_num_node, _num_node);
      }
      ~maxclique_dsatur_coloring_helper()
      {
         delete[] heap_container;
      }
      // no copy constructor
      maxclique_dsatur_coloring_helper(const maxclique_dsatur_coloring_helper& inst) = delete;
      maxclique_dsatur_coloring_helper& operator=(const maxclique_dsatur_coloring_helper& inst) = delete;
      size_type MaxCliqueGreedy()
      {
         SET_container GreedySupport, G1_support;
         typename IG_GraphTraits::vertex_iterator vstart, v, vend;
         boost::tie(vstart, vend) = vertices(FG);
         v = vstart;
         while(v != vend)
         {
            GreedySupport.insert(*v);
            ++v;
         }
         std::swap(GreedySupport, support);
         do
         {
            v = vstart;
            IG_Vertex selected = *v;
            size_type maxdegree = out_degree(*v, FG);
            ;
            ++v;
            while(v != vend)
            {
               size_type tmpdegree = out_degree(*v, FG);
               if(tmpdegree > maxdegree)
               {
                  maxdegree = tmpdegree;
                  selected = *v;
               }
               ++v;
            }
            BestClique.insert(selected);
            typename IG_GraphTraits::adjacency_iterator v1, v1end;
            boost::tie(v1, v1end) = adjacent_vertices(selected, FG);
            G1_support.insert(v1, v1end);
            support = G1_support;
            G1_support.clear();
            boost::tie(vstart, vend) = vertices(FG);
         } while(vstart != vend);
         std::swap(GreedySupport, support);
         return BestClique.size();
      }
      size_type MaxCliqueRec(size_type ub)
      {
         typename IG_GraphTraits::vertex_iterator vstart, v, vend;
         boost::tie(vstart, vend) = vertices(FG);
         if(vstart == vend)
         {
            if(BestClique.size() < C.size())
            {
               BestClique = C;
               // printf("Best clique is %ld (A)\n", BestClique.size());
            }
            return C.size();
         }

         std::vector<size_type> color_vec(num_node);
         ColorMap color(&color_vec.front(), get(vertex_index, FG));
         size_type k = unsorted_coloring(FG, color);
         /// if C is empty and k is good save the coloring
         if(support.size() == num_node && k < BestColoring)
         {
            BestColoring = k;
            for(size_type i = 0; i < num_node; ++i)
               put(CM, vertex(i, G), get(color, vertex(i, G)));
            // printf("Best coloring is %ld\n", BestColoring);
         }
         ub = ub <= k + C.size() ? ub : k + C.size();
         if(ub <= BestClique.size())
            return BestClique.size();

         /// look for the vertex with maximum degree
         v = vstart;
         IG_Vertex selected = *v;
         size_type FG_cardinality = 1;
         size_type mindegree = out_degree(*v, FG);
         size_type maxdegree = mindegree;
         ++v;
         while(v != vend)
         {
            size_type tmpdegree = out_degree(*v, FG);
            if(tmpdegree > maxdegree)
            {
               maxdegree = tmpdegree;
               selected = *v;
            }
            if(tmpdegree < mindegree)
               mindegree = tmpdegree;
            ++v;
            ++FG_cardinality;
         }
         /// check a) condition
         if(FG_cardinality + C.size() <= BestClique.size())
            return BestClique.size();
         /// check for the minimum degree condition
         if(mindegree + 1 == FG_cardinality)
         {
            if(BestClique.size() < C.size() + FG_cardinality)
            {
               BestClique = C;
               BestClique.insert(vstart, vend);
               // printf("Best clique is %ld (B)\n", BestClique.size());
            }
            return BestClique.size();
         }
         /// create the support for the the induced graph N{selected}
         SET_container G1_support;
         typename IG_GraphTraits::adjacency_iterator v1, v1end;
         boost::tie(v1, v1end) = adjacent_vertices(selected, FG);
         G1_support.insert(v1, v1end);
         std::swap(G1_support, support);
         C.insert(selected);
         size_type sizeG1 = MaxCliqueRec(ub);
         C.erase(selected);
         std::swap(G1_support, support);
         if(ub == sizeG1)
            return sizeG1;
         support.erase(selected);
         size_type sizeG0 = MaxCliqueRec(ub);
         support.insert(selected);
         return sizeG0;
      }
      void save_colors()
      {
         size_type i;
         // printf("Best coloring is %ld\n", BestColoring);
         for(i = 0; i < num_node; i++)
            put(CM, vertex(i, G), ColorClass[i]);
      }

      void AssignColor(size_type node, size_type color)
      {
         size_type node1;

         ColorClass[node] = color;
         typename GraphTraits::adjacency_iterator v, vend;
         for(boost::tie(v, vend) = adjacent_vertices(vertex(node, G), G); v != vend; ++v)
         {
            node1 = get(vertex_index, G, *v);
            if(ColorAdj(node1, color) == 0)
               ColorCount[node1]++;
            ColorAdj(node1, color) += 1;
            DegreeCount[node1]--;
            assert(DegreeCount[node1] >= 0);
         }
      }

      void RemoveColor(size_type node, size_type color)
      {
         size_type node1;
         ColorClass[node] = 0;
         typename GraphTraits::adjacency_iterator v, vend;
         for(boost::tie(v, vend) = adjacent_vertices(vertex(node, G), G); v != vend; ++v)
         {
            node1 = get(vertex_index, G, *v);
            assert(ColorAdj(node1, color) != 0);
            ColorAdj(node1, color) -= 1;
            if(ColorAdj(node1, color) == 0)
               ColorCount[node1]--;
            DegreeCount[node1]++;
         }
      }

      size_type SeqColorRec(size_type i, size_type current_color)
      {
         size_type j, new_val, place;
         // int k, count;

         // prob_count++;
         if(current_color + 1 >= BestColoring)
            return (current_color + 1);
         if(BestColoring <= lb)
            return (BestColoring);

         if(i >= num_node)
            return (current_color + 1);
         /*  printf("Node %ld, num_color %ld\n",i,current_color);*/

         /* Find node with maximum color_adj */
         std::make_heap(heap_container + i - clique_size, heap_container + num_node - clique_size, HCF);
         place = heap_container[i];

         /*  printf("Using node %ld at level %ld\n",place,i);*/
         for(j = 0; j <= current_color; j++)
         {
            if(ColorAdj(place, j) == 0)
            {
               AssignColor(place, j);
               new_val = SeqColorRec(i + 1, current_color);
               if(new_val < BestColoring)
               {
                  BestColoring = new_val;
                  save_colors();
               }
               RemoveColor(place, j);
               if(BestColoring <= current_color)
               {
                  return (BestColoring);
               }
            }
         }
         if(current_color + 2 < BestColoring)
         {
            AssignColor(place, current_color + 1);
            new_val = SeqColorRec(i + 1, current_color + 1);
            if(new_val < BestColoring)
            {
               BestColoring = new_val;
               save_colors();
            }

            RemoveColor(place, current_color + 1);
         }
         return (BestColoring);
      }
   };

   template <typename VertexListGraph, typename ColorMap>
   typename property_traits<ColorMap>::value_type maxclique_dsatur_coloring(const VertexListGraph& G, ColorMap color)
   {
      typedef graph_traits<VertexListGraph> GraphTraits;
      typedef typename GraphTraits::vertex_descriptor Vertex;
      typedef typename property_traits<ColorMap>::value_type size_type;

      const size_type num_node = num_vertices(G);
      if(num_node == 0)
         return 0;
      size_type lb = 0, val;
      typedef CustomUnorderedSet<Vertex> SET_container;
      maxclique_dsatur_coloring_helper<VertexListGraph, ColorMap, size_type, SET_container> MDCH(G, color, num_node, lb);

      // size_type best_clique=0;
      // unsigned int num_prob = 0, max_prob = 10000;
      // unsigned int prob_count = 0;
      size_type place = 0;
      lb = MDCH.MaxCliqueGreedy();
      // printf("Lower bound is %ld\n", lb);
      lb = MDCH.MaxCliqueRec(num_node);
      // printf("Lower bound is %ld\n", lb);
      val = MDCH.SeqColorRec(place, place);
      // printf("Best coloring has value %ld, subproblems: %d\n", val, prob_count);
      return val;
   }
} // namespace boost

#endif
