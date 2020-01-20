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
 * @file basic_blocks_graph_constructor.cpp
 * @brief This class provides methods to build a basic blocks graph.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "basic_blocks_graph_constructor.hpp"
#include "basic_block.hpp"         // for BBGraph, BBGraphsCo...
#include "cdfg_edge_info.hpp"      // for CFG_SELECTOR, CDG_S...
#include "exceptions.hpp"          // for THROW_ASSERT
#include "string_manipulation.hpp" // for STR
#include "tree_basic_block.hpp"    // for bloc, BB_ENTRY, BB_...
#include <boost/lexical_cast.hpp>  // for lexical_cast
#include <boost/tuple/tuple.hpp>   // for tie
#include <cstddef>                 // for size_t
#include <string>                  // for allocator, operator+
#include <utility>                 // for pair

BasicBlocksGraphConstructor::BasicBlocksGraphConstructor(const BBGraphsCollectionRef _bg) : bg(_bg), bb_graph(new BBGraph(bg, -1)), bb_index_map(bb_graph->GetBBGraphInfo()->bb_index_map)
{
}

BasicBlocksGraphConstructor::~BasicBlocksGraphConstructor() = default;

vertex BasicBlocksGraphConstructor::add_vertex(blocRef info)
{
   size_t index = boost::num_vertices(*bg);
   vertex v = bg->AddVertex(NodeInfoRef(new BBNodeInfo()));
   if(index == 0)
   {
      THROW_ASSERT(index == BB_ENTRY, "wrong value of the BB_ENTRY constant");
      bb_graph->GetBBGraphInfo()->entry_vertex = v;
   }
   else if(index == 1)
   {
      THROW_ASSERT(index == BB_EXIT, "wrong value of the BB_EXIT constant");
      bb_graph->GetBBGraphInfo()->exit_vertex = v;
   }
   bb_graph->GetBBNodeInfo(v)->block = info;
   bb_index_map[info->number] = v;
   return v;
}

EdgeDescriptor BasicBlocksGraphConstructor::AddEdge(const vertex source, const vertex target, int selector)
{
   return bg->AddEdge(source, target, selector);
}

void BasicBlocksGraphConstructor::Clear()
{
   bg->clear();
   bb_index_map.clear();
}

void BasicBlocksGraphConstructor::RemoveEdge(const vertex source, const vertex target, const int selector)
{
   bg->RemoveSelector(source, target, selector);
}

void BasicBlocksGraphConstructor::RemoveEdge(const EdgeDescriptor edge, int selector)
{
   bg->RemoveSelector(edge, selector);
}

void BasicBlocksGraphConstructor::add_bb_edge_info(const vertex source, const vertex target, int type, const unsigned label)
{
   EdgeDescriptor e;
   bool inserted;
   boost::tie(e, inserted) = boost::edge(source, target, *bg);
   THROW_ASSERT(inserted, "Edge BB" + STR(bb_graph->CGetBBNodeInfo(source)->block->number) + "-->BB" + STR(bb_graph->CGetBBNodeInfo(target)->block->number) + " doesn't exists");
   THROW_ASSERT(type & (CFG_SELECTOR | FB_CFG_SELECTOR | CDG_SELECTOR | FB_CDG_SELECTOR), "Not supported label type");
   bb_graph->GetBBEdgeInfo(e)->labels[type].insert(label);
}

EdgeDescriptor BasicBlocksGraphConstructor::connect_to_exit(const vertex source)
{
   return AddEdge(source, bb_graph->GetBBGraphInfo()->exit_vertex, CFG_SELECTOR);
}

EdgeDescriptor BasicBlocksGraphConstructor::connect_to_entry(const vertex target)
{
   return AddEdge(bb_graph->GetBBGraphInfo()->entry_vertex, target, CFG_SELECTOR);
}

bool BasicBlocksGraphConstructor::check_vertex(unsigned int block_index) const
{
   return bb_index_map.find(block_index) != bb_index_map.end();
}

vertex BasicBlocksGraphConstructor::Cget_vertex(unsigned int block_index) const
{
   THROW_ASSERT(bb_index_map.find(block_index) != bb_index_map.end(), "this vertex does not exist " + boost::lexical_cast<std::string>(block_index));
   return bb_index_map.find(block_index)->second;
}

void BasicBlocksGraphConstructor::add_operation_to_bb(vertex op, unsigned int index)
{
   bb_graph->GetBBNodeInfo(Cget_vertex(index))->statements_list.push_back(op);
}
