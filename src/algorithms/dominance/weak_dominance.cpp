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
 * @file weak_dominance.hpp
 * @brief Class specifying weak dominance calculus.
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

/// Header include
#include "weak_dominance.hpp"

#include "Dominance.hpp"
#include "Parameter.hpp"           // for ParameterConstRef
#include "dbgPrintHelper.hpp"      // for DEBUG_LEVEL_NONE
#include "edge_info.hpp"           // for EdgeInfoRef
#include "exceptions.hpp"          // for THROW_ASSERT
#include "string_manipulation.hpp" // for GET_CLASS
#include <boost/tuple/tuple.hpp>   // for tie
#include <iterator>                // for front_insert_iterator
#include <list>                    // for list, _List_iterator

void weak_dominance::calculate_weak_dominance_info(graphs_collection* output, CustomUnorderedMap<vertex, vertex>& i2o, CustomUnorderedMap<vertex, vertex>& o2i)
{
   if(boost::num_vertices(*input) == 0)
      return;
   // creating maps
   THROW_ASSERT(i2o.size() == o2i.size(), "Different map sizes");
   if(i2o.size() == 0)
   {
      graph::vertex_iterator v, v_end;
      for(boost::tie(v, v_end) = boost::vertices(*input); v != v_end; v++)
      {
         vertex new_vertex = output->AddVertex(NodeInfoRef());
         i2o[*v] = new_vertex;
         o2i[new_vertex] = *v;
      }
   }

   std::list<vertex> levels;
   boost::topological_sort(*input, std::front_inserter(levels));
   CustomUnorderedMap<vertex, unsigned int> sorted_nodes;
   unsigned int counter = 0;
   for(auto& level : levels)
      sorted_nodes[level] = ++counter;

   dominance<graph> dm(*input, start, end, param);
   dm.calculate_dominance_info(dominance<graph>::CDI_POST_DOMINATORS);
   const auto& post_dominators = dm.get_dominator_map();

   // iterate over outgoing edges of the input graph
   EdgeIterator ei, ei_end;
   for(boost::tie(ei, ei_end) = boost::edges(*input); ei != ei_end; ++ei)
   {
      vertex A = boost::source(*ei, *input);
      vertex B = boost::target(*ei, *input);
      InEdgeIterator pd_ei, pd_ei_end;
      {
         vertex current_node = B;
         while(current_node && current_node != A && current_node != post_dominators.at(A))
         {
            if(sorted_nodes[current_node] > sorted_nodes[A])
               add_edge(i2o[A], i2o[current_node], output);
            else
               break;
            current_node = post_dominators.at(current_node);
         }
      }
   }
}

weak_dominance::weak_dominance(const graph* _input, vertex _start, vertex _end, const ParameterConstRef _param, int _selector)
    : input(_input), start(_start), end(_end), selector(_selector), param(_param), debug_level(_param->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE))
{
}

void weak_dominance::add_edge(vertex source, vertex target, graphs_collection* output)
{
   if(output->ExistsEdge(source, target))
      output->AddSelector(source, target, selector);
   else
      output->InternalAddEdge(source, target, selector, EdgeInfoRef());
}
