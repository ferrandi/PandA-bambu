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
 * @file cyclic_topological_sort.hpp
 * @brief File used to compute the topological sort in a cyclic graph
 *
 * File used to compute the topological sort in a cyclic graph
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef CYCLIC_TOPOLOGICAL_SORT_HPP
#define CYCLIC_TOPOLOGICAL_SORT_HPP

#include "boost/graph/depth_first_search.hpp"

template <typename OutputIterator>
struct cyclic_topological_sort_visitor : public boost::dfs_visitor<>
{
   cyclic_topological_sort_visitor(OutputIterator _iter) : m_iter(_iter)
   {
   }

   template <typename Vertex, typename Graph>
   void finish_vertex(const Vertex& u, Graph&)
   {
      *m_iter++ = u;
   }

   OutputIterator m_iter;
};

template <typename VertexListGraph, typename OutputIterator, typename P, typename T, typename R>
void cyclic_topological_sort(VertexListGraph& g, OutputIterator result, const boost::bgl_named_params<P, T, R>& params)
{
   typedef cyclic_topological_sort_visitor<OutputIterator> TopoVisitor;
   boost::depth_first_search(g, params.visitor(TopoVisitor(result)));
}

template <typename VertexListGraph, typename OutputIterator>
void cyclic_topological_sort(VertexListGraph& g, OutputIterator result)
{
   cyclic_topological_sort(g, result, boost::bgl_named_params<int, boost::buffer_param_t>(0)); // bogus
}

#endif
