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
 *
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 *
 */
#ifndef UNFOLDED_CALL_GRAPH_HPP
#define UNFOLDED_CALL_GRAPH_HPP

#include "graph.hpp"

class UnfoldedCallGraph : public RawGraph
{
 public:
   explicit UnfoldedCallGraph(GraphInfoRef g_info) : RawGraph(g_info)
   {
   }

   ~UnfoldedCallGraph() = default;
};

typedef boost::graph_traits<UnfoldedCallGraph>::vertex_descriptor UnfoldedVertexDescriptor;
typedef boost::graph_traits<UnfoldedCallGraph>::vertex_iterator UnfoldedVertexIterator;
#define UNFOLDED_NULL_VERTEX boost::graph_traits<UnfoldedCallGraph>::null_vertex()

typedef boost::graph_traits<UnfoldedCallGraph>::in_edge_iterator UnfoldedInEdgeIterator;
typedef boost::graph_traits<UnfoldedCallGraph>::out_edge_iterator UnfoldedOutEdgeIterator;
typedef boost::graph_traits<UnfoldedCallGraph>::edge_iterator UnfoldedEdgeIterator;
typedef boost::graph_traits<UnfoldedCallGraph>::edge_descriptor UnfoldedEdgeDescriptor;

typedef refcount<UnfoldedCallGraph> UnfoldedCallGraphRef;
typedef refcount<const UnfoldedCallGraph> UnfoldedCallGraphConstRef;
#endif
