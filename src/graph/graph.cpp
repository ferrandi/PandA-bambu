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
 *              Copyright (C) 2015-2020 Politecnico di Milano
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
 * @file graph.cpp
 * @brief Class specification of the graph structures.
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Header include
#include "graph.hpp"

void graphs_collection::RemoveVertex(boost::graph_traits<boost_graphs_collection>::vertex_descriptor v)
{
   boost::remove_vertex(v, *this);
   boost::property_map<boost_graphs_collection, boost::vertex_index_t>::type index_map = boost::get(boost::vertex_index_t(), *this);
   boost::graph_traits<boost_graphs_collection>::vertex_iterator v_it, v_it_end;
   size_t index = 0;
   for(boost::tie(v_it, v_it_end) = boost::vertices(*this); v_it != v_it_end; v_it++, index++)
   {
      index_map[*v_it] = index;
   }
}

boost::graph_traits<boost_graphs_collection>::vertex_descriptor graphs_collection::AddVertex(const NodeInfoRef info)
{
   size_t index = boost::num_vertices(*this);
   boost::graph_traits<boost_graphs_collection>::vertex_descriptor new_vertex = boost::add_vertex(*this);
   boost::property_map<boost_graphs_collection, boost::vertex_index_t>::type index_map = boost::get(boost::vertex_index_t(), *this);
   index_map[new_vertex] = index;
   NodeInfoRef& node_info = (*this)[new_vertex];
   node_info = info;
   return new_vertex;
}
